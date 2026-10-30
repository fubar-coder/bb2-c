/*
 * linklistlib - command library 128: the commands for `Dim List` arrays
 * (FirstItem, AddItem, NextItem, ResetList, KillItem, AddFirst, ClearList,
 * AddLast, PushItem, PopItem, PrevItem, LastItem, ItemStackSize,
 * SortList).
 *
 * C translation of bblibsrc/linklistlib.  The CD binary is OLDER than the
 * source: the source has three more tokens at the end (SetItem, CountList,
 * CountItem) that the binary does not contain; this file follows the
 * binary (the code of the other tokens is the same as in the source).
 *
 * From the original:
 *   20/8/93
 *   added SortList command to sort lists on the first long in item
 *   really pleased with this one (see isometric demo)
 *
 *   list format is as follows (offsets from the array base, arrayslib
 *   newlist):
 *   -32.l : current item, 0 If none
 *   -28.l : List head node of in use   (lh_Head, lh_Tail = 0, lh_TailPred)
 *   -16.l : List head of free          (lh_Head, lh_Tail = 0, lh_TailPred)
 *   -04.l : "lIsT"
 * Every item is a node {next, prev} followed by the element (the array
 * base points behind the list header).  The list headers are used as
 * nodes themselves: &lh_Head is a node whose prev is lh_Tail (0), &lh_Tail
 * a node whose next is 0.
 *
 * The routines that change a list disable interrupts (INTENA $4000) and
 * enable them again ($C000) like the original.
 *
 * Item stack (PushItem/PopItem): a block of ItemStackSize*8 bytes from
 * memlib (globalloc), entries {array base, current item}; init allocates
 * 8 entries.  Without the debugger there is no check at all.
 *
 * DIFF: the original SortList destroys a3 (string pointer) and d6; here
 * a3 is never changed (Blitz programs do not expect a3 to survive a
 * statement, so nothing observable).
 */
#include "blitzlib.h"

/* entry points (defined below) */
ULONG BLITZ_ROUTINE dofirstitem(UBYTE *base BL_REG(d0));
ULONG BLITZ_ROUTINE doadditem(UBYTE *base BL_REG(d0));
ULONG BLITZ_ROUTINE donextitem(UBYTE *base BL_REG(d0));
void BLITZ_ROUTINE doreslist(UBYTE *base BL_REG(d0));
void BLITZ_ROUTINE dokillitem(UBYTE *base BL_REG(d0));
ULONG BLITZ_ROUTINE doaddfirst(UBYTE *base BL_REG(d0));
void BLITZ_ROUTINE doclearlist(UBYTE *base BL_REG(d0));
ULONG BLITZ_ROUTINE doaddlast(UBYTE *base BL_REG(d0));
void BLITZ_ROUTINE dopushcurrent(UBYTE *base BL_REG(d0));
void BLITZ_ROUTINE dopopcurrent(UBYTE *base BL_REG(d0));
ULONG BLITZ_ROUTINE doprevitem(UBYTE *base BL_REG(d0));
ULONG BLITZ_ROUTINE dolastitem(UBYTE *base BL_REG(d0));
void BLITZ_ROUTINE setstack(ULONG n BL_REG(d0));
void BLITZ_ROUTINE dosortlist(UBYTE *base BL_REG(d0), WORD offset BL_REG(d1));
void BLITZ_ROUTINE doinit(void);
/* error checks (debugger only, thunks below) */
void lchk(void), lchk2(void), pushchk(void), popchk(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#linklistlib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_linklistlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(dofirstitem, lchk, 0);
BLITZ_NAME("FirstItem", "(Arrayname())");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(doadditem, lchk, 0);
BLITZ_NAME("AddItem", "(Arrayname())");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(donextitem, lchk, 0);
BLITZ_NAME("NextItem", "(Arrayname())");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(doreslist, lchk, 0);
BLITZ_NAME("ResetList", "Arrayname()");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(dokillitem, lchk2, 0);
BLITZ_NAME("KillItem", "Arrayname()");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(doaddfirst, lchk, 0);
BLITZ_NAME("AddFirst", "(Arrayname())");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(doclearlist, lchk, 0);
BLITZ_NAME("ClearList", "Arrayname()");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(doaddlast, lchk, 0);
BLITZ_NAME("AddLast", "(Arrayname())");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(dopushcurrent, pushchk, 0);
BLITZ_NAME("PushItem", "Arrayname()");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(dopopcurrent, popchk, 0);
BLITZ_NAME("PopItem", "Arrayname()");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(doprevitem, lchk, 0);
BLITZ_NAME("PrevItem", "Arrayname()");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(dolastitem, lchk, 0);
BLITZ_NAME("LastItem", "Arrayname()");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(setstack, 0, 0);
BLITZ_NAME("ItemStackSize", "Max Items");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_ARRAY, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dosortlist, lchk, 0);
BLITZ_NAME("SortList", "Arrayname(),sizeof .type\\offset");

