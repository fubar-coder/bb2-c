/*
 * shapeslib - command library 154: shapes (de-compressed graphics):
 * LoadShape MakeCookie ShapeWidth ShapeHeight Handle AutoCookie CopyShape
 * GetaShape SaveShapes LoadShapes MidHandle DecodeShapes InitShape;
 * object library "Shape" (32 bytes, maximum 100); for other libraries the
 * nullsubs +11 docookie (a3 = shape; shapetrixlib) and +14 b_getashape.
 *
 * C translation of bblibsrc/shapeslib ("20/8/93 waitblit after getashape
 * b4 makecookie added by Simon, cludgeshapes added by simon"); binary of
 * the CD's acidlibs checked: same as the source (LoadShape with ANIM
 * delta frames included).
 *
 * A shape (32 bytes): +0 pixel width (0 = none), +2 height, +4 depth,
 * +6 even byte width, +8 bltsize (height<<6 | words+1), +10/+12 handle,
 * +14 data (plane after plane, CHIP), +18 cookie (OR of all planes, CHIP)
 * or 0, +22 bytes per plane, +24 per plane + one word per row, +26 whole
 * shape, +28 whole + one word per row, +30 "cL" = data is not ours
 * (DecodeShapes in place, never freed).  SaveShapes writes the 32 byte
 * blocks, each followed by the data (+26 bytes) for a used shape;
 * DecodeShapes reads the same format from memory.
 *
 * GetaShape (Amiga mode: OwnBlitter around the Blitz mode routine) lets
 * bitmaplib (+4) create a bitmap of the shape's size in tbmap and copies
 * the area with scrolllib Scroll into it; the planes become the shape's
 * data.  LoadShape reads an ILBM through ilbmifflib (ilbminit,
 * ilbmgetbyte) and iffiolib; after the first frame every "ANHD" (ANIM op
 * 5, interleave 1) gives a delta frame in the NEXT shape object.
 *
 * BUG(orig), kept:
 *  - Row/plane steps ADD x(a3),An are word additions (sign extended for
 *    planes of 32K and more); sizes are low words of MULU. (#263)
 *  - LoadShape: "BODY" is not checked (the decoder reads whatever
 *    follows) (#264); ANIM frames go into the following shape objects
 *    without a check against the maximum (#265).  Not a bug: only `depth`
 *    DLTA pointers are read (one per plane); the copied frame shares
 *    nothing (its cookie is cleared, made again with AutoCookie On).
 *  - LoadShapes: the shape block read from the file keeps its +30
 *    ("cL" from a saved DecodeShapes shape: the data is never freed). (#267)
 *  - CopyShape copies +30 too; freeashape leaves "cL" in +30 (a later
 *    shape in that object is never freed either). (#268, #269)
 *  - Not a bug (by design): DecodeShapes stops silently at the first block
 *    that does not match (the end of the sequence); memory below 2 MB
 *    ($FFE00000 clear, chip RAM) is used in place ("cL"), above it is
 *    copied to CHIP memory.
 *  - Not a bug (word results, the compiler uses only d0.w):
 *    ShapeWidth/ShapeHeight load only the low word of d0.
 *  - DecodeShapes with address 0 jumps (BEQ) into the error check area
 *    (err10): without the debugger behind the library. (#270)
 *  - LoadShapes s,file$ / DecodeShapes s,mem: the last shape is base +
 *    (max - 1) << 5 as a word (SUBQ/LSL/ADD d2,a2): from a maximum of
 *    1025 on it lies before the table. (#939)
 * err2/err3/err7/err8
 * (files, ANIM) are in the normal code with their messages in the error
 * check area, as in the original.
 */
#include "blitzlib.h"
#include <stddef.h>
#define __NOLIBBASE__
#include <proto/dos.h>
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks / assembler, defined below) */
void doshload(void), doshload2(void), docookie(void), doshapew(void);
void doshapeh(void), dohandle(void), doautocookie(void), docopyshape(void);
void dogetashape(void), b_getashape(void), dosaveshapes(void);
void doloadshapes(void), doloadshapes2(void), domidhandle(void);
void _cludgeshapes(void), _cludgeshapes2(void), _initshape(void);
void codeinit(void), freeashape(void);
void cookiechk(void), funcchk(void), gschk(void), schkz(void);

