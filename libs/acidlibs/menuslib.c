/*
 * menuslib - command library 136: Intuition menus - MenuTitle, SetMenu,
 * MenuItem, SubItem, ShapeItem, ShapeSub, MenuGap, SubItemOff, MenuState,
 * MenuColour, MenuChecked.
 *
 * C translation of bblibsrc/menuslib (binary checked: same as the source).
 * An object library (!libfin{_toke,...,5,2}): the Blitz object "MenuList"
 * (dumtoke), objects of 2^2 = 4 bytes = the first struct Menu; _free =
 * freemenu.  Menus (30 bytes) and items (34 bytes + IntuiText or Image,
 * 54 bytes) come from memlib globalloc (MEMF_PUBLIC|MEMF_CLEAR), texts
 * from maxslib allocst ($CF01, freed with $CF02).
 *
 * Positions: findthing walks to entry n of a list; the position (menu
 * LeftEdge, item TopEdge) is the sum of the sizes (Width resp. Height) of
 * the entries before it.  Replacing an existing entry frees it, allocates
 * a new one with its next/sublist pointers and moves all following entries
 * by the size difference.  SetMenu gives all items of a list (and of every
 * sublist) the width of the widest one.
 *
 * The flags argument (#push|#long, read with -(a2) in sussflags):
 *   0 = default ($52), 1 = toggle ($5B), 2 = toggle + exclude the others
 *   ($5B, MutualExclude = all but this item), 3 = checked toggle ($15B),
 *   4 = 3 + exclude, >= 5 = the flags themselves (word compare).
 *
 * Kept from the original (BUG(orig)):
 *   - MenuState on a whole menu (MenuState l,m,onoff) changes bit 4 of the
 *     Menu flags (changestate2, as for items) instead of MENUENABLED: no
 *     effect.  MenuState l,onoff (whole list) uses MENUENABLED. (#216)
 *   - MenuChecked with a subitem: the item and subitem DBF loops jump back
 *     to the menu loop; only (menu,item,sub) = (m,0,0) ends where it
 *     should, other values walk the lists again with a counter of -1
 *     (65536 steps).  Nachgebildet. (#217)
 *   - Not a bug (word result, the compiler uses only d0.w): MenuChecked:
 *     SNE/EXT.w, the upper word of d0 stays.
 *   - ShapeItem/ShapeSub read the flags with MOVE 4(a7),d7, the HIGH word
 *     of the pushed long, through the word `temp` (with the cleared low
 *     word of mexclude in front as upper word): small flag values give
 *     the default flags.  Their item Flags are set (MOVE), not ORed. (#218)
 *   - The flag test is a word compare after a long test: $10000 gives
 *     word 0 < 5 and ends in the "4" branch. (#219)
 *   - Not a bug (the intended result for a new entry): a shortcut sets the
 *     entry's Flags to 4 (COMMSEQ) in findthing, the item flags are ORed
 *     in afterwards.
 *   - Not a bug (a limitation acknowledged in the original source): MenuTitle
 *     width = 8 * length + 8 ("8 wide font, for now").
 *   - SubItem replacing an existing subitem moves the following ones by
 *     (subitem number - old height) instead of the height difference
 *     (d3 is the number again after textcalc). (#220)
 *   - No checks without the debugger: wrong menu/item numbers walk over
 *     the end of the lists. (#416)
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include <stddef.h>
#include <intuition/intuition.h>
#define __NOLIBBASE__
#define INTUITION_BASE_NAME intbase
#include <proto/intuition.h>
#undef __NOLIBBASE__

/* maxslib: string copies in global memory */

