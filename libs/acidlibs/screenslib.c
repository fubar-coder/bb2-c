/*
 * screenslib - command library 158: intuition screens.  Screen
 * WbToScreen LoadScreen FindScreen SMouseX SMouseY ShowScreen ScreenPens
 * ViewPort ShowBitMap CloseScreen HideScreen BeepScreen MoveScreen
 * ScreenTags.
 *
 * C translation of bblibsrc/screenslib (30 Aug 1993 / Aug 1994 "screen
 * rounds size to nearest 16"); binary of the CD's acidlibs checked: same
 * as the source.
 *
 * Screen object (8 bytes, maximum 5): {screen.l, DBufInfo.l (ShowBitMap
 * on V39+)}.  Screens opened by the library get UserData = "bLtZ";
 * CloseScreen/free only closes those (title copy freed with maxslib
 * $CF02, DBufInfo with FreeDBufInfo), screens found with WbToScreen /
 * FindScreen are only forgotten.
 *
 * Screen n,mode[,title$]: depth = mode & 7 (0 = 8), bit 3 hires (640,
 * HIRES), bit 4 interlace (height * 2, LACE), height 200 (NTSC, bit 0 of
 * GfxBase+207) or 256 - through a fake a2 stack (`dummydata`) into the
 * long form Screen n,x,y,w,h,d,viewmode,title$,dpen,bpen[,bitmap#]:
 * width rounded UP to a multiple of 16, ExtNewScreen with SA_DisplayID
 * (= viewmode) and SA_Pens (ScreenPens), the font = the current IntuiFont
 * (intuifontlib's TextAttr), type CUSTOMSCREEN (|CUSTOMBITMAP) | the
 * ScreenPens flag NS_EXTENDED ($1000; ScreenPens 0 clears it).
 * ScreenTags n,title$[,taglist] / [,tag,data...]: OpenScreenTagList with
 * SA_Title, SA_Font, SA_ErrorCode and the user tags (V36+); result: the
 * screen or the error code (-1 if none was set).
 * ShowBitMap: V39+ ChangeVPBitMap (DBufInfo allocated once per object);
 * before: copy the plane pointers into the screen's copper list (BPLxPT
 * moves) or, for interlace, into the screen's BitMap + MakeVPort, then
 * MrgCop/LoadView with interrupts off.  environlib ExecVersion ($6087)
 * decides (the emulator's Kickstart is V40, so only the V39 path runs).
 *
 * Inline routines (`Dc $a000`): ViewPort, ShowScreen, HideScreen,
 * BeepScreen, MoveScreen, SMouseX, SMouseY (assembler text, as in the
 * original).
 *
 * BUG(orig), kept:
 *   - the routines that free the old screen of an object call freescreen
 *     with a2 = their argument stack instead of the graphics base: if the
 *     object has a DBufInfo (ShowBitMap on V39+ before), Screen /
 *     ScreenTags / WbToScreen / FindScreen on that object call
 *     FreeDBufInfo through garbage (crash). (#247)
 *   - Not a bug (library-opened screens are always closed by the library,
 *     no leak found): the title copy (allocst) of FindScreen/WbToScreen
 *     objects is not involved, but a ScreenTags/Screen title is only freed
 *     when the screen is closed by the library.
 *   - ScreenTags with a taglist copies it without a limit into 32 longs. (#249)
 * Fixed (intended deviation, #250): the original gives SA_DisplayID only
 *   the low word of the viewmode (MOVE d1,newmodes+2, the upper word stays
 *   0): display IDs with a monitor part (PAL, NTSC, DblPAL, ...) opened on
 *   the default monitor.  Here SA_DisplayID gets the whole long.
 * Fixed (intended deviation, #251): the original's "Can't open Screen" is
 *   `BEQ err1` from the normal code into the error area (without the
 *   debugger a jump behind the library), and err8/12/13 (WorkBench /
 *   Screen / CopIns not found) are in the normal code with their texts in
 *   the error area (without the debugger d0 points behind the library).
 *   Here all four traps and their texts are in the normal code: the same
 *   runtime errors with the debugger, TRAP #0 with the right text without
 *   it.
 * DIFF: a2 stays unchanged (the original pops the arguments).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#define __NOLIBBASE__
#include <proto/intuition.h>
#include <proto/graphics.h>
#undef __NOLIBBASE__

#define BLTZ            0x624c745aUL                  /* "bLtZ" */

/* entry points (thunks / inline code, defined below) */
void dosmousex(void), dosmousey(void), doshowscreen(void), doscreenpens2(void);
void doscreenpens(void), dosviewport(void), freescreen(void), dohidescreen(void);
void dobeepscreen(void), domovescreen(void), doinit(void);
void modechk(void), prescreen(void), prescreen2(void), loadscreenchk(void);
void showbmchk0(void), showbmchk(void);
/* C routines with register parameters, for the table: e_NAME = NAME */
BLITZ_ENTRY_C(domode);
BLITZ_ENTRY_C(domode2);
BLITZ_ENTRY_C(_screen);
BLITZ_ENTRY_C(_screen2);
BLITZ_ENTRY_C(dowbscreen);
BLITZ_ENTRY_C(dofstoscreen);
BLITZ_ENTRY_C(dofindscreen);
BLITZ_ENTRY_C(doloadscreen);
BLITZ_ENTRY_C(doloadscreen2);
BLITZ_ENTRY_C(doshowbitmap0);
BLITZ_ENTRY_C(doshowbitmap1);
BLITZ_ENTRY_C(doscreentags3);
BLITZ_ENTRY_C(doscreentags);
BLITZ_ENTRY_C(doscreentags2);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);


