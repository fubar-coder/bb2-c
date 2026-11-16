/*
 * qblitlib - command library 148: queued blits (remembered to be cleared
 * or restored later) - Queue, QBlit, UnQueue, QBlitMode, FlushQueue.
 *
 * C translation of bblibsrc/qblitlib (+ bliterrs; binary checked: same as
 * the source).  An object library (!libfin{_toke,...,10,4}): the Blitz
 * object "Queue" (16 bytes): +0 write position, +4 memory (memlib
 * globalloc, MEMF_PUBLIC), +8 its length (14 bytes per item).
 * QBlit blits like blitlib and appends an item {modulo.w, bltsize.w,
 * bitmap planes-1.w, bitmap.l, offset.l}; UnQueue clears these areas in
 * all planes of their bitmaps (D only, minterm 0), UnQueue q,bitmap copies
 * them from that bitmap instead (A -> D); both empty the queue.
 * FlushQueue (inline) only empties it.  Blitz/Amiga variants as in
 * blitlib.
 *
 * By design (#423): only the runtime error checking tests for an
 * overflow.  Fixed (#243): a shape of depth 0 loops 65536 times (DBF);
 * such a shape is an invalid argument, refused in the error check area by
 * InitShape (the loop stays; a shape never created is caught by maxchk);
 * FreeQueue
 * leaves the write position stale (#244).  Not a bug: UnQueue clears all
 * planes of the bitmap, not only those of the shape (QBlit changes them too,
 * and UnQueue is meant to clear the area to colour 0).
 *
 * Speed: the blit core is inline (BL_ALWAYS_INLINE), and the Blitz mode
 * code of QBlit and UnQueue saves no register (BLITZ_ENTRYPOINT, a6 kept
 * by BLITZ_EP_KEEP_A6; the register rule keeps a3-a7).  As the
 * original: QBlit writes the rest of the queue item with (An)+ and reads
 * bcon0 PC-relative (one hunk), UnQueue reads an item with (An)+, and the
 * plane loops of both are the original's loops (asm blocks; GCC wrote
 * the item with offsets, bcon0 absolute and the loop without DBF).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (aliases / thunks / C / asm, defined below) */
void doqueue(void), qblitlib_doblit(void), qblitlib_doblitex(void);
void dounqueue(void), dounqueue2(void), doqblitmode(void), doflushqueue(void);
void qblitlib_codeinit(void), freequeue(void), qblitchk(void);
void unqueuechk(void), unqueuechk2(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#qblitlib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_qblitlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_qblitlib, BL_IA3|BL_PD0);
BLITZ_SUBS(doqueue, 0, 0);
BLITZ_NAME_AT(_toke, "Queue", "Queue#,Max Items");

#define QBLIT_LIBS BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD1, LIB_qblitlib, BL_IA0|BL_PD0, \
                              LIB_bitmaplib, BL_UA2, LIB_chipbaselib, BL_LA1,            \
                              LIB_graphicslib, BL_IFAMIGA|BL_LA6)

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
QBLIT_LIBS;
BLITZ_SUBS(qblitlib_doblit, qblitchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
QBLIT_LIBS;
BLITZ_SUBS(qblitlib_doblitex, qblitchk, 0);
BLITZ_NAME("QBlit", "Queue#,Shape#,X,Y[,Excessonoff]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_qblitlib, BL_IA3|BL_PD0, LIB_chipbaselib, BL_LA1, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(dounqueue, unqueuechk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_qblitlib, BL_IA3|BL_PD0, LIB_chipbaselib, BL_LA1, LIB_graphicslib, BL_IFAMIGA|BL_LA6,
           LIB_bitmaplib, BL_IA2|BL_PD1);
BLITZ_SUBS(dounqueue2, unqueuechk2, 0);
BLITZ_NAME("UnQueue", "Queue#[,BitMap#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doqblitmode, 0, 0);
BLITZ_NAME("QBlitMode", "BLTCON0");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_qblitlib, BL_IA3|BL_PD0);
BLITZ_SUBS(doflushqueue, unqueuechk, 0);
BLITZ_NAME("FlushQueue", "Queue#");

BLITZ_NULLSUB_AT(init, qblitlib_codeinit, 0, 0);

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freequeue, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,10,4} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 10, 4);

