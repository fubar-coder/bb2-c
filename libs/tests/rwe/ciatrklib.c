/*
 * rwe ciatrklib - the C build of ciatrklib (userlib 56, the CIA
 * Protracker library) or the reference build of its source
 * (ref/ciatrklib.obj, see libs/userlibs/rwe/refbuild.cmake), one set as
 * LIBSET says, observed as cases (rwecmp.h, LIBS_ONE_SET; the expected
 * data libs/tests/expected/rwe_ciatrklib.txt).
 *
 *   ciatrklib DIR     (DIR with ciatrklib and memlib, .obj/.orig)
 *
 * Paula's registers cannot be read, so the test observes the library's
 * state: its three data blocks (found in both builds: the library's
 * variables with the module table, the CIA part, the play routine's
 * voices, sample starts, song position, speed ... - the C build keeps
 * the original's layout), with the pointers into the modules taken
 * relative to the module; the register writes of masked channels
 * (fake_channel); DMACONR's audio bits and the filter bit of CIA-A PRA
 * after every call; the modules' bytes (E8x/EFx change samples).  The
 * modules are built in memory: random notes and effects (fixed seed) in
 * 6 patterns, 31 samples (loops, finetunes 0-14, a volume above 64, a
 * sample of length 0).
 *
 * Two phases:
 *   A (CIA): AddICRVector/RemICRVector of the CIA
 *     resources are stand-ins that record their calls (the real ones
 *     with the original's ICR bits would remove the system's ciaa timer
 *     A vector); ciaa's fails, so init takes ciab's timer B.  Observed:
 *     the calls (BUG(orig) #450 #448: the bits are the longs at the addresses 1 and
 *     2; RemICRVector with bit 0), the CIA part, the timer started and
 *     stopped, the interrupt server called by hand (the tempo command,
 *     the event), StartTracker (reinstalls), WaitTrackerEvent with a
 *     WaitTOF that runs the server, ChangeTrackerVBlank (the filter).
 *   B (VBlank, call by call): DecodeModule (chip: linked; fast
 *     memory: copied, size + 3), the information functions also on an
 *     empty slot (BUG(orig) #451 #430 #437: StartTracker returns the slot number,
 *     CheckTrackerID the long at address 0, GetModuleName$ slot*10),
 *     the error checks (no error), StartTracker (mt_init; a slot length
 *     above the sample addresses clears their first longs, BUG(orig), #438),
 *     then PlayTracker tick by tick with all channels on Paula, with the
 *     masks 5 and 0 (fake_channel), TrackerVolume (0, 1, 32, 63, 64,
 *     65), ModulePositionJump (n + 1, BUG(orig), #447), GetModulePosition,
 *     PauseModule, Get/CheckTrackerEvent, WaitTrackerEvent (its WaitTOF
 *     plays one tick), StopTracker, LoadTrackerModule (a6 = DOSBase;
 *     a file size with the low byte $FE: the byte add, BUG(orig), #442),
 *     FreeTrackerModule (loaded, copied, linked), ModuleToChip, finit.
 *     F00 on voice 1, 2 and 3 of a small module (BUG(orig) #475: mt_end loads
 *     a0 with $DFF000, the voices behind read their notes from the
 *     custom chip registers): the $DFF000 of mt_end (the original's
 *     LEA operand, the C build's endcustom behind the CIA part) is
 *     patched in both builds to a RAM copy of the registers, filled
 *     with a note, only for these calls (the routine polls VHPOSR,
 *     DMACONR and INTREQR: they cannot be redirected); DMACONR is not
 *     captured there (the original writes DMACON into the hardware, the
 *     C build into its RAM copy).
 * Not covered: the runtime errors (missing file, error checks that
 * fail), a Read error, failing allocations, sample numbers above 31 and
 * finetune 15 / above 15 (they read behind the tables into data that
 * holds pointers, or behind the data), E8x with a loop length of 0.
 */
#define LIBS_ONE_SET
#include "rwecmp.h"
#include <exec/memory.h>
#include <hardware/custom.h>

#define LIB "ciatrklib"

enum { LOAD, START, STOP, DECODE, SIZE, LOC, FREE, GETEVT, CHKEVT, WAITEVT, CHKID, TOCHIP,
       NAME, POSJUMP, GETPOS, PAUSE, VBLANK, PLAY, VOLUME, MASK, PATTERNS, INIT, FINIT };

#define CUSTOMR (*(volatile struct Custom *)0xdff000)
#define PRA (*(volatile UBYTE *)0xbfe001)

static UBYTE *blkP, *blkC, *blkG;

/* offsets in the blocks */
#define P_CHAN      1200                /* 4 x 44 */
#define P_FAKE      1376
#define P_STARTS    1436
#define P_SONGDATA  1564
#define P_SPEED     1568
#define P_COUNTER   1569
#define P_SONGPOS   1570
#define P_LOWMASK   1574
#define P_ENABLE    1577
#define P_PATTPOS   1578
#define P_TRKEVT    1582
#define P_LSTEVT    1583
#define P_SIZE      1584
#define C_REALTEMPO 0
#define C_CIAADDR   2
#define C_CIABASE   34
#define C_TIMERFLAG 38
#define C_TIMERVAL  42
#define C_MTDATA    46
#define C_INT       68
#define C_SIZE      110
#define G_MODTABLE  10
#define G_SIZE      110

static ULONG GL(const UBYTE *p) { return *(const ULONG *)p; }
static UWORD GW(const UBYTE *p) { return *(const UWORD *)p; }
static void PL(UBYTE *p, ULONG v) { *(ULONG *)p = v; }

