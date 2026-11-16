/*
 * ilbmifflib - command library 161: ILBMInfo ILBMWidth ILBMHeight
 * ILBMDepth ILBMViewMode DecodeILBM, and three routines for other
 * libraries:
 *      +1 ilbminit     a0 = file name -> d0 = -1 (ok) / 0, a0 = iffstuff
 *                      (animlib, lib 154)
 *      +2 ilbmgetbyte  a0 = iffstuff -> d0.b = next BODY byte, ByteRun1
 *                      decoded (lib 154)
 *      +3 ilbmbitmap   a0 = file name, a1 = BitMap: load the ILBM
 *                      (libs 158, 160)
 *
 * C translation of bblibsrc/ilbmifflib; binary of the CD's acidlibs
 * checked: same as the source.  Files are read with iffiolib (ALibJsr
 * #iffiolib LSL 7+n); the CCR results of iffiolib are read with the
 * local IFFJSR = BLITZ_ALIBJSR_CCR_D0A0 (Z flag after the call).
 *
 * iffstuff (14 bytes): width (pixels), height, planes, width in words,
 * compression, repeat count, copy count.  ilbminit reads the BMHD: width,
 * height, skips x/y, nPlanes -> low byte of +4, masking skipped,
 * compression -> low byte of +8, skips 5 bytes, pageWidth >= 480 -> view
 * mode $8000 (HIRES), pageHeight >= 300 -> |4 (LACE); clears the counts.
 * The file stays open (ILBMInfo closes it).
 *
 * BUG(orig), kept:
 *  - Not a bug: ilbmbitmap reads width-in-words*2 bytes per row and
 *    plane and stores only what fits into BytesPerRow/Depth/Rows.
 *  - Not a bug: DecodeILBM (memory): the end test is `end <= pointer`
 *    after skipping a chunk.  Uncompressed rows are copied as words.
 *  - Not a bug (word functions, the compiler uses only d0.w):
 *    ILBMWidth/Height/Depth load only the low word of d0 (here the
 *    upper word is undefined, the register rule); ILBMViewMode is a long
 *    (MOVEQ #0 first).
 *
 * Fixed (intended deviation, #186): ILBMInfo calls iffclose also when the
 * file could not be opened; iffclose (iffiolib) now closes only an open
 * file, the original passed the handle 0 to dos Close.
 *
 * Fixed (intended deviation, #190, #191): DecodeILBM stores only the
 * planes, rows and bytes per row that both the ILBM and the bitmap have,
 * and cuts a ByteRun1 run at the end of the row; the rest of the ILBM's
 * data is decoded and dropped.  The original wrote all planes of the ILBM
 * through the bitmap's plane pointers (more planes than the bitmap has:
 * the pointers behind its depth or behind Planes[]), all its rows, its
 * row length per row, and a run over the row end into the next plane row
 * (with a smaller count for that row).
 *
 * Fixed (intended deviation, #189): DecodeILBM skips the IFF pad byte of
 * a chunk of odd length when it walks the chunks; the original added the
 * length as it is, so after a chunk of odd length BMHD/BODY were not
 * found or read at the wrong place.  A byte other than 0 in the place of
 * the pad byte is taken as the start of the next chunk: the files the
 * original SaveBitmap wrote with an odd CMAP (#184) stay readable.
 *
 * Fixed (intended deviation, #187): ilbmbitmap stops when ilbminit
 * fails: "Can't Open File" (the original's unused mess1) for a file that
 * cannot be opened, "Can't Find IFF 'BMHD'" (the file closed first) for
 * one without a BMHD; the original ignored the result and searched "BODY"
 * in what was left in iffiolib's buffer, with a0 from dos.library as
 * iffstuff (C: iffstuff as after the last ilbminit).
 *
 * Fixed (intended deviation, #1053): ilbmbitmap closes the file before
 * "Can't Find IFF 'BODY'"; the original left it open (iffiolib has no
 * finit: open until reboot).
 *
 * Fixed (intended deviation, #188): ilbmbitmap stores a plane while its
 * number is below the byte Depth (BitMap+5); the original compared with
 * the WORD at BitMap+4 (Flags<<8 | Depth), so into a bitmap whose Flags
 * byte is not 0 it wrote the ILBM's further planes through the plane
 * pointers behind its depth.
 *
 * Fixed (intended deviation, docs/libs/deviations.md): the original has the
 * error routines in the normal code but their messages in the error check
 * area (without the debugger d0 points behind the library); here the
 * messages are in the normal code too. (#192)
 *
 * Registers: ilbminit keeps d1-d7/a1-a6 (a0 = iffstuff, on failure the
 * file name - the original leaves dos.library's a0), ilbmgetbyte
 * everything except d0, ilbmbitmap everything except a0 (= iffstuff).
 */
