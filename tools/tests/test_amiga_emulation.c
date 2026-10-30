/*
 * test_amiga_emulation.c - configuration and script of amiga_emulation
 * (no emulator), the slots that limit the runs, the snapshot mode's
 * configuration, key and pruning.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "testutil.h"
#include "amiga_emulation.h"

/* writes text to dir/max ("" = removes it) */
static void put_max(const char *dir, const char *text)
{
    char path[96];
    FILE *f;

    snprintf(path, sizeof path, "%s/max", dir);
    remove(path);
    if (!*text)
        return;
    f = fopen(path, "wb");
    CHECK(f != NULL);
    if (f) {
        fputs(text, f);
        fclose(f);
    }
}

/* the limit of the slot directory without AMIGA_EMULATION_MAX */
static unsigned max_of(const char *dir, const char *text)
{
    put_max(dir, text);
    return amiga_emulation_slot_max(dir);
}

/* number of lines of cfg that equal line */
static int lines_equal(const char *cfg, const char *line)
{
    size_t n = strlen(line);
    int count = 0;
    const char *p = cfg;

    while (*p) {
        const char *e = strchr(p, '\n');
        size_t len = e ? (size_t)(e - p) : strlen(p);

        if (len == n && !strncmp(p, line, n))
            count++;
        p += len + (e != NULL);
    }
    return count;
}

static int key_lines(const char *cfg, const char *key)
{
    size_t n = strlen(key);
    int count = 0;
    const char *p = cfg;

    while (*p) {
        const char *e = strchr(p, '\n');
        size_t len = e ? (size_t)(e - p) : strlen(p);

        if (len > n && !strncmp(p, key, n) && p[n] == '=')
            count++;
        p += len + (e != NULL);
    }
    return count;
}

/* a file of n bytes */
static void put_file(const char *path, size_t n)
{
    FILE *f = fopen(path, "wb");

    CHECK(f != NULL);
    if (!f)
        return;
    while (n--)
        fputc('x', f);
    fclose(f);
}

/* the snapshot mode's configuration: the slot's fixed paths for TEST:
   and every mount, SNAP: after them; a mount named SNAP is refused */
