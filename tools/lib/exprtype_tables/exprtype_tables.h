/*
 * exprtype_tables.h - readable tables of the type rules of an expression
 * (tools/exprtables; OpenSpec change expr-tree-5-operand-typing, design
 * decision 3): for one operation of two values, keyed by (operator, left
 * type, right type, type so far), the type each operand is taken in and
 * the result type; the elements (a variable or function value) and the
 * constants under each type so far; the page
 * docs/blitz2/expression-type-tables.md and its check.
 *
 * The rules are a rule set (struct ett_rules), so that another rule (a
 * candidate of the decision round) prints the same tables.  Today's rules
 * (ett_rules_flow) are the flow of the type so far through the decisions
 * of programs/blitz2/src/exprtype.c, taken from a set of primitive
 * decisions in exprtype.c's form (struct ett_prims): exprtype.c itself
 * (ett_prims_exprtype) or the rows of its committed table exprtype.tsv
 * (ett_tsv_prims), which shows that the tables reproduce that table.
 */
#ifndef EXPRTYPE_TABLES_H
#define EXPRTYPE_TABLES_H

#include <stddef.h>
#include <stdint.h>
#include "exprtype.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief How an operand is taken (ett_operand.how). */
enum {
    ETT_KEEP,                   /* as a value of type ett_operand.as */
    ETT_RAW                     /* a Quick with its fraction under a whole-number operation, exact (#1178) */
};

/** @brief One operand of an operation. */
struct ett_operand {
    unsigned char as;           /* the type the operation takes it in (ET_*) */
    unsigned char how;          /* ETT_* */
};

/** @brief The facts of a cell beside the types (each 0 or 1). */
struct ett_facts {
    unsigned char rcmp;         /* the right operand is a comparison (its boolean, a Word; #1182) */
    unsigned char words;        /* the operation is in an array index after the first (#1179) */
    unsigned char byte978;      /* the right operand is a constant from 128 to 255 (#978) */
};

/** @brief One operation a OP b. */
struct ett_cell {
    unsigned char result;       /* the type so far after it; 0: "Illegal Operator for Type" */
    struct ett_operand l, r;
    unsigned char exact;        /* a Long and a Quick compared exactly (#1177) */
    unsigned char wordop;       /* two Word values in a later index (#1179) */
    unsigned char byteext;      /* the constant taken as a sign-extended byte (#978; ETB_*) */
};

/** @brief One element (a variable, a function value or a constant). */
struct ett_elem {
    unsigned char result;       /* the type so far after it; 0: "Type Mismatch" */
    unsigned char conv;         /* the value is converted to result */
    unsigned char raw;          /* a Quick held with its fraction, the type so far a Long (#1178) */
    unsigned char litcut;       /* a constant with a point made a whole number (#1186, #1188) */
};

/** @brief A rule set: the decisions the tables show.  Each function
 *         returns 0, or -1 when the rule set has no decision for the
 *         case (the tables are then not made). */
struct ett_rules {
    const char *name;
    void *ctx;
    /** the operation op on a value of own type l and one of own type r
        (variables), the left operand met at the type so far tsf (0: none);
        x the facts (NULL: none) */
    int (*cell)(void *ctx, unsigned op, unsigned tsf, unsigned l, unsigned r, const struct ett_facts *x,
                struct ett_cell *out);
    /** a value of own type own under the type so far tsf */
    int (*element)(void *ctx, unsigned tsf, unsigned own, struct ett_elem *out);
    /** a constant with the facts k under the type so far tsf */
    int (*constant)(void *ctx, unsigned tsf, const struct exprtype_constfacts *k, struct ett_elem *out);
};

/** @brief The primitive decisions in the form of exprtype.c (exprtype.h);
 *         each returns 0, or -1 when it has no decision for the case. */
struct ett_prims {
    void *ctx;
    int (*cutsraw)(void *ctx, unsigned op, int *out);
    int (*leftop)(void *ctx, unsigned op, unsigned left, int lraw, struct exprtype_left *out);
    int (*ownleft)(void *ctx, unsigned from, unsigned to, int *out);
    int (*binop)(void *ctx, unsigned op, unsigned left, unsigned right, const struct exprtype_binfacts *f,
                 struct exprtype_bin *out);
    int (*element)(void *ctx, unsigned tsf, unsigned own, const struct exprtype_constfacts *c,
                   struct exprtype_elem *out);
};

/** @brief The primitives of programs/blitz2/src/exprtype.c. */
void ett_prims_exprtype(struct ett_prims *p);

