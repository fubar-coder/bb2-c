/*
 * blitzres.h - Blitz Basic 2 resident files (blitz2 "Create Resident",
 * blitz2.s:4105 menu5; programs/blitz2/src/stmts.c): writing a resident
 * with constants, reading (dumping) any resident, and the constants of the
 * NDK files (FD files, assembler includes) the residents of RWE's library
 * sources need (docs/rwelibs/README.md 3.1).
 *
 * A resident is a load file with one code hunk (header as blitz2's
 * tsthead):
 *   word ntypes, then per type its fields (var nodes, linked by next)
 *        and the type node behind the last field (fixtypes)
 *   word nmacros, then per macro the node and its text (word aligned)
 *   word nconsts, then the constant nodes (label: next.l refs.l=1
 *        value.l data.l procnum.w len.b name,0; word aligned)
 * The pointers (next, a type's fields, a macro's text) hold offsets in the
 * hunk and are relocated (RELOC32, the offsets in the reverse order of
 * writing, as blitz2's offset list).  The bytes of a node blitz2 does not
 * use (a constant's data and procnum, the padding) are memory junk in the
 * files blitz2 wrote; here they are 0.
 */
#ifndef BLITZRES_H
#define BLITZRES_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* the longest constant name: the node length is a byte (19 + name + 0) */
#define BLITZRES_MAXNAME 235

struct blitzres_const {
    char *name;
    int32_t value;
};

/* constants in the order of definition; skip: names never added */
struct blitzres_list {
    struct blitzres_const *c;
    size_t n, cap;
    char **skip;
    size_t nskip;
};

/* a name blitz2 takes as a constant (letter or _, then letters, digits,
   _), not longer than BLITZRES_MAXNAME */
int blitzres_valid_name(const char *name);

/* append NAME=VALUE: a name on the skip list is left out (1), a name that
   is there already with the same value too (2, blitz2 keeps the first);
   0 = added, -1 = error (invalid name, the name with another value) */
int blitzres_add(struct blitzres_list *l, const char *name, int32_t value, char *err,
                 size_t errlen);
/* NAME is never added (it must be a valid name); 0 = ok, -1 = error */
int blitzres_skip(struct blitzres_list *l, const char *name, char *err, size_t errlen);
/* the value of NAME in the list; 0 = found, -1 = not there */
int blitzres_find(const struct blitzres_list *l, const char *name, int32_t *value);
void blitzres_free(struct blitzres_list *l);

/* the resident of the constants of l (malloc'd, *len bytes);
   0 = ok, -1 = error (more than 65535 constants) */
int blitzres_write(const struct blitzres_list *l, uint8_t **out, size_t *len, char *err,
                   size_t errlen);

/* the contents of a resident: types, macros, constants, one per line
 *   type NAME size=N
 *     field NAME offset=N flags=N [ndims=N] type=N   (ndims: an array)
 *   macro NAME "TEXT"           (bytes outside 32..126, " and \ as \xNN)
 *   const NAME=VALUE
 * without the unused bytes.  0 = ok, -1 = error (no resident, bounds) */
int blitzres_dump(const uint8_t *buf, size_t size, FILE *out, char *err, size_t errlen);

/* the constants of a resident (only the constants: a resident with types
   or macros is an error); appended to l with blitzres_add.  With strict,
   the file must be exactly what blitzres_write makes of them (pointers,
   relocations, unused bytes 0).  0 = ok, -1 = error */
int blitzres_read_consts(const uint8_t *buf, size_t size, int strict, struct blitzres_list *l,
                         char *err, size_t errlen);

/* an FD file (##base/##bias/##public/##private, NAME(args)(regs)): every
   public function as _NAME = -offset (the bias counts private functions
   too).  varargs: then also _NAME without the A of a tag function NAMEA
   whose FD has no NAME (the stubs of amiga.lib, e.g. SetDTAttrs).
   0 = ok, -1 = error (a function before ##bias, blitzres_add) */
int blitzres_fd(const char *text, size_t len, int varargs, struct blitzres_list *l, char *err,
                size_t errlen);

/* an assembler include: every "NAME EQU|equ|SET EXPR" at the start of a
   line (the comment from ; on left out) whose EXPR is a number ($hex,
   0xhex, decimal, 'ABCD' four characters), a symbol defined before (in
   syms or by an earlier line) or + - * << >> & | ^ ~ ( ) of them, as 32
   bit value: NAME is defined in syms and appended to l.  Names ending in
   _I (the include guards) and lines with other expressions (strings,
   unknown symbols) are left out.  0 = ok, -1 = error (blitzres_add) */
int blitzres_equ(const char *text, size_t len, struct blitzres_list *syms,
                 struct blitzres_list *l, char *err, size_t errlen);

#endif