static void snapshot_config(void)
{
    struct amiga_emulation_mount mounts[] = { { "CD", "/data/cd", 0 }, { "W", "/data/w", 1 } };
    struct amiga_emulation_mount snap[] = { { "snap", "/data/s", 1 } };
    struct amiga_emulation_params p = { 0 };
    char err[200], *cfg, *again;

    p.mounts = mounts;
    p.nmounts = 2;
    cfg = amiga_emulation_snapshot_config(&p, "/emu", "/slots/slot-3", err, sizeof err);
    CHECK(cfg != NULL);
    CHECK(cfg && lines_equal(cfg, "filesystem2=rw,DH1:TEST:/slots/slot-3/test,0") == 1);
    CHECK(cfg && lines_equal(cfg, "filesystem2=ro,DH2:CD:/slots/slot-3/mnt-0,0") == 1);
    CHECK(cfg && lines_equal(cfg, "uaehf3=dir,rw,DH3:W:/slots/slot-3/mnt-1,0") == 1);
    CHECK(cfg && lines_equal(cfg, "uaehf4=dir,rw,DH4:SNAP:/slots/slot-3/ctl,0") == 1);
    CHECK(cfg && !strstr(cfg, "/data/"));
    /* the mounts' host directories do not matter, only the slot */
    mounts[0].host_dir = "/elsewhere/cd";
    again = amiga_emulation_snapshot_config(&p, "/emu", "/slots/slot-3", err, sizeof err);
    CHECK(cfg && again && !strcmp(cfg, again));
    free(again);
    again = amiga_emulation_snapshot_config(&p, "/emu", "/slots/slot-4", err, sizeof err);
    CHECK(cfg && again && strcmp(cfg, again));
    free(again);
    free(cfg);

    /* broken: SNAP: is the mode's own volume; a slot path with a comma */
    p.mounts = snap;
    p.nmounts = 1;
    err[0] = 0;
    cfg = amiga_emulation_snapshot_config(&p, "/emu", "/slots/slot-3", err, sizeof err);
    CHECK(cfg == NULL && strstr(err, "SNAP"));
    free(cfg);
    p.nmounts = 0;
    cfg = amiga_emulation_snapshot_config(&p, "/emu", "/slots,x/slot-3", err, sizeof err);
    CHECK(cfg == NULL);
    free(cfg);

    /* the ROM and the hardfile by their real paths: two emulation
       directories with links to the same files give the same text */
    {
        static const char *const files[] = { "A1200.47.115.rom", "Workbench_OS3.2-AB3-AGA.vhd" };
        char dir[] = "/tmp/amiga-emulation-snapreal-XXXXXX", path[160], link[160], real[160];
        char *a, *b;
        size_t i;
        int k;

        CHECK(mkdtemp(dir) != NULL);
        for (k = 0; k < 3; k++) {
            snprintf(path, sizeof path, "%s/%c", dir, "rab"[k]);
            CHECK(mkdir(path, 0700) == 0);
        }
        for (i = 0; i < 2; i++) {
            snprintf(real, sizeof real, "%s/r/%s", dir, files[i]);
            put_file(real, 4);
            for (k = 1; k < 3; k++) {
                snprintf(link, sizeof link, "%s/%c/%s", dir, "rab"[k], files[i]);
                CHECK(symlink(real, link) == 0);
            }
        }
        snprintf(path, sizeof path, "%s/a", dir);
        a = amiga_emulation_snapshot_config(&p, path, "/slots/slot-3", err, sizeof err);
        snprintf(path, sizeof path, "%s/b", dir);
        b = amiga_emulation_snapshot_config(&p, path, "/slots/slot-3", err, sizeof err);
        CHECK(a && b && !strcmp(a, b));
        snprintf(real, sizeof real, "kickstart_rom_file=%s/r/%s", dir, files[0]);
        CHECK(a && lines_equal(a, real) == 1);
        snprintf(real, sizeof real, "hardfile2=ro,DH0:%s/r/%s,32,1,2,512,0,,uae0", dir, files[1]);
        CHECK(a && lines_equal(a, real) == 1);
        snprintf(real, sizeof real, "%s/a/", dir);
        CHECK(a && !strstr(a, real));
        free(a);
        free(b);
        /* broken: missing files stay as they are */
        a = amiga_emulation_snapshot_config(&p, "/emu", "/slots/slot-3", err, sizeof err);
        CHECK(a && lines_equal(a, "kickstart_rom_file=/emu/A1200.47.115.rom") == 1);
        free(a);
        for (i = 0; i < 2; i++)
            for (k = 0; k < 3; k++) {
                snprintf(path, sizeof path, "%s/%c/%s", dir, "rab"[k], files[i]);
                remove(path);
            }
        for (k = 0; k < 3; k++) {
            snprintf(path, sizeof path, "%s/%c", dir, "rab"[k]);
            rmdir(path);
        }
        CHECK(rmdir(dir) == 0);
    }
}

/* the key: changes with the configuration and with each file's identity
   (ROM, hardfile, Amiberry); a missing file is a key of its own */
