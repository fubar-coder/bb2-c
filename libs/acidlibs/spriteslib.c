/*
 * spriteslib - command library 112: GetaSprite ShowSprite InFront
 * InFrontF InFrontB SaveSprites LoadSprites SpriteMode; object library
 * "Sprite" (16 bytes, maximum 20).  init value (long): the address of
 * the sprite mode word (followed by the sprite width in bytes and the
 * width shift).
 *
 * C translation of bblibsrc/spriteslib; binary of the CD's acidlibs
 * checked: same as the source.
 *
 * A sprite (16 bytes): +0 data (CHIP, 0 = none), +4 height + 1 (for the
 * final zeros), +6 number of hardware sprites used, +8 flags (bit 15 =
 * 16 colours: attached pairs; low byte = pixel width), +10 bytes per
 * image, +12/+14 handle.  SpriteMode 0/1/2 = 16/32/64 pixels per sprite
 * (FMODE): widthshift 2/3/4, spritewid 2/4/8 bytes.  GetaSprite takes a
 * 4 or 16 colour shape; per sprite and row the two planes' words/longs
 * (16 colours: planes 2+3 in the attached sprite), then a 0 line (no
 * control words: ShowSprite writes position and control into the
 * slice's copper list).
 *
 * ShowSprite, InFront, InFrontF, InFrontB are Blitz mode only (odd error
 * check address in !subs).  ShowSprite clips at the top (data pointer
 * advanced 4 bytes per line - also in 32/64 pixel modes, BUG(orig) (#283); the
 * offset is a long register whose upper word is the caller's y, as in
 * the original) and
 * the bottom of the slice, switches the sprites off when completely
 * outside.  InFront/InFrontB set the playfield 2 priority bits (BPLCON2
 * 3-5) in the copper list of the slice, InFrontF those of playfield 1.
 *
 * BUG(orig), kept:
 *  - freeasprite (and SaveSprites/LoadSprites) compute the size with the
 *    CURRENT widthshift: after a SpriteMode change a sprite is freed with
 *    a wrong size. (#280)
 *  - SpriteMode reads the width from a table of 3 words: modes > 2 read
 *    the variables behind it (same layout here). (#281)
 *  - GetaSprite reads 16/32/64 pixels per sprite whatever the shape's
 *    width (beyond the row for narrow shapes). (#282)
 *  - LoadSprites s,file$: the last sprite is base + (max - 1) << 4 as a
 *    word: from a maximum of 2049 on it lies before the table. (#939)
 */
#include "blitzlib.h"
#define __NOLIBBASE__
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (thunks, defined below) */
void dogetsprite(void), doshowsprite(void), doinfront1(void), doinfront2(void);
void dosavesprites(void), doloadsprites(void), doloadsprites2(void);
void dospritemode(void), freeasprite(void);
ULONG codeinit(void);
void gspchk(void), sspchk(void), infchk(void), ssz(void);

#define AMIGAONLY(entry) ((char *)(entry) + 1)
#define BLITZONLY(chk)   ((char *)(chk) + 1)

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#spriteslib,init,#long,0,errors} */
BLITZ_LIBHEADER(LIB_spriteslib, init, BL_LONG, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_spriteslib, 0x1380, LIB_shapeslib, 0x1281);
BLITZ_SUBS(dogetsprite, gspchk, 0);
BLITZ_NAME("GetaSprite", "Sprite#,Shape#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_spriteslib, 0x1380, LIB_blitzcoplib, 0x1202);
BLITZ_SUBS(doshowsprite, BLITZONLY(sspchk), 0);
BLITZ_NAME("ShowSprite", "Sprite#,X,Y,Sprite Channel");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_blitzcoplib, 0x1202);
BLITZ_SUBS(doinfront1, BLITZONLY(infchk), 0);
BLITZ_NAME("InFront", "Sprite Channel");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_blitzcoplib, 0x1202);
BLITZ_SUBS(doinfront2, BLITZONLY(infchk), 0);
BLITZ_NAME("InFrontF", "Sprite Channel");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_blitzcoplib, 0x1202);
BLITZ_SUBS(doinfront1, BLITZONLY(infchk), 0);
BLITZ_NAME("InFrontB", "Sprite Channel");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_spriteslib, 0x1380, LIB_spriteslib, 0x1281, LIB_doslib, BL_LA(6));
BLITZ_SUBS(AMIGAONLY(dosavesprites), ssz, 0);
BLITZ_NAME("SaveSprites", "Sprite#,Sprite#,Filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_spriteslib, 0x1380, LIB_spriteslib, 0x1201, LIB_spriteslib, 0x0203,
           LIB_doslib, BL_LA(6));
