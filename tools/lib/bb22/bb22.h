/*
 * bb22.h - reading the "BB22" source format and writing TED's format.
 *
 * 9 sources of RWE's rwelibsrc (archive/aminet/amibl2src/rwelibsrc,
 * saved June to September 1997 by an unknown editor) begin with "BB22"
 * instead of TED's format (docs/rwelibs/README.md 4.1).  All values are
 * big-endian words:
 *
 *   version 10: "BB22", 10, column, row, 0, top, 2      (16 bytes)
 *   version 8:  "BB22",  8, column, row, 0, top         (14 bytes)
 *
 * then the lines up to the end of the file: a prefix (2 bytes in version
 * 10, 1 byte in version 8; always 0), the tokenised text as in TED's
 * format (a byte with bit 7 set and the byte after it are a token), a 0
 * byte.  column/row/top are the cursor (found by comparing the files
 * with their .bak, README 4.1): the line of the cursor is top + row
 * (0-based), top the first line shown, row the line in the window,
 * column the column of the cursor in the text as shown.
 *
 * The reader is strict: whatever it does not know is refused with a
 * message, nothing is guessed - another version, a word at +10 other
 * than 0 or (version 10) at +14 other than 2, a prefix byte other than 0,
 * a line without its 0 byte before the end of the file, a token byte
 * without its second byte, a token whose second byte is 0 (the end of the
 * line or a token $xx00?), an LF in a line (TED would end the line
 * there).
 *
 * TED's format is the lines one after the other, each ended by a 0 byte.
 */
#ifndef BB22_H
#define BB22_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* one line: its text (without prefix and 0 byte) at buf[offset] */
struct bb22_line {
    size_t offset;
    size_t len;
};

struct bb22 {
    unsigned version;           /* word at +4: 8 or 10 */
    size_t header_size;         /* 14 or 16 */
    size_t prefix_size;         /* bytes before each line: 1 or 2 */
    unsigned column, row, top;  /* words at +6, +8, +12: the cursor */
    size_t nlines;
    struct bb22_line *lines;    /* malloc'd (NULL without lines) */
    size_t max_len;             /* longest line in bytes (tokenised) */
};

/* true if buf begins with "BB22" (says nothing about the rest) */
bool bb22_is(const uint8_t *buf, size_t size);

/* reads the BB22 file buf[0..size): 0 = ok (*out filled, bb22_free it);
   -1 = refused, message in err ("offset N: ..." / "line N: ...") and
   *out empty */
int bb22_parse(const uint8_t *buf, size_t size, struct bb22 *out, char *err, size_t errlen);

void bb22_free(struct bb22 *b);

/* size of the file in TED's format: the lines and a 0 byte each */
size_t bb22_ted_size(const struct bb22 *b);

/* writes the lines of b (parsed from buf) in TED's format to out, which
   has bb22_ted_size(b) bytes */
void bb22_to_ted(const struct bb22 *b, const uint8_t *buf, uint8_t *out);

/* the same to a file: 0 = ok, -1 = write error */
int bb22_write_ted(const struct bb22 *b, const uint8_t *buf, FILE *f);

#ifdef __cplusplus
}
#endif

#endif