#define AMIGAONLY(entry) ((char *)(entry) + 1)

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#shapeslib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_shapeslib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, 0x1080);
BLITZ_SUBS(AMIGAONLY(doshload), 0, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1080, LIB_palettelib, 0x1182);
BLITZ_SUBS(AMIGAONLY(doshload2), 0, 0);
BLITZ_NAME("LoadShape", "Shape#,Filename$[,Palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380);
BLITZ_SUBS(docookie, cookiechk, 0);
BLITZ_NAME("MakeCookie", "Shape#");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1080);
BLITZ_SUBS(doshapew, funcchk, 0);
BLITZ_NAME("ShapeWidth", "(Shape#)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1080);
BLITZ_SUBS(doshapeh, funcchk, 0);
BLITZ_NAME("ShapeHeight", "(Shape#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380);
BLITZ_SUBS(dohandle, cookiechk, 0);
BLITZ_NAME("Handle", "Shape#,X,Y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doautocookie, 0, 0);
BLITZ_NAME("AutoCookie", "On|Off");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_shapeslib, 0x1281);
BLITZ_SUBS(docopyshape, cookiechk, 0);
BLITZ_NAME("CopyShape", "Shape#,Shape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_bitmaplib, 0x1202, LIB_graphicslib, 0x5600);
BLITZ_SUBS(dogetashape, gschk, 0);
BLITZ_NAME("GetaShape", "Shape#,X,Y,Width,Height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_shapeslib, 0x1281, LIB_doslib, BL_LA(6));
BLITZ_SUBS(AMIGAONLY(dosaveshapes), schkz, 0);
BLITZ_NAME("SaveShapes", "Shape#,Shape#,Filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_shapeslib, 0x1201, LIB_shapeslib, 0x0203,
           LIB_doslib, BL_LA(6));
BLITZ_SUBS(AMIGAONLY(doloadshapes2), 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_shapeslib, 0x1281, LIB_doslib, BL_LA(6));
BLITZ_SUBS(AMIGAONLY(doloadshapes), schkz, 0);
BLITZ_NAME("LoadShapes", "Shape#[,Shape#],Filename$");

BLITZ_NULLSUB(docookie, 0, 0);

BLITZ_DUMTOKE(_toke, "Shape", "- image");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380);
BLITZ_SUBS(domidhandle, cookiechk, 0);
BLITZ_NAME("MidHandle", "Shape#");

BLITZ_NULLSUB(b_getashape, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_shapeslib, 0x1201, LIB_shapeslib, 0x0203);
BLITZ_SUBS(_cludgeshapes2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_shapeslib, 0x1380, LIB_shapeslib, 0x1281);
BLITZ_SUBS(_cludgeshapes, 0, 0);
BLITZ_NAME("DecodeShapes", "Shape#[,Shape#],MemoryLocation");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1380);
BLITZ_SUBS(_initshape, 0, 0);
BLITZ_NAME("InitShape", "Shape#,Width,Height,Depth");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freeashape, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,100,5} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 100, 5);

/* -------------------------------------------- calls to other libraries */

/* registers for XJSR: in d0-d3/a0-a3, out d0/d1/a0/a1 and the Z flag */
struct xregs {
    ULONG d0, d1, d2, d3, a0, a1, a2, a3;
    UBYTE z;
};

