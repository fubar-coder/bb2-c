/*
 * palettelib - command library 156: palettes and colour cycling -
 * LoadPalette, Cycle, RGB, Red, Green, Blue, PalRGB, SetCycle, StopCycle,
 * AGARGB, AGAPalRGB, AGARed, AGAGreen, AGABlue, ShowPalette,
 * NewPaletteMode, CyclePalette, FadePalette, InitPalette, PaletteRange,
 * DuplicatePalette, DecodePalette, MixPalette.
 *
 * C translation of the CD binary (acidlibs 156).  The binary is newer than
 * bblibsrc/palettelib: Red/Green/Blue take a #long, make4bitrgbs uses the
 * LOW byte of each component (source: the high byte), AGARed/AGAGreen/
 * AGABlue check the screen and mask the colour number - ported from the
 * binary, the rest is the same as the source.  make4bitrgbs takes the TOP
 * byte here (fixed, #224, see below).
 *
 * An object library (!libfin{_toke,...,4,7}): the Blitz object "Palette",
 * default maximum 4, objects of 2^7 = 128 bytes:
 *   +0  palette data (memlib globalloc, MEMF_PUBLIC|MEMF_CLEAR):
 *       numcols.w, 0.w, then red, green, blue as longs (8 bit value
 *       repeated in every byte by PalRGB/AGAPalRGB/LoadPalette; only the
 *       top byte by DecodePalette), 0.l - the table for LoadRGB32
 *   +4.w number of colours
 *   +6  14 cycles {low.w (<0 = end), high.w, speed.w (16384 = one step
 *       per frame, sign = direction), var.w}
 *   _use = ShowPalette unless NewPaletteMode On, _free = freepal.
 *   init (a0 = graphics): agamode = ExecVersion >= 39 (environlib).
 *
 * Several tokens have Blitz and Amiga variants (`Dc $a001`, also some
 * checks): Blitz mode works on the colour moves of the used slice
 * (blitzcoplib: slice+18 = copper list colour moves, +8 = colours),
 * Amiga mode on the used screen's ViewPort (graphics SetRGB4, GetRGB4,
 * LoadRGB4, LoadRGB32, GetRGB32).
 *
 * Kept from the original (BUG(orig)):
 *   - Not a bug (the results are right): FadePalette and MixPalette read
 *     the source components 2 bytes early (palette data + 2): the MULU
 *     takes the upper word of each component (equal to the lower one for
 *     PalRGB colours, the only non-zero part for DecodePalette colours).
 *   - Fixed (intended deviation, #224): make4bitrgbs (Amiga mode with
 *     agamode = 0, i.e. Kickstart < 3.0: ShowPalette/cycling) used the
 *     low byte of each component in the original: DecodePalette colours
 *     (top byte only) came out black there, and the results of
 *     FadePalette/MixPalette (the colour in the top byte) wrong.  Here it
 *     takes the top 4 bits of the top byte (the same for colours with the
 *     byte repeated: PalRGB, AGAPalRGB, LoadPalette).  The older source's
 *     version (top byte, but MOVE.b of the whole green byte) would mix
 *     green's low nibble into blue.
 *   - Not a bug (both are overwritten later): LoadPalette with an offset
 *     first writes colour offset+count with the offset as red value
 *     (overwritten by the loop); CRNG ranges with low = high only
 *     overwrite the low word of the cycle entry.
 *   - DecodePalette walks the chunks without pad bytes (#225), decodes
 *     count = DIVU length/3 (the remainder stays in the upper word), sets
 *     var to the CRNG flag (1) for active cycles (not a bug: only the low
 *     word is used, a start value of 1 is invisible) and re-initialises
 *     (clears) the palette if it is too small (#226).
 *   - PaletteRange with start = end divides by zero (DIVS). (#227)
 *   - StopCycle does not reset `cycon`: a later Cycle does not install
 *     the interrupt again (#228).  The cycle list is freed at the end but
 *     not cleared (not a bug: only at program end).  InitPalette writes no
 *     end marker for the cycles (#229).
 *   - ShowPalette/Use Palette in Amiga mode with an empty palette run
 *     into b_showpal (V39: a0 from somewhere; else make4bitrgbs, a0 = the
 *     palette) and write nothing (count 0) - here both simply return (DIFF). (#230)
 *   - Not a bug (overflows only for palettes of 2731+ colours): sizes with
 *     ADDQ.w #8 (InitPalette), word indices (MULU #12 + LEA 4(a0,d1.w)).
 * DIFF: the original jumps from the normal code into the error check area
 * (AGARed/Green/Blue err3) and uses messages from there (err0, err1,
 * errd); here these runtime errors and messages are in the normal code.
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include <stddef.h>
#include <graphics/view.h>
#include <intuition/screens.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* iffiolib (#iffiolib LSL 7 + n): open (a0 = name), close, byte, word,
   long, find (d0 = id -> d0 = length); Z = ok */
/* environlib ExecVersion */
/* intlib SetInt/ClrInt (Amiga mode): d0 = level, d1 = code */

/* a palette object, 2^7 bytes: struct bb_palette, its colour cycles struct
   bb_cycle (blitzobj.h); the slice (blitzcoplib) struct bb_slice, its colour
   moves (a long there) read as the pointer they are: as a cast value GCC
   would order the address arithmetic differently */
#define COLMOVES(s) (*(UBYTE *const *)&(s)->colmoves)

/* entry points (thunks / C / asm, defined below) */
void dopalload(void), dopalload2(void), getpal(void), getpal2(void);
void docycle(void), dorgb(void), dored(void), dogreen(void), doblue(void);
void do4bitpalrgb(void), dopalrgb(void), dosetcycle1(void), dosetcycle2(void);
void dostopcycle(void), doaargb(void), make4bitrgbs(void);
void doagared(void), doagagreen(void), doagablue(void), showpal(void);
void newpalmode(void), _cyclepalette(void), _fadepalette(void), _initpalette(void);
void _paletterange(void), _duplicatepalette(void), _cludgepalette(void);
void _cludgepalette2(void), _mixpalette(void), usepal(void), freepal(void);
void codeinit(void), codefinit(void), cycleem(void);
void rgbchk(void), colchk(void), prgbchk(void), setcyclechk(void), setcyclechk2(void);
void aargbchk(void), aaprgbchk(void), agacolchk(void), showpalchk(void);
void usepalchk(void), docyclechk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#palettelib,init,1,finit,errors} */
BLITZ_LIBHEADER(LIB_palettelib, init, BL_LONGRET, finit, BLITZ_ERROR_AREA);

/* !libs codes of the Blitz/Amiga tokens: graphics in a6 (Amiga mode),
   used screen in a0/a2 (Amiga), used slice in a0 (Blitz) */