#include "blitzlib.h"
#include <graphics/gfx.h>
#include <datatypes/pictureclass.h>    /* struct BitMapHeader */

/* entry points (aliases / thunks / C, defined below) */
void ilbminit(void), ilbmgetbyte(void), ilbmbitmap(void), doilbminfo(void);
void doilbmwidth(void), doilbmheight(void), doilbmdepth(void);
void doilbmviewmode(void);
void _decodeilbm(void);

/* !libheader{#ilbmifflib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_ilbmifflib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_NULLSUB(ilbminit, 0, 0);
BLITZ_NULLSUB(ilbmgetbyte, 0, 0);
BLITZ_NULLSUB(ilbmbitmap, 0, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(doilbminfo, 0, 0);
BLITZ_NAME("ILBMInfo", "Filename$");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doilbmwidth, 0, 0);
BLITZ_NAME("ILBMWidth", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doilbmheight, 0, 0);
BLITZ_NAME("ILBMHeight", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doilbmdepth, 0, 0);
BLITZ_NAME("ILBMDepth", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doilbmviewmode, 0, 0);
BLITZ_NAME("ILBMViewMode", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, BL_IA3|BL_PD0);
BLITZ_SUBS(_decodeilbm, 0, 0);
BLITZ_NAME("DecodeILBM", "Bitmap#,MemoryLocation");

BLITZ_LIBFIN();

/* ------------------------------------------------------------ iffiolib */


/* ALibJsr tok with d0/a0; returns the Z flag after the call (iffiolib's
   ok), d0 in *valuep, a0 = arg */
#define IFFJSR(tok, valuep, arg) BLITZ_ALIBJSR_CCR_D0A0(tok, "eq", valuep, arg)

/* ------------------------------------------------------------- variables */

/* iffstuff (the original's 14 bytes; other libraries read it through a0) */
struct iffstuff {
    UWORD width;                /* 0 pixels */
    UWORD height;               /* 2 */
    UWORD planes;               /* 4 */
    UWORD wwidth;               /* 6 width in words */
    UWORD comp;                 /* 8 compression */
    UWORD repeat;               /* 10 repeat count */
    UWORD copy;                 /* 12 copy count */
} iffstuff;
static_assert(sizeof iffstuff == 14);
/* the low byte of a word (MOVE.b to the label+1) */
#define LOBYTE(w) (((UBYTE *)&(w))[1])

static UWORD ilbmvmode;
static UBYTE ilbmnofile;        /* the last ilbminit could not open the file */

/* ---------------------------------------------------------------- errors */

/* the messages are in the normal code like the error routines (the
   original has them in the error check area; fixed, #192); mess1 has a
   user since #187 (ilbmbitmap) */
static const char mess1[] = "Can't Open File";
static const char mess2[] = "Can't Read File";
static const char mess3[] = "Can't Find IFF 'BODY'";
static const char mess4[] = "Can't Find IFF 'BMHD'";

/* -------------------------------------------------------------- ilbminit */

/* ID_BMHD, ID_BODY, ID_FORM, ID_ILBM: <datatypes/pictureclass.h> */

