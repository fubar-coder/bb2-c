/*
 * tokenizer.h - turning source text into tokenised lines, as an object
 * with a rule set (design D2 of the change ab3-mode-g2, spec
 * ab3-compat/tokenizer).
 *
 * A struct tokenizer holds a token list (names and token numbers, in the
 * order the compiler hands them to the editor: its own tokens, then the
 * libraries' - the first entry of a name wins) and a rule set:
 *
 *   TOKENIZER_RULES_AB3  AmiBlitz3 v3.14.0's preproc_Tokenize
 *                        (Sourcecodes/Amiblitz3/Shared/
 *                        preprocessor.include.ab3:358 at 58a11658): names
 *                        match byte for byte (case-sensitive), only whole
 *                        words that start after a separator, "name$"
 *                        before "name", strings and comments copied
 *                        untouched.
 *
 * (A rule set for TED's tokeniser, ted.s:4198 tokit, is reserved for the
 * change that lets TED use this component; TED keeps its own for now.)
 *
 * The functions around it follow AmiBlitz3's way of loading a source
 * (CLIModeSupport.ab3 LoadSource, AmiBlitz3.ab3 read_includeline): the end
 * of the embedded header of a main source and of an include, and the
 * lines of a tokenised main source, cut at 250 characters.
 *
 * Shared code like shared/xtra: every user compiles tokenizer.c itself
 * with its own options (ab3cli with TED's, the host tools natively).  C99
 * on the freestanding headers: no libc, no libgcc (no 32-bit multiply or
 * divide on the 68000), no writable static data, no allocation - the
 * caller gives the memory of the object.
 */
#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The rule sets. */
enum tokenizer_rules {
    TOKENIZER_RULES_AB3 = 1     /* AmiBlitz3 v3.14.0 (preproc_Tokenize) */
};

/** @brief Results of the functions (TOKENIZER_OK = 0). */
enum tokenizer_err {
    TOKENIZER_OK,
    TOKENIZER_DUPLICATE,        /* tokenizer_add: the name is there already,
                                   the first entry stays (not an error) */
    TOKENIZER_E_RULES,          /* not an enum tokenizer_rules */
    TOKENIZER_E_BUCKETS,        /* bucket count not a power of 2 (1..65536) */
    TOKENIZER_E_FULL,           /* no entry left */
    TOKENIZER_E_NAME,           /* empty name, or longer than 65535 bytes */
    TOKENIZER_E_SPACE           /* the output buffer is too small */
};

/** @brief Characters of a line of a main source AmiBlitz3 keeps
    (#MAX_Columns, Shared/common.ab3:96). */
#define TOKENIZER_AB3_COLUMNS 250

/** @brief Bytes tokenizer_text and tokenizer_line may write for len bytes
    of text: a name of one letter becomes a 2-byte token, a string or a
    comment at the end of the text gets its 0. */
#define TOKENIZER_BOUND(len) (2 * (size_t)(len) + 2)

/** @brief One name of the token list (the caller's memory). */
struct tokenizer_entry {
    const uint8_t *name;        /* the name's bytes (no 0 needed); must stay */
    uint16_t len;               /* its length */
    uint16_t num;               /* the token: written high byte first */
    uint16_t next;              /* the next entry of its bucket + 1, 0 = none */
};

/** @brief The tokeniser: the token list as a hash table over the caller's
    arrays, and the rule set.  Read-only while tokenising. */
struct tokenizer {
    struct tokenizer_entry *entry;
    uint16_t *bucket;           /* first entry + 1 of each bucket, 0 = none */
    uint32_t n, cap;            /* entries used, entries there are */
    uint32_t mask;              /* buckets - 1 */
    uint8_t rules;              /* enum tokenizer_rules */
};

/**
 * @brief Set up an empty tokeniser.
 * @param t        the object
 * @param rules    an enum tokenizer_rules
 * @param entry    room for cap names (at most 65535)
 * @param cap      the entries there are
 * @param bucket   the hash buckets: nbucket words
 * @param nbucket  a power of 2, 1..65536
 * @return TOKENIZER_OK, TOKENIZER_E_RULES or TOKENIZER_E_BUCKETS
 */
int tokenizer_init(struct tokenizer *t, int rules, struct tokenizer_entry *entry, uint32_t cap,
                   uint16_t *bucket, uint32_t nbucket);

