/*
 * rwe bb_blitlib - the C build of bb_blitlib (userlib 19) against the
 * reference build of rwelibsrc/blitzbomberslib/BB_BlitLib4.bb (blitz2
 * 2.10), method of rwecmp.h.
 *
 *   bb_blitlib DIR     (DIR with bb_blitlib .obj/.orig)
 *
 * Each set has a chip memory block with the same contents at the same
 * offsets: three bitmaps (6 planes of 320x64 and 384x64, the second
 * 320-wide one the source of SpecialBlockBitmap), the data and cookies
 * of the shapes, 16x16 images and the memory of a buffer (bblitlib's
 * Buffer object); the shape, bitmap and buffer objects are built by the
 * test.  Every call runs with the blitter owned (OwnBlitter) and is
 * waited for (WaitBlit); then the bitmaps, shape data and images of both
 * sets are compared (all bytes), the buffer records (the pointers in them
 * relative to the chip block), the created shape objects, d0 of
 * PredatorBlit and the kept registers.
 * Covered: all 12 routines, both forms of SpecialBlock and
 * SpecialBlockBitmap, SetupMapTwo with 48 (and back to 40), shifts 0..15,
 * shapes with handles, colours with bits above the depth and above bit 7,
 * CreateShape with the like shape among the created ones, the two error
 * checks with good shapes.
 * Not covered: the error checks' runtime errors (TRAP #0), depth 0 and
 * count 0 (65536 loops), blits outside the bitmaps.
 */
#include "rwecmp.h"
#include <exec/memory.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

#define LIB "bb_blitlib"

enum { SBLOCK, SBLOCKBM, TBLOCK, PREDATOR, BPREDATOR, SETUP, CREATE, SBTOSHAPE, BTOSHAPE,
       BCOLOUR, TBIMAGE, BTBIMAGE };

/* the chip block */
#define BMA 0x0000                      /* 6 planes 40x64 */
#define BMB 0x3c00                      /* 6 planes 40x64 */
#define BMC 0x7800                      /* 6 planes 48x64 */
#define SDATA 0xc000                    /* shape data, cookies */
#define IMAGES 0xe000                   /* 4 images of 224 bytes */
#define CMPSZ 0xe400                    /* compared as bytes */
#define BUFMEM 0xe400
#define BUFLEN 0x3000
#define CHIPSZ (BUFMEM + BUFLEN)

#define NSH 20
enum { S16TOP, S16BOT, SBIG, SHANDLE, SDEST, SDEST2, SCREATE = 10 };

static struct Library *gfx;
static UBYTE *chip[2];
static UBYTE shp[2][NSH][32];
static UBYTE bm[2][3][8 + 6 * 4];       /* A, B, C */
static ULONG buf[2][4];                 /* pos, last, mem, len */

#define SW(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define SL(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))   /* word, marker above */

static ULONG pick(ULONG a, ULONG b)
{
    return a + rnd() % (b - a + 1);
}

/* DO2 with the blitter owned and the blit waited for */
#define DO2B(fn, setup) do {                                                \
        for (cur = 0; cur < 2; cur++) {                                     \
            APTR fn_;                                                       \
            markregs(&R[cur]);                                              \
            NARGS = 0;                                                      \
            setup;                                                          \
            IN[cur] = R[cur];                                               \
            fn_ = (fn);                                                     \
            if (!fn_) {                                                     \
                check("entry missing", FALSE);                              \
                continue;                                                   \
            }                                                               \
            OwnBlitter();                                                   \
            CCR[cur] = callstk(fn_, &R[cur], ARGS[cur], NARGS, &POP[cur]);  \
            WaitBlit();                                                     \
            DisownBlitter();                                                \
        }                                                                   \
        preserve();                                                         \
    } while (0)

/* ------------------------------------------------------------ objects */

static void fillchip(void)
{
    ULONG i;
    for (i = 0; i < CHIPSZ; i++)
        chip[0][i] = (UBYTE)rnd();
    memcpy(chip[1], chip[0], CHIPSZ);
}