/* -1 = ok (iffstuff filled), 0 = no file / no BMHD */
static BB_BOOL ilbminit_(const char *name)
{
    struct iffstuff *st = &iffstuff;
    ULONG value = 0;
    UWORD vmode = 0;

    ilbmnofile = 1;
    if (!IFFJSR(TOK_iffopen, &value, name))             /* open iff */
        return BB_FALSE;
    ilbmnofile = 0;
    value = ID_BMHD;
    if (!IFFJSR(TOK_ifffind, &value, st))               /* find BMHD */
        return BB_FALSE;
    IFFJSR(TOK_iffword, &value, st);                    /* read word */
    st->width = (UWORD)value;
    st->wwidth = (UWORD)((UWORD)value + 15) >> 4;
    IFFJSR(TOK_iffword, &value, st);                    /* another word! */
    st->height = (UWORD)value;
    IFFJSR(TOK_ifflong, &value, st);
    IFFJSR(TOK_iffbyte, &value, st);
    LOBYTE(st->planes) = (UBYTE)value;
    IFFJSR(TOK_iffbyte, &value, st);
    IFFJSR(TOK_iffbyte, &value, st);
    LOBYTE(st->comp) = (UBYTE)value;
    value = 5;
    IFFJSR(TOK_iffskip, &value, st);
    IFFJSR(TOK_iffword, &value, st);
    if ((UWORD)value >= 480)
        vmode = 0x8000;
    IFFJSR(TOK_iffword, &value, st);
    if ((UWORD)value >= 300)
        vmode |= 4;
    ilbmvmode = vmode;
    *(ULONG *)&st->repeat = 0;                        /* repeat and copy */
    return BB_TRUE;
}

/* the address left in a0 by the ilbminit thunk */
UBYTE *ilbm_a0;

/**
 * @brief ilbminit (+1): open an ILBM file and read its BMHD into iffstuff.
 *
 * For animlib and lib 154.  The file stays open.  CCR: tst.l d0 (Z =
 * failed).
 *
 * @param name  a0: the file name
 * @return d0: BB_TRUE ok, BB_FALSE no file / no BMHD; a0 = iffstuff, on
 *         failure the file name (the original leaves dos.library's a0)
 */
static BB_BOOL BLITZ_ROUTINE ilbminit_c(const char *name BL_REG(a0))
{
    BB_BOOL r = ilbminit_(name);

    ilbm_a0 = r ? (UBYTE *)&iffstuff : (UBYTE *)name;
    return r;
}
BLITZ_THUNK_POST(ilbminit, ilbminit_c, "d1/a1", "move.l _ilbm_a0,a0\n   tst.l d0");

/* ----------------------------------------------------------- ilbmgetbyte */

/* iffbyte, runtime error "Can't Read File" at the end of the data */
static ULONG getme(ULONG byte, struct iffstuff *st)
{
    if (!IFFJSR(TOK_iffbyte, &byte, st))
        BLITZ_RUNERR(mess2);                            /* BNE err2 */
    return byte;
}

/* next byte of the BODY; d0 = the previous byte (kept for repeats) */
static ULONG getbyte(ULONG byte, struct iffstuff *st)
{
    if (st->comp == 0)                                   /* not compressed */
        return getme(byte, st);
    if (st->copy != 0) {                                /* copying */
        st->copy--;
        return getme(byte, st);
    }
    if (st->repeat != 0) {                              /* repeating */
        st->repeat--;
        return byte;
    }
    do                                                  /* control byte */
        byte = getme(byte, st);
    while ((UBYTE)byte == 0x80);
    if ((BYTE)byte < 0)
        LOBYTE(st->repeat) = (UBYTE)-(BYTE)byte;        /* NEG.b: repeat */
    else
        LOBYTE(st->copy) = (UBYTE)byte;                 /* copy */
    return getme(byte, st);
}

/**
 * @brief ilbmgetbyte (+2): the next byte of the BODY, ByteRun1 decoded
 * (for lib 154).
 *
 * Errors: "Can't Read File" at the end of the data (the message in the
 * normal code, fixed: #192).
 *
 * @param byte  d0: the previous byte (returned again inside a repeat run)
 * @param st    a0: iffstuff (from ilbminit)
 * @return d0: the byte in the low byte
 */