/* !libheader{#screenslib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_screenslib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_intuitionlib, BL_LA(6), LIB_screenslib, 0x1380, LIB_intuifontlib, BL_LA(1),
           LIB_screenslib, 0x0604, LIB_graphicslib, BL_LA(0));
BLITZ_SUBS(BLITZ_AMIGAONLY(e_domode), modechk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_intuitionlib, BL_LA(6), LIB_screenslib, 0x1380, LIB_intuifontlib, BL_LA(1),
           LIB_screenslib, 0x0604, LIB_graphicslib, BL_LA(0));
BLITZ_SUBS(BLITZ_AMIGAONLY(e_domode2), modechk, 0);
BLITZ_ARGS(BL_WORD, 0x42, 0x42, 0x42, 0x42, 0x42, BL_LONG, BL_STRING, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_intuitionlib, BL_LA(6), LIB_screenslib, 0x1380, LIB_intuifontlib, BL_LA(1),
           LIB_screenslib, 0x0604);
BLITZ_SUBS(BLITZ_AMIGAONLY(e__screen), prescreen, 0);
BLITZ_ARGS(BL_WORD, 0x42, 0x42, 0x42, 0x42, 0x42, BL_LONG, BL_STRING, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_intuitionlib, BL_LA(6), LIB_screenslib, 0x1380, LIB_bitmaplib, 0x1085,
           LIB_intuifontlib, BL_LA(1), LIB_screenslib, 0x0604);
BLITZ_SUBS(BLITZ_AMIGAONLY(e__screen2), prescreen2, 0);
BLITZ_NAME_AT(_toke, "Screen",
    "[Screen#,Mode[,Title$]]|[Screen#,X,Y,W,H,D,Viewmode,Title$,Dpen,Bpen[,BitMap#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_intuitionlib, BL_LA(6), LIB_screenslib, 0x1380, LIB_screenslib, 0x0604);
BLITZ_SUBS(e_dowbscreen, 0, 0);
BLITZ_NAME("WbToScreen", "Screen#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_screenslib, 0x1080);
BLITZ_SUBS(BLITZ_AMIGAONLY(e_doloadscreen), loadscreenchk, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_screenslib, 0x1080, LIB_palettelib, 0x1182);
BLITZ_SUBS(BLITZ_AMIGAONLY(e_doloadscreen2), loadscreenchk, 0);
BLITZ_NAME("LoadScreen", "Screen#,Filename$[,Palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_intuitionlib, BL_LA(6), LIB_screenslib, 0x1380, LIB_screenslib, 0x0604);
BLITZ_SUBS(e_dofstoscreen, 0, 0);
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_intuitionlib, BL_LA(6), LIB_screenslib, 0x1380, LIB_screenslib, 0x0604);
BLITZ_SUBS(e_dofindscreen, 0, 0);
BLITZ_NAME("FindScreen", "Screen#[,Title$]");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_screenslib, 0x1002);
BLITZ_SUBS(dosmousex, loadscreenchk, 0);
BLITZ_NAME("SMouseX", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_screenslib, 0x1002);
BLITZ_SUBS(dosmousey, loadscreenchk, 0);
BLITZ_NAME("SMouseY", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_screenslib, 0x1080, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(doshowscreen, loadscreenchk, 0);
BLITZ_NAME("ShowScreen", "Screen#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doscreenpens2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doscreenpens, 0, 0);
BLITZ_NAME("ScreenPens", "Text,Shine,Shadow,Fill,FillText,Background,HighLight");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_screenslib, 0x1080);
BLITZ_SUBS(dosviewport, loadscreenchk, 0);
BLITZ_NAME("ViewPort", "(Screen#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS(LIB_graphicslib, BL_LA(6), LIB_intuitionlib, BL_LA(3), LIB_screenslib, 0x1102);
BLITZ_SUBS(e_doshowbitmap0, showbmchk0, 0);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_graphicslib, BL_LA(6), LIB_intuitionlib, BL_LA(3), LIB_bitmaplib, 0x1080,
           LIB_screenslib, 0x1102);
BLITZ_SUBS(e_doshowbitmap1, showbmchk, 0);
BLITZ_NAME("ShowBitMap", "[BitMap#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_screenslib, 0x1380, LIB_intuitionlib, BL_LA(6), LIB_graphicslib, BL_LA(2));
BLITZ_SUBS(freescreen, 0, 0);
BLITZ_NAME("CloseScreen", "Screen#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_screenslib, 0x1080, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(dohidescreen, loadscreenchk, 0);
BLITZ_NAME("HideScreen", "Screen#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_screenslib, 0x1080, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(dobeepscreen, loadscreenchk, 0);
BLITZ_NAME("BeepScreen", "Screen#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_screenslib, 0x1080, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(domovescreen, loadscreenchk, 0);
BLITZ_NAME("MoveScreen", "Screen#,deltax,deltay");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_intuitionlib, BL_LA(6), LIB_screenslib, 0x1380, LIB_intuifontlib, BL_LA(1),
           LIB_screenslib, 0x0604);
BLITZ_SUBS(BLITZ_AMIGAONLY(e_doscreentags2), 0, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_LONG);
BLITZ_LIBS(LIB_intuitionlib, BL_LA(6), LIB_screenslib, 0x1380, LIB_intuifontlib, BL_LA(1),
           LIB_screenslib, 0x0604);
BLITZ_SUBS(BLITZ_AMIGAONLY(e_doscreentags3), 0, 0);
BLITZ_REPARGS(2, 2, BL_WORD, BL_STRING, BL_LONG | BL_PUSH, BL_LONG | BL_PUSH);
BLITZ_LIBS(LIB_intuitionlib, BL_LA(6), LIB_screenslib, 0x1380, LIB_intuifontlib, BL_LA(1),
           LIB_screenslib, 0x0604);
BLITZ_SUBS(BLITZ_AMIGAONLY(e_doscreentags), 0, 0);
BLITZ_NAME("ScreenTags", "Screen#,Title$ [&TagList] or [[,Tag,Data]...]");

BLITZ_NULLSUB_AT(init, doinit, 0, 0);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freescreen, 0, 0, LIB_intuitionlib, BL_LA(6), LIB_graphicslib, BL_LA(2));

BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 3);

/* ------------------------------------------------------------ variables */

/* ExtNewScreen, its tags and the pens, in the order of the original */
static struct BL_PACKED {
    UWORD romrevision;
    struct ExtNewScreen ns;                     /* .Extension: Dc.l extnewscreen */
    struct {                                    /* extnewscreen */
        struct TagItem displayid;               /* SA_DisplayID, newmodes */
        struct TagItem pens;                    /* SA_Pens */
        ULONG end;                              /* TAG_END */
    } tags;
    UWORD pens[13];
    UWORD defstype;
} sd = {
    .ns = { .Type = 0x1000, .Extension = (struct TagItem *)&sd.tags },
    .tags = { { SA_DisplayID, 0 }, { SA_Pens, (ULONG)&sd.pens }, TAG_END },
    .pens = { 2, 1, 1, 2, 1, 3, 1, 0, 2, 1, 2, 1, 0xffff },     /* defaults (WB) */
    .defstype = 0x1000,
};
static_assert(sizeof sd == 2 + 36 + 20 + 26 + 2, "sd layout");

