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
#include "blitzobj.h"
#include <exec/lists.h>

/* entry points (defined below) */
BB_BOOL BLITZ_ROUTINE dofirstitem(UBYTE *base BL_REG(d0));
BB_BOOL BLITZ_ROUTINE doadditem(UBYTE *base BL_REG(d0));
BB_BOOL BLITZ_ROUTINE donextitem(UBYTE *base BL_REG(d0));
void BLITZ_ROUTINE doreslist(UBYTE *base BL_REG(d0));
void BLITZ_ROUTINE dokillitem(UBYTE *base BL_REG(d0));
BB_BOOL BLITZ_ROUTINE doaddfirst(UBYTE *base BL_REG(d0));
void BLITZ_ROUTINE doclearlist(UBYTE *base BL_REG(d0));
BB_BOOL BLITZ_ROUTINE doaddlast(UBYTE *base BL_REG(d0));
void BLITZ_ROUTINE dopushcurrent(UBYTE *base BL_REG(d0));
void BLITZ_ROUTINE dopopcurrent(UBYTE *base BL_REG(d0));
BB_BOOL BLITZ_ROUTINE doprevitem(UBYTE *base BL_REG(d0));
BB_BOOL BLITZ_ROUTINE dolastitem(UBYTE *base BL_REG(d0));
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

/* a list item's node (the element follows) and the list header in front of
   the array base (arrayslib newlist): struct bb_listitem, struct
   bb_listhead (blitzobj.h) */
#define HEAD(b)     ((struct bb_listhead *)((b) - sizeof(struct bb_listhead)))

#define CURRENT(b)  (HEAD(b)->current)
#define USED(b)     ((struct bb_listitem *)&HEAD(b)->used.mlh_Head)  /* &lh_Head as node */
#define USEDTAIL(b) ((struct bb_listitem *)&HEAD(b)->used.mlh_Tail)  /* &lh_Tail as node */
#define TAILPRED(b) (*(struct bb_listitem **)&HEAD(b)->used.mlh_TailPred)
#define FREE(b)     ((struct bb_listitem *)&HEAD(b)->free.mlh_Head)  /* &lh_Head of free */
#define LISTID(b)   (HEAD(b)->id)

/* an entry of the item stack */
struct stackentry {
    UBYTE *base;                                /* the array */
    struct bb_listitem *current;                       /* its current item */
};
static_assert(sizeof(struct stackentry) == 8);

static ULONG maxstack;                          /* size in bytes */
static struct stackentry *currstack;
static struct stackentry *currsp;

/* unlinkitem - item (a1) to unlink */
static void unlinkitem(struct bb_listitem *item)
{
    struct bb_listitem *prev = item->prev, *next = item->next;

    next->prev = prev;
    prev->next = next;
}

/* linkitem - link item (a1) after `after` (a2) */
static void linkitem(struct bb_listitem *item, struct bb_listitem *after)
{
    struct bb_listitem *next = after->next;

    after->next = item;
    next->prev = item;
    item->prev = after;
    item->next = next;
}

/** @brief The library's init: an empty item stack of 8 entries. */
void BLITZ_ROUTINE doinit(void)
{
    currstack = 0;
    currsp = 0;
    setstack(8);
}

/**
 * @brief ItemStackSize Max Items: a new item stack.
 *
 * Frees the old stack (memlib globfree), allocates n*8 bytes (globalloc,
 * MEMF_PUBLIC); the stack is empty afterwards.
 *
 * @param n  d0: the number of entries
 */
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
    currstack = (struct stackentry *)r.d0;
    currsp = (struct stackentry *)r.d0;
}

/**
 * @brief PushItem Arrayname(): push the list and its current item on the
 * item stack.
 *
 * Errors (debugger only): "Current Stack overflow", "Array is not a List"
 * (pushchk).  Without the debugger there is no check.
 *
 * @param base  d0: the list array
 */