static void snapshot_key(void)
{
    char dir[] = "/tmp/amiga-emulation-snapkey-XXXXXX", rom[96], hdf[96], prog[96], *cfg;
    unsigned long long k0, k;
    struct timespec old[2] = { { 1000000000, 0 }, { 1000000000, 0 } };

    CHECK(mkdtemp(dir) != NULL);
    snprintf(rom, sizeof rom, "%s/kick.rom", dir);
    snprintf(hdf, sizeof hdf, "%s/wb.vhd", dir);
    snprintf(prog, sizeof prog, "%s/amiberry", dir);
    put_file(rom, 16);
    put_file(hdf, 16);
    put_file(prog, 16);
    cfg = malloc(512);
    CHECK(cfg != NULL);
    if (!cfg)
        return;
    snprintf(cfg, 512, "kickstart_rom_file=%s\nhardfile2=ro,DH0:%s,32,1,2,512,0,,uae0\ncpu_type=68020\n",
             rom, hdf);
    k0 = amiga_emulation_snapshot_key(cfg, prog);
    CHECK(amiga_emulation_snapshot_key(cfg, prog) == k0);
    /* the configuration */
    cfg[strlen(cfg) - 2] = '1';
    CHECK(amiga_emulation_snapshot_key(cfg, prog) != k0);
    cfg[strlen(cfg) - 2] = '0';
    CHECK(amiga_emulation_snapshot_key(cfg, prog) == k0);
    /* the ROM's size, the hardfile's time, the program's size */
    put_file(rom, 17);
    k = amiga_emulation_snapshot_key(cfg, prog);
    CHECK(k != k0);
    k0 = k;
    CHECK(utimensat(AT_FDCWD, hdf, old, 0) == 0);
    k = amiga_emulation_snapshot_key(cfg, prog);
    CHECK(k != k0);
    k0 = k;
    put_file(prog, 17);
    k = amiga_emulation_snapshot_key(cfg, prog);
    CHECK(k != k0);
    k0 = k;
    /* another file at the same path (another inode) */
    remove(rom);
    put_file(rom, 17);
    CHECK(utimensat(AT_FDCWD, rom, old, 0) == 0);
    k = amiga_emulation_snapshot_key(cfg, prog);
    CHECK(k != k0);
    k0 = k;
    /* broken: files missing, no hardfile line */
    remove(prog);
    k = amiga_emulation_snapshot_key(cfg, prog);
    CHECK(k != k0);
    remove(rom);
    remove(hdf);
    CHECK(amiga_emulation_snapshot_key(cfg, prog) != k);
    CHECK(amiga_emulation_snapshot_key("cpu_type=68020\n", prog) != k);
    free(cfg);
    rmdir(dir);
}

/* pruning: the newest keep snap-*.uss stay, left-over save-*.uss and the
   older ones go, other files stay */
static void snapshot_prune(void)
{
    static const char *const names[] = { "snap-0000000000000001.uss", "snap-0000000000000002.uss",
                                         "snap-0000000000000003.uss", "snap-0000000000000004.uss" };
    char dir[] = "/tmp/amiga-emulation-snapprune-XXXXXX", path[128];
    size_t i;

    CHECK(mkdtemp(dir) != NULL);
    for (i = 0; i < 4; i++) {
        /* names[i] used at time 1000 + i, names[3] last */
        struct timespec t[2] = { { 1000 + (time_t)i, 0 }, { 1000 + (time_t)i, 0 } };

        snprintf(path, sizeof path, "%s/%s", dir, names[i]);
        put_file(path, 4);
        CHECK(utimensat(AT_FDCWD, path, t, 0) == 0);
    }
    snprintf(path, sizeof path, "%s/save-0000000000000005.uss", dir);
    put_file(path, 4);
    snprintf(path, sizeof path, "%s/other.uss", dir);
    put_file(path, 4);
    snprintf(path, sizeof path, "%s/snap-.uss", dir);
    put_file(path, 4);
    /* save-*.uss, then the two oldest */
    CHECK(amiga_emulation_snapshot_prune(dir, 2) == 3);
    for (i = 0; i < 4; i++) {
        snprintf(path, sizeof path, "%s/%s", dir, names[i]);
        CHECK(!access(path, F_OK) == (i >= 2));
    }
    snprintf(path, sizeof path, "%s/other.uss", dir);
    CHECK(!access(path, F_OK));
    CHECK(amiga_emulation_snapshot_prune(dir, 2) == 0);
    CHECK(amiga_emulation_snapshot_prune(dir, 0) == 2);
    remove(path);
    snprintf(path, sizeof path, "%s/snap-.uss", dir);
    remove(path);
    CHECK(rmdir(dir) == 0);
    /* broken: no directory */
    CHECK(amiga_emulation_snapshot_prune(dir, 2) == -1);
}

