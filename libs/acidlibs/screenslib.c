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
 *   - SA_DisplayID gets only the low word of the viewmode (MOVE
 *     d1,newmodes+2), the upper word stays 0. (#250)
 *   - Can't open Screen: `BEQ err1` from the normal code into the error
 *     area (without the debugger a jump behind the library); the C
 *     library calls err1 in the error area in the same way, but per JSR
 *     (DIFF, T: one return address more on the stack at the trap, as in
 *     animlib/displaylib).  err8/12/13
 *     (WorkBench / Screen / CopIns not found) are in the normal code with
 *     their texts in the error area, as in the original. (#251)
 * DIFF: a2 stays unchanged (the original pops the arguments).
 */
#include "blitzlib.h"
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
    struct BL_PACKED {
        WORD left, top, width, height, depth;
        UBYTE dpen, bpen;
        UWORD viewmodes, type;
        ULONG font, title, gadgets, bitmap;
        ULONG ext;                              /* Dc.l extnewscreen */
    } ns;
    ULONG tags[5];                              /* extnewscreen */
    UWORD pens[13];
    UWORD defstype;
} sd = {
    .ns = { .type = 0x1000, .ext = (ULONG)&sd.tags },
    .tags = { 0x80000032, 0, 0x8000003a, (ULONG)&sd.pens, 0 },   /* DisplayID, Pens */
    .pens = { 2, 1, 1, 2, 1, 3, 1, 0, 2, 1, 2, 1, 0xffff },     /* defaults (WB) */
    .defstype = 0x1000,
};

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
    ULONG title[2], font[2], error[2];
    ULONG usertags[32];
} tl = {
    { 0x80000028, 0 }, { 0x8000002b, 0 }, { 0x8000002a, (ULONG)&errcode }, { 0 },
};

/* messages in the error area (BLITZ_ERRCHK_MESSAGE at the end) */
extern const char mess1[], mess2[], mess3[], mess5[], mess6[], mess7[], mess8[];
extern const char mess9[], mess10[], mess11[], mess12[], mess13[], mess14[];
static void err1(void);                         /* in the error area */

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

static void BLITZ_ROUTINE doscreenpens2_c(UWORD on BL_REG(d0))
{
    sd.defstype = on ? 0x1000 : 0;
}
BLITZ_THUNK(doscreenpens2, doscreenpens2_c, "d0-d1/a0-a1");

/* MOVEM d0-d5,pens+4:MOVE -(a2),pens+16 */
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

BLITZ_INLINE(dosviewport,
    "   move.l  (a0),a0\n"
    "   lea     44(a0),a0\n"
    "   move.l  a0,d0");
BLITZ_INLINE(doshowscreen, "   move.l  (a0),a0\n   jsr     -252(a6)");    /* ScreenToFront */
BLITZ_INLINE(dohidescreen, "   move.l  (a0),a0\n   jsr     -246(a6)");    /* ScreenToBack */
BLITZ_INLINE(dobeepscreen, "   move.l  (a0),a0\n   jsr     -96(a6)");     /* DisplayBeep */
BLITZ_INLINE(domovescreen,
    "   move.l  (a0),a0\n"
    "   move.w  d1,d0\n"
    "   move.w  d2,d1\n"
    "   jsr     -162(a6)");                                                /* MoveScreen */
BLITZ_INLINE(dosmousex, "   move.l  (a0),a0\n   move.w  18(a0),d0");
BLITZ_INLINE(dosmousey, "   move.l  (a0),a0\n   move.w  16(a0),d0");

/* ---------------------------------------------------------- freescreen */

/* freescreen - a3 = object, a6 = intuition, a2 = graphics */
static void freescreen_(ULONG *obj, struct IntuitionBase *IntuitionBase,
                        struct GfxBase *GfxBase)
{
    UBYTE *s = (UBYTE *)obj[0];
    struct bl_regs r = { 0 };