/* entry points (thunks / C, defined below) */
void doshapeitem(void), doshapesub(void), domenugap(void), dosubitemoff(void);
void menuslib_domenustate(void), menuslib_domenustate2(void);
void menuslib_domenustate3(void), menuslib_domenustate4(void);
void domenupens(void), menuslib_domenuchecked(void);
void menuslib_domenuchecked2(void), menutitlechk(void), setmenuchk(void);
void itemchk2(void), subchk2(void), shapeitemchk(void), shapesubchk(void);
void menuslib_statechk2(void), menuslib_statechk3(void);
void menuslib_statechk4(void), menuslib_chkchk3(void), menuslib_chkchk4(void);
void BLITZ_ROUTINE dosetmenu(struct Window **win BL_REG(a2),
                             struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE domenutitle(WORD m BL_REG(d1), const UBYTE *text BL_REG(d2),
                               const ULONG *lens BL_REG(a2));
void BLITZ_ROUTINE domenuitem(WORD m BL_REG(d1), WORD i BL_REG(d2),
                              const UBYTE *text BL_REG(d3),
                              struct TextAttr **font BL_REG(a1),
                              const ULONG *lens BL_REG(a2),
                              struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE domenuitem2(WORD m BL_REG(d1), WORD i BL_REG(d2),
                               const UBYTE *text BL_REG(d3),
                               const UBYTE *key BL_REG(d4),
                               struct TextAttr **font BL_REG(a1),
                               const ULONG *lens BL_REG(a2),
                               struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dosubitem(WORD m BL_REG(d1), WORD i BL_REG(d2), WORD s BL_REG(d3),
                             const UBYTE *text BL_REG(d4),
                             struct TextAttr **font BL_REG(a1),
                             const ULONG *lens BL_REG(a2),
                             struct Library *intbase BL_REG(a6));
void BLITZ_ROUTINE dosubitem2(WORD m BL_REG(d1), WORD i BL_REG(d2), WORD s BL_REG(d3),
                              const UBYTE *text BL_REG(d4),
                              const UBYTE *key BL_REG(d5),
                              struct TextAttr **font BL_REG(a1),
                              const ULONG *lens BL_REG(a2),
                              struct Library *intbase BL_REG(a6));
static void BLITZ_ROUTINE freemenu(void);

/* !libheader{#menuslib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_menuslib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_menuslib, 0x1380);
BLITZ_SUBS(domenutitle, menutitlechk, 0);
BLITZ_NAME("MenuTitle", "MenuList#,Menu,Title$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_windowslib, 0x1202, LIB_menuslib, 0x1380, LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(dosetmenu), setmenuchk, 0);           /* Amiga mode only */
BLITZ_NAME("SetMenu", "Menulist#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_PUSH | BL_LONG, BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_menuslib, 0x1380, LIB_intuifontlib, BL_LA(1), LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(domenuitem, itemchk2, 0);
BLITZ_ARGS(BL_WORD, BL_PUSH | BL_LONG, BL_WORD, BL_WORD, BL_STRING, BL_STRING);
BLITZ_LIBS(LIB_menuslib, 0x1380, LIB_intuifontlib, BL_LA(1), LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(domenuitem2, itemchk2, 0);
BLITZ_NAME("MenuItem", "MenuList#,Flags,Menu,Item,Itemtext$[,Shortcut$]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_PUSH | BL_LONG, BL_WORD, BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_menuslib, 0x1380, LIB_intuifontlib, BL_LA(1), LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(dosubitem, subchk2, 0);
BLITZ_ARGS(BL_WORD, BL_PUSH | BL_LONG, BL_WORD, BL_WORD, BL_WORD, BL_STRING, BL_STRING);
BLITZ_LIBS(LIB_menuslib, 0x1380, LIB_intuifontlib, BL_LA(1), LIB_intuitionlib, BL_LA(6));
BLITZ_SUBS(dosubitem2, subchk2, 0);
BLITZ_NAME("SubItem", "MenuList#,Flags,Menu,Item,Subitem,Subitemtext$[,Shortcut$]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_PUSH | BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1380, LIB_shapeslib, 0x1283);
BLITZ_SUBS(doshapeitem, shapeitemchk, 0);
BLITZ_NAME("ShapeItem", "MenuList#,Flags,Menu,Item,Shape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_PUSH | BL_LONG, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1380, LIB_shapeslib, 0x1284);
BLITZ_SUBS(doshapesub, shapesubchk, 0);
BLITZ_NAME("ShapeSub", "MenuList#,Flags,Menu,Item,Subitem,Shape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(domenugap, 0, 0);
BLITZ_NAME("MenuGap", "X Gap,Y Gap");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dosubitemoff, 0, 0);
BLITZ_NAME("SubItemOff", "X Offset,Y Offset");

BLITZ_NULLSUB(0, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1380);
BLITZ_SUBS(menuslib_domenustate, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1380);
BLITZ_SUBS(menuslib_domenustate2, menuslib_statechk2, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1380);
BLITZ_SUBS(menuslib_domenustate3, menuslib_statechk3, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1380);
BLITZ_SUBS(menuslib_domenustate4, menuslib_statechk4, 0);
BLITZ_NAME("MenuState", "MenuList#[,Menu[,Item[,Subitem]]],On|Off");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(domenupens, 0, 0);
BLITZ_NAME("MenuColour", "Colour");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1280);
BLITZ_SUBS(menuslib_domenuchecked, menuslib_chkchk3, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1280);
BLITZ_SUBS(menuslib_domenuchecked2, menuslib_chkchk4, 0);
BLITZ_NAME("MenuChecked", "(MenuList#,Menu,Item[,Subitem])");

BLITZ_DUMTOKE(_toke, "MenuList", "List of Menus");

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freemenu, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,5,2} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 2);

/* ***************** Code Below ******************* */

/* The blocks of a MenuList: a struct Menu (30 bytes) per menu, and per
   item or subitem a struct MenuItem (34) with the IntuiText or Image it
   shows right behind it (20, ItemFill points there). */
struct item {
    struct MenuItem item;
    union {
        struct IntuiText text;  /* MenuItem, SubItem */
        struct Image image;     /* ShapeItem, ShapeSub */
    } fill;
};
static_assert(sizeof(struct Menu) == 30);
static_assert(sizeof(struct item) == 54);
#define MENUSIZE    sizeof(struct Menu)
#define ITEMSIZE    sizeof(struct item)

/* The list code (findthing, walk, setmax, the checks) works on menus and
   items alike, with byte offsets: the next pointer is at 0 in both, and
   the address of a list head (the object, FirstItem, SubItem) is used as
   an entry whose next pointer is the head. */
#define M_LEFT      offsetof(struct Menu, LeftEdge)
#define M_WIDTH     offsetof(struct Menu, Width)        /* also MenuItem */
#define M_FLAGS     offsetof(struct Menu, Flags)        /* also MenuItem */
#define M_FIRST     offsetof(struct Menu, FirstItem)
#define I_TOP       offsetof(struct MenuItem, TopEdge)
#define I_SUB       offsetof(struct MenuItem, SubItem)
#define FLAGS_HI    M_FLAGS                 /* the bytes of Flags: CHECKED */
#define FLAGS_LO    (M_FLAGS + 1)           /* ITEMTEXT, ITEMENABLED, MENUENABLED */
static_assert(offsetof(struct MenuItem, Width) == M_WIDTH);
static_assert(offsetof(struct MenuItem, Flags) == M_FLAGS);

#define W(p, o)  (*(WORD *)((UBYTE *)(p) + (o)))
#define UW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define L(p, o)  (*(ULONG *)((UBYTE *)(p) + (o)))
#define B(p, o)  (*(UBYTE *)((UBYTE *)(p) + (o)))
#define P(p, o)  (*(UBYTE **)((UBYTE *)(p) + (o)))

static UBYTE menupens[2] __attribute__((aligned(2))) = { 1, 2 };  /* fpen, bpen */
static UBYTE menumode;
static UWORD dmenflags = 0x10 + 2 + 0x40;
static UWORD menflags;
static WORD subxoff = 48, subyoff = 4;
static WORD menuxxs = 4, menuyxs = 2;
static WORD fonthite = 8;
static UBYTE shortcut;          /* high byte of the word `shortcut` */
/* mexclude.l followed by temp.w: ShapeItem reads a long at temp-2 */
static struct { ULONG mexclude; UWORD temp; } BL_PACKED mt;
#define mexclude mt.mexclude

/* dumintuitext: an IntuiText for IntuiTextLength */
static struct IntuiText dumintuitext;
#define fontattr dumintuitext.ITextFont

/* maxslib / memlib */
static UBYTE *allocst(const UBYTE *s, ULONG len)
{
    struct bl_regs r = { .d0 = (ULONG)s, .d1 = len };

    return (UBYTE *)BLITZ_ALIBJSR(TOK_allocst, r);
}

static void freest(ULONG s)
{
    struct bl_regs r = { .d0 = s };

    BLITZ_ALIBJSR(TOK_freest, r);
}

static UBYTE *globalloc(ULONG size)
{
    struct bl_regs r = { .d0 = size, .d1 = MEMF_PUBLIC | MEMF_CLEAR };

    return (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(void *mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = (ULONG)mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/**
 * @brief MenuColour Colour - the front pen of the texts of items and
 * subitems created from now on.
 *
 * MOVE.b d0,menufpen.
 *
 * @param c d0: the pen.
 */
static void BLITZ_ROUTINE domenupens_c(UBYTE c BL_REG(d0))
{
    menupens[0] = c;
}
BLITZ_THUNK(domenupens, domenupens_c, "d0-d1/a0-a1");

/* changestate: MENUENABLED (bit 0 of the flags' low byte),
   changestate2: bit 4 (ITEMENABLED) */
static void changestate(UBYTE *p, UWORD on, int bit)
{
    if (on)
        B(p, FLAGS_LO) |= 1 << bit;
    else
        B(p, FLAGS_LO) &= ~(1 << bit);
}

/* walk n (DBF: n+1 times) next pointers */
static UBYTE *walk(UBYTE *p, UWORD n)
{
    do
        p = P(p, 0);
    while (n-- != 0);
    return p;
}

/**
 * @brief MenuState MenuList#,On|Off - enable or disable every menu of the
 * list.
 *
 * Sets or clears MENUENABLED of each menu; the list is in a3.  The
 * original changes d7, a3 and keeps d0-d1/a0-a1.
 *
 * @param on d1: On (<> 0) or Off (0).
 */
static void BLITZ_ROUTINE domenustate_c(UWORD on BL_REG(d1))
{
    UBYTE *p = blitz_string;

    while (P(p, 0)) {
        p = P(p, 0);
        changestate(p, on, 0);
    }
}

/**
 * @brief MenuState MenuList#,Menu,On|Off - enable or disable a menu.
 *
 * BUG(orig) (#216): changes bit 4 of the Menu flags (changestate2, as for
 * items) instead of MENUENABLED: no effect.
 *
 * Errors (debugger only, statechk2): "Illegal Menu number".
 *
 * @param m d1: the menu number.
 * @param on d2: On (<> 0) or Off (0).
 */
static void BLITZ_ROUTINE domenustate2_c(UWORD m BL_REG(d1), UWORD on BL_REG(d2))
{
    changestate(walk(blitz_string, m), on, 4);
}

/**
 * @brief MenuState MenuList#,Menu,Item,On|Off - enable or disable an item.
 *
 * Sets or clears ITEMENABLED.
 *
 * Errors (debugger only, statechk3): "Illegal Menu number", "Illegal Menu
 * Item number".  Without the debugger wrong numbers walk over the end of
 * the lists (#416).
 *
 * @param m d1: the menu number.
 * @param i d2: the item number.
 * @param on d3: On (<> 0) or Off (0).
 */
static void BLITZ_ROUTINE domenustate3_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                         UWORD on BL_REG(d3))
{
    UBYTE *p = walk(blitz_string, m) + M_FIRST;

    changestate(walk(p, i), on, 4);
}

/**
 * @brief MenuState MenuList#,Menu,Item,Subitem,On|Off - enable or disable
 * a subitem.
 *
 * Sets or clears ITEMENABLED.
 *
 * Errors (debugger only, statechk4): "Illegal Menu number", "Illegal Menu
 * Item number", "Illegal Menu Sub Item number".
 *
 * @param m d1: the menu number.
 * @param i d2: the item number.
 * @param s d3: the subitem number.
 * @param on d4: On (<> 0) or Off (0).
 */
static void BLITZ_ROUTINE domenustate4_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                         UWORD s BL_REG(d3), UWORD on BL_REG(d4))
{
    UBYTE *p = walk(blitz_string, m) + M_FIRST;

    p = walk(p, i) + I_SUB;
    changestate(walk(p, s), on, 4);
}
/* the original changes d7, a3 and keeps d0-d1/a0-a1 */
BLITZ_THUNK(menuslib_domenustate, domenustate_c, "d0-d1/a0-a1");
BLITZ_THUNK(menuslib_domenustate2, domenustate2_c, "d0-d1/a0-a1");
BLITZ_THUNK(menuslib_domenustate3, domenustate3_c, "d0-d1/a0-a1");
BLITZ_THUNK(menuslib_domenustate4, domenustate4_c, "d0-d1/a0-a1");

/* checked: BTST #0,12(a2):SNE d0:EXT d0 - upper word of d0 stays */
static ULONG checked(ULONG hiword, const UBYTE *p)
{
    return (hiword & 0xffff0000UL) | ((B(p, FLAGS_HI) & 1) ? 0xffff : 0);
}

/**
 * @brief MenuChecked(MenuList#,Menu,Item) - whether a toggle item is
 * checked.
 *
 * checked: BTST #0,12(a2):SNE d0:EXT d0 (CHECKED).  Not a bug (word
 * result, the compiler uses only d0.w): the upper word of d0 stays.
 *
 * Errors (debugger only, chkchk3): "Illegal Menu number", "Illegal Menu
 * Item number".
 *
 * @param hiword d0: the caller's d0 (its upper word stays).
 * @param p a2: the list.
 * @param m d1: the menu number.
 * @param i d2: the item number.
 * @return d0.w: -1 if checked, else 0.
 */
static ULONG BLITZ_ROUTINE domenuchecked_c(ULONG hiword BL_REG(d0), UBYTE *p BL_REG(a2),
                                           UWORD m BL_REG(d1), UWORD i BL_REG(d2))
{
    p = walk(p, m) + M_FIRST;
    return checked(hiword, walk(p, i));
}
BLITZ_THUNK(menuslib_domenuchecked, domenuchecked_c, "d1/a0-a1");

/**
 * @brief MenuChecked(MenuList#,Menu,Item,Subitem) - whether a toggle
 * subitem is checked.
 *
 * BUG(orig) (#217): both inner DBF jump to the menu loop: only
 * (menu,item,sub) = (m,0,0) ends where it should, other values walk the
 * lists again with a counter of -1 (65536 steps).
 *
 * Errors (debugger only, chkchk4): "Illegal Menu number", "Illegal Menu
 * Item number", "Illegal Menu Sub Item number".
 *
 * @param hiword d0: the caller's d0 (its upper word stays).
 * @param p a2: the list.
 * @param m d1: the menu number.
 * @param i d2: the item number.
 * @param s d3: the subitem number.
 * @return d0.w: -1 if checked, else 0.
 */
static ULONG BLITZ_ROUTINE domenuchecked2_c(ULONG hiword BL_REG(d0), UBYTE *p BL_REG(a2),
                                            UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                            UWORD s BL_REG(d3))
{
menuloop:                                       /* loop1 */
    p = P(p, 0);
    if (m-- != 0)
        goto menuloop;
    p += M_FIRST;
    p = P(p, 0);
    if (i-- != 0)
        goto menuloop;
    p += I_SUB;
    p = P(p, 0);
    if (s-- != 0)
        goto menuloop;
    return checked(hiword, p);
}
BLITZ_THUNK(menuslib_domenuchecked2, domenuchecked2_c, "d1/a0-a1");

/* sussflags - the flags argument (popped long) -> menflags, mexclude;
   d5 = item number (for the exclusion) */
static void sussflags(ULONG flags, UWORD item)
{
    UWORD f = dmenflags;

    mexclude = 0;
    if (flags) {
        f = (UWORD)flags;
        if ((UWORD)flags < 5) {                 /* CMP #5,d7:BCC (word) */
            switch ((UWORD)flags) {
            case 1:
                f = 0x005b;
                break;
            case 3:
                f = 0x015b;
                break;
            default:                            /* 2, and 0 or 4 */
                f = (UWORD)flags == 2 ? 0x005b : 0x015b;
                mexclude = ~(1UL << (item & 31));   /* MOVEQ #-1:BCLR d5 */
                break;
            }
        }
    }
    menflags = f;
}

/* MenuGap / SubItemOff */

/**
 * @brief MenuGap X Gap,Y Gap - the space around the text or shape of items
 * and subitems created from now on.
 *
 * The item size is the text/shape size + 2 * gap; the text/image is
 * placed at (X Gap, Y Gap) in the item.
 *
 * @param x d0: the horizontal gap.
 * @param y d1: the vertical gap.
 */
static void BLITZ_ROUTINE domenugap_c(WORD x BL_REG(d0), WORD y BL_REG(d1))
{
    menuxxs = x;
    menuyxs = y;
}
BLITZ_THUNK(domenugap, domenugap_c, "d0-d1/a0-a1");

/**
 * @brief SubItemOff X Offset,Y Offset - the offset of subitems created
 * from now on, added to their position.
 *
 * @param x d0: the horizontal offset.
 * @param y d1: the vertical offset.
 */
static void BLITZ_ROUTINE dosubitemoff_c(WORD x BL_REG(d0), WORD y BL_REG(d1))
{
    subxoff = x;
    subyoff = y;
}
BLITZ_THUNK(dosubitemoff, dosubitemoff_c, "d0-d1/a0-a1");

/* setmax - a0 = address of a list head: every entry gets the largest
   width (sublists first, recursively) */
static void setmax(UBYTE *head)
{
    UBYTE *p = head;
    UWORD max = 0;

    while (P(p, 0)) {
        p = P(p, 0);
        if (L(p, I_SUB))
            setmax(p + I_SUB);
        if (max < UW(p, M_WIDTH))               /* CMP 8(a0),d1:BCC */
            max = UW(p, M_WIDTH);
    }
    for (p = head; P(p, 0); ) {
        p = P(p, 0);
        UW(p, M_WIDTH) = max;
    }
}

/**
 * @brief SetMenu Menulist# - attach the menu list to the used window
 * (Amiga mode only).
 *
 * All items of a list (and of every sublist) first get the width of the
 * widest one; an old menu strip of the window is cleared.  The list is in
 * a3.
 *
 * Errors (debugger only, setmenuchk): "Uninitialized Window",
 * "Uninitialized Menu List".
 *
 * @param win a2: the used window (object; !libs #windowslib,$1202).
 * @param intbase a6: intuition.library.
 */
void BLITZ_ROUTINE dosetmenu(struct Window **win BL_REG(a2),
                                      struct Library *intbase BL_REG(a6))
{
    UBYTE *list = blitz_string;
    UBYTE *m;

    for (m = list; P(m, 0); ) {
        m = P(m, 0);
        setmax(m + M_FIRST);
    }
    if ((*win)->MenuStrip)
        ClearMenuStrip(*win);
    SetMenuStrip(*win, (struct Menu *)P(list, 0));
}

/* getthing - a0 = list, d1 = number: walk d1+1 next pointers (SUBQ/BPL) */
static UBYTE *getthing(UBYTE *p, WORD n)
{
    do
        p = P(p, 0);
    while (--n >= 0);
    return p;
}

/* findthing - a0 = list head, d1 = number; d0 = size of an entry,
   d2 = offset of its position (size at d2+4), d3 = its size, d4 = offset
   of its sublist.  Returns the (new) entry. */
static UBYTE *findthing(UBYTE *link, WORD n, ULONG memlen, int pos, WORD size, int sub)
{
    WORD at = 0;
    UBYTE *old, *e;

    while (--n >= 0) {                          /* loop0 */
        link = P(link, 0);
        at += W(link, pos + 4);
    }
    old = P(link, 0);
    if (old) {
        /* it already exists: free it, alloc a new one, adjust the
           following ones */
        UBYTE *next = P(old, 0);
        UBYTE *sublist = P(old, sub);

        size -= W(old, pos + 4);
        if (sub != I_SUB)
            freest((ULONG)((struct Menu *)old)->MenuName);
        else if (B(old, FLAGS_LO) & 2)          /* ITEMTEXT */
            freest((ULONG)((struct item *)old)->fill.text.IText);
        globfree(old, memlen);
        e = globalloc(memlen);
        P(e, sub) = sublist;
        P(link, 0) = e;
        P(e, 0) = next;
        for (old = e; P(old, 0); ) {
            old = P(old, 0);
            W(old, pos) += size;
        }
    } else {
        e = globalloc(memlen);                  /* 'ad */
        L(e, 0) = 0;
        P(link, 0) = e;
        L(e, sub) = 0;
    }
    W(e, pos) = at;
    if (shortcut) {                             /* items only */
        ((struct MenuItem *)e)->Command = shortcut;
        ((struct MenuItem *)e)->Flags = COMMSEQ;
    }
    return e;
}

/**
 * @brief MenuTitle MenuList#,Menu,Title$ - create or replace a menu.
 *
 * The title is copied (maxslib allocst).  Not a bug (a limitation
 * acknowledged in the original source): width = 8 * length + 8 ("8 wide
 * font, for now"); the menu is placed behind the ones before it (+2), a
 * replaced menu moves the following ones by the width difference.  The
 * list is in a3.
 *
 * Errors (debugger only, menutitlechk): "Illegal Menu number" (the menu
 * may be one behind the last).
 *
 * @param m d1: the menu number.
 * @param text d2: the title's characters.
 * @param lens a2: the stack pointer of the arguments; the title's length
 *        is the long at -4(a2) (MOVE.l -(a2),d1).
 */
void BLITZ_ROUTINE domenutitle(WORD m BL_REG(d1), const UBYTE *text BL_REG(d2),
                                        const ULONG *lens BL_REG(a2))
{
    ULONG len = lens[-1];                       /* MOVE.l -(a2),d1 */
    WORD w = ((UWORD)len << 3) + 8;             /* 8 wide font, for now */
    UBYTE *title = allocst(text, len);
    struct Menu *e;

    shortcut = 0;
    e = (struct Menu *)findthing(blitz_string, m, MENUSIZE, M_LEFT, w, M_FIRST);
    e->LeftEdge += 2;
    e->MenuName = (CONST_STRPTR)title;
    e->Width = w;
    e->Height = 10;
    e->Flags = MENUENABLED;
}

/* finditem - findthing for an item (54 bytes, TopEdge, SubItem); sets
   size, ItemFill = the IntuiText/Image behind it, MutualExclude.  `size`
   = d3 for moving the following entries: the height, but BUG(orig) (#220) the
   subitem number for SubItem (MOVEM restores d3 after textcalc) */
static struct item *finditem(UBYTE *head, WORD n, WORD width, WORD height, WORD size)
{
    struct item *e = (struct item *)findthing(head, n, ITEMSIZE, I_TOP, size, I_SUB);

    e->item.Width = width;
    e->item.Height = height;
    e->item.ItemFill = &e->fill;
    e->item.MutualExclude = mexclude;
    return e;
}

/* fontcalc - a1 = intuifontlib's value: *a1 = the TextAttr */
static void fontcalc(struct TextAttr **font)
{
    fontattr = *font;
    fonthite = (*font)->ta_YSize;
}

/* text items: d3 = height, d5 = width, d7 = the text */
struct textsize { WORD width, height; UBYTE *text; };

/* textcalc - flags and text length from the stack, d5 = item number */
static struct textsize textcalc(const ULONG **sp, const UBYTE *s, UWORD item,
                                struct Library *intbase)
{
    struct textsize extent;
    ULONG len;

    sussflags(BL_POPL(*sp), item);
    extent.height = menuyxs + menuyxs + fonthite;
    len = BL_POPL(*sp);
    /* BUG(orig) (#981): allocst returns 0 for a text of length 0; the 0
       becomes the IntuiText's IText and IntuiTextLength reads the string
       at address 0 (the item gets that width). MenuTitle (domenutitle) also
       stores MenuName 0 for an empty title, but computes its width from
       the length alone. */
    extent.text = allocst(s, len);
    dumintuitext.IText = extent.text;
    extent.width = (WORD)IntuiTextLength(&dumintuitext) + menuxxs + menuxxs;
    return extent;
}

/* menucont - the item with its IntuiText */
static struct item *menucont(UBYTE *head, WORD n, struct textsize extent, WORD size)
{
    struct item *e = finditem(head, n, extent.width, extent.height, size);
    struct IntuiText *it = &e->fill.text;

    it->ITextFont = fontattr;
    it->IText = extent.text;
    *(UWORD *)&it->FrontPen = *(UWORD *)menupens;   /* FrontPen, BackPen */
    it->DrawMode = menumode;
    it->LeftEdge = menuxxs;
    it->TopEdge = menuyxs;
    e->item.Flags |= menflags | ITEMTEXT;
    return e;
}

/* MenuItem l,flags,m,i,text$[,shortcut$] */
static void menuitem(WORD m, WORD i, const UBYTE *text, struct TextAttr **font,
                     const ULONG *lens, struct Library *intbase)
{
    struct textsize extent;

    fontcalc(font);
    extent = textcalc(&lens, text, i, intbase);
    menucont(getthing(blitz_string, m) + M_FIRST, i, extent, extent.height);
}

/**
 * @brief MenuItem MenuList#,Flags,Menu,Item,Itemtext$ - create or replace
 * a text item.
 *
 * The text is copied (allocst); the item gets the size of the text in the
 * current IntuiFont (IntuiTextLength) + 2 * MenuGap, the pens of
 * MenuColour, and is placed below the items before it; a replaced item
 * moves the following ones by the height difference.  Flags: 0 = default
 * ($52), 1 = toggle ($5B), 2 = toggle + exclude the others, 3 = checked
 * toggle ($15B), 4 = 3 + exclude, >= 5 = the flags themselves.  The list
 * is in a3.
 *
 * BUG(orig) (#219): the flag test is a word compare after a long test:
 * $10000 gives word 0 < 5 and ends in the "4" branch.
 *
 * Errors (debugger only, itemchk2): "Uninitialized IntuiFont", "Illegal
 * Menu number", "Illegal Menu Item number" (the item may be one behind
 * the last).  Without the debugger wrong numbers walk over the end of the
 * lists (#416).
 *
 * @param m d1: the menu number.
 * @param i d2: the item number.
 * @param text d3: the text's characters.
 * @param font a1: intuifontlib's value, the used IntuiFont (*a1 = the
 *        TextAttr).
 * @param lens a2: the stack pointer of the arguments: the pushed Flags
 *        long and the text length are read with -(a2).
 * @param intbase a6: intuition.library.
 */
void BLITZ_ROUTINE domenuitem(WORD m BL_REG(d1), WORD i BL_REG(d2),
                                       const UBYTE *text BL_REG(d3),
                                       struct TextAttr **font BL_REG(a1),
                                       const ULONG *lens BL_REG(a2),
                                       struct Library *intbase BL_REG(a6))
{
    shortcut = 0;
    menuitem(m, i, text, font, lens, intbase);
}

/**
 * @brief MenuItem MenuList#,Flags,Menu,Item,Itemtext$,Shortcut$ - a text
 * item with a keyboard shortcut.
 *
 * As MenuItem without a shortcut; the first character of Shortcut$
 * becomes the Command key.  Not a bug (the intended result for a new
 * entry): the shortcut sets the item's Flags to COMMSEQ in findthing, the
 * item flags are ORed in afterwards.
 *
 * @param m d1: the menu number.
 * @param i d2: the item number.
 * @param text d3: the text's characters.
 * @param key d4: the shortcut's characters.
 * @param font a1: intuifontlib's value, the used IntuiFont.
 * @param lens a2: the stack pointer of the arguments (Flags, text length).
 * @param intbase a6: intuition.library.
 */
void BLITZ_ROUTINE domenuitem2(WORD m BL_REG(d1), WORD i BL_REG(d2),
                                        const UBYTE *text BL_REG(d3),
                                        const UBYTE *key BL_REG(d4),
                                        struct TextAttr **font BL_REG(a1),
                                        const ULONG *lens BL_REG(a2),
                                        struct Library *intbase BL_REG(a6))
{
    shortcut = *key;
    menuitem(m, i, text, font, lens, intbase);
}

/* subcont - subitems are moved by SubItemOff */
static void subcont(struct item *e)
{
    e->item.LeftEdge += subxoff;
    e->item.TopEdge += subyoff;
}

/* SubItem l,flags,m,i,s,text$[,shortcut$] */
static void subitem(WORD m, WORD i, WORD s, const UBYTE *text, struct TextAttr **font,
                    const ULONG *lens, struct Library *intbase)
{
    struct textsize extent;
    UBYTE *p;

    fontcalc(font);
    extent = textcalc(&lens, text, s, intbase);
    p = getthing(blitz_string, m) + M_FIRST;
    p = getthing(p, i) + I_SUB;
    subcont(menucont(p, s, extent, s));
}

/**
 * @brief SubItem MenuList#,Flags,Menu,Item,Subitem,Subitemtext$ - create
 * or replace a text subitem.
 *
 * As MenuItem, in the item's sublist, and moved by SubItemOff.
 *
 * BUG(orig) (#220): replacing an existing subitem moves the following
 * ones by (subitem number - old height) instead of the height difference
 * (d3 is the number again after textcalc).  BUG(orig) (#219) as MenuItem.
 *
 * Errors (debugger only, subchk2): "Uninitialized IntuiFont", "Illegal
 * Menu number", "Illegal Menu Item number", "Illegal Menu Sub Item
 * number" (the subitem may be one behind the last).
 *
 * @param m d1: the menu number.
 * @param i d2: the item number.
 * @param s d3: the subitem number.
 * @param text d4: the text's characters.
 * @param font a1: intuifontlib's value, the used IntuiFont.
 * @param lens a2: the stack pointer of the arguments (Flags, text length).
 * @param intbase a6: intuition.library.
 */
void BLITZ_ROUTINE dosubitem(WORD m BL_REG(d1), WORD i BL_REG(d2), WORD s BL_REG(d3),
                                      const UBYTE *text BL_REG(d4),
                                      struct TextAttr **font BL_REG(a1),
                                      const ULONG *lens BL_REG(a2),
                                      struct Library *intbase BL_REG(a6))
{
    shortcut = 0;
    subitem(m, i, s, text, font, lens, intbase);
}

/**
 * @brief SubItem MenuList#,Flags,Menu,Item,Subitem,Subitemtext$,Shortcut$ -
 * a text subitem with a keyboard shortcut.
 *
 * As SubItem without a shortcut, the shortcut as for MenuItem.
 *
 * @param m d1: the menu number.
 * @param i d2: the item number.
 * @param s d3: the subitem number.
 * @param text d4: the text's characters.
 * @param key d5: the shortcut's characters.
 * @param font a1: intuifontlib's value, the used IntuiFont.
 * @param lens a2: the stack pointer of the arguments (Flags, text length).
 * @param intbase a6: intuition.library.
 */
void BLITZ_ROUTINE dosubitem2(WORD m BL_REG(d1), WORD i BL_REG(d2), WORD s BL_REG(d3),
                                       const UBYTE *text BL_REG(d4), const UBYTE *key BL_REG(d5),
                                       struct TextAttr **font BL_REG(a1),
                                       const ULONG *lens BL_REG(a2),
                                       struct Library *intbase BL_REG(a6))
{
    shortcut = *key;
    subitem(m, i, s, text, font, lens, intbase);
}

/* shapecalc - BUG(orig) (#218): flags = the high word of the pushed long (MOVE
   4(a7),d7 -> temp, read as long from temp-2 behind the cleared low word
   of mexclude); size = shape size + 2 * gap */
static struct textsize shapecalc(UWORD flagshi, UWORD item, const struct bb_shape *shape)
{
    struct textsize extent;

    mt.temp = flagshi;
    mexclude = 0;                               /* CLR.l mexclude in sussflags */
    sussflags(*(ULONG *)((UBYTE *)&mt.temp - 2), item);
    shortcut = 0;
    extent.width = menuxxs + menuxxs + shape->width;
    extent.height = menuyxs + menuyxs + shape->height;
    extent.text = NULL;
    return extent;
}

/* shapecont - the item with an Image of the shape */
static struct item *shapecont(UBYTE *head, WORD n, struct textsize extent,
                              const struct bb_shape *shape)
{
    struct item *e = finditem(head, n, extent.width, extent.height, extent.height);
    struct Image *im = &e->fill.image;

    im->LeftEdge = menuxxs;
    im->TopEdge = menuyxs;
    im->Width = shape->width;
    im->Height = shape->height;
    im->Depth = shape->depth;
    im->ImageData = (UWORD *)shape->data;
    *(UWORD *)&im->PlanePick = 0xffff;          /* PlanePick, PlaneOnOff */
    e->item.ItemFill = im;
    e->item.Width = extent.width;
    e->item.Height = extent.height;
    e->item.Flags = menflags & ~ITEMTEXT;       /* MOVE, not OR */
    return e;
}

/**
 * @brief ShapeItem MenuList#,Flags,Menu,Item,Shape# - create or replace an
 * item showing a shape.
 *
 * The item is an Image of the shape (its planes, PlanePick/PlaneOnOff
 * -1), size = shape size + 2 * MenuGap.
 *
 * BUG(orig) (#218): the flags are the high word of the pushed long (MOVE
 * 4(a7),d7, read as a long behind the cleared low word of mexclude):
 * small flag values give the default flags; the item Flags are set
 * (MOVE), not ORed.
 *
 * Errors (debugger only, shapeitemchk): "Uninitialized Shape", "Illegal
 * Menu number", "Illegal Menu Item number".
 *
 * @param m d1: the menu number.
 * @param i d2: the item number.
 * @param shape a2: the shape (!libs #shapeslib,$1283).
 * @param flagshi d7: the high word of the Flags long (loaded by the thunk
 *        from 8(sp)).
 */
static void BLITZ_ROUTINE doshapeitem_c(WORD m BL_REG(d1), WORD i BL_REG(d2),
                                        const struct bb_shape *shape BL_REG(a2),
                                        UWORD flagshi BL_REG(d7))
{
    struct textsize extent = shapecalc(flagshi, i, shape);

    shapecont(getthing(blitz_string, m) + M_FIRST, i, extent, shape);
}

/**
 * @brief ShapeSub MenuList#,Flags,Menu,Item,Subitem,Shape# - create or
 * replace a subitem showing a shape.
 *
 * As ShapeItem, in the item's sublist, and moved by SubItemOff.
 * BUG(orig) (#218) as ShapeItem.
 *
 * Errors (debugger only, shapesubchk): "Uninitialized Shape", "Illegal
 * Menu number", "Illegal Menu Item number", "Illegal Menu Sub Item
 * number".
 *
 * @param m d1: the menu number.
 * @param i d2: the item number.
 * @param s d3: the subitem number.
 * @param shape a2: the shape (!libs #shapeslib,$1284).
 * @param flagshi d7: the high word of the Flags long.
 */
static void BLITZ_ROUTINE doshapesub_c(WORD m BL_REG(d1), WORD i BL_REG(d2), WORD s BL_REG(d3),
                                       const struct bb_shape *shape BL_REG(a2),
                                       UWORD flagshi BL_REG(d7))
{
    struct textsize extent = shapecalc(flagshi, s, shape);
    UBYTE *p;

    p = getthing(blitz_string, m) + M_FIRST;
    p = getthing(p, i) + I_SUB;
    subcont(shapecont(p, s, extent, shape));
}
/* MOVE 4(a7),d7: the high word of the flags long above the return address */
BLITZ_THUNK_PRE(doshapeitem, doshapeitem_c, "d0", "move.w 8(sp),d7");
BLITZ_THUNK_PRE(doshapesub, doshapesub_c, "d0", "move.w 8(sp),d7");

/* freeitem - an item (54 bytes) and its text */
static void freeitem(struct MenuItem *e)
{
    if (B(e, FLAGS_LO) & 2)                     /* ITEMTEXT */
        freest((ULONG)((struct item *)e)->fill.text.IText);
    globfree(e, ITEMSIZE);
}

/**
 * @brief _free of the MenuList object (Free MenuList, program end) - free
 * every menu, item and subitem with their texts.
 *
 * The list is in a3; it is cleared first.
 */
static void BLITZ_ROUTINE freemenu(void)
{
    struct Menu *m = *(struct Menu **)blitz_string, *nm;
    struct MenuItem *it, *sub, *n;

    *(struct Menu **)blitz_string = NULL;
    while (m) {
        it = m->FirstItem;
        while (it) {
            sub = it->SubItem;
            while (sub) {
                n = sub->NextItem;
                freeitem(sub);
                sub = n;
            }
            n = it->NextItem;
            freeitem(it);
            it = n;
        }
        freest((ULONG)m->MenuName);
        nm = m->NextMenu;
        globfree(m, MENUSIZE);
        m = nm;
    }
}

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess1, "Illegal Menu number");
BLITZ_ERRCHK_MESSAGE(mess2, "Illegal Menu Item number");
BLITZ_ERRCHK_MESSAGE(mess3, "Illegal Menu Sub Item number");
BLITZ_ERRCHK_MESSAGE(mess4, "Uninitialized Shape");
BLITZ_ERRCHK_MESSAGE(mess5, "Uninitialized Window");
BLITZ_ERRCHK_MESSAGE(mess6, "Uninitialized Menu List");
BLITZ_ERRCHK_MESSAGE(mess7, "Uninitialized IntuiFont");

/* existschk - entries 0..n of the list at *a0 exist (DBF); the list
   pointer moves to entry n.  Returns 0 if one is missing. */
static int BLITZ_ERRCHK existschk(UBYTE **entry, UWORD n)
{
    UBYTE *p = *entry;

    do {
        if (!P(p, 0)) {
            *entry = p;
            return 0;
        }
        p = P(p, 0);
    } while (n-- != 0);
    *entry = p;
    return 1;
}

/* nextchk - n may be one more than the last entry */
static int BLITZ_ERRCHK nextchk(UBYTE **entry, UWORD n)
{
    if ((WORD)--n < 0)
        return 1;
    return existschk(entry, n);
}

static void BLITZ_ERRCHK setmenuchk_c(const ULONG *win BL_REG(a2))
{
    if (!*win)
        BLITZ_RUNERR(mess5);
    else if (!*(const ULONG *)blitz_string)
        BLITZ_RUNERR(mess6);
}

static void BLITZ_ERRCHK menutitlechk_c(UWORD m BL_REG(d1))
{
    UBYTE *entry = blitz_string;

    if (!nextchk(&entry, m))
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK itemchk(UBYTE *list, UWORD m, UWORD i)
{
    UBYTE *entry = list;

    if (!existschk(&entry, m)) {
        BLITZ_RUNERR(mess1);
        return;
    }
    entry += M_FIRST;
    if (!nextchk(&entry, i))
        BLITZ_RUNERR(mess2);
}

static void BLITZ_ERRCHK subchk(UBYTE *list, UWORD m, UWORD i, UWORD s)
{
    UBYTE *entry = list;

    if (!existschk(&entry, m)) {
        BLITZ_RUNERR(mess1);
        return;
    }
    entry += M_FIRST;
    if (!existschk(&entry, i)) {
        BLITZ_RUNERR(mess2);
        return;
    }
    entry += I_SUB;
    if (!nextchk(&entry, s))
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK itemchk2_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                    const struct bb_intuifont *font BL_REG(a1))
{
    if (!font->font)
        BLITZ_RUNERR(mess7);
    else
        itemchk(blitz_string, m, i);
}

static void BLITZ_ERRCHK shapeitemchk_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                        const struct bb_shape *shape BL_REG(a2))
{
    if (!shape->width)
        BLITZ_RUNERR(mess4);
    else
        itemchk(blitz_string, m, i);
}

static void BLITZ_ERRCHK subchk2_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2), UWORD s BL_REG(d3),
                                   const struct bb_intuifont *font BL_REG(a1))
{
    if (!font->font)
        BLITZ_RUNERR(mess7);
    else
        subchk(blitz_string, m, i, s);
}

static void BLITZ_ERRCHK shapesubchk_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                       UWORD s BL_REG(d3), const struct bb_shape *shape BL_REG(a2))
{
    if (!shape->width)
        BLITZ_RUNERR(mess4);
    else
        subchk(blitz_string, m, i, s);
}

/* statechk2/3/4 - menu (item (subitem)) must exist; list in a3, or a2
   for MenuChecked (chkchk3/4: EXG a2,a3) */
static int BLITZ_ERRCHK statechk(UBYTE *list, int depth, UWORD m, UWORD i, UWORD s)
{
    UBYTE *entry = list;

    if (!existschk(&entry, m)) {
        BLITZ_RUNERR(mess1);
        return 0;
    }
    if (depth < 3)
        return 1;
    entry += M_FIRST;
    if (!existschk(&entry, i)) {
        BLITZ_RUNERR(mess2);
        return 0;
    }
    if (depth < 4)
        return 1;
    entry += I_SUB;
    if (!existschk(&entry, s)) {
        BLITZ_RUNERR(mess3);
        return 0;
    }
    return 1;
}

static void BLITZ_ERRCHK statechk2_c(UWORD m BL_REG(d1))
{
    statechk(blitz_string, 2, m, 0, 0);
}

static void BLITZ_ERRCHK statechk3_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2))
{
    statechk(blitz_string, 3, m, i, 0);
}

static void BLITZ_ERRCHK statechk4_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2), UWORD s BL_REG(d3))
{
    statechk(blitz_string, 4, m, i, s);
}

static void BLITZ_ERRCHK chkchk3_c(UBYTE *list BL_REG(a2), UWORD m BL_REG(d1), UWORD i BL_REG(d2))
{
    statechk(list, 3, m, i, 0);
}

static void BLITZ_ERRCHK chkchk4_c(UBYTE *list BL_REG(a2), UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                   UWORD s BL_REG(d3))
{
    statechk(list, 4, m, i, s);
}

/* the checks change a0/d7 (and the low word of d0) in the original; here
   d0-d1/a0-a1 are kept */
BLITZ_ERRCHK_THUNK(setmenuchk, setmenuchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(menutitlechk, menutitlechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(itemchk2, itemchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(shapeitemchk, shapeitemchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(subchk2, subchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(shapesubchk, shapesubchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(menuslib_statechk2, statechk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(menuslib_statechk3, statechk3_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(menuslib_statechk4, statechk4_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(menuslib_chkchk3, chkchk3_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(menuslib_chkchk4, chkchk4_c, "d0-d1/a0-a1");
