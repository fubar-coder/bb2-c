/*
 * riarrayslib - RWE userlib 7 (Reflective Images, 1994): FillArray.
 *
 *   FillArray array(),value   every element of a byte, word or long
 *                             array set to value
 *   FillArray array()         every element 0
 *
 * C translation of rwelibsrc/ArraysLib.bb; reference: the reference build
 * of that source (no binary exists, docs/rwelibs/README.md 3.3).  Notes:
 * docs/rwelibs/port/riarrayslib.md.
 *
 * Both forms are inline routines (Dc $a000: the compiler copies the code
 * to the call), so they stay assembler.  The array argument is
 * #unknown|#array|#arrayend: d0 = the first element, the compiler's a2
 * stack holds the type word and, on top, the address behind the last
 * element; the code pops both (a2 - 6).  Types other than byte (1), word
 * (2) and long (3) - quick, float, string ... - do nothing.  init and
 * finit do nothing.
 *
 * BUG(orig): the loops store first and compare after (CMP.l a0,a1:BNE):
 * an end that is not reached exactly (a size that is no multiple of the
 * element size; an array of 0 elements) runs through memory (#882).
 */
#include "blitzlib.h"

/* entry points (inline routines and init/finit, below) */
void _fillarray(void), _fillarray2(void);
static void _libinit(void);
static void _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#riarrayslib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_riarrayslib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_UNKNOWN | BL_ARRAY | BL_ARRAYEND, BL_UNKNOWN);
BLITZ_LIBS0();
BLITZ_SUBS(_fillarray, 0, 0);
BLITZ_ARGS(BL_UNKNOWN | BL_ARRAY | BL_ARRAYEND);
BLITZ_LIBS0();
BLITZ_SUBS(_fillarray2, 0, 0);
BLITZ_NAME("FillArray", "array(),value -- only byte/word/long arrays!");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/** @brief init: nothing (RTS). */
static void _libinit(void)
{
}

/** @brief finit: nothing (RTS). */
static void _libfinit(void)
{
}

/* the loop of both forms (in the order of the source: _fillarray2
   first): src = d1 or #0 (the value), the type in d2 */
#define FILL(src)                                                           \
    "move.l  d0,a0\n"                   /* the first element */             \
    "   move.l  -(a2),a1\n"             /* the end of the array */          \
    "   move.w  -(a2),d2\n"             /* the type */                      \
    "   cmp.b   #1,d2\n"                /* #byte */                         \
    "   beq.s   12f\n"                                                      \
    "   cmp.b   #2,d2\n"                /* #word */                         \
    "   beq.s   11f\n"                                                      \
    "   cmpi.b  #3,d2\n"                /* #long */                         \
    "   bne.s   13f\n"                                                      \
    "10: move.l  " src ",(a0)+\n"                                           \
    "   cmp.l   a0,a1\n"                                                    \
    "   bne.s   10b\n"                                                      \
    "   bra.s   13f\n"                                                      \
    "11: move.w  " src ",(a0)+\n"                                           \
    "   cmp.l   a0,a1\n"                                                    \
    "   bne.s   11b\n"                                                      \
    "   bra.s   13f\n"                                                      \
    "12: move.b  " src ",(a0)+\n"                                           \
    "   cmp.l   a0,a1\n"                                                    \
    "   bne.s   12b\n"                                                      \
    "13:"

/**
 * @brief FillArray array(): every element of a byte, word or long array 0.
 *
 * Inline (Dc $a000).  MOVE.l/.w/.b #0 (not CLR, which reads first on the
 * 68000).  Other types do nothing.  BUG(orig) (#882): the loop stores
 * first and compares after; an end not reached exactly (a size that is
 * no multiple of the element size, 0 elements) runs through memory.
 * Parameters: d0 = the first element; on the a2 stack the type word and,
 * on top, the address behind the last element (both popped).
 * Result: nothing.
 */
BLITZ_INLINE(_fillarray2, FILL("#0"));

/**
 * @brief FillArray array(),value: every element of a byte, word or long
 * array set to value.
 *
 * Inline as FillArray array(), with the value from d1 (its low byte or
 * word for byte and word arrays); the same BUG(orig) (#882).
 * Parameters: d0 = the first element, d1 = the value; the a2 stack as
 * above.  Result: nothing.
 */
BLITZ_INLINE(_fillarray, FILL("d1"));
