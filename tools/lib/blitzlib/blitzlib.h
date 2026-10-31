/*
 * blitzlib.h - Blitz Basic 2 library files: reading the header and the
 * token table (layout: blitz2.s fixlibs, Compiler-Spec 9.2), and the
 * checks of a library file built from C (the former mkblitzlib.py of bb2).
 */
#ifndef BLITZLIB_H
#define BLITZLIB_H

#include <stdio.h>
#include "hunk.h"

/** @brief An entry of the token table. */
struct blitzlib_entry {
    uint32_t offset;            /* of the entry in the code */
    uint16_t kind;              /* 8 dumtoke, &4 AmigaOS call, &$F 0 nullsub,
                                   else statement/function/command */
    const char *name;           /* inside the code (0-terminated); NULL for nullsubs */
    const char *help;
};

/** @brief A library as read by blitzlib_read (blitzlib_free). */
struct blitzlib {
    const uint8_t *code;       /* the code hunk */
    uint32_t size;
    uint16_t libnum;            /* header +4 */
    uint32_t error;             /* header +$2C: start of the error check area, 0 = none */
    unsigned nentries;
    struct blitzlib_entry *entries;
    uint32_t libfin;            /* offset of !libfin's toke field, 0 if the long is 0 */
    uint32_t codestart;         /* first byte behind the token table */
    uint8_t *mask;              /* per byte of [0, codestart): BLITZLIB_* */
};

/** @brief The declaration part (header, token table, !libfin) byte by
    byte, for comparing two builds of a library (blitzlib_compare). */
enum {
    BLITZLIB_DECL,              /* declaration: must be equal */
    BLITZLIB_IGNORE,            /* filled in at run time (next.l, nummer.w,
                                   the long behind the entry kind) or padding */
    BLITZLIB_CODE               /* a code offset (long): only set or not */
};

/**
 * @brief Read header and token table of a library's code hunk
 *        (bounds-checked).
 * @param h       the code hunk
 * @param lib     out: the library (empty on an error)
 * @param err     the message of an error
 * @param errlen  the size of err
 * @return 0 = ok, -1 = broken
 */
int blitzlib_read(const struct hunk *h, struct blitzlib *lib, char *err, size_t errlen);

/** @brief Free what blitzlib_read filled in. */
void blitzlib_free(struct blitzlib *lib);

/**
 * @brief The checks of a library file.
 *
 *   - one hunk, code, relocations only into that hunk
 *   - starts with the library header: MOVEQ #0,d0:RTS (libmacs) or 4 zero
 *     bytes (AmigaOS call libraries of deflibs)
 *   - header and token table read (blitzlib_read)
 *   - warning (no error): a relocation from the normal code into the error
 *     check area (that area is cut away when the debugger is off; the
 *     originals do it too, e.g. memlib err1 -> mess1)
 *
 * @param lf    the library file
 * @param name  the prefix of the messages
 * @param out   the errors and warnings
 * @return 0 = ok, 1 = error
 */
int blitzlib_check(const struct loadfile *lf, const char *name, FILE *out);

/**
 * @brief Compare the declaration parts of two builds of a library.
 *
 * The rules of bb2 biglib.py compare: header, entries (kind, forms with
 * argument types, libs, names, help texts, AmigaOS calls with registers),
 * !libfin; code offsets only set/not set; the number of relocations inside
 * the declaration part.
 *
 * @param orig  the code hunk of one build
 * @param new   the code hunk of the other
 * @param name  the prefix of the messages
 * @param out   the differences
 * @return the number of differences (-1 = a library does not read,
 *         message to out)
 */
int blitzlib_compare(const struct hunk *orig, const struct hunk *new, const char *name, FILE *out);

#endif
