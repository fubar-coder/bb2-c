/*
 * tedab.c - the TED A/B tests, the parts without emulator: see tedab.h.
 */
#define _XOPEN_SOURCE 700       /* strdup, strndup, open_memstream */
#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include "tedab.h"
#include "tdscript.h"
#include "sha256.h"
#include "util.h"

const char *const tedab_combo_names[TEDAB_NCOMBOS] = { "R5", "C5", "Cc", "Rc", "P5", "Pc" };
const int tedab_pairs[TEDAB_NPAIRS][2] = {
    { TEDAB_R5, TEDAB_C5 }, { TEDAB_RC, TEDAB_CC }, { TEDAB_R5, TEDAB_RC }, { TEDAB_C5, TEDAB_CC },
    { TEDAB_C5, TEDAB_P5 }, { TEDAB_CC, TEDAB_PC }
};

int tedab_combo(const char *name)
{
    int c;

    for (c = 0; c < TEDAB_NCOMBOS; c++)
        if (!strcmp(name, tedab_combo_names[c]))
            return c;
    return -1;
}

int tedab_pair(const char *name)
{
    int k;

    for (k = 0; k < TEDAB_NPAIRS; k++) {
        const char *a = tedab_combo_names[tedab_pairs[k][0]];
        const char *b = tedab_combo_names[tedab_pairs[k][1]];
        size_t la = strlen(a);

        if (!strncmp(name, a, la) && name[la] == '-' && !strcmp(name + la + 1, b))
            return k;
    }
    return -1;
}

int tedab_pair_verdict(const struct tedab_test *t, int k, int compare)
{
    if (compare < 0)
        return TEDAB_PAIR_FAIL;
    if (t->fixed[k])
        return compare ? TEDAB_PAIR_FIXED : TEDAB_PAIR_BROKEN;
    return compare ? TEDAB_PAIR_DIFF : TEDAB_PAIR_SAME;
}

int tedab_pair_passes(int verdict)
{
    return verdict == TEDAB_PAIR_SAME || verdict == TEDAB_PAIR_FIXED;
}

const char *tedab_pair_verdict_name(int verdict)
{
    static const char *const names[] = { "SAME", "DIFF", "FAIL", "FIXED", "BROKEN" };

    return verdict >= 0 && verdict < 5 ? names[verdict] : "?";
}

/* ------------------------------------------------------------ directives */

static const struct {
    const char *kind;
    int args;
} expect_kinds[] = {
    { "same", 1 }, { "bak", 1 }, { "exists", 1 }, { "absent", 1 }, { "tok", 2 },
    { "corpus", 2 }, { "tedsave", 2 }, { "cd", 2 }, { "lexists", 1 }, { "opts", 2 },
    /* with a "..." string (any number of words in it) as the last argument */
    { "data", 2 }, { "stdout", 1 }, { "nostdout", 1 }, { "log", 1 }, { "nolog", 1 },
    { "screen", 2 }, { "noscreen", 2 }, { "file", 2 }, { "nofile", 2 },
};

/* the kinds whose last argument is a "..." string */
static int quoted_kind(const char *kind)
{
    return !strcmp(kind, "data") || !strcmp(kind, "stdout") || !strcmp(kind, "nostdout")
           || !strcmp(kind, "log") || !strcmp(kind, "nolog") || !strcmp(kind, "screen")
           || !strcmp(kind, "noscreen") || !strcmp(kind, "file") || !strcmp(kind, "nofile");
}

/* "..." at s (blanks before it skipped) with \n \t \xHH (also \x00), \ +
   anything else = that character, into *out (malloc'd, *len bytes, a 0
   behind them); the rest behind the closing quote in *rest.  0 = ok */
static int unquote(const char *s, uint8_t **out, size_t *len, const char **rest)
{
    uint8_t *d;
    size_t n = 0;

    while (*s == ' ' || *s == '\t')
        s++;
    if (*s++ != '"' || !(d = malloc(strlen(s) + 1)))
        return -1;
    while (*s && *s != '"') {
        if (*s == '\\' && s[1]) {
            s++;
            if (*s == 'n')
                d[n++] = '\n', s++;
            else if (*s == 't')
                d[n++] = '\t', s++;
            else if (*s == 'x' && td_ishex((unsigned char)s[1]) && td_ishex((unsigned char)s[2])) {
                char h[3] = { s[1], s[2], 0 };

                d[n++] = (uint8_t)td_number(h, 16);
                s += 3;
            } else
                d[n++] = (uint8_t)*s++;
        } else
            d[n++] = (uint8_t)*s++;
    }
    if (*s != '"') {
        free(d);
        return -1;
    }
    d[n] = 0;
    *out = d;
    *len = n;
    *rest = s + 1;
    return 0;
}

/* *data += more (more is freed); 0 = ok */
static int append(uint8_t **data, size_t *len, uint8_t *more, size_t n)
{
    uint8_t *d = realloc(*data, *len + n + 1);

    if (!d) {
        free(more);
        return -1;
    }
    memcpy(d + *len, more, n);
    *len += n;
    d[*len] = 0;
    *data = d;
    free(more);
    return 0;
}

/* the words of a directive's value (blanks separate), at most max */
static int words(char *s, char **w, int max)
{
    int n = 0;

    for (;;) {
        while (*s == ' ' || *s == '\t' || *s == '\r')
            s++;
        if (!*s || n >= max)
            return n;
        w[n++] = s;
        while (*s && *s != ' ' && *s != '\t' && *s != '\r')
            s++;
        if (*s)
            *s++ = 0;
    }
}

static int add_expect(struct tedab_test *t, const char *kind, const char *a, const char *b)
{
    struct tedab_expect *e = realloc(t->expects, (t->nexpects + 1) * sizeof *e);

    if (!e)
        return -1;
    t->expects = e;
    e += t->nexpects++;
    snprintf(e->kind, sizeof e->kind, "%s", kind);
    e->a = strdup(a);
    e->b = b ? strdup(b) : NULL;
    e->data = NULL;
    e->len = 0;
    return e->a && (!b || e->b) ? 0 : -1;
}

static const char *base_name(const char *p)
{
    const char *s = strrchr(p, '/');

    return s ? s + 1 : p;
}

