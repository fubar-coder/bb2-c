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
 * Gadgets (44 bytes), IntuiTexts (struct gtext, 24 bytes: +20 = start of
 * the text), Borders (struct gborders, 2 x 36 bytes: a highlight and a
 * shadow border of 5 points each), StringInfo (36), PropInfo (22) and
 * knob Images (20) come from memlib globalloc (MEMF_PUBLIC|MEMF_CLEAR,
 * the string buffer MEMF_PUBLIC only), texts from maxslib allocst.  Every gadget gets
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
 *   - SelectMode only stores the mode (selmode), which nothing reads; the
 *     gadgets get defflags (never set, 0 = GADGHCOMP): SelectMode 1 (box)
 *     has no effect. (#952)
 * Registers: the original changes about everything; plain C functions,
 * the error checks keep d0-d1/a0-a1 (more than the original).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include <intuition/intuition.h>
#define __NOLIBBASE__
#define INTUITION_BASE_NAME intbase
#include <proto/intuition.h>
#undef __NOLIBBASE__
#include <stddef.h>

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
                                 const struct bb_shape *shape BL_REG(a2));
void BLITZ_ROUTINE doshapegadget2(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                  ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                  const struct bb_shape *alt BL_REG(a1),
                                  const struct bb_shape *shape BL_REG(a2));
void dosethprop(void);
void dosetvprop(void);
void dohproppot(void);
void dohpropbody(void);
void dovproppot(void);
void dovpropbody(void);
void BLITZ_ROUTINE doredraw(UWORD id BL_REG(d1), struct Window **win BL_REG(a1),
                             struct Library *intbase BL_REG(a6));
void gadgetslib_doselect(void), gadgetslib_doselect2(void);
void BLITZ_ROUTINE dogborder(WORD x BL_REG(d0), WORD y BL_REG(d1),
                             WORD w BL_REG(d2), WORD h BL_REG(d3),
                             struct Window **win BL_REG(a1),
                             struct Library *intbase BL_REG(a6));
void dogb2(void);
void dogb(void);
ULONG BLITZ_ROUTINE dostringactivate(UWORD id BL_REG(d1),
                                     struct Window **win BL_REG(a1),
                                     struct Library *intbase BL_REG(a6));
