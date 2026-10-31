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

#define SW(s, o) (*(UWORD *)((UBYTE *)(s) + (o)))
#define SL(s, o) (*(ULONG *)((UBYTE *)(s) + (o)))
#define SP(s, o) (*(UBYTE **)((UBYTE *)(s) + (o)))

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

static void BLITZ_ROUTINE codeinit_c(void)
{
    autocook = 0xffff;
}
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");

/* free a shape's data and cookie (a3) */
static void freeashape_(UBYTE *s)
{
    ULONG c;

    if (!SW(s, 0))
        return;
    SW(s, 0) = 0;
    if (SW(s, 30) != CLUDGE) {
        SW(s, 30) = 0;
        globfree(SP(s, 14), SW(s, 26));
    }
    c = SL(s, 18);
    if (c) {
        SL(s, 18) = 0;
        globfree((void *)c, SW(s, 22));
    }
}

static void BLITZ_ROUTINE freeashape_c(void)
{
    freeashape_(blitz_string);
}
BLITZ_THUNK(freeashape, freeashape_c, "d0-d1/a0-a1");

/* make the cookie: OR of all planes */
static void cookie_(UBYTE *s)
{
    UBYTE *plane, *ck;
    UWORD psize, rows;

    if (SL(s, 18)) {
        void *c = SP(s, 18);
        SL(s, 18) = 0;
        globfree(c, SW(s, 22));
    }
    ck = globalloc(SW(s, 22), MEMF_CHIP | MEMF_CLEAR);
    SP(s, 18) = ck;
    psize = SW(s, 22);                          /* offset plane to plane */
    plane = SP(s, 14);
    rows = SW(s, 2) - 1;                        /* hite */
    do {
        UWORD planes = SW(s, 4) - 1;            /* depth */
        UBYTE *src = plane, *dst = ck;

        do {
            UWORD words = (UWORD)(SW(s, 6) >> 1) - 1;

            do {
                *(UWORD *)dst |= *(UWORD *)src;
                src += 2;
                dst += 2;
            } while (words-- != 0);
            src -= (WORD)SW(s, 6);
            src += (WORD)psize;
            dst -= (WORD)SW(s, 6);
            __asm__("" : "+a"(src));            /* no __mulsi3 */
        } while (planes-- != 0);
        plane += (WORD)SW(s, 6);
        ck += (WORD)SW(s, 6);
    } while (rows-- != 0);
}

static void BLITZ_ROUTINE docookie_c(void)
{
    cookie_(blitz_string);
}
BLITZ_THUNK(docookie, docookie_c, "d0-d1/a0-a1");

/* ------------------------------------------------- small statements */

static void BLITZ_ROUTINE doautocookie_c(ULONG v BL_REG(d0))
{
    autocook = (UWORD)v;
}
BLITZ_THUNK(doautocookie, doautocookie_c, "d0-d1/a0-a1");