#define L_GFXA   LIB_graphicslib, 0x5600
#define L_SCRA0  LIB_screenslib, 0x5002
#define L_SCRA2  LIB_screenslib, 0x5202
#define L_SLICE  LIB_blitzcoplib, 0x9002

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_palettelib, 0x1180);
BLITZ_SUBS(BLITZ_AMIGAONLY(dopalload), 0, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_palettelib, 0x1180);
BLITZ_SUBS(BLITZ_AMIGAONLY(dopalload2), 0, 0);
BLITZ_NAME("LoadPalette", "Palette#,Filename$[,Palette Offset]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_palettelib, 0x1380, LIB_screenslib, 0x1202);
BLITZ_SUBS(BLITZ_AMIGAONLY(docycle), docyclechk, 0);
BLITZ_NAME("Cycle", "Palette#");

BLITZ_NULLSUB(getpal, 0, 0);
BLITZ_NULLSUB(getpal2, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(L_GFXA, L_SCRA0, L_SLICE);
BLITZ_SUBS(dorgb, rgbchk, 0);
BLITZ_NAME("RGB", "Colour Register,Red,Green,Blue");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(L_GFXA, L_SCRA0, L_SLICE);
BLITZ_SUBS(dored, colchk, 0);
BLITZ_NAME("Red", "(Colour Register)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(L_GFXA, L_SCRA0, L_SLICE);
BLITZ_SUBS(dogreen, colchk, 0);
BLITZ_NAME("Green", "(Colour Register)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(L_GFXA, L_SCRA0, L_SLICE);
BLITZ_SUBS(doblue, colchk, 0);
BLITZ_NAME("Blue", "(Colour Register)");

BLITZ_DUMTOKE(_toke, "Palette", "colour and cycling information");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, 0x1080);
BLITZ_SUBS(do4bitpalrgb, prgbchk, 0);
BLITZ_NAME("PalRGB", "Palette#,Colour Register,Red,Green,Blue");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, 0x1080);
BLITZ_SUBS(dosetcycle1, setcyclechk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_QUICK);
BLITZ_LIBS(LIB_palettelib, 0x1080);
BLITZ_SUBS(dosetcycle2, setcyclechk2, 0);
BLITZ_NAME("SetCycle", "Palette#,Cycle,Low Colour,High Colour[,Speed]");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dostopcycle, 0, 0);
BLITZ_NAME("StopCycle", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_screenslib, 0x1002, LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(BLITZ_AMIGAONLY(doaargb), aargbchk, 0);
BLITZ_NAME("AGARGB", "Colour Register,Red,Green,Blue");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, 0x1080);
BLITZ_SUBS(dopalrgb, aaprgbchk, 0);
BLITZ_NAME("AGAPalRGB", "Palette#,Colour Register,Red,Green,Blue");

BLITZ_NULLSUB(make4bitrgbs, 0, 0);

#define AGA_FN(code, name)                                              \
    BLITZ_AFUNCTION(BL_WORD);                                           \
    BLITZ_ARGS(BL_WORD);                                                \
    BLITZ_LIBS(LIB_screenslib, 0x1002, LIB_graphicslib, BL_LA(6));      \
    BLITZ_SUBS(code, agacolchk, 0);                                     \
    BLITZ_NAME(name, "(Colour Register)")

AGA_FN(doagared, "AGARed");
AGA_FN(doagagreen, "AGAGreen");
AGA_FN(doagablue, "AGABlue");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_palettelib, 0x1380, L_SCRA2, L_GFXA, L_SLICE);
BLITZ_SUBS(showpal, showpalchk, 0);
BLITZ_NAME("ShowPalette", "Palette#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(newpalmode, 0, 0);
BLITZ_NAME("NewPaletteMode", "On|Off");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_palettelib, 0x1280);
BLITZ_SUBS(_cyclepalette, 0, 0);
BLITZ_NAME("CyclePalette", "Palette#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_QUICK);
BLITZ_LIBS(LIB_palettelib, 0x1080, LIB_palettelib, 0x1181);
BLITZ_SUBS(_fadepalette, 0, 0);
BLITZ_NAME("FadePalette", "SrcPalette#,DestPalette#,Brightness.q");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, 0x1380);
BLITZ_SUBS(_initpalette, 0, 0);
BLITZ_NAME("InitPalette", "Palette#,NumColors");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, 0x1380);
BLITZ_SUBS(_paletterange, 0, 0);
BLITZ_NAME("PaletteRange", "Palette#,StartCol,EndCol,r0,g0,b0,r1,g1,b1");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_palettelib, 0x1280, LIB_palettelib, 0x1381);
BLITZ_SUBS(_duplicatepalette, 0, 0);
BLITZ_NAME("DuplicatePalette", "SrcPalette#,DestPalette#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_palettelib, 0x1380);
BLITZ_SUBS(_cludgepalette, 0, 0);
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_palettelib, 0x1380);
BLITZ_SUBS(_cludgepalette2, 0, 0);
BLITZ_NAME("DecodePalette", "Palette#,MemoryLocation[,Palette Offset]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_QUICK);
BLITZ_LIBS(LIB_palettelib, 0x1080, LIB_palettelib, 0x1281, LIB_palettelib, 0x1382);
BLITZ_SUBS(_mixpalette, 0, 0);
BLITZ_NAME("MixPalette", "Palette#,Palette#,DestPalette#,fraction");

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, usepal, usepalchk, 0, L_SCRA2, L_GFXA, L_SLICE);
BLITZ_NULLSUB_AT(_free, freepal, 0, 0);
BLITZ_NULLSUB_AT(init, codeinit, 0, 0, LIB_graphicslib, BL_LA(0));
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,4,7} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 4, 7);

/* ************** CODE BELOW *************** */

static struct Library *gfxbase;
static WORD cycon;
/* the cycle list: {next, palette, viewport} (12 bytes) */
struct cnode {
    struct cnode *next;
    struct bb_palette *pal;
    struct ViewPort *vp;
};
static struct cnode *cyclelist;
static WORD agamode;
static UWORD rgbs4bit[32];      /* 32 4 bit RGB values */
static WORD palmode;
static UWORD myoff;
static ULONG agatable[3];
/* AGARGB's LoadRGB32 table: one colour, then the end mark */
static struct {
    UWORD count, first;
    ULONG red, green, blue;
    ULONG end;
} table = { .count = 1 };
static_assert(sizeof table == 5 * 4);

/* runtime errors of the normal code (DIFF: messages here, see above) */
static const char mess0[] = "Can't Open IFF";
static const char mess1[] = "Can't find IFF 'CMAP'";
static const char mess3n[] = "Uninitialized Screen";
static const char messd[] = "Cycles must be added in order, you silly sausage!";

#define L(p, o)  (*(ULONG *)((UBYTE *)(p) + (o)))
#define W(p, o)  (*(UWORD *)((UBYTE *)(p) + (o)))
/* the used screen (object: the struct Screen pointer) -> its ViewPort */
#define SL(p)    (&(*(struct Screen **)(p))->ViewPort)
static_assert(offsetof(struct Screen, ViewPort) == 44);

