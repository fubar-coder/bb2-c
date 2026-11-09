/*
 * collslib - command library 104: collisions: SetCollOdd SetColl
 * SetCollHi (hardware collision control CLXCON), ShapesHit
 * ShapeSpriteHit SpritesHit RectsHit (rectangle tests), SColl PColl DoColl
 * (hardware collision bits; Blitz mode only).  Two empty nullsubs first
 * (the commented-out Set_Col/Get_Col of the source).
 *
 * C translation of bblibsrc/collslib; binary of the CD's acidlibs
 * checked: same as the source.
 *
 * The rectangle test (chkifhit): x1/y1 minus handle, size width/height
 * (shapes) resp. pixel width/height (sprites: the low byte of +8 and
 * height+1-1); hit when the rectangles overlap (signed word compares) ->
 * -1, else 0.  RectsHit takes width2/height2 from the a2 argument stack.
 * The test is asymmetric (kept as the original, not a bug: it may be
 * intended, it leaves a gap on one side): the first rectangle's right
 * (bottom) edge must lie beyond the second one's left (top) edge (<=: not
 * a hit), the second's right (bottom) edge only before the first's left
 * (top) edge (<: not a hit).  Rectangles that only touch are a hit when
 * the second lies left of (above) the first and no hit when it lies right
 * of (below) it, so ShapesHit(A,B) and ShapesHit(B,A) differ for shapes
 * that touch.
 * SetColl colour,planes[,playfield] writes CLXCON = $F000 | enable bits
 * for `planes` planes | colour (playfield given: colour bits spread to
 * the odd planes, playfield 2 shifted one up); SetCollHi planes: only the
 * highest plane; SetCollOdd = $F041.  They return CLXDAT (statements, the
 * value is not used).  DoColl latches CLXDAT; SColl/PColl test bits of it.
 *
 * BUG(orig), kept:
 *  - SColl tests with BTST Dn,<memory> a bit of the HIGH byte of the
 *    latched word (intended: CLXDAT bits 9-14 = sprite pairs).  Right for
 *    the lower channel first; with channel 0/1 as the SECOND argument
 *    (SColl(2,0): bit 10 instead of 9, SColl(4,0): 11 instead of 10,
 *    SColl(6,0): 12 instead of 11) or two channels of the same pair
 *    (SColl(0,1): bit 8) the wrong bit is tested. (#110)
 *  - SetColl with 0 planes loops 65536 times (DBF; only the debugger
 *    checks it). (#111)
 */
#include "blitzlib.h"
#include "blitzobj.h"

/* entry points (thunks, defined below) */
void dosetcolodd(void), dosetcol(void), dosetcol2(void), dosetcolhi(void);
void doshapeshit(void), doshsphit(void), dosphit(void), dorectshit(void);
void doscoll(void), dopcoll(void), dodocoll(void);
void setcolchk(void), setcolchk2(void), colbpchk(void), shitchk(void);
void shspchk(void), spspchk(void), scollchk(void), pcollchk(void);

#define BLITZONLY(chk) ((char *)(chk) + 1)

/* !libheader{#collslib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_collslib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_NULLSUB(0, 0, 0);
BLITZ_NULLSUB(0, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dosetcolodd, 0, 0);
BLITZ_NAME("SetCollOdd", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dosetcol, setcolchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dosetcol2, setcolchk2, 0);
BLITZ_NAME("SetColl", "Colour,Bitplanes[,Playfield 1=front, 2=back]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dosetcolhi, colbpchk, 0);
BLITZ_NAME("SetCollHi", "Bitplanes");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1080, LIB_shapeslib, 0x1183);
BLITZ_SUBS(doshapeshit, shitchk, 0);
BLITZ_NAME("ShapesHit", "(Shape#,X,Y,Shape#,X,Y)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1080, LIB_spriteslib, 0x1183);
BLITZ_SUBS(doshsphit, shspchk, 0);
BLITZ_NAME("ShapeSpriteHit", "(Shape#,X,Y,Sprite#,X,Y)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_spriteslib, 0x1080, LIB_spriteslib, 0x1183);
BLITZ_SUBS(dosphit, spspchk, 0);
BLITZ_NAME("SpritesHit", "(Sprite#,X,Y,Sprite#,X,Y)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dorectshit, 0, 0);
BLITZ_NAME("RectsHit", "(X1,Y1,Width1,Height1,X2,Y2,Width2,Height2)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doscoll, BLITZONLY(scollchk), 0);
BLITZ_NAME("SColl", "(Sprite Channel,Sprite Channel)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dopcoll, BLITZONLY(pcollchk), 0);
BLITZ_NAME("PColl", "(Sprite Channel)");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dodocoll, 1, 0);
BLITZ_NAME("DoColl", "");

BLITZ_LIBFIN();

/* ------------------------------------------------------------ hardware */