int tedab_test_parse(const char *name, const char *text, struct tedab_test *t, char *err,
                     size_t errlen)
{
    const char *line = text;
    unsigned no = 0;
    int have_source = 0;
    struct tedab_expect *pre = NULL;
    size_t npre = 0, i;

    memset(t, 0, sizeof *t);
    t->name = strdup(name);
    t->combos = (1u << TEDAB_NCOMBOS) - 1;
    t->font = 8;
    t->est = 5;
    if (!t->name)
        return -1;
    while (*line) {
        const char *eol = strchr(line, '\n'), *p;
        char buf[400], raw[400], *w[24], *key;
        size_t n;
        int nw;

        if (!eol)
            eol = line + strlen(line);
        no++;
        n = (size_t)(eol - line);
        p = line;
        line = *eol ? eol + 1 : eol;
        while (p < eol && (*p == ' ' || *p == '\t'))
            p++;
        if (p >= eol || *p != ';')
            continue;
        p++;
        while (p < eol && (*p == ' ' || *p == '\t'))
            p++;
        if (p >= eol || *p != '@')
            continue;
        n = (size_t)(eol - p - 1);
        if (n >= sizeof buf) {
            snprintf(err, errlen, "%s line %u: directive too long", name, no);
            return -1;
        }
        memcpy(buf, p + 1, n);
        buf[n] = 0;
        key = buf;
        for (p = buf; isalnum((unsigned char)*p) || *p == '_'; p++)
            ;
        if (p == buf)
            continue;
        if (*p && *p != ' ' && *p != '\t' && *p != '\r')
            continue;                   /* "@word" must end at a blank */
        if (!strncmp(buf, "known", 5) && p == buf + 5) {
            const char *r = p;

            while (*r == ' ' || *r == '\t')
                r++;
            free(t->known);
            if (!(t->known = strndup(r, strcspn(r, "\r"))))
                return -1;
            if (!*t->known) {
                snprintf(err, errlen, "%s line %u: @known REASON", name, no);
                return -1;
            }
            continue;
        }
        strcpy(raw, *p ? p + 1 : "");
        nw = *p ? words(buf + (p - buf) + 1, w, 24) : 0;
        buf[p - buf] = 0;
        if (!strcmp(key, "file")) {
            const char *q = raw, *rest;
            uint8_t *d;
            size_t len, k;

            while (*q == ' ' || *q == '\t')
                q++;
            q += strcspn(q, " \t");
            if (nw < 2 || unquote(q, &d, &len, &rest)) {
                snprintf(err, errlen, "%s line %u: @file NAME \"DATA\"", name, no);
                return -1;
            }
            for (k = 0; k < t->nfiles && strcmp(t->files[k].name, w[0]); k++)
                ;
            if (k == t->nfiles) {
                struct tedab_file *f = realloc(t->files, (k + 1) * sizeof *f);

                if (!f) {
                    free(d);
                    return -1;
                }
                t->files = f;
                memset(&f[k], 0, sizeof f[k]);
                if (!(f[k].name = strdup(w[0]))) {
                    free(d);
                    return -1;
                }
                t->nfiles++;
            }
            if (append(&t->files[k].data, &t->files[k].len, d, len))
                return -1;
        } else if (!strcmp(key, "command")) {
            const char *q = raw;

            while (*q == ' ' || *q == '\t')
                q++;
            free(t->command);
            if (!(t->command = strndup(q, strcspn(q, "\r"))))
                return -1;
            if (!*t->command) {
                snprintf(err, errlen, "%s line %u: @command ARGS", name, no);
                return -1;
            }
        } else if (!strcmp(key, "source")) {
            free(t->src_arg);
            t->src_arg = NULL;
            t->src_kind = 0;
            if (nw && strcmp(w[0], "none")) {
                if (nw != 2 || strlen(w[0]) != 1 || !strchr("TESKF", w[0][0])) {
                    snprintf(err, errlen, "%s line %u: @source T|E|S|K|F NAME or none", name,
                             no);
                    return -1;
                }
                t->src_kind = w[0][0];
                if (!(t->src_arg = strdup(w[1])))
                    return -1;
            }
            have_source = 1;
        } else if (!strcmp(key, "combos")) {
            int k;

            t->combos = 0;
            for (k = 0; k < nw; k++) {
                int c = tedab_combo(w[k]);

                if (c < 0) {
                    snprintf(err, errlen, "%s line %u: unknown combination %s", name, no, w[k]);
                    return -1;
                }
                t->combos |= 1u << c;
            }
        } else if (!strcmp(key, "fixed")) {
            /* #NNN PAIR...: NNN > 0 without leading zeros */
            unsigned long issue = 0;
            int k, bad = nw < 2 || w[0][0] != '#' || w[0][1] < '1' || w[0][1] > '9';

            if (!bad) {
                char *e;

                issue = strtoul(w[0] + 1, &e, 10);
                bad = *e || !issue || issue > 999999;
            }
            if (bad) {
                snprintf(err, errlen, "%s line %u: @fixed #NNN PAIR... (PAIR e.g. R5-C5)", name,
                         no);
                return -1;
            }
            for (k = 1; k < nw; k++) {
                int pr = tedab_pair(w[k]);

                if (pr < 0) {
                    snprintf(err, errlen,
                             "%s line %u: @fixed: %s is no compared pair (R5-C5 Rc-Cc R5-Rc "
                             "C5-Cc C5-P5 Cc-Pc)", name, no, w[k]);
                    return -1;
                }
                if (t->fixed[pr]) {
                    snprintf(err, errlen, "%s line %u: @fixed: pair %s named twice", name, no,
                             w[k]);
                    return -1;
                }
                t->fixed[pr] = (unsigned)issue;
            }
        } else if (!strcmp(key, "xtra")) {
            if (nw != 1 || strcmp(w[0], "norunerr")) {
                snprintf(err, errlen, "%s line %u: @xtra norunerr", name, no);
                return -1;
            }
            t->norunerr = 1;
        } else if (!strcmp(key, "est") || !strcmp(key, "font")) {
            char *e;
            long v = nw == 1 ? strtol(w[0], &e, 10) : 0;

            if (nw != 1 || *e || v <= 0) {
                snprintf(err, errlen, "%s line %u: @%s needs a number", name, no, key);
                return -1;
            }
            if (!strcmp(key, "font") && v != 6 && v != 8) {
                snprintf(err, errlen, "%s line %u: @font 6 or 8", name, no);
                return -1;
            }
            *(!strcmp(key, "est") ? &t->est : &t->font) = (int)v;
        } else if (!strcmp(key, "roundtrip")) {
            t->roundtrip = 1;
        } else if (!strcmp(key, "nofiles")) {
            t->nofiles = 1;
        } else if (!strcmp(key, "opts")) {
            uint8_t o[50];

            free(t->opts);
            if (!(t->opts = strdup(nw ? w[0] : "")))
                return -1;
            if (tedab_opts_file(t->opts, o, err, errlen))
                return -1;
        } else if (!strcmp(key, "expect")) {
            size_t k;

            for (k = 0; nw && k < sizeof expect_kinds / sizeof expect_kinds[0]; k++)
                if (!strcmp(w[0], expect_kinds[k].kind))
                    break;
            if (nw && k < sizeof expect_kinds / sizeof expect_kinds[0] && quoted_kind(w[0])) {
                /* KIND [NAME] "TEXT": the string from the raw value */
                const char *q = raw, *rest;
                int skip = expect_kinds[k].args, j, bad;
                uint8_t *d;
                size_t len, e = 0;

                for (j = 0; j < skip; j++) {    /* KIND, NAME */
                    while (*q == ' ' || *q == '\t')
                        q++;
                    q += strcspn(q, " \t");
                }
                bad = unquote(q, &d, &len, &rest);
                if (!bad && rest[strspn(rest, " \t\r")]) {
                    free(d);
                    bad = -1;
                }
                if (bad) {
                    snprintf(err, errlen, "%s line %u: @expect %s %s\"TEXT\"", name, no, w[0],
                             skip == 2 ? "NAME " : "");
                    return -1;
                }
                /* data: the lines of the same NAME append */
                if (!strcmp(w[0], "data"))
                    for (e = 0; e < t->nexpects; e++)
                        if (!strcmp(t->expects[e].kind, "data") && !strcmp(t->expects[e].a, w[1]))
                            break;
                if (strcmp(w[0], "data") || e == t->nexpects) {
                    e = t->nexpects;
                    if (add_expect(t, w[0], skip == 2 ? w[1] : "-", NULL)) {
                        free(d);
                        return -1;
                    }
                }
                if (append(&t->expects[e].data, &t->expects[e].len, d, len))
                    return -1;
                continue;
            }
            if (!nw || k == sizeof expect_kinds / sizeof expect_kinds[0]
                || nw - 1 != expect_kinds[k].args) {
                snprintf(err, errlen, "%s line %u: bad @expect", name, no);
                return -1;
            }
            if (!strcmp(w[0], "opts")) {
                uint8_t o[50];

                if (tedab_opts_file(w[2], o, err, errlen))
                    return -1;
            }
            if (add_expect(t, w[0], w[1], nw > 2 ? w[2] : NULL))
                return -1;
        }
        /* other "@words" in comments are text */
    }
    if (!have_source) {
        snprintf(err, errlen, "%s: no @source", name);
        return -1;
    }
    if (!(t->roundtrip || t->nexpects || t->nofiles)) {
        snprintf(err, errlen, "%s: no @roundtrip/@expect/@nofiles - a result OK alone is no pass",
                 name);
        return -1;
    }
    if (!t->combos) {
        snprintf(err, errlen, "%s: no combination", name);
        return -1;
    }
    for (i = 0; i < TEDAB_NPAIRS; i++) {
        unsigned both = (1u << tedab_pairs[i][0]) | (1u << tedab_pairs[i][1]);

        if (t->fixed[i] && (t->combos & both) != both) {
            snprintf(err, errlen, "%s: @fixed #%u %s-%s: a combination not in @combos (the "
                                  "pair is never compared)", name, t->fixed[i],
                     tedab_combo_names[tedab_pairs[i][0]], tedab_combo_names[tedab_pairs[i][1]]);
            return -1;
        }
    }
    if (t->src_kind) {
        if (t->src_kind == 'K' || t->src_kind == 'F')
            t->file = strdup(base_name(t->src_arg));
        else
            t->file = str_printf("%s.bb2", t->src_arg);
        if (!t->file)
            return -1;
    }
    if (t->src_kind == 'F') {
        for (i = 0; i < t->nfiles && strcmp(t->files[i].name, t->src_arg); i++)
            ;
        if (i == t->nfiles) {
            snprintf(err, errlen, "%s: @source F %s without @file %s", name, t->src_arg,
                     t->src_arg);
            return -1;
        }
    }
    if (t->roundtrip) {
        char *bak;

        if (!t->file) {
            snprintf(err, errlen, "%s: @roundtrip without a source", name);
            return -1;
        }
        /* same FILE, bak FILE.bak in front of the @expect lines */
        pre = t->expects;
        npre = t->nexpects;
        t->expects = NULL;
        t->nexpects = 0;
        bak = str_printf("%s.bak", t->file);
        if (!bak || add_expect(t, "same", t->file, NULL) || add_expect(t, "bak", bak, NULL)) {
            free(bak);
            t->expects = pre;           /* freed with t */
            t->nexpects = npre;
            return -1;
        }
        free(bak);
        for (i = 0; i < npre; i++) {
            if (add_expect(t, pre[i].kind, pre[i].a, pre[i].b))
                return -1;
            t->expects[t->nexpects - 1].data = pre[i].data;
            t->expects[t->nexpects - 1].len = pre[i].len;
            free(pre[i].a);
            free(pre[i].b);
        }
        free(pre);
    }
    for (i = 0; i < t->nexpects; i++)
        if ((!strcmp(t->expects[i].kind, "same") || !strcmp(t->expects[i].kind, "bak"))
            && !t->file) {
            snprintf(err, errlen, "%s: @expect %s without a source", name, t->expects[i].kind);
            return -1;
        }
    return 0;
}

