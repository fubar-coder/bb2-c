/*
 * blitzcoplib - command library 144: the Blitz mode display ("slices",
 * one copper list for all of them) - Slice, CopLoc, CustomCop, Show,
 * ShowF, ShowB, CopLen, ColSplit, ShowBlitz, FreeSlices, DisplayOff,
 * DisplayOn, SetBPLCON0, SetBLWait.
 *
 * C translation of bblibsrc/blitzcoplib (20/8/93).  The source has a
 * token Help more (CNIF #ddftest=1, not assembled: #ddftest=0) - the CD
 * binary is the reference and has 19 entries; otherwise it is the source
 * (checked instruction by instruction).  Behind the code the binary has
 * MOVEQ #0,d0:RTS more, referenced by nothing.
 *
 * Object library "Slice" (64 bytes, maximum 10; Free = freeslice = RTS):
 *   +0  y.w (0 = not in use; the line the slice was inserted at)
 *   +2  flags.w  +4 bitplanes.w (bits 14/15: dual playfield half)
 *   +6  sprites.w  +8 colours.w
 *   +10 pointer to the bitplane pointer moves   +14 sprite pointer moves
 *   +18 colour moves  +22 BPLCON1 value  +26 BPLCON2 value (all in the
 *   copper list)   +30 BPLCON1 copy.w   +32 height.w
 * The copper list (mycop, chip memory, length mycoplen) starts as the
 * dummy list {DMACON $0020, end}; every Slice/ColSplit/CustomCop inserts
 * its instructions at the right line: inscop2 allocates a new list,
 * copies the old one up to the first wait below the line, adds a wait for
 * the line (and the line 256 wait `blwait` if needed), the new
 * instructions, the rest, frees the old list and moves the pointers of
 * every slice (+10..+26; slices at or below the line also by the
 * inserted length).
 *   Slice: the instructions are built in `maxcop` (404 bytes) from the
 *         flags (bit 0 hires, 1 dual playfield, 2 HAM, 3 smooth
 *         scrolling, 4 DMA off/on + end wait, 7 BPL2MOD, 8 BPL1MOD,
 *         10 DDFSTRT/STOP, 12 DIWSTRT/STOP, 13 BPLCON2, 14 BPLCON1,
 *         15 BPLCON0); the quick form uses flags $FFF0 + bits 0-2, 8
 *         sprites, 32 colours, width 320/640, height 256 (200 on NTSC:
 *         GfxBase DisplayFlags bit 0), its arguments in `aqslice`.  A
 *         $01FE1234 marker after the DMA on move is replaced by a NOP and
 *         turns the following word into a wait for line (y+height) & 255.
 *   Show/ShowF/ShowB write the bitplane pointers (+ offset of x,y) and
 *         BPLCON1 of the used slice.
 * Slice, ColSplit and CustomCop are Blitz/Amiga routines ($A001): in
 * Blitz mode they also put the list on the hardware (COP1LC) and tell
 * switchlib ($C205).  ShowBlitz/DisplayOff/DisplayOn are Blitz mode
 * only; FreeSlices puts GfxBase->copinit back on COP1LC and strobes
 * COPJMP1 (also in Amiga mode).  Custom registers are written with MOVE
 * (helpers below), never CLR.
 *
 * Kept from the original (BUG(orig)):
 *   - Slice: unused pointers (+10..+26 = 0) are relocated like the
 *     others (0 - maxcop + position, later + list movements); Show into a
 *     slice without BPLCON1 writes to that address.
 *   - inscop2: an empty CustomCop string copies 2^32 longs (SUBQ.l/BNE);
 *     maxslice 0 runs the pointer loop 65536 times; the marker also
 *     changes the source buffer.
 *   - hwait is a word, written and read as its high byte (MOVE.b).
 *   - the slice width checks (Lores/Hires multiple) are in the check
 *     routine only.
 * DIFF: a2 stays unchanged (CustomCop leaves it decremented by 4; the
 * port's convention, notes/cmdlibs-port.md "Allgemein").
 */
#include "blitzlib.h"
#include <graphics/gfxbase.h>

/* entry points (thunks / C routines, defined below) */
void doqslice(void), doslice(void), docoploc(void), docustomcop(void);
void doshow2(void), doshow(void), doshowf2(void), doshowf3(void), doshowf(void);
void doshowb2(void), doshowb3(void), doshowb(void), docoplen(void);
void docolsplit(void), doshowblitz(void), dokillslices(void);
void dodisplayoff(void), dodisplayon(void), dosetbplcon0(void), dosetblwait(void);
void doinit(void), freeslice(void);
void slicechk(void), slicechk2(void), nshowchk(void), showfchk(void);
void showbchk(void), cspltst(void), sbchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#blitzcoplib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_blitzcoplib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

