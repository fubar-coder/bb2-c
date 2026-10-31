/*
 * blitzoslib - command library 117 (new for bum8): "Blitz OS" pages of
 * boxes (buttons, string gadgets, props, shapes, lists, drags) drawn into
 * a bitmap with the Blitz mode drawing commands, and BlitzEvent, which
 * drives them with joystick, cursor keys and mouse.  Object library
 * "Page" (32 bytes, maximum 4).
 *
 * C translation of bblibsrc/blitzoslib; binary of the CD's acidlibs
 * checked: same as the source.
 *
 * Page (32 byte object): memsize.l, memloc.l (the boxes, 32 bytes each,
 * MEMF_CLEAR), mybitmap.l (bitmap object), numboxes.w, pens c0..c3
 * (dark, light, mid, text; InitPage sets 1,3,2,1 - the help text says
 * 3,1,2,3), selected.l (box).  Box: x y w h, btype (negative = disabled;
 * 0 view, 1 button, 2 string, 3 prop, 4 shape/display, 5 list, 6 drag),
 * color (NOT = inverted bevel), then fields that overlap:
 *   +12 value.w range.w | bval x/y (prop knob 0..16383, -1 = fixed;
 *       drag delta) | showbp.l (view: source bitmap)
 *   +16 isshape.w shapeoff.w | posbp x/y (view)
 *   +20 tlen.l text.l (button/list: strings separated by 0; string box:
 *       text with prompt, padded with spaces)
 *   +28 prompt.w anslen.w | bsize w/h (prop knob)
 *
 * All drawing goes through Blitz mode routines of other libraries
 * (BLibJsr, $C000 placeholders: 2dlib Boxf/Line, shapes Blit, fontlib
 * Locate/Colour/print, scrolllib Scroll; BlitzEvent: Joyx/Joyy/Joyb,
 * MouseXSpeed/MouseX/MouseY/PositionMouse, RawStatus, BlitzKey).  They
 * get their registers through BCALL below (d0-d4, a0-a3 from a struct,
 * result d0 and the Z flag; a3/a4 are restored, like the original saves
 * a3-a4/a4 around its drawing).
 *
 * Most tokens have `1` as the first error check field of !subs (a plain
 * 1, no address; kept).  The checks "UnInitialised Page" / "Illegal Box#
 * For Page" ... are in the normal code (always on, there is no error
 * check area).
 *
 * BUG(orig), kept:
 *  - Box numbers: ASL #5 as word, ADDA.w (box >= 1024 gives negative
 *    offsets) (#81); SelectBox Box# is not range checked (negative or
 *    >= 1024: deselect only) (#82); SpotBox has no page check, with 0
 *    boxes it scans 65536 (#83).
 *  - AddBox with a string box 8..15 pixels wide: tlen 1, the space fill
 *    (DBRA of tlen-2) writes 65536 spaces. (#84)
 *  - BoxString strips trailing spaces backwards without a limit: an
 *    all-space answer runs into the prompt / memory before the text (d0
 *    0: nothing, negative: a huge copy). (#85)
 *  - BoxPrint Bank#,Line: skips lines over 0 bytes too, and replaces the
 *    line's LF in the bank by 0. (#87)
 *  - oneline: a box with tlen 0 but an old text pointer prints from
 *    address 0 with the pointer as length (fresh boxes have text 0). (#89)
 *  - BlitzEvent: with cursor up/down it reads btype of `selected` also
 *    when nothing is selected (#90); Backspace at position 0 leaves the
 *    cursor off (#91).
 *  - DeSelectBox does not clear `selected`. (#94)
 * Kept from the original, not a bug (no visible effect):
 *  - BoxString writes no 0, a3 = the end (the compiler appends
 *    clr.b (a3)+ itself where it passes a string on, blitz2.s:8767
 *    sbtolong). (#86)
 *  - BoxPrint overwrites the string length at -4(a2) with Scroll's size
 *    (that argument area belongs to this call only and is dropped by the
 *    compiler unread). (#88)
 *  - BlitzEvent: the cursor key character cycle uses BMI after CMP.b (N
 *    flag - for all 256 values the same result as an unsigned compare). (#92)
 *  - SelectBox3 redraws the old box also when it is the new one (drawbox
 *    draws completely, so only extra time). (#93)
 * DIFF: the coordinates passed to Boxf/Line/Locate are computed as words
 * and sign extended; the original leaves the sign of the box field in the
 * upper word (differs only for coordinates around 0/negative).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include <graphics/gfx.h>

/* entry points (thunks, defined below) */
void _initpage(void), _freepage(void), _addbox(void), _addbox1(void);
void _addbox2(void), _redrawpage(void), _redrawbox(void), _selectbox(void);
void _selectbox2(void), _selectbox3(void), _deselectbox(void);
void _activateselected(void), _setbox(void), _setbox2(void);
void _boxvalue(void), _boxvalue2(void), _setboxstring(void);
void _boxstring(void), _boxstring2(void), _boxshow(void), _boxshow2(void);
void _boxselected(void), _boxprint(void), _boxprint2(void);
void _setboxcolor(void), _blitzevent(void), _setboxtitle(void);
void _disablebox(void), _enablebox(void), _mouseenabled(void);
void _setpagepens(void), _boxpropx(void), _boxpropy(void), _changebox(void);
void _togglebox(void), _spotbox(void);

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#blitzoslib,0,0,0,0} */
BLITZ_LIBHEADER(LIB_blitzoslib, 0, BL_NORET, 0, 0);

BLITZ_DUMTOKE(_toke, "Page", "");

#define PAGE   LIB_blitzoslib, 0x1380           /* a3 = page of param d0 */
#define UPAGE  LIB_blitzoslib, 0x1302           /* #ua3: a3 = used page */

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1281, PAGE, LIB_blitzoslib, 0x0604);
BLITZ_SUBS(_initpage, 0, 0);
BLITZ_NAME("InitPage", "Page#,BitMap#,NumBoxes");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(PAGE);
BLITZ_SUBS(_freepage, 0, 0);
BLITZ_NAME("FreePage", "Page#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(PAGE);
BLITZ_SUBS(_addbox, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(PAGE);
BLITZ_SUBS(_addbox1, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(PAGE);
BLITZ_SUBS(_addbox2, 0, 0);
BLITZ_NAME("AddBox", "Page#,Box#,x,y,w,h,type[,text$]|[,shape,shape]");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_redrawpage, 1, 0);
BLITZ_NAME("RedrawPage", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_redrawbox, 1, 0);
BLITZ_NAME("RedrawBox", "Box#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_selectbox, 1, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_selectbox2, 1, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_selectbox3, 1, 0);
BLITZ_NAME("SelectBox", "[Box#]|[x+,y+][0,x,y] ;deselects previously selected");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_deselectbox, 1, 0);
BLITZ_NAME("DeSelectBox", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_activateselected, 1, 0);
BLITZ_NAME("ActivateSelectedBox", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_setbox, 0, 0);
BLITZ_ARGS(BL_WORD, BL_QUICK, BL_QUICK);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_setbox2, 1, 0);
BLITZ_NAME("SetBox", "Box#[,Value]|[xprop,yprop]");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_boxvalue, 1, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(PAGE);
BLITZ_SUBS(_boxvalue2, 1, 0);
BLITZ_NAME("BoxValue", "(Box#)|(Page#,Box#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_setboxstring, 0, 0);
BLITZ_NAME("SetBoxString", "Box#,String$");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_blitzoslib, 0x1102);             /* #ua1: a1! */
BLITZ_SUBS(_boxstring, 1, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_blitzoslib, 0x1180);             /* a1! */
BLITZ_SUBS(_boxstring2, 1, 0);
BLITZ_NAME("BoxString", "(Box#)|(Page#,Box#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(UPAGE, LIB_bitmaplib, 0x1281);
BLITZ_SUBS(_boxshow, 1, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_boxshow2, 1, 0);
BLITZ_NAME("BoxShow", "Box#[,Bitmap#,x,y]|[dx,dy]");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_boxselected, 1, 0);
BLITZ_NAME("BoxSelected", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_boxprint, 1, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(UPAGE, LIB_banklib, 0x1281);
BLITZ_SUBS(_boxprint2, 1, 0);
BLITZ_NAME("BoxPrint", "Box#[,String$]|[Bank#,Line]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_setboxcolor, 1, 0);
BLITZ_NAME("SetBoxColor", "Box#,color");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_blitzevent, 1, 0);
BLITZ_NAME("BlitzEvent", ";handles user input and returns event");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_setboxtitle, 1, 0);
BLITZ_NAME("SetBoxTitle", "Box#,String$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_disablebox, 1, 0);
BLITZ_NAME("DisableBox", "Box#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_enablebox, 1, 0);
BLITZ_NAME("EnableBox", "Box#");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_mouseenabled, 1, 0);
BLITZ_NAME("MouseEnabled", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(PAGE);
BLITZ_SUBS(_setpagepens, 0, 0);
BLITZ_NAME("SetPagePens", "Page#,c0,c1,c2,c3 (defaults: 3,1,2,3)");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_boxpropx, 1, 0);
BLITZ_NAME("BoxPropX", "Box#");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_boxpropy, 1, 0);
BLITZ_NAME("BoxPropY", "Box#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(PAGE);
BLITZ_SUBS(_changebox, 0, 0);
BLITZ_NAME("ChangeBox", "Page#,Box#,Value");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_togglebox, 1, 0);
BLITZ_NAME("ToggleBox", "Box#");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(UPAGE);
BLITZ_SUBS(_spotbox, 1, 0);
BLITZ_NAME("SpotBox", "; returns box that mouse is over");

BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, 0, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,4,5} ;4 pages 32 byte */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 4, 5);

/* ------------------------------------------------- Blitz mode routines */