static APTR globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return (APTR)BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(APTR mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = (ULONG)mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* ------------------------------------------------- colours of palettes */

/* rep - MOVE dN,d5:LSL #8,dN:OR d5,dN:MOVE dN,d5:SWAP dN:MOVE d5,dN */
static ULONG rep(UWORD v)
{
    UWORD w = (UWORD)(v << 8) | v;

    return (ULONG)w << 16 | w;
}

/* dopalrgb - a0 = palette, d1 = colour, d2/d3/d4 = red/green/blue (8
   bit); the palette grows if needed.  Keeps d0-d5/a0-a2. */
static void palrgb(struct bb_palette *p, UWORD c, UWORD r, UWORD g, UWORD b)
{
    UBYTE *d = p->pdata;
    ULONG *e;

    if ((WORD)c >= (WORD)p->numcols) {          /* expand the palette */
        UBYTE *n = globalloc((ULONG)(UWORD)(c + 1) * 12 + 8, MEMF_PUBLIC | MEMF_CLEAR);

        if (p->pdata) {
            ULONG *s = (ULONG *)p->pdata, *dst = (ULONG *)n;
            UWORD k = (UWORD)(p->numcols * 3) + 1;

            do
                *dst++ = *s++;
            while (k-- != 0);
            globfree(p->pdata, (ULONG)p->numcols * 12 + 8);
        }
        p->pdata = d = n;
        p->numcols = c + 1;
        W(d, 0) = c + 1;
    }
    e = (ULONG *)(d + 4 + (WORD)(c * 12));
    e[0] = rep(r);
    e[1] = rep(g);
    e[2] = rep(b);
}

/**
 * @brief AGAPalRGB Palette#,Colour Register,Red,Green,Blue - set a colour
 * of a palette, 8 bit components.
 *
 * The value is repeated in every byte of the component's long; the
 * palette grows if needed (new data, the old colours copied).  Keeps
 * d0-d5/a0-a2.
 *
 * Errors (debugger only, aaprgbchk): "Not an AGA machine!", "Colour must
 * be in the range 0-31" (colour > 255), "RGB component out of range"
 * (> 255).
 *
 * @param c d1: the colour register.
 * @param r d2: red (0-255).
 * @param g d3: green.
 * @param b d4: blue.
 * @param p a0: the palette (!libs #palettelib,$1080).
 */
static void BLITZ_ROUTINE dopalrgb_c(UWORD c BL_REG(d1), UWORD r BL_REG(d2), UWORD g BL_REG(d3),
                                     UWORD b BL_REG(d4), struct bb_palette *p BL_REG(a0))
{
    palrgb(p, c, r, g, b);
}
BLITZ_THUNK(dopalrgb, dopalrgb_c, "d0-d1/a0-a1");

/**
 * @brief PalRGB Palette#,Colour Register,Red,Green,Blue - set a colour of
 * a palette, 4 bit components.
 *
 * do4bitpalrgb - 4 bit components: n<<4|n (word), then as AGAPalRGB.
 *
 * Errors (debugger only, prgbchk): "Colour must be in the range 0-31"
 * (colour > 255, or > 31 without AGA), "RGB component out of range"
 * (> 15).
 *
 * @param c d1: the colour register.
 * @param r d2: red (0-15).
 * @param g d3: green.
 * @param b d4: blue.
 * @param p a0: the palette (!libs #palettelib,$1080).
 */
static void BLITZ_ROUTINE do4bitpalrgb_c(UWORD c BL_REG(d1), UWORD r BL_REG(d2),
                                         UWORD g BL_REG(d3), UWORD b BL_REG(d4),
                                         struct bb_palette *p BL_REG(a0))
{
    palrgb(p, c, (UWORD)(r << 4) | r, (UWORD)(g << 4) | g, (UWORD)(b << 4) | b);
}
BLITZ_THUNK(do4bitpalrgb, do4bitpalrgb_c, "d0-d1/a0-a1");

/* make4bitrgbs - a0 = palette: up to 32 4 bit RGB words in rgbs4bit
   (from the top byte of each component, fixed #224, see above); d0 =
   count */
static UWORD make4(const struct bb_palette *p)
{
    UWORD n = p->numcols, k;
    const ULONG *s;
    UWORD *d = rgbs4bit;

    if (!n)
        return 0;
    if (n > 32)
        n = 32;
    s = (const ULONG *)(p->pdata + 4);
    k = n - 1;
    do {
        UWORD v = (UWORD)((s[0] >> 28) << 8);
        v |= (UWORD)((s[1] >> 24) & 0xf0);
        v |= (UWORD)(s[2] >> 28);
        *d++ = v;
        s += 3;
    } while (k-- != 0);
    return n;
}

/**
 * @brief make4bitrgbs (a nullsub for other libraries) - the 4 bit RGB
 * words of a palette, for LoadRGB4.
 *
 * Up to 32 colours, from the top byte of each component.
 *
 * Fixed (intended deviation, #224): the original took the low byte, the
 * colours of DecodePalette (top byte only) came out black.
 *
 * @param p a0: the palette.
 * @return d0.w: the count (MOVE.w: the caller's upper word stays), a1 =
 *         the table (rgbs4bit); keeps a0/a2.
 */
static UWORD BLITZ_ROUTINE make4bitrgbs_c(const struct bb_palette *p BL_REG(a0))
{
    return make4(p);
}
/* returns a1 = the table, d0 = count (MOVE.w: the caller's upper word
   stays); keeps a0/a2 */
BLITZ_THUNK_POST(make4bitrgbs, make4bitrgbs_c, "d0/a0",
                 "move.w d0,2(sp)\n   lea _rgbs4bit,a1");

/* ---------------------------------------------------------- init, finit */

/**
 * @brief Library init - agamode = ExecVersion >= 39 (environlib), no
 * cycling.
 *
 * @param callerd0 d0: the caller's d0 (passed to ExecVersion).
 * @param gfx a0: graphics.library (!libs #graphicslib,#la0).
 * @return d0: agamode, the init value (-1 or 0; EXT.w: upper word from
 *         ExecVersion's d0).
 */
static ULONG BLITZ_ROUTINE codeinit_c(ULONG callerd0 BL_REG(d0), struct Library *gfx BL_REG(a0))
{
    struct bl_regs r = { .d0 = callerd0 };
    ULONG v;

    gfxbase = gfx;
    cyclelist = NULL;
    cycon = 0;
    v = BLITZ_ALIBJSR(TOK_execversion, r);
    agamode = (WORD)v >= 39 ? -1 : 0;
    return (v & 0xffff0000UL) | (UWORD)agamode;
}
BLITZ_THUNK(codeinit, codeinit_c, "d1");

/**
 * @brief Library finit (cycoff) - cycling interrupt off, cycle list freed.
 *
 * The list is freed but not cleared (not a bug: only at program end).
 */
static void BLITZ_ROUTINE codefinit_c(void)
{
    struct cnode *n = cyclelist;

    if (cycon) {
        struct bl_regs r = { .d0 = 0x8005 };

        cycon = ~cycon;
        BLITZ_ALIBJSR(TOK_clrint, r);
    }
    while (n) {
        struct cnode *next = n->next;

        globfree(n, 12);
        n = next;
    }
}
BLITZ_THUNK(codefinit, codefinit_c, "a3");

/* ------------------------------------------------------ loading (IFF) */

/* an iffiolib call: d0/a0 in, d0 out, Z flag -> *z.  Narrower than
   BLITZ_ALIBJSR_CCR_D0A0: iffiolib's routines keep d1-d7/a1-a6 (their
   thunks save d1/a0-a1), so only d1/a1 are declared clobbered. */
#define IFFCALL(tok, in0, ina0, z) ({                                       \
    register ULONG _d0 __asm("d0") = (in0);                                 \
    register ULONG _a0 __asm("a0") = (ina0);                                \
    UBYTE _z;                                                               \
    __asm volatile("jsr _blitz_tok+%c3\n   seq %2"                          \
        : "+d"(_d0), "+a"(_a0), "=d"(_z) : "i"(BL_TOKVAL(tok, 0))           \
        : "d1", "a1", "cc", "memory");                                      \
    (z) = _z; _d0; })

/* loadthepal - a0 = file name, a1 = palette, d2 = offset */
static void loadthepal(const UBYTE *name, struct bb_palette *p, UWORD off)
{
    UBYTE z;
    ULONG cmaplen;
    WORD n;
    UWORD c;
    struct bb_cycle *cy;

    IFFCALL(TOK_iffopen, 0, (ULONG)name, z);
    if (!z) {
        BLITZ_RUNERR(mess0);                    /* err0 */
        return;
    }
    cmaplen = IFFCALL(TOK_ifffind, 0x434d4150, 0, z); /* "CMAP" */
    if (!z) {
        BLITZ_RUNERR(mess1);                    /* err1 */
        return;
    }
    n = (WORD)(UWORD)bl_divu_w((ULONG)(LONG)(WORD)cmaplen, 3) - 1;
    if (n < 0)
        goto none;
    palrgb(p, off + n, off, 0, 0);              /* grow first (dummy colour) */
    c = off;
    do {
        UWORD r, g, b;

        r = (UBYTE)IFFCALL(TOK_iffbyte, 0, 0, z);
        g = (UBYTE)IFFCALL(TOK_iffbyte, 0, 0, z);
        b = (UBYTE)IFFCALL(TOK_iffbyte, 0, 0, z);
        palrgb(p, c, r, g, b);
        c++;
    } while (n-- != 0);
    /* cycle info */
    cy = p->cyc;
    for (;;) {
        WORD rate, lo, hi;

        IFFCALL(TOK_ifffind, 0x43524e47, 0, z);     /* "CRNG" */
        if (!z)
            break;
        rate = (WORD)IFFCALL(TOK_ifflong, 0, 0, z);
        if ((WORD)IFFCALL(TOK_iffword, 0, 0, z) != 1)
            rate = -rate;
        lo = (BYTE)IFFCALL(TOK_iffbyte, 0, 0, z);
        cy->lo = lo;
        hi = (BYTE)IFFCALL(TOK_iffbyte, 0, 0, z);
        if (hi == cy->lo)
            continue;
        cy->hi = hi;
        cy->speed = rate;
        cy->var = 0;
        cy++;
    }
    cy->lo = -1;
none:
    IFFCALL(TOK_iffclose, 0, 0, z);
}

/* getpal - a0 = name, a1 = palette; getpal2 - and d0 = offset.  Keep
   d0-d5. */

/**
 * @brief getpal (a nullsub for other libraries) - load the CMAP and CRNG
 * chunks of an IFF file into a palette, as LoadPalette.
 *
 * Keeps d0-d5.
 *
 * @param name a0: the file name.
 * @param p a1: the palette.
 */
static void BLITZ_ROUTINE getpal_c(const UBYTE *name BL_REG(a0), struct bb_palette *p BL_REG(a1))
{
    loadthepal(name, p, 0);
}
BLITZ_THUNK(getpal, getpal_c, "d0-d1");

/**
 * @brief getpal2 (a nullsub for other libraries) - getpal with a palette
 * offset.
 *
 * @param off d0: the first colour register to load into.
 * @param name a0: the file name.
 * @param p a1: the palette.
 */
static void BLITZ_ROUTINE getpal2_c(UWORD off BL_REG(d0), const UBYTE *name BL_REG(a0),
                                    struct bb_palette *p BL_REG(a1))
{
    loadthepal(name, p, off);
}
BLITZ_THUNK(getpal2, getpal2_c, "d0-d1");

/**
 * @brief LoadPalette Palette#,Filename$ - load the colours (CMAP) and
 * colour cycles (CRNG) of an IFF file into a palette (Amiga mode only).
 *
 * iffiolib opens the file; the palette grows to the CMAP's colours, every
 * component an 8 bit value repeated in each byte.  The cycles end with
 * low = -1.  Not a bug (both are overwritten later): with an offset,
 * colour offset+count is first written with the offset as red value
 * (overwritten by the loop); CRNG ranges with low = high only overwrite
 * the low word of the cycle entry.
 *
 * Errors: "Can't Open IFF", "Can't find IFF 'CMAP'" (runtime errors of
 * the normal code; DIFF: their messages are in the normal code too).
 *
 * @param name d1: the file name.
 * @param p a1: the palette (!libs #palettelib,$1180).
 */
