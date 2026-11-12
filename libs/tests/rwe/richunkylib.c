/*
 * rwe richunkylib - chunky bitmaps (ChunkyBitmap, ChunkyToPlanar,
 * ChunkyData, RaycastSky ...): the C build or the reference build of
 * rwelibsrc/chunkylib.bb (richunkylib.orig), one set as LIBSET says,
 * observed as cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_richunkylib.txt).
 *
 *   richunkylib DIR     (DIR with lmullib, richunkylib .obj/.orig)
 *
 * The objects (16 bytes each) and a bitmap (struct BitMap, 8
 * contiguous planes of 10240 bytes and a guard behind them, filled with a
 * pattern before each conversion) are built here.  The library calls
 * lmullib (width*height, loaded) and memlib's globalloc/globfree: those
 * two are stubs here (sysload_stubs; memlib itself ends the program with
 * a runtime error when an allocation fails, the stub can return 0 like
 * AllocMem, so the library's own failure path can be tested); the stubs
 * keep all registers but d0 and count the blocks.  Observed after every
 * call: d0 where there is a result (pointers as "is the data
 * of the object"), the objects (the data pointer as 0 / not 0), the
 * chunky data, the planes with the guard, the number of blocks
 * allocated, CIA-A PRA bit 1 (the free routine toggles it, BUG(orig) #886;
 * put back before every call and at the end) and the kept registers.
 *
 * Covered: ChunkyBitmap (new, again with data = free first, a failing
 * allocation with and without old data, the unused word
 * at offset 6 set without data: the free routine without data,
 * BUG(orig), #885), ChunkyData, ChunkyToPlanar with random chunky data (depth
 * 8, the depth word $0108 = with bm_Flags, 7 and $8008 = nothing, an
 * object without data = nothing), RaycastSky (random columns, heights
 * 1..256, x and column with marker upper words, negative columns),
 * PlanarToChunky and ChunkyLoadBitMap (no code), the object manager's
 * free routine (a3 = the object; with and without data), init, finit.
 * Not covered: RaycastSky with height 0 (65536 rows) or a negative x
 * (writes before the data), ChunkyToPlanar on undersized data.
 */
#include "rwecmp.h"
#include <exec/memory.h>
#include <graphics/gfx.h>

#define LIB "richunkylib"
enum { CHUNKYBITMAP, C2P, P2C, LOADBITMAP, CHUNKYDATA, RAYCASTSKY, OLOAD, OSAVE, OUSE,
       OFREE, FINIT, INIT };

#define CIAAPRA (*(volatile UBYTE *)0xbfe001)
#define NOBJ 4
#define PLSIZ 10240
#define PLANES (8 * PLSIZ)
#define GUARD 64
#define CHUNKY 81920
#define TEXSZ (320 * 260 + 2048)

static UBYTE *objs;                     /* NOBJ objects of 16 bytes */
static struct BitMap *bm;
static UBYTE *planes;                   /* PLANES + GUARD */
static UBYTE *tex;
static UBYTE hwpra;
static UBYTE pra;                       /* PRA bit 1 before each call (the start's) */

static LONG blocks;                     /* blocks allocated by the stubs */
static int failalloc;                   /* the globalloc stub fails */

/* the stubs of memlib's globalloc (d0 = size, d1 = flags -> d0) and
   globfree (a1 = memory): the size in a longword before the block */
ULONG chunky_fake_alloc(ULONG size, ULONG flags);
void chunky_fake_free(ULONG *mem);
ULONG chunky_fake_alloc(ULONG size, ULONG flags)
{
    ULONG *m = failalloc ? NULL : AllocMem(size + 4, flags);
    if (!m)
        return 0;
    blocks++;
    m[0] = size + 4;
    return (ULONG)(m + 1);
}

void chunky_fake_free(ULONG *mem)
{
    blocks--;
    FreeMem(mem - 1, mem[-1]);
}

void chunky_globalloc(void), chunky_globfree(void);
__asm__("   .text\n   .even\n"
        "   .globl  _chunky_globalloc\n"
        "_chunky_globalloc:\n"
        "   movem.l d1-d7/a0-a6,-(sp)\n"
        "   move.l  d1,-(sp)\n"
        "   move.l  d0,-(sp)\n"
        "   jsr     _chunky_fake_alloc\n"
        "   addq.l  #8,sp\n"
        "   movem.l (sp)+,d1-d7/a0-a6\n"
        "   rts\n"
        "   .globl  _chunky_globfree\n"
        "_chunky_globfree:\n"
        "   movem.l d0-d7/a0-a6,-(sp)\n"
        "   move.l  a1,-(sp)\n"
        "   jsr     _chunky_fake_free\n"
        "   addq.l  #4,sp\n"
        "   movem.l (sp)+,d0-d7/a0-a6\n"
        "   rts\n");

