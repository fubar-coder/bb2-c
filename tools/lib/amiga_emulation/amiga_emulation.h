/*
 * amiga_emulation.h - run an AmigaDOS script in Amiberry for tests (host,
 * POSIX).
 *
 * Every run is isolated, so runs can go in parallel:
 *   - its own work directory: TEST: (the host directory the test reads),
 *     the generated configuration, Amiberry's XDG directories and log
 *   - its own XDG_RUNTIME_DIR (short path under /tmp: the control socket
 *     $XDG_RUNTIME_DIR/amiberry.sock must fit into sun_path)
 *   - the hardfile read-only (amiga-emulation/README.md): the script may
 *     write only to TEST: or RAM:, a write to SYS: blocks on a requester
 * Without `visible`, SDL gets the offscreen video and the dummy audio
 * driver: no window, no host input.  Amiberry's output goes to
 * amiberry.log in the work directory; with `log` that includes its own
 * log (write_log, e.g. "Illegal bget at ..." with log_illegal_mem).
 *
 * The configuration is default.uae (embedded) with the changes of the
 * test: a change replaces every line of its key (value NULL removes them).
 * @EMULATION_DIR@ stands for the directory with the Kickstart ROM and the
 * hardfiles.  TEST: and the extra mounts are appended.
 *
 * The hardfile's user-startup executes TEST:run.script; the library writes
 * the script with FailAt 1000 in front and `Echo >TEST:run.done` behind it.
 * amiga_emulation_join waits for run.done, then ends Amiberry over the
 * control socket (QUIT); at the time limit it takes a screenshot
 * (timeout.png in the work directory) and kills the process group.
 * On Linux, Amiberry also dies (SIGKILL) when the process that started it
 * ends without amiga_emulation_free, e.g. a crashed test runner.
 *
 * For debugging, from the environment (for every runner):
 *   AMIGA_EMULATION_VISIBLE=1   as `visible`: the window, host input
 *   AMIGA_EMULATION_SHOTS=N     a screenshot every N seconds (0.2 too) while
 *                               amiga_emulation_join waits, to
 *                               shots/NNNN.png in the work directory
 */
#ifndef AMIGA_EMULATION_H
#define AMIGA_EMULATION_H

#include <stddef.h>

struct amiga_emulation_option {
    const char *key;
    const char *value;          /* NULL: remove the key */
};

struct amiga_emulation_mount {
    const char *volume;         /* without colon, e.g. "CD" */
    const char *host_dir;       /* no comma */
    int writable;
};

struct amiga_emulation_params {
    const struct amiga_emulation_option *changes;   /* to default.uae */
    size_t nchanges;
    const struct amiga_emulation_mount *mounts;     /* besides TEST: */
    size_t nmounts;
    const char *script;         /* AmigaDOS script text */
    unsigned timeout;           /* seconds for the whole run, boot included */
    int visible;                /* show the window (debugging) */
    int log;                    /* Amiberry's own log (--log) into amiberry.log,
                                   e.g. for log_illegal_mem */
    const char *emulation_dir;  /* NULL: $AMIGA_EMULATION_DIR, else built in */
    const char *work_dir;       /* NULL: new directory under $TMPDIR, removed by free */
    const char *amiberry;       /* NULL: "amiberry" from PATH */
};

/* the RTG variant: Workbench on uaegfx (the other hardfile) */
extern const struct amiga_emulation_option amiga_emulation_rtg[];
extern const size_t amiga_emulation_rtg_count;

enum amiga_emulation_status {
    AMIGA_EMULATION_DONE,       /* the script ran to its end */
    AMIGA_EMULATION_TIMEOUT,    /* killed at the time limit */
    AMIGA_EMULATION_EXITED,     /* Amiberry ended before the script did */
    AMIGA_EMULATION_FAILED      /* the run could not be handled */
};

struct amiga_emulation;

/* starts the run; NULL on error (message in err) */
struct amiga_emulation *amiga_emulation_start(const struct amiga_emulation_params *p,
                                              char *err, size_t errlen);

/* the host directory mounted as TEST: and the work directory */
const char *amiga_emulation_test_dir(const struct amiga_emulation *e);
const char *amiga_emulation_work_dir(const struct amiga_emulation *e);

/* one command over the control socket: args joined with tabs (Amiberry
   splits at tabs), e.g. {"SCREENSHOT", path, NULL}.  Waits up to 20 s for
   the socket.  0 = reply "OK...", 1 = other reply, -1 = no connection;
   the reply line (without newline) in reply. */
int amiga_emulation_command(struct amiga_emulation *e, const char *const *args,
                            char *reply, size_t len);

/* waits for the end of the run (see above); may be called once */
enum amiga_emulation_status amiga_emulation_join(struct amiga_emulation *e);

/* frees the handle; removes the work directory if the library made it */
void amiga_emulation_free(struct amiga_emulation *e);

/* for tests and tools: the configuration text (malloc'd; NULL with a
   message in err) and the script text run.script gets */
char *amiga_emulation_config(const struct amiga_emulation_params *p, const char *emulation_dir,
                             const char *test_dir, char *err, size_t errlen);
char *amiga_emulation_script(const char *script);

const char *amiga_emulation_status_name(enum amiga_emulation_status s);

#endif
