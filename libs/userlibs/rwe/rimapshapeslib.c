/*
 * rimapshapeslib - RWE userlib 6 (Reflective Images, 1994-96): map
 * shapes, blitted into a window's RastPort, and groups of them.
 *
 *   ShapeToMapShape shape#,mapshape#   a map shape from a Blitz shape (its
 *                                      planes, not copied)
 *   BlitMapShape window#,mapshape#,x,y
 *   BlitMapShapeSize window#,mapshape#,x,y,width,height
 *   RemapMapShape mapshape#,botcol,topcol   does nothing
 *   SetNumMapShapes num                (re)allocates num map shapes
 *   MapShapeAddr(mapshape#)
 *   FreeMapShapes first,last           width 0: the shapes are empty
 *   ResetGroup blockwidth,blockheight  empties the group (-> its address)
 *   AddGroupMember mapshape#,dx,dy     at dx*blockwidth, dy*blockheight
 *   BlitGroup window#,x,y              blits the group's members
 *
 * C translation of rwelibsrc/MapEd/MapShape2.bb (22/12/96); reference: the
 * reference build of that source (no binary exists, docs/rwelibs/README.md
 * 3.3).  Notes: docs/rwelibs/port/rimapshapeslib.md.
 *
 * A map shape (64 bytes, mapshape# * 64 from the block SetNumMapShapes
 * allocates): width.w, then a struct BitMap (BytesPerRow = the shape's
 * ebwidth, Rows = height, the depth word = Flags/Depth, pad, 8 planes),
 * which is blitted with BltBitMapRastPort (minterm $C0) from 0,0.  The
 * group: a block of 512 bytes (init, AllocVec MEMF_CLEAR) of members
 * {map shape address.l, dx.w, dy.w}, ended by a 0 address.
 *
 * All C behind thunks; the graphics calls get the registers the original
 * passes, with their upper words: x, y (and width, height of
 * BlitMapShapeSize) as the caller's longs, the width/height/minterm the
 * original loads with MOVE.w into the caller's d4/d5/d6 (in BlitGroup d5
 * is the member's address before).
 *
 * BUG(orig):
 *   - No check of the map shape number: before SetNumMapShapes (or after
 *     one that failed or allocated 0) the map shapes are at address 0.
 *   - ShapeToMapShape of a shape with depth 0 writes 65536 plane pointers
 *     (DBRA), a depth > 8 runs into the next map shape.
 *   - AddGroupMember before ResetGroup writes at address 0; members are
 *     not counted: the 64th member's end mark is written behind the
 *     512-byte block.
 *   - BlitGroup does not load graphics.library (no #graphicslib in its
 *     !libs): it calls BltBitMapRastPort with the caller's a6.
 *   - init does not check its AllocVec: ResetGroup then writes at 0.
 *   - RemapMapShape does nothing (the ReMap call is commented out); its
 *     !libs asks for "library 254, $5600" (sic).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks, below) */
void _shapetomapshape(void), _blitmapshape(void), _remapmapshape(void);
void _blitmapshapesize(void), _setnummapshapes(void), _mapshapeaddr(void);
void _freemapshapes(void), _addgroupmember(void), _blitgroup(void);
void _resetgroup(void), _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* #ia<a>|#pd<p>: the object of the number in d<p> into a<a> */
#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))

/* !libheader {#rimapshapeslib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rimapshapeslib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(0, 0));
BLITZ_SUBS(_shapetomapshape, 0, 0);
BLITZ_NAME("ShapeToMapShape", "source shape,dest Mapshape");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, IA(1, 0), LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(_blitmapshape, 0, 0);
BLITZ_NAME("BlitMapShape", "window#,mapshape#,x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_chipbaselib, BL_LA(2), 254, 0x5600);
BLITZ_SUBS(_remapmapshape, 0, 0);
BLITZ_NAME("RemapMapShape", "mapshape#,botcol,topcol");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, IA(1, 0), LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(_blitmapshapesize, 0, 0);
BLITZ_NAME("BlitMapShapeSize", "window#,mapshape#,x,y,width,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_setnummapshapes, 0, 0);
BLITZ_NAME("SetNumMapShapes", "num");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_mapshapeaddr, 0, 0);
BLITZ_NAME("MapShapeAddr", "mapshape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_freemapshapes, 0, 0);
BLITZ_NAME("FreeMapShapes", "first,last");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_addgroupmember, 0, 0);
BLITZ_NAME("AddGroupMember", "shape#,dx,dy");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, IA(1, 0));
BLITZ_SUBS(_blitgroup, 0, 0);
BLITZ_NAME("BlitGroup", "window#,x,y");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_resetgroup, 0, 0);
BLITZ_NAME("ResetGroup", "blockwidth,blockheight");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

static UWORD blockwidth, blockheight;
static UBYTE *baseshape;                /* the map shapes, 64 bytes each */
static ULONG *nextgroupmember;
static ULONG *groupaddr;                /* the group, 512 bytes */

