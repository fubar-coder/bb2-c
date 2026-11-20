/*
 * riwbspritelib - RWE userlib 10 (literal in the source, Reflective
 * Images, 1994): sprites on Workbench screens through the OS 3 sprite
 * functions of graphics.library -
 *   GetWBSprite(x,y,shape[,spritechannel])   a sprite from a Blitz shape
 *                       (AllocSpriteDataA, GetExtSpriteA); the channel
 *   FreeWBSprite sprite_channel
 *   MoveWBSprite(viewport,x,y,sprite_channel)          MoveSprite
 *   ChangeWBSprite(viewport,sprite_channel,shape#)     a new image
 *                       (ChangeExtSpriteA)
 *   WBSpriteX(sprite_channel), WBSpriteY(sprite_channel)
 * init opens graphics.library (any version; the functions need 39),
 * finit frees the sprites and closes it.
 *
 * C translation of rwelibsrc/wbspritelib/WBSprite-library2.bb;
 * reference: the reference build of that source with blitz2 2.10
 * (REFERENCE refbuild, ref/riwbspritelib.obj) - no binary of the library
 * exists.  Notes: docs/rwelibs/port/riwbspritelib.md.
 *
 * The variables, tag lists, the sprite table (spritepointers: the
 * ExtSprite of each channel), the library name and base are one block
 * in the original's order (struct wbs): the release code indexes the
 * sprite table without bounds and so reads the tag list before it and
 * the name behind it (the error checks refuse such a channel, fixed
 * #916).  A shape is copied into a bitmap of
 * AllocBitMap, which AllocSpriteDataA converts.  Assembler besides the
 * thunks: the calls of MoveSprite and FreeSpriteData (the original
 * returned the d0 they leave; FreeWBSprite still does, a statement).
 *
 * Fixed:
 *   - ChangeExtSprite keeps a3 (the original left it on the tag list it
 *     passes to ChangeExtSpriteA) (#1028).
 *   - GetWBSprite returns -1 for every failure and the channel 0-7 on
 *     success.  The original decided by the table entry of the channel:
 *     if GetExtSpriteA or AllocSpriteDataA failed, the result was still
 *     the channel when the library held a sprite there, else 0; a narrow
 *     shape (< 16) or no graphics.library gave 0, no shape data or a
 *     failing AllocBitMap -1 (0 could not be told from channel 0)
 *     (#920).
 *   - MoveWBSprite returns -1 when it moved the sprite, 0 without one;
 *     the original returned the d0 MoveSprite leaves (declared as a byte
 *     result) (#921).
 *   - ChangeWBSprite frees the new sprite data when ChangeExtSpriteA
 *     fails; the original lost them (chip memory of graphics.library,
 *     until reboot) (#915).
 *   - The shape is copied row by row with the shape's ebwidth and the
 *     bitmap's BytesPerRow; the original copied width/16 words per row
 *     back to back, so a shape whose width is no multiple of 16 (one word
 *     more per row) was sheared and its next planes read from inside
 *     plane 0 (#918).  The sprite keeps its width (rounded down to 16).
 *   - With runtime error checking every command refuses a channel
 *     outside 0-7 (-1 to 7 for GetWBSprite): "Sprite channel must be 0
 *     to 7" / "... -1 to 7" (the original has no error check area).  The
 *     release code is unchanged (an invalid argument, D-005): GetWBSprite
 *     stores at spritepointers + channel*4 (as a word index);
 *     FreeWBSprite takes the channel modulo 16 for the table (entries
 *     8-15 are the name and the base) but passes the whole register to
 *     FreeSprite; MoveWBSprite, ChangeWBSprite, WBSpriteX/Y index the
 *     table with the channel as is (#916).
 *   - With runtime error checking GetWBSprite and ChangeWBSprite refuse
 *     a shape with data and a height of 0 ("Illegal Shape Height") or a
 *     depth of 0 or above 8 ("Illegal Shape Depth"), an invalid object.
 *     The release code is unchanged (D-005): the copy counts height - 1
 *     and depth - 1 with DBRA (65536 rows or planes) (#917).
 *
 * BUG(orig):
 *   - WBSpriteX/Y without a sprite read the words at 6 and 8 (#922).
 *   - finit does not clear the library base: a second finit closes
 *     graphics.library again (#919).
 */
