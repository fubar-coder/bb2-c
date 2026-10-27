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
 *   - GTPalette with a colour writes it to the low word of the TAG_MORE
 *     pointer of its tag list (offset +38 of the source layout, which had
 *     two more tags): the colour is ignored, a GTTags list after it is
 *     reached through a wrong pointer.
 *   - The optional values (cycle/mx active, listview top/selected,
 *     scroller top, slider level, string default) stay in the static tag
 *     lists: a later call without them uses the previous values (GTString
 *     a pointer into the old string work area).
 *   - GTBevelBox: flags <> 0 gives the tag GTBB_Recessed with value 0, so
 *     the box is never recessed.
 *   - GTButton toggle (bit 13) sets TOGGLESELECT even if CreateGadget
 *     failed (writes address 14).
 *   - GTShape: the label of the previous gadget stays in the NewGadget; an
 *     existing GadgetRender of the button is overwritten with the static
 *     Image (20 bytes), else replaced by it.
 *   - getoptions (cycle/mx labels): memlib globalloc (no MEMF_CLEAR, all
 *     used bytes are written), at most 63 labels + NULL (no check), never freed.
 *   - GTSetAttrs/GTChangeList/GTSetHighlight work on the LAST gadget of a
 *     run of gadgets with the same id, GTGetAttrs/GTGadPtr etc. on the
 *     FIRST; GTFreeGadget frees the whole run (first to last).
 *   - Without a used screen handlestd reads the border sizes from 0.
 * DIFF: GTTags/GTSetAttrs do not move a2 (the original pops the pairs);
 * the unused error routine freqchk of the binary is not ported.
 */
#include "blitzlib.h"
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

#define W(p, o)  (*(WORD *)((UBYTE *)(p) + (o)))
#define UW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define L(p, o)  (*(ULONG *)((UBYTE *)(p) + (o)))
#define B(p, o)  (*(UBYTE *)((UBYTE *)(p) + (o)))
#define P(p, o)  (*(UBYTE **)((UBYTE *)(p) + (o)))

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
static ULONG usertags[32];
static ULONG onetag[3];
static ULONG gettag[3] = { 0, 0, 0 };   /* {tag, &value, 0} */
static ULONG value;
static ULONG beveltags[5] = { TAG + 52, 0, TAG + 51, 0, 0 };
static ULONG gatchtags[3] = { TAG + 6, 0, 0 };
static ULONG hightags[6] = { TAG + 54, 0, TAG + 78, 0, 0, 0 };
static ULONG nulltags[4];

/* tag lists of the creators: ..., [TAG_MORE or 0, usertags] */
#define MORE (ULONG)usertags
static ULONG buttags[8] = { T_DISABLED, 0, T_IMMEDIATE, 0, T_UNDERSC, 95, 0, MORE };
static ULONG boxtags[10] = { T_DISABLED, 0, TAG + 4, 0, TAG + 68, 0, T_UNDERSC, 95, 0, MORE };
static ULONG cycletags[10] = { T_DISABLED, 0, TAG + 14, 0, TAG + 15, 0, T_UNDERSC, 95, 0, MORE };
static ULONG mxtags[12] = { T_DISABLED, 0, TAG + 9, 0, TAG + 69, 0, TAG + 10, 0,
                            T_UNDERSC, 95, 0, MORE };
static ULONG texttags[6] = { TAG + 11, 0, TAG + 57, (ULONG)-1, 0, MORE };
static ULONG inttags[10] = { T_DISABLED, 0, T_IMMEDIATE, 0, TAG + 47, 0, T_UNDERSC, 95, 0, MORE };
static ULONG listtags[14] = { T_DISABLED, 0, TAG + 6, 0, TAG + 5, 0, TAG + 54, (ULONG)-1,
                              TAG + 7, 0, T_UNDERSC, 95, 0, MORE };
