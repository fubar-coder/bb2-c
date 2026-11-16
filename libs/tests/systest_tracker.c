/*
 * systest_tracker - trackerlib (PlayModule, StopModule: the NoiseTracker
 * player) on the Amiga with a generated module: one set (the original
 * from acidlibs or the C build, as LIBSET says), observed as cases
 * (sysload.h; the expected data libs/tests/expected/systest_tracker.txt).
 *
 *   systest_tracker DIR
 *
 * PlayModule hands the player's interrupt routine (myint) to intlib's
 * SetInt ($C100); the test does not load intlib but a stub that keeps the
 * routine's address, and calls the routine itself, once per frame, so no
 * interrupt plays anything.  Paula's registers are write-only: every
 * absolute address $DFF000-$DFF1FF in the code of the loaded trackerlib
 * (LEA $DFF0A0, MOVE.w ...,$DFF096, MOVE.l #$DFF0A0 ...; as the test of
 * the RWE ritrackerlib) is patched to a RAM copy of the custom chip
 * registers, filled with a pattern before each call.  After every frame
 * the test observes the audio registers and DMACON (AUDxLC as module
 * offsets), the filter bit of CIA-A PRA, the sample starts and the four
 * voices of the player (found in the library by their DMA bits 1, 2, 4,
 * 8; pointers as module offsets) and the kept registers.
 *
 * The module (M.K., 31 samples, fixed seed) has random notes, samples
 * with and without repeat, finetune bytes, and every effect the player
 * knows (arpeggio, portamento up/down, tone portamento, vibrato, volume
 * slide, position jump, set volume, pattern break, filter, speed).  The
 * inputs avoid what reads outside the module or the library's tables
 * (the result would depend on memory outside the test): sample numbers
 * up to 31 (#295), the highest pattern not at position 126 (#289), the
 * first two bytes of the pattern data 0 (the song end compared there
 * after a break, #292), arpeggio only on a note from the upper part of
 * the period table with steps up to 7 (its table search reads behind
 * the table below period 113).  A position jump is only in the last
 * pattern and goes back to the start, so that the song does not stay in
 * a loop of a few rows.
 */
#include "sysload.h"
#include <exec/memory.h>

/* ------------------------------------------------------------ framework */

static struct regs R, IN;
static const char *what;
static int frame;

/* intlib SetInt / ClrInt: the stub keeps the interrupt routine (d1) */
volatile ULONG tr_intcode, tr_setints, tr_clrints;
extern char tr_setint[], tr_clrint[];
__asm__(
"   .text\n"
"   .even\n"
"   .globl  _tr_setint,_tr_clrint\n"
"_tr_setint: move.l d1,_tr_intcode\n   addq.l #1,_tr_setints\n   rts\n"
"_tr_clrint: addq.l #1,_tr_clrints\n   rts\n"
);

static const struct stub stubs[] = {
    { 0xc100, tr_setint },
    { 0xc101, tr_clrint },
    { 0, NULL }
};

/* the code of form `form` of the command `name` of lib (as in
   systest_blit.c) */
static UBYTE *cmd_named(struct lib *l, const char *name, int form)
{
    UBYTE *b = l->code, *p = b + 0x30;
    while (!(W(p) & 0x8000)) {
        UWORD kind = W(p);
        UBYTE *code = NULL;
        p += 6;
        if ((kind & 0xf) == 0) {                /* nullsub */
            while (W(p)) p += 4;
            p += 2 + 12;
        } else {
            int f = 0;
            if (kind != 8) {
                while (W(p) != 0xffff) {        /* forms */
                    p += 2 + (W(p) & 0xff);
                    p = b + ((p - b + 1) & ~1);
                    while (W(p)) p += 4;
                    p += 2;
                    if (f++ == form)
                        code = (UBYTE *)L(p + 4);
                    p += 12;
                }
                p += 2;
            }
            p += 6;                             /* next.l, num.w */
            if (!strcmp((const char *)p, name)) {
                check("cmd_named: the form exists", code != NULL);
                return (UBYTE *)((ULONG)code & ~1UL);
            }
            while (*p++) ;                      /* name */
            while (*p++) ;                      /* help */
            p = b + ((p - b + 1) & ~1);
        }
    }
    check("cmd_named: the command exists", FALSE);
    return NULL;
}

