/*
 * corpus.c - one pass of the corpus check, see corpus.h.
 */
#define _XOPEN_SOURCE 700       /* nftw, strdup, strndup */
#include <errno.h>
#include <ftw.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include "amiga_emulation.h"
#include "bugorig.h"
#include "corpus.h"
#include "serial.h"
#include "statline_emulation.h"
#include "util.h"

#define VOLUME "CORPUS"                 /* the configuration's directory */

/* ------------------------------------------------------------ strings */


/* ------------------------------------------------------- the pass file */

void corpus_free(struct corpus_pass *p)
{
    size_t i;

    free(p->work);
    free(p->report);
    free(p->ted);
    free(p->launcher);
    free(p->gate);
    free(p->defaultdbug);
    free(p->serial_same_as);
    free(p->expect);
    free(p->wrap);
    free(p->wrap_args);
    free(p->runwrap);
    free(p->runwrap_args);
    free(p->runargs);
    free(p->machine);
    free(p->rcput);
    for (i = 0; i < p->nfiles; i++) {
        free(p->file_dest[i]);
        free(p->file_src[i]);
        free(p->file_tag[i]);
    }
    free(p->file_dest);
    free(p->file_src);
    free(p->file_tag);
    for (i = 0; i < p->ndirs; i++)
        free(p->dirs[i]);
    free(p->dirs);
    for (i = 0; i < p->nremoves; i++)
        free(p->removes[i]);
    free(p->removes);
    for (i = 0; i < p->njoblines; i++)
        free(p->joblines[i]);
    free(p->joblines);
    for (i = 0; i < p->nuae; i++)
        free(p->uae[i]);
    free(p->uae);
    for (i = 0; i < p->nconfigs; i++) {
        free(p->configs[i].tag);
        free(p->configs[i].blitz2);
        free(p->configs[i].deflibs);
        free(p->configs[i].acidlibs);
    }
    free(p->configs);
    for (i = 0; i < p->nsources; i++)
        free(p->sources[i]);
    free(p->sources);
    for (i = 0; i < p->nmounts; i++) {
        free(p->mount_name[i]);
        free(p->mount_dir[i]);
    }
    free(p->mount_name);
    free(p->mount_dir);
    memset(p, 0, sizeof *p);
}

static int fail(struct corpus_pass *p, char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    corpus_free(p);
    return -1;
}

/* the next field up to '|' (malloc'd) */
static char *field(const char **s)
{
    const char *e = strchr(*s, '|');
    char *f;

    if (!e)
        e = *s + strlen(*s);
    f = strndup(*s, (size_t)(e - *s));
    *s = *e ? e + 1 : e;
    return f;
}

/* appends v to the list (takes it); 0 = ok */
static int push(char ***list, size_t *n, char *v)
{
    char **l = realloc(*list, (*n + 1) * sizeof *l);

    if (!l)
        return -1;
    *list = l;
    l[(*n)++] = v;
    return 0;
}

/* a path inside the volume: relative, no colon, no empty or .. part */
static int inside(const char *d)
{
    const char *s = d;

    if (!*d || *d == '/' || strchr(d, ':') || strchr(d, '\\'))
        return 0;
    for (;;) {
        size_t n = strcspn(s, "/");

        if (!n || (n == 2 && !strncmp(s, "..", 2)))
            return 0;
        if (!s[n])
            return 1;
        s += n + 1;
    }
}

