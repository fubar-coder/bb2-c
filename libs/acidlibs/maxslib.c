/*
 * maxslib - system library 64335 ($CF00-$CF02): memory of the object
 * arrays of the object libraries (size = maximum << shift * item size, see
 * calcmaxs in blitz2.s) and of strings copied into global memory (tokens
 * setmaxs, allocst, freest, blitz2.s equates 193-195).
 *
 * C translation of bblibsrc/maxslib (the binary matches the source).
 *   setmaxs:   called by the program start code (blitz2.s ~11236) with
 *              a2 = table of {offset.w, size.l}, d4 = entries - 1: every
 *              entry gets `size` bytes (memlib globalloc, cleared) at
 *              offset(a5).
 *   doallocst: d0 = source, d1 = length -> d0 = copy of the string in
 *              global memory: length.l, bytes, 0 (d0 points to the bytes).
 *              Length 0 -> d0 = 0, nothing allocated.
 *   dofreest:  d0 = such a string (or 0) -> freed again.
 *
 * Note: the library number is given literally in the source (64335 =
 * #maxslib).
 */
#include "blitzlib.h"

/* entry points (thunks, defined below) */
void setmaxs(void), doallocst(void), dofreest(void);

/* !syslibheader{64335,$cf00,$cf03,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_maxslib, 0xcf00, 0xcf03, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(setmaxs, 0, 0);                   /* $CF00 setmaxs */
BLITZ_NULLSUB(doallocst, 0, 0);                 /* $CF01 allocst */
BLITZ_NULLSUB(dofreest, 0, 0);                  /* $CF02 freest */
BLITZ_LIBFIN();

/* doallocst - keeps a0-a1 (MOVEM).
   DIFF: the original returns d1 = -1 after copying (d1 = 0 if nothing was
   copied); the thunk keeps d1. */
static ULONG BLITZ_ROUTINE doallocst_c(const UBYTE *src BL_REG(d0), ULONG len BL_REG(d1))
{
    struct bl_regs r = { 0, 0, 0, 0 };
    UBYTE *dst, *ret;

    if (!len)
        return 0;
    r.d0 = len + 5;                             /* length.l + bytes + 0 */
    r.d1 = MEMF_PUBLIC;                         /* MOVEQ #1,d1 */
    BLITZ_ALIBJSR(TOK_globalloc, r);            /* global allocmem */
    *(ULONG *)r.d0 = len;                       /* MOVE.l d1,(a0)+ */
    dst = ret = (UBYTE *)r.d0 + 4;
    while ((LONG)--len >= 0)                    /* SUBQ.l #1,d1:BMI 'done */
        *dst++ = *src++;
    *dst = 0;
    return (ULONG)ret;
}
BLITZ_THUNK(doallocst, doallocst_c, "d1/a0-a1");

/* dofreest - keeps d1/a0-a1 (MOVEM); d0 = 0: nothing to do.
   DIFF: the thunk keeps d0 too (the original returns memlib's d0). */
static void BLITZ_ROUTINE dofreest_c(ULONG str BL_REG(d0))
{
    struct bl_regs r = { 0, 0, 0, 0 };

    if (!str)
        return;
    r.a1 = str - 4;
    r.d0 = *(ULONG *)r.a1 + 5;                  /* MOVE.l -(a1),d0:ADDQ.l #5,d0 */
    BLITZ_ALIBJSR(TOK_globfree, r);             /* global free */
}
BLITZ_THUNK(dofreest, dofreest_c, "d0-d1/a0-a1");

/* setmaxs - a2 = {offset.w, size.l} table, d4.w = entries - 1.
   a0/a1 are kept (memlib's globalloc keeps them, so the original does).
   The original leaves a2 behind the table, d3 = last offset, d4.w = -1;
   the program start code does not use them (like staticslib). */
struct maxs {
    WORD off;                                   /* in the globals (a5) */
    ULONG size;
};
static_assert(sizeof(struct maxs) == 6);

static void BLITZ_ROUTINE setmaxs_c(const struct maxs *tab BL_REG(a2), UWORD n BL_REG(d4))
{
    do {
        WORD off = tab->off;                    /* MOVE (a2)+,d3 */
        struct bl_regs r = { 0, 0, 0, 0 };
        r.d0 = tab->size;                       /* MOVE.l (a2)+,d0 */
        tab++;
        r.d1 = MEMF_PUBLIC | MEMF_CLEAR;        /* #$10001 */
        BLITZ_ALIBJSR(TOK_globalloc, r);        /* global alloc */
        *(ULONG *)(blitz_globals + off) = r.d0; /* MOVE.l d0,0(a5,d3.w) */
    } while (n-- != 0);                         /* DBF d4,setmaxs */
}
BLITZ_THUNK(setmaxs, setmaxs_c, "a0-a1");
