// libs_tests - all emulator tests of the libraries as snitch test cases.
//
// The test programs stay C (libtest.c, systest*.c, compiled with
// -Dmain=NAME_main); every test case calls one of them.  Each check() of
// the C code is reported to snitch as well (bl_test_check), so a failure
// shows up in snitch's report with its text.
//
// The library files are expected next to the executable (PROGDIR:):
// NAME.orig (original from the CD's acidlibs) and NAME.obj (C build) - see
// emulator.cmake (ctest libs_emulator).  A program that observes one set
// (libtest.c, and every program converted with LIBS_ONE_SET, sysload.h)
// loads NAME.<LIBSET> and writes its cases to PROGDIR:captures/NAME.txt
// (capture.h); the others compare both sets through their tape
// (PROGDIR:tapes/NAME.tape, tape.h).  Select tests with snitch filters:
//   libs_tests "[mathstr]"      libs_tests "libtest ldivlib"
// The test cases tagged [.bug-orig] (bugorig.c) are hidden: they check
// the correct behaviour where the C build reproduces an original bug, so
// they fail until it is fixed; only an explicit filter runs them
// (libs_tests "[bug-orig]", ctest bug_orig_*).  The test cases tagged
// [.selftest] fail on purpose (selftest_fixed.c: expect_fixed with a wrong
// value and with a stale exemption); ctest libs_expect_fixed_wrong and
// libs_expect_fixed_stale check that they do.
#include <snitch/snitch.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include <proto/dos.h>
#include <proto/exec.h>
#include "capture.h"
#include "tape.h"

int libtest_main(int argc, char **argv);
int systest_main(int argc, char **argv);
int systest_mathstr_main(int argc, char **argv);
int systest_runtime_main(int argc, char **argv);
int systest_system_main(int argc, char **argv);
int systest_cmdlibs_main(int argc, char **argv);
int systest_med_main(int argc, char **argv);
int bugorig_cliargs_parpath_cli(const char *dir);
int bugorig_crunch_implode_overread(const char *dir);
int bugorig_crunch_decrunch_corrupt(const char *dir);
int bugorig_crunchlib_crm_minsecdist(const char *dir);
int bugorig_crunchlib_crm_overlap_4gb(const char *dir);
int bugorig_crunchlib_crm2_readtab(const char *dir);
int bugorig_crunchlib_pp_readd1(const char *dir);
int bugorig_debuglib_findoff_flags(const char *dir);
int selftest_fixed_pass(void);
int selftest_fixed_wrong(void);
int selftest_fixed_stale(void);
// the tests of the RWE userlibs (rwe/NAME.c, generated list)
#define RWE_TEST(n) int rwe_##n##_main(int argc, char **argv);
#include "rwe_tests.inc"
#undef RWE_TEST

void bl_test_check(const char *what, int ok)
{
    if (!ok) {
        INFO(what);
        CHECK(false);
    } else {
        CHECK(true);
    }
}

// libnix swaps to a stack of this size at startup (-Wl,-u,___stkinit)
unsigned long __stack = 128 * 1024;

// the library set of the run (sysload.h load_set): the shell variable
// LIBSET, "obj" or "orig"; nullptr if it is not set (or another value)
const char *bl_libset;

// the program run_systest runs: its name, the directories of its tape and
// its cases, and what it began (sysload.h: its first load_sets() calls
// bl_prog_tape, its first load_set() bl_prog_cases)
static const char *prog_name, *prog_tapes, *prog_cases;
static enum { OUT_NONE, OUT_TAPE, OUT_CASES } prog_out;

void bl_prog_tape(void)
{
    if (prog_name && prog_out == OUT_NONE) {
        tape_begin(prog_tapes, prog_name);
        prog_out = OUT_TAPE;
    }
}

void bl_prog_cases(void)
{
    if (prog_name && prog_out == OUT_NONE) {
        cap_begin(prog_cases, prog_name);
        prog_out = OUT_CASES;
    }
}
}

namespace {

// LIBSET (the set a program that observes one set loads) and LIBS_PAD (N
// bytes allocated before a program runs and freed after it, so that the
// libraries and buffers it allocates are elsewhere: the second run of
// libs-capture, design D8 of the openspec change libs-tests-capture)
struct run_env {
    void *pad = nullptr;
    unsigned long padsize = 0;