static ULONG BLITZ_ROUTINE ilbmgetbyte_c(ULONG byte BL_REG(d0), struct iffstuff *st BL_REG(a0))
{
    return getbyte(byte, st);
}
BLITZ_THUNK(ilbmgetbyte, ilbmgetbyte_c, "d1/a0-a1");

/* ------------------------------------------------------------ ilbmbitmap */

/**
 * @brief ilbmbitmap (+3): load an ILBM file into a BitMap (for libs 158,
 * 160).
 *
 * Reads row after row, plane after plane, width-in-words*2 bytes each,
 * and stores what fits into BytesPerRow/Depth/Rows; closes the file.
 * Errors: "Can't Open File", "Can't Find IFF 'BMHD'" (fixed, #187: the
 * original ignored a failed ilbminit and searched "BODY" in what was left
 * in iffiolib's buffer), "Can't Find IFF 'BODY'" (the messages in the
 * normal code, fixed: #192; the file closed first, fixed: #1053, the
 * original left it open); "Can't Read File" from ilbmgetbyte.
 * Fixed (#188): the depth test is on the byte Depth (the original: the
 * word over Flags and Depth).
 *
 * @param value  d0: the start value of the byte reader
 * @param name   a0: the file name
 * @param bm     a1: the BitMap; a0 = iffstuff afterwards
 */
static void BLITZ_ROUTINE ilbmbitmap_c(ULONG value BL_REG(d0), const char *name BL_REG(a0),
                                       struct BitMap *bm BL_REG(a1))
{
    struct iffstuff *st = &iffstuff;
    ULONG rowoff = 0;                                   /* row offset */
    UWORD row = 0;                                      /* rows */
    UWORD depth = bm->Depth;                            /* fixed (#188): the byte */

    if (!ilbminit_(name)) {                             /* fixed (#187) */
        ULONG zero = 0;

        if (ilbmnofile)
            BLITZ_RUNERR(mess1);
        IFFJSR(TOK_iffclose, &zero, st);
        BLITZ_RUNERR(mess4);
    }
    value = ID_BODY;
    if (!IFFJSR(TOK_ifffind, &value, st)) {
        ULONG zero = 0;

        IFFJSR(TOK_iffclose, &zero, st);                /* Fixed (#1053) */
        BLITZ_RUNERR(mess3);
    }
    for (;;) {
        PLANEPTR *planes = bm->Planes;
        UWORD plane = 0;                                /* bitplanes */

        do {
            UWORD count = (UWORD)(st->wwidth << 1) - 1; /* width */
            UWORD col = 0;
            UBYTE *dst = *planes++;

            do {
                value = getbyte(value, st);
                if (col < bm->BytesPerRow && plane < depth)
                    *(dst++ + rowoff) = (UBYTE)value;
                col++;
            } while (count-- != 0);                     /* DBF d3 */
            plane++;
        } while (plane < st->planes);
        row++;
        if (row >= bm->Rows || row >= st->height)
            break;
        rowoff += (LONG)(WORD)bm->BytesPerRow;          /* EXT.l: ADD.l */
    }
    {
        ULONG zero = 0;
        IFFJSR(TOK_iffclose, &zero, st);                /* iff close */
    }
}
BLITZ_THUNK_POST(ilbmbitmap, ilbmbitmap_c, "d0-d1/a1", "lea _iffstuff,a0");

/* ------------------------------------------------------------- functions */

/**
 * @brief ILBMInfo Filename$: read the size, depth and view mode of an ILBM
 * file (for ILBMWidth, ILBMHeight, ILBMDepth, ILBMViewMode); also a
 * function.
 *
 * View mode: pageWidth >= 480 gives $8000 (HIRES), pageHeight >= 300 |4
 * (LACE).
 * Fixed (#186): iffclose closes only an open file (the original: dos
 * Close of 0 for a file that could not be opened).
 *
 * @param name  d0: the file name
 * @return d0: BB_TRUE ok, BB_FALSE no file / no BMHD
 */
