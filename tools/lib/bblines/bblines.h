/*
 * bblines.h - edits of the lines of a tokenised Blitz Basic 2 source, on
 * its bytes (a text round trip through TED would tokenise again with
 * another token list than the author's and change other lines).
 *
 * A tokenised source (TED's format): lines ended by a 0 byte; a byte with
 * bit 7 set starts a token word (two bytes: the second byte may be
 * anything, also 0); text in "..." is plain bytes.
 */
#ifndef BBLINES_H
#define BBLINES_H

#include <stddef.h>
#include <stdint.h>

/* every line whose first statement (after spaces and tabs) is the token
   word TOKEN (bit 15 set, e.g. $80BF INCDIR) becomes ";" - an empty
   comment, the line numbers stay.  Such a line must hold only that
   statement (no ':' outside quotes, else it is an error: the rest would
   go too).  The result is malloc'd (*out, *outlen); *count = the lines
   changed.  0 = ok, -1 = error (err: not tokenised, a line without its
   end, a token without its second byte, a line with more statements) */
int bblines_comment(const uint8_t *src, size_t len, uint16_t token, uint8_t **out,
                    size_t *outlen, size_t *count, char *err, size_t errlen);

#endif
