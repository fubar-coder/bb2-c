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

static void BLITZ_ROUTINE killinvader_c(UBYTE *sprad BL_REG(d0), UWORD x BL_REG(d1),
                                        UWORD y BL_REG(d2), UWORD offset BL_REG(d3))
{
    UBYTE *img, *img48, *imgoff, *img48off;     /* a0-a3; img first the entry */
    UWORD rowoff;                               /* d1 */
    int i;

    img = sprad + (WORD)(UWORD)((x >> 2) << 4);
    img48 = *(UBYTE **)(img + 3 * 16);
    img = *(UBYTE **)img;
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

static void BLITZ_ROUTINE drawstarfield_c(UWORD *table BL_REG(d0), UBYTE *plane BL_REG(d1))
{
    WORD speed;

    while ((speed = (WORD)*table++) >= 0) {
        UBYTE bit = 1 << (*table++ & 7);
        UWORD o = *table;
        plane[(WORD)o] &= ~bit;
        o += speed;
        if (!((WORD)o < PLAYFIELD_Y_END * 40))
            o -= PLAYFIELD_HEIGHT * 40;
        *table++ = o;
        plane[(WORD)o] |= bit;
    }
}

/* the bullet: 3 rows, planes 0-3 (`set`: plane 2 set = colour 4) */
static void bullet(UBYTE *bitm, UWORD x, UWORD y, int set)
{
    UBYTE **p = (UBYTE **)(bitm + 8);
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

static void BLITZ_ROUTINE drawbullet_c(UBYTE *bitm BL_REG(d0), UWORD x BL_REG(d1),
                                       UWORD y BL_REG(d2))
{
    bullet(bitm, x, y, 1);
}

static void BLITZ_ROUTINE erasebullet_c(UBYTE *bitm BL_REG(d0), UWORD x BL_REG(d1),
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