    if (!s)
        return;
    if (*(ULONG *)(s + 342) == BLTZ) {          /* don't close non Blitzs */
        r.d0 = *(ULONG *)(s + 22);              /* title copy */
        BLITZ_ALIBJSR(TOK_freest, r);
        CloseScreen((struct Screen *)obj[0]);
        if (obj[1]) {
            FreeDBufInfo((struct DBufInfo *)obj[1]);
            obj[1] = 0;
        }
    }
    obj[0] = 0;
}

static void BLITZ_ROUTINE freescreen_c(struct IntuitionBase *intui BL_REG(a6),
                                       struct GfxBase *gfx BL_REG(a2))
{
    freescreen_((ULONG *)blitz_string, intui, gfx);
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
    ULONG *obj = (ULONG *)blitz_string;
    const UBYTE *sp = args;
    struct Screen *scr;
    ULONG font = *ifont;                        /* MOVE.l (a1),d7 */
    UWORD w;

    *(ULONG **)(blitz_globals + used) = obj;    /* set used screen! */
    freescreen_(obj, IntuitionBase, (struct GfxBase *)args);   /* BUG(orig) (#247): a2 */
    sd.ns.font = font;
    sd.ns.left = BL_POPW(sp);
    sd.ns.top = BL_POPW(sp);
    w = BL_POPW(sp);
    sd.ns.width = (w + 15) & 0xfff0;
    sd.ns.height = BL_POPW(sp);
    sd.ns.depth = BL_POPW(sp);
    sd.ns.viewmodes = viewmode;
    BL_SETW(sd.tags[1], viewmode);              /* MOVE d1,newmodes+2 */
    sd.ns.dpen = dpen;
    sd.ns.bpen = bpen;
    sd.ns.bitmap = (ULONG)bitmap;
    sd.ns.title = allocst(title, BL_POPL(sp));
    sd.ns.type = (sd.ns.bitmap ? 0x4f : 15) | sd.defstype;
    scr = OpenScreen((struct NewScreen *)&sd.ns);
    obj[0] = (ULONG)scr;
    if (!scr) {
        err1();                                 /* BEQ err1 (error area) */
        return;
    }
    *(ULONG *)((UBYTE *)scr + 342) = BLTZ;
}

void BLITZ_ROUTINE _screen(ULONG viewmode BL_REG(d1), const UBYTE *title BL_REG(d2),
                           UBYTE dpen BL_REG(d3), UBYTE bpen BL_REG(d4),
                           ULONG *ifont BL_REG(a1), WORD used BL_REG(d6),
                           const UBYTE *args BL_REG(a2), struct IntuitionBase *intui BL_REG(a6))
{
    openscreen(0, ifont, used, viewmode, title, dpen, bpen, args, intui);
}

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

void BLITZ_ROUTINE domode(UWORD mode BL_REG(d1), const UBYTE *gfx BL_REG(a0),
                          ULONG *ifont BL_REG(a1), WORD used BL_REG(d6),
                          struct IntuitionBase *intui BL_REG(a6))
{
    modec(mode, 0, 0, gfx, ifont, used, intui);
}

void BLITZ_ROUTINE domode2(UWORD mode BL_REG(d1), const UBYTE *title BL_REG(d2),
                           const UBYTE *gfx BL_REG(a0), ULONG *ifont BL_REG(a1),
                           WORD used BL_REG(d6), const ULONG *lens BL_REG(a2),
                           struct IntuitionBase *intui BL_REG(a6))
{
    const ULONG *sp = lens;

    modec(mode, title, BL_POPL(sp), gfx, ifont, used, intui);
}

/* ------------------------------------------------ WbToScreen, FindScreen */

void BLITZ_ROUTINE dowbscreen(WORD used BL_REG(d6), const UBYTE *args BL_REG(a2),
                              struct IntuitionBase *intui BL_REG(a6))
{
    ULONG *obj = (ULONG *)blitz_string;
    UBYTE *s = (UBYTE *)intui->FirstScreen;