int corpus_parse(const char *text, struct corpus_pass *p, char *err, size_t errlen)
{
    const char *line = text;
    unsigned no = 0;
    size_t i, k;

    memset(p, 0, sizeof *p);
    p->timeout = 300;
    p->parallel = 1;
    p->stack = 40000;
    p->run = 1;
    while (*line) {
        const char *eol = strchr(line, '\n'), *v;
        char key[32], *val;
        size_t n;

        if (!eol)
            eol = line + strlen(line);
        no++;
        n = (size_t)(eol - line);
        if (n && line[n - 1] == '\r')
            n--;
        if (!n || *line == '#') {
            line = *eol ? eol + 1 : eol;
            continue;
        }
        if (!(v = memchr(line, '=', n)) || (size_t)(v - line) >= sizeof key)
            return fail(p, err, errlen, "line %u: expected KEY=VALUE", no);
        memcpy(key, line, (size_t)(v - line));
        key[v - line] = 0;
        v++;
        if (!(val = strndup(v, n - (size_t)(v - line))))
            return fail(p, err, errlen, "out of memory");
        line = *eol ? eol + 1 : eol;

        if (!strcmp(key, "work") || !strcmp(key, "report") || !strcmp(key, "ted")
            || !strcmp(key, "launcher") || !strcmp(key, "gate") || !strcmp(key, "defaultdbug")
            || !strcmp(key, "serial_same_as") || !strcmp(key, "expect") || !strcmp(key, "rcput")) {
            char **dst = !strcmp(key, "work") ? &p->work : !strcmp(key, "report") ? &p->report
                         : !strcmp(key, "ted") ? &p->ted : !strcmp(key, "launcher") ? &p->launcher
                         : !strcmp(key, "gate") ? &p->gate : !strcmp(key, "expect") ? &p->expect
                         : !strcmp(key, "defaultdbug") ? &p->defaultdbug
                         : !strcmp(key, "rcput") ? &p->rcput : &p->serial_same_as;

            free(*dst);
            *dst = val;
            continue;
        }
        if (!strcmp(key, "runtime_errors") || !strcmp(key, "serial") || !strcmp(key, "regression")
            || !strcmp(key, "run") || !strcmp(key, "timeout") || !strcmp(key, "parallel")
            || !strcmp(key, "stack") || !strcmp(key, "fixed")) {
            char *e;
            unsigned long u = strtoul(val, &e, 10);

            if (!strcmp(key, "runtime_errors") || !strcmp(key, "serial")
                || !strcmp(key, "regression") || !strcmp(key, "run")) {
                if (strcmp(val, "on") && strcmp(val, "off")) {
                    free(val);
                    return fail(p, err, errlen, "line %u: %s=on or %s=off", no, key, key);
                }
                *(!strcmp(key, "runtime_errors") ? &p->runtime_errors
                  : !strcmp(key, "serial") ? &p->serial
                  : !strcmp(key, "run") ? &p->run : &p->regression) = !strcmp(val, "on");
            } else if (!*val || *e || !u) {
                free(val);
                return fail(p, err, errlen, "line %u: %s needs a number > 0", no, key);
            } else if (!strcmp(key, "timeout"))
                p->timeout = (unsigned)u;
            else if (!strcmp(key, "parallel"))
                p->parallel = (unsigned)u;
            else if (!strcmp(key, "fixed"))
                p->fixed = (unsigned)u;
            else
                p->stack = (unsigned)u;
            free(val);
            continue;
        }
        if (!strcmp(key, "config")) {
            struct corpus_config c, *v2;
            const char *s = val;
            char *expect;

            c.tag = field(&s);
            expect = field(&s);
            c.blitz2 = field(&s);
            c.deflibs = field(&s);
            c.acidlibs = field(&s);
            c.expect = expect && !strcmp(expect, "ref") ? CORPUS_REF
                       : expect && !strcmp(expect, "exe") ? CORPUS_EXE : CORPUS_OUTPUT;
            if (!c.tag || !*c.tag || !expect || !c.acidlibs || !*c.acidlibs || *s
                || (strcmp(expect, "ref") && strcmp(expect, "exe") && strcmp(expect, "output"))
                || (c.expect == CORPUS_REF && p->nconfigs)
                || !(v2 = realloc(p->configs, (p->nconfigs + 1) * sizeof *v2))) {
                free(c.tag);
                free(expect);
                free(c.blitz2);
                free(c.deflibs);
                free(c.acidlibs);
                free(val);
                return fail(p, err, errlen,
                            "line %u: config=TAG|ref|exe|output|BLITZ2|DEFLIBS|ACIDLIBS "
                            "(the first and only the first is ref)", no);
            }
            free(expect);
            free(val);
            p->configs = v2;
            p->configs[p->nconfigs++] = c;
            continue;
        }
        if (!strcmp(key, "source")) {
            char **v2 = realloc(p->sources, (p->nsources + 1) * sizeof *v2);

            if (!v2) {
                free(val);
                return fail(p, err, errlen, "out of memory");
            }
            p->sources = v2;
            p->sources[p->nsources++] = val;
            continue;
        }
        if (!strcmp(key, "mount")) {
            const char *s = val;
            char *name = field(&s), *dir = field(&s), **n2, **d2;

            if (!name || !*name || !dir || !*dir || *s || strchr(name, ':')
                || !(n2 = realloc(p->mount_name, (p->nmounts + 1) * sizeof *n2))
                || !(p->mount_name = n2, d2 = realloc(p->mount_dir, (p->nmounts + 1) * sizeof *d2))) {
                free(name);
                free(dir);
                free(val);
                return fail(p, err, errlen, "line %u: mount=NAME|DIR (NAME without colon)", no);
            }
            p->mount_dir = d2;
            p->mount_name[p->nmounts] = name;
            p->mount_dir[p->nmounts++] = dir;
            free(val);
            continue;
        }
        if (!strcmp(key, "file")) {
            const char *s = val;
            char *dest = field(&s), *src = field(&s), **d2, **s2, **t2;
            /* a third field (also an empty one) is the configuration */
            char *tag = s > val && s[-1] == '|' ? field(&s) : NULL;

            if (!dest || !src || !inside(dest) || !*src || (tag && !*tag) || *s
                || !(d2 = realloc(p->file_dest, (p->nfiles + 1) * sizeof *d2))
                || !(p->file_dest = d2, s2 = realloc(p->file_src, (p->nfiles + 1) * sizeof *s2))
                || !(p->file_src = s2, t2 = realloc(p->file_tag, (p->nfiles + 1) * sizeof *t2))) {
                free(dest);
                free(src);
                free(tag);
                free(val);
                return fail(p, err, errlen,
                            "line %u: file=DEST|SRC[|TAG] (DEST inside the volume)", no);
            }
            p->file_tag = t2;
            p->file_tag[p->nfiles] = tag;
            p->file_dest[p->nfiles] = dest;
            p->file_src[p->nfiles++] = src;
            free(val);
            continue;
        }
        if (!strcmp(key, "dir") || !strcmp(key, "remove")) {
            int d = !strcmp(key, "dir");

            if (!inside(val) || push(d ? &p->dirs : &p->removes, d ? &p->ndirs : &p->nremoves, val)) {
                free(val);
                return fail(p, err, errlen, "line %u: %s=DEST (inside the volume)", no, key);
            }
            continue;
        }
        if (!strcmp(key, "machine")) {
            if (!*val || !amiga_emulation_machine_known(val)) {
                free(val);
                return fail(p, err, errlen, "line %u: machine=NAME (a1200, a500, a500-68010)", no);
            }
            free(p->machine);
            p->machine = val;
            continue;
        }
        if (!strcmp(key, "uae")) {
            const char *eq = strchr(val, '=');

            if (!eq || eq == val || push(&p->uae, &p->nuae, val)) {
                free(val);
                return fail(p, err, errlen, "line %u: uae=KEY=VALUE", no);
            }
            continue;
        }
        if (!strcmp(key, "jobline")) {
            if (!*val || push(&p->joblines, &p->njoblines, val)) {
                free(val);
                return fail(p, err, errlen, "line %u: jobline=LINE", no);
            }
            continue;
        }
        if (!strcmp(key, "runargs")) {
            if (!*val) {
                free(val);
                return fail(p, err, errlen, "line %u: runargs=ARGS", no);
            }
            free(p->runargs);
            p->runargs = val;
            continue;
        }
        if (!strcmp(key, "wrap") || !strcmp(key, "runwrap")) {
            const char *s = val;
            char *path = field(&s), *args = *val && strchr(val, '|') ? strdup(s) : NULL;
            int run = !strcmp(key, "runwrap");

            if (!path || !*path || (strchr(val, '|') && !args)) {
                free(path);
                free(args);
                free(val);
                return fail(p, err, errlen, "line %u: %s=PATH[|ARGS]", no, key);
            }
            free(run ? p->runwrap : p->wrap);
            free(run ? p->runwrap_args : p->wrap_args);
            *(run ? &p->runwrap : &p->wrap) = path;
            *(run ? &p->runwrap_args : &p->wrap_args) = args;
            free(val);
            continue;
        }
        free(val);
        return fail(p, err, errlen, "line %u: unknown key %s", no, key);
    }
    if (!p->work || !p->report || !p->ted || (p->run && !p->launcher)
        || (!p->defaultdbug && !p->fixed))
        return fail(p, err, errlen, "work, report, ted, launcher and defaultdbug (optional with "
                    "fixed=) are needed");
    /* compile only: nothing that belongs to the runs */
    if (!p->run && (p->launcher || p->gate || p->serial || p->runwrap || p->runargs || p->expect
                    || p->serial_same_as))
        return fail(p, err, errlen, "run=off: no launcher, gate, serial, runwrap, runargs, "
                    "expect or serial_same_as");
    /* a fixed bug-orig test has no reference, every other pass one first */
    if (p->fixed) {
        if (!p->expect || p->regression)
            return fail(p, err, errlen, "fixed= needs expect= and no regression=on");
        if (!p->nconfigs || p->configs[0].expect == CORPUS_REF || !p->nsources)
            return fail(p, err, errlen, "fixed=: configurations without a reference (no "
                        "EXPECT ref) and a source are needed");
    } else if (p->nconfigs && p->configs[0].expect != CORPUS_REF)
        return fail(p, err, errlen, "the first configuration is the reference (EXPECT ref)");
    else if (p->nconfigs < 2 || !p->nsources)
        return fail(p, err, errlen, "a reference, another configuration and sources are needed");
    if (p->expect && (p->nsources != 1 || p->serial_same_as))
        return fail(p, err, errlen, "expect= needs exactly one source and no serial_same_as");
    if (p->regression && !p->expect)
        return fail(p, err, errlen, "regression=on needs expect=");
    /* Kickstart 1.x: no $RC (rcput instead), no gate (reqclosetest needs V36) */
    if (amiga_emulation_machine_kickstart(p->machine) < 36 && (!p->rcput || p->gate))
        return fail(p, err, errlen, "machine=%s (Kickstart 1.x) needs rcput= and no gate=",
                    p->machine);
    /* names: one directory per configuration, AmigaDOS ignores case */
    for (i = 0; i < p->nconfigs; i++)
        for (k = 0; k < i; k++)
            if (!strcasecmp(p->configs[i].tag, p->configs[k].tag))
                return fail(p, err, errlen, "configuration %s twice", p->configs[i].tag);
    /* the configuration of a file= with TAG */
    for (i = 0; i < p->nfiles; i++) {
        if (!p->file_tag[i])
            continue;
        for (k = 0; k < p->nconfigs; k++)
            if (!strcmp(p->file_tag[i], p->configs[k].tag))
                break;
        if (k == p->nconfigs)
            return fail(p, err, errlen, "file=%s: no configuration %s", p->file_dest[i],
                        p->file_tag[i]);
    }
    for (i = 0; i < p->nsources; i++)
        for (k = 0; k < i; k++) {
            char *a = corpus_name(p->sources[i]), *b = corpus_name(p->sources[k]);
            int same = a && b && !strcasecmp(a, b);

            free(a);
            free(b);
            if (same)
                return fail(p, err, errlen, "two sources named like %s", p->sources[i]);
        }
    return 0;
}

