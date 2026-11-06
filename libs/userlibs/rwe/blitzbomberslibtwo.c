/*
 * blitzbomberslibtwo - RWE userlib 31 (Reflective Images): four drawing
 * routines of the space invaders game in Blitz Bombers 2.
 *
 * C translation of rwelibsrc/blitzbomberslib/blitzbombersTwo00.bb;
 * reference: the reference build of the source (no binary exists,
 * docs/rwelibs/README.md 3.3).  Notes:
 * docs/rwelibs/port/blitzbomberslibtwo.md.
 *
 *   KillInvader sprad,x,y,offset   clears 8 rows of an invader in four
 *       sprite images: the words at +0 and +8 of each 16-byte row, at
 *       image + (x&3)*2 + y*224 and the same + offset, the images being
 *       the longs at sprad + (x>>2)*16 and 48 bytes on
 *   DrawStarField table,bitplane   moves the stars of the table (words
 *       speed, bit, offset; speed < 0 ends it): clears the bit at the
 *       old offset, adds the speed (an offset from 236*40 on wraps back
 *       by 204*40) and sets the bit at the new one
 *   DrawBullet bitm,x,y / EraseBullet bitm,x,y   a 1x3 pixel bullet in
 *       planes 0-3 of a 320 pixel wide bitmap: colour 4 / colour 0
 *
 * No init or finit.  Plain memory accesses, no assembler besides the
 * thunks; offsets are words (x(an,dn.w)) as in the original.
 *
 * Original behaviour, not defects: nothing is checked (bitmap width 40
 * bytes and the 236-line playfield are the game's constants);
 * DrawStarField's bit numbers are taken modulo 8 (BCLR/BSET on memory
 * address a byte; the table gives the bit within the star's byte).
 */
#include "blitzlib.h"
#include <graphics/gfx.h>

/* entry points (thunks, below) */
void _KillInvader(void), _DrawStarField(void), _DrawBullet(void), _EraseBullet(void);

