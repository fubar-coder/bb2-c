/*
 * exprtree.h - the nodes of an expression (issue #1180, step 2 of the
 * expression tree: OpenSpec change expr-tree-2-operators): a node for
 * each binary operator, each bracket, each NOT, each run of minus signs
 * and the expression itself, and an opaque leaf for each operand (an
 * element, emitted when it is read, as before: the leaf records where
 * its code begins and ends).  The nodes live in a static arena that
 * prepstack resets for every statement; a node is released when its
 * operator has been emitted, so the arena holds only the window of the
 * pending operators and their left operands.
 *
 * exprtree_climb is the evaluator's walk (blitz2.s:8983 eval5's control):
 * it reads an operand, and while the next operator binds tighter than the
 * pending one it reads that operator's right operand (recursing once per
 * higher precedence, as eval5 did) and emits the operator; along a chain
 * of one precedence it iterates.  What an operand is and what an operator
 * emits are the hooks exprtree_operand to exprtree_emit (eval.c).  The pending
 * operators, brackets, NOTs and the expression are counted: the 65th is
 * "Precedence Stack Overflow" (exprtree_overflow), the limit of the
 * original's precedence stack.
 *
 * Pure C99 without the compiler's globals or AmigaOS headers: blitz2 and
 * blitz2-ab3 compile it, and the host test
 * (programs/blitz2/tests/host/test_exprtree.cpp) compiles it with the
 * native compiler.  Where the original's routines went:
 * docs/blitz2/expression-tree.md.
 */
#ifndef EXPRTREE_H
#define EXPRTREE_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The kinds of node. */
enum {
    EXN_EXPR = 1,               /* an expression (eval3, a function argument, an index...) */
    EXN_BRACKET,                /* ( ) or { } */
    EXN_NOT,                    /* NOT and the rest of its expression */
    EXN_NEG,                    /* a run of minus signs before an element (op: how many) */
    EXN_BINOP,                  /* a binary operator */
    EXN_LEAF,                   /* an operand: the code start..end of what it read */
    EXN_RESULT                  /* an operand that is the result of an operator now */
};

/** @brief The flags of a node (exprnode.flags). */
enum {
    EXF_RAW = 1,                /* LEAF, RESULT: a raw Quick under a Long type so far (#1178) */
    EXF_CMP = 2,                /* LEAF, RESULT: lowered by a comparison (#1182) */
    EXF_LBYTE = 4,              /* BINOP: the left operand is one byte value (#978) */
    EXF_LCONST = 8,             /* BINOP: the left operand is one constant from 128 to 255 (#978) */
    EXF_COMPARE = 16            /* BINOP: a comparison, the right operand starts at type 0 (#1177) */
};

/** @brief The limit of the counted nodes (the original's precstack: 64
 *         entries). */
#define EXPRTREE_PENDING 64

/** @brief The number of nodes of the arena.  The live nodes are at most
 *         the 64 counted ones (the precedence limit), the 65th, which is
 *         made before it is counted, and one leaf or one run of minus
 *         signs per level of the walk, of which there is one per counted
 *         node: 129, reached by 64 times "-(" ("Precedence Stack
 *         Overflow"; 63 times compile with 128 nodes live: the
 *         measurement of change expr-tree-2-operators, design.md); a
 *         margin to 136 (4352 bytes on the 68000). */
#define EXPRTREE_NODES 136

/** @brief A node.  BINOP: op the operator (exprtype.h's ETO_*), prec its
 *         precedence, left the operand before it (a LEAF or a RESULT),
 *         right the one after it (once read), start where the right
 *         operand's code begins, reg the register of the left operand,
 *         tbefore the left operand's type, tafter the result's.  LEAF:
 *         start..end the code it emitted (end 0: none to look at),
 *         tbefore its type.  The counted ones (EXPR, BRACKET, NOT, BINOP)
 *         are linked by down, the pending one below. */
struct exprnode {
    struct exprnode *down;
    struct exprnode *left, *right;
    unsigned long start, end;
    unsigned short prec;
    unsigned short op;
    unsigned short tbefore, tafter;
    unsigned char kind, reg, flags;
};

/** @brief The hooks of the walk, defined by the caller (eval.c; the host
 *         test its own); ctx is the caller's.  Functions, not pointers:
 *         the walk recurses once per level, and each value it keeps is
 *         part of every level's frame (#1080). */
/** reads an operand and the operator after it: its precedence (0: none),
 *  *op its ETO_* */
unsigned exprtree_operand(void *ctx, unsigned *op);
/** the operand just read is l (a new LEAF); consumer is the operator that
 *  will take it (the next one if it binds tighter than the pending one,
 *  else the pending one; ETO_END: none) */
void exprtree_left(void *ctx, struct exprnode *l, unsigned consumer);
/** no operator binds tighter than the pending one: the level ends with
 *  its left operand l */
void exprtree_done(void *ctx, struct exprnode *l);
/** the operator n (op, prec, left set) is next, before it is counted */
void exprtree_pre(void *ctx, struct exprnode *n);
/** n is counted, its right operand is read next */
void exprtree_enter(void *ctx, struct exprnode *n);
/** n's right operand is read (n->right): emit n (and set n->tafter) */
void exprtree_emit(void *ctx, struct exprnode *n);

/** @brief The caller's error functions (never return): the 65th counted
 *         node ("Precedence Stack Overflow"), and a full arena (not
 *         reached: see EXPRTREE_NODES). */
#ifdef __GNUC__
__attribute__((noreturn))
#endif
void exprtree_overflow(void);
#ifdef __GNUC__
__attribute__((noreturn))
#endif
void exprtree_full(void);

/** @brief Every node free, none pending (prepstack). */
void exprtree_reset(void);

/** @brief A new node of kind kind, cleared. */
struct exprnode *exprtree_new(unsigned kind);

/** @brief n counted (pending) with operator op and precedence prec; the
 *         65th calls exprtree_overflow. */
void exprtree_push(struct exprnode *n, unsigned op, unsigned prec);

/** @brief The pending node on top (NULL: none). */
struct exprnode *exprtree_top(void);

/** @brief n and every node after it free; the counted ones among them
 *         are no longer pending. */
void exprtree_release(struct exprnode *n);

/** @brief The node after n if it is in use, else NULL. */
struct exprnode *exprtree_after(struct exprnode *n);

/** @brief The number of nodes in use, of pending ones. */
unsigned exprtree_live(void);
unsigned exprtree_pending(void);

/** @brief The walk of one level (eval5): the precedence of the operator
 *         that ends it (0: none), *op that operator.  There must be a
 *         pending node (the expression's, or an operator's). */
unsigned exprtree_climb(void *ctx, unsigned *op);

#ifdef __cplusplus
}
#endif

#endif
