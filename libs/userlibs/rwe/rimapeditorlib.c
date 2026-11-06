/*
 * rimapeditorlib - RWE userlib 8 (Reflective Images, 1994-97): MapEd's
 * helpers - MapObject objects (block maps of words), shape and bitmap
 * helpers, text output into a RastPort and a parameter parser.
 *
 *   MapObject (32 bytes, 5 objects by default):
 *     0 width.w  2 height.w  4 bwidth.w  6 bheight.w  8 datasize.b (1)
 *     9 mapnull.b  10 data.l (width*height words)  14 viewx.w
 *     16 viewy.w  18 name.l (maxslib string)  22 changed.w
 *     24 last address.l (MapCanDraw)  28 rub-through map.l
 *   CreateMap map#,width,height,bwidth,bheight,name$   (also "uses" it)
 *   MapDataAdr[(x,y)]  MapDataSize  MapObjWidth  MapObjheight
 *   MapObjViewX  MapObjViewY  MapDataChanged[(newstate)]
 *   MapDataGrab address,datasize,mapnull   bytes (0) or words + mapnull
 *   MapObjName$  SetMapObjName$ name$
 *   RubThroughMap[(map#)]  ClearRubThroughMap  SetStencil array
 *   MapCanDraw(shape#,x,y)  MapCanDrawAddr
 *   ShapeTo1616 src,dest   the left 16 pixels of a shape, 16 rows
 *   ShapeMaskFill shape#   the first 32 bytes of the cookie set
 *   ScanShapes(start,highest)  ScanMaxWidth/height/depth
 *   FillBitplane bitmap#,width,height   a mesh in plane 0
 *   MapOutText/MapOutTextCenter/MapOutTextRight x,y,rastport,string
 *   MapOutTextPen rastport,pen
 *   SetWindowDraw window#,mode   area pattern (as rishapezlib)
 *   M_InitParse string  M_NextArg$   comma separated arguments
 *
 * C translation of rwelibsrc/MapEd/MapEd_Bit2.bb; reference: the
 * reference build of that source (no binary exists, docs/rwelibs/README.md
 * 3.3).  Notes: docs/rwelibs/port/rimapeditorlib.md.
 *
 * C behind thunks, except the inline routines (Dc $a000: MapDataAdr,
 * MapDataSize, MapObjWidth/height, MapObjViewX/Y, MapDataChanged without
 * argument, RubThroughMap, ClearRubThroughMap, MapCanDrawAddr), which
 * stay assembler.  a2 stays unchanged (DIFF, the general rule: CreateMap
 * and SetMapObjName$ pop the string length, FillBitplane uses a2 as its
 * pointer); a3 is left as the original leaves it (CreateMap: behind the
 * fields it wrote; the string functions: behind the characters).
 *
 * BUG(orig):
 *   - CreateMap tests the long at 12 (the low word of the data pointer
 *     and viewx) instead of the data pointer to decide whether to free
 *     the old map: a map whose data lies at $xxxx0000 with viewx 0 is not
 *     freed (lost); the fields 24/28 are not cleared then either (#894).
 *   - MapCanDraw compares x/y signed: negative values pass and index far
 *     behind the data (#892); the stencil index is the map value as a
 *     signed word; without SetStencil the stencil is at address 0 (#893).
 *   - MapDataGrab of bytes keeps the upper byte of the previous value in
 *     the register (MOVE.b): a byte + mapnull beyond 255 carries into the
 *     following values (#898).
 *   - The DBRA loops (MapDataGrab, ShapeTo1616, FillBitplane) run 65536
 *     times for a size of 0 (#895); ShapeTo1616 copies a word from every
 *     row regardless of the destination's size (#900).
 *   - M_InitParse copies without limit into its 256-byte buffer (the
 *     pointer behind it is overwritten by a longer string) (#897).
 *   - MapObjName$ returns the length as a byte (ADDQ.b) (#899).
 *   - finit closes graphics.library also if it did not open (#896).
 * Not defects: CreateMap of a map that cannot be allocated (also width
 * or height 0) clears width, height, bwidth, bheight but leaves datasize
 * 1 (nothing reads it then); the comment "strip leading spaces" of
 * M_NextArg$ is not implemented (a comment of the source, not a promise
 * of the command).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include "blitzexec.h"
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
#include <stddef.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks and inline routines, below) */
void _shapetosquare(void), _shapemaskfill(void), _scanshapes(void);
void _scanmaxwidth(void), _scanmaxheight(void), _scanmaxdepth(void);
void _mapdatachanged(void), _mapdatachanged2(void), _fillbitplane(void), _newmap(void);
void _mapdataadr(void), _mapdataadr2(void), _mapdatasize(void), _mapobjwidth(void);
void _mapobjheight(void), _mapgrab(void), _mapobjname(void), _setmapobjname(void);
void _mapobjviewx(void), _mapobjviewy(void), _OutText(void), _rubthroughmap1(void);
void _rubthroughmap2(void), _setstencil(void), _candraw(void), _lastcandrawaddress(void);
void _clearrubthroughmap(void), _OutTextPen(void), _OutTextCenter(void);
void _OutTextRight(void), _SetWindowDraw(void), _InitParse(void), _nextparsearg(void);
void _freemap(void), _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* #ia<a>|#pd<p>: the object of the number in d<p> into a<a>; #ua<a>: the
   used object */
