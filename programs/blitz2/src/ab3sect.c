/*
 * ab3sect.c - AmiBlitz3's sections BSS and Chip in AB3 mode (blitz2-ab3;
 * extension points of ext.h, spec ab3-compat/directives, design D6 of the
 * change ab3-mode-g2).
 *
 * After BSS the code compiled goes into a BSS hunk, after Chip into a
 * data hunk loaded to chip memory (AmiBlitz3.ab3:25960-26035 bssseg,
 * chipseg); each once.  A section ends at the other one or at the end of
 * the program (before its end code, AmiBlitz3.ab3:3549-3566 codeend), so
 * the code hunk is the code before the first section and the end code
 * after the last one.  savefile then writes the hunks (AmiBlitz3.ab3:9827
 * save_binaryfile) and moves the relocations to them.  With runtime error
 * checking both are ignored, as AmiBlitz3 does with its debugger.
 */
#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "blitz2.h"
#include "ab3.h"

#define DEBUGON ((debugga & 0xff00) != 0)
#define NOP     0x4e71

/* absolute addresses in the code buffer (pc), 0: none */
static ULONG bssstart, bssend, bssdata;   /* bssdata: end of its bytes */
static ULONG chipstart, chipend;
static ULONG codeend;                     /* the end of the last section */
static BOOL bssopen, chipopen;

void ab3_sectpass(void)
{
    bssstart = bssend = bssdata = 0;
    chipstart = chipend = codeend = 0;
    bssopen = chipopen = FALSE;
}

/* the start of a section: a NOP, then up to a long boundary */
static ULONG sectstart(void)
{
    pokewd(NOP);
    while (pc & 3)
        pokewd(NOP);
    return pc;
}

/* BSS (AmiBlitz3.ab3:25971 bssseg) */
void ab3_bss(void)
{
    if (DEBUGON)
        return;
    if (bssstart)
        err("Only one BSS / Chip Segment allowed");
    if (chipopen)
        chipopen = FALSE;
    bssstart = sectstart();
    if (chipstart)
        chipend = bssstart;
    bssopen = TRUE;
}

/* Chip (AmiBlitz3.ab3:26009 chipseg) */
void ab3_chip(void)
{
    ULONG at;

    if (DEBUGON)
        return;
    if (chipstart)
        err("Only one BSS / Chip Segment allowed");
    at = pc;
    chipstart = sectstart();
    if (bssopen) {
        bssdata = at;
        bssend = chipstart;
        bssopen = FALSE;
    }
    chipopen = TRUE;
}

/* the program is compiled (before its end code): the open section ends,
   the end code starts on a long boundary; a BSS section must have no
   bytes but 0 (AmiBlitz3.ab3:9855-9866) */
void ext_passend(void)
{
    ULONG a;

    if (progmode != XTRA_MODE_AB3 || (!bssstart && !chipstart))
        return;
    a = pc;
    while (pc & 3)
        pokewd(0);
    if (bssopen) {
        bssdata = a;
        bssend = pc;
        bssopen = FALSE;
    }
    if (chipopen) {
        chipend = pc;
        chipopen = FALSE;
    }
    codeend = pc;
    if (bssstart && !nomemleft)
        for (a = bssstart; a < bssdata; a++)
            if (*(UBYTE *)a)
                err("BSS Segment contains no Data!");
}

/* ------------------------------------------------------------ savefile */

#define HUNK_HEADER  0x3f3
#define HUNK_CODE    0x3e9
#define HUNK_DATA    0x3ea
#define HUNK_BSS     0x3eb
#define HUNK_RELOC32 0x3ec
#define HUNK_END     0x3f2
#define MEMF_CHIPHUNK 0x40000000
#define GROUP        65535              /* offsets of one group (#943) */

static UWORD bsshunk, chiphunk;         /* their numbers, 0: none */

/* the hunk of the address a of the buffer and its offset there */
static UWORD where(ULONG a, ULONG *off)
{
    ULONG base = pcat + noinits;

    if (chipstart && a >= chipstart && a < chipend) {
        *off = a - chipstart;
        return chiphunk;
    }
    if (bssstart && a >= bssstart && a < bssend) {
        *off = a - bssstart;
        return bsshunk;
    }
    if (a >= codeend)
        a -= codeend - (bssstart && (!chipstart || bssstart < chipstart) ? bssstart : chipstart);
    *off = a - base;
    return 0;
}

