/*
 * rwe riintuilib - the C build of riintuilib (userlib 15) against the
 * reference build of rwelibsrc/riintuilib2.bb (ref/riintuilib.obj, see
 * libs/userlibs/rwe/refbuild.cmake), method of rwecmp.h.
 *
 *   riintuilib DIR     (DIR with riintuilib, inputoutputlib .obj/.orig)
 *
 * Each set has its own inputoutputlib, Rast object (128 bytes) and
 * BitMap (320 x 16, 2 planes).  Compared: the Rast (every field but the
 * BitMap pointer, which must be the set's BitMap), both planes after
 * every output, the 128 bytes around the caller's a2 (IPColour's
 * BUG(orig) SetBPen there), the kept registers; the memory back at the
 * end.
 * Covered: init, IPBitMap, IPLocate (also with upper words and a
 * negative word), IPColour both forms with pens >= 0, a negative fgpen
 * (BUG(orig): SetBPen on the caller's a2) and negative bgpen, IPOutput
 * and all three entries of its handler through inputoutputlib's
 * dooutsingle/dooutstring1/dooutstring2: a character (BUG(orig): read
 * from the address d0 - a string's address and the low memory address
 * 65), a0/d0 strings (also length 0), a null terminated string, with
 * the pens of IPColour; finit.
 * Not covered: a0/d0 strings over 1024 characters (BUG(orig): they
 * overwrite the pens; the text would be drawn far beyond the BitMap,
 * which a RastPort without a layer does not clip).
 */
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

static UBYTE rast[2][128];
static struct BitMap *bm[2];
static UBYTE a2area[2][128];            /* the caller's a2 at +64 */

#define WA(v) (0x5A000000UL | ((ULONG)(v) & 0xffff))

/* the code of a library's init (header +22) or finit (+28) nullsub */
static APTR nullsub(const char *name, int off)
{
    struct lib *l = getlib(&sets[cur], name);
    UBYTE *p = (UBYTE *)L(l->code + off);
    p += 6;
    while (W(p))
        p += 4;
    return (APTR)L(p + 2 + 4);
}

/* the Rast of both sets (without the BitMap pointer), the planes, the
   a2 area */
static void same(void)
{
    UBYTE r[2][128];
    int s, p;

    for (s = 0; s < 2; s++) {
        memcpy(r[s], rast[s], 128);
        memset(r[s] + 4, 0, 4);
    }
    SAMEBYTES("Rast", r, 128);
    BOTH("Rast's BitMap", ((struct RastPort *)rast[cur])->BitMap == bm[cur]);
    for (p = 0; p < 2; p++) {
        UBYTE *pl[2] = { bm[0]->Planes[p], bm[1]->Planes[p] };
        SAMEBYTES("plane", pl, BMW / 8 * BMH);
    }
    SAMEBYTES("a2 area", a2area, 128);
}

static void clearplanes(void)
{
    int s, p;
    for (s = 0; s < 2; s++)
        for (p = 0; p < 2; p++)
            memset(bm[s]->Planes[p], 0, BMW / 8 * BMH);
}

static void locate(ULONG x, ULONG y)
{
    ROUT = "IPLocate";
    sprintf(VEC, "$%08lX,$%08lX", x, y);
    DO2(ENT(LIB, E_LOCATE, 0), R[cur].d[0] = x; R[cur].d[1] = y;
        R[cur].a[0] = (ULONG)rast[cur]);
    same();
}

static void colour(int form, ULONG fg, ULONG bg)
{
    ROUT = "IPColour";
    sprintf(VEC, "form %d $%08lX,$%08lX", form, fg, bg);
    DO2(ENT(LIB, E_COLOUR, form), R[cur].d[0] = fg; R[cur].d[1] = bg;
        R[cur].a[0] = (ULONG)rast[cur]; R[cur].a[2] = (ULONG)&a2area[cur][64]);
    same();
}

/* something drawn on the planes of set `cur` */
static int drawn(void)
{
    int p, i;
    for (p = 0; p < 2; p++)
        for (i = 0; i < BMW / 8 * BMH; i++)
            if (bm[cur]->Planes[p][i])
                return 1;
    return 0;
}

