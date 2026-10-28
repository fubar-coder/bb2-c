/*
 * iffmakelib - command library 114: SaveBitmap SaveScreen SaveShape
 * SavePalette (ILBM / CMAP files), plus a nullsub `dosavebitmap` for other
 * libraries (a0 = BitMap, d1 = file name, a1 = palette or 0, a6 = dos).
 *
 * C translation of bblibsrc/iffmakelib ("13/11/93 fixed null to even out
 * bodylength not linelength, also changed compression for repeats of > 2
 * not > 1"); binary of the CD's acidlibs checked: same as the source.
 *
 * All four statements are Amiga mode only: their code addresses in the
 * token table are odd (`!subs{label+1,...}`), the compiler's flag for
 * "not in Blitz mode" (blitz2.s doajsrrout); BLITZ_BLITZ_AMIGAONLY() below.
 *
 * File layout (all writes go through the 4096 byte buffer iffbuff):
 *   FORM <len> ILBM BMHD $14: width = BytesPerRow*8 (shapes: the pixel
 *   width), height = Rows, x = y = 0, nPlanes = the byte at BitMap+5,
 *   masking 0, compression 1 (ByteRun1), pad 0, transparent colour 0,
 *   aspect 10:11, page = width/height;
 *   CMAP (palette given): 3 bytes per colour = the top bytes of the
 *   palette's 32 bit values, no pad byte for an odd count;
 *   BODY: every row of every plane packed separately (savearow), one $80
 *   (no-op) byte if the body length is odd.  The FORM and BODY lengths are
 *   written at the end with Seek.
 * SaveScreen builds the palette from the screen's ColorMap: with Exec 39+
 * (environlib ExecVersion) GetRGB32 one colour at a time, before that the
 * 4 bit values of ColorMap->ColorTable spread to 32 bits ($A -> $AAAAAAAA);
 * at most 256 colours.  The BitMap is copied (64 bytes from the RastPort's
 * BitMap, Flags cleared).  SaveShape describes the shape as a BitMap
 * (BytesPerRow = +6, Rows = +2, depth word = +4, planes from +14 in steps
 * of the plane size +22).
 *
 * BUG(orig), kept:
 *  - nPlanes is the byte at BitMap+5, the plane loop compares with the
 *    WORD at +4 (Flags<<8 | Depth); both loops (rows, planes) run at least
 *    once (and 65536 times for 0 colours in SaveScreen without AGA).
 *  - savearow reads one byte behind the row (literal run test); indexes
 *    are signed words (a2 + d4.w).
 *  - CMAP chunks of an odd size are not padded.
 *  - SaveBitmap with a palette of 0 colours writes 65536 colours (DBF) -
 *    over the end of the buffer (not tested).
 *  - Can't open the file: runtime error "Can't open File" (err2 in the
 *    normal code, message in the error check area as in the original:
 *    without the debugger d0 points behind the library).
 *  - The ExecVersion check uses CMP.w/BLT (signed).
 *  - SaveShape does not check the depth: depth 0 writes 65536 plane
 *    pointers (DBF) from tempbm on, depth 15 and more run past its 14
 *    pointers (usewid, tempcols, then whatever follows).
 *
 * Error checks (debugger only): uninitialized bitmap (BytesPerRow word 0),
 * palette (colour pointer 0), screen (pointer 0), shape (width 0).
 */
#include "blitzlib.h"
#include <graphics/gfx.h>
#include <graphics/view.h>
#define __NOLIBBASE__
#include <proto/dos.h>
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks, defined below) */
void dosavebitmap2(void), dosavebitmap(void), dosavescreen(void);
void dosaveshape1(void), dosaveshape2(void), dosavepalette(void);
void bitchk(void), bitchk2(void), scrchk(void), sschk1(void), sschk2(void);

/* odd code address in the token table: Amiga mode only */

/* !libheader{#iffmakelib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_iffmakelib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_doslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dosavebitmap2), bitchk, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080, LIB_palettelib, 0x1182, LIB_doslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dosavebitmap), bitchk2, 0);
BLITZ_NAME("SaveBitmap", "BitMap#,Filename$[,Palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_screenslib, 0x1080, LIB_graphicslib, BL_LA(1), LIB_doslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dosavescreen), scrchk, 0);
BLITZ_NAME("SaveScreen", "Screen#,Filename$");

BLITZ_NULLSUB(dosavebitmap, 0, 0, 252, BL_LA(6));

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, 0x1080, LIB_doslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dosaveshape1), sschk1, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1080, LIB_palettelib, 0x1182, LIB_doslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dosaveshape2), sschk2, 0);
BLITZ_NAME("SaveShape", "Shape#,Filename$[,Palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_palettelib, 0x1280, LIB_doslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dosavepalette), 0, 0);
BLITZ_NAME("SavePalette", "Palette#,Filename$");

BLITZ_LIBFIN();

/* ------------------------------------------------------------ variables */

