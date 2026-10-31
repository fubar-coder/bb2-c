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
#include "blitzobj.h"
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

/* the objects: the Sprite (16 bytes, the !libfin item size), the shape
   (shapeslib) and the slice (blitzcoplib) - struct bb_sprite, struct
   bb_shape, struct bb_slice (blitzobj.h) */

/* a hardware sprite's part of the slice's copper list (at +14 of the
   slice, 16 bytes per sprite channel): four copper moves, ShowSprite
   writes their values - position, control, data pointer high and low */
struct copmove {
    UWORD reg, val;
};
struct spritecop {
    struct copmove pos, ctl, pth, ptl;
};
static_assert(sizeof(struct spritecop) == 16);

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

static ULONG spsize(const struct bb_sprite *sp)
{
    return (ULONG)lslw(sp->height, sv.shift) * sp->count;
}

ULONG BLITZ_ROUTINE codeinit(void)
{
    sv.mode = 0;
    sv.shift = 2;
    sv.wid = 2;
    return (ULONG)&sv.mode;
}

/**
 * @brief SpriteMode 0=16 1=32 2=64 - the sprite width (FMODE) for the
 * following GetaSprite/ShowSprite.
 *
 * Mode 0/1/2 = 16/32/64 pixels per sprite: widthshift 2/3/4, spritewid
 * 2/4/8 bytes (from the table shlup).
 *
 * BUG(orig) (#281): the width is read from a table of 3 words without a
 * check: modes > 2 read the variables behind it.
 * BUG(orig) (#280): sprites made before the change are later freed (and
 * saved/loaded) with the size of the new mode.
 *
 * @param m d0.w: the mode
 */
static void BLITZ_ROUTINE dospritemode_c(ULONG m BL_REG(d0))
{
    sv.mode = (UWORD)m;
    sv.shift = (UWORD)m + 2;
    sv.wid = *(UWORD *)((UBYTE *)sv.shlup + (WORD)(UWORD)((UWORD)m + (UWORD)m));
}
BLITZ_THUNK(dospritemode, dospritemode_c, "d0-d1/a0-a1");

static void freeasprite_(struct bb_sprite *sp)
{
    void *d = sp->data;

    if (!d)
        return;
    sp->data = 0;
    globfree(d, spsize(sp));
}

static void BLITZ_ROUTINE freeasprite_c(void)
{
    freeasprite_((struct bb_sprite *)blitz_string);
}
BLITZ_THUNK(freeasprite, freeasprite_c, "d0-d1/a0-a1");

/* ----------------------------------------------------------- GetaSprite */

/* one sprite image: two planes from a0 (row step = the shape's byte
   width) to a1, then a 0 line; returns the new a1 */
