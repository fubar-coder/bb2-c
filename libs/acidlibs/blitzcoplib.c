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
 *     slice without BPLCON1 writes to that address. (#407)
 *   - inscop2: an empty CustomCop string copies 2^32 longs (SUBQ.l/BNE)
 *     (#404); maxslice 0 runs the pointer loop 65536 times (#406); the
 *     marker also changes the source buffer (#405).
 * Not a bug: hwait is a word, written and read as its high byte (MOVE.b,
 * consistently); the slice width checks (Lores/Hires multiple) are in the
 * check routine only (runtime checks are debugger-only by design).
 * DIFF: a2 stays unchanged (CustomCop leaves it decremented by 4; the
 * port's convention, notes/cmdlibs-port.md "Allgemein").
 */
#include "blitzlib.h"
#include "blitzobj.h"
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

/* the bitmap (bitmaplib) and the slice objects (64 bytes): struct bb_bitmap,
   struct bb_slice (blitzobj.h); the slice's pointers are addresses in
   the copper list (0 = not there) */

#define TOK_setcoplist 0xc205   /* switchlib: a0 = copper list */
#define TOK_c104       0xc104   /* intlib (Blitz mode), d0 = 0 */

ULONG mycop;                    /* the copper list (chip memory) */
static ULONG mycoplen;
static struct bb_slice *slicebase;
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

/**
 * @brief SetBPLCON0 BPLCON0 Default: the bits every later Slice ORs into
 * BPLCON0.
 *
 * @param v  d0: the default (the low word is used; initially $300)
 */
static void BLITZ_ROUTINE dosetbplcon0_c(ULONG v BL_REG(d0))
{
    bplcon0 = (UWORD)v;
}

/**
 * @brief SetBLWait: the wait the copper list uses for line 256.
 *
 * Initially (and after every init) $FFE1FFFE.
 *
 * @param v  d0: the copper wait instruction
 */
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

/**
 * @brief DisplayOff: switch off bitplane and copper DMA (Blitz mode only).
 *
 * intlib $C104 with d0 = 0, then DMACON = $0180.  Errors: Blitz mode only
 * (err1 = 1).
 */
void BLITZ_ROUTINE dodisplayoff_c(void) { display(0x0180); }
/**
 * @brief DisplayOn: switch bitplane and copper DMA on (Blitz mode only).
 *
 * intlib $C104 with d0 = 0, then DMACON = $8180.  Errors: Blitz mode only
 * (err1 = 1).
 */
void BLITZ_ROUTINE dodisplayon_c(void)  { display(0x8180); }
BLITZ_THUNK(dodisplayoff, dodisplayoff_c, "a0-a1");
BLITZ_THUNK(dodisplayon, dodisplayon_c, "a0-a1");

/* makedumcop - a new dummy list */
static void makedumcop(void)
{
    ULONG *dst;
    const ULONG *src = dumcop;

    mycoplen = sizeof(dumcop);
    mycop = alloc(mycoplen, MEMF_CHIP);
    dst = (ULONG *)mycop;
    do
        *dst++ = *src++;
    while (src < dumcop + 2);
}

/**
 * @brief FreeSlices: free all slices and the copper list.
 *
 * Marks every slice unused, puts GfxBase->copinit back on COP1LC (through
 * switchlib $C205) and strobes COPJMP1 (also in Amiga mode), frees the
 * copper list and starts a new dummy list.
 *
 * @param slice  a0: slice 0 (!libs{#blitzcoplib,$1001})
 * @param n  d0: the maximum number of slices (!libs{#blitzcoplib,$0003})
 * @param gfx  a1: GfxBase
 */
static void BLITZ_ROUTINE dokillslices_c(struct bb_slice *slice BL_REG(a0), UWORD n BL_REG(d0),
                                         struct GfxBase *gfx BL_REG(a1))
{
    ULONG list;

    n--;
    do {
        slice->y = 0;
        slice++;
    } while ((WORD)--n != -1);
    list = setcoplist((ULONG)gfx->copinit);
    custom_l(0x080, list);
    custom_w(0x088, 0);
    dealloc(mycop, mycoplen);
    mycop = 0;
    makedumcop();
}
BLITZ_THUNK(dokillslices, dokillslices_c, "a2");

/**
 * @brief ShowBlitz: put the slices' copper list on the hardware (Blitz
 * mode only).
 *
 * COP1LC = the list (switchlib $C205 is told), strobe COPJMP1, FMODE = 0
 * and BPLCON3 = $C00 ("hires sprite kludge").  Errors (debugger only): "No
 * Blitz Display Set up"; Blitz mode only.
 */
static void BLITZ_ROUTINE doshowblitz_c(void)
{
    ULONG list = setcoplist(mycop);

    custom_l(0x080, list);
    custom_w(0x088, 0);
    custom_w(0x1fc, 0);
    custom_w(0x106, 0xc00);             /* "hires sprite kludge" */
}
BLITZ_THUNK(doshowblitz, doshowblitz_c, "a2");

/**
 * @brief The init routine: remember the slice objects, start the dummy
 * copper list.
 *
 * The list starts as {DMACON $0020, end}; SetBLWait's value is reset to
 * $FFE1FFFE.
 *
 * @param base  a0: slice 0 (!libs{#blitzcoplib,$1001})
 * @param max  d0: the maximum number of slices (!libs{#blitzcoplib,$0003})
 */
static void BLITZ_ROUTINE doinit_c(struct bb_slice *base BL_REG(a0), UWORD max BL_REG(d0))
{
    slicebase = base;
    maxslice = max;
    blwait = 0xffe1fffe;
    makedumcop();
}
BLITZ_THUNK(doinit, doinit_c, "a2");

/**
 * @brief CopLoc: the address of the slices' copper list (chip memory).
 * @return d0: the address
 */
static ULONG BLITZ_ROUTINE docoploc_c(void) { return mycop; }
/**
 * @brief CopLen: the length of the slices' copper list in bytes.
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE docoplen_c(void) { return mycoplen; }
BLITZ_THUNK(docoploc, docoploc_c, "d1/a0-a1");
BLITZ_THUNK(docoplen, docoplen_c, "d1/a0-a1");

/**
 * @brief The slice object's free routine: does nothing (RTS).
 */
void BLITZ_ROUTINE freeslice(void)
{
}

/* ------------------------------------------------- copper list insertion */

/* inscop2 - insert the list at a0 (d0 bytes) at line y into the main list;
   returns where the new instructions start */
static ULONG inscop2(ULONG *list, ULONG len, UWORD y)
{
    ULONG *src = list, *at = (ULONG *)mycop, *dst, *old;
    WORD past256 = 0;
    UWORD wline;                                /* the wait line */
    UWORD addblwait;                            /* insert the line-256 wait */
    UWORD yline;
    ULONG cop, newlen, newlist, inslen, pos, move;
    ULONG count;                                /* d0: the longs to copy */
    UWORD *p;
    UBYTE *wait;

    for (;;) {                                  /* 'fwloop: where it goes */
        cop = *at;
        if (cop == 0xfffffffe)
            break;
        if (cop & (1UL << 16)) {                /* a wait */
            if (cop == blwait) {
                if (y < 251)
                    break;
                past256 = -1;
            } else {
                wline = (UWORD)(cop >> 16) >> 8; /* SWAP:LSR #8 */
                if (past256)
                    wline += 256;
                if (wline > y)
                    break;
            }
        }
        at++;
    }
    inslen = len + 4;                           /* 'here */
    addblwait = 0;
    if (y >= 256 && !past256) {
        inslen += 4;
        addblwait = 1;
        past256 = -1;
    }
    newlen = inslen + mycoplen;                 /* 'lower */
    newlist = alloc(newlen, MEMF_CHIP);
    dst = (ULONG *)newlist;
    for (old = (ULONG *)mycop; old < at; )      /* CMP.l a1,a0:BCC */
        *dst++ = *old++;
    if (addblwait)
        *dst++ = blwait;
    wait = (UBYTE *)dst;
    *wait++ = (UBYTE)y;
    *wait++ = hwait[0];
    *(UWORD *)wait = 0xfffe;
    dst++;
    count = len >> 2;                           /* MOVE.l (a7)+,d0:LSR.l #2 */
    pos = (ULONG)dst;
    do {                                        /* 'cloop2 */
        move = *src++;
        *dst++ = move;
        if (move == 0x01fe1234) {               /* kludgey bit */
            dst[-1] = 0x01fe0000;
            p = (UWORD *)src;
            move = (move & 0xffff0000UL) | *p;  /* MOVE (a2),d1 */
            *(UBYTE *)p = (UBYTE)move;
            ((UBYTE *)p)[1] = 1;
            if (!past256 && (UWORD)move >= 256) {
                dst[-1] = blwait;
                past256 = -1;
            }
        }
    } while (--count);
    for (;;) {                                  /* 'cloop3 */
        ULONG c = *at++;

        if (c == blwait) {
            if (past256)
                continue;
            past256 = -1;
        }
        *dst++ = c;
        if (c == 0xfffffffe)
            break;
    }
    old = (ULONG *)mycop;
    dealloc(mycop, mycoplen);
    mycop = newlist;
    mycoplen = newlen;
    yline = y;
    {
        struct bb_slice *slice = slicebase;
        UWORD k = maxslice - 1;

        do {                                    /* 'adjloop */
            UWORD sy = slice->y;

            if (sy) {
                ULONG *q = slice->moves;
                ULONG shift = 0;
                int i;

                if (!(yline > sy))
                    shift = inslen;
                for (i = 0; i < 5; i++, q++)
                    *q = *q - (ULONG)old + newlist + shift;
            }
            slice++;
        } while ((WORD)--k != -1);
    }
    return pos;
}

/* inscop3 / inscop: returns d0 = position; d1 = mycop is loaded by the
   thunks */
static ULONG inscop3(ULONG *list, ULONG len, UWORD y)
{
    return inscop2(list, len, y);
}

static ULONG inscop(ULONG *list, ULONG len, UWORD y)
{
    hwait[0] = 7;
    return inscop3(list, len, y);
}

/* ------------------------------------------------------------- Slice */

/* doslice2 - a2 = arguments (-2 y, -4 width, -6 height, -8 fscreenw,
   -10 bscreenw), d1 = flags, d2 = bitplanes, d3 = sprites, d4 = colours,
   d6 = offset of the used slice variable, a3 = the slice */
static void doslice2(const UWORD *args, UWORD flags, UWORD planes, UWORD sprites,
                     UWORD colours, WORD used)
{
    struct bb_slice *slice = (struct bb_slice *)blitz_string;
    UWORD *cop;
    UWORD width, step, y;
    UWORD ddfstrt;                              /* hstart */
    UWORD con0;                                 /* BPLCON0 */
    UWORD diw;                                  /* DIWSTRT/DIWSTOP */
    UWORD ddf;                                  /* DDFSTRT/DDFSTOP */
    UWORD modbase;                              /* the modulo base */
    UWORD nplanes;                              /* the plane count */
    UWORD endline;                              /* the end wait line */
    ULONG pos;
    int i;

    *(struct bb_slice **)(blitz_globals + used) = slice; /* set current slice */
    slice->flags = flags;
    slice->height = args[-3];
    if ((flags & 1) && !(flags & 8) && (width = args[-2]) > 640) {
        ddfstrt = width - 640;                  /* hires overscan */
        ddfstrt = (WORD)ddfstrt >> 2;
        ddfstrt = -ddfstrt + 0x3c;
    } else {
        ddfstrt = args[-2];
        if (flags & 1)
            ddfstrt >>= 1;
        if (flags & 8) {                        /* smooth scrolling */
            if (ddfstrt <= 320)
                goto notsmooth2;
            ddfstrt -= 320;
issosmooth:
            ddfstrt = (WORD)ddfstrt >> 1;
            ddfstrt = -ddfstrt + 0x38;
        } else if (ddfstrt >= 352) {
            ddfstrt -= 336;
            goto issosmooth;
        } else {
notsmooth2:
            ddfstrt -= 320;
            ddfstrt &= 0xffe0;
            ddfstrt = (WORD)ddfstrt >> 2;
            ddfstrt = -ddfstrt + 0x38;
        }
    }
    hstart = ddfstrt;                           /* 'gotddf */
    hwait[0] = 1;

    cop = maxcop;
    if (flags & (1 << 4)) {                     /* DMA off */
        *cop++ = 0x0096;
        *cop++ = 0x0120;
    }
    if (flags & (1 << 15)) {                    /* BPLCON0 */
        *cop++ = 0x100;
        con0 = 0;
        if (flags & 1)
            con0 |= 1 << 15;
        con0 |= (UWORD)(planes << 12);
        if (flags & 4)
            con0 |= 1 << 11;
        if (flags & 2)
            con0 |= 1 << 10;
        con0 |= bplcon0;
        *cop++ = con0;
    }
    slice->con1 = 0;
    if (flags & (1 << 14)) {                    /* BPLCON1 */
        *cop++ = 0x102;
        slice->con1 = (ULONG)cop;
        *cop++ = 0;
    }
    slice->con2 = 0;
    if (flags & (1 << 13)) {                    /* BPLCON2 */
        *cop++ = 0x104;
        slice->con2 = (ULONG)cop;
        *cop++ = 0x24;
    }
    if (flags & (1 << 12)) {                    /* DIWSTRT/STOP */
        diw = hstart + 8;
        diw += diw;
        diw += 1;
        *cop++ = 0x08e;
        *cop++ = (UWORD)(args[-1] << 8) | (UBYTE)diw;
        *cop++ = 0x090;
        width = args[-2];
        if (flags & 1)
            width >>= 1;
        diw += width;
        *cop++ = (UWORD)((UWORD)(args[-1] + args[-3]) << 8) | (UBYTE)diw;
    }
    if (flags & (1 << 10)) {                    /* DDFSTRT/STOP */
        ddf = hstart;
        *cop++ = 0x092;
        *cop++ = ddf;
        width = args[-2] >> 4;
        if (flags & 8)
            cop[-1] -= 8;
        if (!(flags & 1)) {
            width -= 1;
            width <<= 3;
        } else {
            width -= 2;
            width <<= 2;
        }
        ddf += width;
        *cop++ = 0x094;
        ddf &= 0xfc;
        *cop++ = ddf;
        if (flags & 1) {
            cop[-3] += 4;
            cop[-1] += 4;
        }
    }
    modbase = args[-2] >> 3;                    /* 'skip6 */
    if (flags & 8) {
        modbase += 2;
        if (flags & 1)
            modbase += 2;
    }
    if (flags & (1 << 8)) {                     /* BPL1MOD */
        *cop++ = 0x108;
        *cop++ = (UWORD)((args[-4] >> 3) - modbase);
    }
    if (flags & (1 << 7)) {                     /* BPL2MOD */
        *cop++ = 0x10a;
        *cop++ = (UWORD)((args[-5] >> 3) - modbase);
    }
    slice->bplmoves = 0;                           /* bitplane pointers */
    slice->planes = planes;
    nplanes = planes & 15;
    if (nplanes) {
        UWORD reg = 0xe0;

        slice->bplmoves = (ULONG)cop;
        nplanes -= 1;
        step = 2;
        planes &= 0xc000;
        if (planes) {
            step = 6;
            if (!(planes & 0x8000))             /* LSL #1,d2:BCS */
                reg = 0xe4;
        }
        do {
            *cop++ = reg;
            *cop++ = 0;
            reg += 2;
            *cop++ = reg;
            *cop++ = 0;
            reg += step;
        } while ((WORD)--nplanes != -1);
    }
    slice->sprmoves = 0;                           /* sprite pointers */
    slice->sprites = sprites;
    if (sprites) {
        UWORD posreg = 0x140, ptrreg = 0x120;

        slice->sprmoves = (ULONG)cop;
        sprites -= 1;
        do {
            *cop++ = posreg;
            *cop++ = 0;
            posreg += 2;
            *cop++ = posreg;
            *cop++ = 0;
            posreg += 6;
            *cop++ = ptrreg;
            *cop++ = 0;
            ptrreg += 2;
            *cop++ = ptrreg;
            *cop++ = 0;
            ptrreg += 2;
        } while ((WORD)--sprites != -1);
    }
    slice->colmoves = 0;                           /* colour moves */
    slice->colours = colours;
    if (colours) {
        UWORD colreg = 0x180, colval = 0;

        slice->colmoves = (ULONG)cop;
        colours -= 1;
        do {
            *cop++ = colreg;
            *cop++ = colval;
            colreg += 2;
            colval += 0x734;
        } while ((WORD)--colours != -1);
    }
    y = args[-1];                               /* 'skip11 */
    endline = y + args[-3];
    if (flags & (1 << 4)) {
        UBYTE *wait = (UBYTE *)cop;

        *wait++ = (UBYTE)y;
        *wait++ = 1;
        cop++;
        *cop++ = 0xfffe;
        *cop++ = 0x0096;
        *cop++ = 0x8120;
        y -= 2;
        *cop++ = 0x01fe;                        /* blwait if necessary */
        *cop++ = 0x1234;
        *cop++ = endline;                       /* wait... */
        *cop++ = 0xfffe;
        *cop++ = 0x0096;                        /* sprites off */
        *cop++ = 0x0020;
    }
    pos = inscop3((ULONG *)maxcop, (ULONG)((UBYTE *)cop - (UBYTE *)maxcop), y);
    slice->y = y;
    for (i = 0; i < 5; i++)                     /* 'fixloop */
        slice->moves[i] = slice->moves[i] - (ULONG)maxcop + pos;
}

/**
 * @brief Slice Slice#,Y,Width,Hite,Flgs,Bitplns,Sprts,Colrs,Fscrnw,Bscrnw
 * (Amiga mode).
 *
 * Builds the slice's copper instructions in maxcop from the flags (bit 0
 * hires, 1 dual playfield, 2 HAM, 3 smooth scrolling, 4 DMA off/on + end
 * wait, 7 BPL2MOD, 8 BPL1MOD, 10 DDFSTRT/STOP, 12 DIWSTRT/STOP, 13
 * BPLCON2, 14 BPLCON1, 15 BPLCON0) and inserts them into the copper list
 * at line Y (inscop3); the slice becomes the used one.  The width checks
 * (Lores/Hires multiple) are in the check routine only.  Errors (debugger
 * only, slicechk2): "Lores slice width MUST be multiple of 16", "Hires
 * slice width MUST be multiple of 32", "Slice already in use".
 * BUG(orig) (#407): unused copper list pointers (0) are relocated like
 * the others; Show into a slice without BPLCON1 writes to that address.
 * BUG(orig) (#405): the $01FE1234 marker also changes maxcop.
 *
 * Reads a3: the slice (!libs{#blitzcoplib,$1380} from Slice# in d0).
 * @param args  a2: the argument stack: -2 Y, -4 Width, -6 Hite, -8 Fscrnw,
 *              -10 Bscrnw (words)
 * @param flags  d1: Flgs
 * @param planes  d2: Bitplns (bits 14/15: dual playfield half)
 * @param sprites  d3: Sprts
 * @param colours  d4: Colrs
 * @param used  d6: the offset of the used slice variable
 *              (!libs{#blitzcoplib,$0604})
 */
static void BLITZ_ROUTINE doslice2_c(const UWORD *args BL_REG(a2), ULONG flags BL_REG(d1),
                                     ULONG planes BL_REG(d2), ULONG sprites BL_REG(d3),
                                     ULONG colours BL_REG(d4), ULONG used BL_REG(d6))
{
    doslice2(args, (UWORD)flags, (UWORD)planes, (UWORD)sprites, (UWORD)colours, (WORD)used);
}

/**
 * @brief Slice Slice#,Y,Width,... in Blitz mode: as in Amiga mode, then
 * ShowBlitz.
 *
 * The $A001 entry for Blitz mode; the parameters are those of doslice2_c.
 */
static void BLITZ_ROUTINE bdoslice_c(const UWORD *args BL_REG(a2), ULONG flags BL_REG(d1),
                                     ULONG planes BL_REG(d2), ULONG sprites BL_REG(d3),
                                     ULONG colours BL_REG(d4), ULONG used BL_REG(d6))
{
    doslice2(args, (UWORD)flags, (UWORD)planes, (UWORD)sprites, (UWORD)colours, (WORD)used);
    doshowblitz_c();
}

/* doqslice2 - d1 = y, d2 = flags (bits 0-2 bitplanes, 3 hires, 4 dual
   playfield, 5 HAM), d6 = used offset, a0 = GfxBase, a3 = slice */
static void doqslice2(UWORD y, UWORD fl, WORD used, const struct GfxBase *gfx)
{
    UWORD flags = 0xfff0, width = 320;

    aqslice[4] = y;
    if (fl & 8) {
        flags |= 1;
        width = 640;
    }
    if (fl & 16)
        flags |= 2;
    if (fl & 32)
        flags |= 4;
    aqslice[2] = 256;
    if (((const UBYTE *)gfx)[207] & 1)          /* BTST #0,207(a0): NTSC */
        aqslice[2] = 200;
    aqslice[3] = width;
    aqslice[1] = width;
    aqslice[0] = width;
    doslice2(aqslice + 5, flags, fl & 7, 8, 32, used);
}

/**
 * @brief Slice Slice#,Y,Flags: a full screen slice (Amiga mode).
 *
 * The long form with flags $FFF0 + hires/dual playfield/HAM from Flags, 8
 * sprites, 32 colours, width 320/640, height 256 (200 on NTSC: GfxBase
 * DisplayFlags bit 0); its arguments go into aqslice.  Errors (debugger
 * only, slicechk): "Slice already in use".
 *
 * Reads a3: the slice (!libs{#blitzcoplib,$1380} from Slice# in d0).
 * @param y  d1.w: Y
 * @param fl  d2.w: Flags - bits 0-2 bitplanes, 3 hires, 4 dual playfield,
 *            5 HAM
 * @param used  d6: the offset of the used slice variable
 *              (!libs{#blitzcoplib,$0604})
 * @param gfx  a0: GfxBase
 */
static void BLITZ_ROUTINE doqslice2_c(UWORD y BL_REG(d1), UWORD fl BL_REG(d2),
                                      ULONG used BL_REG(d6),
                                      const struct GfxBase *gfx BL_REG(a0))
{
    doqslice2(y, fl, (WORD)used, gfx);
}

/**
 * @brief Slice Slice#,Y,Flags in Blitz mode: as in Amiga mode, then
 * ShowBlitz.
 *
 * The $A001 entry for Blitz mode; the parameters are those of
 * doqslice2_c.
 */
static void BLITZ_ROUTINE b_doqslice_c(UWORD y BL_REG(d1), UWORD fl BL_REG(d2),
                                       ULONG used BL_REG(d6),
                                       const struct GfxBase *gfx BL_REG(a0))
{
    doqslice2(y, fl, (WORD)used, gfx);
    doshowblitz_c();
}

/* $A001 routines: Blitz mode entry, then the Amiga mode code */
BLITZ_AB_ROUTINE(doqslice, b_doqslice, "   jmp     _doqslice2_c");
BLITZ_AB_ROUTINE(doslice, bdoslice, "   jmp     _doslice2_c");
BLITZ_THUNK(b_doqslice, b_doqslice_c, "a2");
BLITZ_THUNK(bdoslice, bdoslice_c, "a2");

/* ---------------------------------------------- CustomCop / ColSplit */

/**
 * @brief CustomCop Copins$,Y: insert copper instructions at line Y (Amiga
 * mode).
 *
 * Inserted with inscop (the wait's horizontal position 7).  The original
 * returns d0 = position, d1 = mycop and keeps d2-d7/a1-a3 (inscop3's
 * MOVEM).  DIFF: a2 stays unchanged (the original leaves it decremented
 * by 4).
 * BUG(orig) (#404): an empty string copies 2^32 longs (SUBQ.l/BNE).
 * BUG(orig) (#406): maxslice 0 runs the pointer loop 65536 times.
 *
 * @param str  d0: the copper instructions
 * @param y  d1.w: the line
 * @param lens  a2: the argument stack, -4(a2) = the string's length
 * @return d0: where the instructions start in the copper list
 */
static ULONG BLITZ_ROUTINE customcop_c(ULONG str BL_REG(d0), UWORD y BL_REG(d1),
                                       const ULONG *lens BL_REG(a2))
{
    return inscop((ULONG *)str, lens[-1], y);
}

/**
 * @brief CustomCop Copins$,Y in Blitz mode: as in Amiga mode, then the
 * new list on COP1LC and switchlib $C205.
 *
 * The $A001 entry for Blitz mode; the parameters are those of
 * customcop_c.  Returns what $C205 leaves.
 */
static ULONG BLITZ_ROUTINE bdocustomcop_c(ULONG str BL_REG(d0), UWORD y BL_REG(d1),
                                          const ULONG *lens BL_REG(a2))
{
    ULONG pos = inscop((ULONG *)str, lens[-1], y);

    custom_l(0x080, mycop);
    setcoplist(mycop);
    return pos;
}

/* ColSplit reg,r,g,b,y - a1 = used slice */
static ULONG a_colsplit(UWORD reg, UWORD r, UWORD g, UWORD b, UWORD y, const struct bb_slice *slice)
{
    UWORD *cs = (UWORD *)&colstuff;

    cs[0] = (UWORD)(reg << 1) + 0x180;
    cs[1] = (UWORD)(r << 8) | (UWORD)(g << 4) | b;
    y += slice->y;
    if (slice->flagslo & (1 << 4))
        y += 2;
    return inscop((ULONG *)cs, 4, y);
}

/**
 * @brief ColSplit Colour Register,Red,Green,Blue,Y: a colour change at
 * line Y of the used slice (Amiga mode).
 *
 * One colour move inserted with inscop at the slice's line + Y (+ 2 with
 * flag bit 4, DMA off/on).  Errors (debugger only, cspltst):
 * "Unintialized Slice", "Colour Register out of range" (> 31), "RGB
 * Components out of range" (> 15).
 *
 * @param reg  d0.w: the colour register
 * @param r  d1.w: red
 * @param g  d2.w: green
 * @param b  d3.w: blue
 * @param y  d4.w: the line in the slice
 * @param slice  a1: the used slice (!libs{#blitzcoplib,$1102})
 * @return d0: where the move is in the copper list (d1 = mycop)
 */
static ULONG BLITZ_ROUTINE a_colsplit_c(UWORD reg BL_REG(d0), UWORD r BL_REG(d1),
                                        UWORD g BL_REG(d2), UWORD b BL_REG(d3),
                                        UWORD y BL_REG(d4), const struct bb_slice *slice BL_REG(a1))
{
    return a_colsplit(reg, r, g, b, y, slice);
}

/**
 * @brief ColSplit ... in Blitz mode: as in Amiga mode, then the new list
 * on COP1LC and switchlib $C205.
 *
 * The $A001 entry for Blitz mode; the parameters are those of
 * a_colsplit_c.  Returns what $C205 leaves.
 */
static ULONG BLITZ_ROUTINE b_colsplit_c(UWORD reg BL_REG(d0), UWORD r BL_REG(d1),
                                        UWORD g BL_REG(d2), UWORD b BL_REG(d3),
                                        UWORD y BL_REG(d4), const struct bb_slice *slice BL_REG(a1))
{
    ULONG pos = a_colsplit(reg, r, g, b, y, slice);

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
static LONG calcoff(UWORD *x, WORD y, const struct bb_bitmap *bitmap, const struct bb_slice *slice)
{
    WORD offset;

    if (!(slice->flagslo & 8))
        *x += 16;
    *x -= 1;
    offset = (WORD)*x >> 3;
    if (slice->flagslo & 1) {
        offset <<= 1;
        offset -= 2;
    }
    return (LONG)offset + muls16((WORD)bitmap->bpr, y);
}

/* the bitplane pointers: step = 8 or 16 bytes */
static void setplanes(const struct bb_bitmap *bitmap, UWORD *moves, LONG offset, WORD step)
{
    const ULONG *pl = (const ULONG *)bitmap->planes;
    UWORD n = bitmap->flagsdepth - 1;

    do {
        ULONG plane = *pl++ + offset;

        moves[3] = (UWORD)plane;
        moves[1] = (UWORD)(plane >> 16);
        moves = (UWORD *)((UBYTE *)moves + step);
    } while ((WORD)--n != -1);
}

static void show(const struct bb_bitmap *bitmap, struct bb_slice *slice, UWORD x, WORD y)
{
    LONG offset = calcoff(&x, y, bitmap, slice);
    UWORD scroll;                               /* in x: the BPLCON1 scroll */
    UWORD con1;

    setplanes(bitmap, (UWORD *)slice->bplmoves, offset, 8);
    scroll = (x & 15) ^ 15;
    con1 = scroll | (UWORD)(scroll << 4);
    *(UWORD *)slice->con1 = con1;
    slice->con1val = con1;
}

static void showf(const struct bb_bitmap *bitmap, struct bb_slice *slice, UWORD x, WORD y, UWORD x2)
{
    LONG offset = calcoff(&x, y, bitmap, slice);
    UWORD scroll, scroll2;                      /* in x, x2: the BPLCON1 scrolls */

    setplanes(bitmap, (UWORD *)slice->bplmoves, offset, slice->planeshi ? 8 : 16);
    scroll = (x & 15) ^ 15;
    scroll2 = (UWORD)((((x2 - 1) & 15) ^ 15) << 4);
    scroll |= scroll2;
    *(UWORD *)slice->con1 = scroll;
    slice->con1val = scroll;
}

static void showb(const struct bb_bitmap *bitmap, struct bb_slice *slice, UWORD x, WORD y, UWORD x2)
{
    LONG offset = calcoff(&x, y, bitmap, slice);
    UBYTE *moves = (UBYTE *)slice->bplmoves + 8;   /* start at even */
    WORD step = 16;
    UWORD scroll, scroll2;                      /* in x, x2: the BPLCON1 scrolls */

    if (slice->planeshi) {
        step = 8;
        moves -= step;
    }
    setplanes(bitmap, (UWORD *)moves, offset, step);
    scroll = (UWORD)(((x & 15) ^ 15) << 4);
    scroll2 = ((x2 - 1) & 15) ^ 15;
    scroll |= scroll2;
    *(UWORD *)slice->con1 = scroll;
    slice->con1val = scroll;
}

#define SHOWARGS const struct bb_bitmap *bitmap BL_REG(a0), struct bb_slice *slice BL_REG(a1)
/**
 * @brief Show Bitmap#: show a bitmap in the used slice at 0,0.
 *
 * Writes the bitplane pointers and BPLCON1 of the slice (see doshow_c).
 *
 * @param bitmap  a0: the bitmap (!libs{#bitmaplib,$1080} from Bitmap#)
 * @param slice  a1: the used slice (!libs{#blitzcoplib,$1102})
 */
static void BLITZ_ROUTINE doshow2_c(SHOWARGS) { show(bitmap, slice, 0, 0); }
/**
 * @brief Show Bitmap#,X,Y: show a bitmap in the used slice from X,Y.
 *
 * Writes the bitplane pointers (+ the offset of x,y) and BPLCON1 (the
 * scroll value for both playfields) of the slice.  The originals change
 * d0-d7/a0/a2.  Errors (debugger only, nshowchk): "Unintialized Slice",
 * "Can't Show in Slice" (a dual playfield slice), "Uninitialized BitMap",
 * "Too many Bit planes to Show".
 * BUG(orig) (#407): a slice without BPLCON1 gets the value written at
 * its relocated 0 pointer.
 *
 * @param bitmap  a0: the bitmap (!libs{#bitmaplib,$1080} from Bitmap#)
 * @param slice  a1: the used slice (!libs{#blitzcoplib,$1102})
 * @param x  d1.w: X
 * @param y  d2.w: Y
 */
static void BLITZ_ROUTINE doshow_c(SHOWARGS, UWORD x BL_REG(d1), UWORD y BL_REG(d2))
{
    show(bitmap, slice, x, y);
}
/**
 * @brief ShowF Bitmap#: show a bitmap as the front playfield at 0,0.
 *
 * See doshowf_c.
 *
 * @param bitmap  a0: the bitmap (!libs{#bitmaplib,$1080} from Bitmap#)
 * @param slice  a1: the used slice (!libs{#blitzcoplib,$1102})
 */
static void BLITZ_ROUTINE doshowf2_c(SHOWARGS) { showf(bitmap, slice, 0, 0, 0); }
/**
 * @brief ShowF Bitmap#,X,Y: show a bitmap as the front playfield from X,Y.
 *
 * See doshowf_c; ShowB X = 0.
 *
 * @param bitmap  a0: the bitmap (!libs{#bitmaplib,$1080} from Bitmap#)
 * @param slice  a1: the used slice (!libs{#blitzcoplib,$1102})
 * @param x  d1.w: X
 * @param y  d2.w: Y
 */
static void BLITZ_ROUTINE doshowf3_c(SHOWARGS, UWORD x BL_REG(d1), UWORD y BL_REG(d2))
{
    showf(bitmap, slice, x, y, 0);
}
/**
 * @brief ShowF Bitmap#,X,Y,ShowB X: show a bitmap as the front playfield.
 *
 * Writes the bitplane pointers of the front playfield (every second move
 * of a dual playfield slice) and BPLCON1 with the scroll values of both
 * playfields.  Errors (debugger only, showfchk): "Unintialized Slice",
 * "Uninitialized BitMap", "Too many Bit planes to Show", "Can't Show in
 * Slice" (a dual playfield slice without bit 15 in its bitplanes).
 * BUG(orig) (#407): see Show.
 *
 * @param bitmap  a0: the bitmap (!libs{#bitmaplib,$1080} from Bitmap#)
 * @param slice  a1: the used slice (!libs{#blitzcoplib,$1102})
 * @param x  d1.w: X
 * @param y  d2.w: Y
 * @param x2  d3.w: the X of the back playfield (for BPLCON1)
 */
static void BLITZ_ROUTINE doshowf_c(SHOWARGS, UWORD x BL_REG(d1), UWORD y BL_REG(d2),
                                    UWORD x2 BL_REG(d3))
{
    showf(bitmap, slice, x, y, x2);
}
/**
 * @brief ShowB Bitmap#: show a bitmap as the back playfield at 0,0.
 *
 * See doshowb_c.
 *
 * @param bitmap  a0: the bitmap (!libs{#bitmaplib,$1080} from Bitmap#)
 * @param slice  a1: the used slice (!libs{#blitzcoplib,$1102})
 */
static void BLITZ_ROUTINE doshowb2_c(SHOWARGS) { showb(bitmap, slice, 0, 0, 0); }
/**
 * @brief ShowB Bitmap#,X,Y: show a bitmap as the back playfield from X,Y.
 *
 * See doshowb_c; ShowF X = 0.
 *
 * @param bitmap  a0: the bitmap (!libs{#bitmaplib,$1080} from Bitmap#)
 * @param slice  a1: the used slice (!libs{#blitzcoplib,$1102})
 * @param x  d1.w: X
 * @param y  d2.w: Y
 */
static void BLITZ_ROUTINE doshowb3_c(SHOWARGS, UWORD x BL_REG(d1), UWORD y BL_REG(d2))
{
    showb(bitmap, slice, x, y, 0);
}
/**
 * @brief ShowB Bitmap#,X,Y,ShowF X: show a bitmap as the back playfield.
 *
 * Writes the bitplane pointers of the back playfield and BPLCON1 with the
 * scroll values of both playfields.  Errors (debugger only, showbchk):
 * "Unintialized Slice", "Uninitialized BitMap", "Too many Bit planes to
 * Show", "Can't Show in Slice" (a dual playfield slice without bit 14 in
 * its bitplanes).
 * BUG(orig) (#407): see Show.
 *
 * @param bitmap  a0: the bitmap (!libs{#bitmaplib,$1080} from Bitmap#)
 * @param slice  a1: the used slice (!libs{#blitzcoplib,$1102})
 * @param x  d1.w: X
 * @param y  d2.w: Y
 * @param x2  d3.w: the X of the front playfield (for BPLCON1)
 */
static void BLITZ_ROUTINE doshowb_c(SHOWARGS, UWORD x BL_REG(d1), UWORD y BL_REG(d2),
                                    UWORD x2 BL_REG(d3))
{
    showb(bitmap, slice, x, y, x2);
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
                                   const struct bb_slice *slice BL_REG(a1))
{
    if (!slice->y)
        BLITZ_RUNERR(mess4);
    else if (reg > 31)
        BLITZ_RUNERR(mess5);
    else if (r > 15 || g > 15 || b > 15)
        BLITZ_RUNERR(mess6);
}

static void BLITZ_ERRCHK slicechk_c(void)
{
    if (((struct bb_slice *)blitz_string)->y)                     /* TST (a3) */
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK slicechk2_c(const UWORD *args BL_REG(a2), UWORD flags BL_REG(d1))
{
    UWORD w = args[-2];

    if (!(flags & 1)) {
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
static void BLITZ_ERRCHK showchk(const struct bb_bitmap *bitmap, const struct bb_slice *slice)
{
    if (!bitmap->bpr)
        BLITZ_RUNERR(mess8);
    else if (bitmap->depth > slice->planeslo)
        BLITZ_RUNERR(mess2);
}

static void BLITZ_ERRCHK nshowchk_c(const struct bb_bitmap *bitmap BL_REG(a0),
                                    const struct bb_slice *slice BL_REG(a1))
{
    if (!slice->y)
        BLITZ_RUNERR(mess4);
    else if (slice->planes >= 256)
        BLITZ_RUNERR(mess3);
    else
        showchk(bitmap, slice);
}

/* showfchk/showbchk: below 256 bitplanes words the "dual playfield"
   check (signed byte compare), else bit 7 / 6 of +4 and showchk */
static void BLITZ_ERRCHK showfchk_c(const struct bb_bitmap *bitmap BL_REG(a0),
                                    const struct bb_slice *slice BL_REG(a1))
{
    if (!slice->y)
        BLITZ_RUNERR(mess4);
    else if (slice->planes < 256) {
        if (!bitmap->bpr)
            BLITZ_RUNERR(mess8);
        else if ((BYTE)((UBYTE)(slice->planeslo + 1) >> 1) < (BYTE)bitmap->depth)
            BLITZ_RUNERR(mess2);
    } else if (!(slice->planeshi & 0x80))
        BLITZ_RUNERR(mess3);
    else
        showchk(bitmap, slice);
}

static void BLITZ_ERRCHK showbchk_c(const struct bb_bitmap *bitmap BL_REG(a0),
                                    const struct bb_slice *slice BL_REG(a1))
{
    if (!slice->y)
        BLITZ_RUNERR(mess4);
    else if (slice->planes < 256) {
        if (!bitmap->bpr)
            BLITZ_RUNERR(mess8);
        else if ((BYTE)(slice->planeslo >> 1) < (BYTE)bitmap->depth)
            BLITZ_RUNERR(mess2);
    } else if (!(slice->planeshi & 0x40))
        BLITZ_RUNERR(mess3);
    else
        showchk(bitmap, slice);
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