/* ------------------------------------------------- pointer regions */

/* memory regions (modules and their copies): a pointer into region k is
   observed as k << 24 | offset */
#define NREG 8
static struct { UBYTE *at; ULONG len; } reg[NREG];

static int region(UBYTE *a, ULONG len)
{
    int k;
    for (k = 0; k < NREG; k++)
        if (!reg[k].len) {
            reg[k].at = a;
            reg[k].len = len;
            return k;
        }
    return -1;
}

static void unregion(int k)
{
    if (k >= 0)
        reg[k].len = 0;
}

/* inside a region, else up to 64 KB behind one (a sample offset moves
   n_start beyond the sample) */
static ULONG norm(ULONG v)
{
    ULONG slack;
    int k;
    for (slack = 0; slack <= 0x10000; slack += 0x10000)
        for (k = 0; k < NREG; k++)
            if (reg[k].len && v >= (ULONG)reg[k].at && v < (ULONG)reg[k].at + reg[k].len + slack)
                return 0x80000000UL | (ULONG)k << 24 | (v - (ULONG)reg[k].at);
    return v;
}

/* ------------------------------------------------- block observations */

static UBYTE tmp[P_SIZE];
static UWORD poffs[60];
static int npoffs;

static void sameblock(const char *what, const UBYTE *blk, ULONG size, const UWORD *ptr,
                      int nptr, const UWORD *zero, int nzero)
{
    int i;
    memcpy(tmp, blk, size);
    for (i = 0; i < nptr; i++)
        PL(tmp + ptr[i], norm(GL(tmp + ptr[i])));
    for (i = 0; i < nzero; i++)
        PL(tmp + zero[i], 0);
    OBSBYTES(what, tmp, size);
}

static void sameP(void)
{
    if (blkP)
        sameblock("play routine", blkP, P_SIZE, poffs, npoffs, NULL, 0);
}

