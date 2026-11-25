/*
 * exprmatrix.h - the expression matrix of blitz2 (OpenSpec change
 * expression-type-matrix, spec testing/expression-matrix): the description
 * of the matrix read from a text, its cells (context x operator x operand
 * kinds in order x value set, each with a unique key), and the Blitz
 * sources that print one keyed line per cell (design decision 8), split
 * into sources of at most N cells.
 *
 * The description (tools/exprmatrix/README.md, "The description"), one
 * directive per line, '#' starts a comment, words separated by blanks:
 *
 *   cells-per-source N          at most N cells per source (default 100)
 *   dim N                       the last element of the index arrays (300)
 *   values SET KIND=VALUE...    the operand values of the value set SET
 *   counts SET KIND=VALUE...    the right operand of the shifts and ^ in SET
 *   block NAME                  starts a block (an operator group)
 *   contexts CTX...             the block's contexts
 *   binary OP...                its operators of two operands
 *   unary OP...                 its operators of one operand
 *   kinds KIND...               its operand kinds (both sides)
 *   sets SET...                 its value sets
 *   cell SET CTX OP KIND [KIND] one cell of the block, given by itself
 *
 * A block makes the product of its sets, contexts, operators and kinds
 * (two operands: every ordered pair of its kinds), and its cell lines.
 * The product leaves out the combinations BASIC has no meaning for: a
 * string with a number, a string under an operator other than + and the
 * comparisons, a string + outside Print, a string under a unary operator;
 * a cell line with one of them is an error.
 *
 * Kinds: b w l q f (Byte, Word, Long, Quick, Float variables), s (a
 * string variable), cb cw cl (constants of Byte, Word and Long size), cf
 * (a constant with a fraction).  Operators: + - * / MOD ^ AND OR EOR LSL
 * LSR ASL ASR = <> < <= > >= (binary), - NOT (unary); words in any case.
 * Contexts: db dw dl dq df (destination Byte to Float), pl pw pq pf (a
 * parameter Long, Word, Quick, Float), if, i1 (the first array index), i2
 * (a later index), print.
 *
 * A key is CTX-OP-KIND[-KIND]-SET with OP one of add sub mul div mod pow
 * and or eor lsl lsr asl asr eq ne lt le gt ge (binary), neg not (unary):
 * "dl-mul-q-l-A" is r.l=q*l with the values of set A.  A key made twice is
 * an error.  A source holds the cells of one block, set and context, in
 * the order of the description; its name is BLOCK-CTX-SET-NN (NN from
 * 01), at most EM_NAME_MAX characters.
 */
#ifndef EXPRMATRIX_H
#define EXPRMATRIX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum em_kind { EM_B, EM_W, EM_L, EM_Q, EM_F, EM_S, EM_CB, EM_CW, EM_CL, EM_CF, EM_NKINDS };

enum em_op {
    EM_ADD, EM_SUB, EM_MUL, EM_DIV, EM_MOD, EM_POW, EM_AND, EM_OR, EM_EOR,
    EM_LSL, EM_LSR, EM_ASL, EM_ASR, EM_EQ, EM_NE, EM_LT, EM_LE, EM_GT, EM_GE,
    EM_NEG, EM_NOT, EM_NOPS
};

enum em_ctx {
    EM_DB, EM_DW, EM_DL, EM_DQ, EM_DF, EM_PL, EM_PW, EM_PQ, EM_PF,
    EM_IF, EM_I1, EM_I2, EM_PRINT, EM_NCTX
};

enum {
    EM_NAME_MAX = 25,           /* a source name, without ".bas" (AmigaDOS: 30 with ".exe" and more) */
    EM_KEY_MAX = 40,            /* a key, with its 0 */
    EM_PER_SOURCE_DEFAULT = 100,
    EM_DIM_DEFAULT = 300
};

/* a value set: the text of each kind's value and count (NULL: not given) */
struct em_set {
    char *name;
    int line;
    char *value[EM_NKINDS];
    char *count[EM_NKINDS];
};

struct em_cell {
    enum em_ctx ctx;
    enum em_op op;
    enum em_kind left;
    int right;                  /* enum em_kind, -1 for a unary operator */
    size_t set;                 /* index into em_matrix.sets */
    size_t block;               /* index into em_matrix.blocks */
    int line;                   /* the line of the block or the cell line */
    char key[EM_KEY_MAX];
};

struct em_block {
    char *name;
    int line;
};

