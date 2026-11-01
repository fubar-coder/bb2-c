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

/**
 * @brief Check that the first hunk is code and has the given bytes at the
 *        given offsets.
 * @param lf      the load file
 * @param nspecs  the number of specs
 * @param specs   "OFFSET:HEXBYTES" each (offset decimal or 0x hex, e.g.
 *                "4:48e7")
 * @param name    the prefix of the messages
 * @param out     the messages
 * @return 0 = ok, 1 = mismatch or bad spec
 */
int hunk_check(const struct loadfile *lf, int nspecs, char **specs, const char *name, FILE *out);

/**
 * @brief Check the hunks of a load file: their number, types and chip
 *        memory flags.
 * @param lf      the load file
 * @param layout  the hunks in order, separated by ',': code, data or bss,
 *                each with "+chip" if it must be loaded to chip memory
 *                (e.g. "code,bss,data+chip")
 * @param name    the prefix of the messages
 * @param out     the messages
 * @return 0 = ok, 1 = another layout or a bad layout
 */
int hunk_layout(const struct loadfile *lf, const char *layout, const char *name, FILE *out);

#endif
