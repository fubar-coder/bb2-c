/*
 * blitzlib.h - checks of a Blitz Basic 2 library file (the former
 * mkblitzlib.py of bb2):
 *   - one hunk, code, relocations only into that hunk
 *   - starts with the library header: MOVEQ #0,d0:RTS (libmacs) or 4 zero
 *     bytes (AmigaOS call libraries of deflibs)
 *   - the token table parses (header, entries, $FFFF, !libfin) inside the
 *     code (layout: blitz2.s fixlibs, Compiler-Spec 9.2)
 *   - warning (no error): a relocation from the normal code into the error
 *     check area (that area is cut away when the debugger is off; the
 *     originals do it too, e.g. memlib err1 -> mess1)
 */
#ifndef BLITZLIB_H
#define BLITZLIB_H

#include <stdio.h>
#include "hunk.h"

/* 0 = ok, 1 = error; errors and warnings go to out, prefixed with name */
int blitzlib_check(const struct loadfile *lf, const char *name, FILE *out);

#endif
