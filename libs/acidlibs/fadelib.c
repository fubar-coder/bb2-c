/*
 * fadelib - command library 157: fading the colours of a slice in Blitz
 * mode - FadeIn, FadeOut, ASyncFade, DoFade, FadeStatus.
 *
 * C translation of bblibsrc/fadelib (!libheader{157,...}: the number is
 * given literally - LIB_fadelib_header, #fadelib of libnums.bb is 102;
 * binary checked: same as the source).  FadeIn/FadeOut/DoFade/FadeStatus are Blitz mode only (err1 =
 * 1 resp. a check + 1).  A fade works on the colour pokes of the used
 * slice (blitzcoplib: slice+18 = copper colour moves, slice+8 = number of
 * colours) in 16 steps of 4 bit colours:
 *   calcfade: per colour from low to high the fraction = high byte of the
 *             palette's component >> 4 (palettelib palette, +4 = RGBs as
 *             longs, 12 bytes per colour);
 *   FadeIn starts at 0 and adds the fraction, FadeOut starts at
 *   fraction << 4 and subtracts it; every step (DoFade) writes
 *   r<<8|g<<4|b of the high nibbles into the copper list.
 * Synchronous (default): 16 steps with VWait rate (intlib $C104) between;
 * ASyncFade On: the program calls DoFade, FadeStatus = steps left.
 *
 * Fixed (intended deviation, #134): calcfade shifted a word (MOVE.b:LSR
 *   #4) and kept d0 across the loop: d0.w at the call is palette# * 128
 *   (the compiler's object address, lsl.w #7), its bits 8-11 ended up in
 *   the upper nibble of the red fraction, bits 12-15 in that of the green
 *   fraction of the first colour (palettes 2 and up).  Here the
 *   components are shifted as bytes.
 * Fixed (intended deviation, #135): the error check tested only the high
 *   word of the palette's data pointer (TST (a0)): a palette whose data
 *   lie in the lower 64 KB counted as not there.  Here the long.  (Its
 *   word test of the slice is right: +0 of a slice is the word y.)
 * Fixed (intended deviation, #330): an uninitialized palette gave "Not
 *   enough colours in Slice" (err2; mess3 "Uninitialized Palette" was
 *   never used); here "Uninitialized Palette".
 * Fixed (intended deviation, #331): a low/high colour beyond the slice's
 *   colours gave "Uninitialized Slice" (err1); here "Not enough colours
 *   in Slice" (err2).
 *
 * Kept from the original (BUG(orig)):
 *   - Without high/low colours the range is 0..min(colours of the
 *     palette, colours of the slice)-1; with them nothing is clipped. (#136)
 *   - Not a bug (both are meant to start at 0): the init clears fadenum and
 *     fadetype together (CLR.l fadenum).
 */
#include "blitzlib.h"
#include "blitzobj.h"

/* intlib vwait (Blitz mode): d0 = frames */

/* entry points (aliases / thunks / C, defined below) */
void fadechk0(void), fadechk(void), dosetfade(void), dofadestatus(void);
void fadelib_codeinit(void);
void BLITZ_ROUTINE dofadein2(const struct bb_palette *pal BL_REG(a0),
                             struct bb_slice *slice BL_REG(a1));
void BLITZ_ROUTINE dofadein3(UWORD rate BL_REG(d1),
                             const struct bb_palette *pal BL_REG(a0),
                             struct bb_slice *slice BL_REG(a1));
void BLITZ_ROUTINE dofadein(UWORD rate BL_REG(d1), UWORD lo BL_REG(d2),
                            UWORD hi BL_REG(d3), const struct bb_palette *pal BL_REG(a0),
                            struct bb_slice *slice BL_REG(a1));
void BLITZ_ROUTINE dofadeout2(const struct bb_palette *pal BL_REG(a0),
                              struct bb_slice *slice BL_REG(a1));
void BLITZ_ROUTINE dofadeout3(UWORD rate BL_REG(d1),
                              const struct bb_palette *pal BL_REG(a0),
                              struct bb_slice *slice BL_REG(a1));
void BLITZ_ROUTINE dofadeout(UWORD rate BL_REG(d1), UWORD lo BL_REG(d2),
                             UWORD hi BL_REG(d3), const struct bb_palette *pal BL_REG(a0),
                             struct bb_slice *slice BL_REG(a1));