static UBYTE *onesprite(const UBYTE *src, UBYTE *dst, UWORD height, const struct bb_shape *shape)
{
    UWORD n = height - 2;
    WORD ps = (WORD)shape->planesize, bw = (WORD)shape->ebwidth;

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

/**
 * @brief GetaSprite Sprite#,Shape# - make a sprite from a 4 or 16 colour
 * shape.
 *
 * Frees the sprite's old data, allocates CHIP memory for
 * ((width-1) >> (shift+2)) + 1 hardware sprites (twice for 16 colours:
 * attached pairs, planes 2+3 in the attached sprite) and copies per sprite
 * and row the two planes' words/longs, then a 0 line (no control words:
 * ShowSprite writes position and control into the slice's copper list).
 * Takes the shape's handle.
 *
 * Errors (debugger, gspchk): "Uninitialized Shape", "Shape MUST be 4 or
 * 16 colours", "Sprite too Wide".
 *
 * BUG(orig) (#282): reads 16/32/64 pixels per sprite whatever the
 * shape's width (beyond the row for narrow shapes).
 *
 * @param shape a2: the shape object; a3: the sprite object
 */
static void BLITZ_ROUTINE dogetsprite_c(struct bb_shape *shape BL_REG(a2))
{
    struct bb_sprite *sp = (struct bb_sprite *)blitz_string;
    UBYTE *dst;
    const UBYTE *src;
    UWORD height, count, imgsize;

    freeasprite_(sp);
    sp->handlex = (UWORD)shape->handlex;
    sp->handley = (UWORD)shape->handley;
    height = shape->height + 1;                 /* hite */
    sp->height = height;
    {
        UWORD n = (sv.shift + 2) & 63;          /* LSR.w: count mod 64 */
        count = (UWORD)(n < 16 ? (UWORD)(shape->width - 1) >> n : 0) + 1;
    }
    sp->flags = 0;
    if (shape->depth == 4) {
        sp->attach |= 0x80;
        count += count;
    }
    sp->count = count;
    sp->width = (UBYTE)shape->width;
    imgsize = lslw(height, sv.shift);
    sp->imgsize = imgsize;
    dst = globalloc((ULONG)imgsize * count, MEMF_CHIP);
    sp->data = dst;
    src = shape->data;
    count--;                                    /* number of sprites to do */
    do {
        dst = onesprite(src, dst, height, shape);
        if ((WORD)sp->flags < 0) {
            dst = onesprite(src + 2 * (WORD)shape->planesize, dst, height, shape);
            count--;
        }
        src += (WORD)sv.wid;
    } while (count-- != 0);
}
BLITZ_THUNK(dogetsprite, dogetsprite_c, "d0-d1/a0-a1");

/* ------------------------------------------------------- Blitz mode */

/**
 * @brief ShowSprite Sprite#,X,Y,Sprite Channel - show a sprite on the
 * used slice (Blitz mode only).
 *
 * Writes position, control and data pointer of each hardware sprite into
 * the slice's copper list from the channel on.  Clips at the top (the
 * data pointer is advanced 4 bytes per line; the offset is a long
 * register whose upper word is the caller's y, as in the original) and
 * the bottom of the slice; switches the sprites off when completely
 * outside.
 *
 * Errors (debugger, sspchk): "Uninitialized Sprite", "Uninitialized
 * Slice", "Sprite Channel Unavailable".
 *
 * BUG(orig) (#283): the top clipping skips 4 bytes per line also in the
 * 32/64 pixel modes.
 *
 * @param x     d1: x position
 * @param y     d2: y position
 * @param ch    d3: the first sprite channel
 * @param slice a2: the used slice; a3: the sprite object
 */
static void BLITZ_ROUTINE doshowsprite_c(ULONG x BL_REG(d1), ULONG y BL_REG(d2),
                                         ULONG ch BL_REG(d3), struct bb_slice *slice BL_REG(a2))
{
    struct bb_sprite *sp = (struct bb_sprite *)blitz_string;
    struct spritecop *cop;
    WORD hpos = (WORD)x - (WORD)sp->handlex + 128;
    WORD vpos = (WORD)y - (WORD)sp->handley;     /* d2: the start line */
    WORD vstop;                                 /* d2: the stop line */
    WORD maxh = (WORD)slice->height + 1 - vpos; /* max sprite height */
    ULONG skip = 0;
    WORD height;
    UWORD pos, ctl, n;
    ULONG data, diff;

    if (maxh <= 0)
        goto spriteoff;                         /* off bottom */
    height = (WORD)sp->height;
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
    vpos += (WORD)slice->y;                 /* y offset */
    if (slice->flagslo & 0x10)
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
    if ((WORD)sp->flags < 0)
        ctl |= 0x80;
    cop = (struct spritecop *)((UBYTE *)slice->sprmoves + (WORD)(UWORD)((UWORD)ch << 4));
    data = (ULONG)sp->data + skip;
    diff = (ULONG)(LONG)(WORD)sp->imgsize;
    n = sp->count - 1;
    do {
        cop->pos.val = pos;
        cop->ctl.val = ctl;
        cop->ptl.val = (UWORD)data;
        cop->pth.val = (UWORD)(data >> 16);
        data += diff;
        cop++;
        if ((WORD)sp->flags < 0) {
            cop->pos.val = pos;
            cop->ctl.val = ctl;
            cop->ptl.val = (UWORD)data;
            cop->pth.val = (UWORD)(data >> 16);
            data += diff;
            cop++;
            n--;
        }
        pos += 8;
    } while (n-- != 0);
    return;

spriteoff:
    n = sp->count - 1;
    cop = (struct spritecop *)((UBYTE *)slice->sprmoves + (WORD)(UWORD)((UWORD)ch << 4));
    do {
        cop->pos.val = 0;
        cop->ctl.val = 0;
        cop++;
        if ((WORD)sp->flags < 0) {
            cop->pos.val = 0;
            cop->ctl.val = 0;
            cop++;
            n--;
        }
    } while (n-- != 0);
}
BLITZ_THUNK(doshowsprite, doshowsprite_c, "d0-d1/a0-a1");

/**
 * @brief InFront Sprite Channel, InFrontB Sprite Channel - sprites from
 * the channel on appear in front of playfield 2 (Blitz mode only).
 *
 * Sets the playfield 2 priority bits (BPLCON2 3-5) to channel/2 in the
 * copper list of the used slice.
 *
 * Errors (debugger, infchk): "Sprite Channel not in range" (odd or > 8),
 * "Uninitialized Slice", "Slice does not support priority".
 *
 * @param ch    d0.w: the sprite channel (even, 0-8)
 * @param slice a2: the used slice
 */
static void BLITZ_ROUTINE doinfront1_c(ULONG ch BL_REG(d0), struct bb_slice *slice BL_REG(a2))
{
    UWORD *bplcon2 = (UWORD *)slice->con2;

    *bplcon2 = (UWORD)((((UWORD)ch >> 1) & 7) << 3) | (*bplcon2 & 7);
}
BLITZ_THUNK(doinfront1, doinfront1_c, "d0-d1/a0-a1");

/**
 * @brief InFrontF Sprite Channel - sprites from the channel on appear in
 * front of playfield 1 (Blitz mode only).
 *
 * Sets the playfield 1 priority bits (BPLCON2 0-2) to channel/2 in the
 * copper list of the used slice.
 *
 * Errors (debugger, infchk): as InFront.
 *
 * @param ch    d0.w: the sprite channel (even, 0-8)
 * @param slice a2: the used slice
 */
static void BLITZ_ROUTINE doinfront2_c(ULONG ch BL_REG(d0), struct bb_slice *slice BL_REG(a2))
{
    UWORD *bplcon2 = (UWORD *)slice->con2;

    *bplcon2 = (UWORD)(((UWORD)ch >> 1) & 7) | (*bplcon2 & 0x38);
}
BLITZ_THUNK(doinfront2, doinfront2_c, "d0-d1/a0-a1");

/* ----------------------------------------------------- sprites files */

static struct DosLibrary *dosbase;
#define DOSBase dosbase                         /* for proto/dos.h */

BLITZ_ERRCHK_MESSAGE(mess10, "Can't open Sprites file");
BLITZ_ERRCHK_MESSAGE(mess11, "Error reading/writing Sprites file");

/**
 * @brief SaveSprites Sprite#,Sprite#,Filename$ - save a range of sprites
 * to a file (Amiga mode only).
 *
 * Per sprite the 16 byte object, then its data if it has any.
 *
 * Errors: "Can't open Sprites file", "Error reading/writing Sprites file"
 * (runtime errors also without the debugger); debugger (ssz): "First
 * Sprite must be lower than second".
 *
 * BUG(orig) (#280): the data size uses the current SpriteMode.
 *
 * @param last a2: the last sprite object; a3: the first
 * @param name d2: the file name
 * @param dos  a6: dos.library
 */
static void BLITZ_ROUTINE dosavesprites_c(UBYTE *last BL_REG(a2), const char *name BL_REG(d2),
                                          struct DosLibrary *dos BL_REG(a6))
{
    struct bb_sprite *sp = (struct bb_sprite *)blitz_string;
    BPTR fh;

    dosbase = dos;
    fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
    if (!fh)
        BLITZ_RUNERR(mess10);
    do {
        if (Write(fh, sp, 16) != 16)
            BLITZ_RUNERR(mess11);
        if (sp->data) {
            ULONG n = spsize(sp);

            if ((ULONG)Write(fh, sp->data, n) != n)
                BLITZ_RUNERR(mess11);
        }
        sp++;
    } while ((ULONG)sp <= (ULONG)last);
    Close(fh);
}
BLITZ_THUNK(dosavesprites, dosavesprites_c, "d0-d1/a0-a1");

static void loadsprites(struct bb_sprite *sp, const UBYTE *last, const char *name, struct DosLibrary *dos)
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
        if (sp->data) {
            ULONG n = spsize(sp);

            sp->data = globalloc(n, MEMF_CHIP);
            if ((ULONG)Read(fh, sp->data, n) != n)
                BLITZ_RUNERR(mess11);
        }
        sp++;
    } while ((ULONG)sp <= (ULONG)last);
    Close(fh);
}

