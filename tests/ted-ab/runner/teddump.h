/*
 * teddump.h - screen dumps of teddrive (`dump NAME` -> NAME.scr): read,
 * decode to text, compare (bb2 ted-c/tools/teddump.py).
 *
 * Format (amiga/teddrive.c): text header "key value" lines (magic TEDSCR1,
 * width, height, depth, bytesperrow, rows, rowbytes, interleaved, modeid,
 * mousex, mousey, memtype, title, windows N + N lines "window L T W H
 * FLAGS TITLE"), a line "data", then depth x rows x rowbytes bytes (plane 0
 * first).  Numbers are decimal or $hex.
 *
 * TED's screen (src/screen.c calcscreen/print4): editor rows from pixel row
 * TOPSKIP = 12, fonthite (8 or 6) pixel rows each, one byte column per
 * character; a glyph row is (font[(c & 15) + (c >> 4) * 16 * fonthite +
 * r * 16] & mask(plane)) ^ inverse, mask = $FF if the colour has that
 * plane's bit.  The status line is plane 1 only, raw glyphs (qprint), at
 * pixel row 12 + edrows * fonthite + 9; its field "Free" (printmem, column
 * 38, 5 characters) is masked for comparisons.
 */
#ifndef TEDDUMP_H
#define TEDDUMP_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TEDDUMP_TOPSKIP  12
#define TEDDUMP_BOTSKIP  8
#define TEDDUMP_FREE_COL 38
#define TEDDUMP_FREE_LEN 5

struct teddump_window {
    long left, top, width, height, flags;
    char *title;
};

struct teddump {
    long width, height, depth, bytesperrow, rows, rowbytes, interleaved;
    long modeid, mousex, mousey, memtype;
    char *title;
    struct teddump_window *windows;
    size_t nwindows;
    uint8_t **planes;           /* depth planes of rows * rowbytes bytes */
};

/* the two fonts of TED-C (ted_font8.b: 8 pixel rows, ted_font6.b: 6),
   256 characters: the 128 of TED's chars8.b / chars.b and 128 for the
   literal characters (programs/ted/fonts, spec editor/screen-font) */
#define TEDDUMP_GLYPHS 256
struct teddump_fonts {
    uint8_t f8[TEDDUMP_GLYPHS * 8], f6[TEDDUMP_GLYPHS * 6];
};

/* 0 = ok, -1 = broken (message in err; d empty) */
int teddump_parse(const uint8_t *data, size_t len, struct teddump *d, char *err, size_t errlen);
int teddump_load(const char *path, struct teddump *d, char *err, size_t errlen);
void teddump_free(struct teddump *d);
/* fonts from DIR/ted_font8.b and DIR/ted_font6.b (the build directory's
   programs/ted/fonts); 0 = ok */
int teddump_load_fonts(const char *dir, struct teddump_fonts *f);

/* colour index of a pixel (bit 7 of a byte = leftmost) */
unsigned teddump_pixel(const struct teddump *d, long x, long y);
/* SHA-256 of the planes (64 hex digits) */
void teddump_digest(const struct teddump *d, char hex[65]);

struct teddump_geometry {
    long edrows, cols, top, status;
};
void teddump_geometry(const struct teddump *d, int fh, struct teddump_geometry *g);

/* the decoded screen as text (malloc'd, NULL = out of memory or depth <
   2): a "#" line with geometry and title, one per window, then per editor
   row "NNN |text" (and "    @attrs" if not all normal colour 1), at the
   end "status |text".  Characters: printable as is, others '~', unknown
   cells as a middle dot (UTF-8).  Attributes: '1'-'3' colour, 'a'-'c'
   inverse colour, ' ' blank, '#' inverse blank, '?' unknown. */
char *teddump_text(const struct teddump *d, const struct teddump_fonts *f, int fh);

/* compares a and b (status field "Free" masked unless no_mask): 0 = same,
   1 = different (number of differing bytes and the differing text rows
   to out; "not comparable" if the geometry differs) */
int teddump_compare(const struct teddump *a, const struct teddump *b,
                    const struct teddump_fonts *f, int fh, int no_mask, FILE *out);

#endif
