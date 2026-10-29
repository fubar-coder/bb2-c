/*
 * rilinkedlistlib - RWE userlib 12 (Reflective Images linked list library
 * v0.91, 1995): up to five doubly linked lists of variable copies -
 * InitList, AddListItem, SetListItem, NextListItem, GetListItem,
 * PrevListItem, FirstListItem, LastListItem, AddFirstListItem,
 * AddLastListItem, DelListItem, AddrListItem, SizeOfListItem.
 *
 * C translation of rwelibsrc/Listlib/listlib4.bb; the reference is the
 * reference build of that source by blitz2 2.10 (REFERENCE "refbuild",
 * ref/rilinkedlistlib.obj: the OL RILISTLIB.OBJ plus SizeOfListItem).
 * Declarations equal (ctest rwe_declarations_rilinkedlistlib), the
 * behaviour is compared with it in libs/tests/rwe/rilinkedlistlib.c.
 * Notes: docs/rwelibs/port/rilinkedlistlib.md.
 *
 * A node (18 bytes, memlib globalloc with flags 0): next.l, prev.l,
 * pad.w, data.l (a copy of the variable, globalloc), size.l.  Per list
 * the first, last and current node in tables of five longs (list# * 4
 * indexes them without a check; the tables first, last, an unused one
 * and current follow each other as in the original).  The size of a
 * variable is the word in front of it (-2(var)) minus 8 (getvarsize);
 * the copies are made by RIAmosFuncLib's CopyByte (d0 = source, d1 =
 * destination, d2 = bytes).
 *
 * Everything is C behind thunks that keep d1/a0/a1 (the original changes
 * d1, d2, a0, a1, InitList also a2); memlib and CopyByte through ALibJsr.
 *
 * BUG(orig), kept:
 *   - No command checks the list number or that the list has a current
 *     node (only DelListItem does): AddListItem, SetListItem,
 *     NextListItem, PrevListItem and AddFirstListItem on an empty list
 *     read and write from address 0 on.
 *   - InitList does not free a list that is already there.
 *   - The size: a word of less than 8 in front of the variable gives a
 *     size of almost 4 GB (the allocation fails), exactly 8 a size of 0,
 *     which CopyByte takes as 2^32 bytes.
 *   - AddListItem inserts behind the current node without setting the
 *     prev of the node behind it (it keeps pointing to the current one);
 *     its size field is the size of the last variable any command looked
 *     at (or of the last node DelListItem deleted), its data 0.
 *   - SetListItem gives the node a new copy without freeing the old one
 *     and without setting its size field; GetListItem copies as many
 *     bytes as the variable has, not as the node's data has.
 *   - AddFirstListItem writes the new node into the old first node's
 *     next (not prev; the list becomes a loop), leaves prev, pad, data
 *     and size of the new node as the allocation left them (not
 *     cleared), does not touch the last node, and returns list# * 4.
 *   - DelListItem still reads the nodes of v0.9 (no pad): data is the
 *     long at offset 8 (the pad word and the upper word of the data
 *     pointer), size the long at 14; so it frees a wrong address (memlib:
 *     "Unable to Free Memory") and the data is lost.  Deleting the first
 *     node clears the next (not prev) of the node behind it, the list
 *     ends there; deleting the only node frees nothing but returns 0.
 *     It sets the size of the last variable to that of the node.
 */
#include "blitzlib.h"

/* RIAmosFuncLib (75) CopyByte: index 9 of the token list of the CD's
   BSS2 RIAMOSFUNCLIB.OBJ (d0 = source, d1 = destination, d2 = bytes) */
#define TOK_copybyte ((LIB_riamosfunclib << 7) + 10)

