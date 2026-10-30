/*
 * rwe risoundlib - CludgeSound: the C build against the CD original (BSS2
 * RISOUNDLIB.OBJ), method of rwecmp.h.
 *
 *   risoundlib DIR     (DIR with memlib, risoundlib .obj/.orig)
 *
 * IFF 8SVX files are built in memory (one copy per set, the same bytes)
 * and given to CludgeSound with a sound object (16 bytes + guards) per
 * set; no audio is played.  Compared: d0, the object's period, length and
 * loop fields and its guards, the sample (copied into chip memory: the
 * copy has the BODY's bytes, the pointer is checked on each set: chip
 * memory, or the old pointer when CludgeSound fails), the file unchanged
 * and the registers the original keeps; memlib's memory all back at the
 * end.  Covered: the period (several rates, the signed subtraction loop),
 * BODY behind further chunks (the 7th chunk found, the 8th not), no
 * "8SVX", no "VHDR", a VHDR of another length (BUG(orig) #832: the rate is
 * read at VHDR + length), an odd chunk with its pad byte (BUG(orig) #828: the
 * walk misses BODY) and without it (found), a BODY longer than 65536
 * bytes (BUG(orig) #829: (length - 1) & $FFFF + 1 bytes copied, the length in
 * words of the low word), the old sample freed and the stale pointer on a
 * failure (BUG(orig), #831), an odd BODY length.  Not covered: rate 0 (never
 * returns), an empty BODY (copies 65536 bytes into an 8 byte block).
 */
#include "rwecmp.h"
#include <exec/memory.h>

#define LIB "risoundlib"
#define FILESZ 80000
#define OBJSZ 32
#define OFF 8                           /* the object at obj[OFF] */

static UBYTE *file[2], ref[FILESZ];
static UBYTE obj[2][OBJSZ];
static ULONG flen;                      /* bytes of the file built */

static void putl(UBYTE *p, ULONG v)
{
    p[0] = (UBYTE)(v >> 24);
    p[1] = (UBYTE)(v >> 16);
    p[2] = (UBYTE)(v >> 8);
    p[3] = (UBYTE)v;
}

static void chunk(const char *id, ULONG len, int pad, UBYTE seed)
{
    ULONG i;
    memcpy(ref + flen, id, 4);
    putl(ref + flen + 4, len);
    flen += 8;
    for (i = 0; i < len; i++)
        ref[flen + i] = (UBYTE)(seed + i * 7 + (i >> 8));
    flen += len;
    if (pad && (len & 1))
        ref[flen++] = 0;
}

/* FORM <form> VHDR(vlen, rate) then the chunks of `rest` before BODY */
static void begin(const char *form, const char *vhdr, ULONG vlen, UWORD rate)
{
    memset(ref, 0x5a, FILESZ);
    flen = 0;
    memcpy(ref, "FORM", 4);
    memcpy(ref + 8, form, 4);
    flen = 12;
    chunk(vhdr, vlen, 1, 0);
    if (vlen >= 14) {
        memset(ref + 12 + 8, 0, vlen);
        ref[12 + 8 + 12] = (UBYTE)(rate >> 8);  /* samplesPerSec */
        ref[12 + 8 + 13] = (UBYTE)rate;
        ref[12 + 8 + 14] = 1;                   /* octaves */
    }
    /* the word CludgeSound takes as the rate: VHDR + its length */
    if (vlen + 12 + 2 <= FILESZ) {
        ref[12 + vlen] = (UBYTE)(rate >> 8);
        ref[12 + vlen + 1] = (UBYTE)rate;
    }
}

static void finish(void)
{
    int s;
    putl(ref + 4, flen - 8);
    for (s = 0; s < 2; s++)
        memcpy(file[s], ref, FILESZ);
}

/* where BODY's data is in ref (0: none) */
static ULONG bodyat;

static void body(ULONG len, int pad)
{
    bodyat = flen + 8;
    chunk("BODY", len, pad, 0x33);
}

static ULONG old[2];                    /* the object's sample before the call */

static void cludge(ULONG snd)
{
    int s;
    for (s = 0; s < 2; s++)
        old[s] = *(ULONG *)(obj[s] + OFF);
    ROUT = "CludgeSound";
    DO2(ENT(LIB, 0, 0), R[cur].d[0] = snd; R[cur].d[1] = (ULONG)file[cur];
        R[cur].a[0] = (ULONG)(obj[cur] + OFF));
    SAME("d0", R[cur].d[0]);
    {
        const UBYTE *fl[2] = { obj[0] + OFF + 4, obj[1] + OFF + 4 };
        const UBYTE *gt[2] = { obj[0] + OFF + 16, obj[1] + OFF + 16 };
        SAMEBYTES("object fields", fl, 12);
        SAMEBYTES("guards before", obj, OFF);
        SAMEBYTES("guards behind", gt, OBJSZ - OFF - 16);
    }
    SAME("period", *(UWORD *)(obj[cur] + OFF + 4));
    SAME("words", *(UWORD *)(obj[cur] + OFF + 6));
    BOTH("file unchanged", memcmp(file[cur], ref, FILESZ) == 0);
    if (R[0].d[0] == 0) {
        BOTH("failed: the old pointer", *(ULONG *)(obj[cur] + OFF) == old[cur]);
    } else {
#define SMP (*(UBYTE **)(obj[cur] + OFF))
        ULONG len = ((ref[bodyat - 4] << 24 | ref[bodyat - 3] << 16 | ref[bodyat - 2] << 8
                      | ref[bodyat - 1]) - 1) & 0xffff;
        BOTH("sample in chip memory", SMP && (TypeOfMem(SMP) & MEMF_CHIP));
        BOTH("sample data", memcmp(SMP, ref + bodyat, len + 1) == 0);
#undef SMP
    }
}

