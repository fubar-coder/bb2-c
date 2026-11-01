/*
 * amiga_emulation.c - run an AmigaDOS script in Amiberry for tests, see
 * amiga_emulation.h.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp, nftw, setenv, kill, setpgid */
#define _DEFAULT_SOURCE         /* MAP_ANONYMOUS, syscall */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#include <sys/syscall.h>
#endif
#include "amiga_emulation.h"

#ifndef AMIGA_EMULATION_DIR_DEFAULT
#define AMIGA_EMULATION_DIR_DEFAULT "amiga-emulation"
#endif

/* the templates, 0-terminated */
static const char default_uae[] = {
#embed "default.uae"
    , 0
};
static const char a500_uae[] = {
#embed "a500.uae"
    , 0
};
static const char a600_uae[] = {
#embed "a600.uae"
    , 0
};

static const struct amiga_emulation_option a500_68010[] = {
    { "cpu_type", "68010" },
    { "cpu_model", "68010" },
};

/* the machines (amiga_emulation.h); the first is the default */
static const struct machine {
    const char *name;
    const char *template_text;
    const struct amiga_emulation_option *changes;   /* to the template */
    size_t nchanges;
    int kickstart;                                  /* exec's version */
} machines[] = {
    { "a1200", default_uae, NULL, 0, 47 },
    { "a500", a500_uae, NULL, 0, 34 },
    { "a500-68010", a500_uae, a500_68010, sizeof a500_68010 / sizeof a500_68010[0], 34 },
    { "a600", a600_uae, NULL, 0, 47 },
};

static const struct machine *find_machine(const char *name)
{
    size_t i;

    if (!name)
        return machines;
    for (i = 0; i < sizeof machines / sizeof machines[0]; i++)
        if (!strcmp(machines[i].name, name))
            return machines + i;
    return NULL;
}

int amiga_emulation_machine_known(const char *name)
{
    return find_machine(name) != NULL;
}

int amiga_emulation_machine_kickstart(const char *name)
{
    const struct machine *m = find_machine(name);

    return m ? m->kickstart : 0;
}

const struct amiga_emulation_option amiga_emulation_rtg[] = {
    { "gfxcard_size", "16" },
    { "gfxcard_type", "ZorroIII" },
    { "hardfile2", "ro,DH0:@EMULATION_DIR@/Workbench_OS3.2-AB3.hdf,32,1,2,512,0,,uae0" },
    { "uaehf0", "hdf,ro,DH0:@EMULATION_DIR@/Workbench_OS3.2-AB3.hdf,32,1,2,512,0,,uae0" },
};
const size_t amiga_emulation_rtg_count = sizeof amiga_emulation_rtg / sizeof amiga_emulation_rtg[0];

#define DONE_FILE   "run.done"
/* After TEST:run.done the results are on the host already (the TEST:
   directory is the host's): Amiberry gets QUIT and a short grace, then the
   signals.  Amiberry 8.4 acknowledges QUIT ("IPC: Received QUIT") but often
   does not end within 10 s, which cost every session about 15 s. */
#define QUIT_GRACE  1           /* seconds between QUIT and SIGTERM */
#define KILL_GRACE  1           /* seconds between SIGTERM and SIGKILL */
#define SUP_STOP_GRACE 5        /* seconds amiga_emulation_free gives a supervisor */

/* What a session's supervisor (the process that drives the session, see
   amiga_emulation_start) shares with the caller: a MAP_SHARED page.  Times
   in microseconds of CLOCK_MONOTONIC (the same clock in every process). */
struct shared {
    atomic_int finished;        /* the supervisor has ended the session */
    atomic_int status;          /* enum amiga_emulation_status */
    atomic_int snap_use;        /* enum amiga_emulation_snapshot_use */
    atomic_int pgid;            /* the process group of the running Amiberry, 0: none */
    atomic_llong launched;      /* the last start of Amiberry, 0: none yet */
    atomic_llong end;           /* the end of the session */
};
/* lock-free, else the atomics would not work between processes */
static_assert(ATOMIC_INT_LOCK_FREE == 2 && ATOMIC_LLONG_LOCK_FREE == 2,
              "amiga_emulation: the shared state needs lock-free atomics");

struct amiga_emulation {
    pid_t pid;                  /* Amiberry, also its process group (the supervisor's) */
    pid_t sup;                  /* the supervisor (the caller's handle), 0 in the supervisor */
    int sup_reaped;             /* the caller has reaped the supervisor */
    struct shared *sh;          /* NULL before the supervisor's start */
    double deadline;            /* monotonic seconds */
    char *work_dir;
    char *test_dir;
    char *runtime_dir;          /* XDG_RUNTIME_DIR */
    char *socket_path;
    int own_work_dir;
    int joined;
    int exited;                 /* waitpid has reaped the process */
    int slot;                   /* the locked slot file, -1: none; the caller
                                   holds it until the supervisor runs, then
                                   only the supervisor */
    char *slot_lock;            /* its path, NULL without a slot index */
    int counted;                /* counted in active_runs */
    double shots;               /* $AMIGA_EMULATION_SHOTS: seconds between screenshots */
    unsigned nshots;
    double next_shot;
    enum amiga_emulation_status status;
    /* snapshot mode (amiga_emulation.h) */
    enum { SNAP_OFF, SNAP_CREATING, SNAP_RESTORING, SNAP_RUNNING } snap;
    char *slot_dir;             /* slot-K */
    char *ctl_dir;              /* slot-K/ctl, SNAP: */
    char *snap_path;            /* slot-K/snap-OWNER-ID-KEY.uss */
    unsigned long long snap_key, snap_owner, snap_id;
    char *real_test_dir;        /* the real paths of TEST: and the mounts */
    char **mount_dirs;
    size_t nmounts;
    char *cold_cfg;             /* the configuration without snapshot */
    char *job;                  /* the test's script (run.script without snapshot) */
    double ready_at, restore_until;
    unsigned timeout;
    const char *amiberry;
    int visible, log_on;
    int date_from_host;         /* the clock by DATE (Kickstart 1.3) */
    pid_t parent;
    double t0;                  /* the start, for snapshot.log */
    /* amiga_emulation_stats */
    double slot_from, slot_got, end_at;
    enum amiga_emulation_snapshot_use snap_use;
    char *machine;
};

/* ------------------------------------------------------------- helpers */

static double now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void sleep_ms(long ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };

    nanosleep(&ts, NULL);
}