char *corpus_name(const char *source)
{
    const char *b = strrchr(source, '/') ? strrchr(source, '/') + 1 : source;
    size_t n = strlen(b);

    if (n > 4 && !strcasecmp(b + n - 4, ".bb2"))
        n -= 4;
    return strndup(b, n);
}

void corpus_mask_traps(uint8_t *data, size_t len)
{
    size_t i = 0;
    int pass;

    /* first all trap #1, then all trap #4, each search going on behind the
       zeroed block (as bb2's mask_trap1) */
    for (pass = 0; pass < 2; pass++)
        for (i = 0; i + 10 <= len; ) {
            if (data[i] == 0x4E && data[i + 1] == (pass ? 0x44 : 0x41)) {
                memset(data + i + 2, 0, 8);
                i += 10;
            } else
                i++;
        }
}

/* ------------------------------------------------------------- files */


/* ------------------------------------------------------------- staging */

/* the machine has Kickstart 1.x: AmigaDOS 1.3 scripts, rcput */
static int kick1x(const struct corpus_pass *p)
{
    return amiga_emulation_machine_kickstart(p->machine) < 36;
}

/* the directory of configuration c: WORK/TAG/vol = CORPUS:, WORK/TAG/emu */
static char *dir_of(const struct corpus_pass *p, size_t c, const char *sub)
{
    return str_printf("%s/%s/%s", p->work, p->configs[c].tag, sub);
}

/* creates WORK/TAG/SUB */
static int mkdir_sub(const struct corpus_pass *p, size_t c, const char *sub)
{
    char *d = dir_of(p, c, sub);
    int r = d ? mkdir_p(d) : -1;

    free(d);
    return r;
}