void dostringreset(void), dostringtext(void), dostringclear(void);
void dogpens2(void), dogpens(void), dobpens(void), dogjam(void);
void doselectmode(void), dosetstring(void), dobgroup(void), dogstatus(void);
void dobuttonid(void), gadgetslib_doenable(void), gadgetslib_dodisable(void);
void setdogstatus(void), settextgadget(void), gadgetslib_codeinit(void);
void BLITZ_ROUTINE freegadget(void);
/* error checks (debugger only, thunks below) */
void gadgetslib_sgchk(void), gadgetslib_sgchk2(void), propchk(void);
void redrawchk(void), gchk(void), gadgetslib_wchk(void), actchk(void);
void reschk(void), settest(void), bgroupchk(void);

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
BLITZ_SUBS(doshapegadget, gadgetslib_sgchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1380, LIB_shapeslib, 0x1285, LIB_shapeslib, 0x1186);
BLITZ_SUBS(doshapegadget2, gadgetslib_sgchk2, 0);
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
BLITZ_SUBS(gadgetslib_doselect, gchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(gadgetslib_doselect2, gchk, 0);
BLITZ_NAME("Toggle", "GadgetList#,Id [,On|Off]");

BLITZ_NULLSUB(0, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dogborder), gadgetslib_wchk, 0);
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
BLITZ_SUBS(gadgetslib_doenable, gchk, 0);
BLITZ_NAME("Enable", "(GadgetList#,Id)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_gadgetslib, 0x1180);
BLITZ_SUBS(gadgetslib_dodisable, gchk, 0);
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

BLITZ_NULLSUB_AT(init, gadgetslib_codeinit, 0, 0);

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

/* a box border: a Border and its 5 points (BORDERSIZ bytes); a gadget
   or GadgetBorder gets two of them, the highlight border (left/top,
   NextBorder = the shadow border) and the shadow border (BORDERMEM) */
struct gborder {
    struct Border b;            /* +0 */
    WORD xy[10];                /* +16: 5 points */
};
static_assert(sizeof(struct gborder) == BORDERSIZ);
struct gborders {
    struct gborder light;       /* +0 */
    struct gborder shadow;      /* +36 */
};
static_assert(sizeof(struct gborders) == BORDERMEM);

/* the IntuiText of a text gadget (24 bytes): IText points at the shown
   part of the text, start at the whole text (the parts after "|") */
struct gtext {
    struct IntuiText it;        /* +0, IText at +12 */
    UBYTE *start;               /* +20 */
};
static_assert(sizeof(struct gtext) == 24);

#define GADGETSIZE  44
static_assert(sizeof(struct Gadget) == GADGETSIZE);
static_assert(sizeof(struct StringInfo) == 36);
static_assert(sizeof(struct PropInfo) == 22);
static_assert(sizeof(struct Image) == 20);

/* the bytes of the gadget's word fields the original works on with
   byte instructions (BSET/BCLR/BTST/OR.b, high byte first) */
#define GFLAGS_HI(gad)  (((UBYTE *)&(gad)->Flags)[0])       /* 12: bit 0 DISABLED */
#define GFLAGS_LO(gad)  (((UBYTE *)&(gad)->Flags)[1])       /* 13: SELECTED, GADGIMAGE ... */
#define GACT_HI(gad)    (((UBYTE *)&(gad)->Activation)[0])  /* 14: bit 0 TOGGLESELECT */
#define GACT_LO(gad)    (((UBYTE *)&(gad)->Activation)[1])  /* 15: the border flags */
#define GTYPE_HI(gad)   (((UBYTE *)&(gad)->GadgetType)[0])  /* 16: GZZGADGET */
#define GTYPE_LO(gad)   (((UBYTE *)&(gad)->GadgetType)[1])  /* 17: the gadget type */
/* UserData: "bL" in the high word, the button group in the low word */
#define GUSER_BL(gad)    (((UWORD *)&(gad)->UserData)[0])   /* 40 */
#define GUSER_GROUP(gad) (((UWORD *)&(gad)->UserData)[1])   /* 42 */

/* the PropInfo pots/bodies (setprop, readprop) by offset; the shape object
   (shapeslib) is struct bb_shape (blitzobj.h) */
#define UW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))

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
static struct Gadget *findgadget(struct Gadget **list, UWORD id)
{
    struct Gadget *gad = (struct Gadget *)list;

    do
        gad = gad->NextGadget;
    while ((UWORD)gad->GadgetID != id || (WORD)gad->GadgetType < 0);
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
BLITZ_THUNK(gadgetslib_codeinit, codeinit_c, "d0-d1/a0-a1");

/**
 * @brief ButtonGroup Group: the button group of the following gadgets.
 *
 * Gadgets made with flag bit 9 get it in the low word of UserData
 * (ButtonId).  Errors (debugger, bgroupchk): "Button Group MUST be
 * greater than zero".
 *
 * @param grp  d0: the group
 */
static void BLITZ_ROUTINE dobgroup_c(UWORD grp BL_REG(d0))
{
    g.bgroup = grp;
}
BLITZ_THUNK(dobgroup, dobgroup_c, "d0-d1/a0-a1");

/**
 * @brief SelectMode 1=Box, 0=Inverse: the highlight mode of new gadgets.
 *
 * Stores 1 for any non-zero value, 0 otherwise (g.selmode).  BUG(orig)
 * (#952): nothing reads selmode - the command has no effect.
 *
 * @param on  d0: the mode
 */
static void BLITZ_ROUTINE doselectmode_c(UWORD on BL_REG(d0))
{
    g.selmode = 0;
    if (on)
        g.selmode++;
}
BLITZ_THUNK(doselectmode, doselectmode_c, "d0-d1/a0-a1");

/**
 * @brief GadgetJam Jammode: the draw mode of the gadget texts.
 *
 * @param mode  d0: the IntuiText DrawMode of new text gadgets
 */
static void BLITZ_ROUTINE dogjam_c(UBYTE mode BL_REG(d0))
{
    g.mode = mode;
}
BLITZ_THUNK(dogjam, dogjam_c, "d0-d1/a0-a1");

/**
 * @brief GadgetPens Foreground Colour,Background Colour: the pens of the
 * gadget texts.
 *
 * @param fg  d0: the FrontPen of new text gadgets
 * @param bg  d1: the BackPen
 */
static void BLITZ_ROUTINE dogpens_c(UBYTE fg BL_REG(d0), UBYTE bg BL_REG(d1))
{
    g.bpen = bg;
    g.fpen = fg;
}
BLITZ_THUNK(dogpens, dogpens_c, "d0-d1/a0-a1");

/**
 * @brief GadgetPens Foreground Colour: the front pen only.
 *
 * @param fg  d0: the FrontPen of new text gadgets
 */
static void BLITZ_ROUTINE dogpens2_c(UBYTE fg BL_REG(d0))
{
    g.fpen = fg;
}
BLITZ_THUNK(dogpens2, dogpens2_c, "d0-d1/a0-a1");

/**
 * @brief BorderPens Highlight Colour,Shadow Colour: the pens of the
 * gadget borders.
 *
 * @param light  d0: the pen of the left/top border
 * @param shadow  d1: the pen of the right/bottom border
 */
static void BLITZ_ROUTINE dobpens_c(UBYTE light BL_REG(d0), UBYTE shadow BL_REG(d1))
{
    g.bco1 = light;
    g.bco2 = shadow;
}
BLITZ_THUNK(dobpens, dobpens_c, "d0-d1/a0-a1");

/**
 * @brief Borders Width,Height: the space between a gadget's text and its
 * border.
 *
 * Sets the border excess and its saved copy (the value Borders On
 * restores).
 *
 * @param w  d0: the horizontal excess
 * @param h  d1: the vertical excess
 */
static void BLITZ_ROUTINE dogb_c(UWORD w BL_REG(d0), UWORD h BL_REG(d1))
{
    g.bxxs = w;
    g.byxs = h;
    g.bxxs2 = g.bxxs;
    g.byxs2 = g.byxs;
}
BLITZ_THUNK(dogb, dogb_c, "d0-d1/a0-a1");

/**
 * @brief Borders On|Off: give new gadgets borders or not.
 *
 * Off saves the border excess and sets it to 0, On restores it; nothing
 * happens if the state does not change.
 *
 * @param on  d0: 0 = off, anything else = on
 */
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
static struct gborders *allocborder2(WORD w, WORD h)
{
    struct gborders *border = (struct gborders *)globalloc(BORDERMEM,
                                                           MEMF_PUBLIC | MEMF_CLEAR);

    border->light.b.FrontPen = g.bco1;
    border->shadow.b.FrontPen = g.bco2;
    border->light.b.Count = 5;
    border->shadow.b.Count = 5;
    border->light.b.XY = border->light.xy;
    border->light.b.NextBorder = &border->shadow.b;
    border->shadow.b.XY = border->shadow.xy;
    w--;
    h--;
    /* light: (w,0) (0,0) (0,h) (1,h-1) (1,1) */
    border->light.xy[0] = w;
    border->light.xy[5] = h;
    border->light.xy[7] = h - 1;
    border->light.xy[6] = 1;
    border->light.xy[8] = 1;
    border->light.xy[9] = 1;
    /* shadow: (1,h) (w,h) (w,0) (w-1,1) (w-1,h) */
    border->shadow.xy[1] = h;
    border->shadow.xy[3] = h;
    border->shadow.xy[9] = h;
    border->shadow.xy[7] = 1;
    border->shadow.xy[0] = 1;
    border->shadow.xy[2] = w;
    border->shadow.xy[4] = w;
    border->shadow.xy[6] = w - 1;
    border->shadow.xy[8] = w - 1;
    return border;
}

/* allocborder - 0 if borders are off */
static struct gborders *allocborder(WORD w, WORD h)
{
    if (!g.bordersok)
        return NULL;
    return allocborder2(w, h);
}

/* allocgadget - a new gadget at the end of the list in a3 */
static struct Gadget *allocgadget(WORD x, WORD y, UWORD id, WORD w, WORD h)
{
    struct Gadget *gad = (struct Gadget *)globalloc(GADGETSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    struct Gadget *prev, *next = (struct Gadget *)blitz_string;

    do {
        prev = next;
        next = prev->NextGadget;
    } while (next);
    prev->NextGadget = gad;
    gad->LeftEdge = x;
    gad->TopEdge = y;
    gad->Width = w;
    gad->Height = h;
    /* BUG(orig) (#952): defflags is never set; SelectMode (selmode) is not
       taken into account */
    gad->Flags = g.defflags;
    gad->GadgetID = id;
    gad->Activation = 3;                /* RELVERIFY|GADGIMMEDIATE */
    GUSER_BL(gad) = 0x624c;             /* "bL": blitz type gadget */
    GUSER_GROUP(gad) = 0;
    return gad;
}

/* allocintui - IntuiText (24 bytes) with a copy of the text */
static struct gtext *allocintui(const UBYTE *s, ULONG len)
{
    UBYTE *copy = allocst(s, len);
    struct gtext *it = (struct gtext *)globalloc(sizeof(struct gtext),
                                                 MEMF_PUBLIC | MEMF_CLEAR);

    it->it.IText = copy;
    it->start = copy;
    *(UWORD *)&it->it.FrontPen = *(UWORD *)&g.fpen;     /* fpen, bpen: one word */
    it->it.DrawMode = g.mode;
    it->it.ITextFont = (struct TextAttr *)g.fontattr;
    return it;
}

/* handleflags - d3 = the flags argument (word shifts as the original) */
static void handleflags(struct Gadget *gad, ULONG flags)
{
    UWORD lowflags;

    if (flags & (1UL << 16))
        GTYPE_HI(gad) |= 1 << 5;        /* GZZGADGET */
    if (flags & (1UL << 15))
        GACT_LO(gad) |= 1 << 7;         /* BOTTOMBORDER */
    if (flags & (1UL << 14))
        GACT_LO(gad) |= 1 << 6;         /* TOPBORDER */
    if (flags & (1UL << 13))
        GACT_LO(gad) |= 1 << 5;         /* LEFTBORDER */
    if (flags & (1UL << 12))
        GACT_LO(gad) |= 1 << 4;         /* RIGHTBORDER */
    if (flags & (1UL << 9))
        GUSER_GROUP(gad) = g.bgroup;
    lowflags = (UWORD)flags;
    if (lowflags & 1)
        GACT_HI(gad) |= 1;              /* toggle */
    if (lowflags & 2)
        GFLAGS_LO(gad) |= 1 << 4;       /* rel right */
    if (lowflags & 4)
        GFLAGS_LO(gad) |= 1 << 3;       /* rel bottom */
    if (lowflags & 8)
        GFLAGS_LO(gad) |= 1 << 5;       /* rel width */
    if (lowflags & 16)
        GFLAGS_LO(gad) |= 1 << 6;       /* rel height */
    if (lowflags & 32)
        GFLAGS_LO(gad) |= 1 << 0;       /* box select */
}

/* findmaxlen - text gadget: the "|" become 0, result = the width of the
   widest part (IntuiTextLength) */
static WORD findmaxlen(struct gtext *it, struct Library *intbase)
{
    UBYTE *start = it->it.IText, *part = start, *end;
    WORD maxwidth = 0, width;
    UBYTE c;

    for (;;) {
        end = part;
        while ((c = *end++) != 0 && c != '|')
            ;
        if (!c)
            break;
        end[-1] = 0;
        it->it.IText = part;
        width = (WORD)IntuiTextLength(&it->it);
        part = end;
        if (width > maxwidth)
            maxwidth = width;
    }
    it->it.IText = part;
    width = (WORD)IntuiTextLength(&it->it);
    if (width > maxwidth)
        maxwidth = width;
    it->it.IText = start;
    return maxwidth;
}

/* ------------------------------------------------------------- gadgets */

/**
 * @brief TextGadget GadgetList#,X,Y,Flags,Id,Text$: add a text (boolean)
 * gadget to the list.
 *
 * a3 = the list.  The IntuiText (struct gtext) gets a copy of the text,
 * the gadget pens, the jam mode and the current font; a text with "|"
 * cycles through its parts (SetGadgetStatus/GadgetStatus), the gadget is
 * as wide as the widest part plus the border excess, with a border if
 * Borders are on.  Flags: bit 0 toggle, 1-4 relative right/bottom/width/
 * height, 5 box select, 9 button group, 12-15 right/left/top/bottom
 * border, 16 GZZ gadget.  TextGadget with an empty text has IText 0 (not
 * a bug).
 *
 * @param x  d1: the left edge
 * @param y  d2: the top edge
 * @param flags  d3: the flags
 * @param id  d4: the gadget id
 * @param text  d5: Text$
 * @param font  a1: intuifontlib's current font (a TextAttr)
 * @param lens  a2: the argument stack, the text's length at -4
 * @param intbase  a6: intuition.library
 */
void BLITZ_ROUTINE dotextgadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                const UBYTE *text BL_REG(d5),
                                struct TextAttr **font BL_REG(a1),
                                const ULONG *lens BL_REG(a2),
                                struct Library *intbase BL_REG(a6))
{
    struct gtext *it;
    struct gborders *border;
    struct Gadget *gad;
    WORD w, boxw, boxh;
    WORD xs, ys;                        /* the border excess */

    /* fontcalc */
    g.fontattr = (ULONG)*font;
    g.fhit = (*font)->ta_YSize;
    it = allocintui(text, lens[-1]);
    w = findmaxlen(it, intbase);
    xs = g.bxxs;
    ys = g.byxs;
    it->it.LeftEdge = xs;
    it->it.TopEdge = ys;
    boxw = xs + xs + w;
    boxh = ys + ys + g.fhit;
    border = allocborder(boxw, boxh);
    gad = allocgadget(x, y, id, boxw, boxh);
    gad->GadgetRender = border;
    gad->GadgetText = &it->it;
    gad->GadgetType = 1;                /* BOOLGADGET */
    handleflags(gad, flags);
}

/**
 * @brief StringGadget GadgetList#,X,Y,Flags,Id,Maxlen,Width: add a string
 * gadget to the list.
 *
 * a3 = the list.  The gadget is the font height high, its border (if
 * Borders are on) goes around it; the StringInfo gets a buffer of Maxlen
 * bytes (cleared).  Flags as TextGadget.
 * BUG(orig) (#164): with Borders off the border offsets are written to
 * the addresses 0, 2, 36 and 38.
 * BUG(orig) (#165): Maxlen 0 clears 65536 bytes of the 0 byte buffer.
 *
 * @param x  d1: the left edge
 * @param y  d2: the top edge
 * @param flags  d3: the flags
 * @param id  d4: the gadget id
 * @param maxlen  d5: the size of the buffer
 * @param args  a2: the argument stack, Width at -4(a2) (its low word)
 */
void BLITZ_ROUTINE dostringgadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                  ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                  UWORD maxlen BL_REG(d5),
                                  const UWORD *args BL_REG(a2))
{
    WORD width = (WORD)args[-1];        /* MOVE -(a2),d6: low word */
    struct gborders *border;
    struct Gadget *gad;
    struct StringInfo *si;
    UBYTE *buf;
    WORD offset;
    UWORD n;

    border = allocborder(width + g.bxxs + g.bxxs, g.fhit + g.byxs + g.byxs);
    __asm__("" : "+a"(border));         /* may be 0, see above */
    offset = g.bxxs;
    x += offset;
    offset = -offset;
    *(volatile WORD *)&border->light.b.LeftEdge = offset;   /* border 0: BUG(orig) (#164) */
    *(volatile WORD *)&border->shadow.b.LeftEdge = offset;
    offset = g.byxs;
    y += offset;
    offset = -offset;
    *(volatile WORD *)&border->light.b.TopEdge = offset;
    *(volatile WORD *)&border->shadow.b.TopEdge = offset;
    gad = allocgadget(x, y, id, width, g.fhit);
    gad->GadgetType = 4;                /* STRGADGET */
    handleflags(gad, flags);
    gad->GadgetRender = border;
    si = (struct StringInfo *)globalloc(sizeof(struct StringInfo), MEMF_PUBLIC | MEMF_CLEAR);
    gad->SpecialInfo = si;
    buf = globalloc(maxlen, MEMF_PUBLIC);
    si->Buffer = buf;
    si->MaxChars = maxlen;
    n = maxlen - 1;                     /* DBF: maxlen 0 clears 65536 */
    do
        *buf++ = 0;
    while (n-- != 0);
}

/**
 * @brief PropGadget GadgetList#,X,Y,Flags,Id,Width,Height: add a
 * proportional gadget to the list.
 *
 * a3 = the list.  A PropInfo with AUTOKNOB and PROPNEWLOOK, pots and
 * bodies $8000, and an empty knob Image.  Flags as TextGadget, plus bit
 * 6 FREEHORIZ, 7 FREEVERT, 8 PROPBORDERLESS.
 *
 * @param x  d1: the left edge
 * @param y  d2: the top edge
 * @param flags  d3: the flags
 * @param id  d4: the gadget id
 * @param w  d5: the width
 * @param args  a2: the argument stack, Height at -4(a2) (its low word)
 */
void BLITZ_ROUTINE dopropgadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                UWORD w BL_REG(d5), const UWORD *args BL_REG(a2))
{
    struct Gadget *gad = allocgadget(x, y, id, w, args[-1]);
    struct PropInfo *pi = (struct PropInfo *)globalloc(sizeof(struct PropInfo),
                                                       MEMF_PUBLIC | MEMF_CLEAR);
    UWORD shifted, piflags = 1;         /* AUTOKNOB */

    gad->GadgetType = 3;                /* PROPGADGET */
    handleflags(gad, flags);
    shifted = (UWORD)flags >> 6;        /* handleflags shifted it 6 times */
    if (shifted & 1)
        piflags |= 2;                   /* FREEHORIZ */
    if (shifted & 2)
        piflags |= 4;                   /* FREEVERT */
    if (shifted & 4)
        piflags |= 8;                   /* PROPBORDERLESS */
    piflags |= 16;                      /* PROPNEWLOOK */
    pi->Flags = piflags;
    *(ULONG *)&pi->HorizPot = 0x80008000;       /* HorizPot, VertPot */
    *(ULONG *)&pi->HorizBody = 0x80008000;      /* HorizBody, VertBody */
    gad->SpecialInfo = pi;
    gad->GadgetRender = globalloc(sizeof(struct Image), MEMF_PUBLIC | MEMF_CLEAR);
    GFLAGS_LO(gad) |= 1 << 2;           /* GADGIMAGE */
}

