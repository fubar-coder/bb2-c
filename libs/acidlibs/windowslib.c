/*
 * windowslib - command library 138: Intuition windows, their events and
 * text in/output - Window, WaitEvent, Event, GadgetHit, EventWindow,
 * DefaultIDCMP, MenuHit, ItemHit, SubHit, WindowFont, WPlot, WBox,
 * WCircle, WEllipse, WCls, WLocate, WindowInput, WindowOutput, Qualifier,
 * RawKey, Cursor, Editat, EditFrom, EditExit, WScroll, WMouseX, WMouseY,
 * WColour, WJam, WLine, Activate, WindowX, WindowY, MButtons, Menus,
 * WCursX, WCursY, WPointer, MenusOn, MenusOff, WMove, WSize, WindowWidth,
 * WindowHeight, InnerWidth, InnerHeight, InnerCls, WTopOff, WLeftOff,
 * SizeLimits, EMouseX, EMouseY, AddIDCMP, SubIDCMP, FlushEvents,
 * CatchDosErrs, RastPort, SetEventFilter, WTitle, CloseWindow,
 * WPrintScroll, WBlit, BitMaptoWindow, EventCode, EventQualifier,
 * PositionSuperBitMap, GetSuperBitMap, PutSuperBitMap, WindowTags.
 *
 * C translation of bblibsrc/windowslib ("RESURRECTED WINDOWS LIBRARY",
 * BUM6; binary checked: same as the source).  An object library
 * (!libfin{_toke,...,5,5}): the Blitz object "Window" (the name of the
 * first token), objects of 2^5 = 32 bytes:
 *   +0 struct Window *, +4.w/+6.w text cursor x/y, +8 pointer data,
 *   +12 its size, +16.w/+18.w super bitmap position.
 * _use = WindowInput + WindowOutput, _free = CloseWindow.  Many entries
 * are Amiga mode only (code address + 1 in !subs).
 *
 *   init (a0 = graphics, a1 = intuition): one message port for all
 *         windows (myport, signal of the program's task), the event
 *         list, defaults; the init value (#long) is the port's address.
 *   finit: FreeSignal, pr_WindowPtr back.
 *   Every window gets myport as UserPort before ModifyIDCMP(idcmp).
 *   Events: updateevents moves all messages from the port to an own
 *         list of 24 byte nodes {next, class, code.w, qualifier.w,
 *         iaddress, mousex.w, mousey.w, window} (memlib globalloc) and
 *         replies them at once; MENUVERIFY gets MENUCANCEL, key-up
 *         RAWKEYs are dropped; SetEventFilter's routines run before (d0 =
 *         message, 0 back = drop) and after (d0 = message or 0).
 *   Event/WaitEvent take the first node: gadget up/down (gadgetslib's
 *         button groups and cycle texts, UserData "bL"), menu numbers,
 *         a key (converted with rawkeylib, then "outstanding" for
 *         Inkey$), mouse buttons (outstanding for MButtons).
 *   WindowInput/WindowOutput hand inputoutputlib handlers (ALibJsr
 *         #inputoutputlib LSL 7+3/+4) with BRA entries in front:
 *         input -4 = line editor (inputstr), 0 = one key (windowinp);
 *         output -8 = 0-terminated string, -4 = string with length,
 *         0 = one character (windowprint).
 *
 * Kept from the original (BUG(orig)):
 *   - Menus/MenusOn/MenusOff step through the window objects with 16
 *     bytes (the objects were 16 bytes once: "was 4 (16)"), i.e. object
 *     0, the second half of object 0, object 1 ... and only `maximum`
 *     slots: half of the windows are not reached. (#310)
 *   - A gadget down of a string gadget gives an extra "gadget up" ($40)
 *     on the next Event (kludgegnum), without taking the next message
 *     (which then comes once more). (#311)
 *   - MENUPICK: one more MENUPICK node is appended for the item's
 *     NextSelect (if not $FFFF), with qualifier and iaddress not set
 *     (memlib memory, not cleared). (#312)
 *   - Not a bug (same result: IDCMP classes are single bits): class
 *     comparisons in Event are word compares.
 *   - Not a bug (those words are cleared or reset before use): CLR.l
 *     outmouse/MOVE.l #-1,eventgnum/... write the following word as well
 *     (the variables are kept in the original order).
 *   - Qualifier (a #long function) only sets the low word of d0. (#313)
 *   - WCls fills from y = 1 (#314); WScroll/WPrintScroll set the A pen to
 *     0 (#315).
 *   - WindowTags without tags keeps the user tags of the last
 *     WindowTags (no TAG_DONE written) (#316); &TagList is copied without
 *     a limit into 32 longs (#317).  Window titles (Window, WindowTags,
 *     WTitle) are allocated and never freed (#318).
 *   - Window with flag $80 (super bitmap) uses the bitmap of the last
 *     Window with BitMap#; the NewWindow is static, its BitMap stays. (#319)
 *   - WMove/WSize wait in a loop until the window has the new position/
 *     size (never ends if Intuition clips it; only the checks prevent
 *     that). (#320)
 *   - The line editor (Edit$ in a window) and calcwidth: proportional
 *     widths only for characters lo..lo+127 (SUB.b/BMI). (#321)
 *   - CloseWindow drops all events of all windows. (#322)
 * DIFF:
 *   - err6/err7 (invalid input/output window) are jumps from the normal
 *     code into the error check area in the original (cut away without
 *     the debugger); here they are raised from the normal code.
 *   - The original changes a3 (WBlit, inputstr), d7 (several), a4
 *     (Window), the C version does not; WindowTags with tag pairs does
 *     not write the title length back onto the argument stack, and
 *     WindowTags with &TagList does not park it behind the TAG_DONE pair
 *     in the caller's memory (the original overwrites the 4 bytes after
 *     the list, observable with an array: K).
 *   - Upper words of coordinates passed to graphics/intuition: the
 *     values are the same words; where the original leaves an unknown
 *     upper word (zcursor), the word is sign extended.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <graphics/gfx.h>
#include <stddef.h>
#include <dos/dosextens.h>
#define __NOLIBBASE__
#define INTUITION_BASE_NAME intbase
#define GRAPHICS_BASE_NAME gfxbase
#define LAYERS_BASE_NAME layersbase
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#undef __NOLIBBASE__

/* maxslib: string copies in global memory */
/* rawkeylib: d0 = code, d1 = qualifier -> d0 = character (0 = none) */

/* a window object, 2^5 bytes */
struct bwin {
    struct Window *win;         /* +0 */
    WORD cx, cy;                /* +4/+6 text cursor */
    UWORD *ptrdata;             /* +8 pointer data */
    ULONG ptrsize;              /* +12 */
    WORD sbx, sby;              /* +16/+18 super bitmap position */
} BL_PACKED;
static_assert(sizeof(struct bwin) <= 1 << 5);

/* the window number and the cursor origin in the Window's UserData (the
   long at +120 is used as these four bytes) */
struct wuserdata {
    UWORD num;                  /* +120 the window number */
    BYTE orgx, orgy;            /* +122/+123 the cursor origin */
} BL_PACKED;
static_assert(offsetof(struct Window, UserData) == 120);
static_assert(sizeof(struct wuserdata) == sizeof(APTR));
#define WUD(w) ((struct wuserdata *)&((struct Window *)(w))->UserData)

/* an event node (24 bytes) */
struct evnode {
    struct evnode *next;        /* 00 */
    ULONG class;                /* 04 */
    UWORD code;                 /* 08 */
    UWORD qual;                 /* 10 */
    APTR iaddr;                 /* 12 */
    WORD mx, my;                /* 16/18 */
    struct Window *win;         /* 20 */
};
static_assert(sizeof(struct evnode) == 24);

/* an IntuiFont object of intuifontlib (16 bytes): the TextAttr, +8 the
   opened font */
struct ifont {
    struct TextAttr ta;
    struct TextFont *font;
    ULONG pad;
};
static_assert(sizeof(struct ifont) == 16);


/* entry points (thunks / C / asm, defined below) */
void dogadgethit(void), doeventwindow(void);
void dowindowidcmp(void), domenuhit(void), doitemhit(void), dosubhit(void);
void dowlocate(void);
void doinwindow(void), dooutwindow(void), doqual(void), dorawkey(void);
void docursor(void), doeditat(void), doeditfrom(void), doeditfrom2(void);
void doeditexit(void), dowmousex(void), dowmousey(void);
void dosetpens(void), dosetpens2(void), dosetjam(void);
void dowindowx(void), dowindowy(void);
void dowcursx(void), dowcursy(void);
void dowindowwidth(void), dowindowheight(void), dogzzwidth(void), dogzzheight(void);
void dowtopoff(void), dowleftoff(void);
void doslimits(void), doslimits2(void), doemousex(void), doemousey(void);
void doaddidcmp(void), dosubidcmp(void), doflushevents(void);
void docatchdoserrs(void), dowrport(void), dosetevfilter(void);
void doeventcode(void), doeventqualifier(void);
void dousewindow(void);
void idcmpchk(void), wfontchk(void), wchk(void), wlinechk(void), wpointerchk(void);
void wmovechk(void), wsizechk(void), wblitchk(void), wbtowchk(void);
void BLITZ_ROUTINE doactivate(struct bwin *w BL_REG(a1),
                              struct Library *intbase BL_REG(a6));