static int stage(const struct corpus_pass *p, size_t c, FILE *log)
{
    const struct corpus_config *cf = &p->configs[c];
    char *root = str_printf("%s/%s", p->work, cf->tag), *vol = dir_of(p, c, "vol");
    struct { const char *from, *to; } files[] = {
        { cf->blitz2, "blitz2/blitz2" }, { p->ted, "blitz2/ted" },
        { cf->deflibs, "blitz2/deflibs" }, { cf->acidlibs, "blitz2/acidlibs" },
        { p->defaultdbug, "blitz2/defaultdbug" }, { p->launcher, "run" },
        { p->gate, "gate" }, { p->wrap, "wrap" }, { p->runwrap, "runwrap" },
        { kick1x(p) ? p->rcput : NULL, "rcput" },
    };
    struct text jobs = { 0 };
    size_t i;
    int r = -1;

    if (!root || !vol)
        goto out;
    rm_tree(root);
    if (mkdir_sub(p, c, "vol/blitz2/userlibs") || mkdir_sub(p, c, "vol/src")
        || mkdir_sub(p, c, "vol/out") || mkdir_sub(p, c, "emu")) {
        fprintf(log, "corpus: %s: cannot create the directories\n", root);
        goto out;
    }
    for (i = 0; i < sizeof files / sizeof files[0]; i++) {
        char *to;

        if (!files[i].from)
            continue;
        to = str_printf("%s/%s", vol, files[i].to);
        if (!to || copy_file(files[i].from, to)) {
            fprintf(log, "corpus: cannot copy %s\n", files[i].from);
            free(to);
            goto out;
        }
        free(to);
    }
    for (i = 0; i < p->nfiles; i++) {
        char *to, *slash;
        int bad;

        if (p->file_tag[i] && strcmp(p->file_tag[i], cf->tag))
            continue;
        to = str_printf("%s/%s", vol, p->file_dest[i]);
        slash = to ? strrchr(to, '/') : NULL;
        bad = !to;

        if (slash) {
            *slash = 0;
            bad = mkdir_p(to);
            *slash = '/';
        }
        if (bad || copy_file(p->file_src[i], to)) {
            fprintf(log, "corpus: cannot copy %s to %s\n", p->file_src[i], p->file_dest[i]);
            free(to);
            goto out;
        }
        free(to);
    }
    for (i = 0; i < p->ndirs; i++) {
        char *d = str_printf("%s/%s", vol, p->dirs[i]);

        if (!d || mkdir_p(d)) {
            fprintf(log, "corpus: cannot create %s\n", p->dirs[i]);
            free(d);
            goto out;
        }
        free(d);
    }
    for (i = 0; i < p->nremoves; i++) {
        char *d = str_printf("%s/%s", vol, p->removes[i]);

        if (!d) {
            fprintf(log, "corpus: out of memory\n");
            goto out;
        }
        rm_tree(d);
        free(d);
    }
    text_add(&jobs, "log=" VOLUME ":out/session.log\ndefaults=" VOLUME ":out/defaults.xtra\n");
    if (!p->runtime_errors)
        text_add(&jobs, "runerr=off\n");
    for (i = 0; i < p->nsources; i++) {
        char *name = corpus_name(p->sources[i]), *to = str_printf("%s/src/%s.bb2", vol, name);

        if (!name || !to || copy_file(p->sources[i], to)) {
            fprintf(log, "corpus: cannot copy %s\n", p->sources[i]);
            free(name);
            free(to);
            goto out;
        }
        text_add(&jobs, "job=j%03zu|" VOLUME ":src/%s.bb2|" VOLUME ":out/%s\n", i, name, name);
        free(name);
        free(to);
    }
    for (i = 0; i < p->njoblines; i++)
        text_add(&jobs, "%s\n", p->joblines[i]);
    {
        char *to = str_printf("%s/jobs", vol);

        r = jobs.bad || !to || write_file(to, jobs.p, jobs.n) ? -1 : 0;
        free(to);
    }
out:
    free(jobs.p);
    free(root);
    free(vol);
    return r;
}

/* AmigaDOS 1.3 (Kickstart 1.x, tools/lib/amiga_emulation/README.md "Machine
   a500"): the redirection right behind the command name, no $RC and no >> -
   rcput appends rc=N; no gate (corpus_parse) */
static char *script13(const struct corpus_pass *p)
{
    struct text s = { 0 };
    size_t i;

    text_add(&s, "Assign blitz2: " VOLUME ":blitz2\n"
            "Stack %u\n", p->stack);
    if (p->wrap)
        text_add(&s, VOLUME ":wrap >" VOLUME ":out/blitz2.out %s%sblitz2:blitz2 " VOLUME ":jobs\n",
            p->wrap_args ? p->wrap_args : "", p->wrap_args ? " " : "");
    else
        text_add(&s, "blitz2:blitz2 >" VOLUME ":out/blitz2.out " VOLUME ":jobs\n");
    text_add(&s, VOLUME ":rcput " VOLUME ":out/blitz2.out\n"
            "Assign blitz2:\n");
    for (i = 0; p->run && i < p->nsources; i++) {
        char *n = corpus_name(p->sources[i]);

        text_add(&s, "If EXISTS " VOLUME ":out/%s\n"
                "  " VOLUME ":run >" VOLUME ":out/%s.run %s%s%s" VOLUME ":out/%s%s%s\n"
                "  " VOLUME ":rcput " VOLUME ":out/%s.run\n"
                "EndIf\n", n, n, p->runwrap ? VOLUME ":runwrap " : "",
                p->runwrap_args ? p->runwrap_args : "", p->runwrap_args ? " " : "", n,
                p->runargs ? " " : "", p->runargs ? p->runargs : "", n);
        free(n);
    }
    if (s.bad) {
        free(s.p);
        return NULL;
    }
    return s.p;
}

char *corpus_script(const struct corpus_pass *p)
{
    struct text s = { 0 };
    size_t i;

    if (kick1x(p))
        return script13(p);
    text_add(&s, "Assign blitz2: " VOLUME ":blitz2\n"
            "Stack %u\n"
            "%s%s%sblitz2:blitz2 " VOLUME ":jobs >" VOLUME ":out/blitz2.out\n"
            "Echo \"rc=$RC\" >>" VOLUME ":out/blitz2.out\n"
            "Assign blitz2:\n", p->stack, p->wrap ? VOLUME ":wrap " : "",
            p->wrap_args ? p->wrap_args : "", p->wrap_args ? " " : "");
    if (p->gate)
        text_add(&s, "SetEnv REQCLOSELOG " VOLUME ":out/reqclose.log\n"
                VOLUME ":run " VOLUME ":gate >" VOLUME ":out/gate.run\n"
                "Set gate $RC\n"
                "Echo \"rc=$gate\" >>" VOLUME ":out/gate.run\n"
                "If VAL $gate EQ 0\n");
    for (i = 0; p->run && i < p->nsources; i++) {
        char *n = corpus_name(p->sources[i]);

        text_add(&s, "If EXISTS " VOLUME ":out/%s\n"
                "  " VOLUME ":run %s%s%s" VOLUME ":out/%s%s%s >" VOLUME ":out/%s.run\n"
                "  Echo \"rc=$RC\" >>" VOLUME ":out/%s.run\n"
                "EndIf\n", n, p->runwrap ? VOLUME ":runwrap " : "",
                p->runwrap_args ? p->runwrap_args : "", p->runwrap_args ? " " : "", n,
                p->runargs ? " " : "", p->runargs ? p->runargs : "", n, n);
        free(n);
    }
    if (p->gate)
        text_add(&s, "EndIf\nUnSetEnv REQCLOSELOG\n");
    if (s.bad) {
        free(s.p);
        return NULL;
    }
    return s.p;
}