/* entry points (thunks, defined below) */
void _initlist(void), _addlistitem(void), _setlistitem(void), _nextlistitem(void);
void _getlistitem(void), _prevlistitem(void), _firstlistitem(void), _lastlistitem(void);
void _addfirstitem(void), _addlastitem(void), _dellistitem(void), _addrlistitem(void);
void _sizeoflistitem(void), _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#rilinkedlistlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rilinkedlistlib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_VARPTR);
BLITZ_LIBS0();
BLITZ_SUBS(_initlist, 0, 0);
BLITZ_NAME("InitList", "list#,var");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_addlistitem, 0, 0);
BLITZ_NAME("AddListItem", "list#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_VARPTR);
BLITZ_LIBS0();
BLITZ_SUBS(_setlistitem, 0, 0);
BLITZ_NAME("SetListItem", "list#,var");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_nextlistitem, 0, 0);
BLITZ_NAME("NextListItem", "list#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_VARPTR);
BLITZ_LIBS0();
BLITZ_SUBS(_getlistitem, 0, 0);
BLITZ_NAME("GetListItem", "list#,var");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_prevlistitem, 0, 0);
BLITZ_NAME("PrevListItem", "list#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_firstlistitem, 0, 0);
BLITZ_NAME("FirstListItem", "list#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_lastlistitem, 0, 0);
BLITZ_NAME("LastListItem", "list#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_addfirstitem, 0, 0);
BLITZ_NAME("AddFirstListItem", "list#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_addlastitem, 0, 0);
BLITZ_NAME("AddLastListItem", "list#");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_dellistitem, 0, 0);
BLITZ_NAME("DelListItem", "list#");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_addrlistitem, 0, 0);
BLITZ_NAME("AddrListItem", "list#");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_VARPTR);
BLITZ_LIBS0();
BLITZ_SUBS(_sizeoflistitem, 0, 0);
BLITZ_NAME("SizeOfListItem", "var");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

#define NODENEXT 0
#define NODEPREV 4
#define NODEPAD  8
#define NODEDAT  10
#define NODELEN  14
#define NODESIZE 18

/* a long/word at an address + offset (nodes are word aligned) */
#define NL(a, o) (*(ULONG *)((a) + (o)))
#define NW(a, o) (*(UWORD *)((a) + (o)))

/* _first, _last, _sizes (unused), _current: five longs each, one after
   the other; a table entry is the table's address + list# * 4 */
static ULONG tables[20];
#define FIRST   0
#define LAST    20
#define CURRENT 60
#define TAB(t) (*(ULONG *)((UBYTE *)tables + (t) + listnum))

static ULONG nodeptr, _oldnext, _next, _prev, _data, myvar, _size, listnum;
static UWORD pad;

static ULONG globalloc(ULONG size)
{
    struct bl_regs r = { .d0 = size, .d1 = 0 };

    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(ULONG mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

static void copybyte(ULONG src, ULONG dest, ULONG n)
{
    BLITZ_ALIBJSR_D4(TOK_copybyte, src, dest, n, 0);
}

/* getvarsize: the variable and its size (the word in front of it - 8) */
static ULONG getvarsize(ULONG var)
{
    myvar = var;
    _size = (ULONG)NW(var, -2) - 8;
    return _size;
}

void BLITZ_ROUTINE c_libinit(void)
{
}

ULONG BLITZ_ROUTINE c_libfinit(void)
{
    return 0;
}

/* SizeOfListItem(var) */
ULONG BLITZ_ROUTINE c_sizeoflistitem(ULONG var BL_REG(d0))
{
    return getvarsize(var);
}

/* AddrListItem(list#): the current node */
ULONG BLITZ_ROUTINE c_addrlistitem(ULONG list BL_REG(d0))
{
    listnum = list << 2;
    return TAB(CURRENT);
}

/* InitList list#,var: a list with one node, a copy of var */
ULONG BLITZ_ROUTINE c_initlist(ULONG list BL_REG(d0), ULONG var BL_REG(d1))
{
    ULONG node;

    listnum = list << 2;
    getvarsize(var);
    node = globalloc(NODESIZE);
    if (!node)
        return 0;
    nodeptr = node;
    TAB(FIRST) = node;
    TAB(LAST) = node;
    TAB(CURRENT) = node;
    _data = globalloc(_size);
    if (!_data) {
        globfree(nodeptr, NODESIZE);
        TAB(FIRST) = 0;
        TAB(LAST) = 0;
        TAB(CURRENT) = 0;
        return 0;
    }
    copybyte(myvar, _data, _size);
    node = nodeptr;
    NL(node, NODENEXT) = 0;
    NL(node, NODEPREV) = 0;
    NW(node, NODEPAD) = 0;
    NL(node, NODEDAT) = _data;
    NL(node, NODELEN) = _size;
    return (ULONG)-1;
}

/* AddListItem list#: a new empty node behind the current one, now the
   current one */
ULONG BLITZ_ROUTINE c_addlistitem(ULONG list BL_REG(d0))
{
    ULONG node, cur;

    listnum = list << 2;
    node = globalloc(NODESIZE);
    if (!node)
        return 0;
    nodeptr = node;
    cur = TAB(CURRENT);                 /* BUG(orig): 0 not checked */
    _next = NL(cur, NODENEXT);
    _prev = NL(cur, NODEPREV);
    if (!_next)
        TAB(LAST) = nodeptr;
    cur = TAB(CURRENT);
    TAB(CURRENT) = nodeptr;
    _oldnext = NL(cur, NODENEXT);
    NL(cur, NODENEXT) = nodeptr;
    /* BUG(orig): the prev of the node behind (_oldnext) is not set */
    node = nodeptr;
    NL(node, NODENEXT) = _oldnext;
    NL(node, NODEPREV) = cur;
    NW(node, NODEPAD) = 0;
    NL(node, NODEDAT) = 0;
    NL(node, NODELEN) = _size;          /* BUG(orig): the last size */
    return (ULONG)-1;
}

/* SetListItem list#,var: a new copy of var for the current node */
ULONG BLITZ_ROUTINE c_setlistitem(ULONG list BL_REG(d0), ULONG var BL_REG(d1))
{
    ULONG mem;

    listnum = list << 2;
    mem = globalloc(getvarsize(var));
    if (!mem)
        return 0;
    /* BUG(orig): the old copy is not freed, the size not set */
    NL(TAB(CURRENT), NODEDAT) = mem;
    copybyte(myvar, mem, _size);
    return (ULONG)-1;
}

/* GetListItem list#,var: the current node's data into var (the size of
   var) */
ULONG BLITZ_ROUTINE c_getlistitem(ULONG list BL_REG(d0), ULONG var BL_REG(d1))
{
    ULONG data;

    listnum = list << 2;
    getvarsize(var);
    data = NL(TAB(CURRENT), NODEDAT);
    if (!data)
        return 0;
    copybyte(data, myvar, _size);
    return (ULONG)-1;
}

/* NextListItem / PrevListItem list#: 0 at the end */
ULONG BLITZ_ROUTINE c_nextlistitem(ULONG list BL_REG(d0))
{
    ULONG n;

    listnum = list << 2;
    n = NL(TAB(CURRENT), NODENEXT);
    if (!n)
        return 0;
    TAB(CURRENT) = n;
    return (ULONG)-1;
}

ULONG BLITZ_ROUTINE c_prevlistitem(ULONG list BL_REG(d0))
{
    ULONG n;

    listnum = list << 2;
    n = NL(TAB(CURRENT), NODEPREV);
    if (!n)
        return 0;
    TAB(CURRENT) = n;
    return (ULONG)-1;
}

/* FirstListItem / LastListItem list# */
ULONG BLITZ_ROUTINE c_firstlistitem(ULONG list BL_REG(d0))
{
    listnum = list << 2;
    TAB(CURRENT) = TAB(FIRST);
    return (ULONG)-1;
}

ULONG BLITZ_ROUTINE c_lastlistitem(ULONG list BL_REG(d0))
{
    listnum = list << 2;
    TAB(CURRENT) = TAB(LAST);
    return (ULONG)-1;
}

/* AddLastListItem list#: LastListItem, then AddListItem */
ULONG BLITZ_ROUTINE c_addlastitem(ULONG list BL_REG(d0))
{
    c_lastlistitem(list);
    return c_addlistitem(list);
}

/* AddFirstListItem list#: a new node in front of the first one;
   BUG(orig): see above, the result is list# * 4 */
ULONG BLITZ_ROUTINE c_addfirstitem(ULONG list BL_REG(d0))
{
    ULONG node;

    listnum = list;
    node = globalloc(NODESIZE);
    if (!node)
        return 0;
    nodeptr = node;
    listnum = list << 2;
    _next = TAB(FIRST);
    TAB(FIRST) = nodeptr;
    NL(_next, NODENEXT) = nodeptr;      /* BUG(orig): next, not prev */
    TAB(CURRENT) = nodeptr;
    NL(nodeptr, NODENEXT) = _next;
    return listnum;
}

/* DelListItem list#: free the current node; BUG(orig): see above */
ULONG BLITZ_ROUTINE c_dellistitem(ULONG list BL_REG(d0))
{
    ULONG node;

    listnum = list << 2;
    node = nodeptr = TAB(CURRENT);
    if (!node)
        return 0;
    /* the v0.9 node: next, prev, data, size (no pad) */
    _next = NL(node, 0);
    _prev = NL(node, 4);
    _data = NL(node, 8);
    pad = NW(node, 12);
    _size = NL(node, 14);
    if (_data) {
        globfree(_data, _size);
        NL(nodeptr, NODEDAT) = 0;
        NL(nodeptr, NODELEN) = 0;
    }
    if (!_prev) {
        if (!_next)
            return 0;                   /* the only node */
        NL(_next, NODENEXT) = 0;        /* BUG(orig): next, not prev */
        TAB(FIRST) = _next;
        TAB(CURRENT) = _next;
        globfree(nodeptr, NODESIZE);
        return (ULONG)-1;
    }
    if (!_next) {
        NL(_prev, NODENEXT) = 0;
        TAB(LAST) = _prev;
        TAB(CURRENT) = _prev;
        globfree(nodeptr, NODESIZE);
        return (ULONG)-1;
    }
    NL(_prev, NODENEXT) = _next;
    NL(_next, NODEPREV) = _prev;
    TAB(CURRENT) = _next;
    globfree(nodeptr, NODESIZE);
    return (ULONG)-1;
}

/* entries: keep every one of d0/d1/a0/a1 that is not the result */
BLITZ_THUNK(_libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(_libfinit, c_libfinit, "d1/a0-a1");
BLITZ_THUNK(_sizeoflistitem, c_sizeoflistitem, "d1/a0-a1");
BLITZ_THUNK(_addrlistitem, c_addrlistitem, "d1/a0-a1");
BLITZ_THUNK(_initlist, c_initlist, "d1/a0-a1");
BLITZ_THUNK(_addlistitem, c_addlistitem, "d1/a0-a1");
BLITZ_THUNK(_setlistitem, c_setlistitem, "d1/a0-a1");
BLITZ_THUNK(_getlistitem, c_getlistitem, "d1/a0-a1");
BLITZ_THUNK(_nextlistitem, c_nextlistitem, "d1/a0-a1");
BLITZ_THUNK(_firstlistitem, c_firstlistitem, "d1/a0-a1");
BLITZ_THUNK(_lastlistitem, c_lastlistitem, "d1/a0-a1");
BLITZ_THUNK(_prevlistitem, c_prevlistitem, "d1/a0-a1");
BLITZ_THUNK(_addlastitem, c_addlastitem, "d1/a0-a1");
BLITZ_THUNK(_addfirstitem, c_addfirstitem, "d1/a0-a1");
BLITZ_THUNK(_dellistitem, c_dellistitem, "d1/a0-a1");
