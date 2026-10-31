/*
 * gameiolib - command library 190: joysticks and CD32 pads read from the
 * hardware - Joyx, Joyy, Joyr, Joyb, Gameb.
 *
 * C translation of bblibsrc/gameiolib (binary checked: same as the
 * source).  Every function reads the custom chips/CIA directly (works in
 * Amiga and Blitz mode):
 *   Joyx/Joyy: JOY0DAT ($DFF00A, port 0) or JOY1DAT ($DFF00C, any other
 *         port: TST.w d0), direction from the quadrature bits (x: bit 9 =
 *         left -1, bit 1 = right 1; y: bit 8 ^ bit 9 = up -1, bit 0 ^
 *         bit 1 = down 1), else 0.
 *   Joyr: 8 directions as 0..7 clockwise from up, 8 = centre/invalid,
 *         from a 16 word table indexed by (x & 3) * 2 + (y & 3) * 8.
 *   Joyb: bit 0 = left button (CIAA PRA bit 6/7), bit 1 = right button
 *         (BTST #2 / #6 on the byte $DFF016 = POTGOR bits 10/14; port 1
 *         first writes POTGO = $C000).
 *   Gameb: CD32 pad: the fire line is switched to output (CIAA DDRA),
 *         POTGO set, then 7 bits are clocked out with the fire line (each
 *         after 8 CIA reads as delay) and read from POTGOR; a bit is set
 *         when the line is low: 1 play/pause, 2 reverse, 4 forward,
 *         8 green, 16 yellow, 32 red, 64 blue.
 *   err: Joyport must be 0 or 1 (word compare).
 *
 * Kept from the original (BUG(orig)):
 *   - Gameb on port 0: the DBF of its loop jumps into the loop of port 1
 *     (the code is the same, registers already set), so it also leaves
 *     through the port 1 exit: POTGO = $3000 instead of $0300 (the DDRA
 *     bit cleared is the right one, d3 = 6). (#415)
 *   - Not a bug (word function, the compiler uses the low word only): Joyr:
 *     the upper word of d0 is the one of Joyy's MOVEQ (-1 -> $FFFF).
 * Registers: the thunks keep what the original keeps (Joyx/Joyy/Joyr
 * change d0-d1 (Joyy d2, Joyr d2-d3), Joyb only d0; Gameb changes
 * d0-d4/a0-a1 in the original, GCC keeps d2-d4).
 */
#include "blitzlib.h"

/* entry points (thunks, defined below) */
void _joyx(void), _joyy(void), _joyr(void), _joyb(void), _joychk(void);
ULONG BLITZ_ROUTINE _gameb(UWORD port BL_REG(d0));

/* !libheader{#gameiolib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_gameiolib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_joyx, _joychk, 0);
BLITZ_NAME("Joyx", "(Port)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_joyy, _joychk, 0);
BLITZ_NAME("Joyy", "(Port)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_joyr, _joychk, 0);
BLITZ_NAME("Joyr", "(Port)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_joyb, _joychk, 0);
BLITZ_NAME("Joyb", "(Port)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_gameb, _joychk, 0);
BLITZ_NAME("Gameb", "(Port)");

BLITZ_LIBFIN();

/* ***** CODE SECTION ***** */

#define JOY0DAT (*(volatile UWORD *)0xdff00a)
#define JOY1DAT (*(volatile UWORD *)0xdff00c)
#define POTGOR  (*(volatile UWORD *)0xdff016)
#define POTGORH (*(volatile UBYTE *)0xdff016)   /* BTST #n,$DFF016: high byte */
#define POTGO   (*(volatile UWORD *)0xdff034)
#define CIAAPRA ((volatile UBYTE *)0xbfe001)    /* +$200 = DDRA */

static UWORD joydat(UWORD port)
{
    return port ? JOY1DAT : JOY0DAT;
}

/* _joyx with dat = d1 = the counter word */
static LONG joyx(UWORD dat)
{
    if (dat & (1 << 9))
        return -1;
    if (dat & (1 << 1))
        return 1;
    return 0;
}

/* _joyy2: MOVE d1,d2:LSR #1,d2:EOR d2,d1 */
static LONG joyy(UWORD dat)
{
    dat ^= dat >> 1;
    if (dat & (1 << 8))
        return -1;
    if (dat & 1)                        /* LSR #1,d1:BCS */
        return 1;
    return 0;
}

/**
 * @brief Joyx(Port): the horizontal direction of the joystick.
 *
 * From JOY0DAT (port 0) or JOY1DAT (any other port, TST.w d0): bit 9 =
 * left, bit 1 = right.  Errors (debugger): Port must be 0 or 1.
 *
 * @param port  d0: the port
 * @return d0: -1 left, 1 right, 0 neither
 */
static LONG BLITZ_ROUTINE _joyx_c(UWORD port BL_REG(d0))
{
    return joyx(joydat(port));
}

/**
 * @brief Joyy(Port): the vertical direction of the joystick.
 *
 * From JOY0DAT/JOY1DAT: bit 8 ^ bit 9 = up, bit 0 ^ bit 1 = down.
 * Errors (debugger): Port must be 0 or 1.
 *
 * @param port  d0: the port
 * @return d0: -1 up, 1 down, 0 neither
 */