/** @brief Today's rules: the type so far flows from left to right through
 *         the primitives p (eval.c: exprtree_pre, binopbuild, contleaf).
 *         p must live as long as r. */
void ett_rules_flow(struct ett_rules *r, const struct ett_prims *p);

/** @brief The rows of exprtype.tsv as primitives. */
struct ett_tsv;

/** @brief Reads the table text (n bytes; name for the messages).
 *         Returns 0 and *out, 1 for malformed rows (*errors: one line per
 *         error, FILE:LINE: ..., malloc'd), -1 out of memory. */
int ett_tsv_load(const char *text, size_t n, const char *name, struct ett_tsv **out, char **errors);
void ett_tsv_prims(struct ett_tsv *t, struct ett_prims *p);
/** @brief The rows read, the rows a decision was taken from, the
 *         decisions asked for that have no row, and the key of the first of
 *         them (NULL: none). */
size_t ett_tsv_rows(const struct ett_tsv *t);
size_t ett_tsv_used(const struct ett_tsv *t);
size_t ett_tsv_missing(const struct ett_tsv *t);
const char *ett_tsv_first_missing(const struct ett_tsv *t);
void ett_tsv_free(struct ett_tsv *t);

/** @brief The operator groups of the page. */
enum {
    ETG_ARITH, ETG_BIT, ETG_SHIFT, ETG_MOD, ETG_POW, ETG_COMPARE, ETG_BITTEST, ETG_COUNT
};

/** @brief The counts of a page (ett_page's stats). */
struct ett_stats {
    unsigned tables[ETG_COUNT];         /* the tables of operations of the group */
    unsigned rows[ETG_COUNT];           /* their rows (left type, right type) */
    unsigned cells[ETG_COUNT];          /* their cells (with the type so far) */
    unsigned swapped[ETG_COUNT];        /* the cells that differ from the swapped operation */
    unsigned changed[ETG_COUNT];        /* ett_page_opts: the cells that differ from the base rules */
    unsigned marked[ETG_COUNT];         /* ett_page_opts: the cells with a mark (a CD place) */
};

/** @brief The page docs/blitz2/expression-type-tables.md by the rule set
 *         r (malloc'd text).  NULL: r has no decision for a case or out of
 *         memory (*error: the message, malloc'd).  stats may be NULL. */
char *ett_page(const struct ett_rules *r, struct ett_stats *stats, char **error);

/** @brief The page of another rule set (a candidate of the decision round
 *         of expr-tree-5, open question 1) beside today's: its own heading
 *         and text in place of the introduction and of the notes of
 *         today's exceptions, the cells that differ from base marked
 *         ◆ (counted in stats->changed), the cells mark() names marked
 *         ‡ (stats->marked), and with constbeside the table of the
 *         constants beside an operand of each type. */
struct ett_pageopts {
    const char *title;          /* the heading (without "# ") */
    const char *about;          /* Markdown after it: what the rule set is, how to read its tables */
    const char *notes;          /* Markdown at the end, in place of the exceptions */
    const struct ett_rules *base;
    int (*mark)(void *ctx, unsigned op, unsigned want, unsigned l, unsigned r);
    void *markctx;
    /** the type a constant (text) takes beside an operand of type partner
        under the wanted type want; NULL: no such table */
    int (*constbeside)(void *ctx, unsigned want, unsigned partner, const char *text);
    void *constctx;
};
char *ett_page_opts(const struct ett_rules *r, const struct ett_pageopts *o, struct ett_stats *stats,
                    char **error);

/** @brief A candidate rule set: the cells of the expression matrix's model
 *         (tools/lib/exprmatrix, em_cand_cell) by the rules rules
 *         (EM_R_CAND), a value keeping its own type, a constant alone as
 *         base gives it; base for what the model does not type (the bit
 *         test operators, a comparison or a later index as the right
 *         operand, #978).  c must live as long as r. */
struct ett_cand {
    uint64_t rules;
    const struct ett_rules *base;
};
void ett_rules_cand(struct ett_rules *r, struct ett_cand *c);

/** @brief Compares the committed page want (name for the messages) with
 *         the generated page got line by line.  Returns 0 equal, 1 they
 *         differ (*report: the first ten differences, FILE:LINE: ..., and
 *         their number, malloc'd), -1 out of memory. */
int ett_page_compare(const char *want, size_t wn, const char *got, size_t gn, const char *name, char **report);

/** @brief The text of a type: "-" (none), B W L Q F A S. */
const char *ett_tname(unsigned t);
/** @brief The text of an operator as Blitz writes it ("BitTst"). */
const char *ett_oname(unsigned op);

#ifdef __cplusplus
}
#endif

#endif
