/*
 * selftest_keeps - the checks of KEEPS (sysload.h) with
 * broken input, no library: a set that is no register set, a routine
 * stated twice, and a call of a routine without a stated set (a routine
 * of this program that changes d2).  Each must fail with its message
 * (ctest libs_keeps_wrong; the snitch case "keeps wrong" is [.selftest]
 * and fails on purpose).  A lost stated register is libs_emulator_lost's.
 */
/* not every helper of sysload.h is used here */
#pragma GCC diagnostic ignored "-Wunused-function"
#include "sysload.h"

/* moveq #0,d2 : rts */
static const UWORD clobber_d2[] = { 0x7400, 0x4e75 };

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
    printf("keeps wrong: %d checks, %d failures\n", checks, failures);
    return failures ? 5 : 0;
}