static ULONG numbertags[6] = { TAG + 13, 0, TAG + 58, (ULONG)-1, 0, MORE };
static ULONG palettetags[10] = { T_DISABLED, 0, TAG + 16, 0, TAG + 17, 0, T_UNDERSC, 95, 0, MORE };
static ULONG scrollertags[20] = { T_DISABLED, 0, T_IMMEDIATE, 0, T_FREEDOM, 0, TAG + 23, 0,
                                  TAG + 22, 0, TAG + 59, 16, TAG + 21, 0, T_UNDERSC, 95,
                                  T_RELVERIFY, (ULONG)-1, 0, MORE };
static ULONG slidertags[18] = { T_DISABLED, 0, T_IMMEDIATE, 1, T_FREEDOM, 0, TAG + 38, 0,
                                TAG + 39, 0, TAG + 40, 0, T_UNDERSC, 95, T_RELVERIFY,
                                (ULONG)-1, 0, MORE };
static ULONG stringtags[12] = { T_DISABLED, 0, T_IMMEDIATE, 0, TAG + 46, 0, TAG + 45, 0,
                                T_UNDERSC, 95, 0, MORE };
static ULONG shapetags[8] = { T_DISABLED, 0, T_IMMEDIATE, 0, T_UNDERSC, 95, 0, MORE };

/* GTShape: the two Images and the gadget flags */
static UWORD gflags;
static struct Image image1 = { 0, 0, 0, 0, 0, NULL, 3, 0, NULL };
static struct Image image2 = { 0, 0, 0, 0, 0, NULL, 3, 0, NULL };

static struct TextAttr *ufont;
static struct Screen *uscreen;
static struct NewGadget wrkgadget;

static ULONG allocst(ULONG s, ULONG len)
{
    struct bl_regs r = { .d0 = s, .d1 = len };

    return BLITZ_ALIBJSR(TOK_allocst, r);
}

/* SNE d1:EXT.w d1:EXT.l d1 */
static ULONG bit(ULONG d0, int n)
{
    return (d0 & (1UL << n)) ? (ULONG)-1 : 0;
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

/* _freecontext (_free) - a3 = the GTList */
void BLITZ_ROUTINE _freecontext(void)
{
    struct gtcontext *c = (struct gtcontext *)blitz_string;

    if (!c->context)
        return;
    FreeVisualInfo(c->vinfo);
    FreeGadgets(c->context);
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
    W(c->context, 38) = -1;             /* (binary) */
    c->vinfo = GetVisualInfoA(uscreen, (struct TagItem *)nulltags);
}

/* _handlestd - the NewGadget; returns the a2 cursor behind the label */
static const ULONG *handlestd(UWORD id, WORD x, WORD y, WORD w, WORD h,
                              struct TextAttr **font, struct Screen **scr,
                              const ULONG *a2, int shape)
{
    struct gtcontext *c = (struct gtcontext *)blitz_string;
    struct Screen *s;

    wrkgadget.ng_GadgetID = id;
    ufont = *font;
    uscreen = s = *scr;
    if (cludge) {
        x += B(s, 36);                  /* WBorLeft */
        y += B(s, 35);                  /* WBorTop */
        y += W(s, 142);                 /* RastPort.TxHeight */
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
        len = BL_POPL(a2);
        str = BL_POPL(a2);
        wrkgadget.ng_GadgetText = (UBYTE *)allocst(str, len);
    }
    wrkgadget.ng_VisualInfo = c->vinfo;
    return a2;
}

/* ng_Flags = flags & $3F (low word); TAG_MORE if GTTags were given;
   CreateGadget; usertags cleared */
static struct Gadget *create(ULONG kind, ULONG flags, ULONG *tags, int bt)
{
    struct gtcontext *c = (struct gtcontext *)blitz_string;
    struct Gadget *gad;

    UW(&wrkgadget.ng_Flags, 2) = flags & 0x3f;
    tags[bt] = usertags[0] ? TAG_MORE : 0;
    gad = CreateGadgetA(kind, c->current, &wrkgadget, (struct TagItem *)tags);
    c->current = gad;
    usertags[0] = 0;
    return gad;
}

#define STDARGS UWORD id BL_REG(d1), WORD x BL_REG(d2), WORD y BL_REG(d3), \
    WORD w BL_REG(d4), WORD h BL_REG(d5), struct TextAttr **font BL_REG(a0), \
    struct Screen **scr BL_REG(a1), const ULONG *a2 BL_REG(a2)
#define HANDLESTD(a) handlestd(id, x, y, w, h, font, scr, a, 0)

static void BLITZ_ROUTINE gtbutton_c(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp);
    struct Gadget *gad;

    buttags[1] = bit(f, 6);
    buttags[3] = bit(f, 7);
    gad = create(BUTTON_KIND, f, buttags, 6);
    if (f & (1UL << 13)) {
        __asm__("" : "+a"(gad));        /* may be 0: BUG(orig) */
        B(gad, 14) |= 1;                /* TOGGLESELECT */
    }
}

