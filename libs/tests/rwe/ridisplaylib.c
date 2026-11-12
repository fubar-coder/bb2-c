/*
 * rwe ridisplaylib - InitView, ViewBitmap: the C build or the reference
 * build of rwelibsrc/mydisplaylib.bb (ridisplaylib.orig), one set as LIBSET
 * says, observed as cases (rwecmp.h, LIBS_ONE_SET; the expected data
 * libs/tests/expected/rwe_ridisplaylib.txt).
 *
 *   ridisplaylib DIR     (DIR with ridisplaylib .obj/.orig)
 *
 * The library allocates a copper list per InitView but installs nothing:
 * the display is not touched.  Observed after every call: d0 (InitView:
 * the list, as "is the list the library keeps"), the variables viewsize
 * and cop (found in both libraries by the list InitView returns), the
 * list's 8 words (the words at the absolute addresses $8E, $2C81, $90,
 * $2CC1, $92, $38, $94, $D0, BUG(orig) #889: equal to those words, and the
 * value of the ones in the vector table), the change of
 * graphics.library's open count and the test task's memory balance
 * (rwemem.h), and the kept registers.  The lists InitView
 * leaks (only the last one is kept, BUG(orig), #891) are freed by the test.
 *
 * Covered: init, InitView (sizes with marker upper words, the colour
 * count ignored, BUG(orig), #890), ViewBitmap (no code), finit with a list,
 * finit again (nothing: graphics.library closed), init and finit
 * without a list.  Not covered: InitView without chip memory or with a
 * size that is negative as a word (both write to address 0).
 */
#define LIBS_ONE_SET
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "ridisplaylib"
enum { INITVIEW, VIEWBITMAP, INIT, FINIT };

static struct Library *gfx;
static LONG opened, membal;
static ULONG *vars;                     /* viewsize, cop */
static ULONG oldcop, oldsize;           /* the list before (leaked) */
static ULONG cop;

static ULONG findvars(struct lib *l, ULONG list)
{
    ULONG i;
    for (i = 4; i + 4 <= l->size; i += 2)
        if (L(l->code + i) == list)
            return (ULONG)(l->code + i - 4);
    return 0;
}

/* one call: the open count and this task's memory balance
   around it (rwemem.h; AvailMem() would also count other tasks) */
static void call2(int idx, const char *name, ULONG d0, ULONG d1, ULONG d2, ULONG d3)
{
    APTR fn;
    LONG cnt;
    ROUT = name;
    fn = ENT(LIB, idx, 0);
    cnt = gfx->lib_OpenCnt;
    markregs(&R);
    R.d[0] = d0;
    R.d[1] = d1;
    R.d[2] = d2;
    R.d[3] = d3;
    IN = R;
    NARGS = 0;
    rwe_mem_start();
    CCR = callstk(fn, &R, ARGS, NARGS, &POP);
    membal = rwe_mem_stop();
    opened = gfx->lib_OpenCnt - cnt;
    preserve();
    OBS("graphics.library open count", opened);
    OBS("memory balance", membal);
    if (vars) {
        OBS("viewsize", vars[0]);
        OBS("cop", vars[1] != 0);
    }
}

static void initview(UWORD h, UWORD d, UWORD s, UWORD c)
{
    static const ULONG addr[8] = { 0x8e, 0x2c81, 0x90, 0x2cc1, 0x92, 0x38, 0x94, 0xd0 };
    int i;
    char buf[40];

    sprintf(VEC, "%u,%u,%u,%u", h, d, s, c);
    oldcop = vars ? vars[1] : 0;
    oldsize = vars ? vars[0] : 0;
    call2(INITVIEW, "InitView", 0xa5a50000UL | h, 0x5a5a0000UL | d, 0x12340000UL | s,
          0xfedc0000UL | c);
    cop = R.d[0];
    if (!vars)
        vars = (ULONG *)findvars(getlib(&loaded, LIB), cop);
    BOTH("variables found", vars != NULL);
    if (!vars)
        return;
    BOTH("d0 = cop", cop && vars[1] == cop);
    for (i = 0; i < 8; i++) {
        sprintf(buf, "word %d = ($%lX)", i, addr[i]);
        BOTH(buf, ((UWORD *)cop)[i] == *(volatile UWORD *)addr[i]);
        if (addr[i] & 1)
            continue;
        sprintf(buf, "word %d", i);
        OBS(buf, ((UWORD *)cop)[i]);
    }
    if (oldcop)
        FreeMem((APTR)oldcop, oldsize);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };

    if (argc < 2) {
        printf("usage: ridisplaylib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("InitView", d7|a2|a3);
    KEEPS("ViewBitmap", d0-d7|a0-a3|a6);
    KEEPS("finit", d2-d7|a2|a3);
    KEEPS("finit (again)", d0-d7|a0-a3|a6);
    KEEPS("init (again)", d2-d7|a2|a3);
    KEEPS("finit (no list)", d0-d7|a2|a3);
    gfx = OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    if (!gfx) {
        check("ridisplaylib: graphics.library 39", FALSE);
        return rwe_done(LIB);
    }
    strcpy(VEC, "");
    call2(INIT, "init", 0, 0, 0, 0);
    initview(256, 8, 8, 256);
    initview(200, 5, 0, 32);
    initview(0, 0, 0, 0);
    initview(1000, 8, 8, 2);
    initview(3000, 1, 100, 0xffff);
    strcpy(VEC, "");
    call2(VIEWBITMAP, "ViewBitmap", 0x12345678, 0, 0, 0);
    call2(FINIT, "finit", 0, 0, 0, 0);
    BOTH("freed", vars && !vars[0] && !vars[1]);
    call2(FINIT, "finit (again)", 0, 0, 0, 0);
    call2(INIT, "init (again)", 0, 0, 0, 0);
    call2(FINIT, "finit (no list)", 0, 0, 0, 0);
    CloseLibrary(gfx);
    return rwe_done(LIB);
}