void BLITZ_ROUTINE dopushcurrent(UBYTE *base BL_REG(d0))
{
    struct stackentry *sp = currsp;

    sp->base = base;
    sp->current = CURRENT(base);
    sp++;
    currsp = sp;
}

/**
 * @brief PopItem Arrayname(): pop the current item from the item stack
 * into the given list.
 *
 * Errors (debugger only): "Current Stack Empty", "Wrong Array on Stack"
 * (the array on the stack is only checked then), "Array is not a List"
 * (popchk).
 *
 * @param base  d0: the list array
 */
void BLITZ_ROUTINE dopopcurrent(UBYTE *base BL_REG(d0))
{
    struct stackentry *sp = currsp;

    sp--;
    CURRENT(base) = sp->current;
    currsp = sp;
}

/**
 * @brief ClearList Arrayname(): empty a list.
 *
 * Moves all used items to the front of the free list (one by one, so
 * their order is reversed); no current item.  Interrupts disabled
 * meanwhile.  Errors (debugger only): "Array is not a List" (lchk).
 *
 * @param base  d0: the list array
 */
void BLITZ_ROUTINE doclearlist(UBYTE *base BL_REG(d0))
{
    struct bb_listitem *item;

    BL_INTS_OFF();
    while ((item = USED(base)->next)->next) {
        unlinkitem(item);
        linkitem(item, FREE(base));
    }
    CURRENT(base) = 0;
    BL_INTS_ON();
}

/**
 * @brief ResetList Arrayname(): no current item (the next NextItem gives
 * the first).  Errors (debugger only): "Array is not a List" (lchk).
 *
 * @param base  d0: the list array
 */
void BLITZ_ROUTINE doreslist(UBYTE *base BL_REG(d0))
{
    CURRENT(base) = 0;
}

/* dofirstitem2 - make current = first (0 if the list is empty); -1 if
   there is one, else 0.  Called with interrupts disabled. */
static BB_BOOL firstitem2(UBYTE *base)
{
    struct bb_listitem *item = USED(base)->next;       /* next of head node */
    ULONG link = (ULONG)item->next;             /* d0 */

    if (!link)
        item = 0;
    CURRENT(base) = item;
    BL_INTS_ON();
    return link ? BB_TRUE : BB_FALSE;
}

/* dolastitem2 - make current = last, the same backwards */
static BB_BOOL lastitem2(UBYTE *base)
{
    struct bb_listitem *item = TAILPRED(base);         /* prev of tail node */
    ULONG link = (ULONG)item->prev;             /* d0 */

    if (!link)
        item = 0;
    CURRENT(base) = item;
    BL_INTS_ON();
    return link ? BB_TRUE : BB_FALSE;
}

/**
 * @brief FirstItem(Arrayname()): make the first item the current one.
 *
 * No current item if the list is empty.  Errors (debugger only): "Array
 * is not a List" (lchk).
 *
 * @param base  d0: the list array
 * @return d0: BB_TRUE if there is an item, BB_FALSE if the list is empty
 */
BB_BOOL BLITZ_ROUTINE dofirstitem(UBYTE *base BL_REG(d0))
{
    BL_INTS_OFF();
    return firstitem2(base);
}

/**
 * @brief LastItem(Arrayname()): make the last item the current one, as
 * FirstItem.
 *
 * @param base  d0: the list array
 * @return d0: BB_TRUE if there is an item, BB_FALSE if the list is empty
 */
BB_BOOL BLITZ_ROUTINE dolastitem(UBYTE *base BL_REG(d0))
{
    BL_INTS_OFF();
    return lastitem2(base);
}

/**
 * @brief KillItem Arrayname(): remove the current item.
 *
 * There must be a current item to kill (checked only with the debugger:
 * "No Current Item", lchk2).  It moves to the front of the free list; the
 * new current item is its previous one (none if it was the first).
 *
 * @param base  d0: the list array
 */