/* ALibJsr tok with the registers of r; a3/a4/a6 are restored */
#ifndef BL_CODE_MODEL
#define XJSR(tok, r) do {                                                   \
    register struct xregs *_r __asm("a6") = &(r);                           \
    __asm volatile("movem.l a3-a4/a6,-(sp)\n\t"                             \
                   "movem.l (a6),d0-d3/a0-a3\n\t"                           \
                   "jsr _blitz_tok+%c1\n\t"                                 \
                   "move.l 8(sp),a6\n\t"                                    \
                   "seq 32(a6)\n\t"                                         \
                   "move.l d0,(a6)\n\t"                                     \
                   "move.l d1,4(a6)\n\t"                                    \
                   "move.l a0,16(a6)\n\t"                                   \
                   "move.l a1,20(a6)\n\t"                                   \
                   "movem.l (sp)+,a3-a4/a6"                                 \
        : : "a"(_r), "i"(BL_TOKVAL(tok, 0))                                 \
        : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a0", "a1", "a2", \
          "cc", "memory");                                                  \
} while (0)
#else
#define XJSR(tok, r) ((void)(r))
#endif

#define TOK_extbitmap   ((LIB_bitmaplib << 7) + 4)
#define TOK_scroll      ((LIB_scrolllib << 7) + 2)
#define TOK_palload     ((LIB_palettelib << 7) + 3)
#define TOK_ilbminit    ((LIB_ilbmifflib << 7) + 1)
#define TOK_ilbmgetbyte ((LIB_ilbmifflib << 7) + 2)
#define TOK_iffclose    ((LIB_iffiolib << 7) + 2)
#define TOK_iffbyte     ((LIB_iffiolib << 7) + 3)
#define TOK_ifflong     ((LIB_iffiolib << 7) + 5)
#define TOK_ifffind     ((LIB_iffiolib << 7) + 6)
#define TOK_iffskip     ((LIB_iffiolib << 7) + 7)

static void *globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return (void *)BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(void *mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = (ULONG)mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* ------------------------------------------------------------ shapes */

/* a shape object, 32 bytes (also the block SaveShapes writes) */
struct shape {
    UWORD width;                /* +0  pixel width, 0 = no shape */
    UWORD height;               /* +2 */
    UWORD depth;                /* +4 */
    UWORD ebwidth;              /* +6  even byte width */
    UWORD bltsize;              /* +8  height << 6 | words + 1 */
    WORD handlex, handley;      /* +10/+12 */
    UBYTE *data;                /* +14 plane after plane, CHIP */
    UBYTE *cookie;              /* +18 OR of all planes, CHIP, or 0 */
    UWORD planesize;            /* +22 bytes per plane */
    UWORD xplanesize;           /* +24 per plane + one word per row */
    UWORD size;                 /* +26 the whole shape */
    UWORD xsize;                /* +28 whole + one word per row */
    UWORD cludge;               /* +30 "cL": the data is not ours */
};
static_assert(sizeof(struct shape) == 32, "shape size");

/* other objects (the bitmap, ilbmifflib's state) and nall's fields */
#define SW(s, o) (*(UWORD *)((UBYTE *)(s) + (o)))
#define SL(s, o) (*(ULONG *)((UBYTE *)(s) + (o)))
#define SP(s, o) (*(UBYTE **)((UBYTE *)(s) + (o)))
/* the two handles as one long (CLR.l 10(a3)) */
#define HANDLES(s) (*(ULONG *)&(s)->handlex)

#define CLUDGE 0x634c                           /* "cL" */

static UWORD autocook = 0xffff;                 /* AutoCookie (default on) */

/* tbmap: a bitmap object (64 bytes) for GetaShape, then the 2 words
   Scroll reads below a2 (tdata) */
static struct {
    UBYTE bm[64];
    UWORD h, w;                                 /* -4(a2), -2(a2) */
} BL_PACKED tb __attribute__((aligned(4)));

BLITZ_ERRCHK_MESSAGE(mess1, "Uninitialized Shape");
BLITZ_ERRCHK_MESSAGE(mess2, "Can't open Shapes File");
BLITZ_ERRCHK_MESSAGE(mess3, "Can't Read/Write Shapes File");
BLITZ_ERRCHK_MESSAGE(mess4, "First shape must be lower than second");
BLITZ_ERRCHK_MESSAGE(mess5, "Illegal area for GetaShape");
BLITZ_ERRCHK_MESSAGE(mess6, "Uninitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess7, "ANIM format not supported");
BLITZ_ERRCHK_MESSAGE(mess8, "Illegal ANIM interleave value");
BLITZ_ERRCHK_MESSAGE(mess9, "Cannot GetaShape from an interleaved Bitmap");
BLITZ_ERRCHK_MESSAGE(mess10, "Shapes file at memory location 0? NOWAY!");

/* err10: in the error check area, reached with BEQ from DecodeShapes (as
   in the original: without the debugger a jump behind the library) */
void err10(void);
__asm__("   .section .blitzlib.errchk,\"ax\"\n"
        "   .even\n"
        "_err10:\n"
        "   move.l  #_mess10,d0\n"
        "   trap    #0\n"
        "   .text\n");
/* TST.l d2:BEQ err10 */
#define JMP_ERR10() __asm__ volatile("jmp _err10")

/** @brief Library init: AutoCookie on. */
static void BLITZ_ROUTINE codeinit_c(void)
{
    autocook = 0xffff;
}
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");

/* free a shape's data and cookie (a3) */
static void freeashape_(struct shape *s)
{
    ULONG c;

    if (!s->width)
        return;
    s->width = 0;
    if (s->cludge != CLUDGE) {
        s->cludge = 0;
        globfree(s->data, s->size);
    }
    c = (ULONG)s->cookie;
    if (c) {
        s->cookie = NULL;
        globfree((void *)c, s->planesize);
    }
}

/**
 * @brief Object free routine (_free) of a Shape: free its data and cookie.
 *
 * a3 = the shape.  The width is cleared; the data is freed unless +30 is
 * "cL" (DecodeShapes data in place), the cookie if there is one.
 *
 * BUG(orig) (#269): "cL" stays in +30, so a later shape in this object
 * is never freed either.
 */
static void BLITZ_ROUTINE freeashape_c(void)
{
    freeashape_((struct shape *)blitz_string);
}
BLITZ_THUNK(freeashape, freeashape_c, "d0-d1/a0-a1");

/* make the cookie: OR of all planes */
static void cookie_(struct shape *s)
{
    UBYTE *plane, *ck;
    UWORD psize, rows;

    if (s->cookie) {
        void *c = s->cookie;
        s->cookie = NULL;
        globfree(c, s->planesize);
    }
    ck = globalloc(s->planesize, MEMF_CHIP | MEMF_CLEAR);
    s->cookie = ck;
    psize = s->planesize;                          /* offset plane to plane */
    plane = s->data;
    rows = s->height - 1;                        /* hite */
    do {
        UWORD planes = s->depth - 1;            /* depth */
        UBYTE *src = plane, *dst = ck;

        do {
            UWORD words = (UWORD)(s->ebwidth >> 1) - 1;

            do {
                *(UWORD *)dst |= *(UWORD *)src;
                src += 2;
                dst += 2;
            } while (words-- != 0);
            src -= (WORD)s->ebwidth;
            src += (WORD)psize;
            dst -= (WORD)s->ebwidth;
            __asm__("" : "+a"(src));            /* no __mulsi3 */
        } while (planes-- != 0);
        plane += (WORD)s->ebwidth;
        ck += (WORD)s->ebwidth;
    } while (rows-- != 0);
}

/**
 * @brief MakeCookie Shape# - make the shape's cookie, also nullsub +11
 * docookie for other libraries (shapetrixlib).
 *
 * a3 = the shape.  An old cookie is freed; the new one (bytes per plane,
 * CHIP, cleared, not checked) is the OR of all planes.
 *
 * Errors: "Uninitialized Shape" (cookiechk, debugger only).
 */
static void BLITZ_ROUTINE docookie_c(void)
{
    cookie_((struct shape *)blitz_string);
}
BLITZ_THUNK(docookie, docookie_c, "d0-d1/a0-a1");

/* ------------------------------------------------- small statements */

/**
 * @brief AutoCookie On|Off - make cookies automatically for the shapes of
 * LoadShape(s), GetaShape, InitShape and DecodeShapes.
 *
 * @param v d0.w: <> 0 = on
 */
static void BLITZ_ROUTINE doautocookie_c(ULONG v BL_REG(d0))
{
    autocook = (UWORD)v;
}
BLITZ_THUNK(doautocookie, doautocookie_c, "d0-d1/a0-a1");

/**
 * @brief ShapeWidth(Shape#) - the shape's pixel width.
 *
 * MOVE (a0),d0 - only the low word (not a bug: a word function, the
 * compiler uses only d0.w).
 *
 * Errors: "Uninitialized Shape" (funcchk, debugger only).
 *
 * @param s a0: the shape
 * @return d0.w: the width
 */
static UWORD BLITZ_ROUTINE shapew_c(struct shape *s BL_REG(a0)) { return s->width; }
/**
 * @brief ShapeHeight(Shape#) - the shape's pixel height (only the low word
 * of d0, as ShapeWidth).
 *
 * Errors: "Uninitialized Shape" (funcchk, debugger only).
 *
 * @param s a0: the shape
 * @return d0.w: the height
 */
static UWORD BLITZ_ROUTINE shapeh_c(struct shape *s BL_REG(a0)) { return s->height; }
BLITZ_THUNK_POST(doshapew, shapew_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK_POST(doshapeh, shapeh_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

/**
 * @brief MidHandle Shape# - put the handle in the middle (width/2,
 * height/2); a3 = the shape.
 *
 * Errors: "Uninitialized Shape" (cookiechk, debugger only).
 */
static void BLITZ_ROUTINE domidhandle_c(void)
{
    struct shape *s = (struct shape *)blitz_string;

    s->handlex = s->width >> 1;
    s->handley = s->height >> 1;
}
BLITZ_THUNK(domidhandle, domidhandle_c, "d0-d1/a0-a1");

/**
 * @brief Handle Shape#,X,Y - set the shape's handle; a3 = the shape.
 *
 * Errors: "Uninitialized Shape" (cookiechk, debugger only).
 *
 * @param x d1.w: the handle x
 * @param y d2.w: the handle y
 */
static void BLITZ_ROUTINE dohandle_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2))
{
    struct shape *s = (struct shape *)blitz_string;

    s->handlex = (UWORD)x;
    s->handley = (UWORD)y;
}
BLITZ_THUNK(dohandle, dohandle_c, "d0-d1/a0-a1");

/* CopyShape: a3 = source, a2 = destination (docopyshape2 + cookie) */
static void nall(struct shape *from, struct shape *to, UWORD ptroff, UWORD sizeoff)
{
    UBYTE *src, *dst;
    UWORD n;

    if (!SL(from, ptroff))
        return;
    dst = globalloc(SW(from, sizeoff), MEMF_CHIP);
    SP(to, ptroff) = dst;
    src = SP(from, ptroff);
    n = (UWORD)(SW(from, sizeoff) >> 1) - 1;
    do {
        *(UWORD *)dst = *(UWORD *)src;
        dst += 2;
        src += 2;
    } while (n-- != 0);
}

static void copyshape2(struct shape *from, struct shape *to)
{
    int i;

    freeashape_(to);
    for (i = 0; i < 8; i++)
        ((ULONG *)to)[i] = ((ULONG *)from)[i];
    nall(from, to, offsetof(struct shape, data), offsetof(struct shape, size));
}

/**
 * @brief CopyShape Shape#,Shape# - copy the first shape (a3) into the
 * second.
 *
 * The destination is freed, the 32 byte block copied, then the data and
 * the cookie are copied into new CHIP memory (not checked).
 *
 * BUG(orig) (#268): +30 is copied too ("cL" of a DecodeShapes shape: the
 * copy is never freed).  Errors: "Uninitialized Shape" (cookiechk,
 * debugger only).
 *
 * @param dst a2: the destination shape
 */
static void BLITZ_ROUTINE docopyshape_c(struct shape *dst BL_REG(a2))
{
    copyshape2((struct shape *)blitz_string, dst);
    nall((struct shape *)blitz_string, dst, offsetof(struct shape, cookie),
         offsetof(struct shape, planesize));
}
BLITZ_THUNK(docopyshape, docopyshape_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------ InitShape */

/**
 * @brief InitShape Shape#,Width,Height,Depth - create an empty shape.
 *
 * a3 = the shape; the old one is freed.  Data cleared (CHIP, not
 * checked), handle 0, a cleared cookie with AutoCookie on.
 *
 * BUG(orig) (#263): the sizes are the low words of MULU.
 *
 * @param w d1.w: the width in pixels
 * @param h d2.w: the height
 * @param d d3.w: the depth
 */
static void BLITZ_ROUTINE initshape_c(ULONG w BL_REG(d1), ULONG h BL_REG(d2), ULONG d BL_REG(d3))
{
    struct shape *s = (struct shape *)blitz_string;
    UWORD bpr;
    UWORD bltsize;                              /* d5 */
    UWORD xbpr;                                 /* d5: bpr + the extra word */
    ULONG len, xlen;

    freeashape_(s);
    bltsize = (UWORD)((UWORD)h << 6);
    bpr = (UWORD)((UWORD)(w + 15) >> 4);
    bltsize |= bpr;
    bpr += bpr;
    bltsize += 1;
    s->width = (UWORD)w;
    s->height = (UWORD)h;
    s->depth = (UWORD)d;
    s->ebwidth = bpr;
    s->bltsize = bltsize;
    HANDLES(s) = 0;
    xbpr = bpr + 2;
    len = (ULONG)bpr * (UWORD)h;                /* MULU */
    xlen = (ULONG)xbpr * (UWORD)h;
    s->planesize = (UWORD)len;
    s->xplanesize = (UWORD)xlen;
    len = (ULONG)(UWORD)len * (UWORD)d;
    xlen = (ULONG)(UWORD)xlen * (UWORD)d;
    s->size = (UWORD)len;
    s->xsize = (UWORD)xlen;
    s->data = globalloc(len, MEMF_CHIP | MEMF_CLEAR);
    if (autocook)
        s->cookie = globalloc(s->planesize, MEMF_CHIP | MEMF_CLEAR);
}
BLITZ_THUNK(_initshape, initshape_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------ GetaShape */

/* a3 = shape, a2 = used bitmap, d1 = x, d2 = y, d3 = w, d4 = h */
static void getashape(struct shape *shape, UBYTE *bm, UWORD x, UWORD y, UWORD w, UWORD h)
{
    UBYTE *tbm = tb.bm;
    ULONG p;

    freeashape_(shape);
    {
        struct xregs r = { .d1 = w, .d2 = h, .d3 = SW(bm, 4), .a3 = (ULONG)tbm };
        XJSR(TOK_extbitmap, r);                 /* create bitmap! */
    }
    *(ULONG *)&shape->height = SL(tbm, 2);      /* height, depth (rows, depth) */
    shape->ebwidth = SW(tbm, 0);
    HANDLES(shape) = 0;
    shape->data = SP(tbm, 8);                   /* the bitmap's planes */
    p = (ULONG)SW(tbm, 0) * SW(tbm, 2);
    shape->planesize = (UWORD)p;
    p = (ULONG)(UWORD)p * SW(tbm, 4);
    shape->size = (UWORD)p;
    p = (ULONG)(UWORD)(SW(tbm, 0) + 2) * SW(tbm, 2);
    shape->xplanesize = (UWORD)p;
    p = (ULONG)(UWORD)p * SW(tbm, 4);
    shape->xsize = (UWORD)p;
    shape->bltsize = (UWORD)(SW(tbm, 2) << 6) | (UWORD)((SW(tbm, 0) >> 1) + 1);
    shape->width = w;
    tb.w = w;
    tb.h = h;
    {
        struct xregs r = { .d0 = x, .d1 = y, .d2 = 0, .d3 = 0, .a0 = (ULONG)bm,
                           .a1 = 0xdff000, .a2 = (ULONG)(&tb.w + 1), .a3 = (ULONG)tbm };
        XJSR(TOK_scroll, r);                    /* scroll in shape! */
    }
    {
        volatile UBYTE *dmaconr = (volatile UBYTE *)0xdff002;
        (void)*dmaconr;
        while (*dmaconr & 0x40)
            ;
    }
    if (autocook)
        cookie_(shape);
}

/**
 * @brief GetaShape Shape#,X,Y,Width,Height - make a shape from an area of
 * the used bitmap (Blitz mode; also nullsub +14 b_getashape).
 *
 * a3 = the shape; the old one is freed.  bitmaplib +4 (extbitmap) creates
 * a bitmap of the area's size in tbmap, scrolllib Scroll copies the area
 * into it, and its planes become the shape's data; then a blitter wait
 * ("20/8/93 waitblit after getashape b4 makecookie") and the cookie with
 * AutoCookie on.
 *
 * Errors (gschk, debugger only): "Uninitialized BitMap", "Cannot GetaShape
 * from an interleaved Bitmap", "Illegal area for GetaShape".
 *
 * @param x  d1.w: x
 * @param y  d2.w: y
 * @param w  d3.w: the width
 * @param h  d4.w: the height
 * @param bm a2: the used bitmap
 */
void BLITZ_ROUTINE b_getashape_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), UBYTE *bm BL_REG(a2));
void BLITZ_ROUTINE b_getashape_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), UBYTE *bm BL_REG(a2))
{
    getashape((struct shape *)blitz_string, bm, x, y, w, h);
}
BLITZ_THUNK(b_getashape, b_getashape_c, "d0-d1/a0-a1");

/**
 * @brief GetaShape in Amiga mode: OwnBlitter, the Blitz mode routine,
 * wait, DisownBlitter.
 *
 * @param gfx a6: graphics.library
 */
void BLITZ_ROUTINE a_getashape_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), UBYTE *bm BL_REG(a2),
                                 struct GfxBase *gfx BL_REG(a6));
