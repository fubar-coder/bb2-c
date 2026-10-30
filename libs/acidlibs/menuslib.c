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
#include <intuition/intuition.h>
#define __NOLIBBASE__
#define INTUITION_BASE_NAME intbase
#include <proto/intuition.h>
#undef __NOLIBBASE__

/* maxslib: string copies in global memory */

/* entry points (thunks / C, defined below) */
void doshapeitem(void), doshapesub(void);
void domenugap(void), dosubitemoff(void), domenustate(void), domenustate2(void);
void domenustate3(void), domenustate4(void), domenupens(void);
void domenuchecked(void), domenuchecked2(void);
void menutitlechk(void), setmenuchk(void), itemchk2(void), subchk2(void);
void shapeitemchk(void), shapesubchk(void), statechk2(void), statechk3(void);
void statechk4(void), chkchk3(void), chkchk4(void);
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
void BLITZ_ROUTINE freemenu(void);

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
BLITZ_SUBS(domenustate, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1380);
BLITZ_SUBS(domenustate2, statechk2, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1380);
BLITZ_SUBS(domenustate3, statechk3, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1380);
BLITZ_SUBS(domenustate4, statechk4, 0);
BLITZ_NAME("MenuState", "MenuList#[,Menu[,Item[,Subitem]]],On|Off");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(domenupens, 0, 0);
BLITZ_NAME("MenuColour", "Colour");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1280);
BLITZ_SUBS(domenuchecked, chkchk3, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_menuslib, 0x1280);
BLITZ_SUBS(domenuchecked2, chkchk4, 0);
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

/* struct Menu (30) / struct MenuItem (34) + IntuiText/Image (20) as the
   original sees them: offsets */
#define M_NEXT      0
#define M_LEFT      4
#define M_WIDTH     8
#define M_HEIGHT    10
#define M_FLAGS     12
#define M_NAME      14          /* Menu: MenuName */
#define M_FIRST     18          /* Menu: FirstItem / Item: ItemFill */
#define I_MUTEX     14
#define I_FILL      18
#define I_COMMAND   26
#define I_SUB       28
#define I_NEXTSEL   32
#define I_TEXT      34          /* the IntuiText / Image behind the item */
#define MENUSIZE    30
#define ITEMSIZE    54

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

/* MenuColour - MOVE.b d0,menufpen */
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
        B(p, 13) |= 1 << bit;
    else
        B(p, 13) &= ~(1 << bit);
}

/* walk n (DBF: n+1 times) next pointers */
static UBYTE *walk(UBYTE *p, UWORD n)
{
    do
        p = P(p, 0);
    while (n-- != 0);
    return p;
}

/* MenuState l,onoff - every menu of the list (a3) */
static void BLITZ_ROUTINE domenustate_c(UWORD on BL_REG(d1))
{
    UBYTE *p = blitz_string;

    while (P(p, 0)) {
        p = P(p, 0);
        changestate(p, on, 0);
    }
}

/* MenuState l,m,onoff - BUG(orig) (#216): bit 4 of a Menu */
static void BLITZ_ROUTINE domenustate2_c(UWORD m BL_REG(d1), UWORD on BL_REG(d2))
{
    changestate(walk(blitz_string, m), on, 4);
}

/* MenuState l,m,i,onoff */
static void BLITZ_ROUTINE domenustate3_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                         UWORD on BL_REG(d3))
{
    UBYTE *p = walk(blitz_string, m) + M_FIRST;

    changestate(walk(p, i), on, 4);
}

/* MenuState l,m,i,s,onoff */
static void BLITZ_ROUTINE domenustate4_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                         UWORD s BL_REG(d3), UWORD on BL_REG(d4))
{
    UBYTE *p = walk(blitz_string, m) + M_FIRST;

    p = walk(p, i) + I_SUB;
    changestate(walk(p, s), on, 4);
}
/* the original changes d7, a3 and keeps d0-d1/a0-a1 */
BLITZ_THUNK(domenustate, domenustate_c, "d0-d1/a0-a1");
BLITZ_THUNK(domenustate2, domenustate2_c, "d0-d1/a0-a1");
BLITZ_THUNK(domenustate3, domenustate3_c, "d0-d1/a0-a1");
BLITZ_THUNK(domenustate4, domenustate4_c, "d0-d1/a0-a1");