static void BLITZ_ROUTINE gtcheckbox_c(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp);

    boxtags[1] = bit(f, 6);
    boxtags[3] = bit(f, 8);
    boxtags[5] = bit(f, 9);
    create(CHECKBOX_KIND, f, boxtags, 8);
}

/* getoptions - "a|b|c" (length, string from the a2 stack) -> NULL
   terminated array of labels at +0, the texts at +256 */
static ULONG getoptions(const ULONG **sp)
{
    ULONG len = BL_POPL(*sp);
    const UBYTE *s = (const UBYTE *)BL_POPL(*sp);
    struct bl_regs r = { .d0 = len + 258, .d1 = 0 };    /* MEMF_ANY */
    UWORD n = (UWORD)len;
    UBYTE *blk, *a4;
    UBYTE **a2;

    blk = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
    a2 = (UBYTE **)blk;
    a4 = blk + 256;
    *a2++ = a4;
    while (n-- != 0) {                  /* DBF d2 */
        UBYTE c = *s++;
        if (c == '|') {
            *a4++ = 0;
            *a2++ = a4;
        } else
            *a4++ = c;
    }
    *a4 = 0;
    *a2 = NULL;
    return (ULONG)blk;
}

static void cycle(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp);

    L(cycletags, 12) = getoptions(&sp);
    cycletags[1] = bit(f, 6);
    create(CYCLE_KIND, f, cycletags, 8);
}

static void BLITZ_ROUTINE gtcycle_c(STDARGS)
{
    cycle(id, x, y, w, h, font, scr, a2);
}

static void BLITZ_ROUTINE gtcycle2_c(STDARGS)
{
    UW(cycletags, 22) = UW(a2, -22);
    cycle(id, x, y, w, h, font, scr, a2);
}

static void mx(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp);

    L(mxtags, 12) = getoptions(&sp);
    mxtags[1] = bit(f, 6);
    mxtags[5] = bit(f, 9);
    create(MX_KIND, f, mxtags, 10);
}

static void BLITZ_ROUTINE gtmx_c(STDARGS)
{
    mx(id, x, y, w, h, font, scr, a2);
}

static void BLITZ_ROUTINE gtmx2_c(STDARGS)
{
    UW(mxtags, 30) = UW(a2, -22);
    mx(id, x, y, w, h, font, scr, a2);
}

static void BLITZ_ROUTINE gttext_c(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp), len, str;

    len = BL_POPL(sp);
    str = BL_POPL(sp);
    texttags[1] = allocst(str, len);
    create(TEXT_KIND, f, texttags, 4);
}

static void BLITZ_ROUTINE gtinteger_c(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp);

    inttags[5] = BL_POPL(sp);
    inttags[1] = bit(f, 6);
    inttags[3] = bit(f, 7);
    create(INTEGER_KIND, f, inttags, 8);
}

static void listview(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp);

    listtags[3] = BL_POPL(sp) - 28;     /* the List header of the array */
    listtags[1] = bit(f, 6);
    listtags[9] = bit(f, 12);
    create(LISTVIEW_KIND, f, listtags, 12);
}

static void BLITZ_ROUTINE gtlistview_c(STDARGS)
{
    listview(id, x, y, w, h, font, scr, a2);
}

static void BLITZ_ROUTINE gtlistview1_c(STDARGS)
{
    UW(listtags, 22) = UW(a2, -18);     /* top = selected */
    UW(listtags, 30) = UW(a2, -18);
    listview(id, x, y, w, h, font, scr, a2);
}