/* only in the source, not in the CD binary:
   SetItem (dosetitem), CountList (docountlist), CountItem (docountitem) */

BLITZ_NULLSUB_AT(init, doinit, 0, 0);

BLITZ_LIBFIN();

struct item {
    struct item *next, *prev;                   /* element follows */
};

/* list header fields as offsets from the array base */
#define CURRENT(b)  (*(struct item **)((b) - 32))
#define USED(b)     ((struct item *)((b) - 28))  /* &lh_Head as node */
#define USEDTAIL(b) ((struct item *)((b) - 24))  /* &lh_Tail as node */
#define TAILPRED(b) (*(struct item **)((b) - 20))
#define FREE(b)     ((struct item *)((b) - 16))  /* &lh_Head of free */
#define LISTID(b)   (*(ULONG *)((b) - 4))


static ULONG maxstack;                          /* size in bytes */
static ULONG *currstack;
static ULONG *currsp;

/* unlinkitem - item to unlink in a1 */
static void unlinkitem(struct item *a1)
{
    struct item *a2 = a1->prev, *a3 = a1->next;

    a3->prev = a2;
    a2->next = a3;
}

/* linkitem - link item a1 after a2 */
static void linkitem(struct item *a1, struct item *a2)
{
    struct item *a3 = a2->next;

    a2->next = a1;
    a3->prev = a1;
    a1->prev = a2;
    a1->next = a3;
}

/* doinit - empty item stack of 8 entries */
void BLITZ_ROUTINE doinit(void)
{
    currstack = 0;
    currsp = 0;
    setstack(8);
}

/* setstack - ItemStackSize n: free the old stack (memlib globfree),
   allocate n*8 bytes (globalloc, MEMF_PUBLIC); the stack is empty
   afterwards */
void BLITZ_ROUTINE setstack(ULONG n BL_REG(d0))
{
    struct bl_regs r = { 0, 0, 0, 0 };

    if (currstack) {
        r.a1 = (ULONG)currstack;
        r.d0 = maxstack;
        BLITZ_ALIBJSR(TOK_globfree, r);
        currstack = 0;
    }
    maxstack = n << 3;
    r.d0 = maxstack;
    r.d1 = MEMF_PUBLIC;
    BLITZ_ALIBJSR(TOK_globalloc, r);
    currstack = (ULONG *)r.d0;
    currsp = (ULONG *)r.d0;
}

/* dopushcurrent - push {array, current item} */
void BLITZ_ROUTINE dopushcurrent(UBYTE *base BL_REG(d0))
{
    ULONG *a1 = currsp;

    *a1++ = (ULONG)base;
    *a1++ = (ULONG)CURRENT(base);
    currsp = a1;
}

/* dopopcurrent - pop the current item into the given array (the array
   on the stack is only checked with the debugger) */
void BLITZ_ROUTINE dopopcurrent(UBYTE *base BL_REG(d0))
{
    ULONG *a1 = currsp;

    CURRENT(base) = (struct item *)*--a1;
    a1--;
    currsp = a1;
}

/* doclearlist - move all used items to the front of the free list (one
   by one, so their order is reversed), no current item */
void BLITZ_ROUTINE doclearlist(UBYTE *base BL_REG(d0))
{
    struct item *a1;

    BL_INTS_OFF();
    while ((a1 = USED(base)->next)->next) {
        unlinkitem(a1);
        linkitem(a1, FREE(base));
    }
    CURRENT(base) = 0;
    BL_INTS_ON();
}

/* doreslist - reset a list: no current item */
void BLITZ_ROUTINE doreslist(UBYTE *base BL_REG(d0))
{
    CURRENT(base) = 0;
}

/* dofirstitem2 - make current = first (0 if the list is empty); -1 if
   there is one, else 0.  Called with interrupts disabled. */
static ULONG firstitem2(UBYTE *base)
{
    struct item *a1 = USED(base)->next;         /* next of head node */
    ULONG d0 = (ULONG)a1->next;

    if (!d0)
        a1 = 0;
    CURRENT(base) = a1;
    BL_INTS_ON();
    return d0 ? -1 : 0;
}