#include "blitzlib.h"
#include "blitzobj.h"
#include "blitzexec.h"
#include <graphics/gfx.h>
#include <graphics/sprite.h>
#include <graphics/view.h>
#include <utility/tagitem.h>

/* entry points (thunks, below) */
void _GetWBSprite(void), _freesprite(void), _movewbsprite(void);
void _changewbsprite(void), _wbspritex(void), _wbspritey(void);
void riwbspritelib_libinit(void), riwbspritelib_libfinit(void);
void _getwbspritechk(void), _freesprchk(void), _movewbspritechk(void);
void _changewbspritechk(void), _wbspritexychk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {10,init,0,finit,0}; error field: the error check area of
   the channel checks (fixed, #916) */
BLITZ_LIBHEADER(LIB_riwbspritelib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

/* `#ia0|pd2` in the source: pd2 is 2, the shape of parameter 2 */
BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD2);
/* +1: Amiga mode only; check: fixed #916 */
BLITZ_SUBS((char *)_GetWBSprite + 1, _getwbspritechk, 0);
BLITZ_NAME("GetWBSprite", "x,y,shape[,spritechannel]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS((char *)_freesprite + 1, _freesprchk, 0);       /* check: fixed #916 */
BLITZ_NAME("FreeWBSprite", "sprite_channel");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS((char *)_movewbsprite + 1, _movewbspritechk, 0);    /* check: fixed #916 */
BLITZ_NAME("MoveWBSprite", "viewport,x,y,sprite_channel");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, BL_IA0|BL_PD2);
BLITZ_SUBS((char *)_changewbsprite + 1, _changewbspritechk, 0);    /* check: fixed #916 */
BLITZ_NAME("ChangeWBSprite", "viewport,sprite_channel,shape#");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS((char *)_wbspritex + 1, _wbspritexychk, 0);     /* check: fixed #916 */
BLITZ_NAME("WBSpriteX", "sprite_channel");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS((char *)_wbspritey + 1, _wbspritexychk, 0);     /* check: fixed #916 */
BLITZ_NAME("WBSpriteY", "sprite_channel");

BLITZ_NULLSUB_AT(finit, riwbspritelib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, riwbspritelib_libinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------- the data */

/* the original's data block, in its order */
static struct BL_PACKED wbs {
    ULONG myviewport, oldpointer, mybitmap, shapeaddr;
    LONG sprchannel;
    UWORD sprx, spry;
    ULONG temppointer;
    UWORD sprwidth, sprheight;
    struct TagItem allocspritetaglist[6];
    struct TagItem getspritetaglist[3];
    struct TagItem dudtaglist[1];       /* TAG_DONE; ends getspritetaglist too */
    ULONG spritepointers[8];
    char graphicslibrary[18];
    struct Library *graphicslibbase;
} wd = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    { { 0x81000000UL, 16 },             /* SPRITEA_Width */
      { 0x81000006UL, 16 },             /* SPRITEA_OutputHeight */
      { 0x81000002UL, 0 },              /* SPRITEA_XReplication */
      { 0x81000004UL, 0 },              /* SPRITEA_YReplication */
      { 0x81000008UL, 0 },              /* SPRITEA_Attached */
      { TAG_DONE, 0 } },
    { { 0x82000020UL, 1 },              /* GSTAG_SPRITE_NUM */
      { 0x82000022UL, 0 },              /* GSTAG_ATTACHED */
      { 0x82000024UL, 0 } },            /* GSTAG_SOFTSPRITE (no TAG_DONE: */
    { { TAG_DONE, 0 } },                /* dudtaglist ends it) */
    { 0 },
    "graphics.library",
    NULL,
};
static_assert(sizeof wd == 166);

#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME wd.graphicslibbase
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* the low word of a long (MOVE.w to its address + 2) */
#define LOWORD(l) (((UWORD *)&(l))[1])

/* a Blitz shape (shapeslib): struct bb_shape (blitzobj.h); its data
   address tested as the long the original reads (as a pointer test GCC
   would order the code differently) */
#define SHAPEDATA(s) (*(const ULONG *)&((const struct bb_shape *)(s))->data)

/* ASL.w #2,d: the low word shifted, the upper word kept */
#define ASLW2(d) (((ULONG)(d) & 0xffff0000UL) | (UWORD)((ULONG)(d) << 2))
/* the sprite table entry of a channel indexed as a word, d(a0,d1.w) */
#define ENTRYW(ch) ((ULONG *)((UBYTE *)wd.spritepointers + (WORD)((UWORD)(ch) << 2)))

/* MoveSprite(vp, sprite, x, y) with the registers of the original */
static void movesprite(ULONG vp, ULONG spr, ULONG x, ULONG y)
{
    register ULONG d0 __asm("d0") = x;
    register ULONG d1 __asm("d1") = y;
    register ULONG a0 __asm("a0") = vp;
    register ULONG a1 __asm("a1") = spr;
    register struct Library *a6 __asm("a6") = wd.graphicslibbase;

    __asm volatile("jsr -426(a6)"
        : "+d"(d0), "+d"(d1), "+a"(a0), "+a"(a1), "+a"(a6)
        :
        : "cc", "memory");
}

/* FreeSpriteData(sprite); the d0 graphics.library leaves (FreeWBSprite's
   d0) */
static ULONG freespritedata(ULONG spr)
{
    register ULONG d0 __asm("d0");
    register ULONG a2 __asm("a2") = spr;
    register struct Library *a6 __asm("a6") = wd.graphicslibbase;

    __asm volatile("jsr -1032(a6)"
        : "=d"(d0), "+a"(a2), "+a"(a6)
        :
        : "d1", "a0", "a1", "cc", "memory");
    return d0;
}

/* ChangeExtSpriteA(vp, old, new, tags): tags in a3.  Fixed (#1028): a3
   comes back as it was (the libraries' safe form of the NDK macro,
   cmake/ndk-calls.cmake; the original left it on its tag list) */
static LONG changeextsprite(ULONG vp, ULONG old, ULONG new, ULONG tags)
{
    return ChangeExtSpriteA((struct ViewPort *)vp, (struct ExtSprite *)old,
                            (struct ExtSprite *)new, (struct TagItem *)tags);
}

/**
 * @brief init: opens graphics.library, any version (the sprite functions
 * need 39).
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    wd.graphicslibbase = OpenLibrary((CONST_STRPTR)wd.graphicslibrary, 0);
}

/**
 * @brief finit: every sprite freed (FreeSprite, FreeSpriteData), the
 * library closed.
 *
 * BUG(orig) (#919): the base is kept, a second finit closes
 * graphics.library again.
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    int i;

    if (!wd.graphicslibbase)
        return;
    for (i = 0; i < 8; i++) {
        if (wd.spritepointers[i]) {
            FreeSprite(i);
            FreeSpriteData((struct ExtSprite *)wd.spritepointers[i]);
        }
        wd.spritepointers[i] = 0;
    }
    CloseLibrary(wd.graphicslibbase);
}

/* the shape (wd.shapeaddr) into the planes of `bm`: width/16 words per
   row, the source row after row by the shape's ebwidth, the target by
   the bitmap's BytesPerRow (fixed #918); height rows, depth planes (DBRA
   counts: a height or depth of 0 is refused by the error checks, fixed
   #917) */
static void copyshape(struct BitMap *bm)
{
    const struct bb_shape *shape = (const struct bb_shape *)wd.shapeaddr;
    UWORD wcount = (UWORD)(((WORD)shape->width >> 4) - 1);
    UWORD hcount = (UWORD)(shape->height - 1);
    UWORD pcount = (UWORD)(shape->depth - 1);
    UWORD sstep = shape->ebwidth, dstep = bm->BytesPerRow;
    const UBYTE *src = shape->data;
    PLANEPTR *planes = bm->Planes;

    do {
        UBYTE *dst = (UBYTE *)*planes++;
        UWORD h = hcount;

        do {
            const UWORD *s = (const UWORD *)src;
            UWORD *d = (UWORD *)dst;
            UWORD w = wcount;

            do
                *d++ = *s++;
            while (w-- != 0);
            src += sstep;
            dst += dstep;
            __asm__("" : "+a"(src), "+a"(dst));     /* no __mulsi3 */
        } while (h-- != 0);
    } while (pcount-- != 0);
}

/* the shape (wd.shapeaddr) into a new bitmap (wd.mybitmap) and the
   tag list: 1, 0 if narrower than 16, -1 if AllocBitMap fails */
static int shapebitmap(void)
{
    const struct bb_shape *shape = (const struct bb_shape *)wd.shapeaddr;
    UWORD w = shape->width & 0xfff0;
    struct BitMap *bm;

    if (!w)
        return 0;
    wd.sprwidth = w;
    wd.sprheight = shape->height;
    bm = AllocBitMap(w, wd.sprheight, shape->depth, 0, NULL);
    if (!bm)
        return -1;
    wd.mybitmap = (ULONG)bm;
    copyshape(bm);
    LOWORD(wd.allocspritetaglist[0].ti_Data) = wd.sprwidth;    /* SPRITEA_Width */
    LOWORD(wd.allocspritetaglist[1].ti_Data) = wd.sprheight;   /* SPRITEA_OutputHeight */
    return 1;
}

/* the new sprite data, x/y from sprx/spry: 0 if AllocSpriteDataA fails */
static ULONG newsprite(void)
{
    ULONG spr = (ULONG)AllocSpriteDataA((struct BitMap *)wd.mybitmap,
                                        wd.allocspritetaglist);

    if (!spr)
        return 0;
    wd.temppointer = spr;
    ((struct ExtSprite *)spr)->es_SimpleSprite.x = wd.sprx;
    ((struct ExtSprite *)spr)->es_SimpleSprite.y = wd.spry;
    return spr;
}

/**
 * @brief GetWBSprite(x,y,shape[,spritechannel]): a Workbench sprite from
 * a Blitz shape.
 *
 * Amiga mode only.  The shape is copied into a bitmap of AllocBitMap,
 * converted with AllocSpriteDataA (x, y set) and got with GetExtSpriteA
 * (channel -1: any); the sprite goes into the table.  The copy has
 * DBRA counts; the channel and the shape: getwbspritechk (fixed #916,
 * #917).
 * Fixed (#918): the copy steps by the shape's ebwidth and the bitmap's
 * BytesPerRow.  Fixed (#920): every
 * failure (no graphics.library, no shape data, a shape narrower than
 * 16, a failing AllocBitMap, AllocSpriteDataA or GetExtSpriteA) gives
 * -1; the original decided by the channel's table entry (the channel if
 * the library held a sprite there, else 0) and gave 0 or -1 for the
 * others.
 * @param x      d0: x (a word)
 * @param y      d1: y (a word)
 * @param ch     d3: the channel, -1 for any
 * @param shape  a0: the shape (!libs #shapeslib,#ia0|#pd2)
 * @return d0: the channel 0-7, -1 if no sprite was got
 */
static ULONG BLITZ_ROUTINE getwbsprite_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1),
                                         LONG ch BL_REG(d3), ULONG shape BL_REG(a0))
{
    LONG got = -1;

    if (!wd.graphicslibbase || !SHAPEDATA(shape))
        return (ULONG)-1;
    wd.shapeaddr = shape;
    wd.sprx = x;
    wd.spry = y;
    wd.sprchannel = ch;
    if (shapebitmap() <= 0)
        return (ULONG)-1;
    if (newsprite()) {
        wd.getspritetaglist[0].ti_Data = (ULONG)wd.sprchannel;    /* GSTAG_SPRITE_NUM */
        got = GetExtSpriteA((struct ExtSprite *)wd.temppointer,
                            wd.getspritetaglist);
        if (got == -1) {
            FreeSpriteData((struct ExtSprite *)wd.temppointer);
        } else {
            if (wd.sprchannel == -1)
                wd.sprchannel = got;
            *ENTRYW(wd.sprchannel) = wd.temppointer;
        }
    }
    FreeBitMap((struct BitMap *)wd.mybitmap);           /* 'freemybitmap */
    if (got == -1)
        return (ULONG)-1;
    return (ULONG)wd.sprchannel;
}

