/*
 * risoundcludge.h - the CludgeSound cases of the RWE userlibs risoundlib
 * and risoundlib2 (header only, after rwecmp.h; one routine in both
 * libraries, libs/userlibs/rwe/risoundlib.c and risoundlib2.c):
 * cs_cases(LIB, IDX) runs them on entry IDX of the library LIB of the run.
 * memlib's init must have run (CludgeSound allocates through it).
 *
 * IFF 8SVX files are built in memory and given to CludgeSound with a
 * sound object (16 bytes + guards); no audio is played.  Observed: d0,
 * the object's period, length and loop fields and its guards (risoundlib2
 * writes the rate into the one behind, #926), the sample (copied into
 * chip memory: the copy has all of the BODY's bytes, the pointer is
 * checked on each set: chip memory, or the old pointer when CludgeSound
 * fails), what the call did to this task's memory balance (the old sample
 * freed, the new one allocated), the file unchanged and the registers the
 * original keeps.  Covered: the period (several rates, the signed
 * subtraction loop), BODY behind further chunks (the 7th chunk found,
 * the 8th not), no "8SVX", no "VHDR", a VHDR of another length (the rate
 * read at VHDR + length, #832), an odd chunk with its pad byte (the walk
 * misses BODY, #828) and without it (found), a BODY longer than 65536
 * bytes (((length - 1) & $FFFF) + 1 bytes copied, the length in words of
 * the low word, #829) up to 131071 and 131072 bytes, the old sample freed
 * on a failure and the stale pointer kept (#831), an odd BODY length.
 * Not covered by the original: rate 0 (never returns, #830) and an empty
 * BODY (copies 65536 bytes into an 8 byte block, #829).
 */
#ifndef RISOUNDCLUDGE_H
#define RISOUNDCLUDGE_H

#include <exec/memory.h>

#define CS_FILESZ 140000
#define CS_OBJSZ 32
#define CS_OFF 8                        /* the object at cs_obj[CS_OFF] */

static UBYTE *cs_file, cs_ref[CS_FILESZ];
static UBYTE cs_obj[CS_OBJSZ];
static ULONG cs_flen;                   /* bytes of the file built */
static const char *cs_lib;              /* the library and entry under test */
static int cs_idx;

static void cs_putl(UBYTE *p, ULONG v)
{
    p[0] = (UBYTE)(v >> 24);
    p[1] = (UBYTE)(v >> 16);
    p[2] = (UBYTE)(v >> 8);
    p[3] = (UBYTE)v;
}

static void cs_chunk(const char *id, ULONG len, int pad, UBYTE seed)
{
    ULONG i;
    memcpy(cs_ref + cs_flen, id, 4);
    cs_putl(cs_ref + cs_flen + 4, len);
    cs_flen += 8;
    for (i = 0; i < len; i++)
        cs_ref[cs_flen + i] = (UBYTE)(seed + i * 7 + (i >> 8));
    cs_flen += len;
    if (pad && (len & 1))
        cs_ref[cs_flen++] = 0;
}

/* FORM <form> VHDR(vlen, rate) then the chunks of `rest` before BODY */
static void cs_begin(const char *form, const char *vhdr, ULONG vlen, UWORD rate)
{
    memset(cs_ref, 0x5a, CS_FILESZ);
    cs_flen = 0;
    memcpy(cs_ref, "FORM", 4);
    memcpy(cs_ref + 8, form, 4);
    cs_flen = 12;
    cs_chunk(vhdr, vlen, 1, 0);
    if (vlen >= 14) {
        memset(cs_ref + 12 + 8, 0, vlen);
        cs_ref[12 + 8 + 12] = (UBYTE)(rate >> 8);  /* samplesPerSec */
        cs_ref[12 + 8 + 13] = (UBYTE)rate;
        cs_ref[12 + 8 + 14] = 1;                   /* octaves */
    }
    /* the word the original takes as the rate: VHDR + its length */
    if (vlen + 12 + 2 <= CS_FILESZ) {
        cs_ref[12 + vlen] = (UBYTE)(rate >> 8);
        cs_ref[12 + vlen + 1] = (UBYTE)rate;
    }
}

