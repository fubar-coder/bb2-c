/*
 * xtra_ab3.h - AmiBlitz3's "Embedded .xtra Header" of .ab3 text sources
 * (xtra_ab3.c, docs/formats.md 7) into and out of struct xtra (xtra.h).
 * Compiled by each user together with xtra.c.
 */
#ifndef XTRA_AB3_H
#define XTRA_AB3_H

#include "xtra.h"

#ifdef __cplusplus
extern "C" {
#endif

/* reads the embedded header of an AmiBlitz3 text source buf[0..size):
   x is initialised with xtra_init and AmiBlitz3's defaults for the
   fields the header has (docs/formats.md 7.3), then the keys found are
   set (x->ab3.seen); source = XTRA_SRC_AB3_TEXT.  A source whose first
   line is not "; XTRA" has no header: 0, x->ab3.header = 0, *end = 0.
   Otherwise *end is the offset after the "; /XTRA" line (the program
   text starts there).  0 or enum xtra_err with the line in err (may be
   NULL). */
int xtra_read_ab3(struct xtra *x, const uint8_t *buf, size_t size, size_t *end,
                  struct xtra_error *err);

/* writes x as AmiBlitz3 header - the lines AmiBlitz3's write_AB3Header
   writes, LF line ends - to out (cap bytes, not 0-terminated); the
   maxima with XTRA_MAX_NAME (the others skipped: *skipped, may be NULL).
   The size in *len (also when it does not fit).  0 or XTRA_E_SPACE. */
int xtra_write_ab3(const struct xtra *x, char *out, size_t cap, size_t *len, unsigned *skipped);

#ifdef __cplusplus
}
#endif

#endif
