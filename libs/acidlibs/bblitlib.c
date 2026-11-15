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
 * By design (#73): only the runtime error checking tests for an
 * overflow of the buffer.  Fixed (#74): a shape or bitmap of depth 0 loops
 * 65536 times (DBF); such an object is an invalid argument, refused in the
 * error check area by BitMap, CludgeBitMap and InitShape (the loops stay);
 * FreeBuffer leaves the write position and the last record
 * pointer stale (#75).  Not a bug: the last word mask of the blit is
 * cleared, the first one stays $FFFF from the save blit (exactly the masks
 * blitlib sets for the shape blit, same result).
 * DIFF: the nullsub-less entries keep a4/a5 (the original saves them too).
 * Fixed (#1045): BBlit, UnBuffer and ShowStencil stepped from plane to
 * plane in the record by the plane size as a signed word (ADD.w of
 * shape +24), backwards for a plane of 32 KB or more; the step is now
 * the unsigned word (zero-extended once, outside the plane loops).
 *
 * Speed: the blit core is inline (BL_ALWAYS_INLINE), and the Blitz mode
 * code of BBlit, UnBuffer and ShowStencil saves no register
 * (BLITZ_ENTRYPOINT, a6 kept by BLITZ_EP_KEEP_A6; the register rule keeps
 * a3-a7).  The plane loops of BBlit and UnBuffer are the original's
 * assembler loops (asm blocks: in C, GCC kept the plane pointer, the
 * record pointer or the plane step on the stack or copied them between
 * data and address registers, 1.6 % slower than the original); the C
 * around them computes what it can before each blitter wait.
 */
#include "blitzlib.h"
#include "blitzobj.h"
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (aliases / thunks / C / asm, defined below) */
void dobuffer(void), bblitlib_doblit(void), bblitlib_doblitex(void);
void dounbuffer(void), dobblitmode(void), doshowstencil(void);
void doflushbuff(void), bblitlib_codeinit(void), freebuffer(void);
void bblitchk(void), unbufferchk(void), unbufferchk2(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#bblitlib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_bblitlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_bblitlib, BL_IA3|BL_PD0);
BLITZ_SUBS(dobuffer, 0, 0);
BLITZ_NAME_AT(_toke, "Buffer", "Buffer#,Memorylen");

#define BBLIT_LIBS BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD1, LIB_bblitlib, BL_IA0|BL_PD0, \
                              LIB_bitmaplib, BL_UA2, LIB_chipbaselib, BL_LA1,            \
                              LIB_graphicslib, BL_IFAMIGA|BL_LA6)

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BBLIT_LIBS;
BLITZ_SUBS(bblitlib_doblit, bblitchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BBLIT_LIBS;
BLITZ_SUBS(bblitlib_doblitex, bblitchk, 0);
BLITZ_NAME("BBlit", "Buffer#,Shape#,X,Y[,Excessonoff]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_bblitlib, BL_IA3|BL_PD0, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(dounbuffer, unbufferchk, 0);
BLITZ_NAME("UnBuffer", "Buffer#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dobblitmode, 0, 0);
BLITZ_NAME("BBlitMode", "BLTCON0");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bblitlib, BL_IA3|BL_PD0, LIB_chipbaselib, BL_LA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6,
           LIB_stencillib, BL_IA1|BL_PD1);
BLITZ_SUBS(doshowstencil, unbufferchk2, 0);
BLITZ_NAME("ShowStencil", "Buffer#,Stencil#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_bblitlib, BL_IA3|BL_PD0);
BLITZ_SUBS(doflushbuff, unbufferchk, 0);
BLITZ_NAME("FlushBuffer", "Buffer#");

BLITZ_NULLSUB_AT(init, bblitlib_codeinit, 0, 0);

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

/* the objects: struct bb_shape, struct bb_bitmap (blitzobj.h) */

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

/**
 * @brief BBlitMode BLTCON0: the minterm/channel bits of BBlit's BLTCON0
 *        (ORed with the shift; blitlib's CookieMode ... give the values).
 *
 * @param m  d0.w: the BLTCON0 value without the shift
 */
static void BLITZ_ROUTINE dobblitmode_c(UWORD m BL_REG(d0))
{
    bcon0 = m;
}
BLITZ_ALIAS(dobblitmode, dobblitmode_c);

/**
 * @brief init: BBlitMode $FCA (cookie cut).
 *
 * @param incoming  d0: what the caller had in d0
 * @return d0: incoming with $FCA in the low word
 */
static ULONG BLITZ_ROUTINE codeinit_c(ULONG incoming BL_REG(d0))
{
    bcon0 = 0xfca;
    return (incoming & 0xffff0000UL) | 0xfca;
}
BLITZ_THUNK(bblitlib_codeinit, codeinit_c, "d1/a0-a1");

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

/**
 * @brief Free Buffer (the object's _free nullsub): free the memory of the
 *        buffer in a3.
 *
 * BUG(orig) (#75): the write position and the last record pointer are
 * left stale.
 */
static void BLITZ_ROUTINE freebuffer_c(void)
{
    freebuf((struct buffer *)blitz_string);
}
BLITZ_THUNK(freebuffer, freebuffer_c, "a0");

/**
 * @brief Buffer Buffer#,Memorylen: (re)allocate the buffer.
 *
 * Frees the old memory, allocates Memorylen bytes MEMF_CHIP (not cleared,
 * memlib globalloc); the buffer is empty.  The buffer comes in a3 (!libs
 * {#bblitlib,$1380}, from Buffer# in d0).
 *
 * @param len  d1: the length in bytes
 */
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
BLITZ_ALIAS(dobuffer, dobuffer_c);

/**
 * @brief FlushBuffer Buffer#: empty the buffer without restoring the
 *        background (MOVE.l 8(a3),(a3)+:CLR.l (a3)).
 *
 * Errors (debugger, unbufferchk): "Uninitialized Buffer".  The buffer
 * comes in a3 (!libs {#bblitlib,$1380}, from Buffer# in d0).
 */
static void BLITZ_ROUTINE doflushbuff_c(void)
{
    struct buffer *b = (struct buffer *)blitz_string;

    b->pos = (struct record *)b->mem;
    b->last = NULL;
}
BLITZ_ALIAS(doflushbuff, doflushbuff_c);

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
BL_ALWAYS_INLINE void bblit(struct buffer *b, const struct bb_shape *shape,
                            const struct bb_bitmap *bm, UBYTE *cb,
                            WORD x, WORD y, struct blitstate *st)
{
    struct record *hdr;
    struct buffer *volatile bsave;              /* the original's a4 */
    UBYTE *rec;                                 /* a5: behind the header */
    const ULONG *plane;                         /* a0: the bitmap's planes */
    ULONG off, save, planes, tmp;               /* d3, d1, LEA 8(a2) */
    ULONG admod, cbmod;                         /* d5, d4 */
    UWORD mod, size, n, shift, con0;            /* d5, d6, d7, d0 */

    /* Speed: everything from the shape and the bitmap first, so that the
       bitmap pointer is dead before the record is written; the bitmap's
       Planes as a ULONG in a data register (the original keeps the bitmap
       in a2, which here is the record pointer) and the buffer on the
       stack (stored once, read once): no other value goes to the stack */
    x -= shape->handlex;
    y -= shape->handley;
    shift = (UWORD)(x & 15);
    __asm__("ror.w #4,%0" : "+d"(shift) : : "cc");     /* AND #15:ROR #4 */
    mod = bm->bpr - shape->ebwidth - 2;
    off = bl_blitoff(&bm->bpr, x, y);
    n = bm->flagsdepth - 1;
    planes = (ULONG)bm->planes;
    __asm__("" : "+d"(planes));
    size = shape->bltsize;
    save = shape->xplanesize;                   /* Fixed (#1045): unsigned */
    hdr = b->pos;
    hdr->prev = b->last;                        /* link the record */
    b->last = hdr;
    bsave = b;
    hdr->mod = mod;
    hdr->size = size;
    hdr->planes = n;
    hdr->save = (UWORD)save;
    hdr->off = off;
    rec = (UBYTE *)(hdr + 1);
    plane = (const ULONG *)planes;
    /* the SAVE loop: bitmap -> buffer */
    BL_BLITWAIT(cb);
    BL_CUSTOMW(cb, 0x64) = mod;
    BL_CUSTOMW(cb, 0x66) = 0;
    BL_CUSTOML(cb, 0x40) = 0x09f00000;
    BL_CUSTOML(cb, 0x44) = 0xffffffff;
    /* 'saveit: per plane the plane address + offset into BLTAPT and the
       record, the record's data into BLTDPT (the original's loop) */
    __asm__ volatile(
        "   bra.s   1f\n"
        "0: tst.b   2(%[cb])\n"                 /* blitwait */
        "2: btst    #6,2(%[cb])\n"
        "   bne.s   2b\n"
        "1: move.l  (%[pl])+,%[t]\n"
        "   add.l   %[off],%[t]\n"
        "   move.l  %[t],0x50(%[cb])\n"
        "   move.l  %[t],(%[rec])+\n"
        "   move.l  %[rec],0x54(%[cb])\n"
        "   move.w  %[size],0x58(%[cb])\n"
        "   add.l   %[save],%[rec]\n"
        "   dbf     %[n],0b"
        : [pl] "+a"(plane), [rec] "+a"(rec), [n] "+d"(n), [t] "=&d"(tmp)
        : [cb] "a"(cb), [off] "d"(off), [size] "d"(size), [save] "d"(save)
        : "cc", "memory");
    bsave->pos = (struct record *)rec;
    /* the blit: its values before the wait, after it only the writes */
    admod = 0xfffe0000UL | mod;
    cbmod = admod << 16 | admod >> 16;
    con0 = shift | bcon0;
    st->shift = shift;
    st->cookie = (ULONG)shape->cookie;
    n = shape->depth - 1;
    plane = (const ULONG *)planes;
    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x60) = cbmod;                       /* MOVEM.l d4-d5,$060 */
    BL_CUSTOML(cb, 0x64) = admod;
    BL_CUSTOMW(cb, 0x42) = shift;
    BL_CUSTOMW(cb, 0x40) = con0;
    BL_CUSTOMW(cb, 0x46) = 0;
    BL_CUSTOML(cb, 0x4c) = (ULONG)shape->data;
    /* 'doblit: per shape plane A = cookie, C = D = the plane + offset */
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
        : [cb] "a"(cb), [off] "d"(off), [size] "d"(size), [ck] "d"(st->cookie)
        : "cc", "memory");
    st->size = size;
    st->off = off;
    st->plane = (UBYTE **)plane;
}

/* b_doblitex - excess planes, d4 = bits (one per plane) */
BL_ALWAYS_INLINE void bblitex(struct buffer *b, const struct bb_shape *shape,
                              const struct bb_bitmap *bm, UBYTE *cb,
                              WORD x, WORD y, UWORD bits)
{
    struct blitstate st;
    UWORD p;

    bblit(b, shape, bm, cb, x, y, &st);
    for (p = shape->depth; p < bm->flagsdepth; p++) {
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
BL_ALWAYS_INLINE void unbuffer(struct buffer *b, UBYTE *cb)
{
    const struct record *rec = b->last;

    if (!rec)
        return;
    BL_BLITWAIT(cb);
    BL_CUSTOML(cb, 0x40) = 0x09f00000;
    BL_CUSTOML(cb, 0x44) = 0xffffffff;
    BL_CUSTOMW(cb, 0x64) = 0;
    do {                                        /* loop2 */
        const UBYTE *cur = (const UBYTE *)(rec + 1);    /* a1: in the record */
        UWORD mod = rec->mod, size = rec->size, n = rec->planes;
        ULONG save = rec->save;                 /* Fixed (#1045): unsigned */

        /* the record is read before the wait (for the first record the
           blitter is idle already) */
        BL_BLITWAIT(cb);
        BL_CUSTOMW(cb, 0x66) = mod;
        /* loop3a: per plane the saved data (BLTAPT) back to the plane
           (BLTDPT), the original's loop */
        __asm__ volatile(
            "   bra.s   1f\n"
            "0: tst.b   2(%[cb])\n"             /* blitwait */
            "2: btst    #6,2(%[cb])\n"
            "   bne.s   2b\n"
            "1: move.l  (%[cur])+,0x54(%[cb])\n"
            "   move.l  %[cur],0x50(%[cb])\n"
            "   move.w  %[size],0x58(%[cb])\n"
            "   add.l   %[save],%[cur]\n"
            "   dbf     %[n],0b"
            : [cur] "+a"(cur), [n] "+d"(n)
            : [cb] "a"(cb), [size] "d"(size), [save] "d"(save)
            : "cc", "memory");
        rec = rec->prev;
    } while (rec);
    b->last = NULL;
    b->pos = (struct record *)b->mem;
}

/* b_showstencil - as UnBuffer, through the stencil (a1) as B; the buffer
   stays */
BL_ALWAYS_INLINE void showstencil(struct buffer *b, UBYTE *cb, const struct stencil *stencil)
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
            cur += save;                        /* Fixed (#1045): unsigned */
        } while (n-- != 0);
        rec = rec->prev;
    } while (rec);
}

/* ---------------------------------------------------- the entries */

#define BBLIT_REGS WORD x BL_REG(d2), WORD y BL_REG(d3), struct buffer *b BL_REG(a0), \
                   UBYTE *cb BL_REG(a1), const struct bb_bitmap *bm BL_REG(a2)

/**
 * @brief BBlit Buffer#,Shape#,X,Y: blit the shape into the used bitmap,
 *        the background saved in the buffer (Blitz mode).
 *
 * First the bitmap area under the shape is saved (A -> D copy, every
 * bitmap plane) into a record at the buffer's write position, then the
 * shape is blitted like blitlib's Blit (A = cookie, B = data, C = D =
 * bitmap, BLTCON0 = shift | BBlitMode).  Not a bug: the last word mask of
 * the blit is cleared, the first stays $FFFF from the save blit.
 * BUG(orig): only the error check tests for an overflow of the buffer
 * (#73).  Fixed (#74): a bitmap or shape of depth 0 loops 65536 times
 * (DBF), refused at its creation in the error check area.
 * Errors (debugger, bblitchk): "Uninitialized Buffer", "Shape does not
 * have a cookie cut", "Unitialized BitMap", "Uninialized Shape", "Shape
 * has too many BitPlanes", "Coords outside of BitMap", "Buffer Overflow".
 * The shape comes in a3 (!libs {#shapeslib,$1381}, from Shape# in d1).
 *
 * @param x    d2.w: x (the shape's handle is subtracted)
 * @param y    d3.w: y
 * @param b    a0: the buffer (!libs {#bblitlib,$1080}, from Buffer# in d0)
 * @param cb   a1: the custom chip base $DFF000 (chipbaselib)
 * @param bm   a2: the used bitmap (!libs {#bitmaplib,$1202})
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_doblit_c(BBLIT_REGS);
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_doblit_c(BBLIT_REGS)
{
    BLITZ_EP_KEEP_A6();
    struct blitstate st;

    bblit(b, (const struct bb_shape *)blitz_string, bm, cb, x, y, &st);
    BLITZ_EP_KEEP_A6_END();
}
BLITZ_EP_MARK(b_doblit_c);

/**
 * @brief BBlit Buffer#,Shape#,X,Y (Amiga mode).
 *
 * Amiga mode: OwnBlitter, the blit, wait for the blitter, DisownBlitter.
 * First the bitmap area under the shape is saved (A -> D copy, every
 * bitmap plane) into a record at the buffer's write position, then the
 * shape is blitted like blitlib's Blit (A = cookie, B = data, C = D =
 * bitmap, BLTCON0 = shift | BBlitMode).  Not a bug: the last word mask of
 * the blit is cleared, the first stays $FFFF from the save blit.
 * BUG(orig): only the error check tests for an overflow of the buffer
 * (#73).  Fixed (#74): a bitmap or shape of depth 0 loops 65536 times
 * (DBF), refused at its creation in the error check area.
 * Errors (debugger, bblitchk): "Uninitialized Buffer", "Shape does not
 * have a cookie cut", "Unitialized BitMap", "Uninialized Shape", "Shape
 * has too many BitPlanes", "Coords outside of BitMap", "Buffer Overflow".
 * The shape comes in a3 (!libs {#shapeslib,$1381}, from Shape# in d1).
 *
 * @param x    d2.w: x (the shape's handle is subtracted)
 * @param y    d3.w: y
 * @param b    a0: the buffer (!libs {#bblitlib,$1080}, from Buffer# in d0)
 * @param cb   a1: the custom chip base $DFF000 (chipbaselib)
 * @param bm   a2: the used bitmap (!libs {#bitmaplib,$1202})
 * @param gfx  a6: graphics.library (!libs {#graphicslib,$5600})
 */
static void BLITZ_ROUTINE a_doblit_c(BBLIT_REGS,
                                     struct Library *gfx BL_REG(a6));
static void BLITZ_ROUTINE a_doblit_c(BBLIT_REGS,
                                     struct Library *gfx BL_REG(a6))
{
    struct blitstate st;

    OwnBlitter();
    bblit(b, (const struct bb_shape *)blitz_string, bm, cb, x, y, &st);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(bblitlib_doblit, b_doblit_c, "   jmp     _a_doblit_c");

/**
 * @brief BBlit Buffer#,Shape#,X,Y,Excessonoff (Blitz mode).
 *
 * BBlit, then the bitmap planes beyond the shape's depth get A = cookie,
 * minterm $0A (cleared) or $FA (set), one bit of Excessonoff per plane.
 * Errors (debugger, bblitchk): as BBlit.
 * The shape comes in a3 (!libs {#shapeslib,$1381}, from Shape# in d1).
 *
 * @param x    d2.w: x (the shape's handle is subtracted)
 * @param y    d3.w: y
 * @param b    a0: the buffer (!libs {#bblitlib,$1080}, from Buffer# in d0)
 * @param cb   a1: the custom chip base $DFF000 (chipbaselib)
 * @param bm   a2: the used bitmap (!libs {#bitmaplib,$1202})
 * @param bits d4.w: Excessonoff, one bit per excess plane (set = $FA)
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_doblitex_c(BBLIT_REGS, UWORD bits BL_REG(d4));
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_doblitex_c(BBLIT_REGS, UWORD bits BL_REG(d4))
{
    BLITZ_EP_KEEP_A6();

    bblitex(b, (const struct bb_shape *)blitz_string, bm, cb, x, y, bits);
    BLITZ_EP_KEEP_A6_END();
}
BLITZ_EP_MARK(b_doblitex_c);

/**
 * @brief BBlit Buffer#,Shape#,X,Y,Excessonoff (Amiga mode).
 *
 * Amiga mode: OwnBlitter, the blit, wait for the blitter, DisownBlitter.
 * BBlit, then the bitmap planes beyond the shape's depth get A = cookie,
 * minterm $0A (cleared) or $FA (set), one bit of Excessonoff per plane.
 * Errors (debugger, bblitchk): as BBlit.
 * The shape comes in a3 (!libs {#shapeslib,$1381}, from Shape# in d1).
 *
 * @param x    d2.w: x (the shape's handle is subtracted)
 * @param y    d3.w: y
 * @param b    a0: the buffer (!libs {#bblitlib,$1080}, from Buffer# in d0)
 * @param cb   a1: the custom chip base $DFF000 (chipbaselib)
 * @param bm   a2: the used bitmap (!libs {#bitmaplib,$1202})
 * @param bits d4.w: Excessonoff, one bit per excess plane (set = $FA)
 * @param gfx  a6: graphics.library (!libs {#graphicslib,$5600})
 */
static void BLITZ_ROUTINE a_doblitex_c(BBLIT_REGS, UWORD bits BL_REG(d4),
                                       struct Library *gfx BL_REG(a6));
static void BLITZ_ROUTINE a_doblitex_c(BBLIT_REGS, UWORD bits BL_REG(d4),
                                       struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    bblitex(b, (const struct bb_shape *)blitz_string, bm, cb, x, y, bits);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(bblitlib_doblitex, b_doblitex_c, "   jmp     _a_doblitex_c");

/**
 * @brief UnBuffer Buffer#: restore the background of all BBlits of the
 *        buffer (Blitz mode).
 *
 * The records are copied back (newest first, A -> D), then the buffer is
 * empty.  Errors (debugger, unbufferchk): "Uninitialized Buffer".
 * The buffer comes in a3 (!libs {#bblitlib,$1380}, from Buffer# in d0).
 *
 * @param cb  a2: the custom chip base $DFF000 (chipbaselib)
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_dounbuffer_c(UBYTE *cb BL_REG(a2));
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_dounbuffer_c(UBYTE *cb BL_REG(a2))
{
    BLITZ_EP_KEEP_A6();

    unbuffer((struct buffer *)blitz_string, cb);
    BLITZ_EP_KEEP_A6_END();
}
BLITZ_EP_MARK(b_dounbuffer_c);

/**
 * @brief UnBuffer Buffer# (Amiga mode).
 *
 * Amiga mode: OwnBlitter, the blit, wait for the blitter, DisownBlitter.
 * The records are copied back (newest first, A -> D), then the buffer is
 * empty.  Errors (debugger, unbufferchk): "Uninitialized Buffer".
 * The buffer comes in a3 (!libs {#bblitlib,$1380}, from Buffer# in d0).
 *
 * @param cb   a2: the custom chip base $DFF000 (chipbaselib)
 * @param gfx  a6: graphics.library (!libs {#graphicslib,$5600})
 */
void BLITZ_ROUTINE a_dounbuffer_c(UBYTE *cb BL_REG(a2), struct Library *gfx BL_REG(a6));
void BLITZ_ROUTINE a_dounbuffer_c(UBYTE *cb BL_REG(a2), struct Library *gfx BL_REG(a6))
{
    OwnBlitter();
    unbuffer((struct buffer *)blitz_string, cb);
    BL_BLITWAIT(cb);
    DisownBlitter();
}
BLITZ_AB_ROUTINE(dounbuffer, b_dounbuffer_c, "   jmp     _a_dounbuffer_c");

/**
 * @brief ShowStencil Buffer#,Stencil#: restore the background of the
 *        buffer's BBlits through a stencil (Blitz mode).
 *
 * As UnBuffer, through the stencil as B (minterm $E2); the buffer stays.
 * Errors (debugger, unbufferchk2): "Uninitialized Stencil" (the buffer is
 * not checked).
 * The buffer comes in a3 (!libs {#bblitlib,$1380}, from Buffer# in d0).
 *
 * @param st  a1: the stencil (!libs {#stencillib,$1181}, from Stencil#
 *            in d1)
 * @param cb  a2: the custom chip base $DFF000 (chipbaselib)
 */
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_showstencil_c(const struct stencil *st BL_REG(a1),
                                                           UBYTE *cb BL_REG(a2));
static BLITZ_ENTRYPOINT void BLITZ_ROUTINE b_showstencil_c(const struct stencil *st BL_REG(a1),
                                                           UBYTE *cb BL_REG(a2))
{
    BLITZ_EP_KEEP_A6();

    showstencil((struct buffer *)blitz_string, cb, st);
    BLITZ_EP_KEEP_A6_END();
}
BLITZ_EP_MARK(b_showstencil_c);

/**
 * @brief ShowStencil Buffer#,Stencil# (Amiga mode; the wait is a single
 *        BTST loop).
 *
 * Amiga mode: OwnBlitter, the blit, wait for the blitter, DisownBlitter.
 * As UnBuffer, through the stencil as B (minterm $E2); the buffer stays.
 * Errors (debugger, unbufferchk2): "Uninitialized Stencil" (the buffer is
 * not checked).
 * The buffer comes in a3 (!libs {#bblitlib,$1380}, from Buffer# in d0).
 *
 * @param st   a1: the stencil (!libs {#stencillib,$1181}, from Stencil#
 *             in d1)
 * @param cb   a2: the custom chip base $DFF000 (chipbaselib)
 * @param gfx  a6: graphics.library (!libs {#graphicslib,$5600})
 */
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
static int BLITZ_ERRCHK maxchk(const struct bb_shape *shape, const struct bb_bitmap *bm)
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

static int BLITZ_ERRCHK poschk(const struct bb_shape *shape, const struct bb_bitmap *bm, WORD x, WORD y)
{
    if (shape->depth > bm->flagsdepth) {
        BLITZ_RUNERR(mess0);
        return 0;
    }
    x -= shape->handlex;
    y -= shape->handley;
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
                                    const struct bb_bitmap *bm BL_REG(a2))
{
    const struct bb_shape *shape = (const struct bb_shape *)blitz_string;
    ULONG need;

    if (!b->mem) {
        BLITZ_RUNERR(mess5);
        return;
    }
    if (!maxchk(shape, bm) || !poschk(shape, bm, x, y))
        return;
    need = bl_mulu_w(shape->xplanesize, bm->flagsdepth);
    need += (UWORD)(bm->flagsdepth << 2);
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
