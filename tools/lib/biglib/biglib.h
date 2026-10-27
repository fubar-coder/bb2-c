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

#endif