    run_env()
    {
        static char set[16];
        char v[16];
        bl_libset = nullptr;
        if (GetVar((STRPTR) "LIBSET", (STRPTR)set, sizeof set, 0) > 0
            && (!std::strcmp(set, "obj") || !std::strcmp(set, "orig")))
            bl_libset = set;
        if (GetVar((STRPTR) "LIBS_PAD", (STRPTR)v, sizeof v, 0) > 0)
            padsize = std::strtoul(v, nullptr, 10);
        if (padsize && !(pad = AllocMem(padsize, MEMF_ANY)))
            std::printf("LIBS_PAD: cannot allocate %lu bytes\n", padsize);
    }
    ~run_env()
    {
        if (pad)
            FreeMem(pad, padsize);
    }
    run_env(const run_env &) = delete;
    run_env &operator=(const run_env &) = delete;
};

// full path of the program's directory, e.g. "TEST:"
const char *progdir()
{
    static char buf[256];
    if (!buf[0] && !NameFromLock(GetProgramDir(), (STRPTR)buf, sizeof buf))
        std::snprintf(buf, sizeof buf, "PROGDIR:");
    return buf;
}

// the recordings of the original libraries (tape.h): PROGDIR:tapes/NAME.tape
const char *tapedir()
{
    static char buf[300];
    if (!buf[0])
        std::snprintf(buf, sizeof buf, "%s/tapes", progdir());
    return buf;
}

// the cases of the programs that observe one set (capture.h):
// PROGDIR:captures/NAME.txt
const char *capdir()
{
    static char buf[300];
    if (!buf[0])
        std::snprintf(buf, sizeof buf, "%s/captures", progdir());
    return buf;
}

int run_libtest(const char *lib)
{
    static char file[300], name[40];
    run_env env;
    if (!bl_libset) {
        bl_test_check("libtest: LIBSET is not set (obj or orig)", 0);
        return 20;
    }
    std::snprintf(file, sizeof file, "%s/%s.%s", progdir(), lib, bl_libset);
    std::snprintf(name, sizeof name, "libtest_%s", lib);
    char *argv[] = {(char *)"libtest", file, (char *)capdir(), name, nullptr};
    return libtest_main(4, argv);
}

// a systest or rwe program: it begins its tape (two sets) or its cases
// (one set) when it loads the libraries, ended here
int run_systest(const char *name, int (*fn)(int, char **))
{
    char *argv[] = {(char *)"systest", (char *)progdir(), nullptr};
    run_env env;
    prog_name = name;
    prog_tapes = tapedir();
    prog_cases = capdir();
    prog_out = OUT_NONE;
    int rc = fn(2, argv);
    if (prog_out == OUT_TAPE)
        tape_end();
    else if (prog_out == OUT_CASES && cap_end() && !rc)
        rc = 5;
    prog_name = nullptr;
    prog_out = OUT_NONE;
    return rc;
}

} // namespace

TEST_CASE("libtest ldivlib", "[libtest]") { CHECK(run_libtest("ldivlib") == 0); }
TEST_CASE("libtest lmullib", "[libtest]") { CHECK(run_libtest("lmullib") == 0); }

TEST_CASE("systest: memlib datalib chipbaselib strings1lib staticslib varslib exitslib clrlib", "[sys]")
{
    CHECK(run_systest("systest", systest_main) == 0);
}

TEST_CASE("systest_mathstr: strings2lib ffplib strcomplib floatquicklib modlib maxlenlib", "[mathstr]")
{
    CHECK(run_systest("systest_mathstr", systest_mathstr_main) == 0);
}

TEST_CASE("systest_runtime: arrayslib maxslib errtraplib rundebuglib", "[runtime]")
{
    CHECK(run_systest("systest_runtime", systest_runtime_main) == 0);
}

TEST_CASE("systest_system: wbstartuplib intlib switchlib runerrlib debuglib", "[system]")
{
    CHECK(run_systest("systest_system", systest_system_main) == 0);
}

TEST_CASE("systest_cmdlibs: memacclib sortlib mathtranslib utillib environlib", "[cmd]")
{
    CHECK(run_systest("systest_cmdlibs", systest_cmdlibs_main) == 0);
}

TEST_CASE("systest_med: medlib (generated modules)", "[med]")
{
    CHECK(run_systest("systest_med", systest_med_main) == 0);
}

// the RWE userlibs (libs/userlibs/rwe), each against its CD binary
#define RWE_TEST(n) \
    TEST_CASE("rwe " #n, "[rwe]") { CHECK(run_systest("rwe_" #n, rwe_##n##_main) == 0); }
#include "rwe_tests.inc"
#undef RWE_TEST

// expect_fixed (sysload.h) with results from memory (selftest_fixed.c)
TEST_CASE("expect_fixed passes", "[expect_fixed]") { CHECK(selftest_fixed_pass() == 0); }
TEST_CASE("expect_fixed wrong", "[.selftest]") { CHECK(selftest_fixed_wrong() == 0); }
TEST_CASE("expect_fixed stale", "[.selftest]") { CHECK(selftest_fixed_stale() == 0); }

// reproduced original bugs: fail as long as the C build reproduces them
TEST_CASE("cliargs-parpath-cli", "[.bug-orig]")
{
    CHECK(bugorig_cliargs_parpath_cli(progdir()) == 0);
}

// the verdict on the reads behind the data is emulator.cmake's (ILLEGAL_WATCH)
TEST_CASE("crunch-implode-overread", "[.bug-orig]")
{
    CHECK(bugorig_crunch_implode_overread(progdir()) == 0);
}

TEST_CASE("crunch-decrunch-corrupt", "[.bug-orig]")
{
    CHECK(bugorig_crunch_decrunch_corrupt(progdir()) == 0);
}

TEST_CASE("crunchlib-crm-minsecdist", "[.bug-orig]")
{
    CHECK(bugorig_crunchlib_crm_minsecdist(progdir()) == 0);
}

// a crash (the 4 GB copy) ends the run in the emulator's time limit
TEST_CASE("crunchlib-crm-overlap-4gb", "[.bug-orig]")
{
    CHECK(bugorig_crunchlib_crm_overlap_4gb(progdir()) == 0);
}

TEST_CASE("crunchlib-crm2-readtab", "[.bug-orig]")
{
    CHECK(bugorig_crunchlib_crm2_readtab(progdir()) == 0);
}

TEST_CASE("crunchlib-pp-readd1", "[.bug-orig]")
{
    CHECK(bugorig_crunchlib_pp_readd1(progdir()) == 0);
}

TEST_CASE("debuglib-findoff-flags", "[.bug-orig]")
{
    CHECK(bugorig_debuglib_findoff_flags(progdir()) == 0);
}
