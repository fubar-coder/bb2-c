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

static void BLITZ_ROUTINE dodocoll_c(void)
{
    colval = CLXDAT;
}
BLITZ_THUNK(dodocoll, dodocoll_c, "d0-d1/a0-a1");

/* SColl: d0, d1 = sprite channels */
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

/* PColl: d0 = sprite channel */
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
static ULONG chkifhit(WORD x1, WORD y1, WORD w1, WORD h1,
                      WORD x2, WORD y2, WORD w2, WORD h2)
{
    if ((WORD)(x1 + w1) <= x2)
        return 0;
    if ((WORD)(y1 + h1) <= y2)
        return 0;
    if ((WORD)(x2 + w2) < x1)
        return 0;
    if ((WORD)(y2 + h2) < y1)
        return 0;
    return (ULONG)-1;
}

#define SW(s, o) (*(UWORD *)((UBYTE *)(s) + (o)))

/* a0 = shape 1, a1 = shape 2, d1/d2 = x1/y1, d4/d5 = x2/y2 */
static ULONG BLITZ_ROUTINE doshapeshit_c(ULONG x1 BL_REG(d1), ULONG y1 BL_REG(d2),
                                         ULONG x2 BL_REG(d4), ULONG y2 BL_REG(d5),
                                         UBYTE *a BL_REG(a0), UBYTE *b BL_REG(a1))
{
    return chkifhit((WORD)x1 - SW(a, 10), (WORD)y1 - SW(a, 12), SW(a, 0), SW(a, 2),
                    (WORD)x2 - SW(b, 10), (WORD)y2 - SW(b, 12), SW(b, 0), SW(b, 2));
}
BLITZ_THUNK(doshapeshit, doshapeshit_c, "d1/a0-a1");

/* a0 = shape, a1 = sprite */
static ULONG BLITZ_ROUTINE doshsphit_c(ULONG x1 BL_REG(d1), ULONG y1 BL_REG(d2),
                                       ULONG x2 BL_REG(d4), ULONG y2 BL_REG(d5),
                                       UBYTE *a BL_REG(a0), UBYTE *b BL_REG(a1))
{
    return chkifhit((WORD)x1 - SW(a, 10), (WORD)y1 - SW(a, 12), SW(a, 0), SW(a, 2),
                    (WORD)x2 - SW(b, 12), (WORD)y2 - SW(b, 14),
                    *(UBYTE *)(b + 9), SW(b, 4) - 1);
}
BLITZ_THUNK(doshsphit, doshsphit_c, "d1/a0-a1");

/* a0 = sprite 1, a1 = sprite 2 */
static ULONG BLITZ_ROUTINE dosphit_c(ULONG x1 BL_REG(d1), ULONG y1 BL_REG(d2),
                                     ULONG x2 BL_REG(d4), ULONG y2 BL_REG(d5),
                                     UBYTE *a BL_REG(a0), UBYTE *b BL_REG(a1))
{
    return chkifhit((WORD)x1 - SW(a, 12), (WORD)y1 - SW(a, 14),
                    *(UBYTE *)(a + 9), SW(a, 4) - 1,
                    (WORD)x2 - SW(b, 12), (WORD)y2 - SW(b, 14),
                    *(UBYTE *)(b + 9), SW(b, 4) - 1);
}
BLITZ_THUNK(dosphit, dosphit_c, "d1/a0-a1");

/* RectsHit: d0-d5 = x1,y1,w1,h1,x2,y2; w2, h2 from the a2 stack */
static ULONG BLITZ_ROUTINE dorectshit_c(ULONG x1 BL_REG(d0), ULONG y1 BL_REG(d1),
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

/* d2 = playfield (1 = front, otherwise back: one bit up) */
static ULONG BLITZ_ROUTINE dosetcol2_c(ULONG col BL_REG(d0), ULONG planes BL_REG(d1),
                                       ULONG pf BL_REG(d2))
{
    UWORD spread = spreadit((UWORD)col);

    if ((UWORD)pf != 1)
        spread <<= 1;
    return setcol((col & 0xffff0000UL) | spread, (UWORD)planes);
}
BLITZ_THUNK(dosetcol2, dosetcol2_c, "d1/a0-a1");

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

static void BLITZ_ERRCHK shitchk_c(UWORD *a BL_REG(a0), UWORD *b BL_REG(a1))
{
    if (!*a || !*b)
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK shspchk_c(UWORD *a BL_REG(a0), ULONG *b BL_REG(a1))
{
    if (!*a)
        BLITZ_RUNERR(mess4);
    if (!*b)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK spspchk_c(ULONG *a BL_REG(a0), ULONG *b BL_REG(a1))
{
    if (!*a || !*b)
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
