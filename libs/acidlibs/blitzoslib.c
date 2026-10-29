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
 *    offsets); SelectBox Box# is not range checked (negative or >= 1024:
 *    deselect only); SpotBox has no page check, with 0 boxes it scans
 *    65536.
 *  - AddBox with a string box 8..15 pixels wide: tlen 1, the space fill
 *    (DBRA of tlen-2) writes 65536 spaces.
 *  - BoxString strips trailing spaces backwards without a limit: an
 *    all-space answer runs into the prompt / memory before the text (d0
 *    0: nothing, negative: a huge copy).
 *  - BoxPrint Bank#,Line: skips lines over 0 bytes too, and replaces the
 *    line's LF in the bank by 0.
 *  - oneline: a box with tlen 0 but an old text pointer prints from
 *    address 0 with the pointer as length (fresh boxes have text 0).
 *  - BlitzEvent: with cursor up/down it reads btype of `selected` also
 *    when nothing is selected; Backspace at position 0 leaves the cursor
 *    off.
 *  - DeSelectBox does not clear `selected`.
 * Kept from the original, not a bug (no visible effect):
 *  - BoxString writes no 0, a3 = the end (the compiler appends
 *    clr.b (a3)+ itself where it passes a string on, blitz2.s:8767
 *    sbtolong).
 *  - BoxPrint overwrites the string length at -4(a2) with Scroll's size
 *    (that argument area belongs to this call only and is dropped by the
 *    compiler unread).
 *  - BlitzEvent: the cursor key character cycle uses BMI after CMP.b (N
 *    flag - for all 256 values the same result as an unsigned compare).
 *  - SelectBox3 redraws the old box also when it is the new one (drawbox
 *    draws completely, so only extra time).
 * DIFF: the coordinates passed to Boxf/Line/Locate are computed as words
 * and sign extended; the original leaves the sign of the box field in the
 * upper word (differs only for coordinates around 0/negative).
 */
#include "blitzlib.h"
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
#define BCALL0(tok, d0v) ({ struct bregs r_ = { .d0 = (d0v) }; BCALL(tok, 0, r_); r_.d0; })

/* DIVS.w with the original's overflow handling (BVC: else 32767 in the
   low word); a divisor of 0 raises the divide-by-zero exception */
