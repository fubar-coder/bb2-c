/*
 * sortlib - command library 110: Sort, SortUp, SortDown.
 *
 * C translation of bblibsrc/sortlib (binary checked: same as the source).
 * "This is the Blitz 1 Sort algorithm": a recursive Hoare quicksort over
 * the whole array, ascending (SortUp, also the init) or descending
 * (SortDown).
 *
 * Sort Arrayname(): !args{$38} = #array|#arrayend|#unknown - the compiler
 * passes the base of the array data in d0 and pushes the end address
 * (long) and the element type (word); the routine reads them with
 * MOVE.l -(a2),a1 / MOVE -(a2),d1 (a2 = stack pointer before the pushes).
 * Types: 1 byte, 2 word, 5 float (mathffp SPCmp, a6 = ffplib base),
 * 7 string (strcomplib $CE02 on the string pointers), everything else
 * (3 long, 4 quick) is sorted as signed longs.  As in the original, each
 * element type has its own sort with the comparison in its scan loops
 * (SORT_FN; floats and strings through a call).
 *
 * Not a bug (a property of the quicksort algorithm): recursion depth is
 * O(n) in the worst case (12 bytes per level as in the original for the
 * integer sorts, more for floats and strings).
 * Fixed (intended deviation, #275): word arrays read the pivot with MOVE.l
 *   and compared with CMP.w, so the pivot was the element AFTER the middle
 *   one; for a one element array (Dim a.w(0)) it lay behind the array: the
 *   forward scan read on behind it (up to the next word equal to what it
 *   read there), the backward scan could read words before it (reads only,
 *   the result was right).  Here the pivot is read as a word, the middle
 *   element (the sorted array is the same).
 * Fixed (intended deviation, #276): string elements that were never
 *   assigned (pointer 0) read their length from -4(0) (and their
 *   characters from 0): the order was arbitrary.  Here a pointer 0 is the
 *   empty string.
 * The original leaves a2 decremented by 6; the compiler does not use a2
 * afterwards (see memacclib.c).
 */
#include "blitzlib.h"

/* entry points (defined below) */
void dosort(ULONG base BL_REG(d0), const UBYTE *args BL_REG(a2), const void *ffp BL_REG(a6));
void dosortup(void);
void dosortdown(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#sortlib,init,0,0,0} */
BLITZ_LIBHEADER(LIB_sortlib, init, BL_NORET, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_ARRAY | BL_ARRAYEND | BL_UNKNOWN);       /* $38 */
BLITZ_LIBS(LIB_ffplib, BL_LA6);                         /* for ffp sorts */
BLITZ_SUBS(dosort, 0, 0);
BLITZ_NAME("Sort", "Arrayname()");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dosortup, 0, 0);
BLITZ_NAME("SortUp", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dosortdown, 0, 0);
BLITZ_NAME("SortDown", "");

BLITZ_NULLSUB_AT(init, dosortup, 0, 0);

BLITZ_LIBFIN();

/* sdir: Dc 0 - 0 = low to high, -1 (SortDown) = high to low */
static UWORD sdir;

/**
 * @brief SortUp - the following Sort commands sort ascending.
 *
 * CLR sdir.  Also the library's init (BLITZ_NULLSUB_AT): ascending is the
 * default.
 */
void BLITZ_ROUTINE dosortup(void)
{
    sdir = 0;                                           /* CLR sdir */
}

/**
 * @brief SortDown - the following Sort commands sort descending.
 *
 * MOVE #-1,sdir.
 */
void BLITZ_ROUTINE dosortdown(void)
{
    sdir = 0xffff;                                      /* MOVE #-1,sdir */
}

/* The comparisons of the original's scan loops: CMP.x (a0)+,d0 with the
   pivot in d0 (BGT 'loop1: the pivot is greater, BLT 'loop2: less; CMP
   compares signed); floats and strings through a call that sets the CCR. */

/* compflts - JSR -42(a6) (mathffp SPCmp, d0 = pivot, d1 = element); the
   original branches on the CCR it returns (BGT/BLT), here an Scc right
   after the call.  mathffp may change d0-d1/a0-a1 and keeps d2-d7/a2-a6
   like every system library. */