struct em_matrix {
    struct em_set *sets;
    size_t nsets;
    struct em_block *blocks;
    size_t nblocks;
    struct em_cell *cells;
    size_t ncells;
    long per_source;
    long dim;
};

/* a generated Blitz source (text, LF line ends) */
struct em_source {
    char *name;                 /* BLOCK-CTX-SET-NN, without ".bas" */
    char *text;
    size_t ncells;
};

/* the names of the description and the keys */
const char *em_kind_name(enum em_kind k);
const char *em_op_name(enum em_op o);         /* the key part: add, ..., neg, not */
const char *em_op_text(enum em_op o);         /* the Blitz text: +, MOD, NOT, ... */
const char *em_ctx_name(enum em_ctx c);
int em_op_is_unary(enum em_op o);
int em_op_is_compare(enum em_op o);

/*
 * Reads the description (len bytes of text; file names the messages:
 * "FILE:LINE: message") into *m.  Returns 0; 1 with one line per error in
 * *errors (malloc'd, the caller frees it) and *m empty; -1 out of memory.
 */
int em_parse(const char *text, size_t len, const char *file, struct em_matrix *m, char **errors);
void em_free(struct em_matrix *m);

/*
 * The sources of the matrix, at most per_source cells each (<= 0: the
 * description's cells-per-source); desc is the description's name for the
 * header of each source.  Returns 0; 1 with the errors in *errors (a source
 * name too long); -1 out of memory.
 */
int em_generate(const struct em_matrix *m, const char *desc, long per_source, struct em_source **out,
                size_t *nout, char **errors);
void em_sources_free(struct em_source *s, size_t n);

/*
 * The text of one source of the cells (n >= 1, one value set and one
 * context): head (comment lines, each ending in a newline), the variables,
 * the context's setup and one line per cell, then End.  seterr: every cell
 * behind a SetErr handler that prints "KEY !runtime" and goes on behind
 * the cell (the checked index cells: a runtime error outside the array).
 * line_of (n entries, or NULL): the line of each cell.  malloc'd; NULL out
 * of memory.
 */
char *em_source_text(const struct em_matrix *m, const struct em_cell *const *cells, size_t n, const char *head,
                     int seterr, int *line_of);

/*
 * The source of a test of the BASIC test model of the cells (n >= 1, one
 * value set and one context; tests/README.md, "Tests that judge
 * themselves"): head (comment lines, each ending in a newline), the
 * statement of a parameter context (it stores its argument in the global
 * gT), and the case "cells": the variables, the context's setup and per
 * cell its line with a check named by the cell's key whose expected value
 * is expect[i] (a Long constant: a Quick or Float destination or
 * parameter checks its raw bits, an If 1 or 0, a checked index cell -1
 * for the runtime error); a Print cell prints "KEY VALUE" and has no
 * check (expect may be NULL).  seterr: every index cell behind a SetErr
 * handler that leaves -1 and goes on behind the cell, ClrErr at the end.
 * malloc'd; NULL out of memory.
 */
char *em_case_text(const struct em_matrix *m, const struct em_cell *const *cells, size_t n, const char *head,
                   const char *const *expect, int seterr);

/*
 * The model (model.c, design decision 7): the value a cell prints by a set
 * of rules.  The rules are flags; the original's set is EM_RULES_ORIG
 * (blitz2 1.95 with the CD's libraries), the fixed one EM_RULES_FIXED.
 */
enum {
    EM_R_CDLIB = 1 << 0,        /* the CD's libraries (else the port's C libraries, #199 #233 #234 fixed) */
    EM_R_1177 = 1 << 1,         /* comparisons with their own values (#1177) */
    EM_R_1186 = 1 << 2,         /* the cut fraction literal left of a comparison is a Float (#1186) */
    EM_R_1178 = 1 << 3,         /* Quick arithmetic by variant C (#1178) */
    EM_R_1179 = 1 << 4,         /* every array index a Long (#1179) */
    EM_R_1187 = 1 << 5,         /* a Float condition tests the value (#1187) */
    EM_R_1188 = 1 << 6,         /* a fraction constant keeps its fraction under a Byte type (#1188) */
    EM_R_1189 = 1 << 7,         /* ^ with a negative base and a whole exponent keeps the sign (#1189) */
    EM_R_1209 = 1 << 8          /* AND OR EOR NOT under a Quick type so far on the whole parts (#1209) */
};
#define EM_RULES_ORIG EM_R_CDLIB
#define EM_RULES_PORTCD (EM_R_CDLIB | EM_R_1177)    /* the port as measured, CD's libraries */
#define EM_RULES_PORT EM_R_1177                     /* the port as measured, C libraries */
#define EM_RULES_PORTNOW (EM_R_1177 | EM_R_1186 | EM_R_1178 | EM_R_1179 | EM_R_1187 | EM_R_1188 | EM_R_1189 | EM_R_1209)  /* the port of this commit, C libraries */
/* the port's rules when the matrix was committed: the cells of an issue
   fixed later (a rule outside this set) stay in their bug-orig tests, which
   get fixed-orig = N (expect.c, option T1) */