/* makeintuishape - an Image (20 bytes) for the shape (width, height,
   depth at 0/2/4, data at 14) */
static struct Image *makeintuishape(const struct bb_shape *shape)
{
    struct Image *img = (struct Image *)globalloc(sizeof(struct Image),
                                                  MEMF_PUBLIC | MEMF_CLEAR);
    UWORD planepick = 0, n;

    img->Width = (WORD)shape->width;
    img->Height = (WORD)shape->height;
    img->Depth = (WORD)shape->depth;
    img->ImageData = (UWORD *)shape->data;
    n = shape->depth - 1;
    do
        planepick = (UWORD)(planepick << 1) | 1;
    while (n-- != 0);
    img->PlanePick = (UBYTE)planepick;
    img->PlaneOnOff = 0;
    return img;
}

static void shapegadget(WORD x, WORD y, ULONG flags, UWORD id,
                        const struct bb_shape *shape, struct Image *alt)
{
    struct Image *img;
    struct Gadget *gad;

    x -= shape->handlex;                  /* handle */
    y -= shape->handley;
    img = makeintuishape(shape);
    gad = allocgadget(x, y, id, (WORD)shape->width, (WORD)shape->height);
    gad->GadgetType = 1;
    GFLAGS_LO(gad) |= 1 << 2;           /* GADGIMAGE */
    gad->GadgetRender = img;
    gad->SelectRender = alt;
    if (alt)
        GFLAGS_LO(gad) |= 1 << 1;       /* GADGHIMAGE */
    handleflags(gad, flags);
}

