/*
 * risnowlib - RWE userlib 195 (literal in the source, Reflective Images,
 * 1994): falling snow in a bitmap, driven by lists of pixels -
 *   InitSnowDrop(listnum,numpoints,bitmap,bp_size)  the lists of a snow
 *                       field (returns the address of the list table)
 *   LDoSnow listnum,randomtable    one step: every flake one row down
 *                       (or aside, or stopped), plus a new one
 *   KillPoint(listnum,x,y)         removes a "blue" pixel (plane 2)
 *   LDoSnowActivate(listnum)       stopped snow next to removed pixels
 *                       falls again
 *
 * C translation of rwelibsrc/Snow/SnowLib3.bb; reference: the reference
 * build of that source with blitz2 2.10 (REFERENCE refbuild,
 * ref/risnowlib.obj) - no binary of the library exists.  Notes:
 * docs/rwelibs/port/risnowlib.md.
 *
 * The list table (listpointers): 4 snow fields of 4 longs - drop list A,
 * drop list B, kill list 1, kill list 2.  InitSnowDrop allocates one
 * block (memlib's globalloc, MEMF_CLEAR) for the four lists, each
 * numpoints*16 bytes: a drop list starts with size.l, bitmap.l (the
 * Blitz bitmap object), bp_size.l, last.l (the end of its entries), a
 * kill list with last.l; the entries are offset.w (the byte in the
 * plane), bit.w, ended by $ABCD.  The bitmap's planes: 0 the snow, 1 the
 * stopped snow ("pause"), 2 the "blue" pixels.
 *
 * LDoSnow reads the newer drop list and writes the other (the two swap
 * places): a flake moves one row down if the pixel there is clear,
 * else to the side (left or right first, alternating) where the pixel
 * and the one below it are clear, else it stays; if it cannot move and
 * plane 1 has a pixel below it, it stops (set in plane 1, dropped from
 * the list).  A flake below bp_size is removed.  At the end of the list
 * a new flake from the random table (word index, then pairs offset.b,
 * bit.b; the index steps by 2 modulo 1024).  The routine sets COLOR00 to
 * red while it runs ($F00, then 0: a debug flash).
 *
 * LDoSnow is assembler (the original's loop: as C it was 4.2 times
 * slower); the rest C behind thunks.  The lookup tables keep the
 * original's order (left-first then right-first, 8 entries of 4 words).
 *
 * BUG(orig):
 *   - The lists have no bounds: more entries than numpoints*4 run into
 *     the next list (and behind the block) (#914).
 * Fixed (intended deviations):
 *   - InitSnowDrop computes the size of its block as a long; the
 *     original took numpoints*16 times 4 with ASL.w #2, so from
 *     numpoints 1024 on the block was too small for the lists written
 *     into it (#912);
 *   - InitSnowDrop sets the drop lists' "last" to their end mark, as the
 *     kill lists'; the original left it 0 until the first LDoSnow, so an
 *     LDoSnowActivate before it wrote to address 0 (#908);
 *   - LDoSnowActivate stores the new end of the drop list into its
 *     "last"; the original stored only the kill list's, so a second
 *     LDoSnowActivate before the next LDoSnow overwrote the flakes the
 *     first released (#909);
 *   - init clears the list table, as at the first load; the original's
 *     init was empty, so in a second run of a program from the editor
 *     (the library stays loaded) the table pointed to the lists the first
 *     run had freed, and a snow command before InitSnowDrop worked on
 *     them (#1135);
 *   - InitSnowDrop frees the block the list number had (memlib's
 *     globfree) before it allocates the new one; the original overwrote
 *     the pointers, and each repeated InitSnowDrop of a number lost
 *     numpoints*64+40 bytes until the end of the program (#911).  The
 *     block is the lower of the two drop lists (drop list A, at its
 *     start; LDoSnow swaps the two pointers);
 *   - With runtime error checking every command refuses a list number
 *     outside 0-3: "Snow list number must be 0 to 3" (the original has
 *     no error check area; 16 bytes per number indexed by ASL.w #4 in a
 *     table of 4 numbers, so 4 and up, negative numbers and a non-zero
 *     upper word read and write behind or before the table) (#913).
 *     With it LDoSnow, KillPoint and LDoSnowActivate refuse a list
 *     number without InitSnowDrop ("Snow list needs InitSnowDrop": the
 *     lists would be read at address 0), and KillPoint a pixel outside
 *     the 320 pixels of a row or the bitmap's rows ("KillPoint outside
 *     the bitmap": the library assumes 40 bytes per row).  The release
 *     code is unchanged (invalid arguments, D-005);
 *   - LDoSnow takes the bit of a new flake from the random table modulo
 *     8 (AND #7); the original took the whole byte, and a blocked flake
 *     with a bit above 7 looked up its side moves bit*8 bytes into the
 *     tables of 8 rows: bits 8-15 read the right-first table behind the
 *     left-first one (the wrong neighbours), higher ones the code behind
 *     the tables (#910).  The bit operations themselves took it modulo 8.
 * Not defects: the random table's offsets are bytes (new flakes start
 * in the first 6.4 rows); the allocation is not checked (memlib's globalloc raises
 * Out of Memory itself); LDoSnowActivate checks plane 2 at the killed
 * pixel again (always clear there: redundant, no effect); the end of
 * LDoSnow behind its loop ('endthis) is dead code: the loop ends only at
 * $ABCD with a new flake.
 */
