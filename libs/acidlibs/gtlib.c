/*
 * gtlib - command library 141 (#mygadtoolslib): GadTools gadgets -
 * GTButton, GTCheckBox, GTCycle, GTInteger, GTListView, GTMX, GTNumber,
 * GTPalette, GTScroller, GTSlider, GTString, GTText, AttachGTList,
 * GTTags, GTGadPtr, GTBevelBox, GTChangeList, GTSetAttrs, GTSetString,
 * GTSetInteger, GTGetString, GTGetInteger, GTShape, GTGetAttrs,
 * GTEnable, GTDisable, GTToggle, GTStatus, GTArrowSize, DetachGTList,
 * GTGimmeCludge, GTSetHighlight, GTFreeGadget.
 *
 * C translation of the CD binary (acidlibs).  The source in bblibsrc/gtlib
 * is a SEPARATE development (see notes/cmdlibs-decl.md): it lacks
 * DetachGTList, GTGimmeCludge, GTSetHighlight and GTFreeGadget, has
 * GTMakeCycleTag instead, an AttachGTList without error check and
 * GTSetAttrs as a plain statement.  This file follows the disassembly of
 * the binary; the labels are those of the source where the code matches.
 *
 * An object library (!libfin{...,5,4}): the object "GTList" (16 bytes):
 *   +0 the context (CreateContext, first gadget), +4 VisualInfo of the
 *   used screen, +8 the last created gadget, +12 the window it is
 *   attached to.
 * Every gadget creator fills the static NewGadget (wrkgadget) in
 * handlestd: position (+ the screen's window border sizes and font height
 * unless GTGimmeCludge is on), size, TextAttr of the current IntuiFont, a
 * copy of the label (maxslib allocst, never freed), the id and the
 * VisualInfo; ng_Flags = flags & $3F.  Flag bits: 6 disabled, 7
 * immediate, 8 checked, 9 scaled, 10 vertical, 11 arrows, 12 read only,
 * 13 toggle (button).  The tag lists are static data inside the library,
 * as in the original; GTTags fills `usertags`, which the next gadget gets
 * via TAG_MORE (then cleared).
 *
 * Kept from the original (BUG(orig)):
 *   - GTBevelBox: flags <> 0 gives the tag GTBB_Recessed with value 0, so
 *     the box is never recessed. (#170)
 *   - GTButton toggle (bit 13) sets TOGGLESELECT even if CreateGadget
 *     failed (writes address 14). (#171)
 *   - GTShape: the label of the previous gadget stays in the NewGadget
 *     (#172); an existing GadgetRender of the button is overwritten with
 *     the static Image (20 bytes), else replaced by it (#173).
 *   - GTSetAttrs/GTChangeList/GTSetHighlight work on the LAST gadget of a
 *     run of gadgets with the same id, GTGetAttrs/GTGadPtr etc. on the
 *     FIRST; GTFreeGadget frees the whole run (first to last). (#175)
 *   - Without a used screen handlestd reads the border sizes from 0. (#176)
 *   - GTChangeList, GTSetHighlight, GTSetAttrs, GTSetString,
 *     GTSetInteger and GTGetAttrs pass their tag list in a3 (the string
 *     buffer pointer) and leave a3 there: as a function inside a string
 *     expression the rest of the expression is written over the tag list
 *     and what follows it in the library. (#328)
 * Fixed (intended deviation, #168): GTPalette with a colour wrote it to
 *   the low word of the TAG_MORE pointer of its tag list (offset +38 of
 *   the source layout, which had two more tags): the colour was ignored,
 *   a GTTags list after it was reached through a wrong pointer.  Here the
 *   colour goes to GTPA_Color; the form without it sets GTPA_Color 0 (the
 *   value it always had).
 * Fixed (intended deviation, #169): the optional values (cycle/mx
 *   active, listview top/selected, scroller top, slider level, string
 *   default) stayed in the static tag lists, a later call without them
 *   used the previous values (GTString a pointer into the old string work
 *   area).  Here the forms without them set the initial values of the
 *   lists (0, GTLV_Selected -1, GTST_String NULL).
 * Fixed (intended deviation, #327): GTShape did not check the result of
 *   CreateGadget: if it failed, the images and flags went to addresses
 *   12-25 (the exception vectors), with a GadgetRender read from address
 *   18 even 20 bytes to where that long pointed.  Here a failed gadget is
 *   left alone (as with the other creators the GTList's last gadget is 0
 *   then).
 * Fixed (intended deviation, #174): getoptions (cycle/mx labels) sized
 *   its block as option length + 258 with the pointer array at +0 and the
 *   texts at +256: at most 63 labels + NULL without a check (more
 *   overwrote the texts and, with many short labels, memory behind the
 *   block), and the block was never freed.  Here the block holds the
 *   counted pointers and then the texts; it is remembered with its GTList
 *   and gadget (a list of the library, the GTList object stays 16 bytes)
 *   and freed with the gadget (GTFreeGadget) or the GTList (_freecontext).
 * DIFF: GTTags/GTSetAttrs do not move a2 (the original pops the pairs);
 * the unused error routine freqchk of the binary is not ported.
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include "blitzexec.h"
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#define __NOLIBBASE__
#define GADTOOLS_BASE_NAME gtbase
#define INTUITION_BASE_NAME intbase
#include <proto/gadtools.h>
#include <proto/intuition.h>
#undef __NOLIBBASE__


#define TAG       0x80080000UL
#define T_DISABLED  0x8003000eUL        /* GA_Disabled */
#define T_IMMEDIATE 0x80030015UL        /* GA_Immediate */
#define T_RELVERIFY 0x80030016UL        /* GA_RelVerify */
#define T_FREEDOM   0x80031001UL        /* PGA_Freedom */
#define T_UNDERSC   (TAG + 64)          /* GT_Underscore */

