/*
 * blitzhelp.h - small helpers shared by the C library ports (included by
 * blitzlib.h; no need to include it separately).
 *
 *   register arithmetic   BL_SETW BL_SETB BL_EXTL BL_SWAP bl_divu_w bl_divs_w
 *                         bl_mulu_w bl_muls_w
 *   a2 argument stack     BL_POPL BL_POPW
 *   custom chips          BL_CUSTOM BL_CUSTOMW BL_CUSTOML BL_BLITBUSY
 *                         BL_BLITWAIT BL_INTENA BL_INTS_OFF BL_INTS_ON
 *   mathffp               LVO_SPFix ... LVO_SPCeil (with BLITZ_BASEJSR1/2)
 *   system lib routines   TOK_globalloc ... (for BLITZ_ALIBJSR),
 *                         MEMBASE_localloc ... (memlib table, BLITZ_BASEJSR)
 *   command lib routines  TOK_setin ... TOK_iffopen ... TOK_getpal ...
 *
 * Only what several libraries need; anything specific to one library
 * stays in its source.
 */
#ifndef BLITZHELP_H
#define BLITZHELP_H

#include <exec/types.h>     /* already there via blitzlib.h; for the IDE */

/* ------------------------------------------------- register arithmetic */

/* Word/byte operations of the original change only the low word/byte of
   the register; these keep the rest (r: an ULONG lvalue). */
#define BL_SETW(r, v) ((r) = ((r) & 0xffff0000UL) | (UWORD)(v))   /* op.w */
#define BL_SETB(r, v) ((r) = ((r) & 0xffffff00UL) | (UBYTE)(v))   /* op.b */
#define BL_EXTL(r)    ((ULONG)(LONG)(WORD)(r))                     /* EXT.l */
#define BL_SWAP(r)    ((ULONG)(r) << 16 | (ULONG)(r) >> 16)        /* SWAP */

/* DIVU.w / DIVS.w as instructions: C has nothing with the same behaviour
   - on overflow (quotient does not fit in 16 bits) the register stays
   UNCHANGED, and a divisor of 0 raises the 68000 divide-by-zero exception
   (vector 5) like the original.  Result: remainder<<16 | quotient. */
static inline ULONG bl_divu_w(ULONG d, UWORD s)
{
    __asm__("divu.w %1,%0" : "+d"(d) : "dmi"(s) : "cc");
    return d;
}

static inline ULONG bl_divs_w(ULONG d, WORD s)
{
    __asm__("divs.w %1,%0" : "+d"(d) : "dmi"(s) : "cc");
    return d;
}

/* MULU.w / MULS.w as instructions: 16 x 16 -> 32 bit.  GCC sometimes
   turns a product of two words into a __mulsi3 call on the 68000 (e.g.
   after loop strength reduction), which a library cannot link. */
static inline ULONG bl_mulu_w(UWORD a, UWORD b)
{
    ULONG r = a;

    __asm__("mulu.w %1,%0" : "+d"(r) : "dmi"(b) : "cc");
    return r;
}

static inline LONG bl_muls_w(WORD a, WORD b)
{
    LONG r = a;

    __asm__("muls.w %1,%0" : "+d"(r) : "dmi"(b) : "cc");
    return r;
}

/* ------------------------------------------------- a2 argument stack */

/* Arguments the compiler puts on its stack (string lengths as longs,
   types as words) are read below a2: the routine takes a2 as register
   parameter (`const UBYTE *args BL_REG(a2)`, or `const ULONG *lens`) and
   pops through a local copy, the cursor `sp`; a2 itself stays unchanged
   (see notes/cmdlibs-port.md "Allgemein").  The cursor may be a pointer
   of any type.
     BL_POPL(sp)   MOVE.l -(a2),dN
     BL_POPW(sp)   MOVE.w -(a2),dN */
#define BL_POPL(sp) \
    (*(const ULONG *)((sp) = (const void *)((const UBYTE *)(sp) - 4)))
#define BL_POPW(sp) \
    (*(const UWORD *)((sp) = (const void *)((const UBYTE *)(sp) - 2)))

