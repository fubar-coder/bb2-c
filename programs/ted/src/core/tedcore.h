/*
 * tedcore.h - umbrella of the TED core: data structures and constants that
 * every module needs (line list, token list, label list, block registers).
 *
 * Owner: coordinator (changes: notes/changes.md, see notes/CONVENTIONS.md).
 * The core includes only tedtypes.h, tedcore.h, hooks.h, state.h and the
 * other core headers - never an AmigaOS header.
 */
#ifndef TED_TEDCORE_H
#define TED_TEDCORE_H

#include <stddef.h>
#include "tedtypes.h"

/* ------------------------------------------------------------ constants */

#define ngerms   6      /* ted.s:2   pairs in the germs table */
#define cripple  0      /* ted.s:3   0 = full version (filesave/bloksave) */
#define doslen   2048   /* ted.s:14  size of the DOS read buffer dosbuff */
#define sleepwait 1     /* ted.s:279 VBlanks per busy pointer frame - 1 */

/* AllocMem flags / DOS modes as the asm uses them (numbers, so the core
   does not need exec/dos headers) */
#define TED_MEMF_PUBLIC  1
#define TED_MEMF_CHIP    2
#define TED_MEMF_CLEAR   0x10000
#define TED_MODE_OLDFILE 1005
#define TED_MODE_NEWFILE 1006
#define TED_ACCESS_READ  (-2)

/* ------------------------------------------------------------ line list */

/* ted.s:15-20 rsreset: one source line.  Doubly linked, never empty
   (firstitem/enditem/thisitem/numlines in state.h).  blitz2 reads this list
   directly (menurout gets a5 = firstitem), so on the Amiga the layout is
   fixed (asserted in structs.h).  Allocation size = numchars + lenitem,
   MEMF_PUBLIC (alloc2/allocline/makeline); the size is NOT stored, it is
   rebuilt from numchars when freeing (freethis/freeall/freeupa5/join).
   On the host, ted_alloc() adds slack so that `lenitem + n` bytes are
   enough for the host struct (pointers are 8 bytes there). */
struct item {
    struct item *nextitem;      /* +0 next line / NULL */
    struct item *lastitem;      /* +4 previous line / NULL */
    UBYTE numchars;             /* +8 length INCLUDING the 0 byte */
    UBYTE chars[];              /* +9 text (tokenised: token = 2 bytes, bit 7
                                       in the first), 0-terminated */
};
#define lenitem 9               /* ted.s:20 asm header size - NOT sizeof() */

/* blitz2's token list (edstruct+0 -> `tokens`): {next.l, num.w, name\0,
   help\0, even}.  findtoke clears bit 15 of the token word before the
   compare; tokit writes (num >> 8) | $80, num & $FF. */
struct tokennode {
    struct tokennode *next;     /* +0 */
    UWORD num;                  /* +4 */
    char name[];                /* +6 name\0 help\0 (help may be empty) */
};

/* label column list ("mice", ted.s:2887 remember2): 8 byte nodes,
   AllocMem(8, MEMF_PUBLIC) not cleared; `next` of the last node stays
   uninitialised (BUG(orig) #381, survey-03 I.4) - count with nummice. */
struct micenode {
    struct micenode *next;      /* +0 */
    struct item *line;          /* +4 */
};

/* ---------------------------------------------------- register bundles */

/* d0-d3 as left by sortblox/orderem (survey-04 A): x/y of block start and
   end (columns signed, rows unsigned). */
struct quad {
    WORD d0, d1, d2, d3;
};

/* ------------------------------------------- hazards of the original */

/* ORIG_HAZARD("text"): marks a place where the original reads/writes
   outside its buffers or uses a NULL pointer and the C version keeps that
   behaviour.  Empty on the Amiga; on the host it counts, so
   tests can expect or detect it (tests/host/stubs.c). */
#ifdef TED_HOST
void ted_hazard(const char *what, const char *file, int line);
#define ORIG_HAZARD(what) ted_hazard((what), __FILE__, __LINE__)
#else
#define ORIG_HAZARD(what) ((void)0)
#endif

#endif
