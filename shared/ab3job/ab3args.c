/*
 * ab3args.c - the command line of blitz2-ab3 (ab3job.h; spec
 * ab3-compat/command-line).  The option names are AmiBlitz3's (its CLI
 * mode, the help text of AmiBlitz3 v3.14.0's compiler); -mode is this
 * repository's.  C99, freestanding.
 */
#include "ab3job.h"

/* enum xtra_mode (shared/xtra/xtra.h), not included: the job is plain data */
#define MODE_DETECT 0
#define MODE_BB2 1
#define MODE_AB3 2

const char ab3args_usage[] =
    "usage: blitz2-ab3 -s SOURCE -e EXE [-r] [-mode ab3|bb2] [-w LIST] [-werr LIST]\n"
    "  -s, -source FILE   the source to compile (text or tokenised)\n"
    "  -e, -exe FILE      the executable to write\n"
    "  -r, -release       release executable: no runtime error checking,\n"
    "                     no debugger, no debug information\n"
    "  -mode ab3|bb2      compile in AmiBlitz3 or Blitz Basic 2 mode\n"
    "                     (default: detected from the source)\n"
    "  -w LIST            the warnings shown (default: the source's setting)\n"
    "  -werr LIST         the warnings that are errors\n"
    "                     LIST: all, none, NAME, -NAME, comma-separated,\n"
    "                     left to right; NAME: resclash\n"
    "  -h, -help          this text\n"
    "Without options blitz2-ab3 starts the editor like blitz2.\n";

/* the names by number (enum ab3job_warning); the usage text lists them */
const char *const ab3args_warnings[] = {
    "resclash",
    0
};
/* one name per warning: a check of the build (C99, no _Static_assert) */
typedef char ab3args_warnings_complete[
    sizeof ab3args_warnings / sizeof ab3args_warnings[0] == AB3JOB_W_COUNT + 1 ? 1 : -1];

/* AmiBlitz3's other options (its help text): refused by name - the first
   one stops the parse, so their arguments need not be known */
static const char *const unsupported[] = {
    "d", "debug", "run", "dump", "setcom", "setdbg", "strbufsize", "stacksize",
    "maxerrors", "1", "1pass", "dform", "ped", "save", "v", "verbose", 0
};

/* a == b ignoring the case of ASCII letters */
static int eq(const char *a, const char *b)
{
    for (;; a++, b++) {
        char x = *a, y = *b;

        if (x >= 'A' && x <= 'Z')
            x = (char)(x - 'A' + 'a');
        if (y >= 'A' && y <= 'Z')
            y = (char)(y - 'A' + 'a');
        if (x != y)
            return 0;
        if (!x)
            return 1;
    }
}

struct out {
    char *buf;
    size_t len, cap;
};

static void put(struct out *o, const char *s)
{
    while (*s && o->len + 1 < o->cap)
        o->buf[o->len++] = *s++;
    o->buf[o->len] = 0;
}

static int fail(struct out *o, int result, const char *a, const char *b, const char *c)
{
    put(o, a);
    if (b)
        put(o, b);
    if (c)
        put(o, c);
    return result;
}

/* the item [s, e) equals the name n, ignoring the case of ASCII letters */
static int eqn(const char *s, const char *e, const char *n)
{
    for (; s < e; s++, n++) {
        char x = *s, y = *n;

        if (x >= 'A' && x <= 'Z')
            x = (char)(x - 'A' + 'a');
        if (x != y)                     /* the names are lowercase */
            return 0;
    }
    return !*n;
}

/* a warning list (design D4) applied to *keep, *add: the items of v
   left to right, set = (set & keep) | add.  0 and the message in o if
   an item is empty or not a warning */
static int warnlist(struct out *o, const char *opt, const char *v, uint32_t *keep,
                    uint32_t *add)
{
    const char *s = v;

    for (;;) {
        const char *e = s, *item = s;
        int neg = 0, k;
        uint32_t bit = 0;

        while (*e && *e != ',')
            e++;
        if (*s == '-') {
            neg = 1;
            s++;
        }
        if (s == e) {
            fail(o, 0, "empty item in ", opt, " \"");
            put(o, v);
            put(o, "\"");
            return 0;
        }
        if (!neg && eqn(s, e, "all")) {
            *keep = 0;
            *add = AB3JOB_W_ALL;
        } else if (!neg && eqn(s, e, "none")) {
            *keep = 0;
            *add = 0;
        } else {
            for (k = 0; ab3args_warnings[k]; k++)
                if (eqn(s, e, ab3args_warnings[k]))
                    bit = (uint32_t)1 << k;
            if (!bit) {
                put(o, "unknown warning \"");
                while (item < e && o->len + 1 < o->cap)
                    o->buf[o->len++] = *item++;
                o->buf[o->len] = 0;
                put(o, "\" in ");
                put(o, opt);
                put(o, " (known: all, none");
                for (k = 0; ab3args_warnings[k]; k++) {
                    put(o, ", ");
                    put(o, ab3args_warnings[k]);
                }
                put(o, ")");
                return 0;
            }
            if (neg) {
                *keep &= ~bit;
                *add &= ~bit;
            } else
                *add |= bit;
        }
        if (!*e)
            return 1;
        s = e + 1;
    }
}