static void sameG(void)
{
    static const UWORD ptr[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
    if (blkG)
        sameblock("modtable", blkG, G_SIZE, ptr, 10, NULL, 0);
}

static void sameC(void)
{
    /* the interrupt's node links and its name/code (in the library) */
    static const UWORD ptr[] = { C_MTDATA };
    static const UWORD zero[] = { C_INT, C_INT + 4, C_INT + 10, C_INT + 18 };
    if (blkC)
        sameblock("CIA part", blkC, C_SIZE, ptr, 1, zero, 4);
}

/* after every call: the blocks, Paula's DMA bits, the filter */
static void sameall(void)
{
    sameP();
    sameG();
    sameC();
}

/* the F00 phase (mt_end patched): the C build writes DMACON through its
   patched custom chip pointer into the RAM copy, the original through
   $DFF096 into the hardware, so DMACONR after a call there shows the
   original's writes only: not captured (design D8 of the openspec change
   libs-tests-capture) */
static int fphase;

/* ------------------------------------------------------ calls */

/* the long at a fixed low address (through a register: a constant
   address would be a null pointer offset for GCC) */
static ULONG peekl(ULONG addr)
{
    const ULONG *q;
    __asm__("" : "=a"(q) : "0"(addr));
    return *(volatile const ULONG *)q;
}

/* a statement / function with the state after it observed */
static void stmt(const char *rout, int idx, ULONG d0)
{
    APTR fn = ENT(LIB, idx, 0);
    UWORD dmaconr;
    UBYTE pra;

    ROUT = rout;
    markregs(&R);
    R.d[0] = d0;
    IN = R;
    CCR = callstk(fn, &R, NULL, 0, &POP);
    dmaconr = CUSTOMR.dmaconr & 0xf;
    pra = PRA & 2;
    preserve();
    if (fphase)
        NOT_CAPTURED("DMACONR", "F00 phase: the original writes DMACON into the hardware, "
                     "the C build into its RAM copy");
    else
        OBS("DMACONR", dmaconr);
    OBS("filter", pra);
    sameall();
}

static void func(const char *rout, int idx, ULONG d0)
{
    stmt(rout, idx, d0);
    OBS("d0", norm(R.d[0]));
}

/* ------------------------------------------------------ the modules */

static const UWORD pt0[36] = {
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
    428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
    214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113 };

#define NPAT 6
#define SONGLEN 9
#define MODMAX 12000

static UBYTE ref[MODMAX];
static ULONG reflen;

static void pw(UBYTE *p, UWORD v)
{
    p[0] = (UBYTE)(v >> 8);
    p[1] = (UBYTE)v;
}

static ULONG rndn(ULONG n)
{
    return (rnd() >> 8) % n;
}

/* one note: period, sample, command */
static void cell(UBYTE *c)
{
    UWORD per = 0, smp = 0;
    UBYTE cmd = 0, par = 0;
    ULONG r = rndn(100);

    if (r < 55)
        per = pt0[rndn(36)];
    else if (r < 58)
        per = r == 55 ? 100 : r == 56 ? 900 : 1000;   /* not in the table */
    if (rndn(3))
        smp = 1 + rndn(31);
    switch (rndn(34)) {
    case 0: case 1: case 2: cmd = 0; par = (UBYTE)rnd(); break;
    case 3: cmd = 1; par = (UBYTE)rndn(16); break;
    case 4: cmd = 2; par = (UBYTE)rndn(16); break;
    case 5: case 6: cmd = 3; par = (UBYTE)rndn(32); break;
    case 7: cmd = 4; par = (UBYTE)rnd(); break;
    case 8: cmd = 5; par = (UBYTE)rnd(); break;
    case 9: cmd = 6; par = (UBYTE)rnd(); break;
    case 10: cmd = 7; par = (UBYTE)rnd(); break;
    case 11: case 12: cmd = 8; par = (UBYTE)rndn(16); break;
    case 13: cmd = 9; par = (UBYTE)rndn(8); break;
    case 14: cmd = 0xa; par = (UBYTE)rnd(); break;
    case 15: cmd = 0xb; par = (UBYTE)rndn(SONGLEN + 2); break;
    case 16: cmd = 0xc; par = (UBYTE)rndn(0x50); break;
    case 17: cmd = 0xd; par = (UBYTE)rnd(); break;
    case 18: cmd = 0xf; par = (UBYTE)(1 + rndn(8)); break;
    case 19: cmd = 0xf; par = (UBYTE)(32 + rndn(200)); break;
    case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: {
        UBYTE x = (UBYTE)rndn(16), y = (UBYTE)rndn(16);
        if (x == 5 && y == 15)
            y = 14;                     /* no finetune 15 */
        if ((x == 8 || x == 15) && !smp)
            smp = 1 + rndn(31);         /* E8x/EFx: on a sample of the module */
        if (x == 0xe)
            y &= 3;
        cmd = 0xe;
        par = (UBYTE)(x << 4 | y);
        break;
    }
    default:
        break;
    }
    c[0] = (UBYTE)((smp & 0x10) | (per >> 8));
    c[1] = (UBYTE)per;
    c[2] = (UBYTE)((smp & 0x0f) << 4 | cmd);
    c[3] = par;
}

/* the module in ref: name, 31 samples, song, patterns, sample data */
static void buildmod(void)
{
    UBYTE *h, *pat, *d;
    int i, n;
    ULONG len;

    memset(ref, 0, MODMAX);
    strcpy((char *)ref, "ciatrklib testmod");
    for (i = 1; i <= 31; i++) {
        UWORD l = (UWORD)(8 + (i * 5) % 40);
        h = ref + 20 + (i - 1) * 30;
        sprintf((char *)h, "sample %d", i);
        if (i == 30)
            l = 0;
        pw(h + 22, l);
        h[24] = (UBYTE)(i % 15);        /* finetune 0-14 */
        h[25] = (UBYTE)(i == 7 ? 80 : (i * 7) % 65);
        if (i & 1) {
            pw(h + 26, l / 4);
            pw(h + 28, l / 2);
        } else {
            pw(h + 26, 0);
            pw(h + 28, 1);
        }
    }
    ref[950] = SONGLEN;
    ref[951] = 127;
    {
        static const UBYTE order[SONGLEN] = { 0, 1, 2, 3, 4, 5, 1, 3, 0 };
        memcpy(ref + 952, order, SONGLEN);
    }
    memcpy(ref + 1080, "M.K.", 4);
    pat = ref + 1084;
    for (n = 0; n < NPAT * 64 * 4; n++)
        cell(pat + 4 * n);
    /* row 0 of pattern 0: speed 3 and a 1xx before any E1x (mt_LowMask 0) */
    pat[2] = (pat[2] & 0xf0) | 0xf;
    pat[3] = 3;
    pat[4] = 0x01; pat[5] = 0xac; pat[6] = 0x51; pat[7] = 0x05;
    pat[8] = 0x01; pat[9] = 0x40; pat[10] = 0x12; pat[11] = 0x08;
    /* row 1: 9xx with a note (the offset twice) */
    pat[16 + 12] = 0x00; pat[16 + 13] = 0xd6; pat[16 + 14] = 0x39; pat[16 + 15] = 0x01;
    d = pat + NPAT * 1024;
    for (i = 1; i <= 31; i++) {
        ULONG k;
        len = 2UL * ((ref[20 + (i - 1) * 30 + 22] << 8) | ref[20 + (i - 1) * 30 + 23]);
        for (k = 0; k < len; k++)
            *d++ = (UBYTE)(i * 13 + k * 29 + (k >> 3));
    }
    reflen = d - ref;
}

/* the CIA phase's module: one pattern, events on every row (8xx on
   channel 1), tempo 64 on row 0 */
static void buildciamod(UBYTE *m)
{
    int r;
    memset(m, 0, 1084 + 1024);
    strcpy((char *)m, "cia");
    m[950] = 1;
    memcpy(m + 1080, "M.K.", 4);
    for (r = 0; r < 64; r++) {
        UBYTE *c = m + 1084 + r * 16;
        c[2] = 8;
        c[3] = (UBYTE)(r + 1);
    }
    m[1084 + 4 + 2] = 0xf;
    m[1084 + 4 + 3] = 64;
}

/* ------------------------------------------------- finding the blocks */

static UBYTE *find(struct lib *l, const UBYTE *pat, ULONG n, ULONG zeros)
{
    ULONG i, k;
    for (i = 0; i + n + zeros <= l->size; i += 2)
        if (!memcmp(l->code + i, pat, n)) {
            for (k = 0; k < zeros; k++)
                if (l->code[i + n + k])
                    break;
            if (k == zeros)
                return l->code + i;
        }
    return NULL;
}

static void findblocks(void)
{
    static const UBYTE funk[16] = { 0, 5, 6, 7, 8, 10, 11, 13, 16, 19, 22, 26, 32, 43, 64, 128 };
    static const char cia[] = "ciaa.resource\0ciab.resource";
    struct lib *l = getlib(&loaded, LIB);
    UBYTE *x;
    int i;

    blkP = find(l, funk, 16, 0);
    x = find(l, (const UBYTE *)cia, sizeof cia, 0);
    blkC = x ? x - 6 : NULL;
    check("ciatrklib: play routine block found", blkP != NULL);
    check("ciatrklib: CIA block found", blkC != NULL);
    npoffs = 0;
    for (i = 0; i < 4; i++) {
        poffs[npoffs++] = P_CHAN + 44 * i + 4;
        poffs[npoffs++] = P_CHAN + 44 * i + 10;
        poffs[npoffs++] = P_CHAN + 44 * i + 36;
    }
    poffs[npoffs++] = P_FAKE;
    for (i = 0; i < 32; i++)
        poffs[npoffs++] = P_STARTS + 4 * i;
    poffs[npoffs++] = P_SONGDATA;
}

/* the library's variables after init: master_volume $40, volume_flag 0,
   master_mask $0F00, then 80 zero bytes of the module table */
static UBYTE *findG(void)
{
    static const UBYTE g[6] = { 0, 0x40, 0, 0, 0x0f, 0 };
    UBYTE *x = find(getlib(&loaded, LIB), g, 6, 80);
    return x ? x - 4 : NULL;
}

/* ------------------------------------------------- WaitTOF stand-ins */

#define CIAWAITS 25UL                   /* phase A: frames for an event */

static struct Library *fakegfx;
static int waitmode;                    /* 0: Delay(1), 1: one tick */
static ULONG waits;

/* the CIA interrupt by hand: the server's code with a1 = its data */
static void interrupt(void)
{
    struct regs r;
    markregs(&r);
    r.a[1] = GL(blkC + C_INT + 14);
    callregs((APTR)GL(blkC + C_INT + 18), &r, 0, 0);
}

static void fakewaittof(void)
{
    struct regs r;
    waits++;
    if (waitmode == 0) {
        interrupt();
    } else {
        markregs(&r);
        callregs(ENT(LIB, PLAY, 0), &r, 0, 0);
    }
    if (waits >= (waitmode ? 3000UL : CIAWAITS) && blkP)
        blkP[P_LSTEVT] = (UBYTE)~blkP[P_TRKEVT];        /* give up */
}

/* ------------------------------------------------------ phase A */

/* AddICRVector/RemICRVector of both CIA resources replaced: the real
   calls would take the ICR bit from the addresses 1/2 (BUG(orig), #450) - in
   Amiberry $40 on ciaa, accepted - and RemICRVector with bit 0 removes
   the system's timer A vector of ciaa (later tests hang).  The stand-ins
   record the calls; ciaa's AddICRVector fails, so the library takes the
   ciab path (timer B, free) */
#define NICR 4
static struct icr { ULONG res, bit, intr; } icradd[NICR], icrrem[NICR];
static int nadd, nrem;
static struct Library *ciares[2];

static void icrrec(struct icr *t, int *n, struct Library *base, ULONG bit, ULONG intr)
{
    if (*n < NICR) {
        t[*n].res = base == ciares[1];
        t[*n].bit = bit;
        t[*n].intr = blkC ? intr - (ULONG)blkC : intr;
        (*n)++;
    }
}

static ULONG fakeadd(ULONG bit __asm("d0"), APTR intr __asm("a1"), struct Library *base __asm("a6"))
{
    icrrec(icradd, &nadd, base, bit, (ULONG)intr);
    return base == ciares[0] ? 1 : 0;   /* ciaa: "in use" */
}

static void fakerem(ULONG bit __asm("d0"), APTR intr __asm("a1"), struct Library *base __asm("a6"))
{
    icrrec(icrrem, &nrem, base, bit, (ULONG)intr);
}

struct ciarec {
    ULONG base, flag, addr, timerval, tempo, startbit;
    ULONG evt, pos, waited, gaveup, base2, pratoggled, stopped;
};

static void ciaphase(UBYTE *mod)
{
    static struct ciarec rec;
    struct ciarec *c = &rec;
    struct regs r;
    UBYTE before;
    APTR old[2][2];
    int i;

    ciares[0] = OpenResource((STRPTR)"ciaa.resource");
    ciares[1] = OpenResource((STRPTR)"ciab.resource");
    if (!ciares[0] || !ciares[1])
        return;
    Forbid();
    for (i = 0; i < 2; i++) {
        old[i][0] = SetFunction(ciares[i], -6, (APTR)fakeadd);
        old[i][1] = SetFunction(ciares[i], -12, (APTR)fakerem);
    }
    Permit();

    markregs(&r);
    callregs(ENT(LIB, INIT, 0), &r, 0, 0);
    blkG = findG();
    if (blkG && blkC && blkP) {
        c->base = GL(blkC + C_CIABASE) != 0;
        c->flag = GL(blkC + C_TIMERFLAG);
        c->addr = GL(blkC + C_CIAADDR);
        c->timerval = GL(blkC + C_TIMERVAL);
        if (c->base && c->addr)
            c->startbit = ((volatile UBYTE *)c->addr)[c->flag ? 0xf00 : 0xe00] & 1;

        markregs(&r);
        r.d[0] = 0;
        r.d[1] = (ULONG)mod;
        callregs(ENT(LIB, DECODE, 0), &r, 0, 0);
        markregs(&r);
        r.d[0] = 0;
        callregs(ENT(LIB, START, 0), &r, 0, 0);
        /* the interrupt server by hand: row 0 with F40 on tick 6 (the
           timer of ciab), its event on tick 7 */
        for (i = 0; i < 7; i++)
            interrupt();
        c->tempo = GW(blkC + C_REALTEMPO);
        c->evt = blkP[P_TRKEVT];
        c->pos = GW(blkP + P_PATTPOS);

        waitmode = 0;
        waits = 0;
        markregs(&r);
        r.a[6] = (ULONG)fakegfx;
        callregs(ENT(LIB, WAITEVT, 0), &r, 0, 0);
        c->waited = waits;
        c->gaveup = waits >= CIAWAITS;

        markregs(&r);
        callregs(ENT(LIB, STOP, 0), &r, 0, 0);
        before = PRA & 2;
        markregs(&r);
        callregs(ENT(LIB, VBLANK, 0), &r, 0, 0);
        c->base2 = GL(blkC + C_CIABASE);
        c->pratoggled = (PRA & 2) != before;
        if (c->addr)
            c->stopped = !(((volatile UBYTE *)c->addr)[c->flag ? 0xf00 : 0xe00] & 1);
        markregs(&r);
        r.d[0] = 0;
        callregs(ENT(LIB, FREE, 0), &r, 0, 0);
    }
    Forbid();
    for (i = 0; i < 2; i++) {
        SetFunction(ciares[i], -6, old[i][0]);
        SetFunction(ciares[i], -12, old[i][1]);
    }
    Permit();
    ROUT = "init (CIA)";
    strcpy(VEC, "");
    OBS("AddICRVector calls", nadd);
    for (i = 0; i < nadd && i < NICR; i++) {
        OBS("AddICRVector resource", icradd[i].res);
        OBS("AddICRVector bit", icradd[i].bit);
        OBS("AddICRVector interrupt", icradd[i].intr);
    }
    BOTH("AddICRVector ciaa: the long at 1", nadd > 1 && icradd[0].bit == peekl(1));
    BOTH("AddICRVector ciab: the long at 2", nadd > 1 && icradd[1].bit == peekl(2));
    OBS("CIAAbase set", c->base);
    OBS("TimerFlag", c->flag);
    OBS("CIAAaddr", c->addr);
    OBS("TimerValue", c->timerval);
    OBS("timer started", c->startbit);
    ROUT = "the interrupt server";
    OBS("RealTempo", c->tempo);
    BOTH("RealTempo 64", c->tempo == 64);
    OBS("event", c->evt);
    OBS("PatternPos", c->pos);
    ROUT = "WaitTrackerEvent (CIA)";
    OBS("WaitTOF calls", c->waited);
    OBS("no event (gave up)", c->gaveup);
    ROUT = "ChangeTrackerVBlank";
    OBS("CIAAbase", c->base2);
    OBS("filter toggled", c->pratoggled);
    OBS("timer stopped", c->stopped);
    OBS("RemICRVector calls", nrem);
    for (i = 0; i < nrem && i < NICR; i++) {
        OBS("RemICRVector resource", icrrem[i].res);
        OBS("RemICRVector bit", icrrem[i].bit);
        OBS("RemICRVector interrupt", icrrem[i].intr);
    }
}

/* ------------------------------------------------- F00: mt_end's a0 */

#define FAKESZ 0x200                    /* $DFF000 ... $DFF1FF */
#define FNOTE 0x01ac1c20UL              /* the fake registers: period 428, sample 1, C20 */
#define FMODSZ (1084 + 1024 + 32)
#define FMODALLOC (FMODSZ + 16)         /* the empty samples start at the end,
                                           mt_init clears their first long */

/* the $DFF000 that mt_end loads into a0, returns its offset in the
   library, 0 if not found: in the original the operand of LEA $DFF000,A0
   before CLR.W $A8(A0) (exactly one); in the C build the long behind the
   CIA part (ciatrklib.c ciapart.endcustom), independent of the code GCC
   makes */
static ULONG findend(void)
{
    struct lib *l = getlib(&loaded, LIB);
    ULONG i, at = 0;
    int n = 0;
    if (strcmp(loaded.what, "orig")) {      /* the C build */
        if (!blkC)
            return 0;
        at = blkC + C_SIZE - l->code;
        return at + 4 <= l->size && GL(l->code + at) == 0xdff000UL ? at : 0;
    }
    for (i = 2; i + 8 <= l->size; i += 2)
        if (GL(l->code + i) == 0xdff000UL && GW(l->code + i - 2) == 0x41f9
            && GL(l->code + i + 4) == 0x426800a8UL) {
            at = i;
            n++;
        }
    return n == 1 ? at : 0;
}

/* the module: sample 1 (16 words, volume 64); row 0 notes on all
   voices, F01 on voice 4; row 1 F00 on voice k, the voices behind empty
   (the original reads FNOTE for them) */
static void buildfmod(UBYTE *m, int k)
{
    int v;
    memset(m, 0, FMODALLOC);
    strcpy((char *)m, "f00");
    pw(m + 20 + 22, 16);
    m[20 + 25] = 64;
    pw(m + 20 + 28, 1);
    m[950] = 1;
    memcpy(m + 1080, "M.K.", 4);
    for (v = 0; v < 4; v++) {
        UBYTE *c = m + 1084 + 4 * v;
        c[0] = 0x01; c[1] = 0x40; c[2] = 0x10;  /* period 320, sample 1 */
    }
    m[1084 + 12 + 2] = 0x1f;            /* F01 */
    m[1084 + 12 + 3] = 1;
    m[1084 + 16 + 4 * k + 2] = 0xf;     /* F00 */
    memset(m + 1084 + 1024, 0x33, 32);
}

/* the module stays allocated and a region to the end: the play routine
   keeps its pointers into it */
static UBYTE *fm;

static void f00(void)
{
    UBYTE *fake;
    ULONG at;
    int k, i;

    fm = AllocMem(FMODALLOC, MEMF_CHIP | MEMF_CLEAR);
    fake = AllocMem(FAKESZ, MEMF_ANY | MEMF_CLEAR);
    at = findend();
    check("ciatrklib: mt_end's $DFF000 found", at != 0);
    if (!fm || !fake || !at)
        goto out;
    PL(getlib(&loaded, LIB)->code + at, (ULONG)fake);
    fphase = 1;
    CUSTOMR.dmacon = 0x000f;            /* a defined state: the audio DMA off */
    CacheClearU();
    region(fm, FMODALLOC);
    for (k = 0; k < 3; k++) {
        buildfmod(fm, k);
        for (i = 0; i < FAKESZ; i += 4)
            PL(fake + i, FNOTE);
        ROUT = "DecodeModule";
        sprintf(VEC, "slot 6, F00 on voice %d", k + 1);
        DO1(ENT(LIB, DECODE, 0), R.d[0] = 6; R.d[1] = (ULONG)fm);
        sameall();
        func("StartTracker", START, 6);
        for (i = 0; i < 10; i++) {
            sprintf(VEC, "F00 on voice %d, tick %d", k + 1, i);
            stmt("PlayTracker", PLAY, 0);
        }
        BOTH("stopped", blkP[P_ENABLE] == 0);
        for (i = k + 1; i < 4; i++)
            BOTH("voice behind F00: note from $DFF000 + offset",
                 GL(blkP + P_CHAN + 44 * i) == FNOTE);
        OBSBYTES("module bytes", fm, FMODALLOC);
        sprintf(VEC, "F00 on voice %d", k + 1);
        stmt("StopTracker", STOP, 0);
        stmt("FreeTrackerModule", FREE, 6);
    }
    PL(getlib(&loaded, LIB)->code + at, 0xdff000UL);
    fphase = 0;
    CUSTOMR.dmacon = 0x000f;            /* the audio DMA off, as the original's
                                           StopTracker left it (the C build's went
                                           to the RAM copy) */
    CacheClearU();
out:
    if (fake)
        FreeMem(fake, FAKESZ);
}

/* ------------------------------------------------------ phase B */

static UBYTE *curmod;                   /* the module that plays */
static UBYTE pinit[P_SIZE];             /* the play routine's block after loading */

static void ticks(int n)
{
    int i;
    for (i = 0; i < n; i++) {
        sprintf(VEC, "tick %d", i);
        stmt("PlayTracker", PLAY, 0);
        if (i % 16 == 15)
            OBSBYTES("module bytes", curmod, reflen);
        if (failures > 20)
            break;
    }
}

static void memlib(int idx)
{
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "memlib"), idx, 0), &R, NULL, 0, &POP);
}