/* ShapeWidth/ShapeHeight: MOVE (a0),d0 - only the low word */
static UWORD BLITZ_ROUTINE shapew_c(UBYTE *s BL_REG(a0)) { return SW(s, 0); }
static UWORD BLITZ_ROUTINE shapeh_c(UBYTE *s BL_REG(a0)) { return SW(s, 2); }
BLITZ_THUNK_POST(doshapew, shapew_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK_POST(doshapeh, shapeh_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

static void BLITZ_ROUTINE domidhandle_c(void)
{
    UBYTE *s = blitz_string;

    SW(s, 10) = SW(s, 0) >> 1;
    SW(s, 12) = SW(s, 2) >> 1;
}
BLITZ_THUNK(domidhandle, domidhandle_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dohandle_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2))
{
    UBYTE *s = blitz_string;

    SW(s, 10) = (UWORD)x;
    SW(s, 12) = (UWORD)y;
}
BLITZ_THUNK(dohandle, dohandle_c, "d0-d1/a0-a1");

/* CopyShape: a3 = source, a2 = destination (docopyshape2 + cookie) */
static void nall(UBYTE *from, UBYTE *to, UWORD ptroff, UWORD sizeoff)
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

static void copyshape2(UBYTE *from, UBYTE *to)
{
    int i;

    freeashape_(to);
    for (i = 0; i < 8; i++)
        ((ULONG *)to)[i] = ((ULONG *)from)[i];
    nall(from, to, 14, 26);
}

static void BLITZ_ROUTINE docopyshape_c(UBYTE *dst BL_REG(a2))
{
    copyshape2(blitz_string, dst);
    nall(blitz_string, dst, 18, 22);
}
BLITZ_THUNK(docopyshape, docopyshape_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------ InitShape */

static void BLITZ_ROUTINE initshape_c(ULONG w BL_REG(d1), ULONG h BL_REG(d2), ULONG d BL_REG(d3))
{
    UBYTE *s = blitz_string;
    UWORD bpr, size;
    ULONG len, xlen;

    freeashape_(s);
    size = (UWORD)((UWORD)h << 6);
    bpr = (UWORD)((UWORD)(w + 15) >> 4);
    size |= bpr;
    bpr += bpr;
    size += 1;
    SW(s, 0) = (UWORD)w;
    SW(s, 2) = (UWORD)h;
    SW(s, 4) = (UWORD)d;
    SW(s, 6) = bpr;
    SW(s, 8) = size;
    SL(s, 10) = 0;
    size = bpr + 2;
    len = (ULONG)bpr * (UWORD)h;                /* MULU */
    xlen = (ULONG)size * (UWORD)h;
    SW(s, 22) = (UWORD)len;
    SW(s, 24) = (UWORD)xlen;
    len = (ULONG)(UWORD)len * (UWORD)d;
    xlen = (ULONG)(UWORD)xlen * (UWORD)d;
    SW(s, 26) = (UWORD)len;
    SW(s, 28) = (UWORD)xlen;
    SP(s, 14) = globalloc(len, MEMF_CHIP | MEMF_CLEAR);
    if (autocook)
        SP(s, 18) = globalloc(SW(s, 22), MEMF_CHIP | MEMF_CLEAR);
}
BLITZ_THUNK(_initshape, initshape_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------ GetaShape */

/* a3 = shape, a2 = used bitmap, d1 = x, d2 = y, d3 = w, d4 = h */
static void getashape(UBYTE *shape, UBYTE *bm, UWORD x, UWORD y, UWORD w, UWORD h)
{
    UBYTE *tbm = tb.bm;
    ULONG p;

    freeashape_(shape);
    {
        struct xregs r = { .d1 = w, .d2 = h, .d3 = SW(bm, 4), .a3 = (ULONG)tbm };
        XJSR(TOK_extbitmap, r);                 /* create bitmap! */
    }
    SL(shape, 2) = SL(tbm, 2);                  /* height, depth */
    SW(shape, 6) = SW(tbm, 0);
    SL(shape, 10) = 0;
    SL(shape, 14) = SL(tbm, 8);
    p = (ULONG)SW(tbm, 0) * SW(tbm, 2);
    SW(shape, 22) = (UWORD)p;
    p = (ULONG)(UWORD)p * SW(tbm, 4);
    SW(shape, 26) = (UWORD)p;
    p = (ULONG)(UWORD)(SW(tbm, 0) + 2) * SW(tbm, 2);
    SW(shape, 24) = (UWORD)p;
    p = (ULONG)(UWORD)p * SW(tbm, 4);
    SW(shape, 28) = (UWORD)p;
    SW(shape, 8) = (UWORD)(SW(tbm, 2) << 6) | (UWORD)((SW(tbm, 0) >> 1) + 1);
    SW(shape, 0) = w;
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

void BLITZ_ROUTINE b_getashape_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), UBYTE *bm BL_REG(a2));
void BLITZ_ROUTINE b_getashape_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2), ULONG w BL_REG(d3),
                                 ULONG h BL_REG(d4), UBYTE *bm BL_REG(a2))
{
    getashape(blitz_string, bm, x, y, w, h);
}
BLITZ_THUNK(b_getashape, b_getashape_c, "d0-d1/a0-a1");

