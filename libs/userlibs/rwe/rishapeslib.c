/*
 * rishapeslib - RWE userlib 46 (Reflective Images): shapes in memory and
 * the "LE" shapes file format.
 *   CludgeShapes shape#,numshapes,address    (shapes in memory as shapes)
 *   CludgeShapesToAddress address,numshapes,address
 *   SafeToBlit(shape,x,y)
 *   LELoadShapes shape#[,shape#],filename$[,palette#]
 *   LESaveShapes shape#,shape#,filename$[,palette#]
 *   LECompressShapes True|False   LECludgeShapes shape#,shape#,address,
 *   palette#[,copymemflag]   LEInitShape shape#,width,height,depth
 *   LEScanShapes start,end   LEMaxShapeWidth/Height/Depth
 *   LEAutoCookie on|off
 *
 * C translation of rwelibsrc/shapeslib/RIShapesLib2.bb (31/8/97, BB22
 * format); reference: the reference build of that source with blitz2 2.10
 * (REFERENCE refbuild, ref/rishapeslib.obj) - the CD binaries are an
 * older state (docs/rwelibs/README.md 3.2).  Notes:
 * docs/rwelibs/port/rishapeslib.md.
 *
 * Shape objects (shapeslib, 32 bytes): +0.w width, +2.w height, +4.w
 * depth, +6.w bytes per row (even), +8.w blit size, +10.w/+12.w handles,
 * +14.l data, +18.l cookie, +22.w bytes of one plane, +24.w the same +
 * 2*height, +26.w bytes of all planes, +28.w, +30.w "cL" = the data is
 * not the shape's own (cludged into memory).  Palette objects
 * (palettelib): +0.l data (+0.w colours, +4 12 bytes per colour), +4.w
 * colours.
 *
 * The LE file: "FORM" len "LESH" "SHHD" 20 {count.l, compression.w, 14
 * zero bytes}, optional "CMAP" len {r,g,b bytes}, "SDAT" len {per shape:
 * width.w height.w depth.w xhandle.w yhandle.w size.w, then size bytes
 * (or packedsize.l and ByteRun1 data of RIPackLib's PackPBM; -1 = stored)}.
 *
 * Calls into other libraries (ALibJsr): memlib globalloc/globfree,
 * RIPackLib (73) PackPBM/UnPackPBM, shapeslib LoadShapes form 1 (for a
 * file that is no LE file).  The routines are C behind thunks; the
 * register arithmetic of the original (word operations, MULU.w, DBRA on
 * words, the flags a MOVEA does not set) is reproduced.  Assembler: the
 * call of shapeslib's LoadShapes (a2/a3/a6 as the original leaves them),
 * the flags of dos Write (see savecmap), and the original's stack
 * imbalances (below).
 *
 * BUG(orig), kept:
 *   - SafeToBlit, LELoadShapes and LESaveShapes shift the shape NUMBERS
 *     right by 5 as if they were object offsets: SafeToBlit compares
 *     shape/32 with the maximum (#822), LESaveShapes stores
 *     (s2>>5)-(s1>>5) (the upper word of the register uncleared) as the
 *     shape count, and the shapes passed on to shapeslib's LoadShapes are
 *     s>>5.  The loader reads count+1 shapes, so a saved file of shapes
 *     below 32 loads one shape (#815).
 *   - CludgeShapes: an empty shape in memory (width or height 0) advances
 *     by 32 + the low word of the size of the shape before (also of an
 *     earlier call: the variable stays) (#809); a shape with size 0
 *     copies 65536 bytes (#808).
 *   - The cookie of a shape that is freed by LELoadShapes, LECludgeShapes
 *     or LEInitShape is never freed (the BEQ after a MOVEA tests the CLR
 *     before it, always taken) (#810).
 *   - LELoadShapes/LECludgeShapes on a shape that is cludged ("cL") pops
 *     a0 it never pushed: the shape data is then read from the return
 *     address into the library's code, and the routine returns to its
 *     caller's caller (LELoadShapes then neither frees its buffers nor
 *     closes the file); a second such shape returns into the Blitz
 *     program's stack (#807).  An allocation that fails in the shape loop
 *     or in the cookie returns with a0 still pushed: it jumps to the data
 *     address (#806; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!").  Stack imbalances that C cannot express: see rs_pop
 *     and rs_crash.
 *   - LELoadShapes form 3 (shape#,filename$,palette#) takes the palette of
 *     d3, not of the third argument (!libs #pd3) (#812).  LELoadShapes
 *     hands a non-LE file to shapeslib's LoadShapes with the shapes
 *     exchanged (a3 = the last shape, a2 = the first) (#813).  A failing
 *     chunk allocation frees the header buffer and closes the file twice
 *     (readchunk jumps to the cleanup of its caller and returns there)
 *     (#819; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!").
 *   - read_palette: no check of the allocation (writes to address 0)
 *     (#821; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!"), the colour count of DIVU #3 (overflow: unchanged), 0 colours
 *     decode 65536; sizes are word additions (ADDQ #8) (#820).
 *   - LESaveShapes: whether the palette is written depends on the flags
 *     dos Write leaves (MOVE.l paletteptr,a0 is a MOVEA, the BNE tests
 *     Write's flags) (#814); the FORM length is 8 too large and has the
 *     CMAP size of the last save with a palette (#816); the shape header
 *     buffer is allocated with the upper word of d0 of _saveCMAP (#817); a
 *     palette of 0 colours copies 65536 (#823); a failing pack buffer is a
 *     runtime error (TRAP #0) (#818); PackPBM's failure path runs
 *     UnPackPBM with d3 = 12 (#772).
 *   - LECludgeShapes skips the SHHD chunk as 20 bytes (#811).
 *
 * Original behaviour that is no defect: CludgeShapes shifts shape# as well
 * (unused since the own cookie code).  LECludgeShapes links the shapes
 * into the memory (no copy, "cL") if it is below 2 MB and the file
 * uncompressed, unless copymemflag is negative (intended).
 */
#include "blitzlib.h"
#include <stddef.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dosbase
#include <proto/dos.h>
#undef __NOLIBBASE__

/* RIPackLib (73): PackPBM index 0, UnPackPBM index 7 (ripacklib.c);
   shapeslib LoadShapes index 9, form 1 (shape#,shape#,filename$) */
#define TOK_packpbm ((LIB_ripacklib << 7) + 1)
#define TOK_unpackpbm ((LIB_ripacklib << 7) + 8)
#define TOK_loadshapes ((LIB_shapeslib << 7) + 10)

/* entry points (thunks, defined below) */
void _loadshapes(void), _loadshapestoaddress(void), _safetoblit(void);
void _leloadshapes(void), _leloadshapesnop(void), _leloadshapesinglenop(void);
void _leloadshapesingle(void), _lesaveshapesnop(void), _lesaveshapes(void);
void _setcompress(void), _lecludgeshapes(void), _lecludgeshapes2(void);
void _leinitshape(void), _lescanshapes(void), _leMaxShapeWidth(void);
void _leMaxShapeHeight(void), _leMaxShapeDepth(void), _leautocookie(void);
void _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))   /* #ia<a>|#pd<p> */