/* entry points (defined below) */
void _gtbutton(void), _gtcheckbox(void), _gtcycle(void), _gtcycle2(void);
void _gtinteger(void), _gtlistview(void), _gtlistview1(void), _gtlistview2(void);
void _gtmx(void), _gtmx2(void), _gtnumber(void), _gtpalette(void), _gtpalette2(void);
void _gtscroller(void), _gtscroller2(void), _gtslider(void), _gtslider2(void);
void _gtstring(void), _gtstring2(void), _gttext(void), _attachgtlist(void);
void _gttags(void), _gtptr(void), _gtbevel(void), _gtchnglist1(void);
void _gtchnglist2(void), _gtsetattrs(void), _gtsetstring(void), _gtsetint(void);
void _gtgetstring(void), _gtgetint(void), _gtshape(void), _gtshape2(void);
void _gtgetattrs(void), doenable(void), dodisable(void), doselect(void);
void doselect2(void), _gtgetstatus(void), _gtarrowsize(void), _detachgtlist(void);
void _gtcludge(void), _gtsethighlight(void), _gtfreegadget(void);
void _codeinit(void), _codefinit(void), _freecontext(void);
void sgchk(void), sgchk2(void), attachchk(void), detachchk(void), freegadchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#mygadtoolslib,init,0,finit,errors} */
BLITZ_LIBHEADER(LIB_mygadtoolslib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_DUMTOKE(_toke, "GTList", "A GadTools GadgetList");

#define A (char *)              /* +1: Amiga mode only */
#define STD BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_STRING, BL_LONG
#define STDLIBS BLITZ_LIBS(LIB_mygadtoolslib, 0x1380, LIB_intuifontlib, BL_LA(0), LIB_screenslib, 0x1102)
#define WPUSH (BL_WORD | BL_PUSH)

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD);
STDLIBS;
BLITZ_SUBS(A _gtbutton + 1, 0, 0);
BLITZ_NAME("GTButton", "GTList#,id,x,y,w,h,Text$,flags");

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD);
STDLIBS;
BLITZ_SUBS(A _gtcheckbox + 1, 0, 0);
BLITZ_NAME("GTCheckBox", "GTList#,id,x,y,w,h,Text$,flags");

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD, BL_STRING);
STDLIBS;
BLITZ_SUBS(A _gtcycle + 1, 0, 0);
BLITZ_ARGS(STD, BL_STRING, WPUSH);
STDLIBS;
BLITZ_SUBS(A _gtcycle2 + 1, 0, 0);
BLITZ_NAME("GTCycle", "GTList#,id,x,y,w,h,Text$,flags,Options$[,active]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD, BL_LONG);
STDLIBS;
BLITZ_SUBS(A _gtinteger + 1, 0, 0);
BLITZ_NAME("GTInteger", "GTList#,id,x,y,w,h,Text$,flags,default");

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD, BL_ARRAY | BL_PUSH, WPUSH);
STDLIBS;
BLITZ_SUBS(A _gtlistview1 + 1, 0, 0);
BLITZ_ARGS(STD, BL_ARRAY | BL_PUSH, WPUSH, WPUSH);
STDLIBS;
BLITZ_SUBS(A _gtlistview2 + 1, 0, 0);
BLITZ_ARGS(STD, BL_ARRAY | BL_PUSH);
STDLIBS;
BLITZ_SUBS(A _gtlistview + 1, 0, 0);
BLITZ_NAME("GTListView", "GTList#,id,x,y,w,h,Text$,flags,list()[,selected[,top]]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD, BL_STRING);
STDLIBS;
BLITZ_SUBS(A _gtmx + 1, 0, 0);
BLITZ_ARGS(STD, BL_STRING, WPUSH);
STDLIBS;
BLITZ_SUBS(A _gtmx2 + 1, 0, 0);
BLITZ_NAME("GTMX", "GTList#,id,x,y,w,h,Text$,flags,Options$[,active]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD, BL_LONG);
STDLIBS;
BLITZ_SUBS(A _gtnumber + 1, 0, 0);
BLITZ_NAME("GTNumber", "GTList#,id,x,y,w,h,Text$,flags,value");

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD, BL_LONG);
STDLIBS;
BLITZ_SUBS(A _gtpalette + 1, 0, 0);
BLITZ_ARGS(STD, BL_LONG, WPUSH);
STDLIBS;
BLITZ_SUBS(A _gtpalette2 + 1, 0, 0);
BLITZ_NAME("GTPalette", "GTList#,id,x,y,w,h,Text$,flags,depth[,Color]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD, BL_WORD, BL_WORD);
STDLIBS;
BLITZ_SUBS(A _gtscroller + 1, 0, 0);
BLITZ_ARGS(STD, BL_WORD, BL_WORD, WPUSH);
STDLIBS;
BLITZ_SUBS(A _gtscroller2 + 1, 0, 0);
BLITZ_NAME("GTScroller", "GTList#,id,x,y,w,h,Text$,flags,Visible,Total[,Top]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD, BL_WORD, BL_WORD);
STDLIBS;
BLITZ_SUBS(A _gtslider + 1, 0, 0);
BLITZ_ARGS(STD, BL_WORD, BL_WORD, WPUSH);
STDLIBS;
BLITZ_SUBS(A _gtslider2 + 1, 0, 0);
BLITZ_NAME("GTSlider", "GTList#,id,x,y,w,h,Text$,flags,Min,Max[,Level]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD, BL_WORD);
STDLIBS;
BLITZ_SUBS(A _gtstring + 1, 0, 0);
BLITZ_ARGS(STD, BL_WORD, BL_STRING | BL_PUSH);
STDLIBS;
BLITZ_SUBS(A _gtstring2 + 1, 0, 0);
BLITZ_NAME("GTString", "GTList#,id,x,y,w,h,Text$,flags,MaxChars [,default$]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(STD, BL_STRING);
STDLIBS;
BLITZ_SUBS(A _gttext + 1, 0, 0);
BLITZ_NAME("GTText", "GTList#,id,x,y,w,h,Text$,flags,Display$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180, LIB_windowslib, 0x1081, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(A _attachgtlist + 1, attachchk, 0);
BLITZ_NAME("AttachGTList", "GTList#,Window#");

BLITZ_ASTATEMENT();
BLITZ_REPARGS(0, 2, BL_LONG | BL_PUSH, BL_LONG | BL_PUSH);
BLITZ_LIBS0();
BLITZ_SUBS(A _gttags + 1, 0, 0);
BLITZ_NAME("GTTags", "Tag,Value [,Tag,Value...]");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(A _gtptr + 1, 0, 0);
BLITZ_NAME("GTGadPtr", "(GTList#,id)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1080);
BLITZ_SUBS(A _gtbevel + 1, 0, 0);
BLITZ_NAME("GTBevelBox", "GTList#,x,y,w,h,flags");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_ARRAY | BL_PUSH);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(A _gtchnglist1 + 1, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(A _gtchnglist2 + 1, 0, 0);
BLITZ_NAME("GTChangeList", "GTList#,id [ ,List() ]");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_REPARGS(2, 2, BL_WORD, BL_WORD, BL_LONG | BL_PUSH, BL_LONG | BL_PUSH);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(A _gtsetattrs + 1, 0, 0);
BLITZ_NAME("GTSetAttrs", "GTList#,id [,Tag,Value...]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(A _gtsetstring + 1, 0, 0);
BLITZ_NAME("GTSetString", "GTList#,id,string$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(A _gtsetint + 1, 0, 0);
BLITZ_NAME("GTSetInteger", "GTList#,id,value");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(A _gtgetstring + 1, 0, 0);
BLITZ_NAME("GTGetString", "GTList#,id");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(A _gtgetint + 1, 0, 0);
BLITZ_NAME("GTGetInteger", "GTList#,id");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1380, LIB_intuifontlib, BL_LA(0), LIB_screenslib, 0x1102,
           LIB_shapeslib, 0x1685);
BLITZ_SUBS(A _gtshape + 1, sgchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1380, LIB_intuifontlib, BL_LA(0), LIB_screenslib, 0x1102,
           LIB_shapeslib, 0x1685, LIB_shapeslib, 0x1286);
BLITZ_SUBS(A _gtshape2 + 1, sgchk2, 0);
BLITZ_NAME("GTShape", "GTList#,id,x,y,flags,Shape#[,Shape#]");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(A _gtgetattrs + 1, 0, 0);
BLITZ_NAME("GTGetAttrs", "(GTList#,id,Tag)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(doenable, 0, 0);
BLITZ_NAME("GTEnable", "GTList#,Id");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(dodisable, 0, 0);
BLITZ_NAME("GTDisable", "GTList#,Id");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(doselect, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(doselect2, 0, 0);
BLITZ_NAME("GTToggle", "GTList#,Id [,On|Off]");

BLITZ_AFUNCTION(BL_BYTE);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(_gtgetstatus, 0, 0);
BLITZ_NAME("GTStatus", "GTList#,Id");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_gtarrowsize, 0, 0);
BLITZ_NAME("GTArrowSize", "size");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(A _detachgtlist + 1, detachchk, 0);
BLITZ_NAME("DetachGTList", "GTList#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_gtcludge, 0, 0);
BLITZ_NAME("GTGimmeCludge", "On/Off");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(A _gtsethighlight + 1, 0, 0);
BLITZ_NAME("GTSetHighlight", "GTList#,id,value");

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_mygadtoolslib, 0x1180);
BLITZ_SUBS(A _gtfreegadget + 1, freegadchk, 0);
BLITZ_NAME("GTFreeGadget", "GTList#,id");

BLITZ_NULLSUB_AT(init, _codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, _codefinit, 0, 0);

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, _freecontext, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,5,4} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 4);

/* ----------------------------------------------------------------- */

/* the high and low byte of a (big endian) word, for the byte operations of
   the original on Gadget.Flags and Activation */
#define HI(w) (((UBYTE *)&(w))[0])
#define LO(w) (((UBYTE *)&(w))[1])
#define UW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define L(p, o)  (*(ULONG *)((UBYTE *)(p) + (o)))

/* the GTList object */
struct gtcontext {
    struct Gadget *context;     /* 0 */
    APTR vinfo;                 /* 4 */
    struct Gadget *current;     /* 8 */
    struct Window *window;      /* 12 */
};

static struct Library *gtbase;
static WORD cludge = -1;        /* 0 = GTGimmeCludge on: no border offsets */
static ULONG arrowsize = 16;
/* GTTags' pairs (at most 15 and the end, not checked) */
static struct TagItem usertags[16];
static_assert(sizeof usertags == 32 * 4);
/* GTSetString/GTSetInteger: {tag, value, 0} */
static struct {
    struct TagItem tag;
    ULONG end;
} onetag;
static_assert(sizeof onetag == 3 * 4);
/* GTGetAttrs: {tag, &value, 0} */
static struct {
    struct TagItem tag;
    ULONG end;
} gettag = { { 0, 0 }, TAG_END };
static_assert(sizeof gettag == 3 * 4);
static ULONG value;
static struct {
    struct TagItem vinfo, recessed;
    ULONG end;
} beveltags = { { GT_VisualInfo, 0 }, { GTBB_Recessed, 0 }, TAG_END };
static_assert(sizeof beveltags == 5 * 4);
static struct {
    struct TagItem labels;
    ULONG end;
} gatchtags = { { GTLV_Labels, 0 }, TAG_END };
static_assert(sizeof gatchtags == 3 * 4);
static struct {
    struct TagItem selected, makevisible, end;
} hightags = { { GTLV_Selected, 0 }, { GTLV_MakeVisible, 0 }, { TAG_END, 0 } };
static_assert(sizeof hightags == 6 * 4);
static struct TagItem nulltags[2];              /* GetVisualInfo: no tags */
static_assert(sizeof nulltags == 4 * 4);

/* tag lists of the creators: ..., more = {TAG_MORE or 0, usertags} (set by
   create) */
#define MORE { TAG_END, (ULONG)usertags }
#define UNDERSC { T_UNDERSC, '_' }
static struct {
    struct TagItem disabled, immediate, underscore, more;
} buttags = { { T_DISABLED, 0 }, { T_IMMEDIATE, 0 }, UNDERSC, MORE };
static_assert(sizeof buttags == 8 * 4);
static struct {
    struct TagItem disabled, checked, scaled, underscore, more;
} boxtags = { { T_DISABLED, 0 }, { GTCB_Checked, 0 }, { GTCB_Scaled, 0 }, UNDERSC, MORE };
static_assert(sizeof boxtags == 10 * 4);
static struct {
    struct TagItem disabled, labels, active, underscore, more;
} cycletags = { { T_DISABLED, 0 }, { GTCY_Labels, 0 }, { GTCY_Active, 0 }, UNDERSC, MORE };
static_assert(sizeof cycletags == 10 * 4);
static struct {
    struct TagItem disabled, labels, scaled, active, underscore, more;
} mxtags = { { T_DISABLED, 0 }, { GTMX_Labels, 0 }, { GTMX_Scaled, 0 }, { GTMX_Active, 0 },
             UNDERSC, MORE };
static_assert(sizeof mxtags == 12 * 4);
static struct {
    struct TagItem text, border, more;
} texttags = { { GTTX_Text, 0 }, { GTTX_Border, (ULONG)-1 }, MORE };
static_assert(sizeof texttags == 6 * 4);
static struct {
    struct TagItem disabled, immediate, number, underscore, more;
} inttags = { { T_DISABLED, 0 }, { T_IMMEDIATE, 0 }, { GTIN_Number, 0 }, UNDERSC, MORE };
static_assert(sizeof inttags == 10 * 4);
static struct {
    struct TagItem disabled, labels, top, selected, readonly, underscore, more;
} listtags = { { T_DISABLED, 0 }, { GTLV_Labels, 0 }, { GTLV_Top, 0 },
               { GTLV_Selected, (ULONG)-1 }, { GTLV_ReadOnly, 0 }, UNDERSC, MORE };
static_assert(sizeof listtags == 14 * 4);
static struct {
    struct TagItem number, border, more;
} numbertags = { { GTNM_Number, 0 }, { GTNM_Border, (ULONG)-1 }, MORE };
static_assert(sizeof numbertags == 6 * 4);
static struct {
    struct TagItem disabled, depth, color, underscore, more;
} palettetags = { { T_DISABLED, 0 }, { GTPA_Depth, 0 }, { GTPA_Color, 0 }, UNDERSC, MORE };
static_assert(sizeof palettetags == 10 * 4);
static struct {
    struct TagItem disabled, immediate, freedom, visible, total, arrows, top, underscore,
        relverify, more;
} scrollertags = { { T_DISABLED, 0 }, { T_IMMEDIATE, 0 }, { T_FREEDOM, 0 },
                   { GTSC_Visible, 0 }, { GTSC_Total, 0 }, { GTSC_Arrows, 16 },
                   { GTSC_Top, 0 }, UNDERSC, { T_RELVERIFY, (ULONG)-1 }, MORE };
static_assert(sizeof scrollertags == 20 * 4);
static struct {
    struct TagItem disabled, immediate, freedom, min, max, level, underscore, relverify, more;
} slidertags = { { T_DISABLED, 0 }, { T_IMMEDIATE, 1 }, { T_FREEDOM, 0 }, { GTSL_Min, 0 },
                 { GTSL_Max, 0 }, { GTSL_Level, 0 }, UNDERSC, { T_RELVERIFY, (ULONG)-1 },
                 MORE };
static_assert(sizeof slidertags == 18 * 4);
static struct {
    struct TagItem disabled, immediate, maxchars, string, underscore, more;
} stringtags = { { T_DISABLED, 0 }, { T_IMMEDIATE, 0 }, { GTST_MaxChars, 0 },
                 { GTST_String, 0 }, UNDERSC, MORE };
static_assert(sizeof stringtags == 12 * 4);
static struct {
    struct TagItem disabled, immediate, underscore, more;
} shapetags = { { T_DISABLED, 0 }, { T_IMMEDIATE, 0 }, UNDERSC, MORE };
static_assert(sizeof shapetags == 8 * 4);

/* the low word of a tag's ti_Data (the original's MOVE.w to the label+2) */
#define LOWORD(ti) (((UWORD *)&(ti).ti_Data)[1])

/* GTShape: the two Images and the gadget flags */
static UWORD gflags;
static struct Image image1 = { 0, 0, 0, 0, 0, NULL, 3, 0, NULL };
static struct Image image2 = { 0, 0, 0, 0, 0, NULL, 3, 0, NULL };

static struct TextAttr *ufont;
static struct Screen *uscreen;
static struct NewGadget wrkgadget;

/* memory of the gadgets (fixed, #174: the labels of cycle/mx), memlib
   globalloc with this header in front: the GTList (the object, a3) and
   the gadget it belongs to (0 until CreateGadget returned, or if it
   failed); freed with the gadget or the GTList */
struct gtblock {
    struct gtblock *next;
    struct gtcontext *owner;
    struct Gadget *gad;
    ULONG size;                 /* with the header */
};
static struct gtblock *gtblocks;

static ULONG allocst(ULONG str, ULONG len)
{
    struct bl_regs r = { .d0 = str, .d1 = len };

    return BLITZ_ALIBJSR(TOK_allocst, r);
}

/* a block of `size` bytes for the GTList in a3 (MEMF_ANY, as the
   original's getoptions), in the list of blocks */
static struct gtblock *gtalloc(ULONG size)
{
    struct bl_regs r = { .d0 = sizeof(struct gtblock) + size, .d1 = 0 };
    struct gtblock *b = (struct gtblock *)BLITZ_ALIBJSR(TOK_globalloc, r);

    b->next = gtblocks;
    b->owner = (struct gtcontext *)blitz_string;
    b->gad = NULL;
    b->size = sizeof(struct gtblock) + size;
    gtblocks = b;
    return b;
}

/* free the blocks of GTList c: all (gad 0) or those of gadget gad */
static void gtfreeblocks(struct gtcontext *c, struct Gadget *gad)
{
    struct gtblock **link = &gtblocks, *b;

    while ((b = *link) != NULL) {
        if (b->owner == c && (!gad || b->gad == gad)) {
            struct bl_regs r = { .d0 = b->size, .a1 = (ULONG)b };

            *link = b->next;
            BLITZ_ALIBJSR(TOK_globfree, r);
        } else
            link = &b->next;
    }
}

/* SNE d1:EXT.w d1:EXT.l d1 */
static ULONG bit(ULONG flags, int n)
{
    return (flags & (1UL << n)) ? (ULONG)-1 : 0;
}

/* -------------------------------------------------------- init/finit */

/* _prehandler/_posthandler: d0 = IntuiMessage -> (Post)FilterIMsg;
   a6 = ExecBase on return (MOVE.l 4,a6) */
static ULONG BLITZ_ROUTINE prehandler_c(struct IntuiMessage *m BL_REG(d0))
{
    return (ULONG)GT_FilterIMsg(m);
}
BLITZ_THUNK_POST(_prehandler, prehandler_c, "d1", "move.l  4.w,a6");

static ULONG BLITZ_ROUTINE posthandler_c(struct IntuiMessage *m BL_REG(d0))
{
    return (ULONG)GT_PostFilterIMsg(m);
}
BLITZ_THUNK_POST(_posthandler, posthandler_c, "d1", "move.l  4.w,a6");
void _prehandler(void), _posthandler(void);

void BLITZ_ROUTINE _codeinit(void)
{
    cludge = -1;
    gtbase = OpenLibrary((CONST_STRPTR)"gadtools.library", 0);
    if (gtbase) {
        struct bl_regs r = { .d0 = (ULONG)_prehandler, .d1 = (ULONG)_posthandler };
        BLITZ_ALIBJSR(TOK_setevfilter, r);
    }
}

void BLITZ_ROUTINE _codefinit(void)
{
    if (gtbase) {
        CloseLibrary(gtbase);
        gtbase = NULL;
    }
}

/**
 * @brief Free GTList#: the object's free routine (_free).
 *
 * Frees the VisualInfo and all gadgets of the GTList (a3) and clears the
 * context and the window; nothing for a GTList without gadgets.  It does
 * not detach the gadgets from a window.  Fixed (#174): also the label
 * blocks of its cycle/mx gadgets.
 */
void BLITZ_ROUTINE _freecontext(void)
{
    struct gtcontext *c = (struct gtcontext *)blitz_string;

    if (!c->context)
        return;
    FreeVisualInfo(c->vinfo);
    FreeGadgets(c->context);
    gtfreeblocks(c, NULL);
    c->context = NULL;
    c->window = NULL;
}

/* ------------------------------------------------------- the creators */

/* _getcontext - the context gadget (id -1) and the VisualInfo */
static void getcontext(struct gtcontext *c)
{
    if (c->context)
        return;
    c->context = CreateContext(&c->context);
    c->current = c->context;
    c->context->GadgetID = (UWORD)-1;   /* (binary) */
    c->vinfo = GetVisualInfoA(uscreen, nulltags);
}

/* _handlestd - the NewGadget; returns the a2 cursor behind the label */
static const ULONG *handlestd(UWORD id, WORD x, WORD y, WORD w, WORD h,
                              struct TextAttr **font, struct Screen **scr,
                              const ULONG *args, int shape)
{
    struct gtcontext *c = (struct gtcontext *)blitz_string;
    struct Screen *screen;

    wrkgadget.ng_GadgetID = id;
    ufont = *font;
    uscreen = screen = *scr;
    if (cludge) {
        x += *(UBYTE *)&screen->WBorLeft;
        y += *(UBYTE *)&screen->WBorTop;
        y += *(WORD *)&screen->RastPort.TxHeight;  /* (WORD: same code) */
        y += 1;
    }
    getcontext(c);
    wrkgadget.ng_LeftEdge = x;
    wrkgadget.ng_TopEdge = y;
    if (shape) {
        wrkgadget.ng_TextAttr = ufont;
        wrkgadget.ng_Width = image1.Width;
        wrkgadget.ng_Height = image1.Height;
    } else {
        ULONG len, str;
        wrkgadget.ng_Width = w;
        wrkgadget.ng_Height = h;
        wrkgadget.ng_TextAttr = ufont;
        len = BL_POPL(args);
        str = BL_POPL(args);
        wrkgadget.ng_GadgetText = (UBYTE *)allocst(str, len);
    }
    wrkgadget.ng_VisualInfo = c->vinfo;
    return args;
}

/* ng_Flags = flags & $3F (low word); TAG_MORE if GTTags were given;
   CreateGadget; usertags cleared.  more: the long index of the last
   TagItem's ti_Tag (the original's offset / 4, scaled as a long) */
static struct Gadget *create(ULONG kind, ULONG flags, struct TagItem *tags, int more)
{
    struct gtcontext *c = (struct gtcontext *)blitz_string;
    struct Gadget *gad;

    UW(&wrkgadget.ng_Flags, 2) = flags & 0x3f;
    ((ULONG *)tags)[more] = usertags[0].ti_Tag ? TAG_MORE : TAG_END;
    gad = CreateGadgetA(kind, c->current, &wrkgadget, tags);
    c->current = gad;
    usertags[0].ti_Tag = TAG_END;
    return gad;
}
/* create with a creator's tag list (its last TagItem is `more`) */
#define CREATE(kind, flags, tl) \
    create(kind, flags, (struct TagItem *)&(tl), (sizeof(tl) - sizeof(struct TagItem)) / 4)

/* the arguments of every creator (STDLIBS): GTList# in d0 becomes the
   object in a3 (!libs {#mygadtoolslib,$1380}), d1 = id, d2/d3 = x/y,
   d4/d5 = width/height, a0 = intuifontlib's `textattr` variable (the
   current IntuiFont), a1 = screenslib's used screen variable, a2 = the
   stack of the further arguments: Text$ (length, then address), flags,
   then the creator's own values; optional values are read at fixed
   negative offsets from a2 */
#define STDARGS UWORD id BL_REG(d1), WORD x BL_REG(d2), WORD y BL_REG(d3), \
    WORD w BL_REG(d4), WORD h BL_REG(d5), struct TextAttr **font BL_REG(a0), \
    struct Screen **scr BL_REG(a1), const ULONG *args BL_REG(a2)
#define HANDLESTD(a) handlestd(id, x, y, w, h, font, scr, a, 0)

/**
 * @brief GTButton GTList#,id,x,y,w,h,Text$,flags: add a BUTTON_KIND gadget.
 *
 * Fills the NewGadget (handlestd: position plus the screen's border sizes
 * and font height unless GTGimmeCludge is on, the label copied with
 * allocst) and creates the gadget behind the last one of the GTList (the
 * context is created on first use).  Flag bits: 6 GA_Disabled, 7
 * GA_Immediate, 13 toggle (TOGGLESELECT); ng_Flags = flags & $3F.  A
 * GTTags list given before is appended with TAG_MORE (then cleared).
 * Amiga mode only.
 * BUG(orig) (#171): with bit 13 TOGGLESELECT is set even if CreateGadget
 * failed (writes address 14).  BUG(orig) (#176), all creators: without a
 * used screen the border sizes are read from address 0.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags (see STDARGS); the GTList in a3
 */
static void BLITZ_ROUTINE gtbutton_c(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp);
    struct Gadget *gad;

    buttags.disabled.ti_Data = bit(f, 6);
    buttags.immediate.ti_Data = bit(f, 7);
    gad = CREATE(BUTTON_KIND, f, buttags);
    if (f & (1UL << 13)) {
        __asm__("" : "+a"(gad));        /* may be 0: BUG(orig) (#171) */
        HI(gad->Activation) |= GACT_TOGGLESELECT >> 8;
    }
}

/**
 * @brief GTCheckBox GTList#,id,x,y,w,h,Text$,flags: add a CHECKBOX_KIND
 * gadget.
 *
 * As GTButton.  Flag bits: 6 GA_Disabled, 8 GTCB_Checked, 9 GTCB_Scaled.
 * Amiga mode only.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags (see STDARGS); the GTList in a3
 */
static void BLITZ_ROUTINE gtcheckbox_c(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp);

    boxtags.disabled.ti_Data = bit(f, 6);
    boxtags.checked.ti_Data = bit(f, 8);
    boxtags.scaled.ti_Data = bit(f, 9);
    CREATE(CHECKBOX_KIND, f, boxtags);
}

/* getoptions - "a|b|c" (length, string from the a2 stack) -> NULL
   terminated array of labels, the texts behind it.  Fixed (#174): the
   original's block was length + 258 bytes with the texts at +256 (room
   for 63 labels, not checked) and never freed; here the labels are
   counted, the block is a gtblock (*blk, its gadget set by the caller) */
static ULONG getoptions(const ULONG **sp, struct gtblock **blk)
{
    ULONG len = BL_POPL(*sp);
    const UBYTE *s = (const UBYTE *)BL_POPL(*sp);
    UWORD n = (UWORD)len, k;
    ULONG count = 1;                    /* labels */
    UBYTE *text;
    UBYTE **labels, **first;

    for (k = 0; k < n; k++)
        if (s[k] == '|')
            count++;
    *blk = gtalloc((count + 1) * sizeof(UBYTE *) + n + 1);
    first = labels = (UBYTE **)(*blk + 1);
    text = (UBYTE *)(labels + count + 1);
    *labels++ = text;
    while (n-- != 0) {                  /* DBF d2 */
        UBYTE c = *s++;
        if (c == '|') {
            *text++ = 0;
            *labels++ = text;
        } else
            *text++ = c;
    }
    *text = 0;
    *labels = NULL;
    return (ULONG)first;
}

static void cycle(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp);
    struct gtblock *blk;

    cycletags.labels.ti_Data = getoptions(&sp, &blk);
    cycletags.disabled.ti_Data = bit(f, 6);
    blk->gad = CREATE(CYCLE_KIND, f, cycletags);
}

/**
 * @brief GTCycle GTList#,id,x,y,w,h,Text$,flags,Options$: add a
 * CYCLE_KIND gadget.
 *
 * As GTButton.  Options$ ("a|b|c") becomes the GTCY_Labels array
 * (getoptions: memlib globalloc, freed with the gadget or the GTList;
 * fixed, #174: the original had room for 63 labels without a check and
 * never freed it).  Flag bit 6 GA_Disabled.  Without `active` GTCY_Active
 * is 0 (fixed, #169: the original kept the value of an earlier GTCycle
 * with it).  Amiga mode only.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, Options$ (see STDARGS); the GTList in a3
 */
static void BLITZ_ROUTINE gtcycle_c(STDARGS)
{
    cycletags.active.ti_Data = 0;               /* fixed (#169) */
    cycle(id, x, y, w, h, font, scr, args);
}

/**
 * @brief GTCycle GTList#,id,x,y,w,h,Text$,flags,Options$,active: GTCycle
 * with the active entry (GTCY_Active, the word at a2-22).
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, Options$, active (see STDARGS)
 */
static void BLITZ_ROUTINE gtcycle2_c(STDARGS)
{
    LOWORD(cycletags.active) = UW(args, -22);
    cycle(id, x, y, w, h, font, scr, args);
}

static void mx(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp);
    struct gtblock *blk;

    mxtags.labels.ti_Data = getoptions(&sp, &blk);
    mxtags.disabled.ti_Data = bit(f, 6);
    mxtags.scaled.ti_Data = bit(f, 9);
    blk->gad = CREATE(MX_KIND, f, mxtags);
}

/**
 * @brief GTMX GTList#,id,x,y,w,h,Text$,flags,Options$: add an MX_KIND
 * gadget.
 *
 * As GTCycle (Options$ -> GTMX_Labels, fixed #174).  Flag bits: 6
 * GA_Disabled, 9 GTMX_Scaled.  Without `active` GTMX_Active is 0 (fixed,
 * #169: the original kept an earlier value).  Amiga mode only.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, Options$ (see STDARGS); the GTList in a3
 */
static void BLITZ_ROUTINE gtmx_c(STDARGS)
{
    mxtags.active.ti_Data = 0;                  /* fixed (#169) */
    mx(id, x, y, w, h, font, scr, args);
}

/**
 * @brief GTMX GTList#,id,x,y,w,h,Text$,flags,Options$,active: GTMX with
 * the active entry (GTMX_Active, the word at a2-22).
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, Options$, active (see STDARGS)
 */
static void BLITZ_ROUTINE gtmx2_c(STDARGS)
{
    LOWORD(mxtags.active) = UW(args, -22);
    mx(id, x, y, w, h, font, scr, args);
}

/**
 * @brief GTText GTList#,id,x,y,w,h,Text$,flags,Display$: add a TEXT_KIND
 * gadget.
 *
 * As GTButton; Display$ is copied with allocst (GTTX_Text), with
 * GTTX_Border.  No flag tags.  Amiga mode only.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, Display$ (see STDARGS); the GTList in a3
 */
static void BLITZ_ROUTINE gttext_c(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp), len, str;

    len = BL_POPL(sp);
    str = BL_POPL(sp);
    texttags.text.ti_Data = allocst(str, len);
    CREATE(TEXT_KIND, f, texttags);
}

/**
 * @brief GTInteger GTList#,id,x,y,w,h,Text$,flags,default: add an
 * INTEGER_KIND gadget.
 *
 * As GTButton, with GTIN_Number = default.  Flag bits: 6 GA_Disabled, 7
 * GA_Immediate.  Amiga mode only.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, default (see STDARGS); the GTList in a3
 */
static void BLITZ_ROUTINE gtinteger_c(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp);

    inttags.number.ti_Data = BL_POPL(sp);
    inttags.disabled.ti_Data = bit(f, 6);
    inttags.immediate.ti_Data = bit(f, 7);
    CREATE(INTEGER_KIND, f, inttags);
}

static void listview(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp);

    listtags.labels.ti_Data = BL_POPL(sp) - 28;     /* the List header of the array */
    listtags.disabled.ti_Data = bit(f, 6);
    listtags.readonly.ti_Data = bit(f, 12);
    CREATE(LISTVIEW_KIND, f, listtags);
}

