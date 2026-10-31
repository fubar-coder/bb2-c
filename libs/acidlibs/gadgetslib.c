/*
 * gadgetslib - command library 140: Intuition gadgets - TextGadget,
 * StringGadget, PropGadget, ShapeGadget, SetHProp/SetVProp, HPropPot ...
 * VPropBody, Redraw, Toggle, GadgetBorder, Borders, ActivateString,
 * ResetString, StringText$, ClearString, GadgetPens, BorderPens,
 * GadgetJam, SelectMode, SetString, ButtonGroup, GadgetStatus, ButtonId,
 * Enable, Disable, SetGadgetStatus, SetTextGadget.
 *
 * C translation of bblibsrc/gadgetslib (binary of the CD's acidlibs
 * checked: same as the source).  An object library (!libfin{_toke,...,
 * 5,2}): the Blitz object "GadgetList" (dumtoke), objects of 4 bytes = the
 * pointer to the first gadget; _free = freegadget.  The routines get the
 * list in a3 (#gadgetslib,$1380 - blitz_string) or a1 ($1180).
 *
 * Gadgets (44 bytes), IntuiTexts (24 bytes: +20 = start of the text),
 * Borders (2 x 36 bytes: a highlight and a shadow border of 5 points
 * each), StringInfo (36), PropInfo (22) and knob Images (20) come from
 * memlib globalloc (MEMF_PUBLIC|MEMF_CLEAR, the string buffer
 * MEMF_PUBLIC only), texts from maxslib allocst.  Every gadget gets
 * UserData = "bL" in the high word and the button group in the low word.
 * A text gadget with "|" in its text cycles through the parts
 * (SetGadgetStatus/GadgetStatus): findmaxlen replaces the "|" by 0 and
 * makes the gadget as wide as the widest part.
 *
 * Kept from the original (BUG(orig)):
 *   - freegadget never frees the SpecialInfo (StringInfo + buffer,
 *     PropInfo): the gadget type is loaded and then overwritten by
 *     MOVEQ #22,d0 before the type compares. (#163)
 *   - StringGadget with Borders off writes the border offsets to the
 *     addresses 0, 2, 36 and 38 (border pointer 0). (#164)
 *   - StringGadget with Maxlen 0 clears 65536 bytes of the (0 byte)
 *     buffer (DBF). (#165)
 *   - Not a bug (word functions, the compiler uses only d0.w):
 *     GadgetStatus of a toggle/non-text gadget: SNE/EXT.w, the upper word
 *     of d0 is that of the GadgetText pointer; ButtonId: the upper word of
 *     d0 is that of the gadget pointer.
 *   - findgadget does not stop at the end of the list (only the error
 *     checks of the debugger do) (#166); TextGadget with an empty text has
 *     IText 0 (allocst of length 0; not a bug: Intuition prints the empty
 *     string at address 0, no effect seen).
 *   - makeintuishape: depth 0 gives PlanePick $FF (DBF 65536 times). (#167)
 * Registers: the original changes about everything; plain C functions,
 * the error checks keep d0-d1/a0-a1 (more than the original).
 */
#include "blitzlib.h"
#include <intuition/intuition.h>
#define __NOLIBBASE__
#define INTUITION_BASE_NAME intbase
#include <proto/intuition.h>
#undef __NOLIBBASE__

/* maxslib: string copies in global memory */

/* entry points (defined below) */
void BLITZ_ROUTINE dotextgadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                const UBYTE *text BL_REG(d5),
                                struct TextAttr **font BL_REG(a1),
                                const ULONG *lens BL_REG(a2),
                                struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dostringgadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                  ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                  UWORD maxlen BL_REG(d5),
                                  const UWORD *args BL_REG(a2));
void BLITZ_ROUTINE dopropgadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                UWORD w BL_REG(d5), const UWORD *args BL_REG(a2));
void BLITZ_ROUTINE doshapegadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                 ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                 const UBYTE *shape BL_REG(a2));
void BLITZ_ROUTINE doshapegadget2(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                  ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                  const UBYTE *alt BL_REG(a1),
                                  const UBYTE *shape BL_REG(a2));
void dosethprop(void);
void dosetvprop(void);
void dohproppot(void);
void dohpropbody(void);
void dovproppot(void);
void dovpropbody(void);
void BLITZ_ROUTINE doredraw(UWORD id BL_REG(d1), struct Window **win BL_REG(a1),
                             struct Library *intbase BL_REG(a6));
void doselect(void);
void doselect2(void);
void BLITZ_ROUTINE dogborder(WORD x BL_REG(d0), WORD y BL_REG(d1),
                             WORD w BL_REG(d2), WORD h BL_REG(d3),
                             struct Window **win BL_REG(a1),
                             struct Library *intbase BL_REG(a6));
void dogb2(void);
void dogb(void);
ULONG BLITZ_ROUTINE dostringactivate(UWORD id BL_REG(d1),
                                     struct Window **win BL_REG(a1),
                                     struct Library *intbase BL_REG(a6));