/**
 * @brief Add a name to the token list, in the compiler's order.
 * A name that is there already keeps its first token (AmiBlitz3's
 * preproc_QueryTokens): the call changes nothing.
 * @param t     the tokeniser
 * @param name  the name's bytes (kept by reference)
 * @param len   its length
 * @param num   the token number (bit 15 set as in a tokenised source, or
 *              not: it is written as given)
 * @return TOKENIZER_OK, TOKENIZER_DUPLICATE, TOKENIZER_E_NAME or
 *         TOKENIZER_E_FULL
 */
int tokenizer_add(struct tokenizer *t, const uint8_t *name, size_t len, uint16_t num);

/**
 * @brief Look a name up, byte for byte.
 * @param t     the tokeniser
 * @param name  the bytes
 * @param len   their number
 * @param num   the token if found (may be NULL)
 * @return 1 if found, 0 if not
 */
int tokenizer_find(const struct tokenizer *t, const uint8_t *name, size_t len, uint16_t *num);

/**
 * @brief Tokenise a text as AmiBlitz3 tokenises a whole source buffer.
 * Every LF (and every 0 byte) of the text ends a line with a 0; a TAB
 * outside strings and comments becomes a space, CR, DEL and the other
 * control characters are dropped, a byte from 128 becomes '?'; inside a
 * string or a comment every byte stays except TAB (a space).  A word that
 * ends the text (no character behind it) is not looked up, a string or
 * comment that ends it gets its 0 (AmiBlitz3 reads the 0 behind its
 * buffer).
 * @param t    the tokeniser
 * @param src  the text
 * @param len  its length
 * @param dst  the output (not overlapping src)
 * @param cap  its size; TOKENIZER_BOUND(len) is always enough
 * @param out  the bytes written
 * @return TOKENIZER_OK, TOKENIZER_E_SPACE or TOKENIZER_E_RULES
 */
int tokenizer_text(const struct tokenizer *t, const uint8_t *src, size_t len, uint8_t *dst,
                   size_t cap, size_t *out);

/**
 * @brief Tokenise one line: as tokenizer_text of the line followed by its
 *        LF, so the output ends with the line's 0.
 * @param t    the tokeniser
 * @param src  the line without its LF
 * @param len  its length
 * @param dst  the output (not overlapping src)
 * @param cap  its size; TOKENIZER_BOUND(len) is always enough
 * @param out  the bytes written, the 0 included
 * @return as tokenizer_text
 */
int tokenizer_line(const struct tokenizer *t, const uint8_t *src, size_t len, uint8_t *dst,
                   size_t cap, size_t *out);

/**
 * @brief The end of the embedded header of a main source, as AmiBlitz3's
 *        ParseAB3Header finds it (CLIModeSupport.ab3:576): the lines up to
 *        the first one whose letters, '_', '/' and '-' before a '=' read
 *        "/xtra" in any case, that line with its LF included.
 * @param src  the source text
 * @param len  its length
 * @return the bytes of the header, 0 if there is no such line
 */
size_t tokenizer_ab3_header(const uint8_t *src, size_t len);

/**
 * @brief The end of the embedded header of an include, as AmiBlitz3's
 *        SkipXtraHeader finds it (CLIModeSupport.ab3:401): leading lines
 *        with a ';' up to a "/xtra", "/XTRA" or "/Xtra", that line with
 *        its LF included.
 * @param src  the include's text
 * @param len  its length
 * @return the bytes of the header, 0 if there is none (also if a line
 *         without ';' comes first or the end is reached)
 */
size_t tokenizer_ab3_incheader(const uint8_t *src, size_t len);

/**
 * @brief The lines a header of len bytes has (each LF ends one, a last
 *        line without LF counts).
 */
size_t tokenizer_lines(const uint8_t *src, size_t len);

/**
 * @brief One line of a tokenised main source as AmiBlitz3's LoadSource
 *        takes it (CLIModeSupport.ab3:480): the bytes from pos up to the
 *        next 0 or the end, of which at most TOKENIZER_AB3_COLUMNS are
 *        kept.
 * @param tok   the tokenised text (tokenizer_text)
 * @param len   its length
 * @param pos   the start of the line (< len)
 * @param keep  the bytes of the line kept (without its 0)
 * @return the start of the next line (len after the last one)
 */
size_t tokenizer_ab3_mainline(const uint8_t *tok, size_t len, size_t pos, size_t *keep);

#ifdef __cplusplus
}
#endif

#endif
