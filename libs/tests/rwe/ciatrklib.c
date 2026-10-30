/*
 * rwe ciatrklib - the C build of ciatrklib (userlib 56, the CIA
 * Protracker library) against the reference build of its source
 * (ref/ciatrklib.obj, see libs/userlibs/rwe/refbuild.cmake), method of
 * rwecmp.h.
 *
 *   ciatrklib DIR     (DIR with ciatrklib and memlib, .obj/.orig)
 *
 * Paula's registers cannot be read, so the test compares the library's
 * state: its three data blocks (found in both builds: the library's
 * variables with the module table, the CIA part, the play routine's
 * voices, sample starts, song position, speed ... - the C build keeps
 * the original's layout), with the pointers into the modules taken
 * relative to each set's module; the register writes of masked channels
 * (fake_channel); DMACONR's audio bits and the filter bit of CIA-A PRA
 * after every call; the modules' bytes (E8x/EFx change samples).  The
 * modules are built in memory: random notes and effects (fixed seed) in
 * 6 patterns, 31 samples (loops, finetunes 0-14, a volume above 64, a
 * sample of length 0).
 *
 * Two phases:
 *   A (CIA, each set alone): AddICRVector/RemICRVector of the CIA
 *     resources are stand-ins that record their calls (the real ones
 *     with the original's ICR bits would remove the system's ciaa timer
 *     A vector); ciaa's fails, so init takes ciab's timer B.  Compared:
 *     the calls (BUG(orig): the bits are the longs at the addresses 1 and
 *     2; RemICRVector with bit 0), the CIA part, the timer started and
 *     stopped, the interrupt server called by hand (the tempo command,
 *     the event), StartTracker (reinstalls), WaitTrackerEvent with a
 *     WaitTOF that runs the server, ChangeTrackerVBlank (the filter).
 *   B (VBlank, both sets call by call): DecodeModule (chip: linked; fast
 *     memory: copied, size + 3), the information functions also on an
 *     empty slot (BUG(orig): StartTracker returns the slot number,
 *     CheckTrackerID the long at address 0, GetModuleName$ slot*10),
 *     the error checks (no error), StartTracker (mt_init; a slot length
 *     above the sample addresses clears their first longs, BUG(orig)),
 *     then PlayTracker tick by tick with all channels on Paula, with the
 *     masks 5 and 0 (fake_channel), TrackerVolume (0, 1, 32, 63, 64,
 *     65), ModulePositionJump (n + 1, BUG(orig)), GetModulePosition,
 *     PauseModule, Get/CheckTrackerEvent, WaitTrackerEvent (its WaitTOF
 *     plays one tick), StopTracker, LoadTrackerModule (a6 = DOSBase;
 *     a file size with the low byte $FE: the byte add, BUG(orig)),
 *     FreeTrackerModule (loaded, copied, linked), ModuleToChip, finit.
 * Not covered: the runtime errors (missing file, error checks that
 * fail), a Read error, failing allocations, sample numbers above 31 and
 * finetune 15 / above 15 (they read behind the tables into data that
 * holds pointers, or behind the data), E8x with a loop length of 0.
 */
#include "rwecmp.h"
#include <exec/memory.h>
#include <hardware/custom.h>

#define LIB "ciatrklib"

enum { LOAD, START, STOP, DECODE, SIZE, LOC, FREE, GETEVT, CHKEVT, WAITEVT, CHKID, TOCHIP,
       NAME, POSJUMP, GETPOS, PAUSE, VBLANK, PLAY, VOLUME, MASK, PATTERNS, INIT, FINIT };

#define CUSTOMR (*(volatile struct Custom *)0xdff000)
#define PRA (*(volatile UBYTE *)0xbfe001)

static UBYTE *blkP[2], *blkC[2], *blkG[2];

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

/* memory regions of both sets (modules and their copies): a pointer into
   region k compares as k << 24 | offset */
#define NREG 8
static struct { UBYTE *at[2]; ULONG len; } reg[NREG];