void dostringreset(void);
void dostringtext(void);
void dostringclear(void);
void dogpens2(void);
void dogpens(void);
void dobpens(void);
void dogjam(void);
void doselectmode(void);
void dosetstring(void);
void dobgroup(void);
void dogstatus(void);
void dobuttonid(void);
void doenable(void);
void dodisable(void);
void setdogstatus(void);
void settextgadget(void);
void codeinit(void);
void BLITZ_ROUTINE freegadget(void);
/* error checks (debugger only, thunks below) */
void sgchk(void), sgchk2(void), propchk(void), redrawchk(void), gchk(void);
void wchk(void), actchk(void), reschk(void), settest(void), bgroupchk(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#gadgetslib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_gadgetslib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_LONG, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_gadgetslib, 0x1380, LIB_intuitionlib, BL_LA(6), LIB_intuifontlib, BL_LA(1));
BLITZ_SUBS(dotextgadget, 0, 0);
BLITZ_NAME("TextGadget", "GadgetList#,X,Y,Flags,Id,Text$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1380);
BLITZ_SUBS(dostringgadget, 0, 0);
BLITZ_NAME("StringGadget", "GadgetList#,X,Y,Flags,Id,Maxlen,Width");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1380);
BLITZ_SUBS(dopropgadget, 0, 0);
BLITZ_NAME("PropGadget", "GadgetList#,X,Y,Flags,Id,Width,Height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1380, LIB_shapeslib, 0x1285);
BLITZ_SUBS(doshapegadget, sgchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1380, LIB_shapeslib, 0x1285, LIB_shapeslib, 0x1186);
BLITZ_SUBS(doshapegadget2, sgchk2, 0);
BLITZ_NAME("ShapeGadget", "GadgetList#,X,Y,Flags,Id,Shape#[,Shape#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_QUICK, BL_QUICK);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dosethprop, propchk, 0);
BLITZ_NAME("SetHProp", "GadgetList#,Id,Pot,Body");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_QUICK, BL_QUICK);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dosetvprop, propchk, 0);
BLITZ_NAME("SetVProp", "GadgetList#,Id,Pot,Body");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dohproppot, propchk, 0);
BLITZ_NAME("HPropPot", "(GadgetList#,Id)");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dohpropbody, propchk, 0);
BLITZ_NAME("HPropBody", "(GadgetList#,Id)");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dovproppot, propchk, 0);
BLITZ_NAME("VPropPot", "(GadgetList#,Id)");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dovpropbody, propchk, 0);
BLITZ_NAME("VPropBody", "(GadgetList#,Id)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1180, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(doredraw), redrawchk, 0);             /* Amiga mode only */
BLITZ_NAME("Redraw", "Window#,Id");

BLITZ_NULLSUB(0, 0, 0);

BLITZ_NULLSUB(0, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(doselect, gchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(doselect2, gchk, 0);
BLITZ_NAME("Toggle", "GadgetList#,Id [,On|Off]");

BLITZ_NULLSUB(0, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dogborder), wchk, 0);
BLITZ_NAME("GadgetBorder", "X,Y,Width,Height");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dogb2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dogb, 0, 0);
BLITZ_NAME("Borders", "[On|Off]|[Width,Height]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1180, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dostringactivate), actchk, 0);
BLITZ_NAME("ActivateString", "Window#,Id");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dostringreset, reschk, 0);
BLITZ_NAME("ResetString", "GadgetList#,Id");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dostringtext, reschk, 0);
BLITZ_NAME("StringText$", "(GadgetList#,Id)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dostringclear, reschk, 0);
BLITZ_NAME("ClearString", "GadgetList#,Id");

BLITZ_DUMTOKE(_toke, "GadgetList", "List of Gadgets");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dogpens2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dogpens, 0, 0);
BLITZ_NAME("GadgetPens", "Foreground Colour[,Background Colour]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dobpens, 0, 0);
BLITZ_NAME("BorderPens", "Highlight Colour,Shadow Colour");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dogjam, 0, 0);
BLITZ_NAME("GadgetJam", "Jammode");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doselectmode, 0, 0);
BLITZ_NAME("SelectMode", "1=Box, 0=Inverse");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dosetstring, settest, 0);
BLITZ_NAME("SetString", "GadgetList#,Id,String$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dobgroup, bgroupchk, 0);
BLITZ_NAME("ButtonGroup", "Group");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dogstatus, gchk, 0);
BLITZ_NAME("GadgetStatus", "(GadgetList#,Id)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dobuttonid, 0, 0);
BLITZ_NAME("ButtonId", "(GadgetList#,ButtonGroup)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(doenable, gchk, 0);
BLITZ_NAME("Enable", "(GadgetList#,Id)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(dodisable, gchk, 0);
BLITZ_NAME("Disable", "(GadgetList#,Id)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(setdogstatus, gchk, 0);
BLITZ_NAME("SetGadgetStatus", "(GadgetList#,Id,Value)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(settextgadget, gchk, 0);
BLITZ_NAME("SetTextGadget", "GadgetList#,Id,Text$");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freegadget, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,5,2} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 2);

/* ***************** Code Below ******************* */

#define BORDERSIZ 36
#define BORDERMEM (BORDERSIZ * 2)

/* struct Gadget (44) as the original sees it */
#define G_LEFT      4
#define G_TOP       6
#define G_WIDTH     8
#define G_HEIGHT    10
#define G_FLAGS     12          /* byte 12: DISABLED, byte 13: SELECTED ... */
#define G_ACT       14          /* byte 14 bit 0: TOGGLESELECT */
#define G_TYPE      16
#define G_RENDER    18
#define G_SELECT    22
#define G_TEXT      26
#define G_SPECIAL   34
#define G_ID        38
#define G_USER      40          /* "bL" */
#define G_GROUP     42
#define GADGETSIZE  44

#define W(p, o)  (*(WORD *)((UBYTE *)(p) + (o)))
#define UW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define L(p, o)  (*(ULONG *)((UBYTE *)(p) + (o)))
#define B(p, o)  (*(UBYTE *)((UBYTE *)(p) + (o)))
#define P(p, o)  (*(UBYTE **)((UBYTE *)(p) + (o)))

/* the variables in the code of the original, same layout (long moves
   go over several of them) */
static struct {
    UBYTE fpen, bpen;           /* intuitext pens (word access) */
    UBYTE bco1, bco2;           /* border pens: left/top, right/bottom */
    UBYTE mode, pad;            /* jam mode */
    WORD bxxs, byxs;            /* border x/y excess */
    WORD fhit;                  /* font height */
    WORD bordersok;
    WORD bxxs2, byxs2;
    ULONG fontattr;
    WORD selmode, defflags, bgroup;
} BL_PACKED g = { 1, 0, 1, 2, 0, 0, 4, 2, 8, -1, 4, 2, 0, 0, 0, 0 };

/* maxslib / memlib */
static UBYTE *allocst(const UBYTE *s, ULONG len)
{
    struct bl_regs r = { .d0 = (ULONG)s, .d1 = len };

    return (UBYTE *)BLITZ_ALIBJSR(TOK_allocst, r);
}

static void freest(ULONG str)
{
    struct bl_regs r = { .d0 = str };

    BLITZ_ALIBJSR(TOK_freest, r);
}

static UBYTE *globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(void *mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = (ULONG)mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* findgadget - gadget `id` of the list (no end check), system gadgets
   (GadgetType bit 15) skipped */
static UBYTE *findgadget(UBYTE **list, UWORD id)
{
    UBYTE *gad = (UBYTE *)list;

    do
        gad = P(gad, 0);
    while (UW(gad, G_ID) != id || W(gad, G_TYPE) < 0);
    return gad;
}

/* ------------------------------------------------------------ settings */

static void BLITZ_ROUTINE codeinit_c(void)
{
    g.bgroup = 1;
    g.fpen = 1; g.bpen = 0; g.bco1 = 1; g.bco2 = 2;     /* $01000102 */
    g.mode = 1;
    g.bxxs = 4; g.byxs = 2;
    g.bxxs2 = 4; g.byxs2 = 2;
    g.fhit = 8;
    g.bordersok = -1;
    g.fontattr = 0;
}
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dobgroup_c(UWORD grp BL_REG(d0))
{
    g.bgroup = grp;
}
BLITZ_THUNK(dobgroup, dobgroup_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE doselectmode_c(UWORD on BL_REG(d0))
{
    g.selmode = 0;
    if (on)
        g.selmode++;
}
BLITZ_THUNK(doselectmode, doselectmode_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dogjam_c(UBYTE mode BL_REG(d0))
{
    g.mode = mode;
}
BLITZ_THUNK(dogjam, dogjam_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dogpens_c(UBYTE fg BL_REG(d0), UBYTE bg BL_REG(d1))
{
    g.bpen = bg;
    g.fpen = fg;
}
BLITZ_THUNK(dogpens, dogpens_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dogpens2_c(UBYTE fg BL_REG(d0))
{
    g.fpen = fg;
}
BLITZ_THUNK(dogpens2, dogpens2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dobpens_c(UBYTE light BL_REG(d0), UBYTE shadow BL_REG(d1))
{
    g.bco1 = light;
    g.bco2 = shadow;
}
BLITZ_THUNK(dobpens, dobpens_c, "d0-d1/a0-a1");

/* Borders w,h */
static void BLITZ_ROUTINE dogb_c(UWORD w BL_REG(d0), UWORD h BL_REG(d1))
{
    g.bxxs = w;
    g.byxs = h;
    g.bxxs2 = g.bxxs;
    g.byxs2 = g.byxs;
}
BLITZ_THUNK(dogb, dogb_c, "d0-d1/a0-a1");

/* Borders on/off */
static void BLITZ_ROUTINE dogb2_c(WORD on BL_REG(d0))
{
    if (on) {
        if (g.bordersok)
            return;
        g.bordersok = ~g.bordersok;
        g.bxxs = g.bxxs2;
        g.byxs = g.byxs2;
    } else {
        if (!g.bordersok)
            return;
        g.bordersok = on;
        g.bxxs2 = g.bxxs;
        g.byxs2 = g.byxs;
        g.bxxs = 0;
        g.byxs = 0;
    }
}
BLITZ_THUNK(dogb2, dogb2_c, "d0/a0-a1");

/* --------------------------------------------------- allocation helpers */

/* allocborder2 - two Borders (highlight/shadow, 5 points each) for a box
   of w x h */
static UBYTE *allocborder2(WORD w, WORD h)
{
    UBYTE *border = globalloc(BORDERMEM, MEMF_PUBLIC | MEMF_CLEAR);

    B(border, 4) = g.bco1;
    B(border, 4 + 36) = g.bco2;
    B(border, 7) = 5;
    B(border, 7 + 36) = 5;
    P(border, 8) = border + 16;
    P(border, 12) = border + 36;
    P(border, 8 + 36) = border + 16 + 36;
    w--;
    h--;
    W(border, 16) = w;
    W(border, 26) = h;
    W(border, 30) = h - 1;
    W(border, 28) = 1;
    W(border, 32) = 1;
    W(border, 34) = 1;
    W(border, 36 + 18) = h;
    W(border, 36 + 22) = h;
    W(border, 36 + 34) = h;
    W(border, 36 + 30) = 1;
    W(border, 36 + 16) = 1;
    W(border, 36 + 20) = w;
    W(border, 36 + 24) = w;
    W(border, 36 + 28) = w - 1;
    W(border, 36 + 32) = w - 1;
    return border;
}

/* allocborder - 0 if borders are off */
static UBYTE *allocborder(WORD w, WORD h)
{
    if (!g.bordersok)
        return NULL;
    return allocborder2(w, h);
}

/* allocgadget - a new gadget at the end of the list in a3 */
static UBYTE *allocgadget(WORD x, WORD y, UWORD id, WORD w, WORD h)
{
    UBYTE *gad = globalloc(GADGETSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    UBYTE *prev, *next = blitz_string;

    do {
        prev = next;
        next = P(prev, 0);
    } while (next);
    P(prev, 0) = gad;
    W(gad, G_LEFT) = x;
    W(gad, G_TOP) = y;
    W(gad, G_WIDTH) = w;
    W(gad, G_HEIGHT) = h;
    W(gad, G_FLAGS) = g.defflags;
    UW(gad, G_ID) = id;
    W(gad, G_ACT) = 3;                  /* RELVERIFY|GADGIMMEDIATE */
    UW(gad, G_USER) = 0x624c;           /* "bL": blitz type gadget */
    W(gad, G_GROUP) = 0;
    return gad;
}

/* allocintui - IntuiText (24 bytes) with a copy of the text */
static UBYTE *allocintui(const UBYTE *s, ULONG len)
{
    UBYTE *copy = allocst(s, len);
    UBYTE *it = globalloc(24, MEMF_PUBLIC | MEMF_CLEAR);

    P(it, 12) = copy;
    P(it, 20) = copy;
    UW(it, 0) = UW(&g, 0);              /* fpen, bpen */
    B(it, 2) = g.mode;
    L(it, 8) = g.fontattr;
    return it;
}

/* handleflags - d3 = the flags argument (word shifts as the original) */
static void handleflags(UBYTE *gad, ULONG flags)
{
    UWORD lowflags;

    if (flags & (1UL << 16))
        B(gad, G_TYPE) |= 1 << 5;       /* GZZGADGET */
    if (flags & (1UL << 15))
        B(gad, G_ACT + 1) |= 1 << 7;    /* BOTTOMBORDER */
    if (flags & (1UL << 14))
        B(gad, G_ACT + 1) |= 1 << 6;    /* TOPBORDER */
    if (flags & (1UL << 13))
        B(gad, G_ACT + 1) |= 1 << 5;    /* LEFTBORDER */
    if (flags & (1UL << 12))
        B(gad, G_ACT + 1) |= 1 << 4;    /* RIGHTBORDER */
    if (flags & (1UL << 9))
        W(gad, G_GROUP) = g.bgroup;
    lowflags = (UWORD)flags;
    if (lowflags & 1)
        B(gad, G_ACT) |= 1;             /* toggle */
    if (lowflags & 2)
        B(gad, G_FLAGS + 1) |= 1 << 4;  /* rel right */
    if (lowflags & 4)
        B(gad, G_FLAGS + 1) |= 1 << 3;  /* rel bottom */
    if (lowflags & 8)
        B(gad, G_FLAGS + 1) |= 1 << 5;  /* rel width */
    if (lowflags & 16)
        B(gad, G_FLAGS + 1) |= 1 << 6;  /* rel height */
    if (lowflags & 32)
        B(gad, G_FLAGS + 1) |= 1 << 0;  /* box select */
}

/* findmaxlen - text gadget: the "|" become 0, result = the width of the
   widest part (IntuiTextLength) */
static WORD findmaxlen(UBYTE *it, struct Library *intbase)
{
    UBYTE *start = P(it, 12), *part = start, *end;
    WORD maxwidth = 0, width;
    UBYTE c;

    for (;;) {
        end = part;
        while ((c = *end++) != 0 && c != '|')
            ;
        if (!c)
            break;
        end[-1] = 0;
        P(it, 12) = part;
        width = (WORD)IntuiTextLength((struct IntuiText *)it);
        part = end;
        if (width > maxwidth)
            maxwidth = width;
    }
    P(it, 12) = part;
    width = (WORD)IntuiTextLength((struct IntuiText *)it);
    if (width > maxwidth)
        maxwidth = width;
    P(it, 12) = start;
    return maxwidth;
}

/* ------------------------------------------------------------- gadgets */

/* TextGadget l,x,y,flags,id,text$ (a1 = intuifontlib's current font) */
void BLITZ_ROUTINE dotextgadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                const UBYTE *text BL_REG(d5),
                                struct TextAttr **font BL_REG(a1),
                                const ULONG *lens BL_REG(a2),
                                struct Library *intbase BL_REG(a6))
{
    UBYTE *it, *border, *gad;
    WORD w, boxw, boxh;
    WORD xs, ys;                        /* the border excess */

    /* fontcalc */
    g.fontattr = (ULONG)*font;
    g.fhit = W(*font, 4);
    it = allocintui(text, lens[-1]);
    w = findmaxlen(it, intbase);
    xs = g.bxxs;
    ys = g.byxs;
    W(it, 4) = xs;
    W(it, 6) = ys;
    boxw = xs + xs + w;
    boxh = ys + ys + g.fhit;
    border = allocborder(boxw, boxh);
    gad = allocgadget(x, y, id, boxw, boxh);
    P(gad, G_RENDER) = border;
    P(gad, G_TEXT) = it;
    W(gad, G_TYPE) = 1;                 /* BOOLGADGET */
    handleflags(gad, flags);
}

/* StringGadget l,x,y,flags,id,maxlen,width (width: -4(a2)) */
void BLITZ_ROUTINE dostringgadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                  ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                  UWORD maxlen BL_REG(d5),
                                  const UWORD *args BL_REG(a2))
{
    WORD width = (WORD)args[-1];        /* MOVE -(a2),d6: low word */
    UBYTE *border, *gad, *si, *buf;
    WORD offset;
    UWORD n;

    border = allocborder(width + g.bxxs + g.bxxs, g.fhit + g.byxs + g.byxs);
    __asm__("" : "+a"(border));         /* may be 0, see above */
    offset = g.bxxs;
    x += offset;
    offset = -offset;
    *(volatile WORD *)(border + 0) = offset;        /* border 0: BUG(orig) (#164) */
    *(volatile WORD *)(border + BORDERSIZ) = offset;
    offset = g.byxs;
    y += offset;
    offset = -offset;
    *(volatile WORD *)(border + 2) = offset;
    *(volatile WORD *)(border + 2 + BORDERSIZ) = offset;
    gad = allocgadget(x, y, id, width, g.fhit);
    W(gad, G_TYPE) = 4;                 /* STRGADGET */
    handleflags(gad, flags);
    P(gad, G_RENDER) = border;
    si = globalloc(36, MEMF_PUBLIC | MEMF_CLEAR);
    P(gad, G_SPECIAL) = si;
    buf = globalloc(maxlen, MEMF_PUBLIC);
    P(si, 0) = buf;
    UW(si, 10) = maxlen;
    n = maxlen - 1;                     /* DBF: maxlen 0 clears 65536 */
    do
        *buf++ = 0;
    while (n-- != 0);
}

/* PropGadget l,x,y,flags,id,width,height (height: -4(a2)) */
void BLITZ_ROUTINE dopropgadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                UWORD w BL_REG(d5), const UWORD *args BL_REG(a2))
{
    UBYTE *gad = allocgadget(x, y, id, w, args[-1]);
    UBYTE *pi = globalloc(22, MEMF_PUBLIC | MEMF_CLEAR);
    UWORD shifted, piflags = 1;         /* AUTOKNOB */

    W(gad, G_TYPE) = 3;                 /* PROPGADGET */
    handleflags(gad, flags);
    shifted = (UWORD)flags >> 6;        /* handleflags shifted it 6 times */
    if (shifted & 1)
        piflags |= 2;                   /* FREEHORIZ */
    if (shifted & 2)
        piflags |= 4;                   /* FREEVERT */
    if (shifted & 4)
        piflags |= 8;                   /* PROPBORDERLESS */
    piflags |= 16;                      /* PROPNEWLOOK */
    UW(pi, 0) = piflags;
    L(pi, 2) = 0x80008000;
    L(pi, 6) = 0x80008000;
    P(gad, G_SPECIAL) = pi;
    P(gad, G_RENDER) = globalloc(20, MEMF_PUBLIC | MEMF_CLEAR);
    B(gad, G_FLAGS + 1) |= 1 << 2;      /* GADGIMAGE */
}

/* makeintuishape - an Image (20 bytes) for the shape (width, height,
   depth at 0/2/4, data at 14) */
static UBYTE *makeintuishape(const UBYTE *shape)
{
    UBYTE *img = globalloc(20, MEMF_PUBLIC | MEMF_CLEAR);
    UWORD planepick = 0, n;

    W(img, 4) = W(shape, 0);
    W(img, 6) = W(shape, 2);
    W(img, 8) = W(shape, 4);
    L(img, 10) = L(shape, 14);
    n = UW(shape, 4) - 1;
    do
        planepick = (UWORD)(planepick << 1) | 1;
    while (n-- != 0);
    B(img, 14) = (UBYTE)planepick;
    B(img, 15) = 0;
    return img;
}

static void shapegadget(WORD x, WORD y, ULONG flags, UWORD id,
                        const UBYTE *shape, UBYTE *alt)
{
    UBYTE *img, *gad;

    x -= W(shape, 10);                  /* handle */
    y -= W(shape, 12);
    img = makeintuishape(shape);
    gad = allocgadget(x, y, id, W(shape, 0), W(shape, 2));
    W(gad, G_TYPE) = 1;
    B(gad, G_FLAGS + 1) |= 1 << 2;      /* GADGIMAGE */
    P(gad, G_RENDER) = img;
    P(gad, G_SELECT) = alt;
    if (alt)
        B(gad, G_FLAGS + 1) |= 1 << 1;  /* GADGHIMAGE */
    handleflags(gad, flags);
}

/* ShapeGadget l,x,y,flags,id,shape (a2 = the shape) */
void BLITZ_ROUTINE doshapegadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                 ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                 const UBYTE *shape BL_REG(a2))
{
    shapegadget(x, y, flags, id, shape, NULL);
}

/* ShapeGadget l,x,y,flags,id,shape,alt (a1 = the alternative shape) */
void BLITZ_ROUTINE doshapegadget2(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                  ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                  const UBYTE *alt BL_REG(a1),
                                  const UBYTE *shape BL_REG(a2))
{
    UBYTE *img = makeintuishape(alt);

    shapegadget(x, y, flags, id, shape, img);
}

/* ---------------------------------------------------------- prop gadgets */

/* setprop2 - pot/body (quick, 1.0 = $10000 -> $FFFF) */
static void setprop(UWORD id, ULONG pot, ULONG body, UBYTE **list, int off)
{
    UBYTE *pi;

    if (pot >= 0x10000)
        pot = (pot & 0xffff0000UL) | 0xffff;
    if (body >= 0x10000)
        body = (body & 0xffff0000UL) | 0xffff;
    pi = P(findgadget(list, id), G_SPECIAL);
    UW(pi, off) = (UWORD)pot;
    UW(pi, off + 4) = (UWORD)body;
}

static void BLITZ_ROUTINE dosethprop_c(UWORD id BL_REG(d1), ULONG pot BL_REG(d2),
                              ULONG body BL_REG(d3), UBYTE **list BL_REG(a1))
{
    setprop(id, pot, body, list, 2);
}
BLITZ_THUNK(dosethprop, dosethprop_c, "d0-d1/a1");

static void BLITZ_ROUTINE dosetvprop_c(UWORD id BL_REG(d1), ULONG pot BL_REG(d2),
                              ULONG body BL_REG(d3), UBYTE **list BL_REG(a1))
{
    setprop(id, pot, body, list, 4);
}
BLITZ_THUNK(dosetvprop, dosetvprop_c, "d0-d1/a1");

static ULONG readprop(UWORD id, UBYTE **list, int off)
{
    return UW(P(findgadget(list, id), G_SPECIAL), off);
}

static ULONG BLITZ_ROUTINE dohproppot_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    return readprop(id, list, 2);
}
BLITZ_THUNK(dohproppot, dohproppot_c, "a1");

static ULONG BLITZ_ROUTINE dohpropbody_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    return readprop(id, list, 6);
}
BLITZ_THUNK(dohpropbody, dohpropbody_c, "a1");

static ULONG BLITZ_ROUTINE dovproppot_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    return readprop(id, list, 4);
}
BLITZ_THUNK(dovproppot, dovproppot_c, "a1");

static ULONG BLITZ_ROUTINE dovpropbody_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    return readprop(id, list, 8);
}
BLITZ_THUNK(dovpropbody, dovpropbody_c, "a1");

/* ------------------------------------------------------ window commands */

/* Redraw w,id - RefreshGList(gadget, window, 0, 1) */
void BLITZ_ROUTINE doredraw(UWORD id BL_REG(d1), struct Window **win BL_REG(a1),
                             struct Library *intbase BL_REG(a6))
{
    struct Window *w = *win;
    UBYTE *gad = findgadget((UBYTE **)((UBYTE *)w + 62), id);

    RefreshGList((struct Gadget *)gad, w, NULL, 1);
}

/* ActivateString w,id - ActivateGadget(gadget, window, 0) */
ULONG BLITZ_ROUTINE dostringactivate(UWORD id BL_REG(d1),
                                     struct Window **win BL_REG(a1),
                                     struct Library *intbase BL_REG(a6))
{
    struct Window *w = *win;
    UBYTE *gad = findgadget((UBYTE **)((UBYTE *)w + 62), id);

    return ActivateGadget((struct Gadget *)gad, w, NULL);
}

/* GadgetBorder x,y,w,h - draw a border into the used window */
void BLITZ_ROUTINE dogborder(WORD x BL_REG(d0), WORD y BL_REG(d1),
                             WORD w BL_REG(d2), WORD h BL_REG(d3),
                             struct Window **win BL_REG(a1),
                             struct Library *intbase BL_REG(a6))
{
    UBYTE *border = allocborder2(w + g.bxxs2 + g.bxxs2, h + g.byxs2 + g.byxs2);

    DrawBorder((*win)->RPort, (struct Border *)border, x - g.bxxs2, y - g.byxs2);
    globfree(border, BORDERMEM);
}

/* --------------------------------------------------------- gadget state */

static void BLITZ_ROUTINE doselect_c(UWORD id BL_REG(d1), UWORD on BL_REG(d2),
                            UBYTE **list BL_REG(a1))
{
    UBYTE *gad = findgadget(list, id);

    if (on)
        B(gad, G_FLAGS + 1) |= 0x80;
    else
        B(gad, G_FLAGS + 1) &= ~0x80;
}
BLITZ_THUNK(doselect, doselect_c, "d0-d1/a1");

static void BLITZ_ROUTINE doselect2_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    B(findgadget(list, id), G_FLAGS + 1) ^= 0x80;
}
BLITZ_THUNK(doselect2, doselect2_c, "d0-d1/a1");

static void BLITZ_ROUTINE doenable_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    B(findgadget(list, id), G_FLAGS) &= ~1;
}
BLITZ_THUNK(doenable, doenable_c, "d0-d1/a1");

