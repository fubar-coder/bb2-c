/*
 * exprfold.h - the constant folding of a whole-number or Quick operation
 * in C (OpenSpec change expr-tree-5-operand-typing, task 3.6, design
 * decision 8): what the code eval.c emits for an operator on two
 * constants computes, without running that code.  The operations are
 * exprarith.c's (today's routines: add.w, muls.w, DIVS.W, ldivide,
 * domodword, qmul, qdivide, mixmult, ...); this file adds what lies around
 * them in the emitted code: the conversions of the type table
 * (convtable), the inline handlers (AND, OR, EOR, the shifts, the bit
 * operators, the comparisons, Not, the minus sign) and which bits of a
 * register they read and write.
 *
 * A register is modelled as the 68000 holds it, 32 bits of which the
 * low and the high word are each known or not: a move.w sets the low
 * word only, a Byte or Word handler writes only the low word, a Long one
 * all 32 bits.  An operation that needs a word that is not known, a Float
 * (the FFP routines), a string or the CPU's division trap is not folded
 * here (the functions return 0): eval.c then runs the emitted code, as
 * before.
 *
 * Pure C99 without the compiler's globals or AmigaOS headers: blitz2 and
 * blitz2-ab3 compile it, and the host test
 * (programs/blitz2/tests/host/test_exprfold.cpp) compiles it with the
 * native compiler.
 */
#ifndef EXPRFOLD_H
#define EXPRFOLD_H

#include <stdint.h>

#include "exprtype.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Which words of a register are known (exprfold_reg.known). */
enum { EXF_LO = 1, EXF_HI = 2, EXF_ALL = 3 };

/** @brief A data register: its 32 bits, the words of it that are known. */
struct exprfold_reg {
    uint32_t v;
    unsigned char known;
};

/** @brief move.l #v,Dn and move.w #v,Dn. */
void exprfold_setl(struct exprfold_reg *r, uint32_t v);
void exprfold_setw(struct exprfold_reg *r, uint16_t v);

/** @brief move.w (size 2) or move.l (size 4) of from to to. */
void exprfold_move(struct exprfold_reg *to, const struct exprfold_reg *from, unsigned size);

/** @brief n minus signs on a value of type t (negemit: neg.w below a
 *         Long, else neg.l); 0 for a Float or a word not known. */
int exprfold_neg(struct exprfold_reg *r, unsigned t, unsigned n);

/** @brief Not of a value of type t (notemit: not.w below a Long, else
 *         not.l, then clr.w for a Quick, #1209); 0 as exprfold_neg. */
int exprfold_not(struct exprfold_reg *r, unsigned t);

/** @brief The conversion from -> to of the type table (eval.c convtable)
 *         without its overflow check; 0 for one with a Float or a string,
 *         or a word not known. */
int exprfold_conv(struct exprfold_reg *r, unsigned from, unsigned to);

/** @brief The conversions the operator op with the decision d (of
 *         exprtype_binop; left: the left operand's type) writes before the
 *         check of a division by zero, in eval.c's order (operate,
 *         compareop, mixedop): l is D(regat), r D(regat+1).  Returns 0
 *         when they are not done here. */
int exprfold_prepare(unsigned op, const struct exprtype_bin *d, struct exprfold_reg *l, struct exprfold_reg *r);

/** @brief The divisor of a / or Mod after exprfold_prepare is 0 as
 *         divzerochk takes it (the low word for a Byte or Word handler,
 *         the long for a Long or Quick one): 1 yes, 0 no, -1 not known
 *         here (a Float, a word not known). */
int exprfold_divzero(unsigned op, const struct exprtype_bin *d, const struct exprfold_reg *r);

/** @brief The whole operation op (exprfold_prepare, then the handler of
 *         d) on l (left, of type left) and r: the result in l, of the type
 *         d->result.  Returns 1, or 0 when it is not done here (a Float,
 *         a string, ^, a word not known, the CPU's division trap of a
 *         Quick Mod by a divisor below 1). */
int exprfold_binop(unsigned op, const struct exprtype_bin *d, unsigned left, struct exprfold_reg *l,
                   struct exprfold_reg *r);

/** @brief The operation op with the decision d is a routine of lmullib
 *         (longmult, quickmult), whose result on an overflow depends on the
 *         library loaded: the port's (#199 fixed), the CD's or AmiBlitz3's
 *         (the original's).  eval.c runs its code instead of folding it
 *         here, so that a fold gives what that library gives. */
int exprfold_libdep(unsigned op, const struct exprtype_bin *d);

/** @brief The result of a fold as constfold writes it: the low word for a
 *         type below a Long (move.w), else the long (move.l).  Returns 0
 *         when those bits are not known. */
int exprfold_value(const struct exprfold_reg *r, unsigned t, uint32_t *value);

#ifdef __cplusplus
}
#endif

#endif
