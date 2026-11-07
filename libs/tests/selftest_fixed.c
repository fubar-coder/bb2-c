/*
 * selftest_fixed - the tests of expect_fixed (sysload.h) with results from
 * memory: no library is loaded, the two "sets" are two values per check
 * (res[0] the original's, res[1] the C library's), compared like the
 * systest programs do (SAME, SAME_FIXED, check_fixed).  Issue #99999
 * stands for the issue of a fix.
 *
 *   selftest_fixed_pass    the C library gives the fixed value: passes,
 *                          the other checks still compare with orig
 *   selftest_fixed_wrong   the C library gives neither: fails
 *   selftest_fixed_stale   the C library gives the original's value: fails
 *
 * libs_tests.cpp: "expect_fixed passes" (in libs_emulator and
 * libs_expect_fixed), the two failing ones hidden ([.selftest]) for the
 * ctest tests libs_expect_fixed_wrong and libs_expect_fixed_stale, which
 * pass when libs_tests reports their failure.
 */
/* not every helper of sysload.h is used here */
#pragma GCC diagnostic ignored "-Wunused-function"
#include "sysload.h"

static int cur;
static ULONG res[2];

static void fail(const char *what, ULONG o, ULONG c)
{
    char buf[200];
    printf("FAIL %s: orig $%08lX, C $%08lX\n", what, o, c);
    snprintf(buf, sizeof buf, "%s", what);
    check(buf, FALSE);
}

#define SAME(what, expr) do {                                               \
        ULONG v_[2];                                                        \
        for (cur = 0; cur < 2; cur++) v_[cur] = (ULONG)(expr);              \
        if (v_[0] != v_[1]) fail(what, v_[0], v_[1]); else checks++;        \
    } while (0)

static int done(const char *name)
{
    printf("%s: %d checks, %d failures\n", name, checks, failures);
    return failures ? 5 : 0;
}

/* one scenario: a result the original gets wrong (#99999: 3 instead of 2)
   and one it gets right (7) */
static int scenario(const char *name, ULONG fixed_c)
{
    failures = checks = 0;
    res[0] = 3;          /* the original's result: the bug */
    res[1] = fixed_c;    /* the C library's */
    SAME_FIXED("result", res[cur], expect_fixed(99999, 2));
    check_fixed("result (check_fixed)", expect_fixed(99999, 2), res[0], res[1]);
    res[0] = res[1] = 7;
    SAME("other result", res[cur]);
    return done(name);
}

int selftest_fixed_pass(void)
{
    return scenario("expect_fixed passes", 2);
}

int selftest_fixed_wrong(void)
{
    return scenario("expect_fixed wrong", 4);
}

int selftest_fixed_stale(void)
{
    return scenario("expect_fixed stale", 3);
}
