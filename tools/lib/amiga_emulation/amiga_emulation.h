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
 *
 * Every session has a supervisor: a process amiga_emulation_start forks
 * (no exec; it closes the caller's descriptors from 3 on but its own
 * slot's lock, so it holds no other session's slot) that starts Amiberry
 * and drives the session to its end on its own, whenever the caller joins
 * it: it waits for run.done,
 * then ends Amiberry over the control socket (QUIT); at the time limit it
 * takes a screenshot (timeout.png in the work directory) and kills
 * Amiberry's process group.  While the job runs it asks Amiberry for the
 * emulated CPU's registers once a second (GET_CPU_REGS): a CPU that
 * Amiberry has halted (a double fault, a jump to no memory) never runs
 * again, so the session ends as soon as AMIGA_EMULATION_HALT_SAMPLES
 * answers in a row show it halted (amiga_emulation_cpu_halted), with the
 * screenshot as at the limit.  In snapshot mode (below) it also saves,
 * restores and falls back.  amiga_emulation_join waits for the
 * supervisor's end and takes its result (a page shared with it), so a
 * runner that joins its sessions in start order no longer delays the
 * others: a session waiting for its join has already ended, and its time
 * limit and times (amiga_emulation_stats) end with it.
 * amiga_emulation_free of a session not joined stops it (SIGTERM to the
 * supervisor, SIGKILL after 5 s).  On Linux, the supervisor dies
 * (SIGKILL) when the process that started it ends without
 * amiga_emulation_free, e.g. a crashed test runner, and Amiberry dies
 * with the supervisor.
 *
 * The time limit counts from the start of the Amiberry process that gets
 * the job, the job already in place: for a cold boot TEST:run.script
 * (the boot is included), for a restore SNAP:job.script (the restore is
 * included).  A restart starts it again: a first-save session's restore
 * after the save, the cold boot of a fallback.  A first-save session's
 * cold boot until the save has a limit of the same length from its start.
 * The wait for a slot and the wait for the caller's join do not count.
 *
 * At most AMIGA_EMULATION_MAX runs (0 = no limit) go at the same time on
 * the machine, over all runners, build directories and worktrees:
 * amiga_emulation_start first takes one of the slots (flock on
 * slot-K.lock in $XDG_RUNTIME_DIR/amiga-emulation-slots, else
 * /tmp/amiga-emulation-slots-UID) and waits for a free one.  Once the
 * supervisor runs, only the supervisor holds the slot (the caller closes
 * its copy of the lock): the slot is free again when the session has
 * ended and its slot directory is cleaned up, not only when the caller
 * joins or frees it, so a session that waits for its join (a runner joins
 * in start order) does not keep a slot without an Amiberry.  The kernel
 * frees it if the supervisor dies; the caller then cleans up the slot's
 * directories only if it gets the slot again at once (else another
 * session has it).  The wait does not count into the run's timeout.
 * Without AMIGA_EMULATION_MAX in the environment, the limit is the number in the
 * file max of the slot directory (1 to 64, e.g. `echo 6 > .../max`), else
 * 4; it is read again on every try of a waiting run, so a change takes
 * effect for the runs that already wait.  The limit is hard: every
 * session holds a slot, so no more than the limit of Amiberry instances
 * run.  Only a process without a run of its own waits; for a runner that
 * already runs sessions, amiga_emulation_start takes a free slot or fails
 * at once with amiga_emulation_no_slot() set.  The runner then tries
 * again every AMIGA_EMULATION_SLOT_RETRY_MS and collects (joins, frees)
 * meanwhile those of its own sessions that have ended
 * (amiga_emulation_wait_any), so it gets the next slot that any runner
 * frees, not only one of its own; with none of its own left, the start
 * waits as usual.  The retries cannot deadlock: every session ends at its
 * time limit on its own (its supervisor) and frees its slot then.
 *
 * For debugging, from the environment (for every runner):
 *   AMIGA_EMULATION_VISIBLE=1   as `visible`: the window, host input
 *   AMIGA_EMULATION_SHOTS=N     a screenshot every N seconds (0.2 too) while
 *                               the session runs, to shots/NNNN.png in the
 *                               work directory
 *   AMIGA_EMULATION_TIMEOUT_STATE=1  at the time limit or a halt, before
 *                               Amiberry is killed: the emulated machine's
 *                               state
 *                               (timeout.uss with its configuration
 *                               timeout.uae, for Amiberry to load) and
 *                               timeout.txt: how the session ended
 *                               ("end: timeout" or "end: halt"), whether
 *                               Amiberry still answered, the emulated
 *                               CPU's registers (GET_CPU_REGS) and
 *                               whether the CPU is halted ("cpu state:
 *                               halted", amiga_emulation_cpu_halted
 *                               against the last answer of the
 *                               supervisor's poll: a double bus or
 *                               address error, the program's crash, or
 *                               an emulator failure),
 *                               Amiberry's threads
 *                               with state, CPU ticks in one second and
 *                               kernel wait channel (Linux)
 *   AMIGA_EMULATION_LOG=1       as `log`: Amiberry's own log into
 *                               amiberry.log (e.g. "CPU halted", "JIT:
 *                               Caught SIGSEGV")
 *   AMIGA_EMULATION_COVERAGE=DIR  coverage (below): the coverage files
 *                               of the session go to DIR
 *   AMIGA_EMULATION_CHANGES="key=value;key;..."  changes of the
 *                               configuration on top of the test's, which
 *                               they replace for the same key; a key
 *                               without "=" is removed (e.g.
 *                               "cachesize=0" for a run without JIT).  A
 *                               snapshot is of the configuration with
 *                               them, so they do not replace the usual
 *                               one.  A value cannot contain ";".
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
                                   "a500-68010": a500.uae with a 68010;
                                   "a600": a600.uae (68000, Kickstart 3.2) */
    const struct amiga_emulation_option *changes;   /* to the machine's template */
    size_t nchanges;
    const struct amiga_emulation_mount *mounts;     /* besides TEST: */
    size_t nmounts;
    const char *script;         /* AmigaDOS script text */
    unsigned timeout;           /* seconds for the run, boot or restore included
                                   (from Amiberry's start, see above) */
    int visible;                /* show the window (debugging) */
    int log;                    /* Amiberry's own log (--log) into amiberry.log,
                                   e.g. for log_illegal_mem */
    const char *emulation_dir;  /* NULL: $AMIGA_EMULATION_DIR, else built in */
    const char *work_dir;       /* NULL: new directory under $TMPDIR, removed by free */
    const char *amiberry;       /* NULL: "amiberry" from PATH */
    int cold;                   /* always boot, no snapshot (below) */
};

