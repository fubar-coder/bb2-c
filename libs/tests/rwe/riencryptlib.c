/*
 * rwe riencryptlib - Encrypt, Decrypt, GetWheel: the C build against the
 * CD original (BSS2 RIENCRYPTLIB.OBJ), method of rwecmp.h.
 *
 *   riencryptlib DIR     (DIR with riencryptlib, vallib, floatquicklib
 *                         .obj/.orig)
 *
 * Covered: every form - Encrypt with wheels and with random wheels,
 * Decrypt with and without mode (0, 1, $101, 2), GetWheel 0..4 and $101;
 * lengths 0 (BUG(orig): one byte), 1, 7, 300 and 700 with wheel values
 * that carry into wheel 2 and 3, Decrypt mode 1 with a length whose byte
 * addition overflows (BUG(orig)); buffer with guards, wheels, registers.
 * Encrypt without wheels takes them from vallib's Rnd (hardware timing):
 * not comparable between the sets, so each set's result is checked
 * itself - wheels from GetWheel, Decrypt with them restores the data.
 */
#include "rwecmp.h"

#define LIB "riencryptlib"
#define BUFSZ 720
#define OFF 8                           /* data start; guards around it */

static UBYTE buf[2][BUFSZ], ref[BUFSZ];

static void fill(void)
{
    int i, s;
    for (i = 0; i < BUFSZ; i++)
        ref[i] = (UBYTE)(i * 77 + 5);
    for (s = 0; s < 2; s++)
        memcpy(buf[s], ref, BUFSZ);
}

static void wheels(void)
{
    static const ULONG nums[] = { 1, 2, 3, 0, 4, 0x101, 0xffffff02UL };
    unsigned i;
    const char *rout = ROUT;

    ROUT = "GetWheel";
    for (i = 0; i < sizeof nums / sizeof nums[0]; i++) {
        DO2(ENT(LIB, 2, 0), R[cur].d[0] = nums[i]);
        SAME("d0", R[cur].d[0]);
    }
    ROUT = rout;
}

int main(int argc, char **argv)
{
    static const char *names[] = { "floatquicklib", "vallib", LIB, NULL };
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
    if (!load_sets(argv[1], names))
        return 20;
    /* every routine returns in d0 (Encrypt/Decrypt: the buffer address
       with the last byte in its low byte, GetWheel: the wheel), compared
       by the "d0" checks; whether it equals the input is chance */
    DONTCARE = 1;

    ROUT = "GetWheel start";
    strcpy(VEC, "");
    wheels();

    for (i = 0; i < sizeof v / sizeof v[0]; i++) {
        sprintf(VEC, "len %lu wheels %X %X %X", v[i].len, v[i].w1, v[i].w2, v[i].w3);
        fill();
        ROUT = "Encrypt 5";
        DO2(ENT(LIB, 0, 1), R[cur].d[0] = (ULONG)buf[cur] + OFF; R[cur].d[1] = v[i].len;
            R[cur].d[2] = v[i].w1; R[cur].d[3] = v[i].w2; R[cur].d[4] = v[i].w3);
        SAMEBYTES("buffer", buf, BUFSZ);
        SAME("d0", (R[cur].d[0] & 0xff) | ((R[cur].d[0] ^ ((ULONG)buf[cur] + OFF)) & ~0xffUL));
        SAME("changed", memcmp(buf[cur], ref, BUFSZ) != 0);
        wheels();
        ROUT = "Decrypt 5";
        DO2(ENT(LIB, 1, 0), R[cur].d[0] = (ULONG)buf[cur] + OFF; R[cur].d[1] = v[i].len;
            R[cur].d[2] = v[i].w1; R[cur].d[3] = v[i].w2; R[cur].d[4] = v[i].w3);
        SAMEBYTES("buffer", buf, BUFSZ);
        SAME("d0", (R[cur].d[0] & 0xff) | ((R[cur].d[0] ^ ((ULONG)buf[cur] + OFF)) & ~0xffUL));
        wheels();
        for (m = 0; m < sizeof modes / sizeof modes[0]; m++) {
            sprintf(VEC, "len %lu wheels %X %X %X mode %lX", v[i].len, v[i].w1, v[i].w2,
                    v[i].w3, modes[m]);
            fill();
            ROUT = "Encrypt 5";
            DO2(ENT(LIB, 0, 1), R[cur].d[0] = (ULONG)buf[cur] + OFF; R[cur].d[1] = v[i].len;
                R[cur].d[2] = v[i].w1; R[cur].d[3] = v[i].w2; R[cur].d[4] = v[i].w3);
            ROUT = "Decrypt 6";
            DO2(ENT(LIB, 1, 1), R[cur].d[0] = (ULONG)buf[cur] + OFF; R[cur].d[1] = v[i].len;
                R[cur].d[2] = v[i].w1; R[cur].d[3] = v[i].w2; R[cur].d[4] = v[i].w3;
                R[cur].d[5] = modes[m]);
            SAMEBYTES("buffer", buf, BUFSZ);
            SAME("d0", (R[cur].d[0] & 0xff) | ((R[cur].d[0] ^ ((ULONG)buf[cur] + OFF)) & ~0xffUL));
            SAME("restored", memcmp(buf[cur], ref, BUFSZ) == 0);
            wheels();
        }
    }

    /* random wheels: each set for itself */
    ROUT = "Encrypt 2";
    for (i = 0; i < 4; i++) {
        ULONG len = i == 0 ? 1 : 100 * i + 3;
        ULONG w[2][3];
        int k;
        sprintf(VEC, "len %lu", len);
        fill();
        DO2(ENT(LIB, 0, 0), R[cur].d[0] = (ULONG)buf[cur] + OFF; R[cur].d[1] = len);
        BOTH("guards", memcmp(buf[cur], ref, OFF) == 0
             && memcmp(buf[cur] + OFF + len, ref + OFF + len, BUFSZ - OFF - len) == 0);
        for (cur = 0; cur < 2; cur++)
            for (k = 0; k < 3; k++) {
                struct regs r;
                ULONG popped;
                markregs(&r);
                r.d[0] = k + 1;
                callstk(ENT(LIB, 2, 0), &r, NULL, 0, &popped);
                w[cur][k] = r.d[0];
            }
        BOTH("wheels are bytes", w[cur][0] < 256 && w[cur][1] < 256 && w[cur][2] < 256);
        ROUT = "Decrypt 5 (random)";
        DO2(ENT(LIB, 1, 0), R[cur].d[0] = (ULONG)buf[cur] + OFF; R[cur].d[1] = len;
            R[cur].d[2] = w[cur][0]; R[cur].d[3] = w[cur][1]; R[cur].d[4] = w[cur][2]);
        BOTH("restored", memcmp(buf[cur], ref, BUFSZ) == 0);
        ROUT = "Encrypt 2";
    }
    return rwe_done(LIB);
}