/**
 * @brief ShapeGadget GadgetList#,X,Y,Flags,Id,Shape#: add an image gadget
 * made from a shape.
 *
 * a3 = the list.  The position is moved by the shape's handle, the
 * Image (makeintuishape) uses the shape's data.  Flags as TextGadget.
 * BUG(orig) (#167): a shape of depth 0 gives PlanePick $FF.  Errors
 * (debugger, sgchk): "Uninitialized Shape".
 *
 * @param x  d1: the left edge
 * @param y  d2: the top edge
 * @param flags  d3: the flags
 * @param id  d4: the gadget id
 * @param shape  a2: the shape (!libs #shapeslib,$1285)
 */
void BLITZ_ROUTINE doshapegadget(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                 ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                 const struct bb_shape *shape BL_REG(a2))
{
    shapegadget(x, y, flags, id, shape, NULL);
}

/**
 * @brief ShapeGadget GadgetList#,X,Y,Flags,Id,Shape#,Shape#: an image
 * gadget with an alternative image for the selected state (GADGHIMAGE).
 *
 * As ShapeGadget.  Errors (debugger, sgchk2): "Uninitialized Shape" for
 * either shape.
 *
 * @param x  d1: the left edge
 * @param y  d2: the top edge
 * @param flags  d3: the flags
 * @param id  d4: the gadget id
 * @param alt  a1: the alternative shape (!libs #shapeslib,$1186)
 * @param shape  a2: the shape (!libs #shapeslib,$1285)
 */