static void mkbitmap(int b, ULONG at, UWORD width)
{
    int s, p;
    for (s = 0; s < 2; s++) {
        memset(bm[s][b], 0, sizeof bm[s][b]);
        SW(bm[s][b], 0) = width;
        SW(bm[s][b], 2) = 64;
        SW(bm[s][b], 4) = 6;
        for (p = 0; p < 6; p++)
            SL(bm[s][b], 8 + 4 * p) = (ULONG)chip[s] + at + (ULONG)p * width * 64;
    }
}

/* a shape: w x h x d, data and cookie at offsets of the chip block */
static void mkshape(int n, UWORD w, UWORD h, UWORD d, WORD hx, WORD hy, ULONG data,
                    ULONG cookie)
{
    int s;
    UWORD eb = (UWORD)(((w + 15) >> 4) << 1);
    for (s = 0; s < 2; s++) {
        UBYTE *p = shp[s][n];
        memset(p, 0, 32);
        SW(p, 0) = w;
        SW(p, 2) = h;
        SW(p, 4) = d;
        SW(p, 6) = eb;
        SW(p, 8) = (UWORD)((h << 6) | (eb / 2 + 1));
        SW(p, 10) = (UWORD)hx;
        SW(p, 12) = (UWORD)hy;
        SL(p, 14) = (ULONG)chip[s] + data;
        SL(p, 18) = cookie ? (ULONG)chip[s] + cookie : 0;
        SW(p, 22) = (UWORD)(eb * h);
        SW(p, 24) = (UWORD)((eb + 2) * h);
        SW(p, 26) = (UWORD)(eb * h * d);
    }
}

static void resetbuf(void)
{
    int s;
    for (s = 0; s < 2; s++) {
        buf[s][0] = buf[s][2] = (ULONG)chip[s] + BUFMEM;
        buf[s][1] = 0;
        buf[s][3] = BUFLEN;
    }
}

/* ------------------------------------------------------------ compare */

#define REL(v) ((v) ? (ULONG)(v) - (ULONG)chip[cur] : 0)

static void samechip(void)
{
    SAMEBYTES("chip memory", chip, CMPSZ);
}

/* the records of the buffer (newest first), pointers relative to the
   chip block */
static void samebuffer(void)
{
    UBYTE *r[2];
    int k = 0;
    SAME("buffer pos", REL(buf[cur][0]));
    SAME("buffer last", REL(buf[cur][1]));
    r[0] = (UBYTE *)buf[0][1];
    r[1] = (UBYTE *)buf[1][1];
    while (r[0] && r[1] && k++ < 100) {
        UWORD n, save, size, used, p;
        UBYTE *q[2];
        SAME("record link", REL(SL(r[cur], 0)));
        SAME("record mod", SW(r[cur], 4));
        SAME("record size", SW(r[cur], 6));
        SAME("record planes", SW(r[cur], 8));
        SAME("record save", SW(r[cur], 10));
        SAME("record offset", SL(r[cur], 12));
        n = SW(r[0], 8);
        save = SW(r[0], 10);
        /* the bytes the save blit wrote (BPredatorBlit: one line less
           than the slot; the rest is from earlier records) */
        size = SW(r[0], 6);
        used = (UWORD)((size >> 6) * ((size & 63) * 2));
        if (used > save)
            used = save;
        if (n > 7 || save > 400 || SW(r[1], 8) != n || SW(r[1], 10) != save)
            break;
        q[0] = r[0] + 16;
        q[1] = r[1] + 16;
        for (p = 0; p <= n; p++) {
            const UBYTE *d[2] = { q[0] + 4, q[1] + 4 };
            SAME("record plane", REL(SL(q[cur], 0)));
            SAMEBYTES("record data", d, used);
            q[0] += 4 + save;
            q[1] += 4 + save;
        }
        r[0] = (UBYTE *)SL(r[0], 0);
        r[1] = (UBYTE *)SL(r[1], 0);
    }
    SAME("records end", r[cur] == 0);
}

