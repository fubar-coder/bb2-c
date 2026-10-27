/*
 * vasmlisting.h - reading the listing of vasm (-L FILE -Lall): the rows with
 * address, bytes, source file and line, and the symbol table.
 *
 * Row:     00:AAAAAAAA HEXBYTES<white space with a tab>LINE: TEXT
 * Source:  Source: "FILE"           (the file of the following rows)
 * Symbol:  NAME   00:AAAAAAAA  or  NAME   A:AAAAAAAA   (section
 *          "Symbols by name", up to "Symbols by value")
 */
#ifndef VASMLISTING_H
#define VASMLISTING_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "hunk.h"

struct listing_row {
    uint32_t addr;
    uint8_t *bytes;             /* the bytes of the row (may be none) */
    size_t nbytes;
    const char *file;           /* source file ("?" before the first Source:) */
    unsigned line;
    char *text;                 /* the source text of the row */
};

struct listing_symbol {
    char *name;
    uint32_t value;
};

struct listing {
    struct listing_row *rows;
    size_t nrows;
    struct listing_symbol *syms;        /* in the order of "Symbols by name" */
    size_t nsyms;
    char **files;
    size_t nfiles;
};

/* 0 = ok, -1 = error (message in err, l empty) */
int listing_read(const char *path, struct listing *l, char *err, size_t errlen);
int listing_read_stream(FILE *f, struct listing *l, char *err, size_t errlen);
void listing_free(struct listing *l);

/* where a label is defined: the first row whose text starts with it (case
   insensitive, [A-Za-z_][A-Za-z0-9_]*); NULL if none */
const struct listing_row *listing_defrow(const struct listing *l, const char *name);

/* the views (formats of bb2's tedlisting.py):
   map     "ADDR\tFILE:LINE\tHEXBYTES\tSOURCE" for every row with bytes
   labels  "ADDR\tNAME\tFILE:LINE" sorted by address (stable; ?:0 if the
           definition row is not found); -1 = out of memory */
void listing_print_map(const struct listing *l, FILE *out);
int listing_print_labels(const struct listing *l, FILE *out);
/* every listed byte against the code hunk h (relocated longs skipped: the
   listing shows 0 there); the first 20 differing rows go to out ("diff
   ADDR FILE:LINE SOURCE"); returns the number of differing rows */
size_t listing_verify(const struct listing *l, const struct hunk *h, FILE *out);

#endif
