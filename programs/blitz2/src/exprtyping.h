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
 *   Byte its whole part rounded down, a / at the top under a wanted Byte,
 *   Word or Long towards zero (part 3, #1222); a wanted Quick the 16.16
 *   value; If and Print a Quick when it fits, else the whole part
 *   (rounded down) as a Long.
 * - A constant with a point is fractional, never whole (part 2): a Float
 *   beside whole numbers only or under a wanted Byte, Word, Long or Float
 *   (#1222), a Quick where a Quick is in its expression or wanted; as an
 *   operand of AND, OR, EOR and NOT its whole part rounded down; as a
 *   shift count it is refused; as the shifted operand a Quick.
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
 * state of its own, allocates nothing and calls nothing but itself and
 * memset, so blitz2 and the host test
 * (programs/blitz2/tests/host/test_exprtyping.cpp, which checks every
 * decision against the matrix model's EM_RULES_DECIDED and every class of
 * exprtyping_classify against its EM_RULES_TSR) compile the same file.  The recursion is as deep as the tree; blitz2's
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
    ETY_PLAIN = 'p',            /* both operands in las = ras, today's handler of that type; a
                                   Mod with the result Quick or Float is the exact Mod (an operand
                                   with a fraction) */
    ETY_COMPARE = 'c',          /* a comparison in the common type las = ras, the lower operand up */
    ETY_EXACT = 'x',            /* a comparison of two values in their own formats, exactly (a Long
                                   with a Quick, or the two sides of a fractional comparison) */
    ETY_Q16 = '1',              /* a 16.16 operation (qmul, qdivide, an exact Mod) */
    ETY_Q48 = '3',              /* a 48.16 operation */
    ETY_MIXED = 'm',            /* a Quick and a whole number under a Long by the routines of #1178
                                   (mixmult, lqdiv, longdiv): lwide, rwide mark the Quick operand */
    ETY_SHIFT = 's'             /* a shift by the classes' own code (task 4.4 of type-system-rework):
                                   of Quick class in 48.16 (the left operand in las, a 48.16 pair
                                   when lwide; result ET_LONG, ETF_Q48), else a whole LSR in the
                                   left operand's own width las (result las); the count in ras */
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
    ETC_TRUNC,                  /* a / at the top under a wanted Byte, Word or Long: towards zero, as a Long */
    ETC_FIT,                    /* no wanted type, 48.16: a Quick when the value fits one, else
                                   its whole part rounded down as a Long (decided at run time) */
    ETC_SAT,                    /* a wanted Quick, 48.16 (the classes, task 4.2 of
                                   type-system-rework): the Quick rounded down and saturated,
                                   $7FFFFFFF above and $80000000 below the Quick range */
    ETC_TOFLOAT                 /* a wanted Float, 48.16 (the classes): the Float of the value */
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
                                   extension of the low part; the fast path decided now);
                                   ETY_PLAIN: a Long / of two values in the Word range by the
                                   inline DIVS.W of #1179 (the whole-width rule) */
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
    unsigned char wide;         /* a NEG computed in t, wider than its operand (the whole-width
                                   rule: the operand converted to t first) */
    struct exprtyping_op o;
    /* out of exprtyping_classify */
    unsigned char cls;          /* ET_BYTE, ET_WORD, ET_LONG a whole number of that width,
                                   ET_QUICK 48.16, ET_FLOAT */
    unsigned char exc;          /* ETX_*: the exception that gives this node another type */
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
 *
 * When exprtyping_classify has classed the tree (the root's cls set, as
 * blitz2 does before, p5end), the whole-number operations follow the
 * whole-width rule of type-system-rework (task 4.1): a node of whole class
 * without an exception is computed in the operand typing's type only where
 * that code gives its class value for every operand value (under a
 * destination that wraps through + - *, the unary minus, NOT and the bit
 * operators; or within a range bound), else in the narrowest whole type
 * that does (the operands extended, the Long code; a minus sign wide, a
 * Long / of two Word-range values o.fast).  Mod keeps its type (task
 * 4.7).  The shifts (task 4.4) follow their classes: a whole LSR of a Byte
 * or Word class in its own width (ETY_SHIFT), the other whole shifts as a
 * Long, a shift of Quick class (a Quick left operand, the shift exception)
 * in 16.16 where only its low 32 bits reach the value (LSR, ASR of a
 * 16.16 operand, a left shift at the top under a Quick, Byte or Word
 * destination), else in 48.16 (ETY_SHIFT, ETF_Q48); a Quick count rounded
 * down.  Every operation of Quick class (+ - * / and the
 * unary minus; task 4.2) is computed in 48.16 (ETY_Q48, ETF_Q48) wherever
 * it is, its operands typed by their classes (a whole operand as a whole
 * number, not in a fractional format); the value is taken at the root by
 * the destination: a Quick saturated (ETC_SAT), a whole number rounded
 * down (ETC_FLOOR), a Float (ETC_TOFLOAT), none as ETC_FIT; inside the
 * operation with a Float as a Float, by a bit operator, shift count or
 * NOT as its whole part, a Long rounded down.  Where rule P+
 * reads a constant with a point in such an operation as a Float (part 2),
 * the operation stays rule P+'s Float one until the reading of constants
 * switches (task 3.3).  Without the classes (the host test of rule P+)
 * the pass is P+ alone.
 */
int exprtyping_type(struct exprtyping_node *root, unsigned want, struct exprtyping_result *res);

/** @brief The exceptions of the rules of type-system-rework that give a
 *         node another type or conversion than the standard rules
 *         (exprtyping_node.exc; each one's warning, spec compiler/warnings
 *         "The type exceptions warn", parse.h W_*). */
enum {
    ETX_DIVFRAC = 1,            /* the whole-division exception (C1): a whole / with its fraction
                                   under a Quick or Float destination */
    ETX_DIVQUICK = 2,           /* a whole / beside a Quick-class operand with its fraction (C3) */
    ETX_FLOATTOP = 4,           /* the top operation under a Float destination in Float (D) */
    ETX_SHIFTFRAC = 8,          /* the shift exception (E): a whole right shift with 16 fraction bits */
    ETX_SHIFTWRAP = 16          /* the left-shift wrap (J): the top left shift to a Quick wrapped */
};

/**
 * @brief The classes of the rules of the OpenSpec change
 *        type-system-rework under the destination want (ET_NONE: no
 *        destination, If and Print; else ET_BYTE .. ET_FLOAT, a typed
 *        parameter as its type, an array index ET_LONG).
 *
 * Every node is typed from its operator and its operands only (tasks 3.1,
 * 3.2): an operation on whole numbers one step wider than the wider
 * operand (Byte to Word, Word to Long, Long stays), the unary minus and NOT
 * of a whole operand one step wider than it, NOT of a Quick a Long; an
 * operation with a Quick and no Float 48.16; with a Float Float; ^ Float; a
 * comparison a Word; a bit operator whole; a shift the type of its left
 * operand (a whole LSL, ASL a Long).  A whole constant is its smallest
 * type; a constant with a point a Quick when it is exact in 16.16 or a
 * Quick is the other operand of its operator, else a Float.  The
 * destination reaches into the expression only by the named exceptions
 * (ETX_*).  Writes cls and exc of every node and the set of the
 * exceptions that apply in *warn; returns 0, or 1 when an operator is
 * refused ("Illegal Operator for Type"; *warn 0).  exprtyping_type takes
 * these classes for the whole-number operations (task 4.1); the code of
 * the other families still follows rule P+ (section 4 of the change).
 */
int exprtyping_classify(struct exprtyping_node *root, unsigned want, unsigned *warn);

/** @brief The node is a constant subtree, folded into one constant (no
 *         operation of its own). */
int exprtyping_isconst(const struct exprtyping_node *n);

/** @brief The exact value of a constant subtree as the rule folds it
 *         (the whole parts under AND, OR, EOR, Mod and the shifts, a whole
 *         Mod the floor Mod: -6 Mod 4 is 2, 7 Mod -3 is -2; a
 *         comparison -1 or 0, a division by 0 0) in *num / *den (den > 0).
 *
 * The exact value: where the rule keeps today's types (no fraction under a
 * bit operator), today's fold in those types keeps the register's width
 * (the low word of a type below Long), so it differs from it for a value
 * that does not fit the type; task 3.6 folds those as today.  Returns 0,
 * or 1 when the value is beyond the exact range of the pass (int64_t
 * numerator and denominator). */
int exprtyping_constvalue(const struct exprtyping_node *n, int64_t *num, int64_t *den);

#ifdef __cplusplus
}
#endif

#endif
