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
 *    Close of 0); the result is ilbminit's -1/0.
 *  - ilbmbitmap ignores a failed ilbminit and searches "BODY" in what is
 *    left in iffiolib's buffer (C: iffstuff as after the last ilbminit).
 *  - ilbmbitmap compares the plane number with the WORD at BitMap+4
 *    (Flags<<8 | Depth); reads width-in-words*2 bytes per row and plane,
 *    stores only what fits into BytesPerRow/Depth/Rows.
 *  - DecodeILBM (memory): chunk lengths are not padded to even; the end
 *    test is `end <= pointer` after skipping a chunk; it writes ALL planes
 *    of the ILBM through the BitMap's plane pointers (more planes than the
 *    bitmap has: pointers behind Planes[]), ILBM row length per row, the
 *    BitMap's BytesPerRow as row step; ByteRun1 runs are not clipped at
 *    the row end (a run over the row end continues in the next plane row
 *    with a smaller count).  Uncompressed rows are copied as words.
 *  - The error routines are in the normal code, their messages in the
 *    error check area (as in the original: without the debugger d0 points
 *    behind the library).
 *  - ILBMWidth/Height/Depth load only the low word of d0 (thunks keep the
 *    caller's upper word); ILBMViewMode is a long (MOVEQ #0 first).
 *
 * Registers: ilbminit keeps d1-d7/a1-a6 (a0 = iffstuff, on failure the
 * file name - the original leaves dos.library's a0), ilbmgetbyte
 * everything except d0, ilbmbitmap everything except a0 (= iffstuff).
 */
#include "blitzlib.h"
#include <graphics/gfx.h>

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
   ok), d0 in *d0p */
#define IFFJSR(tok, d0p, a0v) BLITZ_ALIBJSR_CCR_D0A0(tok, "eq", d0p, a0v)

/* ------------------------------------------------------------- variables */

/* iffstuff: words 0 width, 2 height, 4 planes, 6 width in words,
   8 compression, 10 repeat, 12 copy */
UBYTE iffstuff[14] __attribute__((aligned(2)));
#define ST_W(st, off) (*(UWORD *)((st) + (off)))

static UWORD ilbmvmode;

/* ---------------------------------------------------------------- errors */

BLITZ_ERRCHK_MESSAGE(mess1, "Can't Open File");
BLITZ_ERRCHK_MESSAGE(mess2, "Can't Read File");
BLITZ_ERRCHK_MESSAGE(mess3, "Can't Find IFF 'BODY'");
BLITZ_ERRCHK_MESSAGE(mess4, "Can't Find IFF 'BMHD'");

/* -------------------------------------------------------------- ilbminit */

#define ID_BMHD 0x424d4844
#define ID_BODY 0x424f4459
#define ID_FORM 0x464f524d
#define ID_ILBM 0x494c424d

/* -1 = ok (iffstuff filled), 0 = no file / no BMHD */
static LONG ilbminit_(const char *name)
{
    UBYTE *st = iffstuff;
    ULONG d0 = 0;
    UWORD vmode = 0;

    if (!IFFJSR(TOK_iffopen, &d0, name))                /* open iff */
        return 0;
    d0 = ID_BMHD;
    if (!IFFJSR(TOK_ifffind, &d0, st))                  /* find BMHD */
        return 0;
    IFFJSR(TOK_iffword, &d0, st);                       /* read word */
    ST_W(st, 0) = (UWORD)d0;
    ST_W(st, 6) = (UWORD)((UWORD)d0 + 15) >> 4;
    IFFJSR(TOK_iffword, &d0, st);                       /* another word! */
    ST_W(st, 2) = (UWORD)d0;
    IFFJSR(TOK_ifflong, &d0, st);
    IFFJSR(TOK_iffbyte, &d0, st);
    st[5] = (UBYTE)d0;
    IFFJSR(TOK_iffbyte, &d0, st);
    IFFJSR(TOK_iffbyte, &d0, st);
    st[9] = (UBYTE)d0;
    d0 = 5;
    IFFJSR(TOK_iffskip, &d0, st);
    IFFJSR(TOK_iffword, &d0, st);
    if ((UWORD)d0 >= 480)
        vmode = 0x8000;
    IFFJSR(TOK_iffword, &d0, st);
    if ((UWORD)d0 >= 300)
        vmode |= 4;
    ilbmvmode = vmode;
    *(ULONG *)(st + 10) = 0;
    return -1;
}

/* the address left in a0 by the ilbminit thunk */
UBYTE *ilbm_a0;

static LONG BLITZ_ROUTINE ilbminit_c(const char *name BL_REG(a0))
{
    LONG r = ilbminit_(name);

    ilbm_a0 = r ? iffstuff : (UBYTE *)name;
    return r;
}
BLITZ_THUNK_POST(ilbminit, ilbminit_c, "d1/a1", "move.l _ilbm_a0,a0\n   tst.l d0");

/* ----------------------------------------------------------- ilbmgetbyte */

/* iffbyte, runtime error "Can't Read File" at the end of the data */
static ULONG getme(ULONG d0, UBYTE *st)
{
    if (!IFFJSR(TOK_iffbyte, &d0, st))
        BLITZ_RUNERR(mess2);                            /* BNE err2 */
    return d0;
}

/* next byte of the BODY; d0 = the previous byte (kept for repeats) */
static ULONG getbyte(ULONG d0, UBYTE *st)
{
    if (ST_W(st, 8) == 0)                               /* not compressed */
        return getme(d0, st);
    if (ST_W(st, 12) != 0) {                            /* copying */
        ST_W(st, 12)--;
        return getme(d0, st);
    }
    if (ST_W(st, 10) != 0) {                            /* repeating */
        ST_W(st, 10)--;
        return d0;
    }
    do                                                  /* control byte */
        d0 = getme(d0, st);
    while ((UBYTE)d0 == 0x80);
    if ((BYTE)d0 < 0)
        st[11] = (UBYTE)-(BYTE)d0;                      /* NEG.b: repeat */
    else
        st[13] = (UBYTE)d0;                             /* copy */
    return getme(d0, st);
}

static ULONG BLITZ_ROUTINE ilbmgetbyte_c(ULONG d0 BL_REG(d0), UBYTE *st BL_REG(a0))
{
    return getbyte(d0, st);
}
BLITZ_THUNK(ilbmgetbyte, ilbmgetbyte_c, "d1/a0-a1");

/* ------------------------------------------------------------ ilbmbitmap */

static void BLITZ_ROUTINE ilbmbitmap_c(ULONG d0 BL_REG(d0), const char *name BL_REG(a0),
                                       struct BitMap *bm BL_REG(a1))
{
    UBYTE *st = iffstuff;
    ULONG d5 = 0;                                       /* row offset */
    UWORD d1 = 0;                                       /* rows */

    ilbminit_(name);                                    /* result ignored */
    d0 = ID_BODY;
    if (!IFFJSR(TOK_ifffind, &d0, st))
        BLITZ_RUNERR(mess3);
    for (;;) {
        PLANEPTR *a2 = bm->Planes;
        UWORD d2 = 0;                                   /* bitplanes */

        do {
            UWORD d3 = (UWORD)(ST_W(st, 6) << 1) - 1;   /* width */
            UWORD d4 = 0;
            UBYTE *a4 = *a2++;

            do {
                d0 = getbyte(d0, st);
                if (d4 < *(UWORD *)bm && d2 < *(UWORD *)((UBYTE *)bm + 4))
                    *(a4++ + d5) = (UBYTE)d0;
                d4++;
            } while (d3-- != 0);                        /* DBF d3 */
            d2++;
        } while (d2 < ST_W(st, 4));
        d1++;
        if (d1 >= bm->Rows || d1 >= ST_W(st, 2))
            break;
        d5 += (LONG)(WORD)bm->BytesPerRow;              /* EXT.l: ADD.l */
    }
    {
        ULONG z = 0;
        IFFJSR(TOK_iffclose, &z, st);                   /* iff close */
    }
}
BLITZ_THUNK_POST(ilbmbitmap, ilbmbitmap_c, "d0-d1/a1", "lea _iffstuff,a0");

/* ------------------------------------------------------------- functions */

static LONG BLITZ_ROUTINE doilbminfo_c(const char *name BL_REG(d0))
{
    LONG r = ilbminit_(name);
    ULONG z = r;

    IFFJSR(TOK_iffclose, &z, iffstuff);                 /* close ? */
    return r;
}
BLITZ_THUNK(doilbminfo, doilbminfo_c, "d1/a0-a1");

static UWORD BLITZ_ROUTINE ilbmwidth_c(void) { return ST_W(iffstuff, 0); }
static UWORD BLITZ_ROUTINE ilbmheight_c(void) { return ST_W(iffstuff, 2); }
static UWORD BLITZ_ROUTINE ilbmdepth_c(void) { return ST_W(iffstuff, 4); }

/* MOVE x(pc),d0: the upper word of d0 stays */
BLITZ_THUNK_POST(doilbmwidth, ilbmwidth_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK_POST(doilbmheight, ilbmheight_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK_POST(doilbmdepth, ilbmdepth_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

static ULONG BLITZ_ROUTINE ilbmviewmode_c(void)
{
    return ilbmvmode;                                   /* MOVEQ #0:MOVE */
}
BLITZ_THUNK(doilbmviewmode, ilbmviewmode_c, "d1/a0-a1");

/* ------------------------------------------------------------ DecodeILBM */

/* d1 = memory location (FORM ILBM), a3 = the BitMap object */
static void BLITZ_ROUTINE decodeilbm_c(const ULONG *mem BL_REG(d1))
{
    struct BitMap *bm = (struct BitMap *)blitz_string;
    const UBYTE *a0 = (const UBYTE *)mem;
    const UBYTE *end;
    ULONG id, len;
    UWORD d5, d6, d7;
    UBYTE comp;
    LONG a4;

    id = *(const ULONG *)a0;
    len = *(const ULONG *)(a0 + 4);
    a0 += 8;
    if (id != ID_FORM)
        BLITZ_RUNERR(mess4);
    end = a0 + len;
    if (*(const ULONG *)a0 != ID_ILBM)
        BLITZ_RUNERR(mess4);
    a0 += 4;
    for (;;) {                                          /* cv */
        id = *(const ULONG *)a0;
        len = *(const ULONG *)(a0 + 4);
        a0 += 8;
        if (id == ID_BMHD)
            break;
        a0 += len;
        if ((LONG)end <= (LONG)a0)                      /* CMP.l a0,a1:BLE */
            BLITZ_RUNERR(mess4);
    }
    d5 = *(const UWORD *)a0;                            /* fc */
    d6 = *(const UWORD *)(a0 + 2);
    d5 = (UWORD)((UWORD)(d5 + 15) >> 4);
    d5 = (UWORD)(d5 + d5);                              /* bytes per row */
    d7 = a0[8];                                         /* planes */
    comp = a0[10];
    a0 += len;
    for (;;) {                                          /* cb */
        id = *(const ULONG *)a0;
        len = *(const ULONG *)(a0 + 4);
        a0 += 8;
        if (id == ID_BODY)
            break;
        a0 += len;
        if ((LONG)end <= (LONG)a0)
            BLITZ_RUNERR(mess3);
    }
    d7--;                                               /* fb */
    d6--;
    a4 = 0;
    if (comp) {
        do {                                            /* god */
            UWORD d4 = d7;
            PLANEPTR *a2 = bm->Planes;

            do {                                        /* gow */
                UBYTE *a1 = *a2++ + a4;
                WORD d3 = (WORD)d5;

            gol:
                {
                    UWORD n = *a0++;

                    if (n < 0x80) {                     /* copy */
                        d3 -= n;
                        do
                            *a1++ = *a0++;
                        while (n-- != 0);
                    } else {
                        UBYTE b;

                        n = (UBYTE)-(BYTE)n;
                        if (n == 0x80)                  /* $80 = nop: BMI gol */
                            goto gol;
                        d3 -= n;
                        b = *a0++;
                        do
                            *a1++ = b;
                        while (n-- != 0);
                    }
                }
                if (--d3 > 0)                           /* incline */
                    goto gol;
            } while (d4-- != 0);                        /* DBRA d4 */
            a4 += (WORD)bm->BytesPerRow;                /* ADD (a3),a4 */
        } while (d6-- != 0);                            /* DBRA d6 */
    } else {
        d5 = (UWORD)(d5 >> 1) - 1;
        do {                                            /* cod */
            UWORD d4 = d7;
            PLANEPTR *a2 = bm->Planes;

            do {                                        /* cow */
                UWORD *a1 = (UWORD *)(*a2++ + a4);
                UWORD d3 = d5;

                do
                    *a1++ = *(const UWORD *)a0, a0 += 2;
                while (d3-- != 0);
                /* keep GCC from computing the pointer with a multiply
                   (__mulsi3: no libgcc in a library) */
                __asm__("" : "+a"(a0));
            } while (d4-- != 0);
            a4 += (WORD)bm->BytesPerRow;
        } while (d6-- != 0);
    }
}
BLITZ_THUNK(_decodeilbm, decodeilbm_c, "d0-d1/a0-a1");