static char namebuf[32];

static void getname(ULONG slot)
{
    ROUT = "GetModuleName$";
    sprintf(VEC, "slot %lu", slot);
    memset(namebuf, 0x5a, sizeof namebuf);
    DO1(ENT(LIB, NAME, 0), R.d[0] = slot; R.a[3] = (ULONG)namebuf);
    OBS("d0", R.d[0]);
    OBS("a3", R.a[3] - (ULONG)namebuf);
    OBSBYTES("string", namebuf, sizeof namebuf);
}

/* the information functions of a slot */
static void info(ULONG slot)
{
    sprintf(VEC, "slot %lu", slot);
    func("GetTrackerSize", SIZE, 0x5a5a0000UL | slot);
    sprintf(VEC, "slot %lu", slot);
    func("GetTrackerLocation", LOC, slot);
    sprintf(VEC, "slot %lu", slot);
    func("CheckTrackerID", CHKID, slot);
    sprintf(VEC, "slot %lu", slot);
    func("ModulePatterns", PATTERNS, slot);
    getname(slot);
}

static void errchecks(ULONG used, ULONG empty)
{
    static const struct { int idx; const char *name; int empty; } e[] = {
        { LOAD, "_loadcheck", 1 }, { START, "_checkused", 0 }, { DECODE, "_decodecheck", 1 },
        { FREE, "_slotcheck", 0 }, { FREE, "_slotcheck", 1 },
    };
    unsigned i;
    for (i = 0; i < sizeof e / sizeof e[0]; i++) {
        ULONG sl = e[i].empty ? empty : used;
        ROUT = e[i].name;
        sprintf(VEC, "slot %lu", sl);
        DO1(ERR(LIB, e[i].idx, 0), R.d[0] = sl);
    }
}

