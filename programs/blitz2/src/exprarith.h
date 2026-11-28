/*
 * exprarith.h - the arithmetic of the formats of rule P+ (OpenSpec change
 * expr-tree-5-operand-typing, design decisions 6, 8 and 9; issues #1216,
 * #1183): whole numbers as today's code computes them, 16.16 as today's
 * routines (qmul, qdivide with its last bit), 48.16 on int64_t, the casts
 * between them with their overflow, and the routines of #1178 (mixmult,
 * qqmult, lqdiv, longdiv's division by 0).  Bit for bit what the 68k code
 * gives with the port's libraries, so that the compiler's constant
 * folding (task 3.6) and the emulator tests of the code blitz2 emits for
 * these operations (task 3.5) have one oracle; the host test
 * (programs/blitz2/tests/host/test_exprarith.cpp) checks it against the
 * matrix model's values (tools/lib/exprmatrix, EM_RULES_DECIDED and
 * today's rules).
 *
 * The values: a whole number as an int32_t of the register that holds it
 * (a Byte or a Word as its low word, sign-extended: the byte path
 * computes in words, and a conversion of a Byte to a Long takes that
 * word; shown as a Byte, a Byte is its low byte);
 * a 16.16 value as the int32_t of its bits (value * 65536); a 48.16 value
 * as the int64_t of its bits (D1:D0, value * 65536).  The operators are
 * exprtype.h's ETO_*: + - * / Mod.
 *
 * Pure C99 without the compiler's globals or AmigaOS headers: blitz2
 * (the compiler only, not the libraries) and the host test compile it.
 */
#ifndef EXPRARITH_H
#define EXPRARITH_H

#include <stdint.h>

#include "exprtype.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief What happened besides the value (the flags argument, ORed in). */
enum {
    EXA_TRAP = 1,               /* the CPU's division trap: a / or Mod by 0 (type-system-rework
                                   D1/D7); the value is then the dividend */
    EXA_DIV0 = 2,               /* ldivlib's division by 0, whose value is the largest number
                                   with the dividend's sign (#200); the compiled code tests the
                                   divisor before the call (EXA_TRAP with it) */
    EXA_OVF = 4                 /* the result does not fit its format: it wrapped */
};

/** @brief A whole-number operation as today's code computes it.
 *
 * t is the type of both operands (ET_BYTE, ET_WORD, ET_LONG); the result
 * is in the type the handler gives (exprtype_optype: a Byte + or * a
 * Word, a Word + or * a Long, else t): + and * of Bytes and Words as
 * add.w (a Word sum taken as a Long after ext.l, so 20000+20000 is
 * -25536) and muls.w, / as DIVS.W (an overflow leaves the dividend) and
 * Long's ldivide, Mod as domodword and domodlong (the floor Mod: the
 * divisor added to a remainder that is not 0 and of the other sign,
 * #1197), * of Longs modulo 2^32.  A division or Mod by 0 traps
 * (EXA_TRAP; a Long division by 0 also EXA_DIV0 with ldivide's value, the
 * largest number with the dividend's sign, #200). */
int32_t exprarith_whole(unsigned op, unsigned t, int32_t a, int32_t b, unsigned *flags);

/** @brief A 16.16 operation where P+ computes in 16.16, today's routines:
 *         + and - modulo 2^32, * as qmul (the product of the magnitudes,
 *         its low 16 bits cut, the sign applied), / as qdivide (48 steps:
 *         its last bit; a divisor 0 traps, EXA_TRAP and EXA_DIV0 with
 *         qdivide's value), Mod the exact floor Mod (the divisor added to
 *         a remainder that is not 0 and of the other sign, #1197; a
 *         divisor 0 traps). */
int32_t exprarith_q16(unsigned op, int32_t a, int32_t b, unsigned *flags);

/** @brief Today's Quick Mod (domodquick): the whole parts by DIVS.W, the
 *         result a Word (a divisor whose whole part is 0 traps). */
int32_t exprarith_qmod_today(int32_t a, int32_t b, unsigned *flags);

/** @brief A 48.16 operation: + and - modulo 2^64; * the exact product cut
 *         towards zero to 1/65536; / the exact quotient cut towards zero
 *         to 1/65536; Mod the exact floor Mod (#1197); a / or Mod by 0
 *         traps (EXA_TRAP, the dividend returned; type-system-rework
 *         D1/D7). */