static char *str_printf(const char *fmt, ...)
{
    va_list ap;
    int n;
    char *s;

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

static void set_err(char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
}

/* growing string */
struct sb {
    char *p;
    size_t n, cap;
    int bad;
};

static void sb_add(struct sb *b, const char *s, size_t n)
{
    if (b->bad)
        return;
    if (b->n + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 4096;
        char *p;

        while (cap < b->n + n + 1)
            cap *= 2;
        if (!(p = realloc(b->p, cap))) {
            b->bad = 1;
            return;
        }
        b->p = p;
        b->cap = cap;
    }
    memcpy(b->p + b->n, s, n);
    b->n += n;
    b->p[b->n] = 0;
}

static void sb_str(struct sb *b, const char *s)
{
    sb_add(b, s, strlen(s));
}

/* s (n bytes) with @EMULATION_DIR@ replaced */
static void sb_subst(struct sb *b, const char *s, size_t n, const char *emulation_dir)
{
    static const char ph[] = "@EMULATION_DIR@";
    const size_t phn = sizeof ph - 1;
    size_t i = 0, start = 0;

    while (i + phn <= n) {
        if (!memcmp(s + i, ph, phn)) {
            sb_add(b, s + start, i - start);
            sb_str(b, emulation_dir);
            i += phn;
            start = i;
        } else {
            i++;
        }
    }
    sb_add(b, s + start, n - start);
}

static int has_char(const char *s, const char *chars)
{
    return strpbrk(s, chars) != NULL;
}

/* an environment switch: set, not empty and not "0" */
static int env_on(const char *name)
{
    const char *v = getenv(name);

    return v && *v && strcmp(v, "0");
}

static int file_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

static int rm_entry(const char *path, const struct stat *st, int flag, struct FTW *ftw)
{
    (void)st; (void)flag; (void)ftw;
    remove(path);
    return 0;
}

static void rm_tree(const char *dir)
{
    if (dir)
        nftw(dir, rm_entry, 16, FTW_DEPTH | FTW_PHYS);
}

static int mkdirs(const char *base, const char *const *subs)
{
    for (; *subs; subs++) {
        char *d = str_printf("%s/%s", base, *subs);
        int r = d ? mkdir(d, 0700) : -1;

        if (r && errno != EEXIST) {
            free(d);
            return -1;
        }
        free(d);
    }
    return 0;
}

static int write_file(const char *path, const char *text)
{
    FILE *f = fopen(path, "w");

    if (!f)
        return -1;
    if (fputs(text, f) == EOF) {
        fclose(f);
        return -1;
    }
    return fclose(f) ? -1 : 0;
}

/* the value of `key=` in the configuration (malloc'd), NULL if missing */
static char *config_value(const char *cfg, const char *key)
{
    size_t kn = strlen(key);
    const char *p = cfg;

    while (*p) {
        const char *e = strchr(p, '\n');
        size_t n = e ? (size_t)(e - p) : strlen(p);

        if (n > kn && !strncmp(p, key, kn) && p[kn] == '=') {
            char *v = malloc(n - kn);

            if (v) {
                memcpy(v, p + kn + 1, n - kn - 1);
                v[n - kn - 1] = 0;
            }
            return v;
        }
        p += n + (e != NULL);
    }
    return NULL;
}

/* ------------------------------------------------------ configuration */

/* whether one of the n changes has the key key[0..len) */
static int has_key(const struct amiga_emulation_option *ch, size_t n, const char *key, size_t len)
{
    size_t i;

    for (i = 0; i < n; i++)
        if (strlen(ch[i].key) == len && !strncmp(key, ch[i].key, len))
            return 1;
    return 0;
}

char *amiga_emulation_config(const struct amiga_emulation_params *p, const char *emulation_dir,
                             const char *test_dir, char *err, size_t errlen)
{
    struct sb b = { 0 };
    const struct machine *m = find_machine(p->machine);
    const char *line;
    size_t i;

    if (!m) {
        set_err(err, errlen, "unknown machine \"%s\"", p->machine);
        return NULL;
    }
    line = m->template_text;
    if (has_char(test_dir, ",\n") || has_char(emulation_dir, ",\n")) {
        set_err(err, errlen, "directories must not contain commas or newlines");
        return NULL;
    }
    for (i = 0; i < p->nmounts; i++)
        if (!p->mounts[i].volume || !*p->mounts[i].volume || has_char(p->mounts[i].volume, ":,\n")
            || !p->mounts[i].host_dir || has_char(p->mounts[i].host_dir, ",\n")
            || !strcmp(p->mounts[i].volume, "TEST")) {
            set_err(err, errlen, "bad mount %zu (volume without colon/comma, not TEST; directory without comma)", i);
            return NULL;
        }

    /* the template without the lines of changed keys */
    while (*line) {
        const char *end = strchr(line, '\n');
        size_t n = end ? (size_t)(end - line) : strlen(line);
        const char *eq = memchr(line, '=', n);
        int skip = 0;

        if (eq && *line != ';')
            skip = has_key(p->changes, p->nchanges, line, (size_t)(eq - line))
                   || has_key(m->changes, m->nchanges, line, (size_t)(eq - line));
        if (!skip) {
            sb_subst(&b, line, n, emulation_dir);
            sb_str(&b, "\n");
        }
        line += n + (end != NULL);
    }
    /* the machine's changes, unless the test changes the key itself */
    for (i = 0; i < m->nchanges; i++)
        if (!has_key(p->changes, p->nchanges, m->changes[i].key, strlen(m->changes[i].key))) {
            sb_str(&b, m->changes[i].key);
            sb_str(&b, "=");
            sb_str(&b, m->changes[i].value);
            sb_str(&b, "\n");
        }
    /* changed values */
    for (i = 0; i < p->nchanges; i++) {
        if (!p->changes[i].value)
            continue;
        if (has_char(p->changes[i].key, "=\n") || has_char(p->changes[i].value, "\n")) {
            set_err(err, errlen, "bad change \"%s\"", p->changes[i].key);
            free(b.p);
            return NULL;
        }
        sb_str(&b, p->changes[i].key);
        sb_str(&b, "=");
        sb_subst(&b, p->changes[i].value, strlen(p->changes[i].value), emulation_dir);
        sb_str(&b, "\n");
    }
    /* TEST: (DH1) and the extra mounts (DH2...); DH0 is the hardfile */
    for (i = 0; i <= p->nmounts; i++) {
        const char *vol = i ? p->mounts[i - 1].volume : "TEST";
        const char *dir = i ? p->mounts[i - 1].host_dir : test_dir;
        const char *rw = !i || p->mounts[i - 1].writable ? "rw" : "ro";
        char *l = str_printf("filesystem2=%s,DH%zu:%s:%s,0\nuaehf%zu=dir,%s,DH%zu:%s:%s,0\n",
                             rw, i + 1, vol, dir, i + 1, rw, i + 1, vol, dir);

        if (!l) {
            b.bad = 1;
            break;
        }
        sb_str(&b, l);
        free(l);
    }
    if (b.bad) {
        set_err(err, errlen, "out of memory");
        free(b.p);
        return NULL;
    }
    return b.p;
}

char *amiga_emulation_script(const char *script)
{
    size_t n = script ? strlen(script) : 0;

    return str_printf("; generated by amiga_emulation - TEST:%s marks the end\n"
                      "FailAt 1000\n%s%s"
                      "Echo >TEST:%s \"done\"\n",
                      DONE_FILE, script ? script : "", n && script[n - 1] != '\n' ? "\n" : "",
                      DONE_FILE);
}

const char *amiga_emulation_status_name(enum amiga_emulation_status s)
{
    switch (s) {
    case AMIGA_EMULATION_DONE: return "done";
    case AMIGA_EMULATION_TIMEOUT: return "timeout";
    case AMIGA_EMULATION_EXITED: return "exited early";
    default: return "failed";
    }
}

/* ----------------------------------------------------------------- run */

/* the files the configuration needs: Kickstart ROM and hardfile */
int amiga_emulation_check_rom(const char *rom, char *err, size_t errlen)
{
    static const char magic[] = "AMIROMTYPE1";
    char head[sizeof magic - 1], *real, *slash, *key;
    FILE *f = fopen(rom, "rb");
    int encrypted, r = 0;

    if (!f) {
        set_err(err, errlen, "Kickstart ROM %s missing (see amiga-emulation/README.md)", rom);
        return -1;
    }
    encrypted = fread(head, 1, sizeof head, f) == sizeof head && !memcmp(head, magic, sizeof head);
    fclose(f);
    if (!encrypted)
        return 0;
    /* Amiberry looks for rom.key in the directory of the ROM's real path */
    real = realpath(rom, NULL);
    slash = real ? strrchr(real, '/') : NULL;
    key = slash ? str_printf("%.*s/rom.key", (int)(slash - real), real) : NULL;
    if (!key || !file_exists(key)) {
        set_err(err, errlen, "Kickstart ROM %s is encrypted (Cloanto), rom.key missing beside %s "
                "(see amiga-emulation/README.md)", rom, real ? real : rom);
        r = -1;
    }
    free(key);
    free(real);
    return r;
}

static int check_files(const char *cfg, char *err, size_t errlen)
{
    char *rom = config_value(cfg, "kickstart_rom_file");
    char *hdf = config_value(cfg, "hardfile2");
    char *path = NULL;
    int bad = 0;

    if (rom && *rom && amiga_emulation_check_rom(rom, err, errlen)) {
        bad = 1;
    } else if (hdf) {
        char *p = strstr(hdf, "DH0:"), *c;

        if (p && (c = strchr(p + 4, ',')) != NULL) {
            path = strndup(p + 4, (size_t)(c - p - 4));
            if (path && !file_exists(path)) {
                set_err(err, errlen, "hardfile %s missing (see amiga-emulation/README.md)", path);
                bad = 1;
            }
        }
    }
    free(rom);
    free(hdf);
    free(path);
    return bad ? -1 : 0;
}

/* XDG_RUNTIME_DIR: short, the socket path must fit into sun_path */
static char *make_runtime_dir(void)
{
    struct sockaddr_un sa;
    const char *tmp = getenv("TMPDIR");
    char *d;

    if (!tmp || strlen(tmp) + 40 > sizeof sa.sun_path)
        tmp = "/tmp";
    d = str_printf("%s/amiemu-XXXXXX", tmp);
    if (d && !mkdtemp(d)) {
        free(d);
        d = NULL;
    }
    return d;
}

static void child(const char *work_dir, const char *runtime_dir, const char *amiberry, int visible,
                  int log_on, const char *statefile, pid_t parent)
{
    static const char *const xdg[][2] = {
        { "XDG_CONFIG_HOME", "xdg/config" }, { "XDG_DATA_HOME", "xdg/data" },
        { "XDG_STATE_HOME", "xdg/state" }, { "XDG_CACHE_HOME", "xdg/cache" },
    };
    size_t i;
    int log;

    setpgid(0, 0);
#ifdef __linux__
    /* the emulator dies with the process that started it (a crashed or
       killed runner would leave it running in its own group) */
    if (prctl(PR_SET_PDEATHSIG, SIGKILL) || getppid() != parent)
        _exit(126);
#endif
    if (chdir(work_dir))
        _exit(126);
    /* the supervisor ignores SIGPIPE; Amiberry gets the default */
    signal(SIGPIPE, SIG_DFL);
    /* appended: a session can start Amiberry twice (snapshot mode) */
    log = open("amiberry.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log >= 0) {
        dup2(log, 1);
        dup2(log, 2);
        close(log);
    }
    for (i = 0; i < sizeof xdg / sizeof xdg[0]; i++) {
        char *v = str_printf("%s/%s", work_dir, xdg[i][1]);

        if (v)
            setenv(xdg[i][0], v, 1);
    }
    setenv("XDG_RUNTIME_DIR", runtime_dir, 1);
    if (!visible) {
        unsetenv("DISPLAY");
        unsetenv("WAYLAND_DISPLAY");
        unsetenv("XDG_ACTIVATION_TOKEN");
        unsetenv("DESKTOP_STARTUP_ID");
        setenv("SDL_VIDEO_DRIVER", "offscreen", 1);
        setenv("SDL_AUDIO_DRIVER", "dummy", 1);
    }
    {
        const char *argv[8];
        int n = 0;

        argv[n++] = amiberry;
        argv[n++] = "--config";
        argv[n++] = "amiberry.uae";
        argv[n++] = "-G";
        if (log_on)
            argv[n++] = "--log";
        if (statefile) {
            argv[n++] = "--statefile";
            argv[n++] = statefile;
        }
        argv[n] = NULL;
        execvp(amiberry, (char *const *)argv);
    }
    _exit(127);
}

unsigned amiga_emulation_slot_max(const char *dir)
{
    const char *env = getenv("AMIGA_EMULATION_MAX");
    char *path, buf[32], *p, *e;
    unsigned long n;
    size_t len = 0;
    FILE *f;

    if (env && *env)
        return (unsigned)strtoul(env, NULL, 10);
    path = str_printf("%s/max", dir);
    f = path ? fopen(path, "r") : NULL;
    free(path);
    if (f) {
        len = fread(buf, 1, sizeof buf - 1, f);
        /* more than a number fits: not a number of slots */
        if (fgetc(f) != EOF)
            len = 0;
        fclose(f);
    }
    buf[len] = 0;
    for (p = buf; *p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'; p++)
        ;
    if (*p < '0' || *p > '9')
        return AMIGA_EMULATION_SLOTS_DEFAULT;
    errno = 0;
    n = strtoul(p, &e, 10);
    while (*e == ' ' || *e == '\t' || *e == '\n' || *e == '\r')
        e++;
    if (errno || *e || !n || n > AMIGA_EMULATION_SLOTS_LIMIT)
        return AMIGA_EMULATION_SLOTS_DEFAULT;
    return (unsigned)n;
}

static int slot_take(const char *dir, unsigned max, int wait, int *index)
{
    unsigned k, m;

    *index = -1;
    if (!max)
        return open("/dev/null", O_RDONLY | O_CLOEXEC);
    if (mkdir(dir, 0700) && errno != EEXIST)
        return -1;
    for (;;) {
        /* the limit of the slot directory: read again on every try, so a
           change also counts for a waiting run */
        m = max == AMIGA_EMULATION_SLOTS_AUTO ? amiga_emulation_slot_max(dir) : max;
        if (!m)
            return open("/dev/null", O_RDONLY | O_CLOEXEC);
        for (k = 0; k < m; k++) {
            char *path = str_printf("%s/slot-%u.lock", dir, k);
            int fd = path ? open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0600) : -1;

            free(path);
            if (fd < 0)
                return -1;
            if (!flock(fd, LOCK_EX | LOCK_NB)) {
                *index = (int)k;
                return fd;
            }
            close(fd);
        }
        if (!wait) {
            errno = EAGAIN;
            return -1;
        }
        sleep_ms(250);
    }
}

int amiga_emulation_slot(const char *dir, unsigned max, int wait)
{
    int index;

    return slot_take(dir, max, wait, &index);
}

/* snapshot mode unless AMIGA_EMULATION_SNAPSHOT is 0 */
static int snapshot_wanted(void)
{
    const char *v = getenv("AMIGA_EMULATION_SNAPSHOT");

    return !v || strcmp(v, "0");
}

/* the runs of this process that are started and not freed yet */
static unsigned active_runs;

/* the last amiga_emulation_start failed only because no slot was free */
static int no_slot;

int amiga_emulation_no_slot(void)
{
    return no_slot;
}

static char *slots_dir(void)
{
    const char *xdg = getenv("XDG_RUNTIME_DIR");

    return xdg && *xdg ? str_printf("%s/amiga-emulation-slots", xdg)
                       : str_printf("/tmp/amiga-emulation-slots-%u", (unsigned)getuid());
}

/* a slot for this run: $AMIGA_EMULATION_MAX of them, else the number in
   the slot directory's file max, else 4 (amiga_emulation_slot_max).  The
   limit is hard: every session holds a slot.  Only a process without a
   run of its own waits: one that already runs sessions (a runner with
   parallel > 1) takes a free slot if there is one, else the start fails
   with no_slot - waiting while holding slots could deadlock two runners
   that wait for each other's slots.  The runner then ends one of its own
   sessions and tries again.  *index: the slot's number, -1 without a
   limit (AMIGA_EMULATION_MAX=0); *lock: the path of its lock file (NULL
   without a slot number, or out of memory) */
static int take_slot(int *index, char **lock, char *err, size_t errlen)
{
    char *dir = slots_dir();
    int fd = dir ? slot_take(dir, AMIGA_EMULATION_SLOTS_AUTO, active_runs == 0, index) : -1;

    *lock = fd >= 0 && *index >= 0 ? str_printf("%s/slot-%d.lock", dir, *index) : NULL;
    if (fd < 0 && active_runs && errno == EAGAIN) {
        no_slot = 1;
        set_err(err, errlen, "no free emulator slot in %s (sessions of this process running: %u)",
                dir, active_runs);
    } else if (fd < 0) {
        set_err(err, errlen, "cannot take an emulator slot in %s: %s", dir ? dir : "",
                strerror(errno));
    }
    free(dir);
    return fd;
}

static int command_wait(struct amiga_emulation *e, const char *const *args, char *reply,
                        size_t len, double wait);
static int start_supervisor(struct amiga_emulation *e, const char *statefile, char *err,
                            size_t errlen);

static long long to_us(double t)
{
    return (long long)(t * 1e6);
}

static double from_us(long long us)
{
    return us / 1e6;
}

/* starts Amiberry (in the supervisor).  The time limit counts from here:
   the job is in place before (TEST:run.script for a cold boot,
   SNAP:job.script for a restore); a first-save session's cold boot until
   the save has a limit of the same length from its own start. */
static int launch(struct amiga_emulation *e, const char *statefile, char *err, size_t errlen)
{
    /* a second start of the session: not the ended one's socket */
    unlink(e->socket_path);
    e->exited = 0;
    e->deadline = now() + e->timeout;
    e->pid = fork();
    if (e->pid < 0) {
        set_err(err, errlen, "fork: %s", strerror(errno));
        return -1;
    }
    if (e->pid == 0)
        child(e->work_dir, e->runtime_dir, e->amiberry, e->visible, e->log_on, statefile, e->parent);
    setpgid(e->pid, e->pid);
    if (e->sh) {
        atomic_store(&e->sh->pgid, (int)e->pid);
        atomic_store(&e->sh->launched, to_us(e->deadline - e->timeout));
    }
    return 0;
}

static void set_use(struct amiga_emulation *e, enum amiga_emulation_snapshot_use u)
{
    e->snap_use = u;
    if (e->sh)
        atomic_store(&e->sh->snap_use, (int)u);
}

/* ------------------------------------------------------- snapshot mode */

#define SNAP_VOLUME         "SNAP"
#define SNAP_READY          "ready"
#define SNAP_STARTED        "started"
#define SNAP_TESTCHECK      ".snapcheck"    /* on TEST:, for a moment */
#define SNAP_JOB            "job.script"
#define SNAP_WAITER         "waitjob"
#define SNAP_LOG            "snapshot.log"  /* in the work directory */
#define SNAP_SAVE_DELAY     0.5     /* seconds between SNAP:ready and SAVESTATE */
#define SNAP_RESTORE_GRACE  6       /* seconds from the start to SNAP:started */
/* The version of how a state is made, in every key: count it up when a
   change here makes the states saved before unfit for a restore and the
   key does not see it otherwise (the configuration, the prologue, the
   waiter and the media are in the key already), e.g. a change of the
   moment of the save or of what the slot's directories hold then. */
#define SNAP_FORMAT         2

/* TEST:run.script of every session in snapshot mode (AmigaDOS 1.3 syntax:
   it also runs under Kickstart 1.3); the state is saved in the waiter */
static const char snap_prologue[] =
    "FAILAT 1000\n"
    "ECHO >" SNAP_VOLUME ":" SNAP_READY " \"ready\"\n"
    SNAP_VOLUME ":" SNAP_WAITER "\n"
    "EXECUTE " SNAP_VOLUME ":" SNAP_JOB "\n";

/* SNAP:waitjob (waitjob.s, assembled by vasm in the build) */
static constexpr unsigned char snap_waiter[] = {
#embed "waitjob"
};

/* its start: the hunk header of one hunk and the code hunk's type */
struct hunk_start {
    unsigned char type0, type1, type2, type3;       /* HUNK_HEADER */
    unsigned char res0, res1, res2, res3;           /* no resident libraries */
    unsigned char num0, num1, num2, num3;           /* hunks */
    unsigned char first0, first1, first2, first3;
    unsigned char last0, last1, last2, last3;
    unsigned char size0, size1, size2, size3;       /* of hunk 0 in longs */
    unsigned char code0, code1, code2, code3;       /* HUNK_CODE */
};
static constexpr struct hunk_start snap_waiter_start = {
#embed "waitjob" limit(28)
};
#define SNAP_BE32(f) ((unsigned long)snap_waiter_start.f##0 << 24 | (unsigned long)snap_waiter_start.f##1 << 16 \
                      | (unsigned long)snap_waiter_start.f##2 << 8 | snap_waiter_start.f##3)
static_assert(SNAP_BE32(type) == 0x3f3, "waitjob: no HUNK_HEADER (not an AmigaDOS executable)");
static_assert(SNAP_BE32(res) == 0 && SNAP_BE32(num) == 1 && SNAP_BE32(first) == 0
              && SNAP_BE32(last) == 0, "waitjob: not exactly one hunk");
static_assert(SNAP_BE32(code) == 0x3e9, "waitjob: the hunk is not HUNK_CODE");
static_assert(SNAP_BE32(size) > 0 && sizeof snap_waiter == 32 + 4 * SNAP_BE32(size) + 4,
              "waitjob: not one code hunk with HUNK_END only (size, relocations, symbols)");

static unsigned long long fnv(unsigned long long h, const void *p, size_t n)
{
    const unsigned char *s = p;

    while (n--) {
        h ^= *s++;
        h *= 0x100000001b3ULL;
    }
    return h;
}

/* the identity of a file into the hash: its real path, device, inode,
   size and modification time (not its content: a hash of the hardfiles,
   64 MB each, would cost every session more than a restore takes; a
   change through the file system changes the time, a copy the inode) */
static unsigned long long fnv_file(unsigned long long h, const char *path)
{
    struct stat st;
    char *real;

    if (path && !stat(path, &st)) {
        long long v[5] = { (long long)st.st_dev, (long long)st.st_ino, (long long)st.st_size,
                           (long long)st.st_mtim.tv_sec, (long long)st.st_mtim.tv_nsec };

        real = realpath(path, NULL);
        if (real)
            h = fnv(h, real, strlen(real) + 1);
        free(real);
        return fnv(h, v, sizeof v);
    }
    return fnv(h, "?", 1);
}

/* the identity of rom.key beside the Kickstart ROM's real path (Amiberry
   decrypts an encrypted ROM with it, amiga_emulation_check_rom) */
static unsigned long long fnv_rom_key(unsigned long long h, const char *cfg)
{
    char *rom = config_value(cfg, "kickstart_rom_file");
    char *real = rom && *rom ? realpath(rom, NULL) : NULL;
    char *slash = real ? strrchr(real, '/') : NULL;
    char *key = slash ? str_printf("%.*s/rom.key", (int)(slash - real), real) : NULL;

    h = fnv_file(h, key);
    free(key);
    free(real);
    free(rom);
    return h;
}

/* the program's path as execvp finds it (malloc'd), NULL if not found */
static char *find_program(const char *name)
{
    const char *path = getenv("PATH"), *p;

    if (strchr(name, '/'))
        return strdup(name);
    for (p = path ? path : ""; *p;) {
        size_t n = strcspn(p, ":");
        char *f = str_printf("%.*s/%s", (int)n, p, name);

        if (f && !access(f, X_OK))
            return f;
        free(f);
        p += n + (p[n] == ':');
    }
    return NULL;
}

static int write_bytes(const char *path, const void *buf, size_t n)
{
    FILE *f = fopen(path, "wb");

    if (!f)
        return -1;
    if (fwrite(buf, 1, n, f) != n) {
        fclose(f);
        return -1;
    }
    return fclose(f) ? -1 : 0;
}

/* removes what path is: a link (not what it points to), a file, a tree */
static void remove_path(const char *path)
{
    struct stat st;

    if (lstat(path, &st))
        return;
    if (S_ISDIR(st.st_mode))
        rm_tree(path);
    else
        unlink(path);
}

/* path as an empty directory of its own (not a link) */
static int fresh_dir(const char *path)
{
    remove_path(path);
    return mkdir(path, 0700);
}

static char *slot_path(const char *slot_dir, const char *name, size_t i)
{
    return i == (size_t)-1 ? str_printf("%s/%s", slot_dir, name)
                           : str_printf("%s/%s-%zu", slot_dir, name, i);
}

/* the keys whose whole value is a medium the boot reads (empty: none) */
static const char *const media_keys[] = {
    "kickstart_rom_file", "kickstart_ext_rom_file", "cart_file", "flash_file",
    "floppy0", "floppy1", "floppy2", "floppy3",
    "cdimage0", "cdimage1", "cdimage2", "cdimage3",
};

/* the media of a configuration: the files of media_keys and the image of
   every hardfile2 line (ACCESS,DEVICE:PATH,...), in the order of the text;
   a NULL-terminated array of malloc'd paths, NULL without memory.  The
   directory mounts (filesystem2) are not media: they are the session's. */
static char **config_media(const char *cfg)
{
    char **v = calloc(1, sizeof *v);
    size_t n = 0;
    const char *p = cfg;

    while (v && *p) {
        const char *e = strchr(p, '\n');
        size_t len = e ? (size_t)(e - p) : strlen(p), i;
        const char *eq = memchr(p, '=', len), *s = NULL, *c;
        size_t sn = 0;

        for (i = 0; eq && i < sizeof media_keys / sizeof media_keys[0]; i++)
            if ((size_t)(eq - p) == strlen(media_keys[i]) && !strncmp(p, media_keys[i], (size_t)(eq - p))) {
                s = eq + 1;
                sn = len - (size_t)(s - p);
                /* cdimageN=PATH,OPTIONS */
                if (!strncmp(p, "cdimage", 7) && (c = memchr(s, ',', sn)) != NULL)
                    sn = (size_t)(c - s);
            }
        if (eq && (size_t)(eq - p) == 9 && !strncmp(p, "hardfile2", 9)
            && (c = memchr(eq, ',', len - (size_t)(eq - p))) != NULL
            && (s = memchr(c, ':', len - (size_t)(c - p))) != NULL) {
            s++;
            sn = strcspn(s, ",\n");
        }
        if (s && sn) {
            char **nv = realloc(v, (n + 2) * sizeof *v);

            if (!nv || !(nv[n] = strndup(s, sn))) {
                v = nv ? nv : v;
                break;
            }
            v = nv;
            v[++n] = NULL;
        }
        p += len + (e != NULL);
    }
    if (v && *p) {
        /* out of memory */
        for (n = 0; v[n]; n++)
            free(v[n]);
        free(v);
        v = NULL;
    }
    return v;
}

static void free_media(char **v)
{
    size_t i;

    for (i = 0; v && v[i]; i++)
        free(v[i]);
    free(v);
}

/* cfg with every from replaced by to (cfg freed); NULL: out of memory */
static char *replace_all(char *cfg, const char *from, const char *to)
{
    struct sb b = { 0 };
    const char *s = cfg, *f;
    size_t n = strlen(from);

    if (!n)
        return cfg;
    while ((f = strstr(s, from)) != NULL) {
        sb_add(&b, s, (size_t)(f - s));
        sb_str(&b, to);
        s = f + n;
    }
    sb_str(&b, s);
    free(cfg);
    if (b.bad) {
        free(b.p);
        return NULL;
    }
    return b.p;
}

/* cfg without its comment lines (cfg freed); NULL: out of memory */
static char *drop_comments(char *cfg)
{
    struct sb b = { 0 };
    const char *s = cfg;

    while (*s) {
        const char *e = strchr(s, '\n');
        size_t n = e ? (size_t)(e - s) + 1 : strlen(s);

        if (*s != ';')
            sb_add(&b, s, n);
        s += n;
    }
    sb_str(&b, "");
    free(cfg);
    if (b.bad) {
        free(b.p);
        return NULL;
    }
    return b.p;
}

/* the media (ROM, hardfiles, ...) by their real paths, without the
   comments (they name the emulation directory): the same files reached
   through other links (amiga-emulation/ of another worktree) give the
   same configuration, so the same key (the state stores the real path of
   the hardfile anyway); the owner in the file name keeps the snapshots of
   different build directories apart */
static char *resolve_files(char *cfg)
{
    char **media, *real;
    size_t i;

    if (!(cfg = drop_comments(cfg)))
        return NULL;
    if (!(media = config_media(cfg))) {
        free(cfg);
        return NULL;
    }
    for (i = 0; media[i]; i++) {
        real = realpath(media[i], NULL);
        if (cfg && real && strcmp(real, media[i]) && !has_char(real, ",\n"))
            cfg = replace_all(cfg, media[i], real);
        free(real);
    }
    free_media(media);
    return cfg;
}

char *amiga_emulation_snapshot_config(const struct amiga_emulation_params *p,
                                      const char *emulation_dir, const char *slot_dir, char *err,
                                      size_t errlen)
{
    struct amiga_emulation_params q = *p;
    struct amiga_emulation_mount *m = calloc(p->nmounts + 1, sizeof *m);
    char **dirs = calloc(p->nmounts + 2, sizeof *dirs);
    char *cfg = NULL;
    size_t i;
    int bad = !m || !dirs;

    for (i = 0; !bad && i < p->nmounts; i++) {
        if (p->mounts[i].volume && !strcasecmp(p->mounts[i].volume, SNAP_VOLUME)) {
            set_err(err, errlen, "mount %zu is " SNAP_VOLUME ":, the snapshot mode's volume", i);
            goto out;
        }
        m[i] = p->mounts[i];
        m[i].host_dir = dirs[i] = slot_path(slot_dir, "mnt", i);
        bad = !dirs[i];
    }
    if (!bad) {
        m[i].volume = SNAP_VOLUME;
        m[i].host_dir = dirs[i] = slot_path(slot_dir, "ctl", (size_t)-1);
        m[i].writable = 1;
        dirs[i + 1] = slot_path(slot_dir, "test", (size_t)-1);
        bad = !dirs[i] || !dirs[i + 1];
    }
    if (bad) {
        set_err(err, errlen, "out of memory");
        goto out;
    }
    q.mounts = m;
    q.nmounts = p->nmounts + 1;
    cfg = amiga_emulation_config(&q, emulation_dir, dirs[p->nmounts + 1], err, errlen);
    if (cfg && !(cfg = resolve_files(cfg)))
        set_err(err, errlen, "out of memory");
out:
    for (i = 0; dirs && i < p->nmounts + 2; i++)
        free(dirs[i]);
    free(dirs);
    free(m);
    return cfg;
}

/* a string with its length (so that "ab","c" and "a","bc" differ) */
static unsigned long long fnv_str(unsigned long long h, const char *s)
{
    size_t n = s ? strlen(s) : (size_t)-1;

    h = fnv(h, &n, sizeof n);
    return s ? fnv(h, s, n) : h;
}

unsigned long long amiga_emulation_snapshot_key(const char *cfg, const char *slot_dir,
                                                const char *machine, const char *amiberry)
{
    unsigned long long h = 0xcbf29ce484222325ULL;
    char *prog = find_program(amiberry ? amiberry : "amiberry"), *slot, *norm, **media;
    size_t i;

    /* the slot's paths (TEST:, the mounts, SNAP:) do not matter: the same
       configuration has the same key in every slot */
    slot = slot_dir && *slot_dir ? str_printf("%s/", slot_dir) : NULL;
    norm = strdup(cfg);
    if (norm && slot)
        norm = replace_all(norm, slot, "@SLOT@/");
    media = config_media(cfg);
    h = fnv(h, &(unsigned){ SNAP_FORMAT }, sizeof(unsigned));
    h = fnv_str(h, machine ? machine : "a1200");
    h = fnv_str(h, norm ? norm : cfg);
    h = fnv(h, snap_prologue, sizeof snap_prologue);
    h = fnv(h, snap_waiter, sizeof snap_waiter);
    for (i = 0; media && media[i]; i++)
        h = fnv_file(h, media[i]);
    if (!media)
        h = fnv(h, "!", 1);
    h = fnv_rom_key(h, cfg);
    h = fnv_file(h, prog);
    free_media(media);
    free(norm);
    free(slot);
    free(prog);
    return h;
}

unsigned long long amiga_emulation_snapshot_config_id(const struct amiga_emulation_params *p)
{
    unsigned long long h = 0xcbf29ce484222325ULL;
    size_t i;

    h = fnv_str(h, p->machine ? p->machine : "a1200");
    h = fnv(h, &p->nchanges, sizeof p->nchanges);
    for (i = 0; i < p->nchanges; i++)
        h = fnv_str(fnv_str(h, p->changes[i].key), p->changes[i].value);
    h = fnv(h, &p->nmounts, sizeof p->nmounts);
    for (i = 0; i < p->nmounts; i++) {
        h = fnv_str(h, p->mounts[i].volume);
        h = fnv(h, p->mounts[i].writable ? "w" : "r", 1);
    }
    return h;
}

struct snap_entry {
    char *path;
    struct timespec mtime;
};

static int snap_newer(const void *a, const void *b)
{
    const struct snap_entry *x = a, *y = b;

    if (x->mtime.tv_sec != y->mtime.tv_sec)
        return x->mtime.tv_sec > y->mtime.tv_sec ? -1 : 1;
    if (x->mtime.tv_nsec != y->mtime.tv_nsec)
        return x->mtime.tv_nsec > y->mtime.tv_nsec ? -1 : 1;
    return strcmp(x->path, y->path);
}

static int has_affixes(const char *name, const char *prefix, const char *suffix)
{
    size_t n = strlen(name), pn = strlen(prefix), sn = strlen(suffix);

    return n > pn + sn && !strncmp(name, prefix, pn) && !strcmp(name + n - sn, suffix);
}

int amiga_emulation_snapshot_prune(const char *slot_dir, unsigned keep)
{
    DIR *d = opendir(slot_dir);
    struct dirent *de;
    struct snap_entry *v = NULL;
    size_t n = 0, cap = 0, i;
    int removed = 0;

    if (!d)
        return -1;
    while ((de = readdir(d)) != NULL) {
        char *path;
        struct stat st;

        if (has_affixes(de->d_name, "save-", ".uss")) {
            /* left over by a session that ended while saving */
            if ((path = str_printf("%s/%s", slot_dir, de->d_name)) && !unlink(path))
                removed++;
            free(path);
            continue;
        }
        if (!has_affixes(de->d_name, "snap-", ".uss"))
            continue;
        path = str_printf("%s/%s", slot_dir, de->d_name);
        if (!path || stat(path, &st) || !S_ISREG(st.st_mode)) {
            free(path);
            continue;
        }
        if (n == cap) {
            struct snap_entry *nv = realloc(v, (cap = cap ? 2 * cap : 16) * sizeof *v);

            if (!nv) {
                free(path);
                break;
            }
            v = nv;
        }
        v[n].path = path;
        v[n++].mtime = st.st_mtim;
    }
    closedir(d);
    if (n)
        qsort(v, n, sizeof *v, snap_newer);
    for (i = 0; i < n; i++) {
        if (i >= keep && !unlink(v[i].path))
            removed++;
        free(v[i].path);
    }
    free(v);
    return removed;
}

unsigned long long amiga_emulation_snapshot_owner(const char *owner)
{
    unsigned long long h;
    char *real;
    const char *s;

    if (!owner || !*owner)
        return 0;
    /* the same directory through another path (a link) is the same owner */
    real = realpath(owner, NULL);
    s = real ? real : owner;
    h = fnv(0xcbf29ce484222325ULL, s, strlen(s));
    free(real);
    return h ? h : 1;   /* 0 is "no owner" */
}

char *amiga_emulation_snapshot_name(const char *owner, unsigned long long id,
                                    unsigned long long key)
{
    return str_printf("snap-%016llx-%016llx-%016llx.uss", amiga_emulation_snapshot_owner(owner), id,
                      key);
}

int amiga_emulation_snapshot_drop_stale(const char *slot_dir, const char *owner,
                                        unsigned long long id, unsigned long long key)
{
    DIR *d = opendir(slot_dir);
    struct dirent *de;
    unsigned long long o = amiga_emulation_snapshot_owner(owner);
    char same[64], current[80], old[32];
    int removed = 0;

    if (!d)
        return -1;
    snprintf(same, sizeof same, "snap-%016llx-%016llx-", o, id);
    snprintf(current, sizeof current, "snap-%016llx-%016llx-%016llx.uss", o, id, key);
    snprintf(old, sizeof old, "snap-%016llx-", o);
    while ((de = readdir(d)) != NULL) {
        size_t n = strlen(de->d_name);
        char *path;
        struct stat st;

        /* this configuration with another key, or the owner's snapshots
           of the names before the configuration id (snap-OWNER-KEY.uss:
           no configuration tells whether they are still current) */
        if (!strcmp(de->d_name, current)
            || !(has_affixes(de->d_name, same, ".uss")
                 || (has_affixes(de->d_name, old, ".uss") && n == strlen(old) + 16 + 4)))
            continue;
        path = str_printf("%s/%s", slot_dir, de->d_name);
        if (path && !lstat(path, &st) && S_ISREG(st.st_mode) && !unlink(path))
            removed++;
        free(path);
    }
    closedir(d);
    return removed;
}

/* the snapshots of the owner with hash o (snap-O-*.uss, left-over
   save-O-*.uss) in one slot directory; the number removed */
static int purge_slot(const char *slot_dir, unsigned long long o)
{
    DIR *d = opendir(slot_dir);
    struct dirent *de;
    char snap[32], save[32];
    int removed = 0;

    if (!d)
        return 0;
    snprintf(snap, sizeof snap, "snap-%016llx-", o);
    snprintf(save, sizeof save, "save-%016llx-", o);
    while ((de = readdir(d)) != NULL) {
        char *path;
        struct stat st;

        if (!has_affixes(de->d_name, snap, ".uss") && !has_affixes(de->d_name, save, ".uss"))
            continue;
        path = str_printf("%s/%s", slot_dir, de->d_name);
        if (path && !lstat(path, &st) && S_ISREG(st.st_mode) && !unlink(path))
            removed++;
        free(path);
    }
    closedir(d);
    return removed;
}

int amiga_emulation_snapshot_purge(const char *owner)
{
    unsigned long long o = amiga_emulation_snapshot_owner(owner);
    char *dir = slots_dir();
    DIR *d = dir ? opendir(dir) : NULL;
    struct dirent *de;
    int removed = 0;

    if (!d) {
        free(dir);
        return -1;
    }
    while ((de = readdir(d)) != NULL) {
        const char *k = de->d_name + 5;
        char *slot;
        struct stat st;

        /* slot-K, K a number (not slot-K.lock) */
        if (strncmp(de->d_name, "slot-", 5) || !*k || strspn(k, "0123456789") != strlen(k))
            continue;
        slot = str_printf("%s/%s", dir, de->d_name);
        if (slot && !lstat(slot, &st) && S_ISDIR(st.st_mode))
            removed += purge_slot(slot, o);
        free(slot);
    }
    closedir(d);
    free(dir);
    return removed;
}

/* one line into the work directory's snapshot.log */
static void snap_log(struct amiga_emulation *e, const char *fmt, ...)
{
    char *path = str_printf("%s/" SNAP_LOG, e->work_dir);
    FILE *f = path ? fopen(path, "a") : NULL;
    va_list ap;

    free(path);
    if (!f)
        return;
    fprintf(f, "%6.2f ", now() - e->t0);
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}

/* SNAP: for a start: empty, with the waiter */
static int snap_prepare(struct amiga_emulation *e)
{
    char *path = str_printf("%s/" SNAP_WAITER, e->ctl_dir);
    int r = !path || fresh_dir(e->ctl_dir) || write_bytes(path, snap_waiter, sizeof snap_waiter);

    free(path);
    return r ? -1 : 0;
}

/* SNAP:job.script (complete before the waiter can see it): the start mark,
   the clock, the test's script */
static int snap_write_job(struct amiga_emulation *e)
{
    char clock[64], *tmp = str_printf("%s/job.tmp", e->ctl_dir);
    char *dst = str_printf("%s/" SNAP_JOB, e->ctl_dir), *text;
    int r;

    if (e->date_from_host) {
        /* Kickstart 1.3: SetClock LOAD takes about 1.8 s there, Date is
           at once (dd-Mon-yy in English, as 1.3 reads it) */
        static const char mon[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        time_t t = time(NULL);
        struct tm tm;

        localtime_r(&t, &tm);
        snprintf(clock, sizeof clock, "DATE %02d-%s-%02d %02d:%02d:%02d\n", tm.tm_mday,
                 mon[tm.tm_mon], tm.tm_year % 100, tm.tm_hour, tm.tm_min, tm.tm_sec);
    } else {
        snprintf(clock, sizeof clock, "SETCLOCK LOAD\n");
    }
    /* SNAP:started only once TEST: takes a file: a restored state whose
       TEST: does not work (seen once, cause unknown) gets no start and
       is dropped (the session boots cold, restore_until).  The test of
       this check names another volume (AMIGA_EMULATION_SNAP_CHECK_VOLUME) */
    const char *vol = getenv("AMIGA_EMULATION_SNAP_CHECK_VOLUME");

    if (!vol || !*vol)
        vol = "TEST";
    text = str_printf("ECHO >%s:" SNAP_TESTCHECK " \"\"\n"
                      "IF EXISTS %s:" SNAP_TESTCHECK "\n"
                      "DELETE %s:" SNAP_TESTCHECK " QUIET\n"
                      "ECHO >" SNAP_VOLUME ":" SNAP_STARTED " \"started\"\n"
                      "ENDIF\n%s%s", vol, vol, vol, clock, e->job);
    r = !tmp || !dst || !text || write_file(tmp, text) || rename(tmp, dst) ? -1 : 0;
    free(tmp);
    free(dst);
    free(text);
    return r;
}

/* SAVESTATE into snap_path (through a temporary name), then the oldest
   snapshots of the slot beyond AMIGA_EMULATION_SNAP_KEEP removed */
static int snap_save(struct amiga_emulation *e)
{
    char *tmp = str_printf("%s/save-%016llx-%016llx-%016llx.uss", e->slot_dir, e->snap_owner,
                           e->snap_id, e->snap_key);
    char *cfg = str_printf("%s/snapshot.uae", e->work_dir);
    const char *cmd[] = { "SAVESTATE", tmp, cfg, NULL };
    char reply[256];
    int saved = 0;

    if (tmp && cfg && !command_wait(e, cmd, reply, sizeof reply, 5)) {
        /* the reply may come before the file is complete: wait until its
           size stays the same for 0.2 s (at most 5 s) */
        double until = now() + 5;
        off_t last = -1;
        struct stat st;

        while (now() < until) {
            sleep_ms(200);
            if (!stat(tmp, &st) && st.st_size > 0 && st.st_size == last)
                break;
            last = stat(tmp, &st) ? -1 : st.st_size;
        }
        saved = !stat(tmp, &st) && st.st_size > 0 && st.st_size == last
                && !rename(tmp, e->snap_path);
    }
    snap_log(e, saved ? "saved %s" : "not saved %s", e->snap_path);
    if (tmp)
        unlink(tmp);
    if (cfg)
        unlink(cfg);
    free(tmp);
    free(cfg);
    if (saved) {
        /* the state this one replaces: the same configuration of the same
           owner from inputs that have changed since */
        int stale = amiga_emulation_snapshot_drop_stale(e->slot_dir, getenv("AMIGA_EMULATION_OWNER"),
                                                        e->snap_id, e->snap_key);

        if (stale > 0)
            snap_log(e, "removed %d stale snapshot%s", stale, stale == 1 ? "" : "s");
        amiga_emulation_snapshot_prune(e->slot_dir, AMIGA_EMULATION_SNAP_KEEP);
    }
    return saved;
}

/* the slot's TEST: and mounts: real empty directories (link 0) or links
   to the session's directories (link 1).  Amiberry resolves a link when
   it mounts, so the session that saves the state has the real
   directories; on a restore it opens the paths of the state, through the
   links. */
static int snap_dirs(struct amiga_emulation *e, int link)
{
    size_t i;

    for (i = 0; i <= e->nmounts; i++) {
        char *path = i ? slot_path(e->slot_dir, "mnt", i - 1) : slot_path(e->slot_dir, "test", (size_t)-1);
        int r;

        remove_path(path);
        r = !path || (link ? symlink(i ? e->mount_dirs[i - 1] : e->real_test_dir, path) : mkdir(path, 0700));
        if (!r && !link && !i) {
            /* the prologue, as the session's TEST: has it */
            char *rs = str_printf("%s/run.script", path);

            r = !rs || write_file(rs, snap_prologue);
            free(rs);
        }
        free(path);
        if (r)
            return -1;
    }
    return 0;
}

/* removes the slot's SNAP:, TEST: and mounts */
static void snap_clear(struct amiga_emulation *e)
{
    size_t i;

    if (!e->slot_dir)
        return;
    if (e->ctl_dir)
        remove_path(e->ctl_dir);
    for (i = 0; i <= e->nmounts; i++) {
        char *path = i ? slot_path(e->slot_dir, "mnt", i - 1) : slot_path(e->slot_dir, "test", (size_t)-1);

        if (path)
            remove_path(path);
        free(path);
    }
}

/* the end: TEST:run.script the test's script again (as without snapshot),
   the slot's directories removed */
static void snap_finish(struct amiga_emulation *e)
{
    char *path = str_printf("%s/run.script", e->test_dir);

    if (path && e->job)
        write_file(path, e->job);
    free(path);
    snap_clear(e);
    e->snap = SNAP_OFF;
}

/* back to a session without snapshot (the configuration, TEST:run.script
   as without it); 0 = done */
static int snap_off(struct amiga_emulation *e)
{
    char *path = str_printf("%s/amiberry.uae", e->work_dir);
    int r = !path || !e->cold_cfg || write_file(path, e->cold_cfg);

    free(path);
    snap_finish(e);
    return r ? -1 : 0;
}

/* snapshot mode for this run, if it can have it: the configuration with
   the slot's paths into *cfg, the slot's directories; 0 with e->snap set,
   -1: boot cold */
static int snap_setup(struct amiga_emulation *e, const struct amiga_emulation_params *p,
                      const char *emulation_dir, int index, char **cfg)
{
    char *dir = slots_dir(), *name, err[256];
    size_t i;

    *cfg = NULL;
    e->slot_dir = dir ? str_printf("%s/slot-%d", dir, index) : NULL;
    free(dir);
    if (!e->slot_dir || (mkdir(e->slot_dir, 0700) && errno != EEXIST)) {
        snap_log(e, "cold: no slot directory");
        return -1;
    }
    e->ctl_dir = slot_path(e->slot_dir, "ctl", (size_t)-1);
    if (!(*cfg = amiga_emulation_snapshot_config(p, emulation_dir, e->slot_dir, err, sizeof err))) {
        snap_log(e, "cold: %s", err);
        return -1;
    }
    e->snap_key = amiga_emulation_snapshot_key(*cfg, e->slot_dir, p->machine, e->amiberry);
    e->snap_id = amiga_emulation_snapshot_config_id(p);
    e->snap_owner = amiga_emulation_snapshot_owner(getenv("AMIGA_EMULATION_OWNER"));
    name = amiga_emulation_snapshot_name(getenv("AMIGA_EMULATION_OWNER"), e->snap_id, e->snap_key);
    e->snap_path = name ? str_printf("%s/%s", e->slot_dir, name) : NULL;
    free(name);
    e->real_test_dir = realpath(e->test_dir, NULL);
    e->mount_dirs = calloc(p->nmounts + 1, sizeof *e->mount_dirs);
    for (i = 0; e->mount_dirs && i < p->nmounts; i++)
        if (!(e->mount_dirs[i] = realpath(p->mounts[i].host_dir, NULL)))
            break;
    e->nmounts = i;
    if (!e->ctl_dir || !e->snap_path || !e->real_test_dir || !e->mount_dirs || i < p->nmounts) {
        snap_log(e, "cold: no slot paths (a mount directory missing?)");
        free(*cfg);
        *cfg = NULL;
        return -1;
    }
    e->snap = file_exists(e->snap_path) ? SNAP_RESTORING : SNAP_CREATING;
    if (snap_dirs(e, e->snap == SNAP_RESTORING) || snap_prepare(e)) {
        snap_log(e, "cold: cannot prepare the slot directory %s", e->slot_dir);
        snap_finish(e);
        free(*cfg);
        *cfg = NULL;
        return -1;
    }
    return 0;
}

static int snap_date_from_host(const struct amiga_emulation_params *p)
{
    const struct machine *m = find_machine(p->machine);

    return m && m->template_text == a500_uae;
}

struct amiga_emulation *amiga_emulation_start(const struct amiga_emulation_params *p,
                                              char *err, size_t errlen)
{
    static const char *const subs[] = { "test", "xdg", "xdg/config", "xdg/data", "xdg/state",
                                        "xdg/cache", NULL };
    const char *emulation_dir = p->emulation_dir ? p->emulation_dir : getenv("AMIGA_EMULATION_DIR");
    struct amiga_emulation *e = calloc(1, sizeof *e);
    char *cfg = NULL, *script = NULL, *path;
    struct sockaddr_un sa;
    const char *statefile = NULL;
    int slot_index = -1;

    no_slot = 0;
    if (!emulation_dir || !*emulation_dir)
        emulation_dir = AMIGA_EMULATION_DIR_DEFAULT;
    if (!e) {
        set_err(err, errlen, "out of memory");
        return NULL;
    }
    e->status = AMIGA_EMULATION_FAILED;
    e->slot = -1;
    if (p->work_dir) {
        e->work_dir = strdup(p->work_dir);
    } else {
        const char *tmp = getenv("TMPDIR");

        e->work_dir = str_printf("%s/amiga-emulation-XXXXXX", tmp && *tmp ? tmp : "/tmp");
        if (e->work_dir && !mkdtemp(e->work_dir)) {
            set_err(err, errlen, "cannot create a work directory: %s", strerror(errno));
            goto fail;
        }
        e->own_work_dir = 1;
    }
    if (!e->work_dir || mkdirs(e->work_dir, subs)) {
        set_err(err, errlen, "cannot create the work directory %s", e->work_dir ? e->work_dir : "");
        goto fail;
    }
    e->test_dir = str_printf("%s/test", e->work_dir);
    e->t0 = now();
    path = str_printf("%s/" SNAP_LOG, e->work_dir);
    if (path)
        unlink(path);
    free(path);
    e->runtime_dir = make_runtime_dir();
    e->socket_path = e->runtime_dir ? str_printf("%s/amiberry.sock", e->runtime_dir) : NULL;
    if (!e->test_dir || !e->socket_path || strlen(e->socket_path) >= sizeof sa.sun_path) {
        set_err(err, errlen, "cannot create the runtime directory");
        goto fail;
    }

    if (!(cfg = amiga_emulation_config(p, emulation_dir, e->test_dir, err, errlen)))
        goto fail;
    if (check_files(cfg, err, errlen))
        goto fail;
    path = str_printf("%s/amiberry.uae", e->work_dir);
    if (!path || write_file(path, cfg)) {
        set_err(err, errlen, "cannot write the configuration");
        free(path);
        goto fail;
    }
    free(path);
    script = amiga_emulation_script(p->script);
    path = str_printf("%s/run.script", e->test_dir);
    if (!script || !path || write_file(path, script)) {
        set_err(err, errlen, "cannot write TEST:run.script");
        free(path);
        goto fail;
    }
    free(path);

    e->slot_from = now();
    if ((e->slot = take_slot(&slot_index, &e->slot_lock, err, errlen)) < 0)
        goto fail;
    if (slot_index >= 0 && !e->slot_lock) {
        set_err(err, errlen, "out of memory");
        goto fail;
    }
    e->slot_got = now();
    e->machine = strdup(p->machine ? p->machine : "a1200");
    active_runs++;
    e->counted = 1;
    e->parent = getpid();
    e->timeout = p->timeout ? p->timeout : 60;
    e->amiberry = p->amiberry ? p->amiberry : "amiberry";
    e->visible = p->visible || env_on("AMIGA_EMULATION_VISIBLE");
    e->log_on = p->log;
    if (!p->cold && snapshot_wanted() && slot_index >= 0) {
        char *scfg = NULL;

        e->date_from_host = snap_date_from_host(p);
        if (!(e->job = strdup(script)) || !(e->cold_cfg = strdup(cfg))) {
            set_err(err, errlen, "out of memory");
            goto fail;
        }
        if (!snap_setup(e, p, emulation_dir, slot_index, &scfg)) {
            char *cp = str_printf("%s/amiberry.uae", e->work_dir);
            char *rp = str_printf("%s/run.script", e->test_dir);
            int bad = !cp || !rp || write_file(cp, scfg) || write_file(rp, snap_prologue);

            free(cp);
            free(rp);
            free(scfg);
            if (bad || (e->snap == SNAP_RESTORING && snap_write_job(e))) {
                set_err(err, errlen, "cannot write the files of the snapshot mode");
                goto fail;
            }
            if (e->snap == SNAP_RESTORING) {
                statefile = e->snap_path;
                set_use(e, AMIGA_EMULATION_SNAP_RESTORED);
                /* the newest use first when the slot's snapshots are pruned */
                utimensat(AT_FDCWD, e->snap_path, NULL, 0);
                e->restore_until = now() + SNAP_RESTORE_GRACE;
                snap_log(e, "restore %s", e->snap_path);
            } else {
                snap_log(e, "boot cold, save %s", e->snap_path);
                set_use(e, AMIGA_EMULATION_SNAP_FIRST_SAVE);
            }
        }
    }
    if (getenv("AMIGA_EMULATION_SHOTS"))
        e->shots = strtod(getenv("AMIGA_EMULATION_SHOTS"), NULL);
    if (!(e->shots > 0))
        e->shots = 0;
    if (e->shots && mkdirs(e->work_dir, (const char *const[]){ "shots", NULL }))
        e->shots = 0;
    e->next_shot = now() + e->shots;
    path = str_printf("%s/amiberry.log", e->work_dir);
    if (path)
        unlink(path);
    free(path);
    if (start_supervisor(e, statefile, err, errlen))
        goto fail;
    free(cfg);
    free(script);
    return e;

fail:
    free(cfg);
    free(script);
    amiga_emulation_free(e);
    if (no_slot)
        errno = EAGAIN;
    return NULL;
}

const char *amiga_emulation_test_dir(const struct amiga_emulation *e)
{
    return e->test_dir;
}

const char *amiga_emulation_work_dir(const struct amiga_emulation *e)
{
    return e->work_dir;
}

/* the caller: reaps the supervisor if it has ended; 1 = ended */
static int sup_ended(struct amiga_emulation *e)
{
    if (!e->sup_reaped && e->sup > 0 && waitpid(e->sup, NULL, WNOHANG) == e->sup)
        e->sup_reaped = 1;
    return e->sup_reaped;
}

/* reaps Amiberry if it has ended; 1 = ended.  On the caller's side: the
   session has ended (the supervisor runs Amiberry). */
static int reaped(struct amiga_emulation *e)
{
    if (e->sup > 0)
        return atomic_load(&e->sh->finished) || sup_ended(e);
    if (!e->exited && e->pid > 0 && waitpid(e->pid, NULL, WNOHANG) == e->pid)
        e->exited = 1;
    return e->exited;
}

static int command_wait(struct amiga_emulation *e, const char *const *args, char *reply,
                        size_t len, double wait)
{
    struct sockaddr_un sa;
    double until = now() + wait;
    struct sb line = { 0 };
    size_t i, got = 0;
    int fd = -1;

    if (len)
        *reply = 0;
    for (i = 0; args[i]; i++) {
        if (i)
            sb_str(&line, "\t");
        sb_str(&line, args[i]);
    }
    sb_str(&line, "\n");
    if (line.bad)
        return -1;

    memset(&sa, 0, sizeof sa);
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, e->socket_path, sizeof sa.sun_path - 1);
    for (;;) {
        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0 && connect(fd, (struct sockaddr *)&sa, sizeof sa) == 0)
            break;
        if (fd >= 0)
            close(fd);
        fd = -1;
        if (reaped(e) || now() > until) {
            free(line.p);
            return -1;
        }
        sleep_ms(100);
    }
    if (write(fd, line.p, line.n) != (ssize_t)line.n) {
        free(line.p);
        close(fd);
        return -1;
    }
    free(line.p);
    /* one reply line */
    until = now() + 10;
    while (len && got < len - 1) {
        struct pollfd pfd = { fd, POLLIN, 0 };
        ssize_t n;

        if (poll(&pfd, 1, 200) <= 0) {
            if (now() > until)
                break;
            continue;
        }
        n = read(fd, reply + got, len - 1 - got);
        if (n <= 0)
            break;
        got += (size_t)n;
        reply[got] = 0;
        if (memchr(reply, '\n', got))
            break;
    }
    close(fd);
    if (len) {
        reply[got] = 0;
        reply[strcspn(reply, "\r\n")] = 0;
    }
    return len && !strncmp(reply, "OK", 2) ? 0 : 1;
}

