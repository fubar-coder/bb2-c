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
 *     freed (lost); the fields 24/28 are not cleared then either.
 *   - CreateMap of a map that cannot be allocated (also width or height
 *     0) clears width, height, bwidth, bheight but leaves datasize 1.
 *   - MapCanDraw compares x/y signed: negative values pass and index far
 *     behind the data; the stencil index is the map value as a signed
 *     word; without SetStencil the stencil is at address 0.
 *   - MapDataGrab of bytes keeps the upper byte of the previous value in
 *     the register (MOVE.b): a byte + mapnull beyond 255 carries into the
 *     following values.
 *   - The DBRA loops (MapDataGrab, ShapeTo1616, FillBitplane) run 65536
 *     times for a size of 0; ShapeTo1616 copies a word from every row
 *     regardless of the destination's size.
 *   - M_InitParse copies without limit into its 256-byte buffer (the
 *     pointer behind it is overwritten by a longer string); the comment
 *     "strip leading spaces" of M_NextArg$ is not implemented.
 *   - MapObjName$ returns the length as a byte (ADDQ.b).
 *   - finit closes graphics.library also if it did not open.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
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

/* the MapObject */
#define M_WIDTH         0
#define M_HEIGHT        2
#define M_BWIDTH        4
#define M_BHEIGHT       6
#define M_DATASIZE      8
#define M_MAPNULL       9
#define M_MEMPOINTER    10
#define M_VIEWX         14
#define M_VIEWY         16
#define M_FILENAME      18
#define M_CHANGED       22
#define M_LASTADDRESS   24
#define M_RUBTHROUGHMAP 28

#define MW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define ML(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define MP(p, o) (*(UBYTE **)((UBYTE *)(p) + (o)))

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

static void BLITZ_ROUTINE libinit_c(void)
{
    gfxbase = OpenLibrary((CONST_STRPTR)"graphics.library", 0);
}
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");

/* BUG(orig): CloseLibrary also of a base 0 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    CloseLibrary(gfxbase);
    gfxbase = NULL;
}
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- inline readers */

/* RubThroughMap(map#): a0 = that map, a3 = the used one */
BLITZ_INLINE(_rubthroughmap2, "move.l a0,28(a3)");
/* RubThroughMap */
BLITZ_INLINE(_rubthroughmap1, "move.l 28(a3),d0");
/* ClearRubThroughMap */
BLITZ_INLINE(_clearrubthroughmap, "clr.l 28(a3)");
/* MapCanDrawAddr */
BLITZ_INLINE(_lastcandrawaddress, "move.l 24(a3),d0");

/* SetStencil array */
static void BLITZ_ROUTINE setstencil_c(const UBYTE *a BL_REG(d0))
{
    mapstencil = a;
}
BLITZ_THUNK(_setstencil, setstencil_c, "d0-d1/a0-a1");

/* ----------------------------------------------------------- MapCanDraw */

/* MapCanDraw(shape#,x,y): -1 if x/y lie outside the map (or the rub-through
   map), the map's word there is shape# + 1 (with a rub-through map: that
   map's word), or the stencil byte of the map's word is set; else
   shape# + 1 (resp. the rub-through map's word).  The address of the
   map's word is kept at 24.  d0's upper word stays (ADDQ.w). */
static ULONG BLITZ_ROUTINE candraw_c(ULONG d0 BL_REG(d0), UWORD x BL_REG(d1),
                                     UWORD y BL_REG(d2), UBYTE *map BL_REG(a3))
{
    UBYTE *rub;
    UWORD *cell;
    UWORD v;

    if ((WORD)x >= (WORD)MW(map, M_WIDTH) || (WORD)y >= (WORD)MW(map, M_HEIGHT))
        return 0xffffffffUL;
    rub = MP(map, M_RUBTHROUGHMAP);
    if (rub) {
        ULONG off;
        if ((WORD)x >= (WORD)MW(rub, M_WIDTH) || (WORD)y >= (WORD)MW(rub, M_HEIGHT))
            return 0xffffffffUL;
        off = rol1(bl_mulu_w(y, MW(rub, M_WIDTH)) + x);
        BL_SETW(d0, *(UWORD *)(MP(rub, M_MEMPOINTER) + off) - 1);
    }
    cell = (UWORD *)(MP(map, M_MEMPOINTER) + rol1(bl_mulu_w(y, MW(map, M_WIDTH)) + x));
    MP(map, M_LASTADDRESS) = (UBYTE *)cell;
    BL_SETW(d0, d0 + 1);
    v = *cell;
    if ((UWORD)d0 == v)
        return 0xffffffffUL;
    if (mapstencil[(WORD)v])
        return 0xffffffffUL;
    return d0;
}
BLITZ_THUNK(_candraw, candraw_c, "d1/a0-a1");

