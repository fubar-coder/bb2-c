/*
 * rwe ritrackerlib - the Protracker player (TrackInit, TrackPlay, ...):
 * the C build or the reference build of rwelibsrc/ptlib/ptlib6.bb
 * (ritrackerlib.orig), one set as LIBSET says, observed as cases
 * (rwecmp.h, LIBS_ONE_SET; the expected data
 * libs/tests/expected/rwe_ritrackerlib.txt).
 *
 *   ritrackerlib DIR     (DIR with memlib, intlib, ritrackerlib .obj/.orig)
 *
 * Paula's registers are write-only: the test redirects them.  Every
 * absolute address $DFF000-$DFF1FF in the code of the loaded library
 * (LEA $DFF0A0, MOVE.W ...,$DFF096, MOVE.L #$DFF000 ...) is patched to a
 * RAM copy of the custom chip registers (at least one; their number
 * differs between the builds), filled with a pattern before each call; afterwards the registers
 * written (AUDxLC as module offsets, AUDxLEN/PER/VOL, DMACON's last
 * value) are observed, and no audio is played.  Also observed after
 * every call: the data block mt_FunkTable ... mt_olddata (1538 bytes: the
 * voices' n_note ... n_reallength, the sample starts, speed, counter,
 * song position, pattern position, flags, DMA_Wait, playing, status,
 * mask) and the glue variables (ciatime ... mt_data) - the C build has
 * both in the original's layout, the test finds them in the library (by
 * the tables, by RealTempo and the module address); pointers into the
 * module as offsets; the module bytes and 16 bytes behind it (mt_init
 * clears the first longword of every sample, also behind the module, EFx
 * inverts sample bytes); the filter bit of CIA-A PRA; the counter of
 * CIA-B timer A (a tempo command writes the timer's latch; the test
 * allocates the timer through ciab.resource and keeps it stopped, so
 * writing the high byte loads the counter); and the kept registers.
 *
 * The module is built in memory: 31 samples (random lengths,
 * loops, finetunes, volumes; empty ones at the end), four patterns with
 * random notes and every effect (speeds 1-8, tempos, position jumps,
 * pattern breaks, loops, delays, retrigs, invert loop ...), and a
 * second, small module for the speed/tempo corner cases (F80: a speed,
 * BUG(orig) #569; F00 on voice 1 stops, and voices 2-4 then read their notes
 * from the custom chip registers, BUG(orig) #475).  Rows by hand: the first
 * 1xx (mt_LowMask 0, BUG(orig), #478), volume above 64, tremolo's ramp-down
 * with a negative vibrato position (BUG(orig), #482), 9xx with a note
 * (twice, BUG(orig), #473), invert loop, retrig.  Covered besides playing: TrackDMA, Tracker Off
 * (mt_end) / On (stays stopped, BUG(orig), #572), TrackInit while Tracker Off
 * (no mt_init, BUG(orig), #573), TrackerChannels (the voices shift,
 * BUG(orig), #568), TrackerPause, TrackStop, init, finit with and without
 * TrackInit.  Not covered: sample numbers above 31, finetune bytes above
 * 15 (both read outside the library's tables, different memory in the
 * two builds), and the dead CIA/SetInt code.
 */
#define LIBS_ONE_SET
#include "rwecmp.h"
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <hardware/cia.h>
#include <resources/cia.h>
#include <proto/cia.h>

#define LIB "ritrackerlib"
#define CIAAPRA (*(volatile UBYTE *)0xbfe001)
#define CIABTALO (*(volatile UBYTE *)0xbfd400)
#define CIABTAHI (*(volatile UBYTE *)0xbfd500)
#define CIABCRA (*(volatile UBYTE *)0xbfde00)

enum { TRACKINIT, TRACKPLAY, TRACKSTOP, TRACKDMA, TRACKER, TRACKERCHANNELS, TRACKERPAUSE,
       FINIT, INIT };

#define BLOCKSZ 1538                    /* mt_FunkTable ... mt_olddata */
#define GLUESZ 28                       /* ciatime ... mt_data */
#define MODMAX 24000
#define GUARD 16

static UBYTE ref[MODMAX];
static UBYTE *mod;
static ULONG modsz;
static UBYTE *block, *glue;
#define FAKESZ 0x200                    /* $DFF000 ... $DFF1FF */
static UBYTE *fake;                     /* the custom chip registers (RAM) */
static UWORD hwtim;
static UBYTE hwpra;
static int havetimer;