/* !libheader {#rishapeslib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rishapeslib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_shapeslib, IA(0, 0), LIB_execlib, BL_LA(6));
BLITZ_SUBS(_loadshapes, 0, 0);
BLITZ_NAME("CludgeShapes", "shape#,numshapes,address");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(3, 0), LIB_bitmaplib, BL_LA(2) | BL_USED,
           LIB_shapeslib, BL_LD(6) | BL_MAX);
BLITZ_SUBS(_safetoblit, 0, 0);
BLITZ_NAME("SafeToBlit", "shape,x,y");

/* the !libs of the source write `pd1`/`pd3`/`la6` without # (= 1, 3 and
   the flags of #la6, as built) */
BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(2, 0), LIB_shapeslib, IA(3, 1), LIB_palettelib, IA(0, 3),
           LIB_doslib, BL_LA(6));
BLITZ_SUBS(_leloadshapes, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, IA(2, 0), LIB_shapeslib, IA(3, 1), LIB_doslib, BL_LA(6));
BLITZ_SUBS(_leloadshapesnop, 0, 0);
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, IA(2, 0), LIB_shapeslib, BL_LD(4) | BL_MAX,
           LIB_shapeslib, BL_LA(3) | BL_BASE, LIB_doslib, BL_LA(6));
BLITZ_SUBS(_leloadshapesinglenop, 0, 0);
/* "This little bastard won't work below": the palette of d3 */
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(2, 0), LIB_palettelib, IA(0, 3), LIB_shapeslib,
           BL_LD(4) | BL_MAX, LIB_shapeslib, BL_LA(3) | BL_BASE, LIB_doslib, BL_LA(6));
BLITZ_SUBS(_leloadshapesingle, 0, 0);
BLITZ_NAME("LELoadShapes", "shape#[,shape#],filename$[,palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, IA(2, 0), LIB_shapeslib, IA(3, 1), LIB_doslib, BL_LA(6));
BLITZ_SUBS(_lesaveshapesnop, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(2, 0), LIB_shapeslib, IA(3, 1), LIB_palettelib, IA(0, 3),
           LIB_doslib, BL_LA(6));
BLITZ_SUBS(_lesaveshapes, 0, 0);
BLITZ_NAME("LESaveShapes", "shape#,shape#,filename$[,palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_setcompress, 0, 0);
BLITZ_NAME("LECompressShapes", "True|False");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(2, 0), LIB_shapeslib, IA(3, 1), LIB_palettelib, IA(0, 3),
           LIB_doslib, BL_LA(6));
BLITZ_SUBS(_lecludgeshapes, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(2, 0), LIB_shapeslib, IA(3, 1), LIB_palettelib, IA(0, 3),
           LIB_doslib, BL_LA(6));
BLITZ_SUBS(_lecludgeshapes2, 0, 0);
BLITZ_NAME("LECludgeShapes", "shape#,shape#,address,palette#[,copymemflag]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(2, 0));
BLITZ_SUBS(_leinitshape, 0, 0);
BLITZ_NAME("LEInitShape", "shape#,width,height,depth");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(2, 0), LIB_shapeslib, IA(3, 1));
BLITZ_SUBS(_lescanshapes, 0, 0);
BLITZ_NAME("LEScanShapes", "startshape#,endshape#");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_leMaxShapeWidth, 0, 0);
BLITZ_NAME("LEMaxShapeWidth", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_leMaxShapeHeight, 0, 0);
BLITZ_NAME("LEMaxShapeHeight", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_leMaxShapeDepth, 0, 0);
BLITZ_NAME("LEMaxShapeDepth", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_leautocookie, 0, 0);
BLITZ_NAME("LEAutoCookie", "on|off");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_execlib, BL_LA(6));
BLITZ_SUBS(_loadshapestoaddress, 0, 0);
BLITZ_NAME("CludgeShapesToAddress", "Address,numshapes,address");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0, LIB_doslib, BL_LA(6));
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (ULONG)(c) << 8 | (ULONG)(d))
#define CLUDGED 0x634c                  /* "cL" at +30: the data is not ours */

/* the layouts below are read and written through SW/SL at the member's
   offset (the code stays that of the offsets): FW/FL a word/long member,
   FPAIR the long of two word members from f on */
#define FW(p, T, f)    SW(p, offsetof(struct T, f))
#define FL(p, T, f)    SL(p, offsetof(struct T, f))
#define FPAIR(p, T, f) SL(p, offsetof(struct T, f))

/* a shape object of shapeslib (32 bytes; also the header of a shape in
   memory for CludgeShapes) */
struct shapeobj {
    UWORD width, height, depth;         /* 0 2 4 (pixwidth, pixheight) */
    UWORD ebwidth;                      /* 6: bytes per row, even */
    UWORD bltsize;                      /* 8 */
    WORD xhandle, yhandle;              /* 10 12 */
    ULONG data;                         /* 14 */
    ULONG cookie;                       /* 18 */
    UWORD onebpmem;                     /* 22: bytes of one plane */
    UWORD onebpmemx;                    /* 24: the same + 2 * height */
    UWORD allbpmem;                     /* 26: bytes of all planes */
    UWORD allbpmemx;                    /* 28 */
    UWORD cludged;                      /* 30: CLUDGED = not our data */
};
static_assert(sizeof(struct shapeobj) == 32);
#define SHW(p, f)    FW(p, shapeobj, f)
#define SHL(p, f)    FL(p, shapeobj, f)
#define SHPAIR(p, f) FPAIR(p, shapeobj, f)

/* a palette object of palettelib (its start) and its data */
struct paletteobj {
    ULONG data;                         /* 0: struct paldata */
    UWORD colours;                      /* 4 */
};
static_assert(offsetof(struct paletteobj, colours) == 4);
struct paldata {
    UWORD colours, pad;                 /* 0 */
    ULONG rgb[];                        /* 4: r, g, b longs per colour */
};
static_assert(offsetof(struct paldata, rgb) == 4);

/* the LE file: an IFF chunk header, the SHHD data, the file's head (the
   FORM, LESH and the SHHD chunk), one shape in SDAT (then its data).
   shhd and lehead are packed (alignment 1): emptyheader is a byte block
   in the original, GCC writes its longs byte by byte */
struct iffchunk {
    ULONG id, len;
};
static_assert(sizeof(struct iffchunk) == 8);
struct __attribute__((packed)) shhd {
    ULONG count;                        /* 0: the number of shapes - 1 */
    UWORD compression;                  /* 4 */
    UBYTE zero[14];
};
static_assert(sizeof(struct shhd) == 20);
struct __attribute__((packed)) lehead {
    ULONG form, formlen, lesh, shhd, shhdlen;
    struct shhd hd;                     /* 20 */
};
static_assert(sizeof(struct lehead) == 40);
struct leshape {
    UWORD width, height, depth;         /* 0 2 4 */
    WORD xhandle, yhandle;              /* 6 8 */
    UWORD size;                         /* 10: bytes of the data */
};
static_assert(sizeof(struct leshape) == 12);

