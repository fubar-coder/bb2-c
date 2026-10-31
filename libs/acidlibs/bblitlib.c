/*
 * bblitlib - command library 146: buffered blits (the background is saved
 * and can be restored) - Buffer, BBlit, UnBuffer, BBlitMode, ShowStencil,
 * FlushBuffer.
 *
 * C translation of bblibsrc/bblitlib (+ bliterrs; binary checked: same as
 * the source).  An object library (!libfin{_toke,...,10,4}): the Blitz
 * object "Buffer" (16 bytes): +0 write position, +4 last record (0 =
 * empty), +8 memory (MEMF_CHIP, memlib globalloc), +12 its length.
 * BBlit first saves the bitmap area (A -> D copy, every bitmap plane)
 * into a record in the buffer:
 *   +0 previous record, +4.w modulo, +6.w bltsize, +8.w bitmap planes-1,
 *   +10.w bytes saved per plane (shape+24), +12 offset, then per plane
 *   {plane address, saved data}
 * and then blits the shape like blitlib (A = cookie, B = data, C = D =
 * bitmap).  UnBuffer copies the records back (newest first) and empties
 * the buffer; ShowStencil does the same through a stencil (B) without
 * emptying the buffer.  Blitz/Amiga variants as in blitlib.
 *
 * Kept from the original (BUG(orig), #73): only the error check tests for an
 * overflow of the buffer; a shape or bitmap of depth 0 loops 65536 times
 * (DBF) (#74); FreeBuffer leaves the write position and the last record
 * pointer stale (#75).  Not a bug: the last word mask of the blit is
 * cleared, the first one stays $FFFF from the save blit (exactly the masks
 * blitlib sets for the shape blit, same result).
 * DIFF: the nullsub-less entries keep a4/a5 (the original saves them too).
 */
