/*
 * exprtyping.h - the typing pass of rule P+ (OpenSpec change
 * expr-tree-5-operand-typing, design decisions 1 and 6-10; issues #1216,
 * #1183): a pure function over the finished tree of an expression that
 * decides, for every operation, the format it computes in (a whole number
 * of its type, 16.16, 48.16 or Float), the conversions of its two
 * operands, the path of its code and, for the expression, where its value
 * is rounded once (at the consumer).  The flow typing at build time stays
 * for the errors (exprtype.c); this pass is the second one over the tree
 * (design decision 1), its decisions are what the emission follows.
 *
 * The rule, part by part (design decision 6; the parts are the flags
 * EM_R_P* of the matrix model, tools/lib/exprmatrix/exprmatrix.h):
 *
 * - An expression with a Quick variable and no Float (pfloat below) is
 *   computed in a fractional format: 48.16 where a Long is wanted or takes
 *   part (a Long variable or function, a whole constant beyond a Word, a
 *   whole-number subtree of type Long; part 1), else 16.16 with today's
 *   code (qmul, qdivide); under a wanted Byte, Word or Quick an operation
 *   with a Long in it is 48.16, and every operation below a 48.16 one is
 *   48.16 too.  A whole-number subtree in such an expression is computed
 *   in the fractional format when it is an arithmetic operation, unless a
 *   Long is wanted (part 7); a comparison compares its two sides exactly,
 *   each in its own format (part 8).
 * - The value is rounded once, where it is taken: a wanted Long, Word or
 *   Byte its whole part rounded down, a / at the top under a wanted Long
 *   towards zero (part 3); a wanted Quick the 16.16 value; If and Print a
 *   Quick when it fits, else the whole part (rounded down) as a Long.
 * - A constant with a point is fractional, never whole (part 2): a Float
 *   beside whole numbers only or under a wanted Long or Float, a Quick
 *   where a Quick is in its expression or wanted; as an operand of AND,
 *   OR, EOR and NOT its whole part rounded down; as a shift count it is
 *   refused; as the shifted operand a Quick.
 * - An expression without a Quick, or with a Float, is typed as today but
 *   with every operand typed by its own operands (operand typing: the
 *   type of an operation never depends on the order of its operands); a
 *   whole-number operation in a comparison is at least a Long; a Quick
 *   subtree beside a Float is typed by the same rule and taken as a Float
 *   (parts 5-8).
 *
 * The pass reads the tree (struct exprtyping_node: the operands, the
 * operators, the variables' types and the constants' values) and writes
 * its decisions into the nodes (the out part) and the result.  It has no
 * state of its own, allocates nothing and calls nothing but itself, so
 * blitz2 and the host test (programs/blitz2/tests/host/test_exprtyping.cpp,
 * which checks every decision against the matrix model's EM_RULES_DECIDED)
 * compile the same file.  The recursion is as deep as the tree; blitz2's
 * trees are bounded by the precedence stack (exprtree.h, EXPRTREE_PENDING)
 * along a nesting, and along a chain of one precedence the left operand is
 * the deeper one (the tree of l+a+b+... is a chain of left operands).
 *
 * Pure C99 without the compiler's globals or AmigaOS headers.
 */
#ifndef EXPRTYPING_H
#define EXPRTYPING_H

#include <stdint.h>

#include "exprtype.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The kinds of node. */
enum {
    ETN_VAR = 1,                /* a variable or a function value: type its type */
    ETN_CONST,                  /* a literal or a constant: num/den, point */
    ETN_BIN,                    /* a binary operator: op (ETO_*), l, r */
    ETN_NEG,                    /* a minus sign before an element: l */
    ETN_NOT                     /* NOT and the rest of its expression: l */
};

/** @brief The format a value is held in (exprtyping_node.fmt). */
enum {
    ETF_TYPE = 0,               /* as its type is held today: a whole number in its size, a Quick
                                   16.16, a Float FFP */
    ETF_Q16,                    /* 16.16 in one register (D0) */
    ETF_Q48                     /* 48.16 in a register pair: D0 the low 32 bits, D1 the high ones */
};