static struct DosLibrary *dosbase;      /* a6 of the entry (#doslib,#la6) */
static UWORD maxw, maxh, maxd;
static ULONG bytespershape;             /* stays from call to call */
static ULONG centry;                    /* CMAP bytes + 8 of the last save */
static ULONG chunksize, chunkptr, chunkid, filehandle;
static UBYTE *paletteptr, *startshape, *endshape;
static ULONG shapecount;
static UWORD colourcount;               /* the upper word of a long */
static UWORD aciddata_s1, aciddata_s2;
static ULONG aciddata_name;
static UWORD compress, compressed;
static ULONG packsize, packptr, packedsize;
static LONG inchip;
static ULONG cookeeemode = 1;
static struct lehead emptyheader = {
    ID('F', 'O', 'R', 'M'), 0, ID('L', 'E', 'S', 'H'), ID('S', 'H', 'H', 'D'), 20, { 0, 0, { 0 } }
};
static const char pme[] = "Not enough memory to pack shapes";
static const char coer[] = "Shape Compression Failed!";

static ULONG galloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };
    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

static ULONG gfree(ULONG mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = mem };
    return BLITZ_ALIBJSR(TOK_globfree, r);
}

/* ------------------------------------------------- the stack imbalances */

/* the stack pointer at the entries that call readshapes, after the
   thunk saved its 4 registers: rs_sp[4] = the Blitz program's return
   address (rs_stack) */
ULONG *rs_sp;
#define rs_stack (rs_sp + 4)
static int rs_pops;                     /* the a0 popped without a push */

/* an RTS with a0 still on the stack: returns to the address in a0 */
static void __attribute__((noreturn)) rs_crash(UBYTE *pushed)
{
    __asm volatile("jmp (%0)" : : "a"(pushed));
    __builtin_unreachable();
}

/* readshapes' MOVE.l (a7)+,a0 without the MOVE.l a0,-(a7) (a cludged
   shape): the first gives readshapes' return address (into its caller,
   the C analogue of the original's), the next ones the longs of the
   Blitz program's stack from its return address on */
static UBYTE *rs_pop(UBYTE *ret)
{
    UBYTE *popped = rs_pops ? (UBYTE *)rs_stack[rs_pops - 1] : ret;
    rs_pops++;
    return popped;
}

/* readshapes' RTS after k pops: k = 1 returns to the caller of the entry
   (the caller of readshapes then returns at once, see rs_pops), k >= 2
   to the k-th long of the Blitz program's stack */
static void rs_return(void)
{
    if (rs_pops >= 2)
        __asm volatile("move.l %0,sp\n\tjmp (%1)"
                       : : "a"(rs_stack + rs_pops), "a"(rs_stack[rs_pops - 1]));
}

/* ------------------------------------------------------- init / finit */

/**
 * @brief init: LECompressShapes off.
 * @return nothing
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    compress = 0;
}
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");

/**
 * @brief finit (_libfinit): nothing, an RTS.
 */
BLITZ_ASM_ROUTINE(_libfinit, "   rts");

/* ----------------------------------------------------- small commands */

/**
 * @brief LEAutoCookie on|off: whether CludgeShapes makes the cookies of
 * its shapes (on at the start).
 * @param on  d0: any nonzero value is on
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE leautocookie_c(BB_BOOL on BL_REG(d0))
{
    cookeeemode = on;
}
BLITZ_THUNK(_leautocookie, leautocookie_c, "d0-d1/a0-a1");

/**
 * @brief LECompressShapes True|False: whether LESaveShapes packs the shape
 * data (PackPBM of RIPackLib).
 * @param on  d0.w: negative (True) packs, else stored
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE setcompress_c(BB_BOOL on BL_REG(d0))
{
    compress = (UWORD)on;
}
BLITZ_THUNK(_setcompress, setcompress_c, "d0-d1/a0-a1");

/**
 * @brief LEMaxShapeWidth: the largest width of the last LEScanShapes.
 * @return d0.w: the width
 */
static ULONG BLITZ_ROUTINE maxwidth_c(void)
{
    return maxw;
}
BLITZ_THUNK(_leMaxShapeWidth, maxwidth_c, "d1/a0-a1");

/**
 * @brief LEMaxShapeHeight: the largest height of the last LEScanShapes.
 * @return d0.w: the height
 */
static ULONG BLITZ_ROUTINE maxheight_c(void)
{
    return maxh;
}
BLITZ_THUNK(_leMaxShapeHeight, maxheight_c, "d1/a0-a1");

/**
 * @brief LEMaxShapeDepth: the largest depth of the last LEScanShapes.
 * @return d0.w: the depth
 */
static ULONG BLITZ_ROUTINE maxdepth_c(void)
{
    return maxd;
}
BLITZ_THUNK(_leMaxShapeDepth, maxdepth_c, "d1/a0-a1");

/**
 * @brief LEScanShapes startshape#,endshape#: the largest width, height
 * and depth of the used shapes in the range (for LEMaxShape...).
 *
 * Signed word compares; empty shapes (width 0) are skipped.
 * @param shape  a2: the first shape object
 * @param last   a3: the last shape object
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE lescanshapes_c(UBYTE *shape BL_REG(a2), UBYTE *last BL_REG(a3))
{
    WORD w = 0, h = 0, d = 0;

    do {
        if (SHW(shape, width)) {
            if (w <= (WORD)SHW(shape, width))
                w = SHW(shape, width);
            if (h <= (WORD)SHW(shape, height))
                h = SHW(shape, height);
            if (d <= (WORD)SHW(shape, depth))
                d = SHW(shape, depth);
        }
        shape += 32;
    } while ((ULONG)shape <= (ULONG)last);
    maxw = w;
    maxh = h;
    maxd = d;
}
BLITZ_THUNK(_lescanshapes, lescanshapes_c, "d0-d1/a0-a1");

/**
 * @brief SafeToBlit(shape,x,y): whether the shape can be blitted at x,y
 * into the used bitmap.
 *
 * True if the shape exists (in range, has a cookie), x and y are not
 * negative, and it fits into the bitmap's width and height (+56.w/+58.w
 * of the bitmap object).  BUG(orig) (#822): shape/32 is compared with the
 * maximum, not the shape number.
 * @param shapenum   d0.w: shape
 * @param x          d1.w: x
 * @param y          d2.w: y
 * @param bitmap     a2: the used bitmap object
 * @param shape      a3: the shape object
 * @param maxshapes  d6: the maximum of shapes
 * @return d0.w: -1 if safe, else 0
 */
static BB_BOOL BLITZ_ROUTINE safetoblit_c(ULONG shapenum BL_REG(d0), ULONG x BL_REG(d1),
                                          ULONG y BL_REG(d2), UBYTE *bitmap BL_REG(a2),
                                          UBYTE *shape BL_REG(a3), ULONG maxshapes BL_REG(d6))
{
    if ((WORD)maxshapes <= (WORD)((WORD)shapenum >> 5))
        return BB_FALSE;
    if (!SHL(shape, cookie))
        return BB_FALSE;
    if ((WORD)x < 0 || (WORD)y < 0)
        return BB_FALSE;
    if ((UWORD)(x + SHW(shape, width)) > SW(bitmap, 56))
        return BB_FALSE;
    if ((UWORD)(y + SHW(shape, height)) > SW(bitmap, 58))
        return BB_FALSE;
    return BB_TRUE;
}
BLITZ_THUNK(_safetoblit, safetoblit_c, "d1/a0-a1");

/* -------------------------------------------------------- CludgeShapes */