/**
 * @brief GTListView GTList#,id,x,y,w,h,Text$,flags,list(): add a
 * LISTVIEW_KIND gadget.
 *
 * As GTButton.  GTLV_Labels = the List header of the list array (its
 * address - 28).  Flag bits: 6 GA_Disabled, 12 GTLV_ReadOnly.  Without
 * selected/top GTLV_Selected is -1 and GTLV_Top 0 (fixed, #169: the
 * original kept the values of an earlier GTListView with them).  Amiga
 * mode only.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, list() (see STDARGS); the GTList in a3
 */
static void BLITZ_ROUTINE gtlistview_c(STDARGS)
{
    listtags.top.ti_Data = 0;                   /* fixed (#169) */
    listtags.selected.ti_Data = (ULONG)-1;
    listview(id, x, y, w, h, font, scr, args);
}

/**
 * @brief GTListView GTList#,id,x,y,w,h,Text$,flags,list(),selected:
 * GTListView with GTLV_Selected = GTLV_Top = selected (the word at a2-18).
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, list(), selected (see STDARGS)
 */
static void BLITZ_ROUTINE gtlistview1_c(STDARGS)
{
    LOWORD(listtags.top) = UW(args, -18);       /* top = selected */
    LOWORD(listtags.selected) = UW(args, -18);
    listview(id, x, y, w, h, font, scr, args);
}

