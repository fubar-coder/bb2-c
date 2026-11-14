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
 *
 * Fixed (intended deviation, #214): Peeks$ with a negative length counted
 * it unsigned and copied up to 4 GB into the string buffer; here it gives
 * the empty string.  A negative length is a programming error: with
 * runtime error checking, peekschk (error check area only, the original
 * had none) raises "Can't Peeks$ a negative number of Characters!".
 */
#include "blitzlib.h"

/* entry points (thunks / asm, defined below) */
void dopeek(void), dopoke(void), memacclib_docall(void);
void dopeeks(void), peekschk(void);

/* !libheader{#memacclib,0,0,0,errors}: the original has no error check
   area (0); peekschk (#214) is its first check */
BLITZ_LIBHEADER(LIB_memacclib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

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
BLITZ_SUBS(dopeeks, peekschk, 0);
BLITZ_NAME("Peeks$", "(Address,Length)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(memacclib_docall, 0, 0);
BLITZ_NAME("Call", "Address");

BLITZ_LIBFIN();

/**
 * @brief Peek [.Type](Address) - read a byte, word, long, quick, float or
 * string from memory.
 *
 * CMP #2,d0:BHI 'long:BEQ 'word: the type compare is a word compare (type
 * 0 reads a byte too); byte/word results replace only the low byte/word
 * of d0 (MOVE.b (a0),d0 / MOVE (a0),d0), the rest of d0 is the type's
 * upper bits (0 for the compiler's types).  A string is copied to a3 up
 * to and including its 0; a3 then points to the 0.  The original changes
 * only d0, a0 (and a3 for strings).
 *
 * @param type d0: the type the compiler chose (1 byte, 2 word, 3 long,
 *        4 quick, 5 float, 7 string).
 * @param loc d1: the address.
 * @return d0: the value; for a string its length.
 */
static ULONG BLITZ_ROUTINE dopeek_c(ULONG type BL_REG(d0), ULONG loc BL_REG(d1))
{
    UWORD type_word = (UWORD)type;

    if (type_word < 2)                          /* byte (also type 0) */
        return (type & 0xffffff00) | *(const UBYTE *)loc;
    if (type_word == 2)                         /* 'word */
        return (type & 0xffff0000) | *(const UWORD *)loc;
    if (type_word != 7)                         /* 'long: long, quick, float */
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

/**
 * @brief Peeks$(Address,Length) - a string of Length characters from
 * memory.
 *
 * The characters are copied to a3 without a terminating 0 (as in the
 * original, not a bug, #213: the compiler uses d0 = length, and where it
 * passes a string on as an address it appends clr.b (a3)+ itself,
 * blitz2.s:8767 sbtolong); a length of 0 copies nothing.
 *
 * Fixed (#214): a negative length gives the empty string, as 0 does (the
 * original counted it unsigned, SUBQ.l #1,d1:BNE, and copied ~4 GB).
 * Errors (debugger, peekschk): "Can't Peeks$ a negative number of
 * Characters!" (#214: the original had no check).
 *
 * The original changes only d0, d1, a0 (and a3); the thunk keeps a1.
 *
 * @param loc d0: the address.
 * @param n d1: the number of characters.
 * @return d0: the length (= d1, 0 for a negative d1).
 */
static ULONG BLITZ_ROUTINE dopeeks_c(ULONG loc BL_REG(d0), ULONG n BL_REG(d1))
{
    const UBYTE *s = (const UBYTE *)loc;
    ULONG len;

    if ((LONG)n < 0)                            /* fixed (#214): no 4 GB copy */
        n = 0;
    len = n;

    while (n) {
        *blitz_string++ = *s++;
        n--;
    }
    return len;
}
BLITZ_THUNK(dopeeks, dopeeks_c, "a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(peeksmess, "Can't Peeks$ a negative number of Characters!");

/* peekschk - not in the original (Peeks$ had no check, #214): a negative
   length */
static void BLITZ_ERRCHK peekschk_c(ULONG n BL_REG(d1))
{
    if ((LONG)n < 0)
        BLITZ_RUNERR(peeksmess);
}
BLITZ_ERRCHK_THUNK(peekschk, peekschk_c, "d0-d1/a0-a1");

/**
 * @brief Poke [.Type] Address,Data - write a byte, word, long, quick,
 * float or string to memory.
 *
 * The type compare is a word compare as in Peek.  Poke.s copies the
 * length's bytes and writes a 0 behind them.  The original changes a0
 * (strings: also d0, a1, a2); C keeps a2 (the compiler does not use it
 * after the call).
 *
 * @param type d0: the type the compiler chose (1 byte, 2 word, 3 long,
 *        4 quick, 5 float, 7 string).
 * @param loc d1: the address.
 * @param data d2: the value; for a string the characters.
 * @param lens a2: the stack pointer before the arguments; a string's
 *        length is the long at -4(a2) (MOVE.l -(a2),d0).
 */
static void BLITZ_ROUTINE dopoke_c(ULONG type BL_REG(d0), ULONG loc BL_REG(d1),
                                   ULONG data BL_REG(d2), const ULONG *lens BL_REG(a2))
{
    UWORD type_word = (UWORD)type;

    if (type_word < 2)
        *(UBYTE *)loc = (UBYTE)data;
    else if (type_word == 2)
        *(UWORD *)loc = (UWORD)data;
    else if (type_word != 7)
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

/**
 * @brief Call Address - jump to machine code.
 *
 * MOVE.l d0,a0:JMP (a0).  Assembler by nature: the called code gets every
 * register unchanged (a0 = d0 = address) and returns directly to the
 * Blitz program; a C call would have to assume that it keeps
 * d2-d7/a2-a6, which user code need not do.
 *
 * @arg d0 the address of the code.
 */
BLITZ_ASM_ROUTINE(memacclib_docall,
    "   move.l  d0,a0\n"
    "   jmp     (a0)");