void tedab_test_free(struct tedab_test *t)
{
    size_t i;

    for (i = 0; i < t->nexpects; i++) {
        free(t->expects[i].a);
        free(t->expects[i].b);
        free(t->expects[i].data);
    }
    free(t->expects);
    for (i = 0; i < t->nfiles; i++) {
        free(t->files[i].name);
        free(t->files[i].data);
    }
    free(t->files);
    free(t->command);
    free(t->name);
    free(t->src_arg);
    free(t->file);
    free(t->opts);
    free(t->known);
    memset(t, 0, sizeof *t);
}

/* ------------------------------------------------------------------ lint */

static const struct {
    const char *name;
    int lo, hi;
} drv_commands[] = {
    { "timeout", 1, 1 }, { "key", 1, 2 }, { "type", 1, 1 }, { "menu", 2, 3 }, { "gad", 1, 1 },
    { "str", 2, 2 }, { "freq", 1, 3 }, { "dump", 1, 1 }, { "mem", 1, 1 }, { "log", 0, 99 },
}, drv_waits[] = {
    { "screen", 0, 0 }, { "req", 1, 20 }, { "freq", 1, 1 }, { "gone", 0, 0 }, { "idle", 0, 0 },
    { "exit", 0, 0 },
};

/* a whole number: "$" = hex, else base; optional "-" */
static int is_number(const char *s, int base)
{
    if (*s == '-')
        s++;
    if (*s == '$')
        base = 16, s++;
    if (!*s)
        return 0;
    for (; *s; s++)
        if (base == 16 ? !td_ishex((unsigned char)*s) : !isdigit((unsigned char)*s))
            return 0;
    return 1;
}

int tedab_lint(const char *text, FILE *out)
{
    const char *line = text;
    unsigned no = 0;
    int errors = 0;

    while (*line) {
        const char *eol = strchr(line, '\n');
        char buf[1024], *w[128], *cmd, **args;
        const char *buf_line;
        size_t n, k;
        int nw, nargs, found = -1;

        if (!eol)
            eol = line + strlen(line);
        no++;
        n = (size_t)(eol - line);
        if (n > 290) {
            fprintf(out, "line %u: longer than 290 characters (teddrive buffer)\n", no);
            errors++;
        }
        if (n >= sizeof buf)
            n = sizeof buf - 1;
        buf_line = line;
        memcpy(buf, line, n);
        buf[n] = 0;
        line = *eol ? eol + 1 : eol;
        nw = td_split(buf, w, 128);
        if (nw && tedab_postfix(buf_line, n) < 0) {
            fprintf(out, "line %u: unknown postfix (only ; @ignore compilewindow)\n", no);
            errors++;
        }
        if (!nw)
            continue;
        args = w;
        if (!strcmp(w[0], "expectfail")) {
            if (nw == 1) {
                fprintf(out, "line %u: expectfail without command\n", no);
                errors++;
                continue;
            }
            args++;
            nw--;
        }
        cmd = args[0];
        args++;
        nargs = nw - 1;
        if (!strcmp(cmd, "wait")) {
            for (k = 0; nargs && k < sizeof drv_waits / sizeof drv_waits[0]; k++)
                if (!strcmp(args[0], drv_waits[k].name))
                    found = (int)k;
            if (found < 0) {
                fprintf(out, "line %u: unknown wait %s\n", no, nargs ? args[0] : "(none)");
                errors++;
            } else if (nargs - 1 < drv_waits[found].lo || nargs - 1 > drv_waits[found].hi) {
                fprintf(out, "line %u: wait %s: %d arguments\n", no, args[0], nargs - 1);
                errors++;
            }
            continue;
        }
        for (k = 0; k < sizeof drv_commands / sizeof drv_commands[0]; k++)
            if (!strcmp(cmd, drv_commands[k].name))
                found = (int)k;
        if (found < 0) {
            fprintf(out, "line %u: unknown command %s\n", no, cmd);
            errors++;
            continue;
        }
        if (nargs < drv_commands[found].lo || nargs > drv_commands[found].hi) {
            fprintf(out, "line %u: %s: %d arguments\n", no, cmd, nargs);
            errors++;
        }
        if (!strcmp(cmd, "freq") && nargs && strcmp(args[0], "ok") && strcmp(args[0], "cancel")) {
            fprintf(out, "line %u: freq ok|cancel\n", no);
            errors++;
        }
        /* TED prefills the file gadget with the full name and leaves PATH
           empty: a bare name would be saved in the current directory */
        if (!strcmp(cmd, "freq") && nargs == 2 && !strchr(args[1], ':')) {
            fprintf(out, "line %u: freq ok NAME: relative name without PATH "
                         "(use the full name or give PATH)\n", no);
            errors++;
        }
        if (!strcmp(cmd, "key") || !strcmp(cmd, "menu") || !strcmp(cmd, "gad")
            || !strcmp(cmd, "timeout") || !strcmp(cmd, "str")) {
            int m = !strcmp(cmd, "str") ? (nargs ? 1 : 0) : nargs, base = !strcmp(cmd, "key") ? 16 : 10;

            for (k = 0; k < (size_t)m; k++)
                if (!is_number(args[k], base)) {
                    fprintf(out, "line %u: %s: bad number '%s'\n", no, cmd, args[k]);
                    errors++;
                }
        }
    }
    return errors;
}

/* ------------------------------------------------------------ option block */

/* TED's option block (l:blitzeditor.opts, 25 words = defstart, state.c) as
   TED writes it at run time under blitz2: tokenise 1 (getstuff), fontmul =
   fonthite * bmapmul (bmapmul 160: the interleaved 2-plane screen of gate 0) */
static const char *const opts_fields[12] = {
    "fonthite", "fonthite2", "fontmul", "tokenise", "makeicons", "normcol",
    "labcol", "tokcol", "propcol", "hscmarg", "vscmarg", "tabstop",
};
static const unsigned opts_default[12] = { 8, 7, 1280, 1, 1, 1, 2, 3, 0, 12, 6, 2 };
static const unsigned opts_palette[12] = { 8, 8, 8, 15, 15, 15, 0, 0, 0, 15, 13, 0 };