#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))
#define UA(a) (BL_LA(a) | BL_USED)

/* !libheader {#rimapeditorlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rimapeditorlib, init, BL_NORET, finit, 0);

BLITZ_DUMTOKE(_toke, "MapObject", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(0, 0), LIB_shapeslib, IA(1, 1));
BLITZ_SUBS(_shapetosquare, 0, 0);
BLITZ_NAME("ShapeTo1616", "source shape,dest shape");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(0, 0));
BLITZ_SUBS(_shapemaskfill, 0, 0);
BLITZ_NAME("ShapeMaskFill", "shape#");

BLITZ_NULLSUB(0, 0, 0);

BLITZ_NULLSUB(0, 0, 0);

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(0, 0));
BLITZ_SUBS(_scanshapes, 0, 0);
BLITZ_NAME("ScanShapes", "startnum,highest - returns last shape scanned (null shape)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_scanmaxwidth, 0, 0);
BLITZ_NAME("ScanMaxWidth", "- returns max width found after Scan");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_scanmaxheight, 0, 0);
BLITZ_NAME("ScanMaxheight", "- returns max height found after Scan");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_scanmaxdepth, 0, 0);
BLITZ_NAME("ScanMaxdepth", "- returns max depth found after Scan");

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_mapdatachanged, 0, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_mapdatachanged2, 0, 0);
BLITZ_NAME("MapDataChanged", "[newstate] - has map changed since last load?");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, IA(0, 0));
BLITZ_SUBS(_fillbitplane, 0, 0);
BLITZ_NAME("FillBitplane", "bitmap#,width,height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_rimapeditorlib, IA(3, 0), LIB_rimapeditorlib, 0x0604);
BLITZ_SUBS(_newmap, 0, 0);
BLITZ_NAME("CreateMap", "map#,width,height,bwidth,bheight,name$");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS(8, UA(3));                   /* the source's literal 8 */
BLITZ_SUBS(_mapdataadr, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_mapdataadr2, 0, 0);
BLITZ_NAME("MapDataAdr", "[x,y] - returns adress of the current map");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_mapdatasize, 0, 0);
BLITZ_NAME("MapDataSize", "- returns byte size of map data");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_mapobjwidth, 0, 0);
BLITZ_NAME("MapObjWidth", "- returns width of the current map");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_mapobjheight, 0, 0);
BLITZ_NAME("MapObjheight", "- returns height of the current map");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_mapgrab, 0, 0);
BLITZ_NAME("MapDataGrab", "address,datasize,mapnull");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_rimapeditorlib, UA(1));
BLITZ_SUBS(_mapobjname, 0, 0);
BLITZ_NAME("MapObjName$", "- returns name of the current map");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_setmapobjname, 0, 0);
BLITZ_NAME("SetMapObjName$", "name$");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_mapobjviewx, 0, 0);
BLITZ_NAME("MapObjViewX", "- returns viewx of the current map");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_mapobjviewy, 0, 0);
BLITZ_NAME("MapObjViewY", "- returns viewy of the current map");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG, BL_UNKNOWN);
BLITZ_LIBS0();
BLITZ_SUBS(_OutText, 0, 0);
BLITZ_NAME("MapOutText", "x,y,rastport,address/string");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_rubthroughmap1, 0, 0);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_rimapeditorlib, IA(0, 0), LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_rubthroughmap2, 0, 0);
BLITZ_NAME("RubThroughMap", "[mapobject#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(_setstencil, 0, 0);
BLITZ_NAME("SetStencil", "<array> - a byte array holding stencil data");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_candraw, 0, 0);
BLITZ_NAME("MapCanDraw", "(shape#,mapx,mapy)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_lastcandrawaddress, 0, 0);
BLITZ_NAME("MapCanDrawAddr", "- returns last address calculated by DoDraw");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_rimapeditorlib, UA(3));
BLITZ_SUBS(_clearrubthroughmap, 0, 0);
BLITZ_NAME("ClearRubThroughMap", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_OutTextPen, 0, 0);
BLITZ_NAME("MapOutTextPen", "rastport,pen");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG, BL_UNKNOWN);
BLITZ_LIBS0();
BLITZ_SUBS(_OutTextCenter, 0, 0);
BLITZ_NAME("MapOutTextCenter", "x,y,rastport,address/string");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG, BL_UNKNOWN);
BLITZ_LIBS0();
BLITZ_SUBS(_OutTextRight, 0, 0);
BLITZ_NAME("MapOutTextRight", "x,y,rastport,address/string");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, UA(0));
BLITZ_SUBS(_SetWindowDraw, 0, 0);
BLITZ_NAME("SetWindowDraw", "mode    -- 0=normal, 1=meshed");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_InitParse, 0, 0);
BLITZ_NAME("M_InitParse", "string");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_nextparsearg, 0, 0);
BLITZ_NAME("M_NextArg$", "- get next parameter");

BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, _freemap, 0, 0);

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

/* !libfin {_toke,_load,_save,_use,_free,5,5} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 5);

/* the MapObject (32 bytes); the inline routines below use its offsets */
struct mapobject {
    UWORD width, height;                /* 0 2 */
    UWORD bwidth, bheight;              /* 4 6 */
    UBYTE datasize, mapnull;            /* 8 9 */
    UBYTE *data;                        /* 10: width*height words (M_MEMPOINTER) */
    UWORD viewx, viewy;                 /* 14 16 */
    ULONG name;                         /* 18: a maxslib string (M_FILENAME) */
    UWORD changed;                      /* 22 */
    UWORD *lastaddress;                 /* 24: MapCanDraw's map word */
    struct mapobject *rubthrough;       /* 28: the rub-through map */
};
static_assert(sizeof(struct mapobject) == 32);
static_assert(offsetof(struct mapobject, height) == 2);
static_assert(offsetof(struct mapobject, data) == 10);
static_assert(offsetof(struct mapobject, viewx) == 14);
static_assert(offsetof(struct mapobject, viewy) == 16);
static_assert(offsetof(struct mapobject, changed) == 22);
static_assert(offsetof(struct mapobject, lastaddress) == 24);
static_assert(offsetof(struct mapobject, rubthrough) == 28);

/* a Blitz shape object (shapeslib, 32 bytes): struct bb_shape (blitzobj.h) */

static struct Library *gfxbase;
static const UBYTE *mapstencil;
static UWORD maxwidth, maxheight, maxdepth;
static const UWORD mypattern[2] = { 0xaaaa, 0x5555 };
/* M_InitParse's buffer and, behind it as in the original, the pointer */
static struct {
    UBYTE parseadr[256];
    UBYTE *curadr;
} parse;

/* ROL.l #1 */
static inline ULONG rol1(ULONG v)
{
    return v << 1 | v >> 31;
}

/* ---------------------------------------------------------------- init */

/**
 * @brief init: opens graphics.library (any version).
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    gfxbase = OpenLibrary((CONST_STRPTR)"graphics.library", 0);
}
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");

/**
 * @brief finit: closes graphics.library.
 *
 * BUG(orig) (#896): CloseLibrary also of a base 0.
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    CloseLibrary(gfxbase);
    gfxbase = NULL;
}
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- inline readers */

/**
 * @brief RubThroughMap(mapobject#): sets the rub-through map of the used map.
 *
 * Inline: a0 = that map (#ia0|#pd0), a3 = the used one (#ua3); the map's
 * rubthrough (+28) = a0.  MapCanDraw then tests that map too.
 * Returns nothing (d0 unchanged).
 */
BLITZ_INLINE(_rubthroughmap2, "move.l a0,28(a3)");
/**
 * @brief RubThroughMap: the rub-through map of the used map (+28).
 * @par Result
 * d0: the rub-through map, 0 if none
 */
BLITZ_INLINE(_rubthroughmap1, "move.l 28(a3),d0");
/**
 * @brief ClearRubThroughMap: removes the rub-through map of the used map.
 */
BLITZ_INLINE(_clearrubthroughmap, "clr.l 28(a3)");
/**
 * @brief MapCanDrawAddr: the address of the map word of the last MapCanDraw (+24).
 * @par Result
 * d0: the address
 */
BLITZ_INLINE(_lastcandrawaddress, "move.l 24(a3),d0");

/**
 * @brief SetStencil array: the stencil of MapCanDraw.
 *
 * A byte per map value; a set byte makes MapCanDraw fail for that value.
 * @param a  d0: the byte array
 */
static void BLITZ_ROUTINE setstencil_c(const UBYTE *a BL_REG(d0))
{
    mapstencil = a;
}
BLITZ_THUNK(_setstencil, setstencil_c, "d0-d1/a0-a1");

/* ----------------------------------------------------------- MapCanDraw */

/**
 * @brief MapCanDraw(shape#,mapx,mapy): whether a shape can be drawn at a map position.
 *
 * -1 if x/y lie outside the map (or the rub-through map), the map's word
 * there is shape# + 1 (with a rub-through map: that map's word), or the
 * stencil byte of the map's word is set; else shape# + 1 (resp. the
 * rub-through map's word).  The address of the map's word is kept at 24
 * (MapCanDrawAddr).  d0's upper word stays (ADDQ.w).  BUG(orig): x/y are
 * compared signed - negative values pass and index far behind the data
 * (#892); the stencil index is the map value as a signed word, and
 * without SetStencil the stencil is at address 0 (#893).
 * @param shapenum  d0: the shape number
 * @param x         d1: the map x
 * @param y         d2: the map y
 * @param map       a3: the used map (#ua3)
 * @return d0.w: shape# + 1 (or the rub-through word) if it can, -1 if not
 */