#define BUFSIZE 4096

/* a palette object: colour block (one long, then 3 longs per colour),
   number of colours */
struct palette {
    ULONG *cols;
    UWORD num;
};

static UBYTE iffbuff[BUFSIZE] __attribute__((aligned(4)));
static ULONG ifflen, bodylen, bodyseek;
static ULONG tempcols[3 * 256 + 2];
static struct palette temppal = { tempcols, 0 };
static ULONG tempbm[64 / 4];                    /* BitMap (+ 24 bytes) */
static UWORD usewid;

static const UBYTE jeeze[4] = { 0x80, 0, 0, 0 };

static BPTR fh;                                 /* d7 */
static struct DosLibrary *dosbase;
#define DOSBase dosbase                         /* for proto/dos.h */

/* BitMap fields as the original reads them */
#define BM_BPR(bm)    (*(UWORD *)(bm))
#define BM_ROWS(bm)   (*(UWORD *)((UBYTE *)(bm) + 2))
#define BM_DEPTHW(bm) (*(UWORD *)((UBYTE *)(bm) + 4))
#define BM_DEPTH(bm)  (*((UBYTE *)(bm) + 5))

#define ID_FORM 0x464f524d
#define ID_ILBM 0x494c424d
#define ID_BMHD 0x424d4844
#define ID_CMAP 0x434d4150
#define ID_BODY 0x424f4459

BLITZ_ERRCHK_MESSAGE(mess1, "Uninitialized Bitmap");
BLITZ_ERRCHK_MESSAGE(mess2, "Can't open File");
BLITZ_ERRCHK_MESSAGE(mess3, "Uninitialized Palette");
BLITZ_ERRCHK_MESSAGE(mess4, "Uninitialized Screen");
BLITZ_ERRCHK_MESSAGE(mess5, "Uninitialized Shape");

/* -------------------------------------------------------------- chunks */

static void put_l(UBYTE **p, ULONG v)
{
    *(ULONG *)*p = v;
    *p += 4;
}

static void put_w(UBYTE **p, UWORD v)
{
    *(UWORD *)*p = v;
    *p += 2;
}

/* FORM/ILBM/BMHD, 40 bytes */
static void makehead(const UBYTE *bm)
{
    UBYTE *a1 = iffbuff;
    UWORD d0 = usewid, d1;

    put_l(&a1, ID_FORM);
    put_l(&a1, 0);
    put_l(&a1, ID_ILBM);
    put_l(&a1, ID_BMHD);
    put_l(&a1, 0x14);
    if (!d0)
        d0 = (UWORD)(BM_BPR(bm) << 3);
    put_w(&a1, d0);
    d1 = BM_ROWS(bm);
    put_w(&a1, d1);
    put_l(&a1, 0);                              /* x, y */
    *a1++ = BM_DEPTH(bm);                       /* nPlanes */
    *a1++ = 0;                                  /* masking */
    *a1++ = 1;                                  /* compression */
    *a1++ = 0;
    put_l(&a1, 0xa0b);                          /* transparent, aspect */
    put_w(&a1, d0);
    put_w(&a1, d1);
    ifflen = 40;
    Write(fh, iffbuff, 40);
}

/* CMAP of the palette a2 */
static void makecolor(const struct palette *pal)
{
    UBYTE *a1 = iffbuff;
    const ULONG *a0 = pal->cols + 1;
    UWORD d2 = pal->num;
    ULONG d0 = (UWORD)(d2 + (UWORD)(d2 << 1));  /* LSL #1:ADD (words) */

    put_l(&a1, ID_CMAP);
    put_l(&a1, d0);
    ifflen += d0;
    d2--;
    do {                                        /* DBF d2 */
        *a1++ = (UBYTE)(*a0++ >> 24);
        *a1++ = (UBYTE)(*a0++ >> 24);
        *a1++ = (UBYTE)(*a0++ >> 24);
    } while (d2-- != 0);
    ifflen += 8;
    Write(fh, iffbuff, a1 - iffbuff);
}