int tedab_opts_file(const char *spec, uint8_t out[50], char *err, size_t errlen)
{
    unsigned v[25];
    const char *s = spec;
    size_t i;

    memcpy(v, opts_default, sizeof opts_default);
    memcpy(v + 12, opts_palette, sizeof opts_palette);
    v[24] = 0;
    while (*s) {
        const char *e = strchr(s, ','), *eq;
        size_t n = e ? (size_t)(e - s) : strlen(s), k;

        eq = memchr(s, '=', n);
        for (k = 0; eq && k < 12; k++)
            if (strlen(opts_fields[k]) == (size_t)(eq - s) && !strncmp(s, opts_fields[k], eq - s))
                break;
        if (n && (!eq || k == 12)) {
            snprintf(err, errlen, "unknown option field %.*s", (int)(eq ? eq - s : (long)n), s);
            return -1;
        }
        if (n) {
            char num[16], *end;
            size_t m = n - (size_t)(eq + 1 - s);

            if (!m || m >= sizeof num) {
                snprintf(err, errlen, "option field %s: bad number", opts_fields[k]);
                return -1;
            }
            memcpy(num, eq + 1, m);
            num[m] = 0;
            v[k] = (unsigned)strtoul(num, &end, 10);
            if (*end) {
                snprintf(err, errlen, "option field %s: bad number", opts_fields[k]);
                return -1;
            }
        }
        s += n;
        if (*s)
            s++;
    }
    for (i = 0; i < 25; i++) {
        out[2 * i] = (uint8_t)(v[i] >> 8);
        out[2 * i + 1] = (uint8_t)v[i];
    }
    return 0;
}

/* ------------------------------------------------------------ requesters */

static void add_line(char ***lines, size_t *n, char *s)
{
    char **l = realloc(*lines, (*n + 2) * sizeof *l);

    if (!l || !s) {
        free(s);
        return;
    }
    l[(*n)++] = s;
    l[*n] = NULL;
    *lines = l;
}

/* blitz2's compile window: no title, one gadget "STOP!".  It is open only
   while blitz2 compiles, and whether teddrive sees it depends on the timing
   (0, 1 or 2 times for the same run); a step whose script line ends in
   "; @ignore compilewindow" leaves it out. */
#define COMPILE_WINDOW "requester  1:STOP!"

int tedab_postfix(const char *line, size_t len)
{
    const char *p = line, *end = line + len, *w;
    int quoted = 0;

    /* the comment as td_split finds it: ';' at the start of a word, not in
       a "..." string */
    for (; p < end; p++) {
        if (quoted) {
            if (*p == '\\' && p + 1 < end)
                p++;
            else if (*p == '"')
                quoted = 0;
        } else if (*p == '"' && (p == line || p[-1] == ' ' || p[-1] == '\t'))
            quoted = 1;
        else if (*p == ';' && (p == line || p[-1] == ' ' || p[-1] == '\t' || p[-1] == '\r'))
            break;
    }
    if (p >= end)
        return 0;
    for (p++; p < end && (*p == ' ' || *p == '\t'); p++)
        ;
    if (p >= end || *p != '@')
        return 0;                       /* a plain comment */
    w = p + 1;
    if (end - w >= 6 && !strncmp(w, "ignore", 6)) {
        for (w += 6; w < end && (*w == ' ' || *w == '\t'); w++)
            ;
        if (end - w >= 13 && !strncmp(w, "compilewindow", 13)) {
            for (w += 13; w < end && (*w == ' ' || *w == '\t' || *w == '\r'); w++)
                ;
            if (w == end)
                return 1;
        }
    }
    return -1;
}

/* the requester just read (cur) into the sequence, unless it is the compile
   window and appeared in a step that ignores it */
static void flush(char ***out, size_t *n, struct text *cur, int in, int ignore)
{
    if (in == 2 && !cur->bad && !(ignore && !strcmp(cur->p, COMPILE_WINDOW)))
        add_line(out, n, strdup(cur->p));
    free(cur->p);
    memset(cur, 0, sizeof *cur);
}

char **tedab_requesters(const char *log, size_t *n)
{
    char **out = NULL;
    struct text cur = { 0 };
    int in = 0;                         /* 1 = in a window, 2 = a requester */
    int step_ignores = 0, ignore = 0;   /* the step's / the window's */
    const char *line = log;

    *n = 0;
    while (line && *line) {
        const char *eol = strchr(line, '\n'), *p, *q;
        size_t len;

        if (!eol)
            eol = line + strlen(line);
        len = (size_t)(eol - line);
        if (len && line[len - 1] == '\r')
            len--;
        /* step N t=T: SCRIPT LINE (with its comment) */
        if (len > 5 && !strncmp(line, "step ", 5) && isdigit((unsigned char)line[5])) {
            p = memchr(line, ':', len);
            step_ignores = p && tedab_postfix(p + 1, len - (size_t)(p + 1 - line)) == 1;
        }
        /* window new #N (main|requester) "TITLE" pos ... */
        if (len > 12 && !strncmp(line, "window new #", 12)) {
            p = line + 12;
            while (p < line + len && isdigit((unsigned char)*p))
                p++;
            if (p > line + 12 && *p == ' ') {
                int req = !strncmp(p + 1, "requester \"", 11), main = !strncmp(p + 1, "main \"", 6);

                if (req || main) {
                    const char *t = p + 1 + (req ? 11 : 6), *e = NULL;

                    /* the title: up to the last '" pos' (greedy as the regex) */
                    for (q = t; q + 5 <= line + len; q++)
                        if (!strncmp(q, "\" pos", 5))
                            e = q;
                    if (e) {
                        flush(&out, n, &cur, in, ignore);
                        in = req ? 2 : 1;
                        ignore = step_ignores;
                        if (req)
                            text_add(&cur, "requester %.*s", (int)(e - t), t);
                        line = *eol ? eol + 1 : eol;
                        continue;
                    }
                }
            }
        }
        /*   gadget id N ... size W,H "LABEL"[ buffer "..."]: the label is
           the first "..." after the size; the string buffer (diagnostics of
           newer teddrive versions) is not compared */
        if (in && len > 12 && !strncmp(line, "  gadget id ", 12)) {
            const char *id = line + 12, *s = NULL, *lab, *le, *end = line + len;

            for (p = id; p < end && isdigit((unsigned char)*p); p++)
                ;
            if (p > id && *p == ' ') {
                /* " size -?D+,-?D+ \"" somewhere behind the id */
                for (q = p; q + 7 < end && !s; q++) {
                    const char *r;

                    if (strncmp(q, " size ", 6))
                        continue;
                    r = q + 6;
                    if (*r == '-')
                        r++;
                    if (!isdigit((unsigned char)*r))
                        continue;
                    while (r < end && isdigit((unsigned char)*r))
                        r++;
                    if (*r++ != ',')
                        continue;
                    if (*r == '-')
                        r++;
                    if (!isdigit((unsigned char)*r))
                        continue;
                    while (r < end && isdigit((unsigned char)*r))
                        r++;
                    if (r + 1 < end && r[0] == ' ' && r[1] == '"')
                        s = r + 2;
                }
            }
            if (s) {
                /* the label ends at the first '"' that is the end of the
                   line or followed by ' buffer "' */
                for (le = s; le < end; le++)
                    if (*le == '"' && (le + 1 == end || (!strncmp(le, "\" buffer \"", 10)
                                                         && end[-1] == '"')))
                        break;
                if (le < end) {
                    if (in == 2) {
                        lab = s;
                        while (lab < le && *lab == ' ')
                            lab++;
                        while (le > lab && le[-1] == ' ')
                            le--;
                        text_add(&cur, " %.*s:%.*s", (int)(p - id), id, (int)(le - lab), lab);
                    }
                    line = *eol ? eol + 1 : eol;
                    continue;
                }
            }
        }
        if (len < 2 || strncmp(line, "  ", 2)) {
            flush(&out, n, &cur, in, ignore);
            in = 0;
        }
        line = *eol ? eol + 1 : eol;
    }
    flush(&out, n, &cur, in, ignore);
    if (!out)
        out = calloc(1, sizeof *out);
    return out;
}

void tedab_free_lines(char **lines)
{
    size_t i;

    for (i = 0; lines && lines[i]; i++)
        free(lines[i]);
    free(lines);
}

/* -------------------------------------------------------------- scripts */

/* one run; its directory is assigned to TEDRUN:, so that every combination
   sees the same paths (TED shows the file path in the screen title, file
   requesters show the directory) */