static int region(UBYTE *a0, UBYTE *a1, ULONG len)
{
    int k;
    for (k = 0; k < NREG; k++)
        if (!reg[k].len) {
            reg[k].at[0] = a0;
            reg[k].at[1] = a1;
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
static ULONG norm(int s, ULONG v)
{
    ULONG slack;
    int k;
    for (slack = 0; slack <= 0x10000; slack += 0x10000)
        for (k = 0; k < NREG; k++)
            if (reg[k].len && v >= (ULONG)reg[k].at[s]
                && v < (ULONG)reg[k].at[s] + reg[k].len + slack)
                return 0x80000000UL | (ULONG)k << 24 | (v - (ULONG)reg[k].at[s]);
    return v;
}

/* ------------------------------------------------- block compares */

static UBYTE tmp[2][P_SIZE];
static UWORD poffs[60];
static int npoffs;

static void sameblock(const char *what, UBYTE *const blk[2], ULONG size, const UWORD *ptr,
                      int nptr, const UWORD *zero, int nzero)
{
    int s, i;
    ULONG o;
    char w[80];
    for (s = 0; s < 2; s++) {
        memcpy(tmp[s], blk[s], size);
        for (i = 0; i < nptr; i++)
            PL(tmp[s] + ptr[i], norm(s, GL(tmp[s] + ptr[i])));
        for (i = 0; i < nzero; i++)
            PL(tmp[s] + zero[i], 0);
    }
    for (o = 0; o < size; o++)
        if (tmp[0][o] != tmp[1][o])
            break;
    if (o == size) {
        checks++;
        return;
    }
    o &= ~1UL;
    sprintf(w, "%s +%lu", what, o);
    fail(w, (ULONG)tmp[0][o] << 24 | (ULONG)tmp[0][o + 1] << 16 | GW(tmp[0] + o + 2),
         (ULONG)tmp[1][o] << 24 | (ULONG)tmp[1][o + 1] << 16 | GW(tmp[1] + o + 2));
}

static void sameP(void)
{
    if (blkP[0] && blkP[1])
        sameblock("play routine", blkP, P_SIZE, poffs, npoffs, NULL, 0);
}

static void sameG(void)
{
    static const UWORD ptr[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
    if (blkG[0] && blkG[1])
        sameblock("modtable", blkG, G_SIZE, ptr, 10, NULL, 0);
}

static void sameC(void)
{
    /* the interrupt's node links and its name/code (in the library) */
    static const UWORD ptr[] = { C_MTDATA };
    static const UWORD zero[] = { C_INT, C_INT + 4, C_INT + 10, C_INT + 18 };
    if (blkC[0] && blkC[1])
        sameblock("CIA part", blkC, C_SIZE, ptr, 1, zero, 4);
}

/* after every call: the blocks, Paula's DMA bits, the filter */
static void sameall(void)
{
    sameP();
    sameG();
    sameC();
}

static UWORD dmaconr[2];
static UBYTE pra[2];

/* ------------------------------------------------------ calls */

/* the long at a fixed low address (through a register: a constant
   address would be a null pointer offset for GCC) */
static ULONG peekl(ULONG addr)
{
    const ULONG *q;
    __asm__("" : "=a"(q) : "0"(addr));
    return *(volatile const ULONG *)q;
}

/* a statement / function with the state after it compared */
static void stmt(const char *rout, int idx, ULONG d0)
{
    ROUT = rout;
    for (cur = 0; cur < 2; cur++) {
        APTR fn = ENT(LIB, idx, 0);
        markregs(&R[cur]);
        R[cur].d[0] = d0;
        IN[cur] = R[cur];
        CCR[cur] = callstk(fn, &R[cur], NULL, 0, &POP[cur]);
        dmaconr[cur] = CUSTOMR.dmaconr & 0xf;
        pra[cur] = PRA & 2;
    }
    preserve();
    SAME("DMACONR", dmaconr[cur]);
    SAME("filter", pra[cur]);
    sameall();
}

static void func(const char *rout, int idx, ULONG d0)
{
    stmt(rout, idx, d0);
    SAME("d0", norm(cur, R[cur].d[0]));
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
    int s, i;
    for (s = 0; s < 2; s++) {
        struct lib *l = getlib(&sets[s], LIB);
        UBYTE *x;
        blkP[s] = find(l, funk, 16, 0);
        x = find(l, (const UBYTE *)cia, sizeof cia, 0);
        blkC[s] = x ? x - 6 : NULL;
    }
    check("ciatrklib: play routine block found (orig)", blkP[0] != NULL);
    check("ciatrklib: play routine block found (C)", blkP[1] != NULL);
    check("ciatrklib: CIA block found (orig)", blkC[0] != NULL);
    check("ciatrklib: CIA block found (C)", blkC[1] != NULL);
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
static UBYTE *findG(int s)
{
    static const UBYTE g[6] = { 0, 0x40, 0, 0, 0x0f, 0 };
    UBYTE *x = find(getlib(&sets[s], LIB), g, 6, 80);
    return x ? x - 4 : NULL;
}

/* ------------------------------------------------- WaitTOF stand-ins */

#define CIAWAITS 25UL                   /* phase A: frames for an event */

static struct Library *fakegfx;
static int waitmode;                    /* 0: Delay(1), 1: one tick of the set */
static ULONG waits[2];

/* the CIA interrupt by hand: the server's code with a1 = its data */
static void interrupt(int s)
{
    struct regs r;
    markregs(&r);
    r.a[1] = GL(blkC[s] + C_INT + 14);
    callregs((APTR)GL(blkC[s] + C_INT + 18), &r, 0, 0);
}

static void fakewaittof(void)
{
    struct regs r;
    waits[cur]++;
    if (waitmode == 0) {
        interrupt(cur);
    } else {
        markregs(&r);
        callregs(ENT(LIB, PLAY, 0), &r, 0, 0);
    }
    if (waits[cur] >= (waitmode ? 3000UL : CIAWAITS) && blkP[cur])
        blkP[cur][P_LSTEVT] = (UBYTE)~blkP[cur][P_TRKEVT];      /* give up */
}

/* ------------------------------------------------------ phase A */

/* AddICRVector/RemICRVector of both CIA resources replaced: the real
   calls would take the ICR bit from the addresses 1/2 (BUG(orig)) - in
   Amiberry $40 on ciaa, accepted - and RemICRVector with bit 0 removes
   the system's timer A vector of ciaa (later tests hang).  The stand-ins
   record the calls; ciaa's AddICRVector fails, so the library takes the
   ciab path (timer B, free) */
#define NICR 4
static struct icr { ULONG res, bit, intr; } icradd[2][NICR], icrrem[2][NICR];
static int nadd[2], nrem[2];
static struct Library *ciares[2];

static void icrrec(struct icr *t, int *n, struct Library *base, ULONG bit, ULONG intr)
{
    if (*n < NICR) {
        t[*n].res = base == ciares[1];
        t[*n].bit = bit;
        t[*n].intr = blkC[cur] ? intr - (ULONG)blkC[cur] : intr;
        (*n)++;
    }
}

static ULONG fakeadd(ULONG bit __asm("d0"), APTR intr __asm("a1"), struct Library *base __asm("a6"))
{
    icrrec(icradd[cur], &nadd[cur], base, bit, (ULONG)intr);
    return base == ciares[0] ? 1 : 0;   /* ciaa: "in use" */
}

static void fakerem(ULONG bit __asm("d0"), APTR intr __asm("a1"), struct Library *base __asm("a6"))
{
    icrrec(icrrem[cur], &nrem[cur], base, bit, (ULONG)intr);
}

struct ciarec {
    ULONG base, flag, addr, timerval, tempo, startbit;
    ULONG evt, pos, waited, gaveup, base2, pratoggled, stopped;
};

static void ciaphase(UBYTE *const mod[2])
{
    static struct ciarec rec[2];
    APTR old[2][2];
    int s, i;

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
    for (s = 0; s < 2; s++) {
        struct regs r;
        UBYTE before;
        struct ciarec *c = &rec[s];
        cur = s;

        markregs(&r);
        callregs(ENT(LIB, INIT, 0), &r, 0, 0);
        blkG[s] = findG(s);
        if (!blkG[s] || !blkC[s] || !blkP[s])
            continue;
        c->base = GL(blkC[s] + C_CIABASE) != 0;
        c->flag = GL(blkC[s] + C_TIMERFLAG);
        c->addr = GL(blkC[s] + C_CIAADDR);
        c->timerval = GL(blkC[s] + C_TIMERVAL);
        if (c->base && c->addr)
            c->startbit = ((volatile UBYTE *)c->addr)[c->flag ? 0xf00 : 0xe00] & 1;

        markregs(&r);
        r.d[0] = 0;
        r.d[1] = (ULONG)mod[s];
        callregs(ENT(LIB, DECODE, 0), &r, 0, 0);
        markregs(&r);
        r.d[0] = 0;
        callregs(ENT(LIB, START, 0), &r, 0, 0);
        /* the interrupt server by hand: row 0 with F40 on tick 6 (the
           timer of ciab), its event on tick 7 */
        for (i = 0; i < 7; i++)
            interrupt(s);
        c->tempo = GW(blkC[s] + C_REALTEMPO);
        c->evt = blkP[s][P_TRKEVT];
        c->pos = GW(blkP[s] + P_PATTPOS);

        waitmode = 0;
        waits[s] = 0;
        markregs(&r);
        r.a[6] = (ULONG)fakegfx;
        callregs(ENT(LIB, WAITEVT, 0), &r, 0, 0);
        c->waited = waits[s];
        c->gaveup = waits[s] >= CIAWAITS;

        markregs(&r);
        callregs(ENT(LIB, STOP, 0), &r, 0, 0);
        before = PRA & 2;
        markregs(&r);
        callregs(ENT(LIB, VBLANK, 0), &r, 0, 0);
        c->base2 = GL(blkC[s] + C_CIABASE);
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
    SAME("AddICRVector calls", nadd[cur]);
    for (i = 0; i < nadd[0] && i < NICR; i++) {
        SAME("AddICRVector resource", icradd[cur][i].res);
        SAME("AddICRVector bit", icradd[cur][i].bit);
        SAME("AddICRVector interrupt", icradd[cur][i].intr);
    }
    BOTH("AddICRVector ciaa: the long at 1", nadd[cur] > 1 && icradd[cur][0].bit == peekl(1));
    BOTH("AddICRVector ciab: the long at 2", nadd[cur] > 1 && icradd[cur][1].bit == peekl(2));
    SAME("CIAAbase set", rec[cur].base);
    SAME("TimerFlag", rec[cur].flag);
    SAME("CIAAaddr", rec[cur].addr);
    SAME("TimerValue", rec[cur].timerval);
    SAME("timer started", rec[cur].startbit);
    ROUT = "the interrupt server";
    SAME("RealTempo", rec[cur].tempo);
    BOTH("RealTempo 64", rec[cur].tempo == 64);
    SAME("event", rec[cur].evt);
    SAME("PatternPos", rec[cur].pos);
    ROUT = "WaitTrackerEvent (CIA)";
    SAME("WaitTOF calls", rec[cur].waited);
    SAME("no event (gave up)", rec[cur].gaveup);
    ROUT = "ChangeTrackerVBlank";
    SAME("CIAAbase", rec[cur].base2);
    SAME("filter toggled", rec[cur].pratoggled);
    SAME("timer stopped", rec[cur].stopped);
    SAME("RemICRVector calls", nrem[cur]);
    for (i = 0; i < nrem[0] && i < NICR; i++) {
        SAME("RemICRVector resource", icrrem[cur][i].res);
        SAME("RemICRVector bit", icrrem[cur][i].bit);
        SAME("RemICRVector interrupt", icrrem[cur][i].intr);
    }
}

/* ------------------------------------------------------ phase B */

static UBYTE *curmod[2];                /* the module that plays */
static UBYTE pinit[2][P_SIZE];          /* the play routine's block after loading */

static void ticks(int n)
{
    int i;
    for (i = 0; i < n; i++) {
        sprintf(VEC, "tick %d", i);
        stmt("PlayTracker", PLAY, 0);
        if (i % 16 == 15) {
            const UBYTE *m[2] = { curmod[0], curmod[1] };
            SAMEBYTES("module bytes", m, reflen);
        }
        if (failures > 20)
            break;
    }
}

static void memlib(int idx)
{
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), idx, 0), &R[cur], NULL, 0, &POP[cur]);
    }
}

static char namebuf[2][32];

static void getname(ULONG slot)
{
    ROUT = "GetModuleName$";
    sprintf(VEC, "slot %lu", slot);
    for (cur = 0; cur < 2; cur++)
        memset(namebuf[cur], 0x5a, sizeof namebuf[cur]);
    DO2(ENT(LIB, NAME, 0), R[cur].d[0] = slot; R[cur].a[3] = (ULONG)namebuf[cur]);
    SAME("d0", R[cur].d[0]);
    SAME("a3", R[cur].a[3] - (ULONG)namebuf[cur]);
    {
        const UBYTE *b[2] = { (UBYTE *)namebuf[0], (UBYTE *)namebuf[1] };
        SAMEBYTES("string", b, sizeof namebuf[0]);
    }
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
        DO2(ERR(LIB, e[i].idx, 0), R[cur].d[0] = sl);
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
    DO2(ENT(LIB, LOAD, 0), R[cur].d[0] = slot; R[cur].d[1] = (ULONG)fname;
        R[cur].a[6] = (ULONG)DOSBase);
    SAME("d0", R[cur].d[0]);
    {
        UBYTE *e[2] = { blkG[0] + G_MODTABLE + 10 * slot, blkG[1] + G_MODTABLE + 10 * slot };
        UBYTE *m[2] = { (UBYTE *)GL(e[0]), (UBYTE *)GL(e[1]) };
        ULONG len = GL(e[0] + 4);
        SAME("length", GL(e[cur] + 4));
        SAME("flag", GW(e[cur] + 8));
        BOTH("in chip memory", m[cur] && (TypeOfMem(m[cur]) & MEMF_CHIP));
        k = region(m[0], m[1], len);
        sameG();
        if (m[0] && m[1] && len == GL(e[1] + 4)) {
            const UBYTE *mm[2] = { m[0], m[1] };
            ULONG n = len < filelen ? len : filelen;
            SAMEBYTES("the bytes read", mm, n);
            BOTH("the file", memcmp(m[cur], ref, n) == 0);
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
    UBYTE *mod[2], *ciamod[2], *mod2[2], *fast[2];
    UBYTE pra0;
    ULONG avail;
    int s, k;

    if (argc < 2) {
        printf("usage: ciatrklib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    avail = AvailMem(MEMF_ANY);
    buildmod();
    for (s = 0; s < 2; s++) {
        mod[s] = AllocMem(MODMAX, MEMF_CHIP | MEMF_CLEAR);
        mod2[s] = AllocMem(MODMAX, MEMF_CHIP | MEMF_CLEAR);
        ciamod[s] = AllocMem(1084 + 1024, MEMF_CHIP | MEMF_CLEAR);
        fast[s] = AllocMem(MODMAX, MEMF_FAST | MEMF_CLEAR);
        memcpy(mod[s], ref, reflen);
        memcpy(mod2[s], ref, reflen);
        buildciamod(ciamod[s]);
        if (fast[s])
            memcpy(fast[s], ref, reflen);
    }
    findblocks();
    if (!blkP[0] || !blkP[1] || !blkC[0] || !blkC[1])
        return rwe_done(LIB);
    for (s = 0; s < 2; s++)
        memcpy(pinit[s], blkP[s], P_SIZE);
    vec[44] = (APTR)fakewaittof;        /* -270 WaitTOF */
    fakegfx = fake_library("ciatrk_gfx.library", 0, vec, 45);
    pra0 = PRA;
    ROUT = "memlib init";
    memlib(7);

    /* phase A: the CIA interrupt, each set alone */
    k = region(ciamod[0], ciamod[1], 1084 + 1024);
    ciaphase(ciamod);
    check("ciatrklib: module table found (orig)", blkG[0] != NULL);
    check("ciatrklib: module table found (C)", blkG[1] != NULL);
    if (!blkG[0] || !blkG[1])
        return rwe_done(LIB);
    ROUT = "after the CIA phase";
    strcpy(VEC, "");
    sameG();
    sameC();
    /* the ticks of phase A depend on the timing: the play routine's state
       back to the one after loading */
    for (s = 0; s < 2; s++)
        memcpy(blkP[s], pinit[s], P_SIZE);
    BOTH("mt_LowMask 0", blkP[cur][P_LOWMASK] == 0);

    /* phase B: the play routine called by PlayTracker */
    region(mod[0], mod[1], MODMAX);
    curmod[0] = mod[0];
    curmod[1] = mod[1];
    ROUT = "DecodeModule";
    strcpy(VEC, "slot 0, chip");
    DO2(ENT(LIB, DECODE, 0), R[cur].d[0] = 0; R[cur].d[1] = (ULONG)mod[cur]);
    sameall();
    info(0);
    errchecks(0, 4);
    info(3);                            /* empty */
    func("StartTracker", START, 3);     /* empty: the slot number */
    sprintf(VEC, "$%X", 5);
    func("ModuleToChip", TOCHIP, 5);

    /* a slot length above the sample addresses: mt_init clears the first
       long of every sample (BUG(orig)) */
    k = region(mod2[0], mod2[1], MODMAX);
    ROUT = "DecodeModule";
    strcpy(VEC, "slot 2, chip");
    DO2(ENT(LIB, DECODE, 0), R[cur].d[0] = 2; R[cur].d[1] = (ULONG)mod2[cur]);
    for (s = 0; s < 2; s++)
        PL(blkG[s] + G_MODTABLE + 20 + 4, 0x7fffffffUL);
    strcpy(VEC, "slot 2, length $7FFFFFFF");
    func("StartTracker", START, 2);
    {
        const UBYTE *m[2] = { mod2[0], mod2[1] };
        SAMEBYTES("module bytes", m, reflen);
        BOTH("first longs cleared", memcmp(mod2[cur], ref, reflen) != 0);
    }
    stmt("StopTracker", STOP, 0);
    stmt("FreeTrackerModule", FREE, 2);

    strcpy(VEC, "slot 0");
    func("StartTracker", START, 0);
    {
        const UBYTE *m[2] = { mod[0], mod[1] };
        SAMEBYTES("module bytes", m, reflen);
        BOTH("module unchanged", memcmp(mod[cur], ref, reflen) == 0);
    }
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
    BOTH("ModulePositionJump 3: position 4", blkP[cur][P_SONGPOS] == 4);
    stmt("PauseModule", PAUSE, 0);
    ticks(2);
    BOTH("paused", blkP[cur][P_ENABLE] == 0);
    stmt("PauseModule", PAUSE, 0);
    ticks(60);

    /* WaitTrackerEvent: its WaitTOF plays a tick of the set */
    waitmode = 1;
    for (k = 0; k < 4; k++) {
        ROUT = "WaitTrackerEvent";
        sprintf(VEC, "%d", k);
        waits[0] = waits[1] = 0;
        DO2(ENT(LIB, WAITEVT, 0), R[cur].a[6] = (ULONG)fakegfx);
        SAME("WaitTOF calls", waits[cur]);
        BOTH("an event", waits[cur] < 3000);
        sameall();
    }
    strcpy(VEC, "");
    stmt("StopTracker", STOP, 0);
    BOTH("stopped", blkP[cur][P_ENABLE] == 0);
    stmt("FreeTrackerModule", FREE, 0);
    {
        const UBYTE *m[2] = { mod[0], mod[1] };
        SAMEBYTES("module bytes", m, reflen);
    }

    /* DecodeModule from fast memory: a copy in chip memory */
    if (fast[0] && fast[1] && (LONG)fast[0] >= 0x200000 && (LONG)fast[1] >= 0x200000) {
        UBYTE *e[2] = { blkG[0] + G_MODTABLE + 10, blkG[1] + G_MODTABLE + 10 };
        k = region(fast[0], fast[1], MODMAX);
        ROUT = "DecodeModule";
        strcpy(VEC, "slot 1, fast memory");
        DO2(ENT(LIB, DECODE, 0), R[cur].d[0] = 1; R[cur].d[1] = (ULONG)fast[cur]);
        SAME("length", GL(e[cur] + 4));
        SAME("flag", GW(e[cur] + 8));
        BOTH("copied to chip memory", (TypeOfMem((APTR)GL(e[cur])) & MEMF_CHIP) != 0);
        {
            UBYTE *m[2] = { (UBYTE *)GL(e[0]), (UBYTE *)GL(e[1]) };
            int k2 = region(m[0], m[1], GL(e[0] + 4));
            const UBYTE *mm[2] = { m[0], m[1] };
            BOTH("the copy", memcmp(m[cur], ref, reflen) == 0);
            SAMEBYTES("the copy", mm, GL(e[0] + 4));
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
    for (s = 0; s < 2; s++) {
        FreeMem(mod[s], MODMAX);
        FreeMem(mod2[s], MODMAX);
        FreeMem(ciamod[s], 1084 + 1024);
        if (fast[s])
            FreeMem(fast[s], MODMAX);
    }
    fake_remove(fakegfx);
    PRA = (PRA & ~2) | (pra0 & 2);
    check("ciatrklib: all memory back", AvailMem(MEMF_ANY) == avail);
    return rwe_done(LIB);
}