/* GetMapShape: ANDI.l #$ffff:LSL.l #6:ADD.l baseshape */
#define MAPSHAPE(n) (baseshape + ((ULONG)(UWORD)(n) << 6))
#define MW(ms, o) (*(UWORD *)((ms) + (o)))

/* a Blitz shape: pixwidth 0, pixheight 2, depth 4, ebwidth 6, data 14,
   onebpmem 22 */
#define SW(sh, o) (*(UWORD *)((UBYTE *)(sh) + (o)))

/* ---------------------------------------------------------------- init */

static void BLITZ_ROUTINE libinit_c(void)
{
    groupaddr = AllocVec(512, MEMF_PUBLIC | MEMF_CLEAR);
}
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE libfinit_c(void)
{
    if (baseshape) {
        FreeVec(baseshape);
        baseshape = NULL;
    }
    FreeVec(groupaddr);
}
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");

/* ---------------------------------------------------------- map shapes */

/* ShapeToMapShape shape#,mapshape#: a0 = the shape, d1 = the map shape */
static void BLITZ_ROUTINE shapetomapshape_c(UWORD n BL_REG(d1), const UBYTE *sh BL_REG(a0))
{
    UBYTE *ms = MAPSHAPE(n);
    UBYTE **planes = (UBYTE **)(ms + 10);
    UBYTE *p = *(UBYTE **)(sh + 14);
    WORD onebp = (WORD)SW(sh, 22);
    UWORD depth = SW(sh, 4), c;

    MW(ms, 0) = SW(sh, 0);              /* pixwidth */
    MW(ms, 4) = SW(sh, 2);              /* height */
    MW(ms, 6) = depth;
    MW(ms, 2) = SW(sh, 6);              /* ebwidth */
    /* BUG(orig): depth 0 -> 65536 planes (DBRA) */
    c = depth - 1;
    do {
        *planes++ = p;
        p += onebp;                     /* ADD.w d1,a0 */
    } while (c-- != 0);
}
BLITZ_THUNK(_shapetomapshape, shapetomapshape_c, "d0-d1/a0-a1");

/* _blitmapshape2: a map shape that is not empty (width) is blitted to
   x,y; d4/d5/d6 get width/height/minterm in their low words */
static void blit(struct Library *gfx, UBYTE *ms, struct RastPort *rp, ULONG x, ULONG y,
                 ULONG d4, ULONG d5, ULONG d6)
{
    if (!MW(ms, 0))
        return;
    BL_SETW(d4, MW(ms, 0));
    BL_SETW(d5, MW(ms, 4));
    BL_SETW(d6, 0xc0);
    BltBitMapRastPort((struct BitMap *)(ms + 2), 0, 0, rp, (LONG)x, (LONG)y, (LONG)d4,
                      (LONG)d5, d6);
}

/* BlitMapShape window#,mapshape#,x,y: a1 = the window object, a6 =
   graphics.library */
static void BLITZ_ROUTINE blitmapshape_c(UWORD n BL_REG(d1), ULONG x BL_REG(d2),
                                         ULONG y BL_REG(d3), ULONG d4 BL_REG(d4),
                                         ULONG d5 BL_REG(d5), ULONG d6 BL_REG(d6),
                                         struct Window **win BL_REG(a1),
                                         struct Library *gfx BL_REG(a6))
{
    blit(gfx, MAPSHAPE(n), (*win)->RPort, x, y, d4, d5, d6);
}
BLITZ_THUNK(_blitmapshape, blitmapshape_c, "d0-d1/a0-a1");

/* BlitMapShapeSize window#,mapshape#,x,y,width,height: the height at most
   the map shape's (signed) */
static void BLITZ_ROUTINE blitmapshapesize_c(UWORD n BL_REG(d1), ULONG x BL_REG(d2),
                                             ULONG y BL_REG(d3), ULONG w BL_REG(d4),
                                             ULONG h BL_REG(d5), ULONG d6 BL_REG(d6),
                                             struct Window **win BL_REG(a1),
                                             struct Library *gfx BL_REG(a6))
{
    UBYTE *ms = MAPSHAPE(n);

    if (!MW(ms, 0))
        return;
    if ((WORD)h > (WORD)MW(ms, 4))
        BL_SETW(h, MW(ms, 4));
    BL_SETW(d6, 0xc0);
    BltBitMapRastPort((struct BitMap *)(ms + 2), 0, 0, (*win)->RPort, (LONG)x, (LONG)y,
                      (LONG)w, (LONG)h, d6);
}
BLITZ_THUNK(_blitmapshapesize, blitmapshapesize_c, "d0-d1/a0-a1");