static BB_BOOL BLITZ_ROUTINE doilbminfo_c(const char *name BL_REG(d0))
{
    BB_BOOL r = ilbminit_(name);
    ULONG value = r;

    IFFJSR(TOK_iffclose, &value, &iffstuff);            /* close ? */
    return r;
}
BLITZ_ALIAS(doilbminfo, doilbminfo_c);

/**
 * @brief ILBMWidth: the width in pixels of the last ILBMInfo (ilbminit).
 * @return d0.w: the width (the upper word of d0 stays, not a bug: a word
 *         function)
 */
static UWORD BLITZ_ROUTINE ilbmwidth_c(void) { return iffstuff.width; }
/**
 * @brief ILBMHeight: the height of the last ILBMInfo (ilbminit).
 * @return d0.w: the height (the upper word of d0 stays)
 */
static UWORD BLITZ_ROUTINE ilbmheight_c(void) { return iffstuff.height; }
/**
 * @brief ILBMDepth: the number of planes of the last ILBMInfo (ilbminit).
 * @return d0.w: the depth (the upper word of d0 stays)
 */
static UWORD BLITZ_ROUTINE ilbmdepth_c(void) { return iffstuff.planes; }

/* MOVE x(pc),d0 in the original: the upper word of d0 is undefined */
BLITZ_ALIAS(doilbmwidth, ilbmwidth_c);
BLITZ_ALIAS(doilbmheight, ilbmheight_c);
BLITZ_ALIAS(doilbmdepth, ilbmdepth_c);

/**
 * @brief ILBMViewMode: the view mode of the last ILBMInfo (ilbminit):
 * $8000 HIRES, 4 LACE.
 * @return d0: the view mode (a long)
 */
static ULONG BLITZ_ROUTINE ilbmviewmode_c(void)
{
    return ilbmvmode;                                   /* MOVEQ #0:MOVE */
}
BLITZ_ALIAS(doilbmviewmode, ilbmviewmode_c);

/* ------------------------------------------------------------ DecodeILBM */

/* the length of a chunk with its pad byte (fixed, #189).  A writer that
   left the pad byte out - the original SaveBitmap/SaveShape/SaveScreen
   with an odd number of colours (#184) - is recognised by a byte that is
   not 0 where the pad byte should be: then the next chunk starts there */
static ULONG padded(const UBYTE *data, ULONG len)
{
    if ((len & 1) && data[len] == 0)
        len++;
    return len;
}

/* one plane row of a ByteRun1 BODY: decodes `left` bytes (the ILBM's
   row) and stores the first `keep` of them at dst (fixed, #190, #191: a
   run is cut at the end of the row and at `keep`, the rest dropped);
   $80 is a no-op.  Returns the source behind the row. */
static const UBYTE *unpackrow(const UBYTE *src, UBYTE *dst, LONG left, UWORD keep)
{
    while (left > 0) {
        UWORD n = *src++;
        UWORD put;

        if (n < 0x80) {                                 /* copy n+1 */
            n++;
            left -= n;
            put = n < keep ? n : keep;
            keep -= put;
            n -= put;
            for (; put != 0; put--)
                *dst++ = *src++;
            src += n;
        } else if (n != 0x80) {                         /* repeat 257-n */
            UBYTE fill = *src++;

            n = 257 - n;
            left -= n;
            put = n < keep ? n : keep;
            keep -= put;
            for (; put != 0; put--)
                *dst++ = fill;
        }
    }
    return src;
}

/* one plane row of an uncompressed BODY: `len` bytes (even), the first
   `keep` of them stored at dst (fixed, #190), copied as words as the
   original does */
static const UBYTE *copyrow(const UBYTE *src, UBYTE *dst, UWORD len, UWORD keep)
{
    const UBYTE *next = src + len;
    UWORD w;

    for (w = keep >> 1; w != 0; w--) {
        *(UWORD *)dst = *(const UWORD *)src;
        dst += 2;
        src += 2;
    }
    if (keep & 1)
        *dst = *src;
    return next;
}