/* !libheader {#blitzbomberslibtwo,0,0,0,0} */
BLITZ_LIBHEADER(LIB_blitzbomberslibtwo, 0, BL_NORET, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_KillInvader, 0, 0);
BLITZ_NAME("KillInvader", "sprad,x,y,offset");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_DrawStarField, 0, 0);
BLITZ_NAME("DrawStarField", "table,bitplane");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_DrawBullet, 0, 0);
BLITZ_NAME("DrawBullet", "bitm,x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_EraseBullet, 0, 0);
BLITZ_NAME("EraseBullet", "bitm,x,y");

BLITZ_LIBFIN();

#define PLAYFIELD_Y_END 236             /* #playfield_y_start + _height */
#define PLAYFIELD_HEIGHT 204
#define INVADER_HEIGHT 8
#define INVADER_Y_DISP 14               /* #invader_height + _y_gap */

#define WORDAT(p) (*(UWORD *)(p))

/**
 * @brief KillInvader sprad,x,y,offset: clear an invader in four sprite
 * images.
 *
 * The images are the longs at sprad + (x>>2)*16 and 48 bytes further;
 * in each, the 8 rows of 16 bytes from (x&3)*2 + y*224, and the same
 * `offset` bytes behind, get the words at +0 and +8 cleared.  Word
 * arithmetic and word offsets as in the original; nothing is checked.
 * @param sprad   d0: the table of image pointers
 * @param x       d1.w: the invader's column
 * @param y       d2.w: the invader's row
 * @param offset  d3.w: the offset of the second pair of images
 */
static void BLITZ_ROUTINE killinvader_c(UBYTE *sprad BL_REG(d0), UWORD x BL_REG(d1),
                                        UWORD y BL_REG(d2), UWORD offset BL_REG(d3))
{
    UBYTE *entry;                               /* a0: the table entry */
    UBYTE *img, *img48, *imgoff, *img48off;     /* a0-a3 */
    UWORD rowoff;                               /* d1 */
    int i;

    entry = sprad + (WORD)(UWORD)((x >> 2) << 4);
    img48 = *(UBYTE **)(entry + 3 * 16);
    img = *(UBYTE **)entry;
    rowoff = (UWORD)((x & 3) << 1) + (UWORD)((ULONG)y * (INVADER_Y_DISP * 16));
    img += (WORD)rowoff;
    img48 += (WORD)rowoff;
    imgoff = img + (WORD)offset;
    img48off = img48 + (WORD)offset;
    for (i = 0; i < INVADER_HEIGHT; i++) {
        WORDAT(img) = 0;
        WORDAT(img + 8) = 0;
        img += 16;
        WORDAT(img48) = 0;
        WORDAT(img48 + 8) = 0;
        img48 += 16;
        WORDAT(imgoff) = 0;
        WORDAT(imgoff + 8) = 0;
        imgoff += 16;
        WORDAT(img48off) = 0;
        WORDAT(img48off + 8) = 0;
        img48off += 16;
    }
}

/* a star of DrawStarField's table; a negative speed (one word) ends it */
struct star {
    WORD speed;                         /* 0 the offset added per call */
    UWORD bit;                          /* 2 the bit (modulo 8) */
    UWORD offset;                       /* 4 the byte offset in the plane */
};
static_assert(sizeof(struct star) == 6);

/**
 * @brief DrawStarField table,bitplane: move the stars of the table one
 * step.
 *
 * Per star: the bit is cleared at the old offset, the speed added (an
 * offset from 236*40 on wraps back by 204*40), the new offset stored and
 * the bit set there.  The bit numbers are taken modulo 8 (BCLR/BSET on
 * memory address a byte; not a defect).
 * @param s      d0: the table (struct star, ended by a negative speed)
 * @param plane  d1: the bitplane (40 bytes per row)
 */
static void BLITZ_ROUTINE drawstarfield_c(struct star *s BL_REG(d0), UBYTE *plane BL_REG(d1))
{
    WORD speed;

    while ((speed = s->speed) >= 0) {
        UBYTE bit = 1 << (s->bit & 7);
        UWORD o = s->offset;
        plane[(WORD)o] &= ~bit;
        o += speed;
        if (!((WORD)o < PLAYFIELD_Y_END * 40))
            o -= PLAYFIELD_HEIGHT * 40;
        s++->offset = o;
        plane[(WORD)o] |= bit;
    }
}

/* the bullet: 3 rows, planes 0-3 of the bitmap object (a struct BitMap)
   (`set`: plane 2 set = colour 4) */
static void bullet(struct BitMap *bitm, UWORD x, UWORD y, int set)
{
    PLANEPTR *p = bitm->Planes;
    UWORD o = (UWORD)((ULONG)y * 40) + (x >> 3);
    UBYTE bit = 1 << (~x & 7);
    int i;

    for (i = 0; i < 3; i++) {
        p[0][(WORD)o] &= ~bit;
        p[1][(WORD)o] &= ~bit;
        if (set)
            p[2][(WORD)o] |= bit;
        else
            p[2][(WORD)o] &= ~bit;
        p[3][(WORD)o] &= ~bit;
        o += 40;
    }
}

/**
 * @brief DrawBullet bitm,x,y: draw a 1x3 pixel bullet in colour 4.
 *
 * Planes 0, 1 and 3 cleared, plane 2 set, at x,y and the two rows below,
 * in a bitmap 320 pixels (40 bytes) wide; nothing is checked.
 * @param bitm  d0: the bitmap (struct BitMap)
 * @param x     d1.w: the column
 * @param y     d2.w: the top row
 */
static void BLITZ_ROUTINE drawbullet_c(struct BitMap *bitm BL_REG(d0), UWORD x BL_REG(d1),
                                       UWORD y BL_REG(d2))
{
    bullet(bitm, x, y, 1);
}

/**
 * @brief EraseBullet bitm,x,y: erase a bullet (colour 0 in planes 0-3).
 *
 * As DrawBullet, with plane 2 cleared too.
 * @param bitm  d0: the bitmap (struct BitMap)
 * @param x     d1.w: the column
 * @param y     d2.w: the top row
 */
static void BLITZ_ROUTINE erasebullet_c(struct BitMap *bitm BL_REG(d0), UWORD x BL_REG(d1),
                                        UWORD y BL_REG(d2))
{
    bullet(bitm, x, y, 0);
}

/* Registers: the original changes d0-d3/a0-a3 (KillInvader), d0-d3/a0-a1
   (DrawStarField), d0-d2/a0-a3 (the bullets); the thunks keep d0-d1/a0-a1,
   the C code d2-d7/a2-a6. */
BLITZ_THUNK(_KillInvader, killinvader_c, "d0-d1/a0-a1");
BLITZ_THUNK(_DrawStarField, drawstarfield_c, "d0-d1/a0-a1");
BLITZ_THUNK(_DrawBullet, drawbullet_c, "d0-d1/a0-a1");
BLITZ_THUNK(_EraseBullet, erasebullet_c, "d0-d1/a0-a1");
