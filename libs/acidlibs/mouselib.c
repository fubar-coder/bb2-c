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

/* PositionMouse - MOVEM d0-d1,mx */
static void BLITZ_ROUTINE doposmouse_c(WORD x BL_REG(d0), WORD y BL_REG(d1))
{
    mx = x;
    my = y;
}
BLITZ_THUNK(doposmouse, doposmouse_c, "d0-d1/a0-a1");

/* MouseX ... - Quick: MOVE w,d0:SWAP d0:CLR d0 */
static ULONG BLITZ_ROUTINE domousex_c(void)  { return (ULONG)(UWORD)mx << 16; }
static ULONG BLITZ_ROUTINE domousey_c(void)  { return (ULONG)(UWORD)my << 16; }
static ULONG BLITZ_ROUTINE domousexs_c(void) { return (ULONG)(UWORD)xs << 16; }
static ULONG BLITZ_ROUTINE domouseys_c(void) { return (ULONG)(UWORD)ys << 16; }
BLITZ_THUNK(domousex, domousex_c, "d1/a0-a1");
BLITZ_THUNK(domousey, domousey_c, "d1/a0-a1");
BLITZ_THUNK(domousexs, domousexs_c, "d1/a0-a1");
BLITZ_THUNK(domouseys, domouseys_c, "d1/a0-a1");

/* init - d0 = offset of the used slice */
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

/* Pointer Sprite#,Channel - a2 = used slice, d0 = sprite offset */
static void BLITZ_ROUTINE dopointer_c(UWORD off BL_REG(d0), WORD chan BL_REG(d1),
                                      ULONG slice BL_REG(a2))
{
    uslice = slice;
    snum = off >> 4;                    /* LSR #4,d0 */
    cnum = chan;
}
BLITZ_THUNK(dopointer, dopointer_c, "d0-d1/a0-a1");

/* Pointer (no arguments): no mouse sprite */
static void BLITZ_ROUTINE dopointer2_c(void)
{
    snum = -1;
}
BLITZ_THUNK(dopointer2, dopointer2_c, "d0-d1/a0-a1");

/* MouseArea Minx,Miny,Maxx,Maxy */
static void BLITZ_ROUTINE domousearea_c(WORD x0 BL_REG(d0), WORD y0 BL_REG(d1),
                                        WORD x1 BL_REG(d2), WORD y1 BL_REG(d3))
{
    minx = x0;
    miny = y0;
    maxx = x1;
    maxy = y1;
}
BLITZ_THUNK(domousearea, domousearea_c, "d0-d1/a0-a1");

/* Mouse On|Off */
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

/* sspchk - a3 = sprite, a2 = slice, d1 = channel: the sprite's channels
   (word at +6) from d1 on must fit the slice's (word at +6) */
static void BLITZ_ERRCHK sspchk_c(WORD chan BL_REG(d1), const UBYTE *slice BL_REG(a2))
{
    const UBYTE *spr = blitz_string;

    if (!*(const ULONG *)spr)
        BLITZ_RUNERR(mess4);
    else if (!*(const UWORD *)slice)
        BLITZ_RUNERR(mess5);
    else if ((UWORD)(chan + *(const UWORD *)(spr + 6)) > *(const UWORD *)(slice + 6))
        BLITZ_RUNERR(mess6);
}
BLITZ_ERRCHK_THUNK(sspchk, sspchk_c, "d0-d1/a0-a1");
