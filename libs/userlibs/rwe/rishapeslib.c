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
 *     address (#806).  Stack imbalances that C cannot express: see rs_pop
 *     and rs_crash.
 *   - LELoadShapes form 3 (shape#,filename$,palette#) takes the palette of
 *     d3, not of the third argument (!libs #pd3) (#812).  LELoadShapes
 *     hands a non-LE file to shapeslib's LoadShapes with the shapes
 *     exchanged (a3 = the last shape, a2 = the first) (#813).  A failing
 *     chunk allocation frees the header buffer and closes the file twice
 *     (readchunk jumps to the cleanup of its caller and returns there)
 *     (#819).
 *   - read_palette: no check of the allocation (writes to address 0)
 *     (#821), the colour count of DIVU #3 (overflow: unchanged), 0 colours
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
static UBYTE emptyheader[40] = { 'F', 'O', 'R', 'M', 0, 0, 0, 0, 'L', 'E', 'S', 'H',
                                 'S', 'H', 'H', 'D', 0, 0, 0, 20 };
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

static void BLITZ_ROUTINE libinit_c(void)
{
    compress = 0;
}
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");

/* _libfinit: RTS */
BLITZ_ASM_ROUTINE(_libfinit, "   rts");

/* ----------------------------------------------------- small commands */

static void BLITZ_ROUTINE leautocookie_c(ULONG on BL_REG(d0))
{
    cookeeemode = on;
}
BLITZ_THUNK(_leautocookie, leautocookie_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE setcompress_c(ULONG on BL_REG(d0))
{
    compress = (UWORD)on;
}
BLITZ_THUNK(_setcompress, setcompress_c, "d0-d1/a0-a1");

static ULONG BLITZ_ROUTINE maxwidth_c(void)
{
    return maxw;
}
BLITZ_THUNK(_leMaxShapeWidth, maxwidth_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE maxheight_c(void)
{
    return maxh;
}
BLITZ_THUNK(_leMaxShapeHeight, maxheight_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE maxdepth_c(void)
{
    return maxd;
}
BLITZ_THUNK(_leMaxShapeDepth, maxdepth_c, "d1/a0-a1");

/* LEScanShapes start,end: the largest width, height and depth of the
   used shapes (signed word compares) */
static void BLITZ_ROUTINE lescanshapes_c(UBYTE *shape BL_REG(a2), UBYTE *last BL_REG(a3))
{
    WORD w = 0, h = 0, d = 0;

    do {
        if (SW(shape, 0)) {
            if (w <= (WORD)SW(shape, 0))
                w = SW(shape, 0);
            if (h <= (WORD)SW(shape, 2))
                h = SW(shape, 2);
            if (d <= (WORD)SW(shape, 4))
                d = SW(shape, 4);
        }
        shape += 32;
    } while ((ULONG)shape <= (ULONG)last);
    maxw = w;
    maxh = h;
    maxd = d;
}
BLITZ_THUNK(_lescanshapes, lescanshapes_c, "d0-d1/a0-a1");

/* SafeToBlit(shape,x,y): a3 = the shape, a2 = the used bitmap object
   (+56.w/+58.w its width and height), d6 = the maximum of shapes.
   BUG(orig): shape/32 is compared with the maximum (#822). */
static ULONG BLITZ_ROUTINE safetoblit_c(ULONG shapenum BL_REG(d0), ULONG x BL_REG(d1),
                                        ULONG y BL_REG(d2), UBYTE *bitmap BL_REG(a2),
                                        UBYTE *shape BL_REG(a3), ULONG maxshapes BL_REG(d6))
{
    if ((WORD)maxshapes <= (WORD)((WORD)shapenum >> 5))
        return 0;
    if (!SL(shape, 18))
        return 0;
    if ((WORD)x < 0 || (WORD)y < 0)
        return 0;
    if ((UWORD)(x + SW(shape, 0)) > SW(bitmap, 56))
        return 0;
    if ((UWORD)(y + SW(shape, 2)) > SW(bitmap, 58))
        return 0;
    return (ULONG)-1;
}
BLITZ_THUNK(_safetoblit, safetoblit_c, "d1/a0-a1");

/* -------------------------------------------------------- CludgeShapes */

/* CreateCookie: the cookie of shape a2 in chip memory, the planes ORed
   (DBRA on depth - 1 and plane bytes - 1).  a0 = the original's pushed
   a0 (BUG(orig): returned to if the allocation fails, #806). */
static void createcookie(UBYTE *shape, UBYTE *pushed)
{
    UBYTE *src, *dst;
    UWORD planes, n;

    SL(shape, 18) = galloc(SW(shape, 22), MEMF_CHIP | MEMF_CLEAR);
    if (!SL(shape, 18))
        rs_crash(pushed);
    src = (UBYTE *)SL(shape, 14);
    planes = SW(shape, 4) - 1;
    do {
        n = SW(shape, 22) - 1;
        dst = (UBYTE *)SL(shape, 18);
        do
            *dst++ |= *src++;
        while (n-- != 0);
    } while (planes-- != 0);
}

/* _freeshapesm: free the shape's data (not if "cL") and its cookie */
static void freeshapesm(UBYTE *shape)
{
    ULONG cookie;

    SW(shape, 0) = 0;
    if (SW(shape, 30) != CLUDGED) {
        SW(shape, 30) = 0;
        gfree(SL(shape, 14), SW(shape, 26));
    }
    if ((cookie = SL(shape, 18)) != 0) {
        SL(shape, 18) = 0;
        gfree(cookie, SW(shape, 22));
    }
}

/* CludgeShapes shape#,numshapes,address: d0 = shape#, a0 = its object,
   d1 = the number, d2 = the shapes in memory (32 bytes header + data
   each).  -1, or 0 for no shapes or missing memory. */
static ULONG BLITZ_ROUTINE loadshapes_c(ULONG shapenum BL_REG(d0), ULONG numshapes BL_REG(d1),
                                        ULONG address BL_REG(d2), UBYTE *objects BL_REG(a0))
{
    UBYTE *shapeptr = objects, *shaperam = (UBYTE *)address, *src, *dst;
    LONG numbershapes = numshapes, shapenumber = 0;
    UWORD n;

    (void)shapenum;                     /* shapestart: d0/32, never used */
    if (!numshapes)
        return 0;
    do {
        if (SW(shapeptr, 0))
            freeshapesm(shapeptr);
        bytespershape &= 0xffff;        /* MOVE.w #0,bytespershape */
        if (SW(shaperam, 0) && SW(shaperam, 2)) {
            /* calcsize, _allocshapemem */
            bytespershape = bl_mulu_w(SW(shaperam, 22), SW(shaperam, 4));
            SL(shapeptr, 14) = galloc(bytespershape, MEMF_CHIP);
            if (!SL(shapeptr, 14))
                return 0;
            /* the object without +14 (data) and +18 (cookie) */
            SL(shapeptr, 0) = SL(shaperam, 0);
            SL(shapeptr, 4) = SL(shaperam, 4);
            SL(shapeptr, 8) = SL(shaperam, 8);
            SW(shapeptr, 12) = SW(shaperam, 12);
            SW(shapeptr, 22) = SW(shaperam, 22);
            SL(shapeptr, 24) = SL(shaperam, 24);
            SL(shapeptr, 28) = SL(shaperam, 28);
            src = shaperam + 32;
            dst = (UBYTE *)SL(shapeptr, 14);
            n = (UWORD)(bytespershape - 1);
            do
                *dst++ = *src++;
            while (n-- != 0);
            SL(shapeptr, 18) = 0;
            if (cookeeemode)
                createcookie(shapeptr, src);
        }
        shapeptr += 32;
        shaperam += 32 + bytespershape;
        shapenumber++;
    } while (numbershapes > shapenumber);
    return (ULONG)-1;
}
BLITZ_THUNK(_loadshapes, loadshapes_c, "d1/a0-a1");

/* CludgeShapesToAddress address,numshapes,address: the shape objects at
   d0 (MOVE.l d0,a0:MOVEQ #0,d0) */
static ULONG BLITZ_ROUTINE loadshapestoaddress_c(ULONG objects BL_REG(d0),
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
    UWORD work, planebytes;             /* d0: row bytes, words, height, 2 * height; d1 */

    work = (UWORD)(((UWORD)(SW(shape, 0) + 15) >> 4) << 1);
    SW(shape, 6) = work;
    work >>= 1;
    SW(shape, 8) = (UWORD)(work + (UWORD)bl_mulu_w(SW(shape, 2), 64) + 1);
    work = SW(shape, 2);
    planebytes = (UWORD)bl_mulu_w(SW(shape, 6), work);
    SW(shape, 22) = planebytes;
    if (init)
        SW(shape, 26) = (UWORD)bl_mulu_w(planebytes, SW(shape, 4));
    work <<= 1;
    SW(shape, 24) = (UWORD)(planebytes + work);
    SW(shape, 28) = (UWORD)(bl_mulu_w(work, SW(shape, 4)) + SW(shape, 26));
}

/* LEInitShape shape#,width,height,depth: a2 = the shape; new cleared
   chip memory for the data and the cookie (not computed).  BUG(orig):
   the old cookie is not freed (#810). */
static void BLITZ_ROUTINE leinitshape_c(ULONG width BL_REG(d1), ULONG height BL_REG(d2),
                                        ULONG depth BL_REG(d3), UBYTE *shape BL_REG(a2))
{
    if (SW(shape, 0)) {
        SW(shape, 0) = 0;
        if (SW(shape, 30) != CLUDGED)
            gfree(SL(shape, 14), SW(shape, 26));
        SW(shape, 30) = 0;
        /* MOVE.l 18(a2),a1:BEQ - the Z of the CLR: never freed */
    }
    SW(shape, 0) = (UWORD)width;
    SW(shape, 2) = (UWORD)height;
    SW(shape, 4) = (UWORD)depth;
    SL(shape, 10) = 0;
    shapesizes(shape, 1);
    SL(shape, 14) = galloc(SW(shape, 26), MEMF_CHIP | MEMF_CLEAR);
    if (!SL(shape, 14))
        return;
    SL(shape, 18) = galloc(SW(shape, 22), MEMF_CHIP | MEMF_CLEAR);
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
    if (SL(palette, 0)) {
        size = bl_mulu_w(SW(palette, 4), 12);
        BL_SETW(size, size + 8);        /* ADDQ #8,d0: a word addition */
        gfree(SL(palette, 0), size);
    }
    size = bl_mulu_w(colourcount, 12);
    BL_SETW(size, size + 8);
    SL(palette, 0) = galloc(size, MEMF_PUBLIC | MEMF_CLEAR);  /* BUG(orig): unchecked (#821) */
    SW(palette, 4) = colourcount;
    p = (ULONG *)SL(palette, 0);
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
        if (SW(shape, 0)) {
            SW(shape, 0) = 0;
            if (SW(shape, 30) == CLUDGED) {
                /* BUG(orig) #807: nothing pushed, the pop below takes the
                   return address (rs_pop) */
                SW(shape, 30) = 0;
                data = rs_pop(__builtin_return_address(0));
            } else {
                gfree(SL(shape, 14), SW(shape, 26));
                SW(shape, 30) = 0;
                /* the cookie: MOVE.l 18(a2),a1:BEQ after the CLR, never freed */
            }
        }
        SL(shape, 0) = SL(data, 0);     /* width, height */
        SW(shape, 4) = SW(data, 4);     /* depth */
        SL(shape, 10) = SL(data, 6);    /* handles */
        SW(shape, 26) = SW(data, 10);   /* bytes of the shape */
        data += 12;
        shapesizes(shape, 0);
        if (inchip < 0) {
            /* 'decludgeshape: the data stays where it is */
            SL(shape, 14) = (ULONG)data;
            data += SW(shape, 26);
            SW(shape, 30) = CLUDGED;
        } else {
            SL(shape, 14) = galloc(SW(shape, 26), MEMF_CHIP);
            if (!SL(shape, 14))
                rs_crash(data);         /* BUG(orig): RTS with a0 pushed (#806) */
            if ((WORD)compressed < 0 && (LONG)(packedsize = SL(data, 0)) >= 0) {
                /* 'unpack_shape */
                data += 4;
                packptr = (ULONG)data;
                BL_JSR_D4_(BL_TOKVAL(TOK_unpackpbm, 0), (ULONG)data, SL(shape, 14), packedsize,
                           SW(shape, 26));
                data = (UBYTE *)(packptr + packedsize);
            } else {
                if ((WORD)compressed < 0)
                    data += 4;          /* -1: stored ('changedmind') */
                n = SW(shape, 26) - 1;
                dst = (UBYTE *)SL(shape, 14);
                do
                    *dst++ = *data++;
                while (n-- != 0);
            }
        }
        /* 'dodacookie */
        SL(shape, 18) = galloc(SW(shape, 22), MEMF_CHIP | MEMF_CLEAR);
        if (!SL(shape, 18))
            rs_crash(data);             /* BUG(orig): RTS with a0 pushed (#806) */
        {
            const UBYTE *src = (const UBYTE *)SL(shape, 14);
            planes = SW(shape, 4) - 1;
            do {
                n = SW(shape, 22) - 1;
                dst = (UBYTE *)SL(shape, 18);
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

/* LECludgeShapes shape#,shape#,address,palette#[,copymemflag]: a2/a3 the
   first and last shape, a0 the palette, d2 the LE file in memory */
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
    if (SL(p, 0) != ID('F', 'O', 'R', 'M') || SL(p, 8) != ID('L', 'E', 'S', 'H')
        || SL(p, 12) != ID('S', 'H', 'H', 'D'))
        return;
    p += 20;
    shapecount = SL(p, 0);
    compressed = SW(p, 4);
    if (compressed)
        inchip = 0;
    p += 20;                            /* BUG(orig): SHHD as 20 bytes (#811) */
    if (SL(p, 0) == ID('C', 'M', 'A', 'P')) {
        chunksize = SL(p, 4);
        p += 8;
        if (paletteptr) {
            read_palette(p);
            p += bl_mulu_w(colourcount, 3);     /* a0 behind the colours */
            if (!colourcount)
                p += 3 * 65536;
        } else
            p += chunksize;
    }
    if (SL(p, 0) != ID('S', 'D', 'A', 'T'))
        return;
    readshapes(p + 8);
}
BLITZ_THUNK_PRE(_lecludgeshapes2, lecludgeshapes2_c, "d0-d1/a0-a1", "move.l sp,_rs_sp");

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
    ULONG len = SL(chunkid, 4), buf;

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
    if (SL(chunkid, 0) != ID('F', 'O', 'R', 'M')) {
        /* tryacid */
        gfree(chunkid, 8);
        Close(filehandle);
        acidloadshapes(first, last);
        return 0;
    }
    Read(fh, (APTR)buf, 4);
    if (SL(chunkid, 0) != ID('L', 'E', 'S', 'H'))
        return cleanup(FREEMEMORY);
    Read(fh, (APTR)buf, 8);
    if (SL(chunkid, 0) != ID('S', 'H', 'H', 'D'))
        return cleanup(FREEMEMORY);
    if (!(chunk = readchunk(fh)))
        return cleanup(FREEMEMORY);
    shapecount = SL(chunk, 0);
    compressed = SW(chunk, 4);
    freechunk();
    Read(fh, (APTR)buf, 8);
    if (SL(chunkid, 0) == ID('C', 'M', 'A', 'P')) {
        if (!(chunk = readchunk(fh)))
            return cleanup(FREEMEMORY);
        read_palette((const UBYTE *)chunk);
        freechunk();
        Read(fh, (APTR)buf, 8);
    }
    if (SL(chunkid, 0) != ID('S', 'D', 'A', 'T'))
        return cleanup(FREEALL);
    if (!(chunk = readchunk(fh)))
        return cleanup(FREEMEMORY);
    if (readshapes((UBYTE *)chunk))
        return 0;                       /* BUG(orig): returned past us (#807) */
    return cleanup(FREEALL);
}

/* LELoadShapes shape#,shape#,filename$,palette# (d0, d1, d2; a0 the
   palette, a2/a3 the shapes) */
static void BLITZ_ROUTINE leloadshapesp_c(ULONG shape1 BL_REG(d0), ULONG shape2 BL_REG(d1),
                                          ULONG name BL_REG(d2), UBYTE *palette BL_REG(a0),
                                          UBYTE *first BL_REG(a2), UBYTE *last BL_REG(a3),
                                          struct DosLibrary *dos BL_REG(a6))
{
    leloadshapes(shape1, shape2, name, palette, first, last, dos);
}
BLITZ_THUNK_PRE(_leloadshapes, leloadshapesp_c, "d0-d1/a0-a1", "move.l sp,_rs_sp");

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
static void BLITZ_ROUTINE leloadshapesingle_c(ULONG shapenum BL_REG(d0), ULONG name BL_REG(d1),
                                              ULONG maxshapes BL_REG(d4),
                                              UBYTE *palette BL_REG(a0), UBYTE *first BL_REG(a2),
                                              UBYTE *objects BL_REG(a3),
                                              struct DosLibrary *dos BL_REG(a6))
{
    BL_SETW(maxshapes, (UWORD)((UWORD)(maxshapes - 1) << 5));
    leloadshapes(shapenum, maxshapes, name, palette, first, objects + (WORD)maxshapes, dos);
}
BLITZ_THUNK_PRE(_leloadshapesingle, leloadshapesingle_c, "d0-d1/a0-a1", "move.l sp,_rs_sp");

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
    pal = (const UBYTE *)SL(pal, 0);
    centry = bl_mulu_w(SW(pal, 0), 3);
    chunksize = centry + 8;
    mem = galloc(chunksize, 0);
    if (!mem)
        return 0;
    p = (UBYTE *)mem;
    SL(p, 0) = ID('C', 'M', 'A', 'P');
    SL(p, 4) = centry;
    p += 8;
    centry += 8;
    n = SW(pal, 0) - 1;
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
    SL(buf, 0) = ID('S', 'D', 'A', 'T');
    SL(buf, 4) = 0;
    Write(fh, buf, 8);
    shape = startshape;
    last = endshape;
    do {
        SW(buf, 0) = SW(shape, 0);
        SW(buf, 2) = SW(shape, 2);
        SW(buf, 4) = SW(shape, 4);
        SW(buf, 6) = SW(shape, 10);
        SW(buf, 8) = SW(shape, 12);
        SW(buf, 10) = SW(shape, 26);
        Write(fh, buf, 12);
        written += 12;
        if (SW(shape, 0)) {
            if ((WORD)compress >= 0) {
                len = SW(shape, 26);
                written += len;
                Write(fh, (APTR)SL(shape, 14), len);
            } else {
                /* 'packshape: packed size.l + the data of PackPBM */
                pk = (UBYTE *)galloc(SW(shape, 26) + 4, 0);
                packsize = SW(shape, 26);
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
                packed = BL_JSR_D4_(BL_TOKVAL(TOK_packpbm, 0), SL(shape, 14), (ULONG)pk + 4,
                                    SW(shape, 26), 12);
                SL(packptr, 0) = packed;
                if (packed >= 0) {
                    written += packed + 4;
                    Write(fh, (APTR)packptr, packed + 4);
                } else {
                    /* 'compfail: -1 and the data as it is */
                    Write(fh, (APTR)packptr, 4);
                    len = SW(shape, 26);
                    written += len + 4;
                    Write(fh, (APTR)SL(shape, 14), len);
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

/* LESaveShapes shape#,shape#,filename$[,palette#] */
static void BLITZ_ROUTINE lesaveshapes_c(ULONG shape1 BL_REG(d0), ULONG shapes BL_REG(d1),
                                         ULONG name BL_REG(d2), UBYTE *palette BL_REG(a0),
                                         UBYTE *first BL_REG(a2), UBYTE *last BL_REG(a3),
                                         struct DosLibrary *dos BL_REG(a6))
{
    BPTR fh;
    ULONG written, len, result;
    UBYTE z;

    dosbase = dos;
    paletteptr = palette;
    startshape = first;
    endshape = last;
    BL_SETW(shape1, (WORD)shape1 >> 5);
    BL_SETW(shapes, (WORD)shapes >> 5);
    BL_SETW(shapes, shapes - shape1);
    shapecount = shapes;                /* BUG(orig): the upper word too (#815) */
    fh = Open((STRPTR)name, MODE_NEWFILE);
    if (!fh)
        return;
    SL(emptyheader, 20) = shapecount;
    SW(emptyheader, 24) = compress;
    result = write_z(fh, emptyheader, sizeof emptyheader, &z);
    result = savecmap(fh, result, z);
    written = writeshapes(fh, result);
    Seek(fh, -(LONG)(written - 4), OFFSET_CURRENT);
    len = written - 8;
    Write(fh, &len, 4);
    written += centry + 40;             /* BUG(orig): 8 too many (#816) */
    Seek(fh, 4, OFFSET_BEGINNING);
    len = written;
    Write(fh, &len, 4);
    Seek(fh, 20, OFFSET_BEGINNING);
    Write(fh, &shapecount, 4);
    Close(fh);
}
BLITZ_THUNK(_lesaveshapes, lesaveshapes_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE lesaveshapesnop_c(ULONG shape1 BL_REG(d0), ULONG shapes BL_REG(d1),
                                            ULONG name BL_REG(d2), UBYTE *first BL_REG(a2),
                                            UBYTE *last BL_REG(a3),
                                            struct DosLibrary *dos BL_REG(a6))
{
    lesaveshapes_c(shape1, shapes, name, NULL, first, last, dos);
}
BLITZ_THUNK(_lesaveshapesnop, lesaveshapesnop_c, "d0-d1/a0-a1");
