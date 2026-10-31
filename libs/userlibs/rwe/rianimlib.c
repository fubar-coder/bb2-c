/*
 * rianimlib - userlib 57 (RWE, Reflective Images RIAnimLib, Stephen
 * McNamara 1994/95): ANIM5/ANIM7 animations in memory -
 * RIAnimInit, RINextAnimFrame, AnimLoop, RIAnimFrameCount, RIAnimDepth,
 * RIAnimWidth, RIAnimHeight.
 *
 * C translation of rwelibsrc/ANIMLib/Anim-Library3.bb (BB22 format,
 * converted by tools/bb22) without RIDiskAnimInit: the source does not
 * compile (that unfinished command uses diskanimflag and animfile, which
 * are defined nowhere), and the CD's BSS2 RIANIMLIB.OBJ is exactly the
 * source without its scaffolding (its declaration, the data longs bmap
 * ... ilbmbuffer, oops and the routine; docs/rwelibs/README.md 3.2).
 * The declarations equal that binary (ctest rwe_declarations_rianimlib),
 * the behaviour is compared with it in libs/tests/rwe/rianimlib.c.
 * Notes: docs/rwelibs/port/rianimlib.md.
 *
 * RIAnimInit finds the first frame (FORM ILBM in the FORM ANIM), its
 * BMHD (searched up to the end of the ANIM), unpacks it with
 * RIPackLib's UnpackIFF (form 1: d0 source, d1 bitmap#, d2 -1 lines,
 * d3 offset) and ILBMPalette (d0 source, d1 palette#) through ALibJsr,
 * and remembers the second and third frame.  RINextAnimFrame applies the
 * DLTA of the next frame (ANHD operation 5 or 65: byte vertical delta,
 * 7: long vertical delta) to the bitmap's planes (+ the offset), a CMAP
 * of the frame through ILBMPalette; at the end it goes on with the third
 * frame if AnimLoop is on (the default).  The chunk search is the
 * InitFindChunk/FindChunk code of RIPackLib (Pack-Library9).
 *
 * C behind thunks; RINextAnimFrame's thunk is assembler for its stack
 * (BUG(orig) below, #515).  The thunks keep d1/a0/a1 (GCC the rest); the
 * original changes d1, d3, d6, d7 and a2 in RIAnimInit, a0/a2 in
 * RIAnimDepth/Width/Height, a0/a1 in RIAnimFrameCount.
 *
 * BUG(orig):
 *   - InitFindChunk on data that is not a FORM but has the ID at +8 (a
 *     chunk "pointed at directly") pushes a1 and returns without popping
 *     it: the RTS jumps to the caller's a1 (DIFF: the C build returns
 *     -1 as if a1 had been popped - a crash in the original, not
 *     reproducible) (#513).
 *   - RINextAnimFrame returns without restoring its 14 saved registers
 *     when a frame has no DLTA after ANHD (and CMAP): its RTS jumps to
 *     the saved d1 (reproduced by the thunk) (#515).
 *   - The chunk walks (FindChunk, RIAnimFrameCount, the frame pointers)
 *     add the chunk length + 8 without the pad byte of odd lengths
 *     (#505) and compare addresses signed (#506); RIAnimFrameCount counts
 *     every chunk of the ANIM, not only the frames (#510).
 *   - RIAnimInit searches the BMHD up to the end of the ANIM, not of the
 *     first frame (#511); it keeps the ANIM's address and end even if no
 *     ILBM or BMHD is found (RIAnimFrameCount then counts), and the frame
 *     pointers of the animation before (#512).
 *   - RIAnimInit x,y form: the offset is ebwidth * y (MULU.w: the low
 *     words) + x >> 3 (ASR.l); DIVU.w #64 of the bitmap# overflows (no
 *     change) for numbers of 64 * 65536 and above (#514).
 *   - ANIM7: the test for the short (word) form tests the operation (7),
 *     so the long form is always used (the short code is unreachable and
 *     not ported) (#504); the columns are width >> 5 (ASR.w), dropping
 *     the rest of a width that is not a multiple of 32, and a width below
 *     32 gives 65536 columns (DBRA); ANIM5 likewise with width >> 3
 *     (#507).
 *   - The planes loop runs until the counter (a byte) equals the depth:
 *     depth 0 is 256 planes; more than 8 read behind the bitmap's plane
 *     pointers (#509).  A run or copy count of 0 is 256 (DBRA on the
 *     byte) (#508).
 * Not ported (no effect): GetChunkHeader (called by nothing), _libfinit
 * (an RTS no header refers to), the short ANIM7 code.
 */
