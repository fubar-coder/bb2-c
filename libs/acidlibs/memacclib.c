/*
 * memacclib - command library 180: Poke, Peek, Peeks$, Call.
 *
 * C translation of bblibsrc/memacclib (binary checked: same as the
 * source).  Poke and Peek are the only !ustatement/!ufunction entries of
 * BB2: the compiler passes the chosen type (1 byte, 2 word, 3 long,
 * 4 quick, 5 float, 7 string) in d0, the address in d1, Poke's data in d2.
 *
 * String arguments: d2 = the characters, the length is the compiler's
 * stack argument, read with MOVE.l -(a2),d0 (a2 = stack pointer before
 * the arguments were pushed, blitz2.s preplibst).  The original leaves a2
 * decremented; the compiler does not use a2 after the call (it removes
 * the arguments from a7), so the C version keeps a2.
 *
 * String results are written at a3 (a3 then points to the terminating 0,
 * blitz_string), d0 = length.
 */
#include "blitzlib.h"

/* entry points (thunks / asm, defined below) */
void dopeek(void), dopoke(void), docall(void);
ULONG dopeeks(ULONG loc BL_REG(d0), ULONG n BL_REG(d1));

/* !libheader{#memacclib,0,0,0,0} */
BLITZ_LIBHEADER(LIB_memacclib, 0, BL_NORET, 0, 0);

BLITZ_USTATEMENT();
BLITZ_ARGS(BL_LONG, 0);
BLITZ_LIBS0();
BLITZ_SUBS(dopoke, 0, 0);
BLITZ_NAME("Poke", "[.Type] Address,Data");

BLITZ_UFUNCTION();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(dopeek, 0, 0);
BLITZ_NAME("Peek", "[.Type](Address)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(dopeeks, 0, 0);
BLITZ_NAME("Peeks$", "(Address,Length)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(docall, 0, 0);
BLITZ_NAME("Call", "Address");

BLITZ_LIBFIN();

/* dopeek - d0 = type, d1 = address.  CMP #2,d0:BHI 'long:BEQ 'word: the
   type compare is a word compare; byte/word results replace only the low
   byte/word of d0 (MOVE.b (a0),d0 / MOVE (a0),d0), the rest of d0 is the
   type's upper bits (0 for the compiler's types). */
static ULONG BLITZ_ROUTINE dopeek_c(ULONG type BL_REG(d0), ULONG loc BL_REG(d1))
{
    UWORD t = (UWORD)type;

    if (t < 2)                                  /* byte (also type 0) */
        return (type & 0xffffff00) | *(const UBYTE *)loc;
    if (t == 2)                                 /* 'word */
        return (type & 0xffff0000) | *(const UWORD *)loc;
    if (t != 7)                                 /* 'long: long, quick, float */
        return *(const ULONG *)loc;
    {                                           /* 'string: copy up to the 0 */
        const UBYTE *s = (const UBYTE *)loc;
        ULONG n = (ULONG)-1;                    /* MOVEQ #-1,d0 */
        do
            n++;
        while ((*blitz_string++ = *s++) != 0);
        blitz_string--;                         /* SUBQ #1,a3 */
        return n;
    }
}
/* the original changes only d0, a0 (and a3 for strings) */
BLITZ_THUNK(dopeek, dopeek_c, "d1/a1");

/* dopeeks - d0 = address, d1 = number of characters: copied to a3 without
   a terminating 0 (as in the original, not a bug, #213: the compiler uses d0 =
   length, and where it passes a string on as an address it appends
   clr.b (a3)+ itself, blitz2.s:8767 sbtolong); d1 = 0 copies nothing;
   d0 = d1.  BUG(orig) (#214): the count is unsigned 32 bit
   (SUBQ.l #1,d1:BNE), a negative length copies ~4 GB. */
ULONG BLITZ_ROUTINE dopeeks(ULONG loc BL_REG(d0), ULONG n BL_REG(d1))
{
    const UBYTE *s = (const UBYTE *)loc;
    ULONG len = n;

    while (n) {
        *blitz_string++ = *s++;
        n--;
    }
    return len;
}

/* dopoke - d0 = type, d1 = address, d2 = data (string: the characters,
   length at -(a2)).  Poke.s copies the length's bytes and writes a 0
   behind them. */
static void BLITZ_ROUTINE dopoke_c(ULONG type BL_REG(d0), ULONG loc BL_REG(d1),
                                   ULONG data BL_REG(d2), const ULONG *lens BL_REG(a2))
{
    UWORD t = (UWORD)type;

    if (t < 2)
        *(UBYTE *)loc = (UBYTE)data;
    else if (t == 2)
        *(UWORD *)loc = (UWORD)data;
    else if (t != 7)
        *(ULONG *)loc = data;
    else {
        const UBYTE *s = (const UBYTE *)data;
        UBYTE *d = (UBYTE *)loc;
        ULONG n = lens[-1];                     /* MOVE.l -(a2),d0 */
        while (n) {
            *d++ = *s++;
            n--;
        }
        *d = 0;                                 /* CLR.b (a0) */
    }
}
/* the original changes a0 (strings: also d0, a1, a2) */
BLITZ_THUNK(dopoke, dopoke_c, "d0-d1/a1");

/* docall - MOVE.l d0,a0:JMP (a0).  Assembler by nature: the called code
   gets every register unchanged (a0 = d0 = address) and returns directly
   to the Blitz program; a C call would have to assume that it keeps
   d2-d7/a2-a6, which user code need not do. */
BLITZ_ASM_ROUTINE(docall,
    "   move.l  d0,a0\n"
    "   jmp     (a0)");