static ULONG BLITZ_ROUTINE candraw_c(ULONG shapenum BL_REG(d0), UWORD x BL_REG(d1),
                                     UWORD y BL_REG(d2), struct mapobject *map BL_REG(a3))
{
    struct mapobject *rub;
    UWORD *cell;
    UWORD v;
    ULONG result = shapenum;            /* d0: the shape number or the rub-through word, + 1 */

    if ((WORD)x >= (WORD)map->width || (WORD)y >= (WORD)map->height)
        return 0xffffffffUL;
    rub = map->rubthrough;
    if (rub) {
        ULONG off;
        if ((WORD)x >= (WORD)rub->width || (WORD)y >= (WORD)rub->height)
            return 0xffffffffUL;
        off = rol1(bl_mulu_w(y, rub->width) + x);
        BL_SETW(result, *(UWORD *)(rub->data + off) - 1);
    }
    cell = (UWORD *)(map->data + rol1(bl_mulu_w(y, map->width) + x));
    map->lastaddress = cell;
    BL_SETW(result, result + 1);
    v = *cell;
    if ((UWORD)result == v)
        return 0xffffffffUL;
    if (mapstencil[(WORD)v])
        return 0xffffffffUL;
    return result;
}
BLITZ_THUNK(_candraw, candraw_c, "d1/a0-a1");

/* --------------------------------------------------------------- shapes */

/**
 * @brief ScanShapes(startnum,highest): the largest width/height/depth of a run of shapes.
 *
 * From shape startnum to the first empty shape or `highest`, the largest
 * width, height and depth (signed) for ScanMaxWidth/height/depth.
 * @param shapenum  d0: the offset of shape startnum (shape# << 5)
 * @param highest   d1: the last shape number to scan
 * @param sh        a0: shape startnum (#ia0|#pd0)
 * @return d0.w: the last number scanned (the empty shape, or highest)
 */
static ULONG BLITZ_ROUTINE scanshapes_c(ULONG shapenum BL_REG(d0), UWORD highest BL_REG(d1),
                                        const struct bb_shape *sh BL_REG(a0))
{
    UWORD w = 0, h = 0, d = 0;

    BL_SETW(shapenum, (WORD)shapenum >> 5);
    while (sh->width) {
        if (!((WORD)w > (WORD)sh->width))
            w = sh->width;
        if (!((WORD)h > (WORD)sh->height))
            h = sh->height;
        if (!((WORD)d > (WORD)sh->depth))
            d = sh->depth;
        sh++;
        if ((UWORD)shapenum == highest)
            break;
        BL_SETW(shapenum, shapenum + 1);
    }
    maxwidth = w;
    maxheight = h;
    maxdepth = d;
    return shapenum;
}
BLITZ_THUNK(_scanshapes, scanshapes_c, "d1/a0-a1");

/**
 * @brief ScanMaxWidth: the largest width of the last ScanShapes.
 *
 * MOVE.w: d0's upper word stays (also for ScanMaxheight/ScanMaxdepth).
 * @param incoming  d0: the caller's d0
 * @return d0.w: the width
 */
static ULONG BLITZ_ROUTINE scanmaxwidth_c(ULONG incoming BL_REG(d0))
{
    return BL_SETW(incoming, maxwidth);
}
BLITZ_THUNK(_scanmaxwidth, scanmaxwidth_c, "d1/a0-a1");

/**
 * @brief ScanMaxheight: the largest height of the last ScanShapes.
 * @param incoming  d0: the caller's d0
 * @return d0.w: the height
 */
static ULONG BLITZ_ROUTINE scanmaxheight_c(ULONG incoming BL_REG(d0))
{
    return BL_SETW(incoming, maxheight);
}
BLITZ_THUNK(_scanmaxheight, scanmaxheight_c, "d1/a0-a1");

/**
 * @brief ScanMaxdepth: the largest depth of the last ScanShapes.
 * @param incoming  d0: the caller's d0
 * @return d0.w: the depth
 */
static ULONG BLITZ_ROUTINE scanmaxdepth_c(ULONG incoming BL_REG(d0))
{
    return BL_SETW(incoming, maxdepth);
}
BLITZ_THUNK(_scanmaxdepth, scanmaxdepth_c, "d1/a0-a1");

/**
 * @brief ShapeTo1616 source shape,dest shape: the left 16 pixels of a shape, 16 rows.
 *
 * The first word of every row of every plane of src into dest's data,
 * 16 rows per plane (missing rows 0, more rows skipped).  BUG(orig):
 * height or depth 0 -> 65536 (DBRA) (#895); a word of every row is copied
 * regardless of the destination's size (#900).
 * @param src  a0: the source shape (#ia0|#pd0)
 * @param dst  a1: the destination shape (#ia1|#pd1)
 */