/**
 * @brief GTListView GTList#,id,x,y,w,h,Text$,flags,list(),selected,top:
 * GTListView with GTLV_Selected (the word at a2-18) and GTLV_Top (a2-22).
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, list(), selected, top (see STDARGS)
 */
static void BLITZ_ROUTINE gtlistview2_c(STDARGS)
{
    LOWORD(listtags.selected) = UW(args, -18);  /* selected */
    LOWORD(listtags.top) = UW(args, -22);       /* top */
    listview(id, x, y, w, h, font, scr, args);
}

/**
 * @brief GTNumber GTList#,id,x,y,w,h,Text$,flags,value: add a NUMBER_KIND
 * gadget.
 *
 * As GTButton, with GTNM_Number = value and GTNM_Border.  No flag tags.
 * Amiga mode only.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, value (see STDARGS); the GTList in a3
 */
static void BLITZ_ROUTINE gtnumber_c(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp);

    numbertags.number.ti_Data = BL_POPL(sp);
    CREATE(NUMBER_KIND, f, numbertags);
}

static void palette(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp);

    palettetags.depth.ti_Data = BL_POPL(sp);
    palettetags.disabled.ti_Data = bit(f, 6);
    CREATE(PALETTE_KIND, f, palettetags);
}

/**
 * @brief GTPalette GTList#,id,x,y,w,h,Text$,flags,depth: add a
 * PALETTE_KIND gadget.
 *
 * As GTButton, with GTPA_Depth = depth and GTPA_Color 0 (fixed, #168:
 * not the colour of an earlier GTPalette with it).  Flag bit 6
 * GA_Disabled.  Amiga mode only.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, depth (see STDARGS); the GTList in a3
 */
