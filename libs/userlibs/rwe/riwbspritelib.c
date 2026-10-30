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
 * in the original's order (struct wbs): the routines index the sprite
 * table without bounds and so read the tag list before it and the name
 * behind it (BUG(orig)).  A shape is copied into a bitmap of
 * AllocBitMap, which AllocSpriteDataA converts.  Assembler besides the
 * thunks: the calls of MoveSprite and FreeSpriteData (the original
 * returns the d0 they leave) and ChangeExtSpriteA (its tag list in a3).
 *
 * BUG(orig):
 *   - No routine checks the channel: GetWBSprite stores at
 *     spritepointers + channel*4 (as a word index); FreeWBSprite takes
 *     the channel modulo 16 for the table (entries 8-15 are the name and
 *     the base) but passes the whole register to FreeSprite; MoveWBSprite,
 *     ChangeWBSprite, WBSpriteX/Y index the table with the channel as is
 *     (WBSpriteX/Y without a sprite read the words at 6 and 8).
 *   - GetWBSprite: if GetExtSpriteA fails (channel in use), the result
 *     is still the channel when the table has a sprite there (an earlier
 *     one of the library), else 0 (for channel -1 the table's entry -1 is
 *     the end of the tag list before it: 0).  A shape without data gives
 *     -1, a narrow shape (< 16) or no graphics.library 0, a failing
 *     AllocBitMap -1, a failing AllocSpriteDataA 0 or the channel as
 *     above.
 *   - The shape is copied without the bitmap's BytesPerRow, width/16
 *     words per row (the shape's ebwidth is not used either), and with
 *     DBRA counts: height or depth 0 copy 65536 rows/planes.
 *   - ChangeWBSprite: if ChangeExtSpriteA fails, the new sprite data is
 *     not freed.
 *   - MoveWBSprite returns d0 as MoveSprite leaves it (declared as a
 *     byte result).
 *   - finit does not clear the library base: a second finit closes
 *     graphics.library again.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <graphics/gfx.h>
#include <graphics/sprite.h>
#include <graphics/view.h>

/* entry points (thunks, below) */
void _GetWBSprite(void), _freesprite(void), _movewbsprite(void), _changewbsprite(void);
void _wbspritex(void), _wbspritey(void), _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

#define IA(a, p) (BL_LA(a) | BL_ITEM | BL_PD(p))   /* #ia<a>|#pd<p> */

/* !libheader {10,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_riwbspritelib, init, BL_NORET, finit, 0);

/* `#ia0|pd2` in the source: pd2 is 2, the shape of parameter 2 */
BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_shapeslib, IA(0, 2));
BLITZ_SUBS((char *)_GetWBSprite + 1, 0, 0);     /* +1: Amiga mode only */
BLITZ_NAME("GetWBSprite", "x,y,shape[,spritechannel]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS((char *)_freesprite + 1, 0, 0);
BLITZ_NAME("FreeWBSprite", "sprite_channel");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS((char *)_movewbsprite + 1, 0, 0);
BLITZ_NAME("MoveWBSprite", "viewport,x,y,sprite_channel");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_WORD);
BLITZ_LIBS(LIB_shapeslib, IA(0, 2));
BLITZ_SUBS((char *)_changewbsprite + 1, 0, 0);
BLITZ_NAME("ChangeWBSprite", "viewport,sprite_channel,shape#");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS((char *)_wbspritex + 1, 0, 0);
BLITZ_NAME("WBSpriteX", "sprite_channel");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS((char *)_wbspritey + 1, 0, 0);
BLITZ_NAME("WBSpriteY", "sprite_channel");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------- the data */

/* the original's data block, in its order */
static struct BL_PACKED wbs {
    ULONG myviewport, oldpointer, mybitmap, shapeaddr;
    LONG sprchannel;
    UWORD sprx, spry;
    ULONG temppointer;
    UWORD sprwidth, sprheight;
    ULONG allocspritetaglist[12];
    ULONG getspritetaglist[6];
    ULONG dudtaglist[2];
    ULONG spritepointers[8];
    char graphicslibrary[18];
    struct Library *graphicslibbase;
} wd = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    { 0x81000000UL, 16,                 /* SPRITEA_Width */
      0x81000006UL, 16,                 /* SPRITEA_OutputHeight */
      0x81000002UL, 0,                  /* SPRITEA_XReplication */
      0x81000004UL, 0,                  /* SPRITEA_YReplication */
      0x81000008UL, 0,                  /* SPRITEA_Attached */
      0, 0 },
    { 0x82000020UL, 1,                  /* GSTAG_SPRITE_NUM */
      0x82000022UL, 0,                  /* GSTAG_ATTACHED */
      0x82000024UL, 0 },                /* GSTAG_SOFTSPRITE (no TAG_DONE: */
    { 0, 0 },                           /* dudtaglist ends it) */
    { 0 },
    "graphics.library",
    NULL,
};

