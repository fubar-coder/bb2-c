/*
 * rwe ristonecrackerlib - STCLength, STCStart, STCSafeLen, STCLoad: the C
 * build against the CD original (BSS2 RISTONECRACKERLIB.OBJ), method of
 * rwecmp.h.
 *
 *   ristonecrackerlib DIR     (DIR with ristonecrackerlib .obj/.orig)
 *
 * StoneCracker 4.04 ("S404") files are made here: a random sequence of
 * the decruncher's codes (literal bytes, literal runs of 14..45 bytes,
 * matches of 2..3, 4..7, 8..22 and 23+ bytes - the last with 255
 * continuation bytes - at near (5 bits), middle (9 bits) and far
 * distances (0..16 bits, the header's word)) is written as the bit
 * stream the decruncher reads (MSB first, words backwards from the
 * trailer), and the data it must give are worked out at the same time.
 * STCLoad loads the file from RAM: and unpacks it in place; compared
 * are the result, the whole buffer (with the expected data and between
 * the sets), the header functions and the registers the original keeps.
 * Covered: every code of the decruncher, bit buffers of 0..16 bits at
 * the start, literal bytes and runs on every bit position (aligned and
 * across words), the three header functions on S404 data and on other
 * data (0, the S404 check of the binary), STCLoad with a missing file and
 * with a directory (OpenFromLock fails), and with a file that is no S404
 * file (BUG(orig) #836: unpacked anyway - a harmless one: offset and lengths
 * chosen so).  Not covered: corrupt data (writes anywhere), more than 16
 * far offset bits (the DIFF of the run table), a far offset that is
 * negative as a word (16 bits >= $8000: reads before the destination).
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "ristonecrackerlib"
#define MAXOUT 20000
#define BUFSZ (2 * MAXOUT + 16384)
#define FNAME "RAM:rwe_stc.stc"

#include "s404.h"

static UBYTE *buf[2];                   /* STCLoad's buffer per set */

/* ------------------------------------------------------------ the tests */

static void savefile(const UBYTE *d, ULONG len)
{
    BPTR fh = Open((STRPTR)FNAME, MODE_NEWFILE);
    if (fh) {
        Write(fh, (APTR)d, (LONG)len);
        Close(fh);
    }
    check("STC file written", fh != 0);
}

static void headers(const UBYTE *a[2])
{
    static const char *rn[] = { "STCLength", "STCStart", "STCSafeLen" };
    int e;
    for (e = 0; e < 3; e++) {
        ROUT = rn[e];
        DO2(ENT(LIB, e, 0), R[cur].d[0] = (ULONG)a[cur]);
        SAME("d0", e == 1 && R[cur].d[0] ? R[cur].d[0] - (ULONG)a[cur] : R[cur].d[0]);
        if (e == 1)
            BOTH("STCStart = 0 or address + offset", R[cur].d[0] == 0
                 || R[cur].d[0] == (ULONG)a[cur] + *(const ULONG *)(a[cur] + 4));
    }
}

/* STCLoad FNAME into buf (filled with `fill` first); `expect`: the data
   must unpack to out[] */
static void load(int expect, UBYTE fill)
{
    static const char name[] = FNAME;
    ULONG off = *(ULONG *)(file + 4);
    int s;
    for (s = 0; s < 2; s++)
        memset(buf[s], fill, BUFSZ);
    ROUT = "STCLoad";
    DO2(ENT(LIB, 3, 0), R[cur].d[0] = (ULONG)name; R[cur].d[1] = (ULONG)buf[cur]);
    SAME("d0", R[cur].d[0] ? R[cur].d[0] - (ULONG)buf[cur] : 0);
    SAMEBYTES("buffer", buf, BUFSZ);
    if (expect) {
        BOTH("d0 = start", R[cur].d[0] == (ULONG)buf[cur] + off);
        BOTH("data", memcmp(buf[cur] + off, out, N) == 0);
        BOTH("header kept", memcmp(buf[cur], file, 16) == 0);
    }
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    static const UBYTE notstc[16] = "S403\0\0\0\x10\0\0\0\x20\0\0\0\x04";
    const UBYTE *a[2];
    int s, i;

    if (argc < 2) {
        printf("usage: ristonecrackerlib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    for (s = 0; s < 2; s++) {
        buf[s] = AllocMem(BUFSZ, MEMF_PUBLIC);
        /* init: dos.library */
        markregs(&R[s]);
        cur = s;
        callstk(entry_form(getlib(&sets[s], LIB), 4, 0), &R[s], NULL, 0, &POP[s]);
    }

    /* the header functions on other data: 0 (the binary's checks) */
    strcpy(VEC, "S403");
    for (s = 0; s < 2; s++)
        a[s] = notstc;
    headers(a);

    /* unpacking: random code sequences */
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
        if (off + N > BUFSZ) {
            check("test data fits", FALSE);
            continue;
        }
        sprintf(VEC, "#%d: %lu bytes, %lu words, d7 %d, d2 %d, kinds $%X, offset %lu", i, N,
                nwords, firstb, d2, kinds, off);
        for (s = 0; s < 2; s++)
            a[s] = file;
        headers(a);
        savefile(file, filelen);
        load(1, (UBYTE)(i * 13));
    }

    /* no S404 file, unpacked anyway (BUG(orig), #836): a header whose words make
       a short literal stream inside the buffer */
    strcpy(VEC, "S403 file");
    make(100, 16, 4, 0, 0x01);
    memcpy(file, "S403", 4);
    *(ULONG *)(file + 4) = (filelen + 3) & ~3UL;
    savefile(file, filelen);
    load(1, 0x11);

    strcpy(VEC, "missing file");
    DeleteFile((STRPTR)FNAME);
    load(0, 0x22);

    strcpy(VEC, "a directory");
    {
        static const char dir[] = "RAM:";
        ROUT = "STCLoad";
        DO2(ENT(LIB, 3, 0), R[cur].d[0] = (ULONG)dir; R[cur].d[1] = (ULONG)buf[cur]);
        SAME("d0", R[cur].d[0]);
    }

    for (s = 0; s < 2; s++)
        FreeMem(buf[s], BUFSZ);
    return rwe_done(LIB);
}