/* ******************* CODE BELOW **************** */

static UWORD bcon0 = 0xfca;

/* the shape (shapeslib) and the bitmap (bitmaplib) objects: struct bb_shape,
   struct bb_bitmap (blitzobj.h) */

/* a queue item, 14 bytes (QBlit appends, UnQueue reads) */
struct qitem {
    UWORD mod;                  /* +0  destination modulo */
    UWORD size;                 /* +2  BLTSIZE */
    UWORD planes;               /* +4  bitmap depth - 1 */
    const struct bb_bitmap *bm; /* +6  the bitmap object */
    ULONG off;                  /* +10 byte offset in each plane */
};
static_assert(sizeof(struct qitem) == 14, "qitem size");

/* a queue object */
struct queue {
    struct qitem *pos;          /* +0  write position */
    struct qitem *mem;          /* +4  the items */
    ULONG len;                  /* +8  their length in bytes */
};

/**
 * @brief QBlitMode BLTCON0 - set the BLTCON0 value of the following QBlits.
 *
 * Only the minterm and channel bits are used: QBlit ORs the shift of the
 * x position into it.  The default (init) is $FCA (A, B, C, D: cookie cut).
 *
 * @param m d0.w: BLTCON0 without the shift
 */
static void BLITZ_ROUTINE doqblitmode_c(UWORD m BL_REG(d0))
{
    bcon0 = m;
}
BLITZ_ALIAS(doqblitmode, doqblitmode_c);

/**
 * @brief Library init: BLTCON0 back to $FCA.
 *
 * The original is `MOVE #$fca,d0` falling into doqblitmode, so d0.w
 * is $FCA afterwards (its upper word stays).
 *
 * @param hiword d0: the caller's d0 (only its upper word is kept)
 * @return d0: the upper word of @p hiword, $FCA in the low word
 */
static ULONG BLITZ_ROUTINE codeinit_c(ULONG hiword BL_REG(d0))
{
    bcon0 = 0xfca;
    return (hiword & 0xffff0000UL) | 0xfca;
}
BLITZ_THUNK(qblitlib_codeinit, codeinit_c, "d1/a0-a1");

/**
 * @brief FlushQueue Queue# - empty the queue without touching the bitmaps.
 *
 * Inline code `MOVE.l 4(a3),(a3)`: the write position goes back to the
 * start of the queue memory.  a3 = the queue object.
 *
 * Errors: "Uninitialized Queue" (unqueuechk, debugger only).
 */
BLITZ_INLINE(doflushqueue, "move.l 4(a3),(a3)");

/* freequeue (_free) */
static void freeq(struct queue *q)
{
    if (q->mem) {
        struct bl_regs r = { .d0 = q->len, .a1 = (ULONG)q->mem };

        BLITZ_ALIBJSR(TOK_globfree, r);
        q->mem = NULL;
    }
}

/**
 * @brief Object free routine (_free) of a Queue: free its memory.
 *
 * a3 = the queue object; memlib globfree of the items, the memory pointer
 * is cleared.  BUG(orig) (#244): the write position stays stale.
 */
static void BLITZ_ROUTINE freequeue_c(void)
{
    freeq((struct queue *)blitz_string);
}
BLITZ_THUNK(freequeue, freequeue_c, "a0");

/**
 * @brief Queue Queue#,Max Items - set up a queue for that many QBlits.
 *
 * a3 = the queue object.  The old memory is freed first (freequeue), then
 * items * 14 bytes are allocated (memlib globalloc, MEMF_PUBLIC); the
 * write position is set to their start.  The allocation is not checked.
 *
 * @param items d1.w: the maximum number of items
 */
