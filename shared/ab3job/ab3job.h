/*
 * ab3job.h - a compile from the shell with blitz2-ab3 (spec
 * ab3-compat/command-line, docs/ab3/variant.md): the command line, modelled
 * on AmiBlitz3's CLI mode, and the job blitz2-ab3 hands to its headless
 * editor ab3cli.
 *
 *   blitz2-ab3 -s SOURCE -e EXE [-r] [-mode ab3|bb2]
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

/** @brief The return codes of a command line run (design D6). */
enum ab3job_rc {
    AB3JOB_RC_OK = 0,           /* compiled (or -h) */
    AB3JOB_RC_ERROR = 10,       /* compile error */
    AB3JOB_RC_FAIL = 20         /* argument or setup error */
};

/**
 * @brief The job of a command line run.
 *
 * Filled by ab3args_parse (force, release_exe, source, exe), by blitz2-ab3
 * (mode) and by ab3cli (started, result, tokfile).
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
       d1 = their length; d0 = 0: the file cannot be read (reported) */
    void *tokfile;
    char source[AB3JOB_PATHLEN];        /* -s */
    char exe[AB3JOB_PATHLEN];           /* -e */
};

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
 * -s/-source FILE, -e/-exe FILE, -r/-release, -mode ab3|bb2 (this
 * repository's), -h/-help.  The other options of AmiBlitz3's CLI mode
 * (-d/-debug, -run, -dump, -setcom, -setdbg, -strbufsize, -stacksize,
 * -maxerrors, -1/-1pass, -dform, -PED, -save, -v/-verbose) are refused.
 * The first problem decides; -h before it gives AB3ARGS_HELP.  Given
 * twice, the last value counts.
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