#include "blitzlib.h"
#include <graphics/gfx.h>
#include <datatypes/pictureclass.h>
#include <stddef.h>

/* RIPackLib (73): the token list of the CD's BSS2 RIPACKLIB.OBJ */
#define TOK_unpackiff   ((LIB_ripacklib << 7) + 3)   /* form 1 */
#define TOK_ilbmpalette ((LIB_ripacklib << 7) + 4)

/* entry points (thunks, defined below) */
void _InitAnim2(void), _InitAnim(void), _InitAnim3(void), _NextAnimFrame(void);
void _AnimLoop(void), _CountFrames(void), _AnimDepth(void), _AnimWidth(void);
void _AnimHeight(void);

#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))   /* #ia<a>|#pd<p> */

/* !libheader {#rianimlib,0,0,0,0} */
BLITZ_LIBHEADER(LIB_rianimlib, 0, BL_NORET, 0, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_InitAnim2, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_InitAnim, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, IA(0, 1));                /* !bmap {0,1} */
BLITZ_SUBS(_InitAnim3, 0, 0);
BLITZ_NAME("RIAnimInit", "(address,bitmap#,palette# [,xy_offset]|[,x,y])");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS(LIB_bitmaplib, IA(0, 0));                /* !bmap {0,0} */
BLITZ_SUBS(_NextAnimFrame, 0, 0);
BLITZ_NAME("RINextAnimFrame", "(bitmap#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_AnimLoop, 0, 0);
BLITZ_NAME("AnimLoop", "On|Off");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_CountFrames, 0, 0);
BLITZ_NAME("RIAnimFrameCount", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_AnimDepth, 0, 0);
BLITZ_NAME("RIAnimDepth", "address -- address of your animation file");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_AnimWidth, 0, 0);
BLITZ_NAME("RIAnimWidth", "address -- address of your animation file");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_AnimHeight, 0, 0);
BLITZ_NAME("RIAnimHeight", "address -- address of your animation file");

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

#define LG(p) (*(ULONG *)(p))
#define WD(p) (*(UWORD *)(p))

#define ID_ANIM 0x414e494dUL
#define ID_ANHD 0x414e4844UL
#define ID_DLTA 0x444c5441UL

#define SIZEOF_BITMAP 64

static_assert(sizeof(struct BitMapHeader) == 20);

/* the data of an ANHD chunk (the ANIM's AnimHeader; the original reads
   the operation and the bits) */
struct BL_PACKED anhd {
    UBYTE operation;                    /* +0: 5, 65 (byte delta), 7 */
    UBYTE mask;
    UWORD w, h;
    WORD x, y;
    ULONG abstime, reltime;             /* +10, +14 */
    UBYTE interleave, pad0;
    ULONG bits;                         /* +20: ANIM7 options */
    UBYTE pad[16];
};
static_assert(sizeof(struct anhd) == 40);

/* InitFindChunk / FindChunk */
static UBYTE *firstchunk, *lastfoundchunk, *endoffile;

static UBYTE *firstframe, *animaddress, *animend, *nextanimframe;
static UWORD animheight, animwidth;
static UWORD deltacolumns;              /* a delta's columns per plane - 1 (DBRA) */
static UBYTE animdepth;
static PLANEPTR *bitmapaddress;         /* the Planes of the bitmap object */
static ULONG animtype7, bitplaneoffset;
static UBYTE *thirdframe;
static UBYTE *cmapchunk;                /* the CMAP chunk over the ILBMPalette call */
static UBYTE doanimloop = 0xff, bitmapnumber, palettenumber, animtype;

/* InitFindChunk(a0, id): a FORM of type id (its chunks), or a chunk
   with id at +8 */