BLITZ_SUBS(AMIGAONLY(doloadsprites2), 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_spriteslib, 0x1380, LIB_spriteslib, 0x1281, LIB_doslib, BL_LA(6));
BLITZ_SUBS(AMIGAONLY(doloadsprites), ssz, 0);
BLITZ_NAME("LoadSprites", "Sprite#[,Sprite#],Filename$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dospritemode, 0, 0);
BLITZ_NAME("SpriteMode", "0=16 1=32 2=64");

BLITZ_DUMTOKE(_toke, "Sprite", "");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freeasprite, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,20,4} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 20, 4);

/* ------------------------------------------------------------ variables */

#define SW(s, o) (*(UWORD *)((UBYTE *)(s) + (o)))
#define SL(s, o) (*(ULONG *)((UBYTE *)(s) + (o)))
#define SP(s, o) (*(UBYTE **)((UBYTE *)(s) + (o)))

/* shlup, then _spritemode, spritewid, widthshift (one block: SpriteMode
   indexes shlup by the mode, the init value points at _spritemode) */
static struct {
    UWORD shlup[3];
    UWORD mode, wid, shift;
} sv = { { 2, 4, 8 }, 0, 0, 0 };

static void *globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return (void *)BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(void *mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = (ULONG)mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* size of a sprite's data with the current width shift */
/* LSL.w Dn,d: count mod 64 */
static UWORD lslw(UWORD v, UWORD n)
{
    n &= 63;
    return n < 16 ? (UWORD)(v << n) : 0;
}

static ULONG spsize(UBYTE *sp)
{
    return (ULONG)lslw(SW(sp, 4), sv.shift) * SW(sp, 6);
}

ULONG BLITZ_ROUTINE codeinit(void)
{
    sv.mode = 0;
    sv.shift = 2;
    sv.wid = 2;
    return (ULONG)&sv.mode;
}

static void BLITZ_ROUTINE dospritemode_c(ULONG m BL_REG(d0))
{
    sv.mode = (UWORD)m;
    sv.shift = (UWORD)m + 2;
    sv.wid = *(UWORD *)((UBYTE *)sv.shlup + (WORD)(UWORD)((UWORD)m + (UWORD)m));
}
BLITZ_THUNK(dospritemode, dospritemode_c, "d0-d1/a0-a1");

static void freeasprite_(UBYTE *sp)
{
    void *d = SP(sp, 0);

    if (!d)
        return;
    SL(sp, 0) = 0;
    globfree(d, spsize(sp));
}

static void BLITZ_ROUTINE freeasprite_c(void)
{
    freeasprite_(blitz_string);
}
BLITZ_THUNK(freeasprite, freeasprite_c, "d0-d1/a0-a1");

/* ----------------------------------------------------------- GetaSprite */

/* one sprite image: two planes from a0 (row step = the shape's byte
   width) to a1, then a 0 line; returns the new a1 */
static UBYTE *onesprite(const UBYTE *src, UBYTE *dst, UWORD height, const UBYTE *shape)
{
    UWORD n = height - 2;
    WORD ps = (WORD)SW(shape, 22), bw = (WORD)SW(shape, 6);

    if (sv.mode == 0) {
        do {
            *(UWORD *)dst = *(const UWORD *)src;
            *(UWORD *)(dst + 2) = *(const UWORD *)(src + ps);
            dst += 4;
            src += bw;
        } while (n-- != 0);
        *(ULONG *)dst = 0;
        return dst + 4;
    }
    if (sv.mode == 1) {
        do {
            *(ULONG *)dst = *(const ULONG *)src;
            *(ULONG *)(dst + 4) = *(const ULONG *)(src + ps);
            dst += 8;
            src += bw;
        } while (n-- != 0);
        ((ULONG *)dst)[0] = 0;
        ((ULONG *)dst)[1] = 0;
        return dst + 8;
    }
    do {
        ((ULONG *)dst)[0] = ((const ULONG *)src)[0];
        ((ULONG *)dst)[1] = ((const ULONG *)src)[1];
        ((ULONG *)dst)[2] = ((const ULONG *)(src + ps))[0];
        ((ULONG *)dst)[3] = ((const ULONG *)(src + ps))[1];
        dst += 16;
        src += bw;
    } while (n-- != 0);
    ((ULONG *)dst)[0] = 0;
    ((ULONG *)dst)[1] = 0;
    ((ULONG *)dst)[2] = 0;
    ((ULONG *)dst)[3] = 0;
    return dst + 16;
}

/* a3 = sprite, a2 = shape */
static void BLITZ_ROUTINE dogetsprite_c(UBYTE *shape BL_REG(a2))
{
    UBYTE *sp = blitz_string, *dst;
    const UBYTE *src;
    UWORD height, count, imgsize;

    freeasprite_(sp);
    SW(sp, 12) = SW(shape, 10);
    SW(sp, 14) = SW(shape, 12);
    height = SW(shape, 2) + 1;                  /* hite */
    SW(sp, 4) = height;
    {
        UWORD n = (sv.shift + 2) & 63;          /* LSR.w: count mod 64 */
        count = (UWORD)(n < 16 ? (UWORD)(SW(shape, 0) - 1) >> n : 0) + 1;
    }
    SW(sp, 8) = 0;
    if (SW(shape, 4) == 4) {
        *(UBYTE *)(sp + 8) |= 0x80;
        count += count;
    }
    SW(sp, 6) = count;
    *(UBYTE *)(sp + 9) = *(UBYTE *)(shape + 1);
    imgsize = lslw(height, sv.shift);
    SW(sp, 10) = imgsize;
    dst = globalloc((ULONG)imgsize * count, MEMF_CHIP);
    SP(sp, 0) = dst;
    src = SP(shape, 14);
    count--;                                    /* number of sprites to do */
    do {
        dst = onesprite(src, dst, height, shape);
        if ((WORD)SW(sp, 8) < 0) {
            dst = onesprite(src + 2 * (WORD)SW(shape, 22), dst, height, shape);
            count--;
        }
        src += (WORD)sv.wid;
    } while (count-- != 0);
}
BLITZ_THUNK(dogetsprite, dogetsprite_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- Blitz mode */

/* ShowSprite: a3 = sprite, a2 = slice, d1 = x, d2 = y, d3 = channel */
static void BLITZ_ROUTINE doshowsprite_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2),
                                         ULONG ch BL_REG(d3), UBYTE *slice BL_REG(a2))
{
    UBYTE *sp = blitz_string, *cop;
    WORD hpos = (WORD)x - (WORD)SW(sp, 12) + 128;
    WORD vpos = (WORD)y - (WORD)SW(sp, 14);     /* d2: the start line */
    WORD vstop;                                 /* d2: the stop line */
    WORD maxh = (WORD)SW(slice, 32) + 1 - vpos; /* max sprite height */
    ULONG skip = 0;
    WORD height;
    UWORD pos, ctl, n;
    ULONG data, diff;

    if (maxh <= 0)
        goto spriteoff;                         /* off bottom */
    height = (WORD)SW(sp, 4);
    if (vpos < 0) {
        height += vpos;
        if (height <= 0)
            goto spriteoff;                     /* completely off top */
        /* EXG d2,d4:NEG d4:ASL #2,d4 - word operations on the long
           that was d2 (upper word from the caller's y), added as long */
        skip = (y & 0xffff0000UL) | (UWORD)((UWORD)-vpos << 2);
        vpos = 0;
    }
    if ((WORD)(maxh - height) < 0)              /* CMP d5,d0:BPL */
        height = maxh;
    vpos += (WORD)SW(slice, 0);                 /* y offset */
    if (*(UBYTE *)(slice + 3) & 0x10)
        vpos += 2;
    /* pos in d6, ctrl in d7 */
    ctl = 0;
    pos = (UWORD)((UWORD)vpos << 8);
    if ((UWORD)vpos & 0x100)                    /* LSL #8: carry = bit 8 */
        ctl = 4;
    if (hpos & 1)
        ctl++;
    hpos = (WORD)((UWORD)hpos >> 1);
    pos = (pos & 0xff00) | (UBYTE)hpos;
    vstop = vpos + height - 1;                  /* add height! */
    if ((UWORD)vstop & 0x100)
        ctl += 2;
    ctl |= (UWORD)((UWORD)vstop << 8);
    if ((WORD)SW(sp, 8) < 0)
        ctl |= 0x80;
    cop = SP(slice, 14) + (WORD)(UWORD)((UWORD)ch << 4);
    data = SL(sp, 0) + skip;
    diff = (ULONG)(LONG)(WORD)SW(sp, 10);
    n = SW(sp, 6) - 1;
    do {
        SW(cop, 2) = pos;
        SW(cop, 6) = ctl;
        SW(cop, 14) = (UWORD)data;
        SW(cop, 10) = (UWORD)(data >> 16);
        data += diff;
        cop += 16;
        if ((WORD)SW(sp, 8) < 0) {
            SW(cop, 2) = pos;
            SW(cop, 6) = ctl;
            SW(cop, 14) = (UWORD)data;
            SW(cop, 10) = (UWORD)(data >> 16);
            data += diff;
            cop += 16;
            n--;
        }
        pos += 8;
    } while (n-- != 0);
    return;

spriteoff:
    n = SW(sp, 6) - 1;
    cop = SP(slice, 14) + (WORD)(UWORD)((UWORD)ch << 4);
    do {
        SW(cop, 2) = 0;
        SW(cop, 6) = 0;
        cop += 16;
        if ((WORD)SW(sp, 8) < 0) {
            SW(cop, 2) = 0;
            SW(cop, 6) = 0;
            cop += 16;
            n--;
        }
    } while (n-- != 0);
}
BLITZ_THUNK(doshowsprite, doshowsprite_c, "d0-d1/a0-a1");