static void BLITZ_ROUTINE shapetosquare_c(const struct bb_shape *src BL_REG(a0),
                                          struct bb_shape *dst BL_REG(a1))
{
    WORD h = (WORD)src->height;
    ULONG eb = src->ebwidth;
    const UBYTE *s = src->data;
    UWORD *d = (UWORD *)dst->data;
    UWORD planes = src->depth - 1, n;

    if (h == 16) {
        n = (UWORD)bl_mulu_w(src->depth, 16) - 1;   /* depth * 16 rows */
        do {
            *d++ = *(const UWORD *)s;
            s += eb;
            __asm__("" : "+a"(s));     /* no __mulsi3 */
        } while (n-- != 0);
    } else if (h > 16) {
        ULONG skip = bl_mulu_w((UWORD)(h - 16), src->ebwidth);
        do {
            n = 15;
            do {
                *d++ = *(const UWORD *)s;
                s += eb;
                __asm__("" : "+a"(s)); /* no __mulsi3 */
            } while (n-- != 0);
            s += skip;
        } while (planes-- != 0);
    } else {
        UWORD zeros = 16 - h - 1, rows = h - 1;
        do {
            n = rows;
            do {
                *d++ = *(const UWORD *)s;
                s += eb;
                __asm__("" : "+a"(s)); /* no __mulsi3 */
            } while (n-- != 0);
            n = zeros;
            do
                *d++ = 0;
            while (n-- != 0);
        } while (planes-- != 0);
    }
}
BLITZ_THUNK(_shapetosquare, shapetosquare_c, "d0-d1/a0-a1");

/**
 * @brief ShapeMaskFill shape#: sets the first 32 bytes (8 longs) of the shape's cookie.
 * @param sh  a0: the shape (#ia0|#pd0)
 */
static void BLITZ_ROUTINE shapemaskfill_c(const struct bb_shape *sh BL_REG(a0))
{
    ULONG *c = (ULONG *)sh->cookie;
    int i;

    for (i = 0; i < 8; i++)
        *c++ = 0xffffffffUL;
}
BLITZ_THUNK(_shapemaskfill, shapemaskfill_c, "d0-d1/a0-a1");

/**
 * @brief FillBitplane bitmap#,width,height: a mesh in plane 0 of a bitmap.
 *
 * Plane 0 of the bitmap object (a struct BitMap: its ebwidth at 0,
 * planes at 8) gets width/16 words per row (at least one: DBRA) of
 * $5555/$AAAA, alternating per row.  BUG(orig) (#895): height 0 -> 65536
 * rows.  DIFF: a2 unchanged (the original uses it as its pointer).
 * @param w   d1: the width in pixels
 * @param h   d2: the height in rows
 * @param bm  a0: the bitmap object (#ia0|#pd0)
 */