static LONG BLITZ_ROUTINE _joyy_c(UWORD port BL_REG(d0))
{
    return joyy(joydat(port));
}

/* rots: index (x & 3) * 2 + (y & 3) * 8 (words) */
static const UWORD rots[16] = { 8, 2, 8, 6, 4, 3, 8, 5, 8, 8, 8, 8, 0, 1, 8, 7 };

/**
 * @brief Joyr(Port): the direction of the joystick as 0..8.
 *
 * 0..7 clockwise from up, 8 = centre/invalid, from the table rots indexed
 * by (x & 3) * 2 + (y & 3) * 8 of Joyx/Joyy.  Not a bug (word function):
 * the upper word of d0 is the one of Joyy's MOVEQ.  Errors (debugger):
 * Port must be 0 or 1.
 *
 * @param port  d0: the port
 * @return d0.w: the direction
 */
static ULONG BLITZ_ROUTINE _joyr_c(UWORD port BL_REG(d0))
{
    UWORD dat = joydat(port);
    UWORD x = (UWORD)joyx(dat);
    ULONG y = (ULONG)joyy(dat);         /* MOVEQ: upper word kept below */
    UWORD i = (UWORD)((y & 3) << 3 | (x & 3) << 1);

    return (y & 0xffff0000UL) | rots[i >> 1];
}

/**
 * @brief Joyb(Port): the joystick/mouse buttons.
 *
 * Left button: CIAA PRA bit 6 (port 0) / 7 (port 1); right button: bit 2
 * / 6 of the byte $DFF016 (POTGOR bits 10/14; port 1 first writes POTGO =
 * $C000).  Errors (debugger): Port must be 0 or 1.
 *
 * @param port  d0: the port (0 or anything else = 1)
 * @return d0: bit 0 = left, bit 1 = right button; only d0 changed
 */
static ULONG BLITZ_ROUTINE _joyb_c(UWORD port BL_REG(d0))
{
    ULONG buttons = 0;                          /* d0 */

    if (!port) {
        if (!(*CIAAPRA & (1 << 6)))
            buttons = 1;
        if (!(POTGORH & (1 << 2)))      /* DATLY (bit 10) */
            buttons |= 2;
    } else {
        if (!(*CIAAPRA & (1 << 7)))
            buttons = 1;
        POTGO = 0xc000;
        if (!(POTGORH & (1 << 6)))      /* DATRY (bit 14) */
            buttons |= 2;
    }
    return buttons;
}

BLITZ_THUNK(_joyx, _joyx_c, "a0-a1");
BLITZ_THUNK(_joyy, _joyy_c, "a0-a1");
BLITZ_THUNK(_joyr, _joyr_c, "a0-a1");
BLITZ_THUNK(_joyb, _joyb_c, "d1/a0-a1");

/**
 * @brief Gameb(Port): the buttons of a CD32 pad.
 *
 * The fire line is switched to output (CIAA DDRA), POTGO set, then 7 bits
 * are clocked out with the fire line (each after 8 CIA reads as delay)
 * and read from POTGOR.
 * BUG(orig) (#415): POTGO = $3000 at the end also for port 0 (the DBF of
 * the port 0 loop jumps into the loop of port 1).
 * Errors (debugger): Port must be 0 or 1.
 *
 * @param port  d0: the port (0 or anything else = 1)
 * @return d0: 1 play/pause, 2 reverse, 4 forward, 8 green, 16 yellow,
 *         32 red, 64 blue (a bit is set when the line is low)
 */
ULONG BLITZ_ROUTINE _gameb(UWORD port BL_REG(d0))
{
    volatile UBYTE *ciapra = CIAAPRA;           /* a0 */
    UWORD potmask;                              /* d4: the DATxY bit */
    UBYTE bit;
    ULONG buttons = 0;                          /* d0 */
    WORD bitnum = 6;                            /* d1 */
    int i;

    if (port) {
        bit = 1 << 7;
        potmask = 0x4000;
        ciapra[0x200] |= bit;
        *ciapra &= ~bit;
        POTGO = 0x2000;
    } else {
        bit = 1 << 6;
        potmask = 0x0400;
        ciapra[0x200] |= bit;
        *ciapra &= ~bit;
        POTGO = 0x0200;
    }
    do {
        UWORD pot;                              /* d2 */

        for (i = 0; i < 8; i++)         /* TST.b (a0) x 8: delay */
            (void)*ciapra;
        pot = POTGOR;
        *ciapra |= bit;
        *ciapra &= ~bit;
        if (!(pot & potmask))
            buttons |= 1UL << bitnum;
    } while (--bitnum >= 0);            /* DBF d1 */
    POTGO = 0x3000;                     /* BUG(orig) (#415): also for port 0 */
    ciapra[0x200] &= ~bit;
    return buttons;
}

/* -------------------------------------------- errors (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess1, "Joyport must be 0 or 1");

/* _joychk: TST d0:BEQ ok:CMP #1,d0:BEQ ok */
static void BLITZ_ERRCHK _joychk_c(UWORD port BL_REG(d0))
{
    if (port != 0 && port != 1)
        BLITZ_RUNERR(mess1);
}
BLITZ_ERRCHK_THUNK(_joychk, _joychk_c, "d0-d1/a0-a1");
