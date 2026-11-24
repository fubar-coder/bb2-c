/*
 * exprtree.h - the nodes of an expression (issue #1180): a node for each
 * binary operator, each bracket, each NOT, each run of minus signs and the
 * expression itself, and a leaf for each operand.  The nodes live in a
 * static arena that prepstack resets for every statement; a node is
 * released when its operator has been emitted, so the arena holds only the
 * window of the pending operators and their left operands.
 *
 * An operand is a leaf that records where its code begins and ends.  A
 * simple element is parsed into a leaf of its own (op: its ELK_* kind),
 * which its emit part turns into code right after and which is released
 * then; a string literal is such a leaf as well (ELK_STRING), its text
 * written to the literal area while it is read.  A variable's path (an
 * array's indexes, fields, field arrays, pointers) is read by calcvar's
 * parse part (varparse, parse.c) into a node per step, each emitted and
 * released as soon as it is read, and a variable node at the end, which an
 * element makes its leaf; a string variable is a variable's leaf.  A
 * procedure, OS or library call is a node that lives while its parameters
 * are read, with an enter action emitted before them and an exit action
 * after them; it then becomes the leaf of the call's value (a string
 * result is a call's).  A library function's lookup has no side effect:
 * its library is used right after the lookup.  The operators on strings (+
 * and the comparisons) are operator nodes like the others: the string
 * protocol (the work area, a length on the stack per string) is kept by
 * emitting each of them at the original's moment.
 *
 * exprtree_climb is the evaluator's walk (blitz2.s:8983 eval5's control):
 * it reads an operand, and while the next operator binds tighter than the
 * pending one it reads that operator's right operand (recursing once per
 * higher precedence, as eval5 did) and emits the operator; along a chain
 * of one precedence it iterates.  What an operand is and what an operator
 * emits are the hooks exprtree_operand to exprtree_emit (eval.c).  The
 * pending operators, brackets, NOTs and the expression are counted: the
 * 65th is "Precedence Stack Overflow" (exprtree_overflow), the limit of
 * the original's precedence stack.
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
    EXN_LEAF,                   /* an operand: the code start..end of what it read;
                                   op: its element kind (ELK_*); 0: the operand of the walk,
                                   or the value of a call (a CALL made its leaf: callcont;
                                   a library function: varcont) */
    EXN_RESULT,                 /* an operand that is the result of an operator now */
    EXN_VAR,                    /* a variable with its path, read by calcvar's parse part
                                   (varparse, parse.c); an element's leaf in an expression */
    EXN_STEP,                   /* a step of a variable's path (op: its VST_* kind), emitted
                                   when it is read and released then */
    EXN_CALL                    /* a procedure, OS or library call (op: its CLK_* kind), live
                                   while its parameters are read; then the leaf of its value */
};

/** @brief The kinds of call (exprnode.op of a CALL).
 *
 * The call is looked up, its node made, its enter action emitted, its
 * parameters read (each an expression of its own, compiled and pushed as
 * it is read: the node's children), then its exit action emitted
 * (eval.c). */
enum {
    CLK_PROC = 1,               /* a Function in an expression, f{...} (afunction) */
    CLK_OS,                     /* an OS library function, Name_(...) (amigalib) */
    CLK_LIB                     /* a library function, Name(...) (libfunction) */
};

/** @brief The facts of a CALL (exprnode.lflags), set by its actions. */
enum {
    CLF_SB = 1,                 /* PROC: in a string expression: a3 pushed (dopusha3) */
    CLF_A45 = 2,                /* OS: a parameter in a4 or a5: a4/a5 saved (#958) */
    CLF_A3 = 4,                 /* LIB: a3 saved with the registers below regat (#46) */
    CLF_INLINE = 8              /* LIB: compiled inline (an intrinsic, eval.c intrinsics[]) */
};

/** @brief The steps of a variable's path (exprnode.op of a STEP).
 *
 * They are what calcvar's parse part read, in the order their code is
 * written.  The index of an INDEX and of a BRCLOSE (its subtree) is
 * compiled into D(regat) before the step: its code is aux..start. */