#define EM_RULES_MATRIX (EM_R_1177 | EM_R_1186 | EM_R_1187)
/* the fixes made in the C libraries (#1189: mathtranslib's power): the
   port with the CD's libraries keeps the original's rule there */
#define EM_RULES_LIBFIX EM_R_1189
#define EM_RULES_FIXED (EM_R_1177 | EM_R_1186 | EM_R_1178 | EM_R_1179 | EM_R_1187 | EM_R_1188 | EM_R_1189 | EM_R_1209)

/* what keeps a value from being exact or known (the question classes of
   design decision 7) */
enum {
    EM_F_D1 = 1 << 0,           /* a Float condition of the original: what d1 holds (#1187) */
    EM_F_POW = 1 << 1,          /* ^ (mathtrans SPPow, not modelled) */
    EM_F_OVF = 1 << 2,          /* Q-A: an overflow of a result or a conversion */
    EM_F_BITQ = 1 << 3,         /* Q-C: AND OR EOR NOT or a shift with a Quick or Float operand */
    EM_F_MODNF = 1 << 4,        /* Q-D: MOD with negative or fractional operands */
    EM_F_POWINT = 1 << 5,       /* Q-E: ^ with whole-number types */
    EM_F_POWNEG = 1 << 6,       /* ^ with a negative base and a fractional exponent: baseline (#1189 fixes whole exponents) */
    EM_F_POWSIGN = 1 << 7,      /* EM_R_1189: the sign of an odd power applied to what SPPow gives */
    EM_F_MODLF = 1 << 8         /* a MOD in Long or Float (domodl, domodf: not folded; #1049) */
};

enum { EM_TEXT_MAX = 48 };

/* the value of a cell as the measurement records it: the printed value,
   "T"/"F", "!noncompile", "!runtime", "!outside", "!garbage", or "?"
   (not known: EM_F_D1, EM_F_POW); "?-" is not known either, but the
   negation of what the original's SPPow gives (EM_F_POWSIGN, #1189) */
struct em_value {
    char text[EM_TEXT_MAX];
    unsigned flags;
    unsigned long long sig;     /* EM_F_POW: the bits of the two Float operands of ^ */
    int type;                   /* the type so far after the last operation (1 Byte, 2 Word, 3 Long, 4 Quick,
                                   5 Float, 7 string as ET_* of exprtype.h); 0: no expression evaluated (a
                                   cell the model refuses, an index cell) */
};

/*
 * The value of cell c by the rules; checked: runtime error checking on (an
 * index cell's value with it, else without, design decision 8).
 */
void em_model(const struct em_matrix *m, const struct em_cell *c, unsigned rules, int checked, struct em_value *out);

/*
 * The measured cell table (classify.c): one line per cell, tab separated,
 * "key source" and the values of the nine columns below (the header line
 * "key\t..." skipped); a value is a printed value or !noncompile[:MESSAGE],
 * !runtime[:MESSAGE], !outside, !garbage[:VALUE]; the checked columns are
 * empty for a cell that is no index cell.
 */
enum {
    EM_C210, EM_C195, EM_PORTCD, EM_CLIB, EM_AB3,
    EM_C210_ON, EM_C195_ON, EM_PORTCD_ON, EM_CLIB_ON, EM_NCOLS
};

struct em_row {
    char *key;
    int line;
    char v[EM_NCOLS][EM_TEXT_MAX];      /* normalised as em_model gives values */
    int has[EM_NCOLS];
};

struct em_table {
    struct em_row *rows;
    size_t n;
};

const char *em_col_name(int col);

/* Returns 0; 1 with one line per error in *errors (malloc'd): a line
   without the 11 fields, a bad value; -1 out of memory. */
