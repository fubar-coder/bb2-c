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
 * Kept from the original (BUG(orig), #423): only the error check tests for an
 * overflow; a shape of depth 0 loops 65536 times (DBF) (#243); FreeQueue
 * leaves the write position stale (#244).  Not a bug: UnQueue clears all
 * planes of the bitmap, not only those of the shape (QBlit changes them too,
 * and UnQueue is meant to clear the area to colour 0).
 */
#include "blitzlib.h"
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks / C / asm, defined below) */
void doqueue(void), doblit(void), doblitex(void), dounqueue(void), dounqueue2(void);
void doqblitmode(void), doflushqueue(void), codeinit(void), freequeue(void);
void qblitchk(void), unqueuechk(void), unqueuechk2(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#qblitlib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_qblitlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_qblitlib, 0x1380);
BLITZ_SUBS(doqueue, 0, 0);
BLITZ_NAME_AT(_toke, "Queue", "Queue#,Max Items");

#define QBLIT_LIBS BLITZ_LIBS(LIB_shapeslib, 0x1381, LIB_qblitlib, 0x1080,         \
                              LIB_bitmaplib, 0x1202, LIB_chipbaselib, 0x1100,      \
                              LIB_graphicslib, 0x5600)

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
QBLIT_LIBS;
BLITZ_SUBS(doblit, qblitchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
QBLIT_LIBS;
BLITZ_SUBS(doblitex, qblitchk, 0);
BLITZ_NAME("QBlit", "Queue#,Shape#,X,Y[,Excessonoff]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_qblitlib, 0x1380, LIB_chipbaselib, 0x1100, LIB_graphicslib, 0x5600);
BLITZ_SUBS(dounqueue, unqueuechk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_qblitlib, 0x1380, LIB_chipbaselib, 0x1100, LIB_graphicslib, 0x5600,
           LIB_bitmaplib, 0x1281);
BLITZ_SUBS(dounqueue2, unqueuechk2, 0);
BLITZ_NAME("UnQueue", "Queue#[,BitMap#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doqblitmode, 0, 0);
BLITZ_NAME("QBlitMode", "BLTCON0");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_qblitlib, 0x1380);
BLITZ_SUBS(doflushqueue, unqueuechk, 0);
BLITZ_NAME("FlushQueue", "Queue#");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);

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

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))

/* a queue item, 14 bytes (QBlit appends, UnQueue reads) */
struct qitem {
    UWORD mod;                  /* +0  destination modulo */
    UWORD size;                 /* +2  BLTSIZE */
    UWORD planes;               /* +4  bitmap depth - 1 */
    const UBYTE *bm;            /* +6  the bitmap object */
    ULONG off;                  /* +10 byte offset in each plane */
};
static_assert(sizeof(struct qitem) == 14, "qitem size");

/* a queue object */
struct queue {
    struct qitem *pos;          /* +0  write position */
    struct qitem *mem;          /* +4  the items */
    ULONG len;                  /* +8  their length in bytes */
};

/* QBlitMode / init */
static void BLITZ_ROUTINE doqblitmode_c(UWORD m BL_REG(d0))
{
    bcon0 = m;
}
BLITZ_THUNK(doqblitmode, doqblitmode_c, "d0-d1/a0-a1");

static ULONG BLITZ_ROUTINE codeinit_c(ULONG hiword BL_REG(d0))
{
    bcon0 = 0xfca;
    return (hiword & 0xffff0000UL) | 0xfca;
}
BLITZ_THUNK(codeinit, codeinit_c, "d1/a0-a1");

/* FlushQueue - inline MOVE.l 4(a3),(a3) */
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

static void BLITZ_ROUTINE freequeue_c(void)
{
    freeq((struct queue *)blitz_string);
}
BLITZ_THUNK(freequeue, freequeue_c, "a0");

/* Queue n,items - 14 bytes per item */
static void BLITZ_ROUTINE doqueue_c(UWORD items BL_REG(d1))
{
    struct queue *q = (struct queue *)blitz_string;
    struct bl_regs r = { .d1 = MEMF_PUBLIC };

    freeq(q);
    r.d0 = (ULONG)items * 14;
    q->len = r.d0;
    q->pos = q->mem = (struct qitem *)BLITZ_ALIBJSR(TOK_globalloc, r);
}
BLITZ_THUNK(doqueue, doqueue_c, "a0");

struct blitstate {
    ULONG cookie;               /* d0 */
    UWORD shift;                /* d4 */
    UWORD size;                 /* d6 */
    ULONG off;                  /* d3 */
    UBYTE **plane;              /* a0 */
};

/* b_doblit - a0 = queue, a3 = shape, a2 = bitmap, a1 = chip base, d2/d3 */
static void qblit(struct queue *q, const UBYTE *shape, const UBYTE *bm, UBYTE *cb,
                  WORD x, WORD y, struct blitstate *st)
{
    struct qitem *item = q->pos;
    ULONG mods, mods_swapped;
    UWORD n;

    x -= SW(shape, 10);
    y -= SW(shape, 12);
    mods = 0xfffe0000UL | (UWORD)(SW(bm, 0) - SW(shape, 6) - 2);
    item->mod = (UWORD)mods;
    mods_swapped = mods << 16 | mods >> 16;
    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x60) = mods_swapped;
    BL_CUSTOML(cb, 0x64) = mods;
    st->shift = (UWORD)((x & 15) << 12);
    BL_CUSTOMW(cb, 0x42) = st->shift;
    BL_CUSTOMW(cb, 0x40) = st->shift | bcon0;
    BL_CUSTOML(cb, 0x44) = 0xffff0000UL;
    BL_CUSTOML(cb, 0x4c) = SL(shape, 14);
    st->cookie = SL(shape, 18);
    st->off = (ULONG)((LONG)y * (WORD)SW(bm, 0)) + (LONG)(WORD)(x >> 3);
    n = SW(shape, 4) - 1;
    st->size = SW(shape, 8);
    item->size = st->size;
    item->planes = SW(bm, 4) - 1;
    item->bm = bm;
    item->off = st->off;
    q->pos = item + 1;
    st->plane = (UBYTE **)(bm + 8);
    goto doblit;
    do {
        BL_BLITWAIT(cb);
doblit:
        BL_CUSTOML(cb, 0x50) = st->cookie;
        {
            ULONG d = (ULONG)*st->plane++ + st->off;

            BL_CUSTOML(cb, 0x48) = d;
            BL_CUSTOML(cb, 0x54) = d;
        }
        BL_CUSTOMW(cb, 0x58) = st->size;
    } while (n-- != 0);
}