/* InFront/InFrontB: last sprite in front of playfield 2 (BPLCON2 3-5) */
static void BLITZ_ROUTINE doinfront1_c(ULONG ch BL_REG(d0), UBYTE *slice BL_REG(a2))
{
    UWORD *bplcon2 = (UWORD *)SP(slice, 26);

    *bplcon2 = (UWORD)((((UWORD)ch >> 1) & 7) << 3) | (*bplcon2 & 7);
}
BLITZ_THUNK(doinfront1, doinfront1_c, "d0-d1/a0-a1");

/* InFrontF: last sprite in front of playfield 1 (BPLCON2 0-2) */
static void BLITZ_ROUTINE doinfront2_c(ULONG ch BL_REG(d0), UBYTE *slice BL_REG(a2))
{
    UWORD *bplcon2 = (UWORD *)SP(slice, 26);

    *bplcon2 = (UWORD)(((UWORD)ch >> 1) & 7) | (*bplcon2 & 0x38);
}
BLITZ_THUNK(doinfront2, doinfront2_c, "d0-d1/a0-a1");

/* ----------------------------------------------------- sprites files */

static struct DosLibrary *dosbase;
#define DOSBase dosbase                         /* for proto/dos.h */

BLITZ_ERRCHK_MESSAGE(mess10, "Can't open Sprites file");
BLITZ_ERRCHK_MESSAGE(mess11, "Error reading/writing Sprites file");