static LONG initfindchunk(UBYTE *chunk, ULONG id)
{
    if (LG(chunk) == ID_FORM) {
        if (LG(chunk + 8) != id)
            return 0;
        firstchunk = lastfoundchunk = chunk + 12;
        endoffile = chunk + 12 + LG(chunk + 4) - 4;
        return -1;
    }
    if (LG(chunk + 8) != id)
        return 0;
    /* DIFF: the original returns to the caller's a1 here (BUG(orig), #513) */
    firstchunk = lastfoundchunk = chunk;
    endoffile = chunk + LG(chunk + 4) + 8;
    return -1;
}

/* fcloop: the chunk id from p on up to endoffile; *a2 = its data */
static LONG fcloop(UBYTE *p, ULONG id, UBYTE **data)
{
    for (;;) {
        if ((LONG)p >= (LONG)endoffile) {
            *data = p;
            return 0;
        }
        if (LG(p) == id)
            break;
        p += LG(p + 4) + 8;             /* BUG(orig): no pad byte (#505) */
    }
    lastfoundchunk = p;
    *data = p + 8;
    return -1;
}

static LONG findchunk(ULONG id, UBYTE **data)
{
    return fcloop(firstchunk, id, data);
}

static LONG findnextchunk(ULONG id, UBYTE **data)
{
    return fcloop(lastfoundchunk + LG(lastfoundchunk + 4) + 8, id, data);
}

/* findbmhd: the BMHD of the first frame of the ANIM at address */
static LONG findbmhd(ULONG address, struct BitMapHeader **data)
{
    if (!initfindchunk((UBYTE *)address, ID_ANIM))
        return 0;
    if (!initfindchunk(firstchunk, ID_ILBM))
        return 0;
    return findchunk(ID_BMHD, (UBYTE **)data);
}

/* RIAnimDepth / RIAnimWidth / RIAnimHeight (address) */
ULONG BLITZ_ROUTINE c_AnimDepth(ULONG address BL_REG(d0))
{
    struct BitMapHeader *bmhd;

    if (!findbmhd(address, &bmhd))
        return 0;
    return bmhd->bmh_Depth;
}

ULONG BLITZ_ROUTINE c_AnimWidth(ULONG address BL_REG(d0))
{
    struct BitMapHeader *bmhd;

    if (!findbmhd(address, &bmhd))
        return 0;
    return 0xffff0000UL | bmhd->bmh_Width;      /* MOVE.w into d0 = -1 */
}

ULONG BLITZ_ROUTINE c_AnimHeight(ULONG address BL_REG(d0))
{
    struct BitMapHeader *bmhd;

    if (!findbmhd(address, &bmhd))
        return 0;
    return 0xffff0000UL | bmhd->bmh_Height;
}

/* AnimLoop On|Off */
void BLITZ_ROUTINE c_AnimLoop(UBYTE on BL_REG(d0))
{
    doanimloop = on ? 0xff : 0;
}

/* RIAnimInit (address,bitmap#,palette#[,offset]): -1, 0 if no ANIM with
   an ILBM and BMHD */
BB_BOOL BLITZ_ROUTINE c_InitAnim(ULONG address BL_REG(d0), ULONG bm BL_REG(d1),
                                 ULONG pal BL_REG(d2), ULONG offset BL_REG(d3))
{
    UBYTE *chunk;
    struct BitMapHeader *bmhd;
    struct bl_regs r;

    bitmapnumber = (UBYTE)bm;
    palettenumber = (UBYTE)pal;
    bitplaneoffset = offset;
    if (!initfindchunk((UBYTE *)address, ID_ANIM))
        return BB_FALSE;
    firstframe = firstchunk;
    animend = endoffile;
    animaddress = (UBYTE *)address;
    if (!initfindchunk(firstchunk, ID_ILBM))
        return BB_FALSE;
    endoffile = animend;                /* BUG(orig): the whole ANIM (#511) */
    if (!findchunk(ID_BMHD, &chunk))
        return BB_FALSE;
    bmhd = (struct BitMapHeader *)chunk;
    animwidth = bmhd->bmh_Width;
    animheight = bmhd->bmh_Height;
    animdepth = bmhd->bmh_Depth;
    BL_JSR_D4_(BL_TOKVAL(TOK_unpackiff, 1), (ULONG)firstframe, bitmapnumber, (ULONG)-1,
               bitplaneoffset);
    r = (struct bl_regs){ .d0 = (ULONG)firstframe, .d1 = palettenumber };
    BLITZ_ALIBJSR(TOK_ilbmpalette, r);
    /* safety nets for short anims :) */
    nextanimframe = thirdframe = lastfoundchunk = firstframe;
    if (findnextchunk(ID_FORM, &chunk)) {  /* the second frame */
        nextanimframe = chunk - 8;
        if (findnextchunk(ID_FORM, &chunk))     /* the third frame */
            thirdframe = chunk - 8;
    }
    return BB_TRUE;
}

