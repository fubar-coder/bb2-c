/*
 * cdrun - every .bb2/.bb of the CD compiled by one compiler in Amiberry
 * (bb2 compilerlauf/tools/bb2corpus.py + refcompare.py): a table of the
 * results, optionally compared with a reference table.
 *
 *   cdrun PASSFILE
 *
 * The pass file, lines KEY=VALUE, '#' comments:
 *   work=DIR           work directory (emptied first)
 *   output=PATH        the table to write
 *   reference=PATH     optional: compare with this table (report=PATH)
 *   report=PATH        the comparison (also to stdout)
 *   cd=DIR             the CD (archive/cd/ultimate-blitz-basic-2.1)
 *   tokens=PATH        the token list (tests/data/ted-tokens.txt), for
 *                      finding the INCLUDEs of a source
 *   blitz2=, deflibs=, acidlibs=, defaultdbug=, ted=PATH   the blitz2:
 *                      directory (ted = tedstub)
 *   assign=NAME|DIR    an assign to a directory of the CD (DIR relative,
 *                      "." = the CD); a name again is added (Assign ADD);
 *                      blitz2: is the compiler directory, then these
 *   timeout=SEC        per emulator session (it gets all remaining jobs of
 *                      its worker; the time limit ends it, the next session
 *                      goes on)
 *   parallel=N         workers (emulator sessions at the same time)
 *   stack=N            Stack for blitz2
 *   limit=N            only the first N tokenised sources (trial run)
 *   source=REL|OUT     only these sources (REL as below cd=, any number of
 *                      lines): the executable of each one that compiles is
 *                      copied to OUT (OUTs are removed first); a REL that
 *                      is not there is an error, one that does not compile
 *                      gives exit status 1 (the reference build of the RWE
 *                      sources, libs/userlibs/rwe/refbuild.cmake)
 *   cache=PATH         optional: after a run with exit status 0, a record
 *                      of the inputs and outputs to PATH (cdcache.h); the
 *                      next run with the same inputs (the pass file, this
 *                      cdrun - with amiga_emulation and its templates -,
 *                      blitz2=, deflibs=, acidlibs=, defaultdbug=, ted=,
 *                      tokens=, reference=, every file below cd=) whose
 *                      outputs (output=, report=, the OUTs of source=)
 *                      are all there unchanged ends at once with 0,
 *                      without the emulator.  CDRUN_FORCE=1 in the
 *                      environment runs anyway.  Not covered: the ROM and
 *                      the hardfiles, Amiberry itself.
 *
 * Sources: sorted by path; text sources (no 0 byte) are not compiled
 * ("skipped").  A job runs with the source's .xtra (next to it) or the
 * compiler's start options, in the source's directory - or, if a relative
 * INCLUDE/XINCLUDE/INCBIN is not there, in the nearest parent directory
 * on the CD that has it.  A job that started but has no result is tried
 * once more as the first job of the next session, then it is "crash"; two
 * sessions in a row without any started job: the rest is "nostart".
 *
 * Table (TSV, Latin-1): path result line message size sha256_masked xtra
 * note; result ok | error | failed | crash | nostart | skipped; the hash
 * with the trap #1 / #4 parameter blocks zeroed.  Compared: result, line,
 * message, size, sha256_masked.
 *
 * Exit status 0 = written (and no difference), 1 = differences or a
 * source= that does not compile, 77 = the emulator or a file is missing
 * (skipped), 2 = error.
 */
#define _XOPEN_SOURCE 700       /* nftw, strdup, strndup */
#include <ctype.h>
#include <dirent.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "amiga_emulation.h"
#include "cdcache.h"
#include "corpus.h"
#include "sha256.h"
#include "statline_emulation.h"
#include "util.h"

#define VOLUME "CORPUS"         /* the worker's directory */
#define CDVOL  "UBB21"          /* the CD, read-only */
#define MAXSESSIONS 400

struct pass {
    char *work, *output, *reference, *report, *cd, *tokens;
    char *blitz2, *deflibs, *acidlibs, *defaultdbug, *ted, *cache;
    char **assign_name, **assign_dir;
    size_t nassigns;
    char **source_rel, **source_out;    /* source=REL|OUT */
    size_t nsources;
    unsigned timeout, parallel, stack, limit;
};

struct job {
    char *rel;                  /* path on the CD */
    char id[24];                /* s0001 ... (index in the sorted list) */
    int tok;                    /* tokenised (else a text source: skipped) */
    int compile;                /* tokenised and within the limit */
    char *dir;                  /* job directory (CD relative) or NULL */
    char **result;              /* the RESULT fields behind the id, NULL */
    size_t nresult;
    char *xtra, *note;
    int attempts, done;
    unsigned worker;
};

static struct pass P;
static struct job *jobs;
static size_t njobs;

/* ----------------------------------------------------------- pass file */

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