static void BLITZ_ROUTINE dosavesprites_c(UBYTE *last BL_REG(a2), const char *name BL_REG(d2),
                                          struct DosLibrary *dos BL_REG(a6))
{
    UBYTE *sp = blitz_string;
    BPTR fh;

    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (!fh)
        BLITZ_RUNERR(mess10);
    do {
        if (Write(fh, sp, 16) != 16)
            BLITZ_RUNERR(mess11);
        if (SL(sp, 0)) {
            ULONG n = spsize(sp);

            if ((ULONG)Write(fh, SP(sp, 0), n) != n)
                BLITZ_RUNERR(mess11);
        }
        sp += 16;
    } while ((ULONG)sp <= (ULONG)last);
    Close(fh);
}
BLITZ_THUNK(dosavesprites, dosavesprites_c, "d0-d1/a0-a1");

static void loadsprites(UBYTE *sp, UBYTE *last, const char *name, struct DosLibrary *dos)
{
    BPTR fh;

    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_OLDFILE);
    if (!fh)
        BLITZ_RUNERR(mess10);
    do {
        LONG r;

        freeasprite_(sp);
        r = Read(fh, sp, 16);
        if (!r)
            break;
        if (r != 16)
            BLITZ_RUNERR(mess11);
        if (SL(sp, 0)) {
            ULONG n = spsize(sp);

            SP(sp, 0) = globalloc(n, MEMF_CHIP);
            if ((ULONG)Read(fh, SP(sp, 0), n) != n)
                BLITZ_RUNERR(mess11);
        }
        sp += 16;
    } while ((ULONG)sp <= (ULONG)last);
    Close(fh);
}