/* checked: BTST #0,12(a2):SNE d0:EXT d0 - upper word of d0 stays */
static ULONG checked(ULONG d0, const UBYTE *p)
{
    return (d0 & 0xffff0000UL) | ((B(p, 12) & 1) ? 0xffff : 0);
}

/* MenuChecked(l,m,i) - a2 = the list */
static ULONG BLITZ_ROUTINE domenuchecked_c(ULONG d0 BL_REG(d0), UBYTE *p BL_REG(a2),
                                           UWORD m BL_REG(d1), UWORD i BL_REG(d2))
{
    p = walk(p, m) + M_FIRST;
    return checked(d0, walk(p, i));
}
BLITZ_THUNK(domenuchecked, domenuchecked_c, "d1/a0-a1");

/* MenuChecked(l,m,i,s) - BUG(orig) (#217): both inner DBF jump to the menu loop */
static ULONG BLITZ_ROUTINE domenuchecked2_c(ULONG d0 BL_REG(d0), UBYTE *p BL_REG(a2),
                                            UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                            UWORD s BL_REG(d3))
{
loop1:
    p = P(p, 0);
    if (m-- != 0)
        goto loop1;
    p += M_FIRST;
    p = P(p, 0);
    if (i-- != 0)
        goto loop1;
    p += I_SUB;
    p = P(p, 0);
    if (s-- != 0)
        goto loop1;
    return checked(d0, p);
}
BLITZ_THUNK(domenuchecked2, domenuchecked2_c, "d1/a0-a1");

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
static void BLITZ_ROUTINE domenugap_c(WORD x BL_REG(d0), WORD y BL_REG(d1))
{
    menuxxs = x;
    menuyxs = y;
}
BLITZ_THUNK(domenugap, domenugap_c, "d0-d1/a0-a1");

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

/* SetMenu - a2 = used window (object), a3 = the list, a6 = intuition */
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
static UBYTE *findthing(UBYTE *a0, WORD n, ULONG memlen, int pos, WORD size, int sub)
{
    WORD at = 0;
    UBYTE *old, *e;

    while (--n >= 0) {                          /* loop0 */
        a0 = P(a0, 0);
        at += W(a0, pos + 4);
    }
    old = P(a0, 0);
    if (old) {
        /* it already exists: free it, alloc a new one, adjust the
           following ones */
        UBYTE *next = P(old, 0);
        UBYTE *sublist = P(old, sub);

        size -= W(old, pos + 4);
        if (sub != I_SUB)
            freest(L(old, M_NAME));
        else if (B(old, 13) & 2)                /* ITEMTEXT */
            freest(L(old, I_TEXT + 12));
        globfree(old, memlen);
        e = globalloc(memlen);
        P(e, sub) = sublist;
        P(a0, 0) = e;
        P(e, 0) = next;
        for (old = e; P(old, 0); ) {
            old = P(old, 0);
            W(old, pos) += size;
        }
    } else {
        e = globalloc(memlen);                  /* 'ad */
        L(e, 0) = 0;
        P(a0, 0) = e;
        L(e, sub) = 0;
    }
    W(e, pos) = at;
    if (shortcut) {
        B(e, I_COMMAND) = shortcut;
        UW(e, M_FLAGS) = 4;
    }
    return e;
}