/*
 * Snapshot mode (the default; off with `cold` or AMIGA_EMULATION_SNAPSHOT=0):
 * a session with a slot starts Amiberry from a savestate of the booted
 * system instead of booting it (about 0.5 s instead of 14 s).  Amiberry
 * keeps the host paths of the directory mounts in the savestate and uses
 * those on a restore, not the configuration's; the session therefore
 * mounts fixed paths of its slot, which are links to the real
 * directories: slot-K/test to TEST:, slot-K/mnt-I to mount I.  An extra
 * volume SNAP: (slot-K/ctl) holds the waiter and the job; a mount of the
 * test must not be called SNAP (the session then boots cold).
 * TEST:run.script is a fixed prologue (1.3 syntax) that writes
 * SNAP:ready and runs SNAP:waitjob (waitjob.s, 68000, dos.library V0)
 * until SNAP:job.script exists; the job writes SNAP:started once TEST:
 * takes a file (TEST:.snapcheck, deleted at once), sets the
 * clock (SetClock LOAD, on the A500 machines a DATE line with the host's
 * time) and runs the test's script.  The first session of a configuration
 * in a slot boots cold, saves the state at SNAP:ready into
 * slot-K/snap-OWNER-ID-KEY.uss (amiga_emulation_snapshot_name: ID the
 * configuration as the session asks for it, KEY a hash of everything the
 * booted state is made from, amiga_emulation_snapshot_key) and goes on;
 * later ones of the same owner and key restore it.  The save removes the
 * owner's snapshot of the same ID with another key: its inputs (ROM,
 * hardfile, template, Amiberry, waiter) have changed.  The owner is the
 * build directory: $AMIGA_EMULATION_OWNER, which CMake sets for every
 * emulator test (without it, e.g. amiga_run by hand: "no owner", hash 0).
 * The snapshots are kept across ctest runs: the state is saved right
 * after the cold boot, before the job exists (no test's state in it) and
 * never again after a job, and the key changes with every input it is
 * made from.  amiga_emulation_snapshot_purge removes an owner's
 * snapshots by hand (amiga_run --purge-snapshots).  A restore that does
 * not reach SNAP:started within 15 s or whose Amiberry ends is dropped
 * and the session boots cold; the state file is removed only if
 * Amiberry ended or booted instead (SNAP:ready: the prologue ran) - a
 * restore can be that slow on a busy host, the next session tries the
 * file again.  A file that does not begin as a savestate ("ASF ") is
 * removed before the restore, the session saves anew.  A slot keeps its AMIGA_EMULATION_SNAP_KEEP most recently
 * used snapshots.  Sessions end with SIGTERM right after run.done; at the
 * end TEST:run.script is the test's script again and the links are
 * removed.  The work directory's snapshot.log tells what the session did.
 * Without slots (AMIGA_EMULATION_MAX=0, no limit): cold boot.
 */