#include "blitzlib.h"
#include <graphics/gfx.h>

/* entry points (thunks, below) */
void _AsmDoSnow(void), _InitSnowDrop(void), _KillPoint(void);
void _DoKillList(void), risnowlib_libinit(void), risnowlib_libfinit(void);
void initchk(void), fieldchk(void), killchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {195,init,0,finit,0}; error field: the error check area of
   the list number checks (fixed, #913) */
BLITZ_LIBHEADER(LIB_risnowlib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_AsmDoSnow, fieldchk, 0);           /* check: fixed #913 */
BLITZ_NAME("LDoSnow", "listnum,randomtable");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, BL_IA3|BL_PD2);
BLITZ_SUBS(_InitSnowDrop, initchk, 0);         /* check: fixed #913 */
BLITZ_NAME("InitSnowDrop", "listnum,numpoints,bitmap,bp_size");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_KillPoint, killchk, 0);            /* check: fixed #913 */
BLITZ_NAME("KillPoint", "listnum,x,y");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_DoKillList, fieldchk, 0);          /* check: fixed #913 */
BLITZ_NAME("LDoSnowActivate", "listnum");

BLITZ_NULLSUB_AT(finit, risnowlib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, risnowlib_libinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------- the data */

#define END 0xabcd                      /* the end of a list */

/* The lists' addresses are longs, as in the original (as pointers GCC
   builds other code). */

/* a drop list: its header, then the entries (offset.w, bit.w) up to END */
struct droplist {
    ULONG size;                         /* +0: numpoints*16 */
    ULONG bm;                           /* +4: the Blitz bitmap object (struct BitMap) */
    ULONG bpsize;                       /* +8: compared as a word */
    ULONG last;                         /* +12: the END behind the entries */
    UWORD entries[];                    /* +16 */
};
static_assert(sizeof(struct droplist) == 16);

/* a kill list: last, then the entries */
struct killlist {
    ULONG last;                         /* +0: the END behind the entries */
    UWORD entries[];                    /* +4 */
};
static_assert(sizeof(struct killlist) == 4);

/* listpointers: per list number drop A, drop B, kill 1, kill 2 (the
   struct droplist / struct killlist) */
struct snowfield {
    ULONG drop[2];                      /* [0] the newer one */
    ULONG kill[2];                      /* [0] the newer one */
};
static_assert(sizeof(struct snowfield) == 16);
static struct snowfield lp[4];

/* the random table: the word index, then pairs offset.b, bit.b */
struct randomtable {
    UWORD index;
    UBYTE pairs[1024];
};

/* lookup_leftfirst, lookup_rightfirst: per bit (7 = the left pixel of a
   byte) the first and the second side to try, as offset.w, bit.w */
struct side {
    WORD offset;
    UWORD bit;
};
static const struct side lookup[2][8][2] = {
    { { { 0, 1 }, { 1, 7 } }, { { 0, 2 }, { 0, 0 } }, { { 0, 3 }, { 0, 1 } },
      { { 0, 4 }, { 0, 2 } }, { { 0, 5 }, { 0, 3 } }, { { 0, 6 }, { 0, 4 } },
      { { 0, 7 }, { 0, 5 } }, { { -1, 0 }, { 0, 6 } } },
    { { { 1, 7 }, { 0, 1 } }, { { 0, 0 }, { 0, 2 } }, { { 0, 1 }, { 0, 3 } },
      { { 0, 2 }, { 0, 4 } }, { { 0, 3 }, { 0, 5 } }, { { 0, 4 }, { 0, 6 } },
      { { 0, 5 }, { 0, 7 } }, { { 0, 6 }, { -1, 0 } } },
};
static_assert(sizeof lookup == 2 * 8 * 4 * 2);

/* ASL.w #n,d: the low word shifted, the upper word kept */
#define ASLW(d, n) (((ULONG)(d) & 0xffff0000UL) | (UWORD)((ULONG)(d) << (n)))

/* the 4 longs of a list number (ASL.w #4,d0: ADD.l d0,a2); not checked
   in the release code (the error checks refuse a number outside 0-3:
   fixed #913) */
static struct snowfield *entry(ULONG n)
{
    return (struct snowfield *)((UBYTE *)lp + (LONG)ASLW(n, 4));
}

#define DROP(a) ((struct droplist *)(a))
#define KILL(a) ((struct killlist *)(a))
/* plane n of a Blitz bitmap object (struct BitMap) at address bm */
#define PLANE(bm, n) ((UBYTE *)*(ULONG *)&((const struct BitMap *)(bm))->Planes[n])

/* BTST/BCLR/BSET Dn,0(An,Dm.w), the original's instructions: the bit
   modulo 8 of the byte at p + the word index idx (sign-extended).  Inline
   assembler: as C (shifts and masks, the helpers not inlined at -Os)
   LDoSnow took 4.2 times the original's cycles.  The planes are read and
   written only through these (volatile, kept in order). */
static inline __attribute__((always_inline)) int btst(const UBYTE *p, UWORD idx, UWORD bit)
{
    UBYTE r;

    __asm volatile("btst %2,0(%1,%3.w)\n\tsne %0" : "=d"(r) : "a"(p), "d"(bit), "d"(idx) : "cc");
    return r;
}

static inline __attribute__((always_inline)) void bclr(UBYTE *p, UWORD idx, UWORD bit)
{
    __asm volatile("bclr %1,0(%0,%2.w)" : : "a"(p), "d"(bit), "d"(idx) : "cc", "memory");
}

static inline __attribute__((always_inline)) void bset(UBYTE *p, UWORD idx, UWORD bit)
{
    __asm volatile("bset %1,0(%0,%2.w)" : : "a"(p), "d"(bit), "d"(idx) : "cc", "memory");
}

/**
 * @brief init: clears the list table, as at the first load.
 *
 * Fixed (#1135): the original's init was an RTS; in a second run of a
 * program from the editor (the library stays loaded, memlib has freed
 * the blocks) the table still pointed to the first run's lists.
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    ULONG *p = (ULONG *)lp;
    int i;

    for (i = 0; i < 4 * 4; i++)          /* 4 snow fields of 4 longs */
        p[i] = 0;
}