/* the Amiga mode code of a Blitz/Amiga routine (`Dc $a001:Dc.l code` in
   front of it) */
static UBYTE *amigacode(UBYTE *e)
{
    check("a Blitz/Amiga routine", e && W(e) == 0xa001);
    return e && W(e) == 0xa001 ? e + 6 : e;
}

static ULONG seed = 0x2468ace1;
static ULONG rnd(ULONG n)
{
    seed = seed * 1103515245UL + 12345;
    return (seed >> 8) % n;
}

/* ------------------------------------------------------------ the module */

#define MODMAX 24000
#define GUARD 16
#define NPAT 4
static UBYTE *mod;                      /* the module (chip memory) */
static ULONG modsz;
static ULONG obj[2];                    /* the Module object: data, length */

static void putw(UBYTE *p, UWORD v) { p[0] = (UBYTE)(v >> 8); p[1] = (UBYTE)v; }
static void putl(UBYTE *p, ULONG v) { putw(p, (UWORD)(v >> 16)); putw(p + 2, (UWORD)v); }

static const UWORD periods[36] = {
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
    428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
    214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
};

static void note(UBYTE *p, UWORD per, UBYTE smp, UBYTE eff, UBYTE par)
{
    p[0] = (UBYTE)((smp & 0xf0) | (per >> 8 & 0x0f));
    p[1] = (UBYTE)per;
    p[2] = (UBYTE)((smp & 0x0f) << 4 | (eff & 0x0f));
    p[3] = par;
}

/* the module: 31 samples, patterns 0-3, the order below */
static void build(void)
{
    static const UBYTE order[] = { 0, 1, 2, 1, 3, 0, 2, 3 };
    UBYTE *h, *pat;
    ULONG smp = 0;
    int i;

    memset(mod, 0, MODMAX + GUARD);
    memcpy(mod, "trackerlib test", 15);
    for (i = 0; i < 31; i++) {
        UWORD len = 0, rep = 0, rl = 1;

        h = mod + 20 + 30 * i;
        sprintf((char *)h, "sample %d", i + 1);
        if (i < 28) {
            len = (UWORD)(2 + rnd(150));
            if (rnd(2)) {
                rep = (UWORD)rnd(len);
                rl = (UWORD)(2 + rnd(len - rep));
            }
        }
        putw(h + 22, len);
        h[24] = (UBYTE)rnd(16);                 /* finetune (#290) */
        h[25] = (UBYTE)rnd(65);                 /* volume */
        putw(h + 26, rep);
        putw(h + 28, rl);
        smp += 2UL * len;
    }
    mod[950] = sizeof order;
    mod[951] = 0x7f;
    memcpy(mod + 952, order, sizeof order);
    memcpy(mod + 1080, "M.K.", 4);
    pat = mod + 1084;
    for (i = 0; i < NPAT * 64 * 4; i++) {
        UBYTE *p = pat + 4 * i, e = 0, par = 0, s = 0;
        UWORD per = 0;

        if (rnd(3)) {
            per = periods[rnd(36)];
            if (!rnd(25))
                per = 1000;                     /* outside the table */
        }
        if (rnd(3))
            s = (UBYTE)(1 + rnd(31));
        if (rnd(5) < 3) {
            e = (UBYTE)rnd(16);
            par = (UBYTE)rnd(256);
            switch (e) {
            case 0x0:                           /* arpeggio: see the head */
                per = periods[rnd(24)];
                par = (UBYTE)(rnd(8) << 4 | rnd(8));
                break;
            case 0x1:
            case 0x2:
                par = (UBYTE)rnd(16);
                break;
            case 0xb:                           /* only back to the start */
                if (i / 256 == NPAT - 1)
                    par = 0;
                else
                    e = 0xa;
                break;
            case 0xc:
                par = (UBYTE)rnd(0x50);
                break;
            case 0xd:
                if (rnd(3))                     /* fewer breaks */
                    e = 0xa;
                break;
            case 0xf:
                par = rnd(4) ? (UBYTE)(1 + rnd(6)) : (UBYTE)rnd(0x30);
                break;
            }
        }
        note(p, per, s, e, par);
    }
    pat[0] = pat[1] = 0;                        /* #292: see the head */
    note(pat + 4, 428, 1, 0xf, 3);              /* speed 3 */
    h = pat + NPAT * 1024;
    for (i = 0; i < (int)smp; i++)
        h[i] = (UBYTE)rnd(256);
    modsz = (ULONG)(h - mod) + smp;
    memset(mod + modsz, 0x77, GUARD);
}