#define CLXDAT (*(volatile UWORD *)0xdff00e)
#define CLXCON (*(volatile UWORD *)0xdff098)

static UWORD colval;                            /* latched CLXDAT */

/* CLXCON = v, then result.w = CLXDAT (result = d0, its upper word stays) */
static ULONG setclx(ULONG result, UWORD v)
{
    CLXCON = v;
    return (result & 0xffff0000UL) | CLXDAT;
}

/**
 * @brief DoColl: latch the hardware collision bits (Blitz mode only).
 *
 * Reads CLXDAT (which clears it) into colval, which SColl and PColl test.
 * Errors: Blitz mode only (err1 = 1).
 */
static void BLITZ_ROUTINE dodocoll_c(void)
{
    colval = CLXDAT;
}
BLITZ_THUNK(dodocoll, dodocoll_c, "d0-d1/a0-a1");

/**
 * @brief SColl(Sprite Channel,Sprite Channel): whether two sprites collided.
 *
 * Tests the bit of the pair in the CLXDAT value latched by DoColl (bits
 * 9-14 = sprite pairs) - with `BTST Dn,<memory>`: a bit of the HIGH byte
 * of the latched word, bit number mod 8.  Blitz mode only.  A word
 * function: the upper word of d0 is the caller's.
 * Errors (debugger only): "Illegal Sprite channel" (>= 8), "Sprite
 * channels must be different".
 * BUG(orig) (#110): right only with the lower channel first; with channel
 * 0/1 as the SECOND argument (SColl(2,0): bit 10 instead of 9, SColl(4,0):
 * 11 instead of 10, SColl(6,0): 12 instead of 11) or two channels of the
 * same pair (SColl(0,1): bit 8) the wrong bit is tested.
 *
 * @param c0  d0.w: the first sprite channel (0..7)
 * @param c1  d1.w: the second sprite channel (0..7)
 * @return d0.w: -1 = collision, 0 = none
 */
static ULONG BLITZ_ROUTINE doscoll_c(ULONG c0 BL_REG(d0), ULONG c1 BL_REG(d1))
{
    UWORD a = (UWORD)c0 >> 1, b = (UWORD)c1 >> 1;

    if (a) {
        a++;
        b += a;
    }
    /* BTST d1,colval: byte operand (the high byte), bit mod 8 */
    return (c0 & 0xffff0000UL) | (*(UBYTE *)&colval & (1 << (b & 7)) ? 0xffff : 0);
}
BLITZ_THUNK(doscoll, doscoll_c, "d1/a0-a1");

/**
 * @brief PColl(Sprite Channel): whether a sprite collided with the playfield.
 *
 * Tests two bits of the CLXDAT value latched by DoColl (pair + 1 and
 * pair + 5, SNE:AND:EXT).  Blitz mode only.  A word function: the upper
 * word of d0 is the caller's.  Errors (debugger only): "Illegal Sprite
 * channel" (>= 8).
 *
 * @param c  d0.w: the sprite channel (0..7)
 * @return d0.w: -1 = both bits set, 0 otherwise
 */
static ULONG BLITZ_ROUTINE dopcoll_c(ULONG c BL_REG(d0))
{
    UWORD a = ((UWORD)c >> 1) + 1, b = a + 4;
    UWORD r = (colval & (1UL << (a & 31)) ? 0xff : 0) &
              (colval & (1UL << (b & 31)) ? 0xff : 0);

    return (c & 0xffff0000UL) | (UWORD)(WORD)(BYTE)r;  /* SNE:AND:EXT */
}
BLITZ_THUNK(dopcoll, dopcoll_c, "d1/a0-a1");

/* ----------------------------------------------------- rectangle tests */

/* x1,y1,w1,h1 against x2,y2,w2,h2 (signed words) */
static BB_BOOL chkifhit(WORD x1, WORD y1, WORD w1, WORD h1,
                        WORD x2, WORD y2, WORD w2, WORD h2)
{
    if ((WORD)(x1 + w1) <= x2)
        return BB_FALSE;
    if ((WORD)(y1 + h1) <= y2)
        return BB_FALSE;
    if ((WORD)(x2 + w2) < x1)
        return BB_FALSE;
    if ((WORD)(y2 + h2) < y1)
        return BB_FALSE;
    return BB_TRUE;
}