/**
 * @brief FreeWBSprite sprite_channel: frees a sprite (FreeSprite,
 * FreeSpriteData) and clears its table entry.
 *
 * Amiga mode only.  The table entry of channel & 15: entries 8-15 are
 * the library name and base, and FreeSprite gets the whole register (a
 * channel outside 0-7 is refused by freesprchk, fixed #916).
 * @param ch  d0.b: the channel
 * @return d0: 0 without a sprite (failed: MOVEQ #0), else as
 *         FreeSpriteData leaves it (a statement: the value is not used)
 */
static ULONG BLITZ_ROUTINE freesprite_c(ULONG ch BL_REG(d0))
{
    ULONG *e = (ULONG *)((UBYTE *)wd.spritepointers + ((ch & 15) << 2));
    ULONG spr;

    if (!*e)
        return 0;
    wd.temppointer = (ULONG)e;
    FreeSprite((WORD)ch);                               /* the whole d0 (freesprchk, #916) */
    spr = *e;
    *e = 0;
    return freespritedata(spr);
}

/**
 * @brief MoveWBSprite(viewport,x,y,sprite_channel): MoveSprite of the
 * channel's sprite.
 *
 * Amiga mode only; nothing without a sprite.  The channel:
 * movewbspritechk (fixed #916).  Fixed (#921): the result is -1 when
 * the sprite was moved (the original returned the d0 MoveSprite leaves, declared
 * as a byte).
 * @param vp  d0: the ViewPort
 * @param x   d1: x
 * @param y   d2: y
 * @param ch  d3: the channel
 * @return d0: -1 moved, 0 without a sprite
 */