static void BLITZ_ROUTINE gtpalette_c(STDARGS)
{
    palettetags.color.ti_Data = 0;
    palette(id, x, y, w, h, font, scr, args);
}

/**
 * @brief GTPalette GTList#,id,x,y,w,h,Text$,flags,depth,Color: GTPalette
 * with a colour (the word at a2-18).
 *
 * Fixed (#168): the colour goes to GTPA_Color's value; the original wrote
 * it to +38 of the tag list, the low word of the TAG_MORE pointer (the
 * usertags address; the source layout had two more tags): the colour was
 * ignored, a GTTags list after it was reached through a wrong pointer.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, depth, Color (see STDARGS)
 */
static void BLITZ_ROUTINE gtpalette2_c(STDARGS)
{
    /* fixed (#168): the original's +38 is the low word of .more */
    LOWORD(palettetags.color) = UW(args, -18);
    palette(id, x, y, w, h, font, scr, args);
}

static void scroller(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp);
    const UWORD *wp = (const UWORD *)sp;

    LOWORD(scrollertags.visible) = BL_POPW(wp);
    LOWORD(scrollertags.total) = BL_POPW(wp);
    scrollertags.disabled.ti_Data = bit(f, 6);
    scrollertags.immediate.ti_Data = bit(f, 7);
    scrollertags.freedom.ti_Data = (f & (1UL << 10)) ? 2 : 1;
    scrollertags.arrows.ti_Data = (f & (1UL << 11)) ? arrowsize : 0;
    CREATE(SCROLLER_KIND, f, scrollertags);
}

/**
 * @brief GTScroller GTList#,id,x,y,w,h,Text$,flags,Visible,Total: add a
 * SCROLLER_KIND gadget.
 *
 * As GTButton, with GTSC_Visible and GTSC_Total (words), GA_RelVerify.
 * Flag bits: 6 GA_Disabled, 7 GA_Immediate, 10 vertical (PGA_Freedom 2,
 * else 1), 11 arrows (GTSC_Arrows = GTArrowSize's size, else 0).  Without
 * Top GTSC_Top is 0 (fixed, #169: the original kept an earlier value).
 * Amiga mode only.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, Visible, Total (see STDARGS); the GTList
 *              in a3
 */
static void BLITZ_ROUTINE gtscroller_c(STDARGS)
{
    scrollertags.top.ti_Data = 0;               /* fixed (#169) */
    scroller(id, x, y, w, h, font, scr, args);
}

/**
 * @brief GTScroller GTList#,id,x,y,w,h,Text$,flags,Visible,Total,Top:
 * GTScroller with GTSC_Top (the word at a2-18).
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, Visible, Total, Top (see STDARGS)
 */
static void BLITZ_ROUTINE gtscroller2_c(STDARGS)
{
    LOWORD(scrollertags.top) = UW(args, -18);
    scroller(id, x, y, w, h, font, scr, args);
}

static void slider(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp);
    const UWORD *wp = (const UWORD *)sp;

    LOWORD(slidertags.min) = BL_POPW(wp);
    LOWORD(slidertags.max) = BL_POPW(wp);
    slidertags.disabled.ti_Data = bit(f, 6);
    slidertags.immediate.ti_Data = bit(f, 7);
    slidertags.freedom.ti_Data = (f & (1UL << 10)) ? 2 : 1;
    CREATE(SLIDER_KIND, f, slidertags);
}

/**
 * @brief GTSlider GTList#,id,x,y,w,h,Text$,flags,Min,Max: add a
 * SLIDER_KIND gadget.
 *
 * As GTButton, with GTSL_Min and GTSL_Max (words), GA_RelVerify.  Flag
 * bits: 6 GA_Disabled, 7 GA_Immediate (the tag list's initial value is 1),
 * 10 vertical (PGA_Freedom 2, else 1).  Without Level GTSL_Level is 0
 * (fixed, #169: the original kept an earlier value).  Amiga mode only.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, Min, Max (see STDARGS); the GTList in a3
 */
static void BLITZ_ROUTINE gtslider_c(STDARGS)
{
    slidertags.level.ti_Data = 0;               /* fixed (#169) */
    slider(id, x, y, w, h, font, scr, args);
}

/**
 * @brief GTSlider GTList#,id,x,y,w,h,Text$,flags,Min,Max,Level: GTSlider
 * with GTSL_Level (the word at a2-18).
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, Min, Max, Level (see STDARGS)
 */
static void BLITZ_ROUTINE gtslider2_c(STDARGS)
{
    LOWORD(slidertags.level) = UW(args, -18);
    slider(id, x, y, w, h, font, scr, args);
}

static void string(STDARGS)
{
    const ULONG *sp = HANDLESTD(args);
    ULONG f = BL_POPL(sp);
    const UWORD *wp = (const UWORD *)sp;

    LOWORD(stringtags.maxchars) = BL_POPW(wp);
    stringtags.disabled.ti_Data = bit(f, 6);
    stringtags.immediate.ti_Data = bit(f, 7);
    CREATE(STRING_KIND, f, stringtags);
}

/**
 * @brief GTString GTList#,id,x,y,w,h,Text$,flags,MaxChars: add a
 * STRING_KIND gadget.
 *
 * As GTButton, with GTST_MaxChars (word).  Flag bits: 6 GA_Disabled, 7
 * GA_Immediate.  Without default$ GTST_String is NULL (fixed, #169: the
 * original kept the pointer of an earlier GTString with it, into the old
 * string work area).  Amiga mode only.
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, MaxChars (see STDARGS); the GTList in a3
 */
static void BLITZ_ROUTINE gtstring_c(STDARGS)
{
    stringtags.string.ti_Data = 0;              /* fixed (#169) */
    string(id, x, y, w, h, font, scr, args);
}

/**
 * @brief GTString GTList#,id,x,y,w,h,Text$,flags,MaxChars,default$:
 * GTString with GTST_String = default$ (the long at a2-22).
 *
 * @param id,x,y,w,h  d1-d5: gadget id, position and size
 * @param font  a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr   a1: the used screen (screenslib)
 * @param args  a2: Text$, flags, MaxChars, default$ (see STDARGS)
 */
static void BLITZ_ROUTINE gtstring2_c(STDARGS)
{
    stringtags.string.ti_Data = L(args, -22);   /* default$ */
    string(id, x, y, w, h, font, scr, args);
}

/* GTShape l,id,x,y,flags,shape[,shape2]: a6 = shape, a2 = shape 2 */
#define SHAPEARGS UWORD id BL_REG(d1), WORD x BL_REG(d2), WORD y BL_REG(d3), \
    ULONG flags BL_REG(d4), struct TextAttr **font BL_REG(a0), \
    struct Screen **scr BL_REG(a1)

static void doshape(UWORD id, WORD x, WORD y, ULONG flags,
                    struct TextAttr **font, struct Screen **scr)
{
    struct Gadget *gad;