static void run_script(struct text *s, const char *ind, const struct tedab_run *r)
{
    const char *t = r->test->name, *c = tedab_combo_names[r->combo];
    const char *f = r->test->file, *cmd = r->test->command, *at;

    text_add(s, "%sEcho \"RUN %s %s start\" >>" TEDAB_VOL ":session.log\n", ind, t, c);
    text_add(s, "%sAssign blitz2: " TEDAB_VOL ":combo/%s/blitz2\n", ind, c);
    text_add(s, "%sAssign TEDRUN: " TEDAB_VOL ":runs/%s/%s\n", ind, t, c);
    text_add(s, "%sAssign L: TEDRUN:L\n%sAssign L: SYS:L ADD\n%sCD TEDRUN:work\n", ind, ind, ind);
    text_add(s, "%s" TEDAB_VOL ":tools/reqtrap " TEDAB_VOL ":tools/teddrive " TEDAB_VOL
                ":tests/%s.drv TEDRUN:out/ ", ind, t);
    if (!cmd)
        text_add(s, "blitz2:blitz2%s%s", f ? " TEDRUN:work/" : "", f ? f : "");
    else {
        /* @command, @FILE@ = the source in work/ */
        for (; (at = strstr(cmd, "@FILE@")); cmd = at + 6)
            text_add(s, "%.*sTEDRUN:work/%s", (int)(at - cmd), cmd, f ? f : "");
        text_add(s, "%s", cmd);
    }
    text_add(s, " >TEDRUN:out/stdout\n");
    text_add(s, "%sSet tdr $RC\n%sEcho \"rc=$tdr\" >>TEDRUN:out/stdout\n", ind, ind);
    text_add(s, "%sCD " TEDAB_VOL ":\n%sAssign L: SYS:L\n%sAssign TEDRUN:\n%sAssign blitz2:\n",
             ind, ind, ind, ind);
    text_add(s, "%sEcho \"RUN %s %s end\" >>" TEDAB_VOL ":session.log\n", ind, t, c);
}

/* gate D: teddrive against teddrivetest (key gd: the normal script, gr: one
   that fails on purpose and proves the rescue) */
static void gate_script(struct text *s, const char *ind, const char *key, const char *drv,
                        const char *mode)
{
    text_add(s, "%s" TEDAB_VOL ":tools/reqtrap " TEDAB_VOL ":tools/teddrive " TEDAB_VOL
                ":gate/%s " TEDAB_VOL ":gate/out/%s. " TEDAB_VOL ":tools/teddrivetest %s >"
                TEDAB_VOL ":gate/out/%s.stdout\n", ind, drv, key, mode, key);
    text_add(s, "%sSet tdg $RC\n%sEcho \"rc=$tdg\" >>" TEDAB_VOL ":gate/out/%s.stdout\n", ind,
             ind, key);
}

size_t tedab_session_waits_for(const struct tedab_run *runs, size_t i)
{
    size_t k;

    for (k = 0; k < i && runs[k].test != runs[i].test; k++)
        ;
    return k < i ? k : 0;
}

char *tedab_session_script(const struct tedab_run *runs, size_t n, unsigned stack)
{
    struct text s = { 0 };
    const char *log = ">>" TEDAB_VOL ":session.log";
    size_t i;

    /* reboot guard: a crash that reboots the emulator would run the script
       again - the second boot only writes a note */
    text_add(&s, "If EXISTS " TEDAB_VOL ":booted\n"
                 "  Echo \"SESSION second boot - skipped\" %s\n"
                 "Else\n"
                 "  Echo \"boot\" >" TEDAB_VOL ":booted\n"
                 "  Echo \"SESSION start\" %s\n"
                 "  Stack %u\n"
                 "  " TEDAB_VOL ":tools/tedprobe " TEDAB_VOL ":gate/out/gate0.txt " TEDAB_KEYMAP " >NIL:\n"
                 "  If WARN\n"
                 "    Echo \"GATE0 FAIL\" %s\n"
                 "  Else\n"
                 "    Echo \"GATE0 OK\" %s\n", log, log, stack, log, log);
    gate_script(&s, "    ", "gd", "gate-d.drv", "main");
    text_add(&s, "    If VAL $tdg GT 0\n"
                 "      Echo \"GATED FAIL main\" %s\n"
                 "    Else\n", log);
    gate_script(&s, "      ", "gr", "gate-d-rescue.drv", "rescue");
    text_add(&s, "      If VAL $tdg GT 0\n"
                 "        Echo \"GATED FAIL rescue\" %s\n"
                 "      Else\n"
                 "        Echo \"GATED OK\" %s\n", log, log);
    for (i = 0; i < n; i++) {
        const struct tedab_run *r = &runs[i], *first = &runs[tedab_session_waits_for(runs, i)];

        if (!i) {
            /* the first run of the session: everything else needs it OK (a
               tool problem would otherwise hit every test until the
               time limit) */
            run_script(&s, "        ", r);
            continue;
        }
        /* a new test: if the session's first run is OK; another
           combination: if the test's first one (the reference) is OK - a
           tool problem with TED stops the test here */
        text_add(&s, "        If EXISTS " TEDAB_VOL ":runs/%s/%s/out/ok\n", first->test->name,
                 tedab_combo_names[first->combo]);
        run_script(&s, "          ", r);
        text_add(&s, "        Else\n"
                     "          Echo \"RUN %s %s skipped (%s %s not OK)\" %s\n"
                     "        EndIf\n", r->test->name, tedab_combo_names[r->combo],
                 first->test->name, tedab_combo_names[first->combo], log);
    }
    text_add(&s, "      EndIf\n"
                 "    EndIf\n"
                 "  EndIf\n"
                 "  UnSet tdg\n"
                 "  UnSet tdr\n"
                 "  Echo \"SESSION end\" %s\n"
                 "EndIf\n", log);
    if (s.bad) {
        free(s.p);
        return NULL;
    }
    return s.p;
}

int tedab_check_script(const char *script, FILE *out)
{
    const char *line = script;
    unsigned no = 0;
    int depth = 0, errors = 0;

    while (*line) {
        const char *eol = strchr(line, '\n'), *p = line;
        size_t n;

        if (!eol)
            eol = line + strlen(line);
        no++;
        n = (size_t)(eol - line);
        if (n > 255) {
            fprintf(out, "script line %u: %zu characters\n", no, n);
            errors++;
        }
        while (p < eol && (*p == ' ' || *p == '\t'))
            p++;
        if (eol - p >= 2 && !strncmp(p, "If", 2) && (p + 2 == eol || p[2] == ' '))
            depth++;
        else if (eol - p >= 5 && !strncmp(p, "EndIf", 5) && (p + 5 == eol || p[5] == ' ')) {
            if (--depth < 0) {
                fprintf(out, "script line %u: EndIf without If\n", no);
                errors++;
                depth = 0;
            }
        }
        line = *eol ? eol + 1 : eol;
    }
    if (depth) {
        fprintf(out, "script: %d If without EndIf\n", depth);
        errors++;
    }
    return errors;
}

/* ------------------------------------------------------------ session log */

int tedab_session_log(const char *text, const struct tedab_run *runs, size_t n, int *state,
                      struct text *events)
{
    const char *line = text;
    size_t i;
    int found = 0;

    for (i = 0; i < n; i++)
        state[i] = TEDAB_NOTRUN;
    while (line && *line) {
        const char *eol = strchr(line, '\n'), *p = line, *e;
        char t[128], c[8], what[16];

        if (!eol)
            eol = line + strlen(line);
        e = eol;
        while (p < e && (*p == ' ' || *p == '\t'))
            p++;
        while (e > p && (e[-1] == ' ' || e[-1] == '\r'))
            e--;
        line = *eol ? eol + 1 : eol;
        if (p == e)
            continue;
        if (e - p > 4 && !strncmp(p, "RUN ", 4)) {
            char buf[200];
            size_t m = (size_t)(e - p) < sizeof buf ? (size_t)(e - p) : sizeof buf - 1;

            memcpy(buf, p, m);
            buf[m] = 0;
            if (sscanf(buf, "RUN %127s %7s %15s", t, c, what) == 3) {
                int st = !strcmp(what, "start") ? TEDAB_STARTED : !strcmp(what, "end")
                         ? TEDAB_ENDED : !strcmp(what, "skipped") ? TEDAB_SKIPPED : -1;

                for (i = 0; st >= 0 && i < n; i++)
                    if (!strcmp(runs[i].test->name, t) && !strcmp(tedab_combo_names[runs[i].combo], c)) {
                        state[i] = st;
                        found++;
                    }
                if (st >= 0)
                    continue;
            }
        }
        if (events)
            text_add(events, "%s%.*s", events->n ? ", " : "", (int)(e - p), p);
    }
    return found;
}

