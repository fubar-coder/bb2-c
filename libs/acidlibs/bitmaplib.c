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
 *     truncated (the second MULU uses the low word of h * depth).
 *   - CludgeBitMap does not compute the interleave/plane distance fields
 *     (+52 cleared, +54 stays from before; no library of the CD reads +54
 *     of a non-interleaved bitmap, so only the field itself differs).
 *   - CopyBitMap copies plane by plane with (bytes per row * rows) / 2
 *     words (0 words = 4 billion), into one MEMF_CHIP block (not cleared),
 *     and does not set the used bitmap.
 *   - ShapesBitMap adds the plane size as a word (sign extended).
 *   - The planes beyond the depth are not cleared by BitMap (they are
 *     whatever the object held).
 */
#include "blitzlib.h"

/* a bitmap object, 2^6 bytes */
struct bbitmap {
    UWORD bpr;                  /* +0 */
    UWORD rows;                 /* +2 */
    UBYTE flags;                /* +4 */
    UBYTE depth;                /* +5 */
    UWORD oldfd;                /* +6 */
    UBYTE *planes[8];           /* +8 */
    UBYTE pad[12];              /* +40 */
    UBYTE ilflag;               /* +52 bit 7: interleaved */
    UBYTE pad53;
    UWORD planedist;            /* +54 */
    UWORD width;                /* +56 */
    UWORD height;               /* +58 */
    UWORD colours;              /* +60 */
    WORD isreal;                /* +62 */
} BL_PACKED;

/* ilbmifflib: load an ILBM into a bitmap (a0 = name, a1 = bitmap) */
/* palettelib getpal: load a palette (a0 = name, a1 = palette) */

/* entry points (thunks / C, defined below) */
void _bitmapchk(void), _bitloadchk(void), sbchk(void), cbchk(void), shchk(void);
void _usechk(void), extbitmap(void);
void BLITZ_ROUTINE _bitmap(UWORD w BL_REG(d1), UWORD h BL_REG(d2), UWORD d BL_REG(d3),
                           WORD usedoff BL_REG(d6));
void BLITZ_ROUTINE _bitload(const UBYTE *name BL_REG(d1), struct bbitmap *bm BL_REG(a1));
void BLITZ_ROUTINE _bitload2(const UBYTE *name BL_REG(d1), APTR pal BL_REG(a0),
                             struct bbitmap *bm BL_REG(a1));
void BLITZ_ROUTINE doscbitmap(UBYTE **scr BL_REG(a2), WORD usedoff BL_REG(d6));
void BLITZ_ROUTINE docopybitmap(struct bbitmap *src BL_REG(a2));
void BLITZ_ROUTINE doshbitmap(const UBYTE *shape BL_REG(a2), WORD usedoff BL_REG(d6));
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
BLITZ_SUBS(doscbitmap, sbchk, 0);
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

#define BM ((struct bbitmap *)blitz_string)

static void setused(WORD off)
{
    *(struct bbitmap **)(blitz_globals + off) = BM;     /* MOVE.l a3,0(a5,d6) */
}