/* a screen object, 8 bytes */
struct scrobj {
    struct Screen *screen;                      /* +0 */
    struct DBufInfo *dbuf;                      /* +4 ShowBitMap on V39+ */
};
static_assert(sizeof(struct scrobj) == 8, "scrobj size");

/* dummydata: the a2 stack of Screen n,mode: title length, d, h, w, y, x */
static struct BL_PACKED {
    ULONG len;
    UWORD depth, height, width;
    UWORD y, x;
} dummy;
#define DUMMYF ((const UBYTE *)&dummy + sizeof(dummy))

/* ScreenTags */
static LONG errcode = -1;
static struct {
    struct TagItem title, font, error;
    ULONG usertags[32];                         /* tag/data pairs, TAG_END */
} tl = {
    { SA_Title, 0 }, { SA_Font, 0 }, { SA_ErrorCode, (ULONG)&errcode }, { 0 },
};
static_assert(sizeof tl == 3 * 8 + 32 * 4, "tl layout");

/* messages of the runtime errors in the normal code (fixed, #251: in the
   original they lie in the error area) */
static const char mess1[] = "Can't open Screen";
static const char mess8[] = "Can't find WorkBench";
static const char mess12[] = "Can't find Screen";
static const char mess13[] = "Can't find CopIns!";

/* messages in the error area (BLITZ_ERRCHK_MESSAGE at the end) */
extern const char mess2[], mess3[], mess5[], mess6[], mess7[];
extern const char mess9[], mess10[], mess11[], mess14[];

static UWORD execversion(void)
{
    struct bl_regs r = { 0 };

    return BLITZ_ALIBJSR(TOK_execversion, r);
}

static ULONG allocst(const UBYTE *s, ULONG len)
{
    struct bl_regs r = { .d0 = (ULONG)s, .d1 = len };

    return BLITZ_ALIBJSR(TOK_allocst, r);
}

/* ------------------------------------------------------------ init */

/**
 * @brief Library init: ScreenPens on (NS_EXTENDED), the default pens of
 * the Workbench, the Exec version (environlib ExecVersion) in romrevision.
 */
static void BLITZ_ROUTINE doinit_c(void)
{
    static const UWORD defpens[10] = { 1, 2, 1, 3, 1, 0, 2, 1, 2, 1 };
    UWORD i;

    sd.defstype = 0x1000;
    for (i = 0; i < 10; i++)                    /* MOVE.l #...,pens+4 ... */
        sd.pens[2 + i] = defpens[i];
    sd.romrevision = execversion();
}
BLITZ_THUNK(doinit, doinit_c, "d1/a0-a1");

/* --------------------------------------------------------- ScreenPens */

/**
 * @brief ScreenPens On|Off - use the ScreenPens for the following Screens.
 *
 * Sets or clears the NS_EXTENDED flag ($1000) of the screen type, so that
 * OpenScreen reads the tag list with SA_Pens and SA_DisplayID.
 *
 * @param on d0.w: <> 0 = on
 */
static void BLITZ_ROUTINE doscreenpens2_c(UWORD on BL_REG(d0))
{
    sd.defstype = on ? 0x1000 : 0;
}
BLITZ_THUNK(doscreenpens2, doscreenpens2_c, "d0-d1/a0-a1");

/**
 * @brief ScreenPens Text,Shine,Shadow,Fill,FillText,Background,HighLight -
 * the DrawInfo pens of the following Screens (pens 2-8 of SA_Pens).
 *
 * MOVEM d0-d5,pens+4:MOVE -(a2),pens+16; the NS_EXTENDED flag is not
 * changed.
 *
 * @param p0   d0.w: TEXTPEN
 * @param p1   d1.w: SHINEPEN
 * @param p2   d2.w: SHADOWPEN
 * @param p3   d3.w: FILLPEN
 * @param p4   d4.w: FILLTEXTPEN
 * @param p5   d5.w: BACKGROUNDPEN
 * @param args a2: the argument stack, -2(a2) = HIGHLIGHTTEXTPEN
 */
static void BLITZ_ROUTINE doscreenpens_c(UWORD p0 BL_REG(d0), UWORD p1 BL_REG(d1),
                                         UWORD p2 BL_REG(d2), UWORD p3 BL_REG(d3),
                                         UWORD p4 BL_REG(d4), UWORD p5 BL_REG(d5),
                                         const UBYTE *args BL_REG(a2))
{
    const UBYTE *sp = args;

    sd.pens[2] = p0;
    sd.pens[3] = p1;
    sd.pens[4] = p2;
    sd.pens[5] = p3;
    sd.pens[6] = p4;
    sd.pens[7] = p5;
    sd.pens[8] = BL_POPW(sp);
}
BLITZ_THUNK(doscreenpens, doscreenpens_c, "d0-d1/a1");

/* ------------------------------------------------------- inline routines */

/**
 * @brief ViewPort(Screen#) - the address of the screen's ViewPort (inline,
 * a0 = the screen object; result d0 = screen + 44).
 *
 * Errors: "Uninitialized Screen" (loadscreenchk, debugger only).
 */
BLITZ_INLINE(dosviewport,
    "   move.l  (a0),a0\n"
    "   lea     44(a0),a0\n"
    "   move.l  a0,d0");
/**
 * @brief ShowScreen Screen# - ScreenToFront (inline, a0 = the screen
 * object, a6 = intuition).
 *
 * Errors: "Uninitialized Screen" (loadscreenchk, debugger only).
 */
BLITZ_INLINE(doshowscreen, "   move.l  (a0),a0\n   jsr     -252(a6)");    /* ScreenToFront */
/**
 * @brief HideScreen Screen# - ScreenToBack (inline, a0 = the screen
 * object, a6 = intuition).
 *
 * Errors: "Uninitialized Screen" (loadscreenchk, debugger only).
 */
BLITZ_INLINE(dohidescreen, "   move.l  (a0),a0\n   jsr     -246(a6)");    /* ScreenToBack */
/**
 * @brief BeepScreen Screen# - DisplayBeep (inline, a0 = the screen
 * object, a6 = intuition).
 *
 * Errors: "Uninitialized Screen" (loadscreenchk, debugger only).
 */
