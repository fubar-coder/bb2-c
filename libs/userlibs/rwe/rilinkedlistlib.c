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
 * node).  Per list
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
 *   - No command checks the list number (#732).
 *   - InitList does not free a list that is already there (#731).
 *   - The size: a word of less than 8 in front of the variable gives a
 *     size of almost 4 GB (the allocation fails) (#736).  No NEWTYPE
 *     variable has one (memlib's length of a NEWTYPE of at most 32767
 *     bytes is 8 or more), only an address that is not such a variable.
 *     Exactly 8 (an empty NEWTYPE) gives a size of 0, for which CopyByte
 *     copies nothing (since #492; the original's CopyByte copied 2^32
 *     bytes).
 *
 * Fixed (intended deviations):
 *   - AddFirstListItem returns -1 (the original: list# * 4, 0 for list 0)
 *     (#725).
 *   - AddFirstListItem clears prev, pad, data and size of the new node
 *     (the original wrote only its next: the rest stayed as the
 *     allocation, flags 0, left it) (#726).
 *   - AddFirstListItem makes the new node the prev of the old first node
 *     (the original wrote it into that node's next, the place of prev in
 *     v0.9: the two nodes pointed to each other, the list became a loop)
 *     (#724).
 *   - AddListItem in the middle sets the prev of the node behind the new
 *     one (the original left it pointing to the old current node: going
 *     back from there skipped the new node) (#727).
 *   - DelListItem of the first node clears the prev of the node behind it,
 *     the new first node (the original cleared its next, the place of
 *     prev in v0.9: the list ended there) (#728).
 *   - A node's size field is the size of its data: 0 for the new node of
 *     AddListItem (the original stored the size of the last variable any
 *     command looked at, or of the node DelListItem last read), the size
 *     of the new copy after SetListItem (the original left the field as
 *     it was) (#735).
 *   - DelListItem reads the node of v0.91 and frees its copy (the
 *     original still read the node of v0.9, without the pad: its "data"
 *     was the long at offset 8, the pad and the upper word of the data
 *     pointer, so it freed a wrong address, memlib's runtime error
 *     "Unable to Free Memory", and the copy was lost) (#729).
 *   - SetListItem frees the node's old copy once the new one is
 *     allocated (the original did not: each call on a node with data
 *     lost a copy until the end) (#734).
 *   - GetListItem copies the smaller of the variable's size and the
 *     node's size field (the original copied the variable's size: into a
 *     larger variable also what lay behind the copy's allocation) (#730).
 *   - A list without nodes: AddListItem, AddLastListItem, SetListItem,
 *     GetListItem, NextListItem, PrevListItem, FirstListItem,
 *     LastListItem and AddFirstListItem return 0 and change nothing, as
 *     the manual says ("0 indicates failure (no list ...)"; no list is
 *     started).  The original took the current node, 0, unchecked (only
 *     DelListItem tested it): they read and wrote from address 0 on
 *     (SetListItem wrote to address 10, PrevListItem made ExecBase the
 *     current node), and FirstListItem/LastListItem returned -1.  init
 *     clears the tables, so that a second run of a program with the
 *     library still loaded starts without the nodes of the run before
 *     (#733).
 *
 * DelListItem on the only node frees nothing and returns 0: not a defect,
 * the command refuses to delete the last node and says so with 0.
 */
#include "blitzlib.h"

/* RIAmosFuncLib (75) CopyByte: index 9 of the token list of the CD's
   BSS2 RIAMOSFUNCLIB.OBJ (d0 = source, d1 = destination, d2 = bytes) */
#define TOK_copybyte ((LIB_riamosfunclib << 7) + 10)

/* entry points (thunks, defined below) */
void _initlist(void), _addlistitem(void), _setlistitem(void);
void rilinkedlistlib_nextlistitem(void), _getlistitem(void);
void _prevlistitem(void), _firstlistitem(void), _lastlistitem(void);
void _addfirstitem(void), _addlastitem(void), _dellistitem(void);
void _addrlistitem(void), _sizeoflistitem(void), rilinkedlistlib_libinit(void);
void rilinkedlistlib_libfinit(void);

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
BLITZ_SUBS(rilinkedlistlib_nextlistitem, 0, 0);
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

BLITZ_NULLSUB_AT(finit, rilinkedlistlib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, rilinkedlistlib_libinit, 0, 0);

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

/**
 * @brief init: clears the tables.
 *
 * Fixed (#733): the original's init did nothing, and its tables keep the
 * nodes of a run before (memlib frees them at its end) when a program is
 * run again with the library still loaded; the commands that now test
 * for a list would take those freed nodes for one.
 */
static void BLITZ_ROUTINE c_libinit(void)
{
    ULONG *t = (ULONG *)&tables;
    int i;

    for (i = 0; i < (int)(sizeof tables / 4); i++)
        t[i] = 0;
}

/**
 * @brief finit: nothing (the lists are not freed).
 * @return d0: 0
 */
static ULONG BLITZ_ROUTINE c_libfinit(void)
{
    return 0;
}

/**
 * @brief SizeOfListItem(var): the size of a variable as the list commands copy it.
 *
 * The word in front of the variable minus 8 (getvarsize); also sets the
 * "last size" the other commands use.  BUG(orig) (#736): a word of less
 * than 8 (no NEWTYPE variable has one) gives almost 4 GB; exactly 8 a
 * size of 0, for which CopyByte copies nothing (since #492).
 * @param var  d0: the variable (BL_VARPTR)
 * @return d0: the size
 */
ULONG BLITZ_ROUTINE c_sizeoflistitem(ULONG var BL_REG(d0))
{
    return getvarsize(var);
}

/**
 * @brief AddrListItem(list#): the address of the current node of a list.
 *
 * list# is not checked (#732).
 * @param list  d0: list# (0-4)
 * @return d0: the current node (struct node), 0 if there is none
 */
ULONG BLITZ_ROUTINE c_addrlistitem(ULONG list BL_REG(d0))
{
    listnum = list << 2;
    return TAB(current);
}

/**
 * @brief InitList list#,var: start a list with one node holding a copy of var.
 *
 * The node and the copy are memlib globalloc'ed (flags 0), the copy is
 * made by RIAmosFuncLib's CopyByte.  BUG(orig): a list that is already
 * there is not freed (#731); list# is not checked (#732); the size as
 * in SizeOfListItem (#736).
 * @param list  d0: list# (0-4)
 * @param var   d1: the variable (BL_VARPTR)
 * @return d0: BB_TRUE, BB_FALSE if there is no memory (the node is freed
 *         again, the list cleared)
 */
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

/**
 * @brief AddListItem list#: a new empty node behind the current one, then the current one.
 *
 * BUG(orig): list# is not checked (#732).  Fixed (#733): without a
 * current node (no list) 0, before anything is allocated (the original
 * read and wrote from address 0 on).  Fixed (#727):
 * the prev of the node behind the new one is the new node (the original
 * left it pointing to the current one).  Fixed (#735): the new node's
 * data and size are 0 (the original's size field was the last size any
 * command looked at).
 * @param list  d0: list# (0-4)
 * @return d0: BB_TRUE, BB_FALSE without a list or memory
 */
BB_BOOL BLITZ_ROUTINE c_addlistitem(ULONG list BL_REG(d0))
{
    ULONG node, cur;

    listnum = list << 2;
    if (!TAB(current))                  /* fixed (#733): was not checked */
        return BB_FALSE;
    node = globalloc(NODESIZE);
    if (!node)
        return BB_FALSE;
    nodeptr = node;
    cur = TAB(current);
    _next = NODE(cur)->next;
    _prev = NODE(cur)->prev;
    if (!_next)
        TAB(last) = nodeptr;
    cur = TAB(current);
    TAB(current) = nodeptr;
    _oldnext = NODE(cur)->next;
    NODE(cur)->next = nodeptr;
    if (_oldnext)                       /* fixed (#727): was not set */
        NODE(_oldnext)->prev = nodeptr;
    node = nodeptr;
    NODE(node)->next = _oldnext;
    NODE(node)->prev = cur;
    NODE(node)->pad = 0;
    NODE(node)->data = 0;
    NODE(node)->size = 0;               /* fixed (#735): was the last size */
    return BB_TRUE;
}

/**
 * @brief SetListItem list#,var: a new copy of var for the current node.
 *
 * Fixed (#733): without a current node (no list) 0, before anything is
 * allocated (the original wrote the copy's address to address 10).
 * Fixed (#734): the node's old copy is freed once the new one is
 * allocated (the original did not free it; without memory the old copy
 * stays).  Fixed (#735): the size field is the new copy's size (the
 * original left it as it was).
 * @param list  d0: list# (0-4)
 * @param var   d1: the variable (BL_VARPTR)
 * @return d0: BB_TRUE, BB_FALSE without a list or memory
 */
BB_BOOL BLITZ_ROUTINE c_setlistitem(ULONG list BL_REG(d0), ULONG var BL_REG(d1))
{
    ULONG mem, node, old;

    listnum = list << 2;
    if (!TAB(current))                  /* fixed (#733): was not checked */
        return BB_FALSE;
    mem = globalloc(getvarsize(var));
    if (!mem)
        return BB_FALSE;
    node = TAB(current);
    old = NODE(node)->data;
    if (old)                            /* fixed (#734): was not freed */
        globfree(old, NODE(node)->size);
    NODE(node)->data = mem;
    NODE(node)->size = _size;           /* fixed (#735): was not set */
    copybyte(myvar, mem, _size);
    return BB_TRUE;
}

/**
 * @brief GetListItem list#,var: copy the current node's data into var.
 *
 * Fixed (#733): without a current node (no list) 0 (the original copied
 * from the pointer at address 10).  Fixed (#730): it copies the smaller
 * of the variable's size and the node's size field, the rest of a larger
 * variable stays as it is (the original copied as many bytes as the
 * variable has, also what lay behind the copy).
 * @param list  d0: list# (0-4)
 * @param var   d1: the variable (BL_VARPTR)
 * @return d0: BB_TRUE, BB_FALSE without a list or if the node has no
 *         data
 */
BB_BOOL BLITZ_ROUTINE c_getlistitem(ULONG list BL_REG(d0), ULONG var BL_REG(d1))
{
    ULONG data, n;

    listnum = list << 2;
    getvarsize(var);
    if (!TAB(current))                  /* fixed (#733): was not checked */
        return BB_FALSE;
    data = NODE(TAB(current))->data;
    if (!data)
        return BB_FALSE;
    n = NODE(TAB(current))->size;       /* fixed (#730): was _size alone */
    if (n > _size)
        n = _size;
    copybyte(data, myvar, n);
    return BB_TRUE;
}

/**
 * @brief NextListItem list#: make the next node the current one.
 *
 * Fixed (#733): without a current node (no list) 0 (the original took
 * the long at address 0 as the next node).
 * @param list  d0: list# (0-4)
 * @return d0: BB_TRUE, BB_FALSE at the end (the current node stays) or
 *         without a list
 */
BB_BOOL BLITZ_ROUTINE c_nextlistitem(ULONG list BL_REG(d0))
{
    ULONG n;

    listnum = list << 2;
    if (!TAB(current))                  /* fixed (#733): was not checked */
        return BB_FALSE;
    n = NODE(TAB(current))->next;
    if (!n)
        return BB_FALSE;
    TAB(current) = n;
    return BB_TRUE;
}

/**
 * @brief PrevListItem list#: make the previous node the current one.
 *
 * Fixed (#733): without a current node (no list) 0 (the original took
 * the long at address 4, ExecBase, as the previous node).
 * @param list  d0: list# (0-4)
 * @return d0: BB_TRUE, BB_FALSE at the start (the current node stays)
 *         or without a list
 */
BB_BOOL BLITZ_ROUTINE c_prevlistitem(ULONG list BL_REG(d0))
{
    ULONG n;

    listnum = list << 2;
    if (!TAB(current))                  /* fixed (#733): was not checked */
        return BB_FALSE;
    n = NODE(TAB(current))->prev;
    if (!n)
        return BB_FALSE;
    TAB(current) = n;
    return BB_TRUE;
}

/**
 * @brief FirstListItem list#: make the first node the current one.
 *
 * Fixed (#733): without a list 0, as the manual says ("0 indicates
 * failure (no list)"; the original returned -1 always).
 * @param list  d0: list# (0-4)
 * @return d0: BB_TRUE, BB_FALSE without a list
 */
BB_BOOL BLITZ_ROUTINE c_firstlistitem(ULONG list BL_REG(d0))
{
    listnum = list << 2;
    TAB(current) = TAB(first);
    return TAB(first) ? BB_TRUE : BB_FALSE; /* fixed (#733): was always -1 */
}

/**
 * @brief LastListItem list#: make the last node the current one.
 *
 * Fixed (#733): without a list 0, as the manual says ("0 indicates
 * failure (no list)"; the original returned -1 always).
 * @param list  d0: list# (0-4)
 * @return d0: BB_TRUE, BB_FALSE without a list
 */
BB_BOOL BLITZ_ROUTINE c_lastlistitem(ULONG list BL_REG(d0))
{
    listnum = list << 2;
    TAB(current) = TAB(last);
    return TAB(last) ? BB_TRUE : BB_FALSE; /* fixed (#733): was always -1 */
}

/**
 * @brief AddLastListItem list#: a new empty node at the end (LastListItem, then AddListItem).
 * @param list  d0: list# (0-4)
 * @return d0: as AddListItem (BB_FALSE without a list, fixed #733)
 */
BB_BOOL BLITZ_ROUTINE c_addlastitem(ULONG list BL_REG(d0))
{
    c_lastlistitem(list);
    return c_addlistitem(list);
}

/**
 * @brief AddFirstListItem list#: a new node in front of the first one, then the current one.
 *
 * Fixed (#733): without a first node (no list) 0, before anything is
 * allocated; it does not start a list (the manual: "0 indicates failure
 * (no list/no RAM)"; the original wrote the new node to address 0 and
 * left the last node 0).  Fixed (#724): the new node becomes the prev of
 * the old first node (the original wrote it into that node's next: the
 * list became a loop).  Fixed (#725): the result is -1, as the manual
 * says (the original returned list# * 4, 0 for list 0).  Fixed (#726):
 * prev, pad, data and size of the new node are cleared (the original
 * left them as the allocation, flags 0, left them).
 * @param list  d0: list# (0-4)
 * @return d0: BB_TRUE, BB_FALSE without a list or memory
 */
BB_BOOL BLITZ_ROUTINE c_addfirstitem(ULONG list BL_REG(d0))
{
    ULONG node;

    listnum = list << 2;
    if (!TAB(first))                    /* fixed (#733): was not checked */
        return BB_FALSE;
    node = globalloc(NODESIZE);
    if (!node)
        return BB_FALSE;
    nodeptr = node;
    _next = TAB(first);
    TAB(first) = nodeptr;
    NODE(_next)->prev = nodeptr;        /* fixed (#724): was its next */
    TAB(current) = nodeptr;
    NODE(nodeptr)->next = _next;
    NODE(nodeptr)->prev = 0;            /* fixed (#726): not cleared */
    NODE(nodeptr)->pad = 0;
    NODE(nodeptr)->data = 0;
    NODE(nodeptr)->size = 0;
    return BB_TRUE;                     /* fixed (#725): was list# * 4 */
}

/**
 * @brief DelListItem list#: delete the current node; the next (or previous) becomes the current one.
 *
 * It sets the last size to that of the node, which no node takes over
 * since #735 (AddListItem stores 0).  Fixed (#729): it reads the node of
 * v0.91 and frees the copy with the node (the original read the node of
 * v0.9: its data was the long at +8, the pad and the upper word of the
 * data pointer, so it freed a wrong address, memlib: "Unable to Free
 * Memory", and the copy was lost); the only node is not deleted and
 * keeps its copy: 0 (no defect; the original freed the "data" before it
 * looked).  Fixed (#728): deleting the first node clears the prev of the
 * node behind it (the original cleared its next: the list ended there).
 * @param list  d0: list# (0-4)
 * @return d0: BB_TRUE, BB_FALSE without a current node or for the only
 *         node
 */
BB_BOOL BLITZ_ROUTINE c_dellistitem(ULONG list BL_REG(d0))
{
    ULONG node;

    listnum = list << 2;
    node = nodeptr = TAB(current);
    if (!node)
        return BB_FALSE;
    /* fixed (#729): the node of v0.91 (the original read the v0.9 one,
       without the pad: data at +8) */
    _next = NODE(node)->next;
    _prev = NODE(node)->prev;
    _data = NODE(node)->data;
    _size = NODE(node)->size;
    if (!_prev && !_next)
        return BB_FALSE;                /* the only node: kept with its copy */
    if (_data) {
        globfree(_data, _size);
        NODE(nodeptr)->data = 0;
        NODE(nodeptr)->size = 0;
    }
    if (!_prev) {
        NODE(_next)->prev = 0;        /* fixed (#728): was its next */
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
BLITZ_THUNK(rilinkedlistlib_libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(rilinkedlistlib_libfinit, c_libfinit, "d1/a0-a1");
BLITZ_THUNK(_sizeoflistitem, c_sizeoflistitem, "d1/a0-a1");
BLITZ_THUNK(_addrlistitem, c_addrlistitem, "d1/a0-a1");
BLITZ_THUNK(_initlist, c_initlist, "d1/a0-a1");
BLITZ_THUNK(_addlistitem, c_addlistitem, "d1/a0-a1");
BLITZ_THUNK(_setlistitem, c_setlistitem, "d1/a0-a1");
BLITZ_THUNK(_getlistitem, c_getlistitem, "d1/a0-a1");
BLITZ_THUNK(rilinkedlistlib_nextlistitem, c_nextlistitem, "d1/a0-a1");
BLITZ_THUNK(_firstlistitem, c_firstlistitem, "d1/a0-a1");
BLITZ_THUNK(_lastlistitem, c_lastlistitem, "d1/a0-a1");
BLITZ_THUNK(_prevlistitem, c_prevlistitem, "d1/a0-a1");
BLITZ_THUNK(_addlastitem, c_addlastitem, "d1/a0-a1");
BLITZ_THUNK(_addfirstitem, c_addfirstitem, "d1/a0-a1");
BLITZ_THUNK(_dellistitem, c_dellistitem, "d1/a0-a1");