void BLITZ_ROUTINE a_getashape_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), UBYTE *bm BL_REG(a2),
                                 struct GfxBase *gfx BL_REG(a6))
{
    struct GfxBase *GfxBase = gfx;
    volatile UBYTE *dmaconr = (volatile UBYTE *)0xdff002;

    OwnBlitter();
    getashape((struct shape *)blitz_string, bm, x, y, w, h);
    (void)*dmaconr;
    while (*dmaconr & 0x40)
        ;
    DisownBlitter();
}
BLITZ_AB_ROUTINE(dogetashape, b_getashape,
                 "   movem.l d0-d1/a0-a1,-(sp)\n"
                 "   jsr     _a_getashape_c\n"
                 "   movem.l (sp)+,d0-d1/a0-a1\n"
                 "   rts");

/* ----------------------------------------------------- shapes files */

static struct DosLibrary *dosbase;
#define DOSBase dosbase                         /* for proto/dos.h */

/**
 * @brief SaveShapes Shape#,Shape#,Filename$ - save the shapes a3..a2.
 *
 * Every shape's 32 byte block, followed by its data (+26 bytes) for a
 * used shape.
 *
 * Errors: "Can't open Shapes File", "Can't Read/Write Shapes File" (in
 * the normal code, their texts in the error area); "First shape must be
 * lower than second" (schkz, debugger only).
 *
 * @param last a2: the last shape
 * @param name d2: the file name
 * @param dos  a6: dos.library
 */
