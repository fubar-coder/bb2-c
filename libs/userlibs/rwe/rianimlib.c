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
 * BMHD (in that frame), unpacks it with
 * RIPackLib's UnpackIFF (form 1: d0 source, d1 bitmap#, d2 -1 lines,
 * d3 offset) and ILBMPalette (d0 source, d1 palette#) through ALibJsr,
 * and remembers the second and third frame.  RINextAnimFrame applies the
 * DLTA of the next frame (ANHD operation 5 or 65: byte vertical delta,
 * 7: long vertical delta) to the bitmap's planes (+ the offset), a CMAP
 * of the frame through ILBMPalette; at the end it goes on with the third
 * frame if AnimLoop is on (the default).  The chunk search is the
 * InitFindChunk/FindChunk code of RIPackLib (Pack-Library9).
 *
 * C behind thunks, the ANIM7 plane decoder an asm block (ANIM7_PLANE:
 * C stayed behind the original).  The thunks keep d1/a0/a1 (GCC the
 * rest); the original changes d1, d3, d6, d7 and a2 in RIAnimInit, a0/a2
 * in RIAnimDepth/Width/Height, a0/a1 in RIAnimFrameCount.
 *
 * BUG(orig):
 *   - InitFindChunk on data that is not a FORM but has the ID at +8 (a
 *     chunk "pointed at directly") pushes a1 and returns without popping
 *     it: the RTS jumps to the caller's a1 (DIFF: the C build returns
 *     -1 as if a1 had been popped - a crash in the original, not
 *     reproducible) (#513).
 *   - ANIM7 (long data): the columns are width >> 5 (ASR.w), dropping
 *     the rest of a width that is not a multiple of 32, and a width below
 *     32 gives 65536 columns (DBRA) (#1115; what the last long column of
 *     a row with an odd number of words holds is not in the format's
 *     description).
 *   - A run or copy count of 0 is 256 (DBRA on the byte) (#508; kept on
 *     purpose: encoders never write a count of 0 - the ANIM format does
 *     not define it, the reference decoders read it as 0 rows - and a
 *     test would cost every run and copy op of RINextAnimFrame, D-007;
 *     user decision W5 Q4 of 2026-11-19).
 *   - The ops of a DLTA's columns (and the ANIM7 data they take) are not
 *     bounded by the chunk: a corrupt op list reads on behind the DLTA and
 *     writes its deltas into the planes or behind them (#1120, the part
 *     not fixed: a check per op would cost every delta op of
 *     RINextAnimFrame, the game path, D-007; user decision "C2 findings"
 *     of 2026-11-19; test rianimlib-delta-ops-past-chunk).  The plane
 *     offsets are checked (fixed, below).
 * Fixed (intended deviations, docs/libs/deviations.md):
 *   - RIAnimInit searches the BMHD in the first frame only; the original
 *     searched up to the end of the ANIM and took a BMHD chunk of the
 *     ANIM behind the first frame (#511).
 *   - A failed RIAnimInit leaves no animation (RIAnimFrameCount and
 *     RINextAnimFrame return 0); the original kept the ANIM's address and
 *     end when no ILBM or BMHD was found, and the frame pointers of the
 *     animation before (#512).
 *   - RIAnimFrameCount counts the FORM ILBM chunks of the ANIM, the
 *     frames; the original counted every chunk (#510).
 *   - The chunk walks compare addresses unsigned and stop at a chunk whose
 *     length passes the end; RIAnimInit ends the ANIM at the first chunk of
 *     its top level that does not fit (so RINextAnimFrame's step from
 *     frame to frame stays inside the ANIM) and fails if the first frame
 *     does not fit.  The original compared signed: a corrupt length that
 *     moved the pointer past $7FFFFFFF made it "below" the end, and the
 *     walks went on reading chunk headers from there (#506).
 *   - RINextAnimFrame skips other chunks between ANHD (and CMAP) and the
 *     DLTA inside the frame (a CMAP among them goes through ILBMPalette)
 *     and returns 0 for a frame without DLTA; the original returned
 *     without restoring its 14 saved registers, through the saved d1
 *     (#515).
 *   - RIAnimInit fails for a BMHD depth of 0 or above 8, before UnpackIFF;
 *     the original's planes loop counted a byte up to the depth: 0 gave
 *     256 planes, above 8 took plane pointers from behind the bitmap's 8
 *     and plane offsets from behind the DLTA's (#509).
 *   - ANIM5 decodes the byte columns of the word-aligned row, (width + 15)
 *     / 16 * 2; the original took width >> 3, dropping the rest of a width
 *     that is no multiple of 8 and leaving the pad byte of the row, and a
 *     width below 8 gave 65536 columns (DBRA).  RIAnimInit fails for a
 *     width of 0; the counts are computed there (#507, its ANIM5 half).
 *   - With runtime error checking RIAnimInit's x,y form refuses a position
 *     outside the bitmap: "Animation position outside the bitmap" (xychk;
 *     the original has no error check area).  The release code is
 *     unchanged: the offset is ebwidth * y (MULU.w: the low words) + x >> 3
 *     (ASR.l), a negative y or one of 65536 and above gives an offset of up
 *     to 65535 rows (#514; an invalid argument, D-005).
 *   - RIAnimInit fails (0) when UnpackIFF refuses the first frame (a
 *     bitmap too narrow, too shallow or without plane 0); the original
 *     dropped UnpackIFF's result and played the deltas into that bitmap
 *     (#1106).  The height is not checked.
 *   - init (the original has none) clears the animation and sets AnimLoop
 *     on: a second run from the editor with the library still loaded
 *     started with the run before's animation and AnimLoop (#1107).
 *   - The chunk walks (FindChunk, RIAnimFrameCount, the frame pointers,
 *     RINextAnimFrame's steps over the frame, ANHD and CMAP) add the pad
 *     byte of an odd length when it is 0; a non-zero byte there is taken
 *     as the next chunk (as ilbmifflib, #189; that chunk starts at an odd
 *     address, and RIAnimInit refuses such an ANIM, #1119).  The original
 *     added no pad byte: behind an odd-length chunk the walk was one byte
 *     off (#505).
 *   - ANIM7 decodes the short (word) form when bit 0 of the ANHD bits is
 *     clear, with the words of the row as its columns ((width + 15) /
 *     16).  The original tested the operation (always 7) instead of the
 *     bits: every ANIM7 was decoded as long data and its short code was
 *     unreachable (#504).
 *   - RINextAnimFrame skips chunks of the ANIM that are no FORM on its
 *     way to the next frame (an ANNO between two frames); the original
 *     took such a chunk as the end of the animation (#1116).  The step
 *     from frame to frame is unchanged; only the frame's FORM test that
 *     fails leads into the skip.
 *   - RINextAnimFrame's steps over ANHD and CMAP stay inside the frame: a
 *     frame too short for its ILBM and ANHD fields, or an ANHD or CMAP
 *     whose length leaves no chunk header in the frame, returns 0; the
 *     original stepped by the length and read the next chunk ID and
 *     length anywhere (#1117).
 *   - RIAnimInit refuses an ANIM in which a chunk (of its top level or
 *     inside a frame) starts at an odd address, behind an odd length
 *     without its pad byte, and FindChunk (RIAnimDepth, RIAnimWidth,
 *     RIAnimHeight) stops at such a chunk: on a 68000 the long reads of
 *     its ID and length were an address error.  One scan at RIAnimInit,
 *     nothing per frame (#1119).
 *   - RINextAnimFrame refuses (0) a frame whose DLTA leaves the frame, is
 *     shorter than its header of plane offsets (8 longs for ANIM5, 16 for
 *     ANIM7), or has an op list offset (ANIM7: or a data list offset) of
 *     a plane it plays at or behind its end; once per frame, before a
 *     plane is played.  The original took the offsets as they were and
 *     read op lists and data from anywhere (#1120).
 * Kept, without effect: the DIVU.w #64 of the x,y form overflows (no
 * change) for bitmap numbers of 65536 and above; the number is kept as a
 * byte anyway (named in #514).
 * Not ported (no effect): GetChunkHeader (called by nothing), _libfinit
 * (an RTS no header refers to).
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
void _AnimHeight(void), xychk(void);
static void codeinit(void);

BLITZ_ENTRY_DECL(init);

/* !libheader {#rianimlib,0,0,0,0}; init: fixed, #1107 (the original has
   none); error field: the error check area of xychk (fixed, #514) */
BLITZ_LIBHEADER(LIB_rianimlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_InitAnim2, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_InitAnim, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD1);           /* !bmap {0,1} */
BLITZ_SUBS(_InitAnim3, xychk, 0);                  /* check: fixed #514 */
BLITZ_NAME("RIAnimInit", "(address,bitmap#,palette# [,xy_offset]|[,x,y])");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS(LIB_bitmaplib, BL_IA0|BL_PD0);           /* !bmap {0,0} */
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

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);                  /* fixed #1107 */

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
/* the same per decoder (ANIM5, ANIM7 long, ANIM7 short), from RIAnimInit */
enum { DELTA5, DELTA7, DELTA7SHORT };
static UWORD columns[3];
static UBYTE animdepth;
static PLANEPTR *bitmapaddress;         /* the Planes of the bitmap object */
static ULONG animtype7, bitplaneoffset;
static UBYTE *thirdframe;
static UBYTE *cmapchunk;                /* the CMAP chunk over the ILBMPalette call */
static UBYTE doanimloop = 0xff, bitmapnumber, palettenumber, animtype;

/**
 * @brief init: no animation, AnimLoop on.
 *
 * Fixed (#1107): the original has no init, so a second run of a program
 * with the library still loaded (from the editor) started with the
 * animation of the run before: RIAnimFrameCount and RINextAnimFrame
 * before an RIAnimInit walked it (perhaps freed memory), and an AnimLoop
 * Off of the run before stayed off (the manual's default is on).  Clears
 * with CLR.l/ST on absolute addresses: no register changes.
 */
static void BLITZ_ROUTINE codeinit(void)
{
    animaddress = nextanimframe = thirdframe = NULL;
    doanimloop = 0xff;
}

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

/* the length of chunk data at data of length len with its pad byte:
   an odd length is rounded up when the byte behind the data is 0; a
   non-zero byte there is the next chunk, at an odd address (RIAnimInit
   refuses such an ANIM, fixed #1119) (fixed #505: the original added no
   pad byte) */
static inline __attribute__((always_inline)) ULONG padded(const UBYTE *data, ULONG len)
{
    if ((len & 1) && !data[len])
        len++;
    return len;
}

/* the chunk behind the chunk at p (p < end), NULL if its length passes
   end (fixed #506: the original stepped on, and compared signed); with
   its pad byte (fixed #505) */
static UBYTE *nextchunk(UBYTE *p, UBYTE *end)
{
    ULONG room = (ULONG)(end - p), len = LG(p + 4);

    if (room < 8 || len > room - 8)
        return NULL;
    if (len < room - 8)                 /* a pad byte inside the end */
        len = padded(p + 8, len);
    return p + 8 + len;
}

/* fcloop: the chunk id from p on up to endoffile; *a2 = its data */
static LONG fcloop(UBYTE *p, ULONG id, UBYTE **data)
{
    for (;;) {
        UBYTE *next;

        /* fixed #506: unsigned; fixed #1119: a chunk at an odd address
           (no pad byte behind an odd length) ends the walk */
        if (p >= endoffile || ((ULONG)p & 1)) {
            *data = p;
            return 0;
        }
        if (LG(p) == id)
            break;
        next = nextchunk(p, endoffile);
        if (!next) {                    /* fixed #506: past the end */
            *data = p;
            return 0;
        }
        p = next;
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
    UBYTE *p = lastfoundchunk < endoffile ? nextchunk(lastfoundchunk, endoffile) : NULL;

    if (!p) {                           /* fixed #506 */
        *data = lastfoundchunk;
        return 0;
    }
    return fcloop(p, id, data);
}

/* a chunk at an odd address inside the FORM at form (which fits the
   ANIM), stepped as RINextAnimFrame steps through a frame (fixed #1119:
   on a 68000 the long reads of its ID and length are an address error) */
static int oddchunk(UBYTE *form)
{
    UBYTE *end = form + 8 + LG(form + 4), *p;

    for (p = form + 12; p && p < end; p = nextchunk(p, end))
        if ((ULONG)p & 1)
            return 1;
    return 0;
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

/**
 * @brief RIAnimDepth address: the depth (bmh_Depth) of the ANIM's first frame.
 *
 * The BMHD is searched in the first frame (InitFindChunk of the ANIM
 * and of its first ILBM, then FindChunk); the chunk walk adds the pad
 * byte of an odd length when it is 0 (fixed #505) and stops at a chunk at
 * an odd address (fixed #1119).  It changes the chunk search state.
 * @param address  d0: the ANIM file in memory
 * @return d0: the depth (a byte), 0 without ANIM, ILBM or BMHD
 */
ULONG BLITZ_ROUTINE c_AnimDepth(ULONG address BL_REG(d0))
{
    struct BitMapHeader *bmhd;

    if (!findbmhd(address, &bmhd))
        return 0;
    return bmhd->bmh_Depth;
}

/**
 * @brief RIAnimWidth address: the width (bmh_Width) of the ANIM's first frame.
 *
 * As RIAnimDepth; MOVE.w into a d0 that is -1: the upper word is $FFFF.
 * @param address  d0: the ANIM file in memory
 * @return d0: $FFFF0000 | the width, 0 without ANIM, ILBM or BMHD
 */
ULONG BLITZ_ROUTINE c_AnimWidth(ULONG address BL_REG(d0))
{
    struct BitMapHeader *bmhd;

    if (!findbmhd(address, &bmhd))
        return 0;
    return 0xffff0000UL | bmhd->bmh_Width;      /* MOVE.w into d0 = -1 */
}

/**
 * @brief RIAnimHeight address: the height (bmh_Height) of the ANIM's first frame.
 *
 * As RIAnimWidth.
 * @param address  d0: the ANIM file in memory
 * @return d0: $FFFF0000 | the height, 0 without ANIM, ILBM or BMHD
 */
ULONG BLITZ_ROUTINE c_AnimHeight(ULONG address BL_REG(d0))
{
    struct BitMapHeader *bmhd;

    if (!findbmhd(address, &bmhd))
        return 0;
    return 0xffff0000UL | bmhd->bmh_Height;
}

/**
 * @brief AnimLoop On|Off: go on with the third frame at the end of the ANIM (default On).
 * @param on  d0: the byte of On (-1) or Off (0)
 */
void BLITZ_ROUTINE c_AnimLoop(UBYTE on BL_REG(d0))
{
    doanimloop = on ? 0xff : 0;
}

/**
 * @brief RIAnimInit address,bitmap#,palette#,xy_offset: show the first frame of an ANIM.
 *
 * Finds the first frame (FORM ILBM in the FORM ANIM) and its BMHD (in
 * that frame; fixed #511: the original searched up to the end of the
 * ANIM), unpacks it into the bitmap with RIPackLib's UnpackIFF (form
 * 1: d0 source, d1 bitmap#, d2 -1 lines, d3 offset) and sets the palette
 * with ILBMPalette (d0 source, d1 palette#), both through ALibJsr; then
 * remembers the second and third frame (the first if there are none).
 * RIAnimInit fails when UnpackIFF refuses the bitmap (fixed #1106: the
 * original dropped its result): too narrow (fewer bytes per row than the
 * frame's word-aligned row), too shallow or without plane 0; the height
 * is not checked (UnpackIFF unpacks as many rows as the bitmap has).
 * ILBMPalette's result is not looked at, as in the original.
 * A failed RIAnimInit leaves no animation: RIAnimFrameCount and
 * RINextAnimFrame return 0 (fixed #512: the original kept the ANIM's
 * address and end, and the frame pointers of the animation before).  The
 * original returns to the caller's a1 for a chunk "pointed at directly"
 * (#513, DIFF: -1 here).
 * @param address  d0: the ANIM file in memory
 * @param bm       d1: bitmap# (its low byte is kept)
 * @param pal      d2: palette# (its low byte is kept)
 * @param offset   d3: the byte offset into each plane
 * @return d0: BB_TRUE, BB_FALSE if there is no ANIM with an ILBM and BMHD,
 *         the BMHD depth is 0 or above 8 (fixed #509) or its width 0 (fixed #507),
 *         UnpackIFF refuses the bitmap (fixed #1106), or a chunk of the
 *         ANIM or of a frame starts at an odd address (fixed #1119)
 */
BB_BOOL BLITZ_ROUTINE c_InitAnim(ULONG address BL_REG(d0), ULONG bm BL_REG(d1),
                                 ULONG pal BL_REG(d2), ULONG offset BL_REG(d3))
{
    UBYTE *chunk;
    struct BitMapHeader *bmhd;
    struct bl_regs r;

    bitmapnumber = (UBYTE)bm;
    palettenumber = (UBYTE)pal;
    bitplaneoffset = offset;
    /* fixed #512: no animation until this one is found (the original kept
       the frame pointers of the one before, and the new address and end
       from here on) */
    animaddress = nextanimframe = thirdframe = NULL;
    if (!initfindchunk((UBYTE *)address, ID_ANIM))
        return BB_FALSE;
    firstframe = firstchunk;
    animend = endoffile;
    /* fixed #506: the ANIM ends at the first chunk whose length passes its
       end (so RINextAnimFrame's step from frame to frame stays inside it);
       the first frame must fit.  Fixed #1119: an ANIM with a chunk at an
       odd address, of its top level or inside a frame, is refused */
    for (chunk = firstframe; chunk < animend; ) {
        UBYTE *next;

        if ((ULONG)chunk & 1)
            return BB_FALSE;
        next = nextchunk(chunk, animend);
        if (!next) {
            animend = chunk;
            break;
        }
        if (LG(chunk) == ID_FORM && oddchunk(chunk))
            return BB_FALSE;
        chunk = next;
    }
    if (animend == firstframe)
        return BB_FALSE;
    if (!initfindchunk(firstchunk, ID_ILBM))
        return BB_FALSE;
    /* fixed #511: the BMHD of the first frame only (the original set
       endoffile to the ANIM's end here) */
    if (!findchunk(ID_BMHD, &chunk))
        return BB_FALSE;
    endoffile = animend;                /* the frames: the whole ANIM */
    bmhd = (struct BitMapHeader *)chunk;
    animwidth = bmhd->bmh_Width;
    /* fixed #507 (ANIM5): the byte columns of the word-aligned row (the
       original: width >> 3, dropping the rest; 0 columns were 65536), so
       a width of 0 is refused */
    if (!animwidth)
        return BB_FALSE;
    columns[DELTA5] = (UWORD)((((ULONG)animwidth + 15) >> 4 << 1) - 1);
    /* fixed #504: the words of the row for ANIM7's short form */
    columns[DELTA7SHORT] = (UWORD)((((ULONG)animwidth + 15) >> 4) - 1);
    /* BUG(orig) (#1115): width >> 5 (ASR.w), a width below 32 gives
       65536 columns */
    columns[DELTA7] = (UWORD)(((WORD)animwidth >> 5) - 1);
    animheight = bmhd->bmh_Height;
    animdepth = bmhd->bmh_Depth;
    /* fixed #509: the delta planes loop counts a byte up to the depth;
       0 was 256 planes, above 8 behind the bitmap's plane pointers */
    if (!animdepth || animdepth > 8)
        return BB_FALSE;
    /* fixed #1106: UnpackIFF's result (the original dropped it): a bitmap
       too narrow, too shallow or without plane 0 fails RIAnimInit */
    if (!BL_JSR_D4_(BL_TOKVAL(TOK_unpackiff, 1), (ULONG)firstframe, bitmapnumber, (ULONG)-1,
                    bitplaneoffset))
        return BB_FALSE;
    animaddress = (UBYTE *)address;
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

/**
 * @brief RIAnimInit address,bitmap#,palette#: c_InitAnim() with offset 0.
 * @param address  d0: the ANIM file in memory
 * @param bm       d1: bitmap#
 * @param pal      d2: palette#
 * @return d0: as c_InitAnim()
 */
BB_BOOL BLITZ_ROUTINE c_InitAnim2(ULONG address BL_REG(d0), ULONG bm BL_REG(d1),
                                  ULONG pal BL_REG(d2))
{
    return c_InitAnim(address, bm, pal, 0);
}

/**
 * @brief RIAnimInit address,bitmap#,palette#,x,y: c_InitAnim() at the pixel position x,y.
 *
 * The offset is BytesPerRow * y (MULU.w: the low words) + x >> 3
 * (ASR.l); the bitmap# is d1 / 64 with DIVU.w, which overflows (no
 * change) for numbers of 64 * 65536 and above.  A position outside the
 * bitmap is the programmer's: with runtime error checking xychk refuses
 * it (fixed #514).
 * @param address  d0: the ANIM file in memory
 * @param bm       d1: bitmap# * 64 (#pd1)
 * @param pal      d2: palette#
 * @param x        d3: the x position in pixels
 * @param y        d4: the y position in lines
 * @param bitmap   a0: the bitmap object (!bmap {0,1})
 * @return d0: as c_InitAnim()
 */
BB_BOOL BLITZ_ROUTINE c_InitAnim3(ULONG address BL_REG(d0), ULONG bm BL_REG(d1),
                                  ULONG pal BL_REG(d2), ULONG x BL_REG(d3), ULONG y BL_REG(d4),
                                  const struct BitMap *bitmap BL_REG(a0))
{
    ULONG off = bl_mulu_w(bitmap->BytesPerRow, (UWORD)y) + (ULONG)((LONG)x >> 3);

    return c_InitAnim(address, bl_divu_w(bm, SIZEOF_BITMAP), pal, off);
}

/**
 * @brief RIAnimFrameCount: the number of frames of the ANIM of the last RIAnimInit.
 *
 * Counts the FORM ILBM chunks of the ANIM (fixed #510: the original
 * counted every chunk).  The walk is unsigned and stays inside the ANIM
 * (fixed #506), with the pad bytes (fixed #505).
 * @return d0: the number of frames, 0 before an RIAnimInit
 */
ULONG BLITZ_ROUTINE c_CountFrames(void)
{
    UBYTE *p;
    ULONG n = 0;

    if (!animaddress)
        return 0;
    /* fixed #506: unsigned, and no chunk passes animend (RIAnimInit) */
    for (p = animaddress + 12; p < animend; p = nextchunk(p, animend))
        if (LG(p) == ID_FORM && LG(p + 8) == ID_ILBM)   /* fixed #510 */
            n++;
    return n;
}

/* ---------------------------------------------------------- the deltas */

/* the count of a run or copy: DBRA on the byte count - 1 (0 = 256,
   BUG(orig) (#508), kept: no encoder writes 0, see the head) */
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

/* ANIM7: one plane, columns of longs (SZ "l", the long form) or words
   (SZ "w", the short form), STEP the bytes of a column; the data from
   a3.  Assembler (C stays behind: gcc keeps the column or the op pointer
   on the stack, a3-a5 are fixed): the original's loop (UnpackAnim7
   'unpackaplane) with the op's sign as the copy test and the column's
   end behind each op.  The counts as COUNT (0 = 256, #508): the byte - 1
   in a register that is 0 above it, DBF on the word. */
#define ANIM7_PLANE(SZ, STEP)                                               \
    __asm__ volatile(                                                       \
        "0: move.b  (%[ops])+,%[n]\n"   /* the column's op count */         \
        "   beq.s   8f\n"                                                   \
        "   move.l  %[col],%[dst]\n"                                        \
        "1: moveq   #0,%[op]\n"                                             \
        "   move.b  (%[ops])+,%[op]\n"                                      \
        "   beq.s   4f\n"                                                   \
        "   bmi.s   3f\n"                                                   \
        "   mulu.w  %[mod],%[op]\n"     /* skip: op rows */                 \
        "   add.l   %[op],%[dst]\n"                                         \
        "   subq.b  #1,%[n]\n"                                              \
        "   bne.s   1b\n"                                                   \
        "   addq.l  #" STEP ",%[col]\n"                                     \
        "   dbf     %[cols],0b\n"                                           \
        "   bra.s   9f\n"                                                   \
        "3: and.b   #0x7f,%[op]\n"      /* copy: op & $7F values */         \
        "   subq.b  #1,%[op]\n"                                             \
        "5: move." SZ "  (%[data])+,(%[dst])\n"                             \
        "   add.l   %[mod],%[dst]\n"                                        \
        "   dbf     %[op],5b\n"                                             \
        "   subq.b  #1,%[n]\n"                                              \
        "   bne.s   1b\n"                                                   \
        "   addq.l  #" STEP ",%[col]\n"                                     \
        "   dbf     %[cols],0b\n"                                           \
        "   bra.s   9f\n"                                                   \
        "4: move.b  (%[ops])+,%[op]\n"  /* run: its count, one value */     \
        "   subq.b  #1,%[op]\n"                                             \
        "   move." SZ "  (%[data])+,%[v]\n"                                 \
        "6: move." SZ "  %[v],(%[dst])\n"                                   \
        "   add.l   %[mod],%[dst]\n"                                        \
        "   dbf     %[op],6b\n"                                             \
        "   subq.b  #1,%[n]\n"                                              \
        "   bne.s   1b\n"                                                   \
        "8: addq.l  #" STEP ",%[col]\n"                                     \
        "   dbf     %[cols],0b\n"                                           \
        "9:"                                                                \
        : [ops] "+a"(ops), [col] "+a"(column), [data] "+a"(data),           \
          [cols] "+d"(cols), [dst] "=&a"(dst), [n] "=&d"(n),                \
          [op] "=&d"(op), [v] "=&d"(v)                                      \
        : [mod] "d"(mod)                                                    \
        : "cc", "memory")

/* ANIM7 (long form): one plane, columns of longs; the data from a3 */
static inline __attribute__((always_inline)) void plane7(const UBYTE *ops, UBYTE *column,
                                                          const ULONG *data, ULONG mod)
{
    UWORD cols = deltacolumns;
    UBYTE *dst;
    ULONG n, op, v;

    ANIM7_PLANE("l", "4");
}

/* ANIM7 (short form): one plane, columns of words; the data from a3
   (fixed #504: the original never reached its UnpackAnim7Short) */
static inline __attribute__((always_inline)) void plane7short(const UBYTE *ops, UBYTE *column,
                                                               const UWORD *data, ULONG mod)
{
    UWORD cols = deltacolumns;
    UBYTE *dst;
    ULONG n, op, v;

    ANIM7_PLANE("w", "2");
}

/* the planes of an ANIM7 DLTA (body: its data), the decoder of kind
   (DELTA7, DELTA7SHORT) inline: no call per plane; a function of its own,
   so that unpackanim's ANIM5 loop keeps its registers */
static __attribute__((noinline)) void planes7(UBYTE *body, int kind, ULONG mod)
{
    const ULONG *offsets = (const ULONG *)body;
    PLANEPTR *planes = bitmapaddress;
    UBYTE plane = 0;

    do {
        UBYTE *column = *planes++ + bitplaneoffset;
        ULONG op = offsets[0], data = offsets[8];

        offsets++;
        if (op) {
            if (kind == DELTA7)
                plane7(body + op, column, (const ULONG *)(body + data), mod);
            else
                plane7short(body + op, column, (const UWORD *)(body + data), mod);
        }
    } while (++plane != animdepth);
}

/* the DLTA (header at dlta) into the planes of the bitmap with the
   decoder kind (DELTA5, DELTA7, DELTA7SHORT); 0 if its plane offsets do
   not fit it (fixed #1120) */
static int unpackanim(UBYTE *dlta, int kind)
{
    UBYTE *body = dlta + 8;
    const ULONG *offsets = (const ULONG *)body;
    PLANEPTR *planes = bitmapaddress;
    /* the bitmap's ebwidth: BytesPerRow, 8 bytes in front of Planes */
    ULONG mod = WD((UBYTE *)planes - offsetof(struct BitMap, Planes));
    ULONG len = LG(dlta + 4);
    UBYTE plane = 0;

    /* fixed #1120: the DLTA inside the frame, its header (the op list
       offsets; for ANIM7 the data list offsets too) inside the DLTA, and
       the offsets of the planes it plays inside the DLTA, before a plane
       is played (the original took them as they were).  The ops of the
       columns stay unchecked (BUG(orig) (#1120), see the head) */
    if (body > nextanimframe || len > (ULONG)(nextanimframe - body))
        return 0;
    {
        const ULONG *o = offsets;
        UWORD n = animdepth - 1;        /* DBRA: depth 1-8 (fixed #509) */

        if (kind == DELTA5) {
            if (len < 8 * 4)
                return 0;
            do
                if (*o++ >= len)
                    return 0;
            while (n-- != 0);
        } else {
            if (len < 16 * 4)
                return 0;
            do {
                ULONG op = *o++;

                if (op >= len || (op && o[7] >= len))
                    return 0;
            } while (n-- != 0);
        }
    }
    deltacolumns = columns[kind];
    if (kind != DELTA5) {
        planes7(body, kind, mod);
        return 1;
    }
    do {                                /* depth 1-8 (fixed #509, RIAnimInit) */
        UBYTE *column = *planes++ + bitplaneoffset;
        ULONG op = *offsets++;

        if (op)
            plane5(body + op, column, mod);
    } while (++plane != animdepth);
    return 1;
}

/* the DLTA of a frame whose chunk behind ANHD (and CMAP) is another one:
   the chunks up to the frame's end (nextanimframe) skipped, a CMAP through
   ILBMPalette; NULL without a DLTA (fixed #515: the original returned
   through its saved d1 here).  Not on the path of a frame of ANHD, CMAP,
   DLTA. */
static __attribute__((noinline)) UBYTE *otherchunks(UBYTE *chunk)
{
    struct bl_regs r;

    for (;;) {
        if (chunk >= nextanimframe)
            return NULL;
        if (LG(chunk) == ID_DLTA)
            return chunk;
        if (LG(chunk) == ID_CMAP) {
            cmapchunk = chunk;
            r = (struct bl_regs){ .d0 = (ULONG)chunk, .d1 = palettenumber };
            BLITZ_ALIBJSR(TOK_ilbmpalette, r);
            chunk = cmapchunk;
        }
        chunk = nextchunk(chunk, nextanimframe);
        if (!chunk)
            return NULL;
    }
}

/* the chunk behind the chunk at p (its header inside the frame) with its
   pad byte (fixed #505); NULL unless it ends 8 bytes (a chunk header)
   before the frame's end (fixed #1117: the original stepped by the length
   without a look at the frame's end).  On the path from frame to frame:
   two compares more than the plain step. */
static inline __attribute__((always_inline)) UBYTE *framestep(UBYTE *p)
{
    ULONG left = (ULONG)(nextanimframe - p), len = LG(p + 4);

    if (left < 16 || len > left - 16)
        return NULL;
    return p + 8 + padded(p + 8, len);
}

/* the next frame from chunk on: chunks of the ANIM that are no FORM
   skipped; NULL at the end of the ANIM (fixed #1116: the original took
   such a chunk as the end).  Not on the path from frame to frame. */
static __attribute__((noinline)) UBYTE *nextform(UBYTE *chunk)
{
    while (chunk && chunk < animend) {
        if (LG(chunk) == ID_FORM)
            return chunk;
        chunk = nextchunk(chunk, animend);
    }
    return NULL;
}

/**
 * @brief RINextAnimFrame bitmap#: apply the DLTA of the next frame to the bitmap.
 *
 * ANHD operation 5 or 65 (byte vertical delta) or 7 (short or long
 * vertical delta by bit 0 of the ANHD bits; fixed #504: the original
 * always took the long form) on the planes
 * (+ the offset of RIAnimInit); a CMAP of the frame goes through
 * ILBMPalette.  Chunks of the ANIM that are no FORM are skipped (fixed
 * #1116: the original took one between two frames as the end).  At the
 * end of the ANIM it goes on with the third frame
 * if AnimLoop is on, else returns 0.  ANIM5 has the byte columns of the
 * word-aligned row (fixed #507), ANIM7's short form the words of the row
 * (fixed #504).  BUG(orig): the ANIM7 long columns are width >> 5,
 * dropping a rest and giving 65536 columns below 32 pixels (#1115);
 * a run or copy count of 0 is 256 (#508).  The depth is
 * 1 to 8 (fixed #509: RIAnimInit refuses 0 and above 8).  Other chunks between ANHD (and CMAP) and the DLTA are
 * skipped inside the frame, a CMAP among them goes through ILBMPalette; a
 * frame without DLTA returns 0 (fixed #515: the original returned without
 * restoring its 14 saved registers, through the saved d1).  The steps over
 * ANHD and CMAP stay inside the frame: a frame too short for its ILBM
 * and ANHD fields, or an ANHD or CMAP whose length leaves no chunk header
 * in the frame, returns 0 (fixed #1117: the original stepped by the
 * length and read the next chunk anywhere).  A DLTA whose plane offsets
 * leave it returns 0 (fixed #1120; the ops inside the columns stay
 * unchecked, BUG(orig) (#1120)).
 * @param bitmap  a0: the bitmap object (!bmap {0,0})
 * @return d0: -1 if a delta was applied, else 0
 */
ULONG BLITZ_ROUTINE c_NextAnimFrame(struct BitMap *bitmap BL_REG(a0))
{
    UBYTE *chunk = nextanimframe;
    const struct anhd *anhd;
    struct bl_regs r;
    ULONG len;
    int kind;

    if (!chunk)
        return 0;
    bitmapaddress = bitmap->Planes;
    if (chunk >= animend || LG(chunk) != ID_FORM) {    /* fixed #506: unsigned */
        /* fixed #1116: a chunk that is no FORM is skipped, not the end */
        chunk = nextform(chunk);
        if (!chunk) {
            if (!doanimloop)
                return 0;
            chunk = thirdframe;
        }
    }
    len = LG(chunk + 4);
    chunk += 8;
    nextanimframe = chunk + padded(chunk, len);     /* fixed #505 */
    /* fixed #1117: ILBM, the ANHD header and its fields up to the bits
       inside the frame */
    if (len < 4 + 8 + offsetof(struct anhd, bits) + 4)
        return 0;
    chunk += 4;                         /* skip ILBM */
    if (LG(chunk) != ID_ANHD)
        return 0;
    anhd = (const struct anhd *)(chunk + 8);
    animtype = anhd->operation;
    animtype7 = anhd->bits;
    chunk = framestep(chunk);           /* fixed #505, #1117 */
    if (!chunk)
        return 0;
    if (LG(chunk) == ID_CMAP) {
        cmapchunk = chunk;
        r = (struct bl_regs){ .d0 = (ULONG)chunk, .d1 = palettenumber };
        BLITZ_ALIBJSR(TOK_ilbmpalette, r);
        chunk = framestep(cmapchunk);   /* fixed #505, #1117 */
        if (!chunk)
            return 0;
    }
    if (LG(chunk) != ID_DLTA) {         /* fixed #515: was an RTS without MOVEM */
        chunk = otherchunks(chunk);
        if (!chunk)
            return 0;
    }
    if (animtype == 5 || animtype == 65)
        kind = DELTA5;
    else if (animtype == 7)
        /* fixed #504: bit 0 of the ANHD bits (the original tested the
           operation, never the short form) */
        kind = (animtype7 & 1) ? DELTA7 : DELTA7SHORT;
    else
        return 0;
    /* fixed #1120: a DLTA whose plane offsets leave it is refused */
    return unpackanim(chunk, kind) ? (ULONG)-1 : 0;
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
/* RINextAnimFrame keeps all but d0, as the original */
BLITZ_THUNK(_NextAnimFrame, c_NextAnimFrame, "d1/a0-a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(badxy, "Animation position outside the bitmap");  /* fixed, #514 */

/**
 * @brief The error check of RIAnimInit's x,y form (debugger only).
 *
 * Fixed (#514), not in the original (rianimlib had no error check
 * area): the position becomes the byte offset BytesPerRow * y (MULU.w of
 * the low words) + x >> 3 into every plane, without a look at the bitmap;
 * a negative y or one of 65536 and above gave an offset of up to 65535
 * rows, and UnpackIFF and every RINextAnimFrame wrote there.  The release
 * code is unchanged (an invalid argument, D-005).
 *
 * Errors: "Animation position outside the bitmap" (x not in 0 to the
 * bitmap's width in pixels - 1, or y not in 0 to its rows - 1).
 * @param x       d3: the x position in pixels
 * @param y       d4: the y position in lines
 * @param bitmap  a0: the bitmap object (!bmap {0,1})
 */
static void BLITZ_ERRCHK xychk_c(ULONG x BL_REG(d3), ULONG y BL_REG(d4),
                                 const struct BitMap *bitmap BL_REG(a0))
{
    if (x >= (ULONG)bitmap->BytesPerRow * 8 || y >= bitmap->Rows)
        BLITZ_RUNERR(badxy);
}

/* the thunk of the check function */
BLITZ_ERRCHK_THUNK(xychk, xychk_c, "d0-d1/a0-a1");