void BLITZ_ROUTINE doshapegadget2(WORD x BL_REG(d1), WORD y BL_REG(d2),
                                  ULONG flags BL_REG(d3), UWORD id BL_REG(d4),
                                  const struct bb_shape *alt BL_REG(a1),
                                  const struct bb_shape *shape BL_REG(a2))
{
    struct Image *img = makeintuishape(alt);

    shapegadget(x, y, flags, id, shape, img);
}

/* ---------------------------------------------------------- prop gadgets */

/* setprop2 - pot/body (quick, 1.0 = $10000 -> $FFFF) */
/* off = the offset of the pot in the PropInfo, the body is 4 further */
static void setprop(UWORD id, ULONG pot, ULONG body, struct Gadget **list, int off)
{
    UBYTE *pi;

    if (pot >= 0x10000)
        pot = (pot & 0xffff0000UL) | 0xffff;
    if (body >= 0x10000)
        body = (body & 0xffff0000UL) | 0xffff;
    pi = findgadget(list, id)->SpecialInfo;
    UW(pi, off) = (UWORD)pot;
    UW(pi, off + 4) = (UWORD)body;
}

/**
 * @brief SetHProp GadgetList#,Id,Pot,Body: the horizontal pot and body of
 * a prop gadget.
 *
 * Quick values, 1.0 ($10000) or more becomes $FFFF.  The gadget is not
 * redrawn.
 * Errors (debugger, propchk): "Can't Find Gadget" (also if it is no
 * prop gadget).  BUG(orig) (#166): without the debugger the search does
 * not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param pot  d2: the pot (Quick)
 * @param body  d3: the body (Quick)
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 */
static void BLITZ_ROUTINE dosethprop_c(UWORD id BL_REG(d1), ULONG pot BL_REG(d2),
                              ULONG body BL_REG(d3), struct Gadget **list BL_REG(a1))
{
    setprop(id, pot, body, list, offsetof(struct PropInfo, HorizPot));
}
BLITZ_THUNK(dosethprop, dosethprop_c, "d0-d1/a1");

/**
 * @brief SetVProp GadgetList#,Id,Pot,Body: the vertical pot and body of a
 * prop gadget.
 *
 * As SetHProp.
 * Errors (debugger, propchk): "Can't Find Gadget" (also if it is no
 * prop gadget).  BUG(orig) (#166): without the debugger the search does
 * not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param pot  d2: the pot (Quick)
 * @param body  d3: the body (Quick)
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 */
static void BLITZ_ROUTINE dosetvprop_c(UWORD id BL_REG(d1), ULONG pot BL_REG(d2),
                              ULONG body BL_REG(d3), struct Gadget **list BL_REG(a1))
{
    setprop(id, pot, body, list, offsetof(struct PropInfo, VertPot));
}
BLITZ_THUNK(dosetvprop, dosetvprop_c, "d0-d1/a1");

static ULONG readprop(UWORD id, struct Gadget **list, int off)
{
    return UW(findgadget(list, id)->SpecialInfo, off);
}

/**
 * @brief HPropPot(GadgetList#,Id): the HorizPot of a prop gadget.
 *
 * Errors (debugger, propchk): "Can't Find Gadget" (also if it is no
 * prop gadget).  BUG(orig) (#166): without the debugger the search does
 * not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 * @return d0: the HorizPot as a Quick ($FFFF = almost 1.0)
 */
static ULONG BLITZ_ROUTINE dohproppot_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    return readprop(id, list, offsetof(struct PropInfo, HorizPot));
}
BLITZ_THUNK(dohproppot, dohproppot_c, "a1");

/**
 * @brief HPropBody(GadgetList#,Id): the HorizBody of a prop gadget.
 *
 * Errors (debugger, propchk): "Can't Find Gadget" (also if it is no
 * prop gadget).  BUG(orig) (#166): without the debugger the search does
 * not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 * @return d0: the HorizBody as a Quick ($FFFF = almost 1.0)
 */