static void BLITZ_ROUTINE dosaveshapes_c(struct shape *last BL_REG(a2), const char *name BL_REG(d2),
                                         struct DosLibrary *dos BL_REG(a6))
{
    struct shape *s = (struct shape *)blitz_string;
    BPTR fh;

    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (!fh)
        BLITZ_RUNERR(mess2);
    do {
        if (Write(fh, s, 32) != 32)
            BLITZ_RUNERR(mess3);
        if (s->width) {
            if ((UWORD)Write(fh, s->data, s->size) != s->size)
                BLITZ_RUNERR(mess3);
        }
        s++;
    } while ((ULONG)s <= (ULONG)last);
    Close(fh);
}
BLITZ_THUNK(dosaveshapes, dosaveshapes_c, "d0-d1/a0-a1");

static void loadshapes(struct shape *s, struct shape *last, const char *name,
                       struct DosLibrary *dos)
{
    BPTR fh;

    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_OLDFILE);
    if (!fh)
        BLITZ_RUNERR(mess2);
    do {
        LONG n;
        ULONG p;

        freeashape_(s);
        n = Read(fh, s, 32);
        if (!n)
            break;
        if (n != 32)
            BLITZ_RUNERR(mess3);
        s->cookie = NULL;
        if (s->width) {
            /* check validity of shape block... */
            p = (ULONG)s->ebwidth * s->height;
            if ((UWORD)p != s->planesize)
                BLITZ_RUNERR(mess3);
            p = (ULONG)(UWORD)p * s->depth;
            if ((UWORD)p != s->size)
                BLITZ_RUNERR(mess3);
            s->data = globalloc(s->size, MEMF_CHIP);
            if ((UWORD)Read(fh, s->data, s->size) != s->size)
                BLITZ_RUNERR(mess3);
            if (autocook)
                cookie_(s);
        }
        s++;
    } while ((ULONG)s <= (ULONG)last);
    Close(fh);
}

