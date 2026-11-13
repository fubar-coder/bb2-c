/*
 * tedfont.h - TED-C's 256-glyph screen fonts from a glyph text source and
 * TED 1.1's font files (spec tooling/ted-font).
 *
 * A font is a raw 1-bit picture 128 pixels (16 bytes) wide in 16 strips of
 * 16 glyphs, h rows per strip (h = 8 or 6): row r of glyph c is the byte
 * (c & 15) + (c >> 4) * 16 * h + r * 16, bit 7 the left pixel.  TED 1.1's
 * chars8.b and chars.b are the strips 0-7 (glyphs 0-127); the text source
 * gives the glyphs $80-$FF and, at h = 6, may give glyph 26 again.
 *
 * Text source: lines starting with ';' are comments, blank lines are
 * ignored; first "h=8" or "h=6", then per glyph "@XX" (two hexadecimal
 * digits; whitespace and a ';' comment may follow) and h rows of exactly 8
 * characters '#' (set) and '.' (blank).
 */
#ifndef TEDFONT_H
#define TEDFONT_H

#include <stddef.h>
#include <stdint.h>

/** @brief The glyph TED-C may redraw: 26 (TED's a with diaeresis) at h = 6. */
#define TEDFONT_REDRAWN 26

/** @brief A parsed glyph text source. */
struct tedfont_src {
    int height;                 /* 8 or 6 */
    uint8_t glyph[256][8];      /* the rows of each glyph given */
    int line[256];              /* the line of its block, 0 if not given */
};

/**
 * @brief Parse a glyph text source.
 * @param text    the source
 * @param len     its length
 * @param name    its name for the messages ("NAME:LINE: ...")
 * @param src     the result
 * @param err     the message on failure
 * @param errlen  the size of err
 * @return 0, or -1 with err set: no height line, a bad block line, a code
 *         other than $80-$FF (and 26 at h = 6), a code given twice, a row
 *         that is not 8 characters of '#' and '.', a block with too few or
 *         too many rows, a code of $80-$FF not given
 */
int tedfont_parse(const char *text, size_t len, const char *name, struct tedfont_src *src,
                  char *err, size_t errlen);

/** @brief The size of a font of height h: 256 glyphs (the archive file: half). */
static inline size_t tedfont_size(int height)
{
    return (size_t)256 * (size_t)height;
}

/** @brief The offset of row r of glyph c in a font of height h. */
static inline size_t tedfont_at(int height, unsigned c, unsigned r)
{
    return (c & 15) + (c >> 4) * 16 * (size_t)height + r * 16;
}

/**
 * @brief Build the font: the archive file's 128 glyphs, glyph 26 if the
 *        source gives it, the glyphs $80-$FF of the source.
 * @param src      the parsed source
 * @param archive  the archive file (chars8.b or chars.b)
 * @param archlen  its length: 128 * height
 * @param out      the font, tedfont_size(src->height) bytes
 * @param err      the message on failure
 * @param errlen   the size of err
 * @return 0, or -1 with err set (an archive file of another size)
 */
int tedfont_build(const struct tedfont_src *src, const uint8_t *archive, size_t archlen,
                  uint8_t *out, char *err, size_t errlen);

/**
 * @brief Check a font file against its inputs (the check of the build
 *        step): its size; glyphs 0-127 equal the archive file's, except
 *        glyph 26 at h = 6 when the source gives it; glyphs 26-31 at h = 8
 *        and 27-31 at h = 6 the archive's; every glyph $80-$FF the
 *        source's.
 * @param src      the parsed source
 * @param archive  the archive file
 * @param archlen  its length
 * @param font     the font file
 * @param fontlen  its length
 * @param err      the message of the first difference
 * @param errlen   the size of err
 * @return 0, or -1 with err set
 */
int tedfont_check(const struct tedfont_src *src, const uint8_t *archive, size_t archlen,
                  const uint8_t *font, size_t fontlen, char *err, size_t errlen);

#endif
