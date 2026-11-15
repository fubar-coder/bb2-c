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
 * O(n) in the worst case (the C version needs more stack per level than
 * the original's 12 bytes).
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
   in its address register, so that GCC loads with (a0)+ and (a1)
   instead of through a copy of the pointer. */
#define SCAN_UP(T, COND, p) do {                                            \
    T v_;                                                                   \
    do {                                                                    \
        v_ = *p++;                                                          \
        __asm__("" : "+a"(p));                                              \
    } while (COND(pivot, v_, ffp));                                         \
} while (0)
#define SCAN_DOWN(T, COND, p) do {                                          \
    T v_;                                                                   \
    do {                                                                    \
        p--;                                                                \
        __asm__("" : "+a"(p));                                              \
        v_ = *p;                                                            \
    } while (COND(pivot, v_, ffp));                                         \
} while (0)

/* .'recurs - one function per element type T, each with the comparison
   of its type in its scan loops (the original's sortbyte ... sortfloat):
   lo = L, hi = R, both inclusive element addresses; pointer comparisons
   are unsigned (BHI/BLS/BCC).  The pivot is the middle element,
   lo[(R - L) / 2] (LSR.l #1 of the byte distance, AND.l #-size; fixed
   (#275): for words read as a word, not with MOVE.l). */
#define SORT_FN(name, T, GT, LT)                                            \
static void name(T *lo BL_REG(a0), T *hi BL_REG(a1), const void *ffp BL_REG(a6)) \
{                                                                           \
    (void)ffp;                                                              \
    for (;;) {                                                              \
        T *first = lo, *last = hi;                      /* copies of */     \
        T pivot = lo[(ULONG)(hi - lo) >> 1];                                \
        T *i = lo, *j = hi;                                                 \
                                                                            \
        do {                                            /* 'loop0 */        \
            j++;                                                            \
            if (sdir == 0) {                                                \
                SCAN_UP(T, GT, i);                      /* 'loop1: BGT */   \
                SCAN_DOWN(T, LT, j);                    /* 'loop2: BLT */   \
            } else {                                                        \
                SCAN_UP(T, LT, i);                      /* 'loop3: BLT */   \
                SCAN_DOWN(T, GT, j);                    /* 'loop4: BGT */   \
            }                                                               \
            i--;                                                            \
            if (i > j)                                  /* 'lskip: BHI */   \
                break;                                                      \
            {                                                               \
                T t = *i;                                                   \
                                                                            \
                *i++ = *j;                                                  \
                *j-- = t;                                                   \
            }                                                               \
        } while (i <= j);                               /* BLS 'loop0 */    \
                                                                            \
        if (first < j)                                  /* BCC 'skip2 */    \
            name(first, j, ffp);                                            \
        if (!(i < last))                                /* BCC 'skip3 */    \
            return;                                                         \
        lo = i;                                         /* BRA 'recurs */   \
        hi = last;                                                          \
    }                                                                       \
}

SORT_FN(sortbyte, BYTE, GT_I, LT_I)
SORT_FN(sortword, WORD, GT_I, LT_I)
SORT_FN(sortlong, LONG, GT_I, LT_I)
SORT_FN(sortfloat, ULONG, GT_F, LT_F)
SORT_FN(sortstring, ULONG, GT_S, LT_S)


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