/* ---------------------------------------------------------------- files */

char *tedab_resolve(const char *base, const char *rel)
{
    char *path = strdup(base), *r = strdup(rel), *part, *save = NULL;

    if (!path || !r) {
        free(path);
        free(r);
        return NULL;
    }
    for (part = strtok_r(r, "/", &save); part && path; part = strtok_r(NULL, "/", &save)) {
        DIR *d = opendir(path);
        struct dirent *e;
        char *next = NULL;

        while (d && (e = readdir(d)))
            if (!strcasecmp(e->d_name, part)) {
                next = str_printf("%s/%s", path, e->d_name);
                break;
            }
        if (d)
            closedir(d);
        free(path);
        path = next;
    }
    free(r);
    return path;
}

/* ------------------------------------------------------------ evaluation */

static char *read_text(const char *dir, const char *name)
{
    char *p = str_printf("%s/%s", dir, name);
    size_t len;
    char *t = p ? (char *)read_file(p, &len) : NULL;

    free(p);
    return t;
}

/* the value of "KEY value" lines */
static void kv(const char *text, const char *key, char *out, size_t len)
{
    const char *l = text;
    size_t k = strlen(key);

    *out = 0;
    while (l && *l) {
        const char *eol = strchr(l, '\n');

        if (!eol)
            eol = l + strlen(l);
        if ((size_t)(eol - l) >= k && !strncmp(l, key, k) && (l[k] == ' ' || l + k == eol)) {
            const char *v = l + k + (l + k < eol);
            size_t n = (size_t)(eol - v);

            if (n && v[n - 1] == '\r')
                n--;
            snprintf(out, len, "%.*s", (int)n, v);
        }
        l = *eol ? eol + 1 : eol;
    }
}

/* "rc=N" lines of teddrive's stdout, as "[N, ...]" (Python list style as
   bb2's report) */
static void rc_list(const char *out, char *buf, size_t len)
{
    const char *l = out;
    size_t n = 0;

    n += (size_t)snprintf(buf, len, "[");
    while (l && *l) {
        const char *eol = strchr(l, '\n');

        if (!eol)
            eol = l + strlen(l);
        if (eol - l > 3 && !strncmp(l, "rc=", 3)) {
            const char *v = l + 3, *e = v;

            while (e < eol && !isspace((unsigned char)*e))
                e++;
            if (n < len)
                n += (size_t)snprintf(buf + n, len - n, "%s'%.*s'", n > 1 ? ", " : "",
                                      (int)(e - v), v);
        }
        l = *eol ? eol + 1 : eol;
    }
    if (n < len)
        snprintf(buf + n, len - n, "]");
}

static void strip(char *s)
{
    size_t n = strlen(s);

    while (n && isspace((unsigned char)s[n - 1]))
        s[--n] = 0;
    while (isspace((unsigned char)*s))
        memmove(s, s + 1, n--);
}

static int rect_check(const struct teddump *d)
{
    long x, y;

    for (y = TEDAB_GATE_Y0; y <= TEDAB_GATE_Y1; y++)
        for (x = TEDAB_GATE_X0; x <= TEDAB_GATE_X1; x++)
            if (teddump_pixel(d, x, y) != 3)
                return 0;
    return teddump_pixel(d, TEDAB_GATE_X0 - 1, TEDAB_GATE_Y0) != 3
           && teddump_pixel(d, TEDAB_GATE_X1 + 1, TEDAB_GATE_Y1) != 3;
}

int tedab_eval_gate(const char *gdir, struct text *out)
{
    char *g0 = read_text(gdir, "gate0.txt"), v[12][64], verdict[128] = "";
    static const char *const keys[12] = {
        "wb_width", "wb_height", "wb_depth", "wb_modeid", "wb_modename", "wb_plane0_chip",
        "ted_width", "ted_height_f8", "ted_edrows_f8", "sys_l_blitzeditor_opts", "keymap",
        "keys_down",
    };
    static const char *const gkey[2] = { "gd", "gr" }, *const gmode[2] = { "main", "rescue" };
    const char *l;
    int ok = 1, g, k;

    if (!g0) {
        text_add(out, "gate 0: no output (not reached)\n");
        return 0;
    }
    for (k = 0; k < 12; k++) {
        kv(g0, keys[k], v[k], sizeof v[k]);
        if (!*v[k] && k != 11)
            strcpy(v[k], k == 4 ? "?" : "None");
    }
    /* the last line is the verdict */
    for (l = g0; *l;) {
        const char *eol = strchr(l, '\n');

        if (!eol)
            eol = l + strlen(l);
        if (eol > l && strspn(l, " \t\r") < (size_t)(eol - l))
            snprintf(verdict, sizeof verdict, "%.*s", (int)(eol - l), l);
        l = *eol ? eol + 1 : eol;
    }
    strip(verdict);
    text_add(out, "gate 0: %s; Workbench %sx%s depth %s mode %s (%s), chip planes %s, TED screen "
                  "%sx%s (edrows %s), SYS:L/blitzeditor.opts %s, keymap %s\n", verdict, v[0],
             v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10]);
    for (l = g0; *l;) {
        const char *eol = strchr(l, '\n');

        if (!eol)
            eol = l + strlen(l);
        if (!strncmp(l, "gate0 check failed", 18))
            text_add(out, "  %.*s\n", (int)(eol - l), l);
        l = *eol ? eol + 1 : eol;
    }
    strip(v[11]);
    if (*v[11])
        text_add(out, "  keys held in the emulator: %s\n", v[11]);
    free(g0);
    if (strcmp(verdict, "gate0 OK"))
        return 0;
    for (g = 0; g < 2; g++) {
        char name[32], *res, *sout, *log, rcs[64];
        int good, mism = 0;

        snprintf(name, sizeof name, "%s.result", gkey[g]);
        res = read_text(gdir, name);
        snprintf(name, sizeof name, "%s.stdout", gkey[g]);
        sout = read_text(gdir, name);
        snprintf(name, sizeof name, "%s.log", gkey[g]);
        log = read_text(gdir, name);
        if (!res)
            res = strdup("");
        if (res)
            strip(res);
        rc_list(sout, rcs, sizeof rcs);
        good = sout && strstr(sout, "teddrivetest: OK 0 mismatches") && !strcmp(rcs, "['0']");
        if (!g) {
            static const char *const dn[2] = { "start", "end" };
            int d;

            good = good && res && !strcmp(res, "OK");
            for (d = 0; d < 2; d++) {
                char *p = str_printf("%s/gd.gd-%s.scr", gdir, dn[d]), err[200];
                struct teddump dump;

                if (p && !teddump_load(p, &dump, err, sizeof err)) {
                    int rect = rect_check(&dump);

                    text_add(out, "gate D dump gd-%s: %ldx%ldx%ld mode $%08lX rectangle %s\n",
                             dn[d], dump.width, dump.height, dump.depth,
                             (unsigned long)dump.modeid, rect ? "ok" : "WRONG");
                    good = good && rect && dump.depth == 2;
                    teddump_free(&dump);
                } else {
                    text_add(out, "gate D dump gd-%s: %s\n", dn[d], p ? err : "out of memory");
                    good = 0;
                }
                free(p);
            }
        } else
            good = good && res && !strncmp(res, "FAIL step", 9) && strstr(res, "timeout")
                   && log && strstr(log, "rescue: program ended");
        for (l = sout; l && *l;) {
            const char *eol = strchr(l, '\n');

            if (!eol)
                eol = l + strlen(l);
            for (const char *q = l; q + 8 <= eol; q++)
                if (!strncmp(q, "MISMATCH", 8)) {
                    mism++;
                    break;
                }
            l = *eol ? eol + 1 : eol;
        }
        text_add(out, "gate D %s: %s (result \"%s\", teddrivetest rc %s, %d mismatches)\n",
                 gmode[g], good ? "OK" : "FAIL", res ? res : "", rcs, mism);
        for (l = sout; l && *l;) {
            const char *eol = strchr(l, '\n');

            if (!eol)
                eol = l + strlen(l);
            for (const char *q = l; q + 8 <= eol; q++)
                if (!strncmp(q, "MISMATCH", 8)) {
                    text_add(out, "  %.*s\n", (int)(eol - l), l);
                    break;
                }
            l = *eol ? eol + 1 : eol;
        }
        ok = ok && good;
        free(res);
        free(sout);
        free(log);
    }
    return ok;
}