/** @brief finit: RTS (nothing to do). */
static void BLITZ_ROUTINE nothing_c(void)
{
}

/* ------------------------------------------------------- InitSnowDrop */
/* a drop list's header at p: size, bitmap, bp_size, last = its first
   entry (fixed (#908): the original skipped it, 0), then END */
static UBYTE *drophead(UBYTE *p, ULONG size, ULONG bm, ULONG bpsize)
{
    DROP(p)->size = size;
    DROP(p)->bm = bm;
    DROP(p)->bpsize = bpsize;
    DROP(p)->last = (ULONG)DROP(p)->entries;
    DROP(p)->entries[0] = END;
    return p + 16 + size;
}

/* a kill list at p: last = its first entry, END */
static UBYTE *killhead(UBYTE *p, ULONG size)
{
    KILL(p)->last = (ULONG)(p + 4);
    KILL(p)->entries[0] = END;
    return p + 4 + size;
}

/**
 * @brief InitSnowDrop(listnum,numpoints,bitmap,bp_size): the four lists
 * of a snow field.
 *
 * One block (memlib globalloc, MEMF_CLEAR; not checked: memlib raises
 * Out of Memory) for drop lists A and B and kill lists 1 and 2, each
 * numpoints*16 bytes, all empty.  Fixed (#911): the block the number had is freed first
 * (memlib globfree; the original lost it).  Fixed (#912): the size is a long (the original's ASL.w #2
 * made the block too small from numpoints 1024 on).  Fixed (#908): the
 * drop lists' last is set (the original left it 0).
 * @param n       d0: the list number (0-3)
 * @param num     d1: the number of points
 * @param bpsize  d3: the end offset in the plane (bp_size)
 * @param bm      a3: the bitmap object (!libs #bitmaplib,#ia3|#pd2)
 * @return d0: the address of the list table (listpointers)
 */
