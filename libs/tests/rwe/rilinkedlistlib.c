/*
 * rwe rilinkedlistlib - the linked list library: the C build or the
 * reference build of rwelibsrc/Listlib/listlib4.bb (rilinkedlistlib.orig,
 * ref/rilinkedlistlib.obj of ctest rwe_refbuild), one set as LIBSET says,
 * observed as cases (rwecmp.h, LIBS_ONE_SET; the expected data
 * libs/tests/expected/rwe_rilinkedlistlib.txt).
 *
 *   rilinkedlistlib DIR  (DIR with rilinkedlistlib and riamosfunclib
 *                         .obj or .orig)
 *
 * memlib is not loaded: stubs (sysload_stubs) for globalloc/globfree
 * allocate from a 64 KB aligned region (blocks filled with a
 * pattern, as AllocMem without MEMF_CLEAR leaves garbage; freed blocks
 * overwritten), log every call (sizes, flags, addresses relative to the
 * region) and can let the n-th allocation of a call fail.  A wrong free
 * (DelListItem, BUG(orig) #729) is only logged - the real memlib would stop
 * with "Unable to Free Memory".  The copies are made by the real
 * RIAmosFuncLib CopyByte of the set (C build / AmiBlitz3 binary).  The
 * variables lie in the region too, with their size word in front.
 * Observed after every call: d0 (addresses relative to the region), the
 * log, the whole used region (the upper word of a region address written
 * as $5247) and the kept registers.
 *
 * Covered: every command and the init/finit; SizeOfListItem (sizes 12, 0
 * words below 8: almost 4 GB); AddrListItem, FirstListItem, LastListItem,
 * DelListItem on empty lists; InitList (also again on a list that has
 * one: the old one is not freed); AddListItem at the end and in the
 * middle (BUG(orig) #727 #735: prev of the node behind not set, the stale size);
 * SetListItem (the old copy not freed, the size field unchanged);
 * GetListItem (with and without data, a variable larger than the copy);
 * NextListItem/PrevListItem over the list and past its ends;
 * AddLastListItem; AddFirstListItem (BUG(orig) #724 #726 #725: the old first's next, the
 * garbage in the new node, the result list# * 4); DelListItem in the
 * middle, at the end, at the start (BUG(orig) #728: the new first's next
 * cleared), the only node (0), with and without data (BUG(orig) #729: the
 * wrong free); every allocation failing (InitList node and data,
 * AddListItem, SetListItem, AddFirstListItem, AddLastListItem); lists 0
 * to 4.
 * Not covered: the commands on an empty list that read and write at
 * address 0 (AddListItem, SetListItem, NextListItem, PrevListItem,
 * AddFirstListItem, GetListItem), list numbers outside 0..4 (the original
 * indexes on into its code, the C build into other data), a size of 0
 * (CopyByte copies 2^32 bytes).
 */
#define LIBS_ONE_SET
#include "rwecmp.h"

#define LIB "rilinkedlistlib"

enum { INITLIST, ADDLISTITEM, SETLISTITEM, NEXTLISTITEM, GETLISTITEM, PREVLISTITEM,
       FIRSTLISTITEM, LASTLISTITEM, ADDFIRSTLISTITEM, ADDLASTLISTITEM, DELLISTITEM,
       ADDRLISTITEM, SIZEOFLISTITEM, FINIT, INIT };

/* ------------------------------------------------------------ memlib */

#define REGION 0x10000UL
#define POOLSZ 0x6000UL                 /* allocations: region + 0 ... */
#define VAROFF 0x7000UL                 /* the variables */
#define VARSZ  0x400UL

static UBYTE *raw;
static UBYTE *base;
static ULONG top;                       /* next free offset of the pool */
static int failin;                      /* the n-th allocation fails (0: none) */
static int nalloc;

#define LOGMAX 200
static struct { ULONG op, a, b; } mlog[LOGMAX];
static int nlog, logdone;

static UWORD hi(void)
{
    return (UWORD)((ULONG)base >> 16);
}

/* an address in the region relative to it (tagged), the region's upper
   word in the low word (DelListItem's misread data pointer) tagged too */
static ULONG norm(ULONG x)
{
    ULONG b = (ULONG)base;

    if (x >= b && x < b + REGION)
        return 0x40000000UL | (x - b);
    if ((x & 0xffff) == hi())
        return 0x20000000UL | (x & 0xffff0000UL);
    return x;
}