int em_table_parse(const char *text, size_t len, const char *file, struct em_table *t, char **errors);
void em_table_free(struct em_table *t);

/* the result of em_classify (malloc'd texts) */
struct em_classified {
    char *table;                /* the cell table with a class per cell (TSV) */
    char *questions;            /* expr-matrix-questions.md */
    char *report;               /* counts per class and question, the model's mismatches */
    size_t mismatches;          /* original rules against 1.95 */
    size_t port_mismatches;     /* the port's rules against the port's measurement */
};

/*
 * Classifies every cell of table t (design decision 6): agree, baseline
 * (the original's measured value where no recorded rule covers the
 * cell), fixed-N, open-N, port, new, question (with its class Q-B, Q-E or
 * Q-F), noncompile.
 * Returns 0; 1 with the errors in *errors (a key that is no cell, a cell
 * measured twice or not at all, an index cell without its checked
 * values); -1 out of memory.
 */
int em_classify(const struct em_matrix *m, const struct em_table *t, struct em_classified *out, char **errors);
void em_classified_free(struct em_classified *c);

/* the class of a cell as em_classes gives it */
struct em_class {
    const struct em_cell *cell;
    char cls[48];               /* agree, baseline, fixed-N, open-N, port, new, question, noncompile */
    char issues[32];            /* the issues whose fixed rules change the value, "1178+1189" */
    char q;                     /* the question class 'B', 'E', 'F' (0: none) */
    int base;                   /* the expected value is the original's measured one */
    int chk;                    /* an index cell with its checked values */
    char exp[EM_TEXT_MAX], exp_on[EM_TEXT_MAX];         /* the expected value (unchecked, checked) */
};

/* The classes of the rows of t, in their order (*out: t->n entries,
   malloc'd); as em_classify: 0, 1 with the errors, -1 out of memory. */
int em_classes(const struct em_matrix *m, const struct em_table *t, struct em_class **out, char **errors);

/*
 * The committed tests of the matrix (expect.c, task 3.1 of the change).
 * meas: the measured values the model cannot compute (em_measured); every
 * other value is the model's.  The files (paths relative to the root of
 * the source tree, LF line ends): the tests of the BASIC test model of the
 * agreement, baseline and fixed cells, the bug-orig tests of the open
 * cells (and of the cells of an issue fixed after the matrix was
 * committed, EM_RULES_MATRIX, with fixed-orig = N), the tests of the
 * refused cells with their data sources, the AmiBlitz3 cases and the list
 * of the cells left out and the named cells of the decided rule of
 * expr-tree-5 (README.md, "expect").  *report: the counts.  Returns 0; 1 with the
 * errors in *errors; -1 out of memory.
 */
struct em_file {
    char *path;
    char *text;
};
int em_expect(const struct em_matrix *m, const char *desc, const struct em_table *meas, struct em_file **files,
              size_t *nfiles, char **report, char **errors);
/* as em_expect, with the port's rules port in place of EM_RULES_PORTNOW
   (the tests of a rule set in which an issue is fixed) */
int em_expect_port(const struct em_matrix *m, const char *desc, const struct em_table *meas, unsigned port,
                   struct em_file **files, size_t *nfiles, char **report, char **errors);
void em_files_free(struct em_file *f, size_t n);

/* the directories and name patterns of the files em_expect owns: a file
   of these is stale when em_expect does not write it */
int em_expect_owns(const char *path);

/* The rows of the measured table t that em_expect needs (the values the
   model does not know, AmiBlitz3's where it differs from 1.95), as a
   table with the header line; malloc'd, NULL out of memory. */
char *em_measured(const struct em_matrix *m, const struct em_table *t);

/*
 * The candidate rules of the decision round of expr-tree-5-operand-typing
 * (open question 1, task 2.1; model.c, "Candidate rules"): operand typing,
 * the type of each operation decided by its operator, its two operands
 * and the wanted type, not by the order.  The decided rule is P+
 * (EM_RULES_DECIDED below, task 2.2); the others stay for the record of
 * the round.  A rule set with EM_R_CAND(n) (n = EM_CAND_C1 .. EM_CAND_C7) computes a cell by
 * candidate n on top of the port's rules; EM_R_SCOPEB gives the left
 * operand of a comparison the scope (b) of the open question (own typing
 * only with a Quick or Float inside or a Long constant under Float;
 * without it scope (a), every left operand typed by its own operands).
 */
