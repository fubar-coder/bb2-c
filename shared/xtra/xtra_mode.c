/*
 * xtra_mode.c - the mode a source is compiled in by blitz2-ab3 (xtra.h
 * xtra_mode; docs/ab3/variant.md, "Detection"): Blitz Basic 2's rules or
 * AmiBlitz3's.  The same rule decides the mode of the main source and of
 * a tokenised include.  C99, freestanding, like the rest of shared/xtra.
 */
#include "xtra.h"

int xtra_mode(int force, const uint8_t *src, size_t len, const char *path, const uint8_t *xtra,
              size_t xlen)
{
    if (force == XTRA_MODE_BB2 || force == XTRA_MODE_AB3)
        return force;
    switch (xtra_detect(src, len, path)) {
    case XTRA_T_AB3:            /* "; XTRA" header, or a text named *.ab3 */
        return XTRA_MODE_AB3;
    case XTRA_T_TED:            /* tokenised: by its companion */
        return xtra_is_ab3_xtra(xtra, xlen) ? XTRA_MODE_AB3 : XTRA_MODE_BB2;
    default:                    /* BB22, text without header */
        return XTRA_MODE_BB2;
    }
}