/* MenuTitle l,m,title$ - a3 = list, d1 = menu, d2 = text */
void BLITZ_ROUTINE domenutitle(WORD m BL_REG(d1), const UBYTE *text BL_REG(d2),
                                        const ULONG *lens BL_REG(a2))
{
    ULONG len = lens[-1];                       /* MOVE.l -(a2),d1 */
    WORD w = ((UWORD)len << 3) + 8;             /* 8 wide font, for now */
    UBYTE *t = allocst(text, len);
    UBYTE *e;

    shortcut = 0;
    e = findthing(blitz_string, m, MENUSIZE, M_LEFT, w, M_FIRST);
    W(e, M_LEFT) += 2;
    P(e, M_NAME) = t;
    W(e, M_WIDTH) = w;
    W(e, M_HEIGHT) = 10;
    W(e, M_FLAGS) = 1;
}

/* finditem - findthing for an item (54 bytes, TopEdge, SubItem); sets
   size, ItemFill = the IntuiText/Image behind it, MutualExclude.  `size`
   = d3 for moving the following entries: the height, but BUG(orig) (#220) the
   subitem number for SubItem (MOVEM restores d3 after textcalc) */
static UBYTE *finditem(UBYTE *head, WORD n, WORD width, WORD height, WORD size)
{
    UBYTE *e = findthing(head, n, ITEMSIZE, 6, size, I_SUB);

    W(e, M_WIDTH) = width;
    W(e, M_HEIGHT) = height;
    P(e, I_FILL) = e + I_TEXT;
    L(e, I_MUTEX) = mexclude;
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
    struct textsize t;
    ULONG len;

    sussflags(BL_POPL(*sp), item);
    t.height = menuyxs + menuyxs + fonthite;
    len = BL_POPL(*sp);
    t.text = allocst(s, len);
    dumintuitext.IText = t.text;
    t.width = (WORD)IntuiTextLength(&dumintuitext) + menuxxs + menuxxs;
    return t;
}

/* menucont - the item with its IntuiText */
static UBYTE *menucont(UBYTE *head, WORD n, struct textsize t, WORD size)
{
    UBYTE *e = finditem(head, n, t.width, t.height, size);
    UBYTE *it = e + I_TEXT;

    L(it, 8) = (ULONG)fontattr;                 /* ITextFont */
    P(it, 12) = t.text;                         /* IText */
    UW(it, 0) = *(UWORD *)menupens;             /* FrontPen, BackPen */
    B(it, 2) = menumode;                        /* DrawMode */
    W(it, 4) = menuxxs;
    W(it, 6) = menuyxs;
    UW(e, M_FLAGS) |= menflags | 2;             /* ITEMTEXT */
    return e;
}

/* MenuItem l,flags,m,i,text$[,shortcut$] */
static void menuitem(WORD m, WORD i, const UBYTE *text, struct TextAttr **font,
                     const ULONG *lens, struct Library *intbase)
{
    struct textsize t;

    fontcalc(font);
    t = textcalc(&lens, text, i, intbase);
    menucont(getthing(blitz_string, m) + M_FIRST, i, t, t.height);
}

void BLITZ_ROUTINE domenuitem(WORD m BL_REG(d1), WORD i BL_REG(d2),
                                       const UBYTE *text BL_REG(d3),
                                       struct TextAttr **font BL_REG(a1),
                                       const ULONG *lens BL_REG(a2),
                                       struct Library *intbase BL_REG(a6))
{
    shortcut = 0;
    menuitem(m, i, text, font, lens, intbase);
}

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
static void subcont(UBYTE *e)
{
    W(e, 4) += subxoff;
    W(e, 6) += subyoff;
}

/* SubItem l,flags,m,i,s,text$[,shortcut$] */
static void subitem(WORD m, WORD i, WORD s, const UBYTE *text, struct TextAttr **font,
                    const ULONG *lens, struct Library *intbase)
{
    struct textsize t;
    UBYTE *p;

    fontcalc(font);
    t = textcalc(&lens, text, s, intbase);
    p = getthing(blitz_string, m) + M_FIRST;
    p = getthing(p, i) + I_SUB;
    subcont(menucont(p, s, t, s));
}

