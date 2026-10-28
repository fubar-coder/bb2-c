// libs_tests - all emulator tests of the libraries as snitch test cases.
//
// The test programs stay C (libtest.c, systest*.c, compiled with
// -Dmain=NAME_main); every test case calls one of them.  Each check() of
// the C code is reported to snitch as well (bl_test_check), so a failure
// shows up in snitch's report with its text.
//
// The library files are expected next to the executable (PROGDIR:):
// NAME.orig (original from the CD's acidlibs) and NAME.obj (C build) - see
// emulator.cmake (ctest libs_emulator).  Select tests with snitch filters:
//   libs_tests "[mathstr]"      libs_tests "libtest ldivlib"
// The test cases tagged [.bug-orig] (bugorig.c) are hidden: they check
// the correct behaviour where the C build reproduces an original bug, so
// they fail until it is fixed; only an explicit filter runs them
// (libs_tests "[bug-orig]", ctest bug_orig_*).
#include <snitch/snitch.hpp>

#include <cstdio>

extern "C" {
#include <proto/dos.h>

int libtest_main(int argc, char **argv);
int systest_main(int argc, char **argv);
int systest_mathstr_main(int argc, char **argv);
int systest_runtime_main(int argc, char **argv);
int systest_system_main(int argc, char **argv);
int systest_cmdlibs_main(int argc, char **argv);
int bugorig_cliargs_parpath_cli(const char *dir);
int bugorig_crunch_implode_overread(const char *dir);
int bugorig_crunch_decrunch_corrupt(const char *dir);
int bugorig_crunchlib_crm_minsecdist(const char *dir);
int bugorig_crunchlib_crm_overlap_4gb(const char *dir);
int bugorig_crunchlib_crm2_readtab(const char *dir);

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
}

namespace {

// full path of the program's directory, e.g. "TEST:"
const char *progdir()
{
    static char buf[256];
    if (!buf[0] && !NameFromLock(GetProgramDir(), (STRPTR)buf, sizeof buf))
        std::snprintf(buf, sizeof buf, "PROGDIR:");
    return buf;
}

int run_libtest(const char *lib)
{
    static char orig[300], obj[300];
    std::snprintf(orig, sizeof orig, "%s/%s.orig", progdir(), lib);
    std::snprintf(obj, sizeof obj, "%s/%s.obj", progdir(), lib);
    char *argv[] = {(char *)"libtest", orig, obj, nullptr};
    return libtest_main(3, argv);
}

int run_systest(int (*fn)(int, char **))
{
    char *argv[] = {(char *)"systest", (char *)progdir(), nullptr};
    return fn(2, argv);
}

} // namespace

TEST_CASE("libtest ldivlib", "[libtest]") { CHECK(run_libtest("ldivlib") == 0); }
TEST_CASE("libtest lmullib", "[libtest]") { CHECK(run_libtest("lmullib") == 0); }

TEST_CASE("systest: memlib datalib chipbaselib strings1lib staticslib varslib exitslib clrlib", "[sys]")
{
    CHECK(run_systest(systest_main) == 0);
}

TEST_CASE("systest_mathstr: strings2lib ffplib strcomplib floatquicklib modlib maxlenlib", "[mathstr]")
{
    CHECK(run_systest(systest_mathstr_main) == 0);
}

TEST_CASE("systest_runtime: arrayslib maxslib errtraplib rundebuglib", "[runtime]")
{
    CHECK(run_systest(systest_runtime_main) == 0);
}

TEST_CASE("systest_system: wbstartuplib intlib switchlib runerrlib debuglib", "[system]")
{
    CHECK(run_systest(systest_system_main) == 0);
}

TEST_CASE("systest_cmdlibs: memacclib sortlib mathtranslib utillib environlib", "[cmd]")
{
    CHECK(run_systest(systest_cmdlibs_main) == 0);
}

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
