/*
 * rwe riarrayslib - FillArray: the C build or the reference build of
 * rwelibsrc/ArraysLib.bb (blitz2 2.10), one set as LIBSET says, observed
 * as cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_riarrayslib.txt).
 *
 *   riarrayslib DIR     (DIR with riarrayslib .obj/.orig)
 *
 * Both forms are inline routines: observed to be inline ($A000) with
 * their length, then their code (behind the 4 bytes) is called like a
 * subroutine.  The compiler's a2 stack is built here: the type word and,
 * on top, the end of the array.  Observed: the array bytes (and the
 * guard bytes around them), d0-d2/a0-a2 after the call (a0/a1/a2
 * relative to the buffers), the kept registers.  Covered: byte,
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
static UBYTE buf[BUF];
static UBYTE stk[8];                    /* the a2 stack: type.w, end.l */

/* FillArray (form 0: value, 1: zero) on the elements [off, off + n*size)
   of buf with the type word `type` */
static void fill(int form, UWORD type, int size, int off, int n, ULONG value)
{
    ULONG d;
    sprintf(VEC, "form %d type $%04X size %d off %d n %d value $%08lX", form, type, size, off,
            n, value);
    for (d = 0; d < BUF; d++)
        buf[d] = (UBYTE)(rnd() >> 8);
    *(UWORD *)stk = type;
    *(ULONG *)(stk + 2) = (ULONG)buf + off + n * size;
    ROUT = form ? "FillArray zero" : "FillArray";
    DO1(ENT(LIB, FILL, form) + 4, R.d[0] = (ULONG)buf + off; R.d[1] = value;
        R.a[2] = (ULONG)stk + 6);
    OBSBYTES("array", buf, BUF);
    OBS("d0", R.d[0] - (ULONG)buf);
    OBS("d1", R.d[1]);
    OBS("d2", R.d[2]);
    OBS("a0", R.a[0] - (ULONG)buf);
    OBS("a1", R.a[1] - (ULONG)buf);
    OBS("a2", R.a[2] - (ULONG)stk);
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
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d0-d7|a0-a3|a6);
    KEEPS("FillArray", d0|d1|d3-d7|a3|a6);
    KEEPS("FillArray zero", d0|d1|d3-d7|a3|a6);
    KEEPS("finit", d0-d7|a0-a3|a6);

    ROUT = "init";
    DO1(ENT(LIB, INIT, 0), (void)0);

    for (form = 0; form < 2; form++) {
        ROUT = "FillArray";
        sprintf(VEC, "form %d inline", form);
        BOTH("inline", W(ENT(LIB, FILL, form)) == 0xa000);
        OBS("inline length", W(ENT(LIB, FILL, form) + 2));
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
    DO1(ENT(LIB, FINIT, 0), (void)0);
    return rwe_done(LIB);
}
