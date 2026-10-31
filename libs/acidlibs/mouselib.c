/*
 * mouselib - command library 108: the mouse in Blitz mode - Mouse,
 * Pointer, MouseArea, MouseX, MouseY, MouseXSpeed, MouseYSpeed,
 * PositionMouse.
 *
 * C translation of bblibsrc/mouselib (binary checked: same as the
 * source).  Everything except MouseArea and PositionMouse is Blitz mode
 * only (err1 = 1 in !subs: bit 0 of the first error routine = "Blitz mode
 * only", blitz2.s makelsub3).  Pointer's first form has a check routine
 * as well (sspchk+1).
 *   init: a nullsub with !libs{#blitzcoplib,$0004}: d0 = the offset of
 *         blitzcoplib's "used slice" variable in the globals; the init
 *         keeps it and a5.
 *   Mouse On: SetInt (intlib $C100, BLibJsr) with mouseint on level
 *         $8015 (vertical blank); Mouse Off: ClrInt ($C101).
 *   mouseint: reads the mouse counters (JOY0DAT $DFF00A), adds the
 *         difference (byte, sign extended) to the position, clips it to
 *         MouseArea (max is exclusive: max-1) and, if a Pointer is set,
 *         moves that sprite with spriteslib ShowSprite (#spriteslib LSL
 *         7+2: d0 = sprite, d1 = x, d2 = y, d3 = channel) with the
 *         Pointer's slice as used slice (interrupts off meanwhile).
 * MouseX/MouseY/MouseXSpeed/MouseYSpeed are Quick: the word in the upper
 * half of d0 (SWAP:CLR).
 *
 * Kept from the original (BUG(orig)):
 *   - Mouse On initialises the last X counter from $DFF00A (the Y
 *     counter) and the last Y counter from $DFF00B (the X counter): the
 *     first interrupt moves the mouse by the difference of the two. (#223)
 * Not a bug: mouseint leaves a5 = the a5 of the init when a Pointer is set
 * (a5 is a scratch register for interrupt servers); Pointer: the sprite
 * number is the object offset >> 4 (d0 = offset = number * 16, LSR #4 gives
 * the number back).
 */
#include "blitzlib.h"
#include "blitzobj.h"

/* entry points (thunks, defined below) */
void dopointer(void), dopointer2(void), domousearea(void);
void domousex(void), domousey(void), domousexs(void), domouseys(void);
void doposmouse(void), codeinit(void), sspchk(void);
void BLITZ_ROUTINE domouse(WORD on BL_REG(d0));

BLITZ_ENTRY_DECL(init);

/* !libheader{#mouselib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_mouselib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

/* err1 = 1: Blitz mode only */

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(domouse, BLITZ_BLITZONLY, 0);
BLITZ_NAME("Mouse", "On|Off");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_spriteslib, 0x1380, LIB_blitzcoplib, 0x1202);   /* used slice */
BLITZ_SUBS(dopointer, BLITZ_BLITZONLY_CHK(sspchk), 0);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dopointer2, BLITZ_BLITZONLY, 0);
BLITZ_NAME("Pointer", "Sprite#,Sprite Channel");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(domousearea, 0, 0);
BLITZ_NAME("MouseArea", "Minx,Miny,Maxx,Maxy");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(domousex, BLITZ_BLITZONLY, 0);
BLITZ_NAME("MouseX", "");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(domousey, BLITZ_BLITZONLY, 0);
BLITZ_NAME("MouseY", "");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(domousexs, BLITZ_BLITZONLY, 0);
BLITZ_NAME("MouseXSpeed", "");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(domouseys, BLITZ_BLITZONLY, 0);
BLITZ_NAME("MouseYSpeed", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doposmouse, 0, 0);
BLITZ_NAME("PositionMouse", "x,y");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0, LIB_blitzcoplib, 0x0004);

BLITZ_LIBFIN();

/* ********************** CODE BELOW ********************** */

static WORD mx, my;                     /* mousex, mousey */
static UBYTE lx, ly;                    /* last x, last y */
static WORD xs, ys;                     /* x speed, y speed */
static WORD snum;                       /* mouse sprite number (-1 if none) */
static WORD cnum;                       /* mouse sprite channel */
static WORD minx, miny, maxx, maxy;
static UBYTE *inta5;
static ULONG uslice;
static WORD sliceo;

/* hardware registers */
#define JOY0DAT_HI (*(volatile UBYTE *)0xdff00a)
#define JOY0DAT_LO (*(volatile UBYTE *)0xdff00b)

