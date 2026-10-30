/*
 * test_amiga_emulation.c - configuration and script of amiga_emulation
 * (no emulator), the slots that limit the runs.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp */
#include <unistd.h>
#include "testutil.h"
#include "amiga_emulation.h"

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

    /* refused: commas in paths, TEST as extra volume */
    p.nchanges = 0;
    CHECK(amiga_emulation_config(&p, "/emu", "/run,test", err, sizeof err) == NULL);
    mounts[0].volume = "TEST";
    p.nmounts = 1;
    CHECK(amiga_emulation_config(&p, "/emu", "/run/test", err, sizeof err) == NULL && strstr(err, "bad mount"));

    /* the script: FailAt, the text, the end mark */
    s = amiga_emulation_script("Echo hi");
    CHECK(s && strstr(s, "FailAt 1000\nEcho hi\nEcho \"done\" >TEST:run.done\n"));
    free(s);
    s = amiga_emulation_script(NULL);
    CHECK(s && strstr(s, "FailAt 1000\nEcho \"done\" >TEST:run.done\n"));
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
        for (a = 0; a < 2; a++) {
            char lock[64];

            snprintf(lock, sizeof lock, "%s/slot-%d.lock", dir, a);
            remove(lock);
        }
        rmdir(dir);
    }

    if (failures)
        fprintf(stderr, "test_amiga_emulation: %d failures\n", failures);
    return failures != 0;
}