static const char fname[] = "RAM:ciatrk_mod";

static void writefile(const UBYTE *data, ULONG len)
{
    BPTR f = Open((STRPTR)fname, MODE_NEWFILE);
    if (f) {
        Write(f, (APTR)data, len);
        Close(f);
    }
}

/* LoadTrackerModule slot: result, the slot, the bytes read */
static void load(ULONG slot, ULONG filelen)
{
    int k;
    ROUT = "LoadTrackerModule";
    sprintf(VEC, "slot %lu, %lu bytes", slot, filelen);
    writefile(ref, filelen);
    DO1(ENT(LIB, LOAD, 0), R.d[0] = slot; R.d[1] = (ULONG)fname; R.a[6] = (ULONG)DOSBase);
    OBS("d0", R.d[0]);
    {
        UBYTE *e = blkG + G_MODTABLE + 10 * slot;
        UBYTE *m = (UBYTE *)GL(e);
        ULONG len = GL(e + 4);
        OBS("length", len);
        OBS("flag", GW(e + 8));
        BOTH("in chip memory", m && (TypeOfMem(m) & MEMF_CHIP));
        k = region(m, len);
        sameG();
        if (m) {
            ULONG n = len < filelen ? len : filelen;
            OBSBYTES("the bytes read", m, n);
            BOTH("the file", memcmp(m, ref, n) == 0);
        }
    }
    sprintf(VEC, "slot %lu", slot);
    stmt("FreeTrackerModule", FREE, slot);
    unregion(k);
}