/* the shape (shapeslib) and the sprite (spriteslib) objects: struct bb_shape,
   struct bb_sprite (blitzobj.h) */

/**
 * @brief ShapesHit(Shape#,X,Y,Shape#,X,Y): whether two shapes overlap.
 *
 * Rectangle test (chkifhit): the position minus the shape's handle
 * (+10/+12), the size width/height (+0/+2); signed word compares.
 * Errors (debugger only): "Uninitialized Shape".
 *
 * @param x1  d1.w: x of the first shape
 * @param y1  d2.w: y of the first shape
 * @param x2  d4.w: x of the second shape
 * @param y2  d5.w: y of the second shape
 * @param a  a0: the first shape object (from Shape# in d0)
 * @param b  a1: the second shape object (from Shape# in d3)
 * @return d0: BB_TRUE if the rectangles overlap, else BB_FALSE
 */
static BB_BOOL BLITZ_ROUTINE doshapeshit_c(ULONG x1 BL_REG(d1), ULONG y1 BL_REG(d2),
                                           ULONG x2 BL_REG(d4), ULONG y2 BL_REG(d5),
                                           const struct bb_shape *a BL_REG(a0),
                                           const struct bb_shape *b BL_REG(a1))
{
    return chkifhit((WORD)x1 - (UWORD)a->handlex, (WORD)y1 - (UWORD)a->handley, a->width, a->height,
                    (WORD)x2 - (UWORD)b->handlex, (WORD)y2 - (UWORD)b->handley,
                    b->width, b->height);
}
BLITZ_THUNK(doshapeshit, doshapeshit_c, "d1/a0-a1");

/**
 * @brief ShapeSpriteHit(Shape#,X,Y,Sprite#,X,Y): whether a shape and a
 * sprite overlap.
 *
 * Rectangle test (chkifhit) as ShapesHit; the sprite: position minus its
 * handle (+12/+14), width the low byte of +8, height +4 - 1.  Errors
 * (debugger only): "Uninitialized Shape", "Uninitialized Sprite".
 *
 * @param x1  d1.w: x of the shape
 * @param y1  d2.w: y of the shape
 * @param x2  d4.w: x of the sprite
 * @param y2  d5.w: y of the sprite
 * @param a  a0: the shape object (from Shape# in d0)
 * @param b  a1: the sprite object (from Sprite# in d3)
 * @return d0: BB_TRUE if the rectangles overlap, else BB_FALSE
 */
static BB_BOOL BLITZ_ROUTINE doshsphit_c(ULONG x1 BL_REG(d1), ULONG y1 BL_REG(d2),
                                         ULONG x2 BL_REG(d4), ULONG y2 BL_REG(d5),
                                         const struct bb_shape *a BL_REG(a0),
                                         const struct bb_sprite *b BL_REG(a1))
{
    return chkifhit((WORD)x1 - (UWORD)a->handlex, (WORD)y1 - (UWORD)a->handley, a->width, a->height,
                    (WORD)x2 - b->handlex, (WORD)y2 - b->handley,
                    b->width, b->height - 1);
}
BLITZ_THUNK(doshsphit, doshsphit_c, "d1/a0-a1");

/**
 * @brief SpritesHit(Sprite#,X,Y,Sprite#,X,Y): whether two sprites overlap.
 *
 * Rectangle test (chkifhit) with the sprites' handles and pixel sizes
 * (see ShapeSpriteHit).  Errors (debugger only): "Uninitialized Sprite".
 *
 * @param x1  d1.w: x of the first sprite
 * @param y1  d2.w: y of the first sprite
 * @param x2  d4.w: x of the second sprite
 * @param y2  d5.w: y of the second sprite
 * @param a  a0: the first sprite object (from Sprite# in d0)
 * @param b  a1: the second sprite object (from Sprite# in d3)
 * @return d0: BB_TRUE if the rectangles overlap, else BB_FALSE
 */
static BB_BOOL BLITZ_ROUTINE dosphit_c(ULONG x1 BL_REG(d1), ULONG y1 BL_REG(d2),
                                       ULONG x2 BL_REG(d4), ULONG y2 BL_REG(d5),
                                       const struct bb_sprite *a BL_REG(a0),
                                       const struct bb_sprite *b BL_REG(a1))
{
    return chkifhit((WORD)x1 - a->handlex, (WORD)y1 - a->handley,
                    a->width, a->height - 1,
                    (WORD)x2 - b->handlex, (WORD)y2 - b->handley,
                    b->width, b->height - 1);
}
BLITZ_THUNK(dosphit, dosphit_c, "d1/a0-a1");

