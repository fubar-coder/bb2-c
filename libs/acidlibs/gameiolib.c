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
 *         the original from a 16 word table indexed by (x & 3) * 2 +
 *         (y & 3) * 8 of Joyx and Joyy, here from its results in a table
 *         of 16 longs indexed by the counter bits 9, 8, 1 and 0.
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
 * Registers: Joyx, Joyy, Joyr and Joyb are assembler entries that change
 * d0 and d1 only (Joyb only d0; the original's Joyy also changes d2,
 * Joyr d2-d3); Gameb is the C function (BLITZ_ALIAS), it keeps what the
 * register rule needs (docs/libs/deviations.md, "Register
 * preservation"; the original changes d0-d4/a0-a1, GCC keeps d2-d4).
 */
#include "blitzlib.h"

/* entry points (assembler / aliases, defined below) */
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

/* Joyx, Joyy, Joyr and Joyb are assembler entries: a game calls them once
   or more per frame, and as C functions they cost 20 to 340 cycles more
   per call (the port select, the moves between the helpers and, for
   Joyr, the register saves).  The port is selected by a
   word test (TST.w d0, as the original); the original reads JOY0DAT first
   and JOY1DAT for any port but 0, here JOY1DAT first and JOY0DAT for port
   0 (the reads have no side effect; port 1, the joystick port, is 14
   cycles faster, port 0 costs the same). */

/**
 * @brief Joyx(Port): the horizontal direction of the joystick.
 *
 * From JOY0DAT (port 0) or JOY1DAT (any other port): bit 9 = left, bit 1
 * = right (the original's MOVEQ/BTST chain).  Changes d0 and d1.
 * Errors (debugger): Port must be 0 or 1.
 *
 * In:  d0.w the port
 * Out: d0 -1 left, 1 right, 0 neither (a long, MOVEQ)
 */
BLITZ_ASM_ROUTINE(_joyx,
"   move.w  0xdff00c,d1\n"              /* JOY1DAT */
"   tst.w   d0\n"
"   bne.s   0f\n"
"   move.w  0xdff00a,d1\n"              /* JOY0DAT */
"0: moveq   #-1,d0\n"
"   btst    #9,d1\n"
"   bne.s   1f\n"
"   moveq   #1,d0\n"
"   btst    #1,d1\n"
"   bne.s   1f\n"
"   moveq   #0,d0\n"
"1: rts");

/**
 * @brief Joyy(Port): the vertical direction of the joystick.
 *
 * From JOY0DAT/JOY1DAT: bit 8 ^ bit 9 = up, bit 0 ^ bit 1 = down (the
 * original's MOVE d1,d2:LSR #1,d2:EOR d2,d1, here with d0 as the scratch
 * register instead of d2).  Changes d0 and d1.
 * Errors (debugger): Port must be 0 or 1.
 *
 * In:  d0.w the port
 * Out: d0 -1 up, 1 down, 0 neither (a long, MOVEQ)
 */
BLITZ_ASM_ROUTINE(_joyy,
"   move.w  0xdff00c,d1\n"              /* JOY1DAT */
"   tst.w   d0\n"
"   bne.s   0f\n"
"   move.w  0xdff00a,d1\n"              /* JOY0DAT */
"0: move.w  d1,d0\n"
"   lsr.w   #1,d0\n"
"   eor.w   d0,d1\n"
"   moveq   #-1,d0\n"
"   btst    #8,d1\n"
"   bne.s   1f\n"
"   moveq   #1,d0\n"
"   lsr.w   #1,d1\n"
"   bcs.s   1f\n"
"   moveq   #0,d0\n"
"1: rts");

/**
 * @brief Joyr(Port): the direction of the joystick as 0..8.
 *
 * 0..7 clockwise from up, 8 = centre/invalid.  The original looks the
 * value up in the table rots (8,2,8,6, 4,3,8,5, 8,8,8,8, 0,1,8,7) at
 * (Joyx & 3) * 2 + (Joyy & 3) * 8, through BSR _joyx and BSR _joyy2;
 * Joyx and Joyy depend only on the bits 9, 8, 1 and 0 of the counter
 * word, so here one table of 16 longs indexed by those bits holds the
 * original's results, d0 as a whole (checked for every counter word):
 * not a bug (word function), the upper word of d0 is the one of Joyy's
 * MOVEQ, $FFFF for up.  Changes d0 and d1 (the original d0-d3).
 * Errors (debugger): Port must be 0 or 1.
 *
 * In:  d0.w the port
 * Out: d0.w the direction
 */
BLITZ_ASM_ROUTINE(_joyr,
"   move.w  0xdff00c,d1\n"              /* JOY1DAT */
"   tst.w   d0\n"
"   bne.s   0f\n"
"   move.w  0xdff00a,d1\n"              /* JOY0DAT */
"0: move.w  d1,d0\n"
"   lsr.w   #4,d0\n"
"   and.w   #0x30,d0\n"                 /* bit 9 -> 5, bit 8 -> 4 */
"   and.w   #3,d1\n"
"   add.w   d1,d1\n"
"   add.w   d1,d1\n"                    /* bits 1, 0 -> 3, 2 */
"   or.w    d1,d0\n"
"   move.l  1f(pc,d0.w),d0\n"
"   rts\n"
"1: .long   0x00000008,0x00000004,0x00000003,0x00000002\n"   /* bits 9, 8 = 0, 0 */
"   .long   0xffff0000,0xffff0000,0xffff0001,0xffff0001\n"   /* 0, 1: up */
"   .long   0xffff0007,0xffff0007,0xffff0007,0xffff0007\n"   /* 1, 0: up, left */
"   .long   0x00000006,0x00000005,0x00000005,0x00000006");   /* 1, 1: left */

/**
 * @brief Joyb(Port): the joystick/mouse buttons.
 *
 * Left button: CIAA PRA bit 6 (port 0) / 7 (port 1); right button: bit 2
 * / 6 of the byte $DFF016 (POTGOR bits 10/14; port 1 first writes POTGO =
 * $C000).  The original's code.  Errors (debugger): Port must be 0 or 1.
 *
 * In:  d0.w the port (0 or anything else = 1)
 * Out: d0 bit 0 = left, bit 1 = right button; only d0 changed
 */
BLITZ_ASM_ROUTINE(_joyb,
"   tst.w   d0\n"
"   bne.s   2f\n"
"   moveq   #0,d0\n"                    /* port 0 */
"   btst    #6,0xbfe001\n"
"   bne.s   0f\n"
"   moveq   #1,d0\n"
"0: btst    #2,0xdff016\n"              /* DATLY (POTGOR bit 10) */
"   bne.s   1f\n"
"   bset    #1,d0\n"
"1: rts\n"
"2: moveq   #0,d0\n"                    /* port 1 */
"   btst    #7,0xbfe001\n"
"   bne.s   3f\n"
"   moveq   #1,d0\n"
"3: move.w  #0xc000,0xdff034\n"         /* POTGO */
"   btst    #6,0xdff016\n"              /* DATRY (POTGOR bit 14) */
"   bne.s   4f\n"
"   bset    #1,d0\n"
"4: rts");

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
