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
 * #audiolib,#ia0|#pd0): checks "8SVX"/"VHDR" at address+8/+12, takes
 * the sample rate, finds the BODY chunk among the first seven chunks,
 * frees the object's old sample (memlib globfree; not DecodeSound's
 * in-place "cL" data), copies the BODY into chip
 * memory (memlib globalloc) and fills the object: +0 sample, +4 period
 * (3579545 / rate), +6 length in words, +8 .l/+12 .w/+14 .w = 0 (no
 * loop).  Result -1, or 0 when it is no 8SVX/VHDR file, its rate is 0,
 * it has no BODY, an empty one or one of 131072 bytes or more, or the
 * memory is missing.
 *
 * No assembler besides the thunk.
 *
 * Fixed (intended deviation, docs/libs/deviations.md 3a): the old sample
 * is not freed when audiolib's "cL" mark (+14) is set: after a
 * DecodeSound of chip data the sample is the file's own BODY, which
 * memlib never allocated; the original passed it to globfree, which does
 * not find it ("Unable to Free Memory", TRAP #0) (#945).  The sample
 * rate is samplesPerSec, the word at offset 12 of the VHDR data; the
 * original took the word at VHDR + the chunk length, which is that word
 * only for the usual length 20 (#832).  A rate of 0 gives the result 0:
 * the original's subtraction loop for the period never ended (#830).
 * The chunk walk skips the IFF pad byte behind a chunk of odd length
 * (the original added length + 8 and missed BODY behind it), and a chunk
 * that does not fit into the FORM ends it with the result 0; a file
 * without pad bytes (not valid IFF) no longer loads (#828).  The old
 * sample is freed after all checks of the file, and the object's pointer
 * cleared with it: a call that fails on the file leaves the sound as it
 * was.  The original freed it first and kept the pointer, so a failed
 * call left the object pointing to freed memory, which the next
 * CludgeSound freed again ("Unable to Free Memory") (#831).  The whole
 * BODY is copied and its length in words is length / 2; an empty BODY
 * and one of 131072 bytes or more (more than the word at +6 describes,
 * audiolib's own limit) give the result 0.  The original copied with a
 * DBRA loop on length - 1, ((length - 1) & $FFFF) + 1 bytes - 65536 into
 * the block of an empty BODY, the rest modulo 65536 of a longer one - and
 * took (length & $FFFF) / 2 as the length in words (#829).
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
BLITZ_LIBS(LIB_audiolib, BL_IA0|BL_PD0, LIB_execlib, BL_LA6);
BLITZ_SUBS(risoundlib_loadsounds, 0, 0);
BLITZ_NAME("CludgeSound", "sound#,address");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0, LIB_doslib, BL_LA6);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

#define ID(a, b, c, d) ((ULONG)(a) << 24 | (ULONG)(b) << 16 | (c) << 8 | (d))
#define LONGAT(p) (*(ULONG *)(p))
#define WORDAT(p) (*(UWORD *)(p))

/* the longest BODY: the length in words (+6 of the object) is a word, as
   audiolib's limit for LoadSound and DecodeSound ("Sound too long") */
#define MAXBODY 131071

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
 * Checks "8SVX"/"VHDR" at address+8/+12, takes the sample rate
 * (samplesPerSec, fixed: #832), finds the BODY chunk among the first
 * seven chunks of the FORM (with the IFF pad bytes, fixed: #828), then
 * frees the object's old sample and clears its pointer (memlib globfree;
 * not DecodeSound's in-place "cL" data, fixed: #945; after the checks,
 * fixed: #831), copies all of the BODY into chip memory (memlib
 * globalloc; fixed: #829) and fills the object: sample, period
 * 3579545 / rate, length in words, no loop, the "cL" mark cleared.
 * @param sound  d0: the sound number (globfree gets d0 with the old
 *               length in its low word)
 * @param sram   d1: the 8SVX file
 * @param saddr  a0: the sound object (!libs #audiolib,#ia0|#pd0)
 * @return d0: BB_TRUE, BB_FALSE if it is no 8SVX/VHDR file, its rate is
 *         0 (fixed: #830), it has no BODY among the first seven chunks of
 *         the FORM, the BODY is empty or 131072 bytes or longer (fixed:
 *         #829) - the sound unchanged (fixed: #831) - or the chip memory
 *         is missing (the sound then has no sample)
 */
static BB_BOOL BLITZ_ROUTINE loadsounds_c(ULONG sound BL_REG(d0), UBYTE *sram BL_REG(d1),
                                          struct bb_sound *saddr BL_REG(a0))
{
    UBYTE *iff, *src, *dst, *mem;       /* a0: the current IFF chunk */
    ULONG rate, clock, slen, formlen, off, len;
    UWORD sper;
    int chunk;

    if (LONGAT(sram + 8) != ID('8', 'S', 'V', 'X') || LONGAT(sram + 12) != ID('V', 'H', 'D', 'R'))
        return BB_FALSE;
    rate = WORDAT(sram + 12 + 8 + 12);  /* Fixed (#832): samplesPerSec, not
                                           the word at VHDR + its length */
    if (!rate)                          /* Fixed (#830): the loop never ended */
        return BB_FALSE;
    clock = 3579545;                    /* CMP.l d1,d0:BLT: signed */
    sper = 0;
    while ((LONG)clock >= (LONG)rate) {
        sper++;
        clock -= rate;
    }
    /* Fixed (#828): the walk skips the pad byte behind a chunk of odd
       length and stays inside the FORM (a chunk that does not fit ends
       it with 0) */
    formlen = LONGAT(sram + 4);
    off = 12;
    for (chunk = 0; ; ) {
        if (off > formlen)              /* the chunk header outside the FORM */
            return BB_FALSE;
        if (LONGAT(sram + off) == ID('B', 'O', 'D', 'Y'))
            break;
        len = LONGAT(sram + off + 4);
        if (len > formlen - off)        /* the chunk outside the FORM */
            return BB_FALSE;
        off += 8 + len + (len & 1);
        if (++chunk > 6)
            return BB_FALSE;
    }
    iff = sram + off;
    src = iff + 8;
    slen = LONGAT(iff + 4);
    if (!slen || slen > MAXBODY)        /* Fixed (#829): none, or more than
                                           the word at +6 can describe */
        return BB_FALSE;
    /* Fixed (#831): the old sample is freed after the checks, and the
       object no longer points to it (a failed check leaves the sound as
       it was) */
    if (saddr->data && saddr->mark != BB_CLUDGE) {
                                        /* Fixed (#945): not DecodeSound's
                                           in-place ("cL") data */
        struct bl_regs r = { .a1 = (ULONG)saddr->data };
        r.d0 = BL_SETW(sound, saddr->len);
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
    saddr->data = NULL;
    {
        struct bl_regs r = { .d0 = slen, .d1 = MEMF_CHIP };
        mem = dst = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
    }
    if (!mem)
        return BB_FALSE;
    for (len = slen; len; len--)        /* Fixed (#829): all of it */
        *dst++ = *src++;
    saddr->data = mem;
    saddr->period = sper;
    saddr->len = (UWORD)(slen >> 1);
    saddr->rep = NULL;                  /* no loop */
    saddr->replen = 0;
    saddr->mark = 0;
    return BB_TRUE;
}

/* Registers: the original changes d0-d2/a0-a1 (on success a0 = the
   object again) and what memlib changes; the thunk keeps d1/a0/a1. */
BLITZ_THUNK(risoundlib_loadsounds, loadsounds_c, "d1/a0-a1");