static void sameshapes(int from, int to)
{
    int i;
    char w[40];
    for (i = from; i <= to; i++) {
        const UBYTE *a[2] = { shp[0][i], shp[1][i] };
        const UBYTE *b[2] = { shp[0][i] + 22, shp[1][i] + 22 };
        sprintf(w, "shape %d fields", i);
        SAMEBYTES(w, a, 14);
        SAMEBYTES(w, b, 10);
        sprintf(w, "shape %d data", i);
        SAME(w, REL(SL(shp[cur][i], 14)));
        sprintf(w, "shape %d cookie", i);
        SAME(w, REL(SL(shp[cur][i], 18)));
    }
}

/* -------------------------------------------------------------- calls */

#define CB 0xdff000UL
#define BMP(b) ((ULONG)bm[cur][b])
#define SHP(n) ((ULONG)shp[cur][n])

static void specialblock(int b, UWORD x, UWORD y)
{
    ROUT = "SpecialBlock x,y";
    sprintf(VEC, "bitmap %d at %u,%u", b, x, y);
    DO2B(ENT(LIB, SBLOCK, 0), R[cur].d[0] = WA(S16TOP); R[cur].d[1] = WA(S16BOT);
         R[cur].d[2] = WA(x); R[cur].d[3] = WA(y); R[cur].a[0] = BMP(b);
         R[cur].a[1] = SHP(S16TOP); R[cur].a[2] = SHP(S16BOT));
    samechip();
}

static void specialblock2(int b, UWORD off)
{
    ROUT = "SpecialBlock offset";
    sprintf(VEC, "bitmap %d at %u", b, off);
    DO2B(ENT(LIB, SBLOCK, 1), R[cur].d[0] = WA(S16TOP); R[cur].d[1] = WA(S16BOT);
         R[cur].d[2] = WA(off); R[cur].a[0] = BMP(b); R[cur].a[1] = SHP(S16TOP);
         R[cur].a[2] = SHP(S16BOT));
    samechip();
}

static void sblockbm(int form, UWORD x, UWORD y)
{
    ROUT = form ? "SpecialBlockBitmap offset" : "SpecialBlockBitmap x,y";
    sprintf(VEC, "%u,%u", x, y);
    DO2B(ENT(LIB, SBLOCKBM, form), R[cur].d[0] = WA(SBIG); R[cur].d[1] = WA(1);
         R[cur].d[2] = WA(x); R[cur].d[3] = WA(y); R[cur].a[0] = BMP(0);
         R[cur].a[1] = SHP(SBIG); R[cur].a[2] = BMP(1));
    samechip();
}

static void tblock(int b, UWORD x, UWORD y)
{
    ROUT = "TBlock";
    sprintf(VEC, "bitmap %d at %u,%u", b, x, y);
    DO2B(ENT(LIB, TBLOCK, 0), R[cur].d[0] = WA(S16TOP); R[cur].d[1] = WA(x);
         R[cur].d[2] = WA(y); R[cur].a[0] = BMP(b); R[cur].a[1] = SHP(S16TOP));
    samechip();
}

static void predator(int b, UWORD x, UWORD y)
{
    ROUT = "PredatorBlit";
    sprintf(VEC, "bitmap %d at %u,%u", b, x, y);
    DO2B(ENT(LIB, PREDATOR, 0), R[cur].d[0] = WA(S16TOP); R[cur].d[1] = WA(x);
         R[cur].d[2] = WA(y); R[cur].a[0] = BMP(b); R[cur].a[1] = SHP(S16TOP));
    SAME("d0", REL(R[cur].d[0]));
    samechip();
}

static void bpredator(int b, UWORD x, UWORD y)
{
    ROUT = "BPredatorBlit";
    sprintf(VEC, "bitmap %d at %u,%u", b, x, y);
    DO2B(ENT(LIB, BPREDATOR, 0), R[cur].d[0] = WA(0); R[cur].d[1] = WA(SHANDLE);
         R[cur].d[2] = WA(x); R[cur].d[3] = WA(y); R[cur].a[0] = (ULONG)buf[cur];
         R[cur].a[1] = CB; R[cur].a[2] = BMP(b); R[cur].a[3] = SHP(SHANDLE));
    samechip();
    samebuffer();
}