/**
 * @brief LoadShapes Shape#,Shape#,Filename$ - load the shapes a3..a2 from
 * a SaveShapes file.
 *
 * Every block is checked (planesize, size), the data loaded into CHIP
 * memory (not checked), the cookie made with AutoCookie on; the end of
 * the file ends the loading.
 *
 * BUG(orig) (#267): the block keeps its +30 ("cL" of a saved DecodeShapes
 * shape: the data is never freed).  Errors: "Can't open Shapes File",
 * "Can't Read/Write Shapes File" (in the normal code); "First shape must
 * be lower than second" (schkz, debugger only).
 *
 * @param last a2: the last shape
 * @param name d2: the file name
 * @param dos  a6: dos.library
 */
static void BLITZ_ROUTINE doloadshapes_c(struct shape *last BL_REG(a2),
                                         const char *name BL_REG(d2),
                                         struct DosLibrary *dos BL_REG(a6))
{
    loadshapes((struct shape *)blitz_string, last, name, dos);
}
BLITZ_THUNK(doloadshapes, doloadshapes_c, "d0-d1/a0-a1");

/**
 * @brief LoadShapes Shape#,Filename$ - load shapes from a3 on up to the
 * last shape object (as doloadshapes_c).
 *
 * BUG(orig) (#939): the last shape is a2 + (WORD)((max-1) << 5): from a
 * maximum of 1025 on it lies before the table.
 *
 * @param base a2: shape 0
 * @param max  d2: the maximum
 * @param name d1: the file name
 * @param dos  a6: dos.library
 */
static void BLITZ_ROUTINE doloadshapes2_c(UBYTE *base BL_REG(a2), ULONG max BL_REG(d2),
                                          const char *name BL_REG(d1),
                                          struct DosLibrary *dos BL_REG(a6))
{
    loadshapes((struct shape *)blitz_string,
               (struct shape *)(base + (WORD)(UWORD)((UWORD)(max - 1) << 5)), name, dos);
}
BLITZ_THUNK(doloadshapes2, doloadshapes2_c, "d0-d1/a0-a1");

/* -------------------------------------------------------- DecodeShapes */

static void cludge(struct shape *shape, struct shape *last, UBYTE *mem)
{
    UBYTE *src = mem;
    ULONG fastmem = (ULONG)mem & 0xffe00000UL;

    do {
        const struct shape *hd = (const struct shape *)src;    /* the block */
        ULONG p = (ULONG)hd->ebwidth * hd->height;
        int i;

        if ((UWORD)p != hd->planesize)
            return;
        p = (ULONG)(UWORD)p * hd->depth;
        if ((UWORD)p != hd->size)
            return;
        freeashape_(shape);
        for (i = 0; i < 8; i++)                 /* copy header */
            ((ULONG *)shape)[i] = ((ULONG *)src)[i];
        src += sizeof(struct shape);
        shape->cookie = NULL;
        shape->data = NULL;
        if (shape->width) {
            if (fastmem) {
                UBYTE *d = globalloc(shape->size, MEMF_CHIP);
                UWORD n = (UWORD)(shape->size >> 1) - 1;

                shape->data = d;
                do {
                    *(UWORD *)d = *(UWORD *)src;
                    d += 2;
                    src += 2;
                } while (n-- != 0);
            } else {
                shape->data = src;            /* in chip: use in place */
                src += shape->size;
                shape->cludge = CLUDGE;         /* don't free me freeshape! */
            }
            if (autocook)
                cookie_(shape);
        }
        shape++;
    } while ((ULONG)shape <= (ULONG)last);
}

/**
 * @brief DecodeShapes Shape#,Shape#,MemoryLocation - the shapes a3..a2
 * from a SaveShapes file in memory.
 *
 * Stops silently at the first block that does not match (not a bug: the
 * end of the sequence).  Memory below 2 MB ($FFE00000 clear, chip RAM) is
 * used in place and marked "cL" (never freed), above it is copied to CHIP
 * memory.  Cookie with AutoCookie on.
 *
 * BUG(orig) (#270): address 0 jumps (BEQ) into the error check area
 * (err10, "Shapes file at memory location 0? NOWAY!"): without the
 * debugger behind the library.
 *
 * @param last a2: the last shape
 * @param mem  d2: the address of the data
 */
static void BLITZ_ROUTINE cludgeshapes_c(struct shape *last BL_REG(a2), UBYTE *mem BL_REG(d2))
{
    if (!mem)
        JMP_ERR10();
    cludge((struct shape *)blitz_string, last, mem);
}
BLITZ_THUNK(_cludgeshapes, cludgeshapes_c, "d0-d1/a0-a1");

/**
 * @brief DecodeShapes Shape#,MemoryLocation - as cludgeshapes_c from a3 on
 * up to the last shape object.
 *
 * BUG(orig) (#939): the last shape is a2 + (WORD)((max-1) << 5).
 * BUG(orig) (#270): address 0, as cludgeshapes_c.
 *
 * @param base a2: shape 0
 * @param max  d2: the maximum
 * @param mem  d1: the address of the data
 */
