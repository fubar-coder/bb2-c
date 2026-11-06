/*
 * risoundlib - RWE userlib 45 (Reflective Images): CludgeSound - an IFF
 * 8SVX sample in memory as a sound object of audiolib.
 *
 * C translation of rwelibsrc/soundlib/risoundlib.bb; reference BSS2
 * RISOUNDLIB.OBJ, which is newer than the source (docs/rwelibs/README.md
 * 3.2): the source plus two unused variables `sloop.l`/`srep.w` behind
 * `sdata` (the first step of risoundlib2.bb).  Nothing addresses them, so
 * the C build has no counterpart; the code is the same.  Notes:
 * docs/rwelibs/port/risoundlib.md.
 *
 * CludgeSound sound#,address (d0, d1; a0 = the sound object, !libs
 * #audiolib,#ia0|#pd0): frees the object's old sample (memlib globfree),
 * checks "8SVX"/"VHDR" at address+8/+12, takes the sample rate, finds
 * the BODY chunk among the first seven chunks, copies it into chip
 * memory (memlib globalloc) and fills the object: +0 sample, +4 period
 * (3579545 / rate), +6 length in words, +8 .l/+12 .w/+14 .w = 0 (no
 * loop).  Result -1, or 0 when it is no 8SVX/VHDR file, has no BODY or
 * the memory is missing.
 *
 * No assembler besides the thunk.
 *
 * BUG(orig):
 *   - The old sample is freed before the file is checked, but the object
 *     keeps the pointer: when CludgeSound then fails, the sound object
 *     points to freed memory (and the next CludgeSound frees it again)
 *     (#831).
 *   - The old sample is freed without looking at audiolib's "cL" mark
 *     (+14): after a DecodeSound of chip data the sample is the file's
 *     own BODY, which memlib never allocated - globfree does not find it
 *     ("Unable to Free Memory", TRAP #0) (#945).
 *   - The sample rate is the word at VHDR + 4(VHDR) (the chunk length):
 *     right only for the usual VHDR length 20 (samplesPerSec at +12 of
 *     the data) (#832); the period is a subtraction loop, rate 0 never
 *     ends (#830).
 *   - The chunk walk adds length + 8 without the IFF pad byte of an odd
 *     chunk (#828).
 *   - The copy is a DBRA loop on length - 1: it copies ((length - 1) &
 *     $FFFF) + 1 bytes - 65536 for an empty BODY, only the rest modulo
 *     65536 for a longer one; the length in words is (length & $FFFF) / 2
 *     (#829).
 */
#include "blitzlib.h"
#include "blitzobj.h"

/* entry points (thunks, below) */
void risoundlib_loadsounds(void);
static void _libinit(void);
static void _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#risoundlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_risoundlib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_audiolib, BL_LA(0) | BL_ITEM | BL_PD(0), LIB_execlib, BL_LA(6));
BLITZ_SUBS(risoundlib_loadsounds, 0, 0);
BLITZ_NAME("CludgeSound", "sound#,address");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0, LIB_doslib, BL_LA(6));
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

#define ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (c) << 8 | (d))
#define LONGAT(p) (*(ULONG *)(p))
#define WORDAT(p) (*(UWORD *)(p))

/* the sound object of audiolib: struct bb_sound (blitzobj.h) */

/** @brief init: RTS (nothing to set up). */
static void BLITZ_ROUTINE _libinit(void)
{
}

/** @brief finit: RTS (nothing to free). */
static void BLITZ_ROUTINE _libfinit(void)
{
}

/**
 * @brief CludgeSound sound#,address: an IFF 8SVX sample in memory as an
 * audiolib sound object.
 *
 * Frees the object's old sample (memlib globfree), checks "8SVX"/"VHDR"
 * at address+8/+12, takes the sample rate, finds the BODY chunk among
 * the first seven chunks, copies it into chip memory (memlib globalloc)
 * and fills the object: sample, period 3579545 / rate, length in words,
 * no loop, the "cL" mark cleared.
 * BUG(orig):
 *   - the old sample is freed before the file is checked and the object
 *     keeps the pointer: a failing call leaves it pointing to freed
 *     memory (#831);
 *   - the old sample is freed without looking at the "cL" mark (#945);
 *   - the rate is the word at VHDR + its chunk length (right for length
 *     20 only, #832); rate 0 never ends the period loop (#830);
 *   - the chunk walk has no IFF pad byte (#828);
 *   - the copy is a DBRA loop on length - 1, the length in words
 *     (length & $FFFF) / 2 (#829).
 * @param sound  d0: the sound number (globfree gets d0 with the old
 *               length in its low word)
 * @param sram   d1: the 8SVX file
 * @param saddr  a0: the sound object (!libs #audiolib,#ia0|#pd0)
 * @return d0: BB_TRUE, BB_FALSE if it is no 8SVX/VHDR file, has no BODY
 *         or the chip memory is missing
 */
static BB_BOOL BLITZ_ROUTINE loadsounds_c(ULONG sound BL_REG(d0), UBYTE *sram BL_REG(d1),
                                          struct bb_sound *saddr BL_REG(a0))
{
    UBYTE *iff, *src, *dst, *mem;       /* a0: the current IFF chunk */
    ULONG rate, clock, slen;
    UWORD sper, n;
    int chunk;

    if (saddr->data) {                  /* free the old sample (BUG(orig) #831),
                                           also "cL" data (BUG(orig) #945) */
        struct bl_regs r = { .a1 = (ULONG)saddr->data };
        r.d0 = BL_SETW(sound, saddr->len);
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
    if (LONGAT(sram + 8) != ID('8', 'S', 'V', 'X') || LONGAT(sram + 12) != ID('V', 'H', 'D', 'R'))
        return BB_FALSE;
    iff = sram + 12;
    iff += LONGAT(iff + 4);             /* BUG(orig): VHDR length 20 only (#832) */
    rate = WORDAT(iff);
    clock = 3579545;                    /* CMP.l d1,d0:BLT: signed */
    sper = 0;
    while ((LONG)clock >= (LONG)rate) { /* BUG(orig): rate 0 never ends (#830) */
        sper++;
        clock -= rate;
    }
    iff = sram + 12;
    for (chunk = 0; LONGAT(iff) != ID('B', 'O', 'D', 'Y'); ) {
        iff += LONGAT(iff + 4);         /* BUG(orig): no pad byte (#828) */
        iff += 8;
        if (++chunk > 6)
            return BB_FALSE;
    }
    src = iff + 8;
    slen = LONGAT(iff + 4);
    {
        struct bl_regs r = { .d0 = slen, .d1 = MEMF_CHIP };
        mem = dst = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
    }
    if (!mem)
        return BB_FALSE;
    n = (UWORD)(slen - 1);              /* DBRA: BUG(orig) #829, see above */
    do
        *dst++ = *src++;
    while (n-- != 0);
    saddr->data = mem;
    saddr->period = sper;
    saddr->len = (UWORD)((slen & 0xffff) >> 1);
    saddr->rep = NULL;                  /* no loop */
    saddr->replen = 0;
    saddr->mark = 0;
    return BB_TRUE;
}

/* Registers: the original changes d0-d2/a0-a1 (on success a0 = the
   object again) and what memlib changes; the thunk keeps d1/a0/a1. */
BLITZ_THUNK(risoundlib_loadsounds, loadsounds_c, "d1/a0-a1");