#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME wd.graphicslibbase
#include <proto/graphics.h>
#undef __NOLIBBASE__

#define SW(p, off) (*(UWORD *)((UBYTE *)(p) + (off)))
#define SL(p, off) (*(ULONG *)((UBYTE *)(p) + (off)))
/* ASL.w #2,d: the low word shifted, the upper word kept */
#define ASLW2(d) (((ULONG)(d) & 0xffff0000UL) | (UWORD)((ULONG)(d) << 2))
/* the sprite table entry of a channel indexed as a word, d(a0,d1.w) */
#define ENTRYW(ch) ((ULONG *)((UBYTE *)wd.spritepointers + (WORD)((UWORD)(ch) << 2)))

/* MoveSprite(vp, sprite, x, y) with the registers of the original; the
   d0 graphics.library leaves */
static ULONG movesprite(ULONG vp, ULONG spr, ULONG x, ULONG y)
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
    return d0;
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

/* ChangeExtSpriteA(vp, old, new, tags): tags in a3 (fixed in the C code:
   saved around the call) */
static LONG changeextsprite(ULONG vp, ULONG old, ULONG new, ULONG tags)
{
    register ULONG d0 __asm("d0");
    register ULONG a0 __asm("a0") = vp;
    register ULONG a1 __asm("a1") = old;
    register ULONG a2 __asm("a2") = new;
    register struct Library *a6 __asm("a6") = wd.graphicslibbase;

    __asm volatile("move.l a3,-(sp)\n\t"
                   "move.l %5,a3\n\t"
                   "jsr -1026(a6)\n\t"
                   "move.l (sp)+,a3"
        : "=d"(d0), "+a"(a0), "+a"(a1), "+a"(a2), "+a"(a6)
        : "d"(tags)
        : "d1", "cc", "memory");
    return (LONG)d0;
}

/* init: graphics.library, any version */
static void BLITZ_ROUTINE libinit_c(void)
{
    wd.graphicslibbase = OpenLibrary((CONST_STRPTR)wd.graphicslibrary, 0);
}

/* finit: every sprite freed, the library closed (the base kept,
   BUG(orig)) */
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
   row, height rows, depth planes (DBRA counts) */
static void copyshape(struct BitMap *bm)
{
    const UBYTE *shape = (const UBYTE *)wd.shapeaddr;
    UWORD wcount = (UWORD)(((WORD)SW(shape, 0) >> 4) - 1);
    UWORD hcount = (UWORD)(SW(shape, 2) - 1);
    UWORD pcount = (UWORD)(SW(shape, 4) - 1);
    const UWORD *src = (const UWORD *)SL(shape, 14);
    PLANEPTR *planes = bm->Planes;

    do {
        UWORD *dst = (UWORD *)*planes++;
        UWORD h = hcount;

        do {
            UWORD w = wcount;

            do
                *dst++ = *src++;
            while (w-- != 0);
            __asm__("" : "+a"(src));    /* no __mulsi3 */
        } while (h-- != 0);
    } while (pcount-- != 0);
}

/* the shape (wd.shapeaddr) into a new bitmap (wd.mybitmap) and the
   tag list: 1, 0 if narrower than 16, -1 if AllocBitMap fails */
static int shapebitmap(void)
{
    const UBYTE *shape = (const UBYTE *)wd.shapeaddr;
    UWORD w = SW(shape, 0) & 0xfff0;
    struct BitMap *bm;

    if (!w)
        return 0;
    wd.sprwidth = w;
    wd.sprheight = SW(shape, 2);
    bm = AllocBitMap(w, wd.sprheight, SW(shape, 4), 0, NULL);
    if (!bm)
        return -1;
    wd.mybitmap = (ULONG)bm;
    copyshape(bm);
    SW(wd.allocspritetaglist, 6) = wd.sprwidth;
    SW(wd.allocspritetaglist, 14) = wd.sprheight;
    return 1;
}

/* the new sprite data, x/y from sprx/spry: 0 if AllocSpriteDataA fails */
static ULONG newsprite(void)
{
    ULONG spr = (ULONG)AllocSpriteDataA((struct BitMap *)wd.mybitmap,
                                        (struct TagItem *)wd.allocspritetaglist);

    if (!spr)
        return 0;
    wd.temppointer = spr;
    SW(spr, 6) = wd.sprx;
    SW(spr, 8) = wd.spry;
    return spr;
}