/* dolastitem2 - make current = last, the same backwards */
static ULONG lastitem2(UBYTE *base)
{
    struct item *a1 = TAILPRED(base);           /* prev of tail node */
    ULONG d0 = (ULONG)a1->prev;

    if (!d0)
        a1 = 0;
    CURRENT(base) = a1;
    BL_INTS_ON();
    return d0 ? -1 : 0;
}

/* dofirstitem - d0 = array base. make current=first */
ULONG BLITZ_ROUTINE dofirstitem(UBYTE *base BL_REG(d0))
{
    BL_INTS_OFF();
    return firstitem2(base);
}

/* dolastitem - make current=last */
ULONG BLITZ_ROUTINE dolastitem(UBYTE *base BL_REG(d0))
{
    BL_INTS_OFF();
    return lastitem2(base);
}

/* dokillitem - must be a current item to kill!  Move it to the front of
   the free list; the new current is its previous item (0 if it was the
   first) */
void BLITZ_ROUTINE dokillitem(UBYTE *base BL_REG(d0))
{
    struct item *a1, *prev;

    BL_INTS_OFF();
    a1 = CURRENT(base);                         /* current item */
    prev = a1->prev;                            /* old previous of current */
    unlinkitem(a1);                             /* remove from used... */
    linkitem(a1, FREE(base));                   /* add to free */
    a1 = prev;
    if (!a1->prev)
        a1 = 0;
    CURRENT(base) = a1;
    BL_INTS_ON();
}

/* doadditem - take the first free item and insert it after the current
   one (no current: at the beginning); it becomes current.  0 if there is
   no free item. */
ULONG BLITZ_ROUTINE doadditem(UBYTE *base BL_REG(d0))
{
    struct item *a1, *a2;
    ULONG d0;

    BL_INTS_OFF();
    a1 = FREE(base)->next;                      /* next of head node */
    d0 = (ULONG)a1->next;
    if (d0) {
        unlinkitem(a1);
        a2 = CURRENT(base);
        if (!a2)
            a2 = USED(base);
        linkitem(a1, a2);
        CURRENT(base) = a1;                     /* now current */
        d0 = -1;
    }
    BL_INTS_ON();
    return d0;
}

/* doaddfirst - add an item at the beginning of the list */
ULONG BLITZ_ROUTINE doaddfirst(UBYTE *base BL_REG(d0))
{
    struct item *a1;
    ULONG d0;

    BL_INTS_OFF();
    a1 = FREE(base)->next;                      /* next of head of free */
    d0 = (ULONG)a1->next;
    if (d0) {                                   /* else none free */
        CURRENT(base) = a1;                     /* now current */
        unlinkitem(a1);
        linkitem(a1, USED(base));
        d0 = -1;
    }
    BL_INTS_ON();
    return d0;
}

/* doaddlast - add item at the end of the list */
ULONG BLITZ_ROUTINE doaddlast(UBYTE *base BL_REG(d0))
{
    struct item *a1;
    ULONG d0;

    BL_INTS_OFF();
    a1 = FREE(base)->next;                      /* next of free */
    d0 = (ULONG)a1->next;
    if (d0) {
        CURRENT(base) = a1;
        unlinkitem(a1);
        linkitem(a1, TAILPRED(base));           /* prev of tail of used */
        d0 = -1;
    }
    BL_INTS_ON();
    return d0;
}

/* donextitem - if no current, do first item.  At the end the current
   item stays and the result is 0. */
ULONG BLITZ_ROUTINE donextitem(UBYTE *base BL_REG(d0))
{
    struct item *a1;
    ULONG d0;

    BL_INTS_OFF();
    if (!CURRENT(base))
        return firstitem2(base);
    a1 = CURRENT(base)->next;                   /* next of current */
    d0 = (ULONG)a1->next;
    if (d0) {
        CURRENT(base) = a1;
        d0 = -1;
    }
    BL_INTS_ON();
    return d0;
}

/* doprevitem - if no current, do last item */
ULONG BLITZ_ROUTINE doprevitem(UBYTE *base BL_REG(d0))
{
    struct item *a1;
    ULONG d0;

    BL_INTS_OFF();
    if (!CURRENT(base))
        return lastitem2(base);
    a1 = CURRENT(base)->prev;                   /* previous of current */
    d0 = (ULONG)a1->prev;
    if (d0) {
        CURRENT(base) = a1;
        d0 = -1;
    }
    BL_INTS_ON();
    return d0;
}

