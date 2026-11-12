/*
 * rwe rilvcallhooklib - RetrieveCallHook and its listview render hook:
 * the C build or the reference build of
 * rwelibsrc/lvcallhook/LVCallHook3.bb (blitz2 2.10), one set as LIBSET
 * says, observed as cases (rwecmp.h, LIBS_ONE_SET; the expected data
 * libs/tests/expected/rwe_rilvcallhooklib.txt).
 *
 *   rilvcallhooklib DIR     (DIR with rilvcallhooklib .obj or .orig)
 *
 * RetrieveCallHook (a6 = graphics.library) gives the hook
 * function; it is called as GadTools would (a0 = Hook, a1 = LVDrawMsg,
 * a2 = node) with the message's RastPort on a BitMap (2
 * planes, 1280 x 48, cleared to a pattern first).  Observed (after
 * WaitBlit: the hook returns with the last blit running): d0, both
 * planes, the RastPort (pens, draw mode, line pattern, area pattern,
 * the pen position ...; without the BitMap pointer), kept registers.
 *
 * The items are random: bounds, the states 0/1/2/8/4, 1..4 fields of
 * text (left/right aligned, cut by TextFit) and BitMaps (left/right
 * aligned, cut to the field, the BitMap pointer 0), the last field cut at
 * the right edge, with and without an indent of 1..60 pixels (tree lines
 * every 16 pixels, the short last line by the next node: the end of the
 * list or the BUG(orig) (#739) reading of its "indent" at the first field
 * width), and the caller's d7 with upper words 0, $FFFF, $8001, $0001 and
 * $8000 (BUG(orig) #737: a negative d7 draws no tree lines and leaves the line
 * pattern $AAAA).  Also a message other than LV_DRAW (LVCB_UNKNOWN,
 * nothing drawn).
 *
 * Layouts in which graphics.library would get an empty rectangle (a
 * RastPort without a layer is not clipped) are skipped: item heights of
 * 10 lines and more, BitMaps lower than the item, no text and no BitMap
 * that ends exactly at the field's edge.  Not covered: d7 with an indent
 * below 16 and an upper word that makes 65536 tree lines (BUG(orig), #737), a
 * text pointer 0 (BUG(orig) #742: reads address 0), negative indents.
 */
#define LIBS_ONE_SET
#include "rwecmp.h"
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <libraries/gadtools.h>
#include <utility/hooks.h>
static struct Library *lvgfx;
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME lvgfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

#define LIB "rilvcallhooklib"
enum { E_RETRIEVE, E_FINIT, E_INIT };
#define BMW 1280
#define BMH 48
#define DEPTH 2

static struct BitMap bm;
static struct RastPort rp;
static struct LVDrawMsg msg;
static ULONG hookfn;
static struct TextFont *font;

/* the node offsets */
#define N_INDENT 14
#define N_COL 16
#define N_TEXT1 20
#define N_TEXT2 24
#define N_BM1 28
#define N_BM0 32                        /* a BitMap pointer 0 */
#define N_BM2 36
#define NODESZ 512
static UBYTE nodes[3][NODESZ];          /* the item, the next one, the tail */
static UWORD coltab[4];
static UWORD fields[7 * 6];             /* the field table (14 byte entries) */
static struct Hook hook;

static struct BitMap srcbm[3];
static const char *const strs[] = {
    "Hello", "A longer item text here", "x", "WWWWWWWWWWWWWWWW", "iiiiiiiiii",
    "Directory", "file.info", "12345678901234567890", "Ab",
};
#define NSTRS (sizeof strs / sizeof strs[0])

static void mkbitmap(struct BitMap *b, int w, int h)
{
    int p;
    ULONG i, n = (ULONG)((w + 15) / 16 * 2) * h;
    InitBitMap(b, DEPTH, w, h);
    for (p = 0; p < DEPTH; p++) {
        b->Planes[p] = AllocRaster(w, h);
        for (i = 0; i < n; i++)
            b->Planes[p][i] = (UBYTE)rnd();
    }
}