static void BLITZ_ROUTINE dodisable_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    B(findgadget(list, id), G_FLAGS) |= 1;
}
BLITZ_THUNK(dodisable, dodisable_c, "d0-d1/a1");

/* SetGadgetStatus l,id,v - text part v (1 = first) of a cycling text
   gadget; ignored for toggle gadgets and gadgets without text */
static void BLITZ_ROUTINE setdogstatus_c(UWORD id BL_REG(d1), UWORD v BL_REG(d2),
                                UBYTE **list BL_REG(a1))
{
    UBYTE *gad = findgadget(list, id), *it, *text;

    it = P(gad, G_TEXT);
    if (!it || (B(gad, G_ACT) & 1))
        return;
    text = P(it, 20);
    if ((WORD)--v < 0)
        return;
    while (v-- != 0)                    /* BRA 'nxstr: DBRA d2 */
        while (*text++)
            ;
    P(it, 12) = text;
}
BLITZ_THUNK(setdogstatus, setdogstatus_c, "d1");

/* GadgetStatus(l,id) - number of the shown text part, or -1/0 (selected)
   for toggle gadgets and gadgets without text */
static ULONG BLITZ_ROUTINE dogstatus_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    UBYTE *gad = findgadget(list, id), *it, *cur, *p;
    ULONG part;                         /* d0: the part number */

    it = P(gad, G_TEXT);                /* d0 */
    if (it && !(B(gad, G_ACT) & 1)) {
        cur = P(it, 12);
        p = P(it, 20);
        part = 1;
        while (p < cur) {
            part = (part & 0xffff0000UL) | (UWORD)(part + 1);
            while (*p++)
                ;
        }
        return part;
    }
    /* 'skip: BTST #7,13(a0):SNE d0:EXT.w d0 (the upper word of the
       GadgetText pointer stays) */
    return ((ULONG)it & 0xffff0000UL) | ((B(gad, G_FLAGS + 1) & 0x80) ? 0xffff : 0);
}
BLITZ_THUNK(dogstatus, dogstatus_c, "d1");