static void BLITZ_ROUTINE doqueue_c(UWORD items BL_REG(d1))
{
    struct queue *q = (struct queue *)blitz_string;
    struct bl_regs r = { .d1 = MEMF_PUBLIC };

    freeq(q);
    r.d0 = (ULONG)items * 14;
    q->len = r.d0;
    q->pos = q->mem = (struct qitem *)BLITZ_ALIBJSR(TOK_globalloc, r);
}
BLITZ_ALIAS(doqueue, doqueue_c);

struct blitstate {
    ULONG cookie;               /* d0 */
    UWORD shift;                /* d4 */
    UWORD size;                 /* d6 */
    ULONG off;                  /* d3 */
    UBYTE **plane;              /* a0 */
};

/* b_doblit - a0 = queue, a3 = shape, a2 = bitmap, a1 = chip base, d2/d3 */
BL_ALWAYS_INLINE void qblit(struct queue *q, const struct bb_shape *shape,
                            const struct bb_bitmap *bm, UBYTE *cb,
                            WORD x, WORD y, struct blitstate *st)
{
    struct qitem *item = q->pos;
    ULONG mods, mods_swapped, tmp;
    UWORD n, shift;
    const ULONG *plane;

    x -= (UWORD)shape->handlex;
    y -= (UWORD)shape->handley;
    mods = 0xfffe0000UL | (UWORD)(bm->bpr - shape->ebwidth - 2);
    item->mod = (UWORD)mods;
    mods_swapped = mods << 16 | mods >> 16;
    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x60) = mods_swapped;
    BL_CUSTOML(cb, 0x64) = mods;
    shift = (UWORD)(x & 15);
    /* AND #15:ROR #4 (GCC: MOVEQ #12:LSL, 12 cycles more) */
    __asm__("ror.w #4,%0" : "+d"(shift) : : "cc");
    st->shift = shift;
    BL_CUSTOMW(cb, 0x42) = shift;
    /* OR bcon0(pc),d0 (GCC: absolute, 8 cycles more; one hunk) */
    __asm__("or.w _bcon0(pc),%0" : "+d"(shift) : "m"(bcon0) : "cc");
    BL_CUSTOMW(cb, 0x40) = shift;
    BL_CUSTOML(cb, 0x44) = 0xffff0000UL;
    BL_CUSTOML(cb, 0x4c) = (ULONG)shape->data;
    st->cookie = (ULONG)shape->cookie;
    st->off = bl_blitoff(&bm->bpr, x, y);
    n = shape->depth - 1;
    st->size = shape->bltsize;
    /* the rest of the item with (a4)+ as the original: +2 size, +4 planes
       - 1, +6 the bitmap, +10 the offset; then the queue's write position */
    {
        UBYTE *p = (UBYTE *)&item->size;

        __asm__ volatile(
            "   move.w  %[size],(%[p])+\n"
            "   move.w  %[pl],(%[p])+\n"
            "   move.l  %[bm],(%[p])+\n"
            "   move.l  %[off],(%[p])+"
            : [p] "+a"(p)
            : [size] "d"(st->size), [pl] "d"((UWORD)(bm->flagsdepth - 1)),
              [bm] "r"(bm), [off] "d"(st->off)
            : "memory");
        q->pos = (struct qitem *)p;
    }
    plane = (const ULONG *)bm->planes;
    /* 'doblit: per shape plane A = cookie, C = D = the plane + offset (the
       original's loop) */
    __asm__ volatile(
        "   bra.s   1f\n"
        "0: tst.b   2(%[cb])\n"                 /* blitwait */
        "2: btst    #6,2(%[cb])\n"
        "   bne.s   2b\n"
        "1: move.l  %[ck],0x50(%[cb])\n"
        "   move.l  (%[pl])+,%[t]\n"
        "   add.l   %[off],%[t]\n"
        "   move.l  %[t],0x48(%[cb])\n"
        "   move.l  %[t],0x54(%[cb])\n"
        "   move.w  %[size],0x58(%[cb])\n"
        "   dbf     %[n],0b"
        : [pl] "+a"(plane), [n] "+d"(n), [t] "=&d"(tmp)
        : [cb] "a"(cb), [off] "d"(st->off), [size] "d"(st->size), [ck] "d"(st->cookie)
        : "cc", "memory");
    st->plane = (UBYTE **)plane;
}
BL_ALWAYS_INLINE void qblitex(struct queue *q, const struct bb_shape *shape,
                              const struct bb_bitmap *bm, UBYTE *cb,
                              WORD x, WORD y, UWORD bits)
{
    struct blitstate st;
    UWORD p;

    qblit(q, shape, bm, cb, x, y, &st);
    for (p = shape->depth; p < bm->flagsdepth; p++) {
        UWORD con = (bits & 1) ? 0xbfa : 0xb0a;
        ULONG d;

        bits >>= 1;
        con |= st.shift;
        d = (ULONG)*st.plane++ + st.off;
        BL_BLITWAIT(cb);
        BL_CUSTOMW(cb, 0x40) = con;
        BL_CUSTOML(cb, 0x50) = st.cookie;
        BL_CUSTOML(cb, 0x48) = d;
        BL_CUSTOML(cb, 0x54) = d;
        BL_CUSTOMW(cb, 0x58) = st.size;
    }
}