/* the RastPort and BitMap reset before every call */
static void reset(UWORD fg, UWORD bg)
{
    int p;
    for (p = 0; p < DEPTH; p++)
        memset(bm.Planes[p], 0x5a + 0x11 * p, (BMW / 8) * BMH);
    InitRastPort(&rp);
    rp.BitMap = &bm;
    SetFont(&rp, font);
    SetABPenDrMd(&rp, fg, bg, JAM1);
}

static void observe(void)
{
    static struct RastPort r;
    int p;
    WaitBlit();                         /* the last blit may still run */
    for (p = 0; p < DEPTH; p++)
        OBSBYTES(p ? "plane 1" : "plane 0", bm.Planes[p], (BMW / 8) * BMH);
    r = rp;
    r.BitMap = NULL;
    OBSBYTES("RastPort", &r, sizeof r);
}

static void callhook(ULONG method, ULONG state, const struct Rectangle *b, ULONG d7)
{
    msg.lvdm_MethodID = method;
    msg.lvdm_RastPort = &rp;
    msg.lvdm_DrawInfo = NULL;
    msg.lvdm_Bounds = *b;
    msg.lvdm_State = state;
    ROUT = "hook";
    DO1((APTR)hookfn, R.a[0] = (ULONG)&hook; R.a[1] = (ULONG)&msg;
        R.a[2] = (ULONG)nodes[0]; R.d[7] = d7);
    OBS("d0", R.d[0]);
    observe();
}

static int pick(int lo, int hi)
{
    return lo + (int)(rnd() % (ULONG)(hi - lo + 1));
}

/* the width of string s in the font */
static int textw(const char *s)
{
    return TextLength(&rp, (CONST_STRPTR)s, strlen(s));
}