#include "blitzlib.h"
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks / C / asm, defined below) */
void dobuffer(void), doblit(void), doblitex(void), dounbuffer(void);
void dobblitmode(void), doshowstencil(void), doflushbuff(void), codeinit(void);
void freebuffer(void), bblitchk(void), unbufferchk(void), unbufferchk2(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#bblitlib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_bblitlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_bblitlib, 0x1380);
BLITZ_SUBS(dobuffer, 0, 0);
BLITZ_NAME_AT(_toke, "Buffer", "Buffer#,Memorylen");

#define BBLIT_LIBS BLITZ_LIBS(LIB_shapeslib, 0x1381, LIB_bblitlib, 0x1080,        \
                              LIB_bitmaplib, 0x1202, LIB_chipbaselib, 0x1100,      \
                              LIB_graphicslib, 0x5600)

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BBLIT_LIBS;
BLITZ_SUBS(doblit, bblitchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BBLIT_LIBS;
BLITZ_SUBS(doblitex, bblitchk, 0);
BLITZ_NAME("BBlit", "Buffer#,Shape#,X,Y[,Excessonoff]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_bblitlib, 0x1380, LIB_chipbaselib, 0x1200, LIB_graphicslib, 0x5600);
BLITZ_SUBS(dounbuffer, unbufferchk, 0);
BLITZ_NAME("UnBuffer", "Buffer#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dobblitmode, 0, 0);
BLITZ_NAME("BBlitMode", "BLTCON0");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bblitlib, 0x1380, LIB_chipbaselib, 0x1200, LIB_graphicslib, 0x5600,
           LIB_stencillib, 0x1181);
BLITZ_SUBS(doshowstencil, unbufferchk2, 0);
BLITZ_NAME("ShowStencil", "Buffer#,Stencil#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_bblitlib, 0x1380);
BLITZ_SUBS(doflushbuff, unbufferchk, 0);
BLITZ_NAME("FlushBuffer", "Buffer#");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freebuffer, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,10,4} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 10, 4);

/* ******************* CODE BELOW **************** */

static UWORD bcon0 = 0xfca;

/* a shape object of shapeslib (32 bytes; the part read here, every
   field as the word or long the original reads) */
struct shape {
    UWORD width;                /* +0 pixel width, 0 = none */
    UWORD height;               /* +2 */
    UWORD depth;                /* +4 */
    UWORD ebwidth;              /* +6 even byte width */
    UWORD bltsize;              /* +8 */
    UWORD xhandle, yhandle;     /* +10, +12 */
    UBYTE *data;                /* +14 the planes, one after the other */
    UBYTE *cookie;              /* +18 0 = none */
    UWORD planesize;            /* +22 bytes per plane */
    UWORD savesize;             /* +24 per plane + one word per row */
};

/* a bitmap object of bitmaplib (64 bytes; the part read here): +4 is
   flags.b/depth.b, read as one word as in the original */
struct bmap {
    UWORD bpr;                  /* +0 bytes per row */
    UWORD rows;                 /* +2 */
    UWORD depth;                /* +4 flags << 8 | depth */
    UWORD pad6;
    UBYTE *planes[8];           /* +8 */
    UBYTE pad40[16];
    UWORD width;                /* +56 */
    UWORD height;               /* +58 */
    UWORD colours;              /* +60 */
    UWORD isreal;               /* +62 0 = not set up */
};
static_assert(sizeof(struct bmap) == 64);

/* a buffer object */
struct record;
struct buffer {
    struct record *pos;         /* +0 the next record */
    struct record *last;        /* +4 the last record, 0 = empty */
    UBYTE *mem;                 /* +8 */
    ULONG len;                  /* +12 */
};
static_assert(sizeof(struct buffer) == 16);

/* the header of a record in the buffer, followed per bitmap plane by
   {plane address.l, the saved data (save bytes)} */
struct record {
    struct record *prev;        /* +0 the previous record, 0 = none */
    UWORD mod;                  /* +4 modulo */
    UWORD size;                 /* +6 bltsize */
    UWORD planes;               /* +8 bitmap planes - 1 */
    UWORD save;                 /* +10 bytes saved per plane (shape+24) */
    ULONG off;                  /* +12 offset in the bitmap */
};
static_assert(sizeof(struct record) == 16);

/* a stencil object of stencillib (8 bytes) */
struct stencil {
    UWORD bpr;                  /* +0 bytes per row, 0 = none */
    UWORD rows;                 /* +2 */
    ULONG mem;                  /* +4 the memory (CHIP) */
};

/* BBlitMode / init */
static void BLITZ_ROUTINE dobblitmode_c(UWORD m BL_REG(d0))
{
    bcon0 = m;
}
BLITZ_THUNK(dobblitmode, dobblitmode_c, "d0-d1/a0-a1");

/* returns the incoming d0 with 0xfca in the low word */
static ULONG BLITZ_ROUTINE codeinit_c(ULONG incoming BL_REG(d0))
{
    bcon0 = 0xfca;
    return (incoming & 0xffff0000UL) | 0xfca;
}
BLITZ_THUNK(codeinit, codeinit_c, "d1/a0-a1");

/* freebuffer (_free) - a3 = buffer */
static void freebuf(struct buffer *b)
{
    UBYTE *m = b->mem;

    if (m) {
        struct bl_regs r = { .d0 = b->len, .a1 = (ULONG)m };

        b->mem = NULL;
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
}

static void BLITZ_ROUTINE freebuffer_c(void)
{
    freebuf((struct buffer *)blitz_string);
}
BLITZ_THUNK(freebuffer, freebuffer_c, "a0");

/* Buffer n,len - MEMF_CHIP (not cleared) */
static void BLITZ_ROUTINE dobuffer_c(ULONG len BL_REG(d1))
{
    struct buffer *b = (struct buffer *)blitz_string;
    struct bl_regs r = { .d0 = len, .d1 = MEMF_CHIP };
    UBYTE *m;

    freebuf(b);
    b->len = len;
    m = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
    b->pos = (struct record *)m;
    b->last = NULL;
    b->mem = m;
}
BLITZ_THUNK(dobuffer, dobuffer_c, "a0");

/* FlushBuffer n - MOVE.l 8(a3),(a3)+:CLR.l (a3) */
static void BLITZ_ROUTINE doflushbuff_c(void)
{
    struct buffer *b = (struct buffer *)blitz_string;

    b->pos = (struct record *)b->mem;
    b->last = NULL;
}
BLITZ_THUNK(doflushbuff, doflushbuff_c, "d0-d1/a0-a1");

/* the blit state for b_doblitex */
struct blitstate {
    ULONG cookie;               /* d0 */
    UWORD shift;                /* d4 */
    UWORD size;                 /* d6 */
    ULONG off;                  /* d3 */
    UBYTE **plane;              /* a0 */
};

/* b_doblit - a0 = buffer, a3 = shape, a2 = bitmap, a1 = chip base, d2/d3
   = x/y: save the background into a record, then blit */
static void bblit(struct buffer *b, const struct shape *shape, const struct bmap *bm, UBYTE *cb,
                  WORD x, WORD y, struct blitstate *st)
{
    struct record *hdr = b->pos;
    UBYTE *rec;                                 /* behind the header */
    ULONG admod, cbmod;                         /* d5, d4: A:D and C:B modulos */
    UWORD n, save;
    UBYTE **plane;

    x -= shape->xhandle;
    y -= shape->yhandle;
    hdr->prev = b->last;                        /* link the record */
    b->last = hdr;
    admod = 0xfffe0000UL | (UWORD)(bm->bpr - shape->ebwidth - 2);
    hdr->mod = (UWORD)admod;
    cbmod = admod << 16 | admod >> 16;
    st->off = (ULONG)bl_muls_w(y, bm->bpr) + (LONG)(WORD)(x >> 3);
    st->size = shape->bltsize;
    hdr->size = st->size;
    n = bm->depth - 1;
    hdr->planes = n;
    save = shape->savesize;
    hdr->save = save;
    hdr->off = st->off;
    rec = (UBYTE *)(hdr + 1);
    /* the SAVE loop: bitmap -> buffer */
    BL_BLITWAIT(cb);
    BL_CUSTOMW(cb, 0x64) = (UWORD)admod;
    BL_CUSTOMW(cb, 0x66) = 0;
    BL_CUSTOML(cb, 0x40) = 0x09f00000;
    BL_CUSTOML(cb, 0x44) = 0xffffffff;
    plane = (UBYTE **)bm->planes;
    goto saveplane;
    do {
        BL_BLITWAIT(cb);
saveplane:                                      /* saveit2 */
        {
            ULONG a = (ULONG)*plane++ + st->off;

            BL_CUSTOML(cb, 0x50) = a;
            *(ULONG *)rec = a;
            rec += 4;
            BL_CUSTOML(cb, 0x54) = (ULONG)rec;
            BL_CUSTOMW(cb, 0x58) = st->size;
            rec += (WORD)save;
            __asm__("" : "+a"(rec));            /* no IV multiplication */
        }
    } while (n-- != 0);
    b->pos = (struct record *)rec;
    /* the blit */
    st->shift = (UWORD)((x & 15) << 12);
    n = shape->depth - 1;
    st->plane = (UBYTE **)bm->planes;
    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x60) = cbmod;                       /* MOVEM.l d4-d5,$060 */
    BL_CUSTOML(cb, 0x64) = admod;
    BL_CUSTOMW(cb, 0x42) = st->shift;
    BL_CUSTOMW(cb, 0x40) = st->shift | bcon0;
    BL_CUSTOMW(cb, 0x46) = 0;
    BL_CUSTOML(cb, 0x4c) = (ULONG)shape->data;
    st->cookie = (ULONG)shape->cookie;
    goto doblit;
    do {
        BL_BLITWAIT(cb);
doblit:
        BL_CUSTOML(cb, 0x50) = st->cookie;
        {
            ULONG dst = (ULONG)*st->plane++ + st->off;

            BL_CUSTOML(cb, 0x48) = dst;
            BL_CUSTOML(cb, 0x54) = dst;
        }
        BL_CUSTOMW(cb, 0x58) = st->size;
    } while (n-- != 0);
}

/* b_doblitex - excess planes, d4 = bits (one per plane) */
static void bblitex(struct buffer *b, const struct shape *shape, const struct bmap *bm, UBYTE *cb,
                    WORD x, WORD y, UWORD bits)
{
    struct blitstate st;
    UWORD p;

    bblit(b, shape, bm, cb, x, y, &st);
    for (p = shape->depth; p < bm->depth; p++) {
        UWORD con = (bits & 1) ? 0xbfa : 0xb0a;
        ULONG dst;

        bits >>= 1;
        con |= st.shift;
        dst = (ULONG)*st.plane++ + st.off;
        BL_BLITWAIT(cb);
        BL_CUSTOMW(cb, 0x40) = con;
        BL_CUSTOML(cb, 0x50) = st.cookie;
        BL_CUSTOML(cb, 0x48) = dst;
        BL_CUSTOML(cb, 0x54) = dst;
        BL_CUSTOMW(cb, 0x58) = st.size;
    }
}

/* b_dounbuffer - a3 = buffer, a2 = chip base: the records back */
static void unbuffer(struct buffer *b, UBYTE *cb)
{
    struct record *rec = b->last;

    if (!rec)
        return;
    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x40) = 0x09f00000;
    BL_CUSTOML(cb, 0x44) = 0xffffffff;
    BL_CUSTOMW(cb, 0x64) = 0;
    goto nextrec;
    do {
        UBYTE *cur;                             /* a1: in the record */
        UWORD size, n, save;

        BL_BLITWAIT(cb);
nextrec:                                        /* loop2 */
        BL_CUSTOMW(cb, 0x66) = rec->mod;
        size = rec->size;
        n = rec->planes;
        save = rec->save;
        cur = (UBYTE *)(rec + 1);
        goto nextplane;
        do {
            BL_BLITWAIT(cb);
nextplane:                                      /* loop3a */
            BL_CUSTOML(cb, 0x54) = *(ULONG *)cur;
            cur += 4;
            BL_CUSTOML(cb, 0x50) = (ULONG)cur;
            BL_CUSTOMW(cb, 0x58) = size;
            cur += (WORD)save;
        } while (n-- != 0);
        rec = rec->prev;
    } while (rec);
    b->last = NULL;
    b->pos = (struct record *)b->mem;
}