static void BLITZ_ROUTINE gtlistview2_c(STDARGS)
{
    UW(listtags, 30) = UW(a2, -18);     /* selected */
    UW(listtags, 22) = UW(a2, -22);     /* top */
    listview(id, x, y, w, h, font, scr, a2);
}

static void BLITZ_ROUTINE gtnumber_c(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp);

    numbertags[1] = BL_POPL(sp);
    create(NUMBER_KIND, f, numbertags, 4);
}

static void palette(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp);

    palettetags[3] = BL_POPL(sp);
    palettetags[1] = bit(f, 6);
    create(PALETTE_KIND, f, palettetags, 8);
}

static void BLITZ_ROUTINE gtpalette_c(STDARGS)
{
    palette(id, x, y, w, h, font, scr, a2);
}

/* BUG(orig): +38 is the low word of the TAG_MORE pointer */
static void BLITZ_ROUTINE gtpalette2_c(STDARGS)
{
    UW(palettetags, 38) = UW(a2, -18);
    palette(id, x, y, w, h, font, scr, a2);
}

static void scroller(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp);
    const UWORD *wp = (const UWORD *)sp;

    UW(scrollertags, 30) = BL_POPW(wp);         /* visible */
    UW(scrollertags, 38) = BL_POPW(wp);         /* total */
    scrollertags[1] = bit(f, 6);
    scrollertags[3] = bit(f, 7);
    scrollertags[5] = (f & (1UL << 10)) ? 2 : 1;
    scrollertags[11] = (f & (1UL << 11)) ? arrowsize : 0;
    create(SCROLLER_KIND, f, scrollertags, 18);
}

static void BLITZ_ROUTINE gtscroller_c(STDARGS)
{
    scroller(id, x, y, w, h, font, scr, a2);
}

static void BLITZ_ROUTINE gtscroller2_c(STDARGS)
{
    UW(scrollertags, 54) = UW(a2, -18);         /* top */
    scroller(id, x, y, w, h, font, scr, a2);
}

static void slider(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp);
    const UWORD *wp = (const UWORD *)sp;

    UW(slidertags, 30) = BL_POPW(wp);           /* min */
    UW(slidertags, 38) = BL_POPW(wp);           /* max */
    slidertags[1] = bit(f, 6);
    slidertags[3] = bit(f, 7);
    slidertags[5] = (f & (1UL << 10)) ? 2 : 1;
    create(SLIDER_KIND, f, slidertags, 16);
}

static void BLITZ_ROUTINE gtslider_c(STDARGS)
{
    slider(id, x, y, w, h, font, scr, a2);
}

static void BLITZ_ROUTINE gtslider2_c(STDARGS)
{
    UW(slidertags, 46) = UW(a2, -18);           /* level */
    slider(id, x, y, w, h, font, scr, a2);
}

static void string(STDARGS)
{
    const ULONG *sp = HANDLESTD(a2);
    ULONG f = BL_POPL(sp);
    const UWORD *wp = (const UWORD *)sp;

    UW(stringtags, 22) = BL_POPW(wp);           /* maxchars */
    stringtags[1] = bit(f, 6);
    stringtags[3] = bit(f, 7);
    create(STRING_KIND, f, stringtags, 10);
}

static void BLITZ_ROUTINE gtstring_c(STDARGS)
{
    string(id, x, y, w, h, font, scr, a2);
}

static void BLITZ_ROUTINE gtstring2_c(STDARGS)
{
    L(stringtags, 28) = L(a2, -22);             /* default$ */
    string(id, x, y, w, h, font, scr, a2);
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
    shapetags[1] = bit(flags, 6);
    shapetags[3] = bit(flags, 7);
    gad = create(BUTTON_KIND, flags, shapetags, 6);
    usertags[0] = 0;
    __asm__("" : "+a"(gad));
    if (gad->GadgetRender) {
        ULONG *d = (ULONG *)gad->GadgetRender, *s = (ULONG *)&image1;
        int i;
        for (i = 0; i < 5; i++)
            *d++ = *s++;
    } else
        gad->GadgetRender = &image1;
    gad->SelectRender = &image2;
    gad->Flags = gflags;
    gad->Activation = GACT_RELVERIFY;
    gad->GadgetType = gad->GadgetType + GTYP_BOOLGADGET - 1;
    gflags = 0;
}

