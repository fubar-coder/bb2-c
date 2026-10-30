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

#define ID_FORM 0x464f524dUL
#define ID_ANIM 0x414e494dUL
#define ID_ILBM 0x494c424dUL
#define ID_BMHD 0x424d4844UL
#define ID_ANHD 0x414e4844UL
#define ID_CMAP 0x434d4150UL
#define ID_DLTA 0x444c5441UL

#define SIZEOF_BITMAP 64

/* InitFindChunk / FindChunk */
static UBYTE *firstchunk, *lastfoundchunk, *endoffile;

static UBYTE *firstframe, *animaddress, *animend, *nextanimframe;
static UWORD animheight, animwidth, animwidthtemp;
static UBYTE animdepth;
static ULONG *bitmapaddress;            /* the bitmap object's planes */
static ULONG animtype7, bitplaneoffset;
static UBYTE *thirdframe, *mytemp;
static UBYTE doanimloop = 0xff, bitmapnumber, palettenumber, animtype;

/* InitFindChunk(a0, id): a FORM of type id (its chunks), or a chunk
   with id at +8 */
static LONG initfindchunk(UBYTE *a0, ULONG id)
{
    if (LG(a0) == ID_FORM) {
        if (LG(a0 + 8) != id)
            return 0;
        firstchunk = lastfoundchunk = a0 + 12;
        endoffile = a0 + 12 + LG(a0 + 4) - 4;
        return -1;
    }
    if (LG(a0 + 8) != id)
        return 0;
    /* DIFF: the original returns to the caller's a1 here (BUG(orig), #513) */
    firstchunk = lastfoundchunk = a0;
    endoffile = a0 + LG(a0 + 4) + 8;
    return -1;
}

/* fcloop: the chunk id from p on up to endoffile; *a2 = its data */
static LONG fcloop(UBYTE *p, ULONG id, UBYTE **a2)
{
    for (;;) {
        if ((LONG)p >= (LONG)endoffile) {
            *a2 = p;
            return 0;
        }
        if (LG(p) == id)
            break;
        p += LG(p + 4) + 8;             /* BUG(orig): no pad byte (#505) */
    }
    lastfoundchunk = p;
    *a2 = p + 8;
    return -1;
}

static LONG findchunk(ULONG id, UBYTE **a2)
{
    return fcloop(firstchunk, id, a2);
}

static LONG findnextchunk(ULONG id, UBYTE **a2)
{
    return fcloop(lastfoundchunk + LG(lastfoundchunk + 4) + 8, id, a2);
}

/* findbmhd: the BMHD of the first frame of the ANIM at address */
static LONG findbmhd(ULONG address, UBYTE **a2)
{
    if (!initfindchunk((UBYTE *)address, ID_ANIM))
        return 0;
    if (!initfindchunk(firstchunk, ID_ILBM))
        return 0;
    return findchunk(ID_BMHD, a2);
}

/* RIAnimDepth / RIAnimWidth / RIAnimHeight (address) */
ULONG BLITZ_ROUTINE c_AnimDepth(ULONG address BL_REG(d0))
{
    UBYTE *a2;

    if (!findbmhd(address, &a2))
        return 0;
    return a2[8];
}

ULONG BLITZ_ROUTINE c_AnimWidth(ULONG address BL_REG(d0))
{
    UBYTE *a2;

    if (!findbmhd(address, &a2))
        return 0;
    return 0xffff0000UL | WD(a2);       /* MOVE.w into d0 = -1 */
}

ULONG BLITZ_ROUTINE c_AnimHeight(ULONG address BL_REG(d0))
{
    UBYTE *a2;

    if (!findbmhd(address, &a2))
        return 0;
    return 0xffff0000UL | WD(a2 + 2);
}

/* AnimLoop On|Off */
void BLITZ_ROUTINE c_AnimLoop(UBYTE on BL_REG(d0))
{
    doanimloop = on ? 0xff : 0;
}

/* RIAnimInit (address,bitmap#,palette#[,offset]): -1, 0 if no ANIM with
   an ILBM and BMHD */
ULONG BLITZ_ROUTINE c_InitAnim(ULONG address BL_REG(d0), ULONG bm BL_REG(d1),
                               ULONG pal BL_REG(d2), ULONG offset BL_REG(d3))
{
    UBYTE *a2;
    struct bl_regs r;

    bitmapnumber = (UBYTE)bm;
    palettenumber = (UBYTE)pal;
    bitplaneoffset = offset;
    if (!initfindchunk((UBYTE *)address, ID_ANIM))
        return 0;
    firstframe = firstchunk;
    animend = endoffile;
    animaddress = (UBYTE *)address;
    if (!initfindchunk(firstchunk, ID_ILBM))
        return 0;
    endoffile = animend;                /* BUG(orig): the whole ANIM (#511) */
    if (!findchunk(ID_BMHD, &a2))
        return 0;
    animwidth = WD(a2);
    animheight = WD(a2 + 2);
    animdepth = a2[8];
    BL_JSR_D4_(BL_TOKVAL(TOK_unpackiff, 1), (ULONG)firstframe, bitmapnumber, (ULONG)-1,
               bitplaneoffset);
    r = (struct bl_regs){ .d0 = (ULONG)firstframe, .d1 = palettenumber };
    BLITZ_ALIBJSR(TOK_ilbmpalette, r);
    /* safety nets for short anims :) */
    nextanimframe = thirdframe = lastfoundchunk = firstframe;
    if (findnextchunk(ID_FORM, &a2)) {  /* the second frame */
        nextanimframe = a2 - 8;
        if (findnextchunk(ID_FORM, &a2))        /* the third frame */
            thirdframe = a2 - 8;
    }
    return (ULONG)-1;
}