#define AMIGA_EMULATION_SNAP_KEEP 24u

/* the RTG variant: Workbench on uaegfx (the other hardfile) */
extern const struct amiga_emulation_option amiga_emulation_rtg[];
extern const size_t amiga_emulation_rtg_count;

enum amiga_emulation_status {
    AMIGA_EMULATION_DONE,       /* the script ran to its end */
    AMIGA_EMULATION_TIMEOUT,    /* killed at the time limit */
    AMIGA_EMULATION_HALT,       /* killed after the emulated CPU halted
                                   (see above): before the time limit */
    AMIGA_EMULATION_EXITED,     /* Amiberry ended before the script did */
    AMIGA_EMULATION_FAILED      /* the run could not be handled */
};

struct amiga_emulation;

/* starts the run; NULL on error (message in err).  NULL with errno EAGAIN
   and amiga_emulation_no_slot() nonzero: no slot was free and this
   process runs sessions of its own (see above) - not an error; start
   again after AMIGA_EMULATION_SLOT_RETRY_MS or as soon as one of them has
   ended (amiga_emulation_wait_any). */
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

/* waits for the end of the run (the supervisor's, see above) and returns
   its result; again: the same result */
enum amiga_emulation_status amiga_emulation_join(struct amiga_emulation *e);

/* nonzero if the session has ended (amiga_emulation_join returns at
   once), also for NULL; does not wait */
int amiga_emulation_finished(struct amiga_emulation *e);

/* waits up to ms milliseconds until one of the n sessions of e (NULL
   entries are skipped) has ended: the index of the first ended one in e
   (it may have ended before the call), -1 if none has within ms */
int amiga_emulation_wait_any(struct amiga_emulation *const *e, size_t n, unsigned ms);

/* how often a runner whose start found no free slot (amiga_emulation_no_slot)
   tries again, in milliseconds (see above) */
#define AMIGA_EMULATION_SLOT_RETRY_MS 500u

/* what the snapshot mode did for a session */
enum amiga_emulation_snapshot_use {
    AMIGA_EMULATION_SNAP_COLD,          /* booted, no snapshot (cold, off, no slot
                                           directory) */
    AMIGA_EMULATION_SNAP_RESTORED,      /* started from the slot's snapshot */
    AMIGA_EMULATION_SNAP_FIRST_SAVE,    /* booted, saved the snapshot, restored it */
    AMIGA_EMULATION_SNAP_FALLBACK       /* the snapshot did not work (restore or
                                           save failed): booted cold after all */
};

/* the times of a session, for test statistics (the runners' @stat lines,
   tools/lib/teststats/statline.h) */
