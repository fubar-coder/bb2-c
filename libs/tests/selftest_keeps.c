/*
 * selftest_keeps - the checks of KEEPS (sysload.h) with
 * broken input, no library: a set that is no register set, a routine
 * stated twice, and a call of a routine without a stated set (a routine
 * of this program that changes d2).  Each must fail with its message
 * (ctest libs_keeps_wrong; the snitch case "keeps wrong" is [.selftest]
 * and fails on purpose).  The register rule (KEEPS_FUNCTION,
 * KEEPS_STATEMENT): a function that loses a6 and a word function stated
 * with d0u that changes the upper word of d0 fail; a statement that
 * changes d1 and a0 passes.  A lost stated register in a run of the
 * library tests is libs_emulator_lost's.
 */
/* not every helper of sysload.h is used here */
#pragma GCC diagnostic ignored "-Wunused-function"
#include "sysload.h"

/* moveq #0,d2 : rts */
static const UWORD clobber_d2[] = { 0x7400, 0x4e75 };
/* suba.l a6,a6 : rts */
static const UWORD clobber_a6[] = { 0x9dce, 0x4e75 };
/* moveq #0,d1 : suba.l a0,a0 : rts */
static const UWORD clobber_d1_a0[] = { 0x7200, 0x91c8, 0x4e75 };
/* moveq #1,d0 : rts (the marker's upper word of d0 is not 0) */
static const UWORD clobber_d0u[] = { 0x7001, 0x4e75 };

/* call code as the routine rout; a check that it kept its stated set */
static void rule_call(const char *rout, const UWORD *code)
{
    struct regs r, in;
    char t[64], buf[120];
    ULONG lost;

    ROUT = rout;
    markregs(&r);
    in = r;
    CacheClearU();
    callregs((APTR)code, &r, 0, 0);
    lost = obs_kept(&in, &r);
    keeps_text(t, lost);
    snprintf(buf, sizeof buf, "%s: %s%s", rout, lost ? "lost " : "kept", lost ? t : "");
    check(buf, !lost);
}

int selftest_keeps_wrong(void)
{
    struct regs r, in;

    KEEPS("bad", d2|x9);
    KEEPS("twice", d2);
    KEEPS("twice", d3);
    ROUT = "unstated";
    markregs(&r);
    in = r;
    CacheClearU();
    callregs((APTR)clobber_d2, &r, 0, 0);
    obs_kept(&in, &r);
    KEEPS_FUNCTION("function", none);
    KEEPS_STATEMENT("statement", none);
    KEEPS_FUNCTION("word function", d0u);
    rule_call("function", clobber_a6);
    rule_call("statement", clobber_d1_a0);
    rule_call("word function", clobber_d0u);
    printf("keeps wrong: %d checks, %d failures\n", checks, failures);
    return failures ? 5 : 0;
}
