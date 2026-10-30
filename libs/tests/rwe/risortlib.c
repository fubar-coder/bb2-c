/*
 * rwe risortlib - StringSort, ListBase, StringSortItem, StringSortDir,
 * SortPriorityOffset: the C build against the reference build of
 * rwelibsrc/sortlib/RISortlib2.bb (blitz2 2.10), method of rwecmp.h.
 *
 *   risortlib DIR     (DIR with risortlib .obj/.orig)
 *
 * The lists are built here, the same on both sets: a header with "lIsT"
 * at -4, the first item at -28, the current one at -32; items next/prev/
 * data, an end item with next 0.  The data hold string pointers (the
 * same strings for both sets), so after a sort the data of the items in
 * list order are compared between the sets byte for byte.  Covered:
 * before init (no utility.library: 0); no list, start item 0, an empty
 * list, one item; random lists of 2..40 items with strings that differ
 * in case only, are equal, empty, or are prefixes of each other, both
 * directions, both start items (first, current), both forms (string at
 * offset 0 / at the offset given), sizes 8..17 (odd ones too), the
 * word arguments with garbage in the upper words; a string pointer 0
 * (stops: 0); the priority sort (SortPriorityOffset, priorities from a
 * small range so that equal ones fall back to the strings), including
 * BUG(orig) #826 - the stale compare string after a swap by priority, which
 * a1 of the caller can be (set to a string here); ListBase; finit.  For
 * lists of lower case strings without the priority the result is also
 * checked to be sorted.  Not covered: size 0 (BUG(orig) #827: swaps 65536
 * bytes), the 68000 address error of the priority read (BUG(orig), #825).
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "risortlib"
enum { E_SORT, E_BASE, E_SORTITEM, E_DIR, E_PRIO, E_FINIT, E_INIT };

#define MAXN 40
#define MAXSIZE 20
#define ITEM (8 + MAXSIZE)

static UBYTE hdr[2][40];                /* the list: hdr + 32 */
static UBYTE items[2][MAXN + 1][ITEM];  /* + the end item */
static UBYTE flat[2][MAXN * MAXSIZE];   /* the data in list order */
static UBYTE init_data[MAXN][MAXSIZE];
static int nitems, isize;

static const char *const pool[] = {
    "apple", "Apple", "APPLE", "apples", "app", "", "banana", "Banana",
    "cherry", "zebra", "Zebra", "_under", "123", "a", "B", "c", "Mango",
    "mango", "kiwi", "KIWI", "kiwis", "x-ray", "X", "~tilde", "[brk",
};
#define NPOOL (sizeof pool / sizeof pool[0])
static const char *const lower[] = {
    "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
    "iota", "kappa", "lambda", "mu", "nu", "xi", "omicron", "pi", "rho",
    "sigma", "tau", "upsilon", "phi", "chi", "psi", "omega", "al", "be",
};
#define NLOWER (sizeof lower / sizeof lower[0])
static const char a1str[] = "middle";   /* a1 of the caller */

#define LIST(s) (hdr[s] + 32)

/* the list on both sets: n items of `size` bytes from init_data; the
   current item = item `curitem` (or 0: none) */
static void build(int n, int size, int curitem)
{
    int s, i;
    nitems = n;
    isize = size;
    for (s = 0; s < 2; s++) {
        UBYTE *l = LIST(s);
        memset(hdr[s], 0, sizeof hdr[s]);
        memset(items[s], 0xee, sizeof items[s]);
        memcpy(l - 4, "lIsT", 4);
        for (i = 0; i <= n; i++) {
            *(UBYTE **)items[s][i] = i < n ? items[s][i + 1] : NULL;
            *(UBYTE **)(items[s][i] + 4) = i ? items[s][i - 1] : l - 28;
            if (i < n)
                memcpy(items[s][i] + 8, init_data[i], size);
        }
        *(UBYTE **)(l - 28) = items[s][0];
        *(UBYTE **)(l - 32) = curitem >= 0 ? items[s][curitem] : NULL;
    }
}

/* the data in list order (following next from the first item) */
static void flatten(void)
{
    int s, i;
    for (s = 0; s < 2; s++) {
        UBYTE *p = *(UBYTE **)(LIST(s) - 28);
        memset(flat[s], 0, sizeof flat[s]);
        for (i = 0; i < nitems && p; i++, p = *(UBYTE **)p)
            memcpy(flat[s] + i * isize, p + 8, isize);
    }
}

