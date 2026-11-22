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
    EM_R_1189 = 1 << 7          /* ^ with a negative base and a whole exponent keeps the sign (#1189) */
};
#define EM_RULES_ORIG EM_R_CDLIB
#define EM_RULES_PORTCD (EM_R_CDLIB | EM_R_1177)    /* the port as measured, CD's libraries */
#define EM_RULES_PORT EM_R_1177                     /* the port as measured, C libraries */
#define EM_RULES_PORTNOW (EM_R_1177 | EM_R_1186 | EM_R_1187)  /* the port of this commit, C libraries */
#define EM_RULES_FIXED (EM_R_1177 | EM_R_1186 | EM_R_1178 | EM_R_1179 | EM_R_1187 | EM_R_1188 | EM_R_1189)

/* what keeps a value from being exact or known (the question classes of
   design decision 7) */
enum {
    EM_F_D1 = 1 << 0,           /* a Float condition of the original: what d1 holds (#1187) */
    EM_F_POW = 1 << 1,          /* ^ (mathtrans SPPow, not modelled) */
    EM_F_OVF = 1 << 2,          /* Q-A: an overflow of a result or a conversion */
    EM_F_BITQ = 1 << 3,         /* Q-C: AND OR EOR NOT or a shift with a Quick or Float operand */
    EM_F_MODNF = 1 << 4,        /* Q-D: MOD with negative or fractional operands */
    EM_F_POWINT = 1 << 5,       /* Q-E: ^ with whole-number types */
    EM_F_POWNEG = 1 << 6,       /* Q-E: ^ with a negative base and a fractional exponent (#1189: no rule) */
    EM_F_POWSIGN = 1 << 7       /* EM_R_1189: the sign of an odd power applied to what SPPow gives */
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
 * the source tree, LF line ends): the corpus sources and expected files of
 * the agreement, baseline and fixed cells, the bug-orig tests of the open
 * cells, the tests of the refused cells and the list of the cells left
 * out (README.md, "expect").  *report: the counts.  Returns 0; 1 with the
 * errors in *errors; -1 out of memory.
 */
struct em_file {
    char *path;
    char *text;
};
int em_expect(const struct em_matrix *m, const char *desc, const struct em_table *meas, struct em_file **files,
              size_t *nfiles, char **report, char **errors);
void em_files_free(struct em_file *f, size_t n);

/* the directories and name patterns of the files em_expect owns: a file
   of these is stale when em_expect does not write it */
int em_expect_owns(const char *path);

/* The rows of the measured table t that em_expect needs (the values the
   model does not know, AmiBlitz3's where it differs from 1.95), as a
   table with the header line; malloc'd, NULL out of memory. */
char *em_measured(const struct em_matrix *m, const struct em_table *t);

#ifdef __cplusplus
}
#endif

#endif
