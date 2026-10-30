/*
 * rwe riarrayslib - FillArray: the C build against the reference build of
 * rwelibsrc/ArraysLib.bb (blitz2 2.10), method of rwecmp.h.
 *
 *   riarrayslib DIR     (DIR with riarrayslib .obj/.orig)
 *
 * Both forms are inline routines: checked to be inline ($A000) with the
 * same length, then their code (behind the 4 bytes) is called like a
 * subroutine.  The compiler's a2 stack is built here: the type word and,
 * on top, the end of the array.  Compared: the array bytes (and the
 * guard bytes around them), d0-d2/a0-a2 after the call (a0/a1/a2
 * relative to the set's buffers), the kept registers.  Covered: byte,
 * word and long arrays of 1..37 elements at even and odd offsets
 * (bytes), values with garbage above the element size, both forms; the
 * types that do nothing (0, quick, float, string, 8, and a type word
 * with byte/word/long only in its upper byte); init and finit.  Not
 * covered: an end that is not reached exactly (BUG(orig) #882: runs through
 * memory).
 */
#include "rwecmp.h"

#define LIB "riarrayslib"
enum { FILL, FINIT, INIT };

#define BUF 200
static UBYTE buf[2][BUF];
static UBYTE stk[2][8];                 /* the a2 stack: type.w, end.l */
static UBYTE before[BUF];

/* FillArray (form 0: value, 1: zero) on the elements [off, off + n*size)
   of buf with the type word `type` */
static void fill(int form, UWORD type, int size, int off, int n, ULONG value)
{
    ULONG d;
    sprintf(VEC, "form %d type $%04X size %d off %d n %d value $%08lX", form, type, size, off,
            n, value);
    for (d = 0; d < BUF; d++)
        before[d] = (UBYTE)(rnd() >> 8);
    for (cur = 0; cur < 2; cur++) {
        memcpy(buf[cur], before, BUF);
        *(UWORD *)stk[cur] = type;
        *(ULONG *)(stk[cur] + 2) = (ULONG)buf[cur] + off + n * size;
    }
    ROUT = form ? "FillArray zero" : "FillArray";
    DO2(ENT(LIB, FILL, form) + 4, R[cur].d[0] = (ULONG)buf[cur] + off; R[cur].d[1] = value;
        R[cur].a[2] = (ULONG)stk[cur] + 6);
    SAMEBYTES("array", buf, BUF);
    SAME("d0", R[cur].d[0] - (ULONG)buf[cur]);
    SAME("d1", R[cur].d[1]);
    SAME("d2", R[cur].d[2]);
    SAME("a0", R[cur].a[0] - (ULONG)buf[cur]);
    SAME("a1", R[cur].a[1] - (ULONG)buf[cur]);
    SAME("a2", R[cur].a[2] - (ULONG)stk[cur]);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    static const UWORD none[] = { 0, 4, 5, 7, 8, 0x0300, 0x0100, 0xff00 };
    int form, size, i, n;

    if (argc < 2) {
        printf("usage: riarrayslib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;

    ROUT = "init";
    DO2(ENT(LIB, INIT, 0), (void)0);

    for (form = 0; form < 2; form++) {
        ROUT = "FillArray";
        sprintf(VEC, "form %d inline", form);
        BOTH("inline", W(ENT(LIB, FILL, form)) == 0xa000);
        SAME("inline length", W(ENT(LIB, FILL, form) + 2));
        for (size = 1; size <= 4; size <<= 1)
            for (i = 0; i < 12; i++) {
                n = i < 3 ? i + 1 : (int)(rnd() % 37) + 1;
                fill(form, size == 4 ? 3 : size, size, (size == 1 ? (int)(rnd() % 3) : 2 * (i & 1))
                     + 4, n, rnd());
            }
        /* the high byte of the type word is ignored (CMP.b) */
        fill(form, 0xab02, 2, 6, 9, 0x12345678);
        for (i = 0; i < (int)(sizeof none / sizeof none[0]); i++)
            fill(form, none[i], 4, 8, 5, 0x5a5a5a5a);
    }

    ROUT = "finit";
    strcpy(VEC, "");
    DO2(ENT(LIB, FINIT, 0), (void)0);
    return rwe_done(LIB);
}