/* err1 = 1 / check+1: Blitz mode only */
#define BLITZONLY ((void *)1)

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_blitzcoplib, 0x1380, LIB_blitzcoplib, 0x0604, LIB_graphicslib, 0x1000);
BLITZ_SUBS(doqslice, slicechk, 0);
BLITZ_ARGS(BL_WORD, 0x42, 0x42, 0x42, BL_LONG, BL_WORD, BL_WORD, BL_WORD, 0x42, 0x42);
BLITZ_LIBS(LIB_blitzcoplib, 0x1380, LIB_blitzcoplib, 0x0604);
BLITZ_SUBS(doslice, slicechk2, 0);
BLITZ_NAME_AT(_toke, "Slice",
              "[Slice#,Y,Flags]|[Slice#,Y,Width,Hite,Flgs,Bitplns,Sprts,Colrs,Fscrnw,Bscrnw]");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(docoploc, 0, 0);
BLITZ_NAME("CopLoc", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(docustomcop, 0, 0);
BLITZ_NAME("CustomCop", "Copins$,Y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_blitzcoplib, 0x1102);
BLITZ_SUBS(doshow2, nshowchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_blitzcoplib, 0x1102);
BLITZ_SUBS(doshow, nshowchk, 0);
BLITZ_NAME("Show", "Bitmap#[,X,Y]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_blitzcoplib, 0x1102);
BLITZ_SUBS(doshowf2, showfchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_blitzcoplib, 0x1102);
BLITZ_SUBS(doshowf3, showfchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_blitzcoplib, 0x1102);
BLITZ_SUBS(doshowf, showfchk, 0);
BLITZ_NAME("ShowF", "Bitmap#[,X,Y[,ShowB X]]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_blitzcoplib, 0x1102);
BLITZ_SUBS(doshowb2, showbchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_blitzcoplib, 0x1102);
BLITZ_SUBS(doshowb3, showbchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_blitzcoplib, 0x1102);
BLITZ_SUBS(doshowb, showbchk, 0);
BLITZ_NAME("ShowB", "Bitmap#[,X,Y[,ShowF X]]");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(docoplen, 0, 0);
BLITZ_NAME("CopLen", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_blitzcoplib, 0x1102);
BLITZ_SUBS(docolsplit, cspltst, 0);
BLITZ_NAME("ColSplit", "Colour Register,Red,Green,Blue,Y");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doshowblitz, (char *)sbchk + 1, 0);
BLITZ_NAME("ShowBlitz", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_blitzcoplib, 0x1001, LIB_blitzcoplib, 0x0003, LIB_graphicslib, 0x1100);
BLITZ_SUBS(dokillslices, 0, 0);
BLITZ_NAME("FreeSlices", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dodisplayoff, BLITZONLY, 0);
BLITZ_NAME("DisplayOff", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dodisplayon, BLITZONLY, 0);
BLITZ_NAME("DisplayOn", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(dosetbplcon0, 0, 0);
BLITZ_NAME("SetBPLCON0", "BPLCON0 Default");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(dosetblwait, 0, 0);
BLITZ_NAME("SetBLWait", "Wait move for line 256 (eg:$ffe1fffe");

BLITZ_NULLSUB_AT(init, doinit, 0, 0, LIB_blitzcoplib, 0x1001, LIB_blitzcoplib, 0x0003);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freeslice, 0, 0);

BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 10, 6);

/* ******************* CODE BELOW **************** */

#define W(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define L(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define B(p, o) (*(UBYTE *)((UBYTE *)(p) + (o)))

#define TOK_setcoplist 0xc205   /* switchlib: a0 = copper list */
#define TOK_c104       0xc104   /* intlib (Blitz mode), d0 = 0 */

ULONG mycop;                    /* the copper list (chip memory) */
static ULONG mycoplen;
static UBYTE *slicebase;
static UWORD maxslice;
static UBYTE hwait[2];          /* a word, used as its high byte */
static UWORD hstart = 0x81;
static UWORD bplcon0 = 0x300;
static ULONG blwait = 0xffe1fffe;
static UWORD maxcop[202];       /* 404 bytes: one slice's instructions */
static const ULONG dumcop[2] = { 0x00960020, 0xfffffffe };
static ULONG colstuff;
static UWORD aqslice[5];        /* the quick Slice's stacked arguments */

/* custom chip registers: MOVE.w / MOVE.l, never CLR */
#define CUSTOM 0xdff000
static inline void custom_w(ULONG reg, UWORD v)
{
    __asm volatile("move.w %1,%0" : "=m"(*(volatile UWORD *)(CUSTOM + reg)) : "dn"(v));
}
static inline void custom_l(ULONG reg, ULONG v)
{
    __asm volatile("move.l %1,%0" : "=m"(*(volatile ULONG *)(CUSTOM + reg)) : "dn"(v));
}

/* MULS.w as instruction (GCC would call __mulsi3) */
static inline LONG muls16(WORD a, WORD b)
{
    LONG r = a;

    __asm__("muls.w %1,%0" : "+d"(r) : "dmi"(b) : "cc");
    return r;
}

static ULONG alloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void dealloc(ULONG mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* switchlib $C205 with a0 = list; returns a0 (kept by switchlib) */
static ULONG setcoplist(ULONG list)
{
    struct bl_regs r = { .a0 = list };

    BLITZ_ALIBJSR(TOK_setcoplist, r);
    return r.a0;
}

static void BLITZ_ROUTINE dosetbplcon0_c(ULONG v BL_REG(d0))
{
    bplcon0 = (UWORD)v;
}

static void BLITZ_ROUTINE dosetblwait_c(ULONG v BL_REG(d0))
{
    blwait = v;
}
BLITZ_THUNK(dosetbplcon0, dosetbplcon0_c, "d0-d1/a0-a1");
BLITZ_THUNK(dosetblwait, dosetblwait_c, "d0-d1/a0-a1");

/* DisplayOff / DisplayOn (Blitz mode only) */
static void display(UWORD dmacon)
{
    struct bl_regs r = { .d0 = 0 };

    BLITZ_BLIBJSR(TOK_c104, r);
    custom_w(0x096, dmacon);
}

void BLITZ_ROUTINE dodisplayoff_c(void) { display(0x0180); }
void BLITZ_ROUTINE dodisplayon_c(void)  { display(0x8180); }
BLITZ_THUNK(dodisplayoff, dodisplayoff_c, "a0-a1");
BLITZ_THUNK(dodisplayon, dodisplayon_c, "a0-a1");

/* makedumcop - a new dummy list */
static void makedumcop(void)
{
    ULONG *a0;
    const ULONG *a1 = dumcop;

    mycoplen = sizeof(dumcop);
    mycop = alloc(mycoplen, MEMF_CHIP);
    a0 = (ULONG *)mycop;
    do
        *a0++ = *a1++;
    while (a1 < dumcop + 2);
}

/* FreeSlices - a0 = slice 0, d0 = maximum, a1 = GfxBase */
static void BLITZ_ROUTINE dokillslices_c(UBYTE *a0 BL_REG(a0), UWORD n BL_REG(d0),
                                         struct GfxBase *gfx BL_REG(a1))
{
    ULONG list;

    n--;
    do {
        W(a0, 0) = 0;
        a0 += 64;
    } while ((WORD)--n != -1);
    list = setcoplist((ULONG)gfx->copinit);
    custom_l(0x080, list);
    custom_w(0x088, 0);
    dealloc(mycop, mycoplen);
    mycop = 0;
    makedumcop();
}
BLITZ_THUNK(dokillslices, dokillslices_c, "a2");

/* ShowBlitz (Blitz mode only) */
static void BLITZ_ROUTINE doshowblitz_c(void)
{
    ULONG list = setcoplist(mycop);

    custom_l(0x080, list);
    custom_w(0x088, 0);
    custom_w(0x1fc, 0);
    custom_w(0x106, 0xc00);             /* "hires sprite kludge" */
}
BLITZ_THUNK(doshowblitz, doshowblitz_c, "a2");

/* init - a0 = slice 0, d0 = maximum */
static void BLITZ_ROUTINE doinit_c(UBYTE *base BL_REG(a0), UWORD max BL_REG(d0))
{
    slicebase = base;
    maxslice = max;
    blwait = 0xffe1fffe;
    makedumcop();
}
BLITZ_THUNK(doinit, doinit_c, "a2");

/* CopLoc / CopLen */
static ULONG BLITZ_ROUTINE docoploc_c(void) { return mycop; }
static ULONG BLITZ_ROUTINE docoplen_c(void) { return mycoplen; }
BLITZ_THUNK(docoploc, docoploc_c, "d1/a0-a1");
BLITZ_THUNK(docoplen, docoplen_c, "d1/a0-a1");

/* Free: RTS */
void BLITZ_ROUTINE freeslice(void)
{
}

/* ------------------------------------------------- copper list insertion */

/* inscop2 - insert the list at a0 (d0 bytes) at line y into the main list;
   returns where the new instructions start */
static ULONG inscop2(ULONG *a0, ULONG d0, UWORD y)
{
    ULONG *a2 = a0, *a1 = (ULONG *)mycop, *a3, *old;
    const ULONG len = d0;                       /* MOVE.l d0,-(a7) */
    WORD d2 = 0;
    UWORD d3w, d4w;
    ULONG d3, d4, d5, d7, pos, d1;
    UWORD *p;
    UBYTE *a0b;

    for (;;) {                                  /* 'fwloop: where it goes */
        d3 = *a1;
        if (d3 == 0xfffffffe)
            break;
        if (d3 & (1UL << 16)) {                 /* a wait */
            if (d3 == blwait) {
                if (y < 251)
                    break;
                d2 = -1;
            } else {
                d3w = (UWORD)(d3 >> 16) >> 8;   /* SWAP:LSR #8 */
                if (d2)
                    d3w += 256;
                if (d3w > y)
                    break;
            }
        }
        a1++;
    }
    d0 += 4;                                    /* 'here */
    d3w = 0;
    if (y >= 256 && !d2) {
        d0 += 4;
        d3w = 1;
        d2 = -1;
    }
    d7 = d0;                                    /* 'lower */
    d0 += mycoplen;
    d4 = d0;
    d5 = alloc(d0, MEMF_CHIP);
    a3 = (ULONG *)d5;
    for (old = (ULONG *)mycop; old < a1; )      /* CMP.l a1,a0:BCC */
        *a3++ = *old++;
    if (d3w)
        *a3++ = blwait;
    a0b = (UBYTE *)a3;
    *a0b++ = (UBYTE)y;
    *a0b++ = hwait[0];
    *(UWORD *)a0b = 0xfffe;
    a3++;
    d0 = len >> 2;                              /* MOVE.l (a7)+,d0:LSR.l #2 */
    pos = (ULONG)a3;
    do {                                        /* 'cloop2 */
        d1 = *a2++;
        *a3++ = d1;
        if (d1 == 0x01fe1234) {                 /* kludgey bit */
            a3[-1] = 0x01fe0000;
            p = (UWORD *)a2;
            d1 = (d1 & 0xffff0000UL) | *p;      /* MOVE (a2),d1 */
            *(UBYTE *)p = (UBYTE)d1;
            ((UBYTE *)p)[1] = 1;
            if (!d2 && (UWORD)d1 >= 256) {
                a3[-1] = blwait;
                d2 = -1;
            }
        }
    } while (--d0);
    for (;;) {                                  /* 'cloop3 */
        ULONG c = *a1++;

        if (c == blwait) {
            if (d2)
                continue;
            d2 = -1;
        }
        *a3++ = c;
        if (c == 0xfffffffe)
            break;
    }
    old = (ULONG *)mycop;
    dealloc(mycop, mycoplen);
    mycop = d5;
    mycoplen = d4;
    d4w = y;
    {
        UBYTE *s = slicebase;
        UWORD k = maxslice - 1;

        do {                                    /* 'adjloop */
            UWORD sy = W(s, 0);

            if (sy) {
                ULONG *q = (ULONG *)(s + 10);
                ULONG d6 = 0;
                int i;

                if (!(d4w > sy))
                    d6 = d7;
                for (i = 0; i < 5; i++, q++)
                    *q = *q - (ULONG)old + d5 + d6;
            }
            s += 64;
        } while ((WORD)--k != -1);
    }
    return pos;
}

/* inscop3 / inscop: returns d0 = position; d1 = mycop is loaded by the
   thunks */
static ULONG inscop3(ULONG *a0, ULONG len, UWORD y)
{
    return inscop2(a0, len, y);
}

static ULONG inscop(ULONG *a0, ULONG len, UWORD y)
{
    hwait[0] = 7;
    return inscop3(a0, len, y);
}

/* ------------------------------------------------------------- Slice */

/* doslice2 - a2 = arguments (-2 y, -4 width, -6 height, -8 fscreenw,
   -10 bscreenw), d1 = flags, d2 = bitplanes, d3 = sprites, d4 = colours,
   d6 = offset of the used slice variable, a3 = the slice */
static void doslice2(const UWORD *args, UWORD d1, UWORD d2, UWORD d3, UWORD d4, WORD d6)
{
    UBYTE *a3 = blitz_string;
    UWORD *a0;
    UWORD d0, d5, d7, y;
    ULONG pos;
    int i;

    *(UBYTE **)(blitz_globals + d6) = a3;       /* set current slice */
    W(a3, 2) = d1;
    W(a3, 32) = args[-3];
    if ((d1 & 1) && !(d1 & 8) && (d5 = args[-2]) > 640) {
        d0 = d5 - 640;                          /* hires overscan */
        d0 = (WORD)d0 >> 2;
        d0 = -d0 + 0x3c;
    } else {
        d0 = args[-2];
        if (d1 & 1)
            d0 >>= 1;
        if (d1 & 8) {                           /* smooth scrolling */
            if (d0 <= 320)
                goto notsmooth2;
            d0 -= 320;
issosmooth:
            d0 = (WORD)d0 >> 1;
            d0 = -d0 + 0x38;
        } else if (d0 >= 352) {
            d0 -= 336;
            goto issosmooth;
        } else {
notsmooth2:
            d0 -= 320;
            d0 &= 0xffe0;
            d0 = (WORD)d0 >> 2;
            d0 = -d0 + 0x38;
        }
    }
    hstart = d0;                                /* 'gotddf */
    hwait[0] = 1;

    a0 = maxcop;
    if (d1 & (1 << 4)) {                        /* DMA off */
        *a0++ = 0x0096;
        *a0++ = 0x0120;
    }
    if (d1 & (1 << 15)) {                       /* BPLCON0 */
        *a0++ = 0x100;
        d0 = 0;
        if (d1 & 1)
            d0 |= 1 << 15;
        d0 |= (UWORD)(d2 << 12);
        if (d1 & 4)
            d0 |= 1 << 11;
        if (d1 & 2)
            d0 |= 1 << 10;
        d0 |= bplcon0;
        *a0++ = d0;
    }
    L(a3, 22) = 0;
    if (d1 & (1 << 14)) {                       /* BPLCON1 */
        *a0++ = 0x102;
        L(a3, 22) = (ULONG)a0;
        *a0++ = 0;
    }
    L(a3, 26) = 0;
    if (d1 & (1 << 13)) {                       /* BPLCON2 */
        *a0++ = 0x104;
        L(a3, 26) = (ULONG)a0;
        *a0++ = 0x24;
    }
    if (d1 & (1 << 12)) {                       /* DIWSTRT/STOP */
        d0 = hstart + 8;
        d0 += d0;
        d0 += 1;
        *a0++ = 0x08e;
        *a0++ = (UWORD)(args[-1] << 8) | (UBYTE)d0;
        *a0++ = 0x090;
        d5 = args[-2];
        if (d1 & 1)
            d5 >>= 1;
        d0 += d5;
        *a0++ = (UWORD)((UWORD)(args[-1] + args[-3]) << 8) | (UBYTE)d0;
    }
    if (d1 & (1 << 10)) {                       /* DDFSTRT/STOP */
        d0 = hstart;
        *a0++ = 0x092;
        *a0++ = d0;
        d5 = args[-2] >> 4;
        if (d1 & 8)
            a0[-1] -= 8;
        if (!(d1 & 1)) {
            d5 -= 1;
            d5 <<= 3;
        } else {
            d5 -= 2;
            d5 <<= 2;
        }
        d0 += d5;
        *a0++ = 0x094;
        d0 &= 0xfc;
        *a0++ = d0;
        if (d1 & 1) {
            a0[-3] += 4;
            a0[-1] += 4;
        }
    }
    d0 = args[-2] >> 3;                         /* 'skip6 */
    if (d1 & 8) {
        d0 += 2;
        if (d1 & 1)
            d0 += 2;
    }
    if (d1 & (1 << 8)) {                        /* BPL1MOD */
        *a0++ = 0x108;
        *a0++ = (UWORD)((args[-4] >> 3) - d0);
    }
    if (d1 & (1 << 7)) {                        /* BPL2MOD */
        *a0++ = 0x10a;
        *a0++ = (UWORD)((args[-5] >> 3) - d0);
    }
    L(a3, 10) = 0;                              /* bitplane pointers */
    W(a3, 4) = d2;
    d0 = d2 & 15;
    if (d0) {
        UWORD d6r = 0xe0;

        L(a3, 10) = (ULONG)a0;
        d0 -= 1;
        d7 = 2;
        d2 &= 0xc000;
        if (d2) {
            d7 = 6;
            if (!(d2 & 0x8000))                 /* LSL #1,d2:BCS */
                d6r = 0xe4;
        }
        do {
            *a0++ = d6r;
            *a0++ = 0;
            d6r += 2;
            *a0++ = d6r;
            *a0++ = 0;
            d6r += d7;
        } while ((WORD)--d0 != -1);
    }
    L(a3, 14) = 0;                              /* sprite pointers */
    W(a3, 6) = d3;
    if (d3) {
        UWORD s6 = 0x140, s7 = 0x120;

        L(a3, 14) = (ULONG)a0;
        d3 -= 1;
        do {
            *a0++ = s6;
            *a0++ = 0;
            s6 += 2;
            *a0++ = s6;
            *a0++ = 0;
            s6 += 6;
            *a0++ = s7;
            *a0++ = 0;
            s7 += 2;
            *a0++ = s7;
            *a0++ = 0;
            s7 += 2;
        } while ((WORD)--d3 != -1);
    }
    L(a3, 18) = 0;                              /* colour moves */
    W(a3, 8) = d4;
    if (d4) {
        UWORD c6 = 0x180, c7 = 0;

        L(a3, 18) = (ULONG)a0;
        d4 -= 1;
        do {
            *a0++ = c6;
            *a0++ = c7;
            c6 += 2;
            c7 += 0x734;
        } while ((WORD)--d4 != -1);
    }
    y = args[-1];                               /* 'skip11 */
    d0 = y + args[-3];
    if (d1 & (1 << 4)) {
        UBYTE *b = (UBYTE *)a0;

        *b++ = (UBYTE)y;
        *b++ = 1;
        a0++;
        *a0++ = 0xfffe;
        *a0++ = 0x0096;
        *a0++ = 0x8120;
        y -= 2;
        *a0++ = 0x01fe;                         /* blwait if necessary */
        *a0++ = 0x1234;
        *a0++ = d0;                             /* wait... */
        *a0++ = 0xfffe;
        *a0++ = 0x0096;                         /* sprites off */
        *a0++ = 0x0020;
    }
    pos = inscop3((ULONG *)maxcop, (ULONG)((UBYTE *)a0 - (UBYTE *)maxcop), y);
    W(a3, 0) = y;
    for (i = 0; i < 5; i++)                     /* 'fixloop */
        L(a3, 10 + 4 * i) = L(a3, 10 + 4 * i) - (ULONG)maxcop + pos;
}

/* long form: d1 = flags, d2 = bitplanes, d3 = sprites, d4 = colours,
   d6 = used offset, a2 = stacked arguments, a3 = slice */
static void BLITZ_ROUTINE doslice2_c(const UWORD *args BL_REG(a2), ULONG d1 BL_REG(d1),
                                     ULONG d2 BL_REG(d2), ULONG d3 BL_REG(d3),
                                     ULONG d4 BL_REG(d4), ULONG d6 BL_REG(d6))
{
    doslice2(args, (UWORD)d1, (UWORD)d2, (UWORD)d3, (UWORD)d4, (WORD)d6);
}

static void BLITZ_ROUTINE bdoslice_c(const UWORD *args BL_REG(a2), ULONG d1 BL_REG(d1),
                                     ULONG d2 BL_REG(d2), ULONG d3 BL_REG(d3),
                                     ULONG d4 BL_REG(d4), ULONG d6 BL_REG(d6))
{
    doslice2(args, (UWORD)d1, (UWORD)d2, (UWORD)d3, (UWORD)d4, (WORD)d6);
    doshowblitz_c();
}

/* doqslice2 - d1 = y, d2 = flags (bits 0-2 bitplanes, 3 hires, 4 dual
   playfield, 5 HAM), d6 = used offset, a0 = GfxBase, a3 = slice */
static void doqslice2(UWORD y, UWORD fl, WORD d6, const struct GfxBase *gfx)
{
    UWORD d1 = 0xfff0, d5 = 320;

    aqslice[4] = y;
    if (fl & 8) {
        d1 |= 1;
        d5 = 640;
    }
    if (fl & 16)
        d1 |= 2;
    if (fl & 32)
        d1 |= 4;
    aqslice[2] = 256;
    if (((const UBYTE *)gfx)[207] & 1)          /* BTST #0,207(a0): NTSC */
        aqslice[2] = 200;
    aqslice[3] = d5;
    aqslice[1] = d5;
    aqslice[0] = d5;
    doslice2(aqslice + 5, d1, fl & 7, 8, 32, d6);
}

static void BLITZ_ROUTINE doqslice2_c(UWORD y BL_REG(d1), UWORD fl BL_REG(d2),
                                      ULONG d6 BL_REG(d6),
                                      const struct GfxBase *gfx BL_REG(a0))
{
    doqslice2(y, fl, (WORD)d6, gfx);
}

static void BLITZ_ROUTINE b_doqslice_c(UWORD y BL_REG(d1), UWORD fl BL_REG(d2),
                                       ULONG d6 BL_REG(d6),
                                       const struct GfxBase *gfx BL_REG(a0))
{
    doqslice2(y, fl, (WORD)d6, gfx);
    doshowblitz_c();
}

/* $A001 routines: Blitz mode entry, then the Amiga mode code */
BLITZ_AB_ROUTINE(doqslice, b_doqslice, "   jmp     _doqslice2_c");
BLITZ_AB_ROUTINE(doslice, bdoslice, "   jmp     _doslice2_c");
BLITZ_THUNK(b_doqslice, b_doqslice_c, "a2");
BLITZ_THUNK(bdoslice, bdoslice_c, "a2");

/* ---------------------------------------------- CustomCop / ColSplit */

/* CustomCop Copins$,Y: d0 = string, -4(a2) = its length, d1 = y */
static ULONG BLITZ_ROUTINE customcop_c(ULONG s BL_REG(d0), UWORD y BL_REG(d1),
                                       const ULONG *lens BL_REG(a2))
{
    return inscop((ULONG *)s, lens[-1], y);
}

static ULONG BLITZ_ROUTINE bdocustomcop_c(ULONG s BL_REG(d0), UWORD y BL_REG(d1),
                                          const ULONG *lens BL_REG(a2))
{
    ULONG pos = inscop((ULONG *)s, lens[-1], y);

    custom_l(0x080, mycop);
    setcoplist(mycop);
    return pos;
}

/* ColSplit reg,r,g,b,y - a1 = used slice */
static ULONG a_colsplit(UWORD reg, UWORD r, UWORD g, UWORD b, UWORD y, const UBYTE *a1)
{
    UWORD *cs = (UWORD *)&colstuff;

    cs[0] = (UWORD)(reg << 1) + 0x180;
    cs[1] = (UWORD)(r << 8) | (UWORD)(g << 4) | b;
    y += W(a1, 0);
    if (B(a1, 3) & (1 << 4))
        y += 2;
    return inscop((ULONG *)cs, 4, y);
}

static ULONG BLITZ_ROUTINE a_colsplit_c(UWORD reg BL_REG(d0), UWORD r BL_REG(d1),
                                        UWORD g BL_REG(d2), UWORD b BL_REG(d3),
                                        UWORD y BL_REG(d4), const UBYTE *a1 BL_REG(a1))
{
    return a_colsplit(reg, r, g, b, y, a1);
}

static ULONG BLITZ_ROUTINE b_colsplit_c(UWORD reg BL_REG(d0), UWORD r BL_REG(d1),
                                        UWORD g BL_REG(d2), UWORD b BL_REG(d3),
                                        UWORD y BL_REG(d4), const UBYTE *a1 BL_REG(a1))
{
    ULONG pos = a_colsplit(reg, r, g, b, y, a1);

    custom_l(0x080, mycop);
    setcoplist(mycop);
    return pos;
}

/* the original returns d0 = position, d1 = mycop and keeps d2-d7/a1-a3
   (inscop3's MOVEM); the Blitz versions return what $C205 leaves */
BLITZ_THUNK_POST(customcop_a, customcop_c, "a1", "move.l _mycop,d1");
BLITZ_THUNK_POST(bdocustomcop, bdocustomcop_c, "a1", "move.l _mycop,d1");
BLITZ_THUNK_POST(a_colsplit_t, a_colsplit_c, "a1", "move.l _mycop,d1");
BLITZ_THUNK_POST(b_colsplit, b_colsplit_c, "a1", "move.l _mycop,d1");
BLITZ_AB_ROUTINE(docustomcop, bdocustomcop, "   jmp     _customcop_a");
BLITZ_AB_ROUTINE(docolsplit, b_colsplit, "   jmp     _a_colsplit_t");

/* ------------------------------------------------- Show, ShowF, ShowB */

/* calcoff - d1 = x, d2 = y, a1 = slice, a0 = bitmap: d1 (x adjusted)
   and the offset d4 */
static LONG calcoff(UWORD *d1, WORD y, const UBYTE *a0, const UBYTE *a1)
{
    WORD d4;

    if (!(B(a1, 3) & 8))
        *d1 += 16;
    *d1 -= 1;
    d4 = (WORD)*d1 >> 3;
    if (B(a1, 3) & 1) {
        d4 <<= 1;
        d4 -= 2;
    }
    return (LONG)d4 + muls16((WORD)W(a0, 0), y);
}

/* the bitplane pointers: step = 8 or 16 bytes */
static void setplanes(const UBYTE *a0, UWORD *a2, LONG d4, WORD step)
{
    const ULONG *pl = (const ULONG *)(a0 + 8);
    UWORD d5 = W(a0, 4) - 1;

    do {
        ULONG d0 = *pl++ + d4;

        a2[3] = (UWORD)d0;
        a2[1] = (UWORD)(d0 >> 16);
        a2 = (UWORD *)((UBYTE *)a2 + step);
    } while ((WORD)--d5 != -1);
}

static void show(const UBYTE *a0, UBYTE *a1, UWORD d1, WORD d2)
{
    LONG d4 = calcoff(&d1, d2, a0, a1);
    UWORD d0;

    setplanes(a0, (UWORD *)L(a1, 10), d4, 8);
    d1 = (d1 & 15) ^ 15;
    d0 = d1 | (UWORD)(d1 << 4);
    *(UWORD *)L(a1, 22) = d0;
    W(a1, 30) = d0;
}

static void showf(const UBYTE *a0, UBYTE *a1, UWORD d1, WORD d2, UWORD d3)
{
    LONG d4 = calcoff(&d1, d2, a0, a1);

    setplanes(a0, (UWORD *)L(a1, 10), d4, B(a1, 4) ? 8 : 16);
    d1 = (d1 & 15) ^ 15;
    d3 = (UWORD)((((d3 - 1) & 15) ^ 15) << 4);
    d1 |= d3;
    *(UWORD *)L(a1, 22) = d1;
    W(a1, 30) = d1;
}

static void showb(const UBYTE *a0, UBYTE *a1, UWORD d1, WORD d2, UWORD d3)
{
    LONG d4 = calcoff(&d1, d2, a0, a1);
    UBYTE *a2 = (UBYTE *)L(a1, 10) + 8;         /* start at even */
    WORD d6 = 16;

    if (B(a1, 4)) {
        d6 = 8;
        a2 -= d6;
    }
    setplanes(a0, (UWORD *)a2, d4, d6);
    d1 = (UWORD)(((d1 & 15) ^ 15) << 4);
    d3 = ((d3 - 1) & 15) ^ 15;
    d1 |= d3;
    *(UWORD *)L(a1, 22) = d1;
    W(a1, 30) = d1;
}

#define SHOWARGS const UBYTE *a0 BL_REG(a0), UBYTE *a1 BL_REG(a1)
static void BLITZ_ROUTINE doshow2_c(SHOWARGS) { show(a0, a1, 0, 0); }
static void BLITZ_ROUTINE doshow_c(SHOWARGS, UWORD x BL_REG(d1), UWORD y BL_REG(d2))
{
    show(a0, a1, x, y);
}
static void BLITZ_ROUTINE doshowf2_c(SHOWARGS) { showf(a0, a1, 0, 0, 0); }
static void BLITZ_ROUTINE doshowf3_c(SHOWARGS, UWORD x BL_REG(d1), UWORD y BL_REG(d2))
{
    showf(a0, a1, x, y, 0);
}
static void BLITZ_ROUTINE doshowf_c(SHOWARGS, UWORD x BL_REG(d1), UWORD y BL_REG(d2),
                                    UWORD x2 BL_REG(d3))
{
    showf(a0, a1, x, y, x2);
}
static void BLITZ_ROUTINE doshowb2_c(SHOWARGS) { showb(a0, a1, 0, 0, 0); }
static void BLITZ_ROUTINE doshowb3_c(SHOWARGS, UWORD x BL_REG(d1), UWORD y BL_REG(d2))
{
    showb(a0, a1, x, y, 0);
}
static void BLITZ_ROUTINE doshowb_c(SHOWARGS, UWORD x BL_REG(d1), UWORD y BL_REG(d2),
                                    UWORD x2 BL_REG(d3))
{
    showb(a0, a1, x, y, x2);
}
/* the originals change d0-d7/a0/a2; nothing else */
BLITZ_THUNK(doshow2, doshow2_c, "a1");
BLITZ_THUNK(doshow, doshow_c, "a1");
BLITZ_THUNK(doshowf2, doshowf2_c, "a1");
BLITZ_THUNK(doshowf3, doshowf3_c, "a1");
BLITZ_THUNK(doshowf, doshowf_c, "a1");
BLITZ_THUNK(doshowb2, doshowb2_c, "a1");
BLITZ_THUNK(doshowb3, doshowb3_c, "a1");
BLITZ_THUNK(doshowb, doshowb_c, "a1");

/* ****************** ERRORS BELOW **************** */

BLITZ_ERRCHK_MESSAGE(mess1, "Slice already in use");
BLITZ_ERRCHK_MESSAGE(mess2, "Too many Bit planes to Show");
BLITZ_ERRCHK_MESSAGE(mess3, "Can't Show in Slice");
BLITZ_ERRCHK_MESSAGE(mess4, "Unintialized Slice");
BLITZ_ERRCHK_MESSAGE(mess5, "Colour Register out of range");
BLITZ_ERRCHK_MESSAGE(mess6, "RGB Components out of range");
BLITZ_ERRCHK_MESSAGE(mess7, "No Blitz Display Set up");
BLITZ_ERRCHK_MESSAGE(mess8, "Uninitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess9, "Lores slice width MUST be multiple of 16");
BLITZ_ERRCHK_MESSAGE(mess10, "Hires slice width MUST be multiple of 32");

static void BLITZ_ERRCHK cspltst_c(UWORD reg BL_REG(d0), UWORD r BL_REG(d1),
                                   UWORD g BL_REG(d2), UWORD b BL_REG(d3),
                                   const UBYTE *a1 BL_REG(a1))
{
    if (!W(a1, 0))
        BLITZ_RUNERR(mess4);
    else if (reg > 31)
        BLITZ_RUNERR(mess5);
    else if (r > 15 || g > 15 || b > 15)
        BLITZ_RUNERR(mess6);
}

static void BLITZ_ERRCHK slicechk_c(void)
{
    if (W(blitz_string, 0))                     /* TST (a3) */
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK slicechk2_c(const UWORD *args BL_REG(a2), UWORD d1 BL_REG(d1))
{
    UWORD w = args[-2];

    if (!(d1 & 1)) {
        if (w & 15) {
            BLITZ_RUNERR(mess9);
            return;
        }
    } else if (w & 31) {
        BLITZ_RUNERR(mess10);
        return;
    }
    slicechk_c();
}

/* showchk: TST (a0):MOVE.b 5(a0),d7:CMP.b 5(a1),d7:BHI err2 */
static void BLITZ_ERRCHK showchk(const UBYTE *a0, const UBYTE *a1)
{
    if (!W(a0, 0))
        BLITZ_RUNERR(mess8);
    else if (B(a0, 5) > B(a1, 5))
        BLITZ_RUNERR(mess2);
}

static void BLITZ_ERRCHK nshowchk_c(const UBYTE *a0 BL_REG(a0), const UBYTE *a1 BL_REG(a1))
{
    if (!W(a1, 0))
        BLITZ_RUNERR(mess4);
    else if (W(a1, 4) >= 256)
        BLITZ_RUNERR(mess3);
    else
        showchk(a0, a1);
}

/* showfchk/showbchk: below 256 bitplanes words the "dual playfield"
   check (signed byte compare), else bit 7 / 6 of +4 and showchk */
static void BLITZ_ERRCHK showfchk_c(const UBYTE *a0 BL_REG(a0), const UBYTE *a1 BL_REG(a1))
{
    if (!W(a1, 0))
        BLITZ_RUNERR(mess4);
    else if (W(a1, 4) < 256) {
        if (!W(a0, 0))
            BLITZ_RUNERR(mess8);
        else if ((BYTE)((UBYTE)(B(a1, 5) + 1) >> 1) < (BYTE)B(a0, 5))
            BLITZ_RUNERR(mess2);
    } else if (!(B(a1, 4) & 0x80))
        BLITZ_RUNERR(mess3);
    else
        showchk(a0, a1);
}

static void BLITZ_ERRCHK showbchk_c(const UBYTE *a0 BL_REG(a0), const UBYTE *a1 BL_REG(a1))
{
    if (!W(a1, 0))
        BLITZ_RUNERR(mess4);
    else if (W(a1, 4) < 256) {
        if (!W(a0, 0))
            BLITZ_RUNERR(mess8);
        else if ((BYTE)(B(a1, 5) >> 1) < (BYTE)B(a0, 5))
            BLITZ_RUNERR(mess2);
    } else if (!(B(a1, 4) & 0x40))
        BLITZ_RUNERR(mess3);
    else
        showchk(a0, a1);
}

static void BLITZ_ERRCHK sbchk_c(void)
{
    if (!mycop)
        BLITZ_RUNERR(mess7);
}

/* the originals change d7 (slicechk2 d0) - here d0-d1/a0-a1 are kept */
BLITZ_ERRCHK_THUNK(cspltst, cspltst_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(slicechk, slicechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(slicechk2, slicechk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(nshowchk, nshowchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(showfchk, showfchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(showbchk, showbchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(sbchk, sbchk_c, "d0-d1/a0-a1");