int64_t exprarith_q48(unsigned op, int64_t a, int64_t b, unsigned *flags);

/** @brief exprarith_q48 with * and / cut down to 1/65536 instead of
 *         towards zero: a 48.16 product or quotient under a wanted Byte,
 *         Word or Long (the consumer ETC_FLOOR), whose value is rounded
 *         down once (#1221); the other operations as exprarith_q48. */
int64_t exprarith_q48_down(unsigned op, int64_t a, int64_t b, unsigned *flags);

/** @brief The fast path of a 48.16 operation: v is narrow, its high 32
 *         bits the sign extension of its low 32 bits (a 16.16 value). */
int exprarith_narrow(int64_t v);

/** @brief The casts.
 *
 * whole -> 48.16 and 16.16 -> 48.16 are exact; 48.16 -> 16.16 keeps the
 * low 32 bits; 48.16 -> a whole number rounds down (floor) or towards
 * zero (trunc) and keeps the bits of the type t (ET_BYTE, ET_WORD,
 * ET_LONG); 16.16 -> a whole number rounds down (the whole part, as
 * today's conversion); whole -> 16.16 shifts the value into the high
 * word.  EXA_OVF where the value does not fit. */
int64_t exprarith_whole_q48(int32_t v);
int64_t exprarith_q16_q48(int32_t v);
int32_t exprarith_q48_q16(int64_t v, unsigned *flags);
int32_t exprarith_q48_floor(int64_t v, unsigned t, unsigned *flags);
int32_t exprarith_q48_trunc(int64_t v, unsigned t, unsigned *flags);
int32_t exprarith_q16_floor(int32_t v, unsigned t, unsigned *flags);
int32_t exprarith_whole_q16(int32_t v, unsigned *flags);

/** @brief No wanted type (If, Print): a 48.16 value as a Quick when it
 *         fits one (*quick 1, the 16.16 bits), else its whole part rounded
 *         down as a Long (*quick 0). */
int32_t exprarith_q48_fit(int64_t v, int *quick, unsigned *flags);

/** @brief The conversions of OpenSpec change type-system-rework (task 4.3,
 *         compilerlib's q48sat, f2l and f2q).
 *
 * exprarith_q48_quick: a 48.16 value as a Quick, saturated: $7FFFFFFF
 * above the Quick range, $80000000 below it (design.md P2); the value is
 * already in 1/65536.
 *
 * The Float is Motorola FFP as mathffp keeps it: the mantissa in bits
 * 31-8 (bit 31 set unless the value is 0), the sign in bit 7, the
 * exponent in bits 6-0 (excess 64); the value is mantissa / 2^24 *
 * 2^(exponent - 64), 0 when the mantissa is 0.
 *
 * exprarith_ffp_whole: cut towards zero (as mathffp's SPFix in the Long
 * range), the bits of the type t kept (as keep: a Byte and a Word as the
 * low word, sign-extended); beyond the Long range the value cut towards
 * zero wraps to its low 32 bits (I8 A+C), where SPFix saturates.
 *
 * exprarith_ffp_quick: rounded down to 1/65536, saturated as
 * exprarith_q48_quick.  A mantissa with bit 31 clear (not mathffp's
 * form) gives the exact value here as well; compilerlib's f2q saturates
 * such a value when its exponent is 16 or more (excess 64: 80), where a
 * mantissa with bit 31 set is at least 32768.
 *
 * EXA_OVF where the value does not fit (it saturated or wrapped). */
int32_t exprarith_q48_quick(int64_t v, unsigned *flags);
int32_t exprarith_ffp_whole(uint32_t f, unsigned t, unsigned *flags);
int32_t exprarith_ffp_quick(uint32_t f, unsigned *flags);

/** @brief The routines of a Quick under a Long type so far (#1178,
 *         libs/compilerlib and ldivlib): a and b Longs or Quicks (aq, bq:
 *         the operand is a Quick), the result a Long: + - * rounded down
 *         (mixmult, qqmult), / towards zero (lqdiv, longdiv); a divisor 0
 *         the largest number with the dividend's sign (+-$7FFFFFFF; a
 *         Quick divided by a Long +-32767, longdiv's quotient taken as a
 *         Quick).  Results modulo 2^32. */
int32_t exprarith_mixed(unsigned op, int32_t a, int aq, int32_t b, int bq, unsigned *flags);

#ifdef __cplusplus
}
#endif

#endif