/** @brief The path of an operation's code (exprtyping_op.path). */
enum {
    ETY_PLAIN = 'p',            /* both operands in las = ras, today's handler of that type */
    ETY_COMPARE = 'c',          /* a comparison in the common type las = ras, the lower operand up */
    ETY_EXACT = 'x',            /* a comparison of two values in their own formats, exactly (a Long
                                   with a Quick, or the two sides of a fractional comparison) */
    ETY_Q16 = '1',              /* a 16.16 operation (qmul, qdivide, an exact Mod) */
    ETY_Q48 = '3',              /* a 48.16 operation */
    ETY_MIXED = 'm'             /* a Quick and a whole number under a Long by the routines of #1178
                                   (mixmult, lqdiv, longdiv): lwide, rwide mark the Quick operand */
};

/** @brief An operand that is a constant (exprtyping_op.lfrom, rfrom):
 *         folded, it is no operand of its own type. */
#define ETY_CONST 0xff

/** @brief How the expression's value is taken (exprtyping_result.consumer). */
enum {
    ETC_TYPE = 0,               /* not computed in a fractional format: its type, converted to the
                                   wanted one as today */
    ETC_BOOL,                   /* a comparison: Word -1 or 0 */
    ETC_KEEP,                   /* the 16.16 value (a 48.16 one: its low 32 bits) as a Quick; no
                                   wanted type and a whole value at the top (a bit operator,
                                   shift or NOT): that value */
    ETC_FLOOR,                  /* the whole part, rounded down, as a Long */
    ETC_TRUNC,                  /* a / at the top under a wanted Long: towards zero, as a Long */
    ETC_FIT                     /* no wanted type, 48.16: a Quick when the value fits one, else
                                   its whole part rounded down as a Long (decided at run time) */
};

/** @brief The decision for a binary operator (the out part of a BIN).
 *
 * lfrom is the left operand's own type (ETY_CONST: a constant; ET_QUICK
 * for a fractional subtree), las the type it is taken in and lwide that
 * it is taken as 48.16; the same for the right operand.  result is the
 * type after the operation (ETY_Q16: ET_QUICK, ETY_Q48: ET_LONG, a
 * comparison ET_WORD). */
struct exprtyping_op {
    unsigned char path;         /* ETY_* */
    unsigned char lfrom, las, lwide;
    unsigned char rfrom, ras, rwide;
    unsigned char result;
    unsigned char fast;         /* ETY_Q48: both operands narrow (their high part the sign
                                   extension of the low part; the fast path decided now) */
};

/** @brief A node of the tree.
 *
 * The in part is the caller's: kind, op (BIN), type (VAR), the value of a
 * CONST as num/den (den > 0; a literal: its decimal digits, den a power of
 * ten) and point (a literal with a point or an exponent), l and r.  The
 * out part is the pass's: typed (the node was typed), t the type of its
 * value as its consumer takes it, fmt its format, own the type it is
 * computed in when that differs from t (a bit operator in a Float
 * comparison: computed whole, taken as a Float), and o for a BIN. */
struct exprtyping_node {
    /* in */
    unsigned char kind;         /* ETN_* */
    unsigned char op;           /* BIN: ETO_* */
    unsigned char type;         /* VAR: ET_BYTE .. ET_FLOAT */
    unsigned char point;        /* CONST: written with a point */
    int64_t num, den;           /* CONST: the value num/den */
    struct exprtyping_node *l, *r;
    /* out */
    unsigned char typed;
    unsigned char t, fmt, own;
    struct exprtyping_op o;
};

/** @brief The result of the pass. */
struct exprtyping_result {
    unsigned char refused;      /* the compiler refuses the expression ("Illegal Operator for Type") */
    unsigned char t;            /* the root's type before the conversion to the wanted type */
    unsigned char fmt;          /* the root's format (ETF_*) */
    unsigned char consumer;     /* ETC_* */
    unsigned char conv;         /* the wanted type t is converted to (0: none) */
};

/**
 * @brief Types the tree root under the wanted type want (ET_NONE: If and
 *        Print, else ET_BYTE .. ET_FLOAT: a destination or a parameter;
 *        an array index ET_LONG).
 *
 * Writes the out part of every node it types (the nodes of a folded
 * constant subtree are not typed: their constant is) and *res.  Returns 0,
 * or 1 when the expression is refused (res->refused).
 */
int exprtyping_type(struct exprtyping_node *root, unsigned want, struct exprtyping_result *res);

/** @brief The node is a constant subtree, folded into one constant (no
 *         operation of its own). */
int exprtyping_isconst(const struct exprtyping_node *n);

#ifdef __cplusplus
}
#endif

#endif