BLITZ_INLINE(dobeepscreen, "   move.l  (a0),a0\n   jsr     -96(a6)");     /* DisplayBeep */
/**
 * @brief MoveScreen Screen#,deltax,deltay - intuition MoveScreen (inline,
 * a0 = the screen object, d1.w/d2.w = the deltas, a6 = intuition).
 *
 * Errors: "Uninitialized Screen" (loadscreenchk, debugger only).
 */
BLITZ_INLINE(domovescreen,
    "   move.l  (a0),a0\n"
    "   move.w  d1,d0\n"
    "   move.w  d2,d1\n"
    "   jsr     -162(a6)");                                                /* MoveScreen */
/**
 * @brief SMouseX - the mouse x position on the used screen (inline, a0 =
 * the used screen object; d0.w = Screen.MouseX).
 *
 * Errors: "Uninitialized Screen" (loadscreenchk, debugger only).
 */
BLITZ_INLINE(dosmousex, "   move.l  (a0),a0\n   move.w  18(a0),d0");
/**
 * @brief SMouseY - the mouse y position on the used screen (inline, a0 =
 * the used screen object; d0.w = Screen.MouseY).
 *
 * Errors: "Uninitialized Screen" (loadscreenchk, debugger only).
 */
BLITZ_INLINE(dosmousey, "   move.l  (a0),a0\n   move.w  16(a0),d0");

/* ---------------------------------------------------------- freescreen */

/* freescreen - a3 = object, a6 = intuition, a2 = graphics */
static void freescreen_(struct scrobj *obj, struct IntuitionBase *IntuitionBase,
                        struct GfxBase *GfxBase)
{
    struct Screen *s = obj->screen;
    struct bl_regs r = { 0 };

    if (!s)
        return;
    if ((ULONG)s->UserData == BLTZ) {           /* don't close non Blitzs */
        r.d0 = (ULONG)s->Title;                 /* title copy */
        BLITZ_ALIBJSR(TOK_freest, r);
        CloseScreen(obj->screen);
        if (obj->dbuf) {
            FreeDBufInfo(obj->dbuf);
            obj->dbuf = NULL;
        }
    }
    obj->screen = NULL;
}

/**
 * @brief CloseScreen Screen# and the object's free routine (_free).
 *
 * a3 = the screen object.  Only screens the library opened ("bLtZ" in
 * UserData) are closed (the title copy freed with maxslib $CF02, the
 * DBufInfo with FreeDBufInfo); screens of WbToScreen/FindScreen are only
 * forgotten.
 *
 * @param intui a6: intuition.library
 * @param gfx   a2: graphics.library
 */
static void BLITZ_ROUTINE freescreen_c(struct IntuitionBase *intui BL_REG(a6),
                                       struct GfxBase *gfx BL_REG(a2))
{
    freescreen_((struct scrobj *)blitz_string, intui, gfx);
}
BLITZ_THUNK(freescreen, freescreen_c, "d1/a0-a1");

/* -------------------------------------------------------------- Screen */

/* _screen2 - open the screen.  a3 = object, a0 = bitmap (or 0), a1 =
   intuifontlib's &textattr, d1 = viewmode, d2 = title, d3/d4 = pens,
   a2 = stack x,y,w,h,d (words), title length (long) */
static void openscreen(UBYTE *bitmap, ULONG *ifont, WORD used, ULONG viewmode,
                       const UBYTE *title, UBYTE dpen, UBYTE bpen, const UBYTE *args,
                       struct IntuitionBase *IntuitionBase)
{
    struct scrobj *obj = (struct scrobj *)blitz_string;
    const UBYTE *sp = args;
    struct Screen *scr;
    ULONG font = *ifont;                        /* MOVE.l (a1),d7 */
    UWORD w;

    *(struct scrobj **)(blitz_globals + used) = obj;   /* set used screen! */
    freescreen_(obj, IntuitionBase, (struct GfxBase *)args);   /* BUG(orig) (#247): a2 */
    sd.ns.Font = (struct TextAttr *)font;
    sd.ns.LeftEdge = BL_POPW(sp);
    sd.ns.TopEdge = BL_POPW(sp);
    w = BL_POPW(sp);
    sd.ns.Width = (w + 15) & 0xfff0;
    sd.ns.Height = BL_POPW(sp);
    sd.ns.Depth = BL_POPW(sp);
    sd.ns.ViewModes = viewmode;
    /* the whole long (fixed, #250: the original's MOVE d1,newmodes+2 only
       sets the low word) */
    sd.tags.displayid.ti_Data = viewmode;
    sd.ns.DetailPen = dpen;
    sd.ns.BlockPen = bpen;
    sd.ns.CustomBitMap = (struct BitMap *)bitmap;
    sd.ns.DefaultTitle = (STRPTR)allocst(title, BL_POPL(sp));
    sd.ns.Type = (sd.ns.CustomBitMap ? 0x4f : 15) | sd.defstype;
    scr = OpenScreen((struct NewScreen *)&sd.ns);
    obj->screen = scr;
    if (!scr) {
        BLITZ_RUNERR(mess1);                    /* err1, fixed (#251) */
        return;
    }
    scr->UserData = (APTR)BLTZ;
}

/**
 * @brief Screen Screen#,X,Y,W,H,D,Viewmode,Title$,Dpen,Bpen - open a screen
 * (_screen).
 *
 * a3 = the screen object, which becomes the used screen; its old screen
 * is freed first.  The width is rounded UP to a multiple of 16; the
 * ExtNewScreen gets SA_DisplayID (= viewmode) and SA_Pens (ScreenPens),
 * the current IntuiFont and the type CUSTOMSCREEN | the ScreenPens flag
 * NS_EXTENDED; the screen gets "bLtZ" in UserData.
 *
 * BUG(orig) (#247): the old screen is freed with a2 = the argument stack
 * as graphics base (FreeDBufInfo through garbage if it had a DBufInfo).
 * Fixed (#250): SA_DisplayID gets the whole viewmode (the original only
 * its low word).
 * Fixed (#251): "Can't open Screen" is raised in the normal code (the
 * original's `BEQ err1` goes into the error area).
 * Errors (prescreen, debugger only): "Uninitialized IntuiFont", width < 320,
 * height < 10, "Illegal number of BitPlanes".
 *
 * @param viewmode d1: the view mode
 * @param title    d2: the title
 * @param dpen     d3.b: DetailPen
 * @param bpen     d4.b: BlockPen
 * @param ifont    a1: intuifontlib's TextAttr pointer
 * @param used     d6.w: the offset of the used screen in the globals
 * @param args     a2: the argument stack: x, y, w, h, d (words), title
 *                 length (long), popped from the top
 * @param intui    a6: intuition.library
 */