/* --------------------------------------------------------------- shapes */

/* a Blitz shape: pixwidth 0, pixheight 2, depth 4, ebwidth 6, data 14,
   cookie 18 */
#define SW(sh, o) (*(UWORD *)((UBYTE *)(sh) + (o)))

/* ScanShapes startnum,highest: a0 = shape startnum, d0 = its offset
   (shape# << 5); from it to the first empty shape or `highest` the
   largest width/height/depth (signed).  -> d0 = the last number. */
static ULONG BLITZ_ROUTINE scanshapes_c(ULONG d0 BL_REG(d0), UWORD highest BL_REG(d1),
                                        const UBYTE *sh BL_REG(a0))
{
    UWORD w = 0, h = 0, d = 0;

    BL_SETW(d0, (WORD)d0 >> 5);
    while (SW(sh, 0)) {
        if (!((WORD)w > (WORD)SW(sh, 0)))
            w = SW(sh, 0);
        if (!((WORD)h > (WORD)SW(sh, 2)))
            h = SW(sh, 2);
        if (!((WORD)d > (WORD)SW(sh, 4)))
            d = SW(sh, 4);
        sh += 32;
        if ((UWORD)d0 == highest)
            break;
        BL_SETW(d0, d0 + 1);
    }
    maxwidth = w;
    maxheight = h;
    maxdepth = d;
    return d0;
}
BLITZ_THUNK(_scanshapes, scanshapes_c, "d1/a0-a1");

/* ScanMaxWidth / ScanMaxheight / ScanMaxdepth (MOVE.w: d0's upper word
   stays) */
