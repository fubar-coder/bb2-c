/*
 * tokcode.h - the numbers and codes of the compiler's own tokens, derived
 * from the token list (toklist.h), so that a name cannot drift from it.
 *
 * In the tokenised source a token is a word with bit 15 set (parse.h):
 * its code, 0x8000 + its number (toklist.h `num`); the compiler's own
 * tokens have a number below 0x100, a library's token has the library in
 * bits 8-14.  The names:
 *
 *   TOK_NAME           the code of the token NAME: blitz2's name in upper
 *                      case (TOK_GOSUB, TOK_MOD, TOK_RUNERRSOFF),
 *                      AmiBlitz3's for the two `Jimi` slots (TOK_SYNTAX,
 *                      TOK_EXTB); AmiBlitz3's tokens behind blitz2's last
 *                      one too (TOK_OPTIMIZE)
 *   TOK_ASM_MNEMONIC   the code of an asm mnemonic token (TOK_ASM_MOVE); an
 *                      expression's operators AND, OR, EOR, NOT, LSL, LSR,
 *                      ASL and ASR are these tokens (TOK_ASM_AND ...)
 *   TOKNUM(code)       the number of a code (bit 15 cleared)
 *   TOKAT(p, code)     the two bytes at p hold the code
 *
 * The numbers (asm: lnum, fnum, tnum): 1..lnum-1 the first built-in
 * block, fnum..tnum-1 the asm mnemonics (118 and EndAsm), tnum..tnum+32
 * the second built-in block (tnum+3, blitz2.s's "Ern", is left out),
 * tnum+33..tnum+40 AmiBlitz3's tokens.  toklist.h numbers its entries
 * with fnum and tnum; the checks below keep them at the ends of the
 * blocks.
 */
#ifndef BLITZ2_TOKCODE_H
#define BLITZ2_TOKCODE_H

#define lnum 60                 /* behind the first built-in block */
#define fnum 60                 /* the first asm mnemonic (ABCD) */
#define tnum 179                /* behind the asm mnemonics (EndAsm + 1) */

#define TOKNUM(code)    ((code) & 0x7fff)
#define TOKAT(p, code)  ((p)[0] == (UBYTE)((code) >> 8) && (p)[1] == (UBYTE)(code))

#define TK(id, next, num, sym, name, help)  TOK_##sym = 0x8000 + (num),
#define TA(id, next, num, sym, name, ...)   TOK_ASM_##sym = 0x8000 + (num),
#define TKLAST(id, num, sym, name, help)    TOK_##sym = 0x8000 + (num),
#define TK3(id, next, num, sym, name, help) TOK_##sym = 0x8000 + (num),
#define TK3LAST(id, num, sym, name, help)   TOK_##sym = 0x8000 + (num),
#define M(b2, b3) b2

enum tokcode {
#include "toklist.h"
    /* the number without a node: blitz2.s's "Ern", which the function
       evaluator still takes (eval.c function) */
    TOK_ERN = 0x8000 + tnum + 3,
};

#undef TK
#undef TA
#undef TKLAST
#undef TK3
#undef TK3LAST
#undef M

static_assert(TOKNUM(TOK_EXTB) + 1 == lnum, "lnum: behind the first built-in block");
static_assert(lnum == fnum, "the asm mnemonics follow the first built-in block");
static_assert(TOKNUM(TOK_ASM_ABCD) == fnum, "fnum: the first asm mnemonic");
static_assert(TOKNUM(TOK_ASM_UNLK) + 1 == TOKNUM(TOK_ENDASM), "EndAsm: behind the asm mnemonics");
static_assert(TOKNUM(TOK_ENDASM) + 1 == tnum, "tnum: behind EndAsm");
static_assert(TOK_CLRERR + 1 == TOK_ERN && TOK_ERN + 1 == TOK_ERRFAIL, "Ern: the gap between ClrErr and ErrFail");
static_assert(TOKNUM(TOK_RUNERRSOFF) == tnum + 32, "the second built-in block: tnum..tnum+32");
static_assert(TOKNUM(TOK_OPTIMIZE) < 0x100, "the compiler's own tokens: numbers below 0x100");
/* the operators' codes as the original's listing has them (asm:
   (opX-opabcd)/$1c+$8000+fnum, build/blitz2.lst 9728) */
static_assert(TOK_ASM_AND == 0x8042 && TOK_ASM_OR == 0x808a && TOK_ASM_EOR == 0x8073 &&
              TOK_ASM_LSL == 0x807c && TOK_ASM_ASL == 0x8044 && TOK_ASM_LSR == 0x807d &&
              TOK_ASM_NOT == 0x8089, "the operators' codes of blitz2.s");

#endif