static void shape1(const UWORD *shape)
{
    image1.Width = shape[0];
    image1.Height = shape[1];
    image1.Depth = shape[2];
    image1.ImageData = *(UWORD **)(shape + 7);
    gflags += GFLG_GADGIMAGE;
}

static void BLITZ_ROUTINE gtshape_c(SHAPEARGS, const UWORD *shape BL_REG(a6))
{
    shape1(shape);
    doshape(id, x, y, flags, font, scr);
}

static void BLITZ_ROUTINE gtshape2_c(SHAPEARGS, const UWORD *shape2 BL_REG(a2),
                                     const UWORD *shape BL_REG(a6))
{
    image2.Width = shape2[0];
    image2.Height = shape2[1];
    image2.Depth = shape2[2];
    image2.ImageData = *(UWORD **)(shape2 + 7);
    gflags += GFLG_GADGHIMAGE;
    shape1(shape);
    doshape(id, x, y, flags, font, scr);
}

/* --------------------------------------------------------- the others */

/* AddGList with position -1 as a long (MOVEQ #-1,d0; the NDK inline
   passes the UWORD $FFFF) */
static void addglist(struct Library *ib, struct Window *w, struct Gadget *g)
{
    register struct Library *a6 __asm("a6") = ib;
    register struct Window *a0 __asm("a0") = w;
    register struct Gadget *a1 __asm("a1") = g;
    register LONG d0 __asm("d0") = -1;
    register LONG d1 __asm("d1") = -1;
    register APTR a2 __asm("a2") = NULL;

    __asm volatile("jsr -438(a6)"
        : "+a"(a0), "+a"(a1), "+d"(d0), "+d"(d1), "+a"(a2)
        : "a"(a6) : "cc", "memory");
}

/* AttachGTList l,w: a1 = list, a0 = window object, a6 = intuition */
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

/* DetachGTList l */
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
static UBYTE *find(UBYTE *a0, UWORD id)
{
    do {
        a0 = P(a0, 0);
        if (!a0)
            return NULL;
    } while (UW(a0, 38) != id);
    return a0;
}

/* the last of a run of gadgets with the id */
static UBYTE *lastof(UBYTE *a0, UWORD id)
{
    UBYTE *n;

    while ((n = P(a0, 0)) != NULL && UW(n, 38) == id)
        a0 = n;
    return a0;
}

/* GTFreeGadget l,id - unlink the run of gadgets with the id and free
   them; returns their number (0: none) */
static ULONG BLITZ_ROUTINE gtfreegadget_c(UWORD id BL_REG(d1),
                                          struct gtcontext *c BL_REG(a1))
{
    UBYTE *a0 = (UBYTE *)c, *prev, *first, *last;
    UWORD n = 0;

    do {
        prev = a0;
        a0 = P(a0, 0);
        if (!a0)
            return 0;
    } while (UW(a0, 38) != id);
    first = a0;
    do {
        n++;
        last = a0;
        a0 = P(a0, 0);
    } while (a0 && UW(a0, 38) == id);
    P(prev, 0) = P(last, 0);
    P(last, 0) = NULL;
    FreeGadgets((struct Gadget *)first);
    return n;
}

/* GTBevelBox l,x,y,w,h,flags (a0 = list) */
static void BLITZ_ROUTINE gtbevel_c(LONG x BL_REG(d1), LONG y BL_REG(d2),
                                    LONG w BL_REG(d3), LONG h BL_REG(d4),
                                    ULONG flags BL_REG(d5),
                                    struct gtcontext *c BL_REG(a0))
{
    beveltags[1] = (ULONG)c->vinfo;
    if (flags)
        flags = TAG + 51;               /* GTBB_Recessed (value 0) */
    beveltags[2] = flags;
    DrawBevelBoxA(c->window->RPort, x, y, w, h, (struct TagItem *)beveltags);
}

/* GTGadPtr(l,id) */
static ULONG BLITZ_ROUTINE gtptr_c(UWORD id BL_REG(d1), struct gtcontext *c BL_REG(a1))
{
    return (ULONG)find((UBYTE *)c, id);
}