static ULONG BLITZ_ROUTINE dohpropbody_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    return readprop(id, list, offsetof(struct PropInfo, HorizBody));
}
BLITZ_THUNK(dohpropbody, dohpropbody_c, "a1");

/**
 * @brief VPropPot(GadgetList#,Id): the VertPot of a prop gadget.
 *
 * Errors (debugger, propchk): "Can't Find Gadget" (also if it is no
 * prop gadget).  BUG(orig) (#166): without the debugger the search does
 * not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 * @return d0: the VertPot as a Quick ($FFFF = almost 1.0)
 */
static ULONG BLITZ_ROUTINE dovproppot_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    return readprop(id, list, offsetof(struct PropInfo, VertPot));
}
BLITZ_THUNK(dovproppot, dovproppot_c, "a1");

/**
 * @brief VPropBody(GadgetList#,Id): the VertBody of a prop gadget.
 *
 * Errors (debugger, propchk): "Can't Find Gadget" (also if it is no
 * prop gadget).  BUG(orig) (#166): without the debugger the search does
 * not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 * @return d0: the VertBody as a Quick ($FFFF = almost 1.0)
 */
static ULONG BLITZ_ROUTINE dovpropbody_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    return readprop(id, list, offsetof(struct PropInfo, VertBody));
}
BLITZ_THUNK(dovpropbody, dovpropbody_c, "a1");

/* ------------------------------------------------------ window commands */

/**
 * @brief Redraw Window#,Id: redraw a gadget of the window.
 *
 * RefreshGList(gadget, window, 0, 1).  Amiga mode only.  Errors
 * (debugger, redrawchk): "Uninitialized Window", "Can't Find Gadget".
 * BUG(orig) (#166): without the debugger the search does not stop at the
 * end of the window's gadget list.
 *
 * @param id  d1: the gadget id
 * @param win  a1: the window (!libs #windowslib,$1180)
 * @param intbase  a6: intuition.library
 */
void BLITZ_ROUTINE doredraw(UWORD id BL_REG(d1), struct Window **win BL_REG(a1),
                             struct Library *intbase BL_REG(a6))
{
    struct Window *w = *win;
    struct Gadget *gad = findgadget(&w->FirstGadget, id);

    RefreshGList(gad, w, NULL, 1);
}

/**
 * @brief ActivateString Window#,Id: activate a string gadget of the
 * window.
 *
 * ActivateGadget(gadget, window, 0).  Amiga mode only.  Errors (debugger,
 * actchk): "Uninitialized Window", "Can't Find Gadget", "Not a String
 * Gadget".  BUG(orig) (#166) as Redraw.
 *
 * @param id  d1: the gadget id
 * @param win  a1: the window (!libs #windowslib,$1180)
 * @param intbase  a6: intuition.library
 * @return d0: ActivateGadget's result (a statement: not used)
 */
ULONG BLITZ_ROUTINE dostringactivate(UWORD id BL_REG(d1),
                                     struct Window **win BL_REG(a1),
                                     struct Library *intbase BL_REG(a6))
{
    struct Window *w = *win;
    struct Gadget *gad = findgadget(&w->FirstGadget, id);

    return ActivateGadget(gad, w, NULL);
}

/**
 * @brief GadgetBorder X,Y,Width,Height: draw a gadget style border into
 * the used window.
 *
 * The border goes around the box with the saved border excess (also with
 * Borders off); its memory is freed again.  Amiga mode only.  Errors
 * (debugger, wchk): "Uninitialized Window".
 *
 * @param x  d0: the left edge of the box
 * @param y  d1: the top edge
 * @param w  d2: the width
 * @param h  d3: the height
 * @param win  a1: the used window (!libs #windowslib,$1102)
 * @param intbase  a6: intuition.library
 */
void BLITZ_ROUTINE dogborder(WORD x BL_REG(d0), WORD y BL_REG(d1),
                             WORD w BL_REG(d2), WORD h BL_REG(d3),
                             struct Window **win BL_REG(a1),
                             struct Library *intbase BL_REG(a6))
{
    struct gborders *border = allocborder2(w + g.bxxs2 + g.bxxs2, h + g.byxs2 + g.byxs2);

    DrawBorder((*win)->RPort, &border->light.b, x - g.bxxs2, y - g.byxs2);
    globfree(border, BORDERMEM);
}

/* --------------------------------------------------------- gadget state */

/**
 * @brief Toggle GadgetList#,Id,On|Off: select or deselect a gadget.
 *
 * Sets or clears SELECTED; the gadget is not redrawn.
 * Errors (debugger, gchk): "Can't Find Gadget".  BUG(orig) (#166):
 * without the debugger the search does not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param on  d2: 0 = deselect, anything else = select
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 */
static void BLITZ_ROUTINE doselect_c(UWORD id BL_REG(d1), UWORD on BL_REG(d2),
                            struct Gadget **list BL_REG(a1))
{
    struct Gadget *gad = findgadget(list, id);

    if (on)
        GFLAGS_LO(gad) |= 0x80;         /* SELECTED */
    else
        GFLAGS_LO(gad) &= ~0x80;
}
BLITZ_THUNK(gadgetslib_doselect, doselect_c, "d0-d1/a1");

/**
 * @brief Toggle GadgetList#,Id: flip the SELECTED state of a gadget.
 *
 * Errors (debugger, gchk): "Can't Find Gadget".  BUG(orig) (#166):
 * without the debugger the search does not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 */
static void BLITZ_ROUTINE doselect2_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    GFLAGS_LO(findgadget(list, id)) ^= 0x80;
}
BLITZ_THUNK(gadgetslib_doselect2, doselect2_c, "d0-d1/a1");

/**
 * @brief Enable GadgetList#,Id: clear DISABLED of a gadget.
 *
 * Errors (debugger, gchk): "Can't Find Gadget".  BUG(orig) (#166):
 * without the debugger the search does not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 */