void BLITZ_ROUTINE doasyncfade(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{157,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_fadelib_header, init, BL_NORET, 0, BLITZ_ERROR_AREA);

#define FADE_FORMS(palonly, withrate, withrange)                            \
    BLITZ_ARGS(BL_WORD);                                                    \
    BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0, LIB_blitzcoplib, BL_UA1);     \
    BLITZ_SUBS(palonly, BLITZ_BLITZONLY_CHK(fadechk0), 0);                  \
    BLITZ_ARGS(BL_WORD, BL_WORD);                                           \
    BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0, LIB_blitzcoplib, BL_UA1);     \
    BLITZ_SUBS(withrate, BLITZ_BLITZONLY_CHK(fadechk0), 0);                 \
    BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);                         \
    BLITZ_LIBS(LIB_palettelib, BL_IA0|BL_PD0, LIB_blitzcoplib, BL_UA1);     \
    BLITZ_SUBS(withrange, BLITZ_BLITZONLY_CHK(fadechk), 0)

BLITZ_ASTATEMENT();
FADE_FORMS(dofadein2, dofadein3, dofadein);
BLITZ_NAME("FadeIn", "Palette#[,Rate[,Low Colour,High Colour]]");

BLITZ_ASTATEMENT();
FADE_FORMS(dofadeout2, dofadeout3, dofadeout);
BLITZ_NAME("FadeOut", "Palette#[,Rate[,Low Colour,High Colour]]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dosetfade, 0, 0);
BLITZ_NAME("ASyncFade", "On|Off");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doasyncfade, BLITZ_BLITZONLY, 0);
BLITZ_NAME("DoFade", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dofadestatus, BLITZ_BLITZONLY, 0);
BLITZ_NAME("FadeStatus", "");

BLITZ_NULLSUB_AT(init, fadelib_codeinit, 0, 0);

BLITZ_LIBFIN();

/* ********************* CODE BELOW ********************* */

/* the slice's colour moves (a long in blitzcoplib) read as the pointer
   they are: as a cast value GCC would order the address arithmetic
   differently */
#define COLMOVES(s) (*(UBYTE *const *)&(s)->colmoves)

static UBYTE fade[96];          /* rfade, gfade, bfade: fade values */
static UBYTE fadef[96];         /* rfadef, gfadef, bfadef: fractions */

static struct BL_PACKED {
    UWORD fadelow;              /* low fade colour */
    UWORD fadehi;               /* high fade colour */
    struct bb_slice *fadesl;    /* fade slice */
    UWORD faderate;             /* rate of fade */
    UWORD fadenum;              /* fades left to do */
    UWORD fadetype;             /* 1 = async */
} F;

/* init - CLR.l fadenum (fadetype too) */
static void BLITZ_ROUTINE codeinit_c(void)
{
    *(ULONG *)&F.fadenum = 0;
}
BLITZ_THUNK(fadelib_codeinit, codeinit_c, "d0-d1/a0-a1");

/**
 * @brief ASyncFade On|Off: fade synchronously or step by step.
 *
 * Off (default): FadeIn/FadeOut do all 16 steps with VWait rate between
 * them.  On: they only set the fade up, the program calls DoFade.
 *
 * @param type  d0: 0 = synchronous, anything else = asynchronous
 */
static void BLITZ_ROUTINE dosetfade_c(UWORD type BL_REG(d0))
{
    F.fadetype = type;
}
BLITZ_ALIAS(dosetfade, dosetfade_c);

/**
 * @brief FadeStatus: the steps of the fade still to do.
 *
 * Blitz mode only.
 *
 * @return d0.w: the steps left (0..16); the upper word of d0 stays
 */
static UWORD BLITZ_ROUTINE dofadestatus_c(void)
{
    return F.fadenum;
}
BLITZ_ALIAS(dofadestatus, dofadestatus_c);

/**
 * @brief DoFade: one step of the fade.
 *
 * Blitz mode only.  If steps are left: adds the fractions to the colours
 * from the low to the high colour and writes r<<8|g<<4|b of the high
 * nibbles into the copper colour moves of the slice (slice+18).
 */
void BLITZ_ROUTINE doasyncfade(void)
{
    UWORD *poke;
    UWORD i;

    if (!F.fadenum)
        return;
    F.fadenum--;
    i = F.fadelow;
    poke = (UWORD *)(COLMOVES(F.fadesl) + 2 + (WORD)(i << 2));
    do {
        UWORD rgb;

        fade[i] += fadef[i];
        fade[i + 32] += fadef[i + 32];
        fade[i + 64] += fadef[i + 64];
        rgb = (fade[i] & 0xf0) << 4;
        rgb |= fade[i + 32] & 0xf0;
        rgb |= fade[i + 64] >> 4;
        *poke = rgb;
        poke += 2;
        i++;
    } while (i <= F.fadehi);
}