static const struct stub stubs[] = {
    { 0xc002, (APTR)chunky_globalloc },
    { 0xc003, (APTR)chunky_globfree },
    { 0, NULL }
};

static UBYTE *obj(int n) { return objs + 16 * n; }
static ULONG data(int n) { return *(ULONG *)(obj(n) + 8); }

static void fillpattern(void)
{
    ULONG i;
    for (i = 0; i < PLANES + GUARD; i++)
        planes[i] = (UBYTE)(i * 7 + 0x35);
}

/* the objects: fields as words, the data pointer 0 / not 0 */
static void cmpobjs(void)
{
    int n;
    char buf[24];
    for (n = 0; n < NOBJ; n++) {
        sprintf(buf, "object %d w/h/d/6", n);
        OBS(buf, *(ULONG *)obj(n));
        OBS(buf, *(ULONG *)(obj(n) + 4));
        sprintf(buf, "object %d data", n);
        OBS(buf, data(n) != 0);
        sprintf(buf, "object %d 12", n);
        OBS(buf, *(ULONG *)(obj(n) + 12));
    }
}

/* the chunky data of object n */
static void cmpdata(int n, ULONG size)
{
    UBYTE *d = (UBYTE *)data(n);
    if (d)
        OBSBYTES("chunky data", d, size);
}

/* one call: the registers from `set` */
static void call2(int idx, const char *name, void (*set)(void))
{
    APTR fn;
    ROUT = name;
    fn = ENT(LIB, idx, 0);
    markregs(&R);
    set();
    IN = R;
    CIAAPRA = (UBYTE)((CIAAPRA & ~2) | pra);
    if (!fn) {
        check("entry missing", FALSE);
        return;
    }
    NARGS = 0;
    CCR = callstk(fn, &R, ARGS, NARGS, &POP);
    hwpra = CIAAPRA & 2;
    preserve();
    OBS("CIA-A PRA bit 1", hwpra);
    OBS("blocks", blocks);
    cmpobjs();
}

static void set_none(void) { }

/* ChunkyBitmap n,w,h: d0 = n, d1 = w, d2 = h (word arguments: marker
   upper words), a0 = the object */
static int cb_n;
static ULONG cb_w, cb_h;
static void set_cb(void)
{
    R.d[0] = (ULONG)cb_n;
    R.d[1] = (R.d[1] & 0xffff0000UL) | (UWORD)cb_w;
    R.d[2] = (R.d[2] & 0xffff0000UL) | (UWORD)cb_h;
    R.a[0] = (ULONG)obj(cb_n);
}

static void chunkybitmap(int n, ULONG w, ULONG h)
{
    cb_n = n;
    cb_w = w;
    cb_h = h;
    sprintf(VEC, "%d,%ld,%ld", n, (LONG)(WORD)w, (LONG)(WORD)h);
    call2(CHUNKYBITMAP, "ChunkyBitmap", set_cb);
}

/* ChunkyData(n): the result is the object's data */
static void set_cd(void)
{
    R.d[0] = (ULONG)cb_n;
    R.a[0] = (ULONG)obj(cb_n);
}

static void chunkydata(int n)
{
    cb_n = n;
    sprintf(VEC, "%d", n);
    call2(CHUNKYDATA, "ChunkyData", set_cd);
    BOTH("d0 = data", R.d[0] == data(n));
}

/* ChunkyToPlanar n,bitmap: a0 = the object, a1 = the bitmap */
static void set_c2p(void)
{
    R.d[0] = (ULONG)cb_n;
    R.d[1] = 0;
    R.a[0] = (ULONG)obj(cb_n);
    R.a[1] = (ULONG)bm;
}

static void c2p(int n, UWORD depthword, const char *what)
{
    int p;
    bm->BytesPerRow = 40;
    bm->Rows = 256;
    *(UWORD *)&bm->Flags = depthword;
    for (p = 0; p < 8; p++)
        bm->Planes[p] = (PLANEPTR)(planes + p * PLSIZ);
    fillpattern();
    cb_n = n;
    sprintf(VEC, "%d %s", n, what);
    call2(C2P, "ChunkyToPlanar", set_c2p);
    OBSBYTES("planes", planes, PLANES + GUARD);
}

/* RaycastSky n,x,height,column,&texture: d0-d4, a0 = the object */
static ULONG rs_x, rs_h, rs_col, rs_tex;
static void set_rs(void)
{
    R.d[0] = (ULONG)cb_n;
    R.d[1] = (R.d[1] & 0xffff0000UL) | (UWORD)rs_x;
    R.d[2] = (R.d[2] & 0xffff0000UL) | (UWORD)rs_h;
    R.d[3] = (R.d[3] & 0xffff0000UL) | (UWORD)rs_col;
    R.d[4] = (ULONG)tex + rs_tex;
    R.a[0] = (ULONG)obj(cb_n);
}

