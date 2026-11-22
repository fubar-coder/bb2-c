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
 * 5) gives a delta frame in the NEXT shape object, made from the frame its
 * interleave names (n frames back, 0 = 2; before the first frame: the
 * first frame).
 *
 * BUG(orig), kept:
 *  - LoadShape: "BODY" is not checked (the decoder reads whatever
 *    follows) (#264).
 *  - LoadShape of an ANIM: the ops of a DLTA column (skip rows, unique
 *    bytes, a run) are not bounded by the frame's height: an op past the
 *    last row writes into the next plane, from the last plane on behind
 *    the shape's data; a missing or short DLTA is read as $FF bytes
 *    (iffbyte at the end of the file), 255 ops of up to 127 rows per
 *    column (#1193; test shapeslib-anim-delta-bounds).
 *  - Not a bug: only `depth` DLTA pointers are read (one per plane); the
 *    copied frame shares
 *    nothing (its cookie is cleared, made again with AutoCookie On).
 *  - Not a bug (by design): DecodeShapes stops silently at the first block
 *    that does not match (the end of the sequence); memory below 2 MB
 *    ($FFE00000 clear, chip RAM) is used in place ("cL"), above it is
 *    copied to CHIP memory.
 *  - Not a bug (word results, the compiler uses only d0.w):
 *    ShapeWidth/ShapeHeight load only the low word of d0.
 *
 * Fixed (intended deviation, docs/libs/deviations.md):
 *  - freeashape clears +30 of a "cL" shape too; the original left the
 *    mark, and a later shape in that object (InitShape, LoadShape,
 *    GetaShape ...) was never freed. (#269)
 *  - CopyShape clears +30 of the copy, which owns its data; the original
 *    copied "cL" of a DecodeShapes shape along (the copy was never
 *    freed). (#268)
 *  - LoadShapes, and DecodeShapes of memory above 2 MB (which copies the
 *    data), clear +30 of the block they take from the file; the original
 *    kept it, and a file saved from a DecodeShapes shape holds "cL": the
 *    data allocated for it was never freed. (#267)
 *  - DecodeShapes with address 0 branches (BEQ) in the original into the
 *    error check area (err10): without the debugger behind the library.
 *    Here err10 and its message are in the normal code; without the
 *    debugger the runtime error is a TRAP #0 with the right message. (#270)
 *  - err2/err3/err7/err8 (files, ANIM) are in the normal code, in the
 *    original with their messages in the error check area (without the
 *    debugger d0 points behind the library); here the messages are in the
 *    normal code too. (#953)
 *  - LoadShapes s,file$ / DecodeShapes s,mem: the original computes the
 *    last shape as base + (max - 1) << 5 with word operations (SUBQ/LSL/
 *    ADD d2,a2): from a maximum of 1025 on it lies before the table and
 *    only the first shape is loaded.  Here the offset is a long. (#999)
 *  - InitShape had no error check: a depth of 0 made a shape of depth 0
 *    (with a cookie), which the blit commands' checks pass, and their
 *    plane loops (DBF on depth-1) blitted 65536 planes.  Here its error
 *    check (initchk, error check area only) refuses it with "Illegal
 *    Shape Depth"; release code unchanged. (#243, #74)
 *  - InitShape and GetaShape made shapes with a plane of 64K or more
 *    without a check (the word sizes +22/+24 wrong, memory overwritten).
 *    Here their error checks (initchk, gschk; error check area only)
 *    refuse them with "Shape too Big" ((bytes per row + 2) x height of
 *    64K or more), and initchk a width or height of 0 with "Illegal
 *    Shape Size"; release code unchanged. (#1047)
 *  - The blit size word (+8: height << 6 | words + 1, as BLTSIZE) holds
 *    10 bits of height and 6 of width: a shape taller than 1023 rows or
 *    wider than 62 words (992 pixels; from 63 words the + 1 carries into
 *    the height) got a wrong blit size.  Here LoadShape refuses such a
 *    picture (file data: "Can't Read/Write Shapes File", nothing loaded)
 *    and the error checks of InitShape and GetaShape (and of Rotate and
 *    Scale in shapetrixlib) refuse such a shape with "Shape too Big";
 *    the blit commands are unchanged. (#1046)
 *  - LoadShape took the number of planes from the BMHD without a check:
 *    0 planes made a shape of depth 0 with a data block of 0 bytes, and
 *    the plane loop (DBF on depth - 1) wrote 65536 planes of every row
 *    behind it.  Here LoadShape refuses such a picture (file data:
 *    "Illegal Shape Depth", nothing loaded). (#1052)
 *  - LoadShape took the width and height of the BMHD without a check: a
 *    width or height of 0 made a data block of 0 bytes, and the byte
 *    loop (DBF on bytes per row - 1) or the row loop (rows - 1) ran 65536
 *    times behind it.  Here LoadShape refuses such a picture (file data:
 *    "Illegal Shape Size", nothing loaded). (#1054)
 *  - LoadShape raised "ANIM format not supported" and "Illegal ANIM
 *    interleave value" (the first ANHD) without iffclose: the file stayed
 *    open, also after the program's end (iffiolib has no finit), and
 *    locked until reboot.  Here it is closed before the first; the
 *    second is gone (#1166, below); the first frame stays loaded in the
 *    shape, as in the original. (#1072)
 *  - LoadShape of an ANIM read the XOR bit (ANHD bits & 2) only from the
 *    first ANHD and applied every later frame with it; here each frame
 *    takes the bit of its own ANHD. (#266)
 *  - LoadShape of an ANIM put each further frame into the next shape
 *    object without a check against the maximum: frames behind the last
 *    shape object were written over the memory behind the shape table.
 *    Here the load stops at the last shape object (the declarations of
 *    LoadShape pass the table and its maximum in a2/d3). (#265)
 *  - The plane steps (LoadShape, the cookie, the ANIM deltas) were word
 *    additions (sign extended: planes of 32K and more went backwards,
 *    LoadShape wrote before its data block), and the size of the data
 *    (+26, a word) was used for SaveShapes, LoadShapes, DecodeShapes and
 *    CopyShape (a shape of 64K or more was saved and copied in part).
 *    Here the steps are unsigned and the size of the data is bytes per
 *    plane x depth as a long (+26 keeps its low word, as before).  The
 *    format keeps the bytes per plane in a word: LoadShape refuses an
 *    ILBM with a plane of 64K or more ("Can't Read/Write Shapes File",
 *    nothing loaded), LoadShapes/DecodeShapes such a block. (#263)
 *  - LoadShape (#263) tested the plane alone (bytes per row x height),
 *    the error checks of InitShape and GetaShape (#1047) the plane with
 *    the extra word of every row ((bytes per row + 2) x height, the word
 *    +24 that BBlit saves per plane and adds to its buffer pointer): a
 *    picture whose plane was just under 64K but whose +24 was not loaded
 *    with a wrong +24.  Here LoadShape tests +24 as the checks do. (#1055)
 *
 * Extension (user decision 2026-11-20, docs/libs/deviations.md):
 *  - LoadShape of an ANIM took only the interleave 1 (each delta modifies
 *    the frame before it) and stopped on any other value with "Illegal
 *    ANIM interleave value" (the file closed since #1072).  Here every
 *    value is taken as the ANIM specification defines it: the delta
 *    modifies the frame n frames back, 0 = 2 (double buffering, the
 *    default most ANIM5 files carry); a frame with fewer frames before it
 *    is made from the first frame.  Each frame takes the interleave of
 *    its own ANHD; the message is gone. (#1166)
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include <stddef.h>
#define __NOLIBBASE__
#include <proto/dos.h>
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (aliases / thunks / assembler, defined below) */
void doshload(void), doshload2(void), docookie(void), doshapew(void);
void doshapeh(void), dohandle(void), doautocookie(void), docopyshape(void);
void dogetashape(void), b_getashape(void), dosaveshapes(void);
void doloadshapes(void), doloadshapes2(void), domidhandle(void);
void _cludgeshapes(void), _cludgeshapes2(void), _initshape(void);
void shapeslib_codeinit(void), freeashape(void), cookiechk(void), initchk(void);
void funcchk(void), gschk(void), schkz(void);

#define AMIGAONLY(entry) ((char *)(entry) + 1)

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#shapeslib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_shapeslib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
/* fixed #265: + a2 = the shape table, d3 = its maximum (the last object
   for the ANIM frames; the original had only a0) */
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD0, LIB_shapeslib, BL_BA2, LIB_shapeslib, BL_MD3);
BLITZ_SUBS(AMIGAONLY(doshload), 0, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD0, LIB_palettelib, BL_IA1|BL_PD2, LIB_shapeslib, BL_BA2,
           LIB_shapeslib, BL_MD3);
BLITZ_SUBS(AMIGAONLY(doshload2), 0, 0);
BLITZ_NAME("LoadShape", "Shape#,Filename$[,Palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0);
BLITZ_SUBS(docookie, cookiechk, 0);
BLITZ_NAME("MakeCookie", "Shape#");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD0);
BLITZ_SUBS(doshapew, funcchk, 0);
BLITZ_NAME("ShapeWidth", "(Shape#)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD0);
BLITZ_SUBS(doshapeh, funcchk, 0);
BLITZ_NAME("ShapeHeight", "(Shape#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0);
BLITZ_SUBS(dohandle, cookiechk, 0);
BLITZ_NAME("Handle", "Shape#,X,Y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doautocookie, 0, 0);
BLITZ_NAME("AutoCookie", "On|Off");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0, LIB_shapeslib, BL_IA2|BL_PD1);
BLITZ_SUBS(docopyshape, cookiechk, 0);
BLITZ_NAME("CopyShape", "Shape#,Shape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0, LIB_bitmaplib, BL_UA2, LIB_graphicslib, BL_IFAMIGA|BL_LA6);
BLITZ_SUBS(dogetashape, gschk, 0);
BLITZ_NAME("GetaShape", "Shape#,X,Y,Width,Height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0, LIB_shapeslib, BL_IA2|BL_PD1, LIB_doslib, BL_LA6);
BLITZ_SUBS(AMIGAONLY(dosaveshapes), schkz, 0);
BLITZ_NAME("SaveShapes", "Shape#,Shape#,Filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0, LIB_shapeslib, BL_BA2, LIB_shapeslib, BL_MD2,
           LIB_doslib, BL_LA6);
BLITZ_SUBS(AMIGAONLY(doloadshapes2), 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0, LIB_shapeslib, BL_IA2|BL_PD1, LIB_doslib, BL_LA6);
BLITZ_SUBS(AMIGAONLY(doloadshapes), schkz, 0);
BLITZ_NAME("LoadShapes", "Shape#[,Shape#],Filename$");

BLITZ_NULLSUB(docookie, 0, 0);

BLITZ_DUMTOKE(_toke, "Shape", "- image");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0);
BLITZ_SUBS(domidhandle, cookiechk, 0);
BLITZ_NAME("MidHandle", "Shape#");

BLITZ_NULLSUB(b_getashape, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0, LIB_shapeslib, BL_BA2, LIB_shapeslib, BL_MD2);
BLITZ_SUBS(_cludgeshapes2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0, LIB_shapeslib, BL_IA2|BL_PD1);
BLITZ_SUBS(_cludgeshapes, 0, 0);
BLITZ_NAME("DecodeShapes", "Shape#[,Shape#],MemoryLocation");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA3|BL_PD0);
BLITZ_SUBS(_initshape, initchk, 0);
BLITZ_NAME("InitShape", "Shape#,Width,Height,Depth");

BLITZ_NULLSUB_AT(init, shapeslib_codeinit, 0, 0);
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

/* the objects: struct bb_shape, struct bb_bitmap (blitzobj.h); ilbmifflib's
   state and nall's fields by offset */
#define SW(s, o) (*(UWORD *)((UBYTE *)(s) + (o)))
#define SL(s, o) (*(ULONG *)((UBYTE *)(s) + (o)))
#define SP(s, o) (*(UBYTE **)((UBYTE *)(s) + (o)))
/* the two handles as one long (CLR.l 10(a3)) */
#define HANDLES(s) (*(ULONG *)&(s)->handlex)

static UWORD autocook = 0xffff;                 /* AutoCookie (default on) */

/* tbmap: a bitmap object (64 bytes) for GetaShape, then the 2 words
   Scroll reads below a2 (tdata) */
static struct {
    struct bb_bitmap bm;
    UWORD h, w;                                 /* -4(a2), -2(a2) */
} BL_PACKED tb __attribute__((aligned(4)));

BLITZ_ERRCHK_MESSAGE(mess1, "Uninitialized Shape");
BLITZ_ERRCHK_MESSAGE(mess4, "First shape must be lower than second");
BLITZ_ERRCHK_MESSAGE(mess5, "Illegal area for GetaShape");
BLITZ_ERRCHK_MESSAGE(mess6, "Uninitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess9, "Cannot GetaShape from an interleaved Bitmap");
BLITZ_ERRCHK_MESSAGE(mess11, "Illegal Shape Depth");
BLITZ_ERRCHK_MESSAGE(mess12, "Shape too Big");          /* fixed, #1047 */
BLITZ_ERRCHK_MESSAGE(mess13, "Illegal Shape Size");     /* fixed, #1047 */

/* err10 of DecodeShapes (TST.l d2:BEQ err10) and its message in the
   normal code (the original has them in the error check area; fixed,
   #270) */
static const char mess10[] = "Shapes file at memory location 0? NOWAY!";

/* the messages of err2/err3/err7/err8 (files, ANIM) in the normal code
   with their error routines (the original has them in the error check
   area; fixed, #953) */
static const char mess2[] = "Can't open Shapes File";
static const char mess3[] = "Can't Read/Write Shapes File";
/* LoadShape of a picture with 0 planes (fixed, #1052; the text of the
   error check's mess11, here in the normal code) */
static const char mess14[] = "Illegal Shape Depth";
/* LoadShape of a picture with a width or height of 0 (fixed, #1054; the
   text of the error check's mess13, here in the normal code) */
static const char mess15[] = "Illegal Shape Size";
static const char mess7[] = "ANIM format not supported";

/** @brief Library init: AutoCookie on. */
static void BLITZ_ROUTINE codeinit_c(void)
{
    autocook = 0xffff;
}
BLITZ_THUNK(shapeslib_codeinit, codeinit_c, "d0-d1/a0-a1");

/* the size of a shape's data: bytes per plane x depth as a long (fixed
   #263: +26 holds only its low word) */
static ULONG shsize(const struct bb_shape *s)
{
    return bl_mulu_w(s->planesize, s->depth);
}

/* free a shape's data and cookie (a3) */
static void freeashape_(struct bb_shape *s)
{
    ULONG c;

    if (!s->width)
        return;
    s->width = 0;
    if (s->cludge != BB_CLUDGE)
        globfree(s->data, shsize(s));
    s->cludge = 0;                      /* Fixed (#269): also "cL" */
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
 * Fixed (#269): +30 is cleared for a "cL" shape too (the original left
 * the mark, so a later shape in this object was never freed either).
 */
static void BLITZ_ROUTINE freeashape_c(void)
{
    freeashape_((struct bb_shape *)blitz_string);
}
BLITZ_THUNK(freeashape, freeashape_c, "d0-d1/a0-a1");

/* make the cookie: OR of all planes */
static void cookie_(struct bb_shape *s)
{
    UBYTE *plane, *ck;
    ULONG psize;                                /* fixed #263: unsigned */
    UWORD rows;

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
            src += psize;
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
    cookie_((struct bb_shape *)blitz_string);
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
BLITZ_ALIAS(doautocookie, doautocookie_c);

/* ShapeWidth, ShapeHeight and Handle are the original's code (MOVE
   (a0),d0:RTS ...), assembler entries: GCC made the same instructions of
   the C functions, the assembler keeps them so (they change only d0,
   Handle nothing) whatever GCC makes of the rest of the library. */

/**
 * @brief ShapeWidth(Shape#) - the shape's pixel width.
 *
 * MOVE (a0),d0 - only the low word (not a bug: a word function, the
 * compiler uses only d0.w).
 *
 * Errors: "Uninitialized Shape" (funcchk, debugger only).
 *
 * In:  a0 the shape
 * Out: d0.w the width
 */
BLITZ_ASM_ROUTINE(doshapew,
"   move.w  (a0),d0\n"                  /* width */
"   rts");

/**
 * @brief ShapeHeight(Shape#) - the shape's pixel height (only the low word
 * of d0, as ShapeWidth).
 *
 * Errors: "Uninitialized Shape" (funcchk, debugger only).
 *
 * In:  a0 the shape
 * Out: d0.w the height
 */
BLITZ_ASM_ROUTINE(doshapeh,
"   move.w  2(a0),d0\n"                 /* height */
"   rts");

/**
 * @brief MidHandle Shape# - put the handle in the middle (width/2,
 * height/2); a3 = the shape.
 *
 * Errors: "Uninitialized Shape" (cookiechk, debugger only).
 */
static void BLITZ_ROUTINE domidhandle_c(void)
{
    struct bb_shape *s = (struct bb_shape *)blitz_string;

    s->handlex = s->width >> 1;
    s->handley = s->height >> 1;
}
BLITZ_ALIAS(domidhandle, domidhandle_c);

/**
 * @brief Handle Shape#,X,Y - set the shape's handle; a3 = the shape.
 *
 * Errors: "Uninitialized Shape" (cookiechk, debugger only).
 *
 * In:  a3 the shape, d1.w the handle x, d2.w the handle y
 */
BLITZ_ASM_ROUTINE(dohandle,
"   move.w  d1,10(a3)\n"                /* handlex */
"   move.w  d2,12(a3)\n"                /* handley */
"   rts");

/* CopyShape: a3 = source, a2 = destination (docopyshape2 + cookie); size:
   the bytes of the block (fixed #263: the original took the word +26 or
   +22, a shape of 64K or more was copied in part) */
static void nall(struct bb_shape *from, struct bb_shape *to, UWORD ptroff, ULONG size)
{
    UBYTE *src, *dst;
    ULONG n;

    if (!SL(from, ptroff))
        return;
    dst = globalloc(size, MEMF_CHIP);
    SP(to, ptroff) = dst;
    src = SP(from, ptroff);
    n = (size >> 1) - 1;
    do {
        *(UWORD *)dst = *(UWORD *)src;
        dst += 2;
        src += 2;
    } while (n-- != 0);
}

static void copyshape2(struct bb_shape *from, struct bb_shape *to)
{
    int i;

    freeashape_(to);
    for (i = 0; i < 8; i++)
        ((ULONG *)to)[i] = ((ULONG *)from)[i];
    to->cludge = 0;                     /* Fixed (#268): the copy owns its data */
    nall(from, to, offsetof(struct bb_shape, data), shsize(from));
}

/**
 * @brief CopyShape Shape#,Shape# - copy the first shape (a3) into the
 * second.
 *
 * The destination is freed, the 32 byte block copied, then the data and
 * the cookie are copied into new CHIP memory (not checked).
 *
 * Fixed (#268): +30 of the copy is cleared (the original copied "cL" of a
 * DecodeShapes shape along, and the copy was never freed).  Errors:
 * "Uninitialized Shape" (cookiechk, debugger only).
 *
 * @param dst a2: the destination shape
 */
static void BLITZ_ROUTINE docopyshape_c(struct bb_shape *dst BL_REG(a2))
{
    copyshape2((struct bb_shape *)blitz_string, dst);
    nall((struct bb_shape *)blitz_string, dst, offsetof(struct bb_shape, cookie),
         ((struct bb_shape *)blitz_string)->planesize);
}
BLITZ_ALIAS(docopyshape, docopyshape_c);

/* ------------------------------------------------------------ InitShape */

/**
 * @brief InitShape Shape#,Width,Height,Depth - create an empty shape.
 *
 * a3 = the shape; the old one is freed.  Data cleared (CHIP, not
 * checked), handle 0, a cleared cookie with AutoCookie on.
 *
 * The sizes +22/+26 are the low words of MULU; the data is allocated with
 * the long product.  Errors (debugger, initchk): "Illegal Shape Depth"
 * for a depth of 0 (fixed, #243 #74: the original had no check; a shape
 * of depth 0 made the plane loops of the blit commands run 65536 times);
 * "Illegal Shape Size" for a width or height of 0 and "Shape too Big"
 * for a plane of 64K or more with the extra word per row (fixed, #1047:
 * not checked in the original, the word sizes +22/+24 were wrong) or a
 * shape taller than 1023 rows or wider than 62 words (992 pixels; fixed,
 * #1046: the blit size word +8 overflowed).
 *
 * @param w d1.w: the width in pixels
 * @param h d2.w: the height
 * @param d d3.w: the depth
 */
static void BLITZ_ROUTINE initshape_c(ULONG w BL_REG(d1), ULONG h BL_REG(d2), ULONG d BL_REG(d3))
{
    struct bb_shape *s = (struct bb_shape *)blitz_string;
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
BLITZ_ALIAS(_initshape, initshape_c);

/* ------------------------------------------------------------ GetaShape */

/* a3 = shape, a2 = used bitmap, d1 = x, d2 = y, d3 = w, d4 = h */
static void getashape(struct bb_shape *shape, struct bb_bitmap *bm,
                      UWORD x, UWORD y, UWORD w, UWORD h)
{
    struct bb_bitmap *tbm = &tb.bm;
    ULONG p;

    freeashape_(shape);
    {
        struct xregs r = { .d1 = w, .d2 = h, .d3 = bm->flagsdepth, .a3 = (ULONG)tbm };
        XJSR(TOK_extbitmap, r);                 /* create bitmap! */
    }
    *(ULONG *)&shape->height = *(ULONG *)&tbm->rows; /* height, depth (rows, depth) */
    shape->ebwidth = tbm->bpr;
    HANDLES(shape) = 0;
    shape->data = tbm->planes[0];               /* the bitmap's planes */
    p = (ULONG)tbm->bpr * tbm->rows;
    shape->planesize = (UWORD)p;
    p = (ULONG)(UWORD)p * tbm->flagsdepth;
    shape->size = (UWORD)p;
    p = (ULONG)(UWORD)(tbm->bpr + 2) * tbm->rows;
    shape->xplanesize = (UWORD)p;
    p = (ULONG)(UWORD)p * tbm->flagsdepth;
    shape->xsize = (UWORD)p;
    shape->bltsize = (UWORD)(tbm->rows << 6) | (UWORD)((tbm->bpr >> 1) + 1);
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
 * from an interleaved Bitmap", "Illegal area for GetaShape", "Shape too
 * Big" (a plane of 64K or more with the extra word per row, more than
 * 1023 rows or 62 words; fixed, #1047, #1046: the original has no check).
 *
 * @param x  d1.w: x
 * @param y  d2.w: y
 * @param w  d3.w: the width
 * @param h  d4.w: the height
 * @param bm a2: the used bitmap
 */
void BLITZ_ROUTINE b_getashape_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), struct bb_bitmap *bm BL_REG(a2));
void BLITZ_ROUTINE b_getashape_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), struct bb_bitmap *bm BL_REG(a2))
{
    getashape((struct bb_shape *)blitz_string, bm, x, y, w, h);
}
BLITZ_THUNK(b_getashape, b_getashape_c, "d0-d1/a0-a1");

/**
 * @brief GetaShape in Amiga mode: OwnBlitter, the Blitz mode routine,
 * wait, DisownBlitter.
 *
 * @param gfx a6: graphics.library
 */
void BLITZ_ROUTINE a_getashape_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), struct bb_bitmap *bm BL_REG(a2),
                                 struct GfxBase *gfx BL_REG(a6));
void BLITZ_ROUTINE a_getashape_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), struct bb_bitmap *bm BL_REG(a2),
                                 struct GfxBase *gfx BL_REG(a6))
{
    struct GfxBase *GfxBase = gfx;
    volatile UBYTE *dmaconr = (volatile UBYTE *)0xdff002;

    OwnBlitter();
    getashape((struct bb_shape *)blitz_string, bm, x, y, w, h);
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
 * the normal code with their texts, fixed: #953); "First shape must be
 * lower than second" (schkz, debugger only).
 *
 * @param last a2: the last shape
 * @param name d2: the file name
 * @param dos  a6: dos.library
 */
static void BLITZ_ROUTINE dosaveshapes_c(struct bb_shape *last BL_REG(a2), const char *name BL_REG(d2),
                                         struct DosLibrary *dos BL_REG(a6))
{
    struct bb_shape *s = (struct bb_shape *)blitz_string;
    BPTR fh;

    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (!fh)
        BLITZ_RUNERR(mess2);
    do {
        if (Write(fh, s, 32) != 32)
            BLITZ_RUNERR(mess3);
        if (s->width) {
            ULONG n = shsize(s);                /* fixed #263: not +26 */

            if ((ULONG)Write(fh, s->data, n) != n)
                BLITZ_RUNERR(mess3);
        }
        s++;
    } while ((ULONG)s <= (ULONG)last);
    Close(fh);
}
BLITZ_ALIAS(dosaveshapes, dosaveshapes_c);

static void loadshapes(struct bb_shape *s, struct bb_shape *last, const char *name,
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
        s->cludge = 0;                  /* Fixed (#267): not the file's +30 */
        if (s->width) {
            /* check validity of shape block... (fixed #263: a plane of
               64K or more is refused, the data is planesize x depth) */
            p = (ULONG)s->ebwidth * s->height;
            if (p > 0xffff || (UWORD)p != s->planesize)
                BLITZ_RUNERR(mess3);
            p = (ULONG)(UWORD)p * s->depth;
            if ((UWORD)p != s->size)
                BLITZ_RUNERR(mess3);
            s->data = globalloc(p, MEMF_CHIP);
            if ((ULONG)Read(fh, s->data, p) != p)
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
 * Fixed (#267): +30 of the block is cleared (the original kept "cL" of a
 * saved DecodeShapes shape, and the data was never freed).  Errors: "Can't open Shapes File",
 * "Can't Read/Write Shapes File" (in the normal code); "First shape must
 * be lower than second" (schkz, debugger only).
 *
 * @param last a2: the last shape
 * @param name d2: the file name
 * @param dos  a6: dos.library
 */
static void BLITZ_ROUTINE doloadshapes_c(struct bb_shape *last BL_REG(a2),
                                         const char *name BL_REG(d2),
                                         struct DosLibrary *dos BL_REG(a6))
{
    loadshapes((struct bb_shape *)blitz_string, last, name, dos);
}
BLITZ_ALIAS(doloadshapes, doloadshapes_c);

/**
 * @brief LoadShapes Shape#,Filename$ - load shapes from a3 on up to the
 * last shape object (as doloadshapes_c).
 *
 * Fixed (#999): the last shape is a2 + (max - 1) << 5 as a long (the
 * maximum is the low word of d2); the original adds the offset as a word
 * (from a maximum of 1025 on the last shape lies before the table).
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
    loadshapes((struct bb_shape *)blitz_string,
               (struct bb_shape *)(base + ((ULONG)(UWORD)(max - 1) << 5)), name, dos);
}
BLITZ_ALIAS(doloadshapes2, doloadshapes2_c);

/* -------------------------------------------------------- DecodeShapes */

static void cludge(struct bb_shape *shape, struct bb_shape *last, UBYTE *mem)
{
    UBYTE *src = mem;
    ULONG fastmem = (ULONG)mem & 0xffe00000UL;

    do {
        const struct bb_shape *hd = (const struct bb_shape *)src;    /* the block */
        ULONG p = bl_mulu_w(hd->ebwidth, hd->height);
        int i;

        if (p > 0xffff || (UWORD)p != hd->planesize)   /* fixed #263 */
            return;
        p = bl_mulu_w((UWORD)p, hd->depth);
        if ((UWORD)p != hd->size)
            return;
        freeashape_(shape);
        for (i = 0; i < 8; i++)                 /* copy header */
            ((ULONG *)shape)[i] = ((ULONG *)src)[i];
        src += sizeof(struct bb_shape);
        shape->cookie = NULL;
        shape->data = NULL;
        shape->cludge = 0;              /* Fixed (#267): not the block's +30 */
        if (shape->width) {
            if (fastmem) {
                UBYTE *d = globalloc(p, MEMF_CHIP);     /* fixed #263: */
                ULONG n = (p >> 1) - 1;                 /* not +26 */

                shape->data = d;
                do {
                    *(UWORD *)d = *(UWORD *)src;
                    d += 2;
                    src += 2;
                } while (n-- != 0);
            } else {
                shape->data = src;            /* in chip: use in place */
                src += p;                       /* fixed #263: not +26 */
                shape->cludge = BB_CLUDGE;         /* don't free me freeshape! */
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
 * memory.  Cookie with AutoCookie on.  Fixed (#267): +30 of a block is
 * not taken over (a copied shape owns its data).
 *
 * Address 0: runtime error "Shapes file at memory location 0? NOWAY!"
 * (err10, in the normal code; the original branches into the error check
 * area, fixed: #270).
 *
 * @param last a2: the last shape
 * @param mem  d2: the address of the data
 */
static void BLITZ_ROUTINE cludgeshapes_c(struct bb_shape *last BL_REG(a2), UBYTE *mem BL_REG(d2))
{
    if (!mem) {
        BLITZ_RUNERR(mess10);                   /* err10 */
        return;
    }
    cludge((struct bb_shape *)blitz_string, last, mem);
}
BLITZ_ALIAS(_cludgeshapes, cludgeshapes_c);

/**
 * @brief DecodeShapes Shape#,MemoryLocation - as cludgeshapes_c from a3 on
 * up to the last shape object.
 *
 * Fixed (#999): the last shape is a2 + (max - 1) << 5 as a long, as in
 * doloadshapes2_c.  Address 0: runtime error, as cludgeshapes_c (#270).
 *
 * @param base a2: shape 0
 * @param max  d2: the maximum
 * @param mem  d1: the address of the data
 */
static void BLITZ_ROUTINE cludgeshapes2_c(UBYTE *base BL_REG(a2), ULONG max BL_REG(d2),
                                          UBYTE *mem BL_REG(d1))
{
    /* Fixed (#999): a long offset */
    struct bb_shape *last = (struct bb_shape *)(base + ((ULONG)(UWORD)(max - 1) << 5));

    if (!mem) {
        BLITZ_RUNERR(mess10);                   /* err10 */
        return;
    }
    cludge((struct bb_shape *)blitz_string, last, mem);
}
BLITZ_ALIAS(_cludgeshapes2, cludgeshapes2_c);

/* ------------------------------------------------------------ LoadShape */

/* iffiolib/ilbmifflib call with d0 (and a0); result d0, Z in *z */
#define IJSRZ(tok, d0v, a0v, zp) ({ struct xregs r_ = { .d0 = (d0v), .a0 = (ULONG)(a0v), \
    .a3 = (ULONG)blitz_string }; XJSR(tok, r_); *(zp) = r_.z; r_.d0; })
#define IJSR(tok, d0v, a0v) ({ struct xregs r_ = { .d0 = (d0v), .a0 = (ULONG)(a0v), \
    .a3 = (ULONG)blitz_string }; XJSR(tok, r_); r_.d0; })

static UWORD xordelta;                          /* bmode: 2 = xor, 0 = set */
static ULONG planeoffs[8];                      /* offs: the DLTA plane offsets */

static void doshload_(struct bb_shape *shape, const char *name, const struct bb_shape *last)
{
    struct bb_shape *first = shape, *frame;
    UBYTE *st;
    UBYTE *dst;                                 /* a2: the shape data write pointer */
    struct bb_shape *next;                         /* a2: the next frame's shape */
    UWORD back;                                 /* the ANHD interleave: frames back */
    UWORD bpr, rows, depth;
    UWORD planesize;                            /* d7 */
    ULONG pstep;                                /* d7: row end to the next plane
                                                   (fixed #263: unsigned) */
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
    if (!depth) {
        /* fixed #1052: a picture with 0 planes is refused (nothing
           loaded): the shape would get a data block of 0 bytes, and the
           plane loop (DBF on depth - 1) would write 65536 planes of every
           row behind it */
        first->width = 0;
        IJSR(TOK_iffclose, 0, st);
        BLITZ_RUNERR(mess14);
    }
    bpr = (UWORD)(SW(st, 6) << 1);
    if (!rows || !bpr) {
        /* fixed #1054: a picture with a width or height of 0 is refused
           (nothing loaded): the data block would have 0 bytes, and the
           row loop (rows - 1) or the byte loop (DBF on bytes per row - 1)
           would run 65536 times and write behind it */
        first->width = 0;
        IJSR(TOK_iffclose, 0, st);
        BLITZ_RUNERR(mess15);
    }
    first->ebwidth = bpr;
    first->bltsize = bltsize | (UWORD)((bpr >> 1) + 1);
    HANDLES(first) = 0;
    p = (ULONG)(UWORD)(bpr + 2) * rows;
    if (p > 0xffff || rows > 1023 || bpr > 124) {
        /* fixed #263: the shape keeps its plane sizes in words; a plane
           of 64K or more is refused (nothing loaded).  Fixed #1055: the
           plane with the extra word of every row (+24, what BBlit saves
           per plane), as the error checks of InitShape and GetaShape
           test it (#263 tested the plane alone).  Fixed #1046: so is a
           picture taller than 1023 rows or wider than 62 words (992
           pixels), which the blit size word cannot hold */
        first->width = 0;
        IJSR(TOK_iffclose, 0, st);
        BLITZ_RUNERR(mess3);
    }
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
            dst += pstep;
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
    /* fixed #1072: the file is closed before mess7 (the original left it
       open); the first frame stays loaded */
    res = IJSR(TOK_iffbyte, res, st);
    if ((UBYTE)res != 5) {
        IJSR(TOK_iffclose, 0, st);
        BLITZ_RUNERR(mess7);
    }
    res = IJSR(TOK_iffskip, 17, st);
    res = IJSR(TOK_iffbyte, res, st);
    /* #1166: the interleave, how many frames back the delta modifies (0 =
       2, double buffering; the ANIM specification); the original took
       only 1 and stopped on any other value with "Illegal ANIM interleave
       value" */
    back = (UBYTE)res;
    res = IJSR(TOK_iffbyte, res, st);
    res = IJSR(TOK_ifflong, res, st);
    xordelta = (UWORD)res & 2;
    do {                                        /* create next frame... */
        UWORD planes;
        ULONG dpos = 0, *off;
        ULONG *optr;

        if (frame >= last)                      /* fixed #265: no object left */
            break;
        next = frame + 1;
        /* #1166: the frame `back` frames before this one (0 = 2); before
           the first frame there is only the first frame (all buffers of a
           player start with it) */
        {
            UWORD n = back ? back : 2;

            copyshape2((ULONG)(next - first) > n ? next - n : first, next);
        }
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
        /* do planes...  BUG(orig) (#1193): the ops are not bounded
           by the frame's height (they run into the next plane or behind
           the data) */
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
            dst += frame->planesize;    /* pskip (fixed #263: unsigned) */
        } while (planes-- != 0);
        if (autocook)
            cookie_(frame);
        res = IJSRZ(TOK_ifffind, 0x414e4844, st, &z); /* "ANHD" */
        if (!z)
            break;
        /* fixed #266: the XOR bit of this frame's ANHD (the original read
           only the first ANHD's): operation and 17 bytes skipped, the
           interleave (#1166: also of this frame), pad skipped, then the
           bits */
        res = IJSR(TOK_iffskip, 18, st);
        res = IJSR(TOK_iffbyte, res, st);
        back = (UBYTE)res;
        res = IJSR(TOK_iffbyte, res, st);
        res = IJSR(TOK_ifflong, res, st);
        xordelta = (UWORD)res & 2;
    } while (1);
skipanim:
    IJSR(TOK_iffclose, res, st);             /* close the IFF */
}

/**
 * @brief LoadShape Shape#,Filename$ - load an ILBM (or ANIM) as a shape.
 *
 * Through ilbmifflib (ilbminit, ilbmgetbyte) and iffiolib; data in CHIP
 * memory (not checked), cookie with AutoCookie on.  After the first frame
 * every "ANHD" (ANIM op 5) gives a delta frame in the NEXT shape object (a
 * copy of the frame its interleave names with the DLTA applied, XOR or
 * set per the bits of its ANHD; the operation is checked in the first
 * ANHD).
 * Extension (#1166): the interleave of each ANHD is taken as the ANIM
 * specification defines it, n frames back (0 = 2, double buffering); a
 * frame with fewer frames before it is made from the first frame.  The
 * original took only 1 ("Illegal ANIM interleave value").
 * Fixed (#266): each frame takes the XOR bit of its own ANHD; the
 * original read only the first ANHD's.
 * Fixed (#265): the frames stop at the last shape object (the rest of the
 * file is not read); the original wrote them behind the shape table.
 *
 * Fixed (#263): the plane steps are unsigned (the original's word steps
 * went backwards for planes of 32K and more); an ILBM with a plane of 64K
 * or more is refused with "Can't Read/Write Shapes File" (the shape
 * keeps the bytes per plane in a word).  Fixed (#1055): the plane with
 * the extra word of every row ((bytes per row + 2) x height, +24) is
 * tested, as by the error checks of InitShape and GetaShape.
 *
 * BUG(orig): "BODY" is not checked, the decoder reads whatever follows
 * (#264).
 * Errors (in the normal code, their texts in the error area): "Can't open
 * Shapes File", "ANIM format not supported" (op <> 5 in the first ANHD;
 * it closes the file first and keeps the first frame loaded, fixed #1072:
 * the original left the file open),
 * "Can't Read/Write Shapes File" (a plane with
 * the extra word of every row of 64K or more, fixed #263, #1055; more
 * than 1023 rows or 62 words, fixed #1046), "Illegal Shape Depth" (0 planes, fixed #1052: the original
 * wrote 65536 planes of every row behind a block of 0 bytes), "Illegal
 * Shape Size" (a width or height of 0, fixed #1054: the original ran its
 * row or byte loop 65536 times behind a block of 0 bytes); "Can't Read
 * File" of ilbmifflib's ilbmgetbyte (a file that ends inside the BODY;
 * it closes the file first, fixed #1065: the original left it open).
 *
 * @param shape a0: the shape (a3 for the IFF calls)
 * @param name  d1: the file name
 * @param base  a2: the shape table (object 0)
 * @param max   d3: the maximum number of shapes
 */
static void BLITZ_ROUTINE doshload_c(struct bb_shape *shape BL_REG(a0), const char *name BL_REG(d1),
                                     UBYTE *base BL_REG(a2), ULONG max BL_REG(d3))
{
    doshload_(shape, name, (struct bb_shape *)(base + ((ULONG)(UWORD)(max - 1) << 5)));
}
BLITZ_ALIAS(doshload, doshload_c);

/**
 * @brief LoadShape Shape#,Filename$,Palette# - load the file's palette
 * (palettelib +3: a0 = file name, a1 = palette), then as doshload_c.
 *
 * @param shape a0: the shape
 * @param name  d1: the file name
 * @param pal   a1: the palette object
 * @param base  a2: the shape table (object 0)
 * @param max   d3: the maximum number of shapes
 */
static void BLITZ_ROUTINE doshload2_c(struct bb_shape *shape BL_REG(a0), const char *name BL_REG(d1),
                                      void *pal BL_REG(a1), UBYTE *base BL_REG(a2),
                                      ULONG max BL_REG(d3))
{
    struct xregs r = { .d1 = (ULONG)name, .a0 = (ULONG)name, .a1 = (ULONG)pal,
                       .a3 = (ULONG)blitz_string };

    XJSR(TOK_palload, r);
    doshload_(shape, name, (struct bb_shape *)(base + ((ULONG)(UWORD)(max - 1) << 5)));
}
BLITZ_ALIAS(doshload2, doshload2_c);

/* *************************** ERROR CHECKING *************************** */

static void BLITZ_ERRCHK schkz_c(struct bb_shape *last BL_REG(a2))
{
    if ((ULONG)blitz_string > (ULONG)last)
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK funcchk_c(const struct bb_shape *s BL_REG(a0))
{
    if (!s->width)
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK cookiechk_c(void)
{
    if (!((const struct bb_shape *)blitz_string)->width)
        BLITZ_RUNERR(mess1);
}

/* a shape of w x h pixels: is a plane with the extra word of every row
   ((bytes per row + 2) x height, the word +24) 64K or more, or the shape
   taller than 1023 rows or wider than 62 words?  The shape keeps its
   plane sizes and its blit size in words (fixed, #1047, #1046: the
   original has no check) */
static int BLITZ_ERRCHK toobig(UWORD w, UWORD h)
{
    UWORD bpr = (UWORD)((UWORD)(w + 15) >> 4) << 1;

    /* fixed, #1046: the blit size word (height << 6 | words + 1) holds
       at most 1023 rows and 62 words (992 pixels) */
    if (h > 1023 || bpr > 124)
        return 1;
    return bl_mulu_w(bpr + 2, h) > 0xffff;
}

/* initchk: fixed (#243, #74, #1047), not in the original (InitShape had
   no check): a depth of 0 made a shape the blit commands' checks pass; a
   width or height of 0 makes no shape; a plane of 64K or more does not
   fit the shape's word sizes */
static void BLITZ_ERRCHK initchk_c(ULONG w BL_REG(d1), ULONG h BL_REG(d2), ULONG d BL_REG(d3))
{
    if (!(UWORD)d)
        BLITZ_RUNERR(mess11);
    if (!(UWORD)w || !(UWORD)h)
        BLITZ_RUNERR(mess13);
    if (toobig((UWORD)w, (UWORD)h))
        BLITZ_RUNERR(mess12);
}

/* GetaShape: bitmap present, not interleaved, area inside */
static void BLITZ_ERRCHK gschk_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), struct bb_bitmap *bm BL_REG(a2))
{
    UWORD limit;

    if (!bm->isreal)
        BLITZ_RUNERR(mess6);
    if ((WORD)bm->ilword < 0)
        BLITZ_RUNERR(mess9);
    if ((WORD)w < 0 || (WORD)h < 0)
        BLITZ_RUNERR(mess5);
    limit = bm->width;
    if ((UWORD)x >= limit || (UWORD)((UWORD)x + (UWORD)w) > limit)
        BLITZ_RUNERR(mess5);
    limit = bm->height;
    if ((UWORD)y >= limit || (UWORD)((UWORD)y + (UWORD)h) > limit)
        BLITZ_RUNERR(mess5);
    if (toobig((UWORD)w, (UWORD)h))             /* fixed, #1047 */
        BLITZ_RUNERR(mess12);
}

/* the thunks of the check functions */
BLITZ_ERRCHK_THUNK(schkz, schkz_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(funcchk, funcchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(cookiechk, cookiechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(gschk, gschk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(initchk, initchk_c, "d0-d1/a0-a1");