static void call_word(int e, const char *name, UWORD v)
{
    ROUT = name;
    DO2(ENT(LIB, e, 0), R[cur].d[0] = (R[cur].d[0] & 0xffff0000UL) | v);
}

/* StringSort / StringSortItem (e), form 0 (string at 0) or 1 (at off) */
static void sort(int e, int form, int off)
{
    ROUT = e == E_SORT ? (form ? "StringSort offset" : "StringSort")
                       : (form ? "StringSortItem offset" : "StringSortItem");
    DO2(ENT(LIB, e, form), R[cur].d[0] = (ULONG)LIST(cur);
        R[cur].d[1] = (R[cur].d[1] & 0xffff0000UL) | (UWORD)isize;
        if (form) R[cur].d[2] = (R[cur].d[2] & 0xffff0000UL) | (UWORD)off;
        R[cur].a[1] = (ULONG)a1str);
    SAME("d0", R[cur].d[0]);
    flatten();
    SAMEBYTES("data", flat, sizeof flat[0]);
    /* the links are not changed */
    BOTH("links", *(UBYTE **)(LIST(cur) - 28) == items[cur][0]
         && *(UBYTE **)items[cur][nitems - (nitems > 0)] == (nitems ? items[cur][nitems] : NULL));
}

/* init_data: strings from `tab` (k of them) at `soff`, priorities from
   -1..prange-2 at `poff` (prange 0: none), the item number in the last
   byte (if free) */
static void fill(int n, int size, int soff, const char *const *tab, int k, int poff,
                 int prange)
{
    int i;
    memset(init_data, 0x33, sizeof init_data);
    for (i = 0; i < n; i++) {
        *(const char **)(init_data[i] + soff) = tab[rnd() % k];
        if (prange)
            *(LONG *)(init_data[i] + poff) = (LONG)(rnd() % prange) - 1;
        if (size - 1 >= soff + 4 && (!prange || size - 1 >= poff + 4))
            init_data[i][size - 1] = (UBYTE)i;
    }
}

