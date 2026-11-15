/*
 * broken.c - deliberately broken input of the check of the __entrypoint
 * routines (blitzlib entrypoints, test blitzlib_entrypoints_broken in
 * libs/CMakeLists.txt); not a library: only its objects are checked.
 *   availmem    an __entrypoint routine that calls exec: GCC loads a6 with
 *               SysBase and does not save it
 *   twice_c     a C call of an __entrypoint routine
 *   folded/plain  an __entrypoint routine without no_icf and a plain
 *               function with the same body: identical code folding makes
 *               the plain function a jump to it (it loses its saves)
 */
#include "blitzlib.h"
#include "blitzexec.h"

extern ULONG ext(ULONG);
extern ULONG tab[8];

static BLITZ_ENTRYPOINT BLITZ_ROUTINE ULONG availmem_c(ULONG flags BL_REG(d1))
{
    return AvailMem(flags);
}
BLITZ_ALIAS_EP(availmem, availmem_c);

static BLITZ_ENTRYPOINT BLITZ_ROUTINE ULONG sum_c(ULONG n BL_REG(d0), ULONG m BL_REG(d1))
{
    ULONG s = 0, i;

    for (i = 0; i < n; i++)
        s += ext(tab[i & 7] * m);
    return s;
}
BLITZ_ALIAS_EP(sum, sum_c);

ULONG BLITZ_ROUTINE twice_c(ULONG n BL_REG(d0), ULONG m BL_REG(d1))
{
    ULONG x = n * 3 + m, y = m * 5 + n;

    return sum_c(n, m) + x + y;
}

static __entrypoint BLITZ_ROUTINE ULONG folded_c(ULONG n BL_REG(d0), ULONG m BL_REG(d1))
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