void BLITZ_ROUTINE _screen(ULONG viewmode BL_REG(d1), const UBYTE *title BL_REG(d2),
                           UBYTE dpen BL_REG(d3), UBYTE bpen BL_REG(d4),
                           ULONG *ifont BL_REG(a1), WORD used BL_REG(d6),
                           const UBYTE *args BL_REG(a2), struct IntuitionBase *intui BL_REG(a6))
{
    openscreen(0, ifont, used, viewmode, title, dpen, bpen, args, intui);
}

/**
 * @brief Screen Screen#,X,Y,W,H,D,Viewmode,Title$,Dpen,Bpen,BitMap# - open a
 * screen on a bitmap (_screen2).
 *
 * As _screen with CUSTOMBITMAP and the bitmap as CustomBitMap.
 *
 * Errors (prescreen2, debugger only): those of _screen, "Uninitialized
 * BitMap", "Can't use specified BitMap" (smaller than the screen).
 *
 * @param bitmap a0: the bitmap object
 */
void BLITZ_ROUTINE _screen2(ULONG viewmode BL_REG(d1), const UBYTE *title BL_REG(d2),
                            UBYTE dpen BL_REG(d3), UBYTE bpen BL_REG(d4),
                            UBYTE *bitmap BL_REG(a0), ULONG *ifont BL_REG(a1),
                            WORD used BL_REG(d6), const UBYTE *args BL_REG(a2),
                            struct IntuitionBase *intui BL_REG(a6))
{
    openscreen(bitmap, ifont, used, viewmode, title, dpen, bpen, args, intui);
}

/* modec - Screen n,mode[,title$] through the fake stack: d0 = mode, d1 =
   title, d2 = its length, a0 = graphics base */
static void modec(UWORD mode, const UBYTE *title, ULONG len, const UBYTE *gfx,
                  ULONG *ifont, WORD used, struct IntuitionBase *intui)
{
    ULONG vm = 0;
    UWORD d, h, w = 320;

    dummy.len = len;
    d = mode & 7;
    if (!d)
        d = 8;
    dummy.depth = d;
    if (mode & 8) {                             /* hires */
        w = 640;
        vm = 0x8000;
    }
    dummy.width = w;
    h = (gfx[207] & 1) ? 200 : 256;             /* BTST #0,207(a0): NTSC */
    if (mode & 16) {                            /* interlace */
        h <<= 1;
        vm |= 4;
    }
    dummy.height = h;
    openscreen(0, ifont, used, vm, title, 1, 2, DUMMYF, intui);
}

/**
 * @brief Screen Screen#,Mode - open a screen from a mode number.
 *
 * depth = mode & 7 (0 = 8), bit 3 hires (640, HIRES), bit 4 interlace
 * (height * 2, LACE), height 200 (NTSC, bit 0 of GfxBase+207) or 256;
 * pens 1/2, no title.  Through a fake a2 stack (`dummydata`) into the long
 * form (_screen), with its notes.
 *
 * Errors: "Illegal Mode" before V39 (modechk, debugger only).
 *
 * @param mode  d1.w: the mode
 * @param gfx   a0: graphics.library
 * @param ifont a1: intuifontlib's TextAttr pointer
 * @param used  d6.w: the offset of the used screen in the globals
 * @param intui a6: intuition.library
 */
void BLITZ_ROUTINE domode(UWORD mode BL_REG(d1), const UBYTE *gfx BL_REG(a0),
                          ULONG *ifont BL_REG(a1), WORD used BL_REG(d6),
                          struct IntuitionBase *intui BL_REG(a6))
{
    modec(mode, 0, 0, gfx, ifont, used, intui);
}

/**
 * @brief Screen Screen#,Mode,Title$ - as domode with a title.
 *
 * @param title d2: the title
 * @param lens  a2: the argument stack, -4(a2) = the title's length
 */
void BLITZ_ROUTINE domode2(UWORD mode BL_REG(d1), const UBYTE *title BL_REG(d2),
                           const UBYTE *gfx BL_REG(a0), ULONG *ifont BL_REG(a1),
                           WORD used BL_REG(d6), const ULONG *lens BL_REG(a2),
                           struct IntuitionBase *intui BL_REG(a6))
{
    const ULONG *sp = lens;

    modec(mode, title, BL_POPL(sp), gfx, ifont, used, intui);
}

/* ------------------------------------------------ WbToScreen, FindScreen */

/**
 * @brief WbToScreen Screen# - the Workbench screen as a screen object.
 *
 * a3 = the screen object; the first screen with type WBENCHSCREEN, which
 * becomes the used screen.  It is never closed by the library.
 *
 * BUG(orig) (#248): the object's old screen is freed (closed if the
 * library opened it), (#247) with a2 = the argument stack as graphics
 * base.  Errors: "Can't find WorkBench" (err8, in the
 * normal code with its text; fixed, #251: the original's text is in the
 * error area).
 *
 * @param used  d6.w: the offset of the used screen in the globals
 * @param args  a2: the argument stack (passed as graphics base, see above)
 * @param intui a6: intuition.library
 */
void BLITZ_ROUTINE dowbscreen(WORD used BL_REG(d6), const UBYTE *args BL_REG(a2),
                              struct IntuitionBase *intui BL_REG(a6))
{
    struct scrobj *obj = (struct scrobj *)blitz_string;
    struct Screen *s = intui->FirstScreen;

    freescreen_(obj, intui, (struct GfxBase *)args);           /* BUG(orig) (#247, #248): a2 */
    for (;; s = s->NextScreen) {
        if (!s) {
            BLITZ_RUNERR(mess8);                /* err8 */
            return;
        }
        if ((s->Flags & 15) == WBENCHSCREEN)
            break;
    }
    obj->screen = s;                            /* Got WorkBench */
    *(struct scrobj **)(blitz_globals + used) = obj;
}

/* title / default title starting with pfx? */
static int prefix(const UBYTE *title, const UBYTE *pfx)
{
    while (*pfx)
        if (*title++ != *pfx++)                 /* CMPM.b (a1)+,(a2)+ */
            return 0;
    return 1;
}

