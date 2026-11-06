/*
 * cmdref.h - the command table of a Blitz Basic 2 library file as JSON,
 * the data of the command reference of the website
 * (site/data/libraries/GROUP/NAME.json, rendered by Hugo).
 *
 * The file is meant to be diffed and compared byte by byte: fixed key
 * order, two-space indentation, one line per form, ASCII only (every
 * byte outside printable ASCII escaped as \u00XX, the help texts are at
 * most Latin-1), a final newline.
 *
 *   {
 *     "name": "doslib", "group": "deflibs", "number": 252,
 *     "opens": "dos.library",            AmigaOS call libraries with an init
 *     "object": {"name": "Window", "maximum": 20},     object libraries
 *     "entries": [ ... in table order, nullsubs left out ... ]
 *   }
 *
 * An entry: "name", "kind" (statement, function, command, object,
 * keyword, oscall), "typed" (true: the command takes a type suffix,
 * `Peek.w`), "help" (as the library holds it), "result" (the type of a
 * function or command), "forms" (statement/function/command): per form
 * "args", each {"type": T, "code": BYTE[, "flags": [...]]}, and "repeat"
 * {"from": I, "count": N} of a repetition group (`!repargs`); an
 * AmigaOS call: "base", "offset", "registers".
 * Types: byte word long quick float string, any (type 8: the type is
 * passed along), suffix (type 0: the command's type suffix), array (an
 * array, `a()`); flags: address (the address of a variable), stack (the
 * value is pushed), arraytype (an array's type is passed), arrayend (an
 * array's end is passed).
 */
#ifndef CMDREF_H
#define CMDREF_H

#include <stdio.h>
#include "blitzlib.h"

/**
 * @brief The name of the AmigaOS library or resource an AmigaOS call
 *        library opens (its init's `libname[]`).
 *
 * Only for an AmigaOS call library (the header starts with 4 zero
 * bytes): the one 0-terminated printable string in the code behind the
 * token table that ends in ".library", ".resource", ".device" or
 * ".gadget".
 *
 * @param lib     the library (blitzlib_read)
 * @param name    out: the name inside the code, NULL if there is none
 * @param err     the message of an error
 * @param errlen  the size of err
 * @return 0 = ok, -1 = more than one such string
 */
int cmdref_opens(const struct blitzlib *lib, const char **name, char *err, size_t errlen);

/**
 * @brief Write the JSON of a library (format: see the top of cmdref.h).
 * @param out    the file
 * @param lib    the library (blitzlib_read)
 * @param group  the value of "group" (acidlibs, deflibs, userlibs)
 * @param name   the value of "name" (the library's file name without .obj)
 * @param opens  cmdref_opens's name, or NULL
 * @return 0 = ok, -1 = write error (errno)
 */
int cmdref_write(FILE *out, const struct blitzlib *lib, const char *group, const char *name,
                 const char *opens);

#endif