static void cs_finish(void)
{
    cs_putl(cs_ref + 4, cs_flen - 8);
    memcpy(cs_file, cs_ref, CS_FILESZ);
}

/* where BODY's data is in cs_ref (0: none) */
static ULONG cs_bodyat;

static void cs_body(ULONG len, int pad)
{
    cs_bodyat = cs_flen + 8;
    cs_chunk("BODY", len, pad, 0x33);
}

static void cs_cludge(ULONG snd)
{
    ULONG old = *(ULONG *)(cs_obj + CS_OFF); /* the object's sample before the call */
    struct rwe_memmark m = rwe_mem_mark();

    ROUT = "CludgeSound";
    DO1(ENT(cs_lib, cs_idx, 0), R.d[0] = snd; R.d[1] = (ULONG)cs_file;
        R.a[0] = (ULONG)(cs_obj + CS_OFF));
    OBS("d0", R.d[0]);
    OBSBYTES("object fields", cs_obj + CS_OFF + 4, 12);
    OBSBYTES("guards before", cs_obj, CS_OFF);
    OBSBYTES("guards behind", cs_obj + CS_OFF + 16, CS_OBJSZ - CS_OFF - 16);
    OBS("period", *(UWORD *)(cs_obj + CS_OFF + 4));
    OBS("words", *(UWORD *)(cs_obj + CS_OFF + 6));
    OBS("memory balance", (ULONG)(rwe_membal - m.bal));
    BOTH("file unchanged", memcmp(cs_file, cs_ref, CS_FILESZ) == 0);
    if (R.d[0] == 0) {
        BOTH("failed: the old pointer", *(ULONG *)(cs_obj + CS_OFF) == old);
    } else {
#define SMP (*(UBYTE **)(cs_obj + CS_OFF))
        ULONG len = cs_ref[cs_bodyat - 4] << 24 | cs_ref[cs_bodyat - 3] << 16
                    | cs_ref[cs_bodyat - 2] << 8 | cs_ref[cs_bodyat - 1];
        BOTH("sample in chip memory", SMP && (TypeOfMem(SMP) & MEMF_CHIP));
        BOTH("sample data", memcmp(SMP, cs_ref + cs_bodyat, len) == 0);
#undef SMP
    }
}

/* the stale pointer of a failed call of the original (#831) must not be
   freed again */
static void cs_forget(void)
{
    *(ULONG *)(cs_obj + CS_OFF) = 0;
}

/* the cases on entry `idx` of `lib`; the sample of the last one stays in
   the object (memlib's finit frees it) */