static void BLITZ_ROUTINE doloadsprites_c(UBYTE *last BL_REG(a2), const char *name BL_REG(d2),
                                          struct DosLibrary *dos BL_REG(a6))
{
    loadsprites(blitz_string, last, name, dos);
}
BLITZ_THUNK(doloadsprites, doloadsprites_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE doloadsprites2_c(UBYTE *base BL_REG(a2), ULONG max BL_REG(d2),
                                           const char *name BL_REG(d1),
                                           struct DosLibrary *dos BL_REG(a6))
{
    /* BUG(orig) (#939): the last sprite as a word offset */
    loadsprites(blitz_string, base + (WORD)(UWORD)((UWORD)(max - 1) << 4), name, dos);
}
BLITZ_THUNK(doloadsprites2, doloadsprites2_c, "d0-d1/a0-a1");

/* ************************** ERROR CHECKING ************************** */

BLITZ_ERRCHK_MESSAGE(mess1, "Uninitialized Shape");
BLITZ_ERRCHK_MESSAGE(mess2, "Shape MUST be 4 or 16 colours");
BLITZ_ERRCHK_MESSAGE(mess3, "Sprite too Wide");
BLITZ_ERRCHK_MESSAGE(mess4, "Uninitialized Sprite");
BLITZ_ERRCHK_MESSAGE(mess5, "Uninitialized Slice");
BLITZ_ERRCHK_MESSAGE(mess6, "Sprite Channel Unavailable");
BLITZ_ERRCHK_MESSAGE(mess7, "Slice does not support priority");
BLITZ_ERRCHK_MESSAGE(mess8, "Sprite Channel not in range");
BLITZ_ERRCHK_MESSAGE(mess9, "First Sprite must be lower than second");

/* GetaSprite: shape there, 4 or 16 colours, not too wide */
static void BLITZ_ERRCHK gspchk_c(UBYTE *shape BL_REG(a2))
{
    WORD maxw = (WORD)(sv.wid << 5);

    if (!SW(shape, 0))
        BLITZ_RUNERR(mess1);
    if (SW(shape, 4) != 2) {
        if (SW(shape, 4) != 4)
            BLITZ_RUNERR(mess2);
        maxw <<= 1;
    }
    if ((WORD)(maxw - (WORD)SW(shape, 0)) < 0)  /* CMP (a2),d2:BMI */
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK sspchk_c(ULONG ch BL_REG(d3), UBYTE *slice BL_REG(a2))
{
    UBYTE *sp = blitz_string;

    if (!SL(sp, 0))
        BLITZ_RUNERR(mess4);
    if (!SW(slice, 0))
        BLITZ_RUNERR(mess5);
    if ((UWORD)((UWORD)ch + SW(sp, 6)) > SW(slice, 6))
        BLITZ_RUNERR(mess6);
}

static void BLITZ_ERRCHK infchk_c(ULONG ch BL_REG(d0), UBYTE *slice BL_REG(a2))
{
    if ((ch & 1) || (UWORD)ch > 8)
        BLITZ_RUNERR(mess8);
    if (!SW(slice, 0))
        BLITZ_RUNERR(mess5);
    if (!SL(slice, 26))
        BLITZ_RUNERR(mess7);
}

static void BLITZ_ERRCHK ssz_c(UBYTE *last BL_REG(a2))
{
    if ((ULONG)blitz_string > (ULONG)last)
        BLITZ_RUNERR(mess9);
}

/* the thunks after all check functions: their assembler switches back to
   .text, which GCC does not notice (a check function after a thunk would
   end up in the normal code) */
BLITZ_ERRCHK_THUNK(gspchk, gspchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(sspchk, sspchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(infchk, infchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(ssz, ssz_c, "d0-d1/a0-a1");
