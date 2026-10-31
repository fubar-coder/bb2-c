/*
 * checklabels.h - completeness of a translation from assembler: every code
 * label of the source (ted.s) appears exactly once as "ted.s:NNN label" in
 * the C sources (the function comment, or the marker of a goto label / an
 * internal label).
 *
 * Code label = top-level label whose first statement is an instruction
 * (local .labels, data and equ labels excluded; a label alone on its line
 * belongs to the next statement).  Aliases (two labels on the same
 * address, e.g. tokit/tokit4) each need their own marker.  Labels of
 * include files are not counted.  The rules are those of bb2's
 * checklabels.py.
 */
#ifndef CHECKLABELS_H
#define CHECKLABELS_H

#include <stddef.h>
#include <stdio.h>

/** @brief A code label of the assembler source: its line and name. */
struct code_label {
    unsigned line;
    char *name;
};

/** @brief The code labels of an assembler source. */
struct code_labels {
    struct code_label *v;
    size_t n;
};

/**
 * @brief The code labels of the assembler text.
 * @param text  the assembler source
 * @param len   its length
 * @param l     out: the labels (code_labels_free)
 * @return 0 = ok, -1 = out of memory
 */
int code_labels_parse(const char *text, size_t len, struct code_labels *l);

/** @brief Free the labels of code_labels_parse. */
void code_labels_free(struct code_labels *l);

/** @brief A C source: its name (for messages) and its text. */
struct label_source {
    const char *name;
    const char *text;
    size_t len;
};

/**
 * @brief Count the markers "PREFIX:NNN name" of every code label in the
 *        sources.
 *
 * Writes "MISSING PREFIX:NNN name", "TWICE PREFIX:NNN name: FILE, FILE"
 * (verbose: also "ok ...") and a summary to out.
 *
 * @param l        the code labels
 * @param prefix   the prefix, e.g. "ted.s"
 * @param src      the C sources
 * @param nsrc     their number
 * @param verbose  nonzero: also the labels that are right
 * @param out      the result
 * @return the number of labels that are missing or there more than once
 *         (-1 = out of memory)
 */
long labels_check(const struct code_labels *l, const char *prefix,
                  const struct label_source *src, size_t nsrc, int verbose, FILE *out);

#endif