static void BLITZ_ROUTINE fillbitplane_c(UWORD w BL_REG(d1), UWORD h BL_REG(d2),
                                         const struct BitMap *bm BL_REG(a0))
{
    UBYTE *plane = bm->Planes[0];
    ULONG eb = bm->BytesPerRow;
    UWORD xs = (UWORD)((w >> 4) - 1), rows = h - 1, pat = 0xaaaa;

    do {
        UWORD *p = (UWORD *)plane, n = xs;
        pat = ~pat;
        do
            *p++ = pat;
        while (n-- != 0);
        plane += eb;
    } while (rows-- != 0);
}
BLITZ_THUNK(_fillbitplane, fillbitplane_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------- the maps */

/* the object's free routine (and CreateMap's): data (FreeMem, the size
   computed like CreateMap's) and name freed, the 32 bytes cleared */
static void freemap(struct mapobject *map)
{
    ULONG *p = (ULONG *)map;
    int i;

    if (map->data)
        FreeMem(map->data, bl_mulu_w((UWORD)(map->width << 1), map->height));
    if (map->name) {
        struct bl_regs r = { 0, 0, 0, 0 };
        r.d0 = map->name;
        BLITZ_ALIBJSR(TOK_freest, r);
    }
    for (i = 0; i < 8; i++)
        p[i] = 0;
}

/**
 * @brief The MapObject's free routine (!libfin _free): frees a map.
 *
 * The data (FreeMem, width*2*height), the name (maxslib freest), the 32
 * bytes cleared.
 * @param map  a3: the map object
 */
static void BLITZ_ROUTINE freemap_c(struct mapobject *map BL_REG(a3))
{
    freemap(map);
}
BLITZ_THUNK(_freemap, freemap_c, "d0-d1/a0-a1");

/**
 * @brief CreateMap map#,width,height,bwidth,bheight,name$: creates a map and uses it.
 *
 * The map becomes the used one; an old map is freed; data: width*height
 * words (MEMF_PUBLIC|MEMF_CLEAR), datasize 1, mapnull 0, view 0,0, the
 * name a maxslib string.  Without memory (also width or height 0) width,
 * height, bwidth, bheight are cleared (datasize stays 1: not a defect).
 * a3 ends behind the last field written (24, or 10 if the data could not
 * be allocated).  BUG(orig) (#894): the old map is freed only if the long
 * at 12 is set (data's low word and viewx), else it is lost and 24/28 are
 * not cleared.  DIFF: a2 unchanged (the original pops the length).
 * @param w     d1: the width
 * @param h     d2: the height
 * @param bw    d3: the block width
 * @param bh    d4: the block height
 * @param name  d5: name$
 * @param used  d6: the offset of the used map in the globals
 * @param lens  a2: the argument stack (the length of name$ below it)
 * @param map   a3: the map object (#ia3|#pd0)
 */
static void BLITZ_ROUTINE newmap_c(UWORD w BL_REG(d1), UWORD h BL_REG(d2), UWORD bw BL_REG(d3),
                                   UWORD bh BL_REG(d4), ULONG name BL_REG(d5),
                                   UWORD used BL_REG(d6), const ULONG *lens BL_REG(a2),
                                   struct mapobject *map BL_REG(a3))
{
    ULONG len = BL_POPL(lens);
    struct bl_regs r = { 0, 0, 0, 0 };
    UBYTE *mem;

    *(struct mapobject **)(blitz_globals + (WORD)used) = map;  /* MOVE.l a3,0(a5,d6) */
    /* BUG(orig) (#894): the long at 12 - data's low word and viewx - not data */
    if (*(ULONG *)((UBYTE *)map + offsetof(struct mapobject, data) + 2))
        freemap(map);
    map->width = w;
    map->height = h;
    map->bwidth = bw;
    map->bheight = bh;
    map->datasize = 1;
    __asm__("" : "+m"(map->datasize));  /* two byte stores, not merged */
    map->mapnull = 0;
    mem = AllocMem(bl_mulu_w((UWORD)(w << 1), h), MEMF_PUBLIC | MEMF_CLEAR);
    if (!mem) {
        *(ULONG *)&map->width = 0;      /* a long: width and height */
        *(ULONG *)&map->bwidth = 0;     /* a long: bwidth and bheight */
        map->bwidth = 0;                /* (once more, as a word) */
        blitz_string = (UBYTE *)&map->data;
        return;
    }
    map->data = mem;
    map->viewx = 0;
    map->viewy = 0;
    r.d0 = name;
    r.d1 = len;
    map->name = BLITZ_ALIBJSR(TOK_allocst, r);
    map->changed = 0;
    blitz_string = (UBYTE *)&map->lastaddress;
}
BLITZ_THUNK(_newmap, newmap_c, "d0-d1/a0-a1");

/**
 * @brief MapDataAdr: the address of the used map's data (+10).
 * @par Result
 * d0: the address
 */
BLITZ_INLINE(_mapdataadr, "move.l 10(a3),d0");

/**
 * @brief MapDataAdr(x,y): the address of a map word.
 *
 * Inline: the data + (y*width)*2 + (x*2 as a word); no range check.
 * @par Result
 * d0: the address (d1 changed)
 */
BLITZ_INLINE(_mapdataadr2,
             "asl.w   #1,d0\n"
             "   mulu.w  (a3),d1\n"
             "   asl.l   #1,d1\n"
             "   andi.l  #0xffff,d0\n"
             "   add.l   d0,d1\n"
             "   move.l  10(a3),d0\n"
             "   add.l   d1,d0");

/**
 * @brief MapDataSize: the byte size of the used map's data.
 *
 * Inline: (width*2 as a word) * height (MULU.w).
 * @par Result
 * d0: the size
 */
BLITZ_INLINE(_mapdatasize,
             "move.w  (a3),d0\n"
             "   asl.w   #1,d0\n"
             "   mulu.w  2(a3),d0");

/* MapObjWidth, MapObjheight, MapObjViewX, MapObjViewY, MapDataChanged:
   inline MOVE.w of the field (d0's upper word stays) */
/**
 * @brief MapObjWidth: the width of the used map.
 * @par Result
 * d0.w: the width
 */
BLITZ_INLINE(_mapobjwidth, "move.w (a3),d0");
/**
 * @brief MapObjheight: the height of the used map.
 * @par Result
 * d0.w: the height
 */
BLITZ_INLINE(_mapobjheight, "move.w 2(a3),d0");

/**
 * @brief MapDataGrab address,datasize,mapnull: fills the used map from memory.
 *
 * width*height values from address (datasize 0: bytes, else words) +
 * mapnull into the data.  BUG(orig): for bytes the upper byte of the
 * previous value stays in the register (MOVE.b): a byte + mapnull beyond
 * 255 carries into the following values (#898); width or height 0 ->
 * 65536 (DBRA, #895).
 * @param src   d0: the address
 * @param type  d1: the data size (0 = bytes)
 * @param null  d2: mapnull, added to every value
 * @param map   a3: the used map (#ua3)
 */
static void BLITZ_ROUTINE mapgrab_c(const UBYTE *src BL_REG(d0), UWORD type BL_REG(d1),
                                    UWORD null BL_REG(d2), struct mapobject *map BL_REG(a3))
{
    UWORD *dst = (UWORD *)map->data;
    UWORD rows = map->height - 1, n;

    if (!type) {
        UWORD v = 0;                    /* d0.w */
        do {
            n = map->width - 1;
            do {
                v = (v & 0xff00) | *src++;      /* BUG(orig): MOVE.b (#898) */
                v += null;
                *dst++ = v;
            } while (n-- != 0);
        } while (rows-- != 0);
    } else {
        do {
            n = map->width - 1;
            do {
                *dst++ = *(const UWORD *)src + null;
                src += 2;
            } while (n-- != 0);
        } while (rows-- != 0);
    }
}
BLITZ_THUNK(_mapgrab, mapgrab_c, "d0-d1/a0-a1");

/**
 * @brief MapObjName$: the name of the used map.
 *
 * The name's characters at a3 (no 0).  BUG(orig) (#899): the length is
 * counted as a byte (ADDQ.b).
 * @param map  a1: the used map (#ua1)
 * @return d0: the number of characters (the low byte), 0 without a name
 */
static ULONG BLITZ_ROUTINE mapobjname_c(const struct mapobject *map BL_REG(a1))
{
    const UBYTE *s = (const UBYTE *)map->name;
    ULONG n = 0;
    UBYTE c;

    if (!s)
        return 0;
    while ((c = *s++) != 0) {
        BL_SETB(n, n + 1);
        *blitz_string++ = c;
    }
    return n;
}
BLITZ_THUNK(_mapobjname, mapobjname_c, "d1/a0-a1");

/**
 * @brief SetMapObjName$ name$: the name of the used map.
 *
 * The old name is freed (maxslib freest), the new one copied (allocst).
 * DIFF: a2 unchanged (the original pops the length).
 * @param name  d0: name$
 * @param lens  a2: the argument stack (lens[-1] = the length of name$)
 * @param map   a3: the used map (#ua3)
 */
static void BLITZ_ROUTINE setmapobjname_c(ULONG name BL_REG(d0), const ULONG *lens BL_REG(a2),
                                          struct mapobject *map BL_REG(a3))
{
    struct bl_regs r = { 0, 0, 0, 0 };

    if (map->name) {
        r.d0 = map->name;
        BLITZ_ALIBJSR(TOK_freest, r);
    }
    r.d0 = name;
    r.d1 = lens[-1];                    /* MOVE.l -(a2),d1 */
    map->name = BLITZ_ALIBJSR(TOK_allocst, r);
}
BLITZ_THUNK(_setmapobjname, setmapobjname_c, "d0-d1/a0-a1");

/**
 * @brief MapObjViewX: the view x of the used map.
 * @par Result
 * d0.w: viewx
 */
BLITZ_INLINE(_mapobjviewx, "move.w 14(a3),d0");
/**
 * @brief MapObjViewY: the view y of the used map.
 * @par Result
 * d0.w: viewy
 */
BLITZ_INLINE(_mapobjviewy, "move.w 16(a3),d0");
/**
 * @brief MapDataChanged: whether the used map has changed since the last load.
 * @par Result
 * d0.w: the changed state
 */
BLITZ_INLINE(_mapdatachanged, "move.w 22(a3),d0");

/**
 * @brief MapDataChanged(newstate): sets the changed state of the used map.
 * @param newstate  d0: the new state (a word)
 * @param map       a3: the used map (#ua3)
 * @return d0.w: the old state (d0's upper word stays)
 */
static ULONG BLITZ_ROUTINE mapdatachanged2_c(ULONG newstate BL_REG(d0),
                                             struct mapobject *map BL_REG(a3))
{
    UWORD old = map->changed;

    map->changed = (UWORD)newstate;
    return BL_SETW(newstate, old);
}
BLITZ_THUNK(_mapdatachanged2, mapdatachanged2_c, "d1/a0-a1");

/* ----------------------------------------------------------------- text */

/* _outtext2: Move to x, y + the font's baseline (words), Text of the
   string (its length as a word) */
static void outtext(struct Library *gfx, ULONG x, ULONG y, struct RastPort *rp,
                    const UBYTE *s)
{
    const UBYTE *p = s;
    UWORD n = 0;

    BL_SETW(y, y + rp->Font->tf_Baseline);
    Move(rp, (LONG)x, (LONG)y);
    while (*p++)
        n++;
    Text(rp, (CONST_STRPTR)s, n);
}

/* the length of the string in pixels (TextLength, the count a word) */
static UWORD textlength(struct Library *gfx, struct RastPort *rp, const UBYTE *s)
{
    const UBYTE *p = s;
    UWORD n = 0;

    while (*p++)
        n++;
    return (UWORD)TextLength(rp, (CONST_STRPTR)s, n);
}

/**
 * @brief MapOutText x,y,rastport,address/string: prints a text into a RastPort.
 *
 * Move to x, y + the font's baseline (words), Text of the string (its
 * length as a word).
 * @param x   d0: x
 * @param y   d1: y
 * @param rp  d2: the RastPort
 * @param s   d3: the string (or its address)
 */
static void BLITZ_ROUTINE outtext_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1),
                                    struct RastPort *rp BL_REG(d2), const UBYTE *s BL_REG(d3))
{
    outtext(gfxbase, x, y, rp, s);
}
BLITZ_THUNK(_OutText, outtext_c, "d0-d1/a0-a1");