/**
 * @brief RectsHit(X1,Y1,Width1,Height1,X2,Y2,Width2,Height2): whether two
 * rectangles overlap.
 *
 * Hit when the rectangles overlap (chkifhit, signed word compares).
 *
 * @param x1  d0.w: x of the first rectangle
 * @param y1  d1.w: y of the first rectangle
 * @param w1  d2.w: its width
 * @param h1  d3.w: its height
 * @param x2  d4.w: x of the second rectangle
 * @param y2  d5.w: y of the second rectangle
 * @param args  a2: the argument stack with Width2 and Height2 (words)
 * @return d0: BB_TRUE if the rectangles overlap, else BB_FALSE
 */
static BB_BOOL BLITZ_ROUTINE dorectshit_c(ULONG x1 BL_REG(d0), ULONG y1 BL_REG(d1),
                                          ULONG w1 BL_REG(d2), ULONG h1 BL_REG(d3),
                                          ULONG x2 BL_REG(d4), ULONG y2 BL_REG(d5),
                                          const UWORD *args BL_REG(a2))
{
    const UWORD *sp = args;
    WORD w2 = (WORD)BL_POPW(sp);
    WORD h2 = (WORD)BL_POPW(sp);

    return chkifhit(x1, y1, w1, h1, x2, y2, w2, h2);
}
BLITZ_THUNK(dorectshit, dorectshit_c, "d1/a0-a1");

/* ------------------------------------------------------- SetColl ... */

/**
 * @brief SetCollOdd: collisions with the odd bitplanes (CLXCON = $F041).
 *
 * @param result  d0: the caller's register (its upper word stays)
 * @return d0.w: CLXDAT (a statement: the value is not used)
 */
static ULONG BLITZ_ROUTINE dosetcolodd_c(ULONG result BL_REG(d0))
{
    return setclx(result, 64 + 1 + 0xf000);
}
BLITZ_THUNK(dosetcolodd, dosetcolodd_c, "d1/a0-a1");

/* d0 = colour, d1 = bitplanes */
static ULONG setcol(ULONG colour, UWORD planes)
{
    UWORD enable = 0, n = planes - 1;

    do
        enable = (UWORD)(enable << 1) | 0x40;
    while (n-- != 0);
    return setclx(colour, enable | (UWORD)colour | 0xf000);
}

/**
 * @brief SetColl Colour,Bitplanes: collisions with a colour.
 *
 * CLXCON = $F000 | enable bits for the planes | colour.  Errors (debugger
 * only): "Illegal number of BitPlanes" (not 1..6), "Illegal Colour for
 * Collision" (>= 2^planes).
 * BUG(orig) (#111): 0 planes loop 65536 times (DBF).
 *
 * @param col  d0.w: the colour
 * @param planes  d1.w: the number of bitplanes
 * @return d0.w: CLXDAT (a statement: the value is not used); the upper
 *         word is the colour's
 */
static ULONG BLITZ_ROUTINE dosetcol_c(ULONG col BL_REG(d0), ULONG planes BL_REG(d1))
{
    return setcol(col, (UWORD)planes);
}
BLITZ_THUNK(dosetcol, dosetcol_c, "d1/a0-a1");

/* spreadit: insert 0's between the bits (6 bits) */
static UWORD spreadit(UWORD bits)
{
    UWORD spread = 0, pos = 0;
    int i;

    for (i = 0; i < 6; i++) {
        if (bits & 1)
            spread |= (UWORD)(1 << (pos & 15));
        bits >>= 1;
        pos += 2;
    }
    return spread;
}

/**
 * @brief SetColl Colour,Bitplanes,Playfield: collisions with a colour of
 * one playfield.
 *
 * The colour bits are spread to the odd planes (spreadit), playfield 2
 * (anything but 1) one bit up; then as SetColl Colour,Bitplanes.  Errors
 * (debugger only): "PlayField must be 1 or 2", then those of SetColl.
 * BUG(orig) (#111): 0 planes loop 65536 times (DBF).
 *
 * @param col  d0.w: the colour
 * @param planes  d1.w: the number of bitplanes
 * @param pf  d2.w: the playfield, 1 = front, 2 = back
 * @return d0.w: CLXDAT (a statement: the value is not used)
 */