/* dosortlist - sort the used items ascending by the long at `offset` in
   the element (bubble sort; every pass ends one item earlier).  The
   current item stays the same item.
   BUG(orig) (#201): items are compared with CMP.l: BMI, i.e. the sign of the
   difference (wraps for keys more than 2^31 apart), and items with
   equal keys are not swapped. */
#define KEY(i) (*(ULONG *)((UBYTE *)(i) + 8 + offset))

void BLITZ_ROUTINE dosortlist(UBYTE *base BL_REG(d0), WORD offset BL_REG(d1))
{
    struct item *end = USEDTAIL(base);          /* d0 */
    struct item *a1, *a2, *a3, *a4;
    ULONG d1, d2;
    UWORD d3;                                   /* swapped flag */

    BL_INTS_OFF();
    if (USED(base)->next == end)
        goto dun;                               /* empty */
dopass:
    a1 = USED(base)->next;
    a2 = a1->next;
    a3 = a1->prev;
    if (a2 == end)
        goto dun;
    a4 = a2->next;
    d1 = KEY(a1);
    d3 = 0;
    if (!a3)                                    /* never true: a3 = head */
        BLITZ_RUNERR((const char *)end);        /* TRAP #0, d0 = end */
nxpass:
    d2 = KEY(a2);
    if ((LONG)(d2 - d1) < 0)
        goto doswap;
    d1 = d2;
nxitem:
    if (a4 == end)
        goto dunpass;
    a3 = a1;
    a1 = a2;
    a2 = a4;
    a4 = a2->next;
    goto nxpass;
dunpass:
    end = a2;
    if (d3)
        goto dopass;
dun:
    BL_INTS_ON();
    return;

doswap:                                         /* a2 before a1 */
    a3->next = a2;
    a2->next = a1;
    a2->prev = a3;
    a1->prev = a2;
    a1->next = a4;
    a4->prev = a1;
    {                                           /* EXG a1,a2 */
        struct item *t = a1;
        a1 = a2;
        a2 = t;
    }
    d3 = 1;
    goto nxitem;
}

/* ******************* ERROR CHECKING (debugger only) ******************* */

/* the messages, in the error check area (assembler data, see
   BLITZ_ERRCHK_MESSAGE) */
BLITZ_ERRCHK_MESSAGE(mess1, "Array is not a List");
BLITZ_ERRCHK_MESSAGE(mess2, "No Current Item");
BLITZ_ERRCHK_MESSAGE(mess3, "Current Stack overflow");
BLITZ_ERRCHK_MESSAGE(mess4, "Current Stack Empty");
BLITZ_ERRCHK_MESSAGE(mess5, "Wrong Array on Stack");

/* lchk - the array must be a list */
static void BLITZ_ERRCHK lchk_c(UBYTE *base BL_REG(d0))
{
    if (LISTID(base) != 0x6c497354)             /* "lIsT" */
        BLITZ_RUNERR(mess1);
}

/* lchk2 - KillItem: there must be a current item */
static void BLITZ_ERRCHK lchk2_c(UBYTE *base BL_REG(d0))
{
    if (!CURRENT(base))
        BLITZ_RUNERR(mess2);
    lchk_c(base);
}

/* pushchk - room for one more entry */
static void BLITZ_ERRCHK pushchk_c(UBYTE *base BL_REG(d0))
{
    if ((ULONG)currsp - maxstack >= (ULONG)currstack)
        BLITZ_RUNERR(mess3);
    lchk_c(base);
}

/* popchk - stack not empty, the top entry is of this array */
static void BLITZ_ERRCHK popchk_c(UBYTE *base BL_REG(d0))
{
    if ((ULONG)currsp <= (ULONG)currstack)
        BLITZ_RUNERR(mess4);
    if (currsp[-2] != (ULONG)base)
        BLITZ_RUNERR(mess5);
    lchk_c(base);
}

/* The checks run before the routine with its arguments in d0/d1 and keep
   them (the original only changes a0). */
BLITZ_ERRCHK_THUNK(lchk, lchk_c, "d0-d1/a1");
BLITZ_ERRCHK_THUNK(lchk2, lchk2_c, "d0-d1/a1");
BLITZ_ERRCHK_THUNK(pushchk, pushchk_c, "d0-d1/a1");
BLITZ_ERRCHK_THUNK(popchk, popchk_c, "d0-d1/a1");