int main(int argc, char **argv)
{
    static const char *names[] = { "memlib", LIB, NULL };
    static APTR vec[46];
    UBYTE *mod, *ciamod, *mod2, *fast;
    UBYTE pra0;
    ULONG avail;
    int k;

    if (argc < 2) {
        printf("usage: ciatrklib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("DecodeModule", d2|d6|d7|a0-a3|a6);
    KEEPS("GetTrackerSize", d1-d7|a0-a3|a6);
    KEEPS("GetTrackerLocation", d1-d7|a0-a3|a6);
    KEEPS("CheckTrackerID", d2-d7|a0-a3|a6);
    KEEPS("ModulePatterns", d1-d7|a2|a3|a6);
    KEEPS("GetModuleName$", d2-d7|a2|a6);
    KEEPS("_loadcheck", d0-d7|a0-a3|a6);
    KEEPS("_checkused", d0-d7|a0-a3|a6);
    KEEPS("_decodecheck", d0-d7|a0-a3|a6);
    KEEPS("_slotcheck", d0-d7|a0-a3|a6);
    KEEPS("StartTracker", d1-d7|a0-a3|a6);
    KEEPS("ModuleToChip", d0-d7|a0-a3|a6);
    KEEPS("StopTracker", d0-d7|a0-a3|a6);
    KEEPS("FreeTrackerModule", d1-d7|a0|a2|a6);
    KEEPS("PlayTracker", d0-d7|a0-a3|a6);
    KEEPS("GetTrackerEvent", d1-d7|a0-a3|a6);
    KEEPS("CheckTrackerEvent", d2-d7|a0-a3|a6);
    KEEPS("TrackerVolume", d0|d3-d7|a0-a3|a6);
    KEEPS("ChangeTrackerMask", d0-d7|a1-a3|a6);
    KEEPS("GetModulePosition", d1-d7|a0-a3|a6);
    KEEPS("ModulePositionJump", d0-d7|a0-a3|a6);
    KEEPS("PauseModule", d0-d2|d4-d7|a1-a3|a6);
    KEEPS("WaitTrackerEvent", d2-d7|a0-a3|a6);
    KEEPS("LoadTrackerModule", d4-d7|a0-a3|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    buildmod();
    mod = AllocMem(MODMAX, MEMF_CHIP | MEMF_CLEAR);
    mod2 = AllocMem(MODMAX, MEMF_CHIP | MEMF_CLEAR);
    ciamod = AllocMem(1084 + 1024, MEMF_CHIP | MEMF_CLEAR);
    fast = AllocMem(MODMAX, MEMF_FAST | MEMF_CLEAR);
    memcpy(mod, ref, reflen);
    memcpy(mod2, ref, reflen);
    buildciamod(ciamod);
    if (fast)
        memcpy(fast, ref, reflen);
    findblocks();
    if (!blkP || !blkC)
        return rwe_done(LIB);
    memcpy(pinit, blkP, P_SIZE);
    vec[44] = (APTR)fakewaittof;        /* -270 WaitTOF */
    fakegfx = fake_library("ciatrk_gfx.library", 0, vec, 45);
    pra0 = PRA;
    ROUT = "memlib init";
    memlib(7);

    /* phase A: the CIA interrupt */
    k = region(ciamod, 1084 + 1024);
    ciaphase(ciamod);
    check("ciatrklib: module table found", blkG != NULL);
    if (!blkG)
        return rwe_done(LIB);
    ROUT = "after the CIA phase";
    strcpy(VEC, "");
    sameG();
    sameC();
    /* the ticks of phase A depend on the timing: the play routine's state
       back to the one after loading */
    memcpy(blkP, pinit, P_SIZE);
    BOTH("mt_LowMask 0", blkP[P_LOWMASK] == 0);

    /* phase B: the play routine called by PlayTracker */
    region(mod, MODMAX);
    curmod = mod;
    ROUT = "DecodeModule";
    strcpy(VEC, "slot 0, chip");
    DO1(ENT(LIB, DECODE, 0), R.d[0] = 0; R.d[1] = (ULONG)mod);
    sameall();
    info(0);
    errchecks(0, 4);
    info(3);                            /* empty */
    func("StartTracker", START, 3);     /* empty: the slot number */
    sprintf(VEC, "$%X", 5);
    func("ModuleToChip", TOCHIP, 5);

    /* a slot length above the sample addresses: mt_init clears the first
       long of every sample (BUG(orig), #438) */
    k = region(mod2, MODMAX);
    ROUT = "DecodeModule";
    strcpy(VEC, "slot 2, chip");
    DO1(ENT(LIB, DECODE, 0), R.d[0] = 2; R.d[1] = (ULONG)mod2);
    PL(blkG + G_MODTABLE + 20 + 4, 0x7fffffffUL);
    strcpy(VEC, "slot 2, length $7FFFFFFF");
    func("StartTracker", START, 2);
    OBSBYTES("module bytes", mod2, reflen);
    BOTH("first longs cleared", memcmp(mod2, ref, reflen) != 0);
    stmt("StopTracker", STOP, 0);
    stmt("FreeTrackerModule", FREE, 2);

    strcpy(VEC, "slot 0");
    func("StartTracker", START, 0);
    OBSBYTES("module bytes", mod, reflen);
    BOTH("module unchanged", memcmp(mod, ref, reflen) == 0);
    ticks(260);
    strcpy(VEC, "");
    func("GetTrackerEvent", GETEVT, 0xa5a5a5a5UL);
    func("CheckTrackerEvent", CHKEVT, 0xa5a5a5a5UL);
    func("CheckTrackerEvent", CHKEVT, 0xa5a5a5a5UL);

    strcpy(VEC, "32");
    stmt("TrackerVolume", VOLUME, 0x5a5a0020UL);
    ticks(40);
    strcpy(VEC, "5");
    stmt("ChangeTrackerMask", MASK, 5);
    ticks(80);
    strcpy(VEC, "0");
    stmt("ChangeTrackerMask", MASK, 0);
    ticks(80);
    strcpy(VEC, "$F");
    stmt("ChangeTrackerMask", MASK, 0xf);
    {
        static const ULONG vols[] = { 0, 64, 65, 1, 63, 0x10040 };
        unsigned i;
        for (i = 0; i < sizeof vols / sizeof vols[0]; i++) {
            sprintf(VEC, "$%lX", vols[i]);
            stmt("TrackerVolume", VOLUME, vols[i]);
            ticks(3);
        }
    }
    strcpy(VEC, "");
    func("GetModulePosition", GETPOS, 0xa5a5a5a5UL);
    strcpy(VEC, "3");
    stmt("ModulePositionJump", POSJUMP, 0xa5a5a503UL);
    ticks(1);
    strcpy(VEC, "");
    func("GetModulePosition", GETPOS, 0);
    BOTH("ModulePositionJump 3: position 4", blkP[P_SONGPOS] == 4);
    stmt("PauseModule", PAUSE, 0);
    ticks(2);
    BOTH("paused", blkP[P_ENABLE] == 0);
    stmt("PauseModule", PAUSE, 0);
    ticks(60);

    /* WaitTrackerEvent: its WaitTOF plays a tick */
    waitmode = 1;
    for (k = 0; k < 4; k++) {
        ROUT = "WaitTrackerEvent";
        sprintf(VEC, "%d", k);
        waits = 0;
        DO1(ENT(LIB, WAITEVT, 0), R.a[6] = (ULONG)fakegfx);
        OBS("WaitTOF calls", waits);
        BOTH("an event", waits < 3000);
        sameall();
    }
    strcpy(VEC, "");
    stmt("StopTracker", STOP, 0);
    BOTH("stopped", blkP[P_ENABLE] == 0);
    stmt("FreeTrackerModule", FREE, 0);
    OBSBYTES("module bytes", mod, reflen);

    /* F00 on voices 1-3 (BUG(orig), #475) */
    f00();

    /* DecodeModule from fast memory: a copy in chip memory */
    if (fast && (LONG)fast >= 0x200000) {
        UBYTE *e = blkG + G_MODTABLE + 10;
        k = region(fast, MODMAX);
        ROUT = "DecodeModule";
        strcpy(VEC, "slot 1, fast memory");
        DO1(ENT(LIB, DECODE, 0), R.d[0] = 1; R.d[1] = (ULONG)fast);
        OBS("length", GL(e + 4));
        OBS("flag", GW(e + 8));
        BOTH("copied to chip memory", (TypeOfMem((APTR)GL(e)) & MEMF_CHIP) != 0);
        {
            UBYTE *m = (UBYTE *)GL(e);
            int k2 = region(m, GL(e + 4));
            BOTH("the copy", memcmp(m, ref, reflen) == 0);
            OBSBYTES("the copy", m, GL(e + 4));
            sameG();
            info(1);
            strcpy(VEC, "slot 1");
            stmt("FreeTrackerModule", FREE, 1);
            unregion(k2);
        }
        unregion(k);
    } else {
        printf("ciatrklib: no fast memory, DecodeModule's copy not tested\n");
    }

    /* LoadTrackerModule: the length is the file size + 3 (ADD.b) */
    load(4, reflen);
    load(5, 0x10fe);                    /* low byte $FE: $1001 */
    DeleteFile((STRPTR)fname);

    ROUT = "finit";
    strcpy(VEC, "");
    stmt("finit", FINIT, 0);
    ROUT = "memlib finit";
    memlib(8);
    FreeMem(mod, MODMAX);
    FreeMem(mod2, MODMAX);
    FreeMem(ciamod, 1084 + 1024);
    if (fast)
        FreeMem(fast, MODMAX);
    if (fm)
        FreeMem(fm, FMODALLOC);
    fake_remove(fakegfx);
    PRA = (PRA & ~2) | (pra0 & 2);
    /* this task's balance: AvailMem() also counts what other tasks
       allocate meanwhile (rwemem.h) */
    rwe_mem_check(LIB, avail, TRUE);
    return rwe_done(LIB);
}