struct amiga_emulation_stats {
    double wait;                /* seconds amiga_emulation_start waited for a slot */
    double session;             /* seconds from the slot taken to the end of
                                   the session (the supervisor's: Amiberry
                                   ended; until now if it runs), the wait
                                   for the slot and for the join not
                                   included */
    double timed;               /* the part of it counted against the time
                                   limit: from the last (re)start of Amiberry
                                   (a snapshot save or a fallback restarts it)
                                   to the end of the session */
    unsigned timeout;           /* the time limit (seconds) */
    const char *machine;        /* params.machine, "a1200" for the default;
                                   valid as long as the handle */
    enum amiga_emulation_snapshot_use snapshot;
    enum amiga_emulation_status status;     /* of the join; FAILED before it */
};
void amiga_emulation_stats(const struct amiga_emulation *e, struct amiga_emulation_stats *s);
/* "cold", "restored", "first-save", "fallback" */
const char *amiga_emulation_snapshot_use_name(enum amiga_emulation_snapshot_use u);

/* frees the handle; removes the work directory if the library made it */
void amiga_emulation_free(struct amiga_emulation *e);

/* for tests and tools: the configuration text (malloc'd; NULL with a
   message in err), with the changes of $AMIGA_EMULATION_CHANGES, and the
   script text run.script gets */
char *amiga_emulation_config(const struct amiga_emulation_params *p, const char *emulation_dir,
                             const char *test_dir, char *err, size_t errlen);
char *amiga_emulation_script(const char *script);

/* "done", "timeout", "halt", "exited early", "failed" */
const char *amiga_emulation_status_name(enum amiga_emulation_status s);

/* the answers of GET_CPU_REGS ("OK\tD0=...\tPC=...\tSR=...\t...") in a
   row of a halted CPU: the halt (Amiberry's cpu_halt) sets the interrupt
   mask to 7 and the CPU stops, its registers never change again.  A
   running 68k is at mask 7 only for moments (a running program does not
   keep it, the machine would take no interrupts), so: 1 if both answers
   are OK, equal and have an SR with interrupt mask 7, in user or
   supervisor mode; 0 otherwise (also for NULL, a missing or broken SR) */
int amiga_emulation_cpu_halted(const char *before, const char *after);
/* the answers in a row, a second apart, that end a session as halted */
#define AMIGA_EMULATION_HALT_SAMPLES 3u

/*
 * Coverage: the coverage variants of the programs (preset amiga-cov,
 * shared/cov/cov.h) add the functions they ran to TEST:cov/PROGRAM.cov if
 * TEST:cov exists.  With $AMIGA_EMULATION_COVERAGE (CMake sets it for the
 * emulator tests of the coverage configuration: the test's binary
 * directory/coverage/TEST NAME), every session gets TEST:cov at its start,
 * and its join moves the .cov files of TEST:cov to that directory, each
 * merged with the file of the same name there (another session of the
 * same test), and removes TEST:cov if it is empty then.  A failure is
 * reported on stderr; it does not change the session's result.
 */
/* OR the bitmap of old (oldn bytes, NULL: none) into cur, a coverage file
   of curn bytes (shared/cov/covfmt.h): -1 cur is none (wrong magic,
   version or length); 0 cur stays as it is (no old, old is none or of
   another build: the header differs); 1 merged */
int amiga_emulation_cov_merge(unsigned char *cur, size_t curn, const unsigned char *old,
                              size_t oldn);
/* creates test_dir/cov; 0 = ok (also if it exists), -1 = error */
int amiga_emulation_cov_prepare(const char *test_dir);
/* moves the .cov files of test_dir/cov into dest_dir (created), merged as
   above with the file there (an flock on it while merging), and removes
   test_dir/cov if it is empty then.  A file that is no coverage file stays where it
   is.  The number of files moved; -1 on an error (message in err; the
   other files are still moved) */
int amiga_emulation_cov_collect(const char *test_dir, const char *dest_dir, char *err,
                                size_t errlen);

/* for tests: the configuration of the snapshot mode for slot directory
   slot_dir (TEST: slot_dir/test, mount I slot_dir/mnt-I, SNAP: slot_dir/
   ctl; the Kickstart ROM and the hardfile by their real paths if they
   exist); NULL with a message in err, also if a mount is SNAP */
