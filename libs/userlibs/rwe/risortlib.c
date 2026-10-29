/*
 * risortlib - RWE userlib 13 (Reflective Images): sorting Blitz lists
 * (Dim List) by a string field, optionally by a long priority field
 * first - StringSort, ListBase, StringSortItem, StringSortDir,
 * SortPriorityOffset.
 *
 * C translation of rwelibsrc/sortlib/RISortlib2.bb; reference: the
 * reference build of the source (newer than the BSS2 binary, which lacks
 * SortPriorityOffset and the priority sort of 30/8/96;
 * docs/rwelibs/README.md 3.2).  Notes: docs/rwelibs/port/risortlib.md.
 *
 * A list (the array argument, d0): "lIsT" at -4, the first item at -28,
 * the current item at -32; an item: next (0), prev (4), the data of
 * sizeof.type bytes (8); the item whose next is 0 ends the list.
 *
 *   StringSort list,size[,offset]      sort from the first item
 *   StringSortItem list,size[,offset]  sort from the current item
 *       offset: of the string field (a string pointer) in the data.  A
 *       selection sort: each item is compared with all following ones
 *       (utility.library Stricmp, case-insensitive) and the data (size
 *       bytes) swapped where the order is wrong.  -1 when done, 0 if
 *       utility.library is missing, it is no list, the start item is 0
 *       or a string pointer is 0 (then stopped where it was).
 *   ListBase(list)                     the list's address (d0 unchanged)
 *   StringSortDir dir                  0 ascending, else descending
 *   SortPriorityOffset offset          offset of a long in the data that
 *       is compared first (signed; the larger one comes first, equal
 *       priorities by the string); -1 (the default, every negative
 *       value) disables it
 *
 * init opens utility.library (any version), finit closes it.  No
 * assembler besides the thunks.
 *
 * BUG(orig):
 *   - The priority of the item compared with the others is read before
 *     the test whether the priority is enabled: with the default -1 a
 *     long at data - 1, an odd address - an address error on a 68000
 *     (68020 and up read it).
 *   - Swapped by the priority, the item's string is not loaded: the
 *     compare string becomes the one of the last string comparison
 *     (a1, possibly from an earlier item, or a1 of the caller if none
 *     was made yet), and the priority to compare with is not updated
 *     (the swapped-in item's priority is not taken over).
 *   - size 0 swaps 65536 bytes (DBRA with size - 1 = $FFFF).
 *   - finit closes utility.library without checking it was opened.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#define __NOLIBBASE__
#define UTILITY_BASE_NAME util
#include <proto/utility.h>
#undef __NOLIBBASE__

/* entry points (thunks, below) */
void _stringlistsort2(void), _stringlistsort(void), _listbase(void);
void _stringlistsortitem2(void), _stringlistsortitem(void);
void _stringsortdir(void), _sortpriorityoffset(void);
void _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#risortlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_risortlib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_ARRAY, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_stringlistsort2, 0, 0);
BLITZ_ARGS(BL_ARRAY, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_stringlistsort, 0, 0);
BLITZ_NAME("StringSort", "list,sizeof.type[,offset] - sort from start of list");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(_listbase, 0, 0);
BLITZ_NAME("ListBase", "(list)");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_ARRAY, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_stringlistsortitem2, 0, 0);
BLITZ_ARGS(BL_ARRAY, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_stringlistsortitem, 0, 0);
BLITZ_NAME("StringSortItem", "list,sizeof.type[,offset] - sort from current list item");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_stringsortdir, 0, 0);
BLITZ_NAME("StringSortDir", "direction");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_sortpriorityoffset, 0, 0);
BLITZ_NAME("SortPriorityOffset", "offset -- -1 = disabled");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

#define LIST 0x6c497354UL               /* "lIsT" */
#define FIRSTITEM (-28)
#define CURRITEM (-32)

static struct Library *utillibbase;
static const char utillibname[] = "utility.library";
static UWORD sortdir;
static LONG priority;
static WORD priorityoffset = -1;
static UBYTE *lpntr;

#define NEXT(item) (*(UBYTE **)(item))
#define FIELD(item, off, type) (*(type *)((item) + 8 + (off)))

/* init: utility.library, any version */
static void BLITZ_ROUTINE libinit_c(void)
{
    utillibbase = OpenLibrary((CONST_STRPTR)utillibname, 0);
}

/* finit: BUG(orig): also when OpenLibrary failed (CloseLibrary(0)) */
static void BLITZ_ROUTINE libfinit_c(void)
{
    CloseLibrary(utillibbase);
}

/* SortPriorityOffset offset */
static void BLITZ_ROUTINE sortpriorityoffset_c(UWORD off BL_REG(d0))
{
    priorityoffset = (WORD)off;
}

/* StringSortDir direction */
static void BLITZ_ROUTINE stringsortdir_c(UWORD dir BL_REG(d0))
{
    sortdir = dir;
}

