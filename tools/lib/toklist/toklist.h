/*
 * toklist - the compiler's own token list (programs/blitz2/src/toklist.h)
 * read as text, and its BASIC keywords as JSON for the website
 * (site/data/compiler/blitz2.json, the grammar of the Blitz Basic code
 * blocks; tools/toklist).
 *
 * toklist.h is an X-macro list, one entry per token node:
 *
 *     TK(id, next, num, sym, name, help)       a BASIC keyword
 *     TA(id, next, num, sym, name, asment...)  an assembler mnemonic
 *     TKLAST(id, num, sym, name, help)         the last node of blitz2's list
 *     TK3(id, next, num, sym, name, help)      a node only AmiBlitz3's list
 *     TK3LAST(id, num, sym, name, help)        has, after TKLAST
 *
 * name and help are a C string literal or M("blitz2's", "AmiBlitz3's").
 * Between the entries only blanks and comments (C and C++ style).
 * tl_parse() reads every entry with its kind, blitz2's name and help (the
 * first of an M) and its line; it fails for anything else at the top
 * level, a wrong number of arguments, a name or help that is no string
 * literal or M of two, an empty name, a string escape other than \" \\ \'
 * \n \t, an unterminated comment, string or entry, and an order other than
 * TK and TA entries, one TKLAST, then TK3 entries and at most one TK3LAST.
 *
 * tl_json() writes blitz2's keywords: the TK entries and the TKLAST one in
 * the order of the list (the TA mnemonics and AmiBlitz3's TK3 nodes are
 * not keywords of blitz2's BASIC); tl_compare() compares a committed file
 * with it line by line.
 */
#ifndef TL_TOKLIST_H
#define TL_TOKLIST_H

#include <stddef.h>

enum tl_kind { TL_TK, TL_TA, TL_TKLAST, TL_TK3, TL_TK3LAST };

struct tl_entry {
    enum tl_kind kind;
    char *name;                         /* blitz2's name, decoded */
    char *help;                         /* blitz2's help ("" for a TA entry) */
    unsigned line;                      /* line of the macro's name */
};

struct tl_list {
    struct tl_entry *v;
    size_t n, cap;
};

/* the entries of TEXT (toklist.h) into L (empty before); 0, or -1 with ERR
   ("line N: ...", or "out of memory") and L emptied */
int tl_parse(struct tl_list *l, const char *text, char *err, size_t errlen);

/* how many entries have the kind */
size_t tl_count(const struct tl_list *l, enum tl_kind kind);

/* the keywords of blitz2 (TK and TKLAST entries) as JSON: an object with
   "compiler": "blitz2", "generated": ABOUT, "source": SOURCE and
   "keywords": [{"name": ..., "help": ...}, ...], one keyword per line, a
   newline at the end; NULL when out of memory.  The caller frees it. */
char *tl_json(const struct tl_list *l, const char *about, const char *source);

/* the committed file WANT (WANTLEN bytes, named FILE in the messages)
   against the generated text GOT, line by line: 0 equal; 1 different, with
   *REPORT (the caller frees it) naming each differing line, at most ten,
   "FILE:LINE: differs from the generated file:" with the committed and the
   generated line, "FILE:LINE: missing:" with the generated line when the
   file ends earlier, "FILE:LINE: not in the generated file:" when it goes
   on, then "FILE: lines that differ from the generated file: N"; -1 out of
   memory */
int tl_compare(const char *want, size_t wantlen, const char *got, const char *file, char **report);

void tl_free(struct tl_list *l);

#endif
