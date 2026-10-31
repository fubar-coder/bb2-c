/*
 * deflib.h - init code shared by the AmigaOS call libraries of
 * blitz2:deflibs (deflibs/NAME.c, see notes/deflibs.md).
 *
 * The originals (cd/extras/blitzlibs/amigalibs/NAME.library1) all have the
 * same init:
 *
 *   init:  MOVE.l  4,a6
 *          LEA     name(pc),a1
 *   loop:  MOVEQ   #0,d0
 *          JSR     OpenLibrary(a6)       ; resources: OpenResource
 *          TST.l   d0
 *          BEQ     init+6                ; = the middle of the LEA
 *          RTS
 *
 * Fixed (intended deviation, #44): in the original, the init never
 * returns on failure.  The branch target is the displacement word of the
 * LEA, which executes as ORI.b #0,(a0)+ (the MOVEQ is its immediate), so
 * it calls OpenLibrary again with d0 = 0 and whatever a1 holds after the
 * failed call (a scratch register of exec) - an endless loop: a program
 * using a call of a missing library hangs at startup.  Here the init
 * returns the base or 0, like the original's CrM.library (crmlib.c) and
 * AmiBlitz3's deflibs; the finit of a library closes it only if the base
 * is not 0 (CloseLibrary(0) crashes before Kickstart 2.0).  With base 0
 * the program runs; a call of the missing library crashes (JSR to a
 * negative offset from 0), so a program that may run without it checks
 * for it itself (e.g. its own OpenLibrary_) before using its calls.
 * Not a clean exit instead: the deflibs are initialised before the
 * command libraries (inits by falling library number), so an exit from
 * the init (exitslib's End) would run their finits without their inits,
 * and it would need a declaration change (the init's !libs) and take
 * away the use of an optional library.
 *
 * Fixed (intended deviation, #45) for the resources (ciaa, ciab, disk,
 * misc, potgo, battclock, battmem, card): they were made from the library
 * template, whose finit (MOVE.l 4,a6:JMP CloseLibrary(a6)) is 6 bytes
 * longer than theirs (RTS), without adjusting the LEA: in the original it
 * points 6 bytes into the name ("ciab.resource" -> "esource"), so
 * OpenResource always fails and the init hangs.  Here the resource files
 * pass the name itself.
 */
#ifndef DEFLIB_H
#define DEFLIB_H

#include "blitzexec.h"

/* OpenLibrary(name, 0): the base, 0 if it cannot be opened (fixed, #44:
   the original retries forever, see above) */
static inline ULONG deflib_openlibrary(const char *name)
{
    return (ULONG)OpenLibrary((CONST_STRPTR)name, 0);
}

/* finit of a library: CloseLibrary(base) if the init opened it (the
   original: MOVE.l 4,a6:JMP CloseLibrary(a6), never 0 there) */
static inline void deflib_closelibrary(struct Library *base)
{
    if (base)
        CloseLibrary(base);
}

/* OpenResource(name): the base, 0 if there is none (fixed, #44: the
   original retries forever, see above) */
static inline ULONG deflib_openresource(const char *name)
{
    return (ULONG)OpenResource((CONST_STRPTR)name);
}

#endif