/* b_dounqueue - clear the queued areas (D only, minterm 0) */
BL_ALWAYS_INLINE void unqueue(struct queue *q, UBYTE *cb)
{
    struct qitem *end = q->pos;
    const struct qitem *it = q->mem;

    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x40) = 0x01f00000;
    BL_CUSTOMW(cb, 0x74) = 0;
    q->pos = (struct qitem *)it;
    if (it >= end)
        return;
    goto start;
    do {
        UWORD size, n;
        const ULONG *plane;
        ULONG off, tmp;

        if (it >= end)
            return;
        BL_BLITWAIT(cb);
start:
        /* 'start: the item with (a2)+ as the original: the modulo into
           BLTDMOD, size, planes - 1, the bitmap's Planes, the offset */
        __asm__ volatile(
            "   move.w  (%[it])+,0x66(%[cb])\n"
            "   move.w  (%[it])+,%[size]\n"
            "   move.w  (%[it])+,%[n]\n"
            "   move.l  (%[it])+,%[pl]\n"
            "   move.l  (%[it])+,%[off]\n"
            "   addq.l  #8,%[pl]"
            : [it] "+a"(it), [size] "=d"(size), [n] "=d"(n), [pl] "=a"(plane),
              [off] "=d"(off)
            : [cb] "a"(cb)
            : "memory");
        /* 'loop2: per plane the plane + offset into BLTDPT (the original's
           loop, the wait first) */
        __asm__ volatile(
            "0: tst.b   2(%[cb])\n"             /* blitwait */
            "2: btst    #6,2(%[cb])\n"
            "   bne.s   2b\n"
            "   move.l  (%[pl])+,%[t]\n"
            "   add.l   %[off],%[t]\n"
            "   move.l  %[t],0x54(%[cb])\n"
            "   move.w  %[size],0x58(%[cb])\n"
            "   dbf     %[n],0b"
            : [pl] "+a"(plane), [n] "+d"(n), [t] "=&d"(tmp)
            : [cb] "a"(cb), [off] "d"(off), [size] "d"(size)
            : "cc", "memory");
    } while (1);
}
BL_ALWAYS_INLINE void unqueue2(struct queue *q, UBYTE *cb, const struct bb_bitmap *src)
{
    struct qitem *end = q->pos, *it = q->mem;

    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x40) = 0x09f00000;
    BL_CUSTOML(cb, 0x44) = 0xffffffff;
    q->pos = it;
    if (it >= end)
        return;
    goto start;
    do {
        UWORD size, n;
        UBYTE **plane, **from;
        ULONG off;

        if (it >= end)
            return;
        BL_BLITWAIT(cb);
start:
        BL_CUSTOMW(cb, 0x66) = it->mod;
        BL_CUSTOMW(cb, 0x64) = it->mod;
        size = it->size;
        n = it->planes;
        plane = (UBYTE **)it->bm->planes;
        off = it->off;
        it++;
        from = (UBYTE **)src->planes;
        do {
            BL_BLITWAIT(cb);
            BL_CUSTOML(cb, 0x54) = (ULONG)*plane++ + off;
            BL_CUSTOML(cb, 0x50) = (ULONG)*from++ + off;
            BL_CUSTOMW(cb, 0x58) = size;
        } while (n-- != 0);
    } while (1);
}

