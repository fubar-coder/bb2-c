/*
 * mktestdata.h - binary test data (8SVX, ILBM, MED and tracker modules)
 * from a small text description, deterministically.  Used for the
 * bug-orig tests (tests/bug-orig/AREA/NAME.spec -> testdata/NAME).
 *
 * The description is a list of lines; ';' or '#' starts a comment.
 * Values are big-endian (68000):
 *   b V...          bytes
 *   w V...          words (16 bit)
 *   l V...          longs (32 bit)
 *   s "TEXT"        the characters of TEXT (no terminator; \" \\ \n \0)
 *   fill N V        N bytes of value V
 *   align N         zero bytes up to a multiple of N (from the start)
 *   NAME:           a label: the offset at this point
 *   form TYPE [LEN] an IFF FORM: "FORM", its length, TYPE
 *   chunk ID [LEN]  an IFF chunk: ID, its length (an ID with a blank in
 *                   quotes: '(c) ')
 *   end             closes the innermost form/chunk: its length is
 *                   written (the bytes behind the length field) and an
 *                   odd length gets a pad byte; with LEN that length is
 *                   written instead (e.g. a BODY that claims more than
 *                   it holds), the pad byte still follows the real data
 * A value V is an expression: terms joined by '+' and '-'; a term is a
 * number (decimal, $hex or 0xhex), 'ABCD' (1-4 characters, big-endian),
 * or @NAME (the offset of a label, also one defined later).  The counts
 * of fill and align must not refer to later labels.  A value must fit
 * its size (signed or unsigned).
 */
#ifndef MKTESTDATA_H
#define MKTESTDATA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* builds the data of the description `spec` (0-terminated text): 0 = ok,
   *out (malloc'd, free it) with *len bytes; -1 = error, message in err
   ("line N: ...") */
int mktestdata_build(const char *spec, uint8_t **out, size_t *len, char *err, size_t errlen);

#ifdef __cplusplus
}
#endif

#endif