ULONG BLITZ_ROUTINE c_InitAnim2(ULONG address BL_REG(d0), ULONG bm BL_REG(d1),
                                ULONG pal BL_REG(d2))
{
    return c_InitAnim(address, bm, pal, 0);
}

/* RIAnimInit (address,bitmap#,palette#,x,y): a0 = the bitmap object, d1
   = bitmap# * 64 (#pd1) */
ULONG BLITZ_ROUTINE c_InitAnim3(ULONG address BL_REG(d0), ULONG bm BL_REG(d1),
                                ULONG pal BL_REG(d2), ULONG x BL_REG(d3), ULONG y BL_REG(d4),
                                const UWORD *bitmap BL_REG(a0))
{
    ULONG off = bl_mulu_w(*bitmap, (UWORD)y) + (ULONG)((LONG)x >> 3);

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
static void plane5(const UBYTE *a0, UBYTE *a1, ULONG mod)
{
    UWORD col = animwidthtemp, c;
    UBYTE *a2;
    UBYTE n, op, v;

    do {
        a2 = a1++;
        n = *a0++;
        if (!n)
            continue;
        do {
            op = *a0++;
            if (!op) {                  /* run */
                c = COUNT(*a0++);
                v = *a0++;
                do {
                    *a2 = v;
                    a2 += mod;
                    __asm__("" : "+a"(a2));     /* no __mulsi3 */
                } while (c-- != 0);
            } else if (op & 0x80) {     /* copy */
                c = COUNT(op & 0x7f);
                do {
                    *a2 = *a0++;
                    a2 += mod;
                    __asm__("" : "+a"(a2));     /* no __mulsi3 */
                } while (c-- != 0);
            } else                      /* skip */
                a2 += bl_mulu_w(op, (UWORD)mod);
        } while (--n);
    } while (col-- != 0);
}

/* ANIM7 (long form): one plane, columns of longs; the data from a3 */
static void plane7(const UBYTE *a0, UBYTE *a1, const ULONG *a3, ULONG mod)
{
    UWORD col = animwidthtemp, c;
    UBYTE *a2;
    UBYTE n, op;
    ULONG v;

    do {
        a2 = a1;
        a1 += 4;
        n = *a0++;
        if (!n)
            continue;
        do {
            op = *a0++;
            if (!op) {                  /* run */
                c = COUNT(*a0++);
                v = *a3++;
                do {
                    LG(a2) = v;
                    a2 += mod;
                    __asm__("" : "+a"(a2));     /* no __mulsi3 */
                } while (c-- != 0);
            } else if (op & 0x80) {     /* copy */
                c = COUNT(op & 0x7f);
                do {
                    LG(a2) = *a3++;
                    a2 += mod;
                    __asm__("" : "+a"(a2));     /* no __mulsi3 */
                } while (c-- != 0);
            } else                      /* skip */
                a2 += bl_mulu_w(op, (UWORD)mod);
        } while (--n);
    } while (col-- != 0);
}

/* the DLTA (header at dlta) into the planes of the bitmap */
static void unpackanim(UBYTE *dlta, BOOL seven)
{
    UBYTE *a4 = dlta + 8;
    const ULONG *a5 = (const ULONG *)a4;
    ULONG *a6 = bitmapaddress;
    ULONG mod = WD((UBYTE *)a6 - 8);    /* the bitmap's ebwidth */
    UBYTE d4 = 0;

    animwidthtemp = (UWORD)(((WORD)animwidth >> (seven ? 5 : 3)) - 1);
    do {                                /* BUG(orig): up to the byte (#509) */
        UBYTE *a1 = (UBYTE *)*a6++ + bitplaneoffset;
        ULONG op = a5[0], data = a5[8];

        a5++;
        if (op) {
            if (seven)
                plane7(a4 + op, a1, (const ULONG *)(a4 + data), mod);
            else
                plane5(a4 + op, a1, mod);
        }
    } while (++d4 != animdepth);
}

/* RINextAnimFrame (bitmap#) with a0 = the bitmap object: -1 if a delta
   was applied.  The result in the upper half, the lower 1 if the
   original returns without restoring its registers (no DLTA). */
unsigned long long BLITZ_ROUTINE c_NextAnimFrame(ULONG *bitmap BL_REG(a0))
{
    UBYTE *a0 = nextanimframe;
    struct bl_regs r;
    ULONG len;

    if (!a0)
        return 0;
    bitmapaddress = bitmap + 2;
    if ((LONG)a0 >= (LONG)animend || LG(a0) != ID_FORM) {
        if (!doanimloop)
            return 0;
        a0 = thirdframe;
    }
    len = LG(a0 + 4);
    a0 += 8;
    nextanimframe = a0 + len;
    a0 += 4;                            /* skip ILBM */
    if (LG(a0) != ID_ANHD)
        return 0;
    animtype = a0[8];
    animtype7 = LG(a0 + 28);
    a0 += LG(a0 + 4) + 8;
    if (LG(a0) == ID_CMAP) {
        mytemp = a0;
        r = (struct bl_regs){ .d0 = (ULONG)a0, .d1 = palettenumber };
        BLITZ_ALIBJSR(TOK_ilbmpalette, r);
        a0 = mytemp;
        a0 += LG(a0 + 4) + 8;
    }
    if (LG(a0) != ID_DLTA)
        return 1;                       /* BUG(orig): RTS without MOVEM (#515) */
    if (animtype == 5 || animtype == 65)
        unpackanim(a0, FALSE);
    else if (animtype == 7)
        unpackanim(a0, TRUE);           /* BUG(orig): never the short form (#504) */
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