/* one row (BytesPerRow bytes at row) ByteRun1-packed and written */
static void savearow(const UBYTE *row, const UBYTE *bm)
{
    UBYTE *a1 = iffbuff;
    UWORD w = BM_BPR(bm);
    UWORD d4 = 0, d0, d1;
    UBYTE d2;
    LONG d3;

    for (;;) {                                  /* main */
        if (d4 >= w)
            break;
        d2 = row[(WORD)d4];
        d4 += 2;
        if (d4 == w) {                          /* one lone word at the edge */
            *a1++ = 1;
            *a1++ = d2;
            *a1++ = row[(WORD)d4 - 1];
            break;
        }
        d4--;
        if (d4 >= w) {                          /* one lone byte at the edge */
            *a1++ = 0;
            *a1++ = d2;
            break;
        }
        d4--;                                   /* skip */
        d1 = 1;
        d0 = d4 + 2;
        if (row[(WORD)d4 + 1] == d2 && row[(WORD)d4 + 2] == d2) {
            /* a repeating lot */
            while (d1 < 127 && d0 != w && row[(WORD)d0] == d2) {
                d1++;
                d0++;
            }
            d4 = d0;
            *a1++ = (UBYTE)-d1;
            *a1++ = d2;
        } else {
            /* a literal lot */
            d2 = row[(WORD)d4 + 1];             /* last byte */
            while (d1 < 128 && d0 < w) {
                if (row[(WORD)d0 + 1] == d2 && row[(WORD)d0] == d2)
                    break;
                d2 = row[(WORD)d0];
                d1++;
                d0++;
            }
            d1--;
            *a1++ = (UBYTE)d1;
            do                                  /* DBF d1 */
                *a1++ = row[(WORD)d4++];
            while (d1-- != 0);
        }
    }
    d3 = a1 - iffbuff;
    if (d3) {
        ifflen += d3;
        bodylen += d3;
        Write(fh, iffbuff, d3);
    }
}

static void makebody(const UBYTE *bm)
{
    UWORD d6 = 0;

    *(ULONG *)iffbuff = ID_BODY;
    Write(fh, iffbuff, 4);
    bodyseek = Seek(fh, 0, OFFSET_CURRENT);     /* getseek */
    Write(fh, iffbuff, 4);                      /* length, fixed later */
    ifflen += 8;
    bodylen = 0;
    do {                                        /* y counter */
        UWORD d5 = 0;

        do {                                    /* bitplane counter */
            ULONG d3 = (ULONG)d6 * BM_BPR(bm);  /* MULU */
            savearow(*(UBYTE **)(bm + 8 + (WORD)(UWORD)(d5 << 2)) + d3, bm);
            d5++;
        } while (d5 < BM_DEPTHW(bm));
        d6++;
    } while (d6 < BM_ROWS(bm));
    if (bodylen & 1) {
        ifflen++;
        bodylen++;
        Write(fh, (APTR)jeeze, 1);
    }
}

static void fixhead(void)
{
    ifflen -= 8;
    Seek(fh, 4, OFFSET_BEGINNING);
    Write(fh, &ifflen, 4);
}

static void fixbody(void)
{
    Seek(fh, bodyseek, OFFSET_BEGINNING);
    Write(fh, &bodylen, 4);
}

/* ---------------------------------------------------------- statements */

/* dosavebm2 - a0 = BitMap, d1 = file name, a1 = palette or 0 */
static void savebm(const UBYTE *bm, const char *name, const struct palette *pal,
                   struct DosLibrary *dos)
{
    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (!fh)
        BLITZ_RUNERR(mess2);                    /* BEQ err2 */
    makehead(bm);
    if (pal)
        makecolor(pal);
    makebody(bm);
    fixhead();
    fixbody();
    Close(fh);
}

