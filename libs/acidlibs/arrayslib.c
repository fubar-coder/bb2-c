/*
 * arrayslib - system library 65434 ($C400-$C404): memory of arrays and
 * lists for `Dim` / `Dim List` (tokens newarr, newlocarr, arrmult, blitz2.s
 * equates 141-143; the list variants are newarr+3 / newlocarr+3, see
 * dimwit in blitz2.s).
 *
 * C translation of bblibsrc/arrayslib (the binary matches the source).
 * The compiler (dimwit) emits:
 *   MOVE.l #elementsize,d0
 *   for every dimension: <word expression> ; JSR arrmult ; MOVE.l d0,x(a5)
 *   MOVE #arrayptr,d2 : MOVE #sizeslot,d3 [: MOVE #itemsize,d4]
 *   JSR newarr / newlocarr / newlist / newloclist
 * d2.w = offset of the array pointer, d3.w = offset of the long holding the
 * total size in bytes, both from a5 (globals) or a4 (locals, the original
 * swaps a4/a5 around a call of the global routine).
 *
 * From the original:
 *   A List looks like this!
 *   00.l : Current
 *   04.l : \
 *   08.l :  > - freaky list header for items in existance
 *   12.l : /
 *   16.l : \
 *   20.l :  > - freaky list header for free items
 *   24.l : /
 *   28.l : "lIsT" : list ID
 *              32 long
 *
 * DIFF: newlocarr/newloclist do not swap a4/a5 while calling memlib; they
 * pass the base (a4) to the common C code instead.  memlib does not use
 * a5, so the result is the same.
 */
#include "blitzlib.h"
#include <exec/lists.h>

/* entry points (thunks / C, defined below) */
void newarr(void), newlocarr(void), arrmult(void);
void BLITZ_ROUTINE newlist(WORD ptr BL_REG(d2), WORD size BL_REG(d3),
                           WORD itemsize BL_REG(d4));
void BLITZ_ROUTINE newloclist(WORD ptr BL_REG(d2), WORD size BL_REG(d3),
                              WORD itemsize BL_REG(d4));

/* !syslibheader{#arrayslib,$c400,$c405,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_arrayslib, 0xc400, 0xc405, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(newarr, 0, 0);                    /* $C400 newarr */
BLITZ_NULLSUB(newlocarr, 0, 0);                 /* $C401 newlocarr */
BLITZ_NULLSUB(arrmult, 0, 0);                   /* $C402 arrmult */
BLITZ_NULLSUB(newlist, 0, 0);                   /* $C403 (newarr+3) */
BLITZ_NULLSUB(newloclist, 0, 0);                /* $C404 (newlocarr+3) */
BLITZ_LIBFIN();

/* a list's header (32 bytes in front of the first item): the current
   item, the exec style list headers of the items in use and of the free
   items, the ID "lIsT"; an item starts with its node (next.l, last.l) */
struct listhead {
    UBYTE *current;             /* 00 */
    struct MinList used;        /* 04 */
    struct MinList free;        /* 16 */
    ULONG id;                   /* 28 */
};
static_assert(sizeof(struct listhead) == 32);

#define AT(base, off) (*(ULONG *)((base) + (WORD)(off)))  /* 0(a5,dN.w) */

/* newarr - (re)allocate an array: free the old one (if any), allocate
   0(a5,d3) bytes, cleared, and store the address at 0(a5,d2) */
static ULONG new_array(UBYTE *base, WORD ptr, WORD size)
{
    struct bl_regs r = { 0, 0, 0, 0 };

    if (AT(base, ptr)) {
        r.a1 = AT(base, ptr);
        r.d0 = AT(base, size);
        BLITZ_ALIBJSR(TOK_locfree, r);          /* free old one! */
    }
    r.d0 = AT(base, size);
    r.d1 = MEMF_PUBLIC | MEMF_CLEAR;            /* #$10001 */
    BLITZ_ALIBJSR(TOK_localloc, r);
    AT(base, ptr) = r.d0;
    return r.d0;
}

