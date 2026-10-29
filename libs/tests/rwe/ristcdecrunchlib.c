/*
 * rwe ristcdecrunchlib - STCDecrunch source[,dest]: the C build against
 * the reference build of rwelibsrc/stcdecrunchlib.bb (blitz2 2.10),
 * method of rwecmp.h.
 *
 *   ristcdecrunchlib DIR     (DIR with ristcdecrunchlib .obj/.orig)
 *
 * The S404 files are made by s404.h (random sequences of every code of
 * the decruncher, 0..16 far offset bits, first bit buffers of 0..16
 * bits).  Form 1 unpacks a copy of the file in a buffer in place (to
 * buffer + offset), form 2 from the file to another buffer; compared are
 * d0 (form 1: the address of the data, relative to the buffer; form 2:
 * 1), the whole buffers (with the expected data and between the sets)
 * and the registers the original keeps.  Both forms with other data
 * (0, nothing written).  Not covered: corrupt data (writes anywhere),
 * more than 16 far offset bits (the DIFF of the run table), a far offset
 * that is negative as a word.
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "ristcdecrunchlib"
#define MAXOUT 20000
#define BUFSZ (2 * MAXOUT + 16384)

#include "s404.h"

static UBYTE *buf[2];                   /* the file (form 1: in place) */
static UBYTE *dst[2];                   /* form 2's destination */

/* form 1: the file in buf (the rest filled with `fill`), unpacked in
   place; `expect`: an S404 file that must give out[] */
static void inplace(int expect, UBYTE fill)
{
    ULONG off = *(ULONG *)(file + 4);
    int s;
    for (s = 0; s < 2; s++) {
        memset(buf[s], fill, BUFSZ);
        memcpy(buf[s], file, filelen);
    }
    ROUT = "STCDecrunch source";
    DO2(ENT(LIB, 0, 0), R[cur].d[0] = (ULONG)buf[cur]);
    SAME("d0", R[cur].d[0] ? R[cur].d[0] - (ULONG)buf[cur] : 0);
    SAMEBYTES("buffer", buf, BUFSZ);
    if (expect) {
        BOTH("d0 = start", R[cur].d[0] == (ULONG)buf[cur] + off);
        BOTH("data", memcmp(buf[cur] + off, out, N) == 0);
        BOTH("header kept", memcmp(buf[cur], file, 16) == 0);
    } else {
        BOTH("d0 = 0", R[cur].d[0] == 0);
        BOTH("buffer unchanged", memcmp(buf[cur], file, filelen) == 0
             && buf[cur][filelen] == fill && buf[cur][BUFSZ - 1] == fill);
    }
}

/* form 2: the file in buf, unpacked to dst + `at` (dst filled with
   `fill`) */
static void todest(int expect, UBYTE fill, ULONG at)
{
    int s;
    for (s = 0; s < 2; s++) {
        memset(buf[s], 0x5a, BUFSZ);
        memcpy(buf[s], file, filelen);
        memset(dst[s], fill, BUFSZ);
    }
    ROUT = "STCDecrunch source,dest";
    DO2(ENT(LIB, 0, 1), R[cur].d[0] = (ULONG)buf[cur]; R[cur].d[1] = (ULONG)dst[cur] + at);
    SAME("d0", R[cur].d[0]);
    SAMEBYTES("destination", dst, BUFSZ);
    SAMEBYTES("source", buf, BUFSZ);
    BOTH("source unchanged", memcmp(buf[cur], file, filelen) == 0);
    if (expect) {
        BOTH("d0 = 1", R[cur].d[0] == 1);
        BOTH("data", memcmp(dst[cur] + at, out, N) == 0);
        BOTH("below the data unchanged", at == 0 || dst[cur][at - 1] == fill);
        BOTH("above the data unchanged", dst[cur][at + N] == fill);
    } else {
        BOTH("d0 = 0", R[cur].d[0] == 0);
        BOTH("destination unchanged", dst[cur][at] == fill && dst[cur][BUFSZ - 1] == fill);
    }
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    int s, i;

    if (argc < 2) {
        printf("usage: ristcdecrunchlib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    for (s = 0; s < 2; s++) {
        buf[s] = AllocMem(BUFSZ, MEMF_PUBLIC);
        dst[s] = AllocMem(BUFSZ, MEMF_PUBLIC);
    }

    /* random code sequences, both forms */
    for (i = 0; i < 40; i++) {
        static const unsigned kindset[] = { 0xff, 0x01, 0x03, 0x05, 0x09, 0x11, 0x21, 0x41, 0x81 };
        ULONG n = i < 9 ? 200 + 37 * i : pick(1, MAXOUT);
        int firstb = i % 17;
        int d2 = i % 3 == 0 ? 16 : (int)pick(0, 15);
        unsigned kinds = kindset[i % 9];
        ULONG off;
        make(n, firstb, d2, 0, kinds);
        off = (filelen + 3) & ~3UL;     /* above the packed data */
        if (i & 1)
            off += pick(0, 99);
        *(ULONG *)(file + 4) = off;
        if (off + N > BUFSZ || N + 200 > BUFSZ) {
            check("test data fits", FALSE);
            continue;
        }
        sprintf(VEC, "#%d: %lu bytes, %lu words, d7 %d, d2 %d, kinds $%X, offset %lu", i, N,
                nwords, firstb, d2, kinds, off);
        inplace(1, (UBYTE)(i * 13));
        todest(1, (UBYTE)(i * 7 + 1), i & 1 ? pick(1, 199) : 0);
    }

    /* no S404 file: 0, nothing unpacked (both forms check the header) */
    strcpy(VEC, "S403");
    make(100, 16, 4, 0, 0x01);
    memcpy(file, "S403", 4);
    *(ULONG *)(file + 4) = (filelen + 3) & ~3UL;
    inplace(0, 0x11);
    todest(0, 0x22, 10);

    for (s = 0; s < 2; s++) {
        FreeMem(buf[s], BUFSZ);
        FreeMem(dst[s], BUFSZ);
    }
    return rwe_done(LIB);
}