static void bcolour(UWORD x, UWORD y, UWORD colour)
{
    ROUT = "BColourBlit";
    sprintf(VEC, "at %u,%u colour $%X", x, y, colour);
    DO2B(ENT(LIB, BCOLOUR, 0), R[cur].d[0] = WA(0); R[cur].d[1] = WA(SHANDLE);
         R[cur].d[2] = WA(x); R[cur].d[3] = WA(y); R[cur].d[4] = WA(colour);
         R[cur].a[0] = (ULONG)buf[cur]; R[cur].a[1] = CB; R[cur].a[2] = BMP(0);
         R[cur].a[3] = SHP(SHANDLE));
    samechip();
    samebuffer();
}

static void setupmap(UWORD w)
{
    ROUT = "SetupMapTwo";
    sprintf(VEC, "%u", w);
    DO2B(ENT(LIB, SETUP, 0), R[cur].d[0] = WA(w));
}

static void tbimage(UWORD image, UWORD x, UWORD y)
{
    ROUT = "TBlockImage";
    sprintf(VEC, "image %u at %u,%u", image, x, y);
    DO2B(ENT(LIB, TBIMAGE, 0), R[cur].d[0] = (ULONG)chip[cur] + IMAGES; R[cur].d[1] = WA(image);
         R[cur].d[2] = WA(x); R[cur].d[3] = WA(y); R[cur].a[0] = BMP(0));
    samechip();
}

static void btbimage(UWORD image, UWORD x, UWORD y)
{
    ROUT = "BTBlockImage";
    sprintf(VEC, "image %u at %u,%u", image, x, y);
    DO2B(ENT(LIB, BTBIMAGE, 0), R[cur].d[0] = WA(0); R[cur].d[1] = (ULONG)chip[cur] + IMAGES;
         R[cur].d[2] = WA(x); R[cur].d[3] = WA(y); R[cur].d[4] = WA(image);
         R[cur].a[0] = (ULONG)buf[cur]; R[cur].a[1] = CB; R[cur].a[2] = BMP(0));
    samechip();
    samebuffer();
}

static void createshape(int start, UWORD count, ULONG mem, int like)
{
    ROUT = "CreateShape";
    sprintf(VEC, "%d,%u,chip+$%lX,%d", start, count, mem, like);
    DO2B(ENT(LIB, CREATE, 0), R[cur].d[0] = WA(start); R[cur].d[1] = WA(count);
         R[cur].d[2] = (ULONG)chip[cur] + mem; R[cur].d[3] = WA(like);
         R[cur].a[2] = SHP(start); R[cur].a[3] = SHP(like));
    sameshapes(SCREATE, NSH - 1);
}

static void sbtoshape(int dest)
{
    ROUT = "SpecialBlockToShape";
    sprintf(VEC, "into %d", dest);
    DO2B(ERR(LIB, SBTOSHAPE, 0), R[cur].d[0] = WA(S16TOP); R[cur].d[1] = WA(S16BOT);
         R[cur].d[2] = WA(dest); R[cur].a[0] = SHP(dest); R[cur].a[1] = SHP(S16TOP);
         R[cur].a[2] = SHP(S16BOT));
    DO2B(ENT(LIB, SBTOSHAPE, 0), R[cur].d[0] = WA(S16TOP); R[cur].d[1] = WA(S16BOT);
         R[cur].d[2] = WA(dest); R[cur].a[0] = SHP(dest); R[cur].a[1] = SHP(S16TOP);
         R[cur].a[2] = SHP(S16BOT));
    samechip();
}

static void btoshape(int src, int dest)
{
    ROUT = "BlockToShape";
    sprintf(VEC, "%d into %d", src, dest);
    DO2B(ERR(LIB, BTOSHAPE, 0), R[cur].d[0] = WA(src); R[cur].d[1] = WA(dest);
         R[cur].a[1] = SHP(src); R[cur].a[2] = SHP(dest));
    DO2B(ENT(LIB, BTOSHAPE, 0), R[cur].d[0] = WA(src); R[cur].d[1] = WA(dest);
         R[cur].a[1] = SHP(src); R[cur].a[2] = SHP(dest));
    samechip();
}

