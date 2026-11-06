/*
 * bitmaplib - command library 160: bitmaps - BitMap, LoadBitMap,
 * ScreensBitMap, CopyBitMap, ShapesBitMap, CludgeBitMap.
 *
 * C translation of bblibsrc/bitmaplib (binary checked: same as the
 * source).  An object library (!libfin{_toke,...,10,6}): the Blitz object
 * "BitMap" (the name of the first token), default maximum 10, objects of
 * 2^6 = 64 bytes - a struct BitMap with extra fields:
 *   +0.w BytesPerRow  +2.w Rows  +4.b Flags (cleared) +5.b Depth
 *   +6.w copy of +4 before clearing the flags  +8 Planes[8]
 *   +52.b bit 7 = interleaved  +54.w plane distance (0 = planes not
 *   equally spaced)  +56.w width in pixels (per plane line)  +58.w height
 *   +60.w number of colours (1 << depth)
 *   +62.w 0 = no bitmap, 1 = borrowed (screen, shape, memory),
 *         -1 = own memory (freed with the object)
 * Used bitmap: !libs{#bitmaplib,$0604} = d6 = offset of the used bitmap.
 * extbitmap (nullsub, for other libraries): a3 = object, d1/d2/d3 = width,
 * height, depth - allocate and set up without freeing.
 *
 * Kept from the original (BUG(orig)):
 *   - Sizes are MULU word products: h * depth * bytes per row is
 *     truncated (the second MULU uses the low word of h * depth). (#76)
 *   - CopyBitMap copies plane by plane with (bytes per row * rows) / 2
 *     words (0 words = 4 billion) (#78), into one MEMF_CHIP block (not
 *     cleared) and does not set the used bitmap (not a bug: every word is
 *     overwritten by the copy, and CopyBitMap is not documented to select
 *     the bitmap).
 *   - ShapesBitMap adds the plane size as a word (sign extended). (#79)
 *   - The planes beyond the depth are not cleared by BitMap (they are
 *     whatever the object held). (#80)
 * Kept from the original, not a bug (no visible effect):
 *   - CludgeBitMap does not compute the interleave/plane distance fields
 *     (+52 cleared, +54 stays from before; no library of the CD reads +54
 *     of a non-interleaved bitmap, so only the field itself differs). (#77)
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include <intuition/screens.h>

/* the objects: struct bb_bitmap, struct bb_shape (blitzobj.h) */

/* ilbmifflib: load an ILBM into a bitmap (a0 = name, a1 = bitmap) */
/* palettelib getpal: load a palette (a0 = name, a1 = palette) */

/* entry points (thunks / C, defined below) */
void _bitmapchk(void), _bitloadchk(void), bitmaplib_sbchk(void), cbchk(void);
void shchk(void), _usechk(void), extbitmap(void);
void BLITZ_ROUTINE _bitmap(UWORD w BL_REG(d1), UWORD h BL_REG(d2), UWORD d BL_REG(d3),
                           WORD usedoff BL_REG(d6));
void BLITZ_ROUTINE _bitload(const UBYTE *name BL_REG(d1), struct bb_bitmap *bm BL_REG(a1));
void BLITZ_ROUTINE _bitload2(const UBYTE *name BL_REG(d1), APTR pal BL_REG(a0),
                             struct bb_bitmap *bm BL_REG(a1));
void BLITZ_ROUTINE doscbitmap(UBYTE **scr BL_REG(a2), WORD usedoff BL_REG(d6));
void BLITZ_ROUTINE docopybitmap(struct bb_bitmap *src BL_REG(a2));
void BLITZ_ROUTINE doshbitmap(const struct bb_shape *shape BL_REG(a2), WORD usedoff BL_REG(d6));
void BLITZ_ROUTINE _cludgebitmap(UWORD w BL_REG(d1), UWORD h BL_REG(d2), UWORD d BL_REG(d3),
                                 ULONG mem BL_REG(d4), WORD usedoff BL_REG(d6));
void BLITZ_ROUTINE _freebitmap(void);