/* ButtonId(l,group) - id of the selected gadget of the group, -1 */
static ULONG BLITZ_ROUTINE dobuttonid_c(UWORD grp BL_REG(d1), UBYTE **list BL_REG(a1))
{
    UBYTE *gad = (UBYTE *)list;
    ULONG next;

    for (;;) {
        next = L(gad, 0);
        if (!next)
            return (ULONG)-1;
        gad = (UBYTE *)next;
        if (UW(gad, G_GROUP) == grp && (B(gad, G_FLAGS + 1) & 0x80))
            return (next & 0xffff0000UL) | UW(gad, G_ID);
    }
}
BLITZ_THUNK(dobuttonid, dobuttonid_c, "d1/a0");

/* SetTextGadget l,id,text$ - overwrite the text in place, padded with
   spaces, never longer */
static void BLITZ_ROUTINE settextgadget_c(UWORD id BL_REG(d1), const UBYTE *src BL_REG(d2),
                                 UBYTE **list BL_REG(a1))
{
    UBYTE *gad = findgadget(list, id), *it, *text;

    it = P(gad, G_TEXT);
    if (!it)
        return;
    text = P(it, 12);
    if (!text)
        return;
    for (;;) {                          /* nxc1 */
        if (!*text)
            return;
        if ((*text++ = *src++) == 0)
            break;
    }
    text[-1] = ' ';
    while (*text)                       /* nxc2 */
        *text++ = ' ';
}
BLITZ_THUNK(settextgadget, settextgadget_c, "d1");