static UWORD getw(const UBYTE *p) { return (UWORD)(p[0] << 8 | p[1]); }
static void putw(UBYTE *p, UWORD v) { p[0] = (UBYTE)(v >> 8); p[1] = (UBYTE)v; }
static ULONG getl(const UBYTE *p) { return (ULONG)getw(p) << 16 | getw(p + 2); }
static void putl(UBYTE *p, ULONG v) { putw(p, (UWORD)(v >> 16)); putw(p + 2, (UWORD)v); }

/* the data block: mt_FunkTable, mt_VibratoTable, then mt_PeriodTable */
static UBYTE *findblock(struct lib *l)
{
    static const UBYTE funk[16] = { 0, 5, 6, 7, 8, 10, 11, 13, 16, 19, 22, 26, 32, 43, 64, 128 };
    ULONG i;
    for (i = 0; i + BLOCKSZ <= l->size; i += 2)
        if (!memcmp(l->code + i, funk, 16) && getw(l->code + i + 48) == 856
            && getw(l->code + i + 50) == 808)
            return l->code + i;
    return NULL;
}

/* the glue variables: RealTempo (125 before any tempo command), mt_data */
static UBYTE *findglue(struct lib *l, UBYTE *m, UBYTE *blk)
{
    ULONG i;
    for (i = 24; i + 4 <= l->size; i += 2)
        if (getl(l->code + i) == (ULONG)m && getw(l->code + i - 2) == 125
            && l->code + i != blk + 1504)
            return l->code + i - 24;
    return NULL;
}

/* a copy with every pointer into the module as offset | $40000000 */
static void rel(UBYTE *p)
{
    ULONG v = getl(p);
    if (v >= (ULONG)mod && v < (ULONG)mod + modsz + GUARD + 0x10000)
        putl(p, v - (ULONG)mod + 0x40000000UL);
}

static void snapshot(UBYTE *b, UBYTE *g)
{
    int i;
    memcpy(b, block, BLOCKSZ);
    for (i = 0; i < 4; i++) {
        rel(b + 1200 + 44 * i + 4);          /* n_start */
        rel(b + 1200 + 44 * i + 10);         /* n_loopstart */
        rel(b + 1200 + 44 * i + 36);         /* n_wavestart */
    }
    for (i = 0; i < 32; i++)
        rel(b + 1376 + 4 * i);               /* mt_SampleStarts */
    rel(b + 1504);                           /* mt_SongDataPtr */
    rel(b + 1534);                           /* mt_olddata */
    if (g && glue) {
        memcpy(g, glue, GLUESZ);
        rel(g + 24);                         /* mt_data */
    }
}

/* the custom chip registers before each call: notes for an F00 on voices
   1-3 (the voices behind read their notes here, BUG(orig), #475): period 428,
   sample 1, C20 */
static void fillfake(void)
{
    static const UBYTE n[4] = { 0x01, 0xac, 0x1c, 0x20 };
    int i;
    for (i = 0; i < FAKESZ; i++)
        fake[i] = n[i & 3];
}

/* every absolute address $DFF000-$DFF1FF in the library code (the LEA,
   MOVE.L #, absolute word writes) to the set's fake registers */
static int patchcustom(struct lib *l, UBYTE *f)
{
    ULONG i, v;
    int n = 0;
    for (i = 0; i + 4 <= l->size; i += 2) {
        v = getl(l->code + i);
        if (v >= 0xdff000 && v < 0xdff000 + FAKESZ) {
            putl(l->code + i, (ULONG)f + v - 0xdff000);
            n++;
            i += 2;
        }
    }
    return n;
}

static void compare(void)
{
    static UBYTE b[BLOCKSZ], g[GLUESZ], f[FAKESZ];
    int i;
    snapshot(b, g);
    OBSBYTES("state", b, BLOCKSZ);
    if (glue)
        OBSBYTES("glue", g, GLUESZ);
    OBSBYTES("module", mod, modsz + GUARD);
    memcpy(f, fake, FAKESZ);
    for (i = 0; i < 4; i++)
        rel(f + 0xa0 + 16 * i);                 /* AUDxLC */
    OBSBYTES("custom", f, FAKESZ);
    OBS("CIA-A PRA filter", hwpra);
    OBS("CIA-B timer A", hwtim);
}