/* CreateCookie: the cookie of shape a2 in chip memory, the planes ORed
   (DBRA on depth - 1 and plane bytes - 1).  a0 = the original's pushed
   a0 (returned to if the allocation fails - never, #806). */
static void createcookie(UBYTE *shape, UBYTE *pushed)
{
    UBYTE *src, *dst;
    UWORD planes, n;

    SHL(shape, cookie) = galloc(SHW(shape, onebpmem), MEMF_CHIP | MEMF_CLEAR);
    if (!SHL(shape, cookie))
        rs_crash(pushed);
    src = (UBYTE *)SHL(shape, data);
    planes = SHW(shape, depth) - 1;
    do {
        n = SHW(shape, onebpmem) - 1;
        dst = (UBYTE *)SHL(shape, cookie);
        do
            *dst++ |= *src++;
        while (n-- != 0);
    } while (planes-- != 0);
}

/* _freeshapesm: free the shape's data (not if "cL") and its cookie */
static void freeshapesm(UBYTE *shape)
{
    ULONG cookie;

    SHW(shape, width) = 0;
    if (SHW(shape, cludged) != CLUDGED) {
        SHW(shape, cludged) = 0;
        gfree(SHL(shape, data), SHW(shape, allbpmem));
    }
    if ((cookie = SHL(shape, cookie)) != 0) {
        SHL(shape, cookie) = 0;
        gfree(cookie, SHW(shape, onebpmem));
    }
}

/**
 * @brief CludgeShapes shape#,numshapes,address: makes shapes of shapes in
 * memory (a 32-byte shape header + the data each), copied into chip
 * memory.
 *
 * Shapes that are in use are freed first; a cookie is made if
 * LEAutoCookie is on.  BUG(orig): an empty shape in memory (width or
 * height 0) advances by 32 + the low word of the size of the shape before
 * - also of an earlier call, the variable stays (#809); a shape with size
 * 0 copies 65536 bytes (#808); a failing cookie allocation would jump to
 * the data address (#806, never in practice).
 * @param shapenum   d0: shape# (unused: shifted by 5, as the original)
 * @param numshapes  d1: the number of shapes
 * @param address    d2: the shapes in memory
 * @param objects    a0: the shape object of shape#
 * @return d0: -1, or 0 for no shapes or missing chip memory
 */
static BB_BOOL BLITZ_ROUTINE loadshapes_c(ULONG shapenum BL_REG(d0), ULONG numshapes BL_REG(d1),
                                          ULONG address BL_REG(d2), UBYTE *objects BL_REG(a0))
{
    UBYTE *shapeptr = objects, *shaperam = (UBYTE *)address, *src, *dst;
    LONG numbershapes = numshapes, shapenumber = 0;
    UWORD n;

    (void)shapenum;                     /* shapestart: d0/32, never used */
    if (!numshapes)
        return BB_FALSE;
    do {
        if (SHW(shapeptr, width))
            freeshapesm(shapeptr);
        bytespershape &= 0xffff;        /* MOVE.w #0,bytespershape */
        if (SHW(shaperam, width) && SHW(shaperam, height)) {
            /* calcsize, _allocshapemem */
            bytespershape = bl_mulu_w(SHW(shaperam, onebpmem), SHW(shaperam, depth));
            SHL(shapeptr, data) = galloc(bytespershape, MEMF_CHIP);
            if (!SHL(shapeptr, data))
                return BB_FALSE;
            /* the object without +14 (data) and +18 (cookie) */
            SHPAIR(shapeptr, width) = SHPAIR(shaperam, width);
            SHPAIR(shapeptr, depth) = SHPAIR(shaperam, depth);
            SHPAIR(shapeptr, bltsize) = SHPAIR(shaperam, bltsize);
            SHW(shapeptr, yhandle) = SHW(shaperam, yhandle);
            SHW(shapeptr, onebpmem) = SHW(shaperam, onebpmem);
            SHPAIR(shapeptr, onebpmemx) = SHPAIR(shaperam, onebpmemx);
            SHPAIR(shapeptr, allbpmemx) = SHPAIR(shaperam, allbpmemx);
            src = shaperam + 32;
            dst = (UBYTE *)SHL(shapeptr, data);
            n = (UWORD)(bytespershape - 1);
            do
                *dst++ = *src++;
            while (n-- != 0);
            SHL(shapeptr, cookie) = 0;
            if (cookeeemode)
                createcookie(shapeptr, src);
        }
        shapeptr += 32;
        shaperam += 32 + bytespershape;
        shapenumber++;
    } while (numbershapes > shapenumber);
    return BB_TRUE;
}
BLITZ_THUNK(_loadshapes, loadshapes_c, "d1/a0-a1");

/**
 * @brief CludgeShapesToAddress Address,numshapes,address: CludgeShapes
 * into shape objects at an address of the program's own.
 *
 * MOVE.l d0,a0:MOVEQ #0,d0, then as CludgeShapes (#808, #809).
 * @param objects    d0: the first of numshapes shape objects (32 bytes each)
 * @param numshapes  d1: the number of shapes
 * @param address    d2: the shapes in memory
 * @return d0: -1, or 0 for no shapes or missing chip memory
 */
static BB_BOOL BLITZ_ROUTINE loadshapestoaddress_c(ULONG objects BL_REG(d0),
                                                   ULONG numshapes BL_REG(d1),
                                                   ULONG address BL_REG(d2))
{
    return loadshapes_c(0, numshapes, address, (UBYTE *)objects);
}
BLITZ_THUNK(_loadshapestoaddress, loadshapestoaddress_c, "d1/a0-a1");

/* --------------------------------------------------------- LEInitShape */

/* the derived fields of a shape whose width, height and depth are set:
   +6 bytes per row, +8 blit size, +22/+24 plane bytes, +28; `all` = the
   bytes of all planes for +28 (LEInitShape computes +26 first) */
static void shapesizes(UBYTE *shape, int init)
{
    UWORD rowbytes, rowwords;           /* d0 */
    UWORD height, height2;              /* d0: the height, 2 * height */
    UWORD planebytes;                   /* d1 */

    rowbytes = (UWORD)(((UWORD)(SHW(shape, width) + 15) >> 4) << 1);
    SHW(shape, ebwidth) = rowbytes;
    rowwords = rowbytes >> 1;
    SHW(shape, bltsize) = (UWORD)(rowwords + (UWORD)bl_mulu_w(SHW(shape, height), 64) + 1);
    height = SHW(shape, height);
    planebytes = (UWORD)bl_mulu_w(SHW(shape, ebwidth), height);
    SHW(shape, onebpmem) = planebytes;
    if (init)
        SHW(shape, allbpmem) = (UWORD)bl_mulu_w(planebytes, SHW(shape, depth));
    height2 = (UWORD)(height << 1);
    SHW(shape, onebpmemx) = (UWORD)(planebytes + height2);
    SHW(shape, allbpmemx) = (UWORD)(bl_mulu_w(height2, SHW(shape, depth)) + SHW(shape, allbpmem));
}