static ULONG BLITZ_ROUTINE movewbsprite_c(ULONG vp BL_REG(d0), ULONG x BL_REG(d1),
                                          ULONG y BL_REG(d2), ULONG ch BL_REG(d3))
{
    ULONG spr = *ENTRYW(ch);

    if (!spr)
        return 0;
    movesprite(vp, spr, x, y);
    return (ULONG)-1;
}

/**
 * @brief ChangeWBSprite(viewport,sprite_channel,shape#): a new image for
 * a sprite (ChangeExtSpriteA).
 *
 * Amiga mode only.  The shape is converted as in GetWBSprite, at the
 * sprite's position; on success the old sprite data are freed and the
 * new ones replace them in the table.  The channel and the shape:
 * changewbspritechk (fixed #916, #917).  Fixed (#915): if ChangeExtSpriteA fails, the new
 * sprite data are freed (the original lost them).
 * @param vp     d0: the ViewPort
 * @param ch     d1: the channel
 * @param shape  a0: the shape (!libs #shapeslib,#ia0|#pd2)
 * @return d0: BB_TRUE, BB_FALSE without graphics.library, shape data or
 *         sprite, or if a step fails
 */
static BB_BOOL BLITZ_ROUTINE changewbsprite_c(ULONG vp BL_REG(d0), ULONG ch BL_REG(d1),
                                              ULONG shape BL_REG(a0))
{
    ULONG *e;
    LONG r;

    if (!wd.graphicslibbase || !SHAPEDATA(shape))
        return BB_FALSE;
    wd.myviewport = vp;
    wd.sprchannel = (LONG)ch;
    e = (ULONG *)((UBYTE *)wd.spritepointers + (LONG)ASLW2(ch));
    if (!*e)
        return BB_FALSE;
    wd.oldpointer = (ULONG)e;
    wd.sprx = ((struct ExtSprite *)*e)->es_SimpleSprite.x;
    wd.spry = ((struct ExtSprite *)*e)->es_SimpleSprite.y;
    wd.shapeaddr = shape;
    if (shapebitmap() <= 0)
        return BB_FALSE;
    r = 0;
    if (newsprite()) {
        r = changeextsprite(wd.myviewport, *(ULONG *)wd.oldpointer, wd.temppointer,
                            (ULONG)wd.dudtaglist);
        if (r) {
            FreeSpriteData((struct ExtSprite *)*(ULONG *)wd.oldpointer);
            *(ULONG *)wd.oldpointer = wd.temppointer;
            r = -1;
        } else {
            FreeSpriteData((struct ExtSprite *)wd.temppointer);
        }
    }
    FreeBitMap((struct BitMap *)wd.mybitmap);
    return (BB_BOOL)r;                  /* 0 or -1 */
}