BB_BOOL BLITZ_ROUTINE c_InitAnim2(ULONG address BL_REG(d0), ULONG bm BL_REG(d1),
                                  ULONG pal BL_REG(d2))
{
    return c_InitAnim(address, bm, pal, 0);
}

/* RIAnimInit (address,bitmap#,palette#,x,y): a0 = the bitmap object, d1
   = bitmap# * 64 (#pd1) */
BB_BOOL BLITZ_ROUTINE c_InitAnim3(ULONG address BL_REG(d0), ULONG bm BL_REG(d1),
                                  ULONG pal BL_REG(d2), ULONG x BL_REG(d3), ULONG y BL_REG(d4),
                                  const struct BitMap *bitmap BL_REG(a0))
{
    ULONG off = bl_mulu_w(bitmap->BytesPerRow, (UWORD)y) + (ULONG)((LONG)x >> 3);

    return c_InitAnim(address, bl_divu_w(bm, SIZEOF_BITMAP), pal, off);
}

/* RIAnimFrameCount: the chunks of the ANIM (BUG(orig): all of them, #510) */
ULONG BLITZ_ROUTINE c_CountFrames(void)
{
    UBYTE *p;
    ULONG n = 0;

    if (!animaddress)
        return 0;
    for (p = animaddress + 12; (LONG)animend > (LONG)p; p += LG(p + 4) + 8)
        n++;
    return n;
}

/* ---------------------------------------------------------- the deltas */

/* the count of a run or copy: DBRA on the byte count - 1 (0 = 256) */
#define COUNT(c) ((UWORD)(UBYTE)((c) - 1))

/* ANIM5: one plane, columns of bytes */
static void plane5(const UBYTE *ops, UBYTE *column, ULONG mod)
{
    UWORD col = deltacolumns, c;
    UBYTE *dst;
    UBYTE n, op, v;

    do {
        dst = column++;
        n = *ops++;
        if (!n)
            continue;
        do {
            op = *ops++;
            if (!op) {                  /* run */
                c = COUNT(*ops++);
                v = *ops++;
                do {
                    *dst = v;
                    dst += mod;
                    __asm__("" : "+a"(dst));    /* no __mulsi3 */
                } while (c-- != 0);
            } else if (op & 0x80) {     /* copy */
                c = COUNT(op & 0x7f);
                do {
                    *dst = *ops++;
                    dst += mod;
                    __asm__("" : "+a"(dst));    /* no __mulsi3 */
                } while (c-- != 0);
            } else                      /* skip */
                dst += bl_mulu_w(op, (UWORD)mod);
        } while (--n);
    } while (col-- != 0);
}

/* ANIM7 (long form): one plane, columns of longs; the data from a3 */
static void plane7(const UBYTE *ops, UBYTE *column, const ULONG *data, ULONG mod)
{
    UWORD col = deltacolumns, c;
    UBYTE *dst;
    UBYTE n, op;
    ULONG v;

    do {
        dst = column;
        column += 4;
        n = *ops++;
        if (!n)
            continue;
        do {
            op = *ops++;
            if (!op) {                  /* run */
                c = COUNT(*ops++);
                v = *data++;
                do {
                    LG(dst) = v;
                    dst += mod;
                    __asm__("" : "+a"(dst));    /* no __mulsi3 */
                } while (c-- != 0);
            } else if (op & 0x80) {     /* copy */
                c = COUNT(op & 0x7f);
                do {
                    LG(dst) = *data++;
                    dst += mod;
                    __asm__("" : "+a"(dst));    /* no __mulsi3 */
                } while (c-- != 0);
            } else                      /* skip */
                dst += bl_mulu_w(op, (UWORD)mod);
        } while (--n);
    } while (col-- != 0);
}