/**
 * @brief LEInitShape shape#,width,height,depth: makes an empty shape.
 *
 * A shape in use is freed first; the sizes are computed, the data and
 * the cookie get new cleared chip memory (the cookie is not computed),
 * the handles are 0.  Without memory for the data no cookie is
 * allocated.  BUG(orig) (#810): the old cookie is not freed.
 * @param width   d1.w: the width
 * @param height  d2.w: the height
 * @param depth   d3.w: the depth
 * @param shape   a2: the shape object of shape# (d0)
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE leinitshape_c(ULONG width BL_REG(d1), ULONG height BL_REG(d2),
                                        ULONG depth BL_REG(d3), UBYTE *shape BL_REG(a2))
{
    if (SHW(shape, width)) {
        SHW(shape, width) = 0;
        if (SHW(shape, cludged) != CLUDGED)
            gfree(SHL(shape, data), SHW(shape, allbpmem));
        SHW(shape, cludged) = 0;
        /* MOVE.l 18(a2),a1:BEQ - the Z of the CLR: never freed */
    }
    SHW(shape, width) = (UWORD)width;
    SHW(shape, height) = (UWORD)height;
    SHW(shape, depth) = (UWORD)depth;
    SHPAIR(shape, xhandle) = 0;
    shapesizes(shape, 1);
    SHL(shape, data) = galloc(SHW(shape, allbpmem), MEMF_CHIP | MEMF_CLEAR);
    if (!SHL(shape, data))
        return;
    SHL(shape, cookie) = galloc(SHW(shape, onebpmem), MEMF_CHIP | MEMF_CLEAR);
}
BLITZ_THUNK(_leinitshape, leinitshape_c, "d0-d1/a0-a1");

/* ----------------------------------------------------- reading LE data */

/* read_palette: the CMAP at a0 (chunksize bytes) into the palette object
   paletteptr (nothing without one) */
static void read_palette(const UBYTE *cmap)
{
    UBYTE *palette;
    ULONG *p;
    ULONG size, colours;
    UWORD n;
    int i;

    colours = bl_divu_w(chunksize, 3);  /* BUG(orig): overflow keeps it (#820) */
    colourcount = (UWORD)colours;
    if (!paletteptr)
        return;
    palette = paletteptr;
    if (FL(palette, paletteobj, data)) {
        size = bl_mulu_w(FW(palette, paletteobj, colours), 12);
        BL_SETW(size, size + 8);        /* ADDQ #8,d0: a word addition */
        gfree(FL(palette, paletteobj, data), size);
    }
    size = bl_mulu_w(colourcount, 12);
    BL_SETW(size, size + 8);
    /* unchecked: harmless (#821) */
    FL(palette, paletteobj, data) = galloc(size, MEMF_PUBLIC | MEMF_CLEAR);
    FW(palette, paletteobj, colours) = colourcount;
    p = (ULONG *)FL(palette, paletteobj, data);
    *(volatile UWORD *)p = colourcount;
    p++;
    n = colourcount - 1;
    do
        for (i = 0; i < 3; i++)
            *p++ = 0x01010101UL * *cmap++;
    while (n-- != 0);
}

/* readshapes: the shapes from a0 (SDAT data) into startshape..endshape,
   at most shapecount + 1 of them.  Returns nonzero if a cludged shape
   unbalanced the stack (the caller then returns at once). */
static int __attribute__((noinline)) readshapes(UBYTE *data)
{
    UBYTE *shape = startshape, *last = endshape, *dst;
    UWORD n, planes;

    rs_pops = 0;
    for (;;) {
        if (SHW(shape, width)) {
            SHW(shape, width) = 0;
            if (SHW(shape, cludged) == CLUDGED) {
                /* BUG(orig) #807: nothing pushed, the pop below takes the
                   return address (rs_pop) */
                SHW(shape, cludged) = 0;
                data = rs_pop(__builtin_return_address(0));
            } else {
                gfree(SHL(shape, data), SHW(shape, allbpmem));
                SHW(shape, cludged) = 0;
                /* the cookie: MOVE.l 18(a2),a1:BEQ after the CLR, never freed */
            }
        }
        SHPAIR(shape, width) = FPAIR(data, leshape, width);
        SHW(shape, depth) = FW(data, leshape, depth);     /* depth */
        SHPAIR(shape, xhandle) = FPAIR(data, leshape, xhandle);    /* handles */
        SHW(shape, allbpmem) = FW(data, leshape, size);   /* bytes of the shape */
        data += 12;
        shapesizes(shape, 0);
        if (inchip < 0) {
            /* 'decludgeshape: the data stays where it is */
            SHL(shape, data) = (ULONG)data;
            data += SHW(shape, allbpmem);
            SHW(shape, cludged) = CLUDGED;
        } else {
            SHL(shape, data) = galloc(SHW(shape, allbpmem), MEMF_CHIP);
            if (!SHL(shape, data))
                rs_crash(data);         /* never reached (#806) */
            if ((WORD)compressed < 0 && (LONG)(packedsize = SL(data, 0)) >= 0) {
                /* 'unpack_shape */
                data += 4;
                packptr = (ULONG)data;
                BL_JSR_D4_(BL_TOKVAL(TOK_unpackpbm, 0), (ULONG)data, SHL(shape, data), packedsize,
                           SHW(shape, allbpmem));
                data = (UBYTE *)(packptr + packedsize);
            } else {
                if ((WORD)compressed < 0)
                    data += 4;          /* -1: stored ('changedmind') */
                n = SHW(shape, allbpmem) - 1;
                dst = (UBYTE *)SHL(shape, data);
                do
                    *dst++ = *data++;
                while (n-- != 0);
            }
        }
        /* 'dodacookie */
        SHL(shape, cookie) = galloc(SHW(shape, onebpmem), MEMF_CHIP | MEMF_CLEAR);
        if (!SHL(shape, cookie))
            rs_crash(data);             /* never reached (#806) */
        {
            const UBYTE *src = (const UBYTE *)SHL(shape, data);
            planes = SHW(shape, depth) - 1;
            do {
                n = SHW(shape, onebpmem) - 1;
                dst = (UBYTE *)SHL(shape, cookie);
                do
                    *dst++ |= *src++;
                while (n-- != 0);
            } while (planes-- != 0);
        }
        if ((LONG)--shapecount < 0)
            break;
        shape += 32;
        if ((ULONG)shape > (ULONG)last)
            break;
    }
    rs_return();
    return rs_pops;
}

/* ------------------------------------------------------ LECludgeShapes */