enum { EM_CAND_FLOW, EM_CAND_C1, EM_CAND_C2, EM_CAND_C3, EM_CAND_C4, EM_CAND_C5, EM_CAND_C6, EM_CAND_C7, EM_NCAND };
#define EM_R_CAND(n) ((unsigned)(n) << 9)
#define EM_R_CANDMASK (7u << 9)
#define EM_R_SCOPEB (1u << 12)         /* C7: "C7+", its whole-number expressions by C1, not today's */
/* C5, C6: a 32.16 product that does not fit its consumer is a runtime error
   (as under runtime error checking), not cut */
#define EM_R_C5ERR (1u << 13)
/* C7K (task 2.1b, "C7: constants by their smallest type", 2026-11-24): C7
   with constants by their smallest type (Byte, Word, Long; a fraction a
   Quick) and the operands of every comparison typed by their own operands;
   a whole-number operation in them in its own type, at least Long
   (EM_R_C7KLONG) or at least the destination's whole-number width
   (EM_R_C7KDEST) */
#define EM_R_C7K (1u << 14)
#define EM_R_C7KLONG (1u << 15)
#define EM_R_C7KDEST (1u << 16)
/* C7Kx (task 2.1b, a refinement): with C7K, a constant with a fraction is a
   Quick only if a 16.16 holds it exactly or a Quick is beside it, else a
   Float as today */
#define EM_R_C7KEXACT (1u << 17)
/*
 * The decided rule P+ (task 2.2; the analysis
 * ~/.local/share/bb2-c-work/expr-tree-5-analysis/report.md, section 1, with
 * the user's answers in rounds/tree/decisions.md of 2026-11-24, "expr-tree-5
 * Q4 (P+ round 1)" and "(P+ round 2)"): C7Kx with the parts below.  Each
 * part is a flag of its own so that the model can show what a part
 * changes; the rule is all of them, EM_RULES_DECIDED.
 *
 * EM_R_PTYPED  changes 5 and 6: the operands of bit operators, shifts and
 *              Not, and a Quick subtree inside an expression with a Float,
 *              typed by the same rule (rounded once where the operator
 *              takes them); a comparison's boolean beside a whole number
 *              is a Long operation only with a Long; Word+Word a Long only
 *              inside a comparison; a / at the top rounded towards zero
 *              only under a wanted Long.
 * EM_R_PCONST  change 2 (F2, "Float wie Original, sonst Quick"): a
 *              constant with a point is fractional, never integral (also
 *              3.0, also under a Byte type: F6); a Float beside whole
 *              numbers only or under a wanted Long, a Quick where a Quick
 *              is in its expression or wanted.
 * EM_R_PMOD    change 4 (F4, "Heutige Regel, exakt"): Mod by today's sign
 *              rule (domodlong: the divisor added to a negative
 *              remainder), exact with fractions.
 * EM_R_PNARROW change 1 (F5, "16.16 wie Original"): 48.16 only where a
 *              Long is wanted or takes part; otherwise 16.16 with the
 *              original's code and values.
 * EM_R_PDIVL   change 3 (F3, "Ja, sofort ganzzahlig"): a / under a wanted
 *              Long gives a whole number towards zero at the operator.
 * EM_R_PREACH  change 7 (F1, "Ja, außer unter Long-Ziel"): in an
 *              expression with a Quick or Float not wanted as a Long, a
 *              whole-number subtree is computed in the expression's
 *              fractional format.
 * EM_R_PCMP    change 8 (F1): a comparison's common format, the wider
 *              class of its two sides, reaches into both operands.
 * EM_R_PBITS   bit operators and shifts with fractions ("expr-tree-5: bit
 *              operators and shifts with fractions; Quick /0; Mod /0",
 *              2026-11-25): AND, OR, EOR and NOT take a constant with a
 *              point or a fraction on either side as its whole part,
 *              rounded down (b AND 1.5 = 1.5 AND b = b AND 1); a constant
 *              with a point as a shift count is refused.
 * EM_R_PQDIV   the 16.16 routines as today ("expr-tree-5: shared C for
 *              typing, arithmetic and folding"): a 16.16 division is
 *              qdivide bit for bit (its last bit, $7FFFFFFF for a divisor
 *              0); a wider division by 0 gives the largest number with the
 *              dividend's sign, as lqdiv.
 *
 * EM_RULES_P is P (changes 1-6) of the analysis, for the record.
 */