    freescreen_(obj, intui, (struct GfxBase *)args);           /* BUG(orig) (#247, #248): a2 */
    for (;; s = *(UBYTE **)s) {
        if (!s) {
            BLITZ_RUNERR(mess8);                /* err8 */
            return;
        }
        if ((*(UWORD *)(s + 20) & 15) == WBENCHSCREEN)
            break;
    }
    obj[0] = (ULONG)s;                          /* Got WorkBench */
    *(ULONG **)(blitz_globals + used) = obj;
}

/* title / default title starting with t? */
static int prefix(const UBYTE *s, const UBYTE *t)
{
    while (*t)
        if (*s++ != *t++)                       /* CMPM.b (a1)+,(a2)+ */
            return 0;
    return 1;
}

static void findscreen(const UBYTE *t, WORD used, const UBYTE *args,
                       struct IntuitionBase *intui)
{
    ULONG *obj = (ULONG *)blitz_string;
    UBYTE *s = (UBYTE *)&intui->FirstScreen;    /* the list starts at +60 */

    *(ULONG **)(blitz_globals + used) = obj;
    freescreen_(obj, intui, (struct GfxBase *)args);           /* BUG(orig) (#247, #248): a2 */
    for (;;) {
        s = *(UBYTE **)s;
        if (!s) {
            BLITZ_RUNERR(mess12);               /* err12 */
            return;
        }
        if (!t)
            break;
        if (!*(UBYTE **)(s + 22))               /* no title: next screen */
            continue;
        if (prefix(*(UBYTE **)(s + 22), t))
            break;
        if (!*(UBYTE **)(s + 26))
            continue;
        if (prefix(*(UBYTE **)(s + 26), t))     /* DefaultTitle */
            break;
    }
    obj[0] = (ULONG)s;
}

void BLITZ_ROUTINE dofstoscreen(WORD used BL_REG(d6), const UBYTE *args BL_REG(a2),
                                struct IntuitionBase *intui BL_REG(a6))
{
    findscreen(0, used, args, intui);           /* no title */
}

void BLITZ_ROUTINE dofindscreen(const UBYTE *t BL_REG(d1), WORD used BL_REG(d6),
                                const UBYTE *args BL_REG(a2),
                                struct IntuitionBase *intui BL_REG(a6))
{
    findscreen(t, used, args, intui);
}

/* ---------------------------------------------------------- LoadScreen */

static ULONG loadscreen(ULONG *obj, const UBYTE *name)
{
    struct bl_regs r = { 0 };

    r.a1 = *(ULONG *)(obj[0] + 88);             /* screen->RastPort.BitMap */
    r.a0 = (ULONG)name;
    return BLITZ_ALIBJSR(TOK_ilbmbitmap, r);
}

void BLITZ_ROUTINE doloadscreen(ULONG *obj BL_REG(a0), const UBYTE *name BL_REG(d1))
{
    loadscreen(obj, name);
}