/* inputoutputlib's output routine `idx` (to the handler of IPOutput);
   `text`: 1 something is drawn, 0 nothing, -1 not known */
static void out(int idx, const char *what, ULONG d0, const char *a0, int text)
{
    clearplanes();
    ROUT = "output";
    snprintf(VEC, sizeof VEC, "%s d0 $%08lX \"%.40s\"", what, d0, a0 ? a0 : "");
    DO2(ENT(IO, idx, 0), R[cur].d[0] = d0; R[cur].a[0] = (ULONG)a0);
    same();
    if (text >= 0)
        BOTH(text ? "text drawn" : "nothing drawn", drawn() == text);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, IO, NULL };
    static const char q[] = "Q";
    ULONG avail;
    int s;

    if (argc < 2) {
        printf("usage: riintuilib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    ipgfx = OpenLibrary((STRPTR)"graphics.library", 39);
    if (!ipgfx) {
        check("riintuilib: graphics.library 39", FALSE);
        return 20;
    }
    for (s = 0; s < 2; s++) {
        bm[s] = AllocBitMap(BMW, BMH, 2, BMF_CLEAR, NULL);
        if (!bm[s]) {
            check("riintuilib: a BitMap", FALSE);
            return 20;
        }
        memset(rast[s], 0x55, sizeof rast[s]);
        memset(a2area[s], 0, sizeof a2area[s]);
    }
    avail = AvailMem(MEMF_ANY);

    ROUT = "inputoutputlib init";
    strcpy(VEC, "");
    DO2(nullsub(IO, 22), R[cur].a[6] = (ULONG)DOSBase);
    ROUT = "init";
    DO2(nullsub(LIB, 22), (void)0);

    ROUT = "IPBitMap";
    DO2(ENT(LIB, E_BITMAP, 0), R[cur].a[0] = (ULONG)rast[cur]; R[cur].a[1] = (ULONG)bm[cur]);
    same();

    locate(WA(10), WA(7));
    locate(0x12340005, 0xffff0003);
    locate(WA(0xfffe), WA(0x8000));     /* negative words */
    locate(WA(0), WA(0));

    colour(0, WA(3), MARK(1));
    colour(0, WA(0xffff), MARK(1));     /* nothing */
    colour(1, WA(2), WA(1));
    colour(1, WA(0x8001), WA(3));       /* BUG(orig): SetBPen on the caller's a2 */
    colour(1, WA(1), WA(0xffff));       /* the bgpen stays */
    colour(1, WA(0xffff), WA(0xffff));

    ROUT = "IPOutput";
    DO2(ENT(LIB, E_OUTPUT, 0), R[cur].a[0] = (ULONG)rast[cur]);
    same();

    out(IO_OUTSTRING2, "string", MARK(0), "Hello, World", 1);
    out(IO_OUTSTRING2, "empty string", MARK(0), "", 0);
    out(IO_OUTSTRING1, "a0/d0", WA(5), "Blitz Basic", 1);
    out(IO_OUTSTRING1, "a0/d0 0", WA(0), "nothing", 0);
    out(IO_OUTSINGLE, "character", (ULONG)q, NULL, 1);  /* BUG(orig): (d0) */
    out(IO_OUTSINGLE, "character 65", 65, NULL, -1);
    colour(1, WA(1), WA(2));
    out(IO_OUTSTRING2, "pens 1/2", MARK(0), "Pens", 1);
    colour(0, WA(3), 0);
    out(IO_OUTSTRING1, "pen 3", WA(4), "Pen 3!", 1);
    locate(WA(100), WA(10));            /* no effect on the output */
    out(IO_OUTSTRING2, "after IPLocate", MARK(0), "0,0", 1);

    ROUT = "finit";
    strcpy(VEC, "");
    DO2(nullsub(LIB, 28), (void)0);

    if (AvailMem(MEMF_ANY) != avail)
        printf("riintuilib: %ld bytes missing\n", (LONG)(avail - AvailMem(MEMF_ANY)));
    check("riintuilib: all memory back", AvailMem(MEMF_ANY) == avail);
    for (s = 0; s < 2; s++)
        FreeBitMap(bm[s]);
    CloseLibrary(ipgfx);
    return rwe_done(LIB);
}