/* the stale pointer of a failed call (BUG(orig), #831) must not be freed again */
static void forget(void)
{
    int s;
    for (s = 0; s < 2; s++)
        *(ULONG *)(obj[s] + OFF) = 0;
}

static void memlib(int idx)
{
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callstk(entry_form(getlib(&sets[cur], "memlib"), idx, 0), &R[cur], NULL, 0, &POP[cur]);
    }
}

int main(int argc, char **argv)
{
    static const char *names[] = { "memlib", LIB, NULL };
    static const UWORD rates[] = { 8363, 16000, 1, 2, 3579, 22050, 0x7fff, 0x8000, 0xffff };
    ULONG avail;
    unsigned i;
    int s;

    if (argc < 2) {
        printf("usage: risoundlib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    for (s = 0; s < 2; s++)
        file[s] = AllocMem(FILESZ, MEMF_PUBLIC);
    memset(obj, 0xa5, sizeof obj);
    forget();
    for (s = 0; s < 2; s++)
        memset(obj[s] + OFF + 4, 0x77, 12);

    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    ROUT = "memlib init";
    memlib(7);

    /* rates; each call frees the sample of the one before */
    for (i = 0; i < sizeof rates / sizeof rates[0]; i++) {
        sprintf(VEC, "rate %u", rates[i]);
        begin("8SVX", "VHDR", 20, rates[i]);
        body(1000 + i, 1);
        finish();
        cludge(0x10000 + i);
    }

    strcpy(VEC, "chunks before BODY");
    begin("8SVX", "VHDR", 20, 8363);
    chunk("NAME", 6, 1, 1);
    chunk("ANNO", 10, 1, 2);
    chunk("CHAN", 4, 1, 3);
    body(333, 1);
    finish();
    cludge(1);

    strcpy(VEC, "BODY the 7th chunk");
    begin("8SVX", "VHDR", 20, 11025);
    for (i = 0; i < 5; i++)
        chunk("ANNO", 2 * i, 1, (UBYTE)i);
    body(64, 1);
    finish();
    cludge(2);

    strcpy(VEC, "BODY the 8th chunk");
    begin("8SVX", "VHDR", 20, 11025);
    for (i = 0; i < 6; i++)
        chunk("ANNO", 2 * i + 2, 1, (UBYTE)i);
    body(64, 1);
    finish();
    cludge(3);                          /* fails: the freed sample stays */
    forget();

    strcpy(VEC, "not 8SVX");
    begin("AIFF", "VHDR", 20, 8363);
    body(100, 1);
    finish();
    cludge(4);

    strcpy(VEC, "no VHDR");
    begin("8SVX", "NAME", 20, 8363);
    body(100, 1);
    finish();
    cludge(5);

    strcpy(VEC, "after failures");
    begin("8SVX", "VHDR", 20, 8363);
    body(101, 1);
    finish();
    cludge(6);

    strcpy(VEC, "VHDR 24 bytes");
    begin("8SVX", "VHDR", 24, 8363);
    ref[12 + 24] = 0x12;                /* the word at VHDR + 24 */
    ref[12 + 25] = 0x34;
    body(50, 1);
    finish();
    cludge(7);

    strcpy(VEC, "odd chunk with pad byte");
    begin("8SVX", "VHDR", 20, 8363);
    chunk("ANNO", 5, 1, 9);
    /* the walk lands on the pad byte: id 00 'A' 'B' 00, length = 00 and
       the upper three bytes of $400 = 4, then zeros (id 0, length 0) up
       to the 7th chunk - BODY behind is missed (BUG(orig), #828) */
    i = flen + 8;
    chunk("AB\0\0", 0x400, 1, 0);
    memset(ref + i, 0, 0x400);
    body(60, 1);
    finish();
    cludge(13);
    forget();

    strcpy(VEC, "odd chunk without pad byte");
    begin("8SVX", "VHDR", 20, 8363);
    chunk("ANNO", 5, 0, 9);
    body(77, 1);
    finish();
    cludge(8);

    strcpy(VEC, "odd BODY length");
    begin("8SVX", "VHDR", 20, 4000);
    body(99, 1);
    finish();
    cludge(9);

    strcpy(VEC, "BODY 70000 bytes");
    begin("8SVX", "VHDR", 20, 8363);
    body(70000, 1);
    finish();
    cludge(10);

    strcpy(VEC, "BODY 65536 bytes");
    begin("8SVX", "VHDR", 20, 8363);
    body(65536, 1);
    finish();
    cludge(11);

    strcpy(VEC, "BODY 1 byte");
    begin("8SVX", "VHDR", 20, 8363);
    body(1, 1);
    finish();
    cludge(12);

    ROUT = "memlib finit";
    memlib(8);
    rwe_mem_check(LIB, avail, TRUE);
    for (s = 0; s < 2; s++)
        FreeMem(file[s], FILESZ);
    return rwe_done(LIB);
}
