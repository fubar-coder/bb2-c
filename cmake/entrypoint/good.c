/*
 * good.c - the input of the check of the __entrypoint routines that must
 * pass (blitzlib entrypoints, test blitzlib_entrypoints_good in
 * libs/CMakeLists.txt): an __entrypoint routine with BLITZ_ENTRYPOINT
 * (no_icf) and a plain function with the same body stay apart.
 */
#include "blitzlib.h"

extern ULONG ext(ULONG);

static BLITZ_ENTRYPOINT BLITZ_ROUTINE ULONG folded_c(ULONG n BL_REG(d0), ULONG m BL_REG(d1))
{
    ULONG s = 1, i;

    for (i = 0; i < n; i++)
        s ^= ext(i + m);
    return s;
}
BLITZ_ALIAS_EP(folded, folded_c);

ULONG BLITZ_ROUTINE plain_c(ULONG n BL_REG(d0), ULONG m BL_REG(d1))
{
    ULONG s = 1, i;

    for (i = 0; i < n; i++)
        s ^= ext(i + m);
    return s;
}