/**
 * @brief newarr ($C400): (re)allocate the memory of a global array (`Dim`).
 *
 * Frees the old array (memlib locfree) if the pointer at 0(a5,d2) is set,
 * allocates 0(a5,d3) bytes (memlib localloc, MEMF_PUBLIC|MEMF_CLEAR) and
 * stores the address at 0(a5,d2).  a0 (and a1 if nothing was freed) are
 * kept by the original (memlib keeps them) - the thunk keeps both.
 *
 * @param ptr   d2.w: offset of the array pointer from a5
 * @param size  d3.w: offset of the long with the size in bytes from a5
 * @return d0: the array memory
 */
static ULONG BLITZ_ROUTINE newarr_c(WORD ptr BL_REG(d2), WORD size BL_REG(d3))
{
    return new_array(blitz_globals, ptr, size);
}
BLITZ_THUNK(newarr, newarr_c, "a0-a1");

/**
 * @brief newlocarr ($C401): newarr for a local array (`Dim` in a
 *        procedure): the offsets are from a4.
 *
 * DIFF: the original swaps a4/a5 around the call of newarr; here the base
 * (a4) is passed to the common code (memlib does not use a5).
 *
 * @param ptr   d2.w: offset of the array pointer from a4
 * @param size  d3.w: offset of the long with the size in bytes from a4
 * @return d0: the array memory
 */
static ULONG BLITZ_ROUTINE newlocarr_c(WORD ptr BL_REG(d2), WORD size BL_REG(d3))
{
    return new_array(blitz_locals, ptr, size);
}
BLITZ_THUNK(newlocarr, newlocarr_c, "a0-a1");

/**
 * @brief arrmult ($C402): multiply the array size by one dimension
 *        (`Dim`, once per dimension).
 *
 * d0 = d0 * (d1.w + 1), with two 16 bit MULUs:
 *   ADDQ #1,d1:MOVE.l d0,d2:MULU d1,d0
 *   SWAP d2:MULU d1,d2:SWAP d2:ADD.l d2,d0
 * BUG(orig) (#58): the second product is SWAPped, not shifted: its high
 * word (products >= $10000) is added to the low word of the result.
 * DIFF: the original returns d1.w+1 and changes d2; the compiler does not
 * use them (the next dimension is evaluated again), the C build keeps d2
 * and leaves d1 undefined.
 *
 * @param size  d0: the size so far (element size times the dimensions)
 * @param dim   d1.w: the dimension (its highest index)
 * @return d0: the new size
 */
static ULONG BLITZ_ROUTINE arrmult_c(ULONG size BL_REG(d0), ULONG dim BL_REG(d1))
{
    UWORD n = (UWORD)dim + 1;                   /* ADDQ #1,d1 (word) */
    ULONG lo = (ULONG)(UWORD)size * n;          /* MULU d1,d0 */
    ULONG hi = (ULONG)(UWORD)(size >> 16) * n;  /* SWAP d2:MULU d1,d2 */

    hi = hi << 16 | hi >> 16;                   /* SWAP d2 */
    return lo + hi;                             /* ADD.l d2,d0 */
}
BLITZ_THUNK(arrmult, arrmult_c, "a0-a1");

/* newlist - (re)allocate a list: 32 bytes header + 0(base,size) bytes,
   items of itemsize bytes; see newlist below */