/* ------------------------------------------------------------ running */

/* port != 0: the serial port is a TCP server on 127.0.0.1:port */
static struct amiga_emulation *start(const struct corpus_pass *p, size_t c, const char *text,
                                     unsigned port, char *err, size_t errlen)
{
    char *vol = dir_of(p, c, "vol"), *emu = dir_of(p, c, "emu");
    char *tcp = str_printf("TCP://127.0.0.1:%u", port);
    struct amiga_emulation_mount *m = calloc(p->nmounts + 1, sizeof *m);
    struct amiga_emulation_option serial[] = {
        { "serial_port", tcp }, { "serial_on_demand", "false" },
        { "serial_direct", "false" }, { "serial_hardware_ctsrts", "false" },
    };
    size_t nserial = port ? sizeof serial / sizeof serial[0] : 0;
    struct amiga_emulation_option *ch = calloc(nserial + p->nuae + 1, sizeof *ch);
    char **keys = calloc(p->nuae + 1, sizeof *keys);
    struct amiga_emulation_params ep = { 0 };
    struct amiga_emulation *e = NULL;
    size_t i;

    if (vol && emu && m && tcp && ch && keys) {
        for (i = 0; i < nserial; i++)
            ch[i] = serial[i];
        for (i = 0; i < p->nuae && (keys[i] = strdup(p->uae[i])); i++) {
            char *eq = strchr(keys[i], '=');

            *eq = 0;
            ch[nserial + i].key = keys[i];
            ch[nserial + i].value = eq + 1;
        }
        ep.machine = p->machine;
        ep.changes = ch;
        ep.nchanges = nserial + i;
        /* CORPUS: the configuration's directory, the read-only volumes */
        m[0].volume = VOLUME;
        m[0].host_dir = vol;
        m[0].writable = 1;
        for (i = 0; i < p->nmounts; i++) {
            m[i + 1].volume = p->mount_name[i];
            m[i + 1].host_dir = p->mount_dir[i];
        }
        ep.mounts = m;
        ep.nmounts = p->nmounts + 1;
        ep.script = text;
        ep.timeout = p->timeout;
        ep.work_dir = emu;
        e = amiga_emulation_start(&ep, err, errlen);
    } else
        snprintf(err, errlen, "out of memory");
    free(m);
    for (i = 0; keys && i < p->nuae; i++)
        free(keys[i]);
    free(keys);
    free(ch);
    free(tcp);
    free(vol);
    free(emu);
    return e;
}

/* ------------------------------------------------------------ results */

/* the RESULT line of job ID in the session log ("" if none) */
static char *result_of(const char *log, size_t id)
{
    char key[32];
    const char *s;

    snprintf(key, sizeof key, "RESULT\tj%03zu\t", id);
    if (!log || !(s = strstr(log, key)))
        return strdup("no RESULT (compiler crashed or hung?)");
    s += strlen(key);
    return strndup(s, strcspn(s, "\n"));
}

struct outcome {
    uint8_t *exe, *run, *serial;   /* serial: sent over the serial port */
    size_t exelen, runlen, seriallen;
    char *result;
};

static void load_outcome(const struct corpus_pass *p, size_t c, size_t i, const char *log,
                         struct outcome *o)
{
    char *n = corpus_name(p->sources[i]);
    char *exe = str_printf("%s/%s/vol/out/%s", p->work, p->configs[c].tag, n);
    char *run = str_printf("%s/%s/vol/out/%s.run", p->work, p->configs[c].tag, n);
    char *serial = str_printf("%s/%s/vol/out/%s.serial", p->work, p->configs[c].tag, n);

    memset(o, 0, sizeof *o);
    if (exe)
        o->exe = read_file(exe, &o->exelen);
    if (run)
        o->run = read_file(run, &o->runlen);
    if (serial && p->serial)
        o->serial = read_file(serial, &o->seriallen);
    if (o->exe && p->runtime_errors)
        corpus_mask_traps(o->exe, o->exelen);
    o->result = result_of(log, i);
    free(n);
    free(exe);
    free(run);
    free(serial);
}

static void free_outcome(struct outcome *o)
{
    free(o->serial);
    free(o->exe);
    free(o->run);
    free(o->result);
}

/* rc of the gate (gate.run's last "rc=" line), -1 if not there */
static int gate_rc(const struct corpus_pass *p, size_t c)
{
    char *path = str_printf("%s/%s/vol/out/gate.run", p->work, p->configs[c].tag);
    size_t len;
    char *t = path ? (char *)read_file(path, &len) : NULL, *s;
    int rc = -1;

    for (s = t; s && (s = strstr(s, "rc=")); s += 3)
        rc = atoi(s + 3);
    free(t);
    free(path);
    return rc;
}

static int same(const uint8_t *a, size_t al, const uint8_t *b, size_t bl)
{
    return a && b && al == bl && !memcmp(a, b, al);
}

/* ------------------------------------------------------------ bug-orig */

/* the observation of configuration c (bugorig_observe) */
static int observe(const struct corpus_pass *p, size_t c, enum amiga_emulation_status st,
                   const char *slog, struct bugorig_obs *o)
{
    char *n = corpus_name(p->sources[0]);
    char *run = n ? str_printf("%s/%s/vol/out/%s.run", p->work, p->configs[c].tag, n) : NULL;
    char *b2 = str_printf("%s/%s/vol/out/blitz2.out", p->work, p->configs[c].tag);
    size_t len;
    char *rt = run ? (char *)read_file(run, &len) : NULL, *bt = b2 ? (char *)read_file(b2, &len) : NULL;
    struct bugorig_outputs out = { amiga_emulation_status_name(st), slog, bt, rt, "j000" };
    int r = n && run && b2 ? bugorig_observe(&out, o) : -1;

    free(rt);
    free(bt);
    free(run);
    free(b2);
    free(n);
    return r;
}

