/*
 * blitztokens.h - the token list of Blitz Basic 2 as the compiler builds
 * it and hands it to the editor (edstruct, the list TED tokenises with):
 * the compiler's own tokens (blitz2.s 283 `tokens`: the BASIC keywords and
 * the assembler mnemonics, a linked list of nodes next.l num.w "name",0
 * "help",0, word-aligned in blitz2, not in AmiBlitz 2's compilers: a node
 * may begin at any offset) and then the tokens of the libraries in the order the compiler
 * links them (blitz2.s loadlibs: acidlibs, deflibs, then the libraries of
 * blitz2:userlibs).  The number of a library entry (blitz2.s fixlibs):
 * counted from libnum*128+1 for a user library, from libnum for a system
 * library (bit 15 set); every entry counts, a nullsub has no token.
 * TED shows a token as the word num|$8000.
 *
 * The names point into the files the caller passes: they must stay.
 */
#ifndef BLITZTOKENS_H
#define BLITZTOKENS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/** @brief One token. */
struct blitztoken {
    uint16_t num;               /* as in a tokenised source: bit 15 set */
    const char *name;
};

/** @brief A token list (blitztokens_free). */
struct blitztokens {
    struct blitztoken *t;
    size_t n, cap;
};

/**
 * @brief Add the compiler's own tokens from the compiler's load file (the
 *        list that starts with the node of token 1 "NEWTYPE").
 * @param buf     the load file
 * @param size    its size
 * @param out     the list the tokens are appended to
 * @param err     the message of an error
 * @param errlen  the size of err
 * @return 0 = ok, -1 = error
 */
int blitztokens_compiler(const uint8_t *buf, size_t size, struct blitztokens *out,
                         char *err, size_t errlen);

/**
 * @brief Like blitztokens_compiler, for a compiler with several lists
 *        (blitz2-ab3: blitz2's and AmiBlitz3's, programs/blitz2/src/
 *        toklist.h): the list whose last token is `last`.
 * @param buf     the load file
 * @param size    its size
 * @param last    the number of the list's last token (bit 15 set or not);
 *                0 = the only list, as blitztokens_compiler
 * @param out     the list the tokens are appended to
 * @param err     the message of an error (also: no list or two lists end
 *                with `last`)
 * @param errlen  the size of err
 * @return 0 = ok, -1 = error
 */
int blitztokens_compiler_last(const uint8_t *buf, size_t size, uint16_t last,
                              struct blitztokens *out, char *err, size_t errlen);

/**
 * @brief Add the tokens of a library file or of every library of a BigLib
 *        file (acidlibs, deflibs), in order.
 * @param buf     the file
 * @param size    its size
 * @param out     the list the tokens are appended to
 * @param err     the message of an error
 * @param errlen  the size of err
 * @return 0 = ok, -1 = error
 */
int blitztokens_libraries(const uint8_t *buf, size_t size, struct blitztokens *out,
                          char *err, size_t errlen);

/**
 * @brief Write the list as tests/data/ted-tokens.txt: "XXXX name" per line.
 * @param f     the file
 * @param list  the tokens
 * @return 0 = ok, -1 = write error
 */
int blitztokens_write(FILE *f, const struct blitztokens *list);

/**
 * @brief Add the tokens of a list file as blitztokens_write writes it
 *        (tests/data/ted-tokens.txt): "XXXX name" per line, XXXX four hex
 *        digits with bit 15 set, the name up to the end of the line (LF,
 *        CR LF); empty lines are skipped, anything else is refused.
 * @param buf     the file and one byte more (the 0 behind a last line
 *                without LF); changed: every line end becomes 0, the
 *                names point into it
 * @param size    the file's size
 * @param out     the list the tokens are appended to
 * @param err     the message of an error ("line N: ...")
 * @param errlen  the size of err
 * @return 0 = ok, -1 = error (out keeps the tokens before the line)
 */
int blitztokens_read(char *buf, size_t size, struct blitztokens *out, char *err,
                     size_t errlen);

/** @brief Free the list (not the names: they point into the files). */
void blitztokens_free(struct blitztokens *list);

#endif
