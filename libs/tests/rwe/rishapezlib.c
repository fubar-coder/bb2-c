/*
 * rwe rishapezlib - CludgeAcid, PeekByte, SetWinDrawShapeZ: the C build
 * against the reference build of rwelibsrc/shapezlib.bb (blitz2 2.10),
 * method of rwecmp.h.
 *
 *   rishapezlib DIR     (DIR with rishapezlib .obj/.orig)
 *
 * Compared: d0 and a4 after the calls, the RastPort fields the window
 * command sets (the pattern by its words), the kept registers.  Covered:
 * CludgeAcid 1 before anything was saved (a4 = 0), CludgeAcid 0/1 with
 * garbage above the low byte (only d0.b counts); PeekByte inline (same
 * length) and its code called as a subroutine - BUG(orig): it runs into
 * CludgeAcid, a byte 1 loads a4, any other byte saves it; bytes 0, 1, $80,
 * $FF; SetWinDrawShapeZ with a window object whose Window/RastPort are
 * memory here, mode 1, $8000 and garbage above the word, mode 0; init and
 * finit.
 */
#include "rwecmp.h"
#include <intuition/intuition.h>

#define LIB "rishapezlib"
enum { CLUDGE, PEEK, SETDRAW, FINIT, INIT };

static UBYTE bytes[2][8];
static struct Window win[2];
static struct RastPort rp[2];
static struct Window *winobj[2][2];     /* windowslib objects: the Window */

static void cludge(ULONG d0, ULONG a4)
{
    ROUT = "CludgeAcid";
    sprintf(VEC, "d0 $%08lX a4 $%08lX", d0, a4);
    DO2(ENT(LIB, CLUDGE, 0), R[cur].d[0] = d0; R[cur].a[4] = a4);
    SAME("a4", R[cur].a[4]);
    SAME("d0", R[cur].d[0]);
}

/* PeekByte as a subroutine (the inline code + what follows it) */
static void peek(int i, ULONG a4)
{
    ROUT = "PeekByte (JSR)";
    sprintf(VEC, "byte $%02X a4 $%08lX", bytes[0][i], a4);
    DO2(ENT(LIB, PEEK, 0) + 4, R[cur].d[0] = (ULONG)&bytes[cur][i]; R[cur].a[4] = a4);
    SAME("d0.w", (UWORD)R[cur].d[0]);
    SAME("d0 upper", (R[cur].d[0] >> 16) == ((ULONG)&bytes[cur][i] >> 16));
    SAME("a0", R[cur].a[0] - (ULONG)bytes[cur]);
    SAME("a4", R[cur].a[4]);
}

static void setdraw(ULONG mode)
{
    ROUT = "SetWinDrawShapeZ";
    sprintf(VEC, "mode $%08lX", mode);
    for (cur = 0; cur < 2; cur++) {
        memset(&rp[cur], 0x77, sizeof rp[cur]);
        win[cur].RPort = &rp[cur];
        winobj[cur][0] = &win[cur];
    }
    DO2(ENT(LIB, SETDRAW, 0), R[cur].d[0] = 0x5a000003; R[cur].d[1] = mode;
        R[cur].a[0] = (ULONG)winobj[cur]);
    SAME("AreaPtrn set", rp[cur].AreaPtrn != NULL);
    if (rp[0].AreaPtrn && rp[1].AreaPtrn) {
        SAME("AreaPtrn[0]", rp[cur].AreaPtrn[0]);
        SAME("AreaPtrn[1]", rp[cur].AreaPtrn[1]);
    }
    SAME("AreaPtSz", rp[cur].AreaPtSz);
    SAME("DrawMode", rp[cur].DrawMode);
    SAME("AOlPen", rp[cur].AOlPen);
    SAME("TmpRas", (ULONG)rp[cur].TmpRas);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    static const UBYTE vals[] = { 0, 1, 0x80, 0xff, 1, 0x7e };
    int i;

    if (argc < 2) {
        printf("usage: rishapezlib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    for (cur = 0; cur < 2; cur++)
        memcpy(bytes[cur], vals, sizeof vals);

    ROUT = "init";
    strcpy(VEC, "");
    DO2(ENT(LIB, INIT, 0), (void)0);

    cludge(0x5a000001, 0x11111111);     /* nothing saved: a4 = 0 */
    cludge(0x5a000100, 0x22222222);     /* low byte 0: save */
    cludge(0x5a000101, 0x33333333);     /* low byte 1: restore */
    cludge(0x5a000002, 0x44444444);     /* save */
    cludge(0x00000001, 0x55555555);

    ROUT = "PeekByte";
    strcpy(VEC, "inline");
    BOTH("inline", W(ENT(LIB, PEEK, 0)) == 0xa000);
    SAME("inline length", W(ENT(LIB, PEEK, 0) + 2));
    /* BUG(orig): into CludgeAcid */
    peek(0, 0x66666666);                /* 0: saves a4 */
    peek(1, 0x77777777);                /* 1: a4 = $66666666 */
    peek(2, 0x88888888);
    peek(4, 0x99999999);
    peek(3, 0xaaaaaaaa);
    peek(4, 0xbbbbbbbb);
    cludge(0x5a000001, 0xcccccccc);

    setdraw(0x5a000001);
    setdraw(0);
    setdraw(0x5a008000);
    setdraw(0x5a010000);                /* word 0: cleared */

    ROUT = "finit";
    strcpy(VEC, "");
    DO2(ENT(LIB, FINIT, 0), (void)0);
    i = rwe_done(LIB);
    return i;
}