/* !libheader{#bitmaplib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_bitmaplib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1380, LIB_bitmaplib, 0x0604);
BLITZ_SUBS(_bitmap, _bitmapchk, 0);
BLITZ_NAME_AT(_toke, "BitMap", "BitMap#,Width,Height,Depth");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_bitmaplib, 0x1180);
BLITZ_SUBS(BLITZ_AMIGAONLY(_bitload), _bitloadchk, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1180, LIB_palettelib, 0x1082);
BLITZ_SUBS(BLITZ_AMIGAONLY(_bitload2), _bitloadchk, 0);
BLITZ_NAME("LoadBitMap", "BitMap#,Filename$[,Palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_screenslib, 0x1280, LIB_bitmaplib, 0x1381, LIB_bitmaplib, 0x0604);
BLITZ_SUBS(doscbitmap, bitmaplib_sbchk, 0);
BLITZ_NAME("ScreensBitMap", "Screen#,BitMap#");

BLITZ_NULLSUB(extbitmap, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1280, LIB_bitmaplib, 0x1381);
BLITZ_SUBS(docopybitmap, cbchk, 0);
BLITZ_NAME("CopyBitMap", "Src BitMap#,Dest BitMap#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1280, LIB_bitmaplib, 0x1381, LIB_bitmaplib, 0x0604);
BLITZ_SUBS(doshbitmap, shchk, 0);
BLITZ_NAME("ShapesBitMap", "Shape#,BitMap#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, 0x1380, LIB_bitmaplib, 0x0604);
BLITZ_SUBS(_cludgebitmap, 0, 0);
BLITZ_NAME("CludgeBitMap", "BitMap#,Width,Height,Depth,Memory");

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, _usechk, 0);
BLITZ_NULLSUB_AT(_free, _freebitmap, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,10,6} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 10, 6);

/* ********************* Code Follows ************************* */

#define BM ((struct bb_bitmap *)blitz_string)

static void setused(WORD off)
{
    *(struct bb_bitmap **)(blitz_globals + off) = BM;     /* MOVE.l a3,0(a5,d6) */
}