/**
 * @brief DecodeILBM Bitmap#,MemoryLocation: decode an ILBM file in memory
 * into a bitmap.
 *
 * Finds BMHD and BODY in the FORM ILBM; ByteRun1 or uncompressed rows
 * (copied as words).  The bitmap object comes in a3 (!libs
 * {#bitmaplib,$1380}).
 * Errors: "Can't Find IFF 'BMHD'" (no FORM/ILBM/BMHD), "Can't Find IFF
 * 'BODY'" (the messages in the normal code, fixed: #192).
 * Fixed (#189): the pad byte of an odd chunk is skipped (unless it is
 * not 0: a writer without pad bytes, the original SaveBitmap).
 * Fixed (#190, #191): only the planes, rows and bytes per row that both
 * the ILBM and the bitmap have are stored, a run is cut at the row end;
 * the rest is decoded and dropped (the original wrote all planes, rows
 * and bytes of the ILBM, and a run over the row end into the next row).
 *
 * @param mem  d1: the address of the FORM ILBM
 */
static void BLITZ_ROUTINE decodeilbm_c(const ULONG *mem BL_REG(d1))
{
    struct BitMap *bm = (struct BitMap *)blitz_string;
    const UBYTE *src = (const UBYTE *)mem;
    const UBYTE *end;
    ULONG id, len;
    UWORD rowsize, rows, depth;         /* d5: the bytes per row; d6 d7 */
    UBYTE comp;

    id = *(const ULONG *)src;
    len = *(const ULONG *)(src + 4);
    src += 8;
    if (id != ID_FORM)
        BLITZ_RUNERR(mess4);
    end = src + len;
    if (*(const ULONG *)src != ID_ILBM)
        BLITZ_RUNERR(mess4);
    src += 4;
    for (;;) {                                          /* cv */
        id = *(const ULONG *)src;
        len = *(const ULONG *)(src + 4);
        src += 8;
        if (id == ID_BMHD)
            break;
        src += padded(src, len);                        /* fixed (#189) */
        if ((LONG)end <= (LONG)src)                     /* CMP.l a0,a1:BLE */
            BLITZ_RUNERR(mess4);
    }
    rowsize = ((const struct BitMapHeader *)src)->bmh_Width; /* fc */
    rows = ((const struct BitMapHeader *)src)->bmh_Height;
    rowsize = (UWORD)((UWORD)(rowsize + 15) >> 4);
    rowsize = (UWORD)(rowsize + rowsize);               /* bytes per row */
    depth = ((const struct BitMapHeader *)src)->bmh_Depth;  /* planes */
    comp = ((const struct BitMapHeader *)src)->bmh_Compression;
    src += padded(src, len);                            /* fixed (#189) */
    for (;;) {                                          /* cb */
        id = *(const ULONG *)src;
        len = *(const ULONG *)(src + 4);
        src += 8;
        if (id == ID_BODY)
            break;
        src += padded(src, len);                        /* fixed (#189) */
        if ((LONG)end <= (LONG)src)
            BLITZ_RUNERR(mess3);
    }
    /* fb - fixed (#190, #191): the planes, rows and bytes per row that
       both the ILBM and the bitmap have are stored, the rest of the
       ILBM's data is decoded and dropped */
    {
        UWORD planes = depth < bm->Depth ? depth : bm->Depth;
        UWORD keep = rowsize < bm->BytesPerRow ? rowsize : bm->BytesPerRow;
        ULONG step = bm->BytesPerRow;
        ULONG rowoff = 0;

        if (rows > bm->Rows)
            rows = bm->Rows;
        for (; rows != 0; rows--) {
            PLANEPTR *pl = bm->Planes;
            UWORD plane;

            for (plane = 0; plane < depth; plane++) {
                UBYTE *dst = 0;
                UWORD k = 0;

                if (plane < planes) {
                    dst = *pl++ + rowoff;
                    k = keep;
                }
                if (comp)
                    src = unpackrow(src, dst, rowsize, k);
                else
                    src = copyrow(src, dst, rowsize, k);
            }
            rowoff += step;
        }
    }
}
BLITZ_ALIAS(_decodeilbm, decodeilbm_c);