/**
 * @brief MapOutTextCenter x,y,rastport,address/string: as MapOutText, centred on x.
 *
 * x - TextLength/2 (LSR.w), as a word.
 * @param x   d0: x
 * @param y   d1: y
 * @param rp  d2: the RastPort
 * @param s   d3: the string
 */
static void BLITZ_ROUTINE outtextcenter_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1),
                                          struct RastPort *rp BL_REG(d2),
                                          const UBYTE *s BL_REG(d3))
{
    struct Library *gfx = gfxbase;

    BL_SETW(x, x - (textlength(gfx, rp, s) >> 1));
    outtext(gfx, x, y, rp, s);
}
BLITZ_THUNK(_OutTextCenter, outtextcenter_c, "d0-d1/a0-a1");

/**
 * @brief MapOutTextRight x,y,rastport,address/string: as MapOutText, ending at x.
 *
 * x - (TextLength - 1), as a word.
 * @param x   d0: x
 * @param y   d1: y
 * @param rp  d2: the RastPort
 * @param s   d3: the string
 */
static void BLITZ_ROUTINE outtextright_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1),
                                         struct RastPort *rp BL_REG(d2),
                                         const UBYTE *s BL_REG(d3))
{
    struct Library *gfx = gfxbase;

    BL_SETW(x, x - (UWORD)(textlength(gfx, rp, s) - 1));
    outtext(gfx, x, y, rp, s);
}
BLITZ_THUNK(_OutTextRight, outtextright_c, "d0-d1/a0-a1");