/* the names in a directory (sorted, with prefix), appended to *names */
static int by_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

static void list_dir(const char *dir, const char *prefix, const char *suffix, char ***names,
                     size_t *n)
{
    DIR *d = opendir(dir);
    struct dirent *e;

    while (d && (e = readdir(d))) {
        size_t l = strlen(e->d_name), sl = suffix ? strlen(suffix) : 0;

        if (e->d_name[0] == '.' && (!e->d_name[1] || (e->d_name[1] == '.' && !e->d_name[2])))
            continue;
        if (suffix && (l < sl || strcmp(e->d_name + l - sl, suffix)))
            continue;
        add_line(names, n, str_printf("%s%s", prefix, e->d_name));
    }
    if (d)
        closedir(d);
}

/* the union of the names in a and b, sorted, without duplicates */
static char **union_names(const char *a, const char *b, const char *sub, const char *suffix,
                          size_t *n)
{
    char **names = NULL, *da = str_printf("%s/%s", a, sub), *db = str_printf("%s/%s", b, sub);
    char *prefix = strcmp(sub, "out") ? str_printf("%s/", sub) : strdup("");
    size_t i, k = 0;

    *n = 0;
    if (da && db && prefix) {
        list_dir(da, prefix, suffix, &names, n);
        list_dir(db, prefix, suffix, &names, n);
    }
    if (names) {
        qsort(names, *n, sizeof *names, by_str);
        for (i = 0; i < *n; i++)
            if (k && !strcmp(names[k - 1], names[i]))
                free(names[i]);
            else
                names[k++] = names[i];
        *n = k;
        names[k] = NULL;
    }
    free(da);
    free(db);
    free(prefix);
    return names;
}

static int same_lines(char **a, size_t na, char **b, size_t nb)
{
    size_t i;

    if (na != nb)
        return 0;
    for (i = 0; i < na; i++)
        if (strcmp(a[i], b[i]))
            return 0;
    return 1;
}

static void python_list(struct text *t, char **l, size_t n)
{
    size_t i;

    text_add(t, "[");
    for (i = 0; i < n; i++)
        text_add(t, "%s'%s'", i ? ", " : "", l[i]);
    text_add(t, "]");
}

int tedab_compare_runs(const char *a, const char *b, const char *na, const char *nb,
                       const struct teddump_fonts *f, int fh, struct text *out)
{
    char *oa = str_printf("%s/out", a), *ob = str_printf("%s/out", b);
    char *ra = oa ? read_text(oa, "result") : NULL, *rb = ob ? read_text(ob, "result") : NULL;
    char *sa = NULL, *sb = NULL, *la = NULL, *lb = NULL, rca[64], rcb[64], **qa, **qb, **names;
    size_t nqa, nqb, nn, i;
    int diff = 0;

    if (ra)
        strip(ra);
    if (rb)
        strip(rb);
    if (!ra || !rb || strcmp(ra, "OK") || strcmp(rb, "OK")) {
        text_add(out, "    result %s: %s, %s: %s\n", na, ra && *ra ? ra : "-", nb,
                 rb && *rb ? rb : "-");
        diff = -1;
        goto out;
    }
    sa = read_text(oa, "stdout");
    sb = read_text(ob, "stdout");
    rc_list(sa, rca, sizeof rca);
    rc_list(sb, rcb, sizeof rcb);
    if (strcmp(rca, rcb)) {
        text_add(out, "    blitz2 rc %s != %s\n", rca, rcb);
        diff = 1;
    }
    la = read_text(oa, "log");
    lb = read_text(ob, "log");
    qa = tedab_requesters(la, &nqa);
    qb = tedab_requesters(lb, &nqb);
    if (!same_lines(qa, nqa, qb, nqb)) {
        text_add(out, "    requesters ");
        python_list(out, qa, nqa);
        text_add(out, " != ");
        python_list(out, qb, nqb);
        text_add(out, "\n");
        diff = 1;
    }
    tedab_free_lines(qa);
    tedab_free_lines(qb);
    /* the dumps */
    names = union_names(a, b, "out", ".scr", &nn);
    for (i = 0; i < nn; i++) {
        char *pa = str_printf("%s/%s", oa, names[i]), *pb = str_printf("%s/%s", ob, names[i]);
        struct teddump da, db;
        char err[200];
        int ha = pa && !access(pa, R_OK), hb = pb && !access(pb, R_OK);

        if (!ha || !hb) {
            text_add(out, "    dump %s missing in one run\n", names[i]);
            diff = 1;
        } else if (teddump_load(pa, &da, err, sizeof err)) {
            text_add(out, "    dump %s: %s\n", names[i], err);
            diff = 1;
        } else {
            if (teddump_load(pb, &db, err, sizeof err)) {
                text_add(out, "    dump %s: %s\n", names[i], err);
                diff = 1;
            } else {
                char *buf = NULL;
                size_t blen = 0;
                FILE *m = open_memstream(&buf, &blen);

                if (m && teddump_compare(&da, &db, f, fh, 0, m)) {
                    const char *l;
                    int k = 0;

                    fclose(m);
                    m = NULL;
                    text_add(out, "    dump %s: DIFF\n", names[i]);
                    for (l = buf; l && *l && k < 40; k++) {
                        const char *eol = strchr(l, '\n');

                        if (!eol)
                            eol = l + strlen(l);
                        text_add(out, "      %.*s\n", (int)(eol - l), l);
                        l = *eol ? eol + 1 : eol;
                    }
                    diff = 1;
                }
                if (m)
                    fclose(m);
                free(buf);
                teddump_free(&db);
            }
            teddump_free(&da);
        }
        free(pa);
        free(pb);
    }
    tedab_free_lines(names);
    /* the files of work/ and L/ */
    for (int s = 0; s < 2; s++) {
        names = union_names(a, b, s ? "L" : "work", NULL, &nn);
        for (i = 0; i < nn; i++) {
            char *pa = str_printf("%s/%s", a, names[i]), *pb = str_printf("%s/%s", b, names[i]);
            size_t lena, lenb;
            uint8_t *fa = pa ? read_file(pa, &lena) : NULL, *fb = pb ? read_file(pb, &lenb) : NULL;
            /* bb2's names: work.NAME, L.NAME */
            const char *slash = strchr(names[i], '/');
            int pl = (int)(slash - names[i]);

            if (!fa || !fb) {
                text_add(out, "    file %.*s.%s only in %s\n", pl, names[i], slash + 1,
                         fa ? na : nb);
                diff = 1;
            } else if (lena != lenb || memcmp(fa, fb, lena)) {
                text_add(out, "    file %.*s.%s differs\n", pl, names[i], slash + 1);
                diff = 1;
            }
            free(fa);
            free(fb);
            free(pa);
            free(pb);
        }
        tedab_free_lines(names);
    }
out:
    free(oa);
    free(ob);
    free(ra);
    free(rb);
    free(sa);
    free(sb);
    free(la);
    free(lb);
    return diff;
}