    handlestd(id, x, y, 0, 0, font, scr, NULL, 1);
    shapetags.disabled.ti_Data = bit(flags, 6);
    shapetags.immediate.ti_Data = bit(flags, 7);
    gad = CREATE(BUTTON_KIND, flags, shapetags);
    usertags[0].ti_Tag = TAG_END;
    if (!gad) {                 /* fixed (#327): the original went on with 0 */
        gflags = 0;
        return;
    }
    if (gad->GadgetRender) {
        ULONG *dst = (ULONG *)gad->GadgetRender, *src = (ULONG *)&image1;
        int i;
        for (i = 0; i < 5; i++)
            *dst++ = *src++;
    } else
        gad->GadgetRender = &image1;
    gad->SelectRender = &image2;
    gad->Flags = gflags;
    gad->Activation = GACT_RELVERIFY;
    gad->GadgetType = gad->GadgetType + GTYP_BOOLGADGET - 1;
    gflags = 0;
}

static void shape1(const struct bb_shape *shape)
{
    image1.Width = shape->width;
    image1.Height = shape->height;
    image1.Depth = shape->depth;
    image1.ImageData = (UWORD *)shape->data;
    gflags += GFLG_GADGIMAGE;
}

/**
 * @brief GTShape GTList#,id,x,y,flags,Shape#: add a BUTTON_KIND gadget
 * that shows a shape.
 *
 * The shape's width, height, depth and data become the static Image 1
 * (GFLG_GADGIMAGE); the gadget gets the shape's size, GACT_RELVERIFY
 * and the boolean gadget type, GadgetRender = Image 1, SelectRender =
 * Image 2.  Flag bits: 6 GA_Disabled, 7 GA_Immediate.  Amiga mode only.
 * Errors (debugger only): "Uninitialized Shape" (sgchk).
 * BUG(orig): the label of the previous gadget stays in the NewGadget
 * (#172); an existing GadgetRender is overwritten with Image 1 (20 bytes)
 * (#173).  Fixed (#327): if CreateGadget fails, nothing is written (the
 * original wrote to addresses 12-25).
 *
 * @param id,x,y  d1-d3: gadget id and position
 * @param flags   d4: the flags
 * @param font    a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr     a1: the used screen (screenslib)
 * @param shape   a6: the shape (!libs {#shapeslib,$1685}); the GTList in a3
 */
static void BLITZ_ROUTINE gtshape_c(SHAPEARGS, const struct bb_shape *shape BL_REG(a6))
{
    shape1(shape);
    doshape(id, x, y, flags, font, scr);
}

/**
 * @brief GTShape GTList#,id,x,y,flags,Shape#,Shape#: GTShape with a second
 * shape as the selected image (Image 2, GFLG_GADGHIMAGE).
 *
 * Errors (debugger only): "Uninitialized Shape" (sgchk2).
 *
 * @param id,x,y  d1-d3: gadget id and position
 * @param flags   d4: the flags
 * @param font    a0: the current IntuiFont (intuifontlib's textattr)
 * @param scr     a1: the used screen (screenslib)
 * @param shape2  a2: the second shape (!libs {#shapeslib,$1286})
 * @param shape   a6: the shape (!libs {#shapeslib,$1685}); the GTList in a3
 */
static void BLITZ_ROUTINE gtshape2_c(SHAPEARGS, const struct bb_shape *shape2 BL_REG(a2),
                                     const struct bb_shape *shape BL_REG(a6))
{
    image2.Width = shape2->width;
    image2.Height = shape2->height;
    image2.Depth = shape2->depth;
    image2.ImageData = (UWORD *)shape2->data;
    gflags += GFLG_GADGHIMAGE;
    shape1(shape);
    doshape(id, x, y, flags, font, scr);
}

/* --------------------------------------------------------- the others */

/* AddGList with position -1 as a long (MOVEQ #-1,d0; the NDK inline
   passes the UWORD $FFFF) */
static void addglist(struct Library *ib, struct Window *w, struct Gadget *g)
{
    register struct Library *libreg __asm("a6") = ib;
    register struct Window *winreg __asm("a0") = w;
    register struct Gadget *gadreg __asm("a1") = g;
    register LONG position __asm("d0") = -1;
    register LONG numgad __asm("d1") = -1;
    register APTR requester __asm("a2") = NULL;

    __asm volatile("jsr -438(a6)"
        : "+a"(winreg), "+a"(gadreg), "+d"(position), "+d"(numgad), "+a"(requester)
        : "a"(libreg) : "cc", "memory");
}

/**
 * @brief AttachGTList GTList#,Window#: add the GTList's gadgets to a
 * window.
 *
 * AddGList (position -1, all gadgets), RefreshGList and GT_RefreshWindow;
 * the GTList remembers the window.  Amiga mode only.
 * Errors (debugger only): "GTList already attached to window!" if it is
 * attached to this window already (attachchk).
 *
 * @param c        a1: the GTList (!libs {#mygadtoolslib,$1180})
 * @param win      a0: the window object (!libs {#windowslib,$1081})
 * @param intbase  a6: intuition.library
 */
static void BLITZ_ROUTINE attachgtlist_c(struct gtcontext *c BL_REG(a1),
                                         struct Window **win BL_REG(a0),
                                         struct Library *intbase BL_REG(a6))
{
    struct Window *w = *win;
    struct Gadget *g;

    c->window = w;
    g = c->context;
    addglist(intbase, w, g);
    RefreshGList(g, w, NULL, -1);
    GT_RefreshWindow(w, NULL);
}

/**
 * @brief DetachGTList GTList#: remove the GTList's gadgets from its window.
 *
 * RemoveGList of all gadgets; nothing if the GTList is not attached.
 * Amiga mode only.
 * Errors (debugger only): "GTList not attached to a window" (detachchk).
 *
 * @param c        a1: the GTList (!libs {#mygadtoolslib,$1180})
 * @param intbase  a6: intuition.library
 */
static void BLITZ_ROUTINE detachgtlist_c(struct gtcontext *c BL_REG(a1),
                                         struct Library *intbase BL_REG(a6))
{
    struct Window *w = c->window;

    if (!w)
        return;
    c->window = NULL;
    RemoveGList(w, c->context, -1);
}

/* first gadget with the id after the head (0 = none) */
static struct Gadget *find(struct Gadget *gad, UWORD id)
{
    do {
        gad = gad->NextGadget;
        if (!gad)
            return NULL;
    } while (gad->GadgetID != id);
    return gad;
}

/* the last of a run of gadgets with the id */
static struct Gadget *lastof(struct Gadget *gad, UWORD id)
{
    struct Gadget *n;

    while ((n = gad->NextGadget) != NULL && n->GadgetID == id)
        gad = n;
    return gad;
}

/**
 * @brief GTFreeGadget GTList#,id: unlink the run of gadgets with the id
 * and free them.
 *
 * The whole run from the first to the last gadget with the id
 * (FreeGadgets), BUG(orig) (#175); fixed (#174): with the label blocks of
 * the freed cycle/mx gadgets.  Amiga mode only.
 * Errors (debugger only): "GTList must be detached from window first!"
 * (freegadchk).
 *
 * @param id  d1: the gadget id
 * @param c   a1: the GTList (!libs {#mygadtoolslib,$1180})
 * @return d0.w: the number of gadgets freed, 0 if there is none with the id
 */
static ULONG BLITZ_ROUTINE gtfreegadget_c(UWORD id BL_REG(d1),
                                          struct gtcontext *c BL_REG(a1))
{
    struct Gadget *gad = (struct Gadget *)c, *prev, *first, *last;
    UWORD n = 0;

    do {
        prev = gad;
        gad = gad->NextGadget;
        if (!gad)
            return 0;
    } while (gad->GadgetID != id);
    first = gad;
    do {
        n++;
        last = gad;
        gad = gad->NextGadget;
    } while (gad && gad->GadgetID == id);
    prev->NextGadget = last->NextGadget;
    last->NextGadget = NULL;
    for (gad = first; gad; gad = gad->NextGadget)
        gtfreeblocks(c, gad);
    FreeGadgets(first);
    return n;
}

/**
 * @brief GTBevelBox GTList#,x,y,w,h,flags: draw a bevel box into the
 * GTList's window.
 *
 * DrawBevelBox with the GTList's VisualInfo in the RastPort of the window
 * it is attached to (not checked).  Amiga mode only.
 * BUG(orig) (#170): flags <> 0 gives the tag GTBB_Recessed with value 0,
 * flags = 0 ends the tag list there: the box is never recessed.
 *
 * @param x,y,w,h  d1-d4: position and size
 * @param flags    d5: <> 0 meant as "recessed"
 * @param c        a0: the GTList (!libs {#mygadtoolslib,$1080})
 */
static void BLITZ_ROUTINE gtbevel_c(LONG x BL_REG(d1), LONG y BL_REG(d2),
                                    LONG w BL_REG(d3), LONG h BL_REG(d4),
                                    ULONG flags BL_REG(d5),
                                    struct gtcontext *c BL_REG(a0))
{
    beveltags.vinfo.ti_Data = (ULONG)c->vinfo;
    if (flags)
        flags = GTBB_Recessed;          /* BUG(orig) (#170): its value stays 0 */
    beveltags.recessed.ti_Tag = flags;  /* 0: TAG_END, the end of the list */
    DrawBevelBoxA(c->window->RPort, x, y, w, h, &beveltags.vinfo);
}

/**
 * @brief GTGadPtr(GTList#,id): the address of the gadget with the id.
 *
 * The FIRST gadget with the id (the search starts at the context gadget,
 * id -1), BUG(orig) (#175).  Amiga mode only.
 *
 * @param id  d1: the gadget id
 * @param c   a1: the GTList (!libs {#mygadtoolslib,$1180})
 * @return d0: the gadget, 0 if there is none
 */