/* _freebitmap - our own memory (isreal < 0) is freed */
static void freebitmap(struct bbitmap *b)
{
    if (b->isreal < 0) {
        struct bl_regs r = { 0 };

        r.d0 = (ULONG)(UWORD)(b->rows * *(UWORD *)&b->flags) * b->bpr;
        r.a1 = (ULONG)b->planes[0];
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
    b->isreal = 0;
}

void BLITZ_ROUTINE _freebitmap(void)
{
    freebitmap(BM);
}

/* prepxtra - the extra fields */
static void prepxtra(struct bbitmap *b)
{
    UWORD d, w;
    ULONG dist;

    b->oldfd = *(UWORD *)&b->flags;
    b->flags = 0;
    *(UWORD *)&b->ilflag = 0;
    b->height = b->rows;
    d = *(UWORD *)&b->flags;                    /* = depth */
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
static void ext(struct bbitmap *b, UWORD w, UWORD h, UWORD d)
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

static void BLITZ_ROUTINE extbitmap_c(UWORD w BL_REG(d1), UWORD h BL_REG(d2),
                                      UWORD d BL_REG(d3))
{
    ext(BM, w, h, d);
}
BLITZ_THUNK(extbitmap, extbitmap_c, "a0");

/* BitMap n,w,h,d */
void BLITZ_ROUTINE _bitmap(UWORD w BL_REG(d1), UWORD h BL_REG(d2), UWORD d BL_REG(d3),
                           WORD usedoff BL_REG(d6))
{
    struct bbitmap *b = BM;

    setused(usedoff);
    freebitmap(b);
    b->isreal = -1;
    ext(b, w, h, d);
}

/* CludgeBitMap n,w,h,d,mem - borrowed memory; no interleave check */
void BLITZ_ROUTINE _cludgebitmap(UWORD w BL_REG(d1), UWORD h BL_REG(d2), UWORD d BL_REG(d3),
                                 ULONG mem BL_REG(d4), WORD usedoff BL_REG(d6))
{
    struct bbitmap *b = BM;
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
    b->oldfd = *(UWORD *)&b->flags;
    b->flags = 0;
    *(UWORD *)&b->ilflag = 0;
    b->height = b->rows;
    b->colours = *(UWORD *)&b->flags < 16 ? 1u << *(UWORD *)&b->flags : 0;
    b->width = b->bpr << 3;
}

/* CopyBitMap src,dest - a2 = source, a3 = destination */
void BLITZ_ROUTINE docopybitmap(struct bbitmap *src BL_REG(a2))
{
    struct bbitmap *b = BM;
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

/* ScreensBitMap s,b - the screen's BitMap (struct copied) */
void BLITZ_ROUTINE doscbitmap(UBYTE **scr BL_REG(a2), WORD usedoff BL_REG(d6))
{
    struct bbitmap *b = BM;
    const ULONG *s;
    ULONG *d = (ULONG *)b;
    int i;

    setused(usedoff);
    freebitmap(b);
    s = (const ULONG *)*(UBYTE **)(*scr + 88);   /* RastPort.BitMap */
    b->isreal = 1;
    for (i = 0; i < 10; i++)
        *d++ = *s++;
    prepxtra(b);
}

/* ShapesBitMap shape,b - the shape's planes */
void BLITZ_ROUTINE doshbitmap(const UBYTE *shape BL_REG(a2), WORD usedoff BL_REG(d6))
{
    struct bbitmap *b = BM;
    UBYTE *pl;
    WORD size;
    UWORD n;
    UBYTE **p = b->planes;

    setused(usedoff);
    freebitmap(b);
    b->bpr = *(UWORD *)(shape + 6);
    b->rows = *(UWORD *)(shape + 2);
    n = *(UWORD *)(shape + 4);
    *(UWORD *)&b->flags = n;
    b->isreal = 1;
    n--;
    pl = *(UBYTE **)(shape + 14);
    size = *(WORD *)(shape + 22);
    do {
        *p++ = pl;
        pl += size;
    } while (n-- != 0);
    prepxtra(b);
}

/* LoadBitMap n,file$[,palette] - ilbmifflib (a0 = name, a1 = bitmap) */
void BLITZ_ROUTINE _bitload(const UBYTE *name BL_REG(d1), struct bbitmap *bm BL_REG(a1))
{
    struct bl_regs r = { .a0 = (ULONG)name, .a1 = (ULONG)bm };

    BLITZ_ALIBJSR(TOK_ilbmbitmap, r);
}

void BLITZ_ROUTINE _bitload2(const UBYTE *name BL_REG(d1), APTR pal BL_REG(a0),
                             struct bbitmap *bm BL_REG(a1))
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

static void BLITZ_ERRCHK _bitloadchk_c(const struct bbitmap *b BL_REG(a1))
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

static void BLITZ_ERRCHK cbchk_c(const struct bbitmap *b BL_REG(a2))
{
    if (b->isreal >= 0)
        BLITZ_RUNERR(mess5);
}

BLITZ_ERRCHK_THUNK(_bitmapchk, _bitmapchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_usechk, _usechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_bitloadchk, _bitloadchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(sbchk, sbchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(shchk, shchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(cbchk, cbchk_c, "d0-d1/a0-a1");
