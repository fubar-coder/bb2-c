/*
 * ab3job.h - a compile from the shell with blitz2-ab3 (spec
 * ab3-compat/command-line, docs/ab3/variant.md): the command line, modelled
 * on AmiBlitz3's CLI mode, and the job blitz2-ab3 hands to its headless
 * editor ab3cli.
 *
 *   blitz2-ab3 -s SOURCE -e EXE [-r] [-mode ab3|bb2] [-w LIST] [-werr LIST]
 *
 * blitz2-ab3 parses its arguments with ab3args_parse, decides the mode of
 * the source, loads PROGDIR:ab3cli instead of blitz2:ted and calls its main
 * entry with a0 = the job (instead of the file name an editor gets);
 * ab3cli loads and compiles the source and leaves its return code in the
 * job.  The job is plain data, so both sides - compiled in their own
 * worlds - share this layout.
 *
 * Shared code like shared/xtra: every user compiles ab3args.c itself (the
 * host tests in tools/lib/ab3job, blitz2-ab3).  C99 on the freestanding
 * headers: no libc, no writable static data.
 */
#ifndef AB3JOB_H
#define AB3JOB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The job's first bytes: what ab3cli checks before it trusts a0. */
#define AB3JOB_MAGIC "AB3JOB1"
/** @brief Bytes of a file name in the job (with its 0). */
#define AB3JOB_PATHLEN 256

/** @brief The return codes of a command line run (design D6): those of
 *         the NDK's dos/dos.h (ab3cli checks that they agree; the host
 *         has no dos/dos.h). */
enum ab3job_rc {
    AB3JOB_RC_OK = 0,           /* RETURN_OK: compiled without warnings
                                   (or -h) */
    AB3JOB_RC_WARN = 5,         /* RETURN_WARN: compiled with warnings */
    AB3JOB_RC_ERROR = 10,       /* RETURN_ERROR: compile error, or a
                                   warning that is an error */
    AB3JOB_RC_FAIL = 20         /* RETURN_FAIL: argument or setup error */
};

/**
 * @brief The compiler's warnings (change compiler-warnings, design D1):
 *        their numbers, bit n of a warning set.  The compiler core has the
 *        same numbers in parse.h (W_*); ab3main.c checks that they agree.
 */
enum ab3job_warning {
    AB3JOB_W_RESCLASH,          /* a constant of the residents with two
                                   values (stmts.c checkconstclash) */
    AB3JOB_W_COUNT
};
/** @brief The set of all warnings. */
#define AB3JOB_W_ALL ((uint32_t)((1UL << AB3JOB_W_COUNT) - 1))

/** @brief The names of the warnings by number (-w, -werr and the console
 *         line), AB3JOB_W_COUNT entries and a NULL. */
extern const char *const ab3args_warnings[];

/**
 * @brief The job of a command line run.
 *
 * Filled by ab3args_parse (force, release_exe, source, exe, the warning
 * lists), by blitz2-ab3 (mode, the error's place, warnerrs, warnings) and
 * by ab3cli (started, result, tokfile).
 */
struct ab3job {
    char magic[8];              /* AB3JOB_MAGIC with its 0 */
    uint8_t force;              /* -mode: enum xtra_mode, XTRA_MODE_DETECT
                                   (0) without the option */
    uint8_t mode;               /* the program's mode (XTRA_MODE_BB2 or
                                   XTRA_MODE_AB3), decided by blitz2-ab3 */
    uint8_t release_exe;        /* -r: no runtime error checking, no
                                   debugger, no debug information */
    uint8_t started;            /* ab3cli ran the job */
    uint8_t result;             /* enum ab3job_rc, set by ab3cli */
    uint8_t pad;
    /* ab3cli's service for a text include, set by ab3cli: an entry with
       the registers of an editor service (blitz2 callregs) - a0 = the
       include's name; out d0 = its lines as the editor loads them (TED's
       readline: tokenised with the program's token list), each ending in
       its 0, in AllocMem(d1 + 1, MEMF_PUBLIC) memory the caller frees;
       d1 = their length; d2 = the empty lines its embedded header became
       (AB3 mode; 0 without a header and in BB2 mode); d0 = 0: the file
       cannot be read (reported) */
    void *tokfile;
    char source[AB3JOB_PATHLEN];        /* -s */
    char exe[AB3JOB_PATHLEN];           /* -e */
    /* the place of a compile error inside an include (task 3.6): ab3cli
       clears errset before the compile; blitz2-ab3 places the first error
       (errset = 1): errline > 0 - line errline of the include errfile
       (the innermost one that has a line read), 0 - the compiler's line
       of the source */
    uint8_t errset;
    uint8_t pad2[3];
    uint32_t errline;
    char errfile[AB3JOB_PATHLEN];
    /* -w and -werr (design D4): the lists, left to right, as changes of
       the source's setting: set = (set & keep) | add, for the enabled
       warnings (warn_*) and the ones that are errors (werr_*); without
       the option keep = AB3JOB_W_ALL, add = 0 */
    uint32_t warn_keep, warn_add;
    uint32_t werr_keep, werr_add;
    /* the warnings that became errors (blitz2-ab3, ext_warning): ab3cli
       ends with AB3JOB_RC_ERROR when they were all its set-up errors */
    uint8_t warnerrs;
    /* the warnings printed (blitz2-ab3, ext_warning; at most 255): a
       compile without errors ends with AB3JOB_RC_WARN */
    uint8_t warnings;
    uint8_t pad3[2];
};