static void findscreen(const UBYTE *title, WORD used, const UBYTE *args,
                       struct IntuitionBase *intui)
{
    struct scrobj *obj = (struct scrobj *)blitz_string;
    /* the list starts at +60: &FirstScreen taken as a screen's NextScreen */
    struct Screen *s = (struct Screen *)&intui->FirstScreen;

    *(struct scrobj **)(blitz_globals + used) = obj;
    freescreen_(obj, intui, (struct GfxBase *)args);           /* BUG(orig) (#247, #248): a2 */
    for (;;) {
        s = s->NextScreen;
        if (!s) {
            BLITZ_RUNERR(mess12);               /* err12 */
            return;
        }
        if (!title)
            break;
        if (!s->Title)                          /* no title: next screen */
            continue;
        if (prefix(s->Title, title))
            break;
        if (!s->DefaultTitle)
            continue;
        if (prefix(s->DefaultTitle, title))
            break;
    }
    obj->screen = s;
}

/**
 * @brief FindScreen Screen# - the frontmost screen as a screen object.
 *
 * a3 = the screen object, which becomes the used screen.  Notes and errors
 * as dofindscreen.
 *
 * @param used  d6.w: the offset of the used screen in the globals
 * @param args  a2: the argument stack (passed as graphics base, #247)
 * @param intui a6: intuition.library
 */
void BLITZ_ROUTINE dofstoscreen(WORD used BL_REG(d6), const UBYTE *args BL_REG(a2),
                                struct IntuitionBase *intui BL_REG(a6))
{
    findscreen(0, used, args, intui);           /* no title */
}

/**
 * @brief FindScreen Screen#,Title$ - the first screen whose title or
 * default title starts with Title$, as a screen object.
 *
 * a3 = the screen object, which becomes the used screen.  The screen is
 * never closed by the library.
 *
 * BUG(orig) (#248): the object's old screen is freed (closed if the
 * library opened it), (#247) with a2 = the argument stack as graphics
 * base.  Errors: "Can't find Screen" (err12, in the
 * normal code with its text; fixed, #251: the original's text is in the
 * error area).
 *
 * @param title d1: the title prefix
 * @param used  d6.w: the offset of the used screen in the globals
 * @param args  a2: the argument stack (passed as graphics base, see above)
 * @param intui a6: intuition.library
 */
void BLITZ_ROUTINE dofindscreen(const UBYTE *title BL_REG(d1), WORD used BL_REG(d6),
                                const UBYTE *args BL_REG(a2),
                                struct IntuitionBase *intui BL_REG(a6))
{
    findscreen(title, used, args, intui);
}

/* ---------------------------------------------------------- LoadScreen */

static ULONG loadscreen(struct scrobj *obj, const UBYTE *name)
{
    struct bl_regs r = { 0 };

    r.a1 = (ULONG)obj->screen->RastPort.BitMap;
    r.a0 = (ULONG)name;
    return BLITZ_ALIBJSR(TOK_ilbmbitmap, r);
}

/**
 * @brief LoadScreen Screen#,Filename$ - load an ILBM into the screen's
 * bitmap (ilbmifflib ilbmbitmap; its result is not checked here).
 *
 * Errors: "Uninitialized Screen" (loadscreenchk, debugger only).
 *
 * @param obj  a0: the screen object
 * @param name d1: the file name
 */
void BLITZ_ROUTINE doloadscreen(struct scrobj *obj BL_REG(a0), const UBYTE *name BL_REG(d1))
{
    loadscreen(obj, name);
}

/**
 * @brief LoadScreen Screen#,Filename$,Palette# - load the file's palette
 * (palettelib getpal2) and the ILBM into the screen's bitmap.
 *
 * Errors: "Uninitialized Screen" (loadscreenchk, debugger only).
 *
 * @param obj  a0: the screen object
 * @param name d1: the file name
 * @param pal  a1: the palette object
 */
void BLITZ_ROUTINE doloadscreen2(struct scrobj *obj BL_REG(a0), const UBYTE *name BL_REG(d1),
                                 ULONG pal BL_REG(a1))
{
    struct bl_regs r = { .d0 = 0, .a0 = (ULONG)name, .a1 = pal };

    BLITZ_ALIBJSR(TOK_getpal2, r);
    loadscreen(obj, name);
}

/* ---------------------------------------------------------- ShowBitMap */


/* doshowbitmap2 - MrgCop/LoadView of intuition's View, interrupts on */
static void showbitmap2(struct IntuitionBase *intui, struct GfxBase *GfxBase)
{
    MrgCop(&intui->ViewLord);
    LoadView(&intui->ViewLord);
    BL_INTS_ON();
}

/* intshowbitmap - interlace: planes into the screen's BitMap, MakeVPort */
static void intshowbitmap(const struct bb_bitmap *bm, struct Screen *scr,
                          struct IntuitionBase *intui,
                          struct GfxBase *GfxBase)
{
    ULONG *src = (ULONG *)bm->planes, *dst = (ULONG *)scr->BitMap.Planes;
    UWORD n = bm->flagsdepth - 1;

    do
        *dst++ = *src++;
    while (n-- != 0);                           /* DBF d0 */
    MakeVPort(&intui->ViewLord, &scr->ViewPort);
    showbitmap2(intui, GfxBase);
}

/* agashowbitmap - V39+: ChangeVPBitMap, DBufInfo allocated once */
static void agashowbitmap(const struct bb_bitmap *bm, struct scrobj *obj,
                          struct GfxBase *GfxBase)
{
    struct Screen *scr = obj->screen;
    struct DBufInfo *db = obj->dbuf;

    if (!db) {
        db = AllocDBufInfo(&scr->ViewPort);
        obj->dbuf = db;
    }
    ChangeVPBitMap(&scr->ViewPort, (struct BitMap *)bm, db);
}

/* doshowbitmap - a0 = bitmap, a1 = screen object, a3 = intuition, a6 =
   graphics.  Before V39: the BPLxPT moves of the screen's copper list
   (first $0000F0E0 = MOVE to bpl1pth) get the plane pointers. */
static void showbitmap(const struct bb_bitmap *bm, struct scrobj *obj,
                       struct GfxBase *GfxBase)
{
    struct IntuitionBase *intui = (struct IntuitionBase *)blitz_string;   /* a3 */
    struct Screen *scr;
    UBYTE *cl, *ins;
    const UWORD *src;
    UWORD n, nwords;