enum {
    VST_LIST = 1,               /* a List array's current item a(): value the array's offset */
    VST_INDEX,                  /* an array's index in D(regat): the first one (VSF_FIRST)
                                   scaled by value (the element size), a later one multiplied
                                   by the dimension at value (makelmul); off the dimension the
                                   check (VSF_CHECK) compares with */
    VST_ARRAY,                  /* the indexes done: the array's base (value its offset) plus
                                   the index into a2, a List array's item header skipped (VSF_LIST) */
    VST_PFIX,                   /* a pointer array's element followed: its pointer into a2 */
    VST_BASE,                   /* a NewType variable's address into a2: value its offset */
    VST_BROPEN,                 /* [ of a field array: off the offset so far added to a2, a2 saved */
    VST_BRCLOSE,                /* ] : a2 back, the index checked against value (VSF_CHECK) */
    VST_BRSCALE,                /* the field array's index scaled by value and added to a2 */
    VST_DEREF,                  /* a pointer field followed: its pointer at off(a2) into a2,
                                   a BCPL pointer (VSF_BCPL) times 4 */
    VST_END                     /* the path done: off the offset so far added to a2 */
};

/** @brief The facts of a STEP (exprnode.lflags; ELF_LOCAL as for a leaf). */
enum {
    VSF_CHECK = 1,              /* INDEX, LIST, BRCLOSE: runtime error checking's check */
    VSF_FIRST = 2,              /* INDEX: the first index */
    VSF_LIST = 4,               /* ARRAY: a List array */
    VSF_BCPL = 8                /* DEREF: a BCPL pointer (@) */
};

/** @brief The element kinds of a LEAF an element makes (exprnode.op).
 *
 * The element is parsed into its leaf, and the leaf emitted right after. */
enum {
    ELK_CONST = 1,              /* a number: a literal, #constant, SizeOf, On, Off, Maximum... */
    ELK_PI,                     /* Pi */
    ELK_VAR,                    /* the value of a variable or a field */
    ELK_ADDROF,                 /* &var */
    ELK_LABEL,                  /* ?label */
    ELK_USED,                   /* Used Object */
    ELK_ADDR,                   /* Addr Object(n) */
    ELK_ERRNUM,                 /* ErrNum ("Ern") */
    ELK_STRING                  /* a string literal "...": value its place in the literal
                                   area (data1), aux its length */
};

/** @brief The flags of a node (exprnode.flags). */
enum {
    EXF_RAW = 1,                /* LEAF, RESULT: a raw Quick under a Long type so far (#1178) */
    EXF_CMP = 2,                /* LEAF, RESULT: lowered by a comparison (#1182) */
    EXF_LBYTE = 4,              /* BINOP: the left operand is one byte value (#978) */
    EXF_LCONST = 8,             /* BINOP: the left operand is one constant from 128 to 255 (#978) */
    EXF_COMPARE = 16            /* BINOP: a comparison, the right operand starts at type 0 (#1177) */
};

/** @brief The facts of an element's LEAF (exprnode.lflags).
 *
 * They are what its parse part found for its emit part, and what the
 * emitted code is for the code after it. */
enum {
    ELF_POINT = 1,              /* CONST: a literal with a point or an exponent */
    ELF_CUT = 2,                /* CONST: such a literal made a whole number (#1186: litcut) */
    ELF_FOLD = 4,               /* CONST: a constant of the fold (forthstack's constgot 0) */
    ELF_BYTE = 8,               /* VAR: a Byte value, extended to a Word (#978: bytevalend) */
    ELF_RAW = 16,               /* VAR: a Quick held raw under a Long (#1178: rawq) */
    ELF_CONV = 32,              /* VAR: converted from own to tafter (#1177: conv) */
    ELF_WORD = 64,              /* VAR: a Byte or Word value (#1179: wordval) */
    ELF_REF = 128,              /* LABEL: the reference to the label is made at emit */
    ELF_LOCAL = 256             /* VAR, ADDROF, the node VAR and a STEP: a local variable
                                   (varmode: its a4 offset) */
};

/** @brief The limit of the counted nodes (the original's precstack: 64
 *         entries). */
#define EXPRTREE_PENDING 64