/* b_showstencil - as UnBuffer, through the stencil (a1) as B; the buffer
   stays */
static void showstencil(struct buffer *b, UBYTE *cb, const struct stencil *stencil)
{
    struct record *rec = b->last;

    if (!rec)
        return;
    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x40) = 0x0fe20000;
    BL_CUSTOML(cb, 0x44) = 0xffffffff;
    BL_CUSTOMW(cb, 0x64) = 0;
    goto nextrec;
    do {
        UBYTE *cur;                             /* a1: in the record */
        UWORD size, n, save, mod;
        ULONG st;

        BL_BLITWAIT(cb);
nextrec:                                        /* loop2 */
        mod = rec->mod;
        BL_CUSTOMW(cb, 0x66) = mod;
        BL_CUSTOMW(cb, 0x60) = mod;
        BL_CUSTOMW(cb, 0x62) = mod;
        size = rec->size;
        n = rec->planes;
        save = rec->save;
        st = rec->off + stencil->mem;
        cur = (UBYTE *)(rec + 1);
        goto nextplane;
        do {
            BL_BLITWAIT(cb);
nextplane:                                      /* loop3a */
            BL_CUSTOML(cb, 0x54) = *(ULONG *)cur;
            BL_CUSTOML(cb, 0x48) = *(ULONG *)cur;
            cur += 4;
            BL_CUSTOML(cb, 0x50) = (ULONG)cur;
            BL_CUSTOML(cb, 0x4c) = st;
            BL_CUSTOMW(cb, 0x58) = size;
            cur += (WORD)save;
        } while (n-- != 0);
        rec = rec->prev;
    } while (rec);
}

