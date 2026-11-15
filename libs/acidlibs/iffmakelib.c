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
 *  - both loops (rows, planes) run at least once (#181).
 *  - Not a bug in practice: SaveScreen with a ColorMap of 0 colours would
 *    write 65536 colours (#182); a screen always has colours.
 *  - savearow reads one byte behind the row (literal run test) (#183);
 *    indexes are signed words (a2 + d4.w; not a bug: matters only for rows
 *    of 32 KB or more, which are not usable).
 *  - CMAP chunks of an odd size are not padded. (#184)
 *  - Not a bug (exec versions are < 32768): the ExecVersion check uses
 *    CMP.w/BLT (signed).
 *
 * Fixed (intended deviation, #180): the plane loop of the BODY compares
 * with the byte Depth at BitMap+5, as nPlanes of the BMHD; the original
 * compared with the WORD at +4 (Flags<<8 | Depth), so a bitmap whose
 * Flags byte is not 0 got Flags*256 + Depth planes per row.
 *
 * Fixed (intended deviation, #326): SaveShape stops with "Illegal Shape
 * Depth" for a shape of depth 0 or deeper than the 14 plane pointers of
 * bmcopy (tempbm); the original did not check the depth: depth 0 wrote
 * 65536 plane pointers (DBF) from bmcopy on, depth 15 and more ran past
 * its 14 pointers (usewid, scrcols, then whatever follows).  In release
 * code: LoadShape takes the depth of a file's BMHD as it is (a 24 bit
 * ILBM gives a shape of depth 24).
 *
 * Fixed (intended deviation, #182): SaveBitmap and SaveShape with a
 * palette of 0 colours (InitPalette p,0) wrote 65536 colours (DBF) over
 * the end of the 4096 byte buffer; a programming error: the error checks
 * bitchk2/sschk2 (debugger only) now refuse it with "Uninitialized
 * Palette", the commands are unchanged.
 *
 * Fixed (intended deviation, #185): "Can't open File" (err2 in the normal
 * code) has its message in the error check area in the original (without
 * the debugger d0 points behind the library); here the message is in the
 * normal code.
 *
 * Error checks (debugger only): uninitialized bitmap (BytesPerRow word 0),
 * palette (colour pointer 0, or 0 colours: fixed, #182), screen (pointer
 * 0), shape (width 0).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <intuition/screens.h>
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
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD0, LIB_doslib, BL_LA6);
BLITZ_SUBS(BLITZ_AMIGAONLY(dosavebitmap2), bitchk, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD0, LIB_palettelib, BL_IA1|BL_PD2, LIB_doslib, BL_LA6);
BLITZ_SUBS(BLITZ_AMIGAONLY(dosavebitmap), bitchk2, 0);
BLITZ_NAME("SaveBitmap", "BitMap#,Filename$[,Palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_screenslib, BL_IA0|BL_PD0, LIB_graphicslib, BL_LA1, LIB_doslib, BL_LA6);
BLITZ_SUBS(BLITZ_AMIGAONLY(dosavescreen), scrchk, 0);
BLITZ_NAME("SaveScreen", "Screen#,Filename$");

BLITZ_NULLSUB(dosavebitmap, 0, 0, 252, BL_LA6);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD0, LIB_doslib, BL_LA6);
BLITZ_SUBS(BLITZ_AMIGAONLY(dosaveshape1), sschk1, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD0, LIB_palettelib, BL_IA1|BL_PD2, LIB_doslib, BL_LA6);
BLITZ_SUBS(BLITZ_AMIGAONLY(dosaveshape2), sschk2, 0);
BLITZ_NAME("SaveShape", "Shape#,Filename$[,Palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_palettelib, BL_IA2|BL_PD0, LIB_doslib, BL_LA6);
BLITZ_SUBS(BLITZ_AMIGAONLY(dosavepalette), 0, 0);
BLITZ_NAME("SavePalette", "Palette#,Filename$");

BLITZ_LIBFIN();

/* ------------------------------------------------------------ variables */

#define BUFSIZE 4096

/* the start of a palette object (struct bb_palette of blitzobj.h, pdata
   and numcols): kept local, temppal (SaveScreen's palette) is only
   these 6 bytes; colour block: one long, then 3 longs per colour */
struct palette {
    ULONG *cols;
    UWORD num;
};
static_assert(offsetof(struct palette, num) == offsetof(struct bb_palette, numcols));

/* the shape object is struct bb_shape (blitzobj.h) */

static UBYTE iffbuff[BUFSIZE] __attribute__((aligned(4)));
static ULONG ifflen, bodylen, bodyseek;
static ULONG scrcols[3 * 256 + 2];              /* tempcols: SaveScreen's colours */
static struct palette scrpal = { scrcols, 0 };  /* temppal */
/* tempbm: a BitMap (+ 24 bytes: SaveShape's plane pointers of a shape
   deeper than 8 run into them, up to SHAPEPLANES; fixed #326) */
static struct {
    struct BitMap bm;
    ULONG more[6];
} bmcopy;
static_assert(sizeof bmcopy == 64);
#define SHAPEPLANES ((sizeof bmcopy - offsetof(struct BitMap, Planes)) / sizeof(PLANEPTR))
static_assert(SHAPEPLANES == 14);
static UWORD usewid;

static const UBYTE jeeze[4] = { 0x80, 0, 0, 0 };

static BPTR fh;                                 /* d7 */
static struct DosLibrary *dosbase;
#define DOSBase dosbase                         /* for proto/dos.h */

/* BitMap fields as the original reads them */

#define ID_FORM 0x464f524d
#define ID_ILBM 0x494c424d
#define ID_BMHD 0x424d4844
#define ID_CMAP 0x434d4150
#define ID_BODY 0x424f4459

BLITZ_ERRCHK_MESSAGE(mess1, "Uninitialized Bitmap");
BLITZ_ERRCHK_MESSAGE(mess3, "Uninitialized Palette");
BLITZ_ERRCHK_MESSAGE(mess4, "Uninitialized Screen");
BLITZ_ERRCHK_MESSAGE(mess5, "Uninitialized Shape");
/* err2's message in the normal code (fixed, #185: in the original it
   lies in the error check area) */
static const char mess2[] = "Can't open File";
/* SaveShape's depth limit (fixed, #326), in the normal code */
static const char mess6[] = "Illegal Shape Depth";

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
static void makehead(const struct BitMap *bm)
{
    UBYTE *out = iffbuff;
    UWORD width = usewid, height;

    put_l(&out, ID_FORM);
    put_l(&out, 0);
    put_l(&out, ID_ILBM);
    put_l(&out, ID_BMHD);
    put_l(&out, 0x14);
    if (!width)
        width = (UWORD)(bm->BytesPerRow << 3);
    put_w(&out, width);
    height = bm->Rows;
    put_w(&out, height);
    put_l(&out, 0);                             /* x, y */
    *out++ = bm->Depth;                          /* nPlanes */
    *out++ = 0;                                 /* masking */
    *out++ = 1;                                 /* compression */
    *out++ = 0;
    put_l(&out, 0xa0b);                         /* transparent, aspect */
    put_w(&out, width);
    put_w(&out, height);
    ifflen = 40;
    Write(fh, iffbuff, 40);
}

/* CMAP of the palette a2 */
static void makecolor(const struct palette *pal)
{
    UBYTE *out = iffbuff;
    const ULONG *rgb = pal->cols + 1;
    UWORD ncols = pal->num;                     /* d2 */
    UWORD count;                                /* d2: the DBF counter */
    ULONG size = (UWORD)(ncols + (UWORD)(ncols << 1)); /* LSL #1:ADD (words) */

    put_l(&out, ID_CMAP);
    put_l(&out, size);
    ifflen += size;
    count = ncols - 1;
    do {                                        /* DBF d2 */
        *out++ = (UBYTE)(*rgb++ >> 24);
        *out++ = (UBYTE)(*rgb++ >> 24);
        *out++ = (UBYTE)(*rgb++ >> 24);
    } while (count-- != 0);
    ifflen += 8;
    Write(fh, iffbuff, out - iffbuff);
}

/* one row (BytesPerRow bytes at row) ByteRun1-packed and written */
static void savearow(const UBYTE *row, const struct BitMap *bm)
{
    UBYTE *out = iffbuff;
    UWORD bpr = bm->BytesPerRow;
    UWORD pos = 0, scan, runlen;
    UBYTE byte;
    LONG len;

    for (;;) {                                  /* main */
        if (pos >= bpr)
            break;
        byte = row[(WORD)pos];
        pos += 2;
        if (pos == bpr) {                       /* one lone word at the edge */
            *out++ = 1;
            *out++ = byte;
            *out++ = row[(WORD)pos - 1];
            break;
        }
        pos--;
        if (pos >= bpr) {                       /* one lone byte at the edge */
            *out++ = 0;
            *out++ = byte;
            break;
        }
        pos--;                                  /* skip */
        runlen = 1;
        scan = pos + 2;
        if (row[(WORD)pos + 1] == byte && row[(WORD)pos + 2] == byte) {
            /* a repeating lot */
            while (runlen < 127 && scan != bpr && row[(WORD)scan] == byte) {
                runlen++;
                scan++;
            }
            pos = scan;
            *out++ = (UBYTE)-runlen;
            *out++ = byte;
        } else {
            /* a literal lot */
            UBYTE last = row[(WORD)pos + 1];    /* last byte */

            while (runlen < 128 && scan < bpr) {
                if (row[(WORD)scan + 1] == last && row[(WORD)scan] == last)
                    break;
                last = row[(WORD)scan];
                runlen++;
                scan++;
            }
            runlen--;
            *out++ = (UBYTE)runlen;
            do                                  /* DBF d1 */
                *out++ = row[(WORD)pos++];
            while (runlen-- != 0);
        }
    }
    len = out - iffbuff;
    if (len) {
        ifflen += len;
        bodylen += len;
        Write(fh, iffbuff, len);
    }
}

static void makebody(const struct BitMap *bm)
{
    UWORD row = 0;

    *(ULONG *)iffbuff = ID_BODY;
    Write(fh, iffbuff, 4);
    bodyseek = Seek(fh, 0, OFFSET_CURRENT);     /* getseek */
    Write(fh, iffbuff, 4);                      /* length, fixed later */
    ifflen += 8;
    bodylen = 0;
    do {                                        /* y counter */
        UWORD plane = 0;

        do {                                    /* bitplane counter */
            ULONG offset = (ULONG)row * bm->BytesPerRow; /* MULU */
            /* Planes[plane], the offset a signed word: 8(a2,d1.w) */
            savearow(*(PLANEPTR *)((const UBYTE *)bm + __builtin_offsetof(struct BitMap, Planes)
                                  + (WORD)(UWORD)(plane << 2)) + offset,
                     bm);
            plane++;
        } while (plane < bm->Depth);            /* the byte; fixed (#180) */
        row++;
    } while (row < bm->Rows);
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
static void savebm(const struct BitMap *bm, const char *name, const struct palette *pal,
                   struct DosLibrary *dos)
{
    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (!fh)
        BLITZ_RUNERR(mess2);                    /* BEQ err2, fixed (#185) */
    makehead(bm);
    if (pal)
        makecolor(pal);
    makebody(bm);
    fixhead();
    fixbody();
    Close(fh);
}

/**
 * @brief SaveBitmap BitMap#,Filename$: save a bitmap as an ILBM file
 * without a CMAP.
 *
 * FORM/ILBM/BMHD, the ByteRun1 packed BODY (see the head comment).  Amiga
 * mode only.
 * Errors: "Can't open File" if the file cannot be created (the message in
 * the normal code, fixed, #185); debugger only: "Uninitialized Bitmap"
 * (bitchk).
 * BUG(orig): the row and plane loops run at least once (#181), savearow
 * reads one byte behind the row (#183).  Fixed (#180): the plane loop
 * compares with the byte Depth (the original: the word over Flags and
 * Depth).
 *
 * @param bm    a0: the BitMap (!libs {#bitmaplib,$1080})
 * @param name  d1: the file name
 * @param dos   a6: dos.library
 */
static void BLITZ_ROUTINE dosavebitmap2_c(const struct BitMap *bm BL_REG(a0), const char *name BL_REG(d1),
                                          struct DosLibrary *dos BL_REG(a6))
{
    usewid = 0;
    savebm(bm, name, 0, dos);
}
BLITZ_THUNK(dosavebitmap2, dosavebitmap2_c, "d0-d1/a0-a1");

/**
 * @brief SaveBitmap BitMap#,Filename$,Palette#: save a bitmap as an ILBM
 * file with a CMAP; also the nullsub `dosavebitmap` for other libraries.
 *
 * As SaveBitmap without a palette, plus the CMAP chunk (3 bytes per
 * colour, the top bytes of the palette's longs) when a palette is given.
 * Errors (debugger only): also "Uninitialized Palette" (bitchk2; also for
 * a palette of 0 colours, fixed #182: the original wrote 65536 colours over
 * the end of the buffer).
 * BUG(orig): CMAP chunks of an odd size are not padded (#184).
 *
 * @param bm    a0: the BitMap (!libs {#bitmaplib,$1080})
 * @param name  d1: the file name
 * @param pal   a1: the palette (!libs {#palettelib,$1182}), for other
 *              libraries 0 = none
 * @param dos   a6: dos.library
 */
static void BLITZ_ROUTINE dosavebitmap_c(const struct BitMap *bm BL_REG(a0), const char *name BL_REG(d1),
                                         const struct palette *pal BL_REG(a1),
                                         struct DosLibrary *dos BL_REG(a6))
{
    usewid = 0;
    savebm(bm, name, pal, dos);
}
BLITZ_THUNK(dosavebitmap, dosavebitmap_c, "d0-d1/a0-a1");

/* SaveShape: the shape as BitMap in bmcopy */
static void saveshape(const struct bb_shape *shape, const char *name, const struct palette *pal,
                      struct DosLibrary *dos)
{
    PLANEPTR *dst = bmcopy.bm.Planes;
    UBYTE *plane;
    UWORD count;

    if ((UWORD)(shape->depth - 1) >= SHAPEPLANES)  /* fixed (#326) */
        BLITZ_RUNERR(mess6);
    usewid = shape->width;
    bmcopy.bm.BytesPerRow = shape->ebwidth;
    bmcopy.bm.Rows = shape->height;
    *(UWORD *)&bmcopy.bm.Flags = shape->depth;  /* the word: Flags = 0, Depth */
    plane = shape->data;
    count = shape->depth - 1;
    do {
        *dst++ = plane;
        plane += (WORD)shape->planesize;        /* ADD 22(a0),a1 */
    } while (count-- != 0);
    savebm(&bmcopy.bm, name, pal, dos);
}

/**
 * @brief SaveShape Shape#,Filename$: save a shape as an ILBM file.
 *
 * The shape is described as a BitMap (BytesPerRow = its even byte width,
 * Rows = its height, the depth word, plane pointers from its data in steps
 * of its plane size); the BMHD width is the shape's pixel width.  Amiga
 * mode only.  Errors as SaveBitmap; debugger only: "Uninitialized Shape"
 * (sschk1).
 * Errors: also "Illegal Shape Depth" for a depth of 0 or more than 14
 * (fixed, #326: the original wrote 65536 plane pointers for depth 0 and
 * ran past the 14 pointers of tempbm from depth 15 on).
 *
 * @param shape  a0: the shape (!libs {#shapeslib,$1080})
 * @param name   d1: the file name
 * @param dos    a6: dos.library
 */
static void BLITZ_ROUTINE dosaveshape1_c(const struct bb_shape *shape BL_REG(a0), const char *name BL_REG(d1),
                                         struct DosLibrary *dos BL_REG(a6))
{
    saveshape(shape, name, 0, dos);
}
BLITZ_THUNK(dosaveshape1, dosaveshape1_c, "d0-d1/a0-a1");

/**
 * @brief SaveShape Shape#,Filename$,Palette#: SaveShape with a CMAP of the
 * palette (as SaveBitmap with a palette).
 *
 * Errors (debugger only): also "Uninitialized Palette" (sschk2; also for
 * a palette of 0 colours, fixed #182).
 *
 * @param shape  a0: the shape (!libs {#shapeslib,$1080})
 * @param name   d1: the file name
 * @param pal    a1: the palette (!libs {#palettelib,$1182})
 * @param dos    a6: dos.library
 */
static void BLITZ_ROUTINE dosaveshape2_c(const struct bb_shape *shape BL_REG(a0), const char *name BL_REG(d1),
                                         const struct palette *pal BL_REG(a1),
                                         struct DosLibrary *dos BL_REG(a6))
{
    saveshape(shape, name, pal, dos);
}
BLITZ_THUNK(dosaveshape2, dosaveshape2_c, "d0-d1/a0-a1");

/**
 * @brief SavePalette Palette#,Filename$: save a palette as a file with
 * only the CMAP chunk (no FORM).
 *
 * Nothing for a palette without colours.  Amiga mode only.
 * Errors: "Can't open File" as SaveBitmap; BUG(orig): the CMAP is not
 * padded (#184).
 *
 * @param pal   a2: the palette (!libs {#palettelib,$1280})
 * @param name  d1: the file name
 * @param dos   a6: dos.library
 */
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


/**
 * @brief SaveScreen Screen#,Filename$: save a screen as an ILBM file with
 * its colours.
 *
 * The palette from the screen's ColorMap (at most 256 colours): with Exec
 * 39+ (environlib ExecVersion; a signed word compare, not a bug) GetRGB32
 * one colour at a time, before that the 4 bit values of the ColorTable
 * spread to 32 bits.  The BitMap is copied (64 bytes from the RastPort's
 * BitMap, Flags cleared), then saved as SaveBitmap with a palette.  Amiga
 * mode only.  Errors: as SaveBitmap; debugger only: "Uninitialized
 * Screen" (scrchk).
 * Not a bug in practice (#182): a ColorMap of 0 colours would run the
 * colour loops 65536 times; a screen always has colours.
 *
 * @param scr   a0: the screen object (!libs {#screenslib,$1080})
 * @param name  d1: the file name
 * @param gfx   a1: graphics.library
 * @param dos   a6: dos.library
 */
static void BLITZ_ROUTINE dosavescreen_c(struct Screen **scr BL_REG(a0), const char *name BL_REG(d1),
                                         struct GfxBase *gfx BL_REG(a1),
                                         struct DosLibrary *dos BL_REG(a6))
{
    struct Screen *s = *scr;
    struct ColorMap *cm = s->ViewPort.ColorMap;
    UWORD ncols = cm->Count;
    struct bl_regs r = { 0 };
    ULONG *rgb;
    const ULONG *bm;
    UWORD i;

    if (ncols > 256)
        ncols = 256;
    scrpal.num = ncols;
    BLITZ_ALIBJSR(TOK_execversion, r);              /* find aga flag */
    if ((WORD)r.d0 >= 39) {
        /* it's A G A */
        UWORD col = 0;
        struct GfxBase *GfxBase = gfx;

        rgb = scrcols + 1;
        do {
            GetRGB32(cm, col, 1, rgb);
            rgb += 3;
            col++;
        } while (col < ncols);
    } else {
        const UWORD *ctab = (const UWORD *)cm->ColorTable;

        rgb = scrcols + 1;
        ncols--;
        do {
            *rgb++ = makelong(*ctab >> 8);
            *rgb++ = makelong(*ctab >> 4);
            *rgb++ = makelong(*ctab++);
        } while (ncols-- != 0);
    }
    bm = (const ULONG *)s->RastPort.BitMap;     /* 64 bytes from it */
    for (i = 0; i < 16; i++)
        ((ULONG *)&bmcopy)[i] = bm[i];
    bmcopy.bm.Flags = 0;
    usewid = 0;
    savebm(&bmcopy.bm, name, &scrpal, dos);
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

/* a palette with colours: the colour pointer, and fixed (#182): a
   palette of 0 colours (InitPalette p,0) made makecolor write 65536 */
static void BLITZ_ERRCHK palchk_c(const struct palette *pal)
{
    if (!pal->cols || !pal->num)
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK bitchk2_c(UWORD *bm BL_REG(a0), const struct palette *pal BL_REG(a1))
{
    palchk_c(pal);
    bitchk_c(bm);
}

static void BLITZ_ERRCHK sschk1_c(UWORD *shape BL_REG(a0))
{
    if (!*shape)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK sschk2_c(UWORD *shape BL_REG(a0), const struct palette *pal BL_REG(a1))
{
    palchk_c(pal);
    sschk1_c(shape);
}

/* the thunks after all check functions: BLITZ_ERRCHK_THUNK ends with
   .text, and GCC would not switch back to the error check area for a
   check function behind it (see docs/development.md) */
BLITZ_ERRCHK_THUNK(scrchk, scrchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(bitchk, bitchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(bitchk2, bitchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(sschk1, sschk1_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(sschk2, sschk2_c, "d0-d1/a0-a1");