void BLITZ_ROUTINE doloadscreen2(ULONG *obj BL_REG(a0), const UBYTE *name BL_REG(d1),
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
static void intshowbitmap(UBYTE *bm, UBYTE *scr, struct IntuitionBase *intui,
                          struct GfxBase *GfxBase)
{
    ULONG *src = (ULONG *)(bm + 8), *dst = (ULONG *)(scr + 192);
    UWORD n = *(UWORD *)(bm + 4) - 1;

    do
        *dst++ = *src++;
    while (n-- != 0);                           /* DBF d0 */
    MakeVPort(&intui->ViewLord, (struct ViewPort *)(scr + 44));
    showbitmap2(intui, GfxBase);
}

/* agashowbitmap - V39+: ChangeVPBitMap, DBufInfo allocated once */
static void agashowbitmap(UBYTE *bm, ULONG *obj, struct GfxBase *GfxBase)
{
    UBYTE *scr = (UBYTE *)obj[0];
    struct DBufInfo *db = (struct DBufInfo *)obj[1];

    if (!db) {
        db = AllocDBufInfo((struct ViewPort *)(scr + 44));
        obj[1] = (ULONG)db;
    }
    ChangeVPBitMap((struct ViewPort *)(scr + 44), (struct BitMap *)bm, db);
}

/* doshowbitmap - a0 = bitmap, a1 = screen object, a3 = intuition, a6 =
   graphics.  Before V39: the BPLxPT moves of the screen's copper list
   (first $0000F0E0 = MOVE to bpl1pth) get the plane pointers. */
static void showbitmap(UBYTE *bm, ULONG *obj, struct GfxBase *GfxBase)
{
    struct IntuitionBase *intui = (struct IntuitionBase *)blitz_string;   /* a3 */
    UBYTE *scr, *cl, *ins;
    const UWORD *src;
    UWORD n, d1;

    if ((WORD)(execversion() - 39) >= 0) {      /* CMP #39,d0:BPL */
        agashowbitmap(bm, obj, GfxBase);
        return;
    }
    BL_INTS_OFF();
    scr = (UBYTE *)obj[0];
    if (scr[77] & 4) {                          /* interlace! */
        intshowbitmap(bm, scr, intui, GfxBase);
        return;
    }
    if ((WORD)execversion() >= 39) {
        intshowbitmap(bm, scr, intui, GfxBase);
        return;
    }
    cl = scr + 52;                              /* &ViewPort.DspIns */
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
    d1 = *(UWORD *)(bm + 4) * 2;
    src = (const UWORD *)(bm + 8);
    for (;;) {
        do {                                    /* 'loop */
            if (*(UWORD *)ins == 2)
                goto more;
            *(UWORD *)(ins + 4) = *src++;
            ins += 6;
            if (!--d1) {                        /* SUBQ #1,d1:DBEQ:BEQ */
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

void BLITZ_ROUTINE doshowbitmap0(ULONG *obj BL_REG(a1), struct GfxBase *gfx BL_REG(a6))
{
    showbitmap((UBYTE *)obj[0] + 184, obj, gfx);    /* the screen's BitMap */
}

void BLITZ_ROUTINE doshowbitmap1(UBYTE *bm BL_REG(a0), ULONG *obj BL_REG(a1),
                                 struct GfxBase *gfx BL_REG(a6))
{
    showbitmap(bm, obj, gfx);
}

/* ---------------------------------------------------------- ScreenTags */

/* duntags - OpenScreenTagList (V36+), result screen or the error code */
static ULONG duntags(ULONG *obj, struct IntuitionBase *IntuitionBase)
{
    struct Screen *scr;

    if ((WORD)(execversion() - 36) < 0)         /* CMP #36,d0:BMI */
        return errcode;
    scr = OpenScreenTagList(NULL, (struct TagItem *)&tl);
    obj[0] = (ULONG)scr;
    if (!scr)
        return errcode;
    *(ULONG *)((UBYTE *)scr + 342) = BLTZ;
    return (ULONG)scr;
}

/* common start: used, title copy, font, free the old screen */
static const ULONG *tagstart(const UBYTE *title, const ULONG *lens, ULONG *ifont,
                             WORD used, struct IntuitionBase *intui)
{
    ULONG *obj = (ULONG *)blitz_string;
    const ULONG *sp = lens;

    *(ULONG **)(blitz_globals + used) = obj;    /* make used */
    tl.title[1] = allocst(title, BL_POPL(sp));
    tl.font[1] = *ifont;
    freescreen_(obj, intui, (struct GfxBase *)sp);  /* BUG(orig) (#247): a2 */
    return sp;
}

/* ScreenTags n,title$,taglist - copy the taglist (up to TAG_END) */
ULONG BLITZ_ROUTINE doscreentags3(const UBYTE *title BL_REG(d1), const ULONG *tags BL_REG(d2),
                                  ULONG *ifont BL_REG(a1), WORD used BL_REG(d6),
                                  const ULONG *lens BL_REG(a2),
                                  struct IntuitionBase *intui BL_REG(a6))
{
    ULONG *t = tl.usertags, d0;

    tagstart(title, lens, ifont, used, intui);
    do {                                        /* nxtag: MOVEM.l (a2)+,d0-d1 */
        d0 = *tags++;
        t[0] = d0;
        t[1] = *tags++;
        t += 2;
    } while (d0);
    return duntags((ULONG *)blitz_string, intui);
}

/* ScreenTags n,title$[,tag,data...] - d7 = number of pairs (a2 stack) */
ULONG BLITZ_ROUTINE doscreentags(UWORD n BL_REG(d7), const UBYTE *title BL_REG(d1),
                                 ULONG *ifont BL_REG(a1), WORD used BL_REG(d6),
                                 const ULONG *lens BL_REG(a2),
                                 struct IntuitionBase *intui BL_REG(a6))
{
    const ULONG *sp = tagstart(title, lens, ifont, used, intui);
    ULONG *t = tl.usertags;

    for (; n; n--) {                            /* gettags: DBRA d7,tagloop */
        t[0] = BL_POPL(sp);
        t[1] = BL_POPL(sp);
        t += 2;
    }
    *t = 0;                                     /* MOVE.l #0,(a0)+ */
    return duntags((ULONG *)blitz_string, intui);
}

ULONG BLITZ_ROUTINE doscreentags2(const UBYTE *title BL_REG(d1), ULONG *ifont BL_REG(a1),
                                  WORD used BL_REG(d6), const ULONG *lens BL_REG(a2),
                                  struct IntuitionBase *intui BL_REG(a6))
{
    return doscreentags(0, title, ifont, used, lens, intui);   /* no tags */
}

/* ------------------------------------------- error checks (debugger only) */

/* err1 - called from the normal code (Can't open Screen) */
static void BLITZ_ERRCHK err1(void)
{
    BLITZ_RUNERR(mess1);
}

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
static void BLITZ_ERRCHK prescreen2_c(ULONG vm BL_REG(d1), const UWORD *bm BL_REG(a0),
                                      const UBYTE *ifont BL_REG(a1),
                                      const UBYTE *args BL_REG(a2))
{
    prescreen_c(vm, ifont, args);
    if (!bm[0])
        BLITZ_RUNERR(mess2);
    if ((UWORD)(bm[0] << 3) < *(UWORD *)(args - 6)
        || bm[1] < *(UWORD *)(args - 8) || bm[2] < *(UWORD *)(args - 10))
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK loadscreenchk_c(const ULONG *obj BL_REG(a0))
{
    if (!*obj)
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

static void BLITZ_ERRCHK showbmchk0_c(const ULONG *obj BL_REG(a1))
{
    if (!*obj)
        BLITZ_RUNERR(mess7);
}

/* showbmchk - bitmap of the same size as the screen's */
static void BLITZ_ERRCHK showbmchk_c(const UBYTE *bm BL_REG(a0), const ULONG *obj BL_REG(a1))
{
    const UBYTE *s;

    showbmchk0_c(obj);
    if (!*(UWORD *)bm)
        BLITZ_RUNERR(mess2);
    s = (const UBYTE *)*obj + 184;
    if (*(UWORD *)bm != *(UWORD *)s || *(UWORD *)(bm + 2) != *(UWORD *)(s + 2)
        || bm[5] != s[5])
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
BLITZ_ERRCHK_MESSAGE(mess1, "Can't open Screen");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess3, "Can't use specified BitMap");
BLITZ_ERRCHK_MESSAGE(mess5, "Screen width MUST be at least 320");
BLITZ_ERRCHK_MESSAGE(mess6, "Screen Height MUST be at least 10");
BLITZ_ERRCHK_MESSAGE(mess7, "Uninitialized Screen");
BLITZ_ERRCHK_MESSAGE(mess8, "Can't find WorkBench");
BLITZ_ERRCHK_MESSAGE(mess9, "Illegal number of BitPlanes");
BLITZ_ERRCHK_MESSAGE(mess10, "Uninitialized IntuiFont");
BLITZ_ERRCHK_MESSAGE(mess11, "Illegal Mode");
BLITZ_ERRCHK_MESSAGE(mess12, "Can't find Screen");
BLITZ_ERRCHK_MESSAGE(mess13, "Can't find CopIns!");
BLITZ_ERRCHK_MESSAGE(mess14, "Screen and BitMap MUST be same size");
