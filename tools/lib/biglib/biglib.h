/*
 * biglib.h - BigLib files (blitz2:acidlibs, blitz2:deflibs; blitz2.s
 * loadlibgroup): for every library its length as a big-endian long and the
 * library file (an AmigaDOS load file), in order; a long 0 ends the file.
 */
#ifndef BIGLIB_H
#define BIGLIB_H

#include <stdint.h>
#include <stdio.h>

/** @brief One library of a BigLib file. */
struct biglib_entry {
    const uint8_t *data;        /* the library file */
    uint32_t size;
};

/**
 * @brief Write a BigLib file: the entries and the end mark.
 * @param out      the file
 * @param entries  the libraries
 * @param n        their number
 * @return 0 = ok, -1 = write error
 */
int biglib_write(FILE *out, const struct biglib_entry *entries, size_t n);

/**
 * @brief The entries of a BigLib file.
 * @param buf      the file
 * @param size     its size
 * @param entries  out: the entries (pointers into buf; *entries to be
 *                 freed)
 * @param n        out: their number
 * @param err      the message when the file is broken
 * @param errlen   the size of err
 * @return 0 = ok, -1 = broken
 */
int biglib_read(const uint8_t *buf, size_t size, struct biglib_entry **entries, size_t *n,
                char *err, size_t errlen);

/**
 * @brief The number of a library file (word at offset 4 of its code hunk).
 * @param data    the library file
 * @param size    its size
 * @param libnum  out: the number
 * @return 0 = ok, -1 = not a load file with such a hunk
 */
int biglib_number(const uint8_t *data, size_t size, unsigned *libnum);

/**
 * @brief The library with a number.
 * @param entries  the libraries
 * @param n        their number
 * @param libnum   the library number
 * @return the index of the first library with that number, -1 = none
 */
long biglib_find(const struct biglib_entry *entries, size_t n, unsigned libnum);

/**
 * @brief The first library with the number of an earlier one.
 *
 * An entry without a number does not count.  Two libraries with one number
 * cannot both be loaded by blitz2: their tokens and !libs references would
 * be the same.
 *
 * @param entries  the libraries
 * @param n        their number
 * @param earlier  out: the earlier one's index
 * @return its index, -1 = all numbers differ
 */
long biglib_clash(const struct biglib_entry *entries, size_t n, size_t *earlier);

#endif
