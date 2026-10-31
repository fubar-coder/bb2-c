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
 * No assembler besides the thunks.  The lookup tables keep the
 * original's order (left-first then right-first, 8 entries of 4 words).
 *
 * BUG(orig):
 *   - No routine checks the list number (0-3; 16 bytes per number
 *     indexed by ASL.w #4) (#913); InitSnowDrop computes the size with
 *     ASL.w #2 on numpoints*16 (numpoints from 1024 on the block is too
 *     small for the lists written into it) (#912) and never frees an
 *     earlier block of the same number (#911).
 *   - The drop lists' "last" stays 0 until the first LDoSnow:
 *     LDoSnowActivate before it writes to address 0 (#908).
 *   - The lists have no bounds: more entries than numpoints*4 run into
 *     the next list (and behind the block) (#914).
 *   - A bit number above 7 (from the random table) indexes the lookup
 *     tables beyond their end; the offsets of the table are bytes (only
 *     the first rows) (#910).
 *   - LDoSnowActivate does not update the drop list's "last" (the next
 *     LDoSnowActivate before an LDoSnow overwrites the entries) (#909).
 * Not defects: the allocation is not checked (memlib's globalloc raises
 * Out of Memory itself); LDoSnowActivate checks plane 2 at the killed
 * pixel again (always clear there: redundant, no effect); the end of
 * LDoSnow behind its loop ('endthis) is dead code: the loop ends only at
 * $ABCD with a new flake.
 */
#include "blitzlib.h"
#include <graphics/gfx.h>

/* entry points (thunks, below) */
void _AsmDoSnow(void), _InitSnowDrop(void), _KillPoint(void), _DoKillList(void);
void _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))   /* #ia<a>|#pd<p> */

/* !libheader {195,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_risnowlib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_AsmDoSnow, 0, 0);
BLITZ_NAME("LDoSnow", "listnum,randomtable");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, IA(3, 2));
BLITZ_SUBS(_InitSnowDrop, 0, 0);
BLITZ_NAME("InitSnowDrop", "listnum,numpoints,bitmap,bp_size");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_KillPoint, 0, 0);
BLITZ_NAME("KillPoint", "listnum,x,y");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_DoKillList, 0, 0);
BLITZ_NAME("LDoSnowActivate", "listnum");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------- the data */

#define END 0xabcd                      /* the end of a list */
#define COLOR00 (*(volatile UWORD *)0xdff180)

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

/* the 4 longs of a list number (ASL.w #4,d0: ADD.l d0,a2), BUG(orig):
   not checked (#913) */
static struct snowfield *entry(ULONG n)
{
    return (struct snowfield *)((UBYTE *)lp + (LONG)ASLW(n, 4));
}

#define DROP(a) ((struct droplist *)(a))
#define KILL(a) ((struct killlist *)(a))
/* plane n of a Blitz bitmap object (struct BitMap) at address bm */
#define PLANE(bm, n) ((UBYTE *)*(ULONG *)&((const struct BitMap *)(bm))->Planes[n])

/* BTST/BCLR/BSET Dn,d(An,Dm.w): the bit modulo 8 of a byte */
static int btst(const UBYTE *p, UWORD idx, UWORD bit)
{
    return p[(WORD)idx] >> (bit & 7) & 1;
}

static void bclr(UBYTE *p, UWORD idx, UWORD bit)
{
    p[(WORD)idx] &= (UBYTE)~(1 << (bit & 7));
}

static void bset(UBYTE *p, UWORD idx, UWORD bit)
{
    p[(WORD)idx] |= (UBYTE)(1 << (bit & 7));
}

/* init, finit: RTS */
static void BLITZ_ROUTINE nothing_c(void)
{
}

/* ------------------------------------------------------- InitSnowDrop */
/* a drop list's header at p: size, bitmap, bp_size, (last), then END */
static UBYTE *drophead(UBYTE *p, ULONG size, ULONG bm, ULONG bpsize)
{
    DROP(p)->size = size;
    DROP(p)->bm = bm;
    DROP(p)->bpsize = bpsize;
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

/* InitSnowDrop(listnum,numpoints,bitmap,bp_size) (d0, d1, d3; a3 = the
   bitmap object) */
static ULONG BLITZ_ROUTINE initsnowdrop_c(ULONG n BL_REG(d0), ULONG num BL_REG(d1),
                                          ULONG bpsize BL_REG(d3), ULONG bm BL_REG(a3))
{
    struct snowfield *e = entry(n);
    ULONG size = num << 4;              /* each list */
    struct bl_regs r;
    UBYTE *p;

    r.d0 = ASLW(size, 2) + 32 + 8;      /* BUG(orig): ASL.w (#912) */
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

/* LDoSnow listnum,randomtable (d0, d1) */
static void BLITZ_ROUTINE asmdosnow_c(ULONG n BL_REG(d0), struct randomtable *rt BL_REG(d1))
{
    struct snowfield *e;
    UBYTE *in, *out, *plane0, *plane1;
    const UBYTE *bm;
    UWORD bpsize, i;
    UBYTE flip = 0;                     /* d6: left or right first */

    COLOR00 = 0x0f00;
    e = entry(n);
    in = (UBYTE *)e->drop[0];           /* the newer list: read */
    out = (UBYTE *)e->drop[1];          /* the other: written */
    e->drop[0] = (ULONG)out;
    e->drop[1] = (ULONG)in;
    bm = (const UBYTE *)DROP(in)->bm;
    bpsize = (UWORD)DROP(in)->bpsize;   /* compared as a word */
    plane1 = PLANE(bm, 1);
    plane0 = PLANE(bm, 0);
    {
        UWORD *src = DROP(in)->entries, *dst = DROP(out)->entries;

        while (*src != END) {
            UWORD at = *src++, bit = *src++, below = (UWORD)(at + 40);
            const struct side *look;
            UWORD side, sbit;

            if (!((WORD)bpsize > (WORD)below)) {
                bclr(plane0, at, bit);                  /* out of the plane */
                continue;
            }
            if (!btst(plane0, below, bit)) {            /* 'drawit */
                bclr(plane0, at, bit);
                bset(plane0, below, bit);
                *dst++ = below;
                *dst++ = bit;
                continue;
            }
            /* 'look: left or right first, alternating; bit << 3 bytes
               into the table (BUG(orig): a bit above 7 behind it, #910) */
            look = lookup[flip][0];
            flip = !flip;
            look = (const struct side *)((const UBYTE *)look + (UWORD)(bit << 3));
            side = (UWORD)(at + look[0].offset);        /* 'doleft */
            sbit = look[0].bit;
            if (!btst(plane0, side, sbit) && !btst(plane0 + 40, side, sbit)) {
                bclr(plane0, at, bit);
                bset(plane0, side, sbit);
                *dst++ = side;
                *dst++ = sbit;
                continue;
            }
            side = (UWORD)(at + look[1].offset);        /* 'doright */
            sbit = look[1].bit;
            if (!btst(plane0, side, sbit) && !btst(plane0 + 40, side, sbit)) {
                bclr(plane0, at, bit);
                bset(plane0, side, sbit);
                *dst++ = side;
                *dst++ = sbit;
                continue;
            }
            if (!btst(plane1 + 40, at, bit)) {          /* 'skipitstore */
                *dst++ = at;
                *dst++ = bit;
                continue;
            }
            bset(plane1, at, bit);                      /* 'okpause */
        }
        /* 'addone: a new flake from the random table */
        i = rt->index;
        *dst++ = rt->pairs[(WORD)i];
        *dst++ = rt->pairs[1 + (WORD)i];
        rt->index = (UWORD)((i + 2) & 1023);
        *dst = END;
        DROP(out)->last = (ULONG)dst;
    }
    COLOR00 = 0;
}

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

/* LDoSnowActivate(listnum) (d0): the kill list's pixels; the number of
   entries + 1 */
static ULONG BLITZ_ROUTINE dokilllist_c(ULONG n BL_REG(d0))
{
    struct snowfield *e = entry(n);
    UBYTE *drops = (UBYTE *)e->drop[0], *killin, *killout, *plane1, *plane2;
    const UBYTE *bm = (const UBYTE *)DROP(drops)->bm;
    const UBYTE *look = (const UBYTE *)lookup[0];
    UWORD *drop = (UWORD *)DROP(drops)->last, *src, *dst;  /* BUG(orig): 0 before
                                                              LDoSnow (#908) */
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
        idx = (UWORD)(bit << 3);        /* BUG(orig): bit > 7 (#910) */
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
    *drop = END;                        /* the drop list's last not updated (#909) */
    KILL(killout)->last = (ULONG)dst;
    return count;
}

/* ---------------------------------------------------------- KillPoint */

/* KillPoint(listnum,x,y) (d0-d2): a pixel of plane 2 cleared in all
   three planes and added to the kill list; -1, or 0 if it was clear */
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
   keep d1/a0/a1 (and d0 for init and finit, which change nothing), the C
   code d2-d7/a2-a6. */
BLITZ_THUNK(_libinit, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_libfinit, nothing_c, "d0-d1/a0-a1");
BLITZ_THUNK(_InitSnowDrop, initsnowdrop_c, "d1/a0-a1");
BLITZ_THUNK(_AsmDoSnow, asmdosnow_c, "d1/a0-a1");
BLITZ_THUNK(_DoKillList, dokilllist_c, "d1/a0-a1");
BLITZ_THUNK(_KillPoint, killpoint_c, "d1/a0-a1");
