/*
 * rwe riencryptlib - Encrypt, Decrypt, GetWheel: the C build or the CD
 * original (BSS2 RIENCRYPTLIB.OBJ), one set as LIBSET says, observed as
 * cases (rwecmp.h, LIBS_ONE_SET; the expected data
 * libs/tests/expected/rwe_riencryptlib.txt).
 *
 *   riencryptlib DIR     (DIR with riencryptlib .obj/.orig)
 *
 * Covered: every form - Encrypt with wheels and with random wheels,
 * Decrypt with and without mode (0, 1, $101, 2), GetWheel 0..4 and $101;
 * lengths 0 (BUG(orig) (#630): one byte), 1, 7, 300 and 700 with wheel values
 * that carry into wheel 2 and 3, Decrypt mode 1 with a length whose byte
 * addition overflows (BUG(orig) (#629)); buffer with guards, wheels, registers.
 * Encrypt without wheels takes them from vallib's Rnd, which is hardware
 * noise without a seed (libs/acidlibs/vallib.c, genrnd).  So vallib is not
 * loaded: its Rnd token is a stub (sysload_stubs) that answers a fixed
 * sequence, and the random wheels and what follows are defined; the
 * wheels from GetWheel are observed, and Decrypt with them restores the
 * data.
 */
#define LIBS_ONE_SET
#include "rwecmp.h"

#define LIB "riencryptlib"
#define BUFSZ 720
#define OFF 8                           /* data start; guards around it */

/* vallib (library 120) Rnd, its first form (no range): the token
   riencryptlib calls */
#define TOK_rnd ((120 << 7) + 7)

static UBYTE buf[BUFSZ], ref[BUFSZ];

/* ------------------------------------------------------------ the Rnd stub */

static UWORD rnd_state;
static ULONG rnd_calls;

/* the next "random" word: a fixed sequence (an LCG), counted */
static ULONG __attribute__((used)) rnd_c(void)
{
    rnd_calls++;
    rnd_state = (UWORD)(rnd_state * 25173U + 13849U);
    return rnd_state;
}

/* Rnd: d0 = 16 bits in the low word, the upper word 0 (the MOVEQ #0,d0 of
   the original); every other register kept */
void rnd_stub(void);
__asm__("   .text\n"
        "   .even\n"
        "_rnd_stub:\n"
        "   movem.l d1-d7/a0-a6,-(sp)\n"
        "   jsr     _rnd_c\n"
        "   movem.l (sp)+,d1-d7/a0-a6\n"
        "   rts\n");

static const struct stub stubs[] = { { TOK_rnd, (APTR)rnd_stub }, { 0, NULL } };

static void fill(void)
{
    int i;
    for (i = 0; i < BUFSZ; i++)
        ref[i] = (UBYTE)(i * 77 + 5);
    memcpy(buf, ref, BUFSZ);
}