void BLITZ_ROUTINE dokillitem(UBYTE *base BL_REG(d0))
{
    struct bb_listitem *item, *prev;

    BL_INTS_OFF();
    item = CURRENT(base);                       /* current item */
    prev = item->prev;                          /* old previous of current */
    unlinkitem(item);                           /* remove from used... */
    linkitem(item, FREE(base));                 /* add to free */
    CURRENT(base) = prev->prev ? prev : 0;      /* the head node: none */
    BL_INTS_ON();
}

/**
 * @brief AddItem(Arrayname()): insert an item after the current one.
 *
 * Takes the first free item and inserts it after the current one (no
 * current: at the beginning); it becomes current.  Errors (debugger only):
 * "Array is not a List" (lchk).
 *
 * @param base  d0: the list array
 * @return d0: BB_TRUE if an item was added, BB_FALSE if there is no free
 *         item
 */
BB_BOOL BLITZ_ROUTINE doadditem(UBYTE *base BL_REG(d0))
{
    struct bb_listitem *item, *after;
    struct bb_listitem *link;                          /* d0, then -1 */

    BL_INTS_OFF();
    item = FREE(base)->next;                    /* next of head node */
    link = item->next;
    if (link) {
        unlinkitem(item);
        after = CURRENT(base);
        if (!after)
            after = USED(base);
        linkitem(item, after);
        CURRENT(base) = item;                   /* now current */
    }
    BL_INTS_ON();
    return link ? BB_TRUE : BB_FALSE;
}

/**
 * @brief AddFirst(Arrayname()): add an item at the beginning of the list,
 * as AddItem; it becomes current.
 *
 * @param base  d0: the list array
 * @return d0: BB_TRUE if an item was added, BB_FALSE if none is free
 */
BB_BOOL BLITZ_ROUTINE doaddfirst(UBYTE *base BL_REG(d0))
{
    struct bb_listitem *item;
    struct bb_listitem *link;                          /* d0, then -1 */

    BL_INTS_OFF();
    item = FREE(base)->next;                    /* next of head of free */
    link = item->next;
    if (link) {                                 /* else none free */
        CURRENT(base) = item;                   /* now current */
        unlinkitem(item);
        linkitem(item, USED(base));
    }
    BL_INTS_ON();
    return link ? BB_TRUE : BB_FALSE;
}

/**
 * @brief AddLast(Arrayname()): add an item at the end of the list, as
 * AddItem; it becomes current.
 *
 * @param base  d0: the list array
 * @return d0: BB_TRUE if an item was added, BB_FALSE if none is free
 */
BB_BOOL BLITZ_ROUTINE doaddlast(UBYTE *base BL_REG(d0))
{
    struct bb_listitem *item;
    struct bb_listitem *link;                          /* d0, then -1 */

    BL_INTS_OFF();
    item = FREE(base)->next;                    /* next of free */
    link = item->next;
    if (link) {
        CURRENT(base) = item;
        unlinkitem(item);
        linkitem(item, TAILPRED(base));         /* prev of tail of used */
    }
    BL_INTS_ON();
    return link ? BB_TRUE : BB_FALSE;
}

/**
 * @brief NextItem(Arrayname()): make the next item the current one.
 *
 * Without a current item it does FirstItem.  At the end the current item
 * stays.  Errors (debugger only): "Array is not a List" (lchk).
 *
 * @param base  d0: the list array
 * @return d0: BB_TRUE if there is a next item, BB_FALSE at the end
 */
BB_BOOL BLITZ_ROUTINE donextitem(UBYTE *base BL_REG(d0))
{
    struct bb_listitem *item;
    struct bb_listitem *link;                          /* d0, then -1 */

    BL_INTS_OFF();
    if (!CURRENT(base))
        return firstitem2(base);
    item = CURRENT(base)->next;                 /* next of current */
    link = item->next;
    if (link)
        CURRENT(base) = item;
    BL_INTS_ON();
    return link ? BB_TRUE : BB_FALSE;
}