#define EM_R_PTYPED (1u << 18)
#define EM_R_PCONST (1u << 19)
#define EM_R_PMOD (1u << 20)
#define EM_R_PNARROW (1u << 21)
#define EM_R_PDIVL (1u << 22)
#define EM_R_PREACH (1u << 23)
#define EM_R_PCMP (1u << 24)
#define EM_R_PQDIV (1u << 25)
#define EM_R_PBITS (1u << 26)
#define EM_RULES_CANDBASE EM_RULES_PORTNOW      /* the rules the candidates are put on */
#define EM_RULES_C7KX (EM_R_CAND(EM_CAND_C7) | EM_R_C7K | EM_R_C7KLONG | EM_R_C7KEXACT)
#define EM_RULES_P                                                                                                    \
    (EM_RULES_CANDBASE | EM_RULES_C7KX | EM_R_PTYPED | EM_R_PCONST | EM_R_PMOD | EM_R_PNARROW | EM_R_PDIVL            \
     | EM_R_PQDIV)
#define EM_RULES_DECIDED (EM_RULES_P | EM_R_PREACH | EM_R_PCMP | EM_R_PBITS)

/* the short name of candidate n ("flow", "C1", ...) and its title */
const char *em_cand_name(int n);
const char *em_cand_title(int n);

/*
 * An expression as a tree, for the expressions the matrix has no cell for
 * (chains, a compound left operand).  text is BASIC: variables (a name,
 * its type the suffix .b .w .l .q .f or the one in vars, else Quick),
 * constants (decimal, with a point, $hex, %bin), the binary operators of
 * the matrix and & |, with blitz2's precedences (AND OR EOR, comparisons,
 * + -, * /, & | MOD, shifts, ^; left to right), a minus before an
 * element, NOT (the rest of the expression), brackets.  vars: blank
 * separated NAME[.T]=VALUE, the values of the variables.  Returns 0; 1
 * with the message in *error (malloc'd); -1 out of memory.
 */
struct em_expr;
int em_expr_parse(const char *text, const char *vars, struct em_expr **out, char **error);
void em_expr_free(struct em_expr *x);
/* the expression of cell c as a tree (*out, em_expr_free); 1 for a string
   cell (not typed by the candidates), -1 out of memory */
int em_cell_expr(const struct em_matrix *m, const struct em_cell *c, struct em_expr **out);

/* the types of one operation as a rule set decides them (ET-like letters:
   0 none, 1 Byte .. 5 Float as the T* of model.c) */
struct em_optype {
    int op;                     /* enum em_op; -1: no operation recorded */
    int lfrom, las, lraw;       /* the left operand: its own type, the type it is taken in, a raw Quick */
    int rfrom, ras, rraw;
    int path;                   /* 'p' plain, 'm' exact with the fraction (#1178), 'x' a Long and a Quick
                                   compared exactly, 'c' a comparison, the lower one up */
    int result;                 /* the type after it */
};

/* the value of an expression under ctx (not an index context) by rules,
   with the typing of every operation (sig: one entry per operation in
   post order, "OP LFROM>LAS RFROM>RAS PATH RESULT;") and the one of the
   last (top) operation */
struct em_exprval {
    struct em_value v;
    char sig[1024];
    char cells[1024];           /* the cell of each operation: "OP WANT LFROM RFROM;" (k a constant) */
    struct em_optype top;
    int nops;
    /* C7 and P+: how the value of an expression computed in a fractional
       format is taken (c7root): 'c' a comparison's Word, 'q' the 16.16
       value under a wanted Quick, 'z' no wanted type (the Quick when it
       fits, else the whole part), 'f' a wanted whole number rounded down,
       't' towards zero (a / at the top under a wanted Long); 0 none */
    int consumer;
};
void em_expr_eval(const struct em_expr *x, enum em_ctx ctx, unsigned rules, struct em_exprval *out);

/* the types of one operation a OP b of two variables of the types l and r
   (1 Byte .. 5 Float) under the wanted type want (0 none) by rules, as
   the tables of tools/exprtables show it */
void em_cand_cell(unsigned rules, enum em_op op, int want, int l, int r, struct em_optype *out);
/* the type a constant (the literal text, no sign) takes beside an operand of
   type partner (0: alone) under the wanted type want by rules (today's
   rules: consttype2 under want) */
int em_cand_consttype(unsigned rules, int want, int partner, const char *text);

#ifdef __cplusplus
}
#endif

#endif