/* intlib (Blitz mode variants, BLibJsr): d0 = level, d1 = code */
/* spriteslib ShowSprite: d0 = sprite, d1 = x, d2 = y, d3 = channel */

void mouseint(void);

/**
 * @brief PositionMouse x,y - set the mouse position (Blitz and Amiga
 * mode).
 *
 * MOVEM d0-d1,mx: only the position variables; it is not clipped to
 * MouseArea here.
 *
 * @param x d0: the x position.
 * @param y d1: the y position.
 */
static void BLITZ_ROUTINE doposmouse_c(WORD x BL_REG(d0), WORD y BL_REG(d1))
{
    mx = x;
    my = y;
}
BLITZ_THUNK(doposmouse, doposmouse_c, "d0-d1/a0-a1");

/* MouseX ... - Quick: MOVE w,d0:SWAP d0:CLR d0.  Blitz mode only. */

/** @brief MouseX - the mouse's x position (Blitz mode only).
    @return d0: the position as a Quick (the value in the upper word). */
static ULONG BLITZ_ROUTINE domousex_c(void)  { return (ULONG)(UWORD)mx << 16; }
/** @brief MouseY - the mouse's y position (Blitz mode only).
    @return d0: the position as a Quick (the value in the upper word). */
static ULONG BLITZ_ROUTINE domousey_c(void)  { return (ULONG)(UWORD)my << 16; }
/** @brief MouseXSpeed - the last horizontal movement, the counter
    difference of the last vertical blank (Blitz mode only).
    @return d0: the speed as a Quick (the value in the upper word). */
static ULONG BLITZ_ROUTINE domousexs_c(void) { return (ULONG)(UWORD)xs << 16; }
/** @brief MouseYSpeed - the last vertical movement, the counter
    difference of the last vertical blank (Blitz mode only).
    @return d0: the speed as a Quick (the value in the upper word). */
static ULONG BLITZ_ROUTINE domouseys_c(void) { return (ULONG)(UWORD)ys << 16; }
BLITZ_THUNK(domousex, domousex_c, "d1/a0-a1");
BLITZ_THUNK(domousey, domousey_c, "d1/a0-a1");
BLITZ_THUNK(domousexs, domousexs_c, "d1/a0-a1");
BLITZ_THUNK(domouseys, domouseys_c, "d1/a0-a1");

/**
 * @brief Library init - keep a5 and the offset of the used slice; mouse
 * area 0,0,320,200, no Pointer.
 *
 * A nullsub with !libs{#blitzcoplib,$0004}.
 *
 * @param off d0: the offset of blitzcoplib's "used slice" variable in the
 *        globals.
 */
static void BLITZ_ROUTINE codeinit_c(WORD off BL_REG(d0))
{
    inta5 = blitz_globals;
    sliceo = off;
    minx = 0;
    miny = 0;
    maxx = 320;
    maxy = 200;
    snum = -1;
}
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");

/**
 * @brief Pointer Sprite#,Sprite Channel - show a sprite as the mouse
 * pointer (Blitz mode only).
 *
 * The interrupt of Mouse On moves the sprite with the mouse, on the slice
 * that was used when Pointer was called.  Not a bug: the sprite number is
 * the object offset >> 4 (offset = number * 16, LSR #4).
 *
 * Errors (debugger only, sspchk): an uninitialized sprite or slice, or a
 * channel the sprite does not fit in.
 *
 * @param off d0: the sprite's object offset (number * 16).
 * @param chan d1: the sprite channel.
 * @param slice a2: the used slice (!libs #blitzcoplib,$1202).
 */
static void BLITZ_ROUTINE dopointer_c(UWORD off BL_REG(d0), WORD chan BL_REG(d1),
                                      ULONG slice BL_REG(a2))
{
    uslice = slice;
    snum = off >> 4;                    /* LSR #4,d0 */
    cnum = chan;
}
BLITZ_THUNK(dopointer, dopointer_c, "d0-d1/a0-a1");

/** @brief Pointer (no arguments) - no mouse sprite any more (Blitz mode
    only). */
static void BLITZ_ROUTINE dopointer2_c(void)
{
    snum = -1;
}
BLITZ_THUNK(dopointer2, dopointer2_c, "d0-d1/a0-a1");

/**
 * @brief MouseArea Minx,Miny,Maxx,Maxy - the area the mouse position is
 * clipped to (Blitz and Amiga mode).
 *
 * The maximum is exclusive: the interrupt clips to Maxx-1, Maxy-1.
 *
 * @param left d0: Minx.
 * @param top d1: Miny.
 * @param right d2: Maxx.
 * @param bottom d3: Maxy.
 */