/**
 * @brief LoadSprites Sprite#,Sprite#,Filename$ - load a range of sprites
 * from a file made by SaveSprites (Amiga mode only).
 *
 * Frees each sprite, reads its 16 byte object and its data (CHIP
 * memory); stops at the end of the file.
 *
 * Errors: as SaveSprites.
 *
 * BUG(orig) (#280): the data size uses the current SpriteMode.
 *
 * @param last a2: the last sprite object; a3: the first
 * @param name d2: the file name
 * @param dos  a6: dos.library
 */
static void BLITZ_ROUTINE doloadsprites_c(UBYTE *last BL_REG(a2), const char *name BL_REG(d2),
                                          struct DosLibrary *dos BL_REG(a6))
{
    loadsprites((struct bb_sprite *)blitz_string, last, name, dos);
}
BLITZ_THUNK(doloadsprites, doloadsprites_c, "d0-d1/a0-a1");

/**
 * @brief LoadSprites Sprite#,Filename$ - load sprites from the given one
 * up to the last of the object table (Amiga mode only).
 *
 * As LoadSprites with two sprites; the last is base + (maximum - 1) << 4.
 *
 * BUG(orig) (#939): that offset is a word: from a maximum of 2049 on the
 * last sprite lies before the table.
 *
 * @param base a2: the sprite table (object base); a3: the first sprite
 * @param max  d2: the maximum number of sprites
 * @param name d1: the file name
 * @param dos  a6: dos.library
 */
