/*
 * bbdetok.h - a tokenised Blitz 2 / AmiBlitz source (docs/formats.md §3)
 * as plain text, line for line: for reading and diffing sources written
 * with other editors and libraries (the snapshots in archive/sources/).
 *
 * The file is its lines one after the other, each ended by a 0 byte.  A
 * byte with bit 7 set starts a token word: high byte first, the number of
 * a token list entry (bit 15 set, tools/lib/blitztokens).  A token's
 * second byte never ends the line, even if it is 0 (as TED's readline and
 * AmiBlitz3's detokeniser read it).  Where a byte with bit 7 is a token
 * depends on the editor that wrote the file:
 *
 *   BBDETOK_AB   PED, AmiBlitz 2 and AmiBlitz3 (after AmiBlitz3's
 *                preproc_Detokenize, Shared/preprocessor.include.ab3):
 *                strings ("..." up to the next " or the end of the line)
 *                and comments (; outside a string up to the end of the
 *                line) are copied as they are, their bytes from $80 are
 *                characters (ISO 8859-1: the umlauts of German texts).
 *                Unlike AmiBlitz3, a comment may have tokens: a line
 *                commented out by typing a ; in front of it keeps them.
 *                In a comment a byte from $80 and the next one (not 0)
 *                are a token if the list has that number, else the byte
 *                is a character - a guess: an umlaut and the character
 *                after it can make a known number.
 *   BBDETOK_TED  TED 1.1 (ted.s:3455 detok): every byte from $80 starts a
 *                token, in strings and comments too.
 *
 * A token is written as the name of its number (the first entry with that
 * number in the list), a number the list does not have as "{L:N}": L =
 * (word & $7FFF) >> 7 and N = word & $7F, decimal - the library number
 * and, in a user library, its entry counted from 1 (the word is
 * $8000 + L*128 + N; the compiler's own tokens are L 0 and 1, a system
 * library counts from its own number, bit 15 set).  In Blitz code "{"
 * only follows a name (arguments), so "{L:N}" cannot be source text
 * outside strings and comments.  Every other byte is written as it is
 * (TAB, control characters); each line ends with an LF.
 */
#ifndef BBDETOK_H
#define BBDETOK_H

#include <stddef.h>
#include <stdint.h>
#include "blitztokens.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The rule sets (see the top). */
enum bbdetok_rules {
    BBDETOK_AB,                 /* strings and comments as they are */
    BBDETOK_TED                 /* every byte from $80 a token */
};

/** @brief The names by token number (bbdetok_free). */
struct bbdetok {
    const char **name;          /* [word & $7FFF]: NULL = unknown */
};

/** @brief What bbdetok_text found. */
struct bbdetok_result {
    size_t lines;               /* lines written */
    size_t tokens;              /* token words */
    size_t unknown;             /* of them not in the list */
    size_t comment;             /* of them in comments (BBDETOK_AB) */
    size_t kinds;               /* different unknown numbers */
    uint32_t count[32768];      /* [word & $7FFF]: how often unknown */
    int open;                   /* the last line had no 0 (it is written) */
};

/**
 * @brief Set up the names from a token list: the first entry of a number
 *        wins (as TED's findtoke).
 * @param d     the table
 * @param list  the tokens; the names must stay
 * @return 0 = ok, -1 = out of memory
 */
int bbdetok_init(struct bbdetok *d, const struct blitztokens *list);

/** @brief Free the table (not the names). */
void bbdetok_free(struct bbdetok *d);

/**
 * @brief The text of a tokenised source.
 * @param d       the names
 * @param rules   an enum bbdetok_rules
 * @param src     the file
 * @param len     its size
 * @param out     out: the text (malloc'd, free it; NULL on an error)
 * @param outlen  out: its size
 * @param r       out: the counts (filled in also on an error)
 * @param err     the message of an error ("line N: ...")
 * @param errlen  the size of err
 * @return 0 = ok, -1 = error: a token byte as the last byte of the file
 *         (no second byte), out of memory, unknown rules
 */
int bbdetok_text(const struct bbdetok *d, int rules, const uint8_t *src, size_t len,
                 char **out, size_t *outlen, struct bbdetok_result *r, char *err,
                 size_t errlen);

#ifdef __cplusplus
}
#endif

#endif