/**
 * @brief LECludgeShapes shape#,shape#,address,palette#,copymemflag: the
 * shapes (and the palette) of an LE file in memory.
 *
 * Nothing if the file is no FORM LESH with SHHD first or has no SDAT.
 * The shapes are linked into the memory ("cL", no copy) if the file is
 * uncompressed and below 2 MB, unless copymemflag is negative; otherwise
 * copied (unpacked) into chip memory.  Every shape gets a cookie.  At
 * most count + 1 shapes of the file are read.  BUG(orig): the SHHD chunk
 * is skipped as 20 bytes whatever its length (#811); a CMAP size that is
 * no multiple of 3 misses SDAT (#811); the palette as in LELoadShapes
 * (#820); a shape in use that is cludged pops a0 it never pushed and
 * returns past the caller (#807); the old cookies are not freed (#810).
 * @param file     d2: the LE file in memory (address)
 * @param copymem  d4.w: copymemflag (negative: always copy)
 * @param palette  a0: the palette object of palette# (d3)
 * @param first    a2: the first shape object
 * @param last     a3: the last shape object
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE lecludgeshapes2_c(ULONG file BL_REG(d2), ULONG copymem BL_REG(d4),
                                            UBYTE *palette BL_REG(a0), UBYTE *first BL_REG(a2),
                                            UBYTE *last BL_REG(a3))
{
    UBYTE *p;

    inchip = 0;
    startshape = first;
    endshape = last;
    paletteptr = palette;
    p = (UBYTE *)file;
    if ((WORD)copymem >= 0 && !(file & 0xffe00000UL))
        inchip = -1;
    if (FL(p, lehead, form) != ID('F', 'O', 'R', 'M')
        || FL(p, lehead, lesh) != ID('L', 'E', 'S', 'H')
        || FL(p, lehead, shhd) != ID('S', 'H', 'H', 'D'))
        return;
    p += offsetof(struct lehead, hd);
    shapecount = FL(p, shhd, count);
    compressed = FW(p, shhd, compression);
    if (compressed)
        inchip = 0;
    p += 20;                            /* BUG(orig): SHHD as 20 bytes (#811) */
    if (FL(p, iffchunk, id) == ID('C', 'M', 'A', 'P')) {
        chunksize = FL(p, iffchunk, len);
        p += 8;
        if (paletteptr) {
            read_palette(p);
            p += bl_mulu_w(colourcount, 3);     /* a0 behind the colours */
            if (!colourcount)
                p += 3 * 65536;
        } else
            p += chunksize;
    }
    if (FL(p, iffchunk, id) != ID('S', 'D', 'A', 'T'))
        return;
    readshapes(p + 8);
}
BLITZ_THUNK_PRE(_lecludgeshapes2, lecludgeshapes2_c, "d0-d1/a0-a1", "move.l sp,_rs_sp");

/**
 * @brief LECludgeShapes shape#,shape#,address,palette#: as above with
 * copymemflag 0 (CLR.l d4).
 * @param file     d2: the LE file in memory
 * @param palette  a0: the palette object of palette# (d3)
 * @param first    a2: the first shape object
 * @param last     a3: the last shape object
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE lecludgeshapes_c(ULONG file BL_REG(d2), UBYTE *palette BL_REG(a0),
                                           UBYTE *first BL_REG(a2), UBYTE *last BL_REG(a3))
{
    lecludgeshapes2_c(file, 0, palette, first, last);   /* CLR.l d4 */
}
BLITZ_THUNK_PRE(_lecludgeshapes, lecludgeshapes_c, "d0-d1/a0-a1", "move.l sp,_rs_sp");

/* -------------------------------------------------------- LELoadShapes */

/* LoadShapes of shapeslib (form 1) with the shapes of a2 (as the last)
   and a3 (as the first; BUG(orig): exchanged, #813), d2 = the name, a6 = dos */
static void acidloadshapes(UBYTE *first, UBYTE *last)
{
    register ULONG d0 __asm("d0") = aciddata_s1;
    register ULONG d1 __asm("d1") = aciddata_s2;
    register ULONG d2 __asm("d2") = aciddata_name;
    register UBYTE *ra2 __asm("a2") = first;
    register struct DosLibrary *a6 __asm("a6") = dosbase;
    register UBYTE *a0 __asm("a0") = last;

    /* a3 as the original leaves it; a3 is fixed for GCC (blitz_string) */
    __asm volatile("move.l a0,a3\n\tjsr _blitz_tok+%c6"
                   : "+d"(d0), "+d"(d1), "+d"(d2), "+a"(ra2), "+a"(a6), "+a"(a0)
                   : "i"(BL_TOKVAL(TOK_loadshapes, 1))
                   : "d3", "d4", "d5", "d6", "d7", "a1", "cc", "memory");
}

/* freechunk: the current chunk buffer */
static void freechunk(void)
{
    if (chunkptr) {
        gfree(chunkptr, chunksize);
        chunkptr = 0;
    }
}

/* the end of _leloadshapes: freeall (the chunk), freememory (the header
   buffer), freefile; the result packedsize */
enum { FREEALL, FREEMEMORY, FREEFILE };
static ULONG cleanup(int from)
{
    if (from <= FREEALL)
        freechunk();
    if (from <= FREEMEMORY)
        gfree(chunkid, 8);
    Close(filehandle);
    return packedsize;
}

/* readchunk: the chunk of the header buffer (id, length) into a new
   buffer; 0 if the read is short.  BUG(orig) #819: a failing allocation jumps
   to the caller's freeall and returns from there (packedsize), so the
   caller sees packedsize: 0 sends it to freememory again. */
static ULONG readchunk(BPTR fh)
{
    ULONG len = FL(chunkid, iffchunk, len), buf;

    buf = galloc(len, 0);
    if (!buf)
        return cleanup(FREEALL);
    chunkptr = buf;
    if (Read(fh, (APTR)buf, len) != (LONG)len) {
        gfree(buf, len);
        chunkptr = 0;
        return 0;
    }
    chunksize = len;
    return chunkptr;
}

static ULONG leloadshapes(ULONG shape1, ULONG shape2, ULONG name, UBYTE *palette, UBYTE *first,
                          UBYTE *last, struct DosLibrary *dos)
{
    BPTR fh;
    ULONG buf, chunk;

    dosbase = dos;
    paletteptr = palette;
    startshape = first;
    endshape = last;
    inchip = 0;
    aciddata_s1 = (UWORD)((WORD)shape1 >> 5);
    aciddata_s2 = (UWORD)((WORD)shape2 >> 5);
    aciddata_name = name;
    filehandle = fh = Open((STRPTR)name, MODE_OLDFILE);
    if (!fh)
        return packedsize;
    chunkid = buf = galloc(8, 0);
    if (!buf)
        return cleanup(FREEFILE);
    Read(fh, (APTR)buf, 8);
    if (FL(chunkid, iffchunk, id) != ID('F', 'O', 'R', 'M')) {
        /* tryacid */
        gfree(chunkid, 8);
        Close(filehandle);
        acidloadshapes(first, last);
        return 0;
    }
    Read(fh, (APTR)buf, 4);
    if (FL(chunkid, iffchunk, id) != ID('L', 'E', 'S', 'H'))
        return cleanup(FREEMEMORY);
    Read(fh, (APTR)buf, 8);
    if (FL(chunkid, iffchunk, id) != ID('S', 'H', 'H', 'D'))
        return cleanup(FREEMEMORY);
    if (!(chunk = readchunk(fh)))
        return cleanup(FREEMEMORY);
    shapecount = FL(chunk, shhd, count);
    compressed = FW(chunk, shhd, compression);
    freechunk();
    Read(fh, (APTR)buf, 8);
    if (FL(chunkid, iffchunk, id) == ID('C', 'M', 'A', 'P')) {
        if (!(chunk = readchunk(fh)))
            return cleanup(FREEMEMORY);
        read_palette((const UBYTE *)chunk);
        freechunk();
        Read(fh, (APTR)buf, 8);
    }
    if (FL(chunkid, iffchunk, id) != ID('S', 'D', 'A', 'T'))
        return cleanup(FREEALL);
    if (!(chunk = readchunk(fh)))
        return cleanup(FREEMEMORY);
    if (readshapes((UBYTE *)chunk))
        return 0;                       /* BUG(orig): returned past us (#807) */
    return cleanup(FREEALL);
}