static void BLITZ_ROUTINE cludgeshapes2_c(UBYTE *base BL_REG(a2), ULONG max BL_REG(d2),
                                          UBYTE *mem BL_REG(d1))
{
    /* BUG(orig) (#939) */
    struct shape *last = (struct shape *)(base + (WORD)(UWORD)((UWORD)(max - 1) << 5));

    if (!mem)
        JMP_ERR10();
    cludge((struct shape *)blitz_string, last, mem);
}
BLITZ_THUNK(_cludgeshapes2, cludgeshapes2_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------ LoadShape */

/* iffiolib/ilbmifflib call with d0 (and a0); result d0, Z in *z */
#define IJSRZ(tok, d0v, a0v, zp) ({ struct xregs r_ = { .d0 = (d0v), .a0 = (ULONG)(a0v), \
    .a3 = (ULONG)blitz_string }; XJSR(tok, r_); *(zp) = r_.z; r_.d0; })
#define IJSR(tok, d0v, a0v) ({ struct xregs r_ = { .d0 = (d0v), .a0 = (ULONG)(a0v), \
    .a3 = (ULONG)blitz_string }; XJSR(tok, r_); r_.d0; })

static UWORD xordelta;                          /* bmode: 2 = xor, 0 = set */
static ULONG planeoffs[8];                      /* offs: the DLTA plane offsets */

static void doshload_(struct shape *shape, const char *name)
{
    struct shape *first = shape, *frame;
    UBYTE *st;
    UBYTE *dst;                                 /* a2: the shape data write pointer */
    struct shape *next;                         /* a2: the next frame's shape */
    UWORD bpr, rows, depth;
    UWORD planesize;                            /* d7 */
    UWORD pstep;                                /* d7: row end to the next plane */
    UWORD bltsize;                              /* d0 */
    ULONG res;                                  /* d0: chained through the IFF calls */
    ULONG p;
    UBYTE z;

    freeashape_(shape);
    {
        struct xregs r = { .a0 = (ULONG)name, .a3 = (ULONG)blitz_string };
        XJSR(TOK_ilbminit, r);                  /* open the ilbm iff! */
        if (r.z)
            BLITZ_RUNERR(mess2);
        st = (UBYTE *)r.a0;
    }
    first->width = SW(st, 0);
    bltsize = SW(st, 2);
    first->height = bltsize;
    rows = bltsize;
    bltsize = (UWORD)(bltsize << 6);
    depth = SW(st, 4);
    first->depth = depth;
    bpr = (UWORD)(SW(st, 6) << 1);
    first->ebwidth = bpr;
    first->bltsize = bltsize | (UWORD)((bpr >> 1) + 1);
    HANDLES(first) = 0;
    p = (ULONG)(UWORD)(bpr + 2) * rows;
    first->xplanesize = (UWORD)p;
    p = (ULONG)(UWORD)p * depth;
    first->xsize = (UWORD)p;
    p = (ULONG)bpr * rows;
    first->planesize = (UWORD)p;
    planesize = (UWORD)p;
    p = (ULONG)(UWORD)p * depth;
    first->size = (UWORD)p;
    dst = globalloc(p, MEMF_CHIP);              /* global alloc for data! */
    first->data = dst;
    res = IJSR(TOK_ifffind, 0x424f4459, st); /* "BODY" (not checked) */
    pstep = planesize - bpr;
    rows--;
    depth--;
    do {
        UBYTE *row = dst;
        UWORD count = depth;

        do {
            UWORD n = bpr - 1;

            do {
                res = IJSR(TOK_ilbmgetbyte, res, st); /* get byte */
                *dst++ = (UBYTE)res;
            } while (n-- != 0);
            dst += (WORD)pstep;
        } while (count-- != 0);
        dst = row + (WORD)bpr;
    } while (rows-- != 0);
    frame = shape;
    if (autocook)
        cookie_(frame);

    /* first frame decoded...see if it's an anim! */
    res = IJSRZ(TOK_ifffind, 0x414e4844, st, &z); /* "ANHD" */
    if (!z)
        goto skipanim;
    res = IJSR(TOK_iffbyte, res, st);
    if ((UBYTE)res != 5)
        BLITZ_RUNERR(mess7);
    res = IJSR(TOK_iffskip, 17, st);
    res = IJSR(TOK_iffbyte, res, st);
    if ((UBYTE)res != 1)
        BLITZ_RUNERR(mess8);
    res = IJSR(TOK_iffbyte, res, st);
    res = IJSR(TOK_ifflong, res, st);
    xordelta = (UWORD)res & 2;
    do {                                        /* create next frame... */
        UWORD planes;
        ULONG dpos = 0, *off;
        ULONG *optr;

        next = frame + 1;
        copyshape2(frame, next);
        frame = next;
        frame->cookie = NULL;
        res = IJSR(TOK_ifffind, 0x444c5441, st); /* "DLTA" */
        planes = frame->depth - 1;
        optr = planeoffs;
        do {
            dpos += 4;
            res = IJSR(TOK_ifflong, res, st);
            *optr++ = res;
        } while (planes-- != 0);
        /* do planes... */
        planes = frame->depth - 1;
        off = planeoffs;
        dst = frame->data;
        do {
            ULONG o = *off++;

            if (o) {
                o -= dpos;
                if (o) {
                    res = IJSR(TOK_iffskip, o, st);
                    dpos = off[-1];
                }
                {
                    UWORD cols = frame->ebwidth - 1;
                    UBYTE *col = dst;

                    do {                        /* columns */
                        UWORD nops;

                        dpos++;
                        res = IJSR(TOK_iffbyte, res, st);
                        nops = (UBYTE)res;
                        if (nops) {
                            UBYTE *ops = dst;

                            nops--;
                            do {                /* handleops */
                                UBYTE op;

                                dpos++;
                                res = IJSR(TOK_iffbyte, res, st); /* opcode */
                                op = (UBYTE)res;
                                if (op == 0) {
                                    UWORD count;
                                    UBYTE v;

                                    dpos++;
                                    res = IJSR(TOK_iffbyte, res, st);
                                    count = (UWORD)(UBYTE)res - 1;
                                    dpos++;
                                    res = IJSR(TOK_iffbyte, res, st);
                                    v = (UBYTE)res;
                                    do {
                                        if (xordelta)
                                            *dst ^= v;
                                        else
                                            *dst = v;
                                        dst += (WORD)frame->ebwidth;
                                    } while (count-- != 0);
                                } else if ((BYTE)op < 0) {
                                    UWORD count = (UWORD)((UWORD)res & 127) - 1;

                                    do {
                                        dpos++;
                                        res = IJSR(TOK_iffbyte, res, st);
                                        if (xordelta)
                                            *dst ^= (UBYTE)res;
                                        else
                                            *dst = (UBYTE)res;
                                        dst += (WORD)frame->ebwidth;
                                    } while (count-- != 0);
                                } else {
                                    /* EXT d0:MULU 6(a3),d0:ADD.l d0,a2 */
                                    dst += (ULONG)(UWORD)(WORD)(BYTE)op * frame->ebwidth;
                                }
                            } while (nops-- != 0);
                            dst = ops;
                        }
                        dst++;                  /* skipops */
                    } while (cols-- != 0);
                    dst = col;
                }
            }
            dst += (WORD)frame->planesize;         /* pskip */
        } while (planes-- != 0);
        if (autocook)
            cookie_(frame);
        res = IJSRZ(TOK_ifffind, 0x414e4844, st, &z); /* "ANHD" */
    } while (z);
skipanim:
    IJSR(TOK_iffclose, res, st);             /* close the IFF */
}