int amiga_emulation_command(struct amiga_emulation *e, const char *const *args, char *reply,
                            size_t len)
{
    return command_wait(e, args, reply, len, 20);
}

/* SIGTERM to the group, SIGKILL after the grace time */
static void stop_group(struct amiga_emulation *e)
{
    double until = now() + KILL_GRACE;

    if (e->pid <= 0 || e->exited)
        return;
    kill(-e->pid, SIGTERM);
    while (!reaped(e) && now() < until)
        sleep_ms(100);
    if (!e->exited) {
        kill(-e->pid, SIGKILL);
        waitpid(e->pid, NULL, 0);
        e->exited = 1;
    }
}

/* ---------------------------------------------------------- supervisor */

/* SIGTERM to the supervisor: amiga_emulation_free before the end */
static volatile sig_atomic_t stop_asked;

static void on_term(int sig)
{
    (void)sig;
    stop_asked = 1;
}

/* the session in the supervisor, from Amiberry's first start to its end:
   waits for TEST:run.done, saves and restores (snapshot mode), the time
   limit, the screenshots */
static void supervise(struct amiga_emulation *e)
{
    char *done = str_printf("%s/%s", e->test_dir, DONE_FILE);
    double quit_until = 0;

    if (!done) {
        stop_group(e);
        e->status = AMIGA_EMULATION_FAILED;
    }
    while (done) {
        if (stop_asked) {
            if (e->snap != SNAP_OFF)
                snap_log(e, "stopped (amiga_emulation_free)");
            stop_group(e);
            e->status = AMIGA_EMULATION_FAILED;
            break;
        }
        if (e->snap == SNAP_RESTORING) {
            char *started = str_printf("%s/" SNAP_STARTED, e->ctl_dir);
            int ok = started && file_exists(started);

            if (ok) {
                e->snap = SNAP_RUNNING;
                snap_log(e, "job started");
            } else if (reaped(e) || now() > e->restore_until) {
                /* the snapshot does not work: dropped, the session as
                   without snapshot */
                char err[256];

                stop_group(e);
                snap_log(e, "restore failed (%s)%s: dropped, boot cold",
                         e->exited && now() <= e->restore_until ? "Amiberry ended" : "no start",
                         started && file_exists(started) ? ", the job had just started" : "");
                free(started);
                unlink(e->snap_path);
                set_use(e, AMIGA_EMULATION_SNAP_FALLBACK);
                if (snap_off(e) || launch(e, NULL, err, sizeof err)) {
                    e->status = AMIGA_EMULATION_FAILED;
                    break;
                }
                continue;
            }
            free(started);
        }
        if (e->snap == SNAP_CREATING) {
            char *ready = str_printf("%s/" SNAP_READY, e->ctl_dir);

            if (!e->ready_at && ready && file_exists(ready))
                e->ready_at = now() + SNAP_SAVE_DELAY;
            free(ready);
            if (e->ready_at && now() >= e->ready_at && !reaped(e)) {
                /* saved, then restored as every later session (its
                   mounts are the slot's real directories) */
                char err[256];
                int saved = snap_save(e);

                stop_group(e);
                if (saved && !snap_dirs(e, 1) && !snap_prepare(e) && !snap_write_job(e)) {
                    e->snap = SNAP_RESTORING;
                    e->restore_until = now() + SNAP_RESTORE_GRACE;
                    snap_log(e, "restore %s", e->snap_path);
                    if (launch(e, e->snap_path, err, sizeof err)) {
                        e->status = AMIGA_EMULATION_FAILED;
                        break;
                    }
                } else {
                    snap_log(e, "boot cold");
                    set_use(e, AMIGA_EMULATION_SNAP_FALLBACK);
                    if (snap_off(e) || launch(e, NULL, err, sizeof err)) {
                        e->status = AMIGA_EMULATION_FAILED;
                        break;
                    }
                }
                continue;
            }
        }
        if (reaped(e)) {
            e->status = file_exists(done) ? AMIGA_EMULATION_DONE : AMIGA_EMULATION_EXITED;
            break;
        }
        if (e->snap != SNAP_OFF && file_exists(done)) {
            /* the results are on the host, the state is not needed */
            snap_log(e, "done");
            stop_group(e);
            e->status = AMIGA_EMULATION_DONE;
            break;
        }
        if (!quit_until && file_exists(done)) {
            static const char *const quit[] = { "QUIT", NULL };
            char reply[64];

            command_wait(e, quit, reply, sizeof reply, 5);
            quit_until = now() + QUIT_GRACE;
        }
        if (quit_until && now() > quit_until) {
            stop_group(e);
            e->status = AMIGA_EMULATION_DONE;
            break;
        }
        if (e->shots && !quit_until && now() >= e->next_shot) {
            char *png = str_printf("%s/shots/%04u.png", e->work_dir, ++e->nshots);
            const char *shot[] = { "SCREENSHOT", png, NULL };
            char reply[256];

            if (png)
                command_wait(e, shot, reply, sizeof reply, 2);
            free(png);
            e->next_shot = now() + e->shots;
        }
        if (!quit_until && now() > e->deadline) {
            char *png = str_printf("%s/timeout.png", e->work_dir);
            const char *shot[] = { "SCREENSHOT", png, NULL };
            char reply[256];

            if (png)
                command_wait(e, shot, reply, sizeof reply, 2);
            free(png);
            stop_group(e);
            e->status = AMIGA_EMULATION_TIMEOUT;
            break;
        }
        sleep_ms(100);
    }
    free(done);
    stop_group(e);
    if (e->slot_dir)
        snap_finish(e);
}