static void logit(ULONG op, ULONG a, ULONG b)
{
    if (nlog < LOGMAX) {
        mlog[nlog].op = op;
        mlog[nlog].a = a;
        mlog[nlog].b = b;
    }
    nlog++;
}

static ULONG __attribute__((used)) st_globalloc_c(ULONG size, ULONG flags)
{
    UBYTE *p;
    ULONG i, n;

    logit(1, size, flags);
    if (failin && ++nalloc == failin)
        return 0;
    if (size > POOLSZ - top - 8)
        return 0;
    p = base + top;
    *(ULONG *)p = size;
    for (i = 4; i < size + 8; i++)      /* garbage, the same in both sets */
        p[i] = (UBYTE)(0x80 | ((top + i) * 13));
    n = (size + 8 + 7) & ~7UL;
    top += n;
    return (ULONG)(p + 8);
}

static void __attribute__((used)) st_globfree_c(ULONG mem, ULONG size)
{
    ULONG b = (ULONG)base, i, n;

    logit(2, norm(mem), size);
    if (mem < b + 8 || mem >= b + POOLSZ)
        return;
    n = *(ULONG *)(mem - 8);
    for (i = 0; i < n; i++)
        ((UBYTE *)mem)[i] = 0xdd;
}

/* memlib's register interface: d0 (d1) in, d0 out, all others kept */
void st_globalloc(void), st_globfree(void);
__asm__(
"   .text\n"
"   .even\n"
"_st_globalloc:\n"
"   movem.l d1-d7/a0-a6,-(sp)\n"
"   move.l  d1,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   jsr     _st_globalloc_c\n"
"   addq.l  #8,sp\n"
"   movem.l (sp)+,d1-d7/a0-a6\n"
"   rts\n"
"_st_globfree:\n"
"   movem.l d0-d7/a0-a6,-(sp)\n"
"   move.l  d0,-(sp)\n"
"   move.l  a1,-(sp)\n"
"   jsr     _st_globfree_c\n"
"   addq.l  #8,sp\n"
"   movem.l (sp)+,d0-d7/a0-a6\n"
"   rts\n");

static const struct stub stubs[] = {
    { 0xc000, (APTR)st_globalloc },     /* localloc/locfree for RIAmosFuncLib */
    { 0xc001, (APTR)st_globfree },
    { 0xc002, (APTR)st_globalloc },
    { 0xc003, (APTR)st_globfree },
    { 0, NULL }
};

/* ------------------------------------------------------------ helpers */

/* a variable of `size` bytes in slot k: its word in front = size + 8 */
static ULONG var(int k)
{
    return (ULONG)base + VAROFF + 8 + (ULONG)k * 0x40;
}

static void mkvar(int k, UWORD word, UBYTE seed)
{
    int i;
    UBYTE *v = base + VAROFF + 8 + k * 0x40;
    *(UWORD *)(v - 2) = word;
    for (i = 0; i < 0x30; i++)
        v[i] = (UBYTE)(0x80 | (seed + i * 5));
}

/* the log entries since the last observation, the used region: its
   words that are the region's upper word (of an address in it) written
   as $5247 ("RG"; the garbage bytes and the variables have bit 7 set) */
static void same_state(void)
{
    static UWORD pool[POOLSZ / 2];
    int i;
    ULONG w;
    char buf[40];

    OBS("memlib calls", nlog);
    for (i = logdone; i < nlog && i < LOGMAX; i++) {
        sprintf(buf, "memlib call %d", i);
        OBS(buf, mlog[i].op << 28 ^ mlog[i].a);
        sprintf(buf, "memlib call %d d1", i);
        OBS(buf, mlog[i].b);
    }
    logdone = nlog;
    OBS("pool used", top);
    for (w = 0; w < top && w < POOLSZ; w += 2) {
        UWORD a = *(UWORD *)(base + w);
        pool[w / 2] = a == hi() ? 0x5247 : a;
    }
    OBSBYTES("pool", pool, w);
    OBSBYTES("variables", base + VAROFF, VARSZ);
}

/* a call with d0 = list# (and d1 = a variable), compared */
static void call(const char *rout, int idx, ULONG list, int v, int failn)
{
    ROUT = rout;
    sprintf(VEC, "list %lu var %d fail %d", list, v, failn);
    DO1(ENT(LIB, idx, 0), R.d[0] = list; if (v >= 0) R.d[1] = var(v);
        failin = failn; nalloc = 0);
    failin = 0;
    OBS("d0", norm(R.d[0]));
    same_state();
}