/**
 * @brief LoadShape Shape#,Filename$ - load an ILBM (or ANIM) as a shape.
 *
 * Through ilbmifflib (ilbminit, ilbmgetbyte) and iffiolib; data in CHIP
 * memory (not checked), cookie with AutoCookie on.  After the first frame
 * every "ANHD" (ANIM op 5, interleave 1) gives a delta frame in the NEXT
 * shape object (a copy of the previous frame with the DLTA applied, XOR
 * or set per the ANHD bits).
 *
 * BUG(orig): "BODY" is not checked, the decoder reads whatever follows
 * (#264); ANIM frames go into the following shape objects without a check
 * against the maximum (#265); sizes are MULU low words (#263).
 * Errors (in the normal code, their texts in the error area): "Can't open
 * Shapes File", "ANIM format not supported" (op <> 5), "Illegal ANIM
 * interleave value" (<> 1).
 *
 * @param shape a0: the shape (a3 for the IFF calls)
 * @param name  d1: the file name
 */
static void BLITZ_ROUTINE doshload_c(struct shape *shape BL_REG(a0), const char *name BL_REG(d1))
{
    doshload_(shape, name);
}
BLITZ_THUNK(doshload, doshload_c, "d0-d1/a0-a1");

/**
 * @brief LoadShape Shape#,Filename$,Palette# - load the file's palette
 * (palettelib +3: a0 = file name, a1 = palette), then as doshload_c.
 *
 * @param shape a0: the shape
 * @param name  d1: the file name
 * @param pal   a1: the palette object
 */
static void BLITZ_ROUTINE doshload2_c(struct shape *shape BL_REG(a0), const char *name BL_REG(d1),
                                      void *pal BL_REG(a1))
{
    struct xregs r = { .d1 = (ULONG)name, .a0 = (ULONG)name, .a1 = (ULONG)pal,
                       .a3 = (ULONG)blitz_string };

    XJSR(TOK_palload, r);
    doshload_(shape, name);
}
BLITZ_THUNK(doshload2, doshload2_c, "d0-d1/a0-a1");

/* *************************** ERROR CHECKING *************************** */

static void BLITZ_ERRCHK schkz_c(struct shape *last BL_REG(a2))
{
    if ((ULONG)blitz_string > (ULONG)last)
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK funcchk_c(const struct shape *s BL_REG(a0))
{
    if (!s->width)
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK cookiechk_c(void)
{
    if (!((const struct shape *)blitz_string)->width)
        BLITZ_RUNERR(mess1);
}

/* GetaShape: bitmap present, not interleaved, area inside */
static void BLITZ_ERRCHK gschk_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), UBYTE *bm BL_REG(a2))
{
    UWORD limit;

    if (!SW(bm, 62))
        BLITZ_RUNERR(mess6);
    if ((WORD)SW(bm, 52) < 0)
        BLITZ_RUNERR(mess9);
    if ((WORD)w < 0 || (WORD)h < 0)
        BLITZ_RUNERR(mess5);
    limit = SW(bm, 56);
    if ((UWORD)x >= limit || (UWORD)((UWORD)x + (UWORD)w) > limit)
        BLITZ_RUNERR(mess5);
    limit = SW(bm, 58);
    if ((UWORD)y >= limit || (UWORD)((UWORD)y + (UWORD)h) > limit)
        BLITZ_RUNERR(mess5);
}

/* the thunks after all check functions: their assembler switches back to
   .text, which GCC does not notice (a check function after a thunk would
   end up in the normal code) */
BLITZ_ERRCHK_THUNK(schkz, schkz_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(funcchk, funcchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(cookiechk, cookiechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(gschk, gschk_c, "d0-d1/a0-a1");