/* ---------------------------------------------------- the entries */

#define QBLIT_REGS WORD x BL_REG(d2), WORD y BL_REG(d3), struct queue *q BL_REG(a0), \
                   UBYTE *cb BL_REG(a1), const struct bb_bitmap *bm BL_REG(a2)

/**
 * @brief QBlit Queue#,Shape#,X,Y - blit a shape and remember the area (Blitz mode).
 *
 * The shape (a3) is blitted through its cookie into all its planes of the
 * used bitmap at x/y minus the shape's handle (BLTCON0 = shift | the
 * QBlitMode value); an item {modulo, bltsize, bitmap depth - 1, bitmap,
 * offset} is appended to the queue for UnQueue.  Blitz mode version: the
 * blitter is used directly, no OwnBlitter.
 *
 * Errors (qblitchk, debugger only): "Uninitialized Queue", the blitlib
 * checks (cookie, bitmap, shape, depth, coordinates) and "Queue Overflow".
 * By design (#423): only the runtime error checking tests for an overflow.
 * Fixed (#243): a shape of depth 0 loops 65536 times (DBF), refused at its
 * creation in the error check area.
 *
 * @param x  d2.w: the x position
 * @param y  d3.w: the y position
 * @param q  a0: the queue object
 * @param cb a1: the chip base ($DFF000, chipbaselib)
 * @param bm a2: the used bitmap
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_doblit_c(QBLIT_REGS);
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_doblit_c(QBLIT_REGS)
{
    BLITZ_EP_KEEP_A6();
    struct blitstate st;

    qblit(q, (const struct bb_shape *)blitz_string, bm, cb, x, y, &st);
    BLITZ_EP_KEEP_A6_END();
}
BLITZ_EP_MARK(b_doblit_c);

/**
 * @brief QBlit Queue#,Shape#,X,Y in Amiga mode: b_doblit_c between
 * OwnBlitter and DisownBlitter, waiting for the blitter at the end.
 *
 * @param gfx a6: graphics.library
 */
static void BLITZ_ROUTINE a_doblit_c(QBLIT_REGS,
                                     struct Library *gfx BL_REG(a6));
static void BLITZ_ROUTINE a_doblit_c(QBLIT_REGS,
                                     struct Library *gfx BL_REG(a6))
{
    struct blitstate st;