/* copy a file name into the job; 0 if it does not fit */
static int setname(char *dst, const char *src)
{
    size_t i;

    for (i = 0; src[i]; i++) {
        if (i + 1 >= AB3JOB_PATHLEN)
            return 0;
        dst[i] = src[i];
    }
    dst[i] = 0;
    return 1;
}

int ab3args_parse(int argc, char *const argv[], struct ab3job *job, char *msg, size_t cap)
{
    struct out o = { msg, 0, cap };
    const char *magic = AB3JOB_MAGIC;
    unsigned char *p = (unsigned char *)job;
    int i, k, options = 0;

    for (i = 0; i < (int)sizeof *job; i++)
        p[i] = 0;
    for (i = 0; magic[i]; i++)
        job->magic[i] = magic[i];
    job->force = MODE_DETECT;
    job->result = AB3JOB_RC_FAIL;
    job->warn_keep = AB3JOB_W_ALL;      /* no list: the source's setting */
    job->werr_keep = AB3JOB_W_ALL;
    msg[0] = 0;

    for (i = 1; i < argc; i++)
        if (argv[i][0] == '-')
            options = 1;
    if (!options)
        return AB3ARGS_EDITOR;

    for (i = 1; i < argc; i++) {
        const char *a = argv[i], *opt = a + 1;

        if (a[0] != '-')
            return fail(&o, AB3ARGS_FAIL, "unexpected argument \"", a, "\"");
        if (eq(opt, "h") || eq(opt, "help"))
            return AB3ARGS_HELP;
        if (eq(opt, "r") || eq(opt, "release")) {
            job->release_exe = 1;
            continue;
        }
        if (eq(opt, "s") || eq(opt, "source") || eq(opt, "e") || eq(opt, "exe")
            || eq(opt, "mode") || eq(opt, "w") || eq(opt, "werr")) {
            const char *v = i + 1 < argc ? argv[i + 1] : 0;

            if (!v || !v[0])
                return fail(&o, AB3ARGS_FAIL, "option ", a, " needs an argument");
            i++;
            if (eq(opt, "w") || eq(opt, "werr")) {
                int w = eq(opt, "w");

                if (!warnlist(&o, a, v, w ? &job->warn_keep : &job->werr_keep,
                              w ? &job->warn_add : &job->werr_add))
                    return AB3ARGS_FAIL;
            } else if (eq(opt, "mode")) {
                if (eq(v, "ab3"))
                    job->force = MODE_AB3;
                else if (eq(v, "bb2"))
                    job->force = MODE_BB2;
                else
                    return fail(&o, AB3ARGS_FAIL, "unknown mode \"", v, "\" (ab3 or bb2)");
            } else if (!setname(opt[0] == 's' || opt[0] == 'S' ? job->source : job->exe, v))
                return fail(&o, AB3ARGS_FAIL, "file name too long: ", v, 0);
            continue;
        }
        for (k = 0; unsupported[k]; k++)
            if (eq(opt, unsupported[k]))
                return fail(&o, AB3ARGS_UNSUPPORTED, "option ", a,
                            " is not supported by blitz2-ab3");
        return fail(&o, AB3ARGS_FAIL, "unknown option ", a, 0);
    }
    if (!job->source[0])
        return fail(&o, AB3ARGS_FAIL, "no source given (-s)", 0, 0);
    if (!job->exe[0])
        return fail(&o, AB3ARGS_FAIL, "no executable given (-e)", 0, 0);
    return AB3ARGS_COMPILE;
}

int ab3args_rc(int result)
{
    switch (result) {
    case AB3ARGS_HELP:
        return AB3JOB_RC_OK;
    case AB3ARGS_UNSUPPORTED:
    case AB3ARGS_FAIL:
        return AB3JOB_RC_FAIL;
    default:
        return -1;
    }
}
