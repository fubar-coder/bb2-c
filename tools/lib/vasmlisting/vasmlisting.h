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

/** @brief A row of the listing. */
struct listing_row {
    uint32_t addr;
    uint8_t *bytes;             /* the bytes of the row (may be none) */
    size_t nbytes;
    const char *file;           /* source file ("?" before the first Source:) */
    unsigned line;
    char *text;                 /* the source text of the row */
};

/** @brief A symbol of the symbol table. */
struct listing_symbol {
    char *name;
    uint32_t value;
};

/** @brief A listing: rows, symbols and source files (listing_free). */
struct listing {
    struct listing_row *rows;
    size_t nrows;
    struct listing_symbol *syms;        /* in the order of "Symbols by name" */
    size_t nsyms;
    char **files;
    size_t nfiles;
};

/**
 * @brief Read a listing file.
 * @param path    the file
 * @param l       out: the listing (empty on an error)
 * @param err     the message of an error
 * @param errlen  the size of err
 * @return 0 = ok, -1 = error
 */
int listing_read(const char *path, struct listing *l, char *err, size_t errlen);

/** @brief Read a listing from an open file; as listing_read(). */
int listing_read_stream(FILE *f, struct listing *l, char *err, size_t errlen);

/** @brief Free what listing_read filled in. */
void listing_free(struct listing *l);

/**
 * @brief Where a label is defined: the first row whose text starts with it
 *        (case insensitive, [A-Za-z_][A-Za-z0-9_]*).
 * @param l     the listing
 * @param name  the label
 * @return the row, NULL if none
 */
const struct listing_row *listing_defrow(const struct listing *l, const char *name);

/**
 * @brief The map view (format of bb2's tedlisting.py):
 *        "ADDR\tFILE:LINE\tHEXBYTES\tSOURCE" for every row with bytes.
 */
void listing_print_map(const struct listing *l, FILE *out);

/**
 * @brief The labels view (format of bb2's tedlisting.py):
 *        "ADDR\tNAME\tFILE:LINE" sorted by address (stable; ?:0 if the
 *        definition row is not found).
 * @return 0 = ok, -1 = out of memory
 */
int listing_print_labels(const struct listing *l, FILE *out);

/**
 * @brief Compare every listed byte with the code hunk.
 *
 * Relocated longs are skipped (the listing shows 0 there); the first 20
 * differing rows go to out ("diff ADDR FILE:LINE SOURCE").
 *
 * @param l    the listing
 * @param h    the code hunk
 * @param out  the differing rows
 * @return the number of differing rows
 */
size_t listing_verify(const struct listing *l, const struct hunk *h, FILE *out);

#endif
