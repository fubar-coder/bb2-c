/*
 * rwe ridisplaylib - InitView, ViewBitmap: the C build against the
 * reference build of rwelibsrc/mydisplaylib.bb (ridisplaylib.orig),
 * method of rwecmp.h.
 *
 *   ridisplaylib DIR     (DIR with ridisplaylib .obj/.orig)
 *
 * The library allocates a copper list per InitView but installs nothing:
 * the display is not touched.  Compared after every call: d0 (InitView:
 * the list, as "is the list the library keeps"), the variables viewsize
 * and cop (found in both libraries by the list InitView returns), the
 * list's 8 words (the words at the absolute addresses $8E, $2C81, $90,
 * $2CC1, $92, $38, $94, $D0, BUG(orig): equal to those words, and equal
 * between the sets for the ones in the vector table), the change of
 * graphics.library's open count and of the free chip memory, and the
 * registers the original keeps.  The lists InitView leaks (only the last
 * one is kept, BUG(orig)) are freed by the test.
 *
 * Covered: init, InitView (sizes with marker upper words, the colour
 * count ignored, BUG(orig)), ViewBitmap (no code), finit with a list,
 * finit again (nothing: graphics.library closed), init and finit
 * without a list.  Not covered: InitView without chip memory or with a
 * size that is negative as a word (both write to address 0).
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "ridisplaylib"
enum { INITVIEW, VIEWBITMAP, INIT, FINIT };

static struct Library *gfx;
static LONG opened[2], chipfree[2];
static ULONG *vars[2];                  /* viewsize, cop */
static ULONG oldcop[2], oldsize[2];     /* the list before (leaked) */
static ULONG cop[2];

static ULONG findvars(struct lib *l, ULONG list)
{
    ULONG i;
    for (i = 4; i + 4 <= l->size; i += 2)
        if (L(l->code + i) == list)
            return (ULONG)(l->code + i - 4);
    return 0;
}

/* one call on both sets: the open count and free chip memory around it */
static void call2(int idx, const char *name, ULONG d0, ULONG d1, ULONG d2, ULONG d3)
{
    ROUT = name;
    for (cur = 0; cur < 2; cur++) {
        APTR fn = ENT(LIB, idx, 0);
        LONG cnt = gfx->lib_OpenCnt, mem;
        markregs(&R[cur]);
        R[cur].d[0] = d0;
        R[cur].d[1] = d1;
        R[cur].d[2] = d2;
        R[cur].d[3] = d3;
        IN[cur] = R[cur];
        NARGS = 0;
        mem = (LONG)AvailMem(MEMF_CHIP);
        CCR[cur] = callstk(fn, &R[cur], ARGS[cur], NARGS, &POP[cur]);
        chipfree[cur] = (LONG)AvailMem(MEMF_CHIP) - mem;
        opened[cur] = gfx->lib_OpenCnt - cnt;
    }
    preserve();
    SAME("graphics.library open count", opened[cur]);
    SAME("free chip memory", chipfree[cur]);
    if (vars[0] && vars[1]) {
        SAME("viewsize", vars[cur][0]);
        SAME("cop", vars[cur][1] != 0);
    }
}

static void initview(UWORD h, UWORD d, UWORD s, UWORD c)
{
    static const ULONG addr[8] = { 0x8e, 0x2c81, 0x90, 0x2cc1, 0x92, 0x38, 0x94, 0xd0 };
    int i;
    char buf[40];

    sprintf(VEC, "%u,%u,%u,%u", h, d, s, c);
    for (cur = 0; cur < 2; cur++) {
        oldcop[cur] = vars[cur] ? vars[cur][1] : 0;
        oldsize[cur] = vars[cur] ? vars[cur][0] : 0;
    }
    call2(INITVIEW, "InitView", 0xa5a50000UL | h, 0x5a5a0000UL | d, 0x12340000UL | s,
          0xfedc0000UL | c);
    for (cur = 0; cur < 2; cur++) {
        cop[cur] = R[cur].d[0];
        if (!vars[cur])
            vars[cur] = (ULONG *)findvars(getlib(&sets[cur], LIB), cop[cur]);
    }
    BOTH("variables found", vars[cur] != NULL);
    if (!vars[0] || !vars[1])
        return;
    BOTH("d0 = cop", cop[cur] && vars[cur][1] == cop[cur]);
    for (i = 0; i < 8; i++) {
        sprintf(buf, "word %d = ($%lX)", i, addr[i]);
        BOTH(buf, ((UWORD *)cop[cur])[i] == *(volatile UWORD *)addr[i]);
        if (addr[i] & 1)
            continue;
        sprintf(buf, "word %d", i);
        SAME(buf, ((UWORD *)cop[cur])[i]);
    }
    for (cur = 0; cur < 2; cur++)
        if (oldcop[cur])
            FreeMem((APTR)oldcop[cur], oldsize[cur]);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };

    if (argc < 2) {
        printf("usage: ridisplaylib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
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
    BOTH("freed", vars[cur] && !vars[cur][0] && !vars[cur][1]);
    call2(FINIT, "finit (again)", 0, 0, 0, 0);
    call2(INIT, "init (again)", 0, 0, 0, 0);
    call2(FINIT, "finit (no list)", 0, 0, 0, 0);
    CloseLibrary(gfx);
    return rwe_done(LIB);
}
