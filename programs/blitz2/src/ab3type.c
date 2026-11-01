/*
 * ab3type.c - the type rules of AB3 mode that AmiBlitz3 v3.14.0 applies
 * to every program, independent of SYNTAX (spec ab3-compat/type-rules,
 * change ab3-pointer-assign): a pointer takes no basic value other than
 * long.  In BB2 mode none applies.
 *
 * AmiBlitz3 (AmiBlitz3.ab3 at 58a11658) does not take the type of the
 * right side's value: its eval keeps the type of the last variable or
 * function it read (rightsidetype, rightsideflagmask: variable 16443,
 * getvaraddr 15442, handle_function 16620), cleared at the start of the
 * statement and by a token (15087); the parameters of library functions
 * and functions are evaluated with it saved (peval 13114, fetchpees2 5484
 * and 7977-8080, 16770).  The assignment checks it after its eval
 * (23531-23572): a left side that is a pointer (leftsideflagmask 1, not
 * one whose address is computed: 23527) refuses a right side of a basic
 * type other than long (code 3) that is not a pointer (flagmask 1), with
 * "Can't convert types", before the test of syntax_typecheck.  Here the
 * elements of the expression report their type (ext_evalvalue); the one
 * at the depth of the assignment's eval counts, the deeper ones are
 * parameters or indices (design D2).  Casts (typetocast) and NewType left
 * sides are not taken over (G5, G8).
 */
#include "blitz2.h"
#include "ab3.h"

/* the depth of the assignment's eval, 0 if none is evaluated */
static UWORD letdepth;
/* the type of the right side so far (AmiBlitz3's rightsidetype,
   rightsideflagmask) */
static ULONG rightcode;
static UWORD rightflags;

void ext_assignstart(void)
{
    if (progmode != XTRA_MODE_AB3)
        return;
    letdepth = evaldepth + 1;
    rightcode = 0;
    rightflags = 0;
}

void ext_evalvalue(ULONG code, UWORD flags)
{
    if (!letdepth || evaldepth != letdepth)
        return;
    rightcode = code;
    rightflags = flags;
}

void ext_assigncheck(UWORD vartype)
{
    if (!letdepth)
        return;
    letdepth = 0;
    if ((vartype & (TF_ADDRINA2 | TF_POINTER)) != TF_POINTER)
        return;                     /* not a pointer variable */
    if (!rightcode || rightflags == 1 || rightcode >= 256
            || rightcode == T_LONG)
        return;
    err("Can't convert types");
}
