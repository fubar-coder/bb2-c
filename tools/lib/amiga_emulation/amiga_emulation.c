/*
 * amiga_emulation.c - run an AmigaDOS script in Amiberry for tests, see
 * amiga_emulation.h.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp, nftw, setenv, kill, setpgid */
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
#define QUIT_GRACE  10          /* seconds between QUIT and SIGTERM */
#define KILL_GRACE  5           /* seconds between SIGTERM and SIGKILL */

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
                  int log_on, pid_t parent)
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
    if (log_on)
        execlp(amiberry, amiberry, "--config", "amiberry.uae", "-G", "--log", (char *)NULL);
    else
        execlp(amiberry, amiberry, "--config", "amiberry.uae", "-G", (char *)NULL);
    _exit(127);
}

int amiga_emulation_slot(const char *dir, unsigned max, int wait)
{
    unsigned k;

    if (!max)
        return open("/dev/null", O_RDONLY | O_CLOEXEC);
    if (mkdir(dir, 0700) && errno != EEXIST)
        return -1;
    for (;;) {
        for (k = 0; k < max; k++) {
            char *path = str_printf("%s/slot-%u.lock", dir, k);
            int fd = path ? open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0600) : -1;

            free(path);
            if (fd < 0)
                return -1;
            if (!flock(fd, LOCK_EX | LOCK_NB))
                return fd;
            close(fd);
        }
        if (!wait)
            return -1;
        sleep_ms(250);
    }
}

/* the runs of this process that are started and not freed yet */
static unsigned active_runs;

/* a slot for this run: $AMIGA_EMULATION_MAX of them (default 4).  Only a
   process without a run of its own waits: one that already runs sessions
   (a runner with parallel > 1) takes a free slot if there is one and
   otherwise runs this session without one - waiting while holding slots
   could deadlock two runners that wait for each other's slots, and its
   running sessions' time limits would not be watched meanwhile. */
static int take_slot(char *err, size_t errlen)
{
    const char *env = getenv("AMIGA_EMULATION_MAX");
    const char *xdg = getenv("XDG_RUNTIME_DIR");
    unsigned max = env && *env ? (unsigned)strtoul(env, NULL, 10) : 4;
    char *dir = xdg && *xdg ? str_printf("%s/amiga-emulation-slots", xdg)
                            : str_printf("/tmp/amiga-emulation-slots-%u", (unsigned)getuid());
    int fd = dir ? amiga_emulation_slot(dir, max, active_runs == 0) : -1;

    if (fd < 0 && active_runs) {
        free(dir);
        return open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
    if (fd < 0)
        set_err(err, errlen, "cannot take an emulator slot in %s: %s", dir ? dir : "",
                strerror(errno));
    free(dir);
    return fd;
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
    pid_t parent;

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

    if ((e->slot = take_slot(err, errlen)) < 0)
        goto fail;
    active_runs++;
    e->counted = 1;
    parent = getpid();
    e->deadline = now() + (p->timeout ? p->timeout : 60);
    if (getenv("AMIGA_EMULATION_SHOTS"))
        e->shots = strtod(getenv("AMIGA_EMULATION_SHOTS"), NULL);
    if (!(e->shots > 0))
        e->shots = 0;
    if (e->shots && mkdirs(e->work_dir, (const char *const[]){ "shots", NULL }))
        e->shots = 0;
    e->next_shot = now() + e->shots;
    e->pid = fork();
    if (e->pid < 0) {
        set_err(err, errlen, "fork: %s", strerror(errno));
        goto fail;
    }
    if (e->pid == 0)
        child(e->work_dir, e->runtime_dir, p->amiberry ? p->amiberry : "amiberry",
              p->visible || env_on("AMIGA_EMULATION_VISIBLE"),
              p->log, parent);
    setpgid(e->pid, e->pid);
    free(cfg);
    free(script);
    return e;

fail:
    free(cfg);
    free(script);
    amiga_emulation_free(e);
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
    free(e->runtime_dir);
    free(e->socket_path);
    free(e);
}
