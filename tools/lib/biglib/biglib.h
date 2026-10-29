/*
 * biglib.h - BigLib files (blitz2:acidlibs, blitz2:deflibs; blitz2.s
 * loadlibgroup): for every library its length as a big-endian long and the
 * library file (an AmigaDOS load file), in order; a long 0 ends the file.
 */
#ifndef BIGLIB_H
#define BIGLIB_H

#include <stdint.h>
#include <stdio.h>

struct biglib_entry {
    const uint8_t *data;        /* the library file */
    uint32_t size;
};

/* writes the entries and the end mark; 0 = ok, -1 = write error */
int biglib_write(FILE *out, const struct biglib_entry *entries, size_t n);

/* the entries of a BigLib file (pointers into buf; *entries to be freed);
   0 = ok, -1 = broken (message in err) */
int biglib_read(const uint8_t *buf, size_t size, struct biglib_entry **entries, size_t *n,
                char *err, size_t errlen);

/* the number of a library file (word at offset 4 of its code hunk); 0 = ok,
   -1 = not a load file with such a hunk */
int biglib_number(const uint8_t *data, size_t size, unsigned *libnum);

/* the index of the library with that number (the first one), -1 = none */
long biglib_find(const struct biglib_entry *entries, size_t n, unsigned libnum);
/* the first library with the number of an earlier one: its index, and the
   earlier one's in *earlier; -1 = all numbers differ (an entry without a
   number does not count).  Two libraries with one number cannot both be
   loaded by blitz2: their tokens and !libs references would be the same. */
long biglib_clash(const struct biglib_entry *entries, size_t n, size_t *earlier);

#endif