static void BLITZ_ROUTINE doenable_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    GFLAGS_HI(findgadget(list, id)) &= ~1;
}
BLITZ_THUNK(gadgetslib_doenable, doenable_c, "d0-d1/a1");

/**
 * @brief Disable GadgetList#,Id: set DISABLED of a gadget.
 *
 * Errors (debugger, gchk): "Can't Find Gadget".  BUG(orig) (#166):
 * without the debugger the search does not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 */
static void BLITZ_ROUTINE dodisable_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    GFLAGS_HI(findgadget(list, id)) |= 1;
}
BLITZ_THUNK(gadgetslib_dodisable, dodisable_c, "d0-d1/a1");

/**
 * @brief SetGadgetStatus GadgetList#,Id,Value: show text part Value of a
 * cycling text gadget.
 *
 * Value 1 = the first part; ignored for toggle gadgets, gadgets without
 * text and Value 0.  The gadget is not redrawn.
 * Errors (debugger, gchk): "Can't Find Gadget".  BUG(orig) (#166):
 * without the debugger the search does not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param v  d2: the part
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 */
static void BLITZ_ROUTINE setdogstatus_c(UWORD id BL_REG(d1), UWORD v BL_REG(d2),
                                struct Gadget **list BL_REG(a1))
{
    struct Gadget *gad = findgadget(list, id);
    struct gtext *it;
    UBYTE *text;

    it = (struct gtext *)gad->GadgetText;
    if (!it || (GACT_HI(gad) & 1))
        return;
    text = it->start;
    if ((WORD)--v < 0)
        return;
    while (v-- != 0)                    /* BRA 'nxstr: DBRA d2 */
        while (*text++)
            ;
    it->it.IText = text;
}
BLITZ_THUNK(setdogstatus, setdogstatus_c, "d1");

/**
 * @brief GadgetStatus(GadgetList#,Id): the shown text part, or whether
 * the gadget is selected.
 *
 * For a text gadget that is no toggle gadget: the number of the shown
 * part (1 = first).  For toggle gadgets and gadgets without text: -1 if
 * SELECTED, else 0 (SNE/EXT.w: not a bug, the upper word of d0 is that of
 * the GadgetText pointer).
 * Errors (debugger, gchk): "Can't Find Gadget".  BUG(orig) (#166):
 * without the debugger the search does not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 * @return d0.w: the part number, or -1/0
 */
static ULONG BLITZ_ROUTINE dogstatus_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    struct Gadget *gad = findgadget(list, id);
    struct gtext *it;
    UBYTE *cur, *p;
    ULONG part;                         /* d0: the part number */

    it = (struct gtext *)gad->GadgetText;   /* d0 */
    if (it && !(GACT_HI(gad) & 1)) {
        cur = it->it.IText;
        p = it->start;
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
    return ((ULONG)it & 0xffff0000UL) | ((GFLAGS_LO(gad) & 0x80) ? 0xffff : 0);
}
BLITZ_THUNK(dogstatus, dogstatus_c, "d1");

/**
 * @brief ButtonId(GadgetList#,ButtonGroup): the selected gadget of a
 * button group.
 *
 * The first gadget of the list with the group in UserData and SELECTED
 * set.  Not a bug (word function): the upper word of d0 is that of the
 * gadget pointer.
 *
 * @param grp  d1: the button group
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 * @return d0.w: the gadget id, -1 if none is selected
 */
static ULONG BLITZ_ROUTINE dobuttonid_c(UWORD grp BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    struct Gadget *gad = (struct Gadget *)list;
    ULONG next;

    for (;;) {
        next = (ULONG)gad->NextGadget;
        if (!next)
            return (ULONG)-1;
        gad = (struct Gadget *)next;
        if (GUSER_GROUP(gad) == grp && (GFLAGS_LO(gad) & 0x80))
            return (next & 0xffff0000UL) | (UWORD)gad->GadgetID;
    }
}
BLITZ_THUNK(dobuttonid, dobuttonid_c, "d1/a0");

/**
 * @brief SetTextGadget GadgetList#,Id,Text$: overwrite the text of a
 * gadget.
 *
 * In place: the new text is padded with spaces and never longer than the
 * old one; nothing for a gadget without text.  The gadget is not
 * redrawn.
 * Errors (debugger, gchk): "Can't Find Gadget".  BUG(orig) (#166):
 * without the debugger the search does not stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param src  d2: Text$
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 */
static void BLITZ_ROUTINE settextgadget_c(UWORD id BL_REG(d1), const UBYTE *src BL_REG(d2),
                                 struct Gadget **list BL_REG(a1))
{
    struct Gadget *gad = findgadget(list, id);
    struct IntuiText *it;
    UBYTE *text;

    it = gad->GadgetText;
    if (!it)
        return;
    text = it->IText;
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

/**
 * @brief SetString GadgetList#,Id,String$: set the contents of a string
 * gadget.
 *
 * Copies the string (with its 0) into the buffer; the positions stay and
 * the gadget is not redrawn.  Errors (debugger, settest): as ResetString,
 * and "String to long for Gadget" if the string is not shorter than
 * MaxChars.
 *
 * @param id  d1: the gadget id
 * @param s  d2: String$
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 */
static void BLITZ_ROUTINE dosetstring_c(UWORD id BL_REG(d1), const UBYTE *s BL_REG(d2),
                               struct Gadget **list BL_REG(a1))
{
    UBYTE *buf = ((struct StringInfo *)findgadget(list, id)->SpecialInfo)->Buffer;

    while ((*buf++ = *s++) != 0)
        ;
}
BLITZ_THUNK(dosetstring, dosetstring_c, "d0-d1");

/**
 * @brief ClearString GadgetList#,Id: empty a string gadget.
 *
 * BufferPos and DispPos 0, an empty buffer.
 * Errors (debugger, reschk): "Can't Find Gadget", "Not a String
 * Gadget".  BUG(orig) (#166): without the debugger the search does not
 * stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 */
static void BLITZ_ROUTINE dostringclear_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    struct StringInfo *si = findgadget(list, id)->SpecialInfo;

