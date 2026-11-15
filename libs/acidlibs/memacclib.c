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

/* entry points (aliases / asm, defined below) */
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
 * The original's code (MOVE.l d1,a0:CMP #2,d0:BHI 'long:BEQ 'word ...):
 * the type compare is a word compare (type 0 reads a byte too); byte/word
 * results replace only the low byte/word of d0 (MOVE.b (a0),d0 / MOVE
 * (a0),d0), the rest of d0 is the type's upper bits (0 for the compiler's
 * types).  A string is copied to a3 up to and including its 0; a3 then
 * points to the 0.  Changes only d0, a0 (and a3 for strings).  Assembler
 * as in the original: Peek runs in the loops of programs that walk memory,
 * and the C function's compares and merges cost up to 30 cycles more.
 *
 * In:  d0 the type the compiler chose (1 byte, 2 word, 3 long, 4 quick,
 *      5 float, 7 string), d1 the address.
 * Out: d0 the value; for a string its length.
 */
BLITZ_ASM_ROUTINE(dopeek,
"   move.l  d1,a0\n"
"   cmp.w   #2,d0\n"
"   bhi.s   1f\n"
"   beq.s   0f\n"
"   move.b  (a0),d0\n"                 /* byte (also type 0) */
"   rts\n"
"0: move.w  (a0),d0\n"                 /* 'word */
"   rts\n"
"1: cmp.w   #7,d0\n"                   /* 'long: long, quick, float */
"   beq.s   2f\n"
"   move.l  (a0),d0\n"
"   rts\n"
"2: moveq   #-1,d0\n"                  /* 'string: copy up to the 0 */
"3: addq.l  #1,d0\n"
"   move.b  (a0)+,(a3)+\n"
"   bne.s   3b\n"
"   subq.l  #1,a3\n"
"   rts");

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
 * The original changes only d0, d1, a0 (and a3).
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
BLITZ_ALIAS(dopeeks, dopeeks_c);

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
 * The original's code (MOVE.l d1,a0:CMP #2,d0:BHI 'long:BEQ 'word ...),
 * with the type compares as SUBQ on d0.w (the same unsigned word
 * compares; 6 to 8 cycles fewer than CMP, d0 is no result of a
 * statement).  Poke.s copies the length's bytes and writes a 0 behind
 * them.  Changes d0 and a0 (strings: also a1).  The original takes the length with MOVE.l -(a2),d0 and
 * leaves a2 decremented; here MOVE.l -4(a2),d0 keeps a2 (the compiler
 * does not use it after the call).  Assembler as in the original (see
 * Peek).
 *
 * In:  d0 the type the compiler chose (1 byte, 2 word, 3 long, 4 quick,
 *      5 float, 7 string), d1 the address, d2 the value (for a string the
 *      characters), a2 the stack pointer before the arguments (a string's
 *      length is the long at -4(a2)).
 */
BLITZ_ASM_ROUTINE(dopoke,
"   move.l  d1,a0\n"
"   subq.w  #2,d0\n"                   /* the original's CMP #2,d0:BHI:BEQ */
"   bcs.s   1f\n"
"   beq.s   0f\n"
"   subq.w  #5,d0\n"                   /* 'long: CMP #7,d0:BEQ 'string */
"   beq.s   2f\n"
"   move.l  d2,(a0)\n"                 /* long, quick, float */
"   rts\n"
"1: move.b  d2,(a0)\n"                 /* byte (also type 0) */
"   rts\n"
"0: move.w  d2,(a0)\n"                 /* 'word */
"   rts\n"
"2: move.l  d2,a1\n"                   /* 'string */
"   move.l  -4(a2),d0\n"
"   beq.s   4f\n"
"3: move.b  (a1)+,(a0)+\n"
"   subq.l  #1,d0\n"
"   bne.s   3b\n"
"4: clr.b   (a0)\n"
"   rts");

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