/* _freebitmap - our own memory (isreal < 0) is freed */
static void freebitmap(struct bb_bitmap *b)
{
    if (b->isreal < 0) {
        struct bl_regs r = { 0 };

        r.d0 = (ULONG)(UWORD)(b->rows * b->flagsdepth) * b->bpr;
        r.a1 = (ULONG)b->planes[0];
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
    b->isreal = 0;
}

/**
 * @brief Free BitMap (the object's _free nullsub): free the planes of the
 *        bitmap in a3 if they are its own memory (isreal < 0).
 *
 * memlib globfree of rows * depth * bytes per row (BUG(orig) (#76): MULU
 * word products); isreal = 0.
 */
void BLITZ_ROUTINE _freebitmap(void)
{
    freebitmap(BM);
}

/* prepxtra - the extra fields */
static void prepxtra(struct bb_bitmap *b)
{
    UWORD d, w;
    ULONG dist;

    b->oldfd = b->flagsdepth;
    b->flags = 0;
    b->ilword = 0;
    b->height = b->rows;
    d = b->flagsdepth;                          /* = depth */
    b->colours = d < 16 ? 1u << d : 0;          /* LSL d1,d0 (word) */
    w = b->bpr << 3;
    if ((WORD)d >= 2) {
        dist = (ULONG)b->planes[1] - (ULONG)b->planes[0];
        if (dist && dist < 32768 && (UWORD)dist <= b->bpr) {
            w = (UWORD)bl_divu_w(w, d);         /* DIVU d1,d0 */
            b->ilflag |= 0x80;
        }
    }
    b->width = w;
    dist = (ULONG)b->planes[1] - (ULONG)b->planes[0];
    if (dist >= 32768) {
        b->planedist = 0;                       /* fail */
        return;
    }
    if ((WORD)(d - 3) >= 0) {
        UWORD n = d - 3;
        UBYTE **p = &b->planes[1];

        do {
            if ((ULONG)p[1] - (ULONG)p[0] != dist) {
                b->planedist = 0;
                return;
            }
            p++;
        } while (n-- != 0);
    }
    b->planedist = (UWORD)dist;
}

/* extbitmap - a3 = object, d1/d2/d3 = width (pixels), height, depth */
static void ext(struct bb_bitmap *b, UWORD w, UWORD h, UWORD d)
{
    struct bl_regs r = { 0 };
    ULONG mem, psize;
    UWORD n;
    UBYTE **p = b->planes;

    w = (UWORD)((UWORD)(w + 15) >> 4) * 2;      /* even byte width */
    b->bpr = w;
    b->rows = h;
    b->flags = 0;
    b->depth = d;
    psize = (ULONG)w * h;                       /* MULU d1,d2 */
    r.d0 = (ULONG)(UWORD)((ULONG)h * d) * w;    /* MULU d3,d0:MULU d1,d0 */
    r.d1 = MEMF_CHIP | MEMF_CLEAR;
    mem = BLITZ_ALIBJSR(TOK_globalloc, r);
    n = d - 1;
    do {
        *p++ = (UBYTE *)mem;
        mem += psize;
    } while (n-- != 0);
    prepxtra(b);
}

/**
 * @brief extbitmap (nullsub, for other libraries): allocate and set up
 *        the bitmap in a3 without freeing it first.
 *
 * Even byte width, one MEMF_CHIP|MEMF_CLEAR block for all planes (memlib
 * globalloc; BUG(orig) (#76): the size is a MULU word product), then the
 * extra fields (prepxtra).  isreal is not set.
 *
 * @param w  d1.w: the width in pixels
 * @param h  d2.w: the height
 * @param d  d3.w: the depth
 */
static void BLITZ_ROUTINE extbitmap_c(UWORD w BL_REG(d1), UWORD h BL_REG(d2),
                                      UWORD d BL_REG(d3))
{
    ext(BM, w, h, d);
}
BLITZ_THUNK(extbitmap, extbitmap_c, "a0");

/**
 * @brief BitMap BitMap#,Width,Height,Depth: a new bitmap in chip memory.
 *
 * Becomes the used bitmap; frees the old one, isreal = -1 (own memory),
 * then as extbitmap.  BUG(orig): the size is a MULU word product (#76);
 * the planes beyond the depth are not cleared (#80).  Errors (debugger,
 * _bitmapchk): error 1 if the depth is 0 or more than 8.
 * The bitmap object comes in a3 (!libs {#bitmaplib,$1380/$1381}).
 *
 * @param w        d1.w: the width in pixels
 * @param h        d2.w: the height
 * @param d        d3.w: the depth
 * @param usedoff  d6: offset of the used bitmap from a5 (!libs
 *                 {#bitmaplib,$0604}); it becomes this bitmap
 */
void BLITZ_ROUTINE _bitmap(UWORD w BL_REG(d1), UWORD h BL_REG(d2), UWORD d BL_REG(d3),
                           WORD usedoff BL_REG(d6))
{
    struct bb_bitmap *b = BM;

    setused(usedoff);
    freebitmap(b);
    b->isreal = -1;
    ext(b, w, h, d);
}

/**
 * @brief CludgeBitMap BitMap#,Width,Height,Depth,Memory: a bitmap in
 *        memory of the program (borrowed, isreal = 1).
 *
 * Becomes the used bitmap; frees the old one; the planes follow each
 * other in the memory.  No interleave check: not a bug (no visible
 * effect, #77): +52 is cleared, the plane distance +54 stays from before.
 * The bitmap object comes in a3 (!libs {#bitmaplib,$1380/$1381}).
 *
 * @param w        d1.w: the width in pixels
 * @param h        d2.w: the height
 * @param d        d3.w: the depth
 * @param mem      d4: the memory (chip memory for display/blits)
 * @param usedoff  d6: offset of the used bitmap from a5 (!libs
 *                 {#bitmaplib,$0604}); it becomes this bitmap
 */
void BLITZ_ROUTINE _cludgebitmap(UWORD w BL_REG(d1), UWORD h BL_REG(d2), UWORD d BL_REG(d3),
                                 ULONG mem BL_REG(d4), WORD usedoff BL_REG(d6))
{
    struct bb_bitmap *b = BM;
    ULONG psize;
    UWORD n;
    UBYTE **p = b->planes;

    setused(usedoff);
    freebitmap(b);
    b->isreal = 1;
    w = (UWORD)((UWORD)(w + 15) >> 4) * 2;
    b->bpr = w;
    b->rows = h;
    b->flags = 0;
    b->depth = d;
    psize = (ULONG)w * h;
    n = d - 1;
    do {
        *p++ = (UBYTE *)mem;
        mem += psize;
    } while (n-- != 0);
    b->oldfd = b->flagsdepth;
    b->flags = 0;
    b->ilword = 0;
    b->height = b->rows;
    b->colours = b->flagsdepth < 16 ? 1u << b->flagsdepth : 0;
    b->width = b->bpr << 3;
}

/**
 * @brief CopyBitMap Src BitMap#,Dest BitMap#: a copy of the source bitmap
 *        in own chip memory.
 *
 * Frees the destination, allocates one MEMF_CHIP block (not cleared) and
 * copies plane by plane.  BUG(orig): (bytes per row * rows) / 2 words per
 * plane, 0 words = 4 billion (#78); the size is a MULU word product
 * (#76).  Not a bug: the used bitmap is not set (CopyBitMap is not
 * documented to select it).  Errors (debugger, cbchk): "Can't copy
 * borrowed Bitmap" (the destination is borrowed).
 *
 * @param src  a2: the source bitmap (!libs {#bitmaplib,$1280}); the
 *             destination comes in a3 ($1381)
 */
void BLITZ_ROUTINE docopybitmap(struct bb_bitmap *src BL_REG(a2))
{
    struct bb_bitmap *b = BM;
    struct bl_regs r = { 0 };
    ULONG words;
    UBYTE *mem;
    UWORD n;
    int i;

    freebitmap(b);
    words = ((ULONG)src->bpr * src->rows) >> 1;
    r.d0 = (ULONG)(UWORD)((ULONG)src->rows * *(UWORD *)&src->flags) * src->bpr;
    r.d1 = MEMF_CHIP;
    mem = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
    *(ULONG *)&b->bpr = *(ULONG *)&src->bpr;
    *(ULONG *)&b->flags = *(ULONG *)&src->flags;
    b->isreal = -1;
    n = *(UWORD *)&src->flags - 1;
    i = 0;
    do {
        const UWORD *s = (const UWORD *)src->planes[i];
        UWORD *dd = (UWORD *)mem;
        ULONG k = words;

        b->planes[i++] = mem;
        do
            *dd++ = *s++;
        while (--k);
        mem = (UBYTE *)dd;
    } while (n-- != 0);
    prepxtra(b);
}

/**
 * @brief ScreensBitMap Screen#,BitMap#: the bitmap of a screen (borrowed,
 *        isreal = 1).
 *
 * Becomes the used bitmap; frees the old one, copies the screen's struct
 * BitMap (RastPort.BitMap, 40 bytes), then the extra fields.  Errors
 * (debugger, sbchk): "Uninitialized Screen".
 *
 * @param scr      a2: the screen object (!libs {#screenslib,$1280}): its
 *                 Screen; the bitmap comes in a3 ($1381)
 * @param usedoff  d6: offset of the used bitmap from a5 (!libs
 *                 {#bitmaplib,$0604}); it becomes this bitmap
 */
void BLITZ_ROUTINE doscbitmap(UBYTE **scr BL_REG(a2), WORD usedoff BL_REG(d6))
{
    struct bb_bitmap *b = BM;
    const ULONG *s;
    ULONG *d = (ULONG *)b;
    int i;

    setused(usedoff);
    freebitmap(b);
    s = (const ULONG *)((struct Screen *)*scr)->RastPort.BitMap; /* +88 */
    b->isreal = 1;
    for (i = 0; i < 10; i++)
        *d++ = *s++;
    prepxtra(b);
}

/**
 * @brief ShapesBitMap Shape#,BitMap#: a bitmap on the planes of a shape
 *        (borrowed, isreal = 1).
 *
 * Becomes the used bitmap; frees the old one; bytes per row, rows and
 * depth from the shape.  BUG(orig) (#79): the plane size is added as a
 * word (sign extended).  Errors (debugger, shchk): "Uninitialized Shape".
 *
 * @param shape    a2: the shape (!libs {#shapeslib,$1280}); the bitmap
 *                 comes in a3 ($1381)
 * @param usedoff  d6: offset of the used bitmap from a5 (!libs
 *                 {#bitmaplib,$0604}); it becomes this bitmap
 */
void BLITZ_ROUTINE doshbitmap(const struct bb_shape *shape BL_REG(a2), WORD usedoff BL_REG(d6))
{
    struct bb_bitmap *b = BM;
    UBYTE *pl;
    WORD size;
    UWORD n;
    UBYTE **p = b->planes;

    setused(usedoff);
    freebitmap(b);
    b->bpr = shape->ebwidth;
    b->rows = shape->height;
    n = shape->depth;
    b->flagsdepth = n;
    b->isreal = 1;
    n--;
    pl = shape->data;
    size = shape->planesize;
    do {
        *p++ = pl;
        pl += size;
    } while (n-- != 0);
    prepxtra(b);
}

/**
 * @brief LoadBitMap BitMap#,Filename$: load an ILBM into the bitmap.
 *
 * Amiga mode only.  ilbmifflib +3 ilbmbitmap (a0 = name, a1 = bitmap).
 * Errors (debugger, _bitloadchk): "Uninitialized BitMap".
 *
 * @param name  d1: the file name
 * @param bm    a1: the bitmap (!libs {#bitmaplib,$1180})
 */
void BLITZ_ROUTINE _bitload(const UBYTE *name BL_REG(d1), struct bb_bitmap *bm BL_REG(a1))
{
    struct bl_regs r = { .a0 = (ULONG)name, .a1 = (ULONG)bm };

    BLITZ_ALIBJSR(TOK_ilbmbitmap, r);
}

/**
 * @brief LoadBitMap BitMap#,Filename$,Palette#: LoadBitMap, first the
 *        palette (palettelib +3, a0 = name, a1 = palette).
 *
 * Amiga mode only.  Errors (debugger, _bitloadchk): "Uninitialized
 * BitMap".
 *
 * @param name  d1: the file name
 * @param pal   a0: the palette (!libs {#palettelib,$1082})
 * @param bm    a1: the bitmap (!libs {#bitmaplib,$1180})
 */
void BLITZ_ROUTINE _bitload2(const UBYTE *name BL_REG(d1), APTR pal BL_REG(a0),
                             struct bb_bitmap *bm BL_REG(a1))
{
    struct bl_regs r = { .a0 = (ULONG)name, .a1 = (ULONG)pal };

    BLITZ_ALIBJSR(TOK_getpal, r);
    _bitload(name, bm);
}

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess3, "Uninitialized Screen");
BLITZ_ERRCHK_MESSAGE(mess4, "Uninitialized Shape");
BLITZ_ERRCHK_MESSAGE(mess5, "Can't copy borrowed Bitmap");