static void put(BPTR fh, ULONG v)
{
    mywrite(fh, &v, 4);
}

/* the RELOC32 block of the hunk h: its relocations by target hunk, in
   groups of at most 65535; nothing without relocations */
static void relocs(BPTR fh, UWORD h, UWORD nhunks, const ULONG *lo, const UBYTE *lt,
                   ULONG n)
{
    ULONG i, count, left;
    UWORD t;
    BOOL any = FALSE;

    for (t = 0; t < nhunks; t++) {
        for (count = 0, i = 0; i < n; i++)
            count += lt[i] == (UBYTE)(h << 4 | t);
        if (!count)
            continue;
        if (!any)
            put(fh, HUNK_RELOC32);
        any = TRUE;
        for (i = 0, left = count; left; ) {
            ULONG g = left > GROUP ? GROUP : left;

            put(fh, g);
            put(fh, t);
            left -= g;
            for (; g; i++)
                if (lt[i] == (UBYTE)(h << 4 | t)) {
                    put(fh, lo[i]);
                    g--;
                }
        }
    }
    if (any)
        put(fh, 0);
}

/* savefile with sections: the hunks code, BSS, chip data (the debug
   information of the labels is not written) */
BOOL ext_savefile(void)
{
    ULONG s0, codelen, taillen, end, n, i, *lo, sz;
    UBYTE *lt;
    UWORD nhunks;
    struct offnode *off;
    BPTR fh;

    if (progmode != XTRA_MODE_AB3 || (!bssstart && !chipstart) || nomemleft)
        return FALSE;
    nhunks = 1;
    bsshunk = bssstart ? nhunks++ : 0;
    chiphunk = chipstart ? nhunks++ : 0;
    s0 = bssstart && (!chipstart || bssstart < chipstart) ? bssstart : chipstart;
    end = pcat + objlen;
    codelen = s0 - (pcat + noinits);
    taillen = end - codeend;

    /* the relocations: their places and targets in the new hunks */
    n = numoffs;
    lo = AllocMem(n * 4 + 4, MEMF_PUBLIC);
    lt = AllocMem(n + 4, MEMF_PUBLIC);
    if (!lo || !lt) {
        if (lo)
            FreeMem(lo, n * 4 + 4);
        if (lt)
            FreeMem(lt, n + 4);
        execerr();
    }
    for (i = 0, off = firstoff; off && i < n; off = off->next, i++) {
        ULONG at = pcat + off->offset, v = L_(at), tv = v;
        UWORD th = 0, lh;

        if ((WORD)W_(at) >= 0)          /* as blitz2: not the negative ones */
            th = where(v, &tv);
        L_(at) = (WORD)W_(at) >= 0 ? tv : v;
        lh = where(at, &lo[i]);
        lt[i] = (UBYTE)(lh << 4 | th);
    }

    fh = Open((STRPTR)execname, MODE_NEWFILE);
    if (!fh) {
        FreeMem(lo, n * 4 + 4);
        FreeMem(lt, n + 4);
        execerr();
    }
    put(fh, HUNK_HEADER);
    put(fh, 0);
    put(fh, nhunks);
    put(fh, 0);
    put(fh, nhunks - 1);
    sz = (codelen + taillen + 3) >> 2;
    put(fh, sz);
    if (bsshunk)
        put(fh, (bssend - bssstart + 3) >> 2);
    if (chiphunk)
        put(fh, ((chipend - chipstart + 3) >> 2) | MEMF_CHIPHUNK);

    put(fh, HUNK_CODE);
    put(fh, sz);
    mywrite(fh, (APTR)(pcat + noinits), (LONG)codelen);
    mywrite(fh, (APTR)codeend, (LONG)((taillen + 3) & ~3UL));
    relocs(fh, 0, nhunks, lo, lt, n);
    put(fh, HUNK_END);
    if (bsshunk) {
        put(fh, HUNK_BSS);
        put(fh, (bssend - bssstart + 3) >> 2);
        put(fh, HUNK_END);
    }
    if (chiphunk) {
        put(fh, HUNK_DATA);
        put(fh, (chipend - chipstart + 3) >> 2);
        mywrite(fh, (APTR)chipstart, (LONG)((chipend - chipstart + 3) & ~3UL));
        relocs(fh, chiphunk, nhunks, lo, lt, n);
        put(fh, HUNK_END);
    }
    Close(fh);
    FreeMem(lo, n * 4 + 4);
    FreeMem(lt, n + 4);
    return TRUE;
}
