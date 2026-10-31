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

static void drawbox(struct page *p, UBYTE *box)
{
    ULONG disabled;                             /* d0: 0 or -1 */
    ULONG darklight;                            /* d0: the c0:c1 pens */
    ULONG midtext;
    WORD x, y, x2, y2;

    disabled = 0;
    btype = BW(box, B_BTYPE);
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
    BL_SETW(midtext, BW(box, B_COLOR));
    if ((WORD)midtext < 0) {
        darklight = BL_SWAP(darklight);
        BL_SETW(midtext, ~midtext);
        if (btype == 0 && BL(box, B_TLEN))
            darklight = 0;
    }
    midtext = BL_SWAP(midtext);
    c0 = (WORD)(darklight >> 16);
    c1 = (WORD)darklight;
    c2 = (WORD)(midtext >> 16);
    c3 = (WORD)midtext;

    x = BW(box, B_X);
    y = BW(box, B_Y);
    x2 = x + BW(box, B_W) - 1;
    y2 = y + BW(box, B_H) - 1;
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
    if (btype == 6 && !BL(box, B_TLEN))
        goto notext;
    if (btype == 1 && BW(box, B_ISSHAPE)) {
        /* shape button */
        boxf(x + 2, y + 1, x2 - 2, y2 - 1, (LONG)(sel ? c2 : c0));
        {
            struct bregs r = { .d0 = (LONG)(WORD)(BW(box, B_SHAPEOFF) + BW(box, B_VALUE)),
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
        WORD pw = BW(box, B_W) - 3 - BW(box, B_PROMPT);
        WORD ph = BW(box, B_H) - 1 - BW(box, B_ANSLEN);
        WORD valx = BW(box, B_VALUE), valy = BW(box, B_RANGE);
        WORD kx2 = BW(box, B_PROMPT), ky2 = BW(box, B_ANSLEN);  /* bsize, then
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
        WORD visible = (UWORD)(BW(box, B_H) - 3) >> 3, first;
        WORD half;                              /* d0w: half the lines */
        WORD maxfirst;                          /* d0w: the highest first entry */
        UBYTE *entry;

        lines = visible;
        half = (UWORD)visible >> 1;
        first = BW(box, B_VALUE) - half;
        if (first < 0)
            first = 0;
        maxfirst = BW(box, B_RANGE) - lines;
        if (maxfirst < 0) {
            maxfirst = 0;
            lines = BW(box, B_RANGE);
        }
        if (first - maxfirst >= 0)              /* CMP d0,d2:BMI noj */
            first = maxfirst;
        count = BW(box, B_VALUE) - first;
        entry = BP(box, B_TEXT);
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
        ULONG tlen = BL(box, B_TLEN), tptr = BL(box, B_TEXT); /* d0 d1 */
        ULONG ptr, len;                         /* d0 d1 after the EXG */

        ptr = tlen;                             /* no text: d0 = 0, d1 = B_TEXT */
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
                if (BW(box, B_PROMPT) >= 0) {
                    BL_SETW(row, row + 8);
                    BL_SETW(cx, cx - (UWORD)len);
                }
                BL_SETW(row, row + 2);
                locate(cx, BL_SWAP(row) >> 3);
                goto pme;
            }
            if (btype != 6 && BW(box, B_RANGE)) {
                /* the value-th of the strings */
                UWORD index = BW(box, B_VALUE);
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
    WORD color;

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
    color = p->c2;
    if (type & 1)                               /* ASR #1: carry */
        color = ~color;
    type >>= 1;
    BW(b, B_BTYPE) = type;
    BW(b, B_COLOR) = color;
    if (type == 2) {
        /* a string box: spaces */
        UWORD len = (UWORD)w >> 3;
        UBYTE *fill;
        UWORD c;

        if (!len)                               /* EXT.l:BLE err4 */
            BLITZ_RUNERR(mess4);
        BL(b, B_TLEN) = len;
        fill = globalloc(len, 0);
        BP(b, B_TEXT) = fill;
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
        BW(b, B_VALUE) = valx;
        BW(b, B_RANGE) = valy;
        BW(b, B_PROMPT) = knobw;
        BW(b, B_ANSLEN) = knobh;
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
    UBYTE *dst;
    UWORD strings;

    if (BW(b, B_BTYPE) == 2) {
        /* strya: the prompt of a string box */
        BW(b, B_PROMPT) = (WORD)len;
        if ((WORD)BL(b, B_TLEN) <= (WORD)len)   /* CMP d0,d1:BLE err4 */
            BLITZ_RUNERR(mess4);
        dst = BP(b, B_TEXT);
        while (*s)
            *dst++ = *s++;
        return;
    }
    len++;
    BL(b, B_TLEN) = len;
    dst = globalloc(len, 0);
    BP(b, B_TEXT) = dst;
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
    if (BW(b, B_BTYPE) - 2 < 0) {               /* CMP #2:BPL stdun */
        BW(b, B_RANGE) = strings;
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
    WORD value = (WORD)v;

    if (value < 0)
        value += BW(b, B_RANGE);
    if ((UWORD)value >= (UWORD)BW(b, B_RANGE))
        BLITZ_RUNERR(mess3);
    BW(b, B_VALUE) = value;
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
    ULONG len = lens[-1];
    ULONG pad = BL(b, B_TLEN);
    UBYTE *dst = BP(b, B_TEXT);

    BL_SETW(pad, pad - BW(b, B_PROMPT));
    BL_SETW(pad, pad - len);
    if ((WORD)pad <= 0)
        BLITZ_RUNERR(mess4);
    dst += BW(b, B_PROMPT);
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

/* BoxValue: MOVE value,d0 (low word) */
static ULONG BLITZ_ROUTINE boxvalue_c(ULONG n BL_REG(d0))
{
    return (n & 0xffff0000UL) | (UWORD)BW(getbox(PG, n), B_VALUE);
}
BLITZ_THUNK(_boxvalue, boxvalue_c, "d1/a1");

static ULONG BLITZ_ROUTINE boxvalue2_c(ULONG callerd0 BL_REG(d0), ULONG n BL_REG(d1))
{
    BL_SETW(callerd0, n);                       /* MOVE d1,d0 */
    return (callerd0 & 0xffff0000UL) | (UWORD)BW(getbox(PG, callerd0), B_VALUE);
}
BLITZ_THUNK(_boxvalue2, boxvalue2_c, "d1/a1");

/* BoxString: page in a1, result at a3 (no 0 written; a3 = the end) */
static ULONG boxstring(struct page *p, ULONG n)
{
    UBYTE *b = getbox(p, n);
    WORD prompt = BW(b, B_PROMPT);
    ULONG len = BL(b, B_TLEN);
    const UBYTE *answer = BP(b, B_TEXT) + prompt;
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

static ULONG BLITZ_ROUTINE boxstring_c(ULONG n BL_REG(d0), struct page *p BL_REG(a1))
{
    return boxstring(p, n);
}
BLITZ_THUNK(_boxstring, boxstring_c, "d1");

static ULONG BLITZ_ROUTINE boxstring2_c(ULONG callerd0 BL_REG(d0), ULONG n BL_REG(d1),
                                        struct page *p BL_REG(a1))
{
    BL_SETW(callerd0, n);
    return boxstring(p, callerd0);
}
BLITZ_THUNK(_boxstring2, boxstring2_c, "d1");

/* BoxSelected: (selected - memloc) >> 5 (LSR.w), -1 for none */
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
    ULONG len;

    chkpage(p);
    if ((UWORD)n >= p->numboxes)
        BLITZ_RUNERR(mess2);
    b = p->memloc + (WORD)(UWORD)(n << 5);
    if (BW(b, B_BTYPE))
        BLITZ_RUNERR(mess5);
    len = lens[-1];
    if ((UWORD)BW(b, B_H) < 16) {
        /* doleft: scroll the line left, print at the right */
        WORD x = BW(b, B_X), y = BW(b, B_Y), w = BW(b, B_W);
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
        printn(s, len);
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
    WORD dx, dy;

    if (BW(b, B_BTYPE) <= 0)
        return 0;
    dx = BW(b, B_X) - px;
    if (dx > 0)
        return 0;
    dy = BW(b, B_Y) - py;
    if (dy > 0)
        return 0;
    dx = -dx;
    dy = -dy;
    if (BW(b, B_W) <= dx)
        return 0;
    return BW(b, B_H) > dy;
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
    UBYTE *cur, *b, *best;
    WORD edgex, edgey;
    UWORD bestdist, n;

    chkpage(p);
    cur = p->selected;
    if (!cur)
        return;
    best = cur;
    b = p->memloc;
    edgex = BW(cur, B_X);
    edgey = BW(cur, B_Y);
    if (dx > 0)
        edgex += BW(cur, B_W);
    if (dy > 0)
        edgey += BW(cur, B_H) - 1;
    bestdist = 8192;
    n = p->numboxes - 1;
    do {
        WORD distx, disty;

        if (b == cur || BW(b, B_BTYPE) <= 0)
            goto mis;
        distx = BW(b, B_X) - edgex;
        if (dx < 0)
            distx = -distx;
        if (distx < 0) {
            if (dx)
                goto mis;
            distx = -distx;
        }
        disty = BW(b, B_Y) - edgey;
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
        b += 32;
    } while (n-- != 0);
    if (best == cur)
        return;
    p->selected = best;
    drawbox(p, cur);
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
    UBYTE *pos = *(UBYTE **)(bm + 8);
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
static void edstring(struct page *p, UBYTE *b)
{
    UBYTE *bm = p->mybitmap;
    ULONG old = *(ULONG *)(bm + 40);
    WORD prompt = BW(b, B_PROMPT);
    ULONG len = BL(b, B_TLEN);

    *(ULONG *)(bm + 40) = *(ULONG *)edxyc;
    BL_SETW(len, len - prompt);
    BL_SETW(len, len - 1);
    printn(BP(b, B_TEXT) + prompt, len);
    *(ULONG *)(bm + 40) = old;
}

static void activateselected(struct page *p)
{
    UBYTE *b;
    WORD type;

    chkpage(p);
    b = p->selected;
    if (!b)
        return;
    type = BW(b, B_BTYPE);
    if (type == 1 || type == 5) {
        drawbox(p, b);                          /* button up now cycles */
    } else if (type == 2) {
        edxyc[0] = (WORD)((((UWORD)(BW(b, B_X) + 11) >> 3) + BW(b, B_PROMPT)) << 3);
        edxyc[1] = BW(b, B_Y) + 2;
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
    ULONG index = 0;

    do {
        if (inbox(b, mx, my))
            return ((ULONG)(LONG)mx & 0xffff0000UL) | (UWORD)index; /* MOVE d6,d0 */
        index++;
        b += 32;
    } while (n-- != 0);
    return (ULONG)-1;
}
BLITZ_THUNK(_spotbox, spotbox_c, "d1/a0-a1");

/* MouseEnabled: MOVE mo,d0 (low word) / 0 while dragging */
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
    UBYTE *box;

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
        if (BW(box, B_BTYPE) == 5)
            goto movelist;
    }
    {
        WORD i = (WORD)(status << 2) / 2;
        selectbox2(p, delt[i], delt[i + 1]);
    }
    goto dobutton;

movelist:
    {
        WORD v = BW(box, B_VALUE) + status - 2, r = BW(box, B_RANGE);

        if (v < 0)
            v = 0;
        if (v - r >= 0)                         /* CMP d1,d0:BMI fop */
            v = r - 1;
        BW(box, B_VALUE) = v;
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
        last = (WORD)BL(box, B_TLEN) - BW(box, B_PROMPT) - 1;
        if (status == 2 && --cursor < 0)
            cursor = 0;
        if (status == 4) {
            cursor++;
            if (cursor - last >= 0)             /* CMP d1,d0:BMI nri */
                cursor--;
        }
        edxyc[2] = cursor;
        if (status == 1 || status == 3) {
            UBYTE *chr = BP(box, B_TEXT) + BW(box, B_PROMPT) + cursor;

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
        if (!box || BW(box, B_COLOR) >= 0)
            goto noselect;
        BW(box, B_COLOR) = ~BW(box, B_COLOR);
        if (BW(box, B_BTYPE) == 1) {
            UWORD v = BW(box, B_VALUE) + 1;

            if (v >= (UWORD)BW(box, B_RANGE))
                v = 0;
            BW(box, B_VALUE) = v;
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
        BW(box, B_COLOR) = ~BW(box, B_COLOR);
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
        answer = BP(box, B_TEXT) + BW(box, B_PROMPT);
        last = (WORD)BL(box, B_TLEN) - BW(box, B_PROMPT) - 1;
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
    BW(box, B_VALUE) = mx - ox;
    BW(box, B_RANGE) = my - oy;
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
        valx = BW(box, B_VALUE);
        valy = BW(box, B_RANGE);
        if (mo) {
            WORD trackw = BW(box, B_W), trackh = BW(box, B_H);
            WORD knobw = BW(box, B_PROMPT), knobh = BW(box, B_ANSLEN);

            if (valx >= 0) {
                valx = mx;
                trackw = trackw - 4 - knobw;
                valx = valx - BW(box, B_X) - 2;
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
                valy = valy - BW(box, B_Y) - 2;
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
        BW(box, B_VALUE) = valx;
        BW(box, B_RANGE) = valy;
        drawbox(p, box);
        return 2;
    }
}

static ULONG BLITZ_ROUTINE blitzevent_c(void)
{
    return (ULONG)blitzevent(PG);
}
BLITZ_THUNK(_blitzevent, blitzevent_c, "d1/a0-a1");