static void BLITZ_ROUTINE dopalload_c(const UBYTE *name BL_REG(d1), struct bb_palette *p BL_REG(a1))
{
    loadthepal(name, p, 0);
}
BLITZ_THUNK(dopalload, dopalload_c, "d0-d1");

/**
 * @brief LoadPalette Palette#,Filename$,Palette Offset - as LoadPalette,
 * the colours from the given colour register on (Amiga mode only).
 *
 * @param name d1: the file name.
 * @param off d2: the palette offset.
 * @param p a1: the palette.
 */
static void BLITZ_ROUTINE dopalload2_c(const UBYTE *name BL_REG(d1), UWORD off BL_REG(d2),
                                       struct bb_palette *p BL_REG(a1))
{
    loadthepal(name, p, off);
}
BLITZ_THUNK(dopalload2, dopalload2_c, "d0-d1");

/* InitPalette n,cols - a3 = palette: new (cleared) data */
static void initpalette(struct bb_palette *p, UWORD n)
{
    ULONG s;

    if (p->pdata) {
        s = (ULONG)p->numcols * 12;
        globfree(p->pdata, (s & 0xffff0000UL) | (UWORD)(s + 8));   /* ADDQ.w #8 */
    }
    s = (ULONG)n * 12;
    p->pdata = globalloc((s & 0xffff0000UL) | (UWORD)(s + 8), MEMF_PUBLIC | MEMF_CLEAR);
    p->numcols = n;
    W(p->pdata, 0) = n;
}

/**
 * @brief InitPalette Palette#,NumColors - give a palette new, cleared
 * colour data for NumColors colours.
 *
 * The old data is freed.  The palette is in a3.  Not a bug (overflows
 * only for palettes of 2731+ colours): the size with ADDQ.w #8.
 *
 * BUG(orig) (#229): no end marker is written for the cycles.
 *
 * @param n d1: the number of colours.
 */
static void BLITZ_ROUTINE _initpalette_c(UWORD n BL_REG(d1))
{
    initpalette((struct bb_palette *)blitz_string, n);
}
BLITZ_THUNK(_initpalette, _initpalette_c, "a2");

/* DecodePalette n,mem[,offset] - an IFF ILBM in memory: CMAP (top byte
   of each component) and CRNG chunks */
static void cludgepalette(UBYTE *chunk, UWORD off)
{
    struct bb_palette *p = (struct bb_palette *)blitz_string;
    UBYTE *end, *cend, *entry;
    ULONG id, len, ncols;
    struct bb_cycle *cy;
    UWORD n;

    myoff = off;
    id = L(chunk, 0);
    len = L(chunk, 4);
    chunk += 8;
    if (id != 0x464f524d) {                     /* "FORM" */
        BLITZ_RUNERR(mess0);
        return;
    }
    end = chunk + len;
    if (L(chunk, 0) != 0x494c424d) {            /* "ILBM" */
        BLITZ_RUNERR(mess0);
        return;
    }
    chunk += 4;
    for (;;) {                                  /* cv */
        id = L(chunk, 0);
        len = L(chunk, 4);
        chunk += 8;
        if (id == 0x434d4150)                   /* "CMAP" */
            break;
        chunk += len;
        if ((LONG)(end - chunk) <= 0) {
            BLITZ_RUNERR(mess1);
            return;
        }
    }
    cend = chunk + len;                         /* fc */
    ncols = bl_divu_w(len, 3);
    if (!p->pdata || (WORD)(UWORD)(ncols + myoff) > (WORD)p->numcols)
        initpalette(p, (UWORD)(ncols + myoff));
    entry = p->pdata + 4 + (WORD)(UWORD)(myoff * 12);
    n = (UWORD)ncols - 1;
    do {
        entry[0] = *chunk++;
        entry[4] = *chunk++;
        entry[8] = *chunk++;
        entry += 12;
    } while (n-- != 0);
    cy = p->cyc;
    chunk = cend;
    for (;;) {                                  /* yv */
        WORD rate, flags, lo, hi;

        id = L(chunk, 0);
        len = L(chunk, 4);
        chunk += 8;
        if (id != 0x43524e47) {                 /* "CRNG" */
            chunk += len;
            if ((LONG)(end - chunk) <= 0)
                break;
            continue;
        }
        rate = W(chunk, 2);                     /* yc */
        flags = W(chunk, 4);
        if (flags != 1) {
            rate = -rate;
            flags = 0;
        }
        lo = (BYTE)chunk[6];
        hi = (BYTE)chunk[7];
        chunk += 8;
        if (hi == lo)
            continue;
        cy->lo = lo;
        cy->hi = hi;
        cy->speed = rate;
        cy->var = flags;
        cy++;
    }
    cy->lo = -1;                                /* duncp */
}

/**
 * @brief DecodePalette Palette#,MemoryLocation,Palette Offset - take the
 * colours (CMAP) and cycles (CRNG) of an IFF ILBM in memory.
 *
 * The components go into the top byte of each long; count = DIVU
 * length/3 (the remainder stays in the upper word).  Active cycles get
 * var = the CRNG flag (1; not a bug: only the low word is used, a start
 * value of 1 is invisible).  The palette is in a3.
 *
 * BUG(orig) (#225): the chunks are walked without pad bytes.
 * BUG(orig) (#226): a palette too small for offset + count is
 * re-initialised (cleared).
 *
 * Errors: no FORM ILBM - "Can't Open IFF"; no CMAP - "Can't find IFF
 * 'CMAP'".
 *
 * @param mem d1: the IFF file in memory.
 * @param off d2: the palette offset.
 */
static void BLITZ_ROUTINE _cludgepalette_c(UBYTE *mem BL_REG(d1), UWORD off BL_REG(d2))
{
    cludgepalette(mem, off);
}

/**
 * @brief DecodePalette Palette#,MemoryLocation - as DecodePalette with
 * offset 0.
 *
 * @param mem d1: the IFF file in memory.
 */
static void BLITZ_ROUTINE _cludgepalette2_c(UBYTE *mem BL_REG(d1))
{
    cludgepalette(mem, 0);
}
BLITZ_THUNK(_cludgepalette, _cludgepalette_c, "a2");
BLITZ_THUNK(_cludgepalette2, _cludgepalette2_c, "a2");

/**
 * @brief _free of the Palette object (Free Palette, program end) - free
 * the colour data, no cycles.
 *
 * The palette is in a3; nothing happens for 0 colours.
 */
static void BLITZ_ROUTINE freepal_c(void)
{
    struct bb_palette *p = (struct bb_palette *)blitz_string;
    UWORD n = p->numcols;
    UBYTE *d;

    if (!n)
        return;
    p->numcols = 0;
    d = p->pdata;
    p->pdata = NULL;
    p->cyc[0].lo = -1;
    globfree(d, (ULONG)n * 12 + 8);
}
BLITZ_THUNK(freepal, freepal_c, "a2");

/**
 * @brief SetCycle Palette#,Cycle,Low Colour,High Colour,Speed - set a
 * colour cycle of a palette.
 *
 * Low and High are swapped if needed; speed = the Quick >> 2 (16384 = one
 * step per frame, the sign is the direction).  A cycle behind the last
 * one is appended (with a new end mark).
 *
 * Errors: a cycle more than one behind the last - "Cycles must be added
 * in order, you silly sausage!" (a runtime error of the normal code).
 * Debugger only (setcyclechk2): "Cycle speed MUST be >=-1 AND <=1",
 * "Cycle MUST be >=0 AND <=6", "Cycle colours out of range" (>= 32).
 *
 * @param cyc d1: the cycle number.
 * @param lo d2: the low colour.
 * @param hi d3: the high colour.
 * @param speed d4: the speed (Quick).
 * @param p a0: the palette (!libs #palettelib,$1080).
 */
static void BLITZ_ROUTINE dosetcycle2_c(UWORD cyc BL_REG(d1), UWORD lo BL_REG(d2),
                                        UWORD hi BL_REG(d3), ULONG speed BL_REG(d4),
                                        struct bb_palette *p BL_REG(a0))
{
    struct bb_cycle *e = p->cyc, *c;
    WORD app = 0;