/* ------------------------------------------------------- string gadgets */

/* SetString l,id,s$ - copy into the buffer (with 0) */
static void BLITZ_ROUTINE dosetstring_c(UWORD id BL_REG(d1), const UBYTE *s BL_REG(d2),
                               UBYTE **list BL_REG(a1))
{
    UBYTE *buf = P(P(findgadget(list, id), G_SPECIAL), 0);

    while ((*buf++ = *s++) != 0)
        ;
}
BLITZ_THUNK(dosetstring, dosetstring_c, "d0-d1");

static void BLITZ_ROUTINE dostringclear_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    UBYTE *si = P(findgadget(list, id), G_SPECIAL);

    W(si, 8) = 0;                       /* BufferPos */
    W(si, 12) = 0;                      /* DispPos */
    *P(si, 0) = 0;
}
BLITZ_THUNK(dostringclear, dostringclear_c, "d0-d1/a1");

static void BLITZ_ROUTINE dostringreset_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    UBYTE *si = P(findgadget(list, id), G_SPECIAL);

    W(si, 8) = 0;
    W(si, 12) = 0;
}
BLITZ_THUNK(dostringreset, dostringreset_c, "d0-d1/a1");

/* StringText$(l,id) - the buffer to a3 */
static ULONG BLITZ_ROUTINE dostringtext_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    const UBYTE *s = P(P(findgadget(list, id), G_SPECIAL), 0);
    ULONG n = (ULONG)-1;

    do
        n++;
    while ((*blitz_string++ = *s++) != 0);
    blitz_string--;
    return n;
}
BLITZ_THUNK(dostringtext, dostringtext_c, "d1/a1");