    if ((WORD)(execversion() - 39) >= 0) {      /* CMP #39,d0:BPL */
        agashowbitmap(bm, obj, GfxBase);
        return;
    }
    BL_INTS_OFF();
    scr = obj->screen;
    if (scr->ViewPort.Modes & LACE) {           /* interlace! */
        intshowbitmap(bm, scr, intui, GfxBase);
        return;
    }
    if ((WORD)execversion() >= 39) {
        intshowbitmap(bm, scr, intui, GfxBase);
        return;
    }
    cl = (UBYTE *)&scr->ViewPort.DspIns;        /* CopList.Next is at +0 */
    for (;;) {                                  /* 'loop0 */
        cl = *(UBYTE **)cl;
        if (!cl) {
            BLITZ_RUNERR(mess13);               /* err13 */
            return;
        }
        n = *(UWORD *)(cl + 28);
        if (!n)
            continue;
        ins = *(UBYTE **)(cl + 12) - 6;
        n--;
        do {                                    /* CMP.l (a2),d1:DBEQ d0 */
            ins += 6;
            if (*(ULONG *)ins == 0xf0e0)
                goto found;
        } while (n-- != 0);
    }
found:
    nwords = bm->flagsdepth * 2;
    src = (const UWORD *)bm->planes;
    for (;;) {
        do {                                    /* 'loop */
            if (*(UWORD *)ins == 2)
                goto more;
            *(UWORD *)(ins + 4) = *src++;
            ins += 6;
            if (!--nwords) {                    /* SUBQ #1,d1:DBEQ:BEQ */
                showbitmap2(intui, GfxBase);
                return;
            }
        } while (n-- != 0);
more:
        do {                                    /* 'more: next list */
            cl = *(UBYTE **)cl;
            if (!cl) {
                BLITZ_RUNERR(mess13);
                return;
            }
            n = *(UWORD *)(cl + 28);
        } while (!n);
        ins = *(UBYTE **)(cl + 12);
        n--;
    }
}

/**
 * @brief ShowBitMap - show the used screen's own bitmap again.
 *
 * As doshowbitmap1 with the screen's BitMap.
 *
 * Errors: "Uninitialized Screen" (showbmchk0, debugger only).
 *
 * @param obj a1: the used screen object
 * @param gfx a6: graphics.library
 */
void BLITZ_ROUTINE doshowbitmap0(struct scrobj *obj BL_REG(a1), struct GfxBase *gfx BL_REG(a6))
{
    /* a struct BitMap: showbitmap reads only that part of a bitmap object */
    showbitmap((const struct bb_bitmap *)&obj->screen->BitMap, obj, gfx);
}

/**
 * @brief ShowBitMap BitMap# - show a bitmap in the used screen.
 *
 * V39+ (environlib ExecVersion): ChangeVPBitMap, the DBufInfo allocated
 * once per object.  Before: the plane pointers into the BPLxPT moves of
 * the screen's copper list or, for interlace, into the screen's BitMap +
 * MakeVPort, then MrgCop/LoadView with interrupts off.  a3 = intuition.
 *
 * Errors: "Can't find CopIns!" (err13, in the normal code with its text,
 * before V39; fixed, #251);
 * debugger only (showbmchk): "Uninitialized Screen", "Uninitialized
 * BitMap", "Screen and BitMap MUST be same size".
 *
 * @param bm  a0: the bitmap object
 * @param obj a1: the used screen object
 * @param gfx a6: graphics.library
 */
void BLITZ_ROUTINE doshowbitmap1(const struct bb_bitmap *bm BL_REG(a0),
                                 struct scrobj *obj BL_REG(a1),
                                 struct GfxBase *gfx BL_REG(a6))
{
    showbitmap(bm, obj, gfx);
}

/* ---------------------------------------------------------- ScreenTags */

/* duntags - OpenScreenTagList (V36+), result screen or the error code */
static ULONG duntags(struct scrobj *obj, struct IntuitionBase *IntuitionBase)
{
    struct Screen *scr;

    if ((WORD)(execversion() - 36) < 0)         /* CMP #36,d0:BMI */
        return errcode;
    scr = OpenScreenTagList(NULL, &tl.title);
    obj->screen = scr;
    if (!scr)
        return errcode;
    scr->UserData = (APTR)BLTZ;
    return (ULONG)scr;
}

/* common start: used, title copy, font, free the old screen */
static const ULONG *tagstart(const UBYTE *title, const ULONG *lens, ULONG *ifont,
                             WORD used, struct IntuitionBase *intui)
{
    struct scrobj *obj = (struct scrobj *)blitz_string;
    const ULONG *sp = lens;

    *(struct scrobj **)(blitz_globals + used) = obj;   /* make used */
    tl.title.ti_Data = allocst(title, BL_POPL(sp));
    tl.font.ti_Data = *ifont;
    freescreen_(obj, intui, (struct GfxBase *)sp);  /* BUG(orig) (#247): a2 */
    return sp;
}

/**
 * @brief ScreenTags(Screen#,Title$,&TagList) - open a screen with a tag list.
 *
 * a3 = the screen object, which becomes the used screen; its old screen
 * is freed first.  OpenScreenTagList (V36+) with SA_Title (a copy),
 * SA_Font (the IntuiFont), SA_ErrorCode and the user tags, copied up to
 * TAG_END; the screen gets "bLtZ" in UserData.
 *
 * BUG(orig) (#249): the tag list is copied without a limit into 32 longs.
 * BUG(orig) (#247): the old screen is freed with a2 as graphics base.
 *
 * @param title d1: the title
 * @param tags  d2: the tag list
 * @param ifont a1: intuifontlib's TextAttr pointer
 * @param used  d6.w: the offset of the used screen in the globals
 * @param lens  a2: the argument stack, -4(a2) = the title's length
 * @param intui a6: intuition.library
 * @return d0: the screen, or the error code (SA_ErrorCode; -1 if none was
 *         set, also before V36)
 */
ULONG BLITZ_ROUTINE doscreentags3(const UBYTE *title BL_REG(d1), const ULONG *tags BL_REG(d2),
                                  ULONG *ifont BL_REG(a1), WORD used BL_REG(d6),
                                  const ULONG *lens BL_REG(a2),
                                  struct IntuitionBase *intui BL_REG(a6))
{
    ULONG *dst = tl.usertags, tag;