/**
 * @brief WBSpriteX(sprite_channel): the x of the channel's sprite.
 *
 * Amiga mode only.  BUG(orig): without a sprite the word at 6 is read
 * (address 6, #922).  The channel: wbspritexychk (fixed #916).
 * @param ch  d0.w: the channel
 * @return d0: the low word replaced by es_SimpleSprite.x
 */
static ULONG BLITZ_ROUTINE wbspritex_c(ULONG ch BL_REG(d0))
{
    ULONG spr = *ENTRYW(ch);

    return (ch & 0xffff0000UL) | ((struct ExtSprite *)spr)->es_SimpleSprite.x;
}

/**
 * @brief WBSpriteY(sprite_channel): the y of the channel's sprite.
 *
 * As WBSpriteX, the word at 8 (#922; the channel: fixed #916).
 * @param ch  d0.w: the channel
 * @return d0: the low word replaced by es_SimpleSprite.y
 */
static ULONG BLITZ_ROUTINE wbspritey_c(ULONG ch BL_REG(d0))
{
    ULONG spr = *ENTRYW(ch);

    return (ch & 0xffff0000UL) | ((struct ExtSprite *)spr)->es_SimpleSprite.y;
}

/* Registers: the original changes d0-d7/a0-a2/a6 (keeps a3-a5); the
   thunks keep d1/a0/a1, the C code d2-d7/a2-a6. */