/* calcrange - 0 .. min(palette colours, slice colours) - 1 */
static void calcrange(const struct bb_palette *pal, const struct bb_slice *slice, UWORD *lo,
                      UWORD *hi)
{
    UWORD h = pal->numcols - 1;

    if (h >= slice->colours)
        h = slice->colours - 1;
    *lo = 0;
    *hi = h;
}

/* calcfade - the fractions: the high nibble of every component.  Fixed
   (#134): the original shifted d0 as a word (MOVE.b x,d0:LSR #4,d0), and
   d0.w came in as palette# * 128 (the compiler's object offset): its
   bits 8-15 got into the red and green fractions of the first colour */
static void calcfade(const struct bb_palette *pal, struct bb_slice *slice,
                     UWORD rate, UWORD lo,
                     UWORD hi)
{
    const UBYTE *rgbs = pal->pdata + 4;
    UBYTE *frac;
    UWORD ofs, endofs;

    if (lo > hi) {
        UWORD oldlo = lo;
        lo = hi;
        hi = oldlo;
    }
    F.fadesl = slice;
    F.fadelow = lo;
    F.fadehi = hi;
    F.faderate = rate;
    F.fadenum = 16;
    frac = fadef + lo;
    ofs = (UWORD)(lo * 12);
    endofs = (UWORD)(hi * 12);
    do {
        frac[0] = rgbs[(WORD)ofs] >> 4;         /* fixed #134: byte shifts */
        frac[32] = rgbs[(WORD)ofs + 4] >> 4;
        frac[64] = rgbs[(WORD)ofs + 8] >> 4;
        frac++;
        ofs += 12;
    } while (ofs <= endofs);
}

static void dosyncfade(void)
{
    UWORD n = 15;

    if (F.fadetype)
        return;
    do {
        struct bl_regs r = { .d0 = F.faderate };

        BLITZ_BLIBJSR(TOK_vwait, r);
        doasyncfade();
    } while (n-- != 0);
}

/**
 * @brief FadeIn Palette#,Rate,Low Colour,High Colour: fade the colours
 * of the used slice from black to the palette.
 *
 * Blitz mode only.  Sets up 16 steps (calcfade: per colour the fraction
 * = high byte of the component >> 4; low and high are swapped if low >
 * high), starts at 0 and, with ASyncFade Off, does them all with VWait
 * rate between them.
 * Fixed (#134): the fractions are those of the palette alone (the
 * original added bits of d0.w = palette# * 128 to the red and green
 * fractions of the first colour for palettes 2 and up).
 * By design (#136): the range is not clipped to the colours of palette
 * and slice.
 * Errors (debugger, fadechk): "Uninitialized Slice", "Uninitialized
 * Palette" (fixed #330: the original gave "Not enough colours in Slice"),
 * "Not enough colours in Slice", also for a colour beyond the slice's
 * colours (fixed #331: the original gave "Uninitialized Slice"); fixed
 * (#135): the palette's data pointer is tested as a long.
 *
 * @param rate  d1: the frames to wait between the steps (VWait)
 * @param lo  d2: the low colour
 * @param hi  d3: the high colour
 * @param pal  a0: the palette (!libs #palettelib,$1080)
 * @param slice  a1: the used slice (!libs #blitzcoplib,$1102)
 */
void BLITZ_ROUTINE dofadein(UWORD rate BL_REG(d1), UWORD lo BL_REG(d2),
                            UWORD hi BL_REG(d3), const struct bb_palette *pal BL_REG(a0),
                            struct bb_slice *slice BL_REG(a1))
{
    int i;

    calcfade(pal, slice, rate, lo, hi); /* setfadein */
    for (i = 0; i < 96; i++)
        fade[i] = 0;
    dosyncfade();
}

/**
 * @brief FadeIn Palette#,Rate: fade all colours in.
 *
 * As FadeIn with the range 0 .. min(colours of the palette, colours of
 * the slice) - 1.  Errors (debugger, fadechk0): as FadeIn without the
 * range check.
 *
 * @param rate  d1: the frames to wait between the steps
 * @param pal  a0: the palette
 * @param slice  a1: the used slice
 */
void BLITZ_ROUTINE dofadein3(UWORD rate BL_REG(d1),
                             const struct bb_palette *pal BL_REG(a0),
                             struct bb_slice *slice BL_REG(a1))
{
    UWORD lo, hi;