/* closes the descriptors from 3 on but keep (its own slot, -1: none): the
   supervisor must not keep the caller's files open (the locks of its
   other sessions' slots, sockets) */
static void close_fds(int keep)
{
    long max, fd;

#if defined(__linux__) && defined(SYS_close_range)
    if (keep < 3) {
        if (!syscall(SYS_close_range, 3u, ~0u, 0u))
            return;
    } else if ((keep == 3 || !syscall(SYS_close_range, 3u, (unsigned)keep - 1, 0u))
               && !syscall(SYS_close_range, (unsigned)keep + 1, ~0u, 0u)) {
        return;
    }
#endif
    max = sysconf(_SC_OPEN_MAX);
    if (max < 0 || max > 65536)
        max = 65536;
    for (fd = 3; fd < max; fd++)
        if (fd != keep)
            close((int)fd);
}

/* the supervisor's process: starts Amiberry and drives the session to its
   end, whenever the caller joins it */
[[noreturn]] static void supervisor(struct amiga_emulation *e, const char *statefile)
{
    struct sigaction sa;
    sigset_t none;
    char err[256];

#ifdef __linux__
    /* it dies with the caller (Amiberry dies with it, see child) */
    if (prctl(PR_SET_PDEATHSIG, SIGKILL) || getppid() != e->parent)
        _exit(126);
#endif
    /* the slot stays locked while the supervisor lives: the caller closes
       its copy of the descriptor (one open file description, one lock),
       so the slot is free again when the session has ended, not when the
       caller joins it; the descriptor is O_CLOEXEC, Amiberry does not
       inherit it */
    close_fds(e->slot);
    e->parent = getpid();
    memset(&sa, 0, sizeof sa);
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = on_term;
    sigaction(SIGTERM, &sa, NULL);
    sa.sa_handler = SIG_IGN;    /* a write to a socket Amiberry has closed */
    sigaction(SIGPIPE, &sa, NULL);
    sa.sa_handler = SIG_DFL;    /* waitpid for Amiberry */
    sigaction(SIGCHLD, &sa, NULL);
    sigemptyset(&none);
    sigprocmask(SIG_SETMASK, &none, NULL);
    if (launch(e, statefile, err, sizeof err)) {
        snap_log(e, "cannot start Amiberry: %s", err);
        if (e->slot_dir)
            snap_finish(e);
        e->status = AMIGA_EMULATION_FAILED;
    } else {
        supervise(e);
    }
    atomic_store(&e->sh->pgid, 0);
    atomic_store(&e->sh->status, (int)e->status);
    atomic_store(&e->sh->snap_use, (int)e->snap_use);
    atomic_store(&e->sh->end, to_us(now()));
    atomic_store(&e->sh->finished, 1);
    _exit(0);
}

