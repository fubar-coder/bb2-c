/*
 * exprtype.h - the type decisions of an expression (issue #1180, step 1 of
 * the expression tree: OpenSpec change expr-tree-1-typing): for a binary
 * operator the conversions of its operands, the type whose handler
 * computes it and the type so far after it; for an element (a variable,
 * a function value, a constant) the type so far after it.  The evaluator
 * (eval.c: eval5, doop, compareop, mixedop, cmpownleft, varcont;
 * libcall.c: consttype2) asks these functions and emits what they return;
 * the rules are described in docs/blitz2/expression-types.md, their
 * mapping to the original's routines in docs/blitz2/expression-tree.md.
 *
 * Pure C99 without the compiler's globals or AmigaOS headers: blitz2 and
 * blitz2-ab3 compile it, and the host test
 * (programs/blitz2/tests/host/test_exprtype.cpp) compiles it with the
 * native compiler and pins every decision by a table (exprtype.tsv).
 */
#ifndef EXPRTYPE_H
#define EXPRTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The types, as T_* of structs.h (eval.c checks that they agree):
 *         0 none (the type so far not set yet), then Byte to String. */
enum {
    ET_NONE = 0, ET_BYTE = 1, ET_WORD = 2, ET_LONG = 3, ET_QUICK = 4,
    ET_FLOAT = 5, ET_ADDR = 6, ET_STRING = 7, ET_NTYPES = 8
};

/** @brief The operators: the index of their entry in operators[] (eval.c,
 *         blitz2.s:14704), ETO_END for none (the end of the expression, the
 *         sentinel of the operator stack). */
enum exprtype_op {
    ETO_ADD, ETO_SUB, ETO_MUL, ETO_DIV,
    ETO_AMP,                    /* '&': AND */
    ETO_BAR,                    /* '|': OR */
    ETO_AND, ETO_OR, ETO_EOR,   /* the tokens AND, OR, EOR */
    ETO_LSL, ETO_ASL, ETO_LSR, ETO_ASR,
    ETO_MOD, ETO_POW,
    ETO_EQ, ETO_NE, ETO_LT, ETO_LE, ETO_GT, ETO_GE,
    ETO_BITTST, ETO_BITSET, ETO_BITCLR, ETO_BITCHG,
    ETO_COUNT,                  /* the number of entries of operators[] */
    ETO_END = ETO_COUNT
};

/** @brief How an operation is emitted (exprtype_bin.path). */
enum {
    ETP_PLAIN,                  /* the left operand converted to the right one's type, its handler */
    ETP_COMPARE,                /* a comparison: the lower operand up, the common type's handler */
    ETP_CMPLQ,                  /* a comparison of a Long with a Quick, exactly (cmplq, #1177) */
    ETP_MIXED                   /* a raw Quick under a Long (mixedop, #1178) */
};

/** @brief Which constant of an equality comparison is sign-extended as a
 *         byte (#978; exprtype_bin.byteext). */
enum { ETB_NONE, ETB_RIGHT, ETB_LEFT };

/** @brief The facts of the operands of a binary operator that are no
 *         types (each 0 or 1). */
struct exprtype_binfacts {
    unsigned char lraw;         /* #1178: the left operand is a raw Quick under a Long type so far */
    unsigned char rraw;         /* #1178: the right operand is one */
    unsigned char rcmp;         /* #1182: the right operand was lowered by a comparison */
    unsigned char words;        /* #1179: a later array index, both operands Byte or Word values */
    unsigned char lbyte;        /* #978: the left operand is one byte value */
    unsigned char lconst;       /* #978: the left operand is one constant from 128 to 255 */
    unsigned char rbyte;        /* #978: the right operand is one byte value */
    unsigned char rconst;       /* #978: the right operand is one constant from 128 to 255 */
};

/** @brief The decision for a binary operator.  A conversion is from -> to
 *         (from 0: none); the emitter does them in the order of its path
 *         (eval.c, eval5). */
