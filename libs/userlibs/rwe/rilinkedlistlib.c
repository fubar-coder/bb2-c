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
 * pad.w, data.l (a copy of the variable, globalloc), size.l (struct
 * node; DelListItem reads a struct node09).  Per list
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
 *     read and write from address 0 on (#732, #733).
 *   - InitList does not free a list that is already there (#731).
 *   - The size: a word of less than 8 in front of the variable gives a
 *     size of almost 4 GB (the allocation fails), exactly 8 a size of 0,
 *     which CopyByte takes as 2^32 bytes (#736).
 *   - AddListItem inserts behind the current node without setting the
 *     prev of the node behind it (it keeps pointing to the current one)
 *     (#727); its size field is the size of the last variable any command
 *     looked at (or of the last node DelListItem deleted), its data 0
 *     (#735).
 *   - SetListItem gives the node a new copy without freeing the old one
 *     and without setting its size field (#734, #735); GetListItem copies
 *     as many bytes as the variable has, not as the node's data has (#730).
 *   - AddFirstListItem writes the new node into the old first node's
 *     next (not prev; the list becomes a loop) (#724), leaves prev, pad,
 *     data and size of the new node as the allocation left them (not
 *     cleared) (#726), does not touch the last node, and returns list# * 4
 *     (#725).
 *   - DelListItem still reads the nodes of v0.9 (no pad): data is the
 *     long at offset 8 (the pad word and the upper word of the data
 *     pointer), size the long at 14; so it frees a wrong address (memlib:
 *     "Unable to Free Memory") and the data is lost (#729).  Deleting the
 *     first node clears the next (not prev) of the node behind it, the
 *     list ends there (#728).  It sets the size of the last variable to
 *     that of the node (#735).
 *
 * DelListItem on the only node frees nothing and returns 0: not a defect,
 * the command refuses to delete the last node and says so with 0.
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

/* a node of v0.91 (the addresses as longs, like the original's
   variables) */
struct node {
    ULONG next;                         /* +0 */
    ULONG prev;                         /* +4 */
    UWORD pad;                          /* +8 */
    ULONG data;                         /* +10: the copy of the variable */
    ULONG size;                         /* +14 */
};
static_assert(sizeof(struct node) == 18);
#define NODESIZE sizeof(struct node)
#define NODE(a) ((struct node *)(a))

/* the node of v0.9 (no pad), as DelListItem still reads it: data and
   size at the wrong offsets (BUG(orig) (#729)) */
struct node09 {
    ULONG next;                         /* +0 */
    ULONG prev;                         /* +4 */
    ULONG data;                         /* +8: pad and the upper word of data */
    UWORD pad;                          /* +12: the lower word of data */
    ULONG size;                         /* +14 */
};
static_assert(sizeof(struct node09) == 18);

/* the word in front of a variable: its size + 8 */
#define VARSIZEW(var) (*(UWORD *)((var) - 2))

/* _first, _last, _sizes (unused), _current: five longs each, one after
   the other; a table entry is the table's address + list# * 4 (not
   checked: a list# out of 0-4 reaches into the next table) */
static struct {
    ULONG first[5], last[5], sizes[5], current[5];
} tables;
static_assert(sizeof tables == 20 * 4);
#define TAB(t) (*(ULONG *)((UBYTE *)tables.t + listnum))

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
    _size = (ULONG)VARSIZEW(var) - 8;
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
    return TAB(current);
}

/* InitList list#,var: a list with one node, a copy of var */
BB_BOOL BLITZ_ROUTINE c_initlist(ULONG list BL_REG(d0), ULONG var BL_REG(d1))
{
    ULONG node;

    listnum = list << 2;
    getvarsize(var);
    node = globalloc(NODESIZE);
    if (!node)
        return BB_FALSE;
    nodeptr = node;
    TAB(first) = node;
    TAB(last) = node;
    TAB(current) = node;
    _data = globalloc(_size);
    if (!_data) {
        globfree(nodeptr, NODESIZE);
        TAB(first) = 0;
        TAB(last) = 0;
        TAB(current) = 0;
        return BB_FALSE;
    }
    copybyte(myvar, _data, _size);
    node = nodeptr;
    NODE(node)->next = 0;
    NODE(node)->prev = 0;
    NODE(node)->pad = 0;
    NODE(node)->data = _data;
    NODE(node)->size = _size;
    return BB_TRUE;
}

/* AddListItem list#: a new empty node behind the current one, now the
   current one */
BB_BOOL BLITZ_ROUTINE c_addlistitem(ULONG list BL_REG(d0))
{
    ULONG node, cur;

    listnum = list << 2;
    node = globalloc(NODESIZE);
    if (!node)
        return BB_FALSE;
    nodeptr = node;
    cur = TAB(current);                 /* BUG(orig): 0 not checked (#733) */
    _next = NODE(cur)->next;
    _prev = NODE(cur)->prev;
    if (!_next)
        TAB(last) = nodeptr;
    cur = TAB(current);
    TAB(current) = nodeptr;
    _oldnext = NODE(cur)->next;
    NODE(cur)->next = nodeptr;
    /* BUG(orig): the prev of the node behind (_oldnext) is not set (#727) */
    node = nodeptr;
    NODE(node)->next = _oldnext;
    NODE(node)->prev = cur;
    NODE(node)->pad = 0;
    NODE(node)->data = 0;
    NODE(node)->size = _size;           /* BUG(orig): the last size (#735) */
    return BB_TRUE;
}

/* SetListItem list#,var: a new copy of var for the current node */
BB_BOOL BLITZ_ROUTINE c_setlistitem(ULONG list BL_REG(d0), ULONG var BL_REG(d1))
{
    ULONG mem;

    listnum = list << 2;
    mem = globalloc(getvarsize(var));
    if (!mem)
        return BB_FALSE;
    /* BUG(orig): the old copy is not freed, the size not set (#734, #735) */
    NODE(TAB(current))->data = mem;
    copybyte(myvar, mem, _size);
    return BB_TRUE;
}

/* GetListItem list#,var: the current node's data into var (the size of
   var) */
BB_BOOL BLITZ_ROUTINE c_getlistitem(ULONG list BL_REG(d0), ULONG var BL_REG(d1))
{
    ULONG data;

    listnum = list << 2;
    getvarsize(var);
    data = NODE(TAB(current))->data;
    if (!data)
        return BB_FALSE;
    copybyte(data, myvar, _size);
    return BB_TRUE;
}

/* NextListItem / PrevListItem list#: 0 at the end */
BB_BOOL BLITZ_ROUTINE c_nextlistitem(ULONG list BL_REG(d0))
{
    ULONG n;

    listnum = list << 2;
    n = NODE(TAB(current))->next;
    if (!n)
        return BB_FALSE;
    TAB(current) = n;
    return BB_TRUE;
}

BB_BOOL BLITZ_ROUTINE c_prevlistitem(ULONG list BL_REG(d0))
{
    ULONG n;

    listnum = list << 2;
    n = NODE(TAB(current))->prev;
    if (!n)
        return BB_FALSE;
    TAB(current) = n;
    return BB_TRUE;
}

/* FirstListItem / LastListItem list# */
BB_BOOL BLITZ_ROUTINE c_firstlistitem(ULONG list BL_REG(d0))
{
    listnum = list << 2;
    TAB(current) = TAB(first);
    return BB_TRUE;
}

BB_BOOL BLITZ_ROUTINE c_lastlistitem(ULONG list BL_REG(d0))
{
    listnum = list << 2;
    TAB(current) = TAB(last);
    return BB_TRUE;
}

/* AddLastListItem list#: LastListItem, then AddListItem */
BB_BOOL BLITZ_ROUTINE c_addlastitem(ULONG list BL_REG(d0))
{
    c_lastlistitem(list);
    return c_addlistitem(list);
}

/* AddFirstListItem list#: a new node in front of the first one;
   BUG(orig): see above, the result is list# * 4 (#724, #725, #726) */
ULONG BLITZ_ROUTINE c_addfirstitem(ULONG list BL_REG(d0))
{
    ULONG node;

    listnum = list;
    node = globalloc(NODESIZE);
    if (!node)
        return 0;
    nodeptr = node;
    listnum = list << 2;
    _next = TAB(first);
    TAB(first) = nodeptr;
    NODE(_next)->next = nodeptr;        /* BUG(orig): next, not prev (#724) */
    TAB(current) = nodeptr;
    NODE(nodeptr)->next = _next;
    return listnum;
}

/* DelListItem list#: free the current node; BUG(orig): see above (#729) */
BB_BOOL BLITZ_ROUTINE c_dellistitem(ULONG list BL_REG(d0))
{
    ULONG node;

    listnum = list << 2;
    node = nodeptr = TAB(current);
    if (!node)
        return BB_FALSE;
    /* the v0.9 node: next, prev, data, size (no pad) */
    _next = ((struct node09 *)node)->next;
    _prev = ((struct node09 *)node)->prev;
    _data = ((struct node09 *)node)->data; /* BUG(orig) (#729) */
    pad = ((struct node09 *)node)->pad;
    _size = ((struct node09 *)node)->size;
    if (_data) {
        globfree(_data, _size);
        NODE(nodeptr)->data = 0;
        NODE(nodeptr)->size = 0;
    }
    if (!_prev) {
        if (!_next)
            return BB_FALSE;                   /* the only node */
        NODE(_next)->next = 0;        /* BUG(orig): next, not prev (#728) */
        TAB(first) = _next;
        TAB(current) = _next;
        globfree(nodeptr, NODESIZE);
        return BB_TRUE;
    }
    if (!_next) {
        NODE(_prev)->next = 0;
        TAB(last) = _prev;
        TAB(current) = _prev;
        globfree(nodeptr, NODESIZE);
        return BB_TRUE;
    }
    NODE(_prev)->next = _next;
    NODE(_next)->prev = _prev;
    TAB(current) = _next;
    globfree(nodeptr, NODESIZE);
    return BB_TRUE;
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