/* RemapMapShape mapshape#,botcol,topcol: nothing (the original computes
   the map shape's address and returns) */
void _remapmapshape(void)
{
}

/* SetNumMapShapes num: the old block freed, num * 64 bytes allocated
   (AllocVec MEMF_PUBLIC|MEMF_CLEAR, unchecked) */
static void BLITZ_ROUTINE setnummapshapes_c(UWORD num BL_REG(d0))
{
    if (baseshape) {
        FreeVec(baseshape);
        baseshape = NULL;
    }
    baseshape = AllocVec((ULONG)num << 6, MEMF_PUBLIC | MEMF_CLEAR);
}
BLITZ_THUNK(_setnummapshapes, setnummapshapes_c, "d0-d1/a0-a1");

/* MapShapeAddr(mapshape#) */
static UBYTE *BLITZ_ROUTINE mapshapeaddr_c(UWORD n BL_REG(d0))
{
    return MAPSHAPE(n);
}
BLITZ_THUNK(_mapshapeaddr, mapshapeaddr_c, "d1/a0-a1");

/* FreeMapShapes first,last: nothing if last < first (signed words), else
   the widths of the map shapes from first to last cleared (the addresses
   compared signed) */
static void BLITZ_ROUTINE freemapshapes_c(UWORD first BL_REG(d0), UWORD last BL_REG(d1))
{
    UBYTE *p, *e;

    if ((WORD)last < (WORD)first)
        return;
    p = MAPSHAPE(first);
    e = MAPSHAPE(last);
    while ((LONG)e >= (LONG)p) {
        MW(p, 0) = 0;
        p += 64;
    }
}
BLITZ_THUNK(_freemapshapes, freemapshapes_c, "d0-d1/a0-a1");

/* -------------------------------------------------------------- groups */

/* ResetGroup blockwidth,blockheight -> the group's address */
static ULONG *BLITZ_ROUTINE resetgroup_c(UWORD bw BL_REG(d0), UWORD bh BL_REG(d1))
{
    blockwidth = bw;
    blockheight = bh;
    groupaddr[0] = 0;
    nextgroupmember = groupaddr;
    return groupaddr;
}
BLITZ_THUNK(_resetgroup, resetgroup_c, "d1/a0-a1");

/* AddGroupMember mapshape#,dx,dy: {map shape, dx*blockwidth,
   dy*blockheight (MULU.w, the low words)}, then the end mark.
   BUG(orig): no limit, and before ResetGroup at address 0 */
static void BLITZ_ROUTINE addgroupmember_c(UWORD n BL_REG(d0), UWORD dx BL_REG(d1),
                                           UWORD dy BL_REG(d2))
{
    ULONG *p = nextgroupmember;

    *p++ = (ULONG)MAPSHAPE(n);
    ((UWORD *)p)[0] = (UWORD)bl_mulu_w(dx, blockwidth);
    ((UWORD *)p)[1] = (UWORD)bl_mulu_w(dy, blockheight);
    p++;
    *p = 0;
    nextgroupmember = p;
}
BLITZ_THUNK(_addgroupmember, addgroupmember_c, "d0-d1/a0-a1");

/* BlitGroup window#,x,y: every member at x+dx, y+dy (words; the upper
   words of the caller's d2/d3).  BUG(orig): a6 is the caller's, not
   graphics.library's.  The original's d5 is the member's address when
   _blitmapshape2 loads the height into its low word. */
static void BLITZ_ROUTINE blitgroup_c(UWORD x BL_REG(d1), ULONG d2 BL_REG(d2),
                                      ULONG d3 BL_REG(d3), ULONG d4 BL_REG(d4),
                                      ULONG d6 BL_REG(d6), struct Window **win BL_REG(a1),
                                      struct Library *gfx BL_REG(a6))
{
    struct RastPort *rp = (*win)->RPort;
    const ULONG *p = groupaddr;
    ULONG ms;

    BL_SETW(d3, d2);                    /* MOVE.w d2,d3 */
    BL_SETW(d2, x);                     /* MOVE.w d1,d2 */
    while ((ms = *p++) != 0) {
        ULONG mx = d2, my = d3;
        BL_SETW(mx, d2 + ((const UWORD *)p)[0]);
        BL_SETW(my, d3 + ((const UWORD *)p)[1]);
        blit(gfx, (UBYTE *)ms, rp, mx, my, d4, ms, d6);
        p++;
    }
}
BLITZ_THUNK(_blitgroup, blitgroup_c, "d0-d1/a0-a1");