static void cs_cases(const char *lib, int idx)
{
    static const UWORD rates[] = { 8363, 16000, 1, 2, 3579, 22050, 0x7fff, 0x8000, 0xffff };
    unsigned i;

    cs_lib = lib;
    cs_idx = idx;
    cs_file = AllocMem(CS_FILESZ, MEMF_PUBLIC);
    if (!cs_file) {
        check("AllocMem of the 8SVX file", FALSE);
        return;
    }
    memset(cs_obj, 0xa5, sizeof cs_obj);
    cs_forget();
    memset(cs_obj + CS_OFF + 4, 0x77, 12);

    /* rates; each call frees the sample of the one before */
    for (i = 0; i < sizeof rates / sizeof rates[0]; i++) {
        sprintf(VEC, "rate %u", rates[i]);
        cs_begin("8SVX", "VHDR", 20, rates[i]);
        cs_body(1000 + i, 1);
        cs_finish();
        cs_cludge(0x10000 + i);
    }

    strcpy(VEC, "chunks before BODY");
    cs_begin("8SVX", "VHDR", 20, 8363);
    cs_chunk("NAME", 6, 1, 1);
    cs_chunk("ANNO", 10, 1, 2);
    cs_chunk("CHAN", 4, 1, 3);
    cs_body(333, 1);
    cs_finish();
    cs_cludge(1);

    strcpy(VEC, "BODY the 7th chunk");
    cs_begin("8SVX", "VHDR", 20, 11025);
    for (i = 0; i < 5; i++)
        cs_chunk("ANNO", 2 * i, 1, (UBYTE)i);
    cs_body(64, 1);
    cs_finish();
    cs_cludge(2);

    strcpy(VEC, "BODY the 8th chunk");
    cs_begin("8SVX", "VHDR", 20, 11025);
    for (i = 0; i < 6; i++)
        cs_chunk("ANNO", 2 * i + 2, 1, (UBYTE)i);
    cs_body(64, 1);
    cs_finish();
    cs_cludge(3);                       /* fails: the original freed the sample */
    cs_forget();

    strcpy(VEC, "not 8SVX");
    cs_begin("AIFF", "VHDR", 20, 8363);
    cs_body(100, 1);
    cs_finish();
    cs_cludge(4);

    strcpy(VEC, "no VHDR");
    cs_begin("8SVX", "NAME", 20, 8363);
    cs_body(100, 1);
    cs_finish();
    cs_cludge(5);

    strcpy(VEC, "after failures");
    cs_begin("8SVX", "VHDR", 20, 8363);
    cs_body(101, 1);
    cs_finish();
    cs_cludge(6);

    strcpy(VEC, "VHDR 24 bytes");
    cs_begin("8SVX", "VHDR", 24, 8363);
    cs_ref[12 + 24] = 0x12;             /* the word at VHDR + 24 */
    cs_ref[12 + 25] = 0x34;
    cs_body(50, 1);
    cs_finish();
    cs_cludge(7);

    strcpy(VEC, "odd chunk with pad byte");
    cs_begin("8SVX", "VHDR", 20, 8363);
    cs_chunk("ANNO", 5, 1, 9);
    /* the original's walk lands on the pad byte: id 00 'A' 'B' 00,
       length = 00 and the upper three bytes of $400 = 4, then zeros (id 0,
       length 0) up to the 7th chunk - BODY behind is missed (#828) */
    i = cs_flen + 8;
    cs_chunk("AB\0\0", 0x400, 1, 0);
    memset(cs_ref + i, 0, 0x400);
    cs_body(60, 1);
    cs_finish();
    cs_cludge(13);
    cs_forget();

    strcpy(VEC, "odd chunk without pad byte");
    cs_begin("8SVX", "VHDR", 20, 8363);
    cs_chunk("ANNO", 5, 0, 9);
    cs_body(77, 1);
    cs_finish();
    cs_cludge(8);

    strcpy(VEC, "odd BODY length");
    cs_begin("8SVX", "VHDR", 20, 4000);
    cs_body(99, 1);
    cs_finish();
    cs_cludge(9);

    strcpy(VEC, "BODY 70000 bytes");
    cs_begin("8SVX", "VHDR", 20, 8363);
    cs_body(70000, 1);
    cs_finish();
    cs_cludge(10);

    strcpy(VEC, "BODY 65536 bytes");
    cs_begin("8SVX", "VHDR", 20, 8363);
    cs_body(65536, 1);
    cs_finish();
    cs_cludge(11);

    strcpy(VEC, "BODY 131071 bytes");
    cs_begin("8SVX", "VHDR", 20, 8363);
    cs_body(131071, 1);
    cs_finish();
    cs_cludge(14);

    strcpy(VEC, "BODY 131072 bytes");
    cs_begin("8SVX", "VHDR", 20, 8363);
    cs_body(131072, 1);
    cs_finish();
    cs_cludge(15);

    strcpy(VEC, "BODY 1 byte");
    cs_begin("8SVX", "VHDR", 20, 8363);
    cs_body(1, 1);
    cs_finish();
    cs_cludge(12);

    FreeMem(cs_file, CS_FILESZ);
}

#endif
