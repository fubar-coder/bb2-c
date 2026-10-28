/*
 * checkdata.h - data of a C build against the reference build of the
 * assembler original (bb2 ted-c/tools/checkdata.py): for every entry of a
 * table, the bytes of the C symbol (from the HUNK_SYMBOL of a link with
 * symbols, e.g. ted.sym) must equal the bytes of the reference's code hunk
 * from a start label to an end label or for a size (labels from the vasm
 * listing).  Relocated longs of the reference are skipped (pointers);
 * behind the initialised part of a C hunk (BSS, merged BSS) count zeros.
 */
#ifndef CHECKDATA_H
#define CHECKDATA_H

#include <stdio.h>
#include "hunk.h"
#include "vasmlisting.h"

struct checkdata_entry {
    char *csym;                 /* C symbol, e.g. "_sleep_" */
    char *start;                /* start label in the listing */
    char *end;                  /* end label, or NULL: */
    uint32_t size;              /*   size in bytes */
    char *note;
    unsigned line;              /* in the table */
};

struct checkdata_table {
    struct checkdata_entry *v;
    size_t n;
};

/* the table: "CSYM<TAB>START<TAB>END-LABEL|SIZE<TAB>NOTE" per line, '#'
   comments and empty lines; 0 = ok, -1 = error (message in err) */
int checkdata_table_parse(const char *text, size_t len, struct checkdata_table *t,
                          char *err, size_t errlen);
void checkdata_table_free(struct checkdata_table *t);

/* runs every check: ref = the code hunk of the reference, c = the C build
   with symbols.  "FAIL"/"MISSING" lines (verbose: also "ok") and a summary
   to out; returns the number of failed checks */
long checkdata_run(const struct checkdata_table *t, const struct listing *l,
                   const struct hunk *ref, const struct loadfile *c, int verbose, FILE *out);

#endif
