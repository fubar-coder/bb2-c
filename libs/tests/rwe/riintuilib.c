/*
 * rwe riintuilib - riintuilib (userlib 15): the C build or the
 * reference build of rwelibsrc/riintuilib2.bb (ref/riintuilib.obj, see
 * libs/userlibs/rwe/refbuild.cmake), one set as LIBSET says, observed as
 * cases (rwecmp.h, LIBS_ONE_SET; the expected data
 * libs/tests/expected/rwe_riintuilib.txt).
 *
 *   riintuilib DIR     (DIR with riintuilib, inputoutputlib .obj or .orig)
 *
 * The set has its inputoutputlib, a Rast object (128 bytes) and a
 * BitMap (320 x 16, 2 planes).  Observed: the Rast (every field but the
 * BitMap pointer, which must be the BitMap), both planes after
 * every output, the 128 bytes around the caller's a2 (IPColour's
 * BUG(orig) (#696) SetBPen there), the kept registers; the memory back at the
 * end.
 * Covered: init, IPBitMap, IPLocate (also with upper words and a
 * negative word), IPColour both forms with pens >= 0, a negative fgpen
 * (BUG(orig) (#696): SetBPen on the caller's a2) and negative bgpen, IPOutput
 * and all three entries of its handler through inputoutputlib's
 * dooutsingle/dooutstring1/dooutstring2: a character (BUG(orig) (#699): read
 * from the address d0 - a string's address and the low memory address
 * 65), a0/d0 strings (also length 0), a null terminated string, with
 * the pens of IPColour; finit.
 * Not covered: a0/d0 strings over 1024 characters (BUG(orig) (#698): they
 * overwrite the pens; the text would be drawn far beyond the BitMap,
 * which a RastPort without a layer does not clip).
 */
#define LIBS_ONE_SET
#include "rwecmp.h"
#include <exec/memory.h>
#include <proto/dos.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
static struct Library *ipgfx;
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME ipgfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

#define LIB "riintuilib"
#define IO "inputoutputlib"
enum { E_BITMAP, E_LOCATE, E_COLOUR, E_OUTPUT };
enum { IO_OUTSINGLE = 6, IO_OUTSTRING1 = 7, IO_OUTSTRING2 = 8 };
#define BMW 320
#define BMH 16

static UBYTE rast[128];
static struct BitMap *bm;
static UBYTE a2area[128];               /* the caller's a2 at +64 */

#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))

/* the code of a library's init (header +22) or finit (+28) nullsub */
static APTR nullsub(const char *name, int off)
{
    struct lib *l = getlib(&loaded, name);
    UBYTE *p = (UBYTE *)L(l->code + off);
    p += 6;
    while (W(p))
        p += 4;
    return (APTR)L(p + 2 + 4);
}

/* the Rast (without the BitMap pointer), the planes, the a2 area */
static void same(void)
{
    UBYTE r[128];
    int p;

    memcpy(r, rast, 128);
    memset(r + 4, 0, 4);
    OBSBYTES("Rast", r, 128);
    BOTH("Rast's BitMap", ((struct RastPort *)rast)->BitMap == bm);
    for (p = 0; p < 2; p++)
        OBSBYTES("plane", bm->Planes[p], BMW / 8 * BMH);
    OBSBYTES("a2 area", a2area, 128);
}

static void clearplanes(void)
{
    int p;
    for (p = 0; p < 2; p++)
        memset(bm->Planes[p], 0, BMW / 8 * BMH);
}

static void locate(ULONG x, ULONG y)
{
    ROUT = "IPLocate";
    sprintf(VEC, "$%08lX,$%08lX", x, y);
    DO1(ENT(LIB, E_LOCATE, 0), R.d[0] = x; R.d[1] = y;
        R.a[0] = (ULONG)rast);
    same();
}

static void colour(int form, ULONG fg, ULONG bg)
{
    ROUT = "IPColour";
    sprintf(VEC, "form %d $%08lX,$%08lX", form, fg, bg);
    DO1(ENT(LIB, E_COLOUR, form), R.d[0] = fg; R.d[1] = bg;
        R.a[0] = (ULONG)rast; R.a[2] = (ULONG)&a2area[64]);
    same();
}

/* something drawn on the planes */
static int drawn(void)
{
    int p, i;
    for (p = 0; p < 2; p++)
        for (i = 0; i < BMW / 8 * BMH; i++)
            if (bm->Planes[p][i])
                return 1;
    return 0;
}