static ULONG BLITZ_ROUTINE gtptr_c(UWORD id BL_REG(d1), struct gtcontext *c BL_REG(a1))
{
    return (ULONG)find((struct Gadget *)c, id);
}

/* set tags on the last gadget of the run with the id */
static ULONG setattrs(struct gtcontext *c, UWORD id, struct TagItem *tags, int last)
{
    struct Gadget *gad = find(c->context, id);

    if (!gad)
        return 0;
    if (last)
        gad = lastof(gad, id);
    /* BUG(orig) (#328): a3 = tags afterwards */
    GT_SetGadgetAttrsA(gad, c->window, NULL, tags);
    return 0;
}

/**
 * @brief GTChangeList GTList#,id,List(): give a listview new labels.
 *
 * GTLV_Labels = the List header of the list array (its address - 28) on
 * the LAST gadget of the run with the id, BUG(orig) (#175); nothing
 * without a context or gadget.  Amiga mode only.
 * BUG(orig) (#328): the tag list is left in a3.
 *
 * @param id    d1: the gadget id
 * @param c     a1: the GTList (!libs {#mygadtoolslib,$1180})
 * @param args  a2: the stack with the list array's address
 */
static void BLITZ_ROUTINE gtchnglist1_c(UWORD id BL_REG(d1),
                                        struct gtcontext *c BL_REG(a1),
                                        const ULONG *args BL_REG(a2))
{
    gatchtags.labels.ti_Data = args[-1] - 28;
    if (!c->context)
        return;
    setattrs(c, id, &gatchtags.labels, 1);
}

/**
 * @brief GTChangeList GTList#,id: detach the listview's labels
 * (GTLV_Labels = -1), as GTChangeList with a list.
 *
 * Unlike the form with a list it does not test for a context first.
 * BUG(orig) (#328): the tag list is left in a3.
 *
 * @param id  d1: the gadget id
 * @param c   a1: the GTList (!libs {#mygadtoolslib,$1180})
 */
static void BLITZ_ROUTINE gtchnglist2_c(UWORD id BL_REG(d1),
                                        struct gtcontext *c BL_REG(a1))
{
    gatchtags.labels.ti_Data = (ULONG)-1;
    setattrs(c, id, &gatchtags.labels, 1);
}

/**
 * @brief GTSetHighlight GTList#,id,value: select a listview entry and
 * make it visible.
 *
 * GTLV_Selected and GTLV_MakeVisible = value on the LAST gadget of the run
 * with the id, BUG(orig) (#175); nothing without a context or gadget.
 * Amiga mode only.
 * BUG(orig) (#328): the tag list is left in a3.
 *
 * @param id  d1: the gadget id
 * @param v   d2: the entry
 * @param c   a1: the GTList (!libs {#mygadtoolslib,$1180})
 */
static void BLITZ_ROUTINE gtsethighlight_c(UWORD id BL_REG(d1), ULONG v BL_REG(d2),
                                           struct gtcontext *c BL_REG(a1))
{
    hightags.selected.ti_Data = v;
    hightags.makevisible.ti_Data = v;
    if (!c->context)
        return;
    setattrs(c, id, &hightags.selected, 1);
}

/* _gttags - d7 pairs from the a2 stack into usertags */
static const ULONG *gttags(UWORD pairs, const ULONG *sp)
{
    struct TagItem *tag = usertags;

    while (pairs-- != 0) {
        tag->ti_Tag = BL_POPL(sp);
        tag->ti_Data = BL_POPL(sp);
        tag++;
    }
    tag->ti_Tag = TAG_END;
    return sp;
}

/**
 * @brief GTTags Tag,Value[,Tag,Value...]: extra tags for the next gadget.
 *
 * The pairs go to `usertags` (at most 15 without a check), which the next
 * creator appends with TAG_MORE and then clears.  DIFF: a2 is not moved
 * (the original pops the pairs).  Amiga mode only.
 *
 * @param pairs  d7: the number of pairs
 * @param args   a2: the stack of the pairs
 */
static void BLITZ_ROUTINE gttags_c(UWORD pairs BL_REG(d7), const ULONG *args BL_REG(a2))
{
    gttags(pairs, args);
}

/**
 * @brief GTSetAttrs GTList#,id[,Tag,Value...]: set tags of a gadget (also
 * a function).
 *
 * GT_SetGadgetAttrs with the pairs (put into `usertags`) on the LAST
 * gadget of the run with the id, BUG(orig) (#175); nothing without a
 * context or gadget.  DIFF: a2 is not moved.  Amiga mode only.
 * BUG(orig) (#328): a3 = usertags afterwards, also as a function.
 *
 * @param id     d1: the gadget id
 * @param pairs  d7: the number of pairs
 * @param c      a1: the GTList (!libs {#mygadtoolslib,$1180})
 * @param args   a2: the stack of the pairs
 * @return d0: 0 (the original returns what d0 holds)
 */
static ULONG BLITZ_ROUTINE gtsetattrs_c(UWORD id BL_REG(d1), UWORD pairs BL_REG(d7),
                                        struct gtcontext *c BL_REG(a1),
                                        const ULONG *args BL_REG(a2))
{
    struct Gadget *gad;

    if (!c->context)
        return 0;
    gad = find(c->context, id);
    if (!gad)
        return 0;
    gad = lastof(gad, id);
    gttags(pairs, args);
    /* BUG(orig) (#328): a3 = usertags afterwards (also as a function) */
    GT_SetGadgetAttrsA(gad, c->window, NULL, usertags);
    return 0;                           /* the original: what d0 holds */
}

/* GTSetString / GTSetInteger: STRING_KIND/INTEGER_KIND ($104) or
   TEXT_KIND/NUMBER_KIND */
static void setone(struct gtcontext *c, UWORD id, ULONG v, ULONG tstr, ULONG tother)
{
    struct Gadget *gad = find((struct Gadget *)c, id);

    if (!gad)
        return;
    onetag.tag.ti_Tag = gad->GadgetType == 0x104 ? tstr : tother;
    onetag.tag.ti_Data = v;
    /* BUG(orig) (#328): a3 = onetag afterwards */
    GT_SetGadgetAttrsA(gad, c->window, NULL, &onetag.tag);
}

/**
 * @brief GTSetString GTList#,id,string$: set the text of a string or text
 * gadget.
 *
 * GTST_String for a STRING_KIND gadget (GadgetType $104), else GTTX_Text,
 * on the FIRST gadget with the id (the search starts at the context
 * gadget); nothing if there is none.  Amiga mode only.
 * BUG(orig) (#328): the tag list is left in a3.
 *
 * @param id   d1: the gadget id
 * @param str  d2: the string
 * @param c    a1: the GTList (!libs {#mygadtoolslib,$1180})
 */
static void BLITZ_ROUTINE gtsetstring_c(UWORD id BL_REG(d1), ULONG str BL_REG(d2),
                                        struct gtcontext *c BL_REG(a1))
{
    setone(c, id, str, GTST_String, GTTX_Text);
}

/**
 * @brief GTSetInteger GTList#,id,value: set the value of an integer or
 * number gadget.
 *
 * GTIN_Number for an INTEGER_KIND gadget (GadgetType $104), else
 * GTNM_Number, as GTSetString.  Amiga mode only.
 * BUG(orig) (#328): the tag list is left in a3.
 *
 * @param id  d1: the gadget id
 * @param v   d2: the value
 * @param c   a1: the GTList (!libs {#mygadtoolslib,$1180})
 */
static void BLITZ_ROUTINE gtsetint_c(UWORD id BL_REG(d1), ULONG v BL_REG(d2),
                                     struct gtcontext *c BL_REG(a1))
{
    setone(c, id, v, GTIN_Number, GTNM_Number);
}

/**
 * @brief GTGetString(GTList#,id): the contents of a string gadget.
 *
 * Copies the StringInfo buffer of the FIRST gadget with the id to the
 * string buffer (a3, left behind the text).  Amiga mode only.
 *
 * @param id  d1: the gadget id
 * @param c   a1: the GTList (!libs {#mygadtoolslib,$1180})
 * @return d0: the length of the string, 0 if there is no such gadget
 */
static ULONG BLITZ_ROUTINE gtgetstring_c(UWORD id BL_REG(d1),
                                         struct gtcontext *c BL_REG(a1))
{
    struct Gadget *gad = find((struct Gadget *)c, id);
    const UBYTE *text;
    ULONG n = (ULONG)-1;

    if (!gad)
        return 0;
    text = ((struct StringInfo *)gad->SpecialInfo)->Buffer;
    do
        n++;
    while ((*blitz_string++ = *text++) != 0);
    blitz_string--;
    return n;
}

/**
 * @brief GTGetInteger(GTList#,id): the value of an integer gadget.
 *
 * The StringInfo's LongInt of the FIRST gadget with the id.  Amiga mode
 * only.
 *
 * @param id  d1: the gadget id
 * @param c   a1: the GTList (!libs {#mygadtoolslib,$1180})
 * @return d0: the value, 0 if there is no such gadget
 */
static ULONG BLITZ_ROUTINE gtgetint_c(UWORD id BL_REG(d1), struct gtcontext *c BL_REG(a1))
{
    struct Gadget *gad = find((struct Gadget *)c, id);

    if (!gad)
        return 0;
    return ((struct StringInfo *)gad->SpecialInfo)->LongInt;
}

