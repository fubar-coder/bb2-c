/*
 * intuifontlib - command library 134: LoadFont and the IntuiFont objects
 * (TextAttr for intuition: screenslib, windowslib, gadgetslib use the
 * current one).
 *
 * C translation of bblibsrc/intuifontlib.  The CD binary is OLDER than
 * the source: the source has a token `Text` (String$,IntuiFont#,X,Y) with
 * an empty routine that the binary does not contain; this file follows
 * the binary (the rest of the code is the same as in the source).
 *
 * Object (16 bytes, maximum 5, IntuiFont#): {name.l, ysize.w, style.b,
 * flags.b, font.l (from OpenDiskFont), pad.l}.  The init value (long) is
 * the ADDRESS of the variable `textattr`, which points to the current
 * object (LoadFont, Use IntuiFont) or to the default topaz 8 TextAttr.
 *
 * LoadFont n,name$,ysize[,style] (a3 = object, a6 = diskfont, d6 =
 * graphics, d5 = offset of the `used` variable, a2 = name length):
 * closes the object's font, copies the name with maxslib $CF01 (allocst),
 * OpenDiskFont; the CCR (Z = font not opened) is checked by loadchk after
 * the routine (debugger only, "Can't Open Font").
 *
 * BUG(orig), kept:
 *   - the style goes to the WORD at +6 (MOVE.w d3,6(a3)): style byte =
 *     upper byte of the argument (0 for 0..255), flags byte = style. (#197)
 *   - the name copy of a previous LoadFont is never freed (allocst). (#198)
 * Not a bug (the documented Blitz mechanism): the routine table has the code
 * addresses +1 (`dosetifont+1`): bit 0 = Amiga mode only (blitz2.s
 * doajsrrout).
 * DIFF: a2 stays unchanged (the original pops the name length).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include <graphics/text.h>

#define LVO_OpenDiskFont (-30)  /* diskfont: a0 = TextAttr */
#define LVO_CloseFont    (-78)  /* graphics: a1 = font */

/* entry points (thunks, defined below) */
void dosetifont(void), dosetifont2(void), codeinit(void);
void douseifont(void), dofreeifont(void), loadchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#intuifontlib,init,1,0,errors} */
BLITZ_LIBHEADER(LIB_intuifontlib, init, BL_LONGRET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_intuifontlib, 0x1380, LIB_diskfontlib, BL_LA(6),
           LIB_graphicslib, BL_LD(6), LIB_intuifontlib, 0x0504);
BLITZ_SUBS(BLITZ_AMIGAONLY(dosetifont), 0, loadchk);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_intuifontlib, 0x1380, LIB_diskfontlib, BL_LA(6),
           LIB_graphicslib, BL_LD(6), LIB_intuifontlib, 0x0504);
BLITZ_SUBS(BLITZ_AMIGAONLY(dosetifont2), 0, loadchk);
BLITZ_NAME("LoadFont", "IntuiFont#,Fontname.font$,Y size [,style]");

BLITZ_DUMTOKE(_toke, "IntuiFont", "A Font for use by intuition");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, douseifont, 0, 0);
BLITZ_NULLSUB_AT(_free, dofreeifont, 0, 0, LIB_graphicslib, BL_LA(6));

BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 4);

/* ------------------------------------------------------------ variables */

/* one IntuiFont object (16 bytes): struct bb_intuifont (blitzobj.h) */

/* textattr, then the default object and its name, as in the original */
static struct BL_PACKED {
    struct bb_intuifont *textattr;
    struct bb_intuifont defattr;
    char defname[12];
} fd = { .defname = "topaz.font" };

/* ---------------------------------------------------------------- init */

/**
 * @brief The library's init: the default object topaz 8 becomes the
 * current IntuiFont (the pad is not cleared).
 * @return d0: &textattr, the library's init value (other libraries read
 *         the current IntuiFont through it)
 */
static ULONG BLITZ_ROUTINE codeinit_c(void)
{
    fd.textattr = &fd.defattr;
    fd.defattr.ta.ta_Name = (STRPTR)fd.defname;
    fd.defattr.ta.ta_YSize = 8;
    fd.defattr.ta.ta_Style = 0;                 /* CLR defattr+6 */
    fd.defattr.ta.ta_Flags = 0;
    fd.defattr.font = 0;
    return (ULONG)&fd.textattr;
}
BLITZ_THUNK(codeinit, codeinit_c, "d1/a0-a1");

/* ----------------------------------------------------- free / use font */

/* dofreeifont - close the object's font (a3 = object, a6 = graphics) */
static void freeifont(struct bb_intuifont *f, const void *gfx)
{
    struct bl_regs r = { 0 };

    if (!f->font)
        return;
    r.a1 = (ULONG)f->font;
    f->font = 0;
    BLITZ_BASEJSR(gfx, LVO_CloseFont, r);
}

/**
 * @brief Free IntuiFont#: the object's free routine (_free): close its
 * font (CloseFont), if one is open.
 *
 * The name copy is not freed, BUG(orig) (#198).  The object comes in a3.
 *
 * @param gfx  a6: graphics.library
 */
