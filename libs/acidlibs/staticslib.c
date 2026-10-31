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

/* entry points (thunks, defined below) */
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
 * As alstat, with the offsets from a4 (the original swaps a4/a5 around a
 * call of allocst); d0/d1 (procedure parameters) and a0/a1 are preserved.
 *
 * @param tab a2: the compiler's table (calcstatic, blitz2.s 6740)
 * @param n   d4: the number of entries - 1
 */
static void BLITZ_ROUTINE locallocst_c(const struct staticent *tab BL_REG(a2), UWORD n BL_REG(d4))
{
    alloc_statics(tab, n, blitz_locals);
}
BLITZ_THUNK(locallocst, locallocst_c, "d0-d1/a0-a1");