static void qblitex(struct queue *q, const UBYTE *shape, const UBYTE *bm, UBYTE *cb,
                    WORD x, WORD y, UWORD bits)
{
    struct blitstate st;
    UWORD p;

    qblit(q, shape, bm, cb, x, y, &st);
    for (p = SW(shape, 4); p < SW(bm, 4); p++) {
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
static void unqueue(struct queue *q, UBYTE *cb)
{
    struct qitem *end = q->pos, *it = q->mem;

    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x40) = 0x01f00000;
    BL_CUSTOMW(cb, 0x74) = 0;
    q->pos = it;
    if (it >= end)
        return;
    goto start;
    do {
        UWORD size, n;
        UBYTE **plane;
        ULONG off;

        if (it >= end)
            return;
        BL_BLITWAIT(cb);
start:
        BL_CUSTOMW(cb, 0x66) = it->mod;
        size = it->size;
        n = it->planes;
        plane = (UBYTE **)(it->bm + 8);
        off = it->off;
        it++;
        do {
            BL_BLITWAIT(cb);
            BL_CUSTOML(cb, 0x54) = (ULONG)*plane++ + off;
            BL_CUSTOMW(cb, 0x58) = size;
        } while (n-- != 0);
    } while (1);
}

/* b_dounqueue2 - copy the queued areas from the bitmap a2 */
static void unqueue2(struct queue *q, UBYTE *cb, const UBYTE *src)
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
        plane = (UBYTE **)(it->bm + 8);
        off = it->off;
        it++;
        from = (UBYTE **)(src + 8);
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
                   UBYTE *cb BL_REG(a1), const UBYTE *bm BL_REG(a2)

void BLITZ_ROUTINE b_doblit_c(QBLIT_REGS);
void BLITZ_ROUTINE b_doblit_c(QBLIT_REGS)
{
    struct blitstate st;

    qblit(q, blitz_string, bm, cb, x, y, &st);
}

void BLITZ_ROUTINE a_doblit_c(QBLIT_REGS, struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_doblit_c(QBLIT_REGS, struct Library *gfx BL_REG(a6))
{
    struct blitstate st;

    OwnBlitter();
    qblit(q, blitz_string, bm, cb, x, y, &st);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(doblit, b_doblit_c, "   jmp     _a_doblit_c");

void BLITZ_ROUTINE b_doblitex_c(QBLIT_REGS, UWORD bits BL_REG(d4));
void BLITZ_ROUTINE b_doblitex_c(QBLIT_REGS, UWORD bits BL_REG(d4))
{
    qblitex(q, blitz_string, bm, cb, x, y, bits);
}

void BLITZ_ROUTINE a_doblitex_c(QBLIT_REGS, UWORD bits BL_REG(d4),
                                struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_doblitex_c(QBLIT_REGS, UWORD bits BL_REG(d4),
                                struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    qblitex(q, blitz_string, bm, cb, x, y, bits);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(doblitex, b_doblitex_c, "   jmp     _a_doblitex_c");

void BLITZ_ROUTINE b_dounqueue_c(UBYTE *cb BL_REG(a1));
void BLITZ_ROUTINE b_dounqueue_c(UBYTE *cb BL_REG(a1))
{
    unqueue((struct queue *)blitz_string, cb);
}

void BLITZ_ROUTINE a_dounqueue_c(UBYTE *cb BL_REG(a1), struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_dounqueue_c(UBYTE *cb BL_REG(a1), struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    unqueue((struct queue *)blitz_string, cb);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(dounqueue, b_dounqueue_c, "   jmp     _a_dounqueue_c");

void BLITZ_ROUTINE b_dounqueue2_c(UBYTE *cb BL_REG(a1), const UBYTE *bm BL_REG(a2));
void BLITZ_ROUTINE b_dounqueue2_c(UBYTE *cb BL_REG(a1), const UBYTE *bm BL_REG(a2))
{
    unqueue2((struct queue *)blitz_string, cb, bm);
}

void BLITZ_ROUTINE a_dounqueue2_c(UBYTE *cb BL_REG(a1), const UBYTE *bm BL_REG(a2),
                                  struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_dounqueue2_c(UBYTE *cb BL_REG(a1), const UBYTE *bm BL_REG(a2),
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

static int BLITZ_ERRCHK maxchk(const UBYTE *shape, const UBYTE *bm)
{
    if (!SL(shape, 18))
        BLITZ_RUNERR(mess3);
    else if (!SW(bm, 62))
        BLITZ_RUNERR(mess1);
    else if (!SW(shape, 0))
        BLITZ_RUNERR(mess2);
    else
        return 1;
    return 0;
}

static int BLITZ_ERRCHK poschk(const UBYTE *shape, const UBYTE *bm, WORD x, WORD y)
{
    if (SW(shape, 4) > SW(bm, 4)) {
        BLITZ_RUNERR(mess0);
        return 0;
    }
    x -= SW(shape, 10);
    y -= SW(shape, 12);
    if (x < 0 || y < 0
        || (UWORD)(x + SW(shape, 0)) > SW(bm, 56) || (UWORD)(y + SW(shape, 2)) > SW(bm, 58)) {
        BLITZ_RUNERR(mess4);
        return 0;
    }
    return 1;
}

/* qblitchk - queue set up, blitchk2, room for one more item */
static void BLITZ_ERRCHK qblitchk_c(WORD x BL_REG(d2), WORD y BL_REG(d3),
                                    const struct queue *q BL_REG(a0),
                                    const UBYTE *bm BL_REG(a2))
{
    if (!q->mem) {
        BLITZ_RUNERR(mess5);
        return;
    }
    if (!maxchk(blitz_string, bm) || !poschk(blitz_string, bm, x, y))
        return;
    if ((ULONG)(q->pos + 1) > (ULONG)q->mem + q->len)
        BLITZ_RUNERR(mess7);
}

static void BLITZ_ERRCHK unqueuechk_c(void)
{
    if (!((const struct queue *)blitz_string)->mem)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK unqueuechk2_c(const UBYTE *bm BL_REG(a2))
{
    if (!((const struct queue *)blitz_string)->mem)
        BLITZ_RUNERR(mess5);
    else if (!SW(bm, 62))
        BLITZ_RUNERR(mess6);
}

BLITZ_ERRCHK_THUNK(qblitchk, qblitchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(unqueuechk, unqueuechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(unqueuechk2, unqueuechk2_c, "d0-d1/a0-a1");
