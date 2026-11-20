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
 * group: a block of 512 bytes (init, AllocMem MEMF_CLEAR) of members
 * {map shape address.l, dx.w, dy.w}, ended by a 0 address.
 *
 * C behind thunks, but the three blit entries, which are the original's
 * assembler (for speed); their graphics calls get the registers the
 * original passes, with their upper words: x, y (and width, height of
 * BlitMapShapeSize) as the caller's longs, the width/height/minterm the
 * original loads with MOVE.w into the caller's d4/d5/d6 (in BlitGroup d5
 * is the member's address before).
 *
 * Fixed:
 *   - init sets the next-member pointer to its (cleared) group block and
 *     the block size to 0, as at the first load; the original set only
 *     the group's address: an AddGroupMember before the first ResetGroup
 *     wrote at address 0 (#901), and in a second run of a program from
 *     the editor (the library stays loaded) into the run before's freed
 *     group, with the run before's block size (#1134).
 *   - init, finit and SetNumMapShapes allocate and free with AllocMem and
 *     FreeMem (the group block 512 bytes, the map shapes 64 bytes each,
 *     their count kept), on every Kickstart; the original used AllocVec
 *     and FreeVec (exec V36) without a version check: on Kickstart 1.x
 *     every program with the library crashed at start-up (#1141).
 *   - Without the group block (init's allocation failed) ResetGroup writes
 *     nothing and returns 0, AddGroupMember and BlitGroup do nothing; the
 *     original did not check its AllocVec, and the three wrote and read
 *     the group at address 0 (#905).
 *   - BlitGroup's !libs has #graphicslib,#la6, as BlitMapShape's: the
 *     original's had the window only, so BlitGroup called
 *     BltBitMapRastPort through the caller's a6 (#902).
 *   - With runtime error checking ShapeToMapShape refuses a shape of
 *     depth 0 or of more than 8 planes: "Illegal Shape Depth"
 *     (shapetomapshapechk; the original has no error check area).  The
 *     release code is unchanged (an invalid object, D-005): depth 0 writes
 *     65536 plane pointers (DBRA), depths 9-13 write into the unused
 *     bytes 42-63 of the 64-byte map shape, from depth 14 on the pointers
 *     run into the next map shape (#903).
 *   - The group holds at most 63 members (504 bytes and the end mark):
 *     AddGroupMember ignores further ones, and with runtime error checking
 *     refuses them: "Group holds at most 63 members" (addgroupmemberchk).
 *     The original did not count: the 64th member's end mark was written
 *     behind the 512-byte block, each further member 8 bytes more (#904).
 *   - Without map shapes (before SetNumMapShapes, or after one that failed
 *     or asked for 0) ShapeToMapShape, the blits, FreeMapShapes and
 *     AddGroupMember do nothing and MapShapeAddr returns 0; the original
 *     took the map shapes from address 0 on (ShapeToMapShape wrote over
 *     the ExecBase pointer).  With runtime error checking a map shape
 *     number not below the count of the last SetNumMapShapes is refused:
 *     "Map shape not allocated by SetNumMapShapes" (the release code does
 *     not compare it, an invalid argument, D-005), and so is a
 *     BlitMapShapeSize wider than the map shape: "BlitMapShapeSize wider
 *     than the map shape" (#906).
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

/* entry points (thunks, below) */
void _shapetomapshape(void), _blitmapshape(void), _remapmapshape(void);
void _blitmapshapesize(void), _setnummapshapes(void), _mapshapeaddr(void);
void _freemapshapes(void), _addgroupmember(void), _blitgroup(void);
void _resetgroup(void), rimapshapeslib_libinit(void);
void rimapshapeslib_libfinit(void), _shapetomapshapechk(void), _addgroupmemberchk(void);
void _blitmapshapechk(void), _blitmapshapesizechk(void), _mapshapeaddrchk(void);
void _freemapshapeschk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#rimapshapeslib,init,0,finit,0}; error field: the error
   check area (fixed, #903; the original has none) */
BLITZ_LIBHEADER(LIB_rimapshapeslib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD0);
BLITZ_SUBS(_shapetomapshape, _shapetomapshapechk, 0);  /* check: fixed #903 */
BLITZ_NAME("ShapeToMapShape", "source shape,dest Mapshape");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, BL_IA1|BL_PD0, LIB_graphicslib, BL_LA6);
BLITZ_SUBS(_blitmapshape, _blitmapshapechk, 0);        /* check: fixed #906 */
BLITZ_NAME("BlitMapShape", "window#,mapshape#,x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_chipbaselib, BL_LA2, 254, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(_remapmapshape, 0, 0);
BLITZ_NAME("RemapMapShape", "mapshape#,botcol,topcol");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, BL_IA1|BL_PD0, LIB_graphicslib, BL_LA6);
BLITZ_SUBS(_blitmapshapesize, _blitmapshapesizechk, 0);    /* check: fixed #906 */
BLITZ_NAME("BlitMapShapeSize", "window#,mapshape#,x,y,width,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_setnummapshapes, 0, 0);
BLITZ_NAME("SetNumMapShapes", "num");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_mapshapeaddr, _mapshapeaddrchk, 0);        /* check: fixed #906 */
BLITZ_NAME("MapShapeAddr", "mapshape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_freemapshapes, _freemapshapeschk, 0);      /* check: fixed #906 */
BLITZ_NAME("FreeMapShapes", "first,last");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_addgroupmember, _addgroupmemberchk, 0);    /* check: fixed #904 */
BLITZ_NAME("AddGroupMember", "shape#,dx,dy");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
/* fixed (#902): #graphicslib,#la6 as BlitMapShape (the original has the
   window only, so a6 was the caller's) */
BLITZ_LIBS(LIB_windowslib, BL_IA1|BL_PD0, LIB_graphicslib, BL_LA6);
BLITZ_SUBS(_blitgroup, 0, 0);
BLITZ_NAME("BlitGroup", "window#,x,y");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_resetgroup, 0, 0);
BLITZ_NAME("ResetGroup", "blockwidth,blockheight");

BLITZ_NULLSUB_AT(finit, rimapshapeslib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, rimapshapeslib_libinit, 0, 0);

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
/* the map shapes; read by the blit entries (assembler) */
static struct mapshape *baseshape __attribute__((used));
/* their number, for FreeMem (fixed #1141) and the error checks (fixed
   #906); 0 without map shapes */
static UWORD nummapshapes;
static struct member *nextgroupmember;
/* the group, 512 bytes; read by BlitGroup (assembler) */
static struct member *groupaddr __attribute__((used));
/* the place of the 64th member: AddGroupMember adds none there (fixed
   #904; 0 without a group block, so that it adds none at all, #905) */
static struct member *groupend;

/* GetMapShape: ANDI.l #$ffff:LSL.l #6:ADD.l baseshape */
#define MAPSHAPE(n) ((struct mapshape *)((UBYTE *)baseshape + ((ULONG)(UWORD)(n) << 6)))

/* a Blitz shape (shapeslib): struct bb_shape (blitzobj.h) */

/* ---------------------------------------------------------------- init */

/**
 * @brief init: the group block (512 bytes, AllocMem MEMF_PUBLIC|MEMF_CLEAR).
 *
 * AllocMem, not the original's AllocVec (exec V36): the library runs on
 * Kickstart 1.x (fixed #1141).
 *
 * The next-member pointer is set to the (cleared, so empty) group and the
 * block size to 0, as at the first load: fixed (#901, #1134), the original
 * set only the group's address, so an AddGroupMember before ResetGroup
 * wrote at 0, and in a second run from the editor into the run before's
 * freed group with its block size.  The group's end, the place of a 64th
 * member, is kept for AddGroupMember (fixed #904).  Without memory
 * groupaddr, the pointer and the end are 0: the group commands then do
 * nothing (fixed #905, the original did not check the AllocVec).
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    groupaddr = AllocMem(512, MEMF_PUBLIC | MEMF_CLEAR);   /* fixed (#1141) */
    nextgroupmember = groupaddr;        /* fixed (#901, #1134) */
    groupend = groupaddr ? groupaddr + 63 : NULL;   /* fixed (#904) */
    blockwidth = 0;                     /* fixed (#1134): the run before's */
    blockheight = 0;
}
BLITZ_THUNK(rimapshapeslib_libinit, libinit_c, "d0-d1/a0-a1");

/**
 * @brief finit: frees the map shapes and the group block.
 *
 * FreeMem with the sizes the library keeps (fixed #1141: the original's
 * FreeVec is exec V36).
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    if (baseshape) {
        FreeMem(baseshape, (ULONG)nummapshapes << 6);  /* fixed (#1141) */
        baseshape = NULL;
    }
    nummapshapes = 0;                   /* fixed (#906) */
    if (groupaddr) {
        FreeMem(groupaddr, 512);        /* fixed (#1141) */
        groupaddr = NULL;
    }
}
BLITZ_THUNK(rimapshapeslib_libfinit, libfinit_c, "d0-d1/a0-a1");

/* ---------------------------------------------------------- map shapes */

/**
 * @brief ShapeToMapShape shape#,mapshape#: a map shape from a Blitz shape.
 *
 * Width, height, depth and ebwidth of the shape; the plane pointers point
 * into the shape's data (onebpmem apart, not copied).  Without map
 * shapes it does nothing (fixed #906: the original wrote from address 0
 * on); the number is checked by shapetomapshapechk only (fixed #906).
 * Depth 0 writes 65536 plane
 * pointers (DBRA), depths above 8 run into unused[] and the next map
 * shapes: an invalid object, refused by shapetomapshapechk with runtime
 * error checking (fixed #903), here unchecked.
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

    if (!baseshape)                     /* fixed (#906): no map shapes */
        return;
    ms->width = sh->width;
    ms->bm.Rows = sh->height;
    *(UWORD *)&ms->bm.Flags = depth;    /* Flags 0, Depth */
    ms->bm.BytesPerRow = sh->ebwidth;
    /* depth 0 -> 65536 planes (DBRA), more than 8 run into unused[] and
       the next map shapes: refused by the error check (fixed #903) */
    c = depth - 1;
    do {
        *planes++ = p;
        p += onebp;                     /* ADD.w d1,a0 */
    } while (c-- != 0);
}
BLITZ_THUNK(_shapetomapshape, shapetomapshape_c, "d0-d1/a0-a1");

/* The blits are assembler entries, the original's code (for speed:
   BlitMapShape is the per-tile command of a map editor; the C version
   behind a thunk took about 530 cycles more per call than the original,
   9461 against 8931 for a 16 x 16 map shape of 2 planes into a window on
   the a600).  Their register contract is the original's: d0, d1,
   a0, a1 and the low words of d4, d5, d6 change, the rest stays
   (graphics.library keeps d2-d7/a2-a6). */

/**
 * @brief BlitMapShape window#,mapshape#,x,y: a map shape blitted into the
 * window's RastPort (assembler entry).
 *
 * BltBitMapRastPort from 0,0, minterm $C0, the map shape's width and
 * height; nothing for an empty map shape (width 0).  Registers on entry:
 * d1 the map shape number, d2/d3 x and y (the caller's longs go to the
 * call), a1 the window object, a6 graphics.library; width, height and
 * minterm replace the low words of the caller's d4/d5/d6 (MOVE.w).  It
 * falls into _blitmapshape2 (a0 = the map shape, a1 = the RastPort),
 * which BlitGroup calls for each member and which ends in the JMP to
 * BltBitMapRastPort.  Without map shapes it does nothing (fixed #906:
 * the original blitted a "map shape" from address n * 64; one test of
 * the pointer per call); the number is checked by blitmapshapechk only
 * (fixed #906).
 */
BLITZ_ASM_ROUTINE(_blitmapshape,
"   move.l  (a1),a1\n"
"   move.l  50(a1),a1\n"                /* the window's RastPort */
"   move.l  _baseshape(pc),d0\n"        /* GetMapShape */
"   beq.s   0f\n"                       /* fixed (#906): no map shapes */
"   move.l  d0,a0\n"
"   andi.l  #0xffff,d1\n"
"   lsl.l   #6,d1\n"
"   add.l   d1,a0\n"
"_blitmapshape2:\n"
"   tst.w   (a0)\n"                     /* the width: 0 = empty */
"   beq.s   0f\n"
"   moveq   #0,d0\n"
"   moveq   #0,d1\n"
"   move.w  (a0),d4\n"
"   move.w  4(a0),d5\n"                 /* Rows */
"   addq.w  #2,a0\n"                    /* the struct BitMap */
"   move.w  #0xc0,d6\n"
"   jmp     -606(a6)\n"                 /* BltBitMapRastPort */
"0: rts");

/**
 * @brief BlitMapShapeSize window#,mapshape#,x,y,width,height: a part of a
 * map shape blitted into the window's RastPort (assembler entry).
 *
 * As BlitMapShape with the given width (d4) and height (d5), the height
 * at most the map shape's (CMP.w: signed words; the width is not
 * limited); nothing for an empty map shape.  The caller's longs of d2-d5
 * go to the call, the low word of d6 becomes $C0.  Without map shapes it
 * does nothing (fixed #906); the number and the width are checked by
 * blitmapshapesizechk only (fixed #906).
 */
BLITZ_ASM_ROUTINE(_blitmapshapesize,
"   move.l  _baseshape(pc),d0\n"        /* GetMapShape */
"   beq.s   1f\n"                       /* fixed (#906): no map shapes */
"   move.l  d0,a0\n"
"   andi.l  #0xffff,d1\n"
"   lsl.l   #6,d1\n"
"   add.l   d1,a0\n"
"   tst.w   (a0)\n"
"   beq.s   1f\n"
"   move.l  (a1),a1\n"
"   move.l  50(a1),a1\n"
"   moveq   #0,d0\n"
"   moveq   #0,d1\n"
"   cmp.w   4(a0),d5\n"
"   ble.s   0f\n"
"   move.w  4(a0),d5\n"                 /* at most the map shape's height */
"0: addq.w  #2,a0\n"
"   move.w  #0xc0,d6\n"
"   jmp     -606(a6)\n"
"1: rts");

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
 * The old block is freed (FreeMem with the count kept), num * 64 bytes
 * allocated (AllocMem MEMF_PUBLIC|MEMF_CLEAR; fixed #1141: the original's
 * AllocVec and FreeVec are exec V36).  After a failure, or with num 0,
 * there are no map shapes: the commands then do nothing (fixed #906: the
 * original took them from address 0 on).  The number is kept for FreeMem
 * and the error checks (0 without a block).
 * @param num  d0: the number of map shapes (a word)
 */
static void BLITZ_ROUTINE setnummapshapes_c(UWORD num BL_REG(d0))
{
    if (baseshape) {
        FreeMem(baseshape, (ULONG)nummapshapes << 6);  /* fixed (#1141) */
        baseshape = NULL;
    }
    /* AllocMem of 0 bytes returns 0: no map shapes */
    baseshape = AllocMem((ULONG)num << 6, MEMF_PUBLIC | MEMF_CLEAR);  /* fixed (#1141) */
    nummapshapes = baseshape ? num : 0; /* fixed (#906) */
}
BLITZ_THUNK(_setnummapshapes, setnummapshapes_c, "d0-d1/a0-a1");

/**
 * @brief MapShapeAddr(mapshape#): the address of a map shape.
 *
 * Without map shapes 0 (fixed #906: the original returned n * 64).
 * @param n  d0: the map shape number (checked by mapshapeaddrchk only)
 * @return d0: the map shape's address, 0 without map shapes
 */
static struct mapshape *BLITZ_ROUTINE mapshapeaddr_c(UWORD n BL_REG(d0))
{
    if (!baseshape)                     /* fixed (#906) */
        return NULL;
    return MAPSHAPE(n);
}
BLITZ_THUNK(_mapshapeaddr, mapshapeaddr_c, "d1/a0-a1");

/**
 * @brief FreeMapShapes first,last: the map shapes first to last made
 * empty.
 *
 * Nothing if last < first (signed words), else the widths of the map
 * shapes from first to last cleared (the addresses compared signed); the
 * memory stays allocated.  Without map shapes nothing (fixed #906: the
 * original cleared words from address 0 on); the numbers are checked by
 * freemapshapeschk only.
 * @param first  d0: the first map shape number
 * @param last   d1: the last map shape number
 */
static void BLITZ_ROUTINE freemapshapes_c(UWORD first BL_REG(d0), UWORD last BL_REG(d1))
{
    struct mapshape *p, *e;

    if ((WORD)last < (WORD)first || !baseshape)     /* fixed (#906) */
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
 * Without init's block (its allocation failed) it writes nothing and
 * returns 0 (fixed #905: the original cleared the long at address 0).
 * @param bw  d0: the block width
 * @param bh  d1: the block height
 * @return d0: the group's address, 0 without a group block
 */
static struct member *BLITZ_ROUTINE resetgroup_c(UWORD bw BL_REG(d0), UWORD bh BL_REG(d1))
{
    blockwidth = bw;
    blockheight = bh;
    if (groupaddr)                      /* fixed (#905) */
        groupaddr[0].ms = NULL;
    nextgroupmember = groupaddr;
    return groupaddr;
}
BLITZ_THUNK(_resetgroup, resetgroup_c, "d1/a0-a1");

/**
 * @brief AddGroupMember mapshape#,dx,dy: a map shape added to the group.
 *
 * The member {map shape, dx*blockwidth, dy*blockheight (MULU.w, the low
 * words)}, then the end mark.  Before the first ResetGroup it adds to
 * the empty group of init (fixed #901: the original wrote at address 0).
 * A member is added only below the group's end (an unsigned compare): at
 * most 63, further ones are ignored (fixed #904: the original wrote the
 * 64th member's end mark behind the 512-byte block, and each further
 * member 8 bytes more), and none without a group block (fixed #905: the
 * end is 0 then), nor without map shapes (fixed #906: the original's
 * member pointed into low memory).
 * @param n   d0: the map shape number
 * @param dx  d1: the column in blocks
 * @param dy  d2: the row in blocks
 */
static void BLITZ_ROUTINE addgroupmember_c(UWORD n BL_REG(d0), UWORD dx BL_REG(d1),
                                           UWORD dy BL_REG(d2))
{
    struct member *p = nextgroupmember;

    if ((ULONG)p >= (ULONG)groupend)    /* fixed (#904, #905): full, or */
        return;                         /* no group block (both 0) */
    if (!baseshape)                     /* fixed (#906): no map shapes */
        return;
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
 * Without a group block it does nothing (fixed #905: the original read
 * the members from address 0).  a6 is graphics.library: fixed (#902),
 * the original's !libs had no #graphicslib, so a6 was the caller's.
 * Assembler entry, the original's code: d1 x, d2 y, a1 the window
 * object, a6 graphics.library; a5 is kept on the stack, each member's
 * a1/a5/d2/d3 around the call of _blitmapshape2.
 */
BLITZ_ASM_ROUTINE(_blitgroup,
"   move.l  a5,-(a7)\n"
"   move.l  (a1),a1\n"
"   move.l  50(a1),a1\n"                /* the window's RastPort */
"   move.w  d2,d3\n"
"   move.w  d1,d2\n"
"   move.l  _groupaddr(pc),d0\n"
"   beq.s   1f\n"                       /* fixed (#905): no group block */
"   move.l  d0,a5\n"
"0: move.l  (a5)+,d5\n"                 /* the member's map shape */
"   beq.s   1f\n"
"   move.l  d5,a0\n"
"   movem.l a1/a5/d2-d3,-(a7)\n"
"   add.w   (a5)+,d2\n"
"   add.w   (a5)+,d3\n"
"   bsr     _blitmapshape2\n"
"   movem.l (a7)+,a1/a5/d2-d3\n"
"   addq.l  #4,a5\n"
"   bra.s   0b\n"
"1: move.l  (a7)+,a5\n"
"   rts");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(baddepth, "Illegal Shape Depth");          /* fixed, #903 */
BLITZ_ERRCHK_MESSAGE(groupfull, "Group holds at most 63 members");  /* fixed, #904 */
BLITZ_ERRCHK_MESSAGE(nomapshape, "Map shape not allocated by SetNumMapShapes");  /* fixed, #906 */
BLITZ_ERRCHK_MESSAGE(toowide, "BlitMapShapeSize wider than the map shape");   /* fixed, #906 */

/* a map shape number below the count of the last SetNumMapShapes (0
   without map shapes) */
static void BLITZ_ERRCHK numchk(UWORD n)
{
    if (n >= nummapshapes)
        BLITZ_RUNERR(nomapshape);
}

/**
 * @brief The error check of ShapeToMapShape (debugger only).
 *
 * Fixed (#903), not in the original (rimapshapeslib had no error check
 * area): ShapeToMapShape copies one plane pointer per plane of the shape
 * with SUBQ.w #1 ... DBRA into the 8 of the map shape, so a shape of
 * depth 0 writes 65536 pointers and one of more than 8 planes runs into
 * the unused bytes and the next map shape.  The release code is unchanged
 * (an invalid object, D-005).
 *
 * Fixed (#906): the map shape number must be below the count of the last
 * SetNumMapShapes (the release code does not compare it).
 *
 * Errors: "Map shape not allocated by SetNumMapShapes"; "Illegal Shape
 * Depth" (shapeslib's text) for a depth of 0 or above 8.
 * @param n   d1: the map shape number
 * @param sh  a0: the shape (!libs #shapeslib,#ia0|#pd0)
 */
static void BLITZ_ERRCHK shapetomapshapechk_c(UWORD n BL_REG(d1), const struct bb_shape *sh BL_REG(a0))
{
    numchk(n);
    if (!sh->depth || sh->depth > 8)
        BLITZ_RUNERR(baddepth);
}

/**
 * @brief The error check of AddGroupMember (debugger only).
 *
 * Fixed (#904), not in the original: the group block has room for 63
 * members and the end mark; the release code ignores further members,
 * the check tells the programmer.  Without a group block (init's
 * allocation failed) the release code adds nothing, silently (D-018).
 *
 * Fixed (#906): the map shape number must be below the count of the last
 * SetNumMapShapes.
 *
 * Errors: "Map shape not allocated by SetNumMapShapes"; "Group holds at
 * most 63 members" when the group is full.
 * @param n  d0: the map shape number
 */
static void BLITZ_ERRCHK addgroupmemberchk_c(UWORD n BL_REG(d0))
{
    numchk(n);
    if (groupaddr && (ULONG)nextgroupmember >= (ULONG)groupend)
        BLITZ_RUNERR(groupfull);
}

/**
 * @brief The error check of BlitMapShape (debugger only).
 *
 * Fixed (#906), not in the original: the map shape number must be below
 * the count of the last SetNumMapShapes; the release code takes
 * baseshape + number * 64 without a compare.
 *
 * Errors: "Map shape not allocated by SetNumMapShapes".
 * @param n  d1: the map shape number
 */
static void BLITZ_ERRCHK blitmapshapechk_c(UWORD n BL_REG(d1))
{
    numchk(n);
}

/**
 * @brief The error check of BlitMapShapeSize (debugger only).
 *
 * Fixed (#906), not in the original: the number as BlitMapShape's, and
 * the width at most the map shape's (the release code limits the height
 * only: a wider blit reads each row on into the next rows and the last
 * one behind the shape's data).  An empty map shape (width 0) is not
 * blitted and its width not checked.
 *
 * Errors: "Map shape not allocated by SetNumMapShapes"; "BlitMapShapeSize
 * wider than the map shape".
 * @param n  d1: the map shape number
 * @param w  d4: the width
 */
static void BLITZ_ERRCHK blitmapshapesizechk_c(UWORD n BL_REG(d1), UWORD w BL_REG(d4))
{
    const struct mapshape *ms;

    numchk(n);
    ms = MAPSHAPE(n);
    if (ms->width && (WORD)w > (WORD)ms->width)
        BLITZ_RUNERR(toowide);
}

/**
 * @brief The error check of MapShapeAddr (debugger only).
 *
 * Fixed (#906), not in the original: the number as BlitMapShape's.
 *
 * Errors: "Map shape not allocated by SetNumMapShapes".
 * @param n  d0: the map shape number
 */
static void BLITZ_ERRCHK mapshapeaddrchk_c(UWORD n BL_REG(d0))
{
    numchk(n);
}

/**
 * @brief The error check of FreeMapShapes (debugger only).
 *
 * Fixed (#906), not in the original: when the range is not empty (last
 * not below first, signed words, as the release code), both numbers must
 * be below the count of the last SetNumMapShapes.
 *
 * Errors: "Map shape not allocated by SetNumMapShapes".
 * @param first  d0: the first map shape number
 * @param last   d1: the last map shape number
 */
static void BLITZ_ERRCHK freemapshapeschk_c(UWORD first BL_REG(d0), UWORD last BL_REG(d1))
{
    if ((WORD)last < (WORD)first)
        return;
    numchk(first);
    numchk(last);
}

/* the thunks of the check functions */
BLITZ_ERRCHK_THUNK(_shapetomapshapechk, shapetomapshapechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_addgroupmemberchk, addgroupmemberchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_blitmapshapechk, blitmapshapechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_blitmapshapesizechk, blitmapshapesizechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_mapshapeaddrchk, mapshapeaddrchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_freemapshapeschk, freemapshapeschk_c, "d0-d1/a0-a1");