/** @brief The number of nodes of the arena (168, 7728 bytes on the 68000).
 *
 * The live nodes are at most the 64 counted ones (the precedence limit),
 * the 65th, which is made before it is counted, and one leaf or one run
 * of minus signs per level of the walk, of which there is one per counted
 * node: 129, reached by 64 times "-(" ("Precedence Stack Overflow"; 63
 * times compile with 128 nodes live).  An element's leaf lives only while
 * its element is emitted, which makes no node: behind the run of minus
 * signs of the innermost level it is the 129th (63 times "-(" and "-b"),
 * never with the 65th counted node.  A step of a variable's path and the
 * variable node take the leaf's place (made after the index, released
 * before the next node: 62 times "-(" and "-a(-c)" 129).
 *
 * A call's node lives beside the run of minus signs of its level while
 * its parameters are read; the compiler nests at most 32 calls (each
 * holds an entry of forthstack, the 33rd is "Expression too Complex"):
 * 129 + 32 = 161, reached by 32 times "-f{" and 32 times "-(" ("Precedence
 * Stack Overflow").  A library function's call is such a node as well and
 * counts among the 32 (10 times "-Abs(" and 21 or 22 times "-f{" before
 * 32 times "-(": 160 and 161).  The arena has a margin of 7 nodes to 168.
 * The measurements: docs/blitz2/expression-tree.md, "Node bound". */
#define EXPRTREE_NODES 168

/** @brief A node of an expression.
 *
 * BINOP: op the operator (exprtype.h's ETO_*), prec its
 *         precedence, left the operand before it (a LEAF or a RESULT),
 *         right the one after it (once read), start where the right
 *         operand's code begins, reg the register of the left operand,
 *         tbefore the left operand's type, tafter the result's.  LEAF of
 *         the walk (op 0): start..end the code it emitted (end 0: none to
 *         look at), tbefore its type.  LEAF of an element (op its ELK_*):
 *         tbefore the type so far it was read under, own its own type,
 *         tafter the type so far after it, lflags its facts (ELF_*),
 *         value, aux, vtype and off what its kind needs (eval.c,
 *         libcall.c; docs/blitz2/expression-tree.md), start..end the code
 *         its emit part wrote.  VAR (parse.c, varparse): value the
 *         variable or its last field (struct var *), aux its type
 *         (struct type *), vtype and off calcvar's type and offset,
 *         lflags ELF_LOCAL.  STEP (op its VST_*): value, off and lflags
 *         (VSF_*, ELF_LOCAL) what its kind needs, aux of an INDEX and a
 *         BRCLOSE where its index's code begins (it ends at start),
 *         start..end the code stepemit wrote.  CALL (op its CLK_*): tbefore
 *         the type so far it was read under, own the type of its value;
 *         a PROC's value the procedure (struct proc *), aux the movem mask
 *         its enter action saved (savem), reg regat before the
 *         parameters; an OS call's value the library function's entry
 *         (looktoke's sub), aux its token's library (struct blitzlib *)
 *         until the enter action, then the movem.l (a7)+ of the registers
 *         that action saved, off the offset of its library base, reg the
 *         number of parameters; a library function's value its entry
 *         (looktoke's sub), aux its library (struct blitzlib *), off its
 *         form (libform's, as an offset from value), vtype the movem mask
 *         its enter action saved, reg regat before the parameters, prec
 *         its intrinsic (eval.c intrinsics[]: the index + 1; 0 none);
 *         lflags its CLF_*.  The counted ones (EXPR,
 *         BRACKET, NOT, BINOP) are linked by down, the pending one
 *         below. */
struct exprnode {
    struct exprnode *down;
    struct exprnode *left, *right;
    unsigned long start, end;
    unsigned long value, aux;
    unsigned short prec;
    unsigned short op;
    unsigned short tbefore, tafter;
    unsigned short vtype;
    short off;
    unsigned char kind, reg, flags, own;
    unsigned short lflags;
};

/** @brief The hooks of the walk, defined by the caller (eval.c; the host
 *         test its own).
 *
 * ctx is the caller's.  They are functions, not pointers: the walk
 * recurses once per level, and each value it keeps is part of every
 * level's frame (#1080). */
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

/** @brief The caller's error functions, which never return.
 *
 * exprtree_overflow is for the 65th counted node ("Precedence Stack
 * Overflow"), exprtree_full for a full arena (not reached: see
 * EXPRTREE_NODES). */
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

/** @brief Counts n (makes it pending) with operator op and precedence prec.
 *
 * The 65th calls exprtree_overflow. */
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

/** @brief The walk of one level (eval5), returning the precedence of the
 *         operator that ends it (0: none), *op that operator.
 *
 * There must be a pending node (the expression's, or an operator's). */
unsigned exprtree_climb(void *ctx, unsigned *op);

#ifdef __cplusplus
}
#endif

#endif