/**
 * @brief MapOutTextPen rastport,pen: the pen of MapOutText.
 *
 * SetAPen with d0 = the rastport's upper word and the pen (MOVE.w d1,d0).
 * @param rp   d0: the RastPort
 * @param pen  d1: the pen (a word)
 */
static void BLITZ_ROUTINE outtextpen_c(ULONG rp BL_REG(d0), UWORD pen BL_REG(d1))
{
    struct Library *gfx = gfxbase;
    ULONG rppen = rp;

    BL_SETW(rppen, pen);
    SetAPen((struct RastPort *)rp, rppen);
}
BLITZ_THUNK(_OutTextPen, outtextpen_c, "d0-d1/a0-a1");

/**
 * @brief SetWindowDraw window#,mode: the area pattern of the used window (0 normal, 1 meshed).
 *
 * The window's RastPort gets the pattern $AAAA/$5555 (AreaPtSz 1) for a
 * mode other than 0, none for 0.
 * @param mode  d1: the mode
 * @param win   a0: the used window object (#ua0, its first long the Window)
 */
static void BLITZ_ROUTINE setwindowdraw_c(UWORD mode BL_REG(d1),
                                          struct Window **win BL_REG(a0))
{
    struct RastPort *rp = (*win)->RPort;

    if (mode) {
        rp->AreaPtrn = (UWORD *)mypattern;
        rp->AreaPtSz = 1;
    } else {
        rp->AreaPtrn = NULL;
        rp->AreaPtSz = 0;
    }
}
BLITZ_THUNK(_SetWindowDraw, setwindowdraw_c, "d0-d1/a0-a1");

/* -------------------------------------------------------------- parsing */

/**
 * @brief M_InitParse string: the string for M_NextArg$.
 *
 * Copied (with its 0) into the 256 byte buffer.  BUG(orig) (#897): no
 * limit - a longer string overwrites the pointer behind the buffer.
 * @param s  d0: the string
 */
static void BLITZ_ROUTINE initparse_c(const UBYTE *s BL_REG(d0))
{
    UBYTE *d = parse.parseadr;

    parse.curadr = d;
    while ((*d++ = *s++) != 0)
        ;
}
BLITZ_THUNK(_InitParse, initparse_c, "d0-d1/a0-a1");

/**
 * @brief M_NextArg$: the next comma separated argument of M_InitParse's string.
 *
 * The characters up to the next ',' (skipped) or the end at a3; after
 * the last one (and before M_InitParse) "".  Leading spaces are not
 * stripped (the source's comment says so; not a defect).
 * @return d0: the number of characters
 */
static ULONG BLITZ_ROUTINE nextparsearg_c(void)
{
    const UBYTE *p = parse.curadr;
    ULONG n = 0;
    UBYTE c;

    if (!p)
        return 0;
    for (;;) {
        c = *p++;
        if (!c) {
            parse.curadr = NULL;
            return n;
        }
        if (c == ',') {
            parse.curadr = (UBYTE *)p;
            return n;
        }
        *blitz_string++ = c;
        n++;
    }
}
BLITZ_THUNK(_nextparsearg, nextparsearg_c, "d1/a0-a1");