/* one call, the hardware state defined before it */
static void call1(int idx, ULONG d0)
{
    APTR fn = ENT(LIB, idx, 0);
    fillfake();
    if (havetimer) {
        CIABTALO = 0xa5;
        CIABTAHI = 0x5a;
    }
    markregs(&R);
    if (idx == TRACKINIT)
        R.d[0] = d0 ? (ULONG)mod : 0;
    else
        R.d[0] = d0;
    IN = R;
    if (!fn) {
        check("entry missing", FALSE);
        return;
    }
    NARGS = 0;
    CCR = callstk(fn, &R, ARGS, NARGS, &POP);
    hwpra = CIAAPRA & 2;
    hwtim = havetimer ? (UWORD)(CIABTAHI << 8 | CIABTALO) : 0;
    preserve();
    compare();
}

static void cmd(int idx, const char *name, ULONG d0)
{
    ROUT = name;
    sprintf(VEC, "$%lX", d0);
    call1(idx, d0);
}

static void ticks(const char *what, int n)
{
    int i;
    ROUT = "TrackPlay";
    for (i = 0; i < n; i++) {
        sprintf(VEC, "%s tick %d", what, i);
        call1(TRACKPLAY, 0);
    }
}

/* ------------------------------------------------------------ the module */

static const UWORD periods[36] = {
    856,808,762,720,678,640,604,570,538,508,480,453,
    428,404,381,360,339,320,302,285,269,254,240,226,
    214,202,190,180,170,160,151,143,135,127,120,113 };

static ULONG rn(ULONG n) { return rnd() % n; }

static void note(UBYTE *p, UWORD per, UBYTE smp, UBYTE eff, UBYTE par)
{
    p[0] = (UBYTE)((smp & 0xf0) | (per >> 8 & 0x0f));
    p[1] = (UBYTE)per;
    p[2] = (UBYTE)((smp & 0x0f) << 4 | (eff & 0x0f));
    p[3] = par;
}

/* a random effect; speeds 1-8, no F00/F80+, jumps inside the song */
static void randeffect(UBYTE *eff, UBYTE *par, UBYTE songlen)
{
    UBYTE e = (UBYTE)rn(16), p = (UBYTE)rnd();
    switch (e) {
    case 0xb:
        p = (UBYTE)rn(songlen);
        break;
    case 0xd:
        p = (UBYTE)(rn(7) << 4 | rn(10));
        break;
    case 0xe:
        p = (UBYTE)(rn(16) << 4 | rn(16));
        if ((p >> 4) == 0xe)
            p = 0xe0 | (UBYTE)rn(3);
        break;
    case 0xf:
        p = rn(2) ? (UBYTE)(1 + rn(8)) : (UBYTE)(0x20 + rn(0x60));
        break;
    case 0xc:
        p = (UBYTE)rn(0x50);
        break;
    }
    *eff = e;
    *par = p;
}