/* the strings at soff in list order sorted (dir 0 ascending) */
static int sorted(int s, int soff, int dir)
{
    int i;
    for (i = 1; i < nitems; i++) {
        int c = strcmp(*(const char **)(flat[s] + (i - 1) * isize + soff),
                       *(const char **)(flat[s] + i * isize + soff));
        if (dir ? c < 0 : c > 0)
            return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    int s, i;

    if (argc < 2) {
        printf("usage: risortlib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;

    /* before init: no utility.library - 0 */
    strcpy(VEC, "before init");
    fill(5, 8, 0, pool, NPOOL, 0, 0);
    build(5, 8, 2);
    sort(E_SORT, 0, 0);
    BOTH("0", R[cur].d[0] == 0);
    sort(E_SORTITEM, 1, 0);

    strcpy(VEC, "init");
    ROUT = "init";
    DO2(ENT(LIB, E_INIT, 0), (void)0);

    /* ListBase */
    strcpy(VEC, "ListBase");
    ROUT = "ListBase";
    DO2(ENT(LIB, E_BASE, 0), R[cur].d[0] = 0x12345678);
    SAME("d0", R[cur].d[0]);

    /* no list, no start item, empty, one item */
    strcpy(VEC, "no lIsT");
    fill(6, 8, 0, pool, NPOOL, 0, 0);
    build(6, 8, 0);
    for (s = 0; s < 2; s++)
        LIST(s)[-1] = 'S';
    sort(E_SORT, 0, 0);
    BOTH("0", R[cur].d[0] == 0);
    strcpy(VEC, "no current item");
    build(6, 8, -1);
    sort(E_SORTITEM, 0, 0);
    BOTH("0", R[cur].d[0] == 0);
    strcpy(VEC, "empty list");
    build(0, 8, 0);
    sort(E_SORT, 0, 0);
    BOTH("-1", R[cur].d[0] == 0xffffffffUL);
    strcpy(VEC, "one item");
    fill(1, 8, 0, pool, NPOOL, 0, 0);
    build(1, 8, 0);
    sort(E_SORT, 1, 0);
    BOTH("-1", R[cur].d[0] == 0xffffffffUL);

    /* random lists: strings only */
    for (i = 0; i < 60; i++) {
        int n = 2 + (int)(rnd() % (MAXN - 1));
        int form = i & 1, dir = (i >> 1) & 1, e = (i >> 2) & 1 ? E_SORTITEM : E_SORT;
        int size = 8 + (int)(rnd() % 10);
        int soff = form ? 4 * (int)(rnd() % ((size - 4) / 4 + 1)) : 0;
        int low = i % 5 == 0;
        int curi = e == E_SORTITEM ? (int)(rnd() % n) : 0;
        if (soff + 4 > size)
            soff = 0;
        sprintf(VEC, "#%d: %d items of %d bytes, string at %d, dir %d, from %d%s", i, n, size,
                soff, dir, curi, low ? ", lower case" : "");
        call_word(E_DIR, "StringSortDir", (UWORD)(dir ? (i & 8 ? 0x8000 : 1) : 0));
        fill(n, size, soff, low ? lower : pool, low ? NLOWER : NPOOL, 0, 0);
        build(n, size, curi);
        sort(e, form, soff);
        BOTH("-1", R[cur].d[0] == 0xffffffffUL);
        if (low && e == E_SORT)
            BOTH("sorted", sorted(cur, soff, dir));
    }

    /* a string pointer 0: stops with 0 */
    call_word(E_DIR, "StringSortDir", 0);
    for (i = 0; i < 4; i++) {
        int n = 10;
        sprintf(VEC, "string 0 at item %d", i * 3);
        fill(n, 12, 4, pool, NPOOL, 0, 0);
        *(const char **)(init_data[i * 3] + 4) = NULL;
        build(n, 12, 0);
        sort(E_SORT, 1, 4);
        BOTH("0", R[cur].d[0] == 0);
    }

    /* the priority sort: a long at 0, the string at 4 */
    for (i = 0; i < 40; i++) {
        int n = 2 + (int)(rnd() % (MAXN - 1));
        int dir = i & 1, e = (i >> 1) & 1 ? E_SORTITEM : E_SORT;
        int prange = 1 + (int)(rnd() % 5);
        int curi = e == E_SORTITEM ? (int)(rnd() % n) : 0;
        sprintf(VEC, "priority #%d: %d items, priorities -1..%d, dir %d, from %d", i, n,
                prange - 2, dir, curi);
        call_word(E_DIR, "StringSortDir", (UWORD)dir);
        call_word(E_PRIO, "SortPriorityOffset", 0);
        fill(n, 13, 4, pool, NPOOL, 0, prange);
        build(n, 13, curi);
        sort(e, 1, 4);
        BOTH("-1", R[cur].d[0] == 0xffffffffUL);
    }
    /* priority at 8 with the string at 0 (form 0), and disabled again
       with a negative offset other than -1 */
    for (i = 0; i < 6; i++) {
        int n = 20;
        sprintf(VEC, "priority at 8 #%d", i);
        call_word(E_PRIO, "SortPriorityOffset", (UWORD)(i < 3 ? 8 : -2 - i));
        fill(n, 16, 0, pool, NPOOL, 8, 3);
        build(n, 16, 0);
        sort(E_SORT, 0, 0);
        BOTH("-1", R[cur].d[0] == 0xffffffffUL);
    }
    /* BUG(orig) (#826): the first comparison swaps by priority - the compare
       string becomes a1 of the caller ("middle") */
    {
        static const char *const two[] = { "aaa", "zzz" };
        sprintf(VEC, "stale a1");
        call_word(E_PRIO, "SortPriorityOffset", 0);
        memset(init_data, 0, sizeof init_data);
        for (i = 0; i < 6; i++) {
            *(LONG *)init_data[i] = i;          /* rising: every pair swaps */
            *(const char **)(init_data[i] + 4) = two[i & 1];
            init_data[i][11] = (UBYTE)i;
        }
        build(6, 12, 0);
        sort(E_SORT, 1, 4);
        BOTH("-1", R[cur].d[0] == 0xffffffffUL);
    }
    call_word(E_PRIO, "SortPriorityOffset", 0xffff);

    strcpy(VEC, "finit");
    ROUT = "finit";
    DO2(ENT(LIB, E_FINIT, 0), (void)0);
    return rwe_done(LIB);
}