/* ---------------------------------------------------- the entries */

#define BBLIT_REGS WORD x BL_REG(d2), WORD y BL_REG(d3), struct buffer *b BL_REG(a0), \
                   UBYTE *cb BL_REG(a1), const struct bmap *bm BL_REG(a2)

void BLITZ_ROUTINE b_doblit_c(BBLIT_REGS);
void BLITZ_ROUTINE b_doblit_c(BBLIT_REGS)
{
    struct blitstate st;

    bblit(b, (const struct shape *)blitz_string, bm, cb, x, y, &st);
}

void BLITZ_ROUTINE a_doblit_c(BBLIT_REGS, struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_doblit_c(BBLIT_REGS, struct Library *gfx BL_REG(a6))
{
    struct blitstate st;

    OwnBlitter();
    bblit(b, (const struct shape *)blitz_string, bm, cb, x, y, &st);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(doblit, b_doblit_c, "   jmp     _a_doblit_c");

void BLITZ_ROUTINE b_doblitex_c(BBLIT_REGS, UWORD bits BL_REG(d4));
void BLITZ_ROUTINE b_doblitex_c(BBLIT_REGS, UWORD bits BL_REG(d4))
{
    bblitex(b, (const struct shape *)blitz_string, bm, cb, x, y, bits);
}

void BLITZ_ROUTINE a_doblitex_c(BBLIT_REGS, UWORD bits BL_REG(d4),
                                struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_doblitex_c(BBLIT_REGS, UWORD bits BL_REG(d4),
                                struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    bblitex(b, (const struct shape *)blitz_string, bm, cb, x, y, bits);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(doblitex, b_doblitex_c, "   jmp     _a_doblitex_c");

void BLITZ_ROUTINE b_dounbuffer_c(UBYTE *cb BL_REG(a2));
void BLITZ_ROUTINE b_dounbuffer_c(UBYTE *cb BL_REG(a2))
{
    unbuffer((struct buffer *)blitz_string, cb);
}

void BLITZ_ROUTINE a_dounbuffer_c(UBYTE *cb BL_REG(a2), struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_dounbuffer_c(UBYTE *cb BL_REG(a2), struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    unbuffer((struct buffer *)blitz_string, cb);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(dounbuffer, b_dounbuffer_c, "   jmp     _a_dounbuffer_c");

void BLITZ_ROUTINE b_showstencil_c(const struct stencil *st BL_REG(a1), UBYTE *cb BL_REG(a2));
void BLITZ_ROUTINE b_showstencil_c(const struct stencil *st BL_REG(a1), UBYTE *cb BL_REG(a2))
{
    showstencil((struct buffer *)blitz_string, cb, st);
}

/* Amiga mode: the wait is a single BTST loop */
void BLITZ_ROUTINE a_showstencil_c(const struct stencil *st BL_REG(a1), UBYTE *cb BL_REG(a2),
                                   struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_showstencil_c(const struct stencil *st BL_REG(a1), UBYTE *cb BL_REG(a2),
                                   struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    showstencil((struct buffer *)blitz_string, cb, st);
    while (BL_BLITBUSY(cb))
        ;
    DisownBlitter();
}
BLITZ_AB_ROUTINE(doshowstencil, b_showstencil_c, "   jmp     _a_showstencil_c");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess0, "Shape has too many BitPlanes");
BLITZ_ERRCHK_MESSAGE(mess1, "Unitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninialized Shape");
BLITZ_ERRCHK_MESSAGE(mess3, "Shape does not have a cookie cut");
BLITZ_ERRCHK_MESSAGE(mess4, "Coords outside of BitMap");
BLITZ_ERRCHK_MESSAGE(mess5, "Uninitialized Buffer");
BLITZ_ERRCHK_MESSAGE(mess6, "Buffer Overflow");
BLITZ_ERRCHK_MESSAGE(mess7, "Uninitialized Stencil");

/* bliterrs: maxchk (cookie, bitmap, shape), poschk (depth, position) */
static int BLITZ_ERRCHK maxchk(const struct shape *shape, const struct bmap *bm)
{
    if (!(ULONG)shape->cookie)
        BLITZ_RUNERR(mess3);
    else if (!bm->isreal)
        BLITZ_RUNERR(mess1);
    else if (!shape->width)
        BLITZ_RUNERR(mess2);
    else
        return 1;
    return 0;
}

static int BLITZ_ERRCHK poschk(const struct shape *shape, const struct bmap *bm, WORD x, WORD y)
{
    if (shape->depth > bm->depth) {
        BLITZ_RUNERR(mess0);
        return 0;
    }
    x -= shape->xhandle;
    y -= shape->yhandle;
    if (x < 0 || y < 0
        || (UWORD)(x + shape->width) > bm->width || (UWORD)(y + shape->height) > bm->height) {
        BLITZ_RUNERR(mess4);
        return 0;
    }
    return 1;
}

/* bblitchk - buffer set up, blitchk2 (x/y in d2/d3), room for the
   record: 24(shape) * planes + 4 * planes + 16 */
static void BLITZ_ERRCHK bblitchk_c(WORD x BL_REG(d2), WORD y BL_REG(d3),
                                    const struct buffer *b BL_REG(a0),
                                    const struct bmap *bm BL_REG(a2))
{
    const struct shape *shape = (const struct shape *)blitz_string;
    ULONG need;

    if (!b->mem) {
        BLITZ_RUNERR(mess5);
        return;
    }
    if (!maxchk(shape, bm) || !poschk(shape, bm, x, y))
        return;
    need = bl_mulu_w(shape->savesize, bm->depth);
    need += (UWORD)(bm->depth << 2);
    need += 16 + (ULONG)b->pos;
    if (need > (ULONG)b->mem + b->len)
        BLITZ_RUNERR(mess6);
}

static void BLITZ_ERRCHK unbufferchk_c(void)
{
    if (!((const struct buffer *)blitz_string)->mem)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK unbufferchk2_c(const struct stencil *st BL_REG(a1))
{
    if (!st->bpr)
        BLITZ_RUNERR(mess7);
}

BLITZ_ERRCHK_THUNK(bblitchk, bblitchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(unbufferchk, unbufferchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(unbufferchk2, unbufferchk2_c, "d0-d1/a0-a1");
