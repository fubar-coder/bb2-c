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
 * (3 long, 4 quick) is sorted as signed longs.
 *
 * Kept from the original (BUG(orig)):
 *   - word arrays: the pivot is read with MOVE.l and compared with CMP.w,
 *     so the pivot is the element AFTER the middle one; for a one element
 *     array (Dim a.w(0)) it lies behind the array. (#275)
 *   - the pivot of a one element array is never in range: the forward
 *     scan stops at the pivot word behind the array, the backward scan
 *     may read words before it; then i > j, nothing is swapped and there
 *     is no recursion (reads only, the result is right). (#275)
 *   - string elements that were never assigned (pointer 0) read their
 *     length from -4(0). (#276)
 *   - Not a bug (a property of the quicksort algorithm): recursion depth is
 *     O(n) in the worst case (the C version needs more stack per level
 *     than the original's 12 bytes).
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
BLITZ_LIBS(LIB_ffplib, BL_LA(6));                       /* for ffp sorts */
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

/* compflts - JSR -42(a6) (mathffp SPCmp, d0 = pivot, d1 = element); the
   original branches on the CCR it returns (BGT/BLT).  Assembler only to
   turn exactly that CCR into 1 (GT) / -1 (LT) / 0; mathffp keeps
   d2-d7/a2-a6 like every system library. */
LONG ffpcmp(ULONG pivot BL_REG(d0), ULONG elem BL_REG(d1), const void *ffp BL_REG(a6));
__asm__("   .text\n   .even\n_ffpcmp:\n"
    "   jsr     -42(a6)\n"
    "   bgt.s   1f\n"
    "   blt.s   2f\n"
    "   moveq   #0,d0\n"
    "   rts\n"
    "1: moveq   #1,d0\n"
    "   rts\n"
    "2: moveq   #-1,d0\n"
    "   rts\n");

/* the element type's sort parameters */
struct sorter {
    UWORD type;                 /* 1 byte, 2 word, 5 float, 7 string, else long */
    UWORD size;                 /* element size (pointer step) */
    const void *ffp;            /* a6: mathffp base */
};

/* pivot vs. element as the original's branches see it: > 0 = BGT taken
   (pivot greater), < 0 = BLT taken.  CMP.b/CMP.w/CMP.l compare signed. */
static LONG compare(const struct sorter *s, ULONG pivot, const UBYTE *p)
{
    switch (s->type) {
    case 1:
        return (LONG)(BYTE)pivot - (LONG)*(const BYTE *)p;
    case 2:
        return (LONG)(WORD)pivot - (LONG)*(const WORD *)p;
    case 5:                                             /* compflts */
        return ffpcmp(pivot, *(const ULONG *)p, s->ffp);
    case 7: {                                           /* compstrs */
        /* d0/a0 = left (pivot), d1/a1 = right: pointers to the
           characters, lengths at -4.  strcomp2 returns -1/0/1 in d0 and
           the same in the CCR (MOVEQ / TST.l d0). */
        const ULONG *l = (const ULONG *)pivot, *r = *(const ULONG *const *)p;
        struct bl_regs rg = { .d0 = l[-1], .d1 = r[-1], .a0 = (ULONG)l, .a1 = (ULONG)r };
        return (LONG)BLITZ_ALIBJSR(TOK_strcomp2, rg);
    }
    default: {
        LONG a = (LONG)pivot, b = *(const LONG *)p;
        return a > b ? 1 : a < b ? -1 : 0;
    }
    }
}

static void swap(const struct sorter *s, UBYTE *a, UBYTE *b)
{
    if (s->size == 1) {
        UBYTE t = *a; *a = *b; *b = t;
    } else if (s->size == 2) {
        UWORD t = *(UWORD *)a; *(UWORD *)a = *(UWORD *)b; *(UWORD *)b = t;
    } else {
        ULONG t = *(ULONG *)a; *(ULONG *)a = *(ULONG *)b; *(ULONG *)b = t;
    }
}

/* .'recurs - a0 = L (lo), a1 = R (hi), both inclusive element addresses;
   pointer comparisons are unsigned (BHI/BLS/BCC) */
static void recurs(const struct sorter *s, UBYTE *lo, UBYTE *hi)
{
    ULONG step = s->size;

    for (;;) {
        UBYTE *first = lo, *last = hi;                  /* copies of */
        ULONG off = (ULONG)(hi - lo) >> 1;              /* LSR.l #1,d0 */
        ULONG pivot;
        UBYTE *scan_lo = lo, *scan_hi = hi;

        if (step == 1)
            pivot = lo[off];                            /* MOVE.b 0(a0,d0.l),d0 */
        else if (step == 2)                             /* AND.l #$fffffffe:MOVE.l: */
            pivot = *(const ULONG *)(lo + (off & ~1UL));   /* low word = next element */
        else
            pivot = *(const ULONG *)(lo + (off & ~3UL));

        do {                                            /* 'loop0 */
            if (sdir == 0) {
                while (compare(s, pivot, scan_lo) > 0)  /* 'loop1: BGT */
                    scan_lo += step;
                while (compare(s, pivot, scan_hi) < 0)  /* 'loop2: BLT */
                    scan_hi -= step;
            } else {
                while (compare(s, pivot, scan_lo) < 0)  /* 'loop3: BLT */
                    scan_lo += step;
                while (compare(s, pivot, scan_hi) > 0)  /* 'loop4: BGT */
                    scan_hi -= step;
            }
            if (scan_lo > scan_hi)                      /* 'lskip: BHI 'skip */
                break;
            swap(s, scan_lo, scan_hi);
            scan_lo += step;
            scan_hi -= step;
        } while (scan_lo <= scan_hi);                   /* BLS 'loop0 */

        if (first < scan_hi)                            /* CMP.l a1,d6:BCC 'skip2 */
            recurs(s, first, scan_hi);
        if (!(scan_lo < last))                          /* CMP.l d7,a0:BCC 'skip3 */
            return;
        lo = scan_lo;                                   /* MOVE.l d7,a1:BRA 'recurs */
        hi = last;
    }
}

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
 * BUG(orig) (#275): for word arrays the pivot is read with MOVE.l and
 * compared with CMP.w, so it is the element after the middle one; for a
 * one element array it lies behind the array (reads only, the result is
 * right).
 *
 * BUG(orig) (#276): string elements that were never assigned (pointer 0)
 * read their length from -4(0).
 *
 * @param base d0: the base (lowest element) of the array data
 * @param args a2: the stack pointer before the pushes: -4 = the end
 *             address (long), -6 = the element type (word)
 * @param ffp  a6: the mathffp base (ffplib), for float arrays
 */
void BLITZ_ROUTINE dosort(ULONG base BL_REG(d0), const UBYTE *args BL_REG(a2),
                          const void *ffp BL_REG(a6))
{
    struct sorter s;
    UBYTE *hi = (UBYTE *)*(const ULONG *)(args - 4);    /* MOVE.l -(a2),a1 */

    s.type = *(const UWORD *)(args - 6);                /* MOVE -(a2),d1 */
    s.ffp = ffp;
    switch (s.type) {
    case 1:
        s.size = 1;                                     /* sortbyte */
        break;
    case 2:
        s.size = 2;                                     /* sortword */
        break;
    case 5:                                             /* sortfloat */
    case 7:                                             /* sortstring */
        s.size = 4;
        break;
    default:
        s.type = 3;                                     /* sortlong (also 4) */
        s.size = 4;
        break;
    }
    recurs(&s, (UBYTE *)base, hi - s.size);             /* SUBQ #size,a1 */
}