void BLITZ_ROUTINE dosubitem(WORD m BL_REG(d1), WORD i BL_REG(d2), WORD s BL_REG(d3),
                                      const UBYTE *text BL_REG(d4),
                                      struct TextAttr **font BL_REG(a1),
                                      const ULONG *lens BL_REG(a2),
                                      struct Library *intbase BL_REG(a6))
{
    shortcut = 0;
    subitem(m, i, s, text, font, lens, intbase);
}

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
static struct textsize shapecalc(UWORD flagshi, UWORD item, const UWORD *shape)
{
    struct textsize t;

    mt.temp = flagshi;
    mexclude = 0;                               /* CLR.l mexclude in sussflags */
    sussflags(*(ULONG *)((UBYTE *)&mt.temp - 2), item);
    shortcut = 0;
    t.width = menuxxs + menuxxs + shape[0];
    t.height = menuyxs + menuyxs + shape[1];
    t.text = NULL;
    return t;
}

/* shapecont - the item with an Image of the shape */
static UBYTE *shapecont(UBYTE *head, WORD n, struct textsize t, const UBYTE *shape)
{
    UBYTE *e = finditem(head, n, t.width, t.height, t.height);
    UBYTE *im = e + I_TEXT;

    W(im, 0) = menuxxs;
    W(im, 2) = menuyxs;
    UW(im, 4) = UW(shape, 0);                   /* width */
    UW(im, 6) = UW(shape, 2);                   /* height */
    UW(im, 8) = UW(shape, 4);                   /* depth */
    L(im, 10) = L(shape, 14);                   /* data */
    W(im, 14) = -1;                             /* PlanePick, PlaneOnOff */
    P(e, I_FILL) = im;
    W(e, M_WIDTH) = t.width;
    W(e, M_HEIGHT) = t.height;
    UW(e, M_FLAGS) = menflags & ~2;             /* MOVE, not OR */
    return e;
}

/* ShapeItem l,flags,m,i,shape# - a2 = the shape; flags on the stack */
static void BLITZ_ROUTINE doshapeitem_c(WORD m BL_REG(d1), WORD i BL_REG(d2),
                                        const UBYTE *shape BL_REG(a2), UWORD flagshi BL_REG(d7))
{
    struct textsize t = shapecalc(flagshi, i, (const UWORD *)shape);

    shapecont(getthing(blitz_string, m) + M_FIRST, i, t, shape);
}

/* ShapeSub l,flags,m,i,s,shape# */
static void BLITZ_ROUTINE doshapesub_c(WORD m BL_REG(d1), WORD i BL_REG(d2), WORD s BL_REG(d3),
                                       const UBYTE *shape BL_REG(a2), UWORD flagshi BL_REG(d7))
{
    struct textsize t = shapecalc(flagshi, s, (const UWORD *)shape);
    UBYTE *p;

    p = getthing(blitz_string, m) + M_FIRST;
    p = getthing(p, i) + I_SUB;
    subcont(shapecont(p, s, t, shape));
}
/* MOVE 4(a7),d7: the high word of the flags long above the return address */
BLITZ_THUNK_PRE(doshapeitem, doshapeitem_c, "d0", "move.w 8(sp),d7");
BLITZ_THUNK_PRE(doshapesub, doshapesub_c, "d0", "move.w 8(sp),d7");

/* freeitem - an item (54 bytes) and its text */
static void freeitem(UBYTE *e)
{
    if (B(e, 13) & 2)
        freest(L(e, I_TEXT + 12));
    globfree(e, ITEMSIZE);
}

