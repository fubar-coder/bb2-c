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
 *  - ILBMInfo closes the file also when it could not be opened (dos
 *    Close of 0); the result is ilbminit's -1/0. (#186)
 *  - ilbmbitmap ignores a failed ilbminit and searches "BODY" in what is
 *    left in iffiolib's buffer (C: iffstuff as after the last ilbminit). (#187)
 *  - ilbmbitmap compares the plane number with the WORD at BitMap+4
 *    (Flags<<8 | Depth); reads width-in-words*2 bytes per row and plane,
 *    stores only what fits into BytesPerRow/Depth/Rows. (#188)
 *  - DecodeILBM (memory): chunk lengths are not padded to even (#189); the
 *    end test is `end <= pointer` after skipping a chunk; it writes ALL
 *    planes of the ILBM through the BitMap's plane pointers (more planes
 *    than the bitmap has: pointers behind Planes[], #190), ILBM row length
 *    per row, the
 *    BitMap's BytesPerRow as row step; ByteRun1 runs are not clipped at
 *    the row end (a run over the row end continues in the next plane row
 *    with a smaller count) (#191).  Uncompressed rows are copied as words.
 *  - Not a bug (word functions, the compiler uses only d0.w):
 *    ILBMWidth/Height/Depth load only the low word of d0 (thunks keep the
 *    caller's upper word); ILBMViewMode is a long (MOVEQ #0 first).
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

/* entry points (thunks / C, defined below) */
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
BLITZ_LIBS(LIB_bitmaplib, 0x1380);
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

/* ---------------------------------------------------------------- errors */

/* mess1 has no user (#187) and stays in the error check area; the others
   are in the normal code like the error routines (the original has them
   in the error check area; fixed, #192) */
BLITZ_ERRCHK_MESSAGE(mess1, "Can't Open File");
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

    if (!IFFJSR(TOK_iffopen, &value, name))             /* open iff */
        return BB_FALSE;
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
 * Errors: "Can't Find IFF 'BODY'" (the message in the normal code, fixed:
 * #192); "Can't Read File" from ilbmgetbyte.
 * BUG(orig): a failed ilbminit is ignored and "BODY" is searched in what is
 * left in iffiolib's buffer (#187); the depth test is on the word over
 * Flags and Depth (#188).
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

    ilbminit_(name);                                    /* result ignored */
    value = ID_BODY;
    if (!IFFJSR(TOK_ifffind, &value, st))
        BLITZ_RUNERR(mess3);
    for (;;) {
        PLANEPTR *planes = bm->Planes;
        UWORD plane = 0;                                /* bitplanes */

        do {
            UWORD count = (UWORD)(st->wwidth << 1) - 1; /* width */
            UWORD col = 0;
            UBYTE *dst = *planes++;

            do {
                value = getbyte(value, st);
                /* BUG(orig) (#188): the word over Flags and Depth */
                if (col < bm->BytesPerRow && plane < *(UWORD *)&bm->Flags)
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
 * BUG(orig) (#186): the file is closed also when it could not be opened
 * (dos Close of 0).
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
BLITZ_THUNK(doilbminfo, doilbminfo_c, "d1/a0-a1");

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

/* MOVE x(pc),d0: the upper word of d0 stays */
BLITZ_THUNK_POST(doilbmwidth, ilbmwidth_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK_POST(doilbmheight, ilbmheight_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK_POST(doilbmdepth, ilbmdepth_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

/**
 * @brief ILBMViewMode: the view mode of the last ILBMInfo (ilbminit):
 * $8000 HIRES, 4 LACE.
 * @return d0: the view mode (a long)
 */
static ULONG BLITZ_ROUTINE ilbmviewmode_c(void)
{
    return ilbmvmode;                                   /* MOVEQ #0:MOVE */
}
BLITZ_THUNK(doilbmviewmode, ilbmviewmode_c, "d1/a0-a1");

/* ------------------------------------------------------------ DecodeILBM */

/**
 * @brief DecodeILBM Bitmap#,MemoryLocation: decode an ILBM file in memory
 * into a bitmap.
 *
 * Finds BMHD and BODY in the FORM ILBM; ByteRun1 or uncompressed rows
 * (copied as words).  The bitmap object comes in a3 (!libs
 * {#bitmaplib,$1380}).
 * Errors: "Can't Find IFF 'BMHD'" (no FORM/ILBM/BMHD), "Can't Find IFF
 * 'BODY'" (the messages in the normal code, fixed: #192).
 * BUG(orig): chunk lengths are not padded to even (#189); all planes of
 * the ILBM are written through the BitMap's plane pointers (#190); the
 * ILBM's row length per row, the BitMap's BytesPerRow as the row step,
 * runs are not clipped at the row end (#191).
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
    LONG rowoff;                        /* a4 */

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
        src += len;
        if ((LONG)end <= (LONG)src)                     /* CMP.l a0,a1:BLE */
            BLITZ_RUNERR(mess4);
    }
    rowsize = ((const struct BitMapHeader *)src)->bmh_Width; /* fc */
    rows = ((const struct BitMapHeader *)src)->bmh_Height;
    rowsize = (UWORD)((UWORD)(rowsize + 15) >> 4);
    rowsize = (UWORD)(rowsize + rowsize);               /* bytes per row */
    depth = ((const struct BitMapHeader *)src)->bmh_Depth;  /* planes */
    comp = ((const struct BitMapHeader *)src)->bmh_Compression;
    src += len;
    for (;;) {                                          /* cb */
        id = *(const ULONG *)src;
        len = *(const ULONG *)(src + 4);
        src += 8;
        if (id == ID_BODY)
            break;
        src += len;
        if ((LONG)end <= (LONG)src)
            BLITZ_RUNERR(mess3);
    }
    depth--;                                            /* fb */
    rows--;
    rowoff = 0;
    if (comp) {
        do {                                            /* god */
            UWORD plane = depth;
            PLANEPTR *planes = bm->Planes;

            do {                                        /* gow */
                UBYTE *dst = *planes++ + rowoff;
                WORD left = (WORD)rowsize;

            gol:
                {
                    UWORD n = *src++;

                    if (n < 0x80) {                     /* copy */
                        left -= n;
                        do
                            *dst++ = *src++;
                        while (n-- != 0);
                    } else {
                        UBYTE fill;

                        n = (UBYTE)-(BYTE)n;
                        if (n == 0x80)                  /* $80 = nop: BMI gol */
                            goto gol;
                        left -= n;
                        fill = *src++;
                        do
                            *dst++ = fill;
                        while (n-- != 0);
                    }
                }
                if (--left > 0)                         /* incline */
                    goto gol;
            } while (plane-- != 0);                     /* DBRA d4 */
            rowoff += (WORD)bm->BytesPerRow;            /* ADD (a3),a4 */
        } while (rows-- != 0);                          /* DBRA d6 */
    } else {
        UWORD rowwords = (UWORD)(rowsize >> 1) - 1; /* d5: words - 1 */
        do {                                            /* cod */
            UWORD plane = depth;
            PLANEPTR *planes = bm->Planes;

            do {                                        /* cow */
                UWORD *dst = (UWORD *)(*planes++ + rowoff);
                UWORD left = rowwords;

                do
                    *dst++ = *(const UWORD *)src, src += 2;
                while (left-- != 0);
                /* keep GCC from computing the pointer with a multiply
                   (__mulsi3: no libgcc in a library) */
                __asm__("" : "+a"(src));
            } while (plane-- != 0);
            rowoff += (WORD)bm->BytesPerRow;
        } while (rows-- != 0);
    }
}
BLITZ_THUNK(_decodeilbm, decodeilbm_c, "d0-d1/a0-a1");
