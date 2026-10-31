/*
 * amiga_emulation.c - run an AmigaDOS script in Amiberry for tests, see
 * amiga_emulation.h.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp, nftw, setenv, kill, setpgid */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
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
} machines[] = {
    { "a1200", default_uae, NULL, 0 },
    { "a500", a500_uae, NULL, 0 },
    { "a500-68010", a500_uae, a500_68010, sizeof a500_68010 / sizeof a500_68010[0] },
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

const struct amiga_emulation_option amiga_emulation_rtg[] = {
    { "gfxcard_size", "16" },
    { "gfxcard_type", "ZorroIII" },
    { "hardfile2", "ro,DH0:@EMULATION_DIR@/Workbench_OS3.2-AB3.vhd,32,1,2,512,0,,uae0" },
    { "uaehf0", "hdf,ro,DH0:@EMULATION_DIR@/Workbench_OS3.2-AB3.vhd,32,1,2,512,0,,uae0" },
};
const size_t amiga_emulation_rtg_count = sizeof amiga_emulation_rtg / sizeof amiga_emulation_rtg[0];

#define DONE_FILE   "run.done"
/* After TEST:run.done the results are on the host already (the TEST:
   directory is the host's): Amiberry gets QUIT and a short grace, then the
   signals.  Amiberry 8.4 acknowledges QUIT ("IPC: Received QUIT") but often
   does not end within 10 s, which cost every session about 15 s. */
#define QUIT_GRACE  1           /* seconds between QUIT and SIGTERM */
#define KILL_GRACE  1           /* seconds between SIGTERM and SIGKILL */

struct amiga_emulation {
    pid_t pid;                  /* also the process group */
    double deadline;            /* monotonic seconds */
    char *work_dir;
    char *test_dir;
    char *runtime_dir;          /* XDG_RUNTIME_DIR */
    char *socket_path;
    int own_work_dir;
    int joined;
    int exited;                 /* waitpid has reaped the process */
    int slot;                   /* the locked slot file, -1: none */
    int counted;                /* counted in active_runs */
    double shots;               /* $AMIGA_EMULATION_SHOTS: seconds between screenshots */
    unsigned nshots;
    double next_shot;
    enum amiga_emulation_status status;
    /* snapshot mode (EXPERIMENT, amiga_emulation.h) */
    enum { SNAP_OFF, SNAP_CREATING, SNAP_RESTORING, SNAP_RUNNING } snap;
    char *orig_test_dir;        /* the work directory's TEST: */
    char *slot_dir;             /* slot-K */
    char *snap_path;            /* slot-K/snap-HASH.uss */
    char *job;                  /* TEST:job.script */
    char **wm_host, **wm_slot;  /* writable mounts: host and slot directories */
    size_t nwm;
    double ready_at, restore_until;
    unsigned timeout;
    const char *amiberry;
    int visible, log_on;
    int date_from_host;         /* the clock by DATE (Kickstart 1.3) */
    pid_t parent;
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
    log = open("amiberry.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
   that wait for each other's slots, and its running sessions' time limits
   would not be watched meanwhile.  The runner then ends one of its own
   sessions and tries again.  *index: the slot's number, -1 without a
   limit (AMIGA_EMULATION_MAX=0) */
static int take_slot(int *index, char *err, size_t errlen)
{
    char *dir = slots_dir();
    int fd = dir ? slot_take(dir, AMIGA_EMULATION_SLOTS_AUTO, active_runs == 0, index) : -1;

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

static int launch(struct amiga_emulation *e, const char *statefile, char *err, size_t errlen)
{
    e->exited = 0;
    e->pid = fork();
    if (e->pid < 0) {
        set_err(err, errlen, "fork: %s", strerror(errno));
        return -1;
    }
    if (e->pid == 0)
        child(e->work_dir, e->runtime_dir, e->amiberry, e->visible, e->log_on, statefile, e->parent);
    setpgid(e->pid, e->pid);
    return 0;
}

/* ------------------------------------------ snapshot mode (EXPERIMENT) */

#define SNAP_READY          "ready"
#define SNAP_STARTED        "run.started"
#define SNAP_JOB            "job.script"
#define SNAP_WAITER         "waitjob"
#define SNAP_SAVE_DELAY     0.5     /* seconds between TEST:ready and SAVESTATE */
#define SNAP_RESTORE_GRACE  10      /* seconds from the start to TEST:run.started */

/* TEST:run.script of every session in snapshot mode (AmigaDOS 1.3 syntax:
   it also runs under Kickstart 1.3); the snapshot is taken in the waiter */
static const char snap_prologue[] =
    "FAILAT 1000\n"
    "ECHO >TEST:" SNAP_READY " \"ready\"\n"
    "TEST:" SNAP_WAITER "\n"
    "EXECUTE TEST:" SNAP_JOB "\n";

/* TEST:waitjob (waitjob.s, assembled by vasm in the build) */
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

/* the files of the prologue in the slot's TEST:, not copied back */
static const char *const snap_private[] = {
    "run.script", SNAP_JOB, SNAP_WAITER, SNAP_READY, SNAP_STARTED, "job.tmp", NULL
};

static unsigned long long fnv(unsigned long long h, const void *p, size_t n)
{
    const unsigned char *s = p;

    while (n--) {
        h ^= *s++;
        h *= 0x100000001b3ULL;
    }
    return h;
}

/* the identity of a file (device, inode, size, mtime) into the hash */
static unsigned long long fnv_file(unsigned long long h, const char *path)
{
    struct stat st;

    if (path && !stat(path, &st)) {
        long long v[5] = { (long long)st.st_dev, (long long)st.st_ino, (long long)st.st_size,
                           (long long)st.st_mtim.tv_sec, (long long)st.st_mtim.tv_nsec };

        return fnv(h, v, sizeof v);
    }
    return fnv(h, "?", 1);
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

static int copy_file(const char *src, const char *dst, const struct stat *st)
{
    char buf[65536];
    int in = open(src, O_RDONLY | O_CLOEXEC), out, r = 0;
    ssize_t n;

    if (in < 0)
        return -1;
    out = open(dst, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, st->st_mode & 0777);
    if (out < 0) {
        close(in);
        return -1;
    }
    while ((n = read(in, buf, sizeof buf)) > 0)
        if (write(out, buf, (size_t)n) != n) {
            r = -1;
            break;
        }
    if (n < 0)
        r = -1;
    /* the Amiga sees the host's modification time as the file date */
    if (!r) {
        struct timespec ts[2] = { st->st_atim, st->st_mtim };

        futimens(out, ts);
    }
    close(in);
    return close(out) || r ? -1 : 0;
}

static int in_list(const char *name, const char *const *list)
{
    for (; list && *list; list++)
        if (!strcmp(name, *list))
            return 1;
    return 0;
}

/* src into dst (created if missing), without the top-level names in skip */
static int copy_tree(const char *src, const char *dst, const char *const *skip)
{
    DIR *d = opendir(src);
    struct dirent *de;
    int r = 0;

    if (!d)
        return -1;
    if (mkdir(dst, 0700) && errno != EEXIST) {
        closedir(d);
        return -1;
    }
    while ((de = readdir(d)) != NULL) {
        char *s, *t;
        struct stat st;

        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..") || in_list(de->d_name, skip))
            continue;
        s = str_printf("%s/%s", src, de->d_name);
        t = str_printf("%s/%s", dst, de->d_name);
        if (!s || !t || stat(s, &st))
            r = -1;
        else if (S_ISDIR(st.st_mode))
            r |= copy_tree(s, t, NULL);
        else if (S_ISREG(st.st_mode))
            r |= copy_file(s, t, &st);
        free(s);
        free(t);
    }
    closedir(d);
    return r;
}

/* removes the contents of dir except the top-level names in keep */
static int empty_dir(const char *dir, const char *const *keep)
{
    DIR *d = opendir(dir);
    struct dirent *de;

    if (!d)
        return mkdir(dir, 0700) && errno != EEXIST ? -1 : 0;
    while ((de = readdir(d)) != NULL) {
        char *s;

        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..") || in_list(de->d_name, keep))
            continue;
        if ((s = str_printf("%s/%s", dir, de->d_name)) != NULL)
            rm_tree(s);
        free(s);
    }
    closedir(d);
    return 0;
}

/* the slot's directories for a (new) start: the work directory's TEST:
   and the writable mounts copied in, the prologue and the waiter */
static int snap_prepare(struct amiga_emulation *e)
{
    char *path;
    size_t i;
    int r = 0;

    if (empty_dir(e->test_dir, NULL) || copy_tree(e->orig_test_dir, e->test_dir, snap_private))
        return -1;
    for (i = 0; i < e->nwm; i++)
        if (empty_dir(e->wm_slot[i], NULL) || copy_tree(e->wm_host[i], e->wm_slot[i], NULL))
            return -1;
    path = str_printf("%s/run.script", e->test_dir);
    r |= !path || write_file(path, snap_prologue);
    free(path);
    path = str_printf("%s/" SNAP_WAITER, e->test_dir);
    r |= !path || write_bytes(path, snap_waiter, sizeof snap_waiter);
    free(path);
    return r ? -1 : 0;
}

/* TEST:job.script (complete before the waiter can see it): the clock, the
   start mark, the test's script */
static int snap_write_job(struct amiga_emulation *e, int date_from_host)
{
    char clock[64], *tmp = str_printf("%s/job.tmp", e->test_dir);
    char *dst = str_printf("%s/" SNAP_JOB, e->test_dir), *text;
    int r;

    if (date_from_host) {
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
    text = str_printf("%sECHO >TEST:" SNAP_STARTED " \"started\"\n%s", clock, e->job);
    r = !tmp || !dst || !text || write_file(tmp, text) || rename(tmp, dst) ? -1 : 0;
    free(tmp);
    free(dst);
    free(text);
    return r;
}

/* SAVESTATE into snap_path (through a temporary name) */
static void snap_save(struct amiga_emulation *e)
{
    char *tmp = str_printf("%s.tmp.uss", e->snap_path);
    char *cfg = str_printf("%s/snapshot.uae", e->work_dir);
    const char *cmd[] = { "SAVESTATE", tmp, cfg, NULL };
    char reply[256];

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
        if (!stat(tmp, &st) && st.st_size > 0 && st.st_size == last)
            rename(tmp, e->snap_path);
    }
    if (tmp)
        unlink(tmp);
    if (cfg)
        unlink(cfg);
    free(tmp);
    free(cfg);
}

/* the results back: the slot's TEST: into the work directory's, the
   writable mounts into their host directories */
static void snap_finish(struct amiga_emulation *e)
{
    static const char *const keep[] = { "run.script", NULL };
    char *slot_test = e->test_dir;
    size_t i;

    empty_dir(e->orig_test_dir, keep);
    copy_tree(slot_test, e->orig_test_dir, snap_private);
    empty_dir(slot_test, NULL);
    for (i = 0; i < e->nwm; i++) {
        empty_dir(e->wm_host[i], NULL);
        copy_tree(e->wm_slot[i], e->wm_host[i], NULL);
        empty_dir(e->wm_slot[i], NULL);
    }
    e->test_dir = e->orig_test_dir;
    e->orig_test_dir = slot_test;
    e->snap = SNAP_OFF;
}

/* switches the run to the slot's directories; the configuration for them
   into the work directory; *statefile: the snapshot if there is one */
static int snap_setup(struct amiga_emulation *e, const struct amiga_emulation_params *p,
                      const char *emulation_dir, int index, const char **statefile,
                      char *err, size_t errlen)
{
    struct amiga_emulation_params q = *p;
    struct amiga_emulation_mount *mounts = NULL;
    char *dir = slots_dir(), *cfg = NULL, *path, *rom, *hdf, *prog;
    unsigned long long h = 0xcbf29ce484222325ULL;
    size_t i;
    int r = -1;

    *statefile = NULL;
    e->slot_dir = dir ? str_printf("%s/slot-%d", dir, index) : NULL;
    free(dir);
    if (!e->slot_dir || (mkdir(e->slot_dir, 0700) && errno != EEXIST))
        goto out;
    e->orig_test_dir = e->test_dir;
    e->test_dir = str_printf("%s/test", e->slot_dir);
    if (!e->test_dir)
        goto out;
    if (p->nmounts) {
        mounts = calloc(p->nmounts, sizeof *mounts);
        e->wm_host = calloc(p->nmounts, sizeof *e->wm_host);
        e->wm_slot = calloc(p->nmounts, sizeof *e->wm_slot);
        if (!mounts || !e->wm_host || !e->wm_slot)
            goto out;
    }
    for (i = 0; i < p->nmounts; i++) {
        mounts[i] = p->mounts[i];
        if (!p->mounts[i].writable)
            continue;
        e->wm_host[e->nwm] = strdup(p->mounts[i].host_dir);
        e->wm_slot[e->nwm] = str_printf("%s/mnt-%zu", e->slot_dir, i);
        mounts[i].host_dir = e->wm_slot[e->nwm];
        if (!e->wm_host[e->nwm] || !e->wm_slot[e->nwm++])
            goto out;
    }
    q.mounts = mounts;
    if (!(cfg = amiga_emulation_config(&q, emulation_dir, e->test_dir, err, errlen)))
        goto out;
    /* the snapshot's key */
    h = fnv(h, cfg, strlen(cfg));
    h = fnv(h, snap_prologue, sizeof snap_prologue);
    h = fnv(h, snap_waiter, sizeof snap_waiter);
    rom = config_value(cfg, "kickstart_rom_file");
    hdf = config_value(cfg, "hardfile2");
    path = hdf ? strstr(hdf, "DH0:") : NULL;
    if (path) {
        path += 4;
        path[strcspn(path, ",")] = 0;
    }
    prog = find_program(e->amiberry);
    h = fnv_file(fnv_file(fnv_file(h, rom), path), prog);
    free(rom);
    free(hdf);
    free(prog);
    e->snap_path = str_printf("%s/snap-%016llx.uss", e->slot_dir, h);
    path = str_printf("%s/amiberry.uae", e->work_dir);
    if (!e->snap_path || !path || write_file(path, cfg)) {
        free(path);
        set_err(err, errlen, "cannot write the configuration");
        goto out;
    }
    free(path);
    if (snap_prepare(e)) {
        set_err(err, errlen, "cannot prepare the slot directory %s", e->slot_dir);
        goto out;
    }
    if (file_exists(e->snap_path)) {
        *statefile = e->snap_path;
        e->snap = SNAP_RESTORING;
    } else {
        e->snap = SNAP_CREATING;
    }
    r = 0;
out:
    free(mounts);
    free(cfg);
    return r;
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

    if ((e->slot = take_slot(&slot_index, err, errlen)) < 0)
        goto fail;
    active_runs++;
    e->counted = 1;
    e->parent = getpid();
    e->timeout = p->timeout ? p->timeout : 60;
    e->amiberry = p->amiberry ? p->amiberry : "amiberry";
    e->visible = p->visible || env_on("AMIGA_EMULATION_VISIBLE");
    e->log_on = p->log;
    if ((p->snapshot || env_on("AMIGA_EMULATION_SNAPSHOT")) && slot_index >= 0) {
        e->date_from_host = snap_date_from_host(p);
        if (!(e->job = strdup(script))
            || snap_setup(e, p, emulation_dir, slot_index, &statefile, err, errlen))
            goto fail;
        if (e->snap == SNAP_RESTORING) {
            if (snap_write_job(e, e->date_from_host)) {
                set_err(err, errlen, "cannot write TEST:" SNAP_JOB);
                goto fail;
            }
            e->restore_until = now() + SNAP_RESTORE_GRACE;
        }
    }
    e->deadline = now() + e->timeout;
    if (getenv("AMIGA_EMULATION_SHOTS"))
        e->shots = strtod(getenv("AMIGA_EMULATION_SHOTS"), NULL);
    if (!(e->shots > 0))
        e->shots = 0;
    if (e->shots && mkdirs(e->work_dir, (const char *const[]){ "shots", NULL }))
        e->shots = 0;
    e->next_shot = now() + e->shots;
    if (launch(e, statefile, err, errlen))
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

/* reaps the process if it has ended; 1 = ended */
static int reaped(struct amiga_emulation *e)
{
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

enum amiga_emulation_status amiga_emulation_join(struct amiga_emulation *e)
{
    char *done = str_printf("%s/%s", e->test_dir, DONE_FILE);
    double quit_until = 0;

    if (e->joined || !done) {
        free(done);
        return e->status;
    }
    for (;;) {
        if (e->snap == SNAP_RESTORING) {
            char *started = str_printf("%s/" SNAP_STARTED, e->test_dir);
            int ok = started && file_exists(started);

            free(started);
            if (ok) {
                e->snap = SNAP_RUNNING;
            } else if (reaped(e) || now() > e->restore_until) {
                /* the snapshot does not work: drop it, boot cold */
                char err[256];

                stop_group(e);
                unlink(e->snap_path);
                if (snap_prepare(e) || launch(e, NULL, err, sizeof err)) {
                    e->status = AMIGA_EMULATION_FAILED;
                    break;
                }
                e->snap = SNAP_CREATING;
                e->ready_at = 0;
                e->deadline = now() + e->timeout;
                continue;
            }
        }
        if (e->snap == SNAP_CREATING) {
            char *ready = str_printf("%s/" SNAP_READY, e->test_dir);

            if (!e->ready_at && ready && file_exists(ready))
                e->ready_at = now() + SNAP_SAVE_DELAY;
            free(ready);
            if (e->ready_at && now() >= e->ready_at && !reaped(e)) {
                snap_save(e);
                if (snap_write_job(e, e->date_from_host)) {
                    e->status = AMIGA_EMULATION_FAILED;
                    stop_group(e);
                    break;
                }
                e->snap = SNAP_RUNNING;
            }
        }
        if (reaped(e)) {
            e->status = file_exists(done) ? AMIGA_EMULATION_DONE : AMIGA_EMULATION_EXITED;
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
    if (e->snap != SNAP_OFF) {
        stop_group(e);
        snap_finish(e);
    }
    e->joined = 1;
    return e->status;
}

void amiga_emulation_free(struct amiga_emulation *e)
{
    if (!e)
        return;
    if (e->pid > 0 && !e->exited)
        stop_group(e);
    if (e->slot >= 0)
        close(e->slot);
    if (e->counted)
        active_runs--;
    rm_tree(e->runtime_dir);
    if (e->own_work_dir)
        rm_tree(e->work_dir);
    free(e->work_dir);
    free(e->test_dir);
    free(e->orig_test_dir);
    free(e->slot_dir);
    free(e->snap_path);
    free(e->job);
    for (size_t i = 0; i < e->nwm; i++) {
        free(e->wm_host[i]);
        free(e->wm_slot[i]);
    }
    free(e->wm_host);
    free(e->wm_slot);
    free(e->runtime_dir);
    free(e->socket_path);
    free(e);
}