static void BLITZ_ROUTINE domousearea_c(WORD left BL_REG(d0), WORD top BL_REG(d1),
                                        WORD right BL_REG(d2), WORD bottom BL_REG(d3))
{
    minx = left;
    miny = top;
    maxx = right;
    maxy = bottom;
}
BLITZ_THUNK(domousearea, domousearea_c, "d0-d1/a0-a1");

/**
 * @brief Mouse On|Off - read the mouse in the vertical blank interrupt
 * (Blitz mode only).
 *
 * On: SetInt (intlib $C100, BLibJsr) with mouseint on level $8015
 * (vertical blank); Off: ClrInt ($C101).
 *
 * BUG(orig) (#223): Mouse On initialises the last X counter from $DFF00A
 * (the Y counter) and the last Y counter from $DFF00B (the X counter):
 * the first interrupt moves the mouse by the difference of the two.
 *
 * @param on d0: On (<> 0) or Off (0).
 */
void BLITZ_ROUTINE domouse(WORD on BL_REG(d0))
{
    struct bl_regs r = { .d0 = 0x8015 };

    if (on) {
        lx = JOY0DAT_HI;                /* BUG(orig) (#223): Y counter */
        ly = JOY0DAT_LO;                /* BUG(orig) (#223): X counter */
        r.d1 = (ULONG)mouseint;
        BLITZ_BLIBJSR(TOK_setint, r);
    } else
        BLITZ_BLIBJSR(TOK_clrint, r);
}

/* one axis: add the counter difference, clip to [min, max-1] */
static WORD moveaxis(UBYTE now, UBYTE *last, WORD *speed, WORD pos, WORD min, WORD max)
{
    WORD d = (BYTE)(UBYTE)(now - *last);    /* SUB.b:EXT */

    *last = now;
    *speed = d;
    pos += d;
    if (pos < min)
        pos = min;
    else if (pos >= max)
        pos = max - 1;
    return pos;
}

/* ShowSprite with d0-d3 (a function of its own, as before) */
static void showsprite(ULONG spr, ULONG x, ULONG y, ULONG chan)
{
    BLITZ_BLIBJSR_D4(TOK_showsprite, spr, x, y, chan);
}

/* mouseint - the vertical blank server; Z set (d0 = 0) */
static ULONG BLITZ_ROUTINE mouseint_c(void)
{
    mx = moveaxis(JOY0DAT_LO, &lx, &xs, mx, minx, maxx);    /* horizontal */
    my = moveaxis(JOY0DAT_HI, &ly, &ys, my, miny, maxy);    /* vertical */
    if (snum >= 0) {
        ULONG *used, old;

        blitz_globals = inta5;          /* MOVE.l inta5(pc),a5 - stays */
        BL_INTS_OFF();
        used = (ULONG *)(blitz_globals + sliceo);
        old = *used;                    /* "Fucking with Used slice" */
        *used = uslice;
        showsprite(snum, mx, my, cnum);
        *(ULONG *)(blitz_globals + sliceo) = old;
        BL_INTS_ON();
    }
    return 0;
}
BLITZ_THUNK_Z(mouseint, mouseint_c, "a0-a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess4, "Uninitialized Sprite");
BLITZ_ERRCHK_MESSAGE(mess5, "Uninitialized Slice");
BLITZ_ERRCHK_MESSAGE(mess6, "Sprite Channel Unavailable");

/**
 * @brief The error check of Pointer Sprite#,Channel (debugger only).
 *
 * The sprite's channels (word at +6) from d1 on must fit the slice's
 * (word at +6).
 *
 * Errors: "Uninitialized Sprite", "Uninitialized Slice", "Sprite Channel
 * Unavailable".
 *
 * @param chan d1: the sprite channel.
 * @param slice a2: the used slice; a3 = the sprite.
 */
static void BLITZ_ERRCHK sspchk_c(WORD chan BL_REG(d1), const struct bb_slice *slice BL_REG(a2))
{
    const struct bb_sprite *spr = (const struct bb_sprite *)blitz_string;

    if (!spr->data)
        BLITZ_RUNERR(mess4);
    else if (!slice->y)
        BLITZ_RUNERR(mess5);
    else if ((UWORD)(chan + spr->count) > slice->sprites)
        BLITZ_RUNERR(mess6);
}
BLITZ_ERRCHK_THUNK(sspchk, sspchk_c, "d0-d1/a0-a1");