/**
 * @brief PrevItem Arrayname(): make the previous item the current one, as
 * NextItem backwards (without a current item it does LastItem).
 *
 * @param base  d0: the list array
 * @return d0: BB_TRUE if there is a previous item, BB_FALSE at the
 *         beginning
 */
BB_BOOL BLITZ_ROUTINE doprevitem(UBYTE *base BL_REG(d0))
{
    struct bb_listitem *item;
    struct bb_listitem *link;                          /* d0, then -1 */

    BL_INTS_OFF();
    if (!CURRENT(base))
        return lastitem2(base);
    item = CURRENT(base)->prev;                 /* previous of current */
    link = item->prev;
    if (link)
        CURRENT(base) = item;
    BL_INTS_ON();
    return link ? BB_TRUE : BB_FALSE;
}

#define KEY(i) (*(ULONG *)((UBYTE *)((i) + 1) + offset))  /* the element: behind the node */

/**
 * @brief SortList Arrayname(),sizeof .type\\offset: sort a list by a long
 * of its elements.
 *
 * Sorts the used items ascending by the long at `offset` in the element
 * (bubble sort; every pass ends one item earlier).  The current item
 * stays the same item.  Interrupts disabled meanwhile.  Errors (debugger
 * only): "Array is not a List" (lchk).  DIFF: a3 is not changed (the
 * original destroys a3 and d6).
 * BUG(orig) (#201): items are compared with CMP.l: BMI, i.e. the sign of
 * the difference (wraps for keys more than 2^31 apart), and items with
 * equal keys are not swapped.
 *
 * @param base    d0: the list array
 * @param offset  d1: the offset of the key in the element
 */
void BLITZ_ROUTINE dosortlist(UBYTE *base BL_REG(d0), WORD offset BL_REG(d1))
{
    struct bb_listitem *end = USEDTAIL(base);          /* d0 */
    struct bb_listitem *item, *next, *prev, *after;    /* a1, a2, a3, a4 */
    ULONG itemkey, nextkey;                     /* d1, d2 */
    UWORD swapped;                              /* d3: swapped flag */

    BL_INTS_OFF();
    if (USED(base)->next == end)
        goto dun;                               /* empty */
dopass:
    item = USED(base)->next;
    next = item->next;
    prev = item->prev;
    if (next == end)
        goto dun;
    after = next->next;
    itemkey = KEY(item);
    swapped = 0;
    if (!prev)                                  /* never true: prev = head */
        BLITZ_RUNERR((const char *)end);        /* TRAP #0, d0 = end */
nxpass:
    nextkey = KEY(next);
    if ((LONG)(nextkey - itemkey) < 0)
        goto doswap;
    itemkey = nextkey;
nxitem:
    if (after == end)
        goto dunpass;
    prev = item;
    item = next;
    next = after;
    after = next->next;
    goto nxpass;
dunpass:
    end = next;
    if (swapped)
        goto dopass;
dun:
    BL_INTS_ON();
    return;

doswap:                                         /* next (a2) before item (a1) */
    prev->next = next;
    next->next = item;
    next->prev = prev;
    item->prev = next;
    item->next = after;
    after->prev = item;
    {                                           /* EXG a1,a2 */
        struct bb_listitem *other = item;
        item = next;
        next = other;
    }
    swapped = 1;
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
    if (LISTID(base) != BB_LISTID)              /* "lIsT" */
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
    if (currsp[-1].base != base)
        BLITZ_RUNERR(mess5);
    lchk_c(base);
}

/* The checks run before the routine with its arguments in d0/d1 and keep
   them (the original only changes a0). */
BLITZ_ERRCHK_THUNK(lchk, lchk_c, "d0-d1/a1");
BLITZ_ERRCHK_THUNK(lchk2, lchk2_c, "d0-d1/a1");
BLITZ_ERRCHK_THUNK(pushchk, pushchk_c, "d0-d1/a1");
BLITZ_ERRCHK_THUNK(popchk, popchk_c, "d0-d1/a1");