/* err1: MOVEQ #1,d0:TRAP #0 - error number 1 */
#define ERR1 ((const char *)1)

static void BLITZ_ERRCHK _bitmapchk_c(UWORD d BL_REG(d3))
{
    if (!d || d > 8)
        BLITZ_RUNERR(ERR1);
}

static void BLITZ_ERRCHK _usechk_c(void)
{
    if (!BM->isreal)
        BLITZ_RUNERR(mess2);
}

static void BLITZ_ERRCHK _bitloadchk_c(const struct bb_bitmap *b BL_REG(a1))
{
    if (!b->isreal)
        BLITZ_RUNERR(mess2);
}

static void BLITZ_ERRCHK sbchk_c(const ULONG *s BL_REG(a2))
{
    if (!*s)
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK shchk_c(const ULONG *s BL_REG(a2))
{
    if (!*s)
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK cbchk_c(const struct bb_bitmap *b BL_REG(a2))
{
    if (b->isreal >= 0)
        BLITZ_RUNERR(mess5);
}

BLITZ_ERRCHK_THUNK(_bitmapchk, _bitmapchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_usechk, _usechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_bitloadchk, _bitloadchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(bitmaplib_sbchk, sbchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(shchk, shchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(cbchk, cbchk_c, "d0-d1/a0-a1");