/* the free routine of the object manager: a3 = the object */
static void set_free(void)
{
    R.a[3] = (ULONG)obj(cb_n);
}

static void objfree(int n)
{
    cb_n = n;
    sprintf(VEC, "%d", n);
    call2(OFREE, "free (object manager)", set_free);
}

int main(int argc, char **argv)
{
    static const char *names[] = { "lmullib", LIB, NULL };
    UBYTE pra0;
    int i;
    ULONG k;

    if (argc < 2) {
        printf("usage: richunkylib DIR\n");
        return 20;
    }
    sysload_stubs = stubs;
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("ChunkyData", d1-d7|a0-a3|a6);
    KEEPS("ChunkyToPlanar", d2-d7|a2|a3|a6);
    KEEPS("ChunkyBitmap", d3-d6|a0|a2|a3|a6);
    KEEPS("RaycastSky", d0|d1|d3-d7|a2|a3|a6);
    KEEPS("PlanarToChunky", d0-d7|a0-a3|a6);
    KEEPS("ChunkyLoadBitMap", d0-d7|a0-a3|a6);
    KEEPS("free (object manager)", d2-d7|a2|a3|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);
    objs = AllocMem(16 * NOBJ, MEMF_PUBLIC | MEMF_CLEAR);
    bm = AllocMem(sizeof(struct BitMap), MEMF_PUBLIC | MEMF_CLEAR);
    planes = AllocMem(PLANES + GUARD, MEMF_PUBLIC);
    tex = AllocMem(TEXSZ, MEMF_PUBLIC);
    if (!objs || !bm || !planes || !tex) {
        check("richunkylib: memory", FALSE);
        return rwe_done(LIB);
    }
    pra0 = CIAAPRA & 2;
    pra = pra0;
    strcpy(VEC, "");

    call2(INIT, "init", set_none);

    chunkydata(0);                              /* no data: 0 */
    c2p(0, 8, "no data");
    chunkybitmap(0, 320, 256);
    {
        const UBYTE *d = (const UBYTE *)data(0);
        BOTH("cleared", d && !d[0] && !d[CHUNKY / 2] && !d[CHUNKY - 1]);
    }
    chunkydata(0);

    /* random chunky data, then C2P */
    for (i = 0; i < 2; i++) {
        for (k = 0; k < CHUNKY; k++)
            ((UBYTE *)data(0))[k] = (UBYTE)rnd();
        c2p(0, 8, "depth 8");
    }
    c2p(0, 0x0108, "depth word $0108");
    c2p(0, 7, "depth 7");
    c2p(0, 0x8008, "depth word $8008");

    /* RaycastSky: texture columns into the chunky data */
    for (k = 0; k < TEXSZ; k++) {
        UBYTE v = (UBYTE)rnd();
        tex[k] = v;
    }
    cb_n = 0;
    for (i = 0; i < 40; i++) {
        rs_h = 1 + rnd() % 256;
        k = 257 - rs_h < 102 ? 257 - rs_h : 102;        /* x below 32768 */
        rs_x = rnd() % 320 + 320 * (rnd() % k);
        rs_col = (rnd() % 1300) - 1000;
        rs_tex = 1024 + (rnd() % 2) * 320;
        if (i == 0) {
            rs_h = 256;
            rs_x = 0;
        }
        sprintf(VEC, "x %lu height %lu column %ld", rs_x, rs_h, (LONG)rs_col);
        call2(RAYCASTSKY, "RaycastSky", set_rs);
        cmpdata(0, CHUNKY);
    }

    strcpy(VEC, "");
    cb_n = 0;
    call2(P2C, "PlanarToChunky", set_c2p);
    call2(LOADBITMAP, "ChunkyLoadBitMap", set_cd);
    cmpdata(0, CHUNKY);
    OBSBYTES("planes", planes, PLANES + GUARD);

    /* again with data: freed first (the LED toggles) */
    chunkybitmap(0, 100, 50);
    failalloc = 1;                              /* the allocation fails */
    chunkybitmap(0, 320, 256);
    chunkybitmap(1, 16, 16);
    failalloc = 0;
    *(UWORD *)(obj(2) + 6) = 1;                 /* the unused word set */
    chunkybitmap(2, 16, 16);
    chunkybitmap(3, 8, 8);
    chunkydata(3);

    objfree(3);
    objfree(3);                                 /* no data: nothing */
    objfree(1);
    call2(FINIT, "finit", set_none);
    objfree(2);
    BOTH("all blocks freed", blocks == 0);
    CIAAPRA = (UBYTE)((CIAAPRA & ~2) | pra0);
    FreeMem(objs, 16 * NOBJ);
    FreeMem(bm, sizeof(struct BitMap));
    FreeMem(planes, PLANES + GUARD);
    FreeMem(tex, TEXSZ);
    return rwe_done(LIB);
}