/* a 16-pixel aligned x and a y for a 16x16 block on a 64-line bitmap */
#define X16(w) ((UWORD)(pick(0, (w) / 16 - 1) * 16))

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    int s, i;

    if (argc < 2) {
        printf("usage: bb_blitlib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    gfx = OpenLibrary((CONST_STRPTR)"graphics.library", 0);
    for (s = 0; s < 2; s++)
        chip[s] = AllocMem(CHIPSZ, MEMF_CHIP);
    if (!gfx || !chip[0] || !chip[1]) {
        check("graphics.library and chip memory", FALSE);
        return 20;
    }
    fillchip();
    mkbitmap(0, BMA, 40);
    mkbitmap(1, BMB, 40);
    mkbitmap(2, BMC, 48);
    /* 16x16x6 shapes: data 192 bytes, cookie 32 (+2 read by the shifted
       blits); a 32x20x5 one; a 32x12x4 one with handles (5,3) */
    mkshape(S16TOP, 16, 16, 6, 0, 0, SDATA, SDATA + 0x200);
    mkshape(S16BOT, 16, 16, 6, 0, 0, SDATA + 0x300, SDATA + 0x400);
    mkshape(SBIG, 32, 20, 5, 0, 0, SDATA + 0x500, SDATA + 0x700);
    mkshape(SHANDLE, 32, 12, 4, 5, 3, SDATA + 0x800, SDATA + 0x900);
    mkshape(SDEST, 16, 16, 6, 0, 0, SDATA + 0xa00, SDATA + 0xb00);
    mkshape(SDEST2, 16, 16, 6, 0, 0, SDATA + 0xc00, 0);
    resetbuf();

    for (i = 0; i < 12; i++) {
        specialblock(0, X16(320), (UWORD)pick(0, 48));
        specialblock2(0, (UWORD)(pick(0, 48) * 40 + pick(0, 19) * 2));
        sblockbm(0, X16(288), (UWORD)pick(0, 44));
        sblockbm(1, (UWORD)(pick(0, 44) * 40 + pick(0, 18) * 2), 0);
        tblock(0, X16(320), (UWORD)pick(0, 48));
        predator(0, (UWORD)pick(0, 272), (UWORD)pick(0, 47));
        tbimage((UWORD)pick(0, 3), X16(320), (UWORD)pick(0, 48));
    }
    for (i = 0; i < 16; i++) {
        bpredator(0, (UWORD)(pick(0, 272) + 5), (UWORD)(pick(0, 50) + 3));
        bcolour((UWORD)(pick(0, 272) + 5), (UWORD)(pick(0, 51) + 3),
                (UWORD)(i < 8 ? pick(0, 15) : rnd()));
        btbimage((UWORD)pick(0, 3), (UWORD)pick(0, 288), (UWORD)pick(0, 48));
        if (i % 5 == 4)
            resetbuf();
    }

    /* a bitmap of 48 bytes per line */
    setupmap(48);
    for (i = 0; i < 6; i++) {
        specialblock(2, X16(384), (UWORD)pick(0, 48));
        tblock(2, X16(384), (UWORD)pick(0, 48));
        predator(2, (UWORD)pick(0, 336), (UWORD)pick(0, 47));
        bpredator(2, (UWORD)(pick(0, 336) + 5), (UWORD)(pick(0, 50) + 3));
    }
    setupmap(40);
    specialblock(0, X16(320), (UWORD)pick(0, 48));

    /* shapes */
    for (i = 0; i < 4; i++) {
        sbtoshape(SDEST);
        btoshape(S16TOP, SDEST2);
        btoshape(SDEST, S16BOT);
        specialblock(0, X16(320), (UWORD)pick(0, 48));
    }
    for (s = 0; s < 2; s++)
        memset(shp[s][SCREATE], 0, (NSH - SCREATE) * 32);
    createshape(SCREATE, 3, 0x10000, SBIG);
    createshape(SCREATE + 2, 5, 0x20002, S16TOP);
    createshape(SCREATE, 4, 0x30000, SCREATE + 1);  /* the like shape among them */

    WaitBlit();
    for (s = 0; s < 2; s++)
        FreeMem(chip[s], CHIPSZ);
    CloseLibrary(gfx);
    return rwe_done(LIB);
}