/* ------------------------------------------------------------- _free */

/* freerend - Border (72 bytes) or, with GADGIMAGE, Image (20 bytes) */
static void freerend(UBYTE *gad, UBYTE *r)
{
    if (r)
        globfree(r, (B(gad, G_FLAGS + 1) & 4) ? 20 : BORDERMEM);
}

/* freegadget - a3 = the list: free every "bL" gadget */
void BLITZ_ROUTINE freegadget(void)
{
    UBYTE *gad = P(blitz_string, 0), *n;

    P(blitz_string, 0) = NULL;
    while (gad) {
        if (UW(gad, G_USER) != 0x624c) {
            gad = P(gad, 0);
            continue;
        }
        if (P(gad, G_TEXT)) {
            UBYTE *it = P(gad, G_TEXT);
            freest(L(it, 20));
            globfree(it, 24);
        }
        freerend(gad, P(gad, G_RENDER));
        freerend(gad, P(gad, G_SELECT));
        /* BUG(orig) (#163): MOVEQ #22,d0 overwrites the type, the SpecialInfo
           is never freed */
        n = P(gad, 0);
        globfree(gad, GADGETSIZE);
        gad = n;
    }
}

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess1, "Can't Find Gadget");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized Window");
BLITZ_ERRCHK_MESSAGE(mess3, "Not a String Gadget");
BLITZ_ERRCHK_MESSAGE(mess4, "String to long for Gadget");
BLITZ_ERRCHK_MESSAGE(mess5, "Uninitialized Shape");
BLITZ_ERRCHK_MESSAGE(mess6, "Button Group MUST be greater than zero");

