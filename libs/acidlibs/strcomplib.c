/*
 * strcomplib - system library 64435 ($CE00-$CE02): string comparison
 * (tokens strcomp/casestrcomp, blitz2.s equates 188-189: the string
 * operators = <> < <= > >= (doeqs..doges) and Select/Case on strings).
 *
 * C translation of bblibsrc/strcomplib (binary checked: same as the
 * source).  The result is -1/0/1 in d0 AND in the CCR (MOVEQ: N and Z,
 * V=C=0): the compiler branches on the flags right after the call
 * (doeqcmp.., selbne).  The thunks set the same flags with TST.l d0.
 * Bytes compare unsigned (CMPM.b / BCS).
 *
 *   strcomp ($CE00):     both strings at the end of the string work area:
 *                        right string just below a3, left string below
 *                        it; lengths on the stack (8(a7) left, 4(a7)
 *                        right), removed by the routine.  a3 is moved
 *                        back to the start of the left string (output).
 *                        Keeps d0-d1 (result only in the CCR!).
 *   casestrcomp ($CE01): d0 = CASE string, 4(a7) its length (removed by
 *                        the routine), 8(a7) = SELECT string (its length
 *                        at -4; left on the stack for the next CASE).
 *   strcomp2 ($CE02):    a0/d0 = left string/length, a1/d1 = right.
 */
#include "blitzlib.h"

/* entry points (thunks, defined below) */
void strcomp(void), casestrcomp(void), strcomp2(void);

/* !syslibheader{#strcomplib,$ce00,$ce03,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_strcomplib, 0xce00, 0xce03, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(strcomp, 0, 0);                   /* $CE00 strcomp */
BLITZ_NULLSUB(casestrcomp, 0, 0);               /* $CE01 casestrcomp */
BLITZ_NULLSUB(strcomp2, 0, 0);                  /* $CE02 */
BLITZ_LIBFIN();

/* strcomp2 - the loop as in the original: lengths count down as signed
   longs (SUBQ.l #1/BMI); when the left string ends first, the result
   depends on whether d1 is 0 (TST.l d1, d1 not yet decremented) */
static LONG compare(const UBYTE *a0, ULONG d0, const UBYTE *a1, ULONG d1)
{
    for (;;) {
        if ((LONG)--d0 < 0)                     /* 'eos1 */
            return d1 != 0 ? -1 : 0;
        if ((LONG)--d1 < 0)                     /* 'isgt */
            return 1;
        if (*a0 != *a1)                         /* CMPM.b (a1)+,(a0)+ */
            return *a0 < *a1 ? -1 : 1;          /* BCS 'islt */
        a0++;
        a1++;
    }
}

static LONG BLITZ_ROUTINE strcomp2_c(const UBYTE *a BL_REG(a0), ULONG alen BL_REG(d0),
                                     const UBYTE *b BL_REG(a1), ULONG blen BL_REG(d1))
{
    return compare(a, alen, b, blen);
}
/* d1/a0/a1 are kept: the original leaves them unchanged when a string
   is empty (its loop stops before CMPM / SUBQ d1) */
BLITZ_THUNK_POP(strcomp2, strcomp2_c, "d1/a0-a1", "", "tst.l d0", 0);

/* casestrcomp - MOVE.l d0,a0:MOVE.l 4(a7),d0:MOVE.l 8(a7),a1
   MOVE.l -4(a1),d1:MOVE.l (a7)+,(a7) then strcomp2 */
static LONG BLITZ_ROUTINE casestrcomp_c(const UBYTE *cs BL_REG(d0), ULONG cslen BL_REG(d1),
                                        const UBYTE *sel BL_REG(a1))
{
    return compare(cs, cslen, sel, ((const ULONG *)sel)[-1]);
}
BLITZ_THUNK_POP(casestrcomp, casestrcomp_c, "", "move.l 4(sp),d1\n   move.l 8(sp),a1",
                "tst.l d0", 4);

/* strcomp - right string = a3 - right length, left string before it;
   a3 = start of the left string afterwards */
static LONG BLITZ_ROUTINE strcomp_c(ULONG llen BL_REG(d0), ULONG rlen BL_REG(d1))
{
    const UBYTE *r, *l;

    blitz_string -= rlen;                       /* SUB.l d1,a3 */
    r = blitz_string;
    blitz_string -= llen;                       /* SUB.l d0,a3 */
    l = blitz_string;
    return compare(l, llen, r, rlen);
}
/* saved d0-d1 (8 bytes) + return address: right length at 12(sp), left
   length at 16(sp) */
BLITZ_THUNK_POP(strcomp, strcomp_c, "d0-d1", "move.l 16(sp),d0\n   move.l 12(sp),d1",
                "tst.l d0", 8);
