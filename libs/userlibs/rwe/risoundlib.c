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

/* entry points (thunks, below) */
void _loadsounds(void), _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#risoundlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_risoundlib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_audiolib, BL_LA(0) | BL_ITEM | BL_PD(0), LIB_execlib, BL_LA(6));
BLITZ_SUBS(_loadsounds, 0, 0);
BLITZ_NAME("CludgeSound", "sound#,address");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0, LIB_doslib, BL_LA(6));
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

#define ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (c) << 8 | (d))
#define LONGAT(p) (*(ULONG *)(p))
#define WORDAT(p) (*(UWORD *)(p))

/* _libinit, _libfinit: RTS (nothing to set up or free) */
void BLITZ_ROUTINE _libinit(void)
{
}

void BLITZ_ROUTINE _libfinit(void)
{
}

/* CludgeSound sound#,address: d0 = sound#, d1 = the 8SVX file, a0 = the
   sound object */
static ULONG BLITZ_ROUTINE loadsounds_c(ULONG d0 BL_REG(d0), UBYTE *sram BL_REG(d1),
                                        UBYTE *saddr BL_REG(a0))
{
    UBYTE *a0, *src, *dst, *mem;
    ULONG rate, clock, slen;
    UWORD sper, n;
    int chunk;

    if (LONGAT(saddr)) {                /* free the old sample (BUG(orig) #831) */
        struct bl_regs r = { .a1 = LONGAT(saddr) };
        r.d0 = BL_SETW(d0, WORDAT(saddr + 6));
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
    if (LONGAT(sram + 8) != ID('8', 'S', 'V', 'X') || LONGAT(sram + 12) != ID('V', 'H', 'D', 'R'))
        return 0;
    a0 = sram + 12;
    a0 += LONGAT(a0 + 4);               /* BUG(orig): VHDR length 20 only (#832) */
    rate = WORDAT(a0);
    clock = 3579545;                    /* CMP.l d1,d0:BLT: signed */
    sper = 0;
    while ((LONG)clock >= (LONG)rate) { /* BUG(orig): rate 0 never ends (#830) */
        sper++;
        clock -= rate;
    }
    a0 = sram + 12;
    for (chunk = 0; LONGAT(a0) != ID('B', 'O', 'D', 'Y'); ) {
        a0 += LONGAT(a0 + 4);           /* BUG(orig): no pad byte (#828) */
        a0 += 8;
        if (++chunk > 6)
            return 0;
    }
    src = a0 + 8;
    slen = LONGAT(a0 + 4);
    {
        struct bl_regs r = { .d0 = slen, .d1 = MEMF_CHIP };
        mem = dst = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
    }
    if (!mem)
        return 0;
    n = (UWORD)(slen - 1);              /* DBRA: BUG(orig) #829, see above */
    do
        *dst++ = *src++;
    while (n-- != 0);
    LONGAT(saddr) = (ULONG)mem;
    WORDAT(saddr + 4) = sper;
    WORDAT(saddr + 6) = (UWORD)((slen & 0xffff) >> 1);
    LONGAT(saddr + 8) = 0;              /* no loop */
    WORDAT(saddr + 12) = 0;
    WORDAT(saddr + 14) = 0;
    return (ULONG)-1;
}

/* Registers: the original changes d0-d2/a0-a1 (on success a0 = the
   object again) and what memlib changes; the thunk keeps d1/a0/a1. */
BLITZ_THUNK(_loadsounds, loadsounds_c, "d1/a0-a1");
