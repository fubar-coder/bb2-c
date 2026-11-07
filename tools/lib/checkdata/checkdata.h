/*
 * checkdata.h - data of a C build against the reference build of the
 * assembler original (bb2 ted-c/tools/checkdata.py): for every entry of a
 * table, the bytes of the C symbol (from the HUNK_SYMBOL of a link with
 * symbols, e.g. ted.sym) must equal the bytes of the reference's code hunk
 * from a start label to an end label or for a size (labels from the vasm
 * listing).  Relocated longs of the reference are skipped (pointers);
 * behind the initialised part of a C hunk (BSS, merged BSS) count zeros.
 * A list of intended differences (fixed original bugs) names the checks
 * that differ on purpose; each of them must differ.
 */
#ifndef CHECKDATA_H
#define CHECKDATA_H

#include <stdio.h>
#include "hunk.h"
#include "vasmlisting.h"

/** @brief One check: a C symbol against a range of the reference. */
struct checkdata_entry {
    char *csym;                 /* C symbol, e.g. "_sleep_" */
    char *start;                /* start label in the listing */
    char *end;                  /* end label, or NULL: */
    uint32_t size;              /*   size in bytes */
    char *note;
    unsigned line;              /* in the table */
};

/** @brief The checks of a table. */
struct checkdata_table {
    struct checkdata_entry *v;
    size_t n;
};

/**
 * @brief Parse the table of checks.
 * @param text    the table: "CSYM<TAB>START<TAB>END-LABEL|SIZE<TAB>NOTE"
 *                per line, '#' comments and empty lines
 * @param len     its length
 * @param t       out: the checks (checkdata_table_free)
 * @param err     the message of an error
 * @param errlen  the size of err
 * @return 0 = ok, -1 = error
 */
int checkdata_table_parse(const char *text, size_t len, struct checkdata_table *t,
                          char *err, size_t errlen);

/** @brief Free the checks of checkdata_table_parse. */
void checkdata_table_free(struct checkdata_table *t);

/** @brief One intended difference: a check whose bytes differ on purpose. */
struct checkdata_intended_entry {
    char *csym;                 /* the C symbol of the check */
    unsigned issue;             /* the fixed original bug, #NNN */
    char *note;
    unsigned line;              /* in the list */
};

/** @brief The intended differences (fixed original bugs). */
struct checkdata_intended {
    struct checkdata_intended_entry *v;
    size_t n;
};

/**
 * @brief Parse the list of intended differences.
 *
 * Each entry names a check of the table whose bytes differ from the
 * reference because an original bug is fixed; checkdata_run does not count
 * its difference, and the entry must occur (a check without a byte
 * difference, or a symbol that no check has, is a stale entry and fails).
 *
 * @param text    the list: "CSYM<TAB>#NNN<TAB>NOTE" per line (NOTE
 *                optional), '#' comments and empty lines; a symbol at most
 *                once
 * @param len     its length
 * @param in      out: the entries (checkdata_intended_free)
 * @param err     the message of an error
 * @param errlen  the size of err
 * @return 0 = ok, -1 = error
 */
int checkdata_intended_parse(const char *text, size_t len, struct checkdata_intended *in,
                             char *err, size_t errlen);

/** @brief Free the entries of checkdata_intended_parse. */
void checkdata_intended_free(struct checkdata_intended *in);

/**
 * @brief Run every check of the table.
 *
 * Writes "FAIL"/"MISSING" lines (verbose: also "ok"), "intended" lines for
 * the differences of the list, "STALE" lines for its entries that did not
 * occur, and a summary to out.
 *
 * @param t        the checks
 * @param l        the vasm listing of the reference (the labels)
 * @param ref      the code hunk of the reference
 * @param c        the C build with symbols
 * @param in       the intended differences, or NULL: none
 * @param verbose  nonzero: also the checks that pass
 * @param out      the result
 * @return the number of failed checks and stale entries
 */
long checkdata_run(const struct checkdata_table *t, const struct listing *l,
                   const struct hunk *ref, const struct loadfile *c,
                   const struct checkdata_intended *in, int verbose, FILE *out);

#endif