/* set tags on the last gadget of the run with the id */
static ULONG setattrs(struct gtcontext *c, UWORD id, ULONG *tags, int last)
{
    UBYTE *a0 = find((UBYTE *)c->context, id);

    if (!a0)
        return 0;
    if (last)
        a0 = lastof(a0, id);
    GT_SetGadgetAttrsA((struct Gadget *)a0, c->window, NULL, (struct TagItem *)tags);
    return 0;
}

/* GTChangeList l,id[,list()] - GTLV_Labels (-1: detach the labels) */
static void BLITZ_ROUTINE gtchnglist1_c(UWORD id BL_REG(d1),
                                        struct gtcontext *c BL_REG(a1),
                                        const ULONG *a2 BL_REG(a2))
{
    gatchtags[1] = a2[-1] - 28;
    if (!c->context)
        return;
    setattrs(c, id, gatchtags, 1);
}

static void BLITZ_ROUTINE gtchnglist2_c(UWORD id BL_REG(d1),
                                        struct gtcontext *c BL_REG(a1))
{
    gatchtags[1] = (ULONG)-1;
    setattrs(c, id, gatchtags, 1);
}

/* GTSetHighlight l,id,value - GTLV_Selected and GTLV_MakeVisible */
static void BLITZ_ROUTINE gtsethighlight_c(UWORD id BL_REG(d1), ULONG v BL_REG(d2),
                                           struct gtcontext *c BL_REG(a1))
{
    hightags[1] = v;
    hightags[3] = v;
    if (!c->context)
        return;
    setattrs(c, id, hightags, 1);
}

/* _gttags - d7 pairs from the a2 stack into usertags */
static const ULONG *gttags(UWORD d7, const ULONG *sp)
{
    ULONG *a0 = usertags;

    while (d7-- != 0) {
        a0[0] = BL_POPL(sp);
        a0[1] = BL_POPL(sp);
        a0 += 2;
    }
    a0[0] = 0;
    return sp;
}

static void BLITZ_ROUTINE gttags_c(UWORD d7 BL_REG(d7), const ULONG *a2 BL_REG(a2))
{
    gttags(d7, a2);
}

/* GTSetAttrs l,id,tag,value... (also a function: 0 if no gadget) */
static ULONG BLITZ_ROUTINE gtsetattrs_c(UWORD id BL_REG(d1), UWORD d7 BL_REG(d7),
                                        struct gtcontext *c BL_REG(a1),
                                        const ULONG *a2 BL_REG(a2))
{
    UBYTE *a0;

    if (!c->context)
        return 0;
    a0 = find((UBYTE *)c->context, id);
    if (!a0)
        return 0;
    a0 = lastof(a0, id);
    gttags(d7, a2);
    GT_SetGadgetAttrsA((struct Gadget *)a0, c->window, NULL,
                       (struct TagItem *)usertags);
    return 0;                           /* the original: what d0 holds */
}

/* GTSetString / GTSetInteger: STRING_KIND/INTEGER_KIND ($104) or
   TEXT_KIND/NUMBER_KIND */
static void setone(struct gtcontext *c, UWORD id, ULONG v, ULONG tstr, ULONG tother)
{
    UBYTE *a0 = find((UBYTE *)c, id);

    if (!a0)
        return;
    onetag[0] = UW(a0, 16) == 0x104 ? tstr : tother;
    onetag[1] = v;
    GT_SetGadgetAttrsA((struct Gadget *)a0, c->window, NULL, (struct TagItem *)onetag);
}

static void BLITZ_ROUTINE gtsetstring_c(UWORD id BL_REG(d1), ULONG s BL_REG(d2),
                                        struct gtcontext *c BL_REG(a1))
{
    setone(c, id, s, TAG + 45, TAG + 11);
}

static void BLITZ_ROUTINE gtsetint_c(UWORD id BL_REG(d1), ULONG v BL_REG(d2),
                                     struct gtcontext *c BL_REG(a1))
{
    setone(c, id, v, TAG + 47, TAG + 13);
}

