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
 * The configuration is the machine's template (embedded: default.uae for
 * the A1200, a500.uae for the A500 with Kickstart 1.3; a machine may change
 * keys of its template, e.g. a500-68010) with the changes of the test: a
 * change replaces every line of its key (value NULL removes them).
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
 * At most AMIGA_EMULATION_MAX runs (0 = no limit) go at the same time on
 * the machine, over all runners, build directories and worktrees:
 * amiga_emulation_start first takes one of the slots (flock on
 * slot-K.lock in $XDG_RUNTIME_DIR/amiga-emulation-slots, else
 * /tmp/amiga-emulation-slots-UID) and waits for a free one; the slot is
 * held until amiga_emulation_free (the kernel frees it if the runner
 * dies).  The wait does not count into the run's timeout.  Without
 * AMIGA_EMULATION_MAX in the environment, the limit is the number in the
 * file max of the slot directory (1 to 64, e.g. `echo 6 > .../max`), else
 * 4; it is read again on every try of a waiting run, so a change takes
 * effect for the runs that already wait.  The limit is hard: every
 * session holds a slot, so no more than the limit of Amiberry instances
 * run.  Only a process without a run of its own waits; for a runner that
 * already runs sessions, amiga_emulation_start takes a free slot or fails
 * at once with amiga_emulation_no_slot() set (no deadlock between runners
 * that each hold slots and wait for more).  The runner then ends one of
 * its own sessions (join, free) and starts again; with none of its own
 * left, that start waits as usual.
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
    const char *machine;        /* NULL or "a1200": default.uae; "a500":
                                   a500.uae (68000, Kickstart 1.3);
                                   "a500-68010": a500.uae with a 68010 */
    const struct amiga_emulation_option *changes;   /* to the machine's template */
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
    int snapshot;               /* EXPERIMENT: start from a savestate (below) */
};

/*
 * EXPERIMENT (not used by the tests): snapshot mode, `snapshot` or
 * AMIGA_EMULATION_SNAPSHOT=1.  A session with a slot runs with fixed
 * directories in the slot (slot-K/test for TEST:, slot-K/mnt-I for each
 * writable mount; read-only mounts keep their host path): the library
 * copies the work directory's TEST: and the writable mounts in at the
 * start and back at the end of amiga_emulation_join (Amiberry stores the
 * real host paths of the mounts in the savestate and uses those, not the
 * configuration's, when it restores).  TEST:run.script is then a fixed
 * prologue that writes TEST:ready and waits (a 96-byte 68000/Kickstart 1.x
 * waiter, polling every 2 ticks) for TEST:job.script, the test's script.
 * The first session of a configuration boots cold, saves the state at
 * TEST:ready into slot-K/snap-HASH.uss (HASH over the configuration, the
 * prologue, the waiter and the identity of the ROM, the hardfile and the
 * Amiberry binary) and continues; later sessions start Amiberry with
 * --statefile.  A restore that does not reach the job within
 * SNAP_RESTORE_GRACE seconds is dropped (the state file too) and the
 * session boots cold.  The job begins with SetClock LOAD (the restored
 * system clock is the snapshot's; on the A500 machines a DATE line with
 * the host's time instead: SetClock LOAD takes 1.8 s there).  Without a
 * slot: cold boot as usual.  The slot's directories are emptied after the
 * results are copied back; the snapshots stay (0.6 MB A1200, 76 KB A500).
 */

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

/* starts the run; NULL on error (message in err).  NULL with errno EAGAIN
   and amiga_emulation_no_slot() nonzero: no slot was free and this
   process runs sessions of its own (see above) - not an error; end one of
   them and start again. */
struct amiga_emulation *amiga_emulation_start(const struct amiga_emulation_params *p,
                                              char *err, size_t errlen);

/* nonzero if the last amiga_emulation_start of this process failed only
   because no slot was free (errno alone does not tell: fork can also fail
   with EAGAIN) */
int amiga_emulation_no_slot(void);

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

/* checks the Kickstart ROM file before a run: it exists, and an encrypted
   one (Cloanto, "AMIROMTYPE1") has rom.key in the directory of its real
   path (where Amiberry looks).  0, or -1 with a message in err. */
int amiga_emulation_check_rom(const char *rom, char *err, size_t errlen);

/* whether params.machine may be name (NULL: the default) */
int amiga_emulation_machine_known(const char *name);

/* the slots (see above): takes a free one of max in dir (created if
   missing); with wait, waits for one.  The locked file descriptor, -1 if
   none was free (without wait; errno EAGAIN) or on error; release with
   close().  max 0:
   no limit, returns a descriptor of /dev/null.  max
   AMIGA_EMULATION_SLOTS_AUTO: amiga_emulation_slot_max(dir), read again
   on every try. */
#define AMIGA_EMULATION_SLOTS_AUTO    (~0u)
#define AMIGA_EMULATION_SLOTS_DEFAULT 4u
#define AMIGA_EMULATION_SLOTS_LIMIT   64u
int amiga_emulation_slot(const char *dir, unsigned max, int wait);

/* the limit: $AMIGA_EMULATION_MAX if set (0 = no limit), else the number
   in dir/max (decimal, blanks and newlines around it; 1 to
   AMIGA_EMULATION_SLOTS_LIMIT), else (missing, not a number, 0, too
   large) AMIGA_EMULATION_SLOTS_DEFAULT */
unsigned amiga_emulation_slot_max(const char *dir);

#endif