/* Amiga mode: OwnBlitter, the Blitz mode routine, wait, DisownBlitter */
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
    getashape(blitz_string, bm, x, y, w, h);
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

/* SaveShapes: shapes a3..a2 */
static void BLITZ_ROUTINE dosaveshapes_c(UBYTE *last BL_REG(a2), const char *name BL_REG(d2),
                                         struct DosLibrary *dos BL_REG(a6))
{
    UBYTE *s = blitz_string;
    BPTR fh;

    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (!fh)
        BLITZ_RUNERR(mess2);
    do {
        if (Write(fh, s, 32) != 32)
            BLITZ_RUNERR(mess3);
        if (SW(s, 0)) {
            if ((UWORD)Write(fh, SP(s, 14), SW(s, 26)) != SW(s, 26))
                BLITZ_RUNERR(mess3);
        }
        s += 32;
    } while ((ULONG)s <= (ULONG)last);
    Close(fh);
}
BLITZ_THUNK(dosaveshapes, dosaveshapes_c, "d0-d1/a0-a1");

static void loadshapes(UBYTE *s, UBYTE *last, const char *name, struct DosLibrary *dos)
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
        SL(s, 18) = 0;
        if (SW(s, 0)) {
            /* check validity of shape block... */
            p = (ULONG)SW(s, 6) * SW(s, 2);
            if ((UWORD)p != SW(s, 22))
                BLITZ_RUNERR(mess3);
            p = (ULONG)(UWORD)p * SW(s, 4);
            if ((UWORD)p != SW(s, 26))
                BLITZ_RUNERR(mess3);
            SP(s, 14) = globalloc(SW(s, 26), MEMF_CHIP);
            if ((UWORD)Read(fh, SP(s, 14), SW(s, 26)) != SW(s, 26))
                BLITZ_RUNERR(mess3);
            if (autocook)
                cookie_(s);
        }
        s += 32;
    } while ((ULONG)s <= (ULONG)last);
    Close(fh);
}

static void BLITZ_ROUTINE doloadshapes_c(UBYTE *last BL_REG(a2), const char *name BL_REG(d2),
                                         struct DosLibrary *dos BL_REG(a6))
{
    loadshapes(blitz_string, last, name, dos);
}
BLITZ_THUNK(doloadshapes, doloadshapes_c, "d0-d1/a0-a1");

/* LoadShapes Shape#,Filename$: a2 = shape 0, d2 = maximum.
   BUG(orig) (#939): the last shape is a2 + (WORD)((max-1) << 5) */
static void BLITZ_ROUTINE doloadshapes2_c(UBYTE *base BL_REG(a2), ULONG max BL_REG(d2),
                                          const char *name BL_REG(d1),
                                          struct DosLibrary *dos BL_REG(a6))
{
    loadshapes(blitz_string, base + (WORD)(UWORD)((UWORD)(max - 1) << 5), name, dos);
}
BLITZ_THUNK(doloadshapes2, doloadshapes2_c, "d0-d1/a0-a1");

/* -------------------------------------------------------- DecodeShapes */