    calcrange(pal, slice, &lo, &hi);
    dofadein(rate, lo, hi, pal, slice);
}

/**
 * @brief FadeIn Palette#: fade all colours in, rate 0.
 *
 * @param pal  a0: the palette
 * @param slice  a1: the used slice
 */
void BLITZ_ROUTINE dofadein2(const struct bb_palette *pal BL_REG(a0),
                             struct bb_slice *slice BL_REG(a1))
{
    dofadein3(0, pal, slice);
}

/**
 * @brief FadeOut Palette#,Rate,Low Colour,High Colour: fade the colours
 * of the used slice from the palette to black.
 *
 * As FadeIn, but starts at the full colours (fraction << 4) and
 * subtracts the fractions.  Fixed (#134), by design (#136) and errors
 * as FadeIn.
 *
 * @param rate  d1: the frames to wait between the steps (VWait)
 * @param lo  d2: the low colour
 * @param hi  d3: the high colour
 * @param pal  a0: the palette (!libs #palettelib,$1080)
 * @param slice  a1: the used slice (!libs #blitzcoplib,$1102)
 */
void BLITZ_ROUTINE dofadeout(UWORD rate BL_REG(d1), UWORD lo BL_REG(d2),
                             UWORD hi BL_REG(d3), const struct bb_palette *pal BL_REG(a0),
                             struct bb_slice *slice BL_REG(a1))
{
    int i;

    calcfade(pal, slice, rate, lo, hi); /* setfadeout */
    for (i = 0; i < 96; i++) {
        fade[i] = fadef[i] << 4;
        fadef[i] = -fadef[i];
    }
    dosyncfade();
}

/**
 * @brief FadeOut Palette#,Rate: fade all colours out.
 *
 * As FadeOut with the range 0 .. min(colours of the palette, colours of
 * the slice) - 1.
 *
 * @param rate  d1: the frames to wait between the steps
 * @param pal  a0: the palette
 * @param slice  a1: the used slice
 */
void BLITZ_ROUTINE dofadeout3(UWORD rate BL_REG(d1),
                              const struct bb_palette *pal BL_REG(a0),
                              struct bb_slice *slice BL_REG(a1))
{
    UWORD lo, hi;

    calcrange(pal, slice, &lo, &hi);
    dofadeout(rate, lo, hi, pal, slice);
}

/**
 * @brief FadeOut Palette#: fade all colours out, rate 0.
 *
 * @param pal  a0: the palette
 * @param slice  a1: the used slice
 */
void BLITZ_ROUTINE dofadeout2(const struct bb_palette *pal BL_REG(a0),
                              struct bb_slice *slice BL_REG(a1))
{
    dofadeout3(0, pal, slice);
}

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess1, "Uninitialized Slice");
BLITZ_ERRCHK_MESSAGE(mess2, "Not enough colours in Slice");
BLITZ_ERRCHK_MESSAGE(mess3, "Uninitialized Palette");     /* fixed #330: used */

/* fadechk0 - slice (the word y) and palette (its data pointer, a long:
   fixed #135, the original tested its high word) set up, slice with
   colours */
static int BLITZ_ERRCHK fadechk0_x(const struct bb_palette *pal, const struct bb_slice *slice)
{
    if (!slice->y) {
        BLITZ_RUNERR(mess1);
        return 0;
    }
    if (!pal->pdata) {
        BLITZ_RUNERR(mess3);            /* fixed #330: not err2 */
        return 0;
    }
    if (!slice->colours) {
        BLITZ_RUNERR(mess2);
        return 0;
    }
    return 1;
}

static void BLITZ_ERRCHK fadechk0_c(const struct bb_palette *pal BL_REG(a0),
                                    const struct bb_slice *slice BL_REG(a1))
{
    fadechk0_x(pal, slice);
}

/* fadechk - and the colour range within the slice */
static void BLITZ_ERRCHK fadechk_c(UWORD lo BL_REG(d2), UWORD hi BL_REG(d3),
                                   const struct bb_palette *pal BL_REG(a0),
                                   const struct bb_slice *slice BL_REG(a1))
{
    if (!fadechk0_x(pal, slice))
        return;
    if (lo >= slice->colours || hi >= slice->colours)
        BLITZ_RUNERR(mess2);            /* fixed #331: err2, not err1 */
}

BLITZ_ERRCHK_THUNK(fadechk0, fadechk0_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(fadechk, fadechk_c, "d0-d1/a0-a1");