/* ------------------------------------------------------------ the state */

#define FAKESZ 0x200                    /* $DFF000 ... $DFF1FF */
static UBYTE *fake;                     /* the custom chip registers (RAM) */
static UBYTE *voices;                   /* sample starts (31 longs), 4 voices */
#define VOICESZ (31 * 4 + 4 * 28)

/* the sample starts and the voices: the voices have the DMA bits 1, 2, 4
   and 8 at offset $14, 28 bytes apart, the 31 starts in front of them */
static UBYTE *find_voices(struct lib *l)
{
    ULONG o;

    for (o = 124 + 20; o + 4 * 28 <= l->size; o += 2) {
        const UBYTE *p = l->code + o;

        if (W(p) == 1 && W(p + 28) == 2 && W(p + 56) == 4 && W(p + 84) == 8)
            return l->code + o - 20 - 124;
    }
    return NULL;
}

static UWORD getw(const UBYTE *p) { return (UWORD)(p[0] << 8 | p[1]); }
static ULONG getl(const UBYTE *p) { return (ULONG)getw(p) << 16 | getw(p + 2); }

/* a pointer into the module as offset | $40000000 */
static void rel(UBYTE *p)
{
    ULONG v = getl(p);

    if (v >= (ULONG)mod && v < (ULONG)mod + MODMAX + GUARD)
        putl(p, v - (ULONG)mod + 0x40000000UL);
}

/* every absolute address $DFF000-$DFF1FF in the library code to the fake
   registers */
static int patchcustom(struct lib *l)
{
    ULONG i, v;
    int n = 0;

    for (i = 0; i + 4 <= l->size; i += 2) {
        v = getl(l->code + i);
        if (v >= 0xdff000 && v < 0xdff000 + FAKESZ) {
            putl(l->code + i, (ULONG)fake + v - 0xdff000);
            n++;
            i += 2;
        }
    }
    return n;
}

static void fillfake(void)
{
    int i;

    for (i = 0; i < FAKESZ; i++)
        fake[i] = (UBYTE)(0xa5 ^ i);
}

/* what the run went through: the commands of the notes played, the
   filter states */
static ULONG cmdseen, filters;

static void observe(void)
{
    static UBYTE v[VOICESZ], f[0x40];
    UBYTE pra = *(volatile UBYTE *)0xbfe001 & 2;
    int i;

    for (i = 0; i < 4; i++) {
        const UBYTE *n = voices + 124 + 28 * i;

        if ((n[2] & 0x0f) || n[3])
            cmdseen |= 1UL << (n[2] & 0x0f);
    }
    filters |= 1UL << pra;

    memcpy(v, voices, VOICESZ);
    for (i = 0; i < 31; i++)
        rel(v + 4 * i);
    for (i = 0; i < 4; i++) {
        rel(v + 124 + 28 * i + 4);              /* start */
        rel(v + 124 + 28 * i + 10);             /* loop start */
    }
    OBSBYTES("voices", v, VOICESZ);
    memcpy(f, fake + 0xa0, sizeof f);
    for (i = 0; i < 4; i++)
        rel(f + 16 * i);                        /* AUDxLC */
    OBSBYTES("audio registers", f, sizeof f);
    OBS("DMACON", getw(fake + 0x96));
    OBS("CIA-A PRA filter", pra);
}