int main(void)
{
    struct amiga_emulation_option ch[] = {
        { "sound_output", "normal" },           /* replaced */
        { "chipmem_size", NULL },               /* removed */
        { "serial_port", "TCP://127.0.0.1:1234" },  /* new */
    };
    struct amiga_emulation_mount mounts[] = { { "CD", "/data/cd", 0 }, { "W", "/data/w", 1 } };
    struct amiga_emulation_params p = { 0 };
    char err[200], *cfg, *s;

    /* default: placeholders replaced, TEST: as DH1 */
    cfg = amiga_emulation_config(&p, "/emu", "/run/test", err, sizeof err);
    CHECK(cfg != NULL);
    CHECK(cfg && !strstr(cfg, "@EMULATION_DIR@"));
    CHECK(cfg && lines_equal(cfg, "kickstart_rom_file=/emu/A1200.47.115.rom") == 1);
    CHECK(cfg && lines_equal(cfg, "hardfile2=ro,DH0:/emu/Workbench_OS3.2-AB3-AGA.vhd,32,1,2,512,0,,uae0") == 1);
    CHECK(cfg && lines_equal(cfg, "filesystem2=rw,DH1:TEST:/run/test,0") == 1);
    CHECK(cfg && lines_equal(cfg, "uaehf1=dir,rw,DH1:TEST:/run/test,0") == 1);
    CHECK(cfg && lines_equal(cfg, "use_gui=no") == 1 && lines_equal(cfg, "sound_output=interrupts") == 1);
    CHECK(cfg && key_lines(cfg, "gfxcard_size") == 0);
    free(cfg);

    /* changes and mounts */
    p.changes = ch;
    p.nchanges = 3;
    p.mounts = mounts;
    p.nmounts = 2;
    cfg = amiga_emulation_config(&p, "/emu", "/run/test", err, sizeof err);
    CHECK(cfg && key_lines(cfg, "sound_output") == 1 && lines_equal(cfg, "sound_output=normal") == 1);
    CHECK(cfg && key_lines(cfg, "chipmem_size") == 0);
    CHECK(cfg && lines_equal(cfg, "serial_port=TCP://127.0.0.1:1234") == 1);
    CHECK(cfg && lines_equal(cfg, "filesystem2=ro,DH2:CD:/data/cd,0") == 1);
    CHECK(cfg && lines_equal(cfg, "uaehf3=dir,rw,DH3:W:/data/w,0") == 1);
    free(cfg);

    /* the RTG variant */
    p.changes = amiga_emulation_rtg;
    p.nchanges = amiga_emulation_rtg_count;
    p.nmounts = 0;
    cfg = amiga_emulation_config(&p, "/emu", "/run/test", err, sizeof err);
    CHECK(cfg && lines_equal(cfg, "gfxcard_size=16") == 1 && lines_equal(cfg, "gfxcard_type=ZorroIII") == 1);
    CHECK(cfg && key_lines(cfg, "hardfile2") == 1
          && lines_equal(cfg, "hardfile2=ro,DH0:/emu/Workbench_OS3.2-AB3.vhd,32,1,2,512,0,,uae0") == 1);
    free(cfg);

    /* the machines: a1200 is the default; a500 has its own template */
    p.nchanges = 0;
    p.machine = "a1200";
    cfg = amiga_emulation_config(&p, "/emu", "/run/test", err, sizeof err);
    CHECK(cfg && lines_equal(cfg, "kickstart_rom_file=/emu/A1200.47.115.rom") == 1);
    free(cfg);
    p.machine = "a500";
    cfg = amiga_emulation_config(&p, "/emu", "/run/test", err, sizeof err);
    CHECK(cfg && !strstr(cfg, "@EMULATION_DIR@"));
    CHECK(cfg && lines_equal(cfg, "kickstart_rom_file=/emu/amiga-os-130.rom") == 1);
    CHECK(cfg && lines_equal(cfg, "hardfile2=ro,DH0:/emu/workbench-135.hdf,0,0,0,512,0,,uae0") == 1);
    CHECK(cfg && lines_equal(cfg, "uaehf0=hdf,ro,DH0:/emu/workbench-135.hdf,0,0,0,512,0,,uae0") == 1);
    CHECK(cfg && key_lines(cfg, "hardfile2") == 1 && key_lines(cfg, "uaehf0") == 1);
    CHECK(cfg && lines_equal(cfg, "cpu_model=68000") == 1 && lines_equal(cfg, "cpu_24bit_addressing=true") == 1);
    CHECK(cfg && lines_equal(cfg, "chipset=ecs_agnus") == 1 && lines_equal(cfg, "use_gui=no") == 1);
    CHECK(cfg && lines_equal(cfg, "filesystem2=rw,DH1:TEST:/run/test,0") == 1);
    CHECK(cfg && lines_equal(cfg, "uaehf1=dir,rw,DH1:TEST:/run/test,0") == 1);
    /* nothing of the user's host in it */
    CHECK(cfg && !strstr(cfg, "amiberry.") && !strstr(cfg, "/home/") && key_lines(cfg, "gfx_width") == 0);
    free(cfg);
    p.machine = "a500-68010";
    cfg = amiga_emulation_config(&p, "/emu", "/run/test", err, sizeof err);
    CHECK(cfg && key_lines(cfg, "cpu_model") == 1 && lines_equal(cfg, "cpu_model=68010") == 1);
    CHECK(cfg && key_lines(cfg, "cpu_type") == 1 && lines_equal(cfg, "cpu_type=68010") == 1);
    CHECK(cfg && lines_equal(cfg, "kickstart_rom_file=/emu/amiga-os-130.rom") == 1);
    free(cfg);
    /* a change of the test wins over the machine's */
    {
        struct amiga_emulation_option cpu[] = { { "cpu_model", "68020" } };

        p.changes = cpu;
        p.nchanges = 1;
        cfg = amiga_emulation_config(&p, "/emu", "/run/test", err, sizeof err);
        CHECK(cfg && key_lines(cfg, "cpu_model") == 1 && lines_equal(cfg, "cpu_model=68020") == 1);
        CHECK(cfg && lines_equal(cfg, "cpu_type=68010") == 1);
        free(cfg);
        p.nchanges = 0;
    }
    /* refused: an unknown machine */
    CHECK(amiga_emulation_machine_known(NULL) && amiga_emulation_machine_known("a500-68010"));
    CHECK(!amiga_emulation_machine_known("a3000") && !amiga_emulation_machine_known(""));
    CHECK(amiga_emulation_machine_kickstart(NULL) == 47
          && amiga_emulation_machine_kickstart("a1200") == 47);
    CHECK(amiga_emulation_machine_kickstart("a500") == 34
          && amiga_emulation_machine_kickstart("a500-68010") == 34);
    CHECK(amiga_emulation_machine_kickstart("a3000") == 0);
    p.machine = "a3000";
    CHECK(amiga_emulation_config(&p, "/emu", "/run/test", err, sizeof err) == NULL
          && strstr(err, "unknown machine"));
    p.machine = NULL;

    /* refused: commas in paths, TEST as extra volume */
    p.nchanges = 0;
    CHECK(amiga_emulation_config(&p, "/emu", "/run,test", err, sizeof err) == NULL);
    mounts[0].volume = "TEST";
    p.nmounts = 1;
    CHECK(amiga_emulation_config(&p, "/emu", "/run/test", err, sizeof err) == NULL && strstr(err, "bad mount"));

    /* the script: FailAt, the text, the end mark */
    s = amiga_emulation_script("Echo hi");
    CHECK(s && strstr(s, "FailAt 1000\nEcho hi\nEcho >TEST:run.done \"done\"\n"));
    free(s);
    s = amiga_emulation_script(NULL);
    CHECK(s && strstr(s, "FailAt 1000\nEcho >TEST:run.done \"done\"\n"));
    free(s);

    /* the slots: max of them, the next waits (here: refused), a freed
       one is taken again; max 0 means no limit */
    {
        char dir[] = "/tmp/amiga-emulation-slottest-XXXXXX";
        int a, b, c;

        CHECK(mkdtemp(dir) != NULL);
        a = amiga_emulation_slot(dir, 2, 0);
        b = amiga_emulation_slot(dir, 2, 0);
        c = amiga_emulation_slot(dir, 2, 0);
        CHECK(a >= 0 && b >= 0 && c < 0);
        close(a);
        c = amiga_emulation_slot(dir, 2, 0);
        CHECK(c >= 0);
        close(b);
        close(c);
        a = amiga_emulation_slot(dir, 0, 0);
        CHECK(a >= 0);
        close(a);
        CHECK(amiga_emulation_slot("/nonexistent/dir/slots", 2, 0) < 0);

        /* the limit: the environment, else the file max, else 4 - also
           for broken files (garbage, 0, too large, negative, two
           numbers, a number with garbage behind it, too long) */
        {
            char *old = getenv("AMIGA_EMULATION_MAX");
            char *saved = old ? strdup(old) : NULL;
            char big[200];
            pid_t pid;
            int st;

            unsetenv("AMIGA_EMULATION_MAX");
            CHECK(max_of(dir, "") == 4);
            CHECK(max_of(dir, "6\n") == 6);
            CHECK(max_of(dir, " 2 ") == 2);
            CHECK(max_of(dir, "1") == 1);
            CHECK(max_of(dir, "64") == 64);
            CHECK(max_of(dir, "garbage\n") == 4);
            CHECK(max_of(dir, "0\n") == 4);
            CHECK(max_of(dir, "65") == 4);
            CHECK(max_of(dir, "99999999999999999999999") == 4);
            CHECK(max_of(dir, "-3") == 4);
            CHECK(max_of(dir, "3 4") == 4);
            CHECK(max_of(dir, "3x") == 4);
            CHECK(max_of(dir, "\n") == 4);
            memset(big, ' ', sizeof big - 2);
            big[sizeof big - 2] = '5';
            big[sizeof big - 1] = 0;
            CHECK(max_of(dir, big) == 4);
            CHECK(amiga_emulation_slot_max("/nonexistent/dir/slots") == 4);
            /* the environment wins, also 0 (no limit) */
            put_max(dir, "6");
            setenv("AMIGA_EMULATION_MAX", "3", 1);
            CHECK(amiga_emulation_slot_max(dir) == 3);
            setenv("AMIGA_EMULATION_MAX", "0", 1);
            CHECK(amiga_emulation_slot_max(dir) == 0);
            unsetenv("AMIGA_EMULATION_MAX");

            /* AUTO takes the file's limit: two slots, the third refused */
            put_max(dir, "2");
            a = amiga_emulation_slot(dir, AMIGA_EMULATION_SLOTS_AUTO, 0);
            b = amiga_emulation_slot(dir, AMIGA_EMULATION_SLOTS_AUTO, 0);
            CHECK(a >= 0 && b >= 0);
            CHECK(amiga_emulation_slot(dir, AMIGA_EMULATION_SLOTS_AUTO, 0) < 0);
            /* a run that waits gets a slot when the limit is raised while
               it waits (the child; alarm ends it if it never does) */
            pid = fork();
            if (pid == 0) {
                alarm(10);
                c = amiga_emulation_slot(dir, AMIGA_EMULATION_SLOTS_AUTO, 1);
                _exit(c >= 0 ? 0 : 1);
            }
            CHECK(pid > 0);
            nanosleep(&(struct timespec){ 0, 600000000 }, NULL);
            CHECK(pid > 0 && waitpid(pid, &st, WNOHANG) == 0);
            put_max(dir, "3");
            CHECK(pid > 0 && waitpid(pid, &st, 0) == pid && WIFEXITED(st)
                  && WEXITSTATUS(st) == 0);
            close(a);
            close(b);
            put_max(dir, "");
            if (saved) {
                setenv("AMIGA_EMULATION_MAX", saved, 1);
                free(saved);
            }
        }
        for (a = 0; a < 3; a++) {
            char lock[64];

            snprintf(lock, sizeof lock, "%s/slot-%d.lock", dir, a);
            remove(lock);
        }
        rmdir(dir);
    }

    /* the hard limit in amiga_emulation_start (XDG_RUNTIME_DIR and the
       limit of this test, `true` as Amiberry: no emulator, no ROM, no
       hardfile): a process with runs of its own gets no slot-less session
       but "no slot now"; one without waits for a slot */
    {
        static const struct amiga_emulation_option bare[] = {
            { "kickstart_rom_file", NULL }, { "hardfile2", NULL },
        };
        static const struct amiga_emulation_option nohdf[] = {
            { "hardfile2", "ro,DH0:/nonexistent/x.hdf,32,1,2,512,0,,uae0" },
        };
        char xdg[] = "/tmp/amiga-emulation-xdgtest-XXXXXX", slots[96], lock[128];
        char *oxdg = getenv("XDG_RUNTIME_DIR"), *omax = getenv("AMIGA_EMULATION_MAX");
        char *sxdg = oxdg ? strdup(oxdg) : NULL, *smax = omax ? strdup(omax) : NULL;
        struct amiga_emulation_params q = { 0 };
        struct amiga_emulation *a, *b, *c;
        pid_t pid;
        int x, y, st;

        CHECK(mkdtemp(xdg) != NULL);
        snprintf(slots, sizeof slots, "%s/amiga-emulation-slots", xdg);
        setenv("XDG_RUNTIME_DIR", xdg, 1);
        setenv("AMIGA_EMULATION_MAX", "2", 1);
        q.changes = bare;
        q.nchanges = 2;
        q.amiberry = "true";
        q.timeout = 10;

        a = amiga_emulation_start(&q, err, sizeof err);
        CHECK(a != NULL && !amiga_emulation_no_slot());
        b = amiga_emulation_start(&q, err, sizeof err);
        CHECK(b != NULL && !amiga_emulation_no_slot());
        /* both slots taken: the third fails at once, with EAGAIN */
        errno = 0;
        c = amiga_emulation_start(&q, err, sizeof err);
        CHECK(c == NULL && errno == EAGAIN && amiga_emulation_no_slot());
        CHECK(c == NULL && strstr(err, "no free emulator slot"));
        CHECK(amiga_emulation_slot(slots, 2, 0) < 0);
        amiga_emulation_free(c);
        /* another error stays one (not "no slot") */
        q.changes = nohdf;
        q.nchanges = 1;
        CHECK(amiga_emulation_start(&q, err, sizeof err) == NULL && !amiga_emulation_no_slot());
        q.changes = bare;
        q.nchanges = 2;
        /* a freed run's slot is taken again */
        amiga_emulation_free(a);
        c = amiga_emulation_start(&q, err, sizeof err);
        CHECK(c != NULL && !amiga_emulation_no_slot());
        amiga_emulation_free(b);
        amiga_emulation_free(c);

        /* without runs of its own: waits (the child) until a slot is
           freed; alarm ends it if it never gets one */
        x = amiga_emulation_slot(slots, 2, 0);
        y = amiga_emulation_slot(slots, 2, 0);
        CHECK(x >= 0 && y >= 0);
        pid = fork();
        if (pid == 0) {
            /* the copies of the parent's slots would hold their locks */
            close(x);
            close(y);
            alarm(10);
            a = amiga_emulation_start(&q, err, sizeof err);
            st = a != NULL;
            amiga_emulation_free(a);
            _exit(st ? 0 : 1);
        }
        CHECK(pid > 0);
        nanosleep(&(struct timespec){ 0, 600000000 }, NULL);
        CHECK(pid > 0 && waitpid(pid, &st, WNOHANG) == 0);
        close(x);
        CHECK(pid > 0 && waitpid(pid, &st, 0) == pid && WIFEXITED(st)
              && WEXITSTATUS(st) == 0);
        close(y);

        if (sxdg)
            setenv("XDG_RUNTIME_DIR", sxdg, 1);
        else
            unsetenv("XDG_RUNTIME_DIR");
        if (smax)
            setenv("AMIGA_EMULATION_MAX", smax, 1);
        else
            unsetenv("AMIGA_EMULATION_MAX");
        free(sxdg);
        free(smax);
        for (x = 0; x < 2; x++) {
            snprintf(lock, sizeof lock, "%s/slot-%d.lock", slots, x);
            remove(lock);
        }
        rmdir(slots);
        rmdir(xdg);
    }

    /* the ROM check: missing, plain, encrypted without and with rom.key
       (beside the real file, not beside a link to it) */
    {
        char dir[] = "/tmp/amiga-emulation-romtest-XXXXXX";
        char plain[96], enc[96], key[96], sub[96], link[96];
        FILE *f;

        CHECK(mkdtemp(dir) != NULL);
        snprintf(plain, sizeof plain, "%s/plain.rom", dir);
        snprintf(enc, sizeof enc, "%s/enc.rom", dir);
        snprintf(key, sizeof key, "%s/rom.key", dir);
        snprintf(sub, sizeof sub, "%s/sub", dir);
        snprintf(link, sizeof link, "%s/sub/enc.rom", dir);
        CHECK(amiga_emulation_check_rom(plain, err, sizeof err) == -1 && strstr(err, "missing"));
        if ((f = fopen(plain, "wb")) != NULL) {
            fputs("\x11\x11\x4e\xf9 a plain ROM", f);
            fclose(f);
        }
        if ((f = fopen(enc, "wb")) != NULL) {
            fputs("AMIROMTYPE1 encrypted", f);
            fclose(f);
        }
        CHECK(amiga_emulation_check_rom(plain, err, sizeof err) == 0);
        CHECK(amiga_emulation_check_rom(enc, err, sizeof err) == -1 && strstr(err, "rom.key"));
        CHECK(mkdir(sub, 0700) == 0 && symlink(enc, link) == 0);
        if ((f = fopen(key, "wb")) != NULL)
            fclose(f);
        CHECK(amiga_emulation_check_rom(enc, err, sizeof err) == 0);
        CHECK(amiga_emulation_check_rom(link, err, sizeof err) == 0);
        remove(key);
        CHECK(amiga_emulation_check_rom(link, err, sizeof err) == -1);
        remove(link);
        rmdir(sub);
        remove(plain);
        remove(enc);
        rmdir(dir);
    }

    snapshot_config();
    snapshot_key();
    snapshot_prune();

    /* the names of the @stat lines (tools/lib/teststats/statline.h) */
    CHECK(!strcmp(amiga_emulation_snapshot_use_name(AMIGA_EMULATION_SNAP_COLD), "cold"));
    CHECK(!strcmp(amiga_emulation_snapshot_use_name(AMIGA_EMULATION_SNAP_RESTORED), "restored"));
    CHECK(!strcmp(amiga_emulation_snapshot_use_name(AMIGA_EMULATION_SNAP_FIRST_SAVE),
                  "first-save"));
    CHECK(!strcmp(amiga_emulation_snapshot_use_name(AMIGA_EMULATION_SNAP_FALLBACK), "fallback"));

    if (failures)
        fprintf(stderr, "test_amiga_emulation: %d failures\n", failures);
    return failures != 0;
}