    if (p->numcols)                             /* find the last cycle */
        while (e->lo >= 0)
            e++;
    c = (struct bb_cycle *)((UBYTE *)p + 6 + (WORD)(UWORD)(cyc << 3));
    if (c >= e) {
        if (c != e) {
            BLITZ_RUNERR(messd);                /* errd */
            return;
        }
        app = -1;
    }
    if (hi < lo) {
        UWORD old_lo = lo;
        lo = hi;
        hi = old_lo;
    }
    c->lo = lo;
    c->hi = hi;
    c->speed = (WORD)((LONG)speed >> 2);
    c->var = 0;
    if (app < 0)
        c[1].lo = app;
}
BLITZ_THUNK(dosetcycle2, dosetcycle2_c, "d0-d1/a1");

/**
 * @brief SetCycle Palette#,Cycle,Low Colour,High Colour - as SetCycle
 * with speed 1 (one step per frame).
 *
 * Errors as SetCycle (debugger only: setcyclechk, without the speed).
 *
 * @param cyc d1: the cycle number.
 * @param lo d2: the low colour.
 * @param hi d3: the high colour.
 * @param p a0: the palette.
 */
static void BLITZ_ROUTINE dosetcycle1_c(UWORD cyc BL_REG(d1), UWORD lo BL_REG(d2),
                                        UWORD hi BL_REG(d3), struct bb_palette *p BL_REG(a0))
{
    dosetcycle2_c(cyc, lo, hi, 0x10000, p);
}
BLITZ_THUNK(dosetcycle1, dosetcycle1_c, "d0-d1/a1");

/**
 * @brief NewPaletteMode On|Off - whether Use Palette shows the palette
 * (Off, the default) or only makes it the used one (On).
 *
 * @param m d0: On (<> 0) or Off (0).
 */
static void BLITZ_ROUTINE newpalmode_c(UWORD m BL_REG(d0))
{
    palmode = m;
}
BLITZ_THUNK(newpalmode, newpalmode_c, "d0-d1/a0-a1");

/* ------------------------------------------- showing, cycling */

/* showpal (Amiga) - a3 = palette, a2 = used screen, a6 = graphics */
static void showpal_amiga(struct bb_palette *p, APTR *scr, struct Library *gfx)
{
    if (agamode) {
        if (!p->numcols)
            return;                             /* DIFF, see above */
        LoadRGB32(SL(scr), (ULONG *)p->pdata);
    } else {
        UWORD n = make4(p);

        if (!n)
            return;                             /* DIFF, see above */
        LoadRGB4(SL(scr), rgbs4bit, n);
    }
}

/* b_showpal (Blitz) - a0 = used slice: its colour moves */
static void showpal_blitz(struct bb_palette *p, struct bb_slice *slice)
{
    UWORD *poke = (UWORD *)COLMOVES(slice);
    UWORD n = slice->colours;
    const ULONG *s;

    if (n > p->numcols)
        n = p->numcols;
    if ((WORD)--n < 0)
        return;
    poke++;
    s = (const ULONG *)(p->pdata + 4);
    do {
        UWORD v = (UWORD)((s[0] >> 24) >> 4) << 8;
        v |= (s[1] >> 24) & 0xf0;
        v |= (s[2] >> 28);
        *poke = v;
        poke += 2;
        s += 3;
    } while (n-- != 0);
}

void BLITZ_ROUTINE showpal_a(APTR *scr BL_REG(a2), struct Library *gfx BL_REG(a6));
/**
 * @brief ShowPalette Palette#, Amiga mode - load the palette into the used
 * screen's ViewPort.
 *
 * With agamode LoadRGB32 of the palette data, else LoadRGB4 of up to 32
 * colours (make4bitrgbs; with the original's low byte DecodePalette colours
 * were black, fixed: #224).
 * The palette is in a3.
 *
 * BUG(orig) (#230): with an empty palette the original runs into
 * b_showpal and writes nothing (count 0); here it simply returns (DIFF).
 *
 * Errors (debugger only, showpalchk): "Uninitialized Screen".
 *
 * @param scr a2: the used screen (object).
 * @param gfx a6: graphics.library.
 */
void BLITZ_ROUTINE showpal_a(APTR *scr BL_REG(a2), struct Library *gfx BL_REG(a6))
{
    showpal_amiga((struct bb_palette *)blitz_string, scr, gfx);
}

void BLITZ_ROUTINE showpal_b(struct bb_slice *slice BL_REG(a0));
/**
 * @brief ShowPalette Palette#, Blitz mode - write the palette into the
 * colour moves of the used slice.
 *
 * As many colours as the slice has (and the palette), 4 bit each from
 * the top byte of the components.  The palette is in a3.
 *
 * Errors (debugger only, showpalchk): "Uninitialized Slice", "Not enough
 * colours in Slice" (a slice without colours).
 *
 * @param slice a0: the used slice (!libs #blitzcoplib,$9002).
 */
void BLITZ_ROUTINE showpal_b(struct bb_slice *slice BL_REG(a0))
{
    showpal_blitz((struct bb_palette *)blitz_string, slice);
}
BLITZ_AB_ROUTINE(showpal, showpal_b, "   jmp     _showpal_a");

/* usepal - ShowPalette unless NewPaletteMode On */
void BLITZ_ROUTINE usepal_a(APTR *scr BL_REG(a2), struct Library *gfx BL_REG(a6));
/**
 * @brief _use of the Palette object (Use Palette), Amiga mode -
 * ShowPalette unless NewPaletteMode On.
 *
 * Errors (debugger only, usepalchk): as ShowPalette, also only without
 * NewPaletteMode.
 *
 * @param scr a2: the used screen (object).
 * @param gfx a6: graphics.library.
 */
void BLITZ_ROUTINE usepal_a(APTR *scr BL_REG(a2), struct Library *gfx BL_REG(a6))
{
    if (!palmode)
        showpal_a(scr, gfx);
}

void BLITZ_ROUTINE usepal_b(struct bb_slice *slice BL_REG(a0));
/**
 * @brief _use of the Palette object (Use Palette), Blitz mode -
 * ShowPalette unless NewPaletteMode On.
 *
 * @param slice a0: the used slice.
 */
void BLITZ_ROUTINE usepal_b(struct bb_slice *slice BL_REG(a0))
{
    if (!palmode)
        showpal_b(slice);
}
BLITZ_AB_ROUTINE(usepal, usepal_b, "   jmp     _usepal_a");

/* cycle - one palette's cycles one step; returns <>0 if colours moved */
static int cyclepal(struct bb_palette *p)
{
    UBYTE *field = (UBYTE *)p + 4;
    int moved = 0;

    for (;;) {
        WORD lo, hi, sp;
        UWORD v;
        int up = 0;
        UBYTE *c;
        UWORD lo_off, hi_off;
        ULONG save_r, save_g, save_b;

        field += 2;
        lo = W(field, 0);
        field += 2;
        if (lo < 0)
            return moved;
        hi = W(field, 0);
        field += 2;
        sp = W(field, 0);
        field += 2;
        if (sp < 0) {
            sp = -sp;
            up = 1;
        }
        v = sp + W(field, 0);
        W(field, 0) = v;
        if (v < 16384)
            continue;
        v -= 16384;
        W(field, 0) = v;
        moved = 1;
        lo_off = (UWORD)(lo * 12);
        hi_off = (UWORD)(hi * 12);
        c = p->pdata + 4;
        if (up) {
            save_r = L(c, (WORD)lo_off);
            save_g = L(c, (WORD)lo_off + 4);
            save_b = L(c, (WORD)lo_off + 8);
            while (lo_off < hi_off) {
                L(c, (WORD)lo_off) = L(c, (WORD)lo_off + 12);
                L(c, (WORD)lo_off + 4) = L(c, (WORD)lo_off + 16);
                L(c, (WORD)lo_off + 8) = L(c, (WORD)lo_off + 20);
                lo_off += 12;
            }
            L(c, (WORD)lo_off + 8) = save_b;
            L(c, (WORD)lo_off + 4) = save_g;
            L(c, (WORD)lo_off) = save_r;
        } else {
            save_r = L(c, (WORD)hi_off);
            save_g = L(c, (WORD)hi_off + 4);
            save_b = L(c, (WORD)hi_off + 8);
            while (hi_off > lo_off) {
                hi_off -= 12;
                L(c, (WORD)hi_off + 12) = L(c, (WORD)hi_off);
                L(c, (WORD)hi_off + 16) = L(c, (WORD)hi_off + 4);
                L(c, (WORD)hi_off + 20) = L(c, (WORD)hi_off + 8);
            }
            L(c, (WORD)hi_off + 8) = save_b;
            L(c, (WORD)hi_off + 4) = save_g;
            L(c, (WORD)hi_off) = save_r;
        }
    }
}

/**
 * @brief CyclePalette Palette# - move the colours of every cycle of the
 * palette that is due by one step (in the palette only, nothing shown).
 *
 * Each cycle adds its speed to its var; at 16384 the colours low..high
 * rotate by one (the sign of the speed is the direction).
 *
 * @param p a2: the palette (!libs #palettelib,$1280).
 */