/**
 * @brief GTGetAttrs(GTList#,id,Tag): read one attribute of a gadget.
 *
 * GT_GetGadgetAttrs of the tag into the library's `value` on the FIRST
 * gadget with the id.  Amiga mode only.
 * BUG(orig) (#328): a3 = gettag afterwards - in a string expression the
 * rest is written over gettag, value and what follows them.
 *
 * @param id   d1: the gadget id
 * @param tag  d2: the tag
 * @param c    a1: the GTList (!libs {#mygadtoolslib,$1180})
 * @return d0: the value (the previous `value` if GadTools does not set
 *         it), 0 if there is no such gadget
 */
static ULONG BLITZ_ROUTINE gtgetattrs_c(UWORD id BL_REG(d1), ULONG tag BL_REG(d2),
                                        struct gtcontext *c BL_REG(a1))
{
    struct Gadget *gad = find((struct Gadget *)c, id);

    if (!gad)
        return 0;
    gettag.tag.ti_Tag = tag;
    gettag.tag.ti_Data = (ULONG)&value;
    /* BUG(orig) (#328): a3 = gettag afterwards - in a string expression the rest
       is written over gettag, value and what follows them */
    GT_GetGadgetAttrsA(gad, c->window, NULL, &gettag.tag);
    return value;
}

/**
 * @brief GTStatus(GTList#,Id): whether a gadget is selected.
 *
 * GFLG_SELECTED of the FIRST gadget with the id, as 0/1 (not a Blitz
 * truth value).  Function of type byte.
 *
 * @param id  d1: the gadget id
 * @param c   a1: the GTList (!libs {#mygadtoolslib,$1180})
 * @return d0: 1 selected, 0 not selected or no such gadget
 */
static ULONG BLITZ_ROUTINE gtgetstatus_c(UWORD id BL_REG(d1),
                                         struct gtcontext *c BL_REG(a1))
{
    struct Gadget *gad = find((struct Gadget *)c, id);

    if (!gad)
        return 0;
    return (UWORD)((WORD)(gad->Flags & GFLG_SELECTED) >> 7);
}

/**
 * @brief GTArrowSize size: the arrow size of later GTScrollers with
 * arrows (flag bit 11, GTSC_Arrows; initially 16).
 *
 * @param size  d0: the size
 */
static void BLITZ_ROUTINE gtarrowsize_c(ULONG size BL_REG(d0))
{
    arrowsize = size;
}

/**
 * @brief GTGimmeCludge On/Off: on = later gadgets are placed without the
 * screen's border sizes and font height (cludge = 0).
 *
 * @param on  d0.w: <> 0 on, 0 off
 */
static void BLITZ_ROUTINE gtcludge_c(UWORD on BL_REG(d0))
{
    cludge = on ? 0 : -1;
}

/* findgadget (binary: stops at the end, then the caller returns too) */
static struct Gadget *findgadget(struct Gadget *list, UWORD id)
{
    struct Gadget *gad = list;

    for (;;) {
        gad = gad->NextGadget;
        if (!gad)
            return NULL;
        if (gad->GadgetID == id && (WORD)gad->GadgetType >= 0)
            return gad;
    }
}

/**
 * @brief GTEnable GTList#,Id: clear GFLG_DISABLED of a gadget.
 *
 * On the first gadget with the id whose GadgetType has bit 15 clear
 * (findgadget); only the flag, no refresh; nothing if there is none.
 *
 * @param id    d1: the gadget id
 * @param list  a1: the GTList (!libs {#mygadtoolslib,$1180}), its first
 *              long as the head of the gadget list
 */
static void BLITZ_ROUTINE doenable_c(UWORD id BL_REG(d1), struct Gadget *list BL_REG(a1))
{
    struct Gadget *gad = findgadget(list, id);

    if (gad)
        HI(gad->Flags) &= ~(GFLG_DISABLED >> 8);
}

/**
 * @brief GTDisable GTList#,Id: set GFLG_DISABLED of a gadget, as GTEnable.
 *
 * @param id    d1: the gadget id
 * @param list  a1: the GTList (!libs {#mygadtoolslib,$1180})
 */
static void BLITZ_ROUTINE dodisable_c(UWORD id BL_REG(d1), struct Gadget *list BL_REG(a1))
{
    struct Gadget *gad = findgadget(list, id);

    if (gad)
        HI(gad->Flags) |= GFLG_DISABLED >> 8;
}

/**
 * @brief GTToggle GTList#,Id,On|Off: set or clear GFLG_SELECTED of a
 * gadget, as GTEnable.
 *
 * @param id    d1: the gadget id
 * @param on    d2.w: <> 0 set, 0 clear
 * @param list  a1: the GTList (!libs {#mygadtoolslib,$1180})
 */
static void BLITZ_ROUTINE doselect_c(UWORD id BL_REG(d1), UWORD on BL_REG(d2),
                                     struct Gadget *list BL_REG(a1))
{
    struct Gadget *gad = findgadget(list, id);

    if (!gad)
        return;
    if (on)
        LO(gad->Flags) |= GFLG_SELECTED;
    else
        LO(gad->Flags) &= ~GFLG_SELECTED;
}

/**
 * @brief GTToggle GTList#,Id: invert GFLG_SELECTED of a gadget, as
 * GTEnable.
 *
 * @param id    d1: the gadget id
 * @param list  a1: the GTList (!libs {#mygadtoolslib,$1180})
 */
static void BLITZ_ROUTINE doselect2_c(UWORD id BL_REG(d1), struct Gadget *list BL_REG(a1))
{
    struct Gadget *gad = findgadget(list, id);

    if (gad)
        LO(gad->Flags) ^= GFLG_SELECTED;
}

/* entry labels */
#define E(entry, cfunc) __asm__("   .globl _" #entry "\n   .set _" #entry ",_" #cfunc "\n")
E(_gtbutton, gtbutton_c);
E(_gtcheckbox, gtcheckbox_c);
E(_gtcycle, gtcycle_c);
E(_gtcycle2, gtcycle2_c);
E(_gtinteger, gtinteger_c);
E(_gtlistview, gtlistview_c);
E(_gtlistview1, gtlistview1_c);
E(_gtlistview2, gtlistview2_c);
E(_gtmx, gtmx_c);
E(_gtmx2, gtmx2_c);
E(_gtnumber, gtnumber_c);
E(_gtpalette, gtpalette_c);
E(_gtpalette2, gtpalette2_c);
E(_gtscroller, gtscroller_c);
E(_gtscroller2, gtscroller2_c);
E(_gtslider, gtslider_c);
E(_gtslider2, gtslider2_c);
E(_gtstring, gtstring_c);
E(_gtstring2, gtstring2_c);
E(_gttext, gttext_c);
E(_attachgtlist, attachgtlist_c);
E(_detachgtlist, detachgtlist_c);
E(_gtfreegadget, gtfreegadget_c);
E(_gtbevel, gtbevel_c);
E(_gtchnglist1, gtchnglist1_c);
E(_gtchnglist2, gtchnglist2_c);
E(_gtsethighlight, gtsethighlight_c);
E(_gtsetattrs, gtsetattrs_c);
E(_gtsetstring, gtsetstring_c);
E(_gtsetint, gtsetint_c);
E(_gtgetattrs, gtgetattrs_c);
E(_gtshape, gtshape_c);
E(_gtshape2, gtshape2_c);
/* small ones keep what the original keeps */
BLITZ_THUNK(_gttags, gttags_c, "d0-d1/a0-a1");
BLITZ_THUNK(_gtptr, gtptr_c, "d1/a1");
BLITZ_THUNK(_gtgetstring, gtgetstring_c, "d1/a1");
BLITZ_THUNK(_gtgetint, gtgetint_c, "d1/a1");
BLITZ_THUNK(_gtgetstatus, gtgetstatus_c, "d1/a1");
BLITZ_THUNK(_gtarrowsize, gtarrowsize_c, "d0-d1/a0-a1");
BLITZ_THUNK(_gtcludge, gtcludge_c, "d0/a0-a1");
BLITZ_THUNK(doenable, doenable_c, "d1/a1");
BLITZ_THUNK(dodisable, dodisable_c, "d1/a1");
BLITZ_THUNK(doselect, doselect_c, "d1/a1");
BLITZ_THUNK(doselect2, doselect2_c, "d1/a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess4, "Uninitialized Shape");
BLITZ_ERRCHK_MESSAGE(mess5, "GTList not attached to a window");
BLITZ_ERRCHK_MESSAGE(mess6, "GTList already attached to window!");
BLITZ_ERRCHK_MESSAGE(mess7, "GTList must be detached from window first!");

/* sgchk2 - TST (a2):  sgchk - TST (a6) */
static void BLITZ_ERRCHK sgchk2_c(const struct bb_shape *shape2 BL_REG(a2),
                                  const struct bb_shape *shape BL_REG(a6))
{
    if (!shape2->width || !shape->width)
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK sgchk_c(const struct bb_shape *shape BL_REG(a6))
{
    if (!shape->width)
        BLITZ_RUNERR(mess4);
}

/* AttachGTList: not to the window it is attached to already */
static void BLITZ_ERRCHK attachchk_c(struct Window **win BL_REG(a0),
                                     struct gtcontext *c BL_REG(a1))
{
    if (*win == c->window)
        BLITZ_RUNERR(mess6);
}

static void BLITZ_ERRCHK detachchk_c(struct gtcontext *c BL_REG(a1))
{
    if (!c->window)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK freegadchk_c(struct gtcontext *c BL_REG(a1))
{
    if (c->window)
        BLITZ_RUNERR(mess7);
}

/* the thunks last (see gadgetslib.c) */
BLITZ_ERRCHK_THUNK(sgchk2, sgchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(sgchk, sgchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(attachchk, attachchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(detachchk, detachchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(freegadchk, freegadchk_c, "d0-d1/a0-a1");