#define FFP_CC_(cc, pivot, elem, ffp) ({                                    \
    register ULONG _d0 __asm("d0") = (pivot);                               \
    register ULONG _d1 __asm("d1") = (elem);                                \
    register const void *_a6 __asm("a6") = (ffp);                           \
    UBYTE _cc;                                                              \
    __asm volatile("jsr -42(a6)\n\ts" cc " %0"                              \
        : "=d"(_cc), "+d"(_d0), "+d"(_d1) : "a"(_a6)                        \
        : "a0", "a1", "cc", "memory");                                      \
    _cc; })

/* compstrs - strcomplib $CE02 on the string pointers: d0/a0 = left (the
   pivot), d1/a1 = right, the lengths at -4; -1/0/1 in d0.  Fixed (#276):
   a pointer 0 (never assigned) is the empty string, its length is not
   read from -4(0).  Out of line: the call may change every register but
   a4/a5/a7, so the scan loop keeps its state in the registers this
   function saves. */
static __attribute__((noinline)) LONG strcmpptr(ULONG pivot, ULONG elem)
{
    const ULONG *l = (const ULONG *)pivot, *r = (const ULONG *)elem;
    struct bl_regs rg = { .d0 = l ? l[-1] : 0, .d1 = r ? r[-1] : 0,
                          .a0 = (ULONG)l, .a1 = (ULONG)r };

    return (LONG)BLITZ_ALIBJSR(TOK_strcomp2, rg);
}

#define GT_I(p, v, f) ((p) > (v))
#define LT_I(p, v, f) ((p) < (v))
#define GT_F(p, v, f) FFP_CC_("gt", p, v, f)
#define LT_F(p, v, f) FFP_CC_("lt", p, v, f)
#define GT_S(p, v, f) (strcmpptr(p, v) > 0)
#define LT_S(p, v, f) (strcmpptr(p, v) < 0)

/* the scan loops: on while the pivot is COND than the element, the
   forward one with *i++ (CMP.x (a0)+,d0), the backward one with --j
   first (SUBQ #size,a1:CMP.x (a1),d0).  The empty asm keeps the pointer
   in a register of the class R ("a" for the integer types: an address
   register, so that GCC loads with (a0)+ and (a1) instead of through a
   copy of the pointer; "r" for floats and strings, whose comparison is a
   call that may change a0/a1). */
#define SCAN_UP(T, COND, R, p) do {                                         \
    T v_;                                                                   \
    do {                                                                    \
        v_ = *p++;                                                          \
        __asm__("" : "+" R(p));                                             \
    } while (COND(pivot, v_, ffp));                                         \
} while (0)
#define SCAN_DOWN(T, COND, R, p) do {                                       \
    T v_;                                                                   \
    do {                                                                    \
        p--;                                                                \
        __asm__("" : "+" R(p));                                             \
        v_ = *p;                                                            \
    } while (COND(pivot, v_, ffp));                                         \
} while (0)

/* .'recurs - one function per element type T, each with the comparison
   of its type in its scan loops (the original's sortbyte ... sortfloat):
   lo = L, hi = R, both inclusive element addresses; pointer comparisons
   are unsigned (BHI/BLS/BCC).  The pivot is the middle element,
   lo[(R - L) / 2] (LSR.l #1 of the byte distance, AND.l #-size; fixed
   (#275): for words read as a word, not with MOVE.l).  As the original,
   only the left part is a recursive call; the right part is the next
   round of the outer loop (BRA 'recurs), and sdir is read in every round
   of 'loop0 (MOVE sdir(pc),d1).  The original's j++ before the backward
   scan and its j-- after a swap cancel (the scan starts at the element
   below the swapped one either way); here j is decremented only when the
   loop ends after a swap.  The empty asm after i-- makes GCC step i
   itself instead of keeping i - 1 in another register; the memory
   barrier before the swap makes it read *j again (MOVE.x (a1),(a0)+)
   instead of keeping the backward scan's element in a register.  So the
   integer sorts keep two registers of their own per level, as the
   original pushes two longs. */