static WORD divs_sat(ULONG d, WORD s)
{
    __asm__("divs.w %1,%0\n\t"
            "bvc.s 1f\n\t"
            "move.w #32767,%0\n"
            "1:" : "+d"(d) : "dmi"(s) : "cc");
    return (WORD)d;
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

struct page {
    ULONG memsize;
    UBYTE *memloc;
    UBYTE *mybitmap;
    UWORD numboxes;
    WORD c0, c1, c2, c3;        /* drk light mid text */
    UBYTE *selected;            /* pointer to currently selected box */
} BL_PACKED;

/* box fields (byte offsets; several overlap) */
#define B_X       0
#define B_Y       2
#define B_W       4
#define B_H       6
#define B_BTYPE   8
#define B_COLOR   10
#define B_VALUE   12            /* = bval x, showbp */
#define B_RANGE   14            /* = bval y */
#define B_ISSHAPE 16            /* = posbp x */
#define B_SHAPEOFF 18           /* = posbp y */
#define B_TLEN    20
#define B_TEXT    24
#define B_PROMPT  28            /* = bsize w */
#define B_ANSLEN  30            /* = bsize h */
#define BW(b, off) (*(WORD *)((b) + (off)))
#define BL(b, off) (*(ULONG *)((b) + (off)))
#define BP(b, off) (*(UBYTE **)((b) + (off)))

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
static UBYTE *getbox(struct page *p, ULONG n)
{
    if (!p->memsize)
        BLITZ_RUNERR(mess1);
    if ((UWORD)n >= p->numboxes)
        BLITZ_RUNERR(mess2);
    return p->memloc + (WORD)(UWORD)(n << 5);   /* ASL #5:ADD d0,a0 */
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

static void drawbox(struct page *p, UBYTE *a0)
{
    ULONG d0, d1;
    WORD x, y, x2, y2;

    d0 = 0;
    btype = BW(a0, B_BTYPE);
    if (btype < 0) {
        btype = -btype;
        d0 = (ULONG)-1;
    }
    nable = (WORD)d0;
    d0 = (ULONG)(UWORD)p->c0 << 16 | (UWORD)p->c1;
    d1 = (ULONG)(UWORD)p->c2 << 16 | (UWORD)p->c3;
    sel = 0;
    if (p->selected != a0) {
        BL_SETW(d1, d0);                        /* MOVE d0,d1 */
        sel = -1;
    } else if (btype == 4)
        BL_SETW(d0, d1);                        /* MOVE d1,d0 */
    d1 = BL_SWAP(d1);
    BL_SETW(d1, BW(a0, B_COLOR));
    if ((WORD)d1 < 0) {
        d0 = BL_SWAP(d0);
        BL_SETW(d1, ~d1);
        if (btype == 0 && BL(a0, B_TLEN))
            d0 = 0;
    }
    d1 = BL_SWAP(d1);
    c0 = (WORD)(d0 >> 16);
    c1 = (WORD)d0;
    c2 = (WORD)(d1 >> 16);
    c3 = (WORD)d1;

    x = BW(a0, B_X);
    y = BW(a0, B_Y);
    x2 = x + BW(a0, B_W) - 1;
    y2 = y + BW(a0, B_H) - 1;
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
    if (btype == 6 && !BL(a0, B_TLEN))
        goto notext;
    if (btype == 1 && BW(a0, B_ISSHAPE)) {
        /* shape button */
        boxf(x + 2, y + 1, x2 - 2, y2 - 1, (LONG)(sel ? c2 : c0));
        {
            struct bregs r = { .d0 = (LONG)(WORD)(BW(a0, B_SHAPEOFF) + BW(a0, B_VALUE)),
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
        WORD pw = BW(a0, B_W) - 3 - BW(a0, B_PROMPT);
        WORD ph = BW(a0, B_H) - 1 - BW(a0, B_ANSLEN);
        WORD d4 = BW(a0, B_VALUE), d5 = BW(a0, B_RANGE);
        WORD d6 = BW(a0, B_PROMPT), d7 = BW(a0, B_ANSLEN);

        if (d4 >= 0)
            px += (WORD)(((ULONG)(UWORD)pw * (UWORD)(d4 + d4)) >> 16);  /* MULU:SWAP */
        if (d5 >= 0)
            py += (WORD)(((ULONG)(UWORD)ph * (UWORD)(d5 + d5)) >> 16);
        d6 += px - 1;
        d7 += py - 1;
        xyxy2[0] = px;
        xyxy2[1] = py;
        xyxy2[2] = d6;
        xyxy2[3] = d7;
        if (!sel)
            boxf(px, py, d6, d7, (LONG)c3);
        line(px, py, d6, py, (LONG)c1);
        line(d6, py, d6, d7, (LONG)c1);
        line(d6 + 1, py, d6 + 1, d7, (LONG)c1);
        line(px, py, px, d7, (LONG)c0);
        line(px - 1, py + 1, px - 1, d7, (LONG)c0);
        line(px + 1, d7, d6 - 1, d7, (LONG)c0);
    }
    /* dotext */
    if (btype == 5) {
        /* list */
        WORD d0w = (UWORD)(BW(a0, B_H) - 3) >> 3, d2;
        UBYTE *a1;

        lines = d0w;
        d0w = (UWORD)d0w >> 1;
        d2 = BW(a0, B_VALUE) - d0w;
        if (d2 < 0)
            d2 = 0;
        d0w = BW(a0, B_RANGE) - lines;
        if (d0w < 0) {
            d0w = 0;
            lines = BW(a0, B_RANGE);
        }
        if (d2 - d0w >= 0)                      /* CMP d0,d2:BMI noj */
            d2 = d0w;
        count = BW(a0, B_VALUE) - d2;
        a1 = BP(a0, B_TEXT);
        while (d2-- != 0)                       /* BRA inig: DBRA d2 */
            while (*a1++)
                ;
        text[0] = (ULONG)a1;
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
        ULONG d0l = BL(a0, B_TLEN), d1l = BL(a0, B_TEXT);

        if (d0l) {
            ULONG t = d0l;                      /* EXG d0,d1 */
            d0l = d1l;
            d1l = t;
            if (btype == 3) {
                ULONG cx;
                ULONG row;

                text[0] = d0l;
                text[1] = d1l;
                cx = (ULONG)(LONG)xyxy[0];
                BL_SETW(cx, (UWORD)(xyxy[0] + 11) >> 3);
                row = (ULONG)(LONG)xyxy[1];
                BL_SETW(row, row - 8);
                if (BW(a0, B_PROMPT) >= 0) {
                    BL_SETW(row, row + 8);
                    BL_SETW(cx, cx - (UWORD)d1l);
                }
                BL_SETW(row, row + 2);
                locate(cx, BL_SWAP(row) >> 3);
                goto pme;
            }
            if (btype != 6 && BW(a0, B_RANGE)) {
                /* the value-th of the strings */
                UWORD d2 = BW(a0, B_VALUE);
                UBYTE *a1 = (UBYTE *)d0l;
                ULONG n;

                do {
                    n = 0;
                    do
                        n++;
                    while (*a1++);
                } while (d2-- != 0);
                a1 -= n;
                d1l = n;
                BL_SETW(d1l, d1l - 1);
                d0l = (ULONG)a1;
            }
        }
        text[0] = d0l;                          /* t3 */
        text[1] = d1l;
    }
    {
        ULONG d0l = text[1];

        if (!d0l)
            goto notext;
        if (btype == 2) {
            BL_SETW(d0l, (UWORD)(xyxy[0] + 11) >> 3);
        } else {
            BL_SETW(d0l, (UWORD)(((UWORD)(-(WORD)d0l << 3) + xyxy[0] + xyxy[2] + 7)) >> 4);
        }
        locate(d0l, locrow(xyxy[1]));
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

/* InitPage: a3 = page, a2 = bitmap object, d2 = number of boxes, d6 =
   offset of the used page (auto use) */
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

static void BLITZ_ROUTINE freepage_c(void)
{
    freepage_(PG);
}
BLITZ_THUNK(_freepage, freepage_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE setpagepens_c(ULONG a BL_REG(d1), ULONG b BL_REG(d2),
                                        ULONG c BL_REG(d3), ULONG d BL_REG(d4))
{
    struct page *p = PG;

    p->c0 = (WORD)a;
    p->c1 = (WORD)b;
    p->c2 = (WORD)c;
    p->c3 = (WORD)d;
}
BLITZ_THUNK(_setpagepens, setpagepens_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE togglebox_c(ULONG n BL_REG(d0))
{
    UBYTE *b = getbox(PG, n);

    BW(b, B_COLOR) = ~BW(b, B_COLOR);
}
BLITZ_THUNK(_togglebox, togglebox_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE enablebox_c(ULONG n BL_REG(d0))
{
    UBYTE *b = getbox(PG, n);

    if (BW(b, B_BTYPE) < 0)
        BW(b, B_BTYPE) = -BW(b, B_BTYPE);
}
BLITZ_THUNK(_enablebox, enablebox_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE disablebox_c(ULONG n BL_REG(d0))
{
    UBYTE *b = getbox(PG, n);

    if (BW(b, B_BTYPE) > 0)
        BW(b, B_BTYPE) = -BW(b, B_BTYPE);
}
BLITZ_THUNK(_disablebox, disablebox_c, "d0-d1/a0-a1");

/* BoxPropX/Y: the knob position as Quick (word * 2) */
static ULONG BLITZ_ROUTINE boxpropx_c(ULONG n BL_REG(d0))
{
    return (ULONG)(LONG)BW(getbox(PG, n), B_VALUE) << 1;
}
BLITZ_THUNK(_boxpropx, boxpropx_c, "d1/a1");

static ULONG BLITZ_ROUTINE boxpropy_c(ULONG n BL_REG(d0))
{
    return (ULONG)(LONG)BW(getbox(PG, n), B_RANGE) << 1;
}
BLITZ_THUNK(_boxpropy, boxpropy_c, "d1/a1");

/* ----------------------------------------------------------- AddBox */

static UBYTE *addbox(struct page *p, ULONG n, ULONG x, ULONG y, ULONG w, ULONG h, WORD type)
{
    UBYTE *b;
    WORD d1;

    chkpage(p);
    if ((UWORD)n >= p->numboxes)
        BLITZ_RUNERR(mess2);
    b = p->memloc + (WORD)(UWORD)(n << 5);
    if (BL(b, B_TLEN)) {
        globfree(BP(b, B_TEXT), BL(b, B_TLEN));
        BL(b, B_TLEN) = 0;
    }
    BW(b, B_X) = (WORD)x;
    BW(b, B_Y) = (WORD)y;
    BW(b, B_W) = (WORD)w;
    BW(b, B_H) = (WORD)h;
    d1 = p->c2;
    if (type & 1)                               /* ASR #1: carry */
        d1 = ~d1;
    type >>= 1;
    BW(b, B_BTYPE) = type;
    BW(b, B_COLOR) = d1;
    if (type == 2) {
        /* a string box: spaces */
        UWORD len = (UWORD)w >> 3;
        UBYTE *t;
        UWORD c;

        if (!len)                               /* EXT.l:BLE err4 */
            BLITZ_RUNERR(mess4);
        BL(b, B_TLEN) = len;
        t = globalloc(len, 0);
        BP(b, B_TEXT) = t;
        c = (UWORD)(len - 2);
        do
            *t++ = ' ';
        while (c-- != 0);
        *t = 0;
    } else if (type == 3) {
        /* a prop: bsize & bval */
        WORD d2 = -1, d3 = -1;
        WORD d4 = (WORD)w - 4, d5 = (WORD)h - 2;

        if ((WORD)(d4 - 28) >= 0) {             /* CMP #28:BMI px */
            d2 = 16384;
            d4 = 12;
        }
        if ((WORD)(d5 - 10) >= 0) {
            d3 = 16384;
            d5 = 6;
        }
        BW(b, B_VALUE) = d2;
        BW(b, B_RANGE) = d3;
        BW(b, B_PROMPT) = d4;
        BW(b, B_ANSLEN) = d5;
    }
    return b;
}

static void BLITZ_ROUTINE addbox_c(ULONG n BL_REG(d1), ULONG x BL_REG(d2), ULONG y BL_REG(d3),
                                   ULONG w BL_REG(d4), ULONG h BL_REG(d5), const UWORD *args BL_REG(a2))
{
    addbox(PG, n, x, y, w, h, (WORD)args[-1]);
}
BLITZ_THUNK(_addbox, addbox_c, "d0-d1/a0-a1");

/* AddBox ... shape, shape */
static void BLITZ_ROUTINE addbox2_c(ULONG n BL_REG(d1), ULONG x BL_REG(d2), ULONG y BL_REG(d3),
                                    ULONG w BL_REG(d4), ULONG h BL_REG(d5), const UWORD *args BL_REG(a2))
{
    UBYTE *b = addbox(PG, n, x, y, w, h, (WORD)args[-1]);
    WORD s1 = (WORD)args[-2], s2 = (WORD)args[-3];

    BW(b, B_RANGE) = s2 - s1 + 1;
    BW(b, B_SHAPEOFF) = s1;
    BW(b, B_ISSHAPE) = -1;
}
BLITZ_THUNK(_addbox2, addbox2_c, "d0-d1/a0-a1");

/* the text of a box: d0 = length, a1 = string (nostrya / strya) */
static void boxtext(UBYTE *b, ULONG len, const UBYTE *s)
{
    UBYTE *t;
    UWORD d1;

    if (BW(b, B_BTYPE) == 2) {
        /* strya: the prompt of a string box */
        BW(b, B_PROMPT) = (WORD)len;
        if ((WORD)BL(b, B_TLEN) <= (WORD)len)   /* CMP d0,d1:BLE err4 */
            BLITZ_RUNERR(mess4);
        t = BP(b, B_TEXT);
        while (*s)
            *t++ = *s++;
        return;
    }
    len++;
    BL(b, B_TLEN) = len;
    t = globalloc(len, 0);
    BP(b, B_TEXT) = t;
    d1 = 1;
    if (*s == '|') {
        while (*s)                              /* verbatim */
            *t++ = *s++;
    } else {
        UBYTE c;

        while ((c = *s++) != 0) {
            if (c == '|') {
                c = 0;
                d1++;
            }
            *t++ = c;
        }
    }
    *t = 0;
    if (BW(b, B_BTYPE) - 2 < 0) {               /* CMP #2:BPL stdun */
        BW(b, B_RANGE) = d1;
        BW(b, B_VALUE) = 0;
    }
}

/* AddBox ... text$: the string pointer and its length are below the type
   word on the a2 stack */
static void BLITZ_ROUTINE addbox1_c(ULONG n BL_REG(d1), ULONG x BL_REG(d2), ULONG y BL_REG(d3),
                                    ULONG w BL_REG(d4), ULONG h BL_REG(d5), const UBYTE *args BL_REG(a2))
{
    UBYTE *b = addbox(PG, n, x, y, w, h, *(const WORD *)(args - 2));

    boxtext(b, *(const ULONG *)(args - 6), *(const UBYTE *const *)(args - 10));
}
BLITZ_THUNK(_addbox1, addbox1_c, "d0-d1/a0-a1");

/* SetBoxTitle: d1 = string, -4(a2) = length */
static void BLITZ_ROUTINE setboxtitle_c(ULONG n BL_REG(d0), const UBYTE *s BL_REG(d1),
                                        const ULONG *lens BL_REG(a2))
{
    UBYTE *b = getbox(PG, n);
    ULONG len = lens[-1];

    if (BW(b, B_BTYPE) != 2 && BL(b, B_TLEN))
        globfree(BP(b, B_TEXT), BL(b, B_TLEN));
    boxtext(b, len, s);
}
BLITZ_THUNK(_setboxtitle, setboxtitle_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- set values */

static void setbox(ULONG n, ULONG v)
{
    UBYTE *b = getbox(PG, n);
    WORD d1 = (WORD)v;

    if (d1 < 0)
        d1 += BW(b, B_RANGE);
    if ((UWORD)d1 >= (UWORD)BW(b, B_RANGE))
        BLITZ_RUNERR(mess3);
    BW(b, B_VALUE) = d1;
}

static void BLITZ_ROUTINE setbox_c(ULONG n BL_REG(d0), ULONG v BL_REG(d1))
{
    setbox(n, v);
}
BLITZ_THUNK(_setbox, setbox_c, "d0-d1/a0-a1");

/* ChangeBox Page#,Box#,Value */
static void BLITZ_ROUTINE changebox_c(ULONG n BL_REG(d1), ULONG v BL_REG(d2))
{
    setbox(n, v);
}
BLITZ_THUNK(_changebox, changebox_c, "d0-d1/a0-a1");

/* SetBox Box#,xprop,yprop (Quick: (q-1 if q<>0).w >> 1) */
static void BLITZ_ROUTINE setbox2_c(ULONG n BL_REG(d0), ULONG qx BL_REG(d1), ULONG qy BL_REG(d2))
{
    UBYTE *b = getbox(PG, n);

    if (BW(b, B_BTYPE) != 3)
        BLITZ_RUNERR(mess7);
    if (qx)
        qx--;
    if (qy)
        qy--;
    if (BW(b, B_VALUE) >= 0)
        BW(b, B_VALUE) = (UWORD)qx >> 1;
    if (BW(b, B_RANGE) >= 0)
        BW(b, B_RANGE) = (UWORD)qy >> 1;
}
BLITZ_THUNK(_setbox2, setbox2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE setboxcolor_c(ULONG n BL_REG(d0), ULONG c BL_REG(d1))
{
    struct page *p = PG;
    UBYTE *b = getbox(p, n);

    if (BW(b, B_COLOR) < 0)
        c = ~c;
    BW(b, B_COLOR) = (WORD)c;
    drawbox(p, b);
}
BLITZ_THUNK(_setboxcolor, setboxcolor_c, "d0-d1/a0-a1");

/* SetBoxString: the answer of a string box (after the prompt), padded
   with spaces */
static void BLITZ_ROUTINE setboxstring_c(ULONG n BL_REG(d0), const UBYTE *s BL_REG(d1),
                                         const ULONG *lens BL_REG(a2))
{
    UBYTE *b = getbox(PG, n);
    ULONG d1 = lens[-1];
    ULONG d2 = BL(b, B_TLEN);
    UBYTE *t = BP(b, B_TEXT);

    BL_SETW(d2, d2 - BW(b, B_PROMPT));
    BL_SETW(d2, d2 - d1);
    if ((WORD)d2 <= 0)
        BLITZ_RUNERR(mess4);
    t += BW(b, B_PROMPT);
    BL_SETW(d1, d1 - 1);
    if ((WORD)d1 >= 0) {
        UWORD c = (UWORD)d1;
        do
            *t++ = *s++;
        while (c-- != 0);
    }
    BL_SETW(d2, d2 - 2);
    if ((WORD)d2 >= 0) {
        UWORD c = (UWORD)d2;
        do
            *t++ = ' ';
        while (c-- != 0);
    }
    *t = 0;
}
BLITZ_THUNK(_setboxstring, setboxstring_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- functions */

/* BoxValue: MOVE value,d0 (low word) */
static ULONG BLITZ_ROUTINE boxvalue_c(ULONG n BL_REG(d0))
{
    return (n & 0xffff0000UL) | (UWORD)BW(getbox(PG, n), B_VALUE);
}
BLITZ_THUNK(_boxvalue, boxvalue_c, "d1/a1");

static ULONG BLITZ_ROUTINE boxvalue2_c(ULONG d0 BL_REG(d0), ULONG n BL_REG(d1))
{
    BL_SETW(d0, n);                             /* MOVE d1,d0 */
    return (d0 & 0xffff0000UL) | (UWORD)BW(getbox(PG, d0), B_VALUE);
}
BLITZ_THUNK(_boxvalue2, boxvalue2_c, "d1/a1");

/* BoxString: page in a1, result at a3 (no 0 written; a3 = the end) */
static ULONG boxstring(struct page *p, ULONG n)
{
    UBYTE *b = getbox(p, n);
    WORD prompt = BW(b, B_PROMPT);
    ULONG d0 = BL(b, B_TLEN);
    const UBYTE *a0 = BP(b, B_TEXT) + prompt;
    const UBYTE *a1;
    UBYTE *dst = blitz_string;

    BL_SETW(d0, d0 - prompt);
    a1 = a0 + (WORD)d0 - 1;
    do                                          /* crop */
        BL_SETW(d0, d0 - 1);
    while (*--a1 == ' ');
    if ((UWORD)d0) {
        UWORD c = (UWORD)d0 - 1;
        do
            *dst++ = *a0++;
        while (c-- != 0);
    }
    blitz_string = dst;
    return d0;
}

static ULONG BLITZ_ROUTINE boxstring_c(ULONG n BL_REG(d0), struct page *p BL_REG(a1))
{
    return boxstring(p, n);
}
BLITZ_THUNK(_boxstring, boxstring_c, "d1");

static ULONG BLITZ_ROUTINE boxstring2_c(ULONG d0 BL_REG(d0), ULONG n BL_REG(d1), struct page *p BL_REG(a1))
{
    BL_SETW(d0, n);
    return boxstring(p, d0);
}
BLITZ_THUNK(_boxstring2, boxstring2_c, "d1");

/* BoxSelected: (selected - memloc) >> 5 (LSR.w), -1 for none */
static ULONG BLITZ_ROUTINE boxselected_c(void)
{
    struct page *p = PG;
    ULONG d0;

    chkpage(p);
    d0 = (ULONG)p->selected;
    if (!d0)
        return (ULONG)-1;
    d0 -= (ULONG)p->memloc;
    if ((LONG)d0 < 0)
        return (ULONG)-1;
    BL_SETW(d0, (UWORD)d0 >> 5);
    return d0;
}
BLITZ_THUNK(_boxselected, boxselected_c, "d1/a0-a1");

/* ---------------------------------------------------------- drawing */

static void BLITZ_ROUTINE redrawpage_c(void)
{
    struct page *p = PG;
    UBYTE *b;
    WORD n;

    chkpage(p);
    b = p->memloc;
    n = (WORD)(p->numboxes - 1);
    if (n < 0)
        return;
    do {
        if (BW(b, B_W))
            drawbox(p, b);
        b += 32;
    } while (n-- != 0);
}
BLITZ_THUNK(_redrawpage, redrawpage_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE redrawbox_c(ULONG n BL_REG(d0))
{
    struct page *p = PG;

    drawbox(p, getbox(p, n));
}
BLITZ_THUNK(_redrawbox, redrawbox_c, "d0-d1/a0-a1");

/* BoxShow: a view box shows part of a bitmap (Scroll) */
static void dobs(struct page *p, UBYTE *b)
{
    WORD px = BW(b, B_ISSHAPE), py = BW(b, B_SHAPEOFF);
    WORD dx = BW(b, B_X) + 4, dy = BW(b, B_Y) + 2;
    WORD w = BW(b, B_W) - 8, h = BW(b, B_H) - 4;
    UBYTE *src = BP(b, B_VALUE);
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
    BW(b, B_ISSHAPE) = px;
    BW(b, B_SHAPEOFF) = py;
    xyxy[0] = h;                                /* EXG d4,d5:MOVEM d4-d5,xyxy */
    xyxy[1] = w;
    scroll((LONG)px, (LONG)py, (LONG)dx, (LONG)dy, w, h, src, p->mybitmap);
}

static void BLITZ_ROUTINE boxshow_c(ULONG n BL_REG(d0), UBYTE *bm BL_REG(a2),
                                    ULONG x BL_REG(d2), ULONG y BL_REG(d3))
{
    struct page *p = PG;
    UBYTE *b = getbox(p, n);

    if (BW(b, B_BTYPE))
        BLITZ_RUNERR(mess5);
    BP(b, B_VALUE) = bm;
    BW(b, B_ISSHAPE) = (WORD)x;
    BW(b, B_SHAPEOFF) = (WORD)y;
    dobs(p, b);
}
BLITZ_THUNK(_boxshow, boxshow_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE boxshow2_c(ULONG n BL_REG(d0), ULONG dx BL_REG(d1), ULONG dy BL_REG(d2))
{
    struct page *p = PG;
    UBYTE *b = getbox(p, n);

    if (BW(b, B_BTYPE))
        BLITZ_RUNERR(mess5);
    BW(b, B_ISSHAPE) += (WORD)dx;
    BW(b, B_SHAPEOFF) += (WORD)dy;
    dobs(p, b);
}
BLITZ_THUNK(_boxshow2, boxshow2_c, "d0-d1/a0-a1");

static UWORD cx, cy;

/* BoxPrint: d1 = string, -4(a2) = length (overwritten with Scroll's size) */
static void boxprint(struct page *p, ULONG n, const UBYTE *s, ULONG *lens)
{
    UBYTE *b;
    ULONG d7;

    chkpage(p);
    if ((UWORD)n >= p->numboxes)
        BLITZ_RUNERR(mess2);
    b = p->memloc + (WORD)(UWORD)(n << 5);
    if (BW(b, B_BTYPE))
        BLITZ_RUNERR(mess5);
    d7 = lens[-1];
    if ((UWORD)BW(b, B_H) < 16) {
        /* doleft: scroll the line left, print at the right */
        WORD x = BW(b, B_X), y = BW(b, B_Y), w = BW(b, B_W);
        UWORD col = (UWORD)(x + 11) >> 3;
        UWORD d0, d7w = (UWORD)(d7 << 3);
        UWORD *wh = (UWORD *)lens - 2;

        cx = col;
        cy = y + 2;
        d0 = (UWORD)(x - (WORD)(col << 3) + w - 1) >> 3;
        cx += d0;
        d0 <<= 3;
        wh[0] = 8;                              /* -4(a2) = h */
        wh[1] = d0 - d7w;                       /* -2(a2) = w */
        {
            UWORD wv = wh[1];
            scroll((UWORD)((col << 3) + d7w), (UWORD)cy, (UWORD)(col << 3), (UWORD)cy,
                   wv, 8, p->mybitmap, p->mybitmap);
        }
        waitblit();
        locate((ULONG)(LONG)(WORD)(cx - (UWORD)d7), BL_SWAP((ULONG)(LONG)(WORD)cy) >> 3);
        printn(s, d7);
        return;
    }
    {
        /* doup: scroll the box up 8 pixels, print in the last line */
        WORD x = BW(b, B_X), y = BW(b, B_Y), w = BW(b, B_W), h = BW(b, B_H);
        UWORD *wh = (UWORD *)lens - 2;

        wh[0] = h - 12;
        wh[1] = w - 8;
        scroll((LONG)(WORD)(x + 4), (LONG)(WORD)(y + 10), (LONG)(WORD)(x + 4),
               (LONG)(WORD)(y + 2), w - 8, h - 12, p->mybitmap, p->mybitmap);
        boxf(x + 2, y + h - 10, x + w - 3, y + h - 2, (LONG)p->c2);
        waitblit();
        locate((ULONG)(LONG)(WORD)((UWORD)(x + 11) >> 3),
               BL_SWAP((ULONG)(LONG)(WORD)(y + h - 10)) >> 3);
        printn(s, d7);
    }
}

static void BLITZ_ROUTINE boxprint_c(ULONG n BL_REG(d0), const UBYTE *s BL_REG(d1),
                                     ULONG *lens BL_REG(a2))
{
    boxprint(PG, n, s, lens);
}
BLITZ_THUNK(_boxprint, boxprint_c, "d0-d1/a0-a1");

static ULONG linelen[2];                        /* length; a2 points behind */

/* BoxPrint Box#,Bank#,Line: a2 = bank object, d2 = line */
static void BLITZ_ROUTINE boxprint2_c(ULONG n BL_REG(d0), UBYTE **bank BL_REG(a2), ULONG ln BL_REG(d2))
{
    UBYTE *a2 = *bank, *start;
    UWORD d2 = (UWORD)ln;
    ULONG d3;

    if (!a2)
        BLITZ_RUNERR(mess6);
    if ((WORD)--d2 >= 0) {
        do {
            UBYTE c;
            do
                c = *a2++;
            while (c && c != 10);
        } while (d2-- != 0);
    }
    start = a2;
    d3 = (ULONG)-1;
    for (;;) {
        UBYTE c = *a2++;

        d3++;
        if (!c)
            break;
        if (c == 10) {
            a2[-1] = 0;
            break;
        }
    }
    linelen[0] = d3;
    boxprint(PG, n, start, linelen + 1);
}
BLITZ_THUNK(_boxprint2, boxprint2_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- selecting */

static WORD inprop, inedit, indrag;
static WORD edxyc[3];                           /* x, y, cursor */

static WORD status, delay, button, mo, jx, jy, jb, mx, my, mb, ox, oy;
static constexpr WORD dcon = 6;                 /* delay constant */
static constexpr WORD pspd = 1024;              /* prop speed */

/* SelectBox Box# (negative / >= 1024: deselect) */
static void BLITZ_ROUTINE selectbox_c(ULONG n BL_REG(d0))
{
    struct page *p = PG;
    UBYTE *old;
    WORD off;

    chkpage(p);
    old = p->selected;
    p->selected = 0;
    off = (WORD)(UWORD)(n << 5);
    if (off >= 0) {
        p->selected = p->memloc + off;
        drawbox(p, p->selected);
    }
    if (old)
        drawbox(p, old);
}
BLITZ_THUNK(_selectbox, selectbox_c, "d0-d1/a0-a1");

/* find the box at (x, y): enabled, x <= px < x+w, y <= py < y+h */
static int inbox(UBYTE *b, WORD px, WORD py)
{
    WORD d2, d3;

    if (BW(b, B_BTYPE) <= 0)
        return 0;
    d2 = BW(b, B_X) - px;
    if (d2 > 0)
        return 0;
    d3 = BW(b, B_Y) - py;
    if (d3 > 0)
        return 0;
    d2 = -d2;
    d3 = -d3;
    if (BW(b, B_W) <= d2)
        return 0;
    return BW(b, B_H) > d3;
}

/* SelectBox 0,x,y: -1 = found (selected), 0 = none */
static LONG selectbox3(struct page *p, WORD x, WORD y)
{
    UBYTE *old, *b;
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
        b += 32;
    } while (n-- != 0);
    return 0;
}

static ULONG BLITZ_ROUTINE selectbox3_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2))
{
    return selectbox3(PG, (WORD)x, (WORD)y);
}
BLITZ_THUNK(_selectbox3, selectbox3_c, "d1/a0-a1");

/* SelectBox dx,dy: the nearest box in that direction */
static void selectbox2(struct page *p, WORD dx, WORD dy)
{
    UBYTE *a0, *a1, *a2;
    WORD d4, d5;
    UWORD d0, n;

    chkpage(p);
    a0 = p->selected;
    if (!a0)
        return;
    a2 = a0;
    a1 = p->memloc;
    d4 = BW(a0, B_X);
    d5 = BW(a0, B_Y);
    if (dx > 0)
        d4 += BW(a0, B_W);
    if (dy > 0)
        d5 += BW(a0, B_H) - 1;
    d0 = 8192;
    n = p->numboxes - 1;
    do {
        WORD d6, d7;

        if (a1 == a0 || BW(a1, B_BTYPE) <= 0)
            goto mis;
        d6 = BW(a1, B_X) - d4;
        if (dx < 0)
            d6 = -d6;
        if (d6 < 0) {
            if (dx)
                goto mis;
            d6 = -d6;
        }
        d7 = BW(a1, B_Y) - d5;
        if (dy < 0)
            d7 = -d7;
        if (d7 < 0) {
            if (dy)
                goto mis;
            d7 = -d7;
        }
        if (dx) {
            if (!d6)
                goto mis;
            d7 <<= 5;
        }
        if (dy) {
            if (!d7)
                goto mis;
            d6 <<= 5;
        }
        d7 += d6;
        if (d0 < (UWORD)d7)                     /* CMP d7,d0:BCS mis */
            goto mis;
        d0 = (UWORD)d7;
        a2 = a1;
mis:
        a1 += 32;
    } while (n-- != 0);
    if (a2 == a0)
        return;
    p->selected = a2;
    drawbox(p, a0);
    drawbox(p, p->selected);
}

static void BLITZ_ROUTINE selectbox2_c(ULONG dx BL_REG(d0), ULONG dy BL_REG(d1))
{
    selectbox2(PG, (WORD)dx, (WORD)dy);
}
BLITZ_THUNK(_selectbox2, selectbox2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE deselectbox_c(void)
{
    struct page *p = PG;
    UBYTE *b;

    chkpage(p);
    b = p->selected;
    if (!b)
        return;
    if (BW(b, B_COLOR) < 0)
        BW(b, B_COLOR) = ~BW(b, B_COLOR);
    drawbox(p, b);
    inprop = 0;
    inedit = 0;
    indrag = 0;
}
BLITZ_THUNK(_deselectbox, deselectbox_c, "d0-d1/a0-a1");

/* the edit cursor: NOT 8 bytes in plane 0 */
static void docursor(struct page *p)
{
    UBYTE *bm = p->mybitmap;
    UWORD bpr = *(UWORD *)bm;
    UBYTE *a4 = *(UBYTE **)(bm + 8);
    int i;

    a4 += (WORD)((UWORD)edxyc[0] >> 3);
    a4 += (ULONG)(UWORD)edxyc[1] * bpr;         /* MULU */
    a4 += edxyc[2];
    for (i = 0; i < 8; i++) {
        *a4 = ~*a4;
        if (i < 7)
            a4 += (WORD)bpr;
    }
}

/* show the text from the prompt on, at edxyc (the bitmap's cursor) */
static void edstring(struct page *p, UBYTE *b)
{
    UBYTE *bm = p->mybitmap;
    ULONG old = *(ULONG *)(bm + 40);
    WORD prompt = BW(b, B_PROMPT);
    ULONG d0 = BL(b, B_TLEN);

    *(ULONG *)(bm + 40) = *(ULONG *)edxyc;
    BL_SETW(d0, d0 - prompt);
    BL_SETW(d0, d0 - 1);
    printn(BP(b, B_TEXT) + prompt, d0);
    *(ULONG *)(bm + 40) = old;
}

static void activateselected(struct page *p)
{
    UBYTE *b;
    WORD t;

    chkpage(p);
    b = p->selected;
    if (!b)
        return;
    t = BW(b, B_BTYPE);
    if (t == 1 || t == 5) {
        drawbox(p, b);                          /* button up now cycles */
    } else if (t == 2) {
        edxyc[0] = (WORD)((((UWORD)(BW(b, B_X) + 11) >> 3) + BW(b, B_PROMPT)) << 3);
        edxyc[1] = BW(b, B_Y) + 2;
        edxyc[2] = 0;
        inedit = -1;
        drawbox(p, b);
        docursor(p);
    } else if (t == 3) {
        inprop = -1;
        drawbox(p, b);
    } else if (t == 6) {
        indrag = -1;
        drawbox(p, b);
    }
}

static void BLITZ_ROUTINE activateselected_c(void)
{
    activateselected(PG);
}
BLITZ_THUNK(_activateselected, activateselected_c, "d0-d1/a0-a1");

/* SpotBox: the box under (mx,my), -1 for none (no page check) */
static ULONG BLITZ_ROUTINE spotbox_c(void)
{
    struct page *p = PG;
    UBYTE *b = p->memloc;
    UWORD n = p->numboxes - 1;
    ULONG d6 = 0;

    do {
        if (inbox(b, mx, my))
            return ((ULONG)(LONG)mx & 0xffff0000UL) | (UWORD)d6;  /* MOVE d6,d0 */
        d6++;
        b += 32;
    } while (n-- != 0);
    return (ULONG)-1;
}
BLITZ_THUNK(_spotbox, spotbox_c, "d1/a0-a1");

/* MouseEnabled: MOVE mo,d0 (low word) / 0 while dragging */
static ULONG BLITZ_ROUTINE mouseenabled_c(ULONG d0 BL_REG(d0))
{
    if (indrag)
        return 0;
    BL_SETW(d0, mo);
    return d0;
}
BLITZ_THUNK(_mouseenabled, mouseenabled_c, "d1/a0-a1");

/* ------------------------------------------------------- BlitzEvent */

static const WORD delt[] = { 0, 0, 0, -1, -1, 0, 0, 1, 1, 0 };

/* status 1=up 3=down 2=left 4=right */
static LONG blitzevent(struct page *p)
{
    WORD d2;
    UBYTE *a0;

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
    d2 = jx;
    if (d2)
        d2 += 3;
    else {
        d2 = jy;
        if (d2)
            d2 += 2;
    }
    if (rawstatus(0x4c))
        d2 = 1;
    if (rawstatus(0x4d))
        d2 = 3;
    if (rawstatus(0x4e))
        d2 = 4;
    if (rawstatus(0x4f))
        d2 = 2;
    if (rawstatus(0x64) && rawstatus(0x66))
        mb |= 1;
    if (rawstatus(0x67) && rawstatus(0x65))
        mb |= 2;
    if (!d2) {
        status = 0;
        goto dobutton;
    }
    /* changedir */
    mo = 0;
    if (status == d2) {
        if (--delay >= 0)
            goto dobutton;
        delay = dcon;
    } else {
        delay = dcon + dcon;
        status = d2;
    }
    /* dodir */
    if (inedit)
        goto movecursor;
    if (button)
        goto dobutton;
    if (status & 1) {
        a0 = p->selected;
        if (BW(a0, B_BTYPE) == 5)
            goto movelist;
    }
    {
        WORD i = (WORD)(status << 2) / 2;
        selectbox2(p, delt[i], delt[i + 1]);
    }
    goto dobutton;

movelist:
    {
        WORD v = BW(a0, B_VALUE) + status - 2, r = BW(a0, B_RANGE);

        if (v < 0)
            v = 0;
        if (v - r >= 0)                         /* CMP d1,d0:BMI fop */
            v = r - 1;
        BW(a0, B_VALUE) = v;
        drawbox(p, a0);
        return 0;
    }

movecursor:
    {
        WORD d0, d1;
        UBYTE c;

        docursor(p);
        d0 = edxyc[2];
        a0 = p->selected;
        d1 = (WORD)BL(a0, B_TLEN) - BW(a0, B_PROMPT) - 1;
        if (status == 2 && --d0 < 0)
            d0 = 0;
        if (status == 4) {
            d0++;
            if (d0 - d1 >= 0)                   /* CMP d1,d0:BMI nri */
                d0--;
        }
        edxyc[2] = d0;
        if (status == 1 || status == 3) {
            UBYTE *a1 = BP(a0, B_TEXT) + BW(a0, B_PROMPT) + d0;

            c = (UBYTE)((status == 1 ? 1 : -1) + *a1);
            if (c == 31)
                c = 90;
            if (c == 33)
                c = 48;
            if ((BYTE)(c - 48) < 0 || (BYTE)(c - 91) >= 0)  /* BMI after CMP.b */
                c = 32;
            *a1 = c;
            edstring(p, a0);
        }
        docursor(p);
        return 0;
    }

dobutton:
    {
        UWORD d = rawstatus(0x44) ? 0xffff : 0;

        d |= jb | mb;
        if (d)
            goto down;
        if (!button)
            goto noselect;
        button = 0;
        /* upbutton */
        if (inedit)
            goto noselect;
        inprop = 0;
        indrag = 0;
        a0 = p->selected;
        if (!a0 || BW(a0, B_COLOR) >= 0)
            goto noselect;
        BW(a0, B_COLOR) = ~BW(a0, B_COLOR);
        if (BW(a0, B_BTYPE) == 1) {
            UWORD v = BW(a0, B_VALUE) + 1;

            if (v >= (UWORD)BW(a0, B_RANGE))
                v = 0;
            BW(a0, B_VALUE) = v;
        }
        drawbox(p, a0);
        return -1;

down:
        if (inprop)
            goto moveprop;
        if (indrag)
            goto movedrag;
        if (button)
            goto noselect;
        button = d;
        if (inedit) {
            inedit = 0;
            goto noselect;
        }
        if (mo && !selectbox3(p, mx, my))
            return 0;
        a0 = p->selected;
        if (!a0)
            goto noselect;
        BW(a0, B_COLOR) = ~BW(a0, B_COLOR);
        activateselected(p);
        return 1;
    }

noselect:
    {
        WORD d0, d1, d2c, d3;
        UBYTE *a1;

        if (!inedit)
            return 0;
        d0 = (WORD)BCALL0(T_BLITZKEY, 0);
        if (d0 < 0)
            return 0;
        docursor(p);
        a0 = p->selected;
        a1 = BP(a0, B_TEXT) + BW(a0, B_PROMPT);
        d1 = (WORD)BL(a0, B_TLEN) - BW(a0, B_PROMPT) - 1;
        d2c = edxyc[2];
        if ((UWORD)d0 >= 32 && (UWORD)d0 <= 126) {
            if ((UWORD)d0 >= 97)
                d0 -= 32;
            d3 = d1;
            while (--d3 != d2c)
                a1[d3] = a1[d3 - 1];
            a1[d2c] = (UBYTE)d0;
            d2c++;
            if (d2c - d1 >= 0)
                d2c--;
            edxyc[2] = d2c;
        } else {
            if (d0 != 127) {
                if (d0 != 8 || --d2c < 0)
                    return 0;
                edxyc[2] = d2c;
            }
            while (++d2c != d1)
                a1[d2c - 1] = a1[d2c];
            a1[d2c - 1] = ' ';
        }
        edstring(p, a0);
        docursor(p);
        return 0;
    }

movedrag:
    a0 = p->selected;
    BW(a0, B_VALUE) = mx - ox;
    BW(a0, B_RANGE) = my - oy;
    mx = ox;
    my = oy;
    {
        struct bregs r = { .d0 = (LONG)ox, .d1 = (LONG)oy };
        BCALL(T_POSITIONMOUSE, 0, r);
    }
    return 2;

moveprop:
    {
        WORD d0, d1;

        a0 = p->selected;
        d0 = BW(a0, B_VALUE);
        d1 = BW(a0, B_RANGE);
        if (mo) {
            WORD d2w = BW(a0, B_W), d3w = BW(a0, B_H);
            WORD d4 = BW(a0, B_PROMPT), d5 = BW(a0, B_ANSLEN);

            if (d0 >= 0) {
                d0 = mx;
                d2w = d2w - 4 - d4;
                d0 = d0 - BW(a0, B_X) - 2;
                d4 = (UWORD)d4 >> 1;
                d0 -= d4;
                if (d0 < 0)
                    d0 = 0;
                d0 = (UWORD)d0 >> 1;
                d0 = divs_sat((ULONG)(UWORD)d0 << 16, d2w);
            }
            if (d1 >= 0) {
                d1 = my;
                d3w = d3w - 2 - d5;
                d1 = d1 - BW(a0, B_Y) - 2;
                d5 = (UWORD)d5 >> 1;
                d1 -= d5;
                if (d1 < 0)
                    d1 = 0;
                d1 = (UWORD)d1 >> 1;
                d1 = divs_sat((ULONG)(UWORD)d1 << 16, d3w);
            }
        } else {
            WORD d2s = status, d3s = pspd;

            if (d0 >= 0) {
                if (d2s == 2 && (d0 -= d3s) < 0)
                    d0 = 0;
                if (d2s == 4 && (d0 += d3s) < 0)
                    d0 = 32767;
            }
            if (d1 >= 0) {
                if (d2s == 1 && (d1 -= d3s) < 0)
                    d1 = 0;
                if (d2s == 3 && (d1 += d3s) < 0)
                    d1 = 32767;
            }
        }
        BW(a0, B_VALUE) = d0;
        BW(a0, B_RANGE) = d1;
        drawbox(p, a0);
        return 2;
    }
}

static ULONG BLITZ_ROUTINE blitzevent_c(void)
{
    return (ULONG)blitzevent(PG);
}
BLITZ_THUNK(_blitzevent, blitzevent_c, "d1/a0-a1");
