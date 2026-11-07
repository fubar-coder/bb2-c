/*
 * blitzlib.h - Blitz Basic 2 library files: reading the header and the
 * token table (layout: blitz2.s fixlibs), and the
 * checks of a library file built from C (the former mkblitzlib.py of bb2).
 */
#ifndef BLITZLIB_H
#define BLITZLIB_H

#include <stdio.h>
#include "hunk.h"

/** @brief A form of a statement, function or command (`!args`,
    `!repargs`): the arguments it takes. */
struct blitzlib_form {
    uint16_t head;              /* first<<12 | num<<8 | count: count arguments,
                                   of them num from index first repeated
                                   (`!repargs`; 0 = no repetition) */
    const uint8_t *types;       /* the count argument types, inside the code
                                   (blitzlib.h of the libraries: BL_WORD ...
                                   with the flags BL_VARPTR ...) */
};

/** @brief An entry of the token table. */
struct blitzlib_entry {
    uint32_t offset;            /* of the entry in the code */
    uint16_t kind;              /* 8 dumtoke, &4 AmigaOS call, &$F 0 nullsub,
                                   else statement/function/command (low 2 bits
                                   1/2/3, +8 typed by the suffix, the high
                                   byte the result type) */
    const char *name;           /* inside the code (0-terminated); NULL for nullsubs */
    const char *help;
    unsigned form;              /* statement/function/command: the first of
                                   its forms in blitzlib.forms */
    unsigned nforms;            /*   and their number */
    uint16_t oslib;             /* AmigaOS call: the library whose init result is the base */
    int16_t lvo;                /*   the offset of the call */
    const uint8_t *regs;        /*   the argument registers (bit 4 address
                                     register, bits 0-2 the number), inside
                                     the code */
    unsigned nregs;             /*   their number */
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
    const char *object;         /* !libfin: the name its toke points to (the
                                   object of an object library), NULL if
                                   there is none or it is not a string */
    uint16_t defmax;            /* !libfin: the default maximum of the objects */
    unsigned nforms;
    struct blitzlib_form *forms;    /* the forms of all entries */
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
 *   - no relocation from the normal code into the error check area (that
 *     area is cut away when the debugger is off; the originals do it, e.g.
 *     memlib err1 -> mess1, the port has them fixed since 2026-10-31)
 *
 * @param lf    the library file
 * @param name  the prefix of the messages
 * @param out   the errors
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

/**
 * @brief Split a list of intended differences into its lines (in place).
 *
 * One message line of blitzlib_compare per line, exactly as printed
 * (without the newline); empty lines and lines beginning with `#` are
 * skipped.
 *
 * @param text   the list, 0-terminated; its newlines become 0
 * @param lines  the lines found (pointers into text)
 * @param max    room in lines
 * @return the number of lines, -1 if there are more than max
 */
long blitzlib_intended_list(char *text, const char **lines, size_t max);

/**
 * @brief Take the intended differences out of the messages of
 * blitzlib_compare.
 *
 * A message line equal to one of the intended differences is printed as
 * "intended: LINE" and not counted; the other lines are copied as they
 * are.
 *
 * @param msgs      the messages of blitzlib_compare (lines, 0-terminated)
 * @param intended  the intended differences (blitzlib_intended_list)
 * @param n         their number
 * @param seen      per intended difference: set to 1 when found
 * @param out       the other lines and the intended ones found
 * @return the number of the other lines (differences not intended)
 */
int blitzlib_intended(const char *msgs, const char *const *intended, size_t n,
                      unsigned char *seen, FILE *out);

/**
 * @brief Report the intended differences that did not occur (stale
 * entries of the list).
 *
 * Prints "intended difference not found: LINE" for every entry whose seen
 * flag is still 0.
 *
 * @param intended  the intended differences (blitzlib_intended_list)
 * @param n         their number
 * @param seen      per intended difference: 1 = found (blitzlib_intended)
 * @param out       the messages
 * @return the number of stale entries (each one counts as a difference)
 */
int blitzlib_intended_stale(const char *const *intended, size_t n, const unsigned char *seen,
                            FILE *out);

#endif
