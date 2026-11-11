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

/** @brief The reference bytes of one range (START to END label or SIZE). */
struct checkdata_ref_entry {
    char *start;                /* start label */
    char *end;                  /* the third field of the table: end label or size */
    uint32_t n;                 /* number of bytes */
    uint8_t *data;              /* the bytes of the reference */
    uint8_t *skip;              /* nonzero: not compared (a relocated long) */
    unsigned line;              /* in the reference data, 0: made from a listing */
};

/**
 * @brief The reference data of a table: the bytes of every range.
 *
 * Made from the reference build (checkdata_ref_make) or read from a file
 * written by checkdata_ref_write (tests/data/ted-checkdata.txt), so the
 * check needs no reference build.
 */
struct checkdata_ref {
    struct checkdata_ref_entry *v;
    size_t n;
};

/**
 * @brief The reference data of every range of a table.
 *
 * A range whose label is not in the listing is left out (a "FAIL ... label
 * not in the listing" line to msgs, if not NULL); ranges of several checks
 * are made once.
 *
 * @param t     the checks
 * @param l     the vasm listing of the reference (the labels)
 * @param ref   the code hunk of the reference
 * @param r     out: the reference data (checkdata_ref_free)
 * @param msgs  the messages of missing labels, or NULL
 * @return the number of ranges left out, -1 = out of memory
 */
long checkdata_ref_make(const struct checkdata_table *t, const struct listing *l,
                        const struct hunk *ref, struct checkdata_ref *r, FILE *msgs);

/**
 * @brief Write the reference data: "START<TAB>END|SIZE<TAB>HEX" per range,
 *        HEX two digits per byte, ".." for a byte not compared.
 * @param r       the reference data
 * @param header  comment lines written first (each line gets "# "), or NULL
 * @param out     the file
 */
void checkdata_ref_write(const struct checkdata_ref *r, const char *header, FILE *out);

/**
 * @brief Parse reference data of checkdata_ref_write ('#' comments and
 *        empty lines; a range at most once).
 * @param text    the data
 * @param len     its length
 * @param r       out: the reference data (checkdata_ref_free)
 * @param err     the message of an error
 * @param errlen  the size of err
 * @return 0 = ok, -1 = error
 */
int checkdata_ref_parse(const char *text, size_t len, struct checkdata_ref *r, char *err,
                        size_t errlen);

/** @brief Free the reference data. */
void checkdata_ref_free(struct checkdata_ref *r);

/**
 * @brief Run every check of the table.
 *
 * Writes "FAIL"/"MISSING" lines (verbose: also "ok"), "intended" lines for
 * the differences of the list, "STALE" lines for its entries that did not
 * occur, and a summary to out.  A check whose range is not in the
 * reference data fails.
 *
 * @param t        the checks
 * @param r        the reference data (checkdata_ref_make or _parse)
 * @param c        the C build with symbols
 * @param in       the intended differences, or NULL: none
 * @param verbose  nonzero: also the checks that pass
 * @param out      the result
 * @return the number of failed checks and stale entries
 */
long checkdata_run(const struct checkdata_table *t, const struct checkdata_ref *r,
                   const struct loadfile *c, const struct checkdata_intended *in, int verbose,
                   FILE *out);

#endif