/* ------------------------------------------------------- custom chips */

/* Custom chip registers relative to a chip base (chipbaselib's value, a
   register parameter, or BL_CUSTOM): BL_CUSTOMW(cb, 0x58) = BLTSIZE.
   Accesses are volatile and keep the width of the original (MOVE.w /
   MOVE.l - a long write sets two registers at once, e.g. BLTCON0/1). */
#define BL_CUSTOM          ((UBYTE *)0xdff000)
#define BL_CUSTOMW(cb, o)  (*(volatile UWORD *)((UBYTE *)(cb) + (o)))
#define BL_CUSTOML(cb, o)  (*(volatile ULONG *)((UBYTE *)(cb) + (o)))
/* DMACONR BBUSY: BTST #6,$002(a1) (the byte at $002, bit 6 = bit 14) */
#define BL_BLITBUSY(cb)    (*(volatile UBYTE *)((UBYTE *)(cb) + 2) & 0x40)
/* the usual blitter wait (libmacs `blitwait`): BTST once for the Agnus
   bug, then loop while busy */
#define BL_BLITWAIT(cb)    do { (void)BL_BLITBUSY(cb); while (BL_BLITBUSY(cb)) ; } while (0)
/* INTENA: all interrupts off/on (MOVE #$4000/$C000,$DFF09A) */
#define BL_INTENA          (*(volatile UWORD *)0xdff09a)
#define BL_INTS_OFF()      (BL_INTENA = 0x4000)
#define BL_INTS_ON()       (BL_INTENA = 0xc000)

/* ------------------------------------------------------------ mathffp */

/* mathffp.library LVOs, for BLITZ_BASEJSR/BLITZ_BASEJSR1/2 with a6 = the
   ffplib base (!libs #ffplib,#la6).  (Not named SPFix ...: those are the
   NDK's inline macros.) */
#define LVO_SPFix   (-30)
#define LVO_SPFlt   (-36)
#define LVO_SPCmp   (-42)
#define LVO_SPTst   (-48)
#define LVO_SPAbs   (-54)
#define LVO_SPNeg   (-60)
#define LVO_SPAdd   (-66)
#define LVO_SPSub   (-72)
#define LVO_SPMul   (-78)
#define LVO_SPDiv   (-84)
#define LVO_SPFloor (-90)
#define LVO_SPCeil  (-96)

/* ------------------------------------------- routines of system libs */

/* Tokens of system library routines that command libraries call with
   BLITZ_ALIBJSR (names = labels of the original sources). */
#define TOK_localloc   0xc000   /* memlib: d0 = size, d1 = flags -> d0 = mem */
#define TOK_locfree    0xc001   /* memlib: a1 = mem */
#define TOK_globalloc  0xc002   /* memlib: like localloc, global list */
#define TOK_globfree   0xc003   /* memlib: a1 = mem, global list */
#define TOK_getvbr     0xc31d   /* runerrlib: a0 = VBR */
#define TOK_endjmp     0xc800   /* exitslib: End */
#define TOK_lmultiply  0xca01   /* lmullib: d0 = d0 * d1 */
#define TOK_ldivide    0xcb01   /* ldivlib: d0 = d0 / d1 */
#define TOK_strcomp2   0xce02   /* strcomplib */
#define TOK_qutofl     0xd300   /* floatquicklib: Quick -> Float (a6 = ffplib,
                                   BLITZ_ALIBJSR_A6) */
#define TOK_fltoqu     0xd301   /* floatquicklib: Float -> Quick (a6 = ffplib) */
#define TOK_setint     0xc100   /* intlib SetInt: d0 = level, d1 = code
                                   (Blitz/Amiga variants: BLIBJSR/ALIBJSR) */
#define TOK_clrint     0xc101   /* intlib ClrInt: d0 = level */
#define TOK_vwait      0xc104   /* intlib VWait: d0 = frames */
#define TOK_allocst 0xcf01      /* maxslib: d0 = source, d1 = length -> d0 = copy */
#define TOK_freest  0xcf02      /* maxslib: d0 = copy (or 0) */

/* Tokens of command library routines called by other command libraries:
   (lib << 7) + number of the token in the lib */