/* GetWBSprite(x,y,shape,channel) (d0, d1, d3; a0 = the shape) */
static ULONG BLITZ_ROUTINE getwbsprite_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1),
                                         LONG ch BL_REG(d3), ULONG shape BL_REG(a0))
{
    int r;

    if (!wd.graphicslibbase)
        return 0;
    if (!SL(shape, 14))
        return (ULONG)-1;
    wd.shapeaddr = shape;
    wd.sprx = x;
    wd.spry = y;
    wd.sprchannel = ch;
    r = shapebitmap();
    if (r <= 0)
        return r ? (ULONG)-1 : 0;
    if (newsprite()) {
        LONG got;

        wd.getspritetaglist[1] = (ULONG)wd.sprchannel;
        got = GetExtSpriteA((struct ExtSprite *)wd.temppointer,
                            (struct TagItem *)wd.getspritetaglist);
        if (got == -1) {
            FreeSpriteData((struct ExtSprite *)wd.temppointer);
        } else {
            if (wd.sprchannel == -1)
                wd.sprchannel = got;
            *ENTRYW(wd.sprchannel) = wd.temppointer;
        }
    }
    FreeBitMap((struct BitMap *)wd.mybitmap);           /* 'freemybitmap */
    if (!*ENTRYW(wd.sprchannel))
        return 0;
    return (ULONG)wd.sprchannel;
}

/* FreeWBSprite channel (d0.b): the table entry of channel & 15; d0 = 0
   without a sprite (failed: MOVEQ #0), else as FreeSpriteData leaves it
   (a statement: the value is not used) */
static ULONG BLITZ_ROUTINE freesprite_c(ULONG d0 BL_REG(d0))
{
    ULONG *e = (ULONG *)((UBYTE *)wd.spritepointers + ((d0 & 15) << 2));
    ULONG spr;

    if (!*e)
        return 0;
    wd.temppointer = (ULONG)e;
    FreeSprite((WORD)d0);                               /* the whole d0: BUG(orig) */
    spr = *e;
    *e = 0;
    return freespritedata(spr);
}

/* MoveWBSprite(viewport,x,y,channel) (d0-d3) */
static ULONG BLITZ_ROUTINE movewbsprite_c(ULONG vp BL_REG(d0), ULONG x BL_REG(d1),
                                          ULONG y BL_REG(d2), ULONG ch BL_REG(d3))
{
    ULONG spr = *ENTRYW(ch);

    if (!spr)
        return 0;
    return movesprite(vp, spr, x, y);
}

/* ChangeWBSprite(viewport,channel,shape) (d0, d1; a0 = the shape) */
static ULONG BLITZ_ROUTINE changewbsprite_c(ULONG vp BL_REG(d0), ULONG ch BL_REG(d1),
                                            ULONG shape BL_REG(a0))
{
    ULONG *e;
    LONG r;

    if (!wd.graphicslibbase || !SL(shape, 14))
        return 0;
    wd.myviewport = vp;
    wd.sprchannel = (LONG)ch;
    e = (ULONG *)((UBYTE *)wd.spritepointers + (LONG)ASLW2(ch));
    if (!*e)
        return 0;
    wd.oldpointer = (ULONG)e;
    wd.sprx = SW(*e, 6);
    wd.spry = SW(*e, 8);
    wd.shapeaddr = shape;
    if (shapebitmap() <= 0)
        return 0;
    r = 0;
    if (newsprite()) {
        r = changeextsprite(wd.myviewport, *(ULONG *)wd.oldpointer, wd.temppointer,
                            (ULONG)wd.dudtaglist);
        if (r) {
            FreeSpriteData((struct ExtSprite *)*(ULONG *)wd.oldpointer);
            *(ULONG *)wd.oldpointer = wd.temppointer;
            r = -1;
        }
    }
    FreeBitMap((struct BitMap *)wd.mybitmap);
    return (ULONG)r;
}

/* WBSpriteX/Y(channel) (d0.w): the low word of d0 replaced */
static ULONG BLITZ_ROUTINE wbspritex_c(ULONG d0 BL_REG(d0))
{
    ULONG spr = *ENTRYW(d0);

    return (d0 & 0xffff0000UL) | SW(spr, 6);
}

static ULONG BLITZ_ROUTINE wbspritey_c(ULONG d0 BL_REG(d0))
{
    ULONG spr = *ENTRYW(d0);

    return (d0 & 0xffff0000UL) | SW(spr, 8);
}

/* Registers: the original changes d0-d7/a0-a2/a6 (keeps a3-a5); the
   thunks keep d1/a0/a1, the C code d2-d7/a2-a6. */
BLITZ_THUNK(_libinit, libinit_c, "d1/a0-a1");
BLITZ_THUNK(_libfinit, libfinit_c, "d1/a0-a1");
BLITZ_THUNK(_GetWBSprite, getwbsprite_c, "d1/a0-a1");
BLITZ_THUNK(_freesprite, freesprite_c, "d1/a0-a1");
BLITZ_THUNK(_movewbsprite, movewbsprite_c, "d1/a0-a1");
BLITZ_THUNK(_changewbsprite, changewbsprite_c, "d1/a0-a1");
BLITZ_THUNK(_wbspritex, wbspritex_c, "d1/a0-a1");
BLITZ_THUNK(_wbspritey, wbspritey_c, "d1/a0-a1");
