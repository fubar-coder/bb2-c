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
 * BUG(orig): on failure the init never returns.  The branch target is the
 * displacement word of the LEA, which executes as ORI.b #0,(a0)+ (the
 * MOVEQ is its immediate), so it calls OpenLibrary again with d0 = 0 and
 * whatever a1 holds after the failed call (a scratch register of exec) -
 * an endless loop.  The C version keeps the endless retry, with the name
 * each time.
 *
 * BUG(orig) of the resources (ciaa, ciab, disk, misc, potgo, battclock,
 * battmem, card): they were made from the library template, whose finit
 * (MOVE.l 4,a6:JMP CloseLibrary(a6)) is 6 bytes longer than theirs (RTS),
 * without adjusting the LEA: it points 6 bytes into the name
 * ("ciab.resource" -> "esource"), so OpenResource always fails and the
 * init hangs.  Kept: the resource files pass libname + DEFLIB_RESOURCE_SKEW.
 */
#ifndef DEFLIB_H
#define DEFLIB_H

#include "blitzexec.h"

#define DEFLIB_RESOURCE_SKEW 6

/* OpenLibrary(name, 0), retried until it succeeds (BUG(orig), see above) */
static inline ULONG deflib_openlibrary(const char *name)
{
    struct Library *base;

    do
        base = OpenLibrary((CONST_STRPTR)name, 0);
    while (!base);
    return (ULONG)base;
}

/* OpenResource(name), retried until it succeeds (BUG(orig), see above) */
static inline ULONG deflib_openresource(const char *name)
{
    APTR base;

    do
        base = OpenResource((CONST_STRPTR)name);
    while (!base);
    return (ULONG)base;
}

#endif
