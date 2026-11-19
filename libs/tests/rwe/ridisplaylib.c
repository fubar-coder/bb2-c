/*
 * rwe ridisplaylib - InitView, ViewBitmap: the C build or the reference
 * build of rwelibsrc/mydisplaylib.bb (ridisplaylib.orig), one set as LIBSET
 * says, observed as cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_ridisplaylib.txt).
 *
 *   ridisplaylib DIR     (DIR with ridisplaylib .obj/.orig)
 *
 * The library allocates a copper list per InitView but installs nothing:
 * the display is not touched.  Observed after every call: d0 (InitView:
 * the list, as "is the list the library keeps"), the variables viewsize
 * and cop (found in both libraries by the list InitView returns), the
 * list's 8 words (each compared with its register move $008E, $2C81,
 * $0090, $2CC1, $0092, $0038, $0094, $00D0 and with the word at that
 * absolute address - the original's list, #889 -, and the value
 * of the ones at even addresses), the change of
 * graphics.library's open count and the test task's memory balance
 * (rwemem.h), and the kept registers.  The lists the
 * original leaks (it keeps only the last one, #891) are freed by the test
 * after finit: the ones finit's memory balance shows it did not free (all
 * but the last, or all when it freed nothing).
 *
 * Covered: init, InitView (sizes with marker upper words and colour
 * counts other than the height: the original ignores them, #890),
 * ViewBitmap (no code), finit with a list,
 * finit again (nothing: graphics.library closed), init and finit
 * without a list, InitView without init and finit then (graphics.library
 * not open).  Not covered: InitView without chip memory (the original
 * writes to address 0; bug-orig test ridisplaylib-allocmem-unchecked).
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "ridisplaylib"
enum { INITVIEW, VIEWBITMAP, INIT, FINIT };

static struct Library *gfx;
static LONG opened, membal;
static ULONG *vars;                     /* viewsize, cop */
static ULONG cop;
static ULONG lists[8], sizes[8];        /* the lists made, their viewsize */
static int nlists;

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

static void initview(const char *name, UWORD h, UWORD d, UWORD s, UWORD c)
{
    static const ULONG addr[8] = { 0x8e, 0x2c81, 0x90, 0x2cc1, 0x92, 0x38, 0x94, 0xd0 };
    int i;
    char buf[40];
    UWORD w;

    sprintf(VEC, "%u,%u,%u,%u", h, d, s, c);
    call2(INITVIEW, name, 0xa5a50000UL | h, 0x5a5a0000UL | d, 0x12340000UL | s,
          0xfedc0000UL | c);
    cop = R.d[0];
    if (!vars)
        vars = (ULONG *)findvars(getlib(&loaded, LIB), cop);
    BOTH("variables found", vars != NULL);
    if (!vars)
        return;
    BOTH("d0 = cop", cop && vars[1] == cop);
    for (i = 0; i < 8; i++) {
        w = ((UWORD *)cop)[i];
        sprintf(buf, "word %d = $%04lX", i, addr[i]);
        BOTH(buf, w == addr[i]);
        sprintf(buf, "word %d = ($%lX)", i, addr[i]);
        BOTH(buf, w == *(volatile UWORD *)addr[i]);
        if (addr[i] & 1)
            continue;
        sprintf(buf, "word %d", i);
        OBS(buf, ((UWORD *)cop)[i]);
    }
    if (cop && nlists < 8) {
        lists[nlists] = cop;
        sizes[nlists++] = vars[0];
    }
}

/* after a finit: free the lists it left (the original frees only the
   last, and none without graphics.library, #891), as its
   memory balance shows */
static void freeleft(void)
{
    int i, n = 0;

    if (!membal)
        n = nlists;
    else if (nlists > 1 && membal == -(LONG)RWE_MEMSIZE(sizes[nlists - 1]))
        n = nlists - 1;
    for (i = 0; i < n; i++)
        FreeMem((APTR)lists[i], sizes[i]);
    nlists = 0;
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
    KEEPS("InitView (no init)", d7|a2|a3);
    KEEPS("finit (no graphics)", d0-d7|a0-a3|a6);
    gfx = OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    if (!gfx) {
        check("ridisplaylib: graphics.library 39", FALSE);
        return rwe_done(LIB);
    }
    strcpy(VEC, "");
    call2(INIT, "init", 0, 0, 0, 0);
    initview("InitView", 256, 8, 8, 256);
    initview("InitView", 200, 5, 0, 32);
    initview("InitView", 0, 0, 0, 0);
    initview("InitView", 1000, 8, 8, 2);
    initview("InitView", 3000, 1, 100, 0xffff);
    strcpy(VEC, "");
    call2(VIEWBITMAP, "ViewBitmap", 0x12345678, 0, 0, 0);
    call2(FINIT, "finit", 0, 0, 0, 0);
    BOTH("freed", vars && !vars[0] && !vars[1]);
    freeleft();
    call2(FINIT, "finit (again)", 0, 0, 0, 0);
    call2(INIT, "init (again)", 0, 0, 0, 0);
    call2(FINIT, "finit (no list)", 0, 0, 0, 0);
    initview("InitView (no init)", 100, 2, 1, 4);
    strcpy(VEC, "");
    call2(FINIT, "finit (no graphics)", 0, 0, 0, 0);
    freeleft();
    CloseLibrary(gfx);
    return rwe_done(LIB);
}