/* GTGetString(l,id) - the StringInfo buffer to a3 */
static ULONG BLITZ_ROUTINE gtgetstring_c(UWORD id BL_REG(d1),
                                         struct gtcontext *c BL_REG(a1))
{
    UBYTE *a0 = find((UBYTE *)c, id);
    const UBYTE *s;
    ULONG n = (ULONG)-1;

    if (!a0)
        return 0;
    s = P(P(a0, 34), 0);
    do
        n++;
    while ((*blitz_string++ = *s++) != 0);
    blitz_string--;
    return n;
}

/* GTGetInteger(l,id) - StringInfo LongInt */
static ULONG BLITZ_ROUTINE gtgetint_c(UWORD id BL_REG(d1), struct gtcontext *c BL_REG(a1))
{
    UBYTE *a0 = find((UBYTE *)c, id);

    if (!a0)
        return 0;
    return L(P(a0, 34), 28);
}

/* GTGetAttrs(l,id,tag) - GT_GetGadgetAttrs into `value` */
static ULONG BLITZ_ROUTINE gtgetattrs_c(UWORD id BL_REG(d1), ULONG tag BL_REG(d2),
                                        struct gtcontext *c BL_REG(a1))
{
    UBYTE *a0 = find((UBYTE *)c, id);

    if (!a0)
        return 0;
    gettag[0] = tag;
    gettag[1] = (ULONG)&value;
    GT_GetGadgetAttrsA((struct Gadget *)a0, c->window, NULL, (struct TagItem *)gettag);
    return value;
}

/* GTStatus(l,id) - SELECTED as 0/1 */
static ULONG BLITZ_ROUTINE gtgetstatus_c(UWORD id BL_REG(d1),
                                         struct gtcontext *c BL_REG(a1))
{
    UBYTE *a0 = find((UBYTE *)c, id);

    if (!a0)
        return 0;
    return (UWORD)((WORD)(UW(a0, 12) & 0x80) >> 7);
}

/* GTArrowSize size */
static void BLITZ_ROUTINE gtarrowsize_c(ULONG s BL_REG(d0))
{
    arrowsize = s;
}

/* GTGimmeCludge on/off - on: no border offsets (cludge = 0) */
static void BLITZ_ROUTINE gtcludge_c(UWORD on BL_REG(d0))
{
    cludge = on ? 0 : -1;
}

/* findgadget (binary: stops at the end, then the caller returns too) */
static UBYTE *findgadget(UBYTE *list, UWORD id)
{
    UBYTE *a0 = list;

    for (;;) {
        a0 = P(a0, 0);
        if (!a0)
            return NULL;
        if (UW(a0, 38) == id && W(a0, 16) >= 0)
            return a0;
    }
}

static void BLITZ_ROUTINE doenable_c(UWORD id BL_REG(d1), UBYTE *list BL_REG(a1))
{
    UBYTE *a0 = findgadget(list, id);

    if (a0)
        B(a0, 12) &= ~1;
}

static void BLITZ_ROUTINE dodisable_c(UWORD id BL_REG(d1), UBYTE *list BL_REG(a1))
{
    UBYTE *a0 = findgadget(list, id);

    if (a0)
        B(a0, 12) |= 1;
}

static void BLITZ_ROUTINE doselect_c(UWORD id BL_REG(d1), UWORD on BL_REG(d2),
                                     UBYTE *list BL_REG(a1))
{
    UBYTE *a0 = findgadget(list, id);

    if (!a0)
        return;
    if (on)
        B(a0, 13) |= 0x80;
    else
        B(a0, 13) &= ~0x80;
}

static void BLITZ_ROUTINE doselect2_c(UWORD id BL_REG(d1), UBYTE *list BL_REG(a1))
{
    UBYTE *a0 = findgadget(list, id);

    if (a0)
        B(a0, 13) ^= 0x80;
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
static void BLITZ_ERRCHK sgchk2_c(const UWORD *s2 BL_REG(a2), const UWORD *s BL_REG(a6))
{
    if (!*s2 || !*s)
        BLITZ_RUNERR(mess4);
}

static void BLITZ_ERRCHK sgchk_c(const UWORD *s BL_REG(a6))
{
    if (!*s)
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
