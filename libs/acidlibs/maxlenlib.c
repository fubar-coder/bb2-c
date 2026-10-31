/*
 * maxlenlib - system library 64135 ($D000-$D001): MaxLen a$=n - give a
 * string variable a fixed buffer of n characters (tokens maxlen/lmaxlen,
 * blitz2.s equates 199-200; $D000 for global, $D001 for local strings,
 * blitz2.s 12190).
 *
 * C translation of bblibsrc/maxlenlib (binary checked: same as the
 * source).  d0 = address of the string variable, d1 = max len, a6 =
 * memlib's membase table (!libs 65530,#la6).  String memory as in
 * strings2lib: -8 alloc'd len, -4 string len, characters, 0.
 *   - existing memory is freed (memlib locfree/globfree, len = alloc'd+9)
 *     and the variable cleared,
 *   - for n <> 0: n+9 bytes are allocated, alloc'd len = n, string len 0,
 *     empty string; the variable points to the characters.
 * d0 = the new variable contents (0 for n = 0), as in the original.
 *
 * Registers: the original changes d0/d1/a0/a1 and also d2 (= n) and a2
 * (= the variable address); they are no results.
 * DIFF: d2/a2 are kept.  domaxlen leaves a6+8 like the original.
 *
 * echk1 (debugger only): d1 < 0 -> runtime error 1 (MOVEQ #1,d0:TRAP #0,
 * an error number instead of a message pointer).  The header's error
 * field is 0 in the original (the error checks are not in a separate
 * area), so echk1 is ordinary code here as well.
 */
#include "blitzlib.h"

/* entry points (defined below) */
void domaxlen(void), domaxlenl(void), echk1(void);

/* !syslibheader{64135,$d000,$d002,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_maxlenlib, 0xd000, 0xd002, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(domaxlen, echk1, 0, LIB_memlib, BL_LA(6));    /* $D000 maxlen */
BLITZ_NULLSUB(domaxlenl, echk1, 0, LIB_memlib, BL_LA(6));   /* $D001 lmaxlen */
BLITZ_LIBFIN();

/* membase table of memlib (blitzhelp.h MEMBASE_xxx): JSR (a6) alloc,
   JSR 4(a6) free; the global versions are at +8 */

/* the memory of a string variable; the variable points to `chars` */
struct strmem {
    ULONG alloced;                              /* -8: alloc'd len */
    ULONG len;                                  /* -4: string len */
    UBYTE chars[];                              /* characters, 0 */
};
static_assert(sizeof(struct strmem) == 8);

static ULONG maxlen(UBYTE **var, ULONG n, const UBYTE *mem)
{
    struct strmem *h;

    if (*var) {
        h = (struct strmem *)(*var - 8);        /* SUBQ.l #8,d0 */
        {
            struct bl_regs r = { .d0 = h->alloced + 9, .a1 = (ULONG)h };
            BLITZ_BASEJSR(mem, MEMBASE_locfree, r);
        }
        *var = 0;                               /* CLR.l (a2) */
    }
    if (!n)                                     /* 'nofree: BEQ 'skip */
        return 0;
    {
        struct bl_regs r = { .d0 = n + 9, .d1 = MEMF_PUBLIC };
        h = (struct strmem *)BLITZ_BASEJSR(mem, MEMBASE_localloc, r);
    }
    *var = h->chars;
    h->alloced = n;                             /* MOVE.l d2,(a0)+ */
    h->len = 0;                                 /* CLR.l (a0)+ */
    h->chars[0] = 0;                            /* CLR.b (a0) */
    return (ULONG)h->chars;
}

/* domaxlenl - local string; domaxlen: global string, ADDQ #8,a6 first
   (in the thunk, as in the original: a6 stays +8).  a0/a1 are kept: the
   original leaves them unchanged on some paths (no old memory / n = 0). */
static ULONG BLITZ_ROUTINE domaxlenl_c(UBYTE **var BL_REG(d0), ULONG n BL_REG(d1),
                                       const UBYTE *mem BL_REG(a6))
{
    return maxlen(var, n, mem);
}
BLITZ_THUNK(domaxlenl, domaxlenl_c, "a0-a1");
BLITZ_THUNK_PRE(domaxlen, domaxlenl_c, "a0-a1", "addq.w #8,a6");

/* echk1 - TST.l d1:BMI err1:RTS; err1: MOVEQ #1,d0:TRAP #0.
   Keeps all registers (d0 only changed on the error). */
static void BLITZ_ROUTINE echk1_c(LONG n BL_REG(d1))
{
    if (n < 0)
        BLITZ_RUNERR((const char *)1);
}
BLITZ_THUNK(echk1, echk1_c, "d0-d1/a0-a1");
