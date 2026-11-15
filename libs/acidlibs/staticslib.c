/*
 * staticslib - system library 65335 ($C500-$C501): allocate the memory of
 * NEWTYPE variables (global and local statics), tokens alstat/localstat
 * (blitz2.s equates 147-148).
 *
 * C translation of bblibsrc/staticslib.  The compiler emits a table of
 * {offset.w, size.w} pairs (calcstatic, blitz2.s 6740) and calls allocst
 * with a2 = table, d4 = number of entries - 1.  For every entry a block
 * of `size` bytes is allocated (memlib localloc, cleared) and its address
 * stored in the variable at `offset` from a5 (globals) - or from a4
 * (locals) for locallocst.
 */
#include "blitzlib.h"

/* entry points (a thunk, assembler; defined below) */
void allocst(void), locallocst(void);

/* !syslibheader{#staticslib,$c500,$c502,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_staticslib, 0xc500, 0xc502, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(allocst, 0, 0);                   /* $C500 alstat */
BLITZ_NULLSUB(locallocst, 0, 0);                /* $C501 localstat */
BLITZ_LIBFIN();

/* an entry of the compiler's table (calcstatic): DC.w offset, size */
struct staticent {
    WORD offset;                /* of the variable from a5/a4 */
    UWORD size;                 /* of the NEWTYPE */
};
static_assert(sizeof(struct staticent) == 4);

static void alloc_statics(const struct staticent *tab, UWORD n, UBYTE *base)
{
    do {                                        /* 'loop ... DBF d4,'loop */
        WORD off = tab->offset;
        struct bl_regs r;
        tab++;
        r = (struct bl_regs){ .d0 = tab[-1].size, .d1 = MEMF_PUBLIC | MEMF_CLEAR };
        BLITZ_ALIBJSR(TOK_localloc, r);         /* local allocate */
        *(ULONG *)(base + off) = r.d0;          /* MOVE.l d0,0(a5,d2.w) */
    } while (n-- != 0);
}

/**
 * @brief $C500 (alstat) - allocate the NEWTYPE memory of global statics.
 *
 * For every table entry a cleared block of `size` bytes (memlib localloc)
 * is allocated and its address stored at `offset` from a5.  a0/a1 are
 * preserved (memlib's localloc keeps them, so the original does too).
 *
 * @param tab a2: the compiler's table (calcstatic, blitz2.s 6740)
 * @param n   d4: the number of entries - 1
 */
static void BLITZ_ROUTINE allocst_c(const struct staticent *tab BL_REG(a2), UWORD n BL_REG(d4))
{
    alloc_statics(tab, n, blitz_globals);
}
BLITZ_THUNK(allocst, allocst_c, "a0-a1");

/**
 * @brief $C501 (localstat) - allocate the NEWTYPE memory of local
 * statics.
 *
 * The loop of alstat in assembler, with a0 = the table, a1 = the offset
 * from a4, d4 = the count, around memlib localloc (which keeps a0-a1).
 * Saves d0-d1/d4 and changes a0/a1 only: C cannot keep d0/d1 without a
 * thunk.  Fixed (#1063): the original changes d2 (the offset) and d4
 * (the DBF count) although the compiler calls it in the procedure
 * prologue with the parameters still in d0-d5, so the third parameter of
 * a procedure with a local NEWTYPE variable was wrong (a fifth one too, if
 * the compiler did not save d4); it also leaves a2 behind the table.  Here
 * only a0/a1 change (test: library cases "d2 kept" and "d4 kept", bug-orig
 * test staticslib-localstat-d2).
 *
 * As alstat, with the offsets from a4 (the original swaps a4/a5 around a
 * call of allocst).  Keeps d0-d7/a2-a6: the compiler calls it in the
 * procedure prologue with the parameters in d0-d5 (perf-analysis
 * thunks.md 1.4); a0 and a1 are free (the original keeps them), no
 * third-party library calls it.
 *
 * @param tab a2: the compiler's table (calcstatic, blitz2.s 6740)
 * @param n   d4: the number of entries - 1
 */
BLITZ_ASM_ROUTINE(locallocst,
        "   movem.l d0-d1/d4,-(sp)\n"
        "   move.l  a2,a0\n"
        "1: move.w  (a0)+,a1\n"             /* the offset from a4 */
        "   moveq   #0,d0\n"
        "   move.w  (a0)+,d0\n"             /* the size */
        "   move.l  #0x10001,d1\n"          /* MEMF_PUBLIC|MEMF_CLEAR */
        "   jsr     _blitz_tok+0x80000000+" BL_STR(TOK_localloc) "\n"
        "   move.l  d0,0(a4,a1.w)\n"
        "   dbf     d4,1b\n"
        "   movem.l (sp)+,d0-d1/d4\n"
        "   rts");