/* bug-orig mode (expect=): every configuration judged against the
   expectation and the reference; exit status in corpus.h */
static int judge(const struct corpus_pass *p, const enum amiga_emulation_status *st, char **slog,
                 struct statline *stl, FILE *log)
{
    struct bugorig_obs e, *o = calloc(p->nconfigs, sizeof *o);
    struct text rep = { 0 };
    char err[300], *name = corpus_name(p->sources[0]);
    size_t len, c, first;
    char *etext = (char *)read_file(p->expect, &len);
    int rc = 2, worst = 0;

    if (!o || !name) {
        fprintf(log, "corpus: out of memory\n");
        goto out;
    }
    if (!etext || bugorig_parse(etext, &e, err, sizeof err)) {
        fprintf(log, "corpus: %s: %s\n", p->expect, etext ? err : "cannot read it");
        goto out;
    }
    /* a session that could not run is no observation (a timeout is one: a
       crash or a hang; a halt too: a crash that halted the CPU) */
    for (c = 0; c < p->nconfigs; c++)
        if (st[c] == AMIGA_EMULATION_FAILED || st[c] == AMIGA_EMULATION_EXITED) {
            fprintf(log, "corpus: %s: session %s (%s/%s/emu)\n", p->configs[c].tag,
                    amiga_emulation_status_name(st[c]), p->work, p->configs[c].tag);
            goto out_e;
        }
    for (c = 0; c < p->nconfigs; c++)
        if (observe(p, c, st[c], slog[c], &o[c])) {
            fprintf(log, "corpus: out of memory\n");
            goto out_e;
        }
    /* what each configuration showed, for writing an expectation */
    for (c = 0; c < p->nconfigs; c++) {
        struct text t = { 0 };
        char *path = str_printf("%s/%s/observation", p->work, p->configs[c].tag);
        size_t k;

        for (k = 0; k < o[c].n; k++)
            text_add(&t, "[%s]\n%s", o[c].sec[k].name, o[c].sec[k].text);
        if (t.bad || !path || write_file(path, t.p ? t.p : "", t.n))
            fprintf(log, "corpus: cannot write %s\n", path ? path : "the observation");
        free(t.p);
        free(path);
    }
    if (p->fixed)
        text_add(&rep, "== fixed bug-orig %s (#%u): runtime error checking %s, expectation %s\n",
            name, p->fixed, p->runtime_errors ? "on" : "off", p->expect);
    else
        text_add(&rep, "== %s %s: runtime error checking %s, expectation %s\n",
            p->regression ? "port regression" : "bug-orig", name,
            p->runtime_errors ? "on" : "off", p->expect);
    /* a fixed test (fixed=) has no reference: every configuration judged */
    first = p->fixed ? 0 : 1;
    if (!p->fixed)
        statline_set(stl[0].verdict, sizeof stl[0].verdict, "reference");
    /* a bug-orig test needs a reference that does not show the expected
       behaviour, a port regression test (regression=on) one that does */
    if (!p->fixed && bugorig_matches(&e, &o[0]) != p->regression) {
        statline_set(stl[0].verdict, sizeof stl[0].verdict, "BROKEN");
        text_add(&rep, p->regression
            ? "BROKEN TEST: the original does not show the expected behaviour - the "
              "expectation is not what the original does\n"
            : "BROKEN TEST: the original shows the expected (correct) behaviour - it does "
              "not have the bug, or the expectation describes the bug\n");
        worst = 3;
    }
    for (c = first; c < p->nconfigs && worst < 3; c++) {
        struct text why = { 0 };
        enum bugorig_verdict v = p->regression ? bugorig_judge_regression(&e, &o[0], &o[c], &why)
                                 : bugorig_judge(&e, p->fixed ? NULL : &o[0], &o[c], &why);
        /* REGRESSED (fixed=, no reference) counts like REPRODUCED: exit 1 */
        static const int order[] = { [BUGORIG_FIXED] = 0, [BUGORIG_REPRODUCED] = 1,
                                     [BUGORIG_DIFFERS] = 2, [BUGORIG_BROKEN] = 3,
                                     [BUGORIG_REGRESSED] = 1 };

        statline_set(stl[c].verdict, sizeof stl[c].verdict, bugorig_verdict_name(v));
        text_add(&rep, "%s: %s%s\n", p->configs[c].tag, bugorig_verdict_name(v),
            v == BUGORIG_FIXED && p->regression ? " - the configuration does what the original does"
            : v == BUGORIG_FIXED ? " - the configuration behaves correctly"
            : v == BUGORIG_REGRESSED ? " - THE FIX IS UNDONE: the configuration does not behave "
                                       "correctly"
            : v == BUGORIG_REPRODUCED ? " - the bug of the original, reproduced on purpose"
            : p->regression ? " - THE PORT DOES NOT DO WHAT THE ORIGINAL DOES"
            : " - THE PORT DOES NOT REPRODUCE THE ORIGINAL");
        if (why.p)
            text_add(&rep, "%s", why.p);
        free(why.p);
        if (order[v] > worst)
            worst = order[v];
    }
    for (c = 0; c < p->nconfigs; c++)
        text_add(&rep, "session %s: %s (observation: %s/%s/observation)\n", p->configs[c].tag,
            amiga_emulation_status_name(st[c]), p->work, p->configs[c].tag);
    if (rep.bad || write_file(p->report, rep.p, rep.n)) {
        fprintf(log, "corpus: cannot write %s\n", p->report);
        goto out_e;
    }
    fwrite(rep.p, 1, rep.n, log);
    rc = (int[]){ 0, 1, 4, 3 }[worst];
out_e:
    bugorig_free(&e);
out:
    for (c = 0; o && c < p->nconfigs; c++)
        bugorig_free(&o[c]);
    free(o);
    free(etext);
    free(name);
    free(rep.p);
    return rc;
}