#define ADDR(l) call("AddrListItem", ADDRLISTITEM, l, -1, 0)

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, "riamosfunclib", NULL };
    int l;

    if (argc < 2) {
        printf("usage: rilinkedlistlib DIR\n");
        return 20;
    }
    raw = AllocMem(2 * REGION, MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw) {
        check("rilinkedlistlib: memory for the region", FALSE);
        return 20;
    }
    base = (UBYTE *)(((ULONG)raw + REGION - 1) & ~(REGION - 1));
    sysload_stubs = stubs;
    if (!load_set(argv[1], names)) {
        sysload_stubs = NULL;
        FreeMem(raw, 2 * REGION);
        return 20;
    }
    sysload_stubs = NULL;
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("finit", d1-d7|a0-a3|a6);
    KEEPS("SizeOfListItem", d1-d7|a1-a3|a6);
    KEEPS("AddrListItem", d1-d7|a1-a3|a6);
    KEEPS("FirstListItem", d2-d7|a2|a3|a6);
    KEEPS("LastListItem", d2-d7|a2|a3|a6);
    KEEPS("DelListItem", d1-d7|a2|a3|a6);
    KEEPS("InitList", d3-d7|a3|a6);
    KEEPS("GetListItem", d3-d7|a2|a3|a6);
    KEEPS("AddListItem", d2-d7|a2|a3|a6);
    KEEPS("SetListItem", d3-d7|a2|a3|a6);
    KEEPS("AddLastListItem", d2-d7|a2|a3|a6);
    KEEPS("NextListItem", d2-d7|a2|a3|a6);
    KEEPS("PrevListItem", d2-d7|a2|a3|a6);
    KEEPS("AddFirstListItem", d2-d7|a1-a3|a6);
    /* d0 is the result of every routine, observed by "d0" */
    DONTCARE = 1;
    strcpy(VEC, "");

    ROUT = "init";
    DO1(ENT(LIB, INIT, 0), (void)0);
    ROUT = "finit";
    DO1(ENT(LIB, FINIT, 0), (void)0);
    OBS("finit d0", R.d[0]);
    ROUT = "init";
    DO1(ENT(LIB, INIT, 0), (void)0);

    /* the variables: 12, 20, 6, 32 bytes; size words 3 and 7 */
    mkvar(0, 12 + 8, 0x11);
    mkvar(1, 20 + 8, 0x22);
    mkvar(2, 6 + 8, 0x33);
    mkvar(3, 32 + 8, 0x44);
    mkvar(4, 3, 0x55);
    mkvar(5, 7, 0x66);
    mkvar(6, 12 + 8, 0x77);
    mkvar(7, 32 + 8, 0x00);

    for (l = 0; l < 6; l++) {
        ROUT = "SizeOfListItem";
        sprintf(VEC, "var %d", l);
        DO1(ENT(LIB, SIZEOFLISTITEM, 0), R.d[0] = var(l));
        OBS("d0", R.d[0]);
    }

    /* empty lists */
    for (l = 0; l < 5; l++) {
        ADDR(l);
        call("FirstListItem", FIRSTLISTITEM, l, -1, 0);
        call("LastListItem", LASTLISTITEM, l, -1, 0);
        call("DelListItem", DELLISTITEM, l, -1, 0);
    }

    /* InitList: allocations failing, the size too large */
    call("InitList", INITLIST, 3, 0, 1);
    ADDR(3);
    call("InitList", INITLIST, 3, 0, 2);
    ADDR(3);
    call("InitList", INITLIST, 3, 4, 0);
    ADDR(3);
    call("InitList", INITLIST, 3, 5, 0);

    /* list 0: A B C, then more */
    call("InitList", INITLIST, 0, 0, 0);
    ADDR(0);
    call("GetListItem", GETLISTITEM, 0, 7, 0);          /* 32 bytes from 12 */
    call("AddListItem", ADDLISTITEM, 0, -1, 1);         /* fails */
    call("AddListItem", ADDLISTITEM, 0, -1, 0);
    ADDR(0);
    call("GetListItem", GETLISTITEM, 0, 6, 0);          /* no data: 0 */
    call("SetListItem", SETLISTITEM, 0, 1, 1);          /* fails */
    call("SetListItem", SETLISTITEM, 0, 1, 0);
    call("SetListItem", SETLISTITEM, 0, 2, 0);          /* again: old copy lost */
    call("AddLastListItem", ADDLASTLISTITEM, 0, -1, 1); /* fails, current = last */
    call("AddLastListItem", ADDLASTLISTITEM, 0, -1, 0);
    call("SetListItem", SETLISTITEM, 0, 3, 0);
    call("SetListItem", SETLISTITEM, 0, 4, 0);          /* too large: fails */
    call("FirstListItem", FIRSTLISTITEM, 0, -1, 0);
    for (l = 0; l < 4; l++) {
        call("GetListItem", GETLISTITEM, 0, 6, 0);
        ADDR(0);
        call("NextListItem", NEXTLISTITEM, 0, -1, 0);   /* the last: 0 */
    }
    for (l = 0; l < 4; l++) {
        call("PrevListItem", PREVLISTITEM, 0, -1, 0);   /* the first: 0 */
        ADDR(0);
    }
    /* in the middle: behind the first node */
    call("AddListItem", ADDLISTITEM, 0, -1, 0);
    call("SetListItem", SETLISTITEM, 0, 6, 0);
    call("NextListItem", NEXTLISTITEM, 0, -1, 0);
    call("PrevListItem", PREVLISTITEM, 0, -1, 0);       /* BUG(orig) (#727): the first */
    ADDR(0);
    call("LastListItem", LASTLISTITEM, 0, -1, 0);
    ADDR(0);
    for (l = 0; l < 5; l++)
        call("PrevListItem", PREVLISTITEM, 0, -1, 0);
    /* DelListItem: in the middle (next and prev), at the end, at the
       start, with and without data */
    call("FirstListItem", FIRSTLISTITEM, 0, -1, 0);
    call("NextListItem", NEXTLISTITEM, 0, -1, 0);
    call("NextListItem", NEXTLISTITEM, 0, -1, 0);
    call("DelListItem", DELLISTITEM, 0, -1, 0);
    ADDR(0);
    call("AddListItem", ADDLISTITEM, 0, -1, 0);         /* the size of the last */
    call("DelListItem", DELLISTITEM, 0, -1, 0);         /* no data */
    /* A -> C now (B deleted, D reachable from nothing); X behind C, then
       A deleted at the start (BUG(orig) #728: C's next cleared, X lost), X at
       the end */
    call("AddLastListItem", ADDLASTLISTITEM, 0, -1, 0);
    call("FirstListItem", FIRSTLISTITEM, 0, -1, 0);
    call("DelListItem", DELLISTITEM, 0, -1, 0);
    ADDR(0);
    call("NextListItem", NEXTLISTITEM, 0, -1, 0);       /* 0 */
    call("LastListItem", LASTLISTITEM, 0, -1, 0);
    ADDR(0);
    call("DelListItem", DELLISTITEM, 0, -1, 0);
    ADDR(0);
    call("GetListItem", GETLISTITEM, 0, 3, 0);

    /* list 1: one node, deleted (0) */
    call("InitList", INITLIST, 1, 2, 0);
    call("DelListItem", DELLISTITEM, 1, -1, 0);
    ADDR(1);
    /* list 4: two nodes, the second deleted, then the first */
    call("InitList", INITLIST, 4, 1, 0);
    call("AddLastListItem", ADDLASTLISTITEM, 4, -1, 0);
    call("DelListItem", DELLISTITEM, 4, -1, 0);
    ADDR(4);
    call("DelListItem", DELLISTITEM, 4, -1, 0);
    ADDR(4);
    /* InitList over list 1 (the old one is not freed) */
    call("InitList", INITLIST, 1, 3, 0);
    ADDR(1);

    /* list 2: AddFirstListItem (fails, then twice) */
    call("InitList", INITLIST, 2, 0, 0);
    call("AddFirstListItem", ADDFIRSTLISTITEM, 2, -1, 1);
    call("AddFirstListItem", ADDFIRSTLISTITEM, 2, -1, 0);
    ADDR(2);
    call("AddFirstListItem", ADDFIRSTLISTITEM, 2, -1, 0);
    ADDR(2);
    call("NextListItem", NEXTLISTITEM, 2, -1, 0);
    ADDR(2);
    call("NextListItem", NEXTLISTITEM, 2, -1, 0);       /* BUG(orig) (#724): the loop */
    ADDR(2);
    call("LastListItem", LASTLISTITEM, 2, -1, 0);
    ADDR(2);
    call("FirstListItem", FIRSTLISTITEM, 2, -1, 0);
    ADDR(2);

    for (l = 0; l < 5; l++)
        ADDR(l);
    ROUT = "finit";
    strcpy(VEC, "");
    DO1(ENT(LIB, FINIT, 0), (void)0);
    OBS("finit d0", R.d[0]);

    FreeMem(raw, 2 * REGION);
    return rwe_done("rilinkedlistlib");
}