static ULONG BLITZ_ROUTINE scanmaxwidth_c(ULONG d0 BL_REG(d0))
{
    return BL_SETW(d0, maxwidth);
}
BLITZ_THUNK(_scanmaxwidth, scanmaxwidth_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE scanmaxheight_c(ULONG d0 BL_REG(d0))
{
    return BL_SETW(d0, maxheight);
}
BLITZ_THUNK(_scanmaxheight, scanmaxheight_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE scanmaxdepth_c(ULONG d0 BL_REG(d0))
{
    return BL_SETW(d0, maxdepth);
}
BLITZ_THUNK(_scanmaxdepth, scanmaxdepth_c, "d1/a0-a1");

/* ShapeTo1616 src,dest: the first word of every row of every plane of
   src into dest's data, 16 rows per plane (missing rows 0, more rows
   skipped).  BUG(orig): height or depth 0 -> 65536 (DBRA) */
static void BLITZ_ROUTINE shapetosquare_c(const UBYTE *src BL_REG(a0), UBYTE *dst BL_REG(a1))
{
    WORD h = (WORD)SW(src, 2);
    ULONG eb = SW(src, 6);
    const UBYTE *s = MP(src, 14);
    UWORD *d = (UWORD *)MP(dst, 14);
    UWORD planes = SW(src, 4) - 1, n;

    if (h == 16) {
        n = (UWORD)bl_mulu_w(SW(src, 4), 16) - 1;   /* depth * 16 rows */
        do {
            *d++ = *(const UWORD *)s;
            s += eb;
            __asm__("" : "+a"(s));     /* no __mulsi3 */
        } while (n-- != 0);
    } else if (h > 16) {
        ULONG skip = bl_mulu_w((UWORD)(h - 16), SW(src, 6));
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

/* ShapeMaskFill shape#: 8 longs of the cookie = -1 */
static void BLITZ_ROUTINE shapemaskfill_c(const UBYTE *sh BL_REG(a0))
{
    ULONG *c = (ULONG *)MP(sh, 18);
    int i;

    for (i = 0; i < 8; i++)
        *c++ = 0xffffffffUL;
}
BLITZ_THUNK(_shapemaskfill, shapemaskfill_c, "d0-d1/a0-a1");

/* FillBitplane bitmap#,width,height: plane 0 of the bitmap object (its
   ebwidth at 0, planes at 8) gets width/16 words per row (at least one:
   DBRA) of $5555/$AAAA, alternating per row */
static void BLITZ_ROUTINE fillbitplane_c(UWORD w BL_REG(d1), UWORD h BL_REG(d2),
                                         const UBYTE *bm BL_REG(a0))
{
    UBYTE *plane = MP(bm, 8);
    ULONG eb = SW(bm, 0);
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
static void freemap(UBYTE *map)
{
    ULONG *p = (ULONG *)map;
    int i;

    if (ML(map, M_MEMPOINTER))
        FreeMem(MP(map, M_MEMPOINTER),
                bl_mulu_w((UWORD)(MW(map, M_WIDTH) << 1), MW(map, M_HEIGHT)));
    if (ML(map, M_FILENAME)) {
        struct bl_regs r = { 0, 0, 0, 0 };
        r.d0 = ML(map, M_FILENAME);
        BLITZ_ALIBJSR(TOK_freest, r);
    }
    for (i = 0; i < 8; i++)
        p[i] = 0;
}

static void BLITZ_ROUTINE freemap_c(UBYTE *map BL_REG(a3))
{
    freemap(map);
}
BLITZ_THUNK(_freemap, freemap_c, "d0-d1/a0-a1");

/* CreateMap map#,width,height,bwidth,bheight,name$: a3 = the map, d6 =
   the offset of the used map in the globals; the length of name$ on a2.
   Data: width*height words (MEMF_CLEAR).  a3 ends behind the last field
   written (24, or 10 if the data could not be allocated). */
static void BLITZ_ROUTINE newmap_c(UWORD w BL_REG(d1), UWORD h BL_REG(d2), UWORD bw BL_REG(d3),
                                   UWORD bh BL_REG(d4), ULONG name BL_REG(d5),
                                   UWORD used BL_REG(d6), const ULONG *lens BL_REG(a2),
                                   UBYTE *map BL_REG(a3))
{
    ULONG len = BL_POPL(lens);
    struct bl_regs r = { 0, 0, 0, 0 };
    UBYTE *mem;

    *(UBYTE **)(blitz_globals + (WORD)used) = map;      /* MOVE.l a3,0(a5,d6) */
    if (ML(map, 12))                    /* BUG(orig): not 10 */
        freemap(map);
    MW(map, M_WIDTH) = w;
    MW(map, M_HEIGHT) = h;
    MW(map, M_BWIDTH) = bw;
    MW(map, M_BHEIGHT) = bh;
    map[M_DATASIZE] = 1;
    map[M_MAPNULL] = 0;
    mem = AllocMem(bl_mulu_w((UWORD)(w << 1), h), MEMF_PUBLIC | MEMF_CLEAR);
    if (!mem) {
        ML(map, 0) = 0;
        ML(map, 4) = 0;
        MW(map, 4) = 0;
        blitz_string = map + 10;
        return;
    }
    MP(map, M_MEMPOINTER) = mem;
    MW(map, M_VIEWX) = 0;
    MW(map, M_VIEWY) = 0;
    r.d0 = name;
    r.d1 = len;
    ML(map, M_FILENAME) = BLITZ_ALIBJSR(TOK_allocst, r);
    MW(map, M_CHANGED) = 0;
    blitz_string = map + 24;
}
BLITZ_THUNK(_newmap, newmap_c, "d0-d1/a0-a1");

/* MapDataAdr: the data */
BLITZ_INLINE(_mapdataadr, "move.l 10(a3),d0");

/* MapDataAdr(x,y): the data + (y*width)*2 + (x*2 as a word) */
BLITZ_INLINE(_mapdataadr2,
             "asl.w   #1,d0\n"
             "   mulu.w  (a3),d1\n"
             "   asl.l   #1,d1\n"
             "   andi.l  #0xffff,d0\n"
             "   add.l   d0,d1\n"
             "   move.l  10(a3),d0\n"
             "   add.l   d1,d0");

/* MapDataSize: (width*2 as a word) * height */
BLITZ_INLINE(_mapdatasize,
             "move.w  (a3),d0\n"
             "   asl.w   #1,d0\n"
             "   mulu.w  2(a3),d0");

/* MapObjWidth, MapObjheight, MapObjViewX, MapObjViewY, MapDataChanged */
BLITZ_INLINE(_mapobjwidth, "move.w (a3),d0");
BLITZ_INLINE(_mapobjheight, "move.w 2(a3),d0");

/* MapDataGrab address,datasize,mapnull: width*height values from address
   (datasize 0: bytes, else words) + mapnull into the data */
static void BLITZ_ROUTINE mapgrab_c(const UBYTE *src BL_REG(d0), UWORD type BL_REG(d1),
                                    UWORD null BL_REG(d2), UBYTE *map BL_REG(a3))
{
    UWORD *dst = (UWORD *)MP(map, M_MEMPOINTER);
    UWORD rows = MW(map, M_HEIGHT) - 1, n;

    if (!type) {
        UWORD v = 0;                    /* d0.w */
        do {
            n = MW(map, M_WIDTH) - 1;
            do {
                v = (v & 0xff00) | *src++;      /* BUG(orig): MOVE.b */
                v += null;
                *dst++ = v;
            } while (n-- != 0);
        } while (rows-- != 0);
    } else {
        do {
            n = MW(map, M_WIDTH) - 1;
            do {
                *dst++ = *(const UWORD *)src + null;
                src += 2;
            } while (n-- != 0);
        } while (rows-- != 0);
    }
}
BLITZ_THUNK(_mapgrab, mapgrab_c, "d0-d1/a0-a1");

/* MapObjName$: a1 = the used map; the name's characters at a3 (no 0),
   d0 = their number as a byte; d0 = 0 without a name */
static ULONG BLITZ_ROUTINE mapobjname_c(const UBYTE *map BL_REG(a1))
{
    const UBYTE *s = MP(map, M_FILENAME);
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

/* SetMapObjName$ name$: the old name freed, the new one copied */
static void BLITZ_ROUTINE setmapobjname_c(ULONG name BL_REG(d0), const ULONG *lens BL_REG(a2),
                                          UBYTE *map BL_REG(a3))
{
    struct bl_regs r = { 0, 0, 0, 0 };

    if (ML(map, M_FILENAME)) {
        r.d0 = ML(map, M_FILENAME);
        BLITZ_ALIBJSR(TOK_freest, r);
    }
    r.d0 = name;
    r.d1 = lens[-1];                    /* MOVE.l -(a2),d1 */
    ML(map, M_FILENAME) = BLITZ_ALIBJSR(TOK_allocst, r);
}
BLITZ_THUNK(_setmapobjname, setmapobjname_c, "d0-d1/a0-a1");

BLITZ_INLINE(_mapobjviewx, "move.w 14(a3),d0");
BLITZ_INLINE(_mapobjviewy, "move.w 16(a3),d0");
BLITZ_INLINE(_mapdatachanged, "move.w 22(a3),d0");

/* MapDataChanged(newstate): the old state, the new one stored */
static ULONG BLITZ_ROUTINE mapdatachanged2_c(ULONG d0 BL_REG(d0), UBYTE *map BL_REG(a3))
{
    UWORD old = MW(map, M_CHANGED);

    MW(map, M_CHANGED) = (UWORD)d0;
    return BL_SETW(d0, old);
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

/* MapOutText x,y,rastport,string */
static void BLITZ_ROUTINE outtext_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1),
                                    struct RastPort *rp BL_REG(d2), const UBYTE *s BL_REG(d3))
{
    outtext(gfxbase, x, y, rp, s);
}
BLITZ_THUNK(_OutText, outtext_c, "d0-d1/a0-a1");

/* MapOutTextCenter: x - length/2 (LSR.w) */
static void BLITZ_ROUTINE outtextcenter_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1),
                                          struct RastPort *rp BL_REG(d2),
                                          const UBYTE *s BL_REG(d3))
{
    struct Library *gfx = gfxbase;

    BL_SETW(x, x - (textlength(gfx, rp, s) >> 1));
    outtext(gfx, x, y, rp, s);
}
BLITZ_THUNK(_OutTextCenter, outtextcenter_c, "d0-d1/a0-a1");

/* MapOutTextRight: x - (length - 1) */
static void BLITZ_ROUTINE outtextright_c(ULONG x BL_REG(d0), ULONG y BL_REG(d1),
                                         struct RastPort *rp BL_REG(d2),
                                         const UBYTE *s BL_REG(d3))
{
    struct Library *gfx = gfxbase;

    BL_SETW(x, x - (UWORD)(textlength(gfx, rp, s) - 1));
    outtext(gfx, x, y, rp, s);
}
BLITZ_THUNK(_OutTextRight, outtextright_c, "d0-d1/a0-a1");

/* MapOutTextPen rastport,pen: SetAPen with d0 = the rastport's upper word
   and the pen (MOVE.w d1,d0) */
static void BLITZ_ROUTINE outtextpen_c(ULONG rp BL_REG(d0), UWORD pen BL_REG(d1))
{
    struct Library *gfx = gfxbase;
    ULONG d0 = rp;

    BL_SETW(d0, pen);
    SetAPen((struct RastPort *)rp, d0);
}
BLITZ_THUNK(_OutTextPen, outtextpen_c, "d0-d1/a0-a1");

/* SetWindowDraw window#,mode: a0 = the used window object (its first
   long the Window); its RastPort's area pattern */
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

/* M_InitParse string: copied (with its 0) into the buffer.  BUG(orig): no
   limit */
static void BLITZ_ROUTINE initparse_c(const UBYTE *s BL_REG(d0))
{
    UBYTE *d = parse.parseadr;

    parse.curadr = d;
    while ((*d++ = *s++) != 0)
        ;
}
BLITZ_THUNK(_InitParse, initparse_c, "d0-d1/a0-a1");

/* M_NextArg$: the characters up to the next ',' (skipped) or the end at
   a3, d0 = their number; after the last one (and before M_InitParse)
   "" */
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