/* the main module: 31 samples, patterns 0-3, order 0 1 2 1 3 0 2 */
static void build_main(void)
{
    static const UBYTE order[] = { 0, 1, 2, 1, 3, 0, 2 };
    UBYTE *h, *pat;
    ULONG smp = 0;
    int i, r;

    memset(ref, 0, sizeof ref);
    memcpy(ref, "ritrackerlib test", 17);
    for (i = 0; i < 31; i++) {
        UWORD len = 0, rep = 0, rl = 1;
        h = ref + 20 + 30 * i;
        sprintf((char *)h, "sample %d", i + 1);
        if (i > 0 && i < 27) {
            len = (UWORD)(2 + rn(120));
            if (rn(2)) {
                rep = (UWORD)rn(len);
                rl = (UWORD)(1 + rn(len - rep));
            }
        }
        putw(h + 22, len);
        h[24] = (UBYTE)rn(16);                  /* finetune */
        h[25] = i == 5 ? 0x50 : (UBYTE)rn(65);  /* volume (0x50: above 64) */
        putw(h + 26, rep);
        putw(h + 28, rl);
        smp += 2UL * len;
    }
    ref[950] = sizeof order;
    ref[951] = 127;
    memcpy(ref + 952, order, sizeof order);
    memcpy(ref + 1080, "M.K.", 4);
    pat = ref + 1084;
    for (i = 0; i < 4 * 64 * 4; i++) {
        UBYTE *p = pat + 4 * i, e = 0, par = 0;
        UWORD per = 0;
        UBYTE s = 0;
        if (rn(2)) {
            per = periods[rn(36)];
            if (!rn(20))
                per = rn(2) ? 100 : 1000;       /* outside the table */
        }
        if (rn(3))
            s = (UBYTE)rn(32);
        if (rn(5) < 2)
            randeffect(&e, &par, sizeof order);
        note(p, per, s, e, par);
    }
    /* a few rows by hand at the start of pattern 0 */
    r = 0;
    note(pat + 16 * r + 0, 428, 1, 0xf, 3);             /* speed 3 */
    note(pat + 16 * r + 4, 381, 2, 0x0, 0x47);          /* arpeggio */
    note(pat + 16 * r + 8, 320, 3, 0x1, 0x04);          /* first 1xx: LowMask 0 */
    note(pat + 16 * r + 12, 254, 6, 0xa, 0x0f);         /* volume 0x50 slides */
    r = 1;
    note(pat + 16 * r + 0, 0, 0, 0x3, 0x10);
    note(pat + 16 * r + 4, 214, 4, 0xe, 0x71);          /* tremolo ramp */
    note(pat + 16 * r + 8, 0, 0, 0x2, 0x08);
    note(pat + 16 * r + 12, 404, 7, 0x7, 0x6a);         /* tremolo */
    r = 2;
    note(pat + 16 * r + 0, 856, 5, 0xe, 0x41);          /* vibrato ramp */
    note(pat + 16 * r + 4, 0, 0, 0x4, 0xff);            /* vibrato: position */
    note(pat + 16 * r + 8, 113, 9, 0x9, 0x02);          /* 9xx with a note */
    note(pat + 16 * r + 12, 170, 10, 0xe, 0xf8);        /* invert loop */
    r = 3;
    note(pat + 16 * r + 0, 0, 0, 0x4, 0x00);
    note(pat + 16 * r + 4, 0, 0, 0x4, 0xff);            /* ... negative */
    note(pat + 16 * 4 + 4, 0, 0, 0x7, 0x6a);            /* tremolo ramp tests */
    note(pat + 16 * 5 + 4, 0, 0, 0x7, 0x00);            /* the vibrato position */
    note(pat + 16 * r + 8, 0, 0, 0x6, 0x03);
    note(pat + 16 * r + 12, 0, 0, 0xe, 0x93);           /* retrig */
    /* sample data */
    h = pat + 4 * 1024;
    for (i = 0; i < (int)smp; i++)
        h[i] = (UBYTE)rnd();
    modsz = (ULONG)(h - ref) + smp;
}

/* the second module: one pattern for Fxx: F80 (a speed), F20, then F00 */
static void build_speed(void)
{
    UBYTE *pat;
    memset(ref, 0, sizeof ref);
    putw(ref + 20 + 22, 50);
    ref[20 + 25] = 64;
    putw(ref + 20 + 28, 1);
    ref[950] = 2;
    ref[951] = 127;
    ref[952] = 0;
    ref[953] = 0;
    ref[952 + 127] = 1;                 /* position 127: not scanned */
    memcpy(ref + 1080, "M.K.", 4);
    pat = ref + 1084;
    note(pat + 16 * 0, 428, 1, 0xf, 0x81);
    note(pat + 16 * 1, 428, 1, 0xf, 0x02);
    note(pat + 16 * 2, 0, 0, 0xf, 0x20);
    note(pat + 16 * 3, 0, 0, 0xf, 0x7d);
    note(pat + 16 * 4, 0, 0, 0xe, 0x01);                /* filter */
    note(pat + 16 * 5, 0, 0, 0xf, 0x00);                /* stop on voice 1: voices 2-4 read
                                                           their notes from the custom chip
                                                           registers (BUG(orig), #475) */
    modsz = 1084 + 1024 + 100;
    memset(ref + 1084 + 1024, 0x11, 100);
}

static void install(void)
{
    memcpy(mod, ref, MODMAX);
    memset(mod + modsz, 0x77, GUARD);
}

static void dummy_int(void)
{
}