/**
 * @brief The return code at the end of a compile (design D6 of the change
 *        compiler-warnings), what ab3cli leaves in the job.
 *
 * A set-up error (outside the compile: loading libraries or residents)
 * gives AB3JOB_RC_FAIL, unless all of them were warnings that are errors
 * (AB3JOB_RC_ERROR); a compile error AB3JOB_RC_ERROR; no executable
 * written AB3JOB_RC_FAIL; otherwise AB3JOB_RC_WARN after a warning, else
 * AB3JOB_RC_OK.
 *
 * @param job        the job (warnerrs, warnings)
 * @param setuperrs  the set-up errors (0: the compile ran)
 * @param compileerr nonzero: the compile ended with an error
 * @param written    nonzero: the executable was written
 * @return an enum ab3job_rc
 */
static inline int ab3job_rc(const struct ab3job *job, int setuperrs, int compileerr, int written)
{
    if (setuperrs)
        return setuperrs == job->warnerrs ? AB3JOB_RC_ERROR : AB3JOB_RC_FAIL;
    if (compileerr)
        return AB3JOB_RC_ERROR;
    if (!written)
        return AB3JOB_RC_FAIL;
    return job->warnings ? AB3JOB_RC_WARN : AB3JOB_RC_OK;
}

/** @brief What ab3args_parse found. */
enum ab3args_result {
    AB3ARGS_EDITOR,             /* no option: start the editor as blitz2 */
    AB3ARGS_COMPILE,            /* the job is filled: compile it */
    AB3ARGS_HELP,               /* -h: print the usage, return code 0 */
    AB3ARGS_UNSUPPORTED,        /* an option of AmiBlitz3 this change does
                                   not support: print the message, 20 */
    AB3ARGS_FAIL                /* an argument error: print the message and
                                   the usage, 20 */
};

/** @brief The usage (several lines, each ending in LF). */
extern const char ab3args_usage[];

/**
 * @brief Parse the command line of blitz2-ab3.
 *
 * argv[1..argc) as libnix (or a shell) splits them.  Without an argument
 * that starts with '-' the result is AB3ARGS_EDITOR (blitz2's start with
 * at most a project name).  Options, in any case, AmiBlitz3's spelling:
 * -s/-source FILE, -e/-exe FILE, -r/-release, -mode ab3|bb2 and
 * -w/-werr LIST (this repository's), -h/-help.  A LIST is comma-separated
 * items all, none, NAME, -NAME (ab3args_warnings), applied left to right;
 * several -w (-werr) are applied in order.  The other options of AmiBlitz3's CLI mode
 * (-d/-debug, -run, -dump, -setcom, -setdbg, -strbufsize, -stacksize,
 * -maxerrors, -1/-1pass, -dform, -PED, -save, -v/-verbose) are refused.
 * The first problem decides; -h before it gives AB3ARGS_HELP.  Given
 * twice, the last value counts (the lists: both are applied).  An unknown
 * warning name or an empty item is AB3ARGS_FAIL.
 *
 * @param argc  the number of arguments with the program's name
 * @param argv  the arguments; argv[0] is the program's name
 * @param job   the job: initialised (magic, defaults) and filled
 * @param msg   the message for AB3ARGS_UNSUPPORTED and AB3ARGS_FAIL (one
 *              line without LF; "" otherwise)
 * @param cap   the size of msg in bytes (> 0)
 * @return an enum ab3args_result
 */
int ab3args_parse(int argc, char *const argv[], struct ab3job *job, char *msg, size_t cap);

/**
 * @brief The return code that goes with a result of ab3args_parse.
 * @param result  an enum ab3args_result
 * @return AB3JOB_RC_OK for AB3ARGS_HELP, AB3JOB_RC_FAIL for the errors, -1
 *         for AB3ARGS_EDITOR and AB3ARGS_COMPILE (not the end of the run)
 */
int ab3args_rc(int result);

#ifdef __cplusplus
}
#endif

#endif