/* gchk - the gadget exists; 0 (after the runtime error) if not */
static UBYTE *BLITZ_ERRCHK gchk1(UBYTE **list, UWORD id)
{
    UBYTE *gad = (UBYTE *)list;

    for (;;) {
        gad = P(gad, 0);
        if (!gad) {
            BLITZ_RUNERR(mess1);
            return NULL;
        }
        if (UW(gad, G_ID) == id && W(gad, G_TYPE) >= 0)
            return gad;
    }
}

static void BLITZ_ERRCHK gchk_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    gchk1(list, id);
}

static void BLITZ_ERRCHK propchk_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    UBYTE *gad = gchk1(list, id);

    if (gad && B(gad, G_TYPE + 1) != 3)
        BLITZ_RUNERR(mess1);
}

static UBYTE *BLITZ_ERRCHK redrawchk1(UWORD id, struct Window **win)
{
    if (!*win) {
        BLITZ_RUNERR(mess2);
        return NULL;
    }
    return gchk1((UBYTE **)((UBYTE *)*win + 62), id);
}

static void BLITZ_ERRCHK redrawchk_c(UWORD id BL_REG(d1),
                                     struct Window **win BL_REG(a1))
{
    redrawchk1(id, win);
}

static void BLITZ_ERRCHK actchk_c(UWORD id BL_REG(d1),
                                  struct Window **win BL_REG(a1))
{
    UBYTE *gad = redrawchk1(id, win);