/* one random item; 0 if its layout is one of those skipped */
static int item(int i)
{
    static const int indents[] = { 1, 5, 15, 16, 17, 31, 32, 40, 47, 60 };
    static const UWORD up0[] = { 0x0000, 0xffff, 0x8001 };
    static const UWORD up1[] = { 0x0000, 0xffff, 0x0001, 0x8000 };
    static const ULONG states[] = { 0, 1, 2, 8, 4 };
    struct Rectangle b;
    int nf = pick(1, 4), f, ind = 0, minx, maxx;
    ULONG d7, state = states[rnd() % 5];
    UWORD *e;

    b.MinX = (WORD)pick(220, 250);
    b.MinY = (WORD)pick(4, 10);
    b.MaxX = (WORD)(b.MinX + pick(100, 280));
    b.MaxY = (WORD)(b.MinY + pick(9, 20));

    memset(nodes, 0, sizeof nodes);
    for (f = 0; f < 3; f++) {
        int k;
        for (k = 40; k < NODESZ; k += 2)
            *(WORD *)(nodes[f] + k) = (WORD)pick(0, 70);
    }
    *(UBYTE **)nodes[0] = nodes[1];
    *(UBYTE **)nodes[1] = rnd() & 1 ? nodes[2] : NULL;  /* the next is last or not */
    *(UBYTE **)nodes[2] = NULL;
    for (f = 0; f < 4; f++)
        coltab[f] = (UWORD)pick(0, 3);
    *(UWORD **)(nodes[0] + N_COL) = coltab;
    *(const char **)(nodes[0] + N_TEXT1) = strs[rnd() % NSTRS];
    *(const char **)(nodes[0] + N_TEXT2) = strs[rnd() % NSTRS];
    *(struct BitMap **)(nodes[0] + N_BM1) = &srcbm[rnd() % 3];
    *(struct BitMap **)(nodes[0] + N_BM0) = NULL;
    *(struct BitMap **)(nodes[0] + N_BM2) = &srcbm[rnd() % 3];

    memset(fields, 0, sizeof fields);
    if (i % 3) {                        /* with an indent */
        ind = indents[rnd() % 10];
        fields[0] = N_INDENT;
        *(WORD *)(nodes[0] + N_INDENT) = (WORD)ind;
    }
    fields[1] = N_COL;
    if (ind >> 4)
        d7 = (ULONG)up1[rnd() % 4] << 16 | (rnd() & 0xffff);
    else
        d7 = (ULONG)up0[rnd() % 3] << 16 | (rnd() & 0xffff);

    /* the fields, laid out as the hook does to skip the empty rectangles */
    minx = b.MinX + ind;
    for (f = 0, e = fields + 7; f < nf; f++, e += 7) {
        int w = pick(20, 150), bmp = rnd() % 3 == 0, fw, tw;
        int last = w + minx >= b.MaxX;
        e[0] = (UWORD)w;
        e[2] = (UWORD)(rnd() % 3);      /* 0, 1 right, 2 */
        e[3] = bmp ? (UWORD)pick(1, 5) : 0;
        if (bmp) {
            static const UWORD bo[] = { N_BM1, N_BM0, N_BM2 };
            e[1] = bo[rnd() % 3];
        } else
            e[1] = rnd() & 1 ? N_TEXT1 : N_TEXT2;
        maxx = last ? b.MaxX : w + minx - 1;
        fw = maxx - minx + 1;
        if (fw < 12)
            return 0;
        if (bmp) {
            const struct BitMap *sb = *(struct BitMap **)(nodes[0] + e[1]);
            tw = sb ? sb->BytesPerRow * 8 : 0;
            if (tw > fw)
                tw = fw;
            if (e[2] != 1 && tw == fw - 2)
                return 0;
        } else {
            tw = textw(*(const char **)(nodes[0] + e[1]));
            if (tw == fw - 1 || tw == fw - 2)
                return 0;
        }
        minx = maxx + 1;
        if (last)
            break;
    }
    hook.h_Data = fields;
    sprintf(VEC, "#%d: (%d,%d)-(%d,%d), state %lu, %d fields, indent %d, d7 $%08lX", i, b.MinX,
            b.MinY, b.MaxX, b.MaxY, state, nf, ind, d7);
    reset((UWORD)pick(0, 3), (UWORD)pick(0, 3));
    callhook(LV_DRAW, state, &b, d7);
    BOTH("LVCB_OK", R.d[0] == LVCB_OK);
    return 1;
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    static struct TextAttr topaz = { (STRPTR)"topaz.font", 8, 0, 0 };
    int i, n, p;

    if (argc < 2) {
        printf("usage: rilvcallhooklib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("RetrieveCallHook", d1-d7|a1-a3|a6);
    KEEPS("hook", d2-d7|a2|a3|a6);
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);
    lvgfx = OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    font = OpenFont(&topaz);
    InitBitMap(&bm, DEPTH, BMW, BMH);
    for (p = 0; p < DEPTH; p++)
        bm.Planes[p] = AllocRaster(BMW, BMH);
    mkbitmap(&srcbm[0], 16, 4);
    mkbitmap(&srcbm[1], 32, 8);
    mkbitmap(&srcbm[2], 48, 6);
    reset(1, 0);

    strcpy(VEC, "RetrieveCallHook");
    ROUT = "RetrieveCallHook";
    DO1(ENT(LIB, E_RETRIEVE, 0), R.a[6] = (ULONG)lvgfx);
    hookfn = R.d[0];
    BOTH("in the library", hookfn > (ULONG)getlib(&loaded, LIB)->code
         && hookfn < (ULONG)getlib(&loaded, LIB)->code + 16384);

    /* not LV_DRAW */
    {
        static const struct Rectangle b = { 230, 5, 400, 20 };
        strcpy(VEC, "method 1");
        memset(fields, 0, sizeof fields);
        hook.h_Data = fields;
        reset(2, 1);
        callhook(1, 0, &b, 0);
        BOTH("LVCB_UNKNOWN", R.d[0] == LVCB_UNKNOWN);
    }

    for (i = 0, n = 0; n < 150 && i < 2000; i++)
        n += item(i);
    check("enough items", n == 150);

    strcpy(VEC, "init/finit");
    ROUT = "init";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    ROUT = "finit";
    DO1(ENT(LIB, E_FINIT, 0), (void)0);

    for (p = 0; p < DEPTH; p++)
        FreeRaster(bm.Planes[p], BMW, BMH);
    for (i = 0; i < 3; i++) {
        for (p = 0; p < DEPTH; p++)
            FreeRaster(srcbm[i].Planes[p], srcbm[i].BytesPerRow * 8, srcbm[i].Rows);
    }
    CloseFont(font);
    CloseLibrary(lvgfx);
    return rwe_done(LIB);
}