/* inputoutputlib's output routine `idx` (to the handler of IPOutput);
   `text`: 1 something is drawn, 0 nothing, -1 not known; d0txt: how the
   vector names d0 (NULL: its value; an address is no part of a key) */
static void out(int idx, const char *what, ULONG d0, const char *d0txt, const char *a0,
                int text)
{
    clearplanes();
    ROUT = "output";
    if (d0txt)
        snprintf(VEC, sizeof VEC, "%s d0 %s \"%.40s\"", what, d0txt, a0 ? a0 : "");
    else
        snprintf(VEC, sizeof VEC, "%s d0 $%08lX \"%.40s\"", what, d0, a0 ? a0 : "");
    DO1(ENT(IO, idx, 0), R.d[0] = d0; R.a[0] = (ULONG)a0);
    same();
    if (text >= 0)
        BOTH(text ? "text drawn" : "nothing drawn", drawn() == text);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, IO, NULL };
    static const char q[] = "Q";
    ULONG avail;

    if (argc < 2) {
        printf("usage: riintuilib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("inputoutputlib init", d2-d7|a1-a3|a6);
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("IPBitMap", d2-d7|a0-a3);
    KEEPS("IPLocate", d2-d7|a0|a2|a3);
    KEEPS("IPColour", d2-d7|a3);
    KEEPS("IPOutput", d1-d7|a1-a3|a6);
    KEEPS("output", d2-d7|a2|a3|a6);
    KEEPS("finit", d2-d7|a2|a3);
    ipgfx = OpenLibrary((STRPTR)"graphics.library", 39);
    if (!ipgfx) {
        check("riintuilib: graphics.library 39", FALSE);
        return 20;
    }
    bm = AllocBitMap(BMW, BMH, 2, BMF_CLEAR, NULL);
    if (!bm) {
        check("riintuilib: a BitMap", FALSE);
        return 20;
    }
    memset(rast, 0x55, sizeof rast);
    memset(a2area, 0, sizeof a2area);
    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();

    ROUT = "inputoutputlib init";
    strcpy(VEC, "");
    DO1(nullsub(IO, 22), R.a[6] = (ULONG)DOSBase);
    ROUT = "init";
    DO1(nullsub(LIB, 22), (void)0);

    ROUT = "IPBitMap";
    DO1(ENT(LIB, E_BITMAP, 0), R.a[0] = (ULONG)rast; R.a[1] = (ULONG)bm);
    same();

    locate(WA(10), WA(7));
    locate(0x12340005, 0xffff0003);
    locate(WA(0xfffe), WA(0x8000));     /* negative words */
    locate(WA(0), WA(0));

    colour(0, WA(3), MARK(1));
    colour(0, WA(0xffff), MARK(1));     /* nothing */
    colour(1, WA(2), WA(1));
    colour(1, WA(0x8001), WA(3));       /* BUG(orig) (#696): SetBPen on the caller's a2 */
    colour(1, WA(1), WA(0xffff));       /* the bgpen stays */
    colour(1, WA(0xffff), WA(0xffff));

    ROUT = "IPOutput";
    DO1(ENT(LIB, E_OUTPUT, 0), R.a[0] = (ULONG)rast);
    same();

    out(IO_OUTSTRING2, "string", MARK(0), NULL, "Hello, World", 1);
    out(IO_OUTSTRING2, "empty string", MARK(0), NULL, "", 0);
    out(IO_OUTSTRING1, "a0/d0", WA(5), NULL, "Blitz Basic", 1);
    out(IO_OUTSTRING1, "a0/d0 0", WA(0), NULL, "nothing", 0);
    out(IO_OUTSINGLE, "character", (ULONG)q, "&\"Q\"", NULL, 1);  /* BUG(orig) (#699): (d0) */
    out(IO_OUTSINGLE, "character 65", 65, NULL, NULL, -1);
    colour(1, WA(1), WA(2));
    out(IO_OUTSTRING2, "pens 1/2", MARK(0), NULL, "Pens", 1);
    colour(0, WA(3), 0);
    out(IO_OUTSTRING1, "pen 3", WA(4), NULL, "Pen 3!", 1);
    locate(WA(100), WA(10));            /* no effect on the output (#697) */
    out(IO_OUTSTRING2, "after IPLocate", MARK(0), NULL, "0,0", 1);

    ROUT = "finit";
    strcpy(VEC, "");
    DO1(nullsub(LIB, 28), (void)0);
    rwe_mem_check(LIB, avail, FALSE);    /* nothing allocated */
    FreeBitMap(bm);
    CloseLibrary(ipgfx);
    return rwe_done(LIB);
}