/* ListBase(list) */
static ULONG BLITZ_ROUTINE listbase_c(ULONG list BL_REG(d0))
{
    return list;
}

/* dosort: hdr = the list header field of the start item (d6), off = the
   string field (d5), size (d1), a1 = the caller's a1 (BUG(orig): the
   compare string after a swap by priority before any string compare) */
static ULONG dosort(UBYTE *list, UWORD size, WORD off, WORD hdr, const char *a1)
{
    struct Library *util = utillibbase;
    UWORD d7 = (UWORD)(size - 1);
    const char *a0;
    UBYTE *a3;
    WORD d4;

    if (!util)
        return 0;
    if (*(ULONG *)(list - 4) != LIST)
        return 0;
    if (!*(ULONG *)(list + hdr))
        return 0;
    d4 = priorityoffset;
    a3 = *(UBYTE **)(list + hdr);
    for (;;) {                          /* 'outerloop */
        if (!NEXT(a3))
            return (ULONG)-1;
        lpntr = a3;
        a0 = FIELD(a3, off, const char *);
        if (!a0)
            return 0;
        priority = FIELD(a3, d4, LONG); /* BUG(orig): also with d4 = -1 */
        for (;;) {                      /* '_nitem */
            UBYTE *p, *q;
            ULONG n;
            LONG r;

            a3 = NEXT(a3);
            if (!NEXT(a3))
                break;
            if (d4 >= 0) {
                LONG v = FIELD(a3, d4, LONG);
                if (v < priority)
                    continue;
                if (v > priority)
                    goto doswap;        /* BUG(orig): a1 not loaded */
            }
            a1 = FIELD(a3, off, const char *);
            if (!a1)
                return 0;
            r = Stricmp((CONST_STRPTR)a0, (CONST_STRPTR)a1);
            if (sortdir == 0 ? r <= 0 : r >= 0)
                continue;
doswap:
            a0 = a1;
            p = lpntr + 8;
            q = a3 + 8;
            n = (ULONG)d7 + 1;          /* BUG(orig): size 0 = 65536 */
            do {
                UBYTE b = *p;
                *p++ = *q;
                *q++ = b;
            } while (--n);
        }
        a3 = NEXT(lpntr);               /* 'exitcheck */
        if (!NEXT(a3))
            return (ULONG)-1;
    }
}

/* StringSort list,size[,offset] (d0, d1, d2) */
static ULONG BLITZ_ROUTINE stringlistsort2_c(UBYTE *list BL_REG(d0), UWORD size BL_REG(d1),
                                             const char *a1 BL_REG(a1))
{
    return dosort(list, size, 0, FIRSTITEM, a1);
}

static ULONG BLITZ_ROUTINE stringlistsort_c(UBYTE *list BL_REG(d0), UWORD size BL_REG(d1),
                                            UWORD off BL_REG(d2), const char *a1 BL_REG(a1))
{
    return dosort(list, size, (WORD)off, FIRSTITEM, a1);
}

/* StringSortItem list,size[,offset] */
static ULONG BLITZ_ROUTINE stringlistsortitem2_c(UBYTE *list BL_REG(d0), UWORD size BL_REG(d1),
                                                 const char *a1 BL_REG(a1))
{
    return dosort(list, size, 0, CURRITEM, a1);
}

static ULONG BLITZ_ROUTINE stringlistsortitem_c(UBYTE *list BL_REG(d0), UWORD size BL_REG(d1),
                                                UWORD off BL_REG(d2), const char *a1 BL_REG(a1))
{
    return dosort(list, size, (WORD)off, CURRITEM, a1);
}

/* Registers: the original keeps everything in StringSortDir,
   SortPriorityOffset and ListBase; init/finit change d0-d1/a0-a1/a6 (the
   system calls), the sorts d0-d2 (d2 = 0 in the forms without offset)
   d5-d7/a0-a1/a3/a6 and d1 when they swap.  The thunks keep
   d0-d1/a0-a1 (statements, init, finit) or d1/a0-a1 (the others), the C
   code d2-d7/a2-a6. */
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_sortpriorityoffset, sortpriorityoffset_c, "d0-d1/a0-a1");
BLITZ_THUNK(_stringsortdir, stringsortdir_c, "d0-d1/a0-a1");
BLITZ_THUNK(_listbase, listbase_c, "d1/a0-a1");
BLITZ_THUNK(_stringlistsort2, stringlistsort2_c, "d1/a0-a1");
BLITZ_THUNK(_stringlistsort, stringlistsort_c, "d1/a0-a1");
BLITZ_THUNK(_stringlistsortitem2, stringlistsortitem2_c, "d1/a0-a1");
BLITZ_THUNK(_stringlistsortitem, stringlistsortitem_c, "d1/a0-a1");
