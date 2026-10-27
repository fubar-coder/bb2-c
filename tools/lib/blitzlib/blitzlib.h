/*
 * blitzlib.h - Blitz Basic 2 library files: reading the header and the
 * token table (layout: blitz2.s fixlibs, Compiler-Spec 9.2), and the
 * checks of a library file built from C (the former mkblitzlib.py of bb2).
 */
#ifndef BLITZLIB_H
#define BLITZLIB_H

#include <stdio.h>
#include "hunk.h"

struct blitzlib_entry {
    uint32_t offset;            /* of the entry in the code */
    uint16_t kind;              /* 8 dumtoke, &4 AmigaOS call, &$F 0 nullsub,
                                   else statement/function/command */
    const char *name;           /* inside the code (0-terminated); NULL for nullsubs */
    const char *help;
};

struct blitzlib {
    const uint8_t *code;        /* the code hunk */
    uint32_t size;
    uint16_t libnum;            /* header +4 */
    uint32_t error;             /* header +$2C: start of the error check area, 0 = none */
    unsigned nentries;
    struct blitzlib_entry *entries;
    uint32_t libfin;            /* offset of !libfin's toke field, 0 if the long is 0 */
    uint32_t codestart;         /* first byte behind the token table */
};

/* reads header and token table of the code hunk h (bounds-checked);
   0 = ok, -1 = broken (message in err, lib empty) */
int blitzlib_read(const struct hunk *h, struct blitzlib *lib, char *err, size_t errlen);
void blitzlib_free(struct blitzlib *lib);

/* the checks of a library file:
 *   - one hunk, code, relocations only into that hunk
 *   - starts with the library header: MOVEQ #0,d0:RTS (libmacs) or 4 zero
 *     bytes (AmigaOS call libraries of deflibs)
 *   - header and token table read (blitzlib_read)
 *   - warning (no error): a relocation from the normal code into the error
 *     check area (that area is cut away when the debugger is off; the
 *     originals do it too, e.g. memlib err1 -> mess1)
 * 0 = ok, 1 = error; errors and warnings go to out, prefixed with name */
int blitzlib_check(const struct loadfile *lf, const char *name, FILE *out);

#endif
