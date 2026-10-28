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

struct code_label {
    unsigned line;
    char *name;
};

struct code_labels {
    struct code_label *v;
    size_t n;
};

/* the code labels of the assembler text; 0 = ok, -1 = out of memory */
int code_labels_parse(const char *text, size_t len, struct code_labels *l);
void code_labels_free(struct code_labels *l);

/* a C source: its name (for messages) and its text */
struct label_source {
    const char *name;
    const char *text;
    size_t len;
};

/* counts the markers "PREFIX:NNN name" (prefix e.g. "ted.s") of every code
   label in the sources.  Writes "MISSING PREFIX:NNN name", "TWICE
   PREFIX:NNN name: FILE, FILE" (verbose: also "ok ...") and a summary to
   out.  Returns the number of labels that are missing or there more than
   once (-1 = out of memory). */
long labels_check(const struct code_labels *l, const char *prefix,
                  const struct label_source *src, size_t nsrc, int verbose, FILE *out);

#endif
