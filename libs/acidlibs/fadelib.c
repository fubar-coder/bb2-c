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
 * Kept from the original (BUG(orig)):
 *   - calcfade shifts a word (MOVE.b:LSR #4) and keeps d0 across the
 *     loop: d0.w at the call is palette# * 128 (the compiler's object
 *     address, lsl.w #7), its bits 8-11 end up in the upper nibble of the
 *     red fraction, bits 12-15 in that of the green fraction of the FIRST
 *     colour only (the shifts empty the upper byte); palette 0/1 unaffected. (#134)
 *   - The checks test the high word of the slice and palette pointers
 *     (TST.w (a1)/(a0)). (#135)
 *   - Without high/low colours the range is 0..min(colours of the
 *     palette, colours of the slice)-1; with them nothing is clipped. (#136)
 *   - Not a bug (both are meant to start at 0): the init clears fadenum and
 *     fadetype together (CLR.l fadenum).
 *   - Wrong messages: an uninitialized palette gives "Not enough colours
 *     in Slice" (err2; mess3 "Uninitialized Palette" is never used) (#330),
 *     a low/high colour beyond the slice's colours "Uninitialized Slice"
 *     (err1) (#331).
 */
#include "blitzlib.h"

/* intlib vwait (Blitz mode): d0 = frames */

/* entry points (thunks / C, defined below) */
void fadechk0(void), fadechk(void), dosetfade(void), dofadestatus(void);
void codeinit(void);
void BLITZ_ROUTINE dofadein2(ULONG d0 BL_REG(d0), const UBYTE *pal BL_REG(a0),
                             UBYTE *slice BL_REG(a1));
void BLITZ_ROUTINE dofadein3(ULONG d0 BL_REG(d0), UWORD rate BL_REG(d1),
                             const UBYTE *pal BL_REG(a0), UBYTE *slice BL_REG(a1));
void BLITZ_ROUTINE dofadein(ULONG d0 BL_REG(d0), UWORD rate BL_REG(d1), UWORD lo BL_REG(d2),
                            UWORD hi BL_REG(d3), const UBYTE *pal BL_REG(a0),
                            UBYTE *slice BL_REG(a1));
void BLITZ_ROUTINE dofadeout2(ULONG d0 BL_REG(d0), const UBYTE *pal BL_REG(a0),
                              UBYTE *slice BL_REG(a1));
void BLITZ_ROUTINE dofadeout3(ULONG d0 BL_REG(d0), UWORD rate BL_REG(d1),
                              const UBYTE *pal BL_REG(a0), UBYTE *slice BL_REG(a1));
void BLITZ_ROUTINE dofadeout(ULONG d0 BL_REG(d0), UWORD rate BL_REG(d1), UWORD lo BL_REG(d2),
                             UWORD hi BL_REG(d3), const UBYTE *pal BL_REG(a0),
                             UBYTE *slice BL_REG(a1));
void BLITZ_ROUTINE doasyncfade(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{157,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_fadelib_header, init, BL_NORET, 0, BLITZ_ERROR_AREA);

#define FADE_FORMS(c2, c3, c)                                               \
    BLITZ_ARGS(BL_WORD);                                                    \
    BLITZ_LIBS(LIB_palettelib, 0x1080, LIB_blitzcoplib, 0x1102);            \
    BLITZ_SUBS(c2, BLITZ_BLITZONLY_CHK(fadechk0), 0);                                \
    BLITZ_ARGS(BL_WORD, BL_WORD);                                           \
    BLITZ_LIBS(LIB_palettelib, 0x1080, LIB_blitzcoplib, 0x1102);            \
    BLITZ_SUBS(c3, BLITZ_BLITZONLY_CHK(fadechk0), 0);                                \
    BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);                         \
    BLITZ_LIBS(LIB_palettelib, 0x1080, LIB_blitzcoplib, 0x1102);            \
    BLITZ_SUBS(c, BLITZ_BLITZONLY_CHK(fadechk), 0)

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

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);

BLITZ_LIBFIN();

/* ********************* CODE BELOW ********************* */

static UBYTE fade[96];          /* rfade, gfade, bfade: fade values */
static UBYTE fadef[96];         /* rfadef, gfadef, bfadef: fractions */
static struct BL_PACKED {
    UWORD fadelow;              /* low fade colour */
    UWORD fadehi;               /* high fade colour */
    UBYTE *fadesl;              /* fade slice */
    UWORD faderate;             /* rate of fade */
    UWORD fadenum;              /* fades left to do */
    UWORD fadetype;             /* 1 = async */
} F;

/* init - CLR.l fadenum (fadetype too) */
static void BLITZ_ROUTINE codeinit_c(void)
{
    *(ULONG *)&F.fadenum = 0;
}
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE dosetfade_c(UWORD t BL_REG(d0))
{
    F.fadetype = t;
}
BLITZ_THUNK(dosetfade, dosetfade_c, "d0-d1/a0-a1");

static UWORD BLITZ_ROUTINE dofadestatus_c(void)
{
    return F.fadenum;
}
BLITZ_THUNK_POST(dofadestatus, dofadestatus_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

/* DoFade - one step: add the fractions, write the colours */
void BLITZ_ROUTINE doasyncfade(void)
{
    UWORD *poke;
    UWORD i;

    if (!F.fadenum)
        return;
    F.fadenum--;
    i = F.fadelow;
    poke = (UWORD *)(*(UBYTE **)(F.fadesl + 18) + 2 + (WORD)(i << 2));
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
static void calcrange(const UBYTE *pal, const UBYTE *slice, UWORD *lo, UWORD *hi)
{
    UWORD h = *(UWORD *)(pal + 4) - 1;

    if (h >= *(UWORD *)(slice + 8))
        h = *(UWORD *)(slice + 8) - 1;
    *lo = 0;
    *hi = h;
}

/* calcfade - the fractions; d0's upper byte comes in (BUG(orig), #134) */
static void calcfade(ULONG d0, const UBYTE *pal, UBYTE *slice, UWORD rate, UWORD lo, UWORD hi)
{
    const UBYTE *rgbs = *(UBYTE **)pal + 4;
    UBYTE *f;
    UWORD a, b;
    UWORD w = (UWORD)d0;                        /* d0.w, upper byte kept */

    if (lo > hi) {
        UWORD t = lo;
        lo = hi;
        hi = t;
    }
    F.fadesl = slice;
    F.fadelow = lo;
    F.fadehi = hi;
    F.faderate = rate;
    F.fadenum = 16;
    f = fadef + lo;
    a = (UWORD)(lo * 12);
    b = (UWORD)(hi * 12);
    do {
        /* MOVE.b x,d0:LSR #4,d0:MOVE.b d0,f - the word shift moves d0's
         * upper byte on: bits 8-11 reach red, bits 12-15 green of the first
         * colour, after that the upper byte is 0 (BUG(orig), #134) */
        w = (UWORD)(((w & 0xff00) | rgbs[(WORD)a]) >> 4);
        f[0] = (UBYTE)w;
        w = (UWORD)(((w & 0xff00) | rgbs[(WORD)a + 4]) >> 4);
        f[32] = (UBYTE)w;
        w = (UWORD)(((w & 0xff00) | rgbs[(WORD)a + 8]) >> 4);
        f[64] = (UBYTE)w;
        f++;
        a += 12;
    } while (a <= b);
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

/* FadeIn pal[,rate[,lo,hi]] - a0 = palette, a1 = used slice */
void BLITZ_ROUTINE dofadein(ULONG d0 BL_REG(d0), UWORD rate BL_REG(d1), UWORD lo BL_REG(d2),
                            UWORD hi BL_REG(d3), const UBYTE *pal BL_REG(a0),
                            UBYTE *slice BL_REG(a1))
{
    int i;

    calcfade(d0, pal, slice, rate, lo, hi);     /* setfadein */
    for (i = 0; i < 96; i++)
        fade[i] = 0;
    dosyncfade();
}

void BLITZ_ROUTINE dofadein3(ULONG d0 BL_REG(d0), UWORD rate BL_REG(d1),
                             const UBYTE *pal BL_REG(a0), UBYTE *slice BL_REG(a1))
{
    UWORD lo, hi;

    calcrange(pal, slice, &lo, &hi);
    dofadein(d0, rate, lo, hi, pal, slice);
}

void BLITZ_ROUTINE dofadein2(ULONG d0 BL_REG(d0), const UBYTE *pal BL_REG(a0),
                             UBYTE *slice BL_REG(a1))
{
    dofadein3(d0, 0, pal, slice);
}

/* FadeOut - start at the full colours, fractions negated */
void BLITZ_ROUTINE dofadeout(ULONG d0 BL_REG(d0), UWORD rate BL_REG(d1), UWORD lo BL_REG(d2),
                             UWORD hi BL_REG(d3), const UBYTE *pal BL_REG(a0),
                             UBYTE *slice BL_REG(a1))
{
    int i;

    calcfade(d0, pal, slice, rate, lo, hi);     /* setfadeout */
    for (i = 0; i < 96; i++) {
        fade[i] = fadef[i] << 4;
        fadef[i] = -fadef[i];
    }
    dosyncfade();
}

void BLITZ_ROUTINE dofadeout3(ULONG d0 BL_REG(d0), UWORD rate BL_REG(d1),
                              const UBYTE *pal BL_REG(a0), UBYTE *slice BL_REG(a1))
{
    UWORD lo, hi;

    calcrange(pal, slice, &lo, &hi);
    dofadeout(d0, rate, lo, hi, pal, slice);
}

void BLITZ_ROUTINE dofadeout2(ULONG d0 BL_REG(d0), const UBYTE *pal BL_REG(a0),
                              UBYTE *slice BL_REG(a1))
{
    dofadeout3(d0, 0, pal, slice);
}

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess1, "Uninitialized Slice");
BLITZ_ERRCHK_MESSAGE(mess2, "Not enough colours in Slice");
BLITZ_ERRCHK_MESSAGE(mess3, "Uninitialized Palette");     /* BUG(orig) (#330): unused */

/* fadechk0 - slice and palette set up (word tests), slice with colours */
static int BLITZ_ERRCHK fadechk0_x(const UWORD *pal, const UWORD *slice)
{
    if (!slice[0]) {
        BLITZ_RUNERR(mess1);
        return 0;
    }
    if (!pal[0] || !slice[4]) {
        BLITZ_RUNERR(mess2);            /* BUG(orig) (#330): also for the palette */
        return 0;
    }
    return 1;
}

static void BLITZ_ERRCHK fadechk0_c(const UWORD *pal BL_REG(a0), const UWORD *slice BL_REG(a1))
{
    fadechk0_x(pal, slice);
}

/* fadechk - and the colour range within the slice */
static void BLITZ_ERRCHK fadechk_c(UWORD lo BL_REG(d2), UWORD hi BL_REG(d3),
                                   const UWORD *pal BL_REG(a0), const UWORD *slice BL_REG(a1))
{
    if (!fadechk0_x(pal, slice))
        return;
    if (lo >= slice[4] || hi >= slice[4])
        BLITZ_RUNERR(mess1);            /* BUG(orig) (#331): err1, not err2 */
}

BLITZ_ERRCHK_THUNK(fadechk0, fadechk0_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(fadechk, fadechk_c, "d0-d1/a0-a1");
