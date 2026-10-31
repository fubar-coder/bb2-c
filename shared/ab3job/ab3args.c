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
    "usage: blitz2-ab3 -s SOURCE -e EXE [-r] [-mode ab3|bb2]\n"
    "  -s, -source FILE   the source to compile (text or tokenised)\n"
    "  -e, -exe FILE      the executable to write\n"
    "  -r, -release       release executable: no runtime error checking,\n"
    "                     no debugger, no debug information\n"
    "  -mode ab3|bb2      compile in AmiBlitz3 or Blitz Basic 2 mode\n"
    "                     (default: detected from the source)\n"
    "  -h, -help          this text\n"
    "Without options blitz2-ab3 starts the editor like blitz2.\n";

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
            || eq(opt, "mode")) {
            const char *v = i + 1 < argc ? argv[i + 1] : 0;

            if (!v || !v[0])
                return fail(&o, AB3ARGS_FAIL, "option ", a, " needs an argument");
            i++;
            if (eq(opt, "mode")) {
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