static void BLITZ_ROUTINE doloadsprites2_c(UBYTE *base BL_REG(a2), ULONG max BL_REG(d2),
                                           const char *name BL_REG(d1),
                                           struct DosLibrary *dos BL_REG(a6))
{
    /* BUG(orig) (#939): the last sprite as a word offset */
    loadsprites((struct bb_sprite *)blitz_string, base + (WORD)(UWORD)((UWORD)(max - 1) << 4), name, dos);
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
static void BLITZ_ERRCHK gspchk_c(struct bb_shape *shape BL_REG(a2))
{
    WORD maxw = (WORD)(sv.wid << 5);

    if (!shape->width)
        BLITZ_RUNERR(mess1);
    if (shape->depth != 2) {
        if (shape->depth != 4)
            BLITZ_RUNERR(mess2);
        maxw <<= 1;
    }
    if ((WORD)(maxw - (WORD)shape->width) < 0)  /* CMP (a2),d2:BMI */
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK sspchk_c(ULONG ch BL_REG(d3), struct bb_slice *slice BL_REG(a2))
{
    struct bb_sprite *sp = (struct bb_sprite *)blitz_string;

    if (!sp->data)
        BLITZ_RUNERR(mess4);
    if (!slice->y)
        BLITZ_RUNERR(mess5);
    if ((UWORD)((UWORD)ch + sp->count) > slice->sprites)
        BLITZ_RUNERR(mess6);
}

static void BLITZ_ERRCHK infchk_c(ULONG ch BL_REG(d0), struct bb_slice *slice BL_REG(a2))
{
    if ((ch & 1) || (UWORD)ch > 8)
        BLITZ_RUNERR(mess8);
    if (!slice->y)
        BLITZ_RUNERR(mess5);
    if (!slice->con2)
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
