/*
 * hunkcheck.h - check the start of the first hunk of a load file: it must
 * be code and contain the given bytes at the given offsets.  For TED:
 * blitz2 calls offset 4 of the first hunk (`outside`), DOS a standalone
 * TED at offset 0 (bsr.w edstart).
 */
#ifndef HUNKCHECK_H
#define HUNKCHECK_H

#include <stdio.h>
#include "hunk.h"

/* specs: "OFFSET:HEXBYTES" (offset decimal or 0x hex, e.g. "4:48e7");
   0 = ok, 1 = mismatch or bad spec; messages to out, prefixed with name */
int hunk_check(const struct loadfile *lf, int nspecs, char **specs, const char *name, FILE *out);

#endif