static int parse(const char *text, char *err, size_t errlen)
{
    const char *line = text;
    unsigned no = 0;

    P.timeout = 120;
    P.parallel = 1;
    P.stack = 100000;
    while (*line) {
        const char *eol = strchr(line, '\n'), *v;
        char key[32], *val, **dst = NULL;
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
        if (!(v = memchr(line, '=', n)) || (size_t)(v - line) >= sizeof key) {
            snprintf(err, errlen, "line %u: expected KEY=VALUE", no);
            return -1;
        }
        memcpy(key, line, (size_t)(v - line));
        key[v - line] = 0;
        v++;
        val = strndup(v, n - (size_t)(v - line));
        line = *eol ? eol + 1 : eol;
        if (!val)
            return -1;
        if (!strcmp(key, "work")) dst = &P.work;
        else if (!strcmp(key, "output")) dst = &P.output;
        else if (!strcmp(key, "reference")) dst = &P.reference;
        else if (!strcmp(key, "report")) dst = &P.report;
        else if (!strcmp(key, "cd")) dst = &P.cd;
        else if (!strcmp(key, "tokens")) dst = &P.tokens;
        else if (!strcmp(key, "blitz2")) dst = &P.blitz2;
        else if (!strcmp(key, "deflibs")) dst = &P.deflibs;
        else if (!strcmp(key, "acidlibs")) dst = &P.acidlibs;
        else if (!strcmp(key, "defaultdbug")) dst = &P.defaultdbug;
        else if (!strcmp(key, "ted")) dst = &P.ted;
        else if (!strcmp(key, "cache")) dst = &P.cache;
        if (dst) {
            free(*dst);
            *dst = val;
            continue;
        }
        if (!strcmp(key, "timeout") || !strcmp(key, "parallel") || !strcmp(key, "stack")
            || !strcmp(key, "limit")) {
            char *e;
            unsigned long u = strtoul(val, &e, 10);

            if (!*val || *e || !u) {
                snprintf(err, errlen, "line %u: %s needs a number > 0", no, key);
                free(val);
                return -1;
            }
            *(!strcmp(key, "timeout") ? &P.timeout : !strcmp(key, "parallel") ? &P.parallel
              : !strcmp(key, "stack") ? &P.stack : &P.limit) = (unsigned)u;
            free(val);
            continue;
        }
        if (!strcmp(key, "assign")) {
            const char *s = val;
            char *name = field(&s), *dir = field(&s);

            P.assign_name = realloc(P.assign_name, (P.nassigns + 1) * sizeof(char *));
            P.assign_dir = realloc(P.assign_dir, (P.nassigns + 1) * sizeof(char *));
            if (!name || !*name || !dir || !*dir || *s || !P.assign_name || !P.assign_dir) {
                snprintf(err, errlen, "line %u: assign=NAME|DIR", no);
                return -1;
            }
            P.assign_name[P.nassigns] = name;
            P.assign_dir[P.nassigns++] = dir;
            free(val);
            continue;
        }
        if (!strcmp(key, "source")) {
            const char *s = val;
            char *rel = field(&s), *out = field(&s);

            P.source_rel = realloc(P.source_rel, (P.nsources + 1) * sizeof(char *));
            P.source_out = realloc(P.source_out, (P.nsources + 1) * sizeof(char *));
            if (!rel || !*rel || !out || !*out || *s || !P.source_rel || !P.source_out) {
                snprintf(err, errlen, "line %u: source=REL|OUT", no);
                free(rel);
                free(out);
                free(val);
                return -1;
            }
            P.source_rel[P.nsources] = rel;
            P.source_out[P.nsources++] = out;
            free(val);
            continue;
        }
        snprintf(err, errlen, "line %u: unknown key %s", no, key);
        free(val);
        return -1;
    }
    if (!P.work || !P.output || !P.cd || !P.tokens || !P.blitz2 || !P.deflibs || !P.acidlibs
        || !P.defaultdbug || !P.ted || (P.reference && !P.report)) {
        snprintf(err, errlen, "work, output, cd, tokens, blitz2, deflibs, acidlibs, "
                              "defaultdbug, ted are needed (report with reference)");
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------ sources */

static size_t cdlen;

static int collect(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    size_t n = strlen(path);

    (void)st;
    (void)ftw;
    if (type == FTW_F && ((n > 4 && !strcasecmp(path + n - 4, ".bb2"))
                          || (n > 3 && !strcasecmp(path + n - 3, ".bb")))) {
        struct job *j = realloc(jobs, (njobs + 1) * sizeof *j);

        if (!j)
            return 1;
        jobs = j;
        memset(&jobs[njobs], 0, sizeof *jobs);
        if (!(jobs[njobs++].rel = strdup(path + cdlen + 1)))
            return 1;
    }
    return 0;
}

static int by_rel(const void *a, const void *b)
{
    return strcmp(((const struct job *)a)->rel, ((const struct job *)b)->rel);
}

/* token word (without bit 15) -> name, from "XXXX name" lines; the first
   name of a word wins */
static char *names[0x8000];

static int load_tokens(void)
{
    size_t len;
    char *t = (char *)read_file(P.tokens, &len), *l, *save = NULL;

    if (!t)
        return -1;
    for (l = strtok_r(t, "\n", &save); l; l = strtok_r(NULL, "\n", &save)) {
        unsigned w = (unsigned)strtoul(l, NULL, 16) & 0x7FFF;

        if (strlen(l) > 5 && !names[w])
            names[w] = strdup(l + 5);
    }
    free(t);
    return 0;
}

/* the detok text of a tokenised source (bb2tok detokenise: unknown
   tokens as <$XXXX>, lines with LF) */
static char *detok(const uint8_t *d, size_t len)
{
    struct text t = { 0 };
    size_t i;

    for (i = 0; i < len; i++) {
        if (!d[i])
            text_add(&t, "\n");
        else if (d[i] & 0x80 && i + 1 < len) {
            unsigned w = (unsigned)(d[i] << 8 | d[i + 1]);

            if (names[w & 0x7FFF])
                text_add(&t, "%s", names[w & 0x7FFF]);
            else
                text_add(&t, "<$%04X>", w);
            i++;
        } else
            text_add(&t, "%c", d[i]);
    }
    if (t.bad) {
        free(t.p);
        return NULL;
    }
    return t.p ? t.p : strdup("");
}

/* rel (with '/', '..') exists below base, names compared without case */
static int iexists(const char *base, const char *rel)
{
    char *path = strdup(base), *r = strdup(rel), *part, *save = NULL;
    int ok = path && r;

    for (part = ok ? strtok_r(r, "/", &save) : NULL; part && ok;
         part = strtok_r(NULL, "/", &save)) {
        if (!strcmp(part, "..")) {
            char *s = strrchr(path, '/');

            if (s && s != path)
                *s = 0;
            continue;
        }
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
        ok = path != NULL;
    }
    free(path);
    free(r);
    return ok;
}

static int include_word(const char *p, size_t *n)
{
    if (!strncasecmp(p, "XINCLUDE", 8))
        *n = 8;
    else if (!strncasecmp(p, "INCLUDE", 7) || !strncasecmp(p, "INCBIN", 6))
        *n = p[3] == 'L' || p[3] == 'l' ? 7 : 6;
    else
        return 0;
    return 1;
}

/* the job directory (CD relative, malloc'd) or NULL = the source's own
   (bb2corpus include_dir: the first relative include that is not next to
   the source decides; the nearest parent on the CD that has it) */
static char *include_dir(const struct job *j, const char *text)
{
    char *srcdir = str_printf("%s/%s", P.cd, j->rel), *found = NULL;
    const char *p;

    if (!srcdir)
        return NULL;
    *strrchr(srcdir, '/') = 0;
    /* (?:^|[\s:])(?:X?INCLUDE|INCBIN)\s*"?([^";\s,]+), no case, matches
       one after the other */
    for (p = text; *p && !found; p++) {
        const char *q;
        size_t n, k;
        char inc[256];

        if (p != text && !isspace((unsigned char)p[-1]) && p[-1] != ':')
            continue;
        if (!include_word(p, &n))
            continue;
        q = p + n;
        while (isspace((unsigned char)*q))
            q++;
        if (*q == '"')
            q++;
        for (k = 0; q[k] && !strchr("\";, \t\n\r\f\v", q[k]) && k + 1 < sizeof inc; k++)
            inc[k] = q[k];
        inc[k] = 0;
        if (!k)
            continue;                   /* no match here: on at p + 1 */
        p = q + k - 1;                  /* the next match starts behind it */
        if (strchr(inc, ':') || iexists(srcdir, inc))
            continue;
        {
            char *d = strdup(srcdir);

            while (d && strlen(d) > strlen(P.cd)) {
                *strrchr(d, '/') = 0;
                if (iexists(d, inc)) {
                    found = strdup(strlen(d) > strlen(P.cd) ? d + strlen(P.cd) + 1 : ".");
                    break;
                }
            }
            free(d);
        }
    }
    free(srcdir);
    return found;
}

/* ------------------------------------------------------------ sessions */

static char *assign_path(const char *rel)
{
    return strcmp(rel, ".") ? str_printf(CDVOL ":%s", rel) : strdup(CDVOL ":");
}

static char *worker_dir(unsigned w, const char *sub)
{
    return str_printf("%s/w%u/%s", P.work, w, sub);
}

static int stage_worker(unsigned w, FILE *log)
{
    const char *from[] = { P.blitz2, P.ted, P.deflibs, P.acidlibs, P.defaultdbug };
    const char *to[] = { "blitz2", "ted", "deflibs", "acidlibs", "defaultdbug" };
    char *d = worker_dir(w, "vol/blitz2/userlibs"), *o = worker_dir(w, "vol/out");
    size_t i;
    int r = d && o && !mkdir_p(d) && !mkdir_p(o) ? 0 : -1;

    for (i = 0; !r && i < 5; i++) {
        char *t = str_printf("%s/w%u/vol/blitz2/%s", P.work, w, to[i]);

        r = t && !copy_file(from[i], t) ? 0 : -1;
        free(t);
    }
    if (r)
        fprintf(log, "cdrun: cannot set up %s/w%u\n", P.work, w);
    free(d);
    free(o);
    return r;
}

/* the jobs of worker w that are not done, the retries first */
static size_t remaining(unsigned w, size_t *list)
{
    size_t i, n = 0;

    for (i = 0; i < njobs; i++)
        if (jobs[i].worker == w && jobs[i].compile && !jobs[i].done && jobs[i].attempts)
            list[n++] = i;
    for (i = 0; i < njobs; i++)
        if (jobs[i].worker == w && jobs[i].compile && !jobs[i].done && !jobs[i].attempts)
            list[n++] = i;
    return n;
}

static char *session_script(unsigned w, unsigned s, const size_t *list, size_t n)
{
    struct text jl = { 0 }, sc = { 0 };
    char *path = str_printf("%s/w%u/vol/s%03u.jobs", P.work, w, s);
    size_t i;

    text_add(&jl, "log=" VOLUME ":s%03u.log\ndefaults=" VOLUME ":defaults.xtra\n", s);
    for (i = 0; i < n; i++) {
        const struct job *j = &jobs[list[i]];
        char *dir = j->dir ? assign_path(j->dir) : NULL;

        text_add(&jl, "job=%s|" CDVOL ":%s|" VOLUME ":out/%s%s%s\n", j->id, j->rel, j->id,
                 dir ? "|" : "", dir ? dir : "");
        free(dir);
    }
    if (jl.bad || !path || write_file(path, jl.p, jl.n)) {
        free(jl.p);
        free(path);
        return NULL;
    }
    free(jl.p);
    free(path);
    text_add(&sc, "Assign blitz2: " VOLUME ":blitz2\n");
    for (i = 0; i < P.nassigns; i++) {
        size_t k;
        int again = 0;
        char *ap = assign_path(P.assign_dir[i]);

        for (k = 0; k < i; k++)
            again |= !strcasecmp(P.assign_name[k], P.assign_name[i]);
        again |= !strcasecmp(P.assign_name[i], "blitz2");
        text_add(&sc, "Assign %s: %s%s\n", P.assign_name[i], ap ? ap : "", again ? " ADD" : "");
        free(ap);
    }
    text_add(&sc, "Stack %u\n"
                  "blitz2:blitz2 " VOLUME ":s%03u.jobs >" VOLUME ":s%03u.out\n"
                  "Echo \"rc=$RC\" >>" VOLUME ":s%03u.out\n", P.stack, s, s, s);
    text_add(&sc, "Assign blitz2:\n");
    for (i = 0; i < P.nassigns; i++)
        if (strcasecmp(P.assign_name[i], "blitz2"))
            text_add(&sc, "Assign %s:\n", P.assign_name[i]);
    if (sc.bad) {
        free(sc.p);
        return NULL;
    }
    return sc.p;
}

static struct amiga_emulation *start_session(unsigned w, const char *script, char *err,
                                             size_t errlen)
{
    char *vol = worker_dir(w, "vol"), *emu = worker_dir(w, "emu");
    struct amiga_emulation_mount m[2] = { { VOLUME, vol, 1 }, { CDVOL, P.cd, 0 } };
    struct amiga_emulation_params ep = { 0 };
    struct amiga_emulation *e = NULL;

    if (vol && emu) {
        rm_tree(emu);
        mkdir_p(emu);
        ep.mounts = m;
        ep.nmounts = 2;
        ep.script = script;
        ep.timeout = P.timeout;
        ep.work_dir = emu;
        e = amiga_emulation_start(&ep, err, errlen);
    }
    free(vol);
    free(emu);
    return e;
}

static struct job *job_by_id(const char *id)
{
    size_t i;

    for (i = 0; i < njobs; i++)
        if (!strcmp(jobs[i].id, id))
            return &jobs[i];
    return NULL;
}

static void add_note(struct job *j, const char *s)
{
    char *n = j->note ? str_printf("%s; %s", j->note, s) : strdup(s);

    free(j->note);
    j->note = n;
}

/* the session's log: started jobs, results, notes; returns the number of
   started jobs, *unfinished gets those without a result */
static size_t read_session(unsigned w, unsigned s, size_t *unfinished, size_t *nunf, int *exited)
{
    char *path = str_printf("%s/w%u/vol/s%03u.log", P.work, w, s);
    char *opath = str_printf("%s/w%u/vol/s%03u.out", P.work, w, s);
    size_t len, started = 0;
    char *text = path ? (char *)read_file(path, &len) : NULL, *out, *l, *save = NULL;

    *nunf = 0;
    out = opath ? (char *)read_file(opath, &len) : NULL;
    *exited = out && strstr(out, "rc=");
    free(out);
    for (l = text ? strtok_r(text, "\n", &save) : NULL; l; l = strtok_r(NULL, "\n", &save)) {
        char *f[16], *t = l;
        size_t n = 0;
        struct job *j;

        /* split at every tab: empty fields count (Python split) */
        while (n < 16) {
            f[n++] = t;
            if (!(t = strchr(t, '\t')))
                break;
            *t++ = 0;
        }
        if (n < 2 || !(j = job_by_id(f[1])))
            continue;
        if (!strcmp(f[0], "JOB")) {
            started++;
            unfinished[(*nunf)++] = (size_t)(j - jobs);
        } else if (!strcmp(f[0], "RESULT") && n >= 3) {
            size_t k;

            for (k = 0; k < j->nresult; k++)
                free(j->result[k]);
            free(j->result);
            j->result = calloc(n - 2, sizeof(char *));
            j->nresult = n - 2;
            for (k = 2; j->result && k < n; k++)
                j->result[k - 2] = strdup(f[k]);
            j->done = 1;
        } else if (!strcmp(f[0], "XTRA") && n >= 3) {
            free(j->xtra);
            j->xtra = strdup(f[2]);
        } else if (!strcmp(f[0], "XTRAERR") && n >= 3) {
            struct text m = { 0 };
            size_t k;

            text_add(&m, "xtra error:");
            for (k = 2; k < n; k++)
                text_add(&m, " %s", f[k]);
            if (!m.bad)
                add_note(j, m.p);
            free(m.p);
        }
    }
    /* keep only the started jobs without a result */
    {
        size_t i, k = 0;

        for (i = 0; i < *nunf; i++)
            if (!jobs[unfinished[i]].done)
                unfinished[k++] = unfinished[i];
        *nunf = k;
    }
    free(text);
    free(path);
    free(opath);
    return started;
}

static void set_result(struct job *j, const char *a, const char *b)
{
    size_t k;

    for (k = 0; k < j->nresult; k++)
        free(j->result[k]);
    free(j->result);
    j->result = calloc(2, sizeof(char *));
    j->nresult = 2;
    if (j->result) {
        j->result[0] = strdup(a);
        j->result[1] = strdup(b);
    }
    j->done = 1;
}

/* ---------------------------------------------------------------- table */

static void tsv_field(struct text *t, const char *s, int last)
{
    for (; s && *s; s++)
        text_add(t, "%c", *s == '\t' || *s == '\n' ? ' ' : *s);
    text_add(t, last ? "\n" : "\t");
}

static char *table(void)
{
    struct text t = { 0 };
    size_t i;

    text_add(&t, "path\tresult\tline\tmessage\tsize\tsha256_masked\txtra\tnote\n");
    for (i = 0; i < njobs; i++) {
        const struct job *j = &jobs[i];
        const char *result = j->nresult ? j->result[0] : "nostart";
        char *line = NULL, *msg = NULL, *size = NULL, hash[65] = "";

        if (j->tok && !j->compile)
            continue;                   /* behind the limit: not in the table */
        if (!j->tok) {
            result = "skipped";
            msg = strdup("plain text (no $00 line ends)");
        } else if (!strcmp(result, "ok")) {
            char *exe = str_printf("%s/w%u/vol/out/%s", P.work, j->worker, j->id);
            size_t len;
            uint8_t *d = exe ? read_file(exe, &len) : NULL;

            if (d) {
                size = str_printf("%zu", len);
                corpus_mask_traps(d, len);
                sha256_hex_of(d, len, hash);
            } else {
                result = "failed";
                msg = strdup("executable missing on the host");
            }
            free(d);
            free(exe);
        } else if (!strcmp(result, "error")) {
            line = strdup(j->nresult > 1 ? j->result[1] : "");
            msg = j->nresult > 3 && *j->result[3]
                  ? str_printf("%s | %s", j->result[2], j->result[3])
                  : strdup(j->nresult > 2 ? j->result[2] : "");
        } else {
            struct text m = { 0 };
            size_t k;

            for (k = 1; k < j->nresult; k++)
                text_add(&m, k > 1 ? " %s" : "%s", j->result[k]);
            msg = m.p ? m.p : strdup("");
        }
        tsv_field(&t, j->rel, 0);
        tsv_field(&t, result, 0);
        tsv_field(&t, line, 0);
        tsv_field(&t, msg, 0);
        tsv_field(&t, size, 0);
        tsv_field(&t, hash, 0);
        tsv_field(&t, j->xtra, 0);
        tsv_field(&t, j->note, 1);
        free(line);
        free(msg);
        free(size);
    }
    if (t.bad) {
        free(t.p);
        return NULL;
    }
    return t.p;
}

/* rows of a table: path -> the compared fields joined */
struct row {
    char *path, *key, *result;
};

static size_t rows_of(char *t, struct row **out)
{
    size_t n = 0;
    char *l, *save = NULL;

    *out = NULL;
    for (l = strtok_r(t, "\n", &save); l; l = strtok_r(NULL, "\n", &save)) {
        char *f[8] = { 0 }, *p = l;
        size_t k = 0;
        struct row *r;

        if (!strncmp(l, "path\t", 5))
            continue;
        while (k < 8) {
            f[k++] = p;
            if (!(p = strchr(p, '\t')))
                break;
            *p++ = 0;
        }
        if (k < 6 || !(r = realloc(*out, (n + 1) * sizeof *r)))
            continue;
        *out = r;
        r[n].path = f[0];
        r[n].result = f[1];
        r[n].key = str_printf("%s\t%s\t%s\t%s\t%s", f[1], f[2], f[3], f[4], f[5]);
        n++;
    }
    return n;
}

static int compare(const char *mine, FILE *log)
{
    size_t len, na, nb, i, k, differ = 0, same = 0, missing = 0;
    char *ta = (char *)read_file(P.reference, &len), *tb = strdup(mine);
    struct row *a = NULL, *b = NULL;
    struct text rep = { 0 };
    int rc = 1;

    if (!ta || !tb) {
        fprintf(log, "cdrun: cannot read %s\n", P.reference);
        free(ta);
        free(tb);
        return 2;
    }
    na = rows_of(ta, &a);
    nb = rows_of(tb, &b);
    text_add(&rep, "== %s against %s\n", P.output, P.reference);
    for (i = 0; i < na; i++) {
        for (k = 0; k < nb && strcmp(a[i].path, b[k].path); k++)
            ;
        if (k == nb) {
            missing++;
            text_add(&rep, "   %s: not in %s\n", a[i].path, P.output);
        } else if (strcmp(a[i].key, b[k].key)) {
            /* the differing fields: reference -> now */
            static const char *const fields[] = { "result", "line", "message", "size",
                                                  "sha256_masked" };
            const char *x = a[i].key, *y = b[k].key;
            int f;

            differ++;
            text_add(&rep, "   %s:", a[i].path);
            for (f = 0; f < 5; f++) {
                size_t xl = strcspn(x, "\t"), yl = strcspn(y, "\t");

                if (xl != yl || strncmp(x, y, xl))
                    text_add(&rep, " %s %.*s -> %.*s", fields[f], (int)xl, x, (int)yl, y);
                x += xl + (x[xl] == '\t');
                y += yl + (y[yl] == '\t');
            }
            text_add(&rep, "\n");
        } else
            same++;
    }
    for (k = 0; k < nb; k++) {
        for (i = 0; i < na && strcmp(a[i].path, b[k].path); i++)
            ;
        if (i == na) {
            missing++;
            text_add(&rep, "   %s: not in the reference\n", b[k].path);
        }
    }
    text_add(&rep, "%zu sources, %zu equal, %zu different, %zu missing\n", na, same, differ,
             missing);
    if (!rep.bad && !write_file(P.report, rep.p, rep.n)) {
        fwrite(rep.p, 1, rep.n, log);
        rc = differ || missing ? 1 : 0;
    } else
        rc = 2;
    for (i = 0; i < na; i++)
        free(a[i].key);
    for (i = 0; i < nb; i++)
        free(b[i].key);
    free(a);
    free(b);
    free(ta);
    free(tb);
    free(rep.p);
    return rc;
}

/* ----------------------------------------------------------------- run */

/* source=: only the listed sources stay in the job list (each must be
   there) */
static int select_sources(FILE *log)
{
    size_t i, k, n = 0;
    int bad = 0;

    for (k = 0; k < P.nsources; k++) {
        for (i = 0; i < njobs && strcmp(jobs[i].rel, P.source_rel[k]); i++)
            ;
        if (i == njobs) {
            fprintf(log, "cdrun: source %s is not below %s\n", P.source_rel[k], P.cd);
            bad = 1;
        }
    }
    for (i = 0; i < njobs; i++) {
        for (k = 0; k < P.nsources && strcmp(jobs[i].rel, P.source_rel[k]); k++)
            ;
        if (k < P.nsources)
            jobs[n++] = jobs[i];
        else
            free(jobs[i].rel);
    }
    njobs = n;
    return bad ? -1 : 0;
}

/* source=: the executables of the listed sources to their OUT; 1 if one
   did not compile */
static int copy_sources(FILE *log)
{
    size_t i, k;
    int rc = 0;

    for (k = 0; k < P.nsources; k++) {
        struct job *j = NULL;

        for (i = 0; i < njobs; i++)
            if (!strcmp(jobs[i].rel, P.source_rel[k]))
                j = &jobs[i];
        if (!j || !j->tok || !j->nresult || strcmp(j->result[0], "ok")) {
            fprintf(log, "cdrun: %s: %s%s%s\n", P.source_rel[k],
                    !j || !j->tok ? "not tokenised"
                    : j->nresult ? j->result[0] : "nostart",
                    j && j->nresult > 2 ? " " : "", j && j->nresult > 2 ? j->result[2] : "");
            rc = 1;
            continue;
        }
        {
            char *exe = str_printf("%s/w%u/vol/out/%s", P.work, j->worker, j->id);
            char *dir = strdup(P.source_out[k]), *slash = dir ? strrchr(dir, '/') : NULL;

            if (slash)
                *slash = 0;
            if (!exe || !dir || (slash && mkdir_p(dir)) || copy_file(exe, P.source_out[k])) {
                fprintf(log, "cdrun: cannot copy %s to %s\n", exe ? exe : "?",
                        P.source_out[k]);
                rc = 1;
            }
            free(exe);
            free(dir);
        }
    }
    return rc;
}

/* ------------------------------------------------------------- cache */

static const char **cache_outs;
static size_t ncache_outs;

/* the outputs of the pass (cache=); -1 = out of memory */
static int cache_outputs(void)
{
    size_t i;

    if (!(cache_outs = malloc((P.nsources + 2) * sizeof *cache_outs)))
        return -1;
    ncache_outs = 0;
    cache_outs[ncache_outs++] = P.output;
    if (P.report)
        cache_outs[ncache_outs++] = P.report;
    for (i = 0; i < P.nsources; i++)
        cache_outs[ncache_outs++] = P.source_out[i];
    return 0;
}

/* the digest of the inputs (cache=); -1 if one is missing - then no
   record is written */
static int cache_digest(const char *text, char hex[65])
{
    char self[4096];
    ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    const char *files[8];
    const char *dirs[] = { P.cd };
    size_t nfiles = 0;

    if (n <= 0 || (size_t)n >= sizeof self - 1)
        return -1;
    self[n] = 0;
    files[nfiles++] = self;
    files[nfiles++] = P.blitz2;
    files[nfiles++] = P.deflibs;
    files[nfiles++] = P.acidlibs;
    files[nfiles++] = P.defaultdbug;
    files[nfiles++] = P.ted;
    files[nfiles++] = P.tokens;
    if (P.reference)
        files[nfiles++] = P.reference;
    return cdcache_digest(text, files, nfiles, dirs, 1, hex);
}

/* ends session s of worker w (e[w]) and takes over its results; list and
   unf are scratch space */
static void finish_session(unsigned w, unsigned s, struct amiga_emulation **e, size_t *list,
                           size_t *unf, unsigned *empty, FILE *log)
{
    size_t nunf, started, k;
    int exited;
    struct statline st;
    char config[16];

    amiga_emulation_join(e[w]);
    /* its @stat line (statline.h): the worker, no verdict */
    snprintf(config, sizeof config, "w%u", w);
    statline_session(&st, P.work, config, e[w]);
    statline_write(log, &st);
    amiga_emulation_free(e[w]);
    e[w] = NULL;
    started = read_session(w, s, unf, &nunf, &exited);
    fprintf(log, "cdrun: session %03u worker %u: started %zu, unfinished %zu\n", s, w, started,
            nunf);
    if (!started) {
        if (++empty[w] >= 2) {
            size_t n = remaining(w, list);

            for (k = 0; k < n; k++)
                set_result(&jobs[list[k]], "nostart", "compiler/stub did not reach the job list");
        }
        return;
    }
    empty[w] = 0;
    for (k = 0; k < nunf; k++)
        if (++jobs[unf[k]].attempts >= 2)
            set_result(&jobs[unf[k]], "crash", exited ? "blitz2 returned"
                       : "emulator timeout (hang or crash requester)");
}

static int run(FILE *log)
{
    const char *files[] = { P.blitz2, P.deflibs, P.acidlibs, P.defaultdbug, P.ted, P.tokens };
    struct amiga_emulation **e = calloc(P.parallel, sizeof *e);
    size_t *list = NULL, *unf = NULL, i, ntok = 0;
    unsigned w, nworkers, session = 0, *empty = calloc(P.parallel, sizeof *empty);
    char err[300], *t;
    int rc = 2;

    /* source=: no executable of an earlier run stays (nor the record that
       described it) */
    if (P.cache)
        remove(P.cache);
    for (i = 0; i < P.nsources; i++)
        remove(P.source_out[i]);
    for (i = 0; i < sizeof files / sizeof files[0]; i++)
        if (access(files[i], R_OK)) {
            fprintf(log, "cdrun: no %s - skipped\n", files[i]);
            rc = 77;
            goto out;
        }
    if (access(P.cd, R_OK | X_OK)) {
        fprintf(log, "cdrun: no CD %s - skipped\n", P.cd);
        rc = 77;
        goto out;
    }
    if (!e || !empty || load_tokens())
        goto out;
    cdlen = strlen(P.cd);
    while (cdlen > 1 && P.cd[cdlen - 1] == '/')
        P.cd[--cdlen] = 0;
    if (nftw(P.cd, collect, 16, FTW_PHYS))
        goto out;
    qsort(jobs, njobs, sizeof *jobs, by_rel);
    if (P.nsources && select_sources(log))
        goto out;
    /* ids, kinds, job directories; the tokenised ones spread over the
       workers */
    for (i = 0; i < njobs; i++) {
        struct job *j = &jobs[i];
        char *path = str_printf("%s/%s", P.cd, j->rel), *text;
        size_t len;
        uint8_t *d = path ? read_file(path, &len) : NULL;

        snprintf(j->id, sizeof j->id, "s%04zu", i + 1);
        j->tok = d && memchr(d, 0, len) != NULL;
        j->compile = j->tok && (!P.limit || ntok < P.limit);
        if (j->compile) {
            j->worker = (unsigned)(ntok++ % P.parallel);
            if ((text = detok(d, len))) {
                j->dir = include_dir(j, text);
                free(text);
            }
        }
        free(d);
        free(path);
    }
    nworkers = ntok < P.parallel ? (unsigned)ntok : P.parallel;
    list = calloc(njobs + 1, sizeof *list);
    unf = calloc(njobs + 1, sizeof *unf);
    if (!list || !unf)
        goto out;
    rm_tree(P.work);
    for (w = 0; w < nworkers; w++)
        if (stage_worker(w, log))
            goto out;
    fprintf(log, "cdrun: %zu sources, %zu to compile, %u workers\n", njobs, ntok, nworkers);

    /* rounds: one session per worker that has jobs left.  No free emulator
       slot while sessions of this round run: the start is tried again
       every AMIGA_EMULATION_SLOT_RETRY_MS (the slot limit is hard), the
       sessions of the round that end meanwhile are finished */
    while (session < MAXSESSIONS) {
        unsigned started_any = 0, s = ++session;

        for (w = 0; w < nworkers; w++)
            e[w] = NULL;
        for (w = 0; w < nworkers; w++) {
            size_t n = remaining(w, list);
            char *script;

            if (!n || empty[w] >= 2)
                continue;
            if (!(script = session_script(w, s, list, n))) {
                fprintf(log, "cdrun: cannot write the session files\n");
                goto out;
            }
            for (int told = 0; !(e[w] = start_session(w, script, err, sizeof err))
                               && amiga_emulation_no_slot();) {
                int ended;

                if (!told++)
                    fprintf(log, "cdrun: worker %u: %s - waiting for a slot\n", w, err);
                if ((ended = amiga_emulation_wait_any(e, w, AMIGA_EMULATION_SLOT_RETRY_MS)) >= 0)
                    finish_session((unsigned)ended, s, e, list, unf, empty, log);
            }
            free(script);
            if (!e[w]) {
                fprintf(log, "cdrun: %s - skipped\n", err);
                rc = 77;
                goto out;
            }
            started_any++;
        }
        if (!started_any)
            break;
        for (w = 0; w < nworkers; w++)
            if (e[w])
                finish_session(w, s, e, list, unf, empty, log);
        fflush(log);
    }

    if (!(t = table()) || write_file(P.output, t, strlen(t))) {
        fprintf(log, "cdrun: cannot write %s\n", P.output);
        free(t);
        goto out;
    }
    {
        size_t counts[6] = { 0 };
        static const char *const kinds[] = { "ok", "error", "failed", "crash", "nostart",
                                             "skipped" };
        char *l, *save = NULL, *copy = strdup(t);

        for (l = copy ? strtok_r(copy, "\n", &save) : NULL; l; l = strtok_r(NULL, "\n", &save)) {
            char *r = strchr(l, '\t');

            for (i = 0; r && i < 6; i++)
                if (!strncmp(r + 1, kinds[i], strlen(kinds[i])) && r[1 + strlen(kinds[i])] == '\t')
                    counts[i]++;
        }
        free(copy);
        fprintf(log, "cdrun: %s: %u sessions;", P.output, session);
        for (i = 0; i < 6; i++)
            fprintf(log, " %s %zu", kinds[i], counts[i]);
        fprintf(log, "\n");
    }
    rc = P.reference ? compare(t, log) : 0;
    free(t);
    if (P.nsources && rc != 2 && copy_sources(log))
        rc = 1;
out:
    for (w = 0; w < P.parallel; w++)
        if (e && e[w]) {
            amiga_emulation_join(e[w]);
            amiga_emulation_free(e[w]);
        }
    free(e);
    free(empty);
    free(list);
    free(unf);
    return rc;
}

static const char usage[] =
    "usage: cdrun PASSFILE\n"
    "  every .bb2/.bb of the CD compiled by one compiler in Amiberry\n";

int main(int argc, char **argv)
{
    char err[300], digest[65];
    size_t len;
    char *text;
    int rc, cached = 0;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    if (argc != 2) {
        fputs(usage, stderr);
        return 2;
    }
    if (!(text = (char *)read_file(argv[1], &len))) {
        fprintf(stderr, "cdrun: cannot read %s\n", argv[1]);
        return 2;
    }
    if (parse(text, err, sizeof err)) {
        fprintf(stderr, "cdrun: %s: %s\n", argv[1], err);
        free(text);
        return 2;
    }
    if (P.cache) {
        const char *force = getenv("CDRUN_FORCE");

        if (cache_outputs()) {
            fprintf(stderr, "cdrun: out of memory\n");
            free(text);
            return 2;
        }
        cached = !cache_digest(text, digest);
        if (cached && !(force && *force && strcmp(force, "0"))
            && cdcache_hit(P.cache, digest, cache_outs, ncache_outs)) {
            printf("cdrun: inputs and outputs unchanged since the last run (%s): "
                   "no emulator run (CDRUN_FORCE=1 runs anyway)\n", P.cache);
            free(text);
            return 0;
        }
    }
    free(text);
    rc = run(stdout);
    if (!rc && cached && cdcache_write(P.cache, digest, cache_outs, ncache_outs))
        printf("cdrun: cannot write %s (the next run will not be skipped)\n", P.cache);
    return rc;
}