ULONG BLITZ_ROUTINE doevent(void);
ULONG BLITZ_ROUTINE dowaitevent(void);
ULONG BLITZ_ROUTINE buttons(void);
void BLITZ_ROUTINE dowplot(WORD x BL_REG(d0), WORD y BL_REG(d1), UWORD c BL_REG(d2),
                           struct bwin *w BL_REG(a1),
                           struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dowbox(ULONG x0 BL_REG(d0), ULONG y0 BL_REG(d1),
                          ULONG x1 BL_REG(d2), ULONG y1 BL_REG(d3), UWORD c BL_REG(d4),
                          struct bwin *w BL_REG(a1),
                          struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dowcls(struct bwin *w BL_REG(a1),
                          struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dowcls2(UWORD c BL_REG(d0), struct bwin *w BL_REG(a1),
                           struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE doinnercls(struct bwin *w BL_REG(a1),
                              struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE doinnercls2(UWORD c BL_REG(d0), struct bwin *w BL_REG(a1),
                               struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dowellipse(WORD x BL_REG(d0), WORD y BL_REG(d1),
                              ULONG rx BL_REG(d2), ULONG ry BL_REG(d3),
                              UWORD c BL_REG(d4), struct bwin *w BL_REG(a1),
                              struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dowcircle(WORD x BL_REG(d0), WORD y BL_REG(d1), ULONG r BL_REG(d2),
                             UWORD c BL_REG(d3), struct bwin *w BL_REG(a1),
                             struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dowscroll(ULONG x0 BL_REG(d0), ULONG y0 BL_REG(d1),
                             ULONG x1 BL_REG(d2), ULONG y1 BL_REG(d3),
                             WORD dx BL_REG(d4), WORD dy BL_REG(d5),
                             struct bwin *w BL_REG(a1),
                             struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dowprintscroll(struct bwin *w BL_REG(a1),
                                  struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dowline(UWORD c BL_REG(d0), UWORD n BL_REG(d7),
                           UBYTE *args BL_REG(a2), struct bwin *w BL_REG(a1),
                           struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE doflushevents2(ULONG class BL_REG(d0));
void BLITZ_ROUTINE dowtitle(const UBYTE *wt BL_REG(d0), const UBYTE *st BL_REG(d1),
                            const ULONG *lens BL_REG(a2), struct bwin *w BL_REG(a1),
                            struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dowmove(ULONG x BL_REG(d0), ULONG y BL_REG(d1),
                           struct bwin *w BL_REG(a0),
                           struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dowsize(ULONG x BL_REG(d0), ULONG y BL_REG(d1),
                           struct bwin *w BL_REG(a0),
                           struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE domenuson(UBYTE *objs BL_REG(a2), UWORD max BL_REG(d2),
                             struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE domenusoff(UBYTE *objs BL_REG(a2), UWORD max BL_REG(d2),
                              struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE domenusok(UWORD on BL_REG(d0), UBYTE *objs BL_REG(a2),
                             UWORD max BL_REG(d2), struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dowpointer(const UBYTE *shape BL_REG(a2), ULONG xoff_reg BL_REG(d2),
                              ULONG yoff_reg BL_REG(d3), struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dowfont(const struct ifont *font BL_REG(a0),
                           struct bwin *w BL_REG(a1),
                           struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dowfont2(ULONG style BL_REG(d1), const struct ifont *font BL_REG(a0),
                            struct bwin *w BL_REG(a1),
                            struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dobtow1(struct bwin *w BL_REG(a0), struct BitMap *bm BL_REG(a1),
                           struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dobtow2(ULONG sx_reg BL_REG(d0), ULONG sy_reg BL_REG(d1),
                           ULONG sx BL_REG(d2), ULONG sy BL_REG(d3),
                           ULONG dx BL_REG(d4), ULONG dy BL_REG(d5),
                           const UWORD *args BL_REG(a2), struct bwin *w BL_REG(a0),
                           struct BitMap *bm BL_REG(a1),
                           struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE dowblit(ULONG xpos BL_REG(d1), ULONG ypos BL_REG(d2),
                           ULONG y_reg BL_REG(d3), struct bwin *w BL_REG(a0),
                           const UBYTE *shape BL_REG(a2),
                           struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE _showsuper(WORD x BL_REG(d0), WORD y BL_REG(d1),
                              struct bwin *w BL_REG(a1),
                              struct Library *layersbase BL_REG(a6));
void BLITZ_ROUTINE _windowtobitmap(struct bwin *w BL_REG(a0),
                                   struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE _bitmaptowindow(struct bwin *w BL_REG(a0),
                                   struct Library *gfxbase BL_REG(a6));
void BLITZ_ROUTINE freewindow(struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dowindow(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                            const UBYTE *title BL_REG(d2), UBYTE dpen BL_REG(d3),
                            UBYTE bpen BL_REG(d4), ULONG *glist BL_REG(a0),
                            struct Screen **scr BL_REG(a1), WORD usedoff BL_REG(d6),
                            const ULONG *lens BL_REG(a2),
                            struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dowindow2(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                             const UBYTE *title BL_REG(d2), UBYTE dpen BL_REG(d3),
                             UBYTE bpen BL_REG(d4), struct Screen **scr BL_REG(a1),
                             WORD usedoff BL_REG(d6), const ULONG *lens BL_REG(a2),
                             struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dowindow3(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                             const UBYTE *title BL_REG(d2), ULONG *glist BL_REG(a0),
                             struct Screen **scr BL_REG(a1), WORD usedoff BL_REG(d6),
                             const ULONG *lens BL_REG(a2),
                             struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dowindowtags1(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                                 const UBYTE *title BL_REG(d2),
                                 struct Screen **scr BL_REG(a1),
                                 WORD usedoff BL_REG(d6), const ULONG *lens BL_REG(a2),
                                 struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dowindowtags2(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                                 const UBYTE *title BL_REG(d2), UWORD n BL_REG(d7),
                                 struct Screen **scr BL_REG(a1),
                                 WORD usedoff BL_REG(d6), const ULONG *lens BL_REG(a2),
                                 struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dowindowtags3(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                                 const UBYTE *title BL_REG(d2),
                                 const ULONG *tl BL_REG(d3),
                                 struct Screen **scr BL_REG(a1),
                                 WORD usedoff BL_REG(d6), const ULONG *lens BL_REG(a2),
                                 struct Library *intbase BL_REG(a6));
ULONG BLITZ_ROUTINE codeinit(struct Library *gfx BL_REG(a0),
                             struct Library *intb BL_REG(a1));
void BLITZ_ROUTINE codefinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#windowslib,init,#long,finit,errors} - returns the port */
BLITZ_LIBHEADER(LIB_windowslib, init, BL_LONG, finit, BLITZ_ERROR_AREA);

#define W42 (BL_PUSH | BL_WORD)

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, W42, W42, W42, W42, BL_LONG, BL_STRING, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1380, LIB_screenslib, 0x1102, LIB_windowslib, 0x0604,
           LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dowindow2), 0, 0);
BLITZ_ARGS(BL_WORD, W42, W42, W42, W42, BL_LONG, BL_STRING, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1380, LIB_screenslib, 0x1102, LIB_gadgetslib, 0x1085,
           LIB_windowslib, 0x0604, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dowindow), 0, 0);
BLITZ_ARGS(BL_WORD, W42, W42, W42, W42, BL_LONG, BL_STRING, W42, W42, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1380, LIB_screenslib, 0x1102, LIB_gadgetslib, 0x1083,
           LIB_windowslib, 0x0604, LIB_intuitionlib, BL_LA(6), LIB_bitmaplib, 0x1484);
BLITZ_SUBS(BLITZ_AMIGAONLY(dowindow3), 0, 0);
BLITZ_NAME_AT(_toke, "Window",
    "Window#,X,Y,Width,Height,Flags,Title$,Dpen,Bpen[,GadgetList#[,BitMap#]]");

/* Dc 3 LSL 8+3,0,0: statement OR function (#long) */
BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(dowaitevent), 0, 0);
BLITZ_NAME("WaitEvent", "");

#define SIMPLE_FN(type, code, name)                     \
    BLITZ_AFUNCTION(type);                              \
    BLITZ_ARGS0();                                      \
    BLITZ_LIBS0();                                      \
    BLITZ_SUBS(code, 0, 0);                             \
    BLITZ_NAME(name, "")

SIMPLE_FN(BL_LONG, BLITZ_AMIGAONLY(doevent), "Event");
SIMPLE_FN(BL_WORD, dogadgethit, "GadgetHit");
SIMPLE_FN(BL_WORD, doeventwindow, "EventWindow");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(dowindowidcmp, idcmpchk, 0);
BLITZ_NAME("DefaultIDCMP", "IDCMP_Flags");

SIMPLE_FN(BL_WORD, domenuhit, "MenuHit");
SIMPLE_FN(BL_WORD, doitemhit, "ItemHit");
SIMPLE_FN(BL_WORD, dosubhit, "SubHit");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_intuifontlib, 0x1080, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dowfont), wfontchk, 0);
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_intuifontlib, 0x1080, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dowfont2), wfontchk, 0);
BLITZ_NAME("WindowFont", "IntuiFont# [,SoftStyle]");

#define GFX_STMT(code, chk, name, help, ...)            \
    BLITZ_ASTATEMENT();                                 \
    BLITZ_ARGS(__VA_ARGS__);                            \
    BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_graphicslib, BL_LA(6)); \
    BLITZ_SUBS(BLITZ_AMIGAONLY(code), chk, 0);                \
    BLITZ_NAME(name, help)

GFX_STMT(dowplot, wchk, "WPlot", "X,Y,Colour", BL_WORD, BL_WORD, BL_WORD);
GFX_STMT(dowbox, wchk, "WBox", "X1,Y1,X2,Y2,Colour", BL_WORD, BL_WORD, BL_WORD, BL_WORD,
         BL_WORD);
GFX_STMT(dowcircle, wchk, "WCircle", "X,Y,Radius,Colour", BL_WORD, BL_WORD, BL_WORD, BL_WORD);
GFX_STMT(dowellipse, wchk, "WEllipse", "X,Y,X Radius,Y Radius,Colour", BL_WORD, BL_WORD,
         BL_WORD, BL_WORD, BL_WORD);

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dowcls), wchk, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dowcls2), wchk, 0);
BLITZ_NAME("WCls", "[Colour]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1102);
BLITZ_SUBS(dowlocate, wchk, 0);
BLITZ_NAME("WLocate", "Cursor x,Cursor y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1380);
BLITZ_SUBS(doinwindow, 0, 0);
BLITZ_NAME("WindowInput", "Window#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1380);
BLITZ_SUBS(dooutwindow, 0, 0);
BLITZ_NAME("WindowOutput", "Window#");

SIMPLE_FN(BL_LONG, doqual, "Qualifier");
SIMPLE_FN(BL_WORD, dorawkey, "RawKey");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(docursor, 0, 0);
BLITZ_NAME("Cursor", "Thickness : <0 - block, >=0 - underline thickness");

SIMPLE_FN(BL_WORD, doeditat, "Editat");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doeditfrom, 0, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doeditfrom2, 0, 0);
BLITZ_NAME("EditFrom", "[Characterpos]");

SIMPLE_FN(BL_WORD, doeditexit, "EditExit");

GFX_STMT(dowscroll, wchk, "WScroll", "X1,Y1,X2,Y2,Delta X,Delta Y", BL_WORD, BL_WORD, BL_WORD,
         BL_WORD, BL_WORD, BL_WORD);

#define USED_FN(code, name)                             \
    BLITZ_AFUNCTION(BL_WORD);                           \
    BLITZ_ARGS0();                                      \
    BLITZ_LIBS(LIB_windowslib, 0x1102);                 \
    BLITZ_SUBS(code, wchk, 0);                          \
    BLITZ_NAME(name, "")

USED_FN(dowmousex, "WMouseX");
USED_FN(dowmousey, "WMouseY");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dosetpens, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dosetpens2, 0, 0);
BLITZ_NAME("WColour", "Foreground Colour[,Background Colour]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dosetjam, 0, 0);
BLITZ_NAME("WJam", "Jammode");

BLITZ_ASTATEMENT();
BLITZ_REPARGS(0, 2, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dowline), wlinechk, 0);
BLITZ_NAME("WLine", "X1,Y1,X2,Y3[,Xn,Yn...],Colour");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1180, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(doactivate), wchk, 0);
BLITZ_NAME("Activate", "Window#");

USED_FN(dowindowx, "WindowX");
USED_FN(dowindowy, "WindowY");

SIMPLE_FN(BL_WORD, BLITZ_AMIGAONLY(buttons), "MButtons");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1201, LIB_windowslib, 0x0203, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(domenusok), 0, 0);
BLITZ_NAME("Menus", "On|Off");

USED_FN(dowcursx, "WCursX");
USED_FN(dowcursy, "WCursY");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_shapeslib, 0x1280, LIB_windowslib, 0x1302, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dowpointer), wpointerchk, 0);
BLITZ_NAME("WPointer", "Shape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_windowslib, 0x1201, LIB_windowslib, 0x0203, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(domenuson), 0, 0);
BLITZ_NAME("MenusOn", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_windowslib, 0x1201, LIB_windowslib, 0x0203, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(domenusoff), 0, 0);
BLITZ_NAME("MenusOff", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1002, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(dowmove, wmovechk, 0);
BLITZ_NAME("WMove", "X,Y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1002, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(dowsize, wsizechk, 0);
BLITZ_NAME("WSize", "Width,Height");

USED_FN(dowindowwidth, "WindowWidth");
USED_FN(dowindowheight, "WindowHeight");
USED_FN(dogzzwidth, "InnerWidth");
USED_FN(dogzzheight, "InnerHeight");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(doinnercls), wchk, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(doinnercls2), wchk, 0);
BLITZ_NAME("InnerCls", "[Colour]");

USED_FN(dowtopoff, "WTopOff");
USED_FN(dowleftoff, "WLeftOff");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doslimits2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doslimits, 0, 0);
BLITZ_NAME("SizeLimits", "Min Width,Min Height,Max Width,Max Height");

SIMPLE_FN(BL_WORD, doemousex, "EMouseX");
SIMPLE_FN(BL_WORD, doemousey, "EMouseY");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(doaddidcmp, 0, 0);
BLITZ_NAME("AddIDCMP", "IDCMP_Flags");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(dosubidcmp, 0, 0);
BLITZ_NAME("SubIDCMP", "IDCMP_Flags");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doflushevents, 0, 0);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(doflushevents2, 0, 0);
BLITZ_NAME("FlushEvents", "[IDCMP_Flag]");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_windowslib, 0x1102);
BLITZ_SUBS(docatchdoserrs, wchk, 0);
BLITZ_NAME("CatchDosErrs", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1180);
BLITZ_SUBS(dowrport, wchk, 0);
BLITZ_NAME("RastPort", "(Window#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(dosetevfilter, 0, 0);
BLITZ_NAME("SetEventFilter", "preprocess,postprocess");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(dowtitle, wchk, 0);
BLITZ_NAME("WTitle", "windowtitle$,screentitle$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1380, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(freewindow, 0, 0);
BLITZ_NAME("CloseWindow", "Window#");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(dowprintscroll, wchk, 0);
BLITZ_NAME("WPrintScroll", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1002, LIB_shapeslib, 0x1280, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(dowblit, wblitchk, 0);
BLITZ_NAME("WBlit", "Shape#,x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1081, LIB_bitmaplib, 0x1180, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(dobtow1, wbtowchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1081, LIB_bitmaplib, 0x1180, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(dobtow2, wbtowchk, 0);
BLITZ_NAME("BitMaptoWindow", "Bitmap#,Window#[,srcx,srcy,destx,desty,wid,height]");

SIMPLE_FN(BL_WORD, BLITZ_AMIGAONLY(doeventcode), "EventCode");
SIMPLE_FN(BL_WORD, BLITZ_AMIGAONLY(doeventqualifier), "EventQualifier");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1102, LIB_layerslib, BL_LA(6));
BLITZ_SUBS(_showsuper, 0, 0);
BLITZ_NAME("PositionSuperBitMap", "x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_windowslib, 0x1002, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(_windowtobitmap, 0, 0);
BLITZ_NAME("GetSuperBitMap", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_windowslib, 0x1002, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(_bitmaptowindow, 0, 0);
BLITZ_NAME("PutSuperBitMap", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_STRING);
BLITZ_LIBS(LIB_windowslib, 0x1380, LIB_screenslib, 0x1102, LIB_windowslib, 0x0604,
           LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dowindowtags1), 0, 0);
BLITZ_ARGS(BL_WORD, BL_LONG, BL_STRING, BL_LONG);
BLITZ_LIBS(LIB_windowslib, 0x1380, LIB_screenslib, 0x1102, LIB_windowslib, 0x0604,
           LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dowindowtags3), 0, 0);
BLITZ_REPARGS(3, 2, BL_WORD, BL_LONG, BL_STRING, BL_LONG | BL_PUSH, BL_LONG | BL_PUSH);
BLITZ_LIBS(LIB_windowslib, 0x1380, LIB_screenslib, 0x1102, LIB_windowslib, 0x0604,
           LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dowindowtags2), 0, 0);
BLITZ_NAME("WindowTags", "Window#,Flags,Title$ [,&TagList] or [[,Tag,Data]...]");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0, LIB_graphicslib, BL_LA(0), LIB_intuitionlib, BL_LA(1));
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, dousewindow, 0, 0);
BLITZ_NULLSUB_AT(_free, freewindow, 0, 0, LIB_intuitionlib, BL_LA(6));

/* !libfin{_toke,_load,_save,_use,_free,5,5} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 5);

/* ***************** Code Below ******************* */

static ULONG prefilter, postfilter;     /* JMP 0: SetEventFilter */

/* the variables in their original order: some are written as longs
   together with the following word (CLR.l outmouse, MOVE.l #-1,eventgnum,
   MOVE.l 8(a1),evcode, MOVE.l 16(a1),evmousex ...) */
static struct BL_PACKED {
    ULONG zero;                 /* the empty gadget list of Window */
    ULONG idcmp;
    UWORD evcode, evqualifier;
    WORD eventgnum;             /* event gadget number */
    struct Window *eventwin;    /* window the event came from */
    WORD menunum, itemnum, subnum;
    UWORD charbuf;              /* temp: one character for printtext */
    struct Library *gfxbase;
    struct bwin *prwindow;
    UWORD outkey;               /* outstanding keystroke */
    struct Window *outwindow;   /* its window */
    UWORD outq;
    UWORD outr;
    UWORD kqualifier;           /* qualifier of the last keystroke */
    UWORD lrawkey;              /* most recently got raw key */
    UWORD msgon;
    ULONG oldexcode, oldexbits;
    struct evnode *firstmess;   /* the event list */
    struct evnode **nextmess;   /* where to append */
    struct bwin *inwindow;      /* window to get inkeys from */
    WORD underline;             /* cursor: <0 block */
    UWORD efrom;                /* where to begin editing */
    UWORD eat;                  /* where the last edit finished */
    WORD emode;                 /* <>0: edit ends on any event */
    UWORD eexit;                /* ascii that ended the edit */
    UWORD fpen, bpen, tjam;
    UWORD outmouse;             /* outstanding mouse button event */
    struct Window *outmwin;     /* its window */
    ULONG linw;
    UWORD lflgs;
    WORD evmousex, evmousey;
} V = { .idcmp = 0x100 + 0x200 + 0x40 + 0x20 + 0x400 + 8 + 0x40000 + 0x80000 + 2 + 4,
        .nextmess = &V.firstmess, .underline = -1 };

#define LONGAT(field) (*(ULONG *)&V.field)

static struct MsgPort myport;
/* SizeLimits: the NewWindow limits of the next Window */
static struct { WORD minw, minh, maxw, maxh; } sizelimits = { 128, 64, 0, 0 };
static_assert(sizeof sizelimits == 8);
static WORD sizelimitsok = -1;
static APTR oldwptr;
static struct Library *intbase_g;
static UWORD kludgegnum;        /* <>0: outstanding gadget up event */
static struct bwin *bmapbase;   /* dowindow3's bitmap */
static UWORD maxchars;          /* the line editor */
static ULONG origxy;

/* the NewWindow, static like the original's anewwindow */
static struct NewWindow anewwindow = {
    .MinWidth = 0x20, .MinHeight = 0x40, .MaxWidth = 0xffff, .MaxHeight = 0xffff };

/* wintags + usertags: one list (no TAG_MORE); WindowTags copies the user
   tags behind the four fixed ones, BUG(orig) (#317) without a limit, and
   without tags writes no TAG_DONE: the last user tags stay (#316) */
static struct {
    struct TagItem flags, title, screen, newlook;   /* wintags */
    struct TagItem user[16];                        /* usertags: 32 longs */
} tags = {
    { WA_Flags, 0 }, { WA_Title, 0 }, { WA_CustomScreen, 0 }, { WA_NewLookMenus, (ULONG)-1 } };
static_assert(sizeof tags == 40 * 4);

/* WBlit's bitmap */
static struct BitMap mybmap;

/* runtime errors reached from the normal code */
static const char mess1[] = "Couldn't Open Window";
static const char mess6[] = "Invalid Input Window";
static const char mess7[] = "Invalid Output Window";

#define THISPROC ((struct Process *)SysBase->ThisTask)

/* word functions: MOVE.w x,d0 - the upper word of d0 stays */
#define WORD_FN(entry, cfunc) \
    BLITZ_THUNK_POST(entry, cfunc, "d0-d1/a0-a1", "move.w d0,2(sp)")

/* memlib / maxslib */
static APTR globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return (APTR)BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(APTR mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = (ULONG)mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

static UBYTE *allocst(const UBYTE *s, ULONG len)
{
    struct bl_regs r = { .d0 = (ULONG)s, .d1 = len };

    return (UBYTE *)BLITZ_ALIBJSR(TOK_allocst, r);
}

/* rawkeylib: the character of a key (0 = none) */
static UWORD rawkey(UWORD code, UWORD qual)
{
    struct bl_regs r = { .d0 = code, .d1 = qual };

    return (UWORD)BLITZ_ALIBJSR(TOK_convertkey, r);
}

/* ------------------------------------------------------- small ones */

/* SetEventFilter pre,post */
static void BLITZ_ROUTINE dosetevfilter_c(ULONG pre BL_REG(d0), ULONG post BL_REG(d1))
{
    prefilter = pre;
    postfilter = post;
}
BLITZ_THUNK(dosetevfilter, dosetevfilter_c, "d0-d1/a0-a1");

/* RastPort(Window#) - inline: MOVE.l (a1),a1:MOVE.l 50(a1),d0 */
BLITZ_INLINE(dowrport, "move.l (a1),a1\n   move.l 50(a1),d0");

/* CatchDosErrs - DOS requesters on the used window */
static void BLITZ_ROUTINE docatchdoserrs_c(struct bwin *w BL_REG(a1))
{
    THISPROC->pr_WindowPtr = w->win;
}
BLITZ_THUNK(docatchdoserrs, docatchdoserrs_c, "d0-d1/a1");

static void BLITZ_ROUTINE doaddidcmp_c(ULONG f BL_REG(d0))
{
    V.idcmp |= f;
}
BLITZ_THUNK(doaddidcmp, doaddidcmp_c, "d0-d1/a0-a1");

/* SubIDCMP: NOT.l d0 - d0 comes back inverted */
static ULONG BLITZ_ROUTINE dosubidcmp_c(ULONG f BL_REG(d0))
{
    V.idcmp &= ~f;
    return ~f;
}
BLITZ_THUNK(dosubidcmp, dosubidcmp_c, "d1/a0-a1");

static void BLITZ_ROUTINE dowindowidcmp_c(ULONG f BL_REG(d0))
{
    V.idcmp = f;
}
BLITZ_THUNK(dowindowidcmp, dowindowidcmp_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE doslimits_c(WORD a BL_REG(d0), WORD b BL_REG(d1), WORD c BL_REG(d2),
                                      WORD d BL_REG(d3))
{
    sizelimits.minw = a;
    sizelimits.minh = b;
    sizelimits.maxw = c;
    sizelimits.maxh = d;
}
BLITZ_THUNK(doslimits, doslimits_c, "d0-d1/a1");

static void BLITZ_ROUTINE doslimits2_c(WORD on BL_REG(d0))
{
    sizelimitsok = on;
}
BLITZ_THUNK(doslimits2, doslimits2_c, "d0-d1/a0-a1");

/* WTopOff/WLeftOff: MOVEQ #0,d0:MOVE.b border,d0 */
static ULONG BLITZ_ROUTINE dowtopoff_c(struct bwin *w BL_REG(a1))
{
    return (UBYTE)w->win->BorderTop;
}
BLITZ_THUNK(dowtopoff, dowtopoff_c, "d1/a1");

static ULONG BLITZ_ROUTINE dowleftoff_c(struct bwin *w BL_REG(a1))
{
    return (UBYTE)w->win->BorderLeft;
}
BLITZ_THUNK(dowleftoff, dowleftoff_c, "d1/a1");

static UWORD BLITZ_ROUTINE dowindowwidth_c(struct bwin *w BL_REG(a1))
{
    return w->win->Width;
}
WORD_FN(dowindowwidth, dowindowwidth_c);

static UWORD BLITZ_ROUTINE dowindowheight_c(struct bwin *w BL_REG(a1))
{
    return w->win->Height;
}
WORD_FN(dowindowheight, dowindowheight_c);

static UWORD BLITZ_ROUTINE dogzzwidth_c(struct bwin *w BL_REG(a1))
{
    return w->win->GZZWidth;
}
WORD_FN(dogzzwidth, dogzzwidth_c);

static UWORD BLITZ_ROUTINE dogzzheight_c(struct bwin *w BL_REG(a1))
{
    return w->win->GZZHeight;
}
WORD_FN(dogzzheight, dogzzheight_c);

/* Activate Window# - a1 = the window object */
void BLITZ_ROUTINE doactivate(struct bwin *w BL_REG(a1), struct Library *intbase BL_REG(a6))
{
    ActivateWindow(w->win);
}

/* WColour / WJam */
static void BLITZ_ROUTINE dosetpens2_c(UWORD f BL_REG(d0), UWORD b BL_REG(d1))
{
    V.bpen = b;
    V.fpen = f;
}
BLITZ_THUNK(dosetpens2, dosetpens2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dosetpens_c(UWORD f BL_REG(d0))
{
    V.fpen = f;
}
BLITZ_THUNK(dosetpens, dosetpens_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dosetjam_c(UWORD j BL_REG(d0))
{
    V.tjam = j;
}
BLITZ_THUNK(dosetjam, dosetjam_c, "d0-d1/a0-a1");

/* WindowX/WindowY: the window position, + border for GIMMEZEROZERO;
   WMouseX/WMouseY: the mouse in the window, - border for GZZ */
static UWORD BLITZ_ROUTINE dowindowx_c(struct bwin *w BL_REG(a1))
{
    struct Window *win = w->win;
    UWORD x = win->LeftEdge;

    if (win->Flags & WFLG_GIMMEZEROZERO)
        x += (UBYTE)win->BorderLeft;
    return x;
}
WORD_FN(dowindowx, dowindowx_c);

static UWORD BLITZ_ROUTINE dowmousex_c(struct bwin *w BL_REG(a1))
{
    struct Window *win = w->win;
    UWORD x = win->MouseX;

    if (win->Flags & WFLG_GIMMEZEROZERO)
        x -= (UBYTE)win->BorderLeft;
    return x;
}
WORD_FN(dowmousex, dowmousex_c);

static UWORD BLITZ_ROUTINE dowindowy_c(struct bwin *w BL_REG(a1))
{
    struct Window *win = w->win;
    UWORD y = win->TopEdge;

    if (win->Flags & WFLG_GIMMEZEROZERO)
        y += (UBYTE)win->BorderTop;
    return y;
}
WORD_FN(dowindowy, dowindowy_c);

static UWORD BLITZ_ROUTINE dowmousey_c(struct bwin *w BL_REG(a1))
{
    struct Window *win = w->win;
    UWORD y = win->MouseY;

    if (win->Flags & WFLG_GIMMEZEROZERO)
        y -= (UBYTE)win->BorderTop;
    return y;
}
WORD_FN(dowmousey, dowmousey_c);

/* the editor's variables */
static UWORD BLITZ_ROUTINE doeditat_c(void)   { return V.eat; }
static UWORD BLITZ_ROUTINE doeditexit_c(void) { return V.eexit; }
static UWORD BLITZ_ROUTINE dorawkey_c(void)   { return V.lrawkey; }
static UWORD BLITZ_ROUTINE doqual_c(void)     { return V.kqualifier & 255; }
WORD_FN(doeditat, doeditat_c);
WORD_FN(doeditexit, doeditexit_c);
WORD_FN(dorawkey, dorawkey_c);
WORD_FN(doqual, doqual_c);              /* BUG(orig) (#313): a #long function */

static void BLITZ_ROUTINE doeditfrom_c(void)
{
    V.emode = 0;
    V.efrom = 0;
}
BLITZ_THUNK(doeditfrom, doeditfrom_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE doeditfrom2_c(UWORD at BL_REG(d0))
{
    V.emode = -1;
    V.efrom = at;
}
BLITZ_THUNK(doeditfrom2, doeditfrom2_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE docursor_c(WORD u BL_REG(d0))
{
    V.underline = u;
}
BLITZ_THUNK(docursor, docursor_c, "d0-d1/a0-a1");

/* the event data */
static UWORD BLITZ_ROUTINE doemousex_c(void)        { return V.evmousex; }
static UWORD BLITZ_ROUTINE doemousey_c(void)        { return V.evmousey; }
static UWORD BLITZ_ROUTINE dogadgethit_c(void)      { return V.eventgnum; }
static UWORD BLITZ_ROUTINE domenuhit_c(void)        { return V.menunum; }
static UWORD BLITZ_ROUTINE doitemhit_c(void)        { return V.itemnum; }
static UWORD BLITZ_ROUTINE dosubhit_c(void)         { return V.subnum; }
static UWORD BLITZ_ROUTINE doeventcode_c(void)      { return V.evcode; }
static UWORD BLITZ_ROUTINE doeventqualifier_c(void) { return V.evqualifier; }
WORD_FN(doemousex, doemousex_c);
WORD_FN(doemousey, doemousey_c);
WORD_FN(dogadgethit, dogadgethit_c);
WORD_FN(domenuhit, domenuhit_c);
WORD_FN(doitemhit, doitemhit_c);
WORD_FN(dosubhit, dosubhit_c);
WORD_FN(doeventcode, doeventcode_c);
WORD_FN(doeventqualifier, doeventqualifier_c);

/* EventWindow - the window number of the last event (MOVE.w into d0
   after MOVE.l eventwin,d0: upper word = upper word of the pointer),
   -1 if none */
static ULONG BLITZ_ROUTINE doeventwindow_c(void)
{
    ULONG w = (ULONG)V.eventwin;

    if (!w)
        return (ULONG)-1;
    return (w & 0xffff0000UL) | WUD(w)->num;
}
BLITZ_THUNK(doeventwindow, doeventwindow_c, "d1/a0-a1");

/* --------------------------------------------------- windows' text */

/* WCursX/WCursY - cursor minus the origin of the window */
static UWORD BLITZ_ROUTINE dowcursx_c(struct bwin *w BL_REG(a1))
{
    return w->cx - WUD(w->win)->orgx;
}
WORD_FN(dowcursx, dowcursx_c);

static UWORD BLITZ_ROUTINE dowcursy_c(struct bwin *w BL_REG(a1))
{
    return w->cy - WUD(w->win)->orgy;
}
WORD_FN(dowcursy, dowcursy_c);

/* WLocate x,y */
static void BLITZ_ROUTINE dowlocate_c(WORD x BL_REG(d0), WORD y BL_REG(d1),
                                      struct bwin *w BL_REG(a1))
{
    w->cx = x + WUD(w->win)->orgx;
    w->cy = y + WUD(w->win)->orgy;
}
BLITZ_THUNK(dowlocate, dowlocate_c, "a1");

/* setpcursor - move the rastport to the print position, set the pens;
   returns the rastport */
static struct RastPort *setpcursor(struct bwin *pw)
{
    struct Library *gfxbase = V.gfxbase;
    struct RastPort *rp;

    rp = pw->win->RPort;
    Move(rp, (UWORD)pw->cx, (UWORD)(pw->cy + rp->TxBaseline));
    SetAPen(rp, V.fpen);
    SetBPen(rp, V.bpen);
    SetDrMd(rp, V.tjam);
    return rp;
}

/* printtext - Text at the cursor, the cursor moves by its width */
static void printtext(struct bwin *pw, struct RastPort *rp, const UBYTE *s, UWORD n)
{
    struct Library *gfxbase = V.gfxbase;

    pw->cx += TextLength(rp, (STRPTR)s, n);
    Text(rp, (STRPTR)s, n);
}

/* the output window, err7 if there is none */
static struct bwin *outwin(void)
{
    struct bwin *pw = V.prwindow;

    if (!pw || !pw->win) {
        BLITZ_RUNERR(mess7);
        return NULL;
    }
    return pw;
}

/* windowprint - one character in d0; CR/LF: next line (y + TxHeight,
   x = the origin) */
static ULONG windowprint(ULONG c)
{
    struct bwin *pw;

    if ((UBYTE)c == 13 || (UBYTE)c == 10) {
        struct Window *win;

        pw = V.prwindow;
        if (!pw->win) {
            BLITZ_RUNERR(mess7);
            return c;
        }
        win = pw->win;
        pw->cy += win->RPort->TxHeight;
        pw->cx = (UBYTE)WUD(win)->orgx;     /* CLR.w:MOVE.b to the low byte */
        return (c & 0xffff0000UL) | (UWORD)pw->cy;
    }
    pw = outwin();
    if (pw) {
        struct RastPort *rp = setpcursor(pw);

        *(UBYTE *)&V.charbuf = (UBYTE)c;    /* MOVE.b d0,(a0): high byte */
        printtext(pw, rp, (const UBYTE *)&V.charbuf, 1);
    }
    return 0;
}

/* windowprint2 - a0 = string, d0 = length */
static void windowprint2(const UBYTE *s, ULONG n)
{
    struct bwin *pw;

    if (!n)
        return;
    pw = outwin();
    if (pw)
        printtext(pw, setpcursor(pw), s, (UWORD)n);
}

/* windowprint3 - a0 = 0-terminated string */
static void windowprint3(const UBYTE *s)
{
    ULONG n = 0;

    while (s[n])
        n++;
    windowprint2(s, n);
}

static ULONG BLITZ_ROUTINE windowprint_c(ULONG c BL_REG(d0))
{
    return windowprint(c);
}

static void BLITZ_ROUTINE windowprint2_c(const UBYTE *s BL_REG(a0), ULONG n BL_REG(d0))
{
    windowprint2(s, n);
}

static void BLITZ_ROUTINE windowprint3_c(const UBYTE *s BL_REG(a0))
{
    windowprint3(s);
}

/* ----------------------------------------------------------- events */

/* call a SetEventFilter routine: d0 = message, a2 = message, a6 =
   exec; d0 back */
static ULONG callfilter(ULONG code, ULONG msg)
{
    register ULONG _d0 __asm("d0") = msg;
    register ULONG _a0 __asm("a0") = code;
    register ULONG _a2 __asm("a2") = msg;
    register ULONG _a6 __asm("a6") = (ULONG)SysBase;
    __asm volatile("jsr (%1)"
        : "+d"(_d0), "+a"(_a0), "+a"(_a2), "+a"(_a6)
        :
        : "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a1", "cc", "memory");
    return _d0;
}

/* readevent - one message from the port into the list; 0 = none.  The
   message replied is always the one GetMsg returned (mymess); checks and
   copy use the prefilter's result. */
static int readevent(void)
{
    struct IntuiMessage *mymess, *m;

    for (;;) {
        ULONG msg;

        mymess = (struct IntuiMessage *)GetMsg(&myport);
        if (!mymess)
            return 0;
        msg = (ULONG)mymess;
        if (prefilter) {
            msg = callfilter(prefilter, msg);
            if (!msg)
                goto dunmess;
        }
        m = (struct IntuiMessage *)msg;         /* yuck: MOVE.l d0,a2 */
        if ((UWORD)m->Class == 0x2000) {        /* MENUVERIFY */
            m->Code = 2;                        /* MENUCANCEL */
            goto dunmess;
        }
        if ((UWORD)m->Class == 0x400 && (m->Code & 0x80))   /* key up */
            goto dunmess;
        {
            /* domess */
            struct evnode *e = globalloc(24, MEMF_PUBLIC);

            *V.nextmess = e;
            V.nextmess = &e->next;
            e->next = NULL;
            e->class = m->Class;
            e->code = m->Code;
            e->qual = m->Qualifier;
            e->iaddr = m->IAddress;
            e->mx = m->MouseX;
            e->my = m->MouseY;
            e->win = m->IDCMPWindow;
            if (postfilter)
                callfilter(postfilter, (ULONG)m);
            ReplyMsg((struct Message *)mymess);
            return 1;
        }
dunmess:
        if (postfilter)
            callfilter(postfilter, msg);
        ReplyMsg((struct Message *)mymess);
    }
}

/* updateevents - all outstanding messages into the list */
static void updateevents(void)
{
    while (readevent())
        ;
}

/* unlinkmess - free the node after `prev` */
static void unlinkmess(struct evnode **prev, struct evnode *e)
{
    *prev = e->next;
    if (V.nextmess == &e->next)
        V.nextmess = prev;
    globfree(e, 24);
}

/* unlinkall - free ALL nodes */
static void unlinkall(void)
{
    while (V.firstmess)
        unlinkmess(&V.firstmess, V.firstmess);
}

/* FlushEvents */
static void BLITZ_ROUTINE doflushevents_c(void)
{
    updateevents();
    unlinkall();
    V.outkey = 0;
    V.outmouse = 0;
}
BLITZ_THUNK(doflushevents, doflushevents_c, "a0");

/* FlushEvents class - all events of that class (long compare) */
void BLITZ_ROUTINE doflushevents2(ULONG class BL_REG(d0))
{
    struct evnode **prev = &V.firstmess;

    updateevents();
    while (*prev) {
        struct evnode *e = *prev;

        if (e->class == class)
            unlinkmess(prev, e);
        else
            prev = &e->next;
    }
    if (class == 0x400)
        V.outkey = 0;
    if (class == 8)
        V.outmouse = 0;
}

/* redrawgad - RefreshGList(gadget, the event's window, 0, 1) */
static void redrawgad(struct Gadget *g, struct evnode *e)
{
    struct Library *intbase = intbase_g;

    RefreshGList(g, e->win, NULL, 1);
}

/* refgada3 - redraw a button; for GADGHCOMP/GADGHBOX without its
   imagery and SELECTED */
static void refgada3(struct Gadget *g, struct evnode *e)
{
    APTR render, select, text;
    UWORD f;

    if ((g->Flags & 3) > 1) {
        redrawgad(g, e);
        return;
    }
    render = g->GadgetRender;
    g->GadgetRender = NULL;
    select = g->SelectRender;
    g->SelectRender = NULL;
    text = g->GadgetText;
    g->GadgetText = NULL;
    f = g->Flags;
    g->Flags |= GFLG_SELECTED;
    redrawgad(g, e);
    g->Flags = f;
    g->GadgetText = text;
    g->SelectRender = select;
    g->GadgetRender = render;
}

/* gadgetslib's UserData of a gadget: "bL" and the button group */
struct gaduserdata {
    UWORD magic;                /* "bL": a Blitz gadget */
    UWORD group;                /* the button group, 0 = none */
};
static_assert(sizeof(struct gaduserdata) == sizeof(APTR));
#define GUD(g) ((struct gaduserdata *)&(g)->UserData)

/* gadgetslib's IntuiText (24 bytes): +20 = the start of its texts */
struct gadtext {
    struct IntuiText it;
    UBYTE *texts;
};
static_assert(sizeof(struct gadtext) == 24);

/* notbuttongad - a "bL" gadget without group: cycle its text (gadgetslib:
   the texts behind each other, their start at IntuiText+20, total length
   in front of it) */
static void notbuttongad(struct Gadget *g, struct evnode *e)
{
    struct gadtext *it = (struct gadtext *)g->GadgetText;
    UBYTE *cur, *base;
    LONG n;

    if (!it || (g->Activation & 0x100))
        return;
    cur = it->it.IText;
    base = it->texts;
    n = ((LONG *)base)[-1] - (cur - base);
    for (;;) {
        if (--n < 0) {
            it->it.IText = base;                /* wrap */
            break;
        }
        if (!*cur++) {
            it->it.IText = cur;
            break;
        }
    }
    redrawgad(g, e);
}

/* gadgetevent - a1 = the node of a gadget up */
static void gadgetevent(struct evnode *e)
{
    struct Gadget *g = e->iaddr, *o;
    UWORD grp;

    V.eventgnum = g->GadgetID;
    if (GUD(g)->magic != ('b' << 8 | 'L'))
        return;
    grp = GUD(g)->group;
    if (!grp) {
        notbuttongad(g, e);
        return;
    }
    /* button style gadget: switch off the others of the group */
    for (o = e->win->FirstGadget; o; o = o->NextGadget) {
        if (GUD(o)->magic != ('b' << 8 | 'L') || GUD(o)->group != grp)
            continue;
        if (o == g || !(o->Flags & GFLG_SELECTED))
            continue;
        o->Flags &= ~GFLG_SELECTED;
        refgada3(o, e);
    }
    if (!(g->Flags & GFLG_SELECTED)) {
        g->Flags |= GFLG_SELECTED;
        refgada3(g, e);
    }
}

/* findmenuitem - the item (or subitem) of a menu code */
static struct MenuItem *findmenuitem(struct Menu *m, UWORD code)
{
    struct MenuItem *it;
    WORD n;

    n = code & 31;
    if (n) {
        n--;
        do
            m = m->NextMenu;
        while (n-- != 0);
    }
    it = m->FirstItem;                          /* 18(a0) */
    n = (code >> 5) & 63;
    if (n) {
        n--;
        do
            it = it->NextItem;
        while (n-- != 0);
    }
    n = (UWORD)(code << 5 | code >> 11) & 31;   /* ROL #5 */
    if (n != 31) {
        it = it->SubItem;                       /* 28(a0) */
        if (--n >= 0) {
            do
                it = it->NextItem;
            while (n-- != 0);
        }
    }
    return it;
}

/* the number of a menu code part: 31/63 (NOMENU ...) -> -1 */
static WORD menupart(UWORD v, UWORD all)
{
    return v == all ? -1 : v;
}

/* doevent2 - take the first event node; returns the class (0 = none
   or nothing to report) */
static ULONG doevent2(void)
{
    struct evnode *e = V.firstmess;
    ULONG class;
    UWORD c;

    if (!e)
        return 0;
    class = e->class;
    LONGAT(evcode) = *(ULONG *)&e->code;        /* code + qualifier */
    c = (UWORD)class;
    if (c == 0x40) {                            /* gadget up! */
        kludgegnum = 0;
        gadgetevent(e);
        goto done;
    }
    if (kludgegnum) {                           /* a dummy 'gadget up' */
        LONGAT(evmousex) = *(ULONG *)&e->mx;
        V.eventgnum = kludgegnum;
        kludgegnum = 0;
        return 0x40;
    }
    if (c == 0x20) {                            /* gadget down! */
        struct Gadget *g = e->iaddr;

        V.eventgnum = g->GadgetID;
        if (g->GadgetType & GTYP_STRGADGET)
            kludgegnum = V.eventgnum;
        goto done;
    }
    if (c == 0x100) {                           /* menu */
        UWORD code = e->code;
        struct Menu *strip;
        struct MenuItem *it;

        if (code == 0xffff) {
            class = 0;
            goto done;
        }
        V.menunum = menupart(code & 31, 31);
        V.itemnum = menupart((code >> 5) & 63, 63);
        V.subnum = menupart((UWORD)(code << 5 | code >> 11) & 31, 31);
        /* multiple select: one more node for the NextSelect */
        strip = e->win->MenuStrip;
        V.eventwin = e->win;                    /* BSR 'done */
        LONGAT(evmousex) = *(ULONG *)&e->mx;
        unlinkmess(&V.firstmess, e);
        it = findmenuitem(strip, code);
        code = it->NextSelect;
        if (code != 0xffff) {
            struct evnode *n = globalloc(24, MEMF_PUBLIC);

            *V.nextmess = n;
            V.nextmess = &n->next;
            n->next = NULL;
            n->class = 256;
            n->code = code;                     /* qualifier, iaddress: not set */
            *(ULONG *)&n->mx = LONGAT(evmousex);
            n->win = V.eventwin;
        }
        return 256;
    }
    if (c == 0x400) {                           /* a key */
        UWORD k = rawkey(e->code, e->qual);

        if (!k) {
            class = 0;
            goto done3;
        }
        V.outkey = k;                           /* outstanding keystroke */
        V.outr = e->code;
        V.outq = e->qual;
        V.outwindow = e->win;
        goto done;
    }
    if (c == 8) {                               /* mouse buttons */
        V.outmouse = e->code;
        V.outmwin = e->win;
    }
done:
    V.eventwin = e->win;
    LONGAT(evmousex) = *(ULONG *)&e->mx;
done3:
    unlinkmess(&V.firstmess, e);
    return class;
}

/* Event */
ULONG BLITZ_ROUTINE doevent(void)
{
    updateevents();
    return doevent2();
}

/* WaitEvent */
ULONG BLITZ_ROUTINE dowaitevent(void)
{
    for (;;) {
        ULONG c;

        updateevents();
        if (V.firstmess) {
            c = doevent2();
            if (c)
                return c;
            continue;
        }
        WaitPort(&myport);
    }
}

/* EventWindow etc. are above; MButtons: 0 = nothing, 1 left down, 2
   right down, 5 left up, 6 right up (only events of the input window) */
ULONG BLITZ_ROUTINE buttons(void)
{
    struct bwin *iw = V.inwindow;
    struct Window *win;
    ULONG code, r;

    if (!iw || !(win = iw->win)) {
        BLITZ_RUNERR(mess6);                    /* err6 */
        return 0;
    }
    if (V.outmwin) {
        if (V.outmwin == win) {
            code = V.outmouse;
            LONGAT(outmouse) = 0;               /* also the high word of outmwin */
            goto mgot;
        }
        V.outmwin = NULL;                       /* outskip */
    }
    updateevents();
    {
        struct evnode **prev = &V.firstmess, *e;

        for (;;) {
            e = *prev;
            if (!e)
                return 0;
            if (e->class != 8) {
                prev = &e->next;
                continue;
            }
            if (e->win == win)
                break;
            unlinkmess(prev, e);                /* wrong window: away */
        }
        code = e->code;
        unlinkmess(prev, e);
    }
mgot:
    r = 0;
    if (code & 0x80) {
        code &= ~0x80;
        r = 4;
    }
    if ((UBYTE)code == 104)
        r |= 1;
    if ((UBYTE)code == 105)
        r |= 2;
    return r;
}

/* ------------------------------------------------- input handlers */

/* windowinp - one key of the input window: d0 = character, -1 = none */
static ULONG windowinp(void)
{
    struct evnode **prev, *e;
    struct Window *iw;

    if (V.outkey) {
        if (!V.inwindow->win) {
            BLITZ_RUNERR(mess6);                /* err6 */
            return (ULONG)-1;
        }
        if (V.inwindow->win == V.outwindow) {
            ULONG k = V.outkey;

            V.outkey = 0;
            V.lrawkey = V.outr;
            V.kqualifier = V.outq;
            V.eventwin = V.outwindow;
            return k;
        }
    }
    updateevents();
    prev = &V.firstmess;
again:
    iw = V.inwindow->win;
    for (;;) {
        UWORD k;

        e = *prev;
        if (!e)
            return (ULONG)-1;
        if (e->class != 0x400) {
            prev = &e->next;
            continue;
        }
        if (iw && e->win != iw)
            goto notme;
        k = rawkey(e->code, e->qual);
        if (!k)
            goto notme;
        V.eventwin = e->win;
        V.lrawkey = e->code;
        V.kqualifier = e->qual;
        unlinkmess(prev, e);
        return k;
notme:
        unlinkmess(prev, e);                    /* wrong window: away */
        goto again;
    }
}

static ULONG BLITZ_ROUTINE windowinp_c(void)
{
    return windowinp();
}

/* calcwidth - width of a character in the font */
static WORD calcwidth(struct TextFont *f, UBYTE c)
{
    WORD w = f->tf_XSize;

    if ((f->tf_Flags & FPF_PROPORTIONAL) && c <= f->tf_HiChar) {
        BYTE i = (BYTE)(c - f->tf_LoChar);      /* SUB.b:BMI */

        if (i >= 0) {
            w = ((WORD *)f->tf_CharSpace)[(UBYTE)i];
            if (f->tf_CharKern)
                w += ((WORD *)f->tf_CharKern)[(UBYTE)i];
        }
    }
    return w;
}

/* the line editor's state */
struct ed {
    UBYTE *buf;                 /* a5 */
    WORD x;                     /* d7: cursor in the buffer */
    struct bwin *w;             /* a3 */
    struct TextFont *font;      /* a4 */
};

/* zcursor - the cursor (complement) at the cursor position */
static void zcursor(struct ed *ed)
{
    struct Library *gfxbase = V.gfxbase;
    struct RastPort *rp = ed->w->win->RPort;
    UBYTE mode = rp->DrawMode;
    WORD x0, y0, x1, y1;

    SetDrMd(rp, 3);
    x1 = calcwidth(ed->font, ed->buf[ed->x]) - 1;
    y1 = ed->font->tf_YSize - 1;
    x0 = ed->w->cx;
    y0 = ed->w->cy;
    x1 += x0;
    y1 += y0;
    if (V.underline >= 0)
        y0 = y1 - V.underline;
    RectFill(rp, x0, y0, x1, y1);
    SetDrMd(rp, mode);
}

/* update - redraw the characters from the cursor on (and a space) */
static void update(struct ed *ed)
{
    ULONG xy = *(ULONG *)&ed->w->cx;

    windowprint2(ed->buf + ed->x, (UWORD)(maxchars - ed->x));
    windowprint(32);
    *(ULONG *)&ed->w->cx = xy;
}

/* calclen - length without trailing spaces */
static UWORD calclen(struct ed *ed)
{
    WORD i = maxchars - 1;

    while (ed->buf[i] == 32 && --i != -1)       /* CMP.b:DBNE */
        ;
    return i + 1;
}

/* fill with spaces: DBF over maxchars - from - 1 */
static void blankfrom(UBYTE *buf, WORD from)
{
    UWORD n = maxchars - from - 1;

    buf += from;
    do
        *buf++ = 32;
    while (n-- != 0);
}

/* inputstr - the line editor: a0 = buffer, d0 = its length (maximum);
   returns the length of the line without trailing spaces */
static ULONG inputstr(UBYTE *buf, UWORD max)
{
    struct ed ed;
    UWORD c;
    WORD i;

    ed.buf = buf;
    maxchars = max;
    ed.w = V.prwindow;
    if (!ed.w || !ed.w->win) {
        BLITZ_RUNERR(mess7);                    /* err7 */
        return 0;
    }
    origxy = *(ULONG *)&ed.w->cx;
    ed.font = ed.w->win->RPort->Font;
    ed.x = 0;
    update(&ed);
    ed.x = V.efrom;
    if ((UWORD)ed.x >= maxchars)
        ed.x = maxchars - 1;
    for (i = 0; i < ed.x; i++)                  /* to the start position */
        ed.w->cx += calcwidth(ed.font, buf[i]);
    goto inmain2;

inmain:
    update(&ed);
inmain2:
    zcursor(&ed);
    {
        ULONG k = windowinp();

        if ((WORD)k >= 0) {
            c = k;
            goto gotone;
        }
    }
    for (;;) {                                  /* wloop */
        ULONG k;

        WaitPort(&myport);
        k = windowinp();
        if ((WORD)k >= 0) {
            c = k;
            goto gotone;
        }
        if (!V.firstmess) {
            k = windowinp();                    /* 'loop */
            if ((WORD)k >= 0) {
                c = k;
                goto gotone;
            }
            continue;
        }
        /* an event which wasn't a keypress: in editor mode, return */
        if (V.emode) {
            zcursor(&ed);
            c = 0xffff;                         /* MOVEQ #-1,d6 */
            goto doreturn;
        }
    }

gotone:
    zcursor(&ed);
    if (c < 32)
        goto lowchar;
    if (c > 126)
        goto hichar;
    if (V.kqualifier & 0x80) {                  /* right Amiga */
        if (c == 'd') {                         /* clear the line */
            blankfrom(buf, 0);
            *(ULONG *)&ed.w->cx = origxy;
            ed.x = 0;
            goto inmain;
        }
        if (c == 'y') {                         /* clear to the end */
            blankfrom(buf, ed.x);
            goto inmain;
        }
    }
    /* ascii: insert */
    for (i = maxchars; ; ) {
        i--;
        if ((UWORD)ed.x >= (UWORD)i)
            break;
        buf[i] = buf[i - 1];
    }
    buf[ed.x] = c;
    update(&ed);
    goto cursrite;

lowchar:
    if (c == 31)
        goto cursleft;
    if (c == 30)
        goto docursrite;
    if (c == 8)
        goto backspace;
    if (c == 13 || c == 10)
        goto doreturn;
    goto funnychar;

hichar:
    if (c == 127)
        goto delete;
funnychar:
    if (!V.emode)
        goto inmain2;                           /* ignore */
doreturn:
    V.eexit = c;
    V.eat = ed.x;
    windowprint(10);
    return calclen(&ed);

backspace:
    if (!ed.x)
        goto inmain2;
    ed.x--;                                     /* docursleft2 */
    ed.w->cx -= calcwidth(ed.font, buf[ed.x]);
delete:
    for (i = ed.x + 1; (UWORD)i < maxchars; i++)
        buf[i - 1] = buf[i];
    buf[i - 1] = 32;
    goto inmain;

cursleft:
    if (ed.x) {
        if (V.kqualifier & 3) {                 /* shift: to the start */
            *(ULONG *)&ed.w->cx = origxy;
            ed.x = 0;
        } else {
            ed.x--;
            ed.w->cx -= calcwidth(ed.font, buf[ed.x]);
        }
    }
    goto inmain2;

docursrite:
    if (V.kqualifier & 3) {                     /* shrite: to the end */
        WORD n = calclen(&ed), x;

        if ((UWORD)n >= maxchars)
            n--;
        ed.x = 0;
        x = (WORD)(origxy >> 16);
        while (--n >= 0)
            x += calcwidth(ed.font, buf[ed.x++]);
        ed.w->cx = x;
        goto inmain2;
    }
cursrite:
    ed.x++;
    if ((UWORD)ed.x >= maxchars) {
        ed.x--;
        goto inmain2;
    }
    ed.w->cx += calcwidth(ed.font, buf[ed.x - 1]);
    goto inmain2;
}

static ULONG BLITZ_ROUTINE inputstr_c(UBYTE *buf BL_REG(a0), UWORD max BL_REG(d0))
{
    return inputstr(buf, max);
}

/* The handlers with their BRA entries in front.  Each keeps what the
   original keeps (GCC keeps d2-d7/a2-a6 anyway). */
BLITZ_THUNK(inputstr_h, inputstr_c, "d1/a1");
BLITZ_THUNK(windowprint2_h, windowprint2_c, "d1/a1");
BLITZ_THUNK(windowprint3_h, windowprint3_c, "d1/a1");
__asm__("   .text\n   .even\n"
        "   bra.w   _inputstr_h\n"                /* -4 */
        "   .globl  _windowinp_h\n"
        "_windowinp_h:\n"                         /*  0 */
        "   movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _windowinp_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n"
        "   bra.w   _windowprint3_h\n"            /* -8 */
        "   bra.w   _windowprint2_h\n"            /* -4 */
        "   .globl  _windowprint_h\n"
        "_windowprint_h:\n"                       /*  0 */
        "   movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _windowprint_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n");
void windowinp_h(void), windowprint_h(void);

/* WindowInput / WindowOutput / _use (both) - a3 = the window object */
static void BLITZ_ROUTINE doinwindow_c(void)
{
    struct bl_regs r = { .a0 = (ULONG)windowinp_h };

    V.inwindow = (struct bwin *)blitz_string;
    BLITZ_ALIBJSR(TOK_setin, r);
}
BLITZ_THUNK(doinwindow, doinwindow_c, "d1/a1");

static void BLITZ_ROUTINE dooutwindow_c(void)
{
    struct bl_regs r = { .a0 = (ULONG)windowprint_h };

    V.prwindow = (struct bwin *)blitz_string;
    BLITZ_ALIBJSR(TOK_setout, r);
}
BLITZ_THUNK(dooutwindow, dooutwindow_c, "d1/a1");

static void dousewindow2(void)
{
    dooutwindow_c();
    doinwindow_c();
}

static void BLITZ_ROUTINE dousewindow_c(void)
{
    dousewindow2();
}
BLITZ_THUNK(dousewindow, dousewindow_c, "d1/a1");

/* ---------------------------------------------------------- drawing */

/* setpen2 - the window's rastport with pen, bpen and jam mode */
static struct RastPort *setpen2(struct bwin *w, UWORD pen, struct Library *gfxbase)
{
    struct RastPort *rp = w->win->RPort;

    SetAPen(rp, pen);
    SetBPen(rp, V.bpen);
    SetDrMd(rp, V.tjam);
    return rp;
}

/* WPlot x,y,c */
void BLITZ_ROUTINE dowplot(WORD x BL_REG(d0), WORD y BL_REG(d1), UWORD c BL_REG(d2),
                             struct bwin *w BL_REG(a1), struct Library *gfxbase BL_REG(a6))
{
    WritePixel(setpen2(w, c, gfxbase), x, y);
}

/* dowbox2 - RectFill */
static void dowbox2(struct bwin *w, UWORD c, WORD x0, WORD y0, ULONG x1, ULONG y1,
                    struct Library *gfxbase)
{
    RectFill(setpen2(w, c, gfxbase), x0, y0, x1, y1);
}

/* WBox x1,y1,x2,y2,c - corners sorted (unsigned word compare) */
void BLITZ_ROUTINE dowbox(ULONG x0 BL_REG(d0), ULONG y0 BL_REG(d1), ULONG x1 BL_REG(d2),
                            ULONG y1 BL_REG(d3), UWORD c BL_REG(d4),
                            struct bwin *w BL_REG(a1), struct Library *gfxbase BL_REG(a6))
{
    ULONG t;

    if ((UWORD)x0 > (UWORD)x1) {                /* EXG d0,d2 */
        t = x0; x0 = x1; x1 = t;
    }
    if ((UWORD)y0 > (UWORD)y1) {
        t = y0; y0 = y1; y1 = t;
    }
    dowbox2(w, c, x0, y0, x1, y1, gfxbase);
}

/* WCls [c] - RectFill 0,1,Width-1,Height-1 */
void BLITZ_ROUTINE dowcls2(UWORD c BL_REG(d0), struct bwin *w BL_REG(a1),
                             struct Library *gfxbase BL_REG(a6))
{
    struct Window *win = w->win;

    dowbox2(w, c, 0, 1, (WORD)(win->Width - 1), (WORD)(win->Height - 1), gfxbase);
}

void BLITZ_ROUTINE dowcls(struct bwin *w BL_REG(a1), struct Library *gfxbase BL_REG(a6))
{
    dowcls2(0, w, gfxbase);
}

/* InnerCls [c] - inside the borders */
void BLITZ_ROUTINE doinnercls2(UWORD c BL_REG(d0), struct bwin *w BL_REG(a1),
                                 struct Library *gfxbase BL_REG(a6))
{
    struct Window *win = w->win;
    WORD x1 = win->Width - win->BorderRight - 1;
    WORD y1 = win->Height - win->BorderBottom - 1;

    dowbox2(w, c, (UBYTE)win->BorderLeft, (UBYTE)win->BorderTop, x1, y1, gfxbase);
}

void BLITZ_ROUTINE doinnercls(struct bwin *w BL_REG(a1), struct Library *gfxbase BL_REG(a6))
{
    doinnercls2(0, w, gfxbase);
}

/* WCircle x,y,r,c / WEllipse x,y,rx,ry,c */
void BLITZ_ROUTINE dowellipse(WORD x BL_REG(d0), WORD y BL_REG(d1), ULONG rx BL_REG(d2),
                                ULONG ry BL_REG(d3), UWORD c BL_REG(d4),
                                struct bwin *w BL_REG(a1), struct Library *gfxbase BL_REG(a6))
{
    DrawEllipse(setpen2(w, c, gfxbase), x, y, rx, ry);
}

void BLITZ_ROUTINE dowcircle(WORD x BL_REG(d0), WORD y BL_REG(d1), ULONG r BL_REG(d2),
                               UWORD c BL_REG(d3), struct bwin *w BL_REG(a1),
                               struct Library *gfxbase BL_REG(a6))
{
    dowellipse(x, y, r, r, c, w, gfxbase);
}

/* WScroll x1,y1,x2,y2,dx,dy - BUG(orig) (#315): A pen 0 */
void BLITZ_ROUTINE dowscroll(ULONG x0 BL_REG(d0), ULONG y0 BL_REG(d1), ULONG x1 BL_REG(d2),
                               ULONG y1 BL_REG(d3), WORD dx BL_REG(d4), WORD dy BL_REG(d5),
                               struct bwin *w BL_REG(a1), struct Library *gfxbase BL_REG(a6))
{
    ScrollRaster(setpen2(w, 0, gfxbase), dx, dy, x0, y0, x1, y1);
}

/* WPrintScroll - scroll the window if the cursor is below its inner
   height */
void BLITZ_ROUTINE dowprintscroll(struct bwin *w BL_REG(a1),
                                    struct Library *gfxbase BL_REG(a6))
{
    struct Window *win = w->win;
    WORD d = w->cy - win->GZZHeight + win->RPort->TxHeight;

    if (d <= 0)
        return;
    w->cy -= d;
    ScrollRaster(setpen2(w, 0, gfxbase), 0, d, 0, 0, win->GZZWidth, win->GZZHeight);
}

/* WLine x1,y1,...,c - d7 = number of points (pairs on the argument
   stack, swapped in place to x,y), d0 = colour */
void BLITZ_ROUTINE dowline(UWORD c BL_REG(d0), UWORD n BL_REG(d7), UBYTE *args BL_REG(a2),
                             struct bwin *w BL_REG(a1), struct Library *gfxbase BL_REG(a6))
{
    struct RastPort *rp = setpen2(w, c, gfxbase);
    WORD *pts = (WORD *)(args - (WORD)(n << 2));
    WORD *p = pts;
    UWORD i = n - 1;

    do {                                        /* cswap */
        WORD t = p[0];

        p[0] = p[1];
        p[1] = t;
        p += 2;
    } while (i-- != 0);
    Move(rp, pts[0], pts[1]);
    PolyDraw(rp, (WORD)(n - 1), pts + 2);
}

/* the drawing entries are Amiga-only (+1 in !subs) and end in a graphics
   JMP: no registers to keep */

/* ------------------------------------------------- window operations */

/* WTitle win$,scr$ - copies of both strings (allocst, never freed) */
void BLITZ_ROUTINE dowtitle(const UBYTE *wt BL_REG(d0), const UBYTE *st BL_REG(d1),
                              const ULONG *lens BL_REG(a2), struct bwin *w BL_REG(a1),
                              struct Library *intbase BL_REG(a6))
{
    struct Window *win = w->win;
    const ULONG *sp = lens;
    UBYTE *wtitle, *stitle;

    wtitle = allocst(wt, BL_POPL(sp));
    stitle = allocst(st, BL_POPL(sp));
    SetWindowTitles(win, (STRPTR)wtitle, (STRPTR)stitle);
}

/* WMove x,y - a0 = used window; waits until Intuition has moved it */
void BLITZ_ROUTINE dowmove(ULONG x BL_REG(d0), ULONG y BL_REG(d1), struct bwin *w BL_REG(a0),
                             struct Library *intbase BL_REG(a6))
{
    volatile struct Window *win = w->win;

    MoveWindow((struct Window *)win, (x & 0xffff0000UL) | (UWORD)(x - win->LeftEdge),
               (y & 0xffff0000UL) | (UWORD)(y - win->TopEdge));
    while (win->LeftEdge != (WORD)x || win->TopEdge != (WORD)y)
        ;
}

/* WSize w,h */
void BLITZ_ROUTINE dowsize(ULONG x BL_REG(d0), ULONG y BL_REG(d1), struct bwin *w BL_REG(a0),
                             struct Library *intbase BL_REG(a6))
{
    volatile struct Window *win = w->win;

    SizeWindow((struct Window *)win, (x & 0xffff0000UL) | (UWORD)(x - win->Width),
               (y & 0xffff0000UL) | (UWORD)(y - win->Height));
    while (win->Width != (WORD)x || win->Height != (WORD)y)
        ;
}

/* newidcmp - a2 = window objects (base), d2 = maximum; RMBTRAP and
   MENUVERIFY of every window (BUG(orig) (#310): 16 byte steps) */
static void newidcmp(UBYTE *objs, UWORD max, UWORD verify, UWORD rmbtrap,
                     struct Library *intbase)
{
    UWORD n = max - 1;

    do {
        struct Window *win = *(struct Window **)objs;

        if (win) {
            ULONG f;

            ((UBYTE *)&win->Flags)[1] &= ~1;    /* BCLR #0,25(a0) */
            ((UWORD *)&win->Flags)[0] |= rmbtrap;
            f = win->IDCMPFlags & ~0x2000UL;
            f |= verify;                        /* OR.w */
            ModifyIDCMP(win, f);
        }
        objs += 16;
    } while (n-- != 0);
}

void BLITZ_ROUTINE domenuson(UBYTE *objs BL_REG(a2), UWORD max BL_REG(d2),
                               struct Library *intbase BL_REG(a6))
{
    newidcmp(objs, max, 0, 0, intbase);
}

void BLITZ_ROUTINE domenusoff(UBYTE *objs BL_REG(a2), UWORD max BL_REG(d2),
                                struct Library *intbase BL_REG(a6))
{
    newidcmp(objs, max, 0x2000, 1, intbase);
}

void BLITZ_ROUTINE domenusok(UWORD on BL_REG(d0), UBYTE *objs BL_REG(a2), UWORD max BL_REG(d2),
                               struct Library *intbase BL_REG(a6))
{
    if (on)
        newidcmp(objs, max, 0, 0, intbase);
    else
        newidcmp(objs, max, 0x2000, 1, intbase);
}

/* WPointer shape# - a2 = the shape, a3 = the used window: a sprite
   pointer from the first two planes (one word wide) */
void BLITZ_ROUTINE dowpointer(const UBYTE *shape BL_REG(a2), ULONG xoff_reg BL_REG(d2),
                                ULONG yoff_reg BL_REG(d3), struct Library *intbase BL_REG(a6))
{
    struct bwin *w = (struct bwin *)blitz_string;
    const UWORD *src;
    UWORD *dst, n, pl;
    ULONG size;

    if (w->ptrdata) {                           /* pointer already there? */
        ClearPointer(w->win);
        globfree(w->ptrdata, w->ptrsize);
        w->ptrdata = NULL;
    }
    size = (UWORD)((UWORD)(*(UWORD *)(shape + 2) + 2) << 2);
    w->ptrsize = size;
    dst = globalloc(size, MEMF_CHIP);
    w->ptrdata = dst;
    *(ULONG *)dst = 0;
    dst += 2;
    src = *(const UWORD **)(shape + 14);
    pl = *(UWORD *)(shape + 22);
    n = *(UWORD *)(shape + 2) - 1;
    do {
        *dst++ = *src++;
        *dst++ = *(const UWORD *)((const UBYTE *)src - 2 + (WORD)pl);
    } while (n-- != 0);
    *(ULONG *)dst = 0;
    SetPointer(w->win, w->ptrdata, *(UWORD *)(shape + 2), *(UWORD *)shape,
               (xoff_reg & 0xffff0000UL) | *(UWORD *)(shape + 10),
               (yoff_reg & 0xffff0000UL) | *(UWORD *)(shape + 12));
}

/* WindowFont font#[,style] - a0 = the IntuiFont object (+8 = TextFont) */
void BLITZ_ROUTINE dowfont(const struct ifont *font BL_REG(a0), struct bwin *w BL_REG(a1),
                             struct Library *gfxbase BL_REG(a6))
{
    SetFont(w->win->RPort, font->font);
}

void BLITZ_ROUTINE dowfont2(ULONG style BL_REG(d1), const struct ifont *font BL_REG(a0),
                              struct bwin *w BL_REG(a1), struct Library *gfxbase BL_REG(a6))
{
    struct RastPort *rp = w->win->RPort;

    SetFont(rp, font->font);
    SetSoftStyle(rp, style, (ULONG)-1);
}

/* BitMaptoWindow bm#,win#[,sx,sy,dx,dy,w,h] - a0 = window, a1 = bitmap */
void BLITZ_ROUTINE dobtow1(struct bwin *w BL_REG(a0), struct BitMap *bm BL_REG(a1),
                             struct Library *gfxbase BL_REG(a6))
{
    BltBitMapRastPort(bm, 0, 0, w->win->RPort, 0, 0, (WORD)(bm->BytesPerRow << 3),
                      (WORD)bm->Rows, 0xc0);
}

void BLITZ_ROUTINE dobtow2(ULONG sx_reg BL_REG(d0), ULONG sy_reg BL_REG(d1), ULONG sx BL_REG(d2),
                             ULONG sy BL_REG(d3), ULONG dx BL_REG(d4), ULONG dy BL_REG(d5),
                             const UWORD *args BL_REG(a2), struct bwin *w BL_REG(a0),
                             struct BitMap *bm BL_REG(a1), struct Library *gfxbase BL_REG(a6))
{
    const UWORD *sp = args;
    UWORD wd = BL_POPW(sp), ht = BL_POPW(sp);

    /* MOVE.w: the upper words of the registers stay */
    BltBitMapRastPort(bm, (sx_reg & 0xffff0000UL) | (UWORD)sx, (sy_reg & 0xffff0000UL) | (UWORD)sy,
                      w->win->RPort, (sx & 0xffff0000UL) | (UWORD)dx,
                      (sy & 0xffff0000UL) | (UWORD)dy, (dx & 0xffff0000UL) | wd,
                      (dy & 0xffff0000UL) | ht, 0xc0);
}

/* WBlit shape#,x,y - a0 = used window, a2 = the shape: its planes as a
   BitMap (mybmap), with the cookie as mask if there is one */
void BLITZ_ROUTINE dowblit(ULONG xpos BL_REG(d1), ULONG ypos BL_REG(d2), ULONG y_reg BL_REG(d3),
                             struct bwin *w BL_REG(a0), const UBYTE *shape BL_REG(a2),
                             struct Library *gfxbase BL_REG(a6))
{
    struct RastPort *rp = w->win->RPort;
    UBYTE *plane = *(UBYTE **)(shape + 14);
    WORD psize = *(WORD *)(shape + 22);
    ULONG x, y, cookie;
    int i;

    *(UWORD *)&mybmap.BytesPerRow = *(UWORD *)(shape + 6);
    *(UWORD *)&mybmap.Rows = *(UWORD *)(shape + 2);
    *(UWORD *)&mybmap.Flags = *(UWORD *)(shape + 4);
    for (i = 0; i < 8; i++) {
        mybmap.Planes[i] = plane;
        plane += psize;
    }
    y = (y_reg & 0xffff0000UL) | (UWORD)(ypos - *(UWORD *)(shape + 12));
    x = (ypos & 0xffff0000UL) | (UWORD)(xpos - *(UWORD *)(shape + 10));
    cookie = *(ULONG *)(shape + 18);
    if (cookie)
        BltMaskBitMapRastPort(&mybmap, 0, 0, rp, x, y, *(WORD *)shape, *(WORD *)(shape + 2),
                              0xe0, (PLANEPTR)cookie);
    else
        BltBitMapRastPort(&mybmap, 0, 0, rp, x, y, *(WORD *)shape, *(WORD *)(shape + 2), 0xc0);
}

/* PositionSuperBitMap x,y - ScrollLayer by the difference */
void BLITZ_ROUTINE _showsuper(WORD x BL_REG(d0), WORD y BL_REG(d1), struct bwin *w BL_REG(a1),
                                struct Library *layersbase BL_REG(a6))
{
    WORD ox = w->sbx, oy = w->sby;

    w->sbx = x;
    w->sby = y;
    ScrollLayer(0, w->win->RPort->Layer, (WORD)(x - ox), (WORD)(y - oy));
}

/* GetSuperBitMap / PutSuperBitMap */
void BLITZ_ROUTINE _windowtobitmap(struct bwin *w BL_REG(a0),
                                     struct Library *gfxbase BL_REG(a6))
{
    SyncSBitMap(w->win->RPort->Layer);
}

void BLITZ_ROUTINE _bitmaptowindow(struct bwin *w BL_REG(a0),
                                     struct Library *gfxbase BL_REG(a6))
{
    CopySBitMap(w->win->RPort->Layer);
}

/* ------------------------------------------------ opening, closing */

/* freewindow (CloseWindow, _free) - a3 = the window object */
void BLITZ_ROUTINE freewindow(struct Library *intbase BL_REG(a6))
{
    struct bwin *w = (struct bwin *)blitz_string;
    struct Window *win = w->win;
    struct Message *m;

    if (!win)
        return;
    Forbid();
    while ((m = GetMsg(&myport)) != NULL)       /* try flushing messages! */
        ReplyMsg(m);
    if (THISPROC->pr_WindowPtr == win)
        THISPROC->pr_WindowPtr = NULL;
    if (win->MenuStrip)
        ClearMenuStrip(win);
    w->win = NULL;
    win->UserPort = NULL;
    CloseWindow(win);
    Permit();
    unlinkall();
}

/* the end of Window/WindowTags: our port, window number, cursor origin */
static void windowopened(struct bwin *w, struct Window *win, UWORD num,
                         struct Library *intbase)
{
    win->UserPort = &myport;
    WUD(win)->num = num;
    /* orgx, orgy = BorderLeft, BorderTop (one word) */
    *(UWORD *)&WUD(win)->orgx = *(UWORD *)&win->BorderLeft;
    if (win->Flags & WFLG_GIMMEZEROZERO)
        *(UWORD *)&WUD(win)->orgx = 0;
    *(ULONG *)&w->cx = 0;
    *(ULONG *)&w->sbx = 0;
    ((UBYTE *)&w->cx)[1] = WUD(win)->orgx;
    ((UBYTE *)&w->cy)[1] = WUD(win)->orgy;
    ModifyIDCMP(win, V.idcmp);
}

/* Window - a3 = the object, a1 = used screen, a0 = gadget list, d6 =
   offset of the used window, d0 = object offset (number << 5) */
static void openwindow(struct bwin *w, UWORD objoff, ULONG flags, const UBYTE *title,
                       UBYTE dpen, UBYTE bpen, ULONG *glist, struct Screen **scr,
                       WORD usedoff, const ULONG *lens, struct Library *intbase)
{
    struct NewWindow *nw = &anewwindow;
    const WORD *sp = (const WORD *)lens;
    struct Window *win;
    UWORD num;

    *(struct bwin **)(blitz_globals + usedoff) = w;     /* set used window! */
    num = objoff >> 5;
    freewindow(intbase);
    nw->MinWidth = nw->MinHeight = 0;
    nw->MaxWidth = nw->MaxHeight = 0;
    nw->DetailPen = dpen;
    nw->BlockPen = bpen;
    nw->FirstGadget = (struct Gadget *)*glist;
    if (sizelimitsok) {
        nw->MinWidth = sizelimits.minw;
        nw->MinHeight = sizelimits.minh;
        nw->MaxWidth = sizelimits.maxw;
        nw->MaxHeight = sizelimits.maxh;
    }
    nw->LeftEdge = BL_POPW(sp);
    nw->TopEdge = BL_POPW(sp);
    nw->Width = BL_POPW(sp);
    nw->Height = BL_POPW(sp);
    if (V.idcmp & 0x10)                         /* MOUSEMOVE: REPORTMOUSE */
        flags |= 0x200;
    if (flags & 0x80) {                         /* super bitmap */
        nw->BitMap = (struct BitMap *)bmapbase;
        flags |= 0x20000;
    }
    nw->Flags = flags;
    nw->Title = allocst(title, *(const ULONG *)((const UBYTE *)lens - 12));
    nw->Screen = *scr;
    nw->Type = (*scr)->Flags & 15;
    win = OpenWindow(nw);
    w->win = win;
    if (!win) {
        BLITZ_RUNERR(mess1);                    /* err1 */
        return;
    }
    windowopened(w, win, num, intbase);
    blitz_string = (UBYTE *)w;                  /* MOVE.l (a7)+,a3 */
    dousewindow2();
}

/* the three forms: d1 = flags, d2 = title, d3/d4 = pens */
void BLITZ_ROUTINE dowindow(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                              const UBYTE *title BL_REG(d2), UBYTE dpen BL_REG(d3),
                              UBYTE bpen BL_REG(d4), ULONG *glist BL_REG(a0),
                              struct Screen **scr BL_REG(a1), WORD usedoff BL_REG(d6),
                              const ULONG *lens BL_REG(a2), struct Library *intbase BL_REG(a6))
{
    openwindow((struct bwin *)blitz_string, objoff, flags, title, dpen, bpen, glist, scr,
               usedoff, lens, intbase);
}

void BLITZ_ROUTINE dowindow2(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                               const UBYTE *title BL_REG(d2), UBYTE dpen BL_REG(d3),
                               UBYTE bpen BL_REG(d4),
                               struct Screen **scr BL_REG(a1), WORD usedoff BL_REG(d6),
                               const ULONG *lens BL_REG(a2), struct Library *intbase BL_REG(a6))
{
    openwindow((struct bwin *)blitz_string, objoff, flags, title, dpen, bpen, &V.zero, scr,
               usedoff, lens, intbase);
}

/* dowindow3 - pens from the stack (-10/-12(a2)), a4 = the bitmap */
void BLITZ_ROUTINE dowindow3(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                               const UBYTE *title BL_REG(d2), ULONG *glist BL_REG(a0),
                               struct Screen **scr BL_REG(a1), WORD usedoff BL_REG(d6),
                               const ULONG *lens BL_REG(a2), struct Library *intbase BL_REG(a6))
{
    const UWORD *sp = (const UWORD *)lens;

    bmapbase = (struct bwin *)blitz_locals;     /* MOVE.l a4,bmapbase */
    openwindow((struct bwin *)blitz_string, objoff, flags, title, (UBYTE)sp[-5], (UBYTE)sp[-6],
               glist, scr, usedoff, lens, intbase);
}

/* WindowTags - d1 = flags, d2 = title (copied), screen, then the user
   tags behind the four fixed ones */
static void opentags(struct bwin *w, UWORD objoff, ULONG flags, const UBYTE *title, ULONG len,
                     struct Screen **scr, WORD usedoff, struct Library *intbase)
{
    UBYTE *wtitle = allocst(title, len);
    struct Window *win;
    UWORD num;

    *(struct bwin **)(blitz_globals + usedoff) = w;     /* set used window! */
    num = objoff >> 5;
    freewindow(intbase);
    tags.flags.ti_Data = flags;
    tags.title.ti_Data = (ULONG)wtitle;
    tags.screen.ti_Data = (ULONG)*scr;
    win = OpenWindowTagList(NULL, &tags.flags);
    w->win = win;
    if (!win) {
        BLITZ_RUNERR(mess1);                    /* err1 */
        return;
    }
    windowopened(w, win, num, intbase);
    blitz_string = (UBYTE *)w;
    dousewindow2();
}

void BLITZ_ROUTINE dowindowtags1(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                                   const UBYTE *title BL_REG(d2),
                                   struct Screen **scr BL_REG(a1), WORD usedoff BL_REG(d6),
                                   const ULONG *lens BL_REG(a2),
                                   struct Library *intbase BL_REG(a6))
{
    opentags((struct bwin *)blitz_string, objoff, flags, title, lens[-1], scr, usedoff, intbase);
}

/* &TagList: copied up to and including TAG_DONE */
void BLITZ_ROUTINE dowindowtags3(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                                   const UBYTE *title BL_REG(d2), const ULONG *tl BL_REG(d3),
                                   struct Screen **scr BL_REG(a1), WORD usedoff BL_REG(d6),
                                   const ULONG *lens BL_REG(a2),
                                   struct Library *intbase BL_REG(a6))
{
    struct TagItem *d = tags.user;     /* BUG(orig) (#317): no limit */
    ULONG tag;

    do {
        tag = *tl++;
        d->ti_Tag = tag;
        d->ti_Data = *tl++;
        d++;
    } while (tag);
    /* DIFF (K): the original keeps the title length with MOVE.l -(a2),
       -(a7) / MOVE.l (a7)+,(a2)+ in the 4 bytes behind the TAG_DONE pair
       of the caller's list (a2 = the list) and reads it back there; the C
       version takes it from the argument stack and writes nothing */
    opentags((struct bwin *)blitz_string, objoff, flags, title, lens[-1], scr, usedoff, intbase);
}

/* tag pairs on the argument stack, d7 = their number */
void BLITZ_ROUTINE dowindowtags2(UWORD objoff BL_REG(d0), ULONG flags BL_REG(d1),
                                   const UBYTE *title BL_REG(d2), UWORD n BL_REG(d7),
                                   struct Screen **scr BL_REG(a1), WORD usedoff BL_REG(d6),
                                   const ULONG *lens BL_REG(a2),
                                   struct Library *intbase BL_REG(a6))
{
    const ULONG *sp = lens;
    ULONG len = BL_POPL(sp);
    struct TagItem *d = tags.user;

    while (n-- != 0) {                          /* DBRA d7 */
        d->ti_Tag = BL_POPL(sp);
        d->ti_Data = BL_POPL(sp);
        d++;
    }
    d->ti_Tag = TAG_DONE;                       /* data not set */
    opentags((struct bwin *)blitz_string, objoff, flags, title, len, scr, usedoff, intbase);
}

/* ------------------------------------------------------ init, finit */

/* init - a0 = graphics, a1 = intuition: the port; returns its address */
ULONG BLITZ_ROUTINE codeinit(struct Library *gfx BL_REG(a0), struct Library *intb BL_REG(a1))
{
    struct List *l = &myport.mp_MsgList;

    V.gfxbase = gfx;
    intbase_g = intb;
    oldwptr = THISPROC->pr_WindowPtr;
    l->lh_TailPred = (struct Node *)l;          /* the list header */
    l->lh_Head = (struct Node *)&l->lh_Tail;
    l->lh_Tail = NULL;
    myport.mp_SigTask = FindTask(NULL);
    myport.mp_SigBit = AllocSignal(-1);
    myport.mp_Flags = 0;                        /* myact: PA_SIGNAL */
    V.firstmess = NULL;
    V.nextmess = &V.firstmess;
    *(ULONG *)&V.eventgnum = (ULONG)-1;         /* + high word of eventwin */
    LONGAT(menunum) = (ULONG)-1;                /* + itemnum */
    V.subnum = -1;
    V.prwindow = NULL;
    V.outkey = 0;
    V.eventwin = NULL;
    V.kqualifier = 0;
    V.underline = -1;
    V.emode = 0;
    V.efrom = 0;
    V.eexit = 0;
    V.fpen = 1;
    V.bpen = 0;
    V.tjam = 1;
    V.outmwin = NULL;
    V.linw = 0;
    sizelimits.minw = 128;
    sizelimits.minh = 64;
    sizelimits.maxw = -1;
    sizelimits.maxh = -1;
    sizelimitsok = -1;
    LONGAT(evmousex) = 0;
    return (ULONG)&myport;
}

/* finit - FreeSignal, the old pr_WindowPtr */
void BLITZ_ROUTINE codefinit(void)
{
    FreeSignal(myport.mp_SigBit);
    THISPROC->pr_WindowPtr = oldwptr;
}


/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized Screen");
BLITZ_ERRCHK_MESSAGE(mess3, "Uninitialized Window");
BLITZ_ERRCHK_MESSAGE(mess4, "Uninitialized IntuiFont");
BLITZ_ERRCHK_MESSAGE(mess5, "Not Enough Coordinates");
BLITZ_ERRCHK_MESSAGE(mess8, "Uninitialized Shape");
BLITZ_ERRCHK_MESSAGE(mess10, "Can't move Window");
BLITZ_ERRCHK_MESSAGE(mess11, "Can't Size Window");
BLITZ_ERRCHK_MESSAGE(mess12, "Ininitialised Shape");
BLITZ_ERRCHK_MESSAGE(mess13, "Ininitialised BitMap");
BLITZ_ERRCHK_MESSAGE(idcmpmess, "IDCMP must not be 0");

/* wchk - a1 = used window, open */
static void BLITZ_ERRCHK wchk_c(const ULONG *w BL_REG(a1))
{
    if (!*w)
        BLITZ_RUNERR(mess3);
}

/* wfontchk - and a0 = an IntuiFont with font */
static void BLITZ_ERRCHK wfontchk_c(const ULONG *w BL_REG(a1),
                                    const struct ifont *font BL_REG(a0))
{
    if (!*w)
        BLITZ_RUNERR(mess3);
    else if (!font->font)
        BLITZ_RUNERR(mess4);
}

/* wlinechk - at least 2 points (d7, unsigned word) */
static void BLITZ_ERRCHK wlinechk_c(UWORD n BL_REG(d7), const ULONG *w BL_REG(a1))
{
    if (n < 2)
        BLITZ_RUNERR(mess5);
    else if (!*w)
        BLITZ_RUNERR(mess3);
}

/* wpointerchk - a2 = shape (word), a3 = window */
static void BLITZ_ERRCHK wpointerchk_c(const UWORD *shape BL_REG(a2))
{
    if (!*shape)
        BLITZ_RUNERR(mess8);
    else if (!*(const ULONG *)blitz_string)
        BLITZ_RUNERR(mess3);
}

/* wmovechk - a0 = used window; new position >= 0 and the window inside
   its screen (unsigned word compares) */
static void BLITZ_ERRCHK wmovechk_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1),
                                    struct Window **w BL_REG(a0))
{
    struct Window *win = *w;

    if (!win) {
        BLITZ_RUNERR(mess3);
        return;
    }
    if ((WORD)x < 0 || (WORD)y < 0) {
        BLITZ_RUNERR(mess10);
        return;
    }
    if ((UWORD)(x + win->Width) > (UWORD)win->WScreen->Width
        || (UWORD)(y + win->Height) > (UWORD)win->WScreen->Height)
        BLITZ_RUNERR(mess10);
}

/* wsizechk - size > 0 and the window inside its screen */
static void BLITZ_ERRCHK wsizechk_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1),
                                    struct Window **w BL_REG(a0))
{
    struct Window *win = *w;

    if (!win) {
        BLITZ_RUNERR(mess3);
        return;
    }
    if ((WORD)x <= 0 || (WORD)y <= 0) {
        BLITZ_RUNERR(mess11);
        return;
    }
    if ((UWORD)(x + win->LeftEdge) > (UWORD)win->WScreen->Width
        || (UWORD)(y + win->TopEdge) > (UWORD)win->WScreen->Height)
        BLITZ_RUNERR(mess11);
}

static void BLITZ_ERRCHK idcmpchk_c(ULONG f BL_REG(d0))
{
    if (!f)
        BLITZ_RUNERR(idcmpmess);
}

/* wblitchk - a0 = window, a2 = shape */
static void BLITZ_ERRCHK wblitchk_c(const ULONG *w BL_REG(a0), const UWORD *shape BL_REG(a2))
{
    if (!*w)
        BLITZ_RUNERR(mess3);
    else if (!*shape)
        BLITZ_RUNERR(mess12);
}

/* wbtowchk - a0 = window, a1 = bitmap (word at 62) */
static void BLITZ_ERRCHK wbtowchk_c(const ULONG *w BL_REG(a0), const UBYTE *bm BL_REG(a1))
{
    if (!*w)
        BLITZ_RUNERR(mess3);
    else if (!*(const UWORD *)(bm + 62))
        BLITZ_RUNERR(mess13);
}

/* the original checks change d6/d7, a1/a2 (wmovechk, wsizechk); the C
   checks keep d0-d1/a0-a1 */
BLITZ_ERRCHK_THUNK(wchk, wchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(wfontchk, wfontchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(wlinechk, wlinechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(wpointerchk, wpointerchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(wmovechk, wmovechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(wsizechk, wsizechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(idcmpchk, idcmpchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(wblitchk, wblitchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(wbtowchk, wbtowchk_c, "d0-d1/a0-a1");