static void cludge(UBYTE *shape, UBYTE *last, UBYTE *mem)
{
    UBYTE *src = mem;
    ULONG fastmem = (ULONG)mem & 0xffe00000UL;

    do {
        ULONG p = (ULONG)SW(src, 6) * SW(src, 2);
        int i;

        if ((UWORD)p != SW(src, 22))
            return;
        p = (ULONG)(UWORD)p * SW(src, 4);
        if ((UWORD)p != SW(src, 26))
            return;
        freeashape_(shape);
        for (i = 0; i < 8; i++)                 /* copy header */
            ((ULONG *)shape)[i] = ((ULONG *)src)[i];
        src += 32;
        SL(shape, 18) = 0;
        SL(shape, 14) = 0;
        if (SW(shape, 0)) {
            if (fastmem) {
                UBYTE *d = globalloc(SW(shape, 26), MEMF_CHIP);
                UWORD n = (UWORD)(SW(shape, 26) >> 1) - 1;

                SP(shape, 14) = d;
                do {
                    *(UWORD *)d = *(UWORD *)src;
                    d += 2;
                    src += 2;
                } while (n-- != 0);
            } else {
                SP(shape, 14) = src;            /* in chip: use in place */
                src += SW(shape, 26);
                SW(shape, 30) = CLUDGE;         /* don't free me freeshape! */
            }
            if (autocook)
                cookie_(shape);
        }
        shape += 32;
    } while ((ULONG)shape <= (ULONG)last);
}

static void BLITZ_ROUTINE cludgeshapes_c(UBYTE *last BL_REG(a2), UBYTE *mem BL_REG(d2))
{
    if (!mem)
        JMP_ERR10();
    cludge(blitz_string, last, mem);
}
BLITZ_THUNK(_cludgeshapes, cludgeshapes_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE cludgeshapes2_c(UBYTE *base BL_REG(a2), ULONG max BL_REG(d2),
                                          UBYTE *mem BL_REG(d1))
{
    UBYTE *last = base + (WORD)(UWORD)((UWORD)(max - 1) << 5); /* BUG(orig) (#939) */

    if (!mem)
        JMP_ERR10();
    cludge(blitz_string, last, mem);
}
BLITZ_THUNK(_cludgeshapes2, cludgeshapes2_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------ LoadShape */

/* iffiolib/ilbmifflib call with d0 (and a0); result d0, Z in *z */
#define IJSRZ(tok, d0v, a0v, zp) ({ struct xregs r_ = { .d0 = (d0v), .a0 = (ULONG)(a0v), \
    .a3 = (ULONG)blitz_string }; XJSR(tok, r_); *(zp) = r_.z; r_.d0; })
#define IJSR(tok, d0v, a0v) ({ struct xregs r_ = { .d0 = (d0v), .a0 = (ULONG)(a0v), \
    .a3 = (ULONG)blitz_string }; XJSR(tok, r_); r_.d0; })

static UWORD bmode;                             /* 2 = xor, 0 = set */
static ULONG offs[8];