int main(int argc, char **argv)
{
    static const char *names[] = { "memlib", "intlib", LIB, NULL };
    static struct Interrupt timint;
    struct Library *ciab;
    UBYTE cra = 0;
    int npatch = 0;

    if (argc < 2) {
        printf("usage: ritrackerlib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("finit (no TrackInit)", d0-d7|a0-a3|a6);
    KEEPS("TrackInit", d3-d7|a3|a6);
    KEEPS("TrackPlay", d0-d7|a0-a3|a6);
    KEEPS("TrackDMA", d0-d7|a0-a3|a6);
    KEEPS("TrackerChannels", d0-d7|a1-a3|a6);
    KEEPS("Tracker", d0-d7|a0-a3|a6);
    KEEPS("TrackerPause", d0-d7|a0-a3|a6);
    KEEPS("TrackInit (Tracker Off)", d0-d7|a0-a3|a6);
    KEEPS("TrackStop", d0-d7|a1-a3|a6);
    KEEPS("TrackInit (speed module)", d3-d7|a3|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);
    KEEPS("finit (again)", d0-d7|a0-a3|a6);
    mod = AllocMem(MODMAX + GUARD, MEMF_CHIP | MEMF_CLEAR);
    block = findblock(getlib(&loaded, LIB));
    fake = AllocMem(FAKESZ, MEMF_PUBLIC | MEMF_CLEAR);
    if (fake)
        npatch = patchcustom(getlib(&loaded, LIB), fake);
    CacheClearU();
    printf("custom chip addresses patched: %d\n", npatch);
    check("ritrackerlib: custom chip addresses patched", npatch > 0);
    check("ritrackerlib: data block found", block != NULL);
    if (!block || !mod || !fake)
        return rwe_done(LIB);

    /* CIA-B timer A for the test, stopped, continuous mode */
    timint.is_Node.ln_Type = NT_INTERRUPT;
    timint.is_Node.ln_Name = (char *)"ritrackerlib test";
    timint.is_Code = (void (*)())dummy_int;
    ciab = OpenResource((CONST_STRPTR)CIABNAME);
    if (ciab && !AddICRVector(ciab, CIAICRB_TA, &timint)) {
        havetimer = 1;
        cra = CIABCRA;
        CIABCRA = cra & 0xf6;
    }
    printf("CIA-B timer A %s\n", havetimer ? "allocated" : "in use: not compared");

    ROUT = "init";
    strcpy(VEC, "");
    call1(INIT, 0);
    cmd(FINIT, "finit (no TrackInit)", 0);

    build_main();
    install();
    cmd(TRACKINIT, "TrackInit", 1);
    glue = findglue(getlib(&loaded, LIB), mod, block);
    check("ritrackerlib: glue found", glue != NULL);
    compare();

    ticks("DMA_Wait 4000", 12);
    cmd(TRACKDMA, "TrackDMA", 0xa5a50014UL);
    ticks("main", 1300);

    cmd(TRACKERCHANNELS, "TrackerChannels", 0x0b);
    ticks("voice 2 off", 60);
    cmd(TRACKERCHANNELS, "TrackerChannels", 0x06);
    ticks("voices 1 and 4 off", 40);
    cmd(TRACKERCHANNELS, "TrackerChannels", 0x0f);

    cmd(TRACKER, "Tracker", 0xff);              /* = status: nothing */
    cmd(TRACKER, "Tracker", 0);
    ticks("Tracker Off", 4);
    cmd(TRACKER, "Tracker", 0xff);
    ticks("Tracker On after Off", 4);
    cmd(TRACKERPAUSE, "TrackerPause", 0);
    ticks("after TrackerPause 0", 30);
    cmd(TRACKERPAUSE, "TrackerPause", 1);
    ticks("paused", 3);
    cmd(TRACKERPAUSE, "TrackerPause", 0);
    ticks("continued", 20);

    cmd(TRACKER, "Tracker", 0);
    build_speed();
    install();
    cmd(TRACKINIT, "TrackInit (Tracker Off)", 1);
    cmd(TRACKER, "Tracker", 1);
    ticks("Tracker 1: old song", 20);
    cmd(TRACKSTOP, "TrackStop", 0);

    cmd(TRACKINIT, "TrackInit (speed module)", 1);
    ticks("F81", 140);
    ticks("F02 ... F00", 30);
    cmd(TRACKSTOP, "TrackStop", 0);
    cmd(FINIT, "finit", 0);
    cmd(FINIT, "finit (again)", 0);

    if (havetimer) {
        CIABCRA = cra;
        RemICRVector(ciab, CIAICRB_TA, &timint);
    }
    FreeMem(mod, MODMAX + GUARD);
    FreeMem(fake, FAKESZ);
    return rwe_done(LIB);
}