static void setvec(void)
{
    sprintf(VEC, "%s frame %d", what, frame);
}

/* call fn (registers: markers + setup), then observe */
#define CALL(rout, fn, setup) do {                                          \
        ROUT = (rout);                                                      \
        setvec();                                                           \
        fillfake();                                                         \
        markregs(&R);                                                       \
        setup;                                                              \
        IN = R;                                                             \
        callregs((fn), &R, 0, 0);                                           \
        obs_kept(&IN, &R);                                                  \
        observe();                                                          \
    } while (0)

/* n frames of the interrupt routine */
static void frames(int n)
{
    int k;

    for (k = 0; k < n; k++, frame++)
        CALL("myint", (APTR)tr_intcode, (void)0);
}

/* ------------------------------------------------------------------ main */

static const char *names[] = { "memlib", "trackerlib", NULL };

int main(int argc, char **argv)
{
    struct lib *l;
    UBYTE *play, *stop, *finit;
    int npatch;
    /* the filter bit the player changes: restored at the end for the
       programs after this one (rwe_richunkylib reads it) */
    UBYTE pra = *(volatile UBYTE *)0xbfe001 & 2;

    if (argc < 2) {
        printf("usage: systest_tracker DIR\n");
        return 20;
    }
    sysload_stubs = stubs;
    if (!load_set(argv[1], names))
        return 20;
    KEEPS_STATEMENT("PlayModule", none);
    KEEPS_STATEMENT("StopModule", none);
    /* the interrupt routine keeps every register (the original saves
       them, the C build too) */
    KEEPS("myint", d0-d7|a0-a3|a6);
    KEEPS("trackerlib finit", d1-d7|a0|a1|a3|a6);
    l = getlib(&loaded, "trackerlib");
    play = amigacode(cmd_named(l, "PlayModule", 0));
    stop = amigacode(cmd_named(l, "StopModule", 0));
    finit = entry_code(l, 4);
    voices = find_voices(l);
    mod = AllocMem(MODMAX + GUARD, MEMF_CHIP | MEMF_CLEAR);
    fake = AllocMem(FAKESZ, MEMF_PUBLIC | MEMF_CLEAR);
    if (!play || !stop || !finit || !voices || !mod || !fake) {
        printf("trackerlib %s: entries, voices or memory missing\n", loaded.what);
        return 20;
    }
    npatch = patchcustom(l);
    CacheClearU();
    printf("trackerlib %s: custom chip addresses patched: %d\n", loaded.what, npatch);
    check("trackerlib: custom chip addresses patched", npatch > 0);

    build();
    obj[0] = (ULONG)mod;
    obj[1] = modsz;
    what = "PlayModule";
    frame = 0;
    CALL("PlayModule", play, R.a[3] = (ULONG)obj);
    OBS("SetInt calls", tr_setints);
    OBS("interrupt routine set", tr_intcode != 0);
    /* mt_init clears the first long of each sample */
    OBSBYTES("module", mod, modsz + GUARD);
    if (!tr_intcode)
        return 20;
    what = "playing";
    frames(4000);
    printf("trackerlib %s: commands $%04lX, filter states $%lX\n", loaded.what, cmdseen,
           filters);
    check("trackerlib: the module plays every command, the filter on and off",
          cmdseen == 0xffff && filters == 5);
    what = "StopModule";
    CALL("StopModule", stop, (void)0);
    OBS("ClrInt calls", tr_clrints);

    /* again from the start, then the library's finit */
    build();
    what = "PlayModule again";
    frame = 0;
    CALL("PlayModule", play, R.a[3] = (ULONG)obj);
    what = "playing again";
    frames(300);
    what = "finit";
    CALL("trackerlib finit", finit, (void)0);
    OBS("ClrInt calls", tr_clrints);
    OBSBYTES("module", mod, modsz + GUARD);

    *(volatile UBYTE *)0xbfe001 = (*(volatile UBYTE *)0xbfe001 & ~2) | pra;
    FreeMem(mod, MODMAX + GUARD);
    FreeMem(fake, FAKESZ);
    printf("%d checks, %d failures\n", checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}