static void doshload_(UBYTE *shape, const char *name)
{
    UBYTE *first = shape, *frame, *dst, *st;
    UWORD bpr, rows, depth, pstep;
    ULONG res, p;
    UBYTE z;

    freeashape_(shape);
    {
        struct xregs r = { .a0 = (ULONG)name, .a3 = (ULONG)blitz_string };
        XJSR(TOK_ilbminit, r);                  /* open the ilbm iff! */
        if (r.z)
            BLITZ_RUNERR(mess2);
        st = (UBYTE *)r.a0;
    }
    SW(first, 0) = SW(st, 0);
    res = SW(st, 2);
    SW(first, 2) = (UWORD)res;
    rows = (UWORD)res;
    res = (UWORD)(res << 6);
    depth = SW(st, 4);
    SW(first, 4) = depth;
    bpr = (UWORD)(SW(st, 6) << 1);
    SW(first, 6) = bpr;
    SW(first, 8) = (UWORD)res | (UWORD)((bpr >> 1) + 1);
    SL(first, 10) = 0;
    p = (ULONG)(UWORD)(bpr + 2) * rows;
    SW(first, 24) = (UWORD)p;
    p = (ULONG)(UWORD)p * depth;
    SW(first, 28) = (UWORD)p;
    p = (ULONG)bpr * rows;
    SW(first, 22) = (UWORD)p;
    pstep = (UWORD)p;
    p = (ULONG)(UWORD)p * depth;
    SW(first, 26) = (UWORD)p;
    dst = globalloc(p, MEMF_CHIP);              /* global alloc for data! */
    SP(first, 14) = dst;
    res = IJSR(TOK_ifffind, 0x424f4459, st); /* "BODY" (not checked) */
    pstep -= bpr;
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
    bmode = (UWORD)res & 2;
    do {                                        /* create next frame... */
        UWORD planes;
        ULONG dpos = 0, *off;
        ULONG *optr;

        dst = frame + 32;
        copyshape2(frame, dst);
        frame = dst;
        SL(frame, 18) = 0;
        res = IJSR(TOK_ifffind, 0x444c5441, st); /* "DLTA" */
        planes = SW(frame, 4) - 1;
        optr = offs;
        do {
            dpos += 4;
            res = IJSR(TOK_ifflong, res, st);
            *optr++ = res;
        } while (planes-- != 0);
        /* do planes... */
        planes = SW(frame, 4) - 1;
        off = offs;
        dst = SP(frame, 14);
        do {
            ULONG o = *off++;

            if (o) {
                o -= dpos;
                if (o) {
                    res = IJSR(TOK_iffskip, o, st);
                    dpos = off[-1];
                }
                {
                    UWORD cols = SW(frame, 6) - 1;
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
                                        if (bmode)
                                            *dst ^= v;
                                        else
                                            *dst = v;
                                        dst += (WORD)SW(frame, 6);
                                    } while (count-- != 0);
                                } else if ((BYTE)op < 0) {
                                    UWORD count = (UWORD)((UWORD)res & 127) - 1;

                                    do {
                                        dpos++;
                                        res = IJSR(TOK_iffbyte, res, st);
                                        if (bmode)
                                            *dst ^= (UBYTE)res;
                                        else
                                            *dst = (UBYTE)res;
                                        dst += (WORD)SW(frame, 6);
                                    } while (count-- != 0);
                                } else {
                                    /* EXT d0:MULU 6(a3),d0:ADD.l d0,a2 */
                                    dst += (ULONG)(UWORD)(WORD)(BYTE)op * SW(frame, 6);
                                }
                            } while (nops-- != 0);
                            dst = ops;
                        }
                        dst++;                  /* skipops */
                    } while (cols-- != 0);
                    dst = col;
                }
            }
            dst += (WORD)SW(frame, 22);         /* pskip */
        } while (planes-- != 0);
        if (autocook)
            cookie_(frame);
        res = IJSRZ(TOK_ifffind, 0x414e4844, st, &z); /* "ANHD" */
    } while (z);
skipanim:
    IJSR(TOK_iffclose, res, st);             /* close the IFF */
}

static void BLITZ_ROUTINE doshload_c(UBYTE *shape BL_REG(a0), const char *name BL_REG(d1))
{
    doshload_(shape, name);
}
BLITZ_THUNK(doshload, doshload_c, "d0-d1/a0-a1");

/* LoadShape with a palette: palettelib +3 (a0 = file name, a1 = palette) */
static void BLITZ_ROUTINE doshload2_c(UBYTE *shape BL_REG(a0), const char *name BL_REG(d1),
                                      void *pal BL_REG(a1))
{
    struct xregs r = { .d1 = (ULONG)name, .a0 = (ULONG)name, .a1 = (ULONG)pal,
                       .a3 = (ULONG)blitz_string };

    XJSR(TOK_palload, r);
    doshload_(shape, name);
}
BLITZ_THUNK(doshload2, doshload2_c, "d0-d1/a0-a1");

/* *************************** ERROR CHECKING *************************** */

static void BLITZ_ERRCHK schkz_c(UBYTE *last BL_REG(a2))
{
    if ((ULONG)blitz_string > (ULONG)last)
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK funcchk_c(UWORD *s BL_REG(a0))
{
    if (!*s)
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK cookiechk_c(void)
{
    if (!*(UWORD *)blitz_string)
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