/* the DLTA (header at dlta) into the planes of the bitmap */
static void unpackanim(UBYTE *dlta, BOOL seven)
{
    UBYTE *body = dlta + 8;
    const ULONG *offsets = (const ULONG *)body;
    PLANEPTR *planes = bitmapaddress;
    /* the bitmap's ebwidth: BytesPerRow, 8 bytes in front of Planes */
    ULONG mod = WD((UBYTE *)planes - offsetof(struct BitMap, Planes));
    UBYTE plane = 0;

    deltacolumns = (UWORD)(((WORD)animwidth >> (seven ? 5 : 3)) - 1);
    do {                                /* BUG(orig): up to the byte (#509) */
        UBYTE *column = *planes++ + bitplaneoffset;
        ULONG op = offsets[0], data = offsets[8];

        offsets++;
        if (op) {
            if (seven)
                plane7(body + op, column, (const ULONG *)(body + data), mod);
            else
                plane5(body + op, column, mod);
        }
    } while (++plane != animdepth);
}

/* RINextAnimFrame (bitmap#) with a0 = the bitmap object: -1 if a delta
   was applied.  The result in the upper half, the lower 1 if the
   original returns without restoring its registers (no DLTA). */
unsigned long long BLITZ_ROUTINE c_NextAnimFrame(struct BitMap *bitmap BL_REG(a0))
{
    UBYTE *chunk = nextanimframe;
    const struct anhd *anhd;
    struct bl_regs r;
    ULONG len;

    if (!chunk)
        return 0;
    bitmapaddress = bitmap->Planes;
    if ((LONG)chunk >= (LONG)animend || LG(chunk) != ID_FORM) {
        if (!doanimloop)
            return 0;
        chunk = thirdframe;
    }
    len = LG(chunk + 4);
    chunk += 8;
    nextanimframe = chunk + len;
    chunk += 4;                         /* skip ILBM */
    if (LG(chunk) != ID_ANHD)
        return 0;
    anhd = (const struct anhd *)(chunk + 8);
    animtype = anhd->operation;
    animtype7 = anhd->bits;
    chunk += LG(chunk + 4) + 8;
    if (LG(chunk) == ID_CMAP) {
        cmapchunk = chunk;
        r = (struct bl_regs){ .d0 = (ULONG)chunk, .d1 = palettenumber };
        BLITZ_ALIBJSR(TOK_ilbmpalette, r);
        chunk = cmapchunk;
        chunk += LG(chunk + 4) + 8;
    }
    if (LG(chunk) != ID_DLTA)
        return 1;                       /* BUG(orig): RTS without MOVEM (#515) */
    if (animtype == 5 || animtype == 65)
        unpackanim(chunk, FALSE);
    else if (animtype == 7)
        unpackanim(chunk, TRUE);        /* BUG(orig): never the short form (#504) */
    else
        return 0;
    return 0xffffffffULL << 32;
}

/* entries: keep every one of d0/d1/a0/a1 that is not the result */
BLITZ_THUNK(_InitAnim2, c_InitAnim2, "d1/a0-a1");
BLITZ_THUNK(_InitAnim, c_InitAnim, "d1/a0-a1");
BLITZ_THUNK(_InitAnim3, c_InitAnim3, "d1/a0-a1");
BLITZ_THUNK(_AnimLoop, c_AnimLoop, "d0-d1/a0-a1");
BLITZ_THUNK(_CountFrames, c_CountFrames, "d1/a0-a1");
BLITZ_THUNK(_AnimDepth, c_AnimDepth, "d1/a0-a1");
BLITZ_THUNK(_AnimWidth, c_AnimWidth, "d1/a0-a1");
BLITZ_THUNK(_AnimHeight, c_AnimHeight, "d1/a0-a1");

/* RINextAnimFrame: d1-d7/a0-a6 saved as by the original; restored unless
   the C function says the original does not (d1 = 1: the RTS takes the
   saved d1 as its return address, BUG(orig), #515) */
__asm__(
"   .text\n"
"   .even\n"
"   .globl  __NextAnimFrame\n"
"__NextAnimFrame:\n"
"   movem.l d1-d7/a0-a6,-(sp)\n"
"   jsr     _c_NextAnimFrame\n"
"   tst.l   d1\n"
"   bne.s   1f\n"
"   movem.l (sp)+,d1-d7/a0-a6\n"
"1: rts\n");