#define TOK_setin   ((LIB_inputoutputlib << 7) + 3)  /* inputoutputlib: input handler */
#define TOK_setout  ((LIB_inputoutputlib << 7) + 4)  /* inputoutputlib: output handler */
#define TOK_iogetsingle   ((LIB_inputoutputlib << 7) + 5)  /* -> d0 = character */
#define TOK_iogetstring   ((LIB_inputoutputlib << 7) + 6)  /* a0 = buffer, d0 = max */
#define TOK_iooutsingle   ((LIB_inputoutputlib << 7) + 7)  /* d0 = character */
#define TOK_iooutstring1  ((LIB_inputoutputlib << 7) + 8)  /* a0 = characters, d0 = length */
#define TOK_iooutstring2  ((LIB_inputoutputlib << 7) + 9)  /* a0 = string with 0 */
#define TOK_ustr   ((LIB_printlib << 7) + 6)   /* printlib UStr$: d0 = value, a2 = type */
#define TOK_val    ((LIB_vallib << 7) + 1)     /* vallib Val: d0 = string, a2 = length */
#define TOK_exists  ((LIB_fileiolib << 7) + 21)      /* fileiolib Exists: d1 = name,
                                                        a6 = dos -> d0 = size */
#define TOK_execversion ((LIB_environlib << 7) + 7)   /* environlib: -> d0.w */
#define TOK_setevfilter  ((LIB_windowslib << 7) + 58)   /* windowslib SetEventFilter */
#define TOK_extbitmap   ((LIB_bitmaplib << 7) + 4)  /* bitmaplib: a3 = object,
                                                       d1-d3 = w, h, depth */
#define TOK_getpal   ((LIB_palettelib << 7) + 3)    /* palettelib: a0 = name, a1 = palette */
#define TOK_getpal2  ((LIB_palettelib << 7) + 4)    /* palettelib: and d0 = offset */
#define TOK_showsprite ((LIB_spriteslib << 7) + 2)  /* spriteslib ShowSprite (Blitz):
                                                       d0-d3 = sprite, x, y, channel */
/* rawkeylib */
#define TOK_convertkey  ((LIB_rawkeylib << 7) + 1)   /* d0 = code, d1 = qualifier -> d0 */
#define TOK_unconvert   ((LIB_rawkeylib << 7) + 2)   /* d0 = ascii -> d0 = code, d1 */
#define TOK_qconvertkey ((LIB_rawkeylib << 7) + 3)
/* iffiolib (Z flag = ok: BLITZ_ALIBJSR_CCR) */
#define TOK_iffopen  ((LIB_iffiolib << 7) + 1)       /* a0 = name */
#define TOK_iffclose ((LIB_iffiolib << 7) + 2)
#define TOK_iffbyte     ((LIB_iffiolib << 7) + 3)    /* -> d0 */
#define TOK_iffword  ((LIB_iffiolib << 7) + 4)       /* -> d0 */
#define TOK_ifflong     ((LIB_iffiolib << 7) + 5)    /* -> d0 */
#define TOK_ifffind     ((LIB_iffiolib << 7) + 6)    /* d0 = id -> d0 = length */
#define TOK_iffskip     ((LIB_iffiolib << 7) + 7)    /* d0 = longs */
#define TOK_iffread   ((LIB_iffiolib << 7) + 8)      /* a0 = buffer, d0 = bytes */
/* ilbmifflib */
#define TOK_ilbminit    ((LIB_ilbmifflib << 7) + 1)
#define TOK_ilbmgetbyte ((LIB_ilbmifflib << 7) + 2)
#define TOK_ilbmbitmap  ((LIB_ilbmifflib << 7) + 3)  /* a0 = name, a1 = bitmap */

/* memlib's init value (!libs #memlib,#la6): a table of BRA instructions,
   called with BLITZ_BASEJSR(mem, MEMBASE_xxx, r), same registers as the
   tokens above. */
#define MEMBASE_localloc   0
#define MEMBASE_locfree    4
#define MEMBASE_globalloc  8
#define MEMBASE_globfree   12

#endif