    if (gad && !(B(gad, G_TYPE + 1) & 4))
        BLITZ_RUNERR(mess3);
}

static UBYTE *BLITZ_ERRCHK reschk1(UWORD id, UBYTE **list)
{
    UBYTE *gad = gchk1(list, id);

    if (gad && !(B(gad, G_TYPE + 1) & 4)) {
        BLITZ_RUNERR(mess3);
        return NULL;
    }
    return gad;
}

static void BLITZ_ERRCHK reschk_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1))
{
    reschk1(id, list);
}

/* settest - SetString: the string must be shorter than MaxChars */
static void BLITZ_ERRCHK settest_c(UWORD id BL_REG(d1), UBYTE **list BL_REG(a1),
                                   const ULONG *lens BL_REG(a2))
{
    UBYTE *gad = reschk1(id, list);

    if (gad && (ULONG)UW(P(gad, G_SPECIAL), 10) <= lens[-1])
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK wchk_c(struct Window **win BL_REG(a1))
{
    if (!*win)
        BLITZ_RUNERR(mess2);
}

/* sgchk2 - TST (a1): the alternative shape; sgchk - TST (a2) */
static void BLITZ_ERRCHK sgchk2_c(const UWORD *alt BL_REG(a1),
                                  const UWORD *shape BL_REG(a2))
{
    if (!*alt || !*shape)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK sgchk_c(const UWORD *shape BL_REG(a2))
{
    if (!*shape)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK bgroupchk_c(WORD grp BL_REG(d0))
{
    if (grp <= 0)
        BLITZ_RUNERR(mess6);
}

/* the thunks last: GCC does not know that their asm leaves the error
   check section, so no C function may follow them */
BLITZ_ERRCHK_THUNK(gchk, gchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(propchk, propchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(redrawchk, redrawchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(actchk, actchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(reschk, reschk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(settest, settest_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(wchk, wchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(sgchk2, sgchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(sgchk, sgchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(bgroupchk, bgroupchk_c, "d0-d1/a0-a1");