static ULONG BLITZ_ROUTINE dosetcol2_c(ULONG col BL_REG(d0), ULONG planes BL_REG(d1),
                                       ULONG pf BL_REG(d2))
{
    UWORD spread = spreadit((UWORD)col);

    if ((UWORD)pf != 1)
        spread <<= 1;
    return setcol((col & 0xffff0000UL) | spread, (UWORD)planes);
}
BLITZ_THUNK(dosetcol2, dosetcol2_c, "d1/a0-a1");

/**
 * @brief SetCollHi Bitplanes: collisions with the highest bitplane only.
 *
 * CLXCON = $F000 | the enable and match bit of plane `planes`.  Errors
 * (debugger only): "Illegal number of BitPlanes" (not 1..6).
 *
 * @param planes  d0.w: the number of bitplanes
 * @return d0.w: CLXDAT (a statement: the value is not used)
 */
static ULONG BLITZ_ROUTINE dosetcolhi_c(ULONG planes BL_REG(d0))
{
    UWORD n = (UWORD)(planes - 1) & 63;
    UWORD enable = n < 16 ? (UWORD)(1 << n) : 0;

    enable |= (UWORD)(enable << 6);
    return setclx(planes, enable | 0xf000);
}
BLITZ_THUNK(dosetcolhi, dosetcolhi_c, "d1/a0-a1");

/* ************************** ERROR CHECKING ************************** */

BLITZ_ERRCHK_MESSAGE(mess1, "Illegal number of BitPlanes");
BLITZ_ERRCHK_MESSAGE(mess2, "Illegal Colour for Collision");
BLITZ_ERRCHK_MESSAGE(mess3, "PlayField must be 1 or 2");
BLITZ_ERRCHK_MESSAGE(mess4, "Uninitialized Shape");
BLITZ_ERRCHK_MESSAGE(mess5, "Uninitialized Sprite");
BLITZ_ERRCHK_MESSAGE(mess6, "Illegal Sprite channel");
BLITZ_ERRCHK_MESSAGE(mess7, "Sprite channels must be different");

/* d0 = colour, d1 = bitplanes */
static void BLITZ_ERRCHK setcolchk_c(ULONG col BL_REG(d0), ULONG planes BL_REG(d1))
{
    UWORD n = (UWORD)planes & 63;

    if (!(UWORD)planes || (UWORD)planes > 6)
        BLITZ_RUNERR(mess1);
    if ((UWORD)col >= (UWORD)(n < 16 ? 1 << n : 0))   /* MOVEQ #1:LSL d1 (long) */
        BLITZ_RUNERR(mess2);
}

static void BLITZ_ERRCHK setcolchk2_c(ULONG col BL_REG(d0), ULONG planes BL_REG(d1),
                                      ULONG pf BL_REG(d2))
{
    if ((UWORD)pf != 1 && (UWORD)pf != 2)
        BLITZ_RUNERR(mess3);
    setcolchk_c(col, planes);
}

static void BLITZ_ERRCHK colbpchk_c(ULONG planes BL_REG(d0))
{
    if (!(UWORD)planes || (UWORD)planes > 6)
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK shitchk_c(const struct bb_shape *a BL_REG(a0),
                                   const struct bb_shape *b BL_REG(a1))
{
    if (!a->width || !b->width)
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK shspchk_c(const struct bb_shape *a BL_REG(a0),
                                   const struct bb_sprite *b BL_REG(a1))
{
    if (!a->width)
        BLITZ_RUNERR(mess4);
    if (!b->data)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK spspchk_c(const struct bb_sprite *a BL_REG(a0),
                                   const struct bb_sprite *b BL_REG(a1))
{
    if (!a->data || !b->data)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK scollchk_c(ULONG c0 BL_REG(d0), ULONG c1 BL_REG(d1))
{
    if ((UWORD)c0 >= 8 || (UWORD)c1 >= 8)
        BLITZ_RUNERR(mess6);
    if ((UWORD)c0 == (UWORD)c1)
        BLITZ_RUNERR(mess7);
}

static void BLITZ_ERRCHK pcollchk_c(ULONG c BL_REG(d0))
{
    if ((UWORD)c >= 8)
        BLITZ_RUNERR(mess6);
}

/* the thunks after all check functions: their assembler switches back to
   .text, which GCC does not notice (a check function after a thunk would
   end up in the normal code) */
BLITZ_ERRCHK_THUNK(setcolchk, setcolchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(setcolchk2, setcolchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(colbpchk, colbpchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(shitchk, shitchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(shspchk, shspchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(spspchk, spspchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(scollchk, scollchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(pcollchk, pcollchk_c, "d0-d1/a0-a1");