struct exprtype_bin {
    unsigned char path;         /* ETP_* */
    unsigned char rup_from, rup_to;     /* the right operand up to the left one's type (#1182; a comparison) */
    unsigned char lcut, rcut;   /* a raw Quick operand rounded down first */
    unsigned char lconv_from, lconv_to; /* the left operand (convtypef) */
    unsigned char rconv_from, rconv_to; /* the right operand after it (^ of a raw Quick) */
    unsigned char lq, rq;       /* ETP_MIXED: the left operand a Quick (raw or not), the right one raw */
    unsigned char handler;      /* the type whose handler computes (0: none, inline code) */
    unsigned char result;       /* the type so far after the operation; 0: "Illegal Operator for Type" */
    unsigned char cmpres;       /* #1182: the result is lowered by a comparison */
    unsigned char wordop;       /* #1179: the handler computes two Word values inline */
    unsigned char byteext;      /* #978: ETB_* */
};

/** @brief The facts of a constant (consttype2) beside its size. */
struct exprtype_constfacts {
    unsigned char point;        /* a literal with a point or an exponent (numfloat) */
    unsigned char fix;          /* point: its whole part fits a Long (SPFix without overflow) */
    unsigned char whole;        /* point and fix: the value is that whole number */
    unsigned char data;         /* a Data value (#1188) */
    unsigned char qfit;         /* point and fix: the value as a Quick fits (.quick2) */
    unsigned char size;         /* ET_BYTE, ET_WORD, ET_LONG: the size of the whole number (point: of its whole part) */
};

/** @brief The decision for an element: the type so far after it. */
struct exprtype_elem {
    unsigned char conv_from, conv_to;   /* the value converted (from 0: none) */
    unsigned char result;       /* the type so far; 0: "Type Mismatch" (a constant) */
    unsigned char raw;          /* #1178: a Quick held raw under a Long type so far */
    unsigned char ext;          /* a Byte value extended to a Word (ext.w) */
    unsigned char wordval;      /* #1179: a Byte or Word value */
    unsigned char litcut;       /* #1186: a literal with a point made a whole number */
};

/** @brief The type the handler of op gives for operands of type t
 *         (operators[], the handlers' "now a word" and so on); 0: op has
 *         no handler for t. */
unsigned exprtype_optype(unsigned op, unsigned t);

/** @brief op is a comparison (= <> < <= > >=). */
int exprtype_iscompare(unsigned op);

/** @brief A raw Quick whose consumer is op (ETO_END: none) is rounded
 *         down at once, as the original's cut at fetch gave it (#1178):
 *         a shift, a logic or bit operator, Mod. */
int exprtype_cutsraw(unsigned op);

/** @brief The left operand before the right one is read. */
struct exprtype_left {
    unsigned char left;         /* the left operand's type */
    unsigned char lraw;         /* it stays a raw Quick */
    unsigned char ownleft;      /* a comparison: cmpownleft may give it its own type back (#1177) */
    unsigned char compare;      /* a comparison: the right operand starts at type 0 (#1177) */
};
struct exprtype_left exprtype_leftop(unsigned op, unsigned left, int lraw);

/** @brief cmpownleft drops the conversion from -> to of a one-element
 *         left operand of a comparison (#1177): one that changes how the
 *         value is held, a whole number to Quick or Float, a Quick to
 *         Float. */
int exprtype_ownleft(unsigned from, unsigned to);

/** @brief The binary operator op on a left operand of type left (the type
 *         so far after it) and a right one whose evaluation ended with the
 *         type so far right. */
struct exprtype_bin exprtype_binop(unsigned op, unsigned left, unsigned right, const struct exprtype_binfacts *f);

/** @brief An element of its own type own under the type so far tsf; c is
 *         NULL for a value (varcont), else the facts of a constant
 *         (consttype2; own is then ignored). */
struct exprtype_elem exprtype_element(unsigned tsf, unsigned own, const struct exprtype_constfacts *c);

#ifdef __cplusplus
}
#endif

#endif