/**
 * @brief LELoadShapes shape#,shape#,filename$,palette#: loads the shapes
 * (and the palette) of an LE file into the range of shapes.
 *
 * At most count + 1 shapes of the file, unpacked if compressed, each with
 * a cookie; the CMAP into the palette object.  A file that is no FORM goes
 * to shapeslib's LoadShapes (form 1).  BUG(orig): the shape numbers given
 * on to LoadShapes are shifted right by 5 (#815), and the first and last
 * shape are exchanged (#813); a failing chunk allocation frees the header
 * buffer and closes the file twice (#819, never in practice); the
 * palette: no check of the allocation (#821), DIVU #3 overflow, 0 colours
 * = 65536 (#820); a shape in use that is cludged pops a0 it never pushed
 * and returns past the caller without freeing the buffers or closing the
 * file (#807); the old cookies are not freed (#810).
 * @param shape1   d0.w: the first shape#
 * @param shape2   d1.w: the last shape#
 * @param name     d2: filename$
 * @param palette  a0: the palette object of palette# (d3)
 * @param first    a2: the first shape object
 * @param last     a3: the last shape object
 * @param dos      a6: dos.library
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE leloadshapesp_c(ULONG shape1 BL_REG(d0), ULONG shape2 BL_REG(d1),
                                          ULONG name BL_REG(d2), UBYTE *palette BL_REG(a0),
                                          UBYTE *first BL_REG(a2), UBYTE *last BL_REG(a3),
                                          struct DosLibrary *dos BL_REG(a6))
{
    leloadshapes(shape1, shape2, name, palette, first, last, dos);
}
BLITZ_THUNK_PRE(_leloadshapes, leloadshapesp_c, "d0-d1/a0-a1", "move.l sp,_rs_sp");

/**
 * @brief LELoadShapes shape#,shape#,filename$: as above without a palette
 * (the CMAP is skipped).
 * @param shape1  d0.w: the first shape#
 * @param shape2  d1.w: the last shape#
 * @param name    d2: filename$
 * @param first   a2: the first shape object
 * @param last    a3: the last shape object
 * @param dos     a6: dos.library
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE leloadshapesnop_c(ULONG shape1 BL_REG(d0), ULONG shape2 BL_REG(d1),
                                            ULONG name BL_REG(d2), UBYTE *first BL_REG(a2),
                                            UBYTE *last BL_REG(a3),
                                            struct DosLibrary *dos BL_REG(a6))
{
    leloadshapes(shape1, shape2, name, NULL, first, last, dos);
}
BLITZ_THUNK_PRE(_leloadshapesnop, leloadshapesnop_c, "d0-d1/a0-a1", "move.l sp,_rs_sp");

/* the one-shape forms: the last shape = the maximum - 1 (SUBQ/LSL/ADD
   on words: a3 + (WORD)((max - 1) << 5)); d1 = d4 (the name in d1) */

/**
 * @brief LELoadShapes shape#,filename$,palette#: as above from shape# up
 * to the last shape (the maximum - 1), as many as the file has.
 *
 * BUG(orig) (#812): the palette is that of d3, not of the third argument
 * (!libs #pd3).
 * @param shapenum   d0.w: shape#
 * @param name       d1: filename$
 * @param maxshapes  d4: the maximum of shapes
 * @param palette    a0: the palette object of the number in d3 (#812)
 * @param first      a2: the shape object of shape#
 * @param objects    a3: the first shape object (item 0)
 * @param dos        a6: dos.library
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE leloadshapesingle_c(ULONG shapenum BL_REG(d0), ULONG name BL_REG(d1),
                                              ULONG maxshapes BL_REG(d4),
                                              UBYTE *palette BL_REG(a0), UBYTE *first BL_REG(a2),
                                              UBYTE *objects BL_REG(a3),
                                              struct DosLibrary *dos BL_REG(a6))
{
    ULONG lastoff = maxshapes;          /* d4: the last shape's offset, upper word kept */

    BL_SETW(lastoff, (UWORD)((UWORD)(maxshapes - 1) << 5));
    leloadshapes(shapenum, lastoff, name, palette, first, objects + (WORD)lastoff, dos);
}
BLITZ_THUNK_PRE(_leloadshapesingle, leloadshapesingle_c, "d0-d1/a0-a1", "move.l sp,_rs_sp");

/**
 * @brief LELoadShapes shape#,filename$: as above without a palette.
 * @param shapenum   d0.w: shape#
 * @param name       d1: filename$
 * @param maxshapes  d4: the maximum of shapes
 * @param first      a2: the shape object of shape#
 * @param objects    a3: the first shape object (item 0)
 * @param dos        a6: dos.library
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE leloadshapesinglenop_c(ULONG shapenum BL_REG(d0), ULONG name BL_REG(d1),
                                                 ULONG maxshapes BL_REG(d4),
                                                 UBYTE *first BL_REG(a2), UBYTE *objects BL_REG(a3),
                                                 struct DosLibrary *dos BL_REG(a6))
{
    leloadshapesingle_c(shapenum, name, maxshapes, NULL, first, objects, dos);
}
BLITZ_THUNK_PRE(_leloadshapesinglenop, leloadshapesinglenop_c, "d0-d1/a0-a1",
                "move.l sp,_rs_sp");

/* -------------------------------------------------------- LESaveShapes */

/* dos Write with the Z flag it leaves (BUG(orig): _saveCMAP's BNE, #814) */
static LONG write_z(BPTR fh, APTR buf, LONG len, UBYTE *z)
{
    register LONG d0 __asm("d0");
    register BPTR d1 __asm("d1") = fh;
    register APTR d2 __asm("d2") = buf;
    register LONG d3 __asm("d3") = len;
    register struct DosLibrary *a6 __asm("a6") = dosbase;

    __asm volatile("jsr -48(a6)\n\tseq %1"
                   : "=d"(d0), "=m"(*z), "+d"(d1), "+d"(d2), "+d"(d3), "+a"(a6)
                   :
                   : "a0", "a1", "cc", "memory");
    return d0;
}

/* _saveCMAP: the CMAP chunk of paletteptr; `z` = the Z flag of the Write
   before; returns the d0 the original leaves */
static ULONG savecmap(BPTR fh, ULONG result, UBYTE z)
{
    const UBYTE *pal;
    UBYTE *p;
    ULONG mem;
    UWORD n;

    if (z)                              /* MOVEA sets no flags: BNE */
        return result;
    pal = *(UBYTE *volatile *)&paletteptr;
    if (!*(volatile ULONG *)pal)        /* address 0 without a palette */
        return result;
    pal = (const UBYTE *)FL(pal, paletteobj, data);
    centry = bl_mulu_w(FW(pal, paldata, colours), 3);
    chunksize = centry + 8;
    mem = galloc(chunksize, 0);
    if (!mem)
        return 0;
    p = (UBYTE *)mem;
    FL(p, iffchunk, id) = ID('C', 'M', 'A', 'P');
    FL(p, iffchunk, len) = centry;
    p += 8;
    centry += 8;
    n = FW(pal, paldata, colours) - 1;
    pal += 4;
    do {
        *p++ = pal[0];
        *p++ = pal[4];
        *p++ = pal[8];
        pal += 12;
    } while (n-- != 0);
    Write(fh, (APTR)mem, chunksize);
    return gfree(mem, chunksize);
}