static void wheels(void)
{
    static const ULONG nums[] = { 1, 2, 3, 0, 4, 0x101, 0xffffff02UL };
    unsigned i;
    const char *rout = ROUT;

    ROUT = "GetWheel";
    for (i = 0; i < sizeof nums / sizeof nums[0]; i++) {
        DO1(ENT(LIB, 2, 0), R.d[0] = nums[i]);
        OBS("d0", R.d[0]);
    }
    ROUT = rout;
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    static const struct { ULONG len; UWORD w1, w2, w3; } v[] = {
        { 1, 0, 0, 0 }, { 7, 0x12, 0x34, 0x56 }, { 300, 0xfe, 0xff, 0x7f },
        { 700, 0x80, 0xfe, 0xff }, { 300, 0xff, 0xff, 0xff }, { 0, 0x11, 0x22, 0x33 },
        { 5, 0x1234, 0xabcd, 0x8001 },
    };
    static const ULONG modes[] = { 0, 1, 0x101, 2 };
    unsigned i, m;

    if (argc < 2) {
        printf("usage: riencryptlib DIR\n");
        return 20;
    }
    sysload_stubs = stubs;
    if (!load_set(argv[1], names)) {
        sysload_stubs = NULL;
        return 20;
    }
    sysload_stubs = NULL;
    KEEPS("GetWheel", d2-d7|a0-a3|a6);
    KEEPS("Encrypt 5", d5-d7|a1-a3|a6);
    KEEPS("Decrypt 5", d5-d7|a1-a3|a6);
    KEEPS("Decrypt 6", d7|a1-a3|a6);
    KEEPS("Encrypt 2", d5-d7|a1-a3|a6);
    KEEPS("Decrypt 5 (random)", d4-d7|a1-a3|a6);
    /* every routine returns in d0 (Encrypt/Decrypt: the buffer address
       with the last byte in its low byte, GetWheel: the wheel), observed
       by the "d0" cases; whether it equals the input is chance */
    DONTCARE = 1;

    ROUT = "GetWheel start";
    strcpy(VEC, "");
    wheels();

    for (i = 0; i < sizeof v / sizeof v[0]; i++) {
        sprintf(VEC, "len %lu wheels %X %X %X", v[i].len, v[i].w1, v[i].w2, v[i].w3);
        fill();
        ROUT = "Encrypt 5";
        DO1(ENT(LIB, 0, 1), R.d[0] = (ULONG)buf + OFF; R.d[1] = v[i].len;
            R.d[2] = v[i].w1; R.d[3] = v[i].w2; R.d[4] = v[i].w3);
        OBSBYTES("buffer", buf, BUFSZ);
        OBS("d0", (R.d[0] & 0xff) | ((R.d[0] ^ ((ULONG)buf + OFF)) & ~0xffUL));
        OBS("changed", memcmp(buf, ref, BUFSZ) != 0);
        wheels();
        ROUT = "Decrypt 5";
        DO1(ENT(LIB, 1, 0), R.d[0] = (ULONG)buf + OFF; R.d[1] = v[i].len;
            R.d[2] = v[i].w1; R.d[3] = v[i].w2; R.d[4] = v[i].w3);
        OBSBYTES("buffer", buf, BUFSZ);
        OBS("d0", (R.d[0] & 0xff) | ((R.d[0] ^ ((ULONG)buf + OFF)) & ~0xffUL));
        wheels();
        for (m = 0; m < sizeof modes / sizeof modes[0]; m++) {
            sprintf(VEC, "len %lu wheels %X %X %X mode %lX", v[i].len, v[i].w1, v[i].w2,
                    v[i].w3, modes[m]);
            fill();
            ROUT = "Encrypt 5";
            DO1(ENT(LIB, 0, 1), R.d[0] = (ULONG)buf + OFF; R.d[1] = v[i].len;
                R.d[2] = v[i].w1; R.d[3] = v[i].w2; R.d[4] = v[i].w3);
            ROUT = "Decrypt 6";
            DO1(ENT(LIB, 1, 1), R.d[0] = (ULONG)buf + OFF; R.d[1] = v[i].len;
                R.d[2] = v[i].w1; R.d[3] = v[i].w2; R.d[4] = v[i].w3;
                R.d[5] = modes[m]);
            OBSBYTES("buffer", buf, BUFSZ);
            OBS("d0", (R.d[0] & 0xff) | ((R.d[0] ^ ((ULONG)buf + OFF)) & ~0xffUL));
            OBS("restored", memcmp(buf, ref, BUFSZ) == 0);
            wheels();
        }
    }

    /* random wheels: from the Rnd stub's fixed sequence, started here; the
       wheels, the data and the number of Rnd calls are observed */
    rnd_state = 0x1234;
    ROUT = "Encrypt 2";
    for (i = 0; i < 4; i++) {
        ULONG len = i == 0 ? 1 : 100 * i + 3;
        ULONG w[3];
        int k;
        sprintf(VEC, "len %lu", len);
        fill();
        rnd_calls = 0;
        DO1(ENT(LIB, 0, 0), R.d[0] = (ULONG)buf + OFF; R.d[1] = len);
        OBS("Rnd calls", rnd_calls);
        OBSBYTES("buffer", buf, BUFSZ);
        OBS("d0", (R.d[0] & 0xff) | ((R.d[0] ^ ((ULONG)buf + OFF)) & ~0xffUL));
        BOTH("guards", memcmp(buf, ref, OFF) == 0
             && memcmp(buf + OFF + len, ref + OFF + len, BUFSZ - OFF - len) == 0);
        for (k = 0; k < 3; k++) {
            struct regs r;
            ULONG popped;
            markregs(&r);
            r.d[0] = k + 1;
            callstk(ENT(LIB, 2, 0), &r, NULL, 0, &popped);
            w[k] = r.d[0];
        }
        OBS("wheel 1", w[0]);
        OBS("wheel 2", w[1]);
        OBS("wheel 3", w[2]);
        BOTH("wheels are bytes", w[0] < 256 && w[1] < 256 && w[2] < 256);
        ROUT = "Decrypt 5 (random)";
        DO1(ENT(LIB, 1, 0), R.d[0] = (ULONG)buf + OFF; R.d[1] = len;
            R.d[2] = w[0]; R.d[3] = w[1]; R.d[4] = w[2]);
        BOTH("restored", memcmp(buf, ref, BUFSZ) == 0);
        ROUT = "Encrypt 2";
    }
    return rwe_done(LIB);
}