/* registers for BCALL: in d0-d4/a0-a3, out d0 and the Z flag */
struct bregs {
    ULONG d0, d1, d2, d3, d4, a0, a1, a2, a3;
    UBYTE z;
};

/* BLibJsr tok,form with the registers of r (d0-d4/a0-a2; a3 stays the
   page, as in the original; BCALL_A3 also loads r.a3); a3/a4 (and a6) are
   restored, d0-d7/a0-a2 may change */
#ifndef BL_CODE_MODEL
#define BCALL_(tok, form, r, regs) do {                                     \
    register struct bregs *_r __asm("a6") = &(r);                           \
    __asm volatile("movem.l a3-a4/a6,-(sp)\n\t"                             \
                   "movem.l (a6)," regs "\n\t"                               \
                   "jsr _blitz_tok+%c1\n\t"                                 \
                   "move.l 8(sp),a6\n\t"                                    \
                   "seq 36(a6)\n\t"                                         \
                   "move.l d0,(a6)\n\t"                                     \
                   "movem.l (sp)+,a3-a4/a6"                                 \
        : : "a"(_r), "i"(BL_BTOKVAL(tok, form))                             \
        : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a0", "a1", "a2", \
          "cc", "memory");                                                  \
} while (0)
#else
#define BCALL_(tok, form, r, regs) ((void)(r))
#endif
#define BCALL(tok, form, r)    BCALL_(tok, form, r, "d0-d4/a0-a2")
#define BCALL_A3(tok, form, r) BCALL_(tok, form, r, "d0-d4/a0-a3")

/* tokens (the binary's placeholders) */
#define T_RAWSTATUS     0x3102  /* blitzkeyslib: d0 = raw key -> Z = up */
#define T_BLITZKEY      0x3106  /* blitzkeyslib: -> d0 = key (<0 none) */
#define T_SCROLL        0x3202  /* scrolllib */
#define T_MOUSEX        0x3604  /* mouselib: -> d0 (Quick) */
#define T_MOUSEY        0x3605
#define T_MOUSEXSPEED   0x3606
#define T_POSITIONMOUSE 0x3608
#define T_BOXF          0x3b84  /* 2dlib: d0-d3, d4 = colour */
#define T_LINE          0x3b85
#define T_COLOUR        0x4703  /* fontlib, form 1: d0 = fg, d1 = bg */
#define T_LOCATE        0x4704  /* fontlib: d0 = column, d1 = row (y<<13) */
#define T_PRINTN        0x470e  /* fontlib: a0 = text, d0 = length */
#define T_PRINT0        0x470f  /* fontlib: a0 = null terminated text */
#define T_BLIT          0x4b01  /* d0 = shape, d1 = x, d2 = y */
#define T_JOYX          0x5f01
#define T_JOYY          0x5f02
#define T_JOYB          0x5f04

static void boxf(WORD x1, WORD y1, WORD x2, WORD y2, ULONG c)
{
    struct bregs r = { .d0 = (LONG)x1, .d1 = (LONG)y1, .d2 = (LONG)x2,
                       .d3 = (LONG)y2, .d4 = c };
    BCALL(T_BOXF, 0, r);
}

static void line(WORD x1, WORD y1, WORD x2, WORD y2, ULONG c)
{
    struct bregs r = { .d0 = (LONG)x1, .d1 = (LONG)y1, .d2 = (LONG)x2,
                       .d3 = (LONG)y2, .d4 = c };
    BCALL(T_LINE, 0, r);
}

static void locate(ULONG x, ULONG y)
{
    struct bregs r = { .d0 = x, .d1 = y };
    BCALL(T_LOCATE, 0, r);
}

static void colour(ULONG fg, ULONG bg)
{
    struct bregs r = { .d0 = fg, .d1 = bg };
    BCALL(T_COLOUR, 1, r);
}

static void printn(const UBYTE *s, ULONG len)
{
    struct bregs r = { .d0 = len, .a0 = (ULONG)s };
    BCALL(T_PRINTN, 0, r);
}

/* Scroll: src (sx,sy) of bitmap src to (dx,dy) of dst, w x h */
static void scroll(ULONG sx, ULONG sy, ULONG dx, ULONG dy, UWORD w, UWORD h,
                   void *src, void *dst)
{
    UWORD wh[2] = { h, w };                     /* -4(a2) = h, -2(a2) = w */
    struct bregs r = { .d0 = sx, .d1 = sy, .d2 = dx, .d3 = dy,
                       .a0 = (ULONG)src, .a1 = 0xdff000,
                       .a2 = (ULONG)(wh + 2), .a3 = (ULONG)dst };
    BCALL_A3(T_SCROLL, 0, r);
}

/* RawStatus: Z clear = key down */
static int rawstatus(UWORD key)
{
    struct bregs r = { .d0 = key };
    BCALL(T_RAWSTATUS, 0, r);
    return !r.z;
}

/* BLibJsr tok with d0 -> d0 */
#define BCALL0(tok, value) ({ struct bregs r_ = { .d0 = (value) }; BCALL(tok, 0, r_); r_.d0; })

/* DIVS.w with the original's overflow handling (BVC: else 32767 in the
   low word); a divisor of 0 raises the divide-by-zero exception */
static WORD divs_sat(ULONG dividend, WORD divisor)
{
    __asm__("divs.w %1,%0\n\t"
            "bvc.s 1f\n\t"
            "move.w #32767,%0\n"
            "1:" : "+d"(dividend) : "dmi"(divisor) : "cc");
    return (WORD)dividend;
}

/* !waitblit: wait for the blitter (DMACONR bit 14) */
static void waitblit(void)
{
    volatile UBYTE *dmaconr = (volatile UBYTE *)0xdff002;

    (void)*dmaconr;
    while (*dmaconr & 0x40)
        ;
}

/* ------------------------------------------------------------ memlib */

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

/* ------------------------------------------------------- page and box */

/* a box (32 bytes; the fields from +12 overlap, depending on the type) */
struct box {
    WORD x, y, w, h;            /* +0 */
    WORD btype;                 /* +8: negative = disabled */
    WORD color;                 /* +10: NOT = inverted bevel */
    union {                     /* +12 */
        struct {
            WORD value;         /* button/list: the selected string */
            WORD range;         /* the number of strings */
        };
        struct {
            WORD propx, propy;  /* bval: prop knob 0..16383, -1 = fixed */
        };
        struct {
            WORD dx, dy;        /* drag: the mouse delta */
        };
        UBYTE *showbp;          /* view: the source bitmap */
    };
    union {                     /* +16 */
        struct {
            WORD isshape;       /* shape button: -1 */
            WORD shapeoff;      /* the first shape */
        };
        struct {
            WORD posx, posy;    /* posbp: view: the position in showbp */
        };
    };
    ULONG tlen;                 /* +20: the size of text's memory */
    UBYTE *text;                /* +24: strings separated by 0 (string box:
                                   prompt and answer, padded with spaces) */
    union {                     /* +28 */
        struct {
            WORD prompt;        /* string box: the length of the prompt */
            WORD anslen;
        };
        struct {
            WORD knobw, knobh;  /* bsize: the size of the prop knob */
        };
    };
} BL_PACKED;
static_assert(sizeof(struct box) == 32);

/* the page object (the first 26 of its 32 bytes) */
struct page {
    ULONG memsize;
    struct box *memloc;         /* numboxes boxes */
    UBYTE *mybitmap;
    UWORD numboxes;
    WORD c0, c1, c2, c3;        /* drk light mid text */
    struct box *selected;       /* pointer to currently selected box */
} BL_PACKED;
static_assert(sizeof(struct page) == 26);

/* box n: memloc + n << 5 as a word (ASL #5:ADDA.w, BUG(orig) (#81)) */
#define BOXAT(p, off) ((struct box *)((UBYTE *)(p)->memloc + (off)))

static const char mess1[] = "UnInitialised Page";
static const char mess2[] = "Illegal Box# For Page";
static const char mess3[] = "Value Out of Range";
static const char mess4[] = "No Room For String";
static const char mess5[] = "Illegal Box Type";
static const char mess6[] = "Unitialised Bank";
static const char mess7[] = "Not a Prop Box";
static const char mess8[] = "Unitialised BitMap";

#define PG ((struct page *)blitz_string)

/* TST.l (a3):BEQ err1:CMP numboxes,d0:BCC err2; box address */
static struct box *getbox(struct page *p, ULONG n)
{
    if (!p->memsize)
        BLITZ_RUNERR(mess1);
    if ((UWORD)n >= p->numboxes)
        BLITZ_RUNERR(mess2);
    return BOXAT(p, (WORD)(UWORD)(n << 5));   /* ASL #5:ADD d0,a0 */
}

static void chkpage(struct page *p)
{
    if (!p->memsize)
        BLITZ_RUNERR(mess1);
}

/* drawing state (variables of the original) */
static WORD xyxy[4], xyxy2[4];
static ULONG text[2];                           /* text pointer, length */
static WORD btype;
static WORD c0, c1, c2, c3;
static WORD sel;                                /* -1 = NOT the selected box */
static WORD nable;
static ULONG xx, yy;
static WORD lines, count;

/* ---------------------------------------------------------- drawbox */

/* row for Locate: (y + 2) as 16.16, / 8 (EXT.l:ADDQ:SWAP:LSR.l #3) */
static ULONG locrow(WORD y)
{
    ULONG v = (ULONG)(LONG)y;

    BL_SETW(v, v + 2);
    return BL_SWAP(v) >> 3;
}

static void drawbox(struct page *p, struct box *box)
{
    ULONG disabled;                             /* d0: 0 or -1 */
    ULONG darklight;                            /* d0: the c0:c1 pens */
    ULONG midtext;
    WORD x, y, x2, y2;

    disabled = 0;
    btype = box->btype;
    if (btype < 0) {
        btype = -btype;
        disabled = (ULONG)-1;
    }
    nable = (WORD)disabled;
    darklight = (ULONG)(UWORD)p->c0 << 16 | (UWORD)p->c1;
    midtext = (ULONG)(UWORD)p->c2 << 16 | (UWORD)p->c3;
    sel = 0;
    if (p->selected != box) {
        BL_SETW(midtext, darklight);            /* MOVE d0,d1 */
        sel = -1;
    } else if (btype == 4)
        BL_SETW(darklight, midtext);            /* MOVE d1,d0 */
    midtext = BL_SWAP(midtext);
    BL_SETW(midtext, box->color);
    if ((WORD)midtext < 0) {
        darklight = BL_SWAP(darklight);
        BL_SETW(midtext, ~midtext);
        if (btype == 0 && box->tlen)
            darklight = 0;
    }
    midtext = BL_SWAP(midtext);
    c0 = (WORD)(darklight >> 16);
    c1 = (WORD)darklight;
    c2 = (WORD)(midtext >> 16);
    c3 = (WORD)midtext;

    x = box->x;
    y = box->y;
    x2 = x + box->w - 1;
    y2 = y + box->h - 1;
    xyxy[0] = x;
    xyxy[1] = y;
    xyxy[2] = x2;
    xyxy[3] = y2;
    if (nable) {
        boxf(x, y, x2, y2, (LONG)c2);
        return;
    }
    if (btype == 4)
        goto notext;
    if (btype == 6 && !box->tlen)
        goto notext;
    if (btype == 1 && box->isshape) {
        /* shape button */
        boxf(x + 2, y + 1, x2 - 2, y2 - 1, (LONG)(sel ? c2 : c0));
        {
            struct bregs r = { .d0 = (LONG)(WORD)(box->shapeoff + box->value),
                               .d1 = (LONG)(WORD)(xyxy[0] + 2),
                               .d2 = (LONG)(WORD)(xyxy[1] + 1) };
            BCALL(T_BLIT, 0, r);
        }
        goto notext;
    }
    boxf(x + 2, y + 1, x2 - 2, y2 - 1, (LONG)c2);
    if (btype == 3) {
        /* prop: the knob */
        WORD px = x + 2, py = y + 1;
        WORD pw = box->w - 3 - box->knobw;
        WORD ph = box->h - 1 - box->knobh;
        WORD valx = box->propx, valy = box->propy;
        WORD kx2 = box->knobw, ky2 = box->knobh;  /* bsize, then
                                                                   the knob's x2/y2 */

        if (valx >= 0)
            px += (WORD)(((ULONG)(UWORD)pw * (UWORD)(valx + valx)) >> 16); /* MULU:SWAP */
        if (valy >= 0)
            py += (WORD)(((ULONG)(UWORD)ph * (UWORD)(valy + valy)) >> 16);
        kx2 += px - 1;
        ky2 += py - 1;
        xyxy2[0] = px;
        xyxy2[1] = py;
        xyxy2[2] = kx2;
        xyxy2[3] = ky2;
        if (!sel)
            boxf(px, py, kx2, ky2, (LONG)c3);
        line(px, py, kx2, py, (LONG)c1);
        line(kx2, py, kx2, ky2, (LONG)c1);
        line(kx2 + 1, py, kx2 + 1, ky2, (LONG)c1);
        line(px, py, px, ky2, (LONG)c0);
        line(px - 1, py + 1, px - 1, ky2, (LONG)c0);
        line(px + 1, ky2, kx2 - 1, ky2, (LONG)c0);
    }
    /* dotext */
    if (btype == 5) {
        /* list */
        WORD visible = (UWORD)(box->h - 3) >> 3, first;
        WORD half;                              /* d0w: half the lines */
        WORD maxfirst;                          /* d0w: the highest first entry */
        UBYTE *entry;

        lines = visible;
        half = (UWORD)visible >> 1;
        first = box->value - half;
        if (first < 0)
            first = 0;
        maxfirst = box->range - lines;
        if (maxfirst < 0) {
            maxfirst = 0;
            lines = box->range;
        }
        if (first - maxfirst >= 0)              /* CMP d0,d2:BMI noj */
            first = maxfirst;
        count = box->value - first;
        entry = box->text;
        while (first-- != 0)                    /* BRA inig: DBRA d2 */
            while (*entry++)
                ;
        text[0] = (ULONG)entry;
        xx = (ULONG)(LONG)xyxy[0];
        BL_SETW(xx, (UWORD)(xyxy[0] + 11) >> 3);
        yy = locrow(xyxy[1]);
        colour((LONG)c1, (LONG)c2);
        do {
            const UBYTE *s, *e;

            locate(xx, yy);
            yy += 0x10000;                      /* ADD #1,yy (high word) */
            s = (const UBYTE *)text[0];
            e = s;
            while (*e++)
                ;
            text[0] = (ULONG)e;
            waitblit();
            {
                struct bregs r = { .a0 = (ULONG)s };
                BCALL(T_PRINT0, 0, r);
            }
        } while (--lines > 0);
        if (sel)
            goto notext;
        {
            WORD hy = xyxy[1] + (count << 3) + 1;

            boxf(xyxy[0] + 2, hy, xyxy[2] - 2, hy + 8, 0x8001);
        }
        goto notext;
    }
    /* oneline */
    {
        /* d0 d1; text is read as a long (as a pointer GCC allocates the
           registers differently) */
        ULONG tlen = box->tlen, tptr = *(ULONG *)&box->text;
        ULONG ptr, len;                         /* d0 d1 after the EXG */

        ptr = tlen;                             /* no text: d0 = 0, d1 = text (#89) */
        len = tptr;
        if (tlen) {
            ptr = tptr;                         /* EXG d0,d1 */
            len = tlen;
            if (btype == 3) {
                ULONG cx;
                ULONG row;

                text[0] = ptr;
                text[1] = len;
                cx = (ULONG)(LONG)xyxy[0];
                BL_SETW(cx, (UWORD)(xyxy[0] + 11) >> 3);
                row = (ULONG)(LONG)xyxy[1];
                BL_SETW(row, row - 8);
                if (box->knobw >= 0) {
                    BL_SETW(row, row + 8);
                    BL_SETW(cx, cx - (UWORD)len);
                }
                BL_SETW(row, row + 2);
                locate(cx, BL_SWAP(row) >> 3);
                goto pme;
            }
            if (btype != 6 && box->range) {
                /* the value-th of the strings */
                UWORD index = box->value;
                UBYTE *str = (UBYTE *)ptr;
                ULONG n;

                do {
                    n = 0;
                    do
                        n++;
                    while (*str++);
                } while (index-- != 0);
                str -= n;
                len = n;
                BL_SETW(len, len - 1);
                ptr = (ULONG)str;
            }
        }
        text[0] = ptr;                          /* t3 */
        text[1] = len;
    }
    {
        ULONG tl = text[1];                     /* d0: the text length */
        ULONG col = tl;                         /* d0.w: the column */

        if (!tl)
            goto notext;
        if (btype == 2) {
            BL_SETW(col, (UWORD)(xyxy[0] + 11) >> 3);
        } else {
            BL_SETW(col, (UWORD)(((UWORD)(-(WORD)tl << 3) + xyxy[0] + xyxy[2] + 7)) >> 4);
        }
        locate(col, locrow(xyxy[1]));
    }
pme:
    colour((LONG)c3, (LONG)c2);
    waitblit();
    printn((const UBYTE *)text[0], text[1]);
notext:
    if (!c0 && !c1)                             /* TST.l c0 */
        return;
    x = xyxy[0];
    y = xyxy[1];
    x2 = xyxy[2];
    y2 = xyxy[3];
    line(x, y, x2, y, (LONG)c0);
    line(x2, y, x2, y2, (LONG)c1);
    line(x2 - 1, y + 1, x2 - 1, y2, (LONG)c1);
    line(x, y, x, y2, (LONG)c0);
    line(x + 1, y + 1, x + 1, y2, (LONG)c0);
    line(x + 1, y2, x2 - 1, y2, (LONG)c1);
}

/* ------------------------------------------------------- page commands */

static void freepage_(struct page *p);

/**
 * @brief InitPage Page#,BitMap#,NumBoxes: a page of boxes drawn into a
 * bitmap.
 *
 * Frees the page's old boxes, allocates NumBoxes cleared boxes (32 bytes
 * each, MEMF_PUBLIC|MEMF_CLEAR), sets the pens 1,3,2,1 (the help text of
 * SetPagePens says 3,1,2,3) and makes the page the used one.  Errors
 * (always checked): "Unitialised BitMap".
 *
 * Reads a3: the page (!libs{#blitzoslib,$1380} from Page# in d0).
 * @param n  d2: NumBoxes
 * @param bm  a2: the bitmap object (!libs{#bitmaplib,$1281} from BitMap#)
 * @param used  d6: the offset of the used page variable (auto use)
 */
static void BLITZ_ROUTINE initpage_c(ULONG n BL_REG(d2), UWORD *bm BL_REG(a2), ULONG used BL_REG(d6))
{
    struct page *p = PG;

    *(struct page **)(blitz_globals + (WORD)used) = p;      /* auto use */
    if (!*bm)
        BLITZ_RUNERR(mess8);
    freepage_(p);
    p->numboxes = (UWORD)n;
    p->memsize = (ULONG)(LONG)(WORD)n << 5;
    p->memloc = globalloc(p->memsize, MEMF_PUBLIC | MEMF_CLEAR);
    p->c0 = 1;
    p->c1 = 3;
    p->c2 = 2;
    p->c3 = 1;
    p->mybitmap = (UBYTE *)bm;
}
BLITZ_THUNK(_initpage, initpage_c, "d0-d1/a0-a1");

static void freepage_(struct page *p)
{
    if (!p->memsize)
        return;
    globfree(p->memloc, p->memsize);
    p->memsize = 0;
    p->numboxes = 0;
}

/**
 * @brief FreePage Page#: free the boxes of a page.
 *
 * Nothing for an unused page.  The texts of the boxes are not freed
 * (memlib's global memory, freed with the program).
 *
 * Reads a3: the page (!libs{#blitzoslib,$1380} from Page# in d0).
 */
static void BLITZ_ROUTINE freepage_c(void)
{
    freepage_(PG);
}
BLITZ_THUNK(_freepage, freepage_c, "d0-d1/a0-a1");

/**
 * @brief SetPagePens Page#,c0,c1,c2,c3: the pens of a page.
 *
 * Reads a3: the page (!libs{#blitzoslib,$1380} from Page# in d0).
 * @param dark  d1.w: c0, the dark bevel pen
 * @param light  d2.w: c1, the light bevel pen
 * @param mid  d3.w: c2, the background pen
 * @param txt  d4.w: c3, the text pen
 */
static void BLITZ_ROUTINE setpagepens_c(ULONG dark BL_REG(d1), ULONG light BL_REG(d2),
                                        ULONG mid BL_REG(d3), ULONG txt BL_REG(d4))
{
    struct page *p = PG;

    p->c0 = (WORD)dark;
    p->c1 = (WORD)light;
    p->c2 = (WORD)mid;
    p->c3 = (WORD)txt;
}
BLITZ_THUNK(_setpagepens, setpagepens_c, "d0-d1/a0-a1");

/**
 * @brief ToggleBox Box#: invert the bevel of a box (NOT of its colour).
 *
 * Not redrawn.  Errors (always checked): "UnInitialised Page", "Illegal
 * Box# For Page".
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 */
static void BLITZ_ROUTINE togglebox_c(ULONG n BL_REG(d0))
{
    struct box *b = getbox(PG, n);

    b->color = ~b->color;
}
BLITZ_THUNK(_togglebox, togglebox_c, "d0-d1/a0-a1");

/**
 * @brief EnableBox Box#: enable a disabled box (a positive type).
 *
 * Not redrawn.  Errors (always checked): "UnInitialised Page", "Illegal
 * Box# For Page".
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 */
static void BLITZ_ROUTINE enablebox_c(ULONG n BL_REG(d0))
{
    struct box *b = getbox(PG, n);

    if (b->btype < 0)
        b->btype = -b->btype;
}
BLITZ_THUNK(_enablebox, enablebox_c, "d0-d1/a0-a1");

/**
 * @brief DisableBox Box#: disable a box (a negative type).
 *
 * Not redrawn; a disabled box is drawn as a plain c2 box and cannot be
 * selected with the mouse.  Errors (always checked): "UnInitialised
 * Page", "Illegal Box# For Page".
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 */
static void BLITZ_ROUTINE disablebox_c(ULONG n BL_REG(d0))
{
    struct box *b = getbox(PG, n);

    if (b->btype > 0)
        b->btype = -b->btype;
}
BLITZ_THUNK(_disablebox, disablebox_c, "d0-d1/a0-a1");

/**
 * @brief BoxPropX Box#: the horizontal knob position of a prop box.
 *
 * Errors (always checked): "UnInitialised Page", "Illegal Box# For
 * Page".
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @return d0: the knob position as Quick (the word * 2; -1 * 2 for a
 *         fixed axis)
 */
static ULONG BLITZ_ROUTINE boxpropx_c(ULONG n BL_REG(d0))
{
    return (ULONG)(LONG)getbox(PG, n)->propx << 1;
}
BLITZ_THUNK(_boxpropx, boxpropx_c, "d1/a1");

/**
 * @brief BoxPropY Box#: the vertical knob position of a prop box.
 *
 * As BoxPropX.
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @return d0: the knob position as Quick (the word * 2)
 */
static ULONG BLITZ_ROUTINE boxpropy_c(ULONG n BL_REG(d0))
{
    return (ULONG)(LONG)getbox(PG, n)->propy << 1;
}
BLITZ_THUNK(_boxpropy, boxpropy_c, "d1/a1");

/* ----------------------------------------------------------- AddBox */

static struct box *addbox(struct page *p, ULONG n, ULONG x, ULONG y, ULONG w, ULONG h, WORD type)
{
    struct box *b;
    WORD color;

    chkpage(p);
    if ((UWORD)n >= p->numboxes)
        BLITZ_RUNERR(mess2);
    b = BOXAT(p, (WORD)(UWORD)(n << 5));
    if (b->tlen) {
        globfree(b->text, b->tlen);
        b->tlen = 0;
    }
    b->x = (WORD)x;
    b->y = (WORD)y;
    b->w = (WORD)w;
    b->h = (WORD)h;
    color = p->c2;
    if (type & 1)                               /* ASR #1: carry */
        color = ~color;
    type >>= 1;
    b->btype = type;
    b->color = color;
    if (type == 2) {
        /* a string box: spaces */
        UWORD len = (UWORD)w >> 3;
        UBYTE *fill;
        UWORD c;

        if (!len)                               /* EXT.l:BLE err4 */
            BLITZ_RUNERR(mess4);
        b->tlen = len;
        fill = globalloc(len, 0);
        b->text = fill;
        c = (UWORD)(len - 2);
        do
            *fill++ = ' ';
        while (c-- != 0);
        *fill = 0;
    } else if (type == 3) {
        /* a prop: bsize & bval */
        WORD valx = -1, valy = -1;
        WORD knobw = (WORD)w - 4, knobh = (WORD)h - 2;

        if ((WORD)(knobw - 28) >= 0) {          /* CMP #28:BMI px */
            valx = 16384;
            knobw = 12;
        }
        if ((WORD)(knobh - 10) >= 0) {
            valy = 16384;
            knobh = 6;
        }
        b->propx = valx;
        b->propy = valy;
        b->knobw = knobw;
        b->knobh = knobh;
    }
    return b;
}

/**
 * @brief AddBox Page#,Box#,x,y,w,h,type: define a box.
 *
 * Frees the box's old text; type bit 0 = inverted bevel (NOT of the
 * colour c2), type >> 1 = the box type (0 view, 1 button, 2 string, 3
 * prop, 4 shape/display, 5 list, 6 drag).  A string box gets w/8 spaces,
 * a prop box its knob (fixed axis -1 if the track is too short).  Not
 * drawn.  Errors (always checked): "UnInitialised Page", "Illegal Box#
 * For Page", "No Room For String" (a string box narrower than 8 pixels).
 * BUG(orig) (#84): a string box 8..15 pixels wide writes 65536 spaces.
 *
 * Reads a3: the page (!libs{#blitzoslib,$1380} from Page# in d0).
 * @param n  d1.w: Box#
 * @param x  d2.w: x
 * @param y  d3.w: y
 * @param w  d4.w: the width
 * @param h  d5.w: the height
 * @param args  a2: the argument stack, -2(a2) = type
 */
static void BLITZ_ROUTINE addbox_c(ULONG n BL_REG(d1), ULONG x BL_REG(d2), ULONG y BL_REG(d3),
                                   ULONG w BL_REG(d4), ULONG h BL_REG(d5), const UWORD *args BL_REG(a2))
{
    addbox(PG, n, x, y, w, h, (WORD)args[-1]);
}
BLITZ_THUNK(_addbox, addbox_c, "d0-d1/a0-a1");

/**
 * @brief AddBox Page#,Box#,x,y,w,h,type,shape,shape: a shape button.
 *
 * As AddBox, then the box shows shapes (isshape -1): the first shape and
 * the number of shapes (range = second - first + 1); the button's value
 * selects the shape.
 *
 * Reads a3: the page (!libs{#blitzoslib,$1380} from Page# in d0).
 * @param n  d1.w: Box#
 * @param x  d2.w: x
 * @param y  d3.w: y
 * @param w  d4.w: the width
 * @param h  d5.w: the height
 * @param args  a2: the argument stack, -2(a2) = type, -4 the first shape,
 *              -6 the last shape
 */
static void BLITZ_ROUTINE addbox2_c(ULONG n BL_REG(d1), ULONG x BL_REG(d2), ULONG y BL_REG(d3),
                                    ULONG w BL_REG(d4), ULONG h BL_REG(d5), const UWORD *args BL_REG(a2))
{
    struct box *b = addbox(PG, n, x, y, w, h, (WORD)args[-1]);
    WORD s1 = (WORD)args[-2], s2 = (WORD)args[-3];

    b->range = s2 - s1 + 1;
    b->shapeoff = s1;
    b->isshape = -1;
}
BLITZ_THUNK(_addbox2, addbox2_c, "d0-d1/a0-a1");

/* the text of a box: d0 = length, a1 = string (nostrya / strya) */
static void boxtext(struct box *b, ULONG len, const UBYTE *s)
{
    UBYTE *dst;
    UWORD strings;

    if (b->btype == 2) {
        /* strya: the prompt of a string box */
        b->prompt = (WORD)len;
        if ((WORD)b->tlen <= (WORD)len)   /* CMP d0,d1:BLE err4 */
            BLITZ_RUNERR(mess4);
        dst = b->text;
        while (*s)
            *dst++ = *s++;
        return;
    }
    len++;
    b->tlen = len;
    dst = globalloc(len, 0);
    b->text = dst;
    strings = 1;
    if (*s == '|') {
        while (*s)                              /* verbatim */
            *dst++ = *s++;
    } else {
        UBYTE c;

        while ((c = *s++) != 0) {
            if (c == '|') {
                c = 0;
                strings++;
            }
            *dst++ = c;
        }
    }
    *dst = 0;
    if (b->btype - 2 < 0) {               /* CMP #2:BPL stdun */
        b->range = strings;
        b->value = 0;
    }
}

/**
 * @brief AddBox Page#,Box#,x,y,w,h,type,text$: a box with a text.
 *
 * As AddBox, then the text: for a string box the prompt (copied into the
 * spaces; error "No Room For String" if it does not fit); otherwise a
 * copy (memlib globalloc) with "|" as separator of several strings (a
 * text starting with "|" is taken verbatim), view and button boxes get
 * value 0 and range = the number of strings.
 *
 * Reads a3: the page (!libs{#blitzoslib,$1380} from Page# in d0).
 * @param n  d1.w: Box#
 * @param x  d2.w: x
 * @param y  d3.w: y
 * @param w  d4.w: the width
 * @param h  d5.w: the height
 * @param args  a2: the argument stack: the string pointer and its length
 *              are below the type word
 */
static void BLITZ_ROUTINE addbox1_c(ULONG n BL_REG(d1), ULONG x BL_REG(d2), ULONG y BL_REG(d3),
                                    ULONG w BL_REG(d4), ULONG h BL_REG(d5), const UBYTE *args BL_REG(a2))
{
    struct box *b = addbox(PG, n, x, y, w, h, *(const WORD *)(args - 2));

    boxtext(b, *(const ULONG *)(args - 6), *(const UBYTE *const *)(args - 10));
}
BLITZ_THUNK(_addbox1, addbox1_c, "d0-d1/a0-a1");

/**
 * @brief SetBoxTitle Box#,String$: the text of a box.
 *
 * Frees the old text (not of a string box) and sets the new one as
 * AddBox ...,text$.  Not redrawn.  Errors (always checked):
 * "UnInitialised Page", "Illegal Box# For Page", "No Room For String".
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @param s  d1: the string
 * @param lens  a2: the argument stack, -4(a2) = the string's length
 */
static void BLITZ_ROUTINE setboxtitle_c(ULONG n BL_REG(d0), const UBYTE *s BL_REG(d1),
                                        const ULONG *lens BL_REG(a2))
{
    struct box *b = getbox(PG, n);
    ULONG len = lens[-1];

    if (b->btype != 2 && b->tlen)
        globfree(b->text, b->tlen);
    boxtext(b, len, s);
}
BLITZ_THUNK(_setboxtitle, setboxtitle_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- set values */

static void setbox(ULONG n, ULONG v)
{
    struct box *b = getbox(PG, n);
    WORD value = (WORD)v;

    if (value < 0)
        value += b->range;
    if ((UWORD)value >= (UWORD)b->range)
        BLITZ_RUNERR(mess3);
    b->value = value;
}

/**
 * @brief SetBox Box#,Value: the value of a box (the selected string).
 *
 * A negative value counts from the end (+ range).  Not redrawn.  Errors
 * (always checked): "UnInitialised Page", "Illegal Box# For Page",
 * "Value Out of Range" (not 0 <= value < range).
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @param v  d1.w: Value
 */
static void BLITZ_ROUTINE setbox_c(ULONG n BL_REG(d0), ULONG v BL_REG(d1))
{
    setbox(n, v);
}
BLITZ_THUNK(_setbox, setbox_c, "d0-d1/a0-a1");

/**
 * @brief ChangeBox Page#,Box#,Value: SetBox for the box of a page.
 *
 * Reads a3: the page (!libs{#blitzoslib,$1380} from Page# in d0).
 * @param n  d1.w: Box#
 * @param v  d2.w: Value
 */
static void BLITZ_ROUTINE changebox_c(ULONG n BL_REG(d1), ULONG v BL_REG(d2))
{
    setbox(n, v);
}
BLITZ_THUNK(_changebox, changebox_c, "d0-d1/a0-a1");

/**
 * @brief SetBox Box#,xprop,yprop: the knob position of a prop box.
 *
 * Quick values: (q-1 if q<>0).w >> 1; a fixed axis (-1) stays.  Not
 * redrawn.  Errors (always checked): "UnInitialised Page", "Illegal Box#
 * For Page", "Not a Prop Box".
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @param qx  d1: xprop (Quick)
 * @param qy  d2: yprop (Quick)
 */
static void BLITZ_ROUTINE setbox2_c(ULONG n BL_REG(d0), ULONG qx BL_REG(d1), ULONG qy BL_REG(d2))
{
    struct box *b = getbox(PG, n);

    if (b->btype != 3)
        BLITZ_RUNERR(mess7);
    if (qx)
        qx--;
    if (qy)
        qy--;
    if (b->propx >= 0)
        b->propx = (UWORD)qx >> 1;
    if (b->propy >= 0)
        b->propy = (UWORD)qy >> 1;
}
BLITZ_THUNK(_setbox2, setbox2_c, "d0-d1/a0-a1");

/**
 * @brief SetBoxColor Box#,color: the background colour of a box.
 *
 * Keeps an inverted bevel (NOT), then draws the box.  Errors (always
 * checked): "UnInitialised Page", "Illegal Box# For Page".
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @param c  d1.w: the colour
 */
static void BLITZ_ROUTINE setboxcolor_c(ULONG n BL_REG(d0), ULONG c BL_REG(d1))
{
    struct page *p = PG;
    struct box *b = getbox(p, n);

    if (b->color < 0)
        c = ~c;
    b->color = (WORD)c;
    drawbox(p, b);
}
BLITZ_THUNK(_setboxcolor, setboxcolor_c, "d0-d1/a0-a1");

/**
 * @brief SetBoxString Box#,String$: the answer of a string box.
 *
 * Written after the prompt, padded with spaces.  Not redrawn.  Errors
 * (always checked): "UnInitialised Page", "Illegal Box# For Page", "No
 * Room For String".
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @param s  d1: the string
 * @param lens  a2: the argument stack, -4(a2) = the string's length
 */
static void BLITZ_ROUTINE setboxstring_c(ULONG n BL_REG(d0), const UBYTE *s BL_REG(d1),
                                         const ULONG *lens BL_REG(a2))
{
    struct box *b = getbox(PG, n);
    ULONG len = lens[-1];
    ULONG pad = b->tlen;
    UBYTE *dst = b->text;

    BL_SETW(pad, pad - b->prompt);
    BL_SETW(pad, pad - len);
    if ((WORD)pad <= 0)
        BLITZ_RUNERR(mess4);
    dst += b->prompt;
    BL_SETW(len, len - 1);
    if ((WORD)len >= 0) {
        UWORD c = (UWORD)len;
        do
            *dst++ = *s++;
        while (c-- != 0);
    }
    BL_SETW(pad, pad - 2);
    if ((WORD)pad >= 0) {
        UWORD c = (UWORD)pad;
        do
            *dst++ = ' ';
        while (c-- != 0);
    }
    *dst = 0;
}
BLITZ_THUNK(_setboxstring, setboxstring_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- functions */

/**
 * @brief BoxValue(Box#): the value of a box of the used page.
 *
 * MOVE value,d0: a word function, the upper word of d0 is the caller's.
 * Errors (always checked): "UnInitialised Page", "Illegal Box# For
 * Page".
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @return d0.w: the value
 */
static ULONG BLITZ_ROUTINE boxvalue_c(ULONG n BL_REG(d0))
{
    return (n & 0xffff0000UL) | (UWORD)getbox(PG, n)->value;
}
BLITZ_THUNK(_boxvalue, boxvalue_c, "d1/a1");

/**
 * @brief BoxValue(Page#,Box#): the value of a box of a page.
 *
 * As BoxValue(Box#) (MOVE d1,d0 first).
 *
 * Reads a3: the page (!libs{#blitzoslib,$1380} from Page# in d0).
 * @param callerd0  d0: Page# (its upper word stays)
 * @param n  d1.w: Box#
 * @return d0.w: the value
 */
static ULONG BLITZ_ROUTINE boxvalue2_c(ULONG callerd0 BL_REG(d0), ULONG n BL_REG(d1))
{
    BL_SETW(callerd0, n);                       /* MOVE d1,d0 */
    return (callerd0 & 0xffff0000UL) | (UWORD)getbox(PG, callerd0)->value;
}
BLITZ_THUNK(_boxvalue2, boxvalue2_c, "d1/a1");

/* BoxString: page in a1, result at a3 (no 0 written; a3 = the end) */
static ULONG boxstring(struct page *p, ULONG n)
{
    struct box *b = getbox(p, n);
    WORD prompt = b->prompt;
    ULONG len = b->tlen;
    const UBYTE *answer = b->text + prompt;
    const UBYTE *end;
    UBYTE *dst = blitz_string;

    BL_SETW(len, len - prompt);
    end = answer + (WORD)len - 1;
    do                                          /* crop */
        BL_SETW(len, len - 1);
    while (*--end == ' ');
    if ((UWORD)len) {
        UWORD c = (UWORD)len - 1;
        do
            *dst++ = *answer++;
        while (c-- != 0);
    }
    blitz_string = dst;
    return len;
}

/**
 * @brief BoxString(Box#): the answer of a string box, without trailing
 * spaces.
 *
 * The answer after the prompt goes to a3; no 0 is written, a3 = the end
 * (not a bug, #86: the compiler appends the 0 itself).  Errors (always
 * checked): "UnInitialised Page", "Illegal Box# For Page".
 * BUG(orig) (#85): the trailing spaces are stripped backwards without a
 * limit: an all-space answer runs into the prompt / memory before the
 * text (d0 0: nothing, negative: a huge copy).
 *
 * @param n  d0.w: Box#
 * @param p  a1: the used page (!libs{#blitzoslib,$1102})
 * @return d0: the length of the string at a3
 */
static ULONG BLITZ_ROUTINE boxstring_c(ULONG n BL_REG(d0), struct page *p BL_REG(a1))
{
    return boxstring(p, n);
}
BLITZ_THUNK(_boxstring, boxstring_c, "d1");

/**
 * @brief BoxString(Page#,Box#): BoxString of a box of a page.
 *
 * @param callerd0  d0: Page# (its upper word stays)
 * @param n  d1.w: Box#
 * @param p  a1: the page (!libs{#blitzoslib,$1180} from Page#)
 * @return d0: the length of the string at a3
 */
static ULONG BLITZ_ROUTINE boxstring2_c(ULONG callerd0 BL_REG(d0), ULONG n BL_REG(d1),
                                        struct page *p BL_REG(a1))
{
    BL_SETW(callerd0, n);
    return boxstring(p, callerd0);
}
BLITZ_THUNK(_boxstring2, boxstring2_c, "d1");

/**
 * @brief BoxSelected: the selected box of the used page.
 *
 * (selected - memloc) >> 5 (LSR.w).  Errors (always checked):
 * "UnInitialised Page".
 *
 * Reads a3: the used page.
 * @return d0.w: the Box#, -1 for none
 */
static ULONG BLITZ_ROUTINE boxselected_c(void)
{
    struct page *p = PG;
    ULONG offset;

    chkpage(p);
    offset = (ULONG)p->selected;
    if (!offset)
        return (ULONG)-1;
    offset -= (ULONG)p->memloc;
    if ((LONG)offset < 0)
        return (ULONG)-1;
    BL_SETW(offset, (UWORD)offset >> 5);
    return offset;
}
BLITZ_THUNK(_boxselected, boxselected_c, "d1/a0-a1");

/* ---------------------------------------------------------- drawing */

/**
 * @brief RedrawPage: draw all boxes of the used page.
 *
 * Draws every box with a width.  Errors (always checked): "UnInitialised
 * Page".
 *
 * Reads a3: the used page.
 */
static void BLITZ_ROUTINE redrawpage_c(void)
{
    struct page *p = PG;
    struct box *b;
    WORD n;

    chkpage(p);
    b = p->memloc;
    n = (WORD)(p->numboxes - 1);
    if (n < 0)
        return;
    do {
        if (b->w)
            drawbox(p, b);
        b++;
    } while (n-- != 0);
}
BLITZ_THUNK(_redrawpage, redrawpage_c, "d0-d1/a0-a1");

/**
 * @brief RedrawBox Box#: draw a box of the used page.
 *
 * Errors (always checked): "UnInitialised Page", "Illegal Box# For
 * Page".
 * BUG(orig) (#89): a box with tlen 0 but an old text pointer prints from
 * address 0 with the pointer as length (in drawbox).
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 */
static void BLITZ_ROUTINE redrawbox_c(ULONG n BL_REG(d0))
{
    struct page *p = PG;

    drawbox(p, getbox(p, n));
}
BLITZ_THUNK(_redrawbox, redrawbox_c, "d0-d1/a0-a1");

/* BoxShow: a view box shows part of a bitmap (Scroll) */
static void dobs(struct page *p, struct box *b)
{
    WORD px = b->posx, py = b->posy;
    WORD dx = b->x + 4, dy = b->y + 2;
    WORD w = b->w - 8, h = b->h - 4;
    UBYTE *src = b->showbp;
    WORD mw, mh;

    if (px < 0)
        px = 0;
    if (py < 0)
        py = 0;
    mw = *(WORD *)(src + 56) - w;
    mh = *(WORD *)(src + 58) - h;
    if ((UWORD)mw < (UWORD)px)                  /* CMP d0,d6:BCC kox */
        px = mw;
    if ((UWORD)mh < (UWORD)py)
        py = mh;
    b->posx = px;
    b->posy = py;
    xyxy[0] = h;                                /* EXG d4,d5:MOVEM d4-d5,xyxy */
    xyxy[1] = w;
    scroll((LONG)px, (LONG)py, (LONG)dx, (LONG)dy, w, h, src, p->mybitmap);
}

/**
 * @brief BoxShow Box#,Bitmap#,x,y: show part of a bitmap in a view box.
 *
 * Scrolls the bitmap from x,y (clipped to the bitmap) into the box.
 * Errors (always checked): "UnInitialised Page", "Illegal Box# For
 * Page", "Illegal Box Type" (not a view box).
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @param bm  a2: the bitmap object (!libs{#bitmaplib,$1281} from Bitmap#)
 * @param x  d2.w: x in the bitmap
 * @param y  d3.w: y in the bitmap
 */
static void BLITZ_ROUTINE boxshow_c(ULONG n BL_REG(d0), UBYTE *bm BL_REG(a2),
                                    ULONG x BL_REG(d2), ULONG y BL_REG(d3))
{
    struct page *p = PG;
    struct box *b = getbox(p, n);

    if (b->btype)
        BLITZ_RUNERR(mess5);
    b->showbp = bm;
    b->posx = (WORD)x;
    b->posy = (WORD)y;
    dobs(p, b);
}
BLITZ_THUNK(_boxshow, boxshow_c, "d0-d1/a0-a1");

/**
 * @brief BoxShow Box#,dx,dy: move the view of a view box.
 *
 * As BoxShow Box#,Bitmap#,x,y with the position moved by dx,dy.  Errors
 * as BoxShow.
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @param dx  d1.w: the x offset
 * @param dy  d2.w: the y offset
 */
static void BLITZ_ROUTINE boxshow2_c(ULONG n BL_REG(d0), ULONG dx BL_REG(d1), ULONG dy BL_REG(d2))
{
    struct page *p = PG;
    struct box *b = getbox(p, n);

    if (b->btype)
        BLITZ_RUNERR(mess5);
    b->posx += (WORD)dx;
    b->posy += (WORD)dy;
    dobs(p, b);
}
BLITZ_THUNK(_boxshow2, boxshow2_c, "d0-d1/a0-a1");

static UWORD cx, cy;

/* BoxPrint: d1 = string, -4(a2) = length (overwritten with Scroll's size) */
static void boxprint(struct page *p, ULONG n, const UBYTE *s, ULONG *lens)
{
    struct box *b;
    ULONG len;

    chkpage(p);
    if ((UWORD)n >= p->numboxes)
        BLITZ_RUNERR(mess2);
    b = BOXAT(p, (WORD)(UWORD)(n << 5));
    if (b->btype)
        BLITZ_RUNERR(mess5);
    len = lens[-1];
    if ((UWORD)b->h < 16) {
        /* doleft: scroll the line left, print at the right */
        WORD x = b->x, y = b->y, w = b->w;
        UWORD col = (UWORD)(x + 11) >> 3;
        UWORD width, lenpix = (UWORD)(len << 3);
        UWORD *wh = (UWORD *)lens - 2;

        cx = col;
        cy = y + 2;
        width = (UWORD)(x - (WORD)(col << 3) + w - 1) >> 3;
        cx += width;
        width <<= 3;
        wh[0] = 8;                              /* -4(a2) = h */
        wh[1] = width - lenpix;                 /* -2(a2) = w */
        {
            UWORD wv = wh[1];
            scroll((UWORD)((col << 3) + lenpix), (UWORD)cy, (UWORD)(col << 3), (UWORD)cy,
                   wv, 8, p->mybitmap, p->mybitmap);
        }
        waitblit();
        locate((ULONG)(LONG)(WORD)(cx - (UWORD)len), BL_SWAP((ULONG)(LONG)(WORD)cy) >> 3);
        printn(s, len);
        return;
    }
    {
        /* doup: scroll the box up 8 pixels, print in the last line */
        WORD x = b->x, y = b->y, w = b->w, h = b->h;
        UWORD *wh = (UWORD *)lens - 2;

        wh[0] = h - 12;
        wh[1] = w - 8;
        scroll((LONG)(WORD)(x + 4), (LONG)(WORD)(y + 10), (LONG)(WORD)(x + 4),
               (LONG)(WORD)(y + 2), w - 8, h - 12, p->mybitmap, p->mybitmap);
        boxf(x + 2, y + h - 10, x + w - 3, y + h - 2, (LONG)p->c2);
        waitblit();
        locate((ULONG)(LONG)(WORD)((UWORD)(x + 11) >> 3),
               BL_SWAP((ULONG)(LONG)(WORD)(y + h - 10)) >> 3);
        printn(s, len);
    }
}

/**
 * @brief BoxPrint Box#,String$: print a line into a view box.
 *
 * A box lower than 16 pixels scrolls its line left and prints at the
 * right; otherwise the box scrolls up 8 pixels and the string goes into
 * the last line.  Errors (always checked): "UnInitialised Page",
 * "Illegal Box# For Page", "Illegal Box Type" (not a view box).
 * Not a bug (#88): overwrites the string length at -4(a2) with Scroll's
 * size.
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @param s  d1: the string
 * @param lens  a2: the argument stack, -4(a2) = the string's length
 */
static void BLITZ_ROUTINE boxprint_c(ULONG n BL_REG(d0), const UBYTE *s BL_REG(d1),
                                     ULONG *lens BL_REG(a2))
{
    boxprint(PG, n, s, lens);
}
BLITZ_THUNK(_boxprint, boxprint_c, "d0-d1/a0-a1");

static ULONG linelen[2];                        /* length; a2 points behind */

/**
 * @brief BoxPrint Box#,Bank#,Line: print a line of a bank into a view box.
 *
 * The line (ended by LF or 0) is printed as BoxPrint Box#,String$.
 * Errors (always checked): "Unitialised Bank", then those of BoxPrint.
 * BUG(orig) (#87): lines over 0 bytes are skipped too, and the line's LF
 * in the bank is replaced by 0.
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 * @param bank  a2: the bank object (!libs{#banklib,$1281} from Bank#)
 * @param ln  d2.w: Line (from 0)
 */
static void BLITZ_ROUTINE boxprint2_c(ULONG n BL_REG(d0), UBYTE **bank BL_REG(a2), ULONG ln BL_REG(d2))
{
    UBYTE *pos = *bank, *start;
    UWORD skip = (UWORD)ln;
    ULONG len;

    if (!pos)
        BLITZ_RUNERR(mess6);
    if ((WORD)--skip >= 0) {
        do {
            UBYTE c;
            do
                c = *pos++;
            while (c && c != 10);
        } while (skip-- != 0);
    }
    start = pos;
    len = (ULONG)-1;
    for (;;) {
        UBYTE c = *pos++;

        len++;
        if (!c)
            break;
        if (c == 10) {
            pos[-1] = 0;
            break;
        }
    }
    linelen[0] = len;
    boxprint(PG, n, start, linelen + 1);
}
BLITZ_THUNK(_boxprint2, boxprint2_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- selecting */

static WORD inprop, inedit, indrag;
static WORD edxyc[3];                           /* x, y, cursor */

static WORD status, delay, button, mo, jx, jy, jb, mx, my, mb, ox, oy;
static constexpr WORD dcon = 6;                 /* delay constant */
static constexpr WORD pspd = 1024;              /* prop speed */

/**
 * @brief SelectBox Box#: select a box (and draw it and the old one).
 *
 * Errors (always checked): "UnInitialised Page".
 * BUG(orig) (#81): the offset is computed as a word (ASL #5).
 * BUG(orig) (#82): Box# is not range checked: negative or >= 1024 only
 * deselect.
 *
 * Reads a3: the used page.
 * @param n  d0.w: Box#
 */
static void BLITZ_ROUTINE selectbox_c(ULONG n BL_REG(d0))
{
    struct page *p = PG;
    struct box *old;
    WORD off;

    chkpage(p);
    old = p->selected;
    p->selected = 0;
    off = (WORD)(UWORD)(n << 5);
    if (off >= 0) {
        p->selected = BOXAT(p, off);
        drawbox(p, p->selected);
    }
    if (old)
        drawbox(p, old);
}
BLITZ_THUNK(_selectbox, selectbox_c, "d0-d1/a0-a1");

/* find the box at (x, y): enabled, x <= px < x+w, y <= py < y+h */
static int inbox(struct box *b, WORD px, WORD py)
{
    WORD dx, dy;

    if (b->btype <= 0)
        return 0;
    dx = b->x - px;
    if (dx > 0)
        return 0;
    dy = b->y - py;
    if (dy > 0)
        return 0;
    dx = -dx;
    dy = -dy;
    if (b->w <= dx)
        return 0;
    return b->h > dy;
}

/* SelectBox 0,x,y: -1 = found (selected), 0 = none */
static LONG selectbox3(struct page *p, WORD x, WORD y)
{
    struct box *old, *b;
    UWORD n;

    chkpage(p);
    old = p->selected;
    b = p->memloc;
    n = p->numboxes - 1;
    do {
        if (inbox(b, x, y)) {
            p->selected = b;
            drawbox(p, b);
            if (old)
                drawbox(p, old);
            return -1;
        }
        b++;
    } while (n-- != 0);
    return 0;
}

/**
 * @brief SelectBox 0,x,y: select the enabled box at x,y.
 *
 * Draws the new box and the old one.  Errors (always checked):
 * "UnInitialised Page".
 * Not a bug (#93): redraws the old box also when it is the new one.
 *
 * Reads a3: the used page.
 * @param x  d1.w: x
 * @param y  d2.w: y
 * @return d0: -1 = a box was found and selected, 0 = none (a statement:
 *         the value is used only by BlitzEvent)
 */
static ULONG BLITZ_ROUTINE selectbox3_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2))
{
    return selectbox3(PG, (WORD)x, (WORD)y);
}
BLITZ_THUNK(_selectbox3, selectbox3_c, "d1/a0-a1");

/* SelectBox dx,dy: the nearest box in that direction */
static void selectbox2(struct page *p, WORD dx, WORD dy)
{
    struct box *cur, *b, *best;
    WORD edgex, edgey;
    UWORD bestdist, n;

    chkpage(p);
    cur = p->selected;
    if (!cur)
        return;
    best = cur;
    b = p->memloc;
    edgex = cur->x;
    edgey = cur->y;
    if (dx > 0)
        edgex += cur->w;
    if (dy > 0)
        edgey += cur->h - 1;
    bestdist = 8192;
    n = p->numboxes - 1;
    do {
        WORD distx, disty;

        if (b == cur || b->btype <= 0)
            goto mis;
        distx = b->x - edgex;
        if (dx < 0)
            distx = -distx;
        if (distx < 0) {
            if (dx)
                goto mis;
            distx = -distx;
        }
        disty = b->y - edgey;
        if (dy < 0)
            disty = -disty;
        if (disty < 0) {
            if (dy)
                goto mis;
            disty = -disty;
        }
        if (dx) {
            if (!distx)
                goto mis;
            disty <<= 5;
        }
        if (dy) {
            if (!disty)
                goto mis;
            distx <<= 5;
        }
        disty += distx;
        if (bestdist < (UWORD)disty)            /* CMP d7,d0:BCS mis */
            goto mis;
        bestdist = (UWORD)disty;
        best = b;
mis:
        b++;
    } while (n-- != 0);
    if (best == cur)
        return;
    p->selected = best;
    drawbox(p, cur);
    drawbox(p, p->selected);
}

/**
 * @brief SelectBox x+,y+: select the nearest box in a direction.
 *
 * From the selected box, the nearest enabled box in direction dx,dy
 * (signs); nothing without a selected box.  Errors (always checked):
 * "UnInitialised Page".
 *
 * Reads a3: the used page.
 * @param dx  d0.w: the x direction (<0, 0, >0)
 * @param dy  d1.w: the y direction (<0, 0, >0)
 */
static void BLITZ_ROUTINE selectbox2_c(ULONG dx BL_REG(d0), ULONG dy BL_REG(d1))
{
    selectbox2(PG, (WORD)dx, (WORD)dy);
}
BLITZ_THUNK(_selectbox2, selectbox2_c, "d0-d1/a0-a1");

/**
 * @brief DeSelectBox: draw the selected box as not pressed.
 *
 * Restores an inverted bevel and ends editing, prop and drag.  Errors
 * (always checked): "UnInitialised Page".
 * BUG(orig) (#94): does not clear `selected`.
 *
 * Reads a3: the used page.
 */
static void BLITZ_ROUTINE deselectbox_c(void)
{
    struct page *p = PG;
    struct box *b;

    chkpage(p);
    b = p->selected;
    if (!b)
        return;
    if (b->color < 0)
        b->color = ~b->color;
    drawbox(p, b);
    inprop = 0;
    inedit = 0;
    indrag = 0;
}
BLITZ_THUNK(_deselectbox, deselectbox_c, "d0-d1/a0-a1");

/* the edit cursor: NOT 8 bytes in plane 0 */
static void docursor(struct page *p)
{
    const struct bb_bitmap *bm = (const struct bb_bitmap *)p->mybitmap;
    UWORD bpr = bm->bpr;
    UBYTE *pos = bm->planes[0];
    int i;

    pos += (WORD)((UWORD)edxyc[0] >> 3);
    pos += (ULONG)(UWORD)edxyc[1] * bpr;        /* MULU */
    pos += edxyc[2];
    for (i = 0; i < 8; i++) {
        *pos = ~*pos;
        if (i < 7)
            pos += (WORD)bpr;
    }
}

/* show the text from the prompt on, at edxyc (the bitmap's cursor) */
static void edstring(struct page *p, struct box *b)
{
    struct bb_bitmap *bm = (struct bb_bitmap *)p->mybitmap;
    ULONG old = *(ULONG *)&bm->cursx;
    WORD prompt = b->prompt;
    ULONG len = b->tlen;

    *(ULONG *)&bm->cursx = *(ULONG *)edxyc;
    BL_SETW(len, len - prompt);
    BL_SETW(len, len - 1);
    printn(b->text + prompt, len);
    *(ULONG *)&bm->cursx = old;
}

static void activateselected(struct page *p)
{
    struct box *b;
    WORD type;

    chkpage(p);
    b = p->selected;
    if (!b)
        return;
    type = b->btype;
    if (type == 1 || type == 5) {
        drawbox(p, b);                          /* button up now cycles */
    } else if (type == 2) {
        edxyc[0] = (WORD)((((UWORD)(b->x + 11) >> 3) + b->prompt) << 3);
        edxyc[1] = b->y + 2;
        edxyc[2] = 0;
        inedit = -1;
        drawbox(p, b);
        docursor(p);
    } else if (type == 3) {
        inprop = -1;
        drawbox(p, b);
    } else if (type == 6) {
        indrag = -1;
        drawbox(p, b);
    }
}

/**
 * @brief ActivateSelectedBox: activate the selected box.
 *
 * Button and list: redraw; string box: start editing (cursor after the
 * prompt); prop: start moving the knob; drag: start dragging.  Errors
 * (always checked): "UnInitialised Page".
 *
 * Reads a3: the used page.
 */
static void BLITZ_ROUTINE activateselected_c(void)
{
    activateselected(PG);
}
BLITZ_THUNK(_activateselected, activateselected_c, "d0-d1/a0-a1");

/**
 * @brief SpotBox: the box that the mouse is over.
 *
 * The first enabled box under (mx,my) of the last BlitzEvent.
 * BUG(orig) (#83): no page check; with 0 boxes it scans 65536.
 *
 * Reads a3: the used page.
 * @return d0.w: the Box#, -1 for none (the upper word is mx's sign)
 */
static ULONG BLITZ_ROUTINE spotbox_c(void)
{
    struct page *p = PG;
    struct box *b = p->memloc;
    UWORD n = p->numboxes - 1;
    ULONG index = 0;

    do {
        if (inbox(b, mx, my))
            return ((ULONG)(LONG)mx & 0xffff0000UL) | (UWORD)index; /* MOVE d6,d0 */
        index++;
        b++;
    } while (n-- != 0);
    return (ULONG)-1;
}
BLITZ_THUNK(_spotbox, spotbox_c, "d1/a0-a1");

/**
 * @brief MouseEnabled: whether BlitzEvent follows the mouse.
 *
 * MOVE mo,d0 (low word): set when the mouse moved fast enough, cleared
 * by a direction of the joystick or the cursor keys; 0 while dragging.
 *
 * @param callerd0  d0: the caller's register (its upper word stays)
 * @return d0.w: -1 = mouse, 0 = joystick/keys
 */
static ULONG BLITZ_ROUTINE mouseenabled_c(ULONG callerd0 BL_REG(d0))
{
    if (indrag)
        return 0;
    BL_SETW(callerd0, mo);
    return callerd0;
}
BLITZ_THUNK(_mouseenabled, mouseenabled_c, "d1/a0-a1");

/* ------------------------------------------------------- BlitzEvent */

static const WORD delt[] = { 0, 0, 0, -1, -1, 0, 0, 1, 1, 0 };

/* status 1=up 3=down 2=left 4=right */
static LONG blitzevent(struct page *p)
{
    WORD dir;
    struct box *box;

    jx = (WORD)BCALL0(T_JOYX, 1);
    jy = (WORD)BCALL0(T_JOYY, 1);
    jb = (WORD)(BCALL0(T_JOYB, 1) & ~2UL);
    mb = (WORD)BCALL0(T_JOYB, 0);
    if (!mo) {
        LONG sp = (LONG)BCALL0(T_MOUSEXSPEED, 0);

        if (sp < 0)
            sp = -sp;
        if (sp - 0x50000 < 0)                   /* CMP.l:BMI */
            goto checkdir;
        mo = -1;
    }
    ox = mx;                                    /* yomo */
    oy = my;
    mx = (WORD)(BCALL0(T_MOUSEX, 0) >> 16);
    my = (WORD)(BCALL0(T_MOUSEY, 0) >> 16);
checkdir:
    dir = jx;
    if (dir)
        dir += 3;
    else {
        dir = jy;
        if (dir)
            dir += 2;
    }
    if (rawstatus(0x4c))
        dir = 1;
    if (rawstatus(0x4d))
        dir = 3;
    if (rawstatus(0x4e))
        dir = 4;
    if (rawstatus(0x4f))
        dir = 2;
    if (rawstatus(0x64) && rawstatus(0x66))
        mb |= 1;
    if (rawstatus(0x67) && rawstatus(0x65))
        mb |= 2;
    if (!dir) {
        status = 0;
        goto dobutton;
    }
    /* changedir */
    mo = 0;
    if (status == dir) {
        if (--delay >= 0)
            goto dobutton;
        delay = dcon;
    } else {
        delay = dcon + dcon;
        status = dir;
    }
    /* dodir */
    if (inedit)
        goto movecursor;
    if (button)
        goto dobutton;
    if (status & 1) {
        box = p->selected;
        if (box->btype == 5)
            goto movelist;
    }
    {
        WORD i = (WORD)(status << 2) / 2;
        selectbox2(p, delt[i], delt[i + 1]);
    }
    goto dobutton;

movelist:
    {
        WORD v = box->value + status - 2, r = box->range;

        if (v < 0)
            v = 0;
        if (v - r >= 0)                         /* CMP d1,d0:BMI fop */
            v = r - 1;
        box->value = v;
        drawbox(p, box);
        return 0;
    }

movecursor:
    {
        WORD cursor, last;
        UBYTE c;

        docursor(p);
        cursor = edxyc[2];
        box = p->selected;
        last = (WORD)box->tlen - box->prompt - 1;
        if (status == 2 && --cursor < 0)
            cursor = 0;
        if (status == 4) {
            cursor++;
            if (cursor - last >= 0)             /* CMP d1,d0:BMI nri */
                cursor--;
        }
        edxyc[2] = cursor;
        if (status == 1 || status == 3) {
            UBYTE *chr = box->text + box->prompt + cursor;

            c = (UBYTE)((status == 1 ? 1 : -1) + *chr);
            if (c == 31)
                c = 90;
            if (c == 33)
                c = 48;
            if ((BYTE)(c - 48) < 0 || (BYTE)(c - 91) >= 0)  /* BMI after CMP.b */
                c = 32;
            *chr = c;
            edstring(p, box);
        }
        docursor(p);
        return 0;
    }

dobutton:
    {
        UWORD buttons = rawstatus(0x44) ? 0xffff : 0;

        buttons |= jb | mb;
        if (buttons)
            goto down;
        if (!button)
            goto noselect;
        button = 0;
        /* upbutton */
        if (inedit)
            goto noselect;
        inprop = 0;
        indrag = 0;
        box = p->selected;
        if (!box || box->color >= 0)
            goto noselect;
        box->color = ~box->color;
        if (box->btype == 1) {
            UWORD v = box->value + 1;

            if (v >= (UWORD)box->range)
                v = 0;
            box->value = v;
        }
        drawbox(p, box);
        return -1;

down:
        if (inprop)
            goto moveprop;
        if (indrag)
            goto movedrag;
        if (button)
            goto noselect;
        button = buttons;
        if (inedit) {
            inedit = 0;
            goto noselect;
        }
        if (mo && !selectbox3(p, mx, my))
            return 0;
        box = p->selected;
        if (!box)
            goto noselect;
        box->color = ~box->color;
        activateselected(p);
        return 1;
    }

noselect:
    {
        WORD key, last, cursor, k;
        UBYTE *answer;

        if (!inedit)
            return 0;
        key = (WORD)BCALL0(T_BLITZKEY, 0);
        if (key < 0)
            return 0;
        docursor(p);
        box = p->selected;
        answer = box->text + box->prompt;
        last = (WORD)box->tlen - box->prompt - 1;
        cursor = edxyc[2];
        if ((UWORD)key >= 32 && (UWORD)key <= 126) {
            if ((UWORD)key >= 97)
                key -= 32;
            k = last;
            while (--k != cursor)
                answer[k] = answer[k - 1];
            answer[cursor] = (UBYTE)key;
            cursor++;
            if (cursor - last >= 0)
                cursor--;
            edxyc[2] = cursor;
        } else {
            if (key != 127) {
                if (key != 8 || --cursor < 0)
                    return 0;
                edxyc[2] = cursor;
            }
            while (++cursor != last)
                answer[cursor - 1] = answer[cursor];
            answer[cursor - 1] = ' ';
        }
        edstring(p, box);
        docursor(p);
        return 0;
    }

movedrag:
    box = p->selected;
    box->dx = mx - ox;
    box->dy = my - oy;
    mx = ox;
    my = oy;
    {
        struct bregs r = { .d0 = (LONG)ox, .d1 = (LONG)oy };
        BCALL(T_POSITIONMOUSE, 0, r);
    }
    return 2;

moveprop:
    {
        WORD valx, valy;

        box = p->selected;
        valx = box->propx;
        valy = box->propy;
        if (mo) {
            WORD trackw = box->w, trackh = box->h;
            WORD knobw = box->knobw, knobh = box->knobh;

            if (valx >= 0) {
                valx = mx;
                trackw = trackw - 4 - knobw;
                valx = valx - box->x - 2;
                knobw = (UWORD)knobw >> 1;
                valx -= knobw;
                if (valx < 0)
                    valx = 0;
                valx = (UWORD)valx >> 1;
                valx = divs_sat((ULONG)(UWORD)valx << 16, trackw);
            }
            if (valy >= 0) {
                valy = my;
                trackh = trackh - 2 - knobh;
                valy = valy - box->y - 2;
                knobh = (UWORD)knobh >> 1;
                valy -= knobh;
                if (valy < 0)
                    valy = 0;
                valy = (UWORD)valy >> 1;
                valy = divs_sat((ULONG)(UWORD)valy << 16, trackh);
            }
        } else {
            WORD stat = status, speed = pspd;

            if (valx >= 0) {
                if (stat == 2 && (valx -= speed) < 0)
                    valx = 0;
                if (stat == 4 && (valx += speed) < 0)
                    valx = 32767;
            }
            if (valy >= 0) {
                if (stat == 1 && (valy -= speed) < 0)
                    valy = 0;
                if (stat == 3 && (valy += speed) < 0)
                    valy = 32767;
            }
        }
        box->propx = valx;
        box->propy = valy;
        drawbox(p, box);
        return 2;
    }
}

/**
 * @brief BlitzEvent: handle the user input of the used page.
 *
 * Reads joystick, mouse, cursor keys (Return as button, left Alt + left
 * Amiga and right Amiga + right Alt as the mouse buttons) and the
 * keyboard (BlitzKey): moves the selection, edits string boxes, moves
 * list entries, prop knobs and drag boxes, presses and releases buttons.
 * BUG(orig) (#90): with cursor up/down it reads btype of `selected` also
 * when nothing is selected.
 * BUG(orig) (#91): Backspace at position 0 leaves the cursor off.
 * Not a bug (#92): the character cycle of the cursor keys uses BMI after
 * CMP.b.
 *
 * Reads a3: the used page.
 * @return d0: -1 = a button was released (clicked), 1 = a box was
 *         activated, 2 = a prop or drag box moved, 0 = nothing
 */
static ULONG BLITZ_ROUTINE blitzevent_c(void)
{
    return (ULONG)blitzevent(PG);
}
BLITZ_THUNK(_blitzevent, blitzevent_c, "d1/a0-a1");