static ULONG BLITZ_ROUTINE initsnowdrop_c(ULONG n BL_REG(d0), ULONG num BL_REG(d1),
                                          ULONG bpsize BL_REG(d3), ULONG bm BL_REG(a3))
{
    struct snowfield *e = entry(n);
    ULONG size = num << 4;              /* each list */
    struct bl_regs r;
    UBYTE *p;

    if (e->drop[0]) {                   /* fixed (#911): the old block freed */
        r.d0 = 0;
        r.d1 = 0;
        r.a0 = 0;
        r.a1 = e->drop[0] < e->drop[1] ? e->drop[0] : e->drop[1];   /* drop list A */
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
    r.d0 = (size << 2) + 32 + 8;        /* a long: fixed (#912) */
    r.d1 = 0x10000;                     /* MEMF_CLEAR */
    r.a0 = 0;
    r.a1 = 0;
    BLITZ_ALIBJSR(TOK_globalloc, r);
    p = (UBYTE *)r.d0;                  /* not checked: memlib raises Out of Memory */
    e->drop[0] = (ULONG)p;
    p = drophead(p, size, bm, bpsize);
    e->drop[1] = (ULONG)p;
    p = drophead(p, size, bm, bpsize);
    e->kill[0] = (ULONG)p;
    p = killhead(p, size);
    e->kill[1] = (ULONG)p;
    killhead(p, size);
    return (ULONG)lp;
}

/* ------------------------------------------------------------ LDoSnow */

/**
 * @brief LDoSnow listnum,randomtable: one step of the snow.
 *
 * Reads the newer drop list and writes the other (they swap): a flake
 * moves one row down if the pixel there is clear, else to the side (left
 * or right first, alternating) where the pixel and the one below are
 * clear, else it stays; one that cannot move with plane 1 set below it
 * stops (set in plane 1, dropped).  A flake below bp_size (compared as a
 * signed word) is removed.  Then a new flake from the random table (the
 * index steps by 2 modulo 1024).  COLOR00 is red while it runs ($F00,
 * then 0: a debug flash).
 *
 * An assembler entry, the original's loop (registers as there: a1 plane
 * 0, a4 plane 1, a0 the list read, a2 the list written, a5 the random
 * table, d7 bp_size, d6 left or right first, a3 the side table): as C
 * (even with the bit operations as inline assembler) it took 4.2 and 1.7
 * times the original's cycles per call (perf bsnow).  Keeps d1/a0/a1 and
 * a4-a6 (the original a4-a6, the C thunk d1/a0/a1); changes d0, d2-d7,
 * a2, a3.
 *
 * BUG(orig): no bounds for the lists (#914).  The list number: fieldchk
 * (fixed #913).  Fixed (#910): the bit of a new flake is taken modulo 8
 * (a bit above 7 read behind the side tables).
 * @arg d0 the list number
 * @arg d1 the random table (index.w, then pairs offset.b, bit.b)
 */
BLITZ_ASM_ROUTINE(_AsmDoSnow,
"   movem.l d1/a0-a1/a4-a6,-(sp)\n"
"   move.w  #0x0f00,0xdff180\n"           /* COLOR00 red */
"   lea     _lp(pc),a2\n"
"   asl.w   #4,d0\n"                      /* ASL.w: the list number's upper word kept */
"   add.l   d0,a2\n"
"   move.l  (a2),d0\n"                    /* the newer list: read */
"   move.l  4(a2),d2\n"                   /* the other: written */
"   move.l  d2,(a2)\n"
"   move.l  d0,4(a2)\n"
"   move.l  d1,a5\n"
"   move.l  d0,a0\n"
"   move.l  4(a0),a1\n"                   /* the bitmap */
"   move.l  8(a0),d7\n"                   /* bp_size */
"   lea     16(a0),a0\n"                  /* its entries */
"   add.l   #12,d2\n"
"   move.l  d2,-(sp)\n"                   /* &last of the list written */
"   addq.l  #4,d2\n"
"   move.l  d2,a2\n"                      /* its entries */
"   move.l  12(a1),a4\n"                  /* plane 1 */
"   move.l  8(a1),a1\n"                   /* plane 0 */
"   moveq   #0,d4\n"
"   moveq   #0,d6\n"
"0: cmp.w   #0xabcd,(a0)\n"               /* 'loop */
"   beq.s   8f\n"
"   move.w  (a0)+,d1\n"
"   move.w  (a0)+,d2\n"
"   move.w  d1,d3\n"
"   add.w   #40,d1\n"
"   cmp.w   d1,d7\n"
"   bgt.s   1f\n"
"   bclr    d2,0(a1,d3.w)\n"              /* out of the plane */
"   bra.s   0b\n"
"1: btst    d2,0(a1,d1.w)\n"              /* 'insidebitplane */
"   bne.s   2f\n"
"   bclr    d2,0(a1,d3.w)\n"              /* 'drawit */
"   bset    d2,0(a1,d1.w)\n"
"   move.w  d1,(a2)+\n"
"   move.w  d2,(a2)+\n"
"   bra.s   0b\n"
"8: move.w  (a5),d4\n"                    /* 'addone: a new flake */
"   moveq   #0,d1\n"
"   move.b  2(a5,d4.w),d1\n"
"   move.w  d1,(a2)+\n"
"   move.b  3(a5,d4.w),d1\n"
"   and.w   #7,d1\n"                      /* the bit modulo 8: fixed (#910) */
"   move.w  d1,(a2)+\n"
"   addq.w  #2,d4\n"
"   and.w   #1023,d4\n"
"   move.w  d4,(a5)\n"
"   move.w  #0xabcd,(a2)\n"
"   move.l  (sp)+,a0\n"
"   move.l  a2,(a0)\n"                    /* last */
"   move.w  #0,0xdff180\n"
"   movem.l (sp)+,d1/a0-a1/a4-a6\n"
"   rts\n"
"2: lea     _lookup(pc),a3\n"             /* 'look: left first */
"   tst.b   d6\n"
"   jeq     3f\n"
"   lea     64(a3),a3\n"                  /* right first */
"   moveq   #0,d6\n"
"   jra     4f\n"
"3: moveq   #1,d6\n"
"4: move.w  d2,d4\n"                      /* 'gottable: bit * 8 bytes in */
"   asl.w   #3,d4\n"
"   add.l   d4,a3\n"
"   move.w  d3,d5\n"                      /* 'doleft */
"   add.w   (a3)+,d5\n"
"   move.w  (a3)+,d4\n"
"   btst    d4,0(a1,d5.w)\n"
"   jne     5f\n"
"   btst    d4,40(a1,d5.w)\n"
"   jne     5f\n"
"   bclr    d2,0(a1,d3.w)\n"
"   bset    d4,0(a1,d5.w)\n"
"   move.w  d5,(a2)+\n"
"   move.w  d4,(a2)+\n"
"   jra     0b\n"
"5: move.w  d3,d5\n"                      /* 'doright */
"   add.w   (a3)+,d5\n"
"   move.w  (a3)+,d4\n"
"   btst    d4,0(a1,d5.w)\n"
"   jne     6f\n"
"   btst    d4,40(a1,d5.w)\n"
"   jne     6f\n"
"   bclr    d2,0(a1,d3.w)\n"
"   bset    d4,0(a1,d5.w)\n"
"   move.w  d5,(a2)+\n"
"   move.w  d4,(a2)+\n"
"   jra     0b\n"
"6: btst    d2,40(a4,d3.w)\n"             /* 'skipitstore */
"   jne     7f\n"
"   move.w  d3,(a2)+\n"
"   move.w  d2,(a2)+\n"
"   jra     0b\n"
"7: bset    d2,0(a4,d3.w)\n"              /* 'okpause: stopped */
"   jra     0b");


/* ---------------------------------------------------- LDoSnowActivate */

/* stopped snow (plane 1 set, plane 2 clear) at the pixel `at`/`bit`:
   cleared and added to the drop list */
static int unpause(UBYTE *plane1, const UBYTE *plane2, UWORD at, UWORD bit, UWORD **drop)
{
    if (btst(plane2, at, bit) || !btst(plane1, at, bit))
        return 0;
    bclr(plane1, at, bit);
    *(*drop)++ = at;
    *(*drop)++ = bit;
    return 1;
}

/**
 * @brief LDoSnowActivate(listnum): stopped snow next to removed pixels
 * falls again.
 *
 * For each pixel of the kill list (the two kill lists swap): stopped snow
 * (plane 1 set, plane 2 clear) at the two sides below and at the pixel
 * is cleared in plane 1 and added to the newer drop list; the pixel above
 * goes into the other kill list.  Checking plane 2 at the killed pixel
 * again is redundant (no defect).  The bits of the entries are 0-7
 * (fixed #910: LDoSnow masks the random table's).  The list number: fieldchk (fixed #913).  Fixed (#909): the drop list's
 * last is updated (the original's stayed, so the next call overwrote
 * the entries).  Fixed (#908): before the first LDoSnow the drop list's last is
 * its first entry (InitSnowDrop sets it; the original's 0 wrote at
 * address 0).
 * @param n  d0: the list number
 * @return d0: the number of kill list entries + 1
 */
static ULONG BLITZ_ROUTINE dokilllist_c(ULONG n BL_REG(d0))
{
    struct snowfield *e = entry(n);
    UBYTE *drops = (UBYTE *)e->drop[0], *killin, *killout, *plane1, *plane2;
    const UBYTE *bm = (const UBYTE *)DROP(drops)->bm;
    const UBYTE *look = (const UBYTE *)lookup[0];
    UWORD *drop = (UWORD *)DROP(drops)->last, *src, *dst;  /* set by InitSnowDrop:
                                                              fixed (#908) */
    ULONG count = 0;

    plane1 = PLANE(bm, 1);
    plane2 = PLANE(bm, 2);
    killin = (UBYTE *)e->kill[0];
    killout = (UBYTE *)e->kill[1];
    e->kill[0] = (ULONG)killout;
    e->kill[1] = (ULONG)killin;
    src = KILL(killin)->entries;
    dst = KILL(killout)->entries;
    for (;;) {
        UWORD at, bit, idx;
        int found = 0;

        count++;
        if (*src == END)
            break;
        at = *src++;
        bit = *src++;
        if (btst(plane2, at, bit))
            continue;
        idx = (UWORD)(bit << 3);        /* bit 0-7: fixed (#910) */
        /* the two sides below (left-first table: struct side at idx and
           idx + 4), then the pixel */
        found |= unpause(plane1, plane2, (UWORD)(at + 40 + *(const WORD *)(look + (WORD)idx)),
                         *(const UWORD *)(look + (WORD)idx + 2), &drop);
        found |= unpause(plane1, plane2, (UWORD)(at + 40 + *(const WORD *)(look + (WORD)idx + 4)),
                         *(const UWORD *)(look + (WORD)idx + 6), &drop);
        found |= unpause(plane1, plane2, at, bit, &drop);
        if (found) {
            *dst++ = (UWORD)(at - 40);
            *dst++ = bit;
        }
    }
    *dst = END;
    *drop = END;
    DROP(drops)->last = (ULONG)drop;    /* fixed (#909): the original kept it */
    KILL(killout)->last = (ULONG)dst;
    return count;
}

/* ---------------------------------------------------------- KillPoint */

/**
 * @brief KillPoint(listnum,x,y): removes a "blue" pixel (plane 2).
 *
 * The pixel is cleared in all three planes and the pixel above it added
 * to the kill list.  The list number and the pixel: killchk (fixed #913).
 * @param n  d0: the list number
 * @param x  d1: x (a word)
 * @param y  d2: y (a word; 40 bytes per row)
 * @return d0: BB_TRUE, BB_FALSE if the pixel was clear in plane 2
 */
static BB_BOOL BLITZ_ROUTINE killpoint_c(ULONG n BL_REG(d0), ULONG x BL_REG(d1),
                                         ULONG y BL_REG(d2))
{
    struct snowfield *e = entry(n);
    UBYTE *kill = (UBYTE *)e->kill[0];
    UWORD *dst = (UWORD *)KILL(kill)->last;
    const UBYTE *bm = (const UBYTE *)DROP(e->drop[0])->bm;
    UBYTE *plane0 = PLANE(bm, 0), *plane1 = PLANE(bm, 1), *plane2 = PLANE(bm, 2);
    UWORD at = (UWORD)((UWORD)y * 40UL + (UWORD)((WORD)x >> 3));
    UWORD bit = (UWORD)(~x & 7);

    if (!btst(plane2, at, bit))
        return BB_FALSE;
    bclr(plane2, at, bit);
    bclr(plane1, at, bit);
    bclr(plane0, at, bit);
    *dst++ = (UWORD)(at - 40);
    *dst++ = bit;
    *dst = END;
    KILL(kill)->last = (ULONG)dst;
    return BB_TRUE;
}

/* Registers: the original changes d0-d7/a0-a3 (keeps a4-a6); the thunks
   keep d1/a0/a1 (and d0 for init and finit, which return nothing), the C
   code d2-d7/a2-a6. */
BLITZ_THUNK(risnowlib_libinit, libinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(risnowlib_libfinit, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_InitSnowDrop, initsnowdrop_c, "d1/a0-a1");
BLITZ_THUNK(_DoKillList, dokilllist_c, "d1/a0-a1");
BLITZ_THUNK(_KillPoint, killpoint_c, "d1/a0-a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(badlist, "Snow list number must be 0 to 3");   /* fixed, #913 */
BLITZ_ERRCHK_MESSAGE(noinit, "Snow list needs InitSnowDrop");       /* fixed, #913 */
BLITZ_ERRCHK_MESSAGE(outside, "KillPoint outside the bitmap");      /* fixed, #913 */

/* the list number n: 0 to 3 (the whole long) */
static void BLITZ_ERRCHK listchk(ULONG n)
{
    if (n >= 4)
        BLITZ_RUNERR(badlist);
}

/* the list number n, and a field InitSnowDrop made for it */
static void BLITZ_ERRCHK initdonechk(ULONG n)
{
    listchk(n);
    if (!lp[n].drop[0])
        BLITZ_RUNERR(noinit);
}

/**
 * @brief The error check of InitSnowDrop (debugger only).
 *
 * Fixed (#913), not in the original (risnowlib had no error check area):
 * the list number indexes the table of 4 numbers with ASL.w #4 without a
 * check, so 4 and up (and negative numbers, a non-zero upper word) wrote
 * the four list pointers behind or before the table.  The release code is
 * unchanged (an invalid argument, D-005).
 *
 * Errors: "Snow list number must be 0 to 3".
 * @param n  d0: the list number
 */
static void BLITZ_ERRCHK initchk_c(ULONG n BL_REG(d0))
{
    listchk(n);
}

/**
 * @brief The error check of LDoSnow and LDoSnowActivate (debugger only).
 *
 * Fixed (#913), not in the original: the list number was not checked
 * (lists and bitmap read behind the table); with it a list number
 * without InitSnowDrop, whose entry is 0 (the lists and the bitmap read
 * at address 0).  The release code is unchanged (D-005).
 *
 * Errors: "Snow list number must be 0 to 3", "Snow list needs
 * InitSnowDrop".
 * @param n  d0: the list number
 */
static void BLITZ_ERRCHK fieldchk_c(ULONG n BL_REG(d0))
{
    initdonechk(n);
}

/**
 * @brief The error check of KillPoint (debugger only).
 *
 * Fixed (#913), not in the original: as fieldchk_c, and the pixel: x
 * outside the 320 pixels of a row (the library assumes 40 bytes per row)
 * or y outside the rows of the field's bitmap cleared bits and appended
 * kill entries outside the planes.  The release code is unchanged (D-005).
 *
 * Errors: "Snow list number must be 0 to 3", "Snow list needs
 * InitSnowDrop", "KillPoint outside the bitmap".
 * @param n  d0: the list number
 * @param x  d1: x (a word)
 * @param y  d2: y (a word)
 */
static void BLITZ_ERRCHK killchk_c(ULONG n BL_REG(d0), ULONG x BL_REG(d1), ULONG y BL_REG(d2))
{
    const struct BitMap *bm;

    initdonechk(n);
    bm = (const struct BitMap *)DROP(lp[n].drop[0])->bm;
    if ((UWORD)x >= 320 || (UWORD)y >= bm->Rows)
        BLITZ_RUNERR(outside);
}

/* the thunks of the check functions: they keep every register */
BLITZ_ERRCHK_THUNK(initchk, initchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(fieldchk, fieldchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(killchk, killchk_c, "d0-d1/a0-a1");
