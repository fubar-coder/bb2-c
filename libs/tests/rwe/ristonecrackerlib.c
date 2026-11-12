/*
 * rwe ristonecrackerlib - STCLength, STCStart, STCSafeLen, STCLoad: the C
 * build or the CD original (BSS2 RISTONECRACKERLIB.OBJ), one set as LIBSET
 * says, observed as cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_ristonecrackerlib.txt).
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
 * STCLoad loads the file from RAM: and unpacks it in place; observed
 * are the result, the whole buffer (also checked against the expected
 * data), the header functions and the kept registers.
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

static UBYTE *buf;                      /* STCLoad's buffer */

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

static void headers(const UBYTE *a)
{
    static const char *rn[] = { "STCLength", "STCStart", "STCSafeLen" };
    int e;
    for (e = 0; e < 3; e++) {
        ROUT = rn[e];
        DO1(ENT(LIB, e, 0), R.d[0] = (ULONG)a);
        OBS("d0", e == 1 && R.d[0] ? R.d[0] - (ULONG)a : R.d[0]);
        if (e == 1)
            BOTH("STCStart = 0 or address + offset", R.d[0] == 0
                 || R.d[0] == (ULONG)a + *(const ULONG *)(a + 4));
    }
}

/* STCLoad FNAME into buf (filled with `fill` first); `expect`: the data
   must unpack to out[] */
static void load(int expect, UBYTE fill)
{
    static const char name[] = FNAME;
    ULONG off = *(ULONG *)(file + 4);
    memset(buf, fill, BUFSZ);
    ROUT = "STCLoad";
    DO1(ENT(LIB, 3, 0), R.d[0] = (ULONG)name; R.d[1] = (ULONG)buf);
    OBS("d0", R.d[0] ? R.d[0] - (ULONG)buf : 0);
    OBSBYTES("buffer", buf, BUFSZ);
    if (expect) {
        BOTH("d0 = start", R.d[0] == (ULONG)buf + off);
        BOTH("data", memcmp(buf + off, out, N) == 0);
        BOTH("header kept", memcmp(buf, file, 16) == 0);
    }
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    static const UBYTE notstc[16] = "S403\0\0\0\x10\0\0\0\x20\0\0\0\x04";
    int i;

    if (argc < 2) {
        printf("usage: ristonecrackerlib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("STCLength", d1-d7|a1-a3|a6);
    KEEPS("STCStart", d1-d7|a1-a3|a6);
    KEEPS("STCSafeLen", d1-d7|a1-a3|a6);
    KEEPS("STCLoad", a2|a3);
    buf = AllocMem(BUFSZ, MEMF_PUBLIC);
    /* init: dos.library */
    markregs(&R);
    callstk(entry_form(getlib(&loaded, LIB), 4, 0), &R, NULL, 0, &POP);

    /* the header functions on other data: 0 (the binary's checks) */
    strcpy(VEC, "S403");
    headers(notstc);

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
        headers(file);
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
        DO1(ENT(LIB, 3, 0), R.d[0] = (ULONG)dir; R.d[1] = (ULONG)buf);
        OBS("d0", R.d[0]);
    }

    FreeMem(buf, BUFSZ);
    return rwe_done(LIB);
}