/* the expectations of one run; the wanted contents from the sources */
int tedab_check_expects(const struct tedab_test *t, const char *rundir,
                        const struct tedab_sources *src, struct text *out)
{
    size_t i, olen = 0;
    uint8_t *orig = t->file && src->staged ? read_file(src->staged, &olen) : NULL;
    int problems = 0;

    for (i = 0; i < t->nexpects; i++) {
        const struct tedab_expect *e = &t->expects[i];
        int l = !strcmp(e->kind, "lexists") || !strcmp(e->kind, "opts");

        if (quoted_kind(e->kind) && strcmp(e->kind, "data")) {
            /* [no]stdout, [no]log, [no]screen: TEXT in a file of out/;
               [no]file: in work/NAME */
            int neg = !strncmp(e->kind, "no", 2);
            const char *k = e->kind + (neg ? 2 : 0);
            char *p = !strcmp(k, "screen") ? str_printf("%s/out/%s.txt", rundir, e->a)
                      : !strcmp(k, "file") ? str_printf("%s/work/%s", rundir, e->a)
                      : str_printf("%s/out/%s", rundir, k);
            size_t len;
            char *text = p ? (char *)read_file(p, &len) : NULL;
            int has = text && strstr(text, (const char *)e->data) != NULL;

            free(p);
            int named = !strcmp(k, "screen") || !strcmp(k, "file");

            if (!text && !neg) {
                text_add(out, "  %s expect: %s%s%s missing\n", src->combo,
                         !strcmp(k, "screen") ? "dump " : "", named ? e->a : k,
                         !strcmp(k, "screen") ? ".txt" : "");
                problems++;
            } else if (has == neg) {
                /* TEXT as in the directive: \n, \t, \xHH */
                struct text q = { 0 };

                for (size_t j = 0; j < e->len; j++) {
                    uint8_t c = e->data[j];

                    if (c == '\n')
                        text_add(&q, "\\n");
                    else if (c == '\t')
                        text_add(&q, "\\t");
                    else if (c < 32 || c == 127)
                        text_add(&q, "\\x%02x", c);
                    else
                        text_add(&q, "%c", c);
                }
                text_add(out, "  %s expect: %s%s%s %s \"%s\"\n", src->combo, k,
                         named ? " " : "", named ? e->a : "", neg ? "has" : "lacks",
                         q.p ? q.p : "");
                free(q.p);
                problems++;
            }
            free(text);
            continue;
        }
        char *p = str_printf("%s/%s/%s", rundir, l ? "L" : "work", e->a);
        size_t len, wlen = 0;
        uint8_t *data = p ? read_file(p, &len) : NULL, *want = NULL, o[50];
        int wanted = 0, bad = 0;
        char err[200];

        free(p);
        if (!strcmp(e->kind, "absent")) {
            if (data) {
                text_add(out, "  %s expect: %s exists, expected absent\n", src->combo, e->a);
                problems++;
            }
            free(data);
            continue;
        }
        if (!data) {
            text_add(out, "  %s expect: %s missing\n", src->combo, e->a);
            problems++;
            continue;
        }
        if (!strcmp(e->kind, "data")) {
            want = e->data;
            wlen = e->len;
            wanted = 1;
        } else if (!strcmp(e->kind, "same") || !strcmp(e->kind, "bak")) {
            want = orig;
            wlen = olen;
            wanted = 1;
            bad = !orig;
        } else if (!strcmp(e->kind, "tok")) {
            char *q = str_printf("%s/%s.bb2", src->expect_dir, e->b);

            want = q ? read_file(q, &wlen) : NULL;
            free(q);
            wanted = 1;
            bad = !want;
        } else if (!strcmp(e->kind, "corpus")) {
            char *q = str_printf("%s/%s", src->corpus_out, e->b);

            want = q ? read_file(q, &wlen) : NULL;
            free(q);
            wanted = 1;
            bad = !want;
        } else if (!strcmp(e->kind, "cd")) {
            char *q = tedab_resolve(src->cd, e->b);

            want = q ? read_file(q, &wlen) : NULL;
            free(q);
            wanted = 1;
            bad = !want;
        } else if (!strcmp(e->kind, "opts")) {
            if (!tedab_opts_file(e->b, o, err, sizeof err)) {
                want = o;
                wlen = sizeof o;
            }
            wanted = 1;
            bad = !want;
        } else if (!strcmp(e->kind, "tedsave")) {
            char hex[65];

            if (tedab_oracle_hash(src->oracle, e->b, hex)) {
                text_add(out, "  %s expect: %s: no save/%s in the oracle\n", src->combo, e->a, e->b);
                problems++;
            } else {
                char got[65];

                sha256_hex_of(data, len, got);
                if (strcmp(got, hex)) {
                    text_add(out, "  %s expect: %s differs from the expected contents (tedsave)\n",
                             src->combo, e->a);
                    problems++;
                }
            }
        }
        if (bad) {
            text_add(out, "  %s expect: %s: the expected contents (%s) are missing\n",
                     src->combo, e->a, e->kind);
            problems++;
        } else if (wanted && (len != wlen || memcmp(data, want, len))) {
            text_add(out, "  %s expect: %s differs from the expected contents (%s)\n",
                     src->combo, e->a, e->kind);
            problems++;
        }
        if (want != orig && want != o && want != e->data)
            free(want);
        free(data);
    }
    free(orig);
    return problems;
}

int tedab_oracle_hash(const char *manifest, const char *cdpath, char hex[65])
{
    size_t len, k = strlen(cdpath);
    char *m = manifest ? (char *)read_file(manifest, &len) : NULL, *l;
    int r = -1;

    for (l = m; l && *l && r;) {
        char *eol = strchr(l, '\n');

        if (!eol)
            eol = l + strlen(l);
        /* "HASH  save/PATH" */
        if (eol - l > 64 + 2 + 5 + (long)k - 1 && !strncmp(l + 64, "  save/", 7)
            && (size_t)(eol - (l + 71)) >= k && !strncasecmp(l + 71, cdpath, k)
            && (l + 71 + k == eol || l[71 + k] == '\r')) {
            memcpy(hex, l, 64);
            hex[64] = 0;
            r = 0;
        }
        l = *eol ? eol + 1 : eol;
    }
    free(m);
    return r;
}

/* ------------------------------------------------------------- bug-orig */

int tedab_bugorig_verdict(const char *ref_state, const char *ref_problems,
                          const char *port_state, const char *port_problems, int compare)
{
    int ref_ok = !strcmp(ref_state, "OK"), port_ok = !strcmp(port_state, "OK");

    if (ref_ok && !*ref_problems)
        return TEDAB_BROKEN;
    if (port_ok && !*port_problems)
        return TEDAB_FIXED;
    if (!strcmp(ref_state, port_state) && !strcmp(ref_problems, port_problems)
        && (!ref_ok || !compare))
        return TEDAB_REPRODUCED;
    return TEDAB_DIFFERS;
}

/* the recorded original: "# NOTE", "[state]", the state line, "[problems]",
   the problem lines */
int tedab_orig_format(const char *state, const char *problems, const char *note, struct text *t,
                      char *err, size_t errlen)
{
    char *s = NULL, *p = NULL;
    int r;

    memset(t, 0, sizeof *t);
    if (!*state || strchr(state, '\n') || (note && strchr(note, '\n'))) {
        snprintf(err, errlen, "the state is empty or has a line feed");
        return -1;
    }
    if (note && *note)
        text_add(t, "# %s\n", note);
    text_add(t, "[state]\n%s\n[problems]\n%s", state, problems);
    /* it must read back as it was (the last problem line ends with a LF) */
    r = t->bad ? -1 : tedab_orig_parse(t->p, &s, &p, err, errlen);
    if (!r && (strcmp(s, state) || strcmp(p, problems))) {
        snprintf(err, errlen, "the observation does not read back as it is");
        r = -1;
    }
    free(s);
    free(p);
    if (r) {
        free(t->p);
        memset(t, 0, sizeof *t);
    }
    return r;
}

int tedab_orig_parse(const char *text, char **state, char **problems, char *err, size_t errlen)
{
    const char *p = text, *eol, *st;

    *state = *problems = NULL;
    while (*p == '#') {                 /* the note */
        eol = strchr(p, '\n');
        p = eol ? eol + 1 : p + strlen(p);
    }
    if (strncmp(p, "[state]\n", 8)) {
        snprintf(err, errlen, "[state] expected");
        return -1;
    }
    st = p + 8;
    if (!(eol = strchr(st, '\n')) || eol == st || strncmp(eol + 1, "[problems]\n", 11)) {
        snprintf(err, errlen, "a state line and [problems] expected");
        return -1;
    }
    *state = strndup(st, (size_t)(eol - st));
    *problems = strdup(eol + 12);
    if (!*state || !*problems) {
        free(*state);
        free(*problems);
        *state = *problems = NULL;
        snprintf(err, errlen, "out of memory");
        return -1;
    }
    return 0;
}

const char *tedab_verdict_name(int verdict)
{
    switch (verdict) {
    case TEDAB_FIXED:
        return "FIXED";
    case TEDAB_REPRODUCED:
        return "REPRODUCED";
    case TEDAB_BROKEN:
        return "BROKEN";
    case TEDAB_DIFFERS:
        return "DIFFERS";
    }
    return "?";
}
