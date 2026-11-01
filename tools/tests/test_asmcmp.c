/*
 * test_asmcmp.c - the operand normalisation of asmcmp and broken routine
 * specs (the disassembly itself is exercised by the asmcmp_* ctests of
 * the m68k build, which know objdump).
 */
#include "testutil.h"
#include "asmcmp.h"

static int is(const char *text, int rel, const char *want)
{
    char *n = asmcmp_normalise(text, rel);
    int ok = n && !strcmp(n, want);

    if (!ok)
        fprintf(stderr, "normalise(\"%s\", %d) = \"%s\", want \"%s\"\n", text, rel, n, want);
    free(n);
    return ok;
}

static int spec(const char *s, const char *want)
{
    struct loadfile c = { 0 };
    struct hunk h = { HUNK_CODE, (const uint8_t *)"\x4e\x75", 2, 0, NULL, NULL, 0, NULL, 0 };
    char msg[300];
    FILE *out = tmpfile();
    int r = asmcmp_routine("false", &h, &c, s, out);

    rewind(out);
    msg[fread(msg, 1, sizeof msg - 1, out)] = 0;
    fclose(out);
    return r == -1 && strstr(msg, want) != NULL;
}

int main(void)
{
    /* PC relative */
    CHECK(is("lea %pc@(0x1a),a0", 0, "lea %pc@(REL),a0"));
    CHECK(is("lea %pc@(1a),a0", 0, "lea %pc@(REL),a0"));
    CHECK(is("movew %pc@(0x2,d0:w),d1", 0, "movew %pc@(A,d0:w),d1"));
    /* addresses; ALibJsr placeholders and custom chips stay */
    CHECK(is("jsr 0x1234", 0, "jsr A"));
    CHECK(is("bsrw 0x64", 0, "bsrw A"));
    CHECK(is("jsr 0x80001234", 0, "jsr 0x80001234"));
    CHECK(is("movew d0,0xdff180", 0, "movew d0,0xdff180"));
    CHECK(is("movew d0,0xdff200", 0, "movew d0,A"));
    CHECK(is("movew d0,0xdfeffe", 0, "movew d0,A"));
    /* immediates: only with a relocation, only beyond +-255 */
    CHECK(is("movel #300,d0", 0, "movel #300,d0"));
    CHECK(is("movel #300,d0", 1, "movel #A,d0"));
    CHECK(is("movel #-256,d0", 1, "movel #A,d0"));
    CHECK(is("moveq #-5,d0", 1, "moveq #-5,d0"));
    CHECK(is("cmpiw #255,d0", 1, "cmpiw #255,d0"));
    CHECK(is("movel #0x80000004,d0", 1, "movel #0x80000004,d0"));
    CHECK(is("movel a0@(fffffffffffffff8,d0:w),d0", 1, "movel a0@(fffffffffffffff8,d0:w),d0"));

    /* broken specs (rejected before objdump runs) */
    CHECK(spec("x:604", "expected NAME:ORIGADDR"));
    CHECK(spec("x:604:0:13 y:1:2:3", "expected NAME:ORIGADDR"));
    CHECK(spec("x:60g:0:13", "expected NAME:ORIGADDR"));
    CHECK(spec("x:604:0:0", "expected NAME:ORIGADDR"));
    CHECK(spec("x:604:0+:3", "expected NAME:ORIGADDR"));
    CHECK(spec("x:604:nosuchsym:3", "no symbol nosuchsym"));
    return failures != 0;
}