static void new_list(UBYTE *base, WORD ptr, WORD size, WORD itemsize)
{
    struct bl_regs r = { 0, 0, 0, 0 };
    struct listhead *h;
    UBYTE *item, *end;
    struct MinNode *tail, *last;

    if (AT(base, ptr)) {
        r.a1 = AT(base, ptr) - 32;              /* MOVEQ #32,d0:SUB d0,a1 */
        r.d0 = 32 + AT(base, size);
        BLITZ_ALIBJSR(TOK_locfree, r);
    }
    r.d0 = 32 + AT(base, size);
    r.d1 = MEMF_PUBLIC | MEMF_CLEAR;
    BLITZ_ALIBJSR(TOK_localloc, r);
    h = (struct listhead *)r.d0;                /* a2 */
    item = (UBYTE *)(h + 1);                    /* a0 */
    AT(base, ptr) = (ULONG)item;
    end = item + AT(base, size);                /* a1 */

    /* Empty out the IN-USE list, FREE list, and set ID */
    h->used.mlh_TailPred = (struct MinNode *)&h->used.mlh_Head;
    h->used.mlh_Head = (struct MinNode *)&h->used.mlh_Tail;
    h->free.mlh_TailPred = (struct MinNode *)&h->free.mlh_Head;
    h->free.mlh_Head = (struct MinNode *)&h->free.mlh_Tail;
    h->id = 0x6c497354;                         /* "lIsT" */

    /* OK, link all things together..... */
    /* BUG(orig) (#59): every linked item gets 8 bytes written (next.l, last.l at
       item+0/+4), but the loop only checks that the item STARTS below
       `end`.  The compiler's Dim List passes itemsize = listsize (element
       size + 8, >= 9) and a size that is a multiple of it, so the last
       item always has its 8 bytes.  Only size 0 (dimension -1, an
       overflowed arrmult, or a direct call with any sizes) writes past the
       block: the do-while still links one item at h+32, so all 8 bytes
       land behind the block.  Kept as in the original. */
    tail = (struct MinNode *)&h->free.mlh_Tail; /* tail node */
    do {                                        /* insert a0 node before a2 node */
        struct MinNode *node = (struct MinNode *)item;

        last = tail->mln_Pred;                  /* old last of tail */
        node->mln_Succ = tail;                  /* new next of me */
        node->mln_Pred = last;                  /* new last of me */
        tail->mln_Pred = node;                  /* new last of tail */
        last->mln_Succ = node;                  /* new next of old last */
        item += itemsize;                       /* ADD d4,a0 (word) */
    } while (item < end);                       /* CMP.l a1,a0:BCS 'loop */
}

/**
 * @brief newlist ($C403, newarr+3): (re)allocate a global list
 *        (`Dim List`).
 *
 * Frees the old list (memlib locfree, from 32 bytes before 0(a5,d2)),
 * allocates the 32 byte header and 0(a5,d3) bytes (memlib localloc,
 * MEMF_PUBLIC|MEMF_CLEAR), sets up the header (empty list of items in
 * use, "lIsT") and links all items of d4.w bytes into the free list;
 * 0(a5,d2) = the first item (header+32).  BUG(orig) (#59): a size of 0
 * still links one item, its 8 bytes land behind the block.  The original
 * changes d0/d1/a0-a3 (a3 too); the C build keeps a2/a3.
 *
 * @param ptr       d2.w: offset of the list pointer from a5
 * @param size      d3.w: offset of the long with the size of the items in
 *                  bytes from a5
 * @param itemsize  d4.w: the size of one item (element size + 8)
 */
void BLITZ_ROUTINE newlist(WORD ptr BL_REG(d2), WORD size BL_REG(d3),
                           WORD itemsize BL_REG(d4))
{
    new_list(blitz_globals, ptr, size, itemsize);
}

/**
 * @brief newloclist ($C404, newlocarr+3): newlist for a local list
 *        (`Dim List` in a procedure): the offsets are from a4.
 *
 * DIFF: no a4/a5 swap, the base (a4) is passed to the common code.
 *
 * @param ptr       d2.w: offset of the list pointer from a4
 * @param size      d3.w: offset of the long with the size of the items in
 *                  bytes from a4
 * @param itemsize  d4.w: the size of one item (element size + 8)
 */
void BLITZ_ROUTINE newloclist(WORD ptr BL_REG(d2), WORD size BL_REG(d3),
                              WORD itemsize BL_REG(d4))
{
    new_list(blitz_locals, ptr, size, itemsize);
}
