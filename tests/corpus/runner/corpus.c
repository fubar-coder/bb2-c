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
#include "corpus.h"

#define VOLUME "CORPUS"                 /* the configuration's directory */

/* ------------------------------------------------------------ strings */

static char *str_printf(const char *fmt, ...)
{
    va_list ap;
    char *s;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0 || !(s = malloc((size_t)n + 1)))
        return NULL;
    va_start(ap, fmt);
    vsnprintf(s, (size_t)n + 1, fmt, ap);
    va_end(ap);
    return s;
}

/* growing text */
struct text {
    char *p;
    size_t n, cap;
    int bad;
};

static void add(struct text *t, const char *fmt, ...)
{
    va_list ap;
    int n;

    if (t->bad)
        return;
    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0 || (t->n + (size_t)n + 1 > t->cap
                  && !(t->p = realloc(t->p, t->cap = (t->n + (size_t)n + 1) * 2)))) {
        t->bad = 1;
        return;
    }
    va_start(ap, fmt);
    vsnprintf(t->p + t->n, (size_t)n + 1, fmt, ap);
    va_end(ap);
    t->n += (size_t)n;
}

/* ------------------------------------------------------- the pass file */

void corpus_free(struct corpus_pass *p)
{
    size_t i;

    free(p->work);
    free(p->report);
    free(p->ted);
    free(p->runner);
    free(p->gate);
    free(p->defaultdbug);
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

int corpus_parse(const char *text, struct corpus_pass *p, char *err, size_t errlen)
{
    const char *line = text;
    unsigned no = 0;
    size_t i, k;

    memset(p, 0, sizeof *p);
    p->timeout = 300;
    p->parallel = 1;
    p->stack = 40000;
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
            || !strcmp(key, "runner") || !strcmp(key, "gate") || !strcmp(key, "defaultdbug")) {
            char **dst = !strcmp(key, "work") ? &p->work : !strcmp(key, "report") ? &p->report
                         : !strcmp(key, "ted") ? &p->ted : !strcmp(key, "runner") ? &p->runner
                         : !strcmp(key, "gate") ? &p->gate : &p->defaultdbug;

            free(*dst);
            *dst = val;
            continue;
        }
        if (!strcmp(key, "runerr") || !strcmp(key, "timeout") || !strcmp(key, "parallel")
            || !strcmp(key, "stack")) {
            char *e;
            unsigned long u = strtoul(val, &e, 10);

            if (!strcmp(key, "runerr")) {
                if (strcmp(val, "on") && strcmp(val, "off")) {
                    free(val);
                    return fail(p, err, errlen, "line %u: runerr=on or runerr=off", no);
                }
                p->runerr = !strcmp(val, "on");
            } else if (!*val || *e || !u) {
                free(val);
                return fail(p, err, errlen, "line %u: %s needs a number > 0", no, key);
            } else if (!strcmp(key, "timeout"))
                p->timeout = (unsigned)u;
            else if (!strcmp(key, "parallel"))
                p->parallel = (unsigned)u;
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
                || (c.expect == CORPUS_REF) != (p->nconfigs == 0)
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
        free(val);
        return fail(p, err, errlen, "line %u: unknown key %s", no, key);
    }
    if (!p->work || !p->report || !p->ted || !p->runner || !p->defaultdbug)
        return fail(p, err, errlen, "work, report, ted, runner and defaultdbug are needed");
    if (p->nconfigs < 2 || !p->nsources)
        return fail(p, err, errlen, "a reference, another configuration and sources are needed");
    /* names: one directory per configuration, AmigaDOS ignores case */
    for (i = 0; i < p->nconfigs; i++)
        for (k = 0; k < i; k++)
            if (!strcasecmp(p->configs[i].tag, p->configs[k].tag))
                return fail(p, err, errlen, "configuration %s twice", p->configs[i].tag);
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

static int rm_entry(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    (void)st;
    (void)ftw;
    return type == FTW_DP ? rmdir(path) : unlink(path);
}

static void rm_tree(const char *dir)
{
    nftw(dir, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
}

static int mkdir_p(const char *path)
{
    char *p = strdup(path), *s;
    int r = 0;

    if (!p)
        return -1;
    for (s = p + 1; *s && !r; s++)
        if (*s == '/') {
            *s = 0;
            r = mkdir(p, 0755) && errno != EEXIST;
            *s = '/';
        }
    if (!r)
        r = mkdir(p, 0755) && errno != EEXIST;
    free(p);
    return r ? -1 : 0;
}

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
        s[n] = 0;
        *len = (size_t)n;
    } else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

static int write_file(const char *path, const void *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    int ok = f && fwrite(data, 1, len, f) == len;

    if (f && fclose(f))
        ok = 0;
    return ok ? 0 : -1;
}

static int copy_file(const char *from, const char *to)
{
    size_t len;
    uint8_t *d = read_file(from, &len);
    int r = d ? write_file(to, d, len) : -1;

    free(d);
    return r;
}

/* ------------------------------------------------------------- staging */

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
        { p->defaultdbug, "blitz2/defaultdbug" }, { p->runner, "run" },
        { p->gate, "gate" },
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
    add(&jobs, "log=" VOLUME ":out/session.log\ndefaults=" VOLUME ":out/defaults.xtra\n");
    if (!p->runerr)
        add(&jobs, "runerr=off\n");
    for (i = 0; i < p->nsources; i++) {
        char *name = corpus_name(p->sources[i]), *to = str_printf("%s/src/%s.bb2", vol, name);

        if (!name || !to || copy_file(p->sources[i], to)) {
            fprintf(log, "corpus: cannot copy %s\n", p->sources[i]);
            free(name);
            free(to);
            goto out;
        }
        add(&jobs, "job=j%03zu|" VOLUME ":src/%s.bb2|" VOLUME ":out/%s\n", i, name, name);
        free(name);
        free(to);
    }
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

static char *script(const struct corpus_pass *p)
{
    struct text s = { 0 };
    size_t i;

    add(&s, "Assign blitz2: " VOLUME ":blitz2\n"
            "Stack %u\n"
            "blitz2:blitz2 " VOLUME ":jobs >" VOLUME ":out/blitz2.out\n"
            "Echo \"rc=$RC\" >>" VOLUME ":out/blitz2.out\n"
            "Assign blitz2:\n", p->stack);
    if (p->gate)
        add(&s, "SetEnv REQCLOSELOG " VOLUME ":out/reqclose.log\n"
                VOLUME ":run " VOLUME ":gate >" VOLUME ":out/gate.run\n"
                "Set gate $RC\n"
                "Echo \"rc=$gate\" >>" VOLUME ":out/gate.run\n"
                "If VAL $gate EQ 0\n");
    for (i = 0; i < p->nsources; i++) {
        char *n = corpus_name(p->sources[i]);

        add(&s, "If EXISTS " VOLUME ":out/%s\n"
                "  " VOLUME ":run " VOLUME ":out/%s >" VOLUME ":out/%s.run\n"
                "  Echo \"rc=$RC\" >>" VOLUME ":out/%s.run\n"
                "EndIf\n", n, n, n, n);
        free(n);
    }
    if (p->gate)
        add(&s, "EndIf\nUnSetEnv REQCLOSELOG\n");
    if (s.bad) {
        free(s.p);
        return NULL;
    }
    return s.p;
}

/* ------------------------------------------------------------ running */

static struct amiga_emulation *start(const struct corpus_pass *p, size_t c, const char *text,
                                     char *err, size_t errlen)
{
    char *vol = dir_of(p, c, "vol"), *emu = dir_of(p, c, "emu");
    struct amiga_emulation_mount *m = calloc(p->nmounts + 1, sizeof *m);
    struct amiga_emulation_params ep = { 0 };
    struct amiga_emulation *e = NULL;
    size_t i;

    if (vol && emu && m) {
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
    uint8_t *exe, *run;
    size_t exelen, runlen;
    char *result;
};

static void load_outcome(const struct corpus_pass *p, size_t c, size_t i, const char *log,
                         struct outcome *o)
{
    char *n = corpus_name(p->sources[i]);
    char *exe = str_printf("%s/%s/vol/out/%s", p->work, p->configs[c].tag, n);
    char *run = str_printf("%s/%s/vol/out/%s.run", p->work, p->configs[c].tag, n);

    memset(o, 0, sizeof *o);
    if (exe)
        o->exe = read_file(exe, &o->exelen);
    if (run)
        o->run = read_file(run, &o->runlen);
    if (o->exe && p->runerr)
        corpus_mask_traps(o->exe, o->exelen);
    o->result = result_of(log, i);
    free(n);
    free(exe);
    free(run);
}

static void free_outcome(struct outcome *o)
{
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

int corpus_run(const struct corpus_pass *p, FILE *log)
{
    struct amiga_emulation **e = calloc(p->nconfigs, sizeof *e);
    enum amiga_emulation_status *st = calloc(p->nconfigs, sizeof *st);
    char **slog = calloc(p->nconfigs, sizeof *slog);
    char *text = script(p), err[300];
    struct text rep = { 0 };
    size_t c, i, next = 0, done = 0;
    long bad = 0;
    int rc = 2;

    if (!e || !st || !slog || !text)
        goto out;
    /* without the files of the pass (the CD's libraries...) or the emulator:
       skipped */
    for (c = 0; c < p->nconfigs; c++) {
        const char *f[] = { p->configs[c].blitz2, p->configs[c].deflibs, p->configs[c].acidlibs,
                            p->ted, p->runner, p->defaultdbug, p->gate };

        for (i = 0; i < sizeof f / sizeof f[0]; i++)
            if (f[i] && access(f[i], R_OK)) {
                fprintf(log, "corpus: no %s - skipped\n", f[i]);
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
    /* at most `parallel` sessions at a time, joined in start order */
    while (done < p->nconfigs) {
        while (next < p->nconfigs && next - done < p->parallel) {
            fprintf(log, "corpus: %s: emulator session\n", p->configs[next].tag);
            fflush(log);
            if (!(e[next] = start(p, next, text, err, sizeof err))) {
                fprintf(log, "corpus: %s: %s - skipped\n", p->configs[next].tag, err);
                rc = next ? 2 : 77;
                goto out;
            }
            next++;
        }
        st[done] = amiga_emulation_join(e[done]);
        amiga_emulation_free(e[done]);
        e[done] = NULL;
        done++;
    }

    for (c = 0; c < p->nconfigs; c++) {
        char *path = str_printf("%s/%s/vol/out/session.log", p->work, p->configs[c].tag);
        size_t len;

        slog[c] = path ? (char *)read_file(path, &len) : NULL;
        free(path);
    }
    add(&rep, "== runtime error checking %s, %zu sources:", p->runerr ? "on" : "off",
        p->nsources);
    for (c = 0; c < p->nconfigs; c++)
        add(&rep, " %s%s", p->configs[c].tag, c ? "" : " (reference)");
    add(&rep, "\n");
    for (c = 0; c < p->nconfigs; c++)
        if (st[c] != AMIGA_EMULATION_DONE) {
            add(&rep, "session %s: %s (%s/%s/emu)\n", p->configs[c].tag,
                amiga_emulation_status_name(st[c]), p->work, p->configs[c].tag);
            bad++;
        }
    if (p->gate)
        for (c = 0; c < p->nconfigs; c++)
            if (gate_rc(p, c) != 0) {
                add(&rep, "gate %s: rc %d - the programs did not run (%s/%s/vol/out/gate.run)\n",
                    p->configs[c].tag, gate_rc(p, c), p->work, p->configs[c].tag);
                bad++;
            }
    for (i = 0; i < p->nsources; i++) {
        struct outcome ref, o;
        char *name = corpus_name(p->sources[i]);
        int diff = 0;

        load_outcome(p, 0, i, slog[0], &ref);
        add(&rep, "%-16s", name ? name : "?");
        if (!ref.exe) {
            add(&rep, " reference did not compile: %s\n", ref.result);
            bad++;
            free_outcome(&ref);
            free(name);
            continue;
        }
        for (c = 1; c < p->nconfigs; c++) {
            const struct corpus_config *cf = &p->configs[c];

            load_outcome(p, c, i, slog[c], &o);
            add(&rep, "%s %s:", c > 1 ? ";" : "", cf->tag);
            if (cf->expect == CORPUS_EXE) {
                int ok = same(ref.exe, ref.exelen, o.exe, o.exelen);

                add(&rep, ok ? " exe identical," : o.exe ? " EXE DIFFERS," : " NO EXE (%s),",
                    o.result);
                diff |= !ok;
            }
            if (!o.run)
                add(&rep, " NO OUTPUT");
            else if (same(ref.run, ref.runlen, o.run, o.runlen))
                add(&rep, " output same");
            else
                add(&rep, " OUTPUT DIFFERS");
            diff |= !same(ref.run, ref.runlen, o.run, o.runlen);
            free_outcome(&o);
        }
        add(&rep, "\n");
        bad += diff;
        free_outcome(&ref);
        free(name);
    }
    add(&rep, "%zu checks, %ld with differences\n", p->nsources, bad);
    if (rep.bad || write_file(p->report, rep.p, rep.n)) {
        fprintf(log, "corpus: cannot write %s\n", p->report);
        goto out;
    }
    fwrite(rep.p, 1, rep.n, log);
    rc = bad ? 1 : 0;
out:
    for (c = 0; e && c < p->nconfigs; c++)
        if (e[c]) {
            amiga_emulation_join(e[c]);
            amiga_emulation_free(e[c]);
        }
    for (c = 0; slog && c < p->nconfigs; c++)
        free(slog[c]);
    free(slog);
    free(e);
    free(st);
    free(text);
    free(rep.p);
    return rc;
}