static void BLITZ_ROUTINE _cyclepalette_c(struct bb_palette *p BL_REG(a2))
{
    if (p->numcols)
        cyclepal(p);
}
BLITZ_THUNK(_cyclepalette, _cyclepalette_c, "d0-d1/a0-a1");

/* cycleem - the vertical blank routine: every palette of the list, the
   ViewPort reloaded when colours moved */
static ULONG BLITZ_ROUTINE cycleem_c(void)
{
    struct Library *gfx = gfxbase;
    struct cnode *n;

    for (n = cyclelist; n; n = n->next) {
        if (!cyclepal(n->pal))
            continue;
        if (agamode)
            LoadRGB32(n->vp, (ULONG *)n->pal->pdata);
        else {
            UWORD k = make4(n->pal);

            LoadRGB4(n->vp, rgbs4bit, k);
        }
    }
    return 0;
}
BLITZ_THUNK_Z(cycleem, cycleem_c, "d1/a0-a1");

/**
 * @brief Cycle Palette# - cycle the palette's colours on the used screen
 * (Amiga mode only).
 *
 * The palette and the screen's ViewPort go into the cycle list (12 bytes,
 * globalloc); the first Cycle installs cycleem in the vertical blank
 * (intlib SetInt, level $8005), which cycles every palette of the list
 * and reloads the ViewPort when colours moved.  The palette is in a3.
 *
 * Errors (debugger only, docyclechk): "Uninitialized cycle information"
 * (a palette without colours).
 *
 * @param scr a2: the used screen (object; !libs #screenslib,$1202).
 */
static void BLITZ_ROUTINE docycle_c(APTR *scr BL_REG(a2))
{
    struct cnode *n = globalloc(12, MEMF_PUBLIC);

    n->pal = (struct bb_palette *)blitz_string;
    n->vp = SL(scr);
    n->next = cyclelist;
    cyclelist = n;
    if (!cycon) {
        struct bl_regs r = { .d0 = 0x8005, .d1 = (ULONG)cycleem };

        cycon = ~cycon;
        BLITZ_ALIBJSR(TOK_setint, r);
    }
}
BLITZ_THUNK(docycle, docycle_c, "a0");

/**
 * @brief StopCycle - stop the colour cycling.
 *
 * ClrInt only (cycon stays).
 *
 * BUG(orig) (#228): StopCycle does not reset `cycon`: a later Cycle does
 * not install the interrupt again.
 */
static void BLITZ_ROUTINE dostopcycle_c(void)
{
    struct bl_regs r = { .d0 = 0x8005 };

    BLITZ_ALIBJSR(TOK_clrint, r);
}
BLITZ_THUNK(dostopcycle, dostopcycle_c, "a0");

/* ------------------------------------------------- reading colours */

/* Red/Green/Blue (Amiga) - GetRGB4 of the used screen's ColorMap; the
   upper word of d0 is GetRGB4's */
static ULONG getcol_a(ULONG c, APTR *scr, struct Library *gfx, int shift)
{
    ULONG v = GetRGB4(SL(scr)->ColorMap, c);

    return (v & 0xffff0000UL) | (((UWORD)v >> shift) & 15);
}

/* Blitz: the colour move of the used slice; upper word of d0 stays */
static ULONG getcol_b(ULONG c, struct bb_slice *slice, int shift)
{
    UWORD v = *(UWORD *)(COLMOVES(slice) + 2 + (WORD)(UWORD)(c << 2));

    return (c & 0xffff0000UL) | ((v >> shift) & 15);
}