static void BLITZ_ROUTINE dosavebitmap2_c(const UBYTE *bm BL_REG(a0), const char *name BL_REG(d1),
                                          struct DosLibrary *dos BL_REG(a6))
{
    usewid = 0;
    savebm(bm, name, 0, dos);
}
BLITZ_THUNK(dosavebitmap2, dosavebitmap2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dosavebitmap_c(const UBYTE *bm BL_REG(a0), const char *name BL_REG(d1),
                                         const struct palette *pal BL_REG(a1),
                                         struct DosLibrary *dos BL_REG(a6))
{
    usewid = 0;
    savebm(bm, name, pal, dos);
}
BLITZ_THUNK(dosavebitmap, dosavebitmap_c, "d0-d1/a0-a1");

/* SaveShape: the shape as BitMap in tempbm */
static void saveshape(const UBYTE *shape, const char *name, const struct palette *pal,
                      struct DosLibrary *dos)
{
    UBYTE *a2 = (UBYTE *)tempbm;
    UBYTE *a1;
    UWORD d0;

    usewid = *(const UWORD *)shape;
    *(UWORD *)a2 = *(const UWORD *)(shape + 6);
    *(UWORD *)(a2 + 2) = *(const UWORD *)(shape + 2);
    *(UWORD *)(a2 + 4) = *(const UWORD *)(shape + 4);
    a2 += 8;
    a1 = *(UBYTE *const *)(shape + 14);
    d0 = *(const UWORD *)(shape + 4) - 1;   /* BUG(orig): depth unchecked */
    do {
        *(UBYTE **)a2 = a1;
        a2 += 4;
        a1 += *(const WORD *)(shape + 22);      /* ADD 22(a0),a1 */
    } while (d0-- != 0);
    savebm((UBYTE *)tempbm, name, pal, dos);
}

static void BLITZ_ROUTINE dosaveshape1_c(const UBYTE *shape BL_REG(a0), const char *name BL_REG(d1),
                                         struct DosLibrary *dos BL_REG(a6))
{
    saveshape(shape, name, 0, dos);
}
BLITZ_THUNK(dosaveshape1, dosaveshape1_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dosaveshape2_c(const UBYTE *shape BL_REG(a0), const char *name BL_REG(d1),
                                         const struct palette *pal BL_REG(a1),
                                         struct DosLibrary *dos BL_REG(a6))
{
    saveshape(shape, name, pal, dos);
}
BLITZ_THUNK(dosaveshape2, dosaveshape2_c, "d0-d1/a0-a1");

/* SavePalette - nothing for a palette without colours */
static void BLITZ_ROUTINE dosavepalette_c(const struct palette *pal BL_REG(a2),
                                          const char *name BL_REG(d1),
                                          struct DosLibrary *dos BL_REG(a6))
{
    if (!pal->num)
        return;
    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (!fh)
        BLITZ_RUNERR(mess2);
    makecolor(pal);
    Close(fh);
}
BLITZ_THUNK(dosavepalette, dosavepalette_c, "d0-d1/a0-a1");

/* 4 bit colour value -> 32 bits */
static ULONG makelong(UWORD v)
{
    v &= 15;
    v |= v << 4;
    v |= v << 8;
    return (ULONG)v << 16 | v;
}


/* SaveScreen - a0 = screen object, a1 = graphics base */
static void BLITZ_ROUTINE dosavescreen_c(void **scr BL_REG(a0), const char *name BL_REG(d1),
                                         struct GfxBase *gfx BL_REG(a1),
                                         struct DosLibrary *dos BL_REG(a6))
{
    UBYTE *s = (UBYTE *)*scr;
    struct ColorMap *cm = *(struct ColorMap **)(s + 48);   /* ViewPort.ColorMap */
    UWORD d7 = cm->Count;
    struct bl_regs r = { 0 };
    ULONG *a1;
    UBYTE *bm;
    UWORD i;

    if (d7 > 256)
        d7 = 256;
    temppal.num = d7;
    BLITZ_ALIBJSR(TOK_execversion, r);              /* find aga flag */
    if ((WORD)r.d0 >= 39) {
        /* it's A G A */
        UWORD d6 = 0;
        struct GfxBase *GfxBase = gfx;

        a1 = tempcols + 1;
        do {
            GetRGB32(cm, d6, 1, a1);
            a1 += 3;
            d6++;
        } while (d6 < d7);
    } else {
        const UWORD *a2 = (const UWORD *)cm->ColorTable;

        a1 = tempcols + 1;
        d7--;
        do {
            *a1++ = makelong(*a2 >> 8);
            *a1++ = makelong(*a2 >> 4);
            *a1++ = makelong(*a2++);
        } while (d7-- != 0);
    }
    bm = *(UBYTE **)(s + 88);                   /* RastPort.BitMap */
    for (i = 0; i < 16; i++)
        tempbm[i] = ((ULONG *)bm)[i];
    ((UBYTE *)tempbm)[4] = 0;                   /* Flags */
    usewid = 0;
    savebm((UBYTE *)tempbm, name, &temppal, dos);
}
BLITZ_THUNK(dosavescreen, dosavescreen_c, "d0-d1/a0-a1");

/* ************************* ERROR CHECKING ************************* */

static void BLITZ_ERRCHK scrchk_c(ULONG *scr BL_REG(a0))
{
    if (!*scr)
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK bitchk_c(UWORD *bm BL_REG(a0))
{
    if (!*bm)
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK bitchk2_c(UWORD *bm BL_REG(a0), ULONG *pal BL_REG(a1))
{
    if (!*pal)
        BLITZ_RUNERR(mess3);
    bitchk_c(bm);
}

static void BLITZ_ERRCHK sschk1_c(UWORD *shape BL_REG(a0))
{
    if (!*shape)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK sschk2_c(UWORD *shape BL_REG(a0), ULONG *pal BL_REG(a1))
{
    if (!*pal)
        BLITZ_RUNERR(mess3);
    sschk1_c(shape);
}

/* the thunks after all check functions: BLITZ_ERRCHK_THUNK ends with
   .text, and GCC would not switch back to the error check area for a
   check function behind it (see README) */
BLITZ_ERRCHK_THUNK(scrchk, scrchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(bitchk, bitchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(bitchk2, bitchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(sschk1, sschk1_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(sschk2, sschk2_c, "d0-d1/a0-a1");