/* the shared state and the supervisor (amiga_emulation.h, "Every session
   has a supervisor") */
static int start_supervisor(struct amiga_emulation *e, const char *statefile, char *err,
                            size_t errlen)
{
    void *m = mmap(NULL, sizeof *e->sh, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    pid_t pid;

    if (m == MAP_FAILED) {
        set_err(err, errlen, "mmap: %s", strerror(errno));
        return -1;
    }
    e->sh = m;
    atomic_init(&e->sh->finished, 0);
    atomic_init(&e->sh->status, (int)AMIGA_EMULATION_FAILED);
    atomic_init(&e->sh->snap_use, (int)e->snap_use);
    atomic_init(&e->sh->pgid, 0);
    atomic_init(&e->sh->launched, 0);
    atomic_init(&e->sh->end, 0);
    pid = fork();
    if (pid < 0) {
        set_err(err, errlen, "fork: %s", strerror(errno));
        return -1;
    }
    if (pid == 0)
        supervisor(e, statefile);
    e->sup = pid;
    /* from here on the supervisor holds the slot (see supervisor) */
    if (e->slot >= 0) {
        close(e->slot);
        e->slot = -1;
    }
    return 0;
}

/* the caller, when the supervisor has ended without finishing the session
   (killed): the slot again, to clean up its directories.  1 = locked (in
   *fd, -1 without a slot lock file); 0: another session has the slot by
   now and has its directories. */
static int slot_again(struct amiga_emulation *e, int *fd)
{
    *fd = -1;
    /* still held (no supervisor started), or no lock file */
    if (e->slot >= 0 || !e->slot_lock)
        return 1;
    *fd = open(e->slot_lock, O_RDWR | O_CLOEXEC);
    if (*fd >= 0 && !flock(*fd, LOCK_EX | LOCK_NB))
        return 1;
    if (*fd >= 0)
        close(*fd);
    *fd = -1;
    return 0;
}

/* the caller, after the supervisor has ended: the result; if it did not
   end the session itself (killed), its Amiberry and the slot's
   directories (if the slot is still free, slot_again) */
static void collect(struct amiga_emulation *e)
{
    if (atomic_load(&e->sh->finished)) {
        e->status = (enum amiga_emulation_status)atomic_load(&e->sh->status);
        e->end_at = from_us(atomic_load(&e->sh->end));
        e->snap = SNAP_OFF;     /* snap_finish done */
    } else {
        int pg = atomic_load(&e->sh->pgid), fd;

        if (pg > 0)
            kill(-pg, SIGKILL);
        if (!slot_again(e, &fd)) {
            /* the slot's directories are not ours any more */
            free(e->slot_dir);
            e->slot_dir = NULL;
        }
        if (e->slot_dir)
            snap_finish(e);
        if (fd >= 0)
            close(fd);
        e->status = AMIGA_EMULATION_FAILED;
        e->end_at = now();
    }
    e->snap_use = (enum amiga_emulation_snapshot_use)atomic_load(&e->sh->snap_use);
}

enum amiga_emulation_status amiga_emulation_join(struct amiga_emulation *e)
{
    if (e->joined)
        return e->status;
    if (e->sup > 0) {
        while (!e->sup_reaped && waitpid(e->sup, NULL, 0) < 0 && errno == EINTR)
            ;
        e->sup_reaped = 1;
        collect(e);
    }
    e->joined = 1;
    return e->status;
}

void amiga_emulation_stats(const struct amiga_emulation *e, struct amiga_emulation_stats *s)
{
    double end = e->joined ? e->end_at
                 : e->sh && atomic_load(&e->sh->finished) ? from_us(atomic_load(&e->sh->end))
                 : now();
    long long launched = e->sh ? atomic_load(&e->sh->launched) : 0;

    s->wait = e->slot_got > e->slot_from ? e->slot_got - e->slot_from : 0;
    s->session = e->slot_got ? end - e->slot_got : 0;
    /* the time limit counts from the last (re)start of Amiberry */
    s->timed = launched ? end - from_us(launched) : 0;
    if (s->timed < 0)
        s->timed = 0;
    s->timeout = e->timeout;
    s->machine = e->machine ? e->machine : "a1200";
    s->snapshot = e->sh ? (enum amiga_emulation_snapshot_use)atomic_load(&e->sh->snap_use)
                        : e->snap_use;
    s->status = e->joined ? e->status : AMIGA_EMULATION_FAILED;
}

const char *amiga_emulation_snapshot_use_name(enum amiga_emulation_snapshot_use u)
{
    switch (u) {
    case AMIGA_EMULATION_SNAP_RESTORED: return "restored";
    case AMIGA_EMULATION_SNAP_FIRST_SAVE: return "first-save";
    case AMIGA_EMULATION_SNAP_FALLBACK: return "fallback";
    default: return "cold";
    }
}

void amiga_emulation_free(struct amiga_emulation *e)
{
    if (!e)
        return;
    if (e->sup > 0 && !e->sup_reaped) {
        /* not joined: the supervisor stops the session (bounded, then
           killed; collect ends its Amiberry then) */
        double until = now() + SUP_STOP_GRACE;

        kill(e->sup, SIGTERM);
        while (!sup_ended(e) && now() < until)
            sleep_ms(50);
        if (!e->sup_reaped) {
            kill(e->sup, SIGKILL);
            while (waitpid(e->sup, NULL, 0) < 0 && errno == EINTR)
                ;
            e->sup_reaped = 1;
        }
    }
    if (e->sh && !e->joined)
        collect(e);
    if (e->snap != SNAP_OFF)
        snap_finish(e);
    if (e->slot >= 0)
        close(e->slot);
    if (e->counted)
        active_runs--;
    rm_tree(e->runtime_dir);
    if (e->own_work_dir)
        rm_tree(e->work_dir);
    free(e->work_dir);
    free(e->test_dir);
    free(e->slot_dir);
    free(e->slot_lock);
    free(e->machine);
    free(e->ctl_dir);
    free(e->snap_path);
    free(e->job);
    free(e->cold_cfg);
    free(e->real_test_dir);
    for (size_t i = 0; e->mount_dirs && i < e->nmounts; i++)
        free(e->mount_dirs[i]);
    free(e->mount_dirs);
    free(e->runtime_dir);
    free(e->socket_path);
    if (e->sh)
        munmap(e->sh, sizeof *e->sh);
    free(e);
}