#define SORT_FN(name, T, GT, LT, R)                                         \
static void name(T *lo BL_REG(a0), T *hi BL_REG(a1), const void *ffp BL_REG(a6)) \
{                                                                           \
    (void)ffp;                                                              \
    for (;;) {                                                              \
        T pivot = lo[(ULONG)(hi - lo) >> 1];                                \
        T *i = lo, *j = hi + 1;                                             \
                                                                            \
        for (;;) {                                      /* 'loop0 */        \
            if (sdir == 0) {                                                \
                SCAN_UP(T, GT, R, i);                   /* 'loop1: BGT */   \
                i--;                                                        \
                __asm__("" : "+" R(i));                                     \
                SCAN_DOWN(T, LT, R, j);                 /* 'loop2: BLT */   \
            } else {                                                        \
                SCAN_UP(T, LT, R, i);                   /* 'loop3: BLT */   \
                i--;                                                        \
                __asm__("" : "+" R(i));                                     \
                SCAN_DOWN(T, GT, R, j);                 /* 'loop4: BGT */   \
            }                                                               \
            if (i > j)                                  /* 'lskip: BHI */   \
                break;                                                      \
            __asm__("" ::: "memory");                                       \
            {                                                               \
                T t = *i;                                                   \
                                                                            \
                *i++ = *j;                                                  \
                *j = t;                                                     \
            }                                                               \
            if (i >= j) {                               /* BLS 'loop0 */    \
                j--;                                                        \
                break;                                                      \
            }                                                               \
        }                                                                   \
                                                                            \
        if (lo < j)                                     /* BCC 'skip2 */    \
            name(lo, j, ffp);                                               \
        if (!(i < hi))                                  /* BCC 'skip3 */    \
            return;                                                         \
        lo = i;                                         /* BRA 'recurs */   \
    }                                                                       \
}

SORT_FN(sortbyte, BYTE, GT_I, LT_I, "a")
SORT_FN(sortword, WORD, GT_I, LT_I, "a")
SORT_FN(sortlong, LONG, GT_I, LT_I, "a")
SORT_FN(sortfloat, ULONG, GT_F, LT_F, "r")
SORT_FN(sortstring, ULONG, GT_S, LT_S, "r")


/**
 * @brief Sort Arrayname() - sort the whole array in place.
 *
 * "The Blitz 1 Sort algorithm": a recursive Hoare quicksort, ascending or
 * descending as set by SortUp/SortDown.  !args{$38} (#array|#arrayend|
 * #unknown): the compiler passes the base of the array data in d0 and
 * pushes the end address (long) and the element type (word); the routine
 * reads them with MOVE.l -(a2),a1 / MOVE -(a2),d1.  Types: 1 byte, 2 word,
 * 5 float (mathffp SPCmp), 7 string (strcomplib $CE02 on the string
 * pointers), everything else (3 long, 4 quick) as signed longs.  The
 * original leaves a2 decremented by 6; the compiler does not use a2
 * afterwards.
 *
 * Fixed (#275): for word arrays the pivot is read as a word, the middle
 * element (the original read it with MOVE.l and compared with CMP.w: the
 * element after the middle one, for a one element array the word behind
 * the array; reads only, the result was right).
 *
 * Fixed (#276): string elements that were never assigned (pointer 0) are
 * the empty string (the original read their length from -4(0)).
 *
 * @param base d0: the base (lowest element) of the array data
 * @param args a2: the stack pointer before the pushes: -4 = the end
 *             address (long), -6 = the element type (word)
 * @param ffp  a6: the mathffp base (ffplib), for float arrays
 */
void BLITZ_ROUTINE dosort(ULONG base BL_REG(d0), const UBYTE *args BL_REG(a2),
                          const void *ffp BL_REG(a6))
{
    UBYTE *hi = (UBYTE *)*(const ULONG *)(args - 4);    /* MOVE.l -(a2),a1 */

    switch (*(const UWORD *)(args - 6)) {               /* MOVE -(a2),d1 */
    case 1:                                             /* SUBQ #size,a1 */
        sortbyte((BYTE *)base, (BYTE *)(hi - 1), ffp);
        break;
    case 2:
        sortword((WORD *)base, (WORD *)(hi - 2), ffp);
        break;
    case 5:
        sortfloat((ULONG *)base, (ULONG *)(hi - 4), ffp);
        break;
    case 7:
        sortstring((ULONG *)base, (ULONG *)(hi - 4), ffp);
        break;
    default:                                            /* also 3 and 4 */
        sortlong((LONG *)base, (LONG *)(hi - 4), ffp);
        break;
    }
}