    si->BufferPos = 0;
    si->DispPos = 0;
    *si->Buffer = 0;
}
BLITZ_THUNK(dostringclear, dostringclear_c, "d0-d1/a1");

/**
 * @brief ResetString GadgetList#,Id: the cursor of a string gadget to
 * the start.
 *
 * BufferPos and DispPos 0, the contents stay.
 * Errors (debugger, reschk): "Can't Find Gadget", "Not a String
 * Gadget".  BUG(orig) (#166): without the debugger the search does not
 * stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 */
static void BLITZ_ROUTINE dostringreset_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    struct StringInfo *si = findgadget(list, id)->SpecialInfo;

    si->BufferPos = 0;
    si->DispPos = 0;
}
BLITZ_THUNK(dostringreset, dostringreset_c, "d0-d1/a1");

/**
 * @brief StringText$(GadgetList#,Id): the contents of a string gadget.
 *
 * Copies the buffer to a3 (a3 is left on the terminating 0).
 * Errors (debugger, reschk): "Can't Find Gadget", "Not a String
 * Gadget".  BUG(orig) (#166): without the debugger the search does not
 * stop at the end of the list.
 *
 * @param id  d1: the gadget id
 * @param list  a1: the gadget list (!libs #gadgetslib,$1180)
 * @return d0: the length
 */
static ULONG BLITZ_ROUTINE dostringtext_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    const UBYTE *s = ((struct StringInfo *)findgadget(list, id)->SpecialInfo)->Buffer;
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
static void freerend(struct Gadget *gad, APTR r)
{
    if (r)
        globfree(r, (GFLAGS_LO(gad) & 4) ? sizeof(struct Image) : BORDERMEM);
}

/**
 * @brief _free of a GadgetList object: free its gadgets.
 *
 * a3 = the list; the list is cleared, every "bL" gadget is freed with its
 * IntuiText and text, its render and select Border or Image.  BUG(orig)
 * (#163): the SpecialInfo (StringInfo and buffer, PropInfo) is never
 * freed.
 */
void BLITZ_ROUTINE freegadget(void)
{
    struct Gadget **list = (struct Gadget **)blitz_string;
    struct Gadget *gad = *list, *n;

    *list = NULL;
    while (gad) {
        if (GUSER_BL(gad) != 0x624c) {
            gad = gad->NextGadget;
            continue;
        }
        if (gad->GadgetText) {
            struct gtext *it = (struct gtext *)gad->GadgetText;
            freest((ULONG)it->start);
            globfree(it, sizeof(struct gtext));
        }
        freerend(gad, gad->GadgetRender);
        freerend(gad, gad->SelectRender);
        /* BUG(orig) (#163): MOVEQ #22,d0 overwrites the type, the SpecialInfo
           is never freed */
        n = gad->NextGadget;
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
static struct Gadget *BLITZ_ERRCHK gchk1(struct Gadget **list, UWORD id)
{
    struct Gadget *gad = (struct Gadget *)list;

    for (;;) {
        gad = gad->NextGadget;
        if (!gad) {
            BLITZ_RUNERR(mess1);
            return NULL;
        }
        if ((UWORD)gad->GadgetID == id && (WORD)gad->GadgetType >= 0)
            return gad;
    }
}

static void BLITZ_ERRCHK gchk_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    gchk1(list, id);
}

static void BLITZ_ERRCHK propchk_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    struct Gadget *gad = gchk1(list, id);

    if (gad && GTYPE_LO(gad) != 3)
        BLITZ_RUNERR(mess1);
}

static struct Gadget *BLITZ_ERRCHK redrawchk1(UWORD id, struct Window **win)
{
    if (!*win) {
        BLITZ_RUNERR(mess2);
        return NULL;
    }
    return gchk1(&(*win)->FirstGadget, id);
}

static void BLITZ_ERRCHK redrawchk_c(UWORD id BL_REG(d1),
                                     struct Window **win BL_REG(a1))
{
    redrawchk1(id, win);
}

static void BLITZ_ERRCHK actchk_c(UWORD id BL_REG(d1),
                                  struct Window **win BL_REG(a1))
{
    struct Gadget *gad = redrawchk1(id, win);

    if (gad && !(GTYPE_LO(gad) & 4))
        BLITZ_RUNERR(mess3);
}

static struct Gadget *BLITZ_ERRCHK reschk1(UWORD id, struct Gadget **list)
{
    struct Gadget *gad = gchk1(list, id);

    if (gad && !(GTYPE_LO(gad) & 4)) {
        BLITZ_RUNERR(mess3);
        return NULL;
    }
    return gad;
}

static void BLITZ_ERRCHK reschk_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1))
{
    reschk1(id, list);
}

/* settest - SetString: the string must be shorter than MaxChars */
static void BLITZ_ERRCHK settest_c(UWORD id BL_REG(d1), struct Gadget **list BL_REG(a1),
                                   const ULONG *lens BL_REG(a2))
{
    struct Gadget *gad = reschk1(id, list);

    if (gad && (ULONG)(UWORD)((struct StringInfo *)gad->SpecialInfo)->MaxChars <= lens[-1])
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK wchk_c(struct Window **win BL_REG(a1))
{
    if (!*win)
        BLITZ_RUNERR(mess2);
}

/* sgchk2 - TST (a1): the alternative shape; sgchk - TST (a2) */
static void BLITZ_ERRCHK sgchk2_c(const struct bb_shape *alt BL_REG(a1),
                                  const struct bb_shape *shape BL_REG(a2))
{
    if (!alt->width || !shape->width)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK sgchk_c(const struct bb_shape *shape BL_REG(a2))
{
    if (!shape->width)
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
BLITZ_ERRCHK_THUNK(gadgetslib_wchk, wchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(gadgetslib_sgchk2, sgchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(gadgetslib_sgchk, sgchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(bgroupchk, bgroupchk_c, "d0-d1/a0-a1");