    tagstart(title, lens, ifont, used, intui);
    do {                                        /* nxtag: MOVEM.l (a2)+,d0-d1 */
        tag = *tags++;
        dst[0] = tag;
        dst[1] = *tags++;
        dst += 2;
    } while (tag);
    return duntags((struct scrobj *)blitz_string, intui);
}

/**
 * @brief ScreenTags(Screen#,Title$,Tag,Data...) - open a screen with tags
 * given as arguments.
 *
 * As doscreentags3, the tag/data pairs from the argument stack.
 *
 * @param n     d7.w: the number of tag/data pairs on the a2 stack
 * @param title d1: the title
 * @param ifont a1: intuifontlib's TextAttr pointer
 * @param used  d6.w: the offset of the used screen in the globals
 * @param lens  a2: the argument stack: the title's length, then the pairs
 * @param intui a6: intuition.library
 * @return d0: the screen, or the error code (-1 if none was set)
 */
ULONG BLITZ_ROUTINE doscreentags(UWORD n BL_REG(d7), const UBYTE *title BL_REG(d1),
                                 ULONG *ifont BL_REG(a1), WORD used BL_REG(d6),
                                 const ULONG *lens BL_REG(a2),
                                 struct IntuitionBase *intui BL_REG(a6))
{
    const ULONG *sp = tagstart(title, lens, ifont, used, intui);
    ULONG *dst = tl.usertags;

    for (; n; n--) {                            /* gettags: DBRA d7,tagloop */
        dst[0] = BL_POPL(sp);
        dst[1] = BL_POPL(sp);
        dst += 2;
    }
    *dst = 0;                                   /* MOVE.l #0,(a0)+ */
    return duntags((struct scrobj *)blitz_string, intui);
}

/**
 * @brief ScreenTags(Screen#,Title$) - as doscreentags without user tags.
 *
 * @return d0: the screen, or the error code (-1 if none was set)
 */
ULONG BLITZ_ROUTINE doscreentags2(const UBYTE *title BL_REG(d1), ULONG *ifont BL_REG(a1),
                                  WORD used BL_REG(d6), const ULONG *lens BL_REG(a2),
                                  struct IntuitionBase *intui BL_REG(a6))
{
    return doscreentags(0, title, ifont, used, lens, intui);   /* no tags */
}

/* ------------------------------------------- error checks (debugger only) */

/* prescreen - IntuiFont, width >= 320, height >= 10, depth */
static void BLITZ_ERRCHK prescreen_c(ULONG vm BL_REG(d1), const UBYTE *ifont BL_REG(a1),
                                     const UBYTE *args BL_REG(a2))
{
    WORD d;

    if (!*(ULONG *)(ifont + 8))
        BLITZ_RUNERR(mess10);
    if (*(WORD *)(args - 6) < 320)
        BLITZ_RUNERR(mess5);
    if (*(WORD *)(args - 8) < 10)
        BLITZ_RUNERR(mess6);
    d = *(WORD *)(args - 10);
    if ((WORD)execversion() >= 39) {
        if (d <= 0 || d > 8)
            BLITZ_RUNERR(mess9);
        return;
    }
    if (d < 0 || d > 6)
        BLITZ_RUNERR(mess9);
    if ((vm & 0x8000) && d > 4)
        BLITZ_RUNERR(mess9);
}

/* prescreen2 - prescreen and the bitmap at least as big as the screen */
static void BLITZ_ERRCHK prescreen2_c(ULONG vm BL_REG(d1), const struct bb_bitmap *bm BL_REG(a0),
                                      const UBYTE *ifont BL_REG(a1),
                                      const UBYTE *args BL_REG(a2))
{
    prescreen_c(vm, ifont, args);
    if (!bm->bpr)
        BLITZ_RUNERR(mess2);
    if ((UWORD)(bm->bpr << 3) < *(UWORD *)(args - 6)
        || bm->rows < *(UWORD *)(args - 8) || bm->flagsdepth < *(UWORD *)(args - 10))
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK loadscreenchk_c(const struct scrobj *obj BL_REG(a0))
{
    if (!obj->screen)
        BLITZ_RUNERR(mess7);
}

/* modechk - before V39: 1-6 planes, hires at most 4 */
static void BLITZ_ERRCHK modechk_c(UWORD mode BL_REG(d1))
{
    UWORD d;

    if ((WORD)execversion() >= 39)
        return;
    d = mode & 7;
    if (!d || d > 6)
        BLITZ_RUNERR(mess11);
    if ((mode & 8) && d > 4)
        BLITZ_RUNERR(mess11);
}

static void BLITZ_ERRCHK showbmchk0_c(const struct scrobj *obj BL_REG(a1))
{
    if (!obj->screen)
        BLITZ_RUNERR(mess7);
}

/* showbmchk - bitmap of the same size as the screen's */
static void BLITZ_ERRCHK showbmchk_c(const struct bb_bitmap *bm BL_REG(a0),
                                     const struct scrobj *obj BL_REG(a1))
{
    const UBYTE *s;

    showbmchk0_c(obj);
    if (!bm->bpr)
        BLITZ_RUNERR(mess2);
    s = (const UBYTE *)&obj->screen->BitMap;
    if (bm->bpr != *(UWORD *)s || bm->rows != *(UWORD *)(s + 2)
        || bm->depth != s[5])
        BLITZ_RUNERR(mess14);
}

/* the thunks and messages last: they leave the assembler in .text while
   GCC still assumes the error check section */
BLITZ_ERRCHK_THUNK(prescreen, prescreen_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(prescreen2, prescreen2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(loadscreenchk, loadscreenchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(modechk, modechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(showbmchk0, showbmchk0_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(showbmchk, showbmchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess3, "Can't use specified BitMap");
BLITZ_ERRCHK_MESSAGE(mess5, "Screen width MUST be at least 320");
BLITZ_ERRCHK_MESSAGE(mess6, "Screen Height MUST be at least 10");
BLITZ_ERRCHK_MESSAGE(mess7, "Uninitialized Screen");
BLITZ_ERRCHK_MESSAGE(mess9, "Illegal number of BitPlanes");
BLITZ_ERRCHK_MESSAGE(mess10, "Uninitialized IntuiFont");
BLITZ_ERRCHK_MESSAGE(mess11, "Illegal Mode");
BLITZ_ERRCHK_MESSAGE(mess14, "Screen and BitMap MUST be same size");