    OwnBlitter();
    qblit(q, (const struct bb_shape *)blitz_string, bm, cb, x, y, &st);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(qblitlib_doblit, b_doblit_c, "   jmp     _a_doblit_c");

/**
 * @brief QBlit Queue#,Shape#,X,Y,Excessonoff - QBlit and fill the bitmap's
 * planes beyond the shape's depth (Blitz mode).
 *
 * As b_doblit_c, then for every plane from the shape's depth up to the
 * bitmap's depth one bit of @p bits (bit 0 first): set = the cookie area
 * is set (minterm $BFA), clear = it is cleared ($B0A).  The queue item is
 * the one of b_doblit_c (UnQueue clears all planes of the bitmap anyway).
 *
 * Errors: as b_doblit_c (qblitchk).
 *
 * @param bits d4.w: one bit per excess plane
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_doblitex_c(QBLIT_REGS, UWORD bits BL_REG(d4));
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_doblitex_c(QBLIT_REGS, UWORD bits BL_REG(d4))
{
    BLITZ_EP_KEEP_A6();

    qblitex(q, (const struct bb_shape *)blitz_string, bm, cb, x, y, bits);
    BLITZ_EP_KEEP_A6_END();
}
BLITZ_EP_MARK(b_doblitex_c);

/**
 * @brief QBlit Queue#,Shape#,X,Y,Excessonoff in Amiga mode: b_doblitex_c
 * between OwnBlitter and DisownBlitter, waiting for the blitter at the end.
 *
 * @param bits d4.w: one bit per excess plane
 * @param gfx  a6: graphics.library
 */
static void BLITZ_ROUTINE a_doblitex_c(QBLIT_REGS, UWORD bits BL_REG(d4),
                                       struct Library *gfx BL_REG(a6));
static void BLITZ_ROUTINE a_doblitex_c(QBLIT_REGS, UWORD bits BL_REG(d4),
                                       struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    qblitex(q, (const struct bb_shape *)blitz_string, bm, cb, x, y, bits);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(qblitlib_doblitex, b_doblitex_c, "   jmp     _a_doblitex_c");

/**
 * @brief UnQueue Queue# - clear all queued areas and empty the queue (Blitz mode).
 *
 * a3 = the queue object.  Every item's area is cleared in all planes of
 * its bitmap (D only, minterm 0), not only in those of the shape: QBlit
 * with Excessonoff changes them too, and UnQueue is meant to clear the
 * area to colour 0 (not a bug).  The write position goes back to the
 * start.
 *
 * Errors: "Uninitialized Queue" (unqueuechk, debugger only).
 *
 * @param cb a1: the chip base ($DFF000)
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_dounqueue_c(UBYTE *cb BL_REG(a1));
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_dounqueue_c(UBYTE *cb BL_REG(a1))
{
    BLITZ_EP_KEEP_A6();

    unqueue((struct queue *)blitz_string, cb);
    BLITZ_EP_KEEP_A6_END();
}
BLITZ_EP_MARK(b_dounqueue_c);

/**
 * @brief UnQueue Queue# in Amiga mode: b_dounqueue_c between OwnBlitter
 * and DisownBlitter, waiting for the blitter at the end.
 *
 * @param cb  a1: the chip base ($DFF000)
 * @param gfx a6: graphics.library
 */
void BLITZ_ROUTINE a_dounqueue_c(UBYTE *cb BL_REG(a1), struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_dounqueue_c(UBYTE *cb BL_REG(a1), struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    unqueue((struct queue *)blitz_string, cb);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(dounqueue, b_dounqueue_c, "   jmp     _a_dounqueue_c");

/**
 * @brief UnQueue Queue#,BitMap# - restore all queued areas from a bitmap
 * and empty the queue (Blitz mode).
 *
 * As b_dounqueue_c, but every area is copied from the same place of
 * @p bm (A -> D, minterm $9F0), plane by plane, instead of cleared.
 *
 * Errors (unqueuechk2, debugger only): "Uninitialized Queue",
 * "Uninitialized BitMap".
 *
 * @param cb a1: the chip base ($DFF000)
 * @param bm a2: the bitmap to copy from
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_dounqueue2_c(UBYTE *cb BL_REG(a1),
                                                     const struct bb_bitmap *bm BL_REG(a2));
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_dounqueue2_c(UBYTE *cb BL_REG(a1),
                                                     const struct bb_bitmap *bm BL_REG(a2))
{
    BLITZ_EP_KEEP_A6();

    unqueue2((struct queue *)blitz_string, cb, bm);
    BLITZ_EP_KEEP_A6_END();
}
BLITZ_EP_MARK(b_dounqueue2_c);

/**
 * @brief UnQueue Queue#,BitMap# in Amiga mode: b_dounqueue2_c between
 * OwnBlitter and DisownBlitter, waiting for the blitter at the end.
 *
 * @param cb  a1: the chip base ($DFF000)
 * @param bm  a2: the bitmap to copy from
 * @param gfx a6: graphics.library
 */
void BLITZ_ROUTINE a_dounqueue2_c(UBYTE *cb BL_REG(a1), const struct bb_bitmap *bm BL_REG(a2),
                                  struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_dounqueue2_c(UBYTE *cb BL_REG(a1), const struct bb_bitmap *bm BL_REG(a2),
                                  struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    unqueue2((struct queue *)blitz_string, cb, bm);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(dounqueue2, b_dounqueue2_c, "   jmp     _a_dounqueue2_c");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess0, "Shape has too many BitPlanes");
BLITZ_ERRCHK_MESSAGE(mess1, "Unitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninialized Shape");
BLITZ_ERRCHK_MESSAGE(mess3, "Shape does not have a cookie cut");
BLITZ_ERRCHK_MESSAGE(mess4, "Coords outside of BitMap");
BLITZ_ERRCHK_MESSAGE(mess5, "Uninitialized Queue");
BLITZ_ERRCHK_MESSAGE(mess6, "Uninitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess7, "Queue Overflow");

static int BLITZ_ERRCHK maxchk(const struct bb_shape *shape, const struct bb_bitmap *bm)
{
    if (!shape->cookie)
        BLITZ_RUNERR(mess3);
    else if (!bm->isreal)
        BLITZ_RUNERR(mess1);
    else if (!shape->width)
        BLITZ_RUNERR(mess2);
    else
        return 1;
    return 0;
}

static int BLITZ_ERRCHK poschk(const struct bb_shape *shape, const struct bb_bitmap *bm,
                               WORD x, WORD y)
{
    if (shape->depth > bm->flagsdepth) {
        BLITZ_RUNERR(mess0);
        return 0;
    }
    x -= (UWORD)shape->handlex;
    y -= (UWORD)shape->handley;
    if (x < 0 || y < 0
        || (UWORD)(x + shape->width) > bm->width || (UWORD)(y + shape->height) > bm->height) {
        BLITZ_RUNERR(mess4);
        return 0;
    }
    return 1;
}

/* qblitchk - queue set up, blitchk2, room for one more item */
static void BLITZ_ERRCHK qblitchk_c(WORD x BL_REG(d2), WORD y BL_REG(d3),
                                    const struct queue *q BL_REG(a0),
                                    const struct bb_bitmap *bm BL_REG(a2))
{
    if (!q->mem) {
        BLITZ_RUNERR(mess5);
        return;
    }
    if (!maxchk((const struct bb_shape *)blitz_string, bm)
        || !poschk((const struct bb_shape *)blitz_string, bm, x, y))
        return;
    if ((ULONG)(q->pos + 1) > (ULONG)q->mem + q->len)
        BLITZ_RUNERR(mess7);
}

static void BLITZ_ERRCHK unqueuechk_c(void)
{
    if (!((const struct queue *)blitz_string)->mem)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK unqueuechk2_c(const struct bb_bitmap *bm BL_REG(a2))
{
    if (!((const struct queue *)blitz_string)->mem)
        BLITZ_RUNERR(mess5);
    else if (!bm->isreal)
        BLITZ_RUNERR(mess6);
}

BLITZ_ERRCHK_THUNK(qblitchk, qblitchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(unqueuechk, unqueuechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(unqueuechk2, unqueuechk2_c, "d0-d1/a0-a1");
