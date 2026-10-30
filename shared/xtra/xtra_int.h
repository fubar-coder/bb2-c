/*
 * xtra_int.h - what xtra.c and xtra_ab3.c share inside shared/xtra (not
 * for the users: they include xtra.h).
 */
#ifndef XTRA_INT_H
#define XTRA_INT_H

#include "xtra.h"

/* sets err (may be NULL) and gives code */
int xtra_fail(struct xtra_error *err, int code, int in, size_t offset, uint32_t line);

/* 1 if the first line of buf[0..size) is "; XTRA" */
int xtra_ab3_has_header(const uint8_t *buf, size_t size);

/* AmiBlitz3's defaults for what its header can set (x initialised) */
void xtra_ab3_defaults(struct xtra *x);

/* reads the embedded header of buf[0..size) into x (initialised by
   xtra_load): x->hdr, the views, the numbers; 0 or enum xtra_err */
int xtra_ab3_read(struct xtra *x, const uint8_t *buf, size_t size, struct xtra_error *err);

/* the next "Max NAME = N" line of the header h from *pos (before end) into
   m, *pos behind it; 0 if there is none */
int xtra_ab3_next_max(const uint8_t *h, uint32_t *pos, uint32_t end, struct xtra_max *m);

#endif