int corpus_run(const struct corpus_pass *p, FILE *log)
{
    struct amiga_emulation **e = calloc(p->nconfigs, sizeof *e);
    enum amiga_emulation_status *st = calloc(p->nconfigs, sizeof *st);
    struct responder **resp = calloc(p->nconfigs, sizeof *resp);
    int *nsessions = calloc(p->nconfigs, sizeof *nsessions);
    char **slog = calloc(p->nconfigs, sizeof *slog);
    struct statline *stl = calloc(p->nconfigs, sizeof *stl);   /* test[0]: joined */
    int *vbad = calloc(p->nconfigs, sizeof *vbad);             /* the verdict "differs" */
    char *text = corpus_script(p), err[300];
    struct text rep = { 0 };
    size_t c, i, next = 0, done = 0, announced = 0;   /* announced: sessions logged */
    unsigned waiting_port = 0;  /* the port of resp[next] */
    long bad = 0;
    int rc = 2;

    if (!e || !st || !resp || !nsessions || !slog || !stl || !vbad || !text)
        goto out;
    /* without the files of the pass (the CD's libraries...) or the emulator:
       skipped */
    for (c = 0; c < p->nconfigs; c++) {
        const char *f[] = { p->configs[c].blitz2, p->configs[c].deflibs, p->configs[c].acidlibs,
                            p->ted, p->launcher, p->defaultdbug, p->gate,
                            kick1x(p) ? p->rcput : NULL };

        for (i = 0; i < sizeof f / sizeof f[0]; i++)
            if (f[i] && access(f[i], R_OK)) {
                fprintf(log, "corpus: no %s - skipped\n", f[i]);
                rc = 77;
                goto out;
            }
    }
    for (i = 0; i <= p->nfiles + 1; i++) {
        const char *f = i < p->nfiles ? p->file_src[i] : i == p->nfiles ? p->wrap : p->runwrap;

        if (f && access(f, R_OK)) {
            fprintf(log, "corpus: no %s - skipped\n", f);
            rc = 77;
            goto out;
        }
    }
    for (i = 0; i < p->nmounts; i++)
        if (access(p->mount_dir[i], R_OK | X_OK)) {
            fprintf(log, "corpus: no %s for %s: - skipped\n", p->mount_dir[i], p->mount_name[i]);
            rc = 77;
            goto out;
        }
    for (i = 0; i < p->nsources; i++)
        if (access(p->sources[i], R_OK)) {
            fprintf(log, "corpus: no %s\n", p->sources[i]);
            goto out;
        }
    for (c = 0; c < p->nconfigs; c++)
        if (stage(p, c, log))
            goto out;
    /* at most `parallel` sessions at a time, started in order, each
       collected as soon as it has ended; with serial=on each one with its
       own port and remote end.  No free emulator slot while sessions of
       this pass run: the start is tried again every
       AMIGA_EMULATION_SLOT_RETRY_MS, any slot that becomes free is taken
       (the slot limit is hard; the remote end of the session not started
       stays for the next try) */
    while (done < p->nconfigs) {
        int ended;

        while (next < p->nconfigs && next - done < p->parallel) {
            unsigned port = 0;

            if (resp[next])
                port = waiting_port;
            else if (p->serial
                     && (!(port = serial_free_port()) || !(resp[next] = serial_start(port)))) {
                fprintf(log, "corpus: %s: no TCP port for the serial port\n",
                        p->configs[next].tag);
                goto out;
            }
            waiting_port = port;
            if (next >= announced) {
                fprintf(log, "corpus: %s: emulator session%s\n", p->configs[next].tag,
                        port ? ", serial port" : "");
                fflush(log);
            }
            if (!(e[next] = start(p, next, text, port, err, sizeof err)) && next > done
                && amiga_emulation_no_slot()) {
                if (next >= announced)
                    fprintf(log, "corpus: %s: %s - waiting for a slot\n", p->configs[next].tag,
                            err);
                announced = next + 1;
                break;
            }
            announced = next + 1;
            if (!e[next]) {
                fprintf(log, "corpus: %s: %s - skipped\n", p->configs[next].tag, err);
                rc = next ? 2 : 77;
                goto out;
            }
            next++;
        }
        /* the session that has ended; none within the time: start again */
        if ((ended = amiga_emulation_wait_any(e, p->nconfigs, AMIGA_EMULATION_SLOT_RETRY_MS)) < 0)
            continue;
        c = (size_t)ended;
        st[c] = amiga_emulation_join(e[c]);
        statline_session(&stl[c], p->work, p->configs[c].tag, e[c]);
        amiga_emulation_free(e[c]);
        e[c] = NULL;
        if (resp[c]) {
            char *out = dir_of(p, c, "vol/out");

            nsessions[c] = out ? serial_stop(resp[c], out) : -1;
            resp[c] = NULL;
            free(out);
        }
        done++;
    }

    for (c = 0; c < p->nconfigs; c++) {
        char *path = str_printf("%s/%s/vol/out/session.log", p->work, p->configs[c].tag);
        size_t len;

        slog[c] = path ? (char *)read_file(path, &len) : NULL;
        free(path);
    }
    if (p->expect) {
        rc = judge(p, st, slog, stl, log);
        goto out;
    }
    text_add(&rep, "== runtime error checking %s, %zu sources:", p->runtime_errors ? "on" : "off",
        p->nsources);
    for (c = 0; c < p->nconfigs; c++)
        text_add(&rep, " %s%s", p->configs[c].tag, c ? "" : " (reference)");
    text_add(&rep, "\n");
    if (p->serial)
        for (c = 0; c < p->nconfigs; c++) {
            text_add(&rep, "serial port %s: %d sessions\n", p->configs[c].tag, nsessions[c]);
            if (nsessions[c] < 0) {
                bad++;
                vbad[c] = 1;
            }
        }
    for (c = 0; c < p->nconfigs; c++)
        if (st[c] != AMIGA_EMULATION_DONE) {
            text_add(&rep, "session %s: %s (%s/%s/emu)\n", p->configs[c].tag,
                amiga_emulation_status_name(st[c]), p->work, p->configs[c].tag);
            bad++;
            vbad[c] = 1;
        }
    if (p->gate)
        for (c = 0; c < p->nconfigs; c++)
            if (gate_rc(p, c) != 0) {
                text_add(&rep, "gate %s: rc %d - the programs did not run (%s/%s/vol/out/gate.run)\n",
                    p->configs[c].tag, gate_rc(p, c), p->work, p->configs[c].tag);
                bad++;
                vbad[c] = 1;
            }
    for (i = 0; i < p->nsources; i++) {
        struct outcome ref, o;
        char *name = corpus_name(p->sources[i]);
        int diff = 0;

        load_outcome(p, 0, i, slog[0], &ref);
        text_add(&rep, "%-16s", name ? name : "?");
        /* compile only: the reference's result, and its executable (if any) */
        if (!p->run) {
            int noresult = !strncmp(ref.result, "no RESULT", 9);

            if (noresult) {
                text_add(&rep, " reference: %s", ref.result);
                diff = 1;
                vbad[0] = 1;
            } else
                text_add(&rep, " %s", ref.exe ? "compiles" : "does not compile");
            for (c = 1; c < p->nconfigs; c++) {
                const struct corpus_config *cf = &p->configs[c];
                int ok;

                load_outcome(p, c, i, slog[c], &o);
                ok = !strcmp(ref.result, o.result);
                text_add(&rep, "; %s:%s", cf->tag, ok ? " result same" : " RESULT DIFFERS (");
                if (!ok)
                    text_add(&rep, "%s / %s)", ref.result, o.result);
                if (cf->expect == CORPUS_EXE && (ref.exe || o.exe)) {
                    int same_exe = same(ref.exe, ref.exelen, o.exe, o.exelen);

                    text_add(&rep, same_exe ? ", exe identical" : o.exe && ref.exe ? ", EXE DIFFERS"
                                   : o.exe ? ", AN EXE" : ", NO EXE");
                    ok &= same_exe;
                }
                diff |= !ok;
                vbad[c] |= !ok;
                free_outcome(&o);
            }
            text_add(&rep, "\n");
            bad += diff;
            free_outcome(&ref);
            free(name);
            continue;
        }
        if (!ref.exe) {
            text_add(&rep, " reference did not compile: %s\n", ref.result);
            bad++;
            vbad[0] = 1;
            free_outcome(&ref);
            free(name);
            continue;
        }
        for (c = 1; c < p->nconfigs; c++) {
            const struct corpus_config *cf = &p->configs[c];

            load_outcome(p, c, i, slog[c], &o);
            text_add(&rep, "%s %s:", c > 1 ? ";" : "", cf->tag);
            if (cf->expect == CORPUS_EXE) {
                int ok = same(ref.exe, ref.exelen, o.exe, o.exelen);

                text_add(&rep, ok ? " exe identical," : o.exe ? " EXE DIFFERS," : " NO EXE (%s),",
                    o.result);
                diff |= !ok;
                vbad[c] |= !ok;
            }
            if (!o.run)
                text_add(&rep, " NO OUTPUT");
            else if (same(ref.run, ref.runlen, o.run, o.runlen))
                text_add(&rep, " output same");
            else
                text_add(&rep, " OUTPUT DIFFERS");
            diff |= !same(ref.run, ref.runlen, o.run, o.runlen);
            vbad[c] |= !same(ref.run, ref.runlen, o.run, o.runlen);
            if (p->serial) {
                int ok = same(ref.serial, ref.seriallen, o.serial, o.seriallen);

                text_add(&rep, !ref.serial ? ", NO REFERENCE SERIAL DATA" : ok ? ", serial data same"
                               : ", SERIAL DATA DIFFERS");
                diff |= !ok;
                vbad[c] |= !ok;
                if (o.run && strstr((const char *)o.run, "TIMEOUT")) {
                    text_add(&rep, ", TIMEOUT");
                    diff = 1;
                    vbad[c] = 1;
                }
            }
            free_outcome(&o);
        }
        if (p->serial && ref.run && strstr((const char *)ref.run, "TIMEOUT")) {
            text_add(&rep, "; reference: TIMEOUT");
            diff = 1;
            vbad[0] = 1;
        }
        /* the reference's serial data also as in the other pass (runtime error
           checking off/on); not there: that pass has not run */
        if (p->serial && p->serial_same_as && name) {
            char *other = str_printf("%s/%s.serial", p->serial_same_as, name);
            size_t olen;
            uint8_t *od = other ? read_file(other, &olen) : NULL;

            if (!od)
                text_add(&rep, "; (no serial data of the other pass)");
            else if (same(ref.serial, ref.seriallen, od, olen))
                text_add(&rep, "; serial data as in the other pass");
            else {
                text_add(&rep, "; SERIAL DATA DIFFERS FROM THE OTHER PASS");
                diff = 1;
                vbad[0] = 1;
            }
            free(od);
            free(other);
        }
        text_add(&rep, "\n");
        bad += diff;
        free_outcome(&ref);
        free(name);
    }
    text_add(&rep, "%zu checks, %ld with differences\n", p->nsources, bad);
    if (rep.bad || write_file(p->report, rep.p, rep.n)) {
        fprintf(log, "corpus: cannot write %s\n", p->report);
        goto out;
    }
    fwrite(rep.p, 1, rep.n, log);
    rc = bad ? 1 : 0;
    for (c = 0; c < p->nconfigs; c++)
        statline_set(stl[c].verdict, sizeof stl[c].verdict, vbad[c] ? "differs" : "same");
out:
    /* the sessions' @stat lines (statline.h), after the report */
    for (c = 0; stl && c < p->nconfigs; c++)
        if (stl[c].test[0])
            statline_write(log, &stl[c]);
    for (c = 0; e && c < p->nconfigs; c++)
        if (e[c]) {
            amiga_emulation_join(e[c]);
            amiga_emulation_free(e[c]);
        }
    for (c = 0; resp && c < p->nconfigs; c++)
        if (resp[c]) {
            char *out = dir_of(p, c, "vol/out");

            serial_stop(resp[c], out ? out : ".");
            free(out);
        }
    for (c = 0; slog && c < p->nconfigs; c++)
        free(slog[c]);
    free(slog);
    free(stl);
    free(vbad);
    free(resp);
    free(nsessions);
    free(e);
    free(st);
    free(text);
    free(rep.p);
    return rc;
}