char *amiga_emulation_snapshot_config(const struct amiga_emulation_params *p,
                                      const char *emulation_dir, const char *slot_dir, char *err,
                                      size_t errlen);
/* the key of a snapshot: a hash of what the booted state is made from -
   the version of the library's way of making it, the machine (NULL:
   "a1200"), that configuration with the slot's paths normalised away
   (slot_dir/ in it stands for any slot, so neither the slot nor the
   session's TEST: and mounts change the key, but every other line does:
   CPU, memory, chipset, ROM, ...), the prologue, the waiter, and the
   identity (real path, device, inode, size, modification time) of every
   medium of the configuration (Kickstart ROM and the other files of
   kickstart_ext_rom_file, cart_file, flash_file, floppyN, cdimageN, the
   image of every hardfile2 line), of rom.key beside the ROM and of the
   Amiberry program (amiberry: NULL for "amiberry" from PATH).  A missing
   file is an identity of its own. */
unsigned long long amiga_emulation_snapshot_key(const char *cfg, const char *slot_dir,
                                                const char *machine, const char *amiberry);
/* what the snapshot is a state of, as the session asks for it: the
   machine, the changes and the mounts' volumes and access (not their
   directories).  Of one owner, a slot keeps one snapshot per id: a save
   removes the owner's snapshots of the same id with another key
   (amiga_emulation_snapshot_drop_stale), the states of inputs that have
   changed since. */
unsigned long long amiga_emulation_snapshot_config_id(const struct amiga_emulation_params *p);
/* keeps the keep most recently used snap-*.uss of slot_dir (by
   modification time; a restore touches its file) and removes the rest and
   left-over save-*.uss; the number removed, -1 without the directory */
int amiga_emulation_snapshot_prune(const char *slot_dir, unsigned keep);
/* the hash of a snapshot owner (a build directory, by its real path if it
   exists); 0 for NULL or "" (no owner), never 0 otherwise */
unsigned long long amiga_emulation_snapshot_owner(const char *owner);
/* the file name of the snapshot with configuration id and key of owner
   (NULL: no owner): "snap-OWNERHASH-ID-KEY.uss", 16 hex digits each;
   malloc'd, NULL without memory */
char *amiga_emulation_snapshot_name(const char *owner, unsigned long long id,
                                    unsigned long long key);
/* removes from slot_dir the snapshots of owner with configuration id and
   another key than key, and the owner's snapshots of the older names
   without id (snap-OWNERHASH-KEY.uss); other owners' files, other ids
   and the snapshot with key stay.  The number removed, -1 without the
   directory. */
int amiga_emulation_snapshot_drop_stale(const char *slot_dir, const char *owner,
                                        unsigned long long id, unsigned long long key);
/* removes the snapshots of owner (snap-OWNERHASH-*.uss and left-over
   save-OWNERHASH-*.uss) from every slot-K of the slot directory
   ($XDG_RUNTIME_DIR/amiga-emulation-slots, see above) and touches nothing
   else: the number removed, -1 without the slot directory.  For manual
   use (amiga_run --purge-snapshots); the tests do not need it.  It takes
   no slot locks: a session of another owner never uses these files; a
   session of the same owner that runs meanwhile and whose snapshot goes
   away falls back to a cold boot. */
int amiga_emulation_snapshot_purge(const char *owner);

/* checks the Kickstart ROM file before a run: it exists, and an encrypted
   one (Cloanto, "AMIROMTYPE1") has rom.key in the directory of its real
   path (where Amiberry looks).  0, or -1 with a message in err. */
int amiga_emulation_check_rom(const char *rom, char *err, size_t errlen);

/* whether params.machine may be name (NULL: the default) */
int amiga_emulation_machine_known(const char *name);

/* the version of the Kickstart (exec) of machine name (NULL: the
   default), e.g. 34 = 1.3, 47 = 3.2; 0 = unknown machine.  Below 36 the
   scripts need AmigaDOS 1.3 syntax (amiga-emulation/README.md, "A500") */
int amiga_emulation_machine_kickstart(const char *name);

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
