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
 *     one that failed or allocated 0) the map shapes are at address 0
 *     (#906).
 *   - ShapeToMapShape of a shape with depth 0 writes 65536 plane pointers
 *     (DBRA); the depth is not limited to the 8 pointers: depths 9-13
 *     write into the unused bytes 42-63 of the 64-byte map shape, from
 *     depth 14 on the pointers run into the next map shape (#903).
 *   - AddGroupMember before ResetGroup writes at address 0 (#901);
 *     members are not counted: the 64th member's end mark is written
 *     behind the 512-byte block (#904).
 *   - BlitGroup does not load graphics.library (no #graphicslib in its
 *     !libs): it calls BltBitMapRastPort with the caller's a6 (#902).
 *   - init does not check its AllocVec: ResetGroup then writes at 0
 *     (#905).
 * Not a defect: RemapMapShape does nothing (the ReMap call is commented
 * out in the source: a command left unfinished); its !libs asks for
 * "library 254, $5600" (sic).
 */
#include "blitzlib.h"
#include "blitzobj.h"
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

/* a map shape: the width and a struct BitMap at +2 (BytesPerRow =
   ebwidth, Rows = height, Flags/Depth written as one word = the depth),
   22 bytes unused */
struct mapshape {
    UWORD width;                        /* +0: 0 = empty */
    struct BitMap bm;                   /* +2 */
    UBYTE unused[22];                   /* +42 */
};
static_assert(sizeof(struct mapshape) == 64);

/* a group member; the group ends with a 0 mapshape */
struct member {
    struct mapshape *ms;                /* +0 */
    UWORD dx, dy;                       /* +4, +6: in pixels */
};
static_assert(sizeof(struct member) == 8);

static UWORD blockwidth, blockheight;
static struct mapshape *baseshape;      /* the map shapes */
static struct member *nextgroupmember;
static struct member *groupaddr;        /* the group, 512 bytes */

/* GetMapShape: ANDI.l #$ffff:LSL.l #6:ADD.l baseshape */
#define MAPSHAPE(n) ((struct mapshape *)((UBYTE *)baseshape + ((ULONG)(UWORD)(n) << 6)))

/* a Blitz shape (shapeslib): struct bb_shape (blitzobj.h) */

/* ---------------------------------------------------------------- init */

/**
 * @brief init: the group block (512 bytes, AllocVec MEMF_PUBLIC|MEMF_CLEAR).
 *
 * BUG(orig) (#905): the AllocVec is not checked (ResetGroup then writes
 * at 0).
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    groupaddr = AllocVec(512, MEMF_PUBLIC | MEMF_CLEAR);
}
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");

/**
 * @brief finit: frees the map shapes and the group block.
 */
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

/**
 * @brief ShapeToMapShape shape#,mapshape#: a map shape from a Blitz shape.
 *
 * Width, height, depth and ebwidth of the shape; the plane pointers point
 * into the shape's data (onebpmem apart, not copied).  BUG(orig): no
 * check of the map shape number (#906); depth 0 writes 65536 plane
 * pointers (DBRA), depths above 8 run into unused[] and the next map
 * shapes (#903).
 * @param n   d1: the map shape number
 * @param sh  a0: the shape (!libs #shapeslib,#ia0|#pd0)
 */
static void BLITZ_ROUTINE shapetomapshape_c(UWORD n BL_REG(d1), const struct bb_shape *sh BL_REG(a0))
{
    struct mapshape *ms = MAPSHAPE(n);
    PLANEPTR *planes = ms->bm.Planes;
    UBYTE *p = sh->data;
    WORD onebp = (WORD)sh->planesize;
    UWORD depth = sh->depth, c;

    ms->width = sh->width;
    ms->bm.Rows = sh->height;
    *(UWORD *)&ms->bm.Flags = depth;    /* Flags 0, Depth */
    ms->bm.BytesPerRow = sh->ebwidth;
    /* BUG(orig): depth 0 -> 65536 planes (DBRA), more than 8 run into
       unused[] and the next map shapes (#903) */
    c = depth - 1;
    do {
        *planes++ = p;
        p += onebp;                     /* ADD.w d1,a0 */
    } while (c-- != 0);
}
BLITZ_THUNK(_shapetomapshape, shapetomapshape_c, "d0-d1/a0-a1");

/* _blitmapshape2: a map shape that is not empty (width) is blitted to
   x,y; d4/d5/d6 get width/height/minterm in their low words */
static void blit(struct Library *gfx, struct mapshape *ms, struct RastPort *rp, ULONG x, ULONG y,
                 ULONG width, ULONG height, ULONG minterm)
{
    if (!ms->width)
        return;
    BL_SETW(width, ms->width);
    BL_SETW(height, ms->bm.Rows);
    BL_SETW(minterm, 0xc0);
    BltBitMapRastPort(&ms->bm, 0, 0, rp, (LONG)x, (LONG)y, (LONG)width,
                      (LONG)height, minterm);
}

/**
 * @brief BlitMapShape window#,mapshape#,x,y: a map shape blitted into the
 * window's RastPort.
 *
 * BltBitMapRastPort from 0,0, minterm $C0, the map shape's width and
 * height; nothing for an empty map shape (width 0).  x and y go to the
 * call as the caller's longs; width, height and minterm replace the low
 * words of the caller's d4/d5/d6.  BUG(orig) (#906): the map shape
 * number is not checked.
 * @param n        d1: the map shape number
 * @param x        d2: x
 * @param y        d3: y
 * @param width    d4: the caller's d4 (upper word)
 * @param height   d5: the caller's d5 (upper word)
 * @param minterm  d6: the caller's d6 (upper word)
 * @param win      a1: the window object
 * @param gfx      a6: graphics.library
 */
static void BLITZ_ROUTINE blitmapshape_c(UWORD n BL_REG(d1), ULONG x BL_REG(d2),
                                         ULONG y BL_REG(d3), ULONG width BL_REG(d4),
                                         ULONG height BL_REG(d5), ULONG minterm BL_REG(d6),
                                         struct Window **win BL_REG(a1),
                                         struct Library *gfx BL_REG(a6))
{
    blit(gfx, MAPSHAPE(n), (*win)->RPort, x, y, width, height, minterm);
}
BLITZ_THUNK(_blitmapshape, blitmapshape_c, "d0-d1/a0-a1");

/**
 * @brief BlitMapShapeSize window#,mapshape#,x,y,width,height: a part of a
 * map shape blitted into the window's RastPort.
 *
 * As BlitMapShape with the given width and height, the height at most
 * the map shape's (signed words; the width is not limited); nothing for
 * an empty map shape.  BUG(orig) (#906): the number is not checked.
 * @param n        d1: the map shape number
 * @param x        d2: x
 * @param y        d3: y
 * @param w        d4: the width (the caller's long)
 * @param h        d5: the height (the caller's long, low word limited)
 * @param minterm  d6: the caller's d6 (upper word; the low word $C0)
 * @param win      a1: the window object
 * @param gfx      a6: graphics.library
 */
static void BLITZ_ROUTINE blitmapshapesize_c(UWORD n BL_REG(d1), ULONG x BL_REG(d2),
                                             ULONG y BL_REG(d3), ULONG w BL_REG(d4),
                                             ULONG h BL_REG(d5), ULONG minterm BL_REG(d6),
                                             struct Window **win BL_REG(a1),
                                             struct Library *gfx BL_REG(a6))
{
    struct mapshape *ms = MAPSHAPE(n);

    if (!ms->width)
        return;
    if ((WORD)h > (WORD)ms->bm.Rows)
        BL_SETW(h, ms->bm.Rows);
    BL_SETW(minterm, 0xc0);
    BltBitMapRastPort(&ms->bm, 0, 0, (*win)->RPort, (LONG)x, (LONG)y,
                      (LONG)w, (LONG)h, minterm);
}
BLITZ_THUNK(_blitmapshapesize, blitmapshapesize_c, "d0-d1/a0-a1");

/**
 * @brief RemapMapShape mapshape#,botcol,topcol: does nothing.
 *
 * The original computes the map shape's address and returns (the ReMap
 * call is commented out in the source: a command left unfinished, not a
 * defect).  No result.
 */
void _remapmapshape(void)
{
}

/**
 * @brief SetNumMapShapes num: (re)allocates num map shapes.
 *
 * The old block is freed, num * 64 bytes allocated (AllocVec
 * MEMF_PUBLIC|MEMF_CLEAR, unchecked: after a failure, or with num 0, the
 * map shapes are at 0, BUG(orig) #906).
 * @param num  d0: the number of map shapes (a word)
 */
static void BLITZ_ROUTINE setnummapshapes_c(UWORD num BL_REG(d0))
{
    if (baseshape) {
        FreeVec(baseshape);
        baseshape = NULL;
    }
    baseshape = AllocVec((ULONG)num << 6, MEMF_PUBLIC | MEMF_CLEAR);
}
BLITZ_THUNK(_setnummapshapes, setnummapshapes_c, "d0-d1/a0-a1");

/**
 * @brief MapShapeAddr(mapshape#): the address of a map shape.
 * @param n  d0: the map shape number (not checked)
 * @return d0: the map shape's address
 */
static struct mapshape *BLITZ_ROUTINE mapshapeaddr_c(UWORD n BL_REG(d0))
{
    return MAPSHAPE(n);
}
BLITZ_THUNK(_mapshapeaddr, mapshapeaddr_c, "d1/a0-a1");

/**
 * @brief FreeMapShapes first,last: the map shapes first to last made
 * empty.
 *
 * Nothing if last < first (signed words), else the widths of the map
 * shapes from first to last cleared (the addresses compared signed); the
 * memory stays allocated.
 * @param first  d0: the first map shape number
 * @param last   d1: the last map shape number
 */
static void BLITZ_ROUTINE freemapshapes_c(UWORD first BL_REG(d0), UWORD last BL_REG(d1))
{
    struct mapshape *p, *e;

    if ((WORD)last < (WORD)first)
        return;
    p = MAPSHAPE(first);
    e = MAPSHAPE(last);
    while ((LONG)e >= (LONG)p) {
        p->width = 0;
        p++;
    }
}
BLITZ_THUNK(_freemapshapes, freemapshapes_c, "d0-d1/a0-a1");

/* -------------------------------------------------------------- groups */

/**
 * @brief ResetGroup(blockwidth,blockheight): empties the group and sets
 * the block size for AddGroupMember.
 *
 * BUG(orig) (#905): without init's block (not checked) it writes at 0.
 * @param bw  d0: the block width
 * @param bh  d1: the block height
 * @return d0: the group's address
 */
static struct member *BLITZ_ROUTINE resetgroup_c(UWORD bw BL_REG(d0), UWORD bh BL_REG(d1))
{
    blockwidth = bw;
    blockheight = bh;
    groupaddr[0].ms = NULL;
    nextgroupmember = groupaddr;
    return groupaddr;
}
BLITZ_THUNK(_resetgroup, resetgroup_c, "d1/a0-a1");

/**
 * @brief AddGroupMember mapshape#,dx,dy: a map shape added to the group.
 *
 * The member {map shape, dx*blockwidth, dy*blockheight (MULU.w, the low
 * words)}, then the end mark.  BUG(orig): no limit, the 64th member's
 * end mark is written behind the 512-byte block (#904); before
 * ResetGroup it writes at address 0 (#901).
 * @param n   d0: the map shape number
 * @param dx  d1: the column in blocks
 * @param dy  d2: the row in blocks
 */
static void BLITZ_ROUTINE addgroupmember_c(UWORD n BL_REG(d0), UWORD dx BL_REG(d1),
                                           UWORD dy BL_REG(d2))
{
    struct member *p = nextgroupmember;

    p->ms = MAPSHAPE(n);
    p->dx = (UWORD)bl_mulu_w(dx, blockwidth);
    p->dy = (UWORD)bl_mulu_w(dy, blockheight);
    p++;
    p->ms = NULL;                       /* the end mark */
    nextgroupmember = p;
}
BLITZ_THUNK(_addgroupmember, addgroupmember_c, "d0-d1/a0-a1");

/**
 * @brief BlitGroup window#,x,y: every member of the group blitted at
 * x+dx, y+dy.
 *
 * As BlitMapShape for each member (empty ones skipped); x+dx and y+dy
 * are words, with the upper words of the caller's d2/d3.  The original's
 * d5 is the member's address when _blitmapshape2 loads the height into
 * its low word.  d2 comes in with y in its low word and becomes xbase.
 * BUG(orig) (#902): a6 is the caller's, not graphics.library's (no
 * #graphicslib in the !libs).
 * @param x        d1: x (a word)
 * @param yin      d2: y in the low word, the caller's upper word
 * @param ybase    d3: the caller's d3 (upper word)
 * @param width    d4: the caller's d4 (upper word)
 * @param minterm  d6: the caller's d6 (upper word)
 * @param win      a1: the window object
 * @param gfx      a6: the caller's a6
 */
static void BLITZ_ROUTINE blitgroup_c(UWORD x BL_REG(d1), ULONG yin BL_REG(d2),
                                      ULONG ybase BL_REG(d3), ULONG width BL_REG(d4),
                                      ULONG minterm BL_REG(d6), struct Window **win BL_REG(a1),
                                      struct Library *gfx BL_REG(a6))
{
    struct RastPort *rp = (*win)->RPort;
    const struct member *p = groupaddr;
    ULONG xbase = yin;                  /* d2 */
    struct mapshape *ms;

    BL_SETW(ybase, yin);                /* MOVE.w d2,d3 */
    BL_SETW(xbase, x);                  /* MOVE.w d1,d2 */
    while ((ms = p->ms) != NULL) {
        ULONG mx = xbase, my = ybase;
        BL_SETW(mx, xbase + p->dx);
        BL_SETW(my, ybase + p->dy);
        blit(gfx, ms, rp, mx, my, width, (ULONG)ms, minterm);
        p++;
    }
}
BLITZ_THUNK(_blitgroup, blitgroup_c, "d0-d1/a0-a1");