#define COLFN(name, shift)                                                  \
    ULONG BLITZ_ROUTINE name##_a(ULONG c BL_REG(d0), APTR *scr BL_REG(a0),  \
                                 struct Library *gfx BL_REG(a6));           \
    ULONG BLITZ_ROUTINE name##_a(ULONG c BL_REG(d0), APTR *scr BL_REG(a0),  \
                                 struct Library *gfx BL_REG(a6))            \
    {                                                                       \
        return getcol_a(c, scr, gfx, shift);                                \
    }                                                                       \
    ULONG BLITZ_ROUTINE name##_b(ULONG c BL_REG(d0), struct bb_slice *slice BL_REG(a0)); \
    ULONG BLITZ_ROUTINE name##_b(ULONG c BL_REG(d0), struct bb_slice *slice BL_REG(a0)) \
    {                                                                       \
        return getcol_b(c, slice, shift);                                   \
    }                                                                       \
    BLITZ_AB_ROUTINE(name, name##_b, "   jmp     _" #name "_a")

/**
 * @brief Red(Colour Register), Green(...), Blue(...) - the 4 bit component
 * of a colour of the used screen (Amiga mode) or slice (Blitz mode).
 *
 * dored_a/_b, dogreen_a/_b, doblue_a/_b: Amiga - GetRGB4 of the used
 * screen's ColorMap (the upper word of d0 is GetRGB4's); Blitz - the
 * colour move of the used slice (the upper word of d0 stays).  The binary
 * takes the colour as a #long.
 *
 * Errors (debugger only, colchk): "Uninitialized Screen", "Uninitialized
 * Slice".
 *
 * @arg c d0: the colour register.
 * @arg scr a0: Amiga mode: the used screen (object).
 * @arg gfx a6: Amiga mode: graphics.library.
 * @arg slice a0: Blitz mode: the used slice.
 * @par Result
 * d0.w: the component (0-15).
 */
COLFN(dored, 8);
COLFN(dogreen, 4);
COLFN(doblue, 0);

/* AGARed/AGAGreen/AGABlue - GetRGB32 of one colour (d0 & 255); the top
   byte of the component (ROL.l #8:AND.w #255) */
static ULONG agacol(ULONG c, APTR *scr, struct Library *gfx, int i)
{
    ULONG v;

    if (!*scr) {
        BLITZ_RUNERR(mess3n);                   /* err3 */
        return c;
    }
    GetRGB32(SL(scr)->ColorMap, c & 255, 1, agatable);
    v = agatable[i];
    v = v << 8 | v >> 24;
    return (v & 0xffff0000UL) | (v & 0xff);
}

/**
 * @brief AGARed(Colour Register) - the 8 bit red component of a colour of
 * the used screen.
 *
 * GetRGB32 of one colour (d0 & 255); the top byte of the component
 * (ROL.l #8:AND.w #255).  The binary checks the screen and masks the
 * colour number.
 *
 * Errors: "Uninitialized Screen" (err3; DIFF: the original jumps into the
 * error check area for it, here it is in the normal code).  Debugger only
 * (agacolchk): "Not an AGA machine!".
 *
 * @param c d0: the colour register.
 * @param scr a0: the used screen (object; !libs #screenslib,$1002).
 * @param gfx a6: graphics.library.
 * @return d0.w: the component (0-255); the upper word is ROL's.
 */
static ULONG BLITZ_ROUTINE doagared_c(ULONG c BL_REG(d0), APTR *scr BL_REG(a0),
                                      struct Library *gfx BL_REG(a6))
{
    return agacol(c, scr, gfx, 0);
}

/**
 * @brief AGAGreen(Colour Register) - the 8 bit green component, as
 * AGARed.
 *
 * @param c d0: the colour register.
 * @param scr a0: the used screen (object).
 * @param gfx a6: graphics.library.
 * @return d0.w: the component (0-255).
 */
static ULONG BLITZ_ROUTINE doagagreen_c(ULONG c BL_REG(d0), APTR *scr BL_REG(a0),
                                        struct Library *gfx BL_REG(a6))
{
    return agacol(c, scr, gfx, 1);
}

/**
 * @brief AGABlue(Colour Register) - the 8 bit blue component, as AGARed.
 *
 * @param c d0: the colour register.
 * @param scr a0: the used screen (object).
 * @param gfx a6: graphics.library.
 * @return d0.w: the component (0-255).
 */
static ULONG BLITZ_ROUTINE doagablue_c(ULONG c BL_REG(d0), APTR *scr BL_REG(a0),
                                       struct Library *gfx BL_REG(a6))
{
    return agacol(c, scr, gfx, 2);
}
BLITZ_THUNK(doagared, doagared_c, "a2");
BLITZ_THUNK(doagagreen, doagagreen_c, "a2");
BLITZ_THUNK(doagablue, doagablue_c, "a2");

/* ------------------------------------------------- setting colours */

/* RGB c,r,g,b - Amiga: SetRGB4 of the used screen; Blitz: the colour
   move r<<8|g<<4|b of the used slice.  The Amiga variant is the
   original's three instructions: SetRGB4 gets the caller's a1 unchanged
   (with a1 = the ViewPort, as a C call left it, the emulator's
   graphics.library stored green $8C instead of $88 for 8 - t122) */
void BLITZ_ROUTINE dorgb_b(UWORD c BL_REG(d0), UWORD r BL_REG(d1), UWORD g BL_REG(d2),
                           UWORD b BL_REG(d3), struct bb_slice *slice BL_REG(a0));
/**
 * @brief RGB Colour Register,Red,Green,Blue - set a colour of the used
 * screen (Amiga mode) or slice (Blitz mode), 4 bit components.
 *
 * Blitz mode (dorgb_b): the colour move r<<8|g<<4|b of the used slice.
 * Amiga mode: SetRGB4 of the used screen's ViewPort, in assembler (see
 * above; d0-d3 and a6 as SetRGB4 takes them).
 *
 * Errors (debugger only, rgbchk): Amiga - "Uninitialized Screen", "Colour
 * must be in the range 0-31" (> 255, or > 31 without AGA); Blitz -
 * "Uninitialized Slice", "Colour must be in the range 0-31", "Not enough
 * colours in Slice"; both "RGB component out of range" (> 15).
 *
 * @param c d0: the colour register.
 * @param r d1: red (0-15).
 * @param g d2: green.
 * @param b d3: blue.
 * @param slice a0: Blitz mode: the used slice; Amiga mode: the used
 *        screen (object), a6 = graphics.library.
 */
void BLITZ_ROUTINE dorgb_b(UWORD c BL_REG(d0), UWORD r BL_REG(d1), UWORD g BL_REG(d2),
                           UWORD b BL_REG(d3), struct bb_slice *slice BL_REG(a0))
{
    *(UWORD *)(COLMOVES(slice) + 2 + (WORD)(UWORD)(c << 2)) =
        (UWORD)(r << 8) | (UWORD)(g << 4) | b;
}
BLITZ_AB_ROUTINE(dorgb, dorgb_b,
"   move.l  (a0),a0\n"
"   lea     44(a0),a0\n"
"   jmp     -288(a6)");                     /* SetRGB4 */

/* AGARGB c,r,g,b - LoadRGB32 with one colour, 8 bit components (word
   r<<8|r.b in both halves) */
static ULONG rep8(UWORD v)
{
    UWORD w = (UWORD)(v << 8) | (UBYTE)v;

    return (ULONG)w << 16 | w;
}

/**
 * @brief AGARGB Colour Register,Red,Green,Blue - set a colour of the used
 * screen, 8 bit components (Amiga mode only).
 *
 * LoadRGB32 with one colour; each component's word r<<8|r.b in both
 * halves.
 *
 * Errors (debugger only, aargbchk): "Not an AGA machine!", "Uninitialized
 * Screen", "Colour must be in the range 0-31" (> 255), "RGB component out
 * of range" (> 255).
 *
 * @param c d0: the colour register.
 * @param r d1: red (0-255).
 * @param g d2: green.
 * @param b d3: blue.
 * @param scr a0: the used screen (object; !libs #screenslib,$1002).
 * @param gfx a6: graphics.library.
 */
static void BLITZ_ROUTINE doaargb_c(UBYTE c BL_REG(d0), UWORD r BL_REG(d1), UWORD g BL_REG(d2),
                                    UWORD b BL_REG(d3), APTR *scr BL_REG(a0),
                                    struct Library *gfx BL_REG(a6))
{
    ((UBYTE *)&table.first)[1] = c;             /* MOVE.b: the low byte */
    table.red = rep8(r);
    table.green = rep8(g);
    table.blue = rep8(b);
    LoadRGB32(SL(scr), (ULONG *)&table);
}
BLITZ_THUNK(doaargb, doaargb_c, "a2");

/* -------------------------------------------- palette calculations */

/**
 * @brief FadePalette SrcPalette#,DestPalette#,Brightness.q - the source
 * colours times the brightness into the destination.
 *
 * Every component: MULU of its upper word (not a bug: the source is read
 * 2 bytes early, see the top) with brightness >> 8, limited to $FF0000,
 * << 8.  Nothing happens for an empty source or a destination with fewer
 * colours.
 *
 * @param br d2: the brightness (Quick, 1.0 = full brightness).
 * @param s a0: the source palette.
 * @param d a1: the destination palette.
 */
static void BLITZ_ROUTINE _fadepalette_c(ULONG br BL_REG(d2), struct bb_palette *s BL_REG(a0),
                                         struct bb_palette *d BL_REG(a1))
{
    UWORD n = s->numcols;
    const ULONG *src;
    ULONG *dst;
    UWORD k;

    if (!n || (WORD)n > (WORD)d->numcols)
        return;
    br >>= 8;
    src = (const ULONG *)(s->pdata + 2);        /* +2: not a bug, see top */
    dst = (ULONG *)(d->pdata + 4);
    k = n - 1;
    do {
        int i;

        for (i = 0; i < 3; i++) {
            ULONG v = (ULONG)(UWORD)src[i] * (UWORD)br;

            if (v >= 0xff0000)
                v = 0xff0000;
            dst[i] = v << 8;
        }
        src += 3;
        dst += 3;
    } while (k-- != 0);
}
BLITZ_THUNK(_fadepalette, _fadepalette_c, "d0-d1/a0-a1");

/* PaletteRange n,start,end,r0,g0,b0,r1,g1,b1 - d3-d5 = r0,g0,b0, the
   end colour on the argument stack (-2 r1, -4 g1, -6 b1) */
static ULONG step(WORD delta, UWORD n)
{
    ULONG q = bl_divs_w((ULONG)((LONG)delta << 8), n);

    q = (ULONG)(LONG)(WORD)q;                   /* EXT.l */
    return q << 16 | q >> 16;                   /* SWAP */
}

static ULONG startval(WORD v)
{
    ULONG s = (ULONG)(LONG)v;

    return (s << 16 | s >> 16) << 8;            /* EXT.l:SWAP:ASL.l #8 */
}

/**
 * @brief PaletteRange Palette#,StartCol,EndCol,r0,g0,b0,r1,g1,b1 - a
 * colour gradient from r0,g0,b0 at StartCol to r1,g1,b1 at EndCol.
 *
 * The step of each component is DIVS (delta << 8)/(EndCol-StartCol).
 * Nothing happens for EndCol < StartCol or a range larger than the
 * palette.  The palette is in a3.
 *
 * BUG(orig) (#227): StartCol = EndCol divides by zero (DIVS).
 *
 * @param start d1: StartCol.
 * @param stop d2: EndCol.
 * @param r0 d3: the start red.
 * @param g0 d4: the start green.
 * @param b0 d5: the start blue.
 * @param args a2: the argument stack: r1, g1, b1 at -2, -4, -6.
 */
static void BLITZ_ROUTINE _paletterange_c(UWORD start BL_REG(d1), UWORD stop BL_REG(d2),
                                          WORD r0 BL_REG(d3), WORD g0 BL_REG(d4),
                                          WORD b0 BL_REG(d5), const WORD *args BL_REG(a2))
{
    struct bb_palette *p = (struct bb_palette *)blitz_string;
    WORD n = stop - start;
    ULONG *e, dr, dg, db, r, g, b;

    if (n < 0)
        return;
    n++;
    if (n > (WORD)p->numcols)
        return;
    n--;
    e = (ULONG *)(p->pdata + 4 + (WORD)(UWORD)(start * 12));
    db = step(args[-3] - b0, n);
    dg = step(args[-2] - g0, n);
    dr = step(args[-1] - r0, n);
    r = startval(r0);
    g = startval(g0);
    b = startval(b0);
    do {
        e[0] = r;
        e[1] = g;
        e[2] = b;
        e += 3;
        r += dr;
        g += dg;
        b += db;
    } while (n-- != 0);
}
BLITZ_THUNK(_paletterange, _paletterange_c, "d0-d1/a0-a1");

/**
 * @brief DuplicatePalette SrcPalette#,DestPalette# - copy the colours (not
 * the cycles) of a palette.
 *
 * The destination (a3) is initialised to the source's number of colours
 * first (InitPalette); nothing happens for an empty source.
 *
 * @param s a2: the source palette (!libs #palettelib,$1280).
 */
static void BLITZ_ROUTINE _duplicatepalette_c(struct bb_palette *s BL_REG(a2))
{
    struct bb_palette *d = (struct bb_palette *)blitz_string;
    const ULONG *a;
    ULONG *b;
    UWORD k;

    if (!s->numcols)
        return;
    initpalette(d, s->numcols);
    k = (UWORD)(s->numcols * 3) - 1;
    a = (const ULONG *)(s->pdata + 4);
    b = (ULONG *)(d->pdata + 4);
    do
        *b++ = *a++;
    while (k-- != 0);
}
BLITZ_THUNK(_duplicatepalette, _duplicatepalette_c, "a2");

/**
 * @brief MixPalette Palette#,Palette#,DestPalette#,fraction - mix two
 * palettes into the destination.
 *
 * Every component = first * fraction + second * (1 - fraction), with the
 * upper words of the sources (+2, not a bug, see top); as many colours as
 * the first palette has, the destination (a3) is not checked.
 *
 * @param f d3: the fraction (Quick, 0..1).
 * @param p1 a0: the first palette.
 * @param p2 a2: the second palette.
 */
static void BLITZ_ROUTINE _mixpalette_c(LONG f BL_REG(d3), struct bb_palette *p1 BL_REG(a0),
                                        struct bb_palette *p2 BL_REG(a2))
{
    struct bb_palette *d = (struct bb_palette *)blitz_string;
    UWORD k = (UWORD)(p1->numcols * 3) - 1;
    const ULONG *a = (const ULONG *)(p1->pdata + 2);
    const ULONG *b = (const ULONG *)(p2->pdata + 2);
    ULONG *c = (ULONG *)(d->pdata + 4);
    UWORD f1, f2;

    f >>= 1;
    f1 = (UWORD)f;
    f2 = 32768 - f1;
    do {
        ULONG v = (ULONG)(UWORD)*a++ * f1 + (ULONG)(UWORD)*b++ * f2;

        *c++ = v + v;
    } while (k-- != 0);
}
BLITZ_THUNK(_mixpalette, _mixpalette_c, "d0-d1/a0-a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess3, "Uninitialized Screen");
BLITZ_ERRCHK_MESSAGE(mess4, "Uninitialized Palette");
BLITZ_ERRCHK_MESSAGE(mess5, "RGB component out of range");
BLITZ_ERRCHK_MESSAGE(mess6, "Uninitialized Slice");
BLITZ_ERRCHK_MESSAGE(mess7, "Not enough colours in Slice");
BLITZ_ERRCHK_MESSAGE(mess8, "Colour must be in the range 0-31");
BLITZ_ERRCHK_MESSAGE(mess9, "Cycle colours out of range");
BLITZ_ERRCHK_MESSAGE(messa, "Cycle speed MUST be >=-1 AND <=1");
BLITZ_ERRCHK_MESSAGE(messb, "Cycle MUST be >=0 AND <=6");
BLITZ_ERRCHK_MESSAGE(messc, "Uninitialized cycle information");
BLITZ_ERRCHK_MESSAGE(messe, "Not an AGA machine!");

/* rgbchk2 - 4 bit components */
static int BLITZ_ERRCHK rgbchk2(UWORD r, UWORD g, UWORD b)
{
    if (r > 15 || g > 15 || b > 15) {
        BLITZ_RUNERR(mess5);
        return 0;
    }
    return 1;
}

static void BLITZ_ERRCHK rgbchk_a_c(UWORD c BL_REG(d0), UWORD r BL_REG(d1), UWORD g BL_REG(d2),
                                    UWORD b BL_REG(d3), const ULONG *scr BL_REG(a0))
{
    if (!*scr)
        BLITZ_RUNERR(mess3);
    else if (c > 255 || (!agamode && c > 31))
        BLITZ_RUNERR(mess8);
    else
        rgbchk2(r, g, b);
}

static void BLITZ_ERRCHK rgbchk_b_c(UWORD c BL_REG(d0), UWORD r BL_REG(d1), UWORD g BL_REG(d2),
                                    UWORD b BL_REG(d3), const struct bb_slice *slice BL_REG(a0))
{
    if (!slice->y)
        BLITZ_RUNERR(mess6);
    else if (c > 31)
        BLITZ_RUNERR(mess8);
    else if (c >= slice->colours)
        BLITZ_RUNERR(mess7);
    else
        rgbchk2(r, g, b);
}

static void BLITZ_ERRCHK aargbchk_c(UWORD c BL_REG(d0), UWORD r BL_REG(d1), UWORD g BL_REG(d2),
                                    UWORD b BL_REG(d3), const ULONG *scr BL_REG(a0))
{
    if (!agamode)
        BLITZ_RUNERR(messe);
    else if (!*scr)
        BLITZ_RUNERR(mess3);
    else if (c > 255)
        BLITZ_RUNERR(mess8);
    else if (r > 255 || g > 255 || b > 255)
        BLITZ_RUNERR(mess5);
}

/* showpalchk / usepalchk: Amiga - the screen (a2); Blitz - the slice
   (a0) with colours; usepalchk only without NewPaletteMode */
static void BLITZ_ERRCHK showpalchk_a_c(const ULONG *scr BL_REG(a2))
{
    if (!*scr)
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK showpalchk_b_c(const struct bb_slice *slice BL_REG(a0))
{
    if (!slice->y)
        BLITZ_RUNERR(mess6);
    else if (!slice->colours)
        BLITZ_RUNERR(mess7);
}

static void BLITZ_ERRCHK usepalchk_a_c(const ULONG *scr BL_REG(a2))
{
    if (!palmode)
        showpalchk_a_c(scr);
}

static void BLITZ_ERRCHK usepalchk_b_c(const struct bb_slice *slice BL_REG(a0))
{
    if (!palmode)
        showpalchk_b_c(slice);
}

static void BLITZ_ERRCHK docyclechk_c(void)
{
    if (!((struct bb_palette *)blitz_string)->numcols)
        BLITZ_RUNERR(messc);
}

static void BLITZ_ERRCHK colchk_a_c(const ULONG *scr BL_REG(a0))
{
    if (!*scr)
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK colchk_b_c(const struct bb_slice *slice BL_REG(a0))
{
    if (!slice->y)
        BLITZ_RUNERR(mess6);
}

static void BLITZ_ERRCHK agacolchk_c(const ULONG *scr BL_REG(a0))
{
    if (!agamode)
        BLITZ_RUNERR(messe);
    else if (!*scr)
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK prgbchk_c(UWORD c BL_REG(d1), UWORD r BL_REG(d2), UWORD g BL_REG(d3),
                                   UWORD b BL_REG(d4))
{
    if (c > 255 || (!agamode && c > 31))
        BLITZ_RUNERR(mess8);
    else
        rgbchk2(r, g, b);
}

static void BLITZ_ERRCHK aaprgbchk_c(UWORD c BL_REG(d1), UWORD r BL_REG(d2), UWORD g BL_REG(d3),
                                     UWORD b BL_REG(d4))
{
    if (!agamode)
        BLITZ_RUNERR(messe);
    else if (c > 255)
        BLITZ_RUNERR(mess8);
    else if (r > 255 || g > 255 || b > 255)
        BLITZ_RUNERR(mess5);
}

static void BLITZ_ERRCHK setcyclechk_c(UWORD c BL_REG(d1), UWORD lo BL_REG(d2), UWORD hi BL_REG(d3))
{
    if (c >= 7)
        BLITZ_RUNERR(messb);
    else if (lo >= 32 || hi >= 32)
        BLITZ_RUNERR(mess9);
}

static void BLITZ_ERRCHK setcyclechk2_c(UWORD c BL_REG(d1), UWORD lo BL_REG(d2),
                                        UWORD hi BL_REG(d3), LONG sp BL_REG(d4))
{
    if (sp < -0x10000 || sp > 0x10000)
        BLITZ_RUNERR(messa);
    else
        setcyclechk_c(c, lo, hi);
}

#define ERRTHUNK(n) BLITZ_ERRCHK_THUNK(n, n##_c, "d0-d1/a0-a1")
ERRTHUNK(rgbchk_a);
ERRTHUNK(rgbchk_b);
ERRTHUNK(aargbchk);
ERRTHUNK(showpalchk_a);
ERRTHUNK(showpalchk_b);
ERRTHUNK(usepalchk_a);
ERRTHUNK(usepalchk_b);
ERRTHUNK(docyclechk);
ERRTHUNK(colchk_a);
ERRTHUNK(colchk_b);
ERRTHUNK(agacolchk);
ERRTHUNK(prgbchk);
ERRTHUNK(aaprgbchk);
ERRTHUNK(setcyclechk);
ERRTHUNK(setcyclechk2);
BLITZ_ERRCHK_AB(rgbchk, rgbchk_b, rgbchk_a);
BLITZ_ERRCHK_AB(usepalchk, usepalchk_b, usepalchk_a);
BLITZ_ERRCHK_AB(showpalchk, showpalchk_b, showpalchk_a);
BLITZ_ERRCHK_AB(colchk, colchk_b, colchk_a);