/* _writeshapes: the SDAT chunk of startshape..endshape; d0 = the d0 of
   _saveCMAP (MOVE.w #12,d0 keeps its upper word); returns the bytes
   written (d5) */
static ULONG writeshapes(BPTR fh, ULONG bufsize)
{
    UBYTE *shape, *last, *buf, *pk;
    ULONG written = 0, len;
    LONG packed;

    BL_SETW(bufsize, 12);
    buf = (UBYTE *)galloc(bufsize, 0);
    if (!buf)
        return 0;
    FL(buf, iffchunk, id) = ID('S', 'D', 'A', 'T');
    FL(buf, iffchunk, len) = 0;
    Write(fh, buf, 8);
    shape = startshape;
    last = endshape;
    do {
        FW(buf, leshape, width) = SHW(shape, width);
        FW(buf, leshape, height) = SHW(shape, height);
        FW(buf, leshape, depth) = SHW(shape, depth);
        FW(buf, leshape, xhandle) = SHW(shape, xhandle);
        FW(buf, leshape, yhandle) = SHW(shape, yhandle);
        FW(buf, leshape, size) = SHW(shape, allbpmem);
        Write(fh, buf, 12);
        written += 12;
        if (SHW(shape, width)) {
            if ((WORD)compress >= 0) {
                len = SHW(shape, allbpmem);
                written += len;
                Write(fh, (APTR)SHL(shape, data), len);
            } else {
                /* 'packshape: packed size.l + the data of PackPBM */
                pk = (UBYTE *)galloc(SHW(shape, allbpmem) + 4, 0);
                packsize = SHW(shape, allbpmem);
                packptr = (ULONG)pk;
                if (!pk) {
                    /* packmemerr: TRAP #0, falls into comperr (TRAP #0)
                       and CreateCookie, whose RTS returns from
                       _writeshapes; a0 = the header buffer + 12 (as far
                       as dos Write keeps it) */
                    BLITZ_RUNERR(pme);
                    BLITZ_RUNERR(coer);
                    createcookie(shape, buf + 12);
                    return written;
                }
                /* d3 = 12 (the last Write): BUG(orig) of PackPBM's failure (#772) */
                packed = BL_JSR_D4_(BL_TOKVAL(TOK_packpbm, 0), SHL(shape, data), (ULONG)pk + 4,
                                    SHW(shape, allbpmem), 12);
                SL(packptr, 0) = packed;
                if (packed >= 0) {
                    written += packed + 4;
                    Write(fh, (APTR)packptr, packed + 4);
                } else {
                    /* 'compfail: -1 and the data as it is */
                    Write(fh, (APTR)packptr, 4);
                    len = SHW(shape, allbpmem);
                    written += len + 4;
                    Write(fh, (APTR)SHL(shape, data), len);
                }
                gfree(packptr, packsize + 4);
            }
        }
        shape += 32;
    } while ((ULONG)shape <= (ULONG)last);
    written += 8;
    gfree((ULONG)buf, 14);
    return written;
}

/**
 * @brief LESaveShapes shape#,shape#,filename$,palette#: saves the range of
 * shapes (and the palette) as an LE file.
 *
 * FORM LESH, SHHD (count, compression), CMAP if there is a palette, SDAT
 * with the shapes (packed with PackPBM if LECompressShapes is on, -1 =
 * stored when packing fails).  Nothing if the file cannot be opened.
 * BUG(orig): the count is (shape2>>5)-(shape1>>5) with the upper word of
 * the register uncleared, so a file of shapes below 32 loads one shape
 * (#815); whether the CMAP is written depends on dos Write's Z flag
 * (#814); the FORM length is 8 too large and has the CMAP size of the
 * last save with a palette (#816); the shape header buffer is allocated
 * with the upper word of d0 of _saveCMAP and freed as 14 bytes (#817); a
 * palette of 0 colours copies 65536 (#823); a missing pack buffer is two
 * runtime errors (#818); PackPBM's failure runs UnPackPBM with d3 = 12
 * (#772).
 * @param shape1   d0.w: the first shape#
 * @param shape2   d1.w: the last shape#
 * @param name     d2: filename$
 * @param palette  a0: the palette object of palette# (d3)
 * @param first    a2: the first shape object
 * @param last     a3: the last shape object
 * @param dos      a6: dos.library
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE lesaveshapes_c(ULONG shape1 BL_REG(d0), ULONG shape2 BL_REG(d1),
                                         ULONG name BL_REG(d2), UBYTE *palette BL_REG(a0),
                                         UBYTE *first BL_REG(a2), UBYTE *last BL_REG(a3),
                                         struct DosLibrary *dos BL_REG(a6))
{
    BPTR fh;
    ULONG written, len, result;
    ULONG count = shape2;               /* d1: the shape count, upper word kept */
    UWORD num2;                         /* d1: shape2 >> 5 */
    UBYTE z;

    dosbase = dos;
    paletteptr = palette;
    startshape = first;
    endshape = last;
    BL_SETW(shape1, (WORD)shape1 >> 5);
    num2 = (UWORD)((WORD)shape2 >> 5);
    BL_SETW(count, num2 - shape1);
    shapecount = count;                 /* BUG(orig): the upper word too (#815) */
    fh = Open((STRPTR)name, MODE_NEWFILE);
    if (!fh)
        return;
    FL(&emptyheader, lehead, hd.count) = shapecount;
    FW(&emptyheader, lehead, hd.compression) = compress;
    result = write_z(fh, &emptyheader, sizeof emptyheader, &z);
    result = savecmap(fh, result, z);
    written = writeshapes(fh, result);
    Seek(fh, -(LONG)(written - 4), OFFSET_CURRENT);
    len = written - 8;
    Write(fh, &len, 4);
    written += centry + 40;             /* BUG(orig): 8 too many (#816) */
    Seek(fh, 4, OFFSET_BEGINNING);
    len = written;
    Write(fh, &len, 4);
    Seek(fh, offsetof(struct lehead, hd.count), OFFSET_BEGINNING);
    Write(fh, &shapecount, 4);
    Close(fh);
}
BLITZ_THUNK(_lesaveshapes, lesaveshapes_c, "d0-d1/a0-a1");

/**
 * @brief LESaveShapes shape#,shape#,filename$: as above without a palette
 * (the Write's flags decide as above, #814).
 * @param shape1  d0.w: the first shape#
 * @param shape2  d1.w: the last shape#
 * @param name    d2: filename$
 * @param first   a2: the first shape object
 * @param last    a3: the last shape object
 * @param dos     a6: dos.library
 * @return nothing (all registers kept)
 */
static void BLITZ_ROUTINE lesaveshapesnop_c(ULONG shape1 BL_REG(d0), ULONG shape2 BL_REG(d1),
                                            ULONG name BL_REG(d2), UBYTE *first BL_REG(a2),
                                            UBYTE *last BL_REG(a3),
                                            struct DosLibrary *dos BL_REG(a6))
{
    lesaveshapes_c(shape1, shape2, name, NULL, first, last, dos);
}
BLITZ_THUNK(_lesaveshapesnop, lesaveshapesnop_c, "d0-d1/a0-a1");