static void BLITZ_ROUTINE dofreeifont_c(const void *gfx BL_REG(a6))
{
    freeifont((struct bb_intuifont *)blitz_string, gfx);   /* a3 = object */
}
BLITZ_THUNK(dofreeifont, dofreeifont_c, "d1/a0-a1");

/**
 * @brief Use IntuiFont#: the object's use routine (_use): the object (a3)
 * becomes the current IntuiFont (MOVE.l a3,textattr).
 */
static void BLITZ_ROUTINE douseifont_c(void)
{
    fd.textattr = (struct bb_intuifont *)blitz_string;     /* MOVE.l a3,textattr */
}
BLITZ_THUNK(douseifont, douseifont_c, "d0-d1/a0-a1");

/* ------------------------------------------------------------ LoadFont */

static ULONG setifont(const UBYTE *name, UWORD ysize, UWORD style,
                      const ULONG *lens, WORD used, const void *gfx,
                      const void *diskfont)
{
    struct bb_intuifont *f = (struct bb_intuifont *)blitz_string;     /* a3 = object */
    const ULONG *sp = lens;
    struct bl_regs r = { 0 };

    *(struct bb_intuifont **)(blitz_globals + used) = f;   /* MOVE.l a3,0(a5,d5) */
    fd.textattr = f;
    freeifont(f, gfx);
    f->ta.ta_YSize = ysize;
    /* CLR 6(a3):MOVE.w d3,6(a3) - BUG(orig) (#197): the word over ta_Style
       and ta_Flags */
    *(UWORD *)&f->ta.ta_Style = style;
    r.d0 = (ULONG)name;
    r.d1 = BL_POPL(sp);
    f->ta.ta_Name = (STRPTR)BLITZ_ALIBJSR(TOK_allocst, r);
    r.d0 = r.d1 = r.a1 = 0;
    r.a0 = (ULONG)f;
    f->font = (struct TextFont *)BLITZ_BASEJSR(diskfont, LVO_OpenDiskFont, r);
    return (ULONG)f->font;                      /* MOVE.l d0,8(a3): Z for loadchk */
}

/**
 * @brief LoadFont IntuiFont#,Fontname.font$,Y size: load a font into an
 * IntuiFont object (style 0).
 *
 * The object (a3) becomes the used and the current IntuiFont; its old
 * font is closed, the name copied with maxslib allocst (the copy of a
 * previous LoadFont is never freed, BUG(orig) (#198)), then OpenDiskFont.
 * Amiga mode only.  DIFF: a2 stays unchanged (the original pops the name
 * length).
 * Errors (debugger only): "Can't Open Font" - loadchk tests the Z flag
 * the routine leaves.
 *
 * @param name      d1: the font name
 * @param ysize     d2: the height
 * @param lens      a2: the stack with the name's length
 * @param used      d5: the offset of the `used` variable from a5
 * @param gfx       d6: graphics.library
 * @param diskfont  a6: diskfont.library
 * @return d0: the font, 0 if it could not be opened (CCR Z then)
 */
static ULONG BLITZ_ROUTINE dosetifont_c(const UBYTE *name BL_REG(d1), UWORD ysize BL_REG(d2),
                                        const ULONG *lens BL_REG(a2), WORD used BL_REG(d5),
                                        const void *gfx BL_REG(d6), const void *diskfont BL_REG(a6))
{
    return setifont(name, ysize, 0, lens, used, gfx, diskfont);     /* MOVEQ #0,d3 */
}
BLITZ_THUNK_Z(dosetifont, dosetifont_c, "d1");

/**
 * @brief LoadFont IntuiFont#,Fontname.font$,Y size,style: LoadFont with a
 * style.
 *
 * BUG(orig) (#197): the style goes to the WORD at +6: ta_Style = the
 * upper byte of the argument (0 for 0..255), ta_Flags = the style.
 *
 * @param name      d1: the font name
 * @param ysize     d2: the height
 * @param style     d3: the style
 * @param lens      a2: the stack with the name's length
 * @param used      d5: the offset of the `used` variable from a5
 * @param gfx       d6: graphics.library
 * @param diskfont  a6: diskfont.library
 * @return d0: the font, 0 if it could not be opened (CCR Z then)
 */
static ULONG BLITZ_ROUTINE dosetifont2_c(const UBYTE *name BL_REG(d1), UWORD ysize BL_REG(d2),
                                         UWORD style BL_REG(d3), const ULONG *lens BL_REG(a2),
                                         WORD used BL_REG(d5), const void *gfx BL_REG(d6),
                                         const void *diskfont BL_REG(a6))
{
    return setifont(name, ysize, style, lens, used, gfx, diskfont);
}
BLITZ_THUNK_Z(dosetifont2, dosetifont2_c, "d1");

/* ------------------------------------------- error check (debugger only) */

/* loadchk - BEQ err1:RTS, err1: MOVE.l #mess1,d0:TRAP #0.  Assembler: it
   tests the CCR that LoadFont leaves (Z = font not opened). */
__asm__("   .section .blitzlib.errchk,\"ax\"\n"
        "   .even\n   .globl  _loadchk\n"
        "_loadchk:\n"
        "   beq.s   1f\n"
        "   rts\n"
        "1: move.l  #_mess1,d0\n"
        "   trap    #0\n"
        "   .text\n");

BLITZ_ERRCHK_MESSAGE(mess1, "Can't Open Font");