/* freemenu - the list in a3: every menu, item, subitem */
void BLITZ_ROUTINE freemenu(void)
{
    UBYTE *m = P(blitz_string, 0), *it, *sub, *n;

    L(blitz_string, 0) = 0;
    while (m) {
        it = P(m, M_FIRST);
        while (it) {
            sub = P(it, I_SUB);
            while (sub) {
                n = P(sub, 0);
                freeitem(sub);
                sub = n;
            }
            n = P(it, 0);
            freeitem(it);
            it = n;
        }
        freest(L(m, M_NAME));
        n = P(m, 0);
        globfree(m, MENUSIZE);
        m = n;
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
static int BLITZ_ERRCHK existschk(UBYTE **a0, UWORD n)
{
    UBYTE *p = *a0;

    do {
        if (!P(p, 0)) {
            *a0 = p;
            return 0;
        }
        p = P(p, 0);
    } while (n-- != 0);
    *a0 = p;
    return 1;
}

/* nextchk - n may be one more than the last entry */
static int BLITZ_ERRCHK nextchk(UBYTE **a0, UWORD n)
{
    if ((WORD)--n < 0)
        return 1;
    return existschk(a0, n);
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
    UBYTE *a0 = blitz_string;

    if (!nextchk(&a0, m))
        BLITZ_RUNERR(mess1);
}

static void BLITZ_ERRCHK itemchk(UBYTE *list, UWORD m, UWORD i)
{
    UBYTE *a0 = list;

    if (!existschk(&a0, m)) {
        BLITZ_RUNERR(mess1);
        return;
    }
    a0 += M_FIRST;
    if (!nextchk(&a0, i))
        BLITZ_RUNERR(mess2);
}

static void BLITZ_ERRCHK subchk(UBYTE *list, UWORD m, UWORD i, UWORD s)
{
    UBYTE *a0 = list;

    if (!existschk(&a0, m)) {
        BLITZ_RUNERR(mess1);
        return;
    }
    a0 += M_FIRST;
    if (!existschk(&a0, i)) {
        BLITZ_RUNERR(mess2);
        return;
    }
    a0 += I_SUB;
    if (!nextchk(&a0, s))
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK itemchk2_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                    const UBYTE *font BL_REG(a1))
{
    if (!L(font, 8))
        BLITZ_RUNERR(mess7);
    else
        itemchk(blitz_string, m, i);
}

static void BLITZ_ERRCHK shapeitemchk_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                        const UWORD *shape BL_REG(a2))
{
    if (!*shape)
        BLITZ_RUNERR(mess4);
    else
        itemchk(blitz_string, m, i);
}

static void BLITZ_ERRCHK subchk2_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2), UWORD s BL_REG(d3),
                                   const UBYTE *font BL_REG(a1))
{
    if (!L(font, 8))
        BLITZ_RUNERR(mess7);
    else
        subchk(blitz_string, m, i, s);
}

static void BLITZ_ERRCHK shapesubchk_c(UWORD m BL_REG(d1), UWORD i BL_REG(d2),
                                       UWORD s BL_REG(d3), const UWORD *shape BL_REG(a2))
{
    if (!*shape)
        BLITZ_RUNERR(mess4);
    else
        subchk(blitz_string, m, i, s);
}

/* statechk2/3/4 - menu (item (subitem)) must exist; list in a3, or a2
   for MenuChecked (chkchk3/4: EXG a2,a3) */
static int BLITZ_ERRCHK statechk(UBYTE *list, int depth, UWORD m, UWORD i, UWORD s)
{
    UBYTE *a0 = list;

    if (!existschk(&a0, m)) {
        BLITZ_RUNERR(mess1);
        return 0;
    }
    if (depth < 3)
        return 1;
    a0 += M_FIRST;
    if (!existschk(&a0, i)) {
        BLITZ_RUNERR(mess2);
        return 0;
    }
    if (depth < 4)
        return 1;
    a0 += I_SUB;
    if (!existschk(&a0, s)) {
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
BLITZ_ERRCHK_THUNK(statechk2, statechk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(statechk3, statechk3_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(statechk4, statechk4_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(chkchk3, chkchk3_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(chkchk4, chkchk4_c, "d0-d1/a0-a1");