BLITZ_THUNK(riwbspritelib_libinit, libinit_c, "d1/a0-a1");
BLITZ_THUNK(riwbspritelib_libfinit, libfinit_c, "d1/a0-a1");
BLITZ_THUNK(_GetWBSprite, getwbsprite_c, "d1/a0-a1");
BLITZ_THUNK(_freesprite, freesprite_c, "d1/a0-a1");
BLITZ_THUNK(_movewbsprite, movewbsprite_c, "d1/a0-a1");
BLITZ_THUNK(_changewbsprite, changewbsprite_c, "d1/a0-a1");
BLITZ_THUNK(_wbspritex, wbspritex_c, "d1/a0-a1");
BLITZ_THUNK(_wbspritey, wbspritey_c, "d1/a0-a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(badgetch, "Sprite channel must be -1 to 7");    /* fixed, #916 */
BLITZ_ERRCHK_MESSAGE(badch, "Sprite channel must be 0 to 7");        /* fixed, #916 */
BLITZ_ERRCHK_MESSAGE(badheight, "Illegal Shape Height");             /* fixed, #917 */
BLITZ_ERRCHK_MESSAGE(baddepth, "Illegal Shape Depth");               /* fixed, #917 */

/* a channel 0 to 7 */
static void BLITZ_ERRCHK chchk(ULONG ch)
{
    if (ch > 7)
        BLITZ_RUNERR(badch);
}

/* a shape with data: height not 0, depth 1 to 8 (one without data the
   release code refuses itself) */
static void BLITZ_ERRCHK shapechk(const struct bb_shape *sh)
{
    if (!SHAPEDATA(sh))
        return;
    if (!sh->height)
        BLITZ_RUNERR(badheight);
    if (!sh->depth || sh->depth > 8)
        BLITZ_RUNERR(baddepth);
}

/**
 * @brief The error check of GetWBSprite (debugger only).
 *
 * Fixed (#916), not in the original (riwbspritelib had no error check
 * area): the channel indexes the sprite table of 8 entries without a
 * check (the tag lists before it, the library name and base behind it).
 * The release code is unchanged (an invalid argument, D-005).
 *
 * Fixed (#917): a shape with data and a height of 0, a depth of 0 or
 * above 8 (an invalid object: Blitz's shape commands make none) is
 * refused before AllocBitMap; the copy counts height - 1 and depth - 1
 * with DBRA (65536 rows or planes; the planes read behind the 8 of the
 * bitmap).  Release code unchanged (D-005).
 *
 * Errors: "Sprite channel must be -1 to 7"; "Illegal Shape Height",
 * "Illegal Shape Depth".
 * @param ch     d3: the channel (a long), -1 for any
 * @param shape  a0: the shape
 */
static void BLITZ_ERRCHK getwbspritechk_c(LONG ch BL_REG(d3),
                                          const struct bb_shape *shape BL_REG(a0))
{
    if (ch < -1 || ch > 7)
        BLITZ_RUNERR(badgetch);
    shapechk(shape);
}

/**
 * @brief The error check of FreeWBSprite (debugger only).
 *
 * Fixed (#916), not in the original: the release code takes the channel
 * modulo 16 for the table (entries 8-15 are the library name and base)
 * and passes the whole register to FreeSprite.  Release code unchanged
 * (D-005).
 *
 * Errors: "Sprite channel must be 0 to 7".
 * @param ch  d0.b: the channel
 */
static void BLITZ_ERRCHK freesprchk_c(ULONG ch BL_REG(d0))
{
    chchk((UBYTE)ch);
}

/**
 * @brief The error check of MoveWBSprite (debugger only).
 *
 * Fixed (#916), not in the original: the channel (a long) indexes the
 * table without a check.  Release code unchanged (D-005).
 *
 * Errors: "Sprite channel must be 0 to 7".
 * @param ch  d3: the channel
 */
static void BLITZ_ERRCHK movewbspritechk_c(ULONG ch BL_REG(d3))
{
    chchk(ch);
}

/**
 * @brief The error check of ChangeWBSprite (debugger only).
 *
 * Fixed (#916), not in the original: the channel (a long) indexes the
 * table without a check.  Release code unchanged (D-005).
 *
 * Fixed (#917): the shape as GetWBSprite's.
 *
 * Errors: "Sprite channel must be 0 to 7"; "Illegal Shape Height",
 * "Illegal Shape Depth".
 * @param ch     d1: the channel
 * @param shape  a0: the shape
 */
static void BLITZ_ERRCHK changewbspritechk_c(ULONG ch BL_REG(d1),
                                             const struct bb_shape *shape BL_REG(a0))
{
    chchk(ch);
    shapechk(shape);
}

/**
 * @brief The error check of WBSpriteX and WBSpriteY (debugger only).
 *
 * Fixed (#916), not in the original: the channel (a word) indexes the
 * table without a check.  Release code unchanged (D-005).
 *
 * Errors: "Sprite channel must be 0 to 7".
 * @param ch  d0.w: the channel
 */
static void BLITZ_ERRCHK wbspritexychk_c(ULONG ch BL_REG(d0))
{
    chchk((UWORD)ch);
}

/* the thunks of the check functions */
BLITZ_ERRCHK_THUNK(_getwbspritechk, getwbspritechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_freesprchk, freesprchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_movewbspritechk, movewbspritechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_changewbspritechk, changewbspritechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(_wbspritexychk, wbspritexychk_c, "d0-d1/a0-a1");
