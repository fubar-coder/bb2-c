// test_ab3args - the command line of blitz2-ab3 (ab3args_parse,
// shared/ab3job): the scenarios of the spec ab3-compat/command-line -
// valid compiles (short and long names, -r, -mode), missing arguments,
// unknown and unsupported options, the usage, and the editor start
// without options, and the return code at the end of a compile.
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "ab3job.h"
}

namespace {

struct parsed {
    int result;
    ab3job job;
    std::string msg;
};

parsed parse(std::vector<std::string> args)
{
    std::vector<char *> argv;
    static std::vector<std::string> keep;
    parsed p{};
    char msg[200];

    keep = std::move(args);
    keep.insert(keep.begin(), "blitz2-ab3");
    for (auto &a : keep)
        argv.push_back(a.data());
    argv.push_back(nullptr);
    std::memset(msg, 'x', sizeof msg);
    p.result = ab3args_parse(int(keep.size()), argv.data(), &p.job, msg, sizeof msg);
    p.msg = msg;
    return p;
}

} // namespace

TEST_CASE("compile: -s and -e", "[ab3args]")
{
    auto p = parse({ "-s", "prog.ab3", "-e", "RAM:prog" });
    CHECK(p.result == AB3ARGS_COMPILE);
    CHECK(ab3args_rc(p.result) == -1);
    CHECK(std::string(p.job.magic) == AB3JOB_MAGIC);
    CHECK(std::string(p.job.source) == "prog.ab3");
    CHECK(std::string(p.job.exe) == "RAM:prog");
    CHECK(p.job.force == 0);    // XTRA_MODE_DETECT
    CHECK(p.job.release_exe == 0);
    CHECK(p.job.started == 0);
    CHECK(p.job.result == AB3JOB_RC_FAIL);
    CHECK(p.msg.empty());
}

TEST_CASE("compile: long option names, any case", "[ab3args]")
{
    auto a = parse({ "-source", "prog.ab3", "-exe", "RAM:prog" });
    auto b = parse({ "-SOURCE", "prog.ab3", "-Exe", "RAM:prog" });
    for (auto *p : { &a, &b }) {
        CHECK(p->result == AB3ARGS_COMPILE);
        CHECK(std::string(p->job.source) == "prog.ab3");
        CHECK(std::string(p->job.exe) == "RAM:prog");
    }
}

TEST_CASE("compile: -r / -release", "[ab3args]")
{
    for (const char *r : { "-r", "-release", "-R" }) {
        auto p = parse({ "-s", "a.ab3", r, "-e", "RAM:a" });
        CHECK(p.result == AB3ARGS_COMPILE);
        CHECK(p.job.release_exe == 1);
    }
}

TEST_CASE("compile: -mode forces the mode", "[ab3args]")
{
    auto bb2 = parse({ "-s", "prog.ab3", "-e", "RAM:prog", "-mode", "bb2" });
    CHECK(bb2.result == AB3ARGS_COMPILE);
    CHECK(bb2.job.force == 1);  // XTRA_MODE_BB2
    auto ab3 = parse({ "-mode", "AB3", "-s", "prog.bb", "-e", "RAM:prog" });
    CHECK(ab3.result == AB3ARGS_COMPILE);
    CHECK(ab3.job.force == 2);  // XTRA_MODE_AB3
    auto bad = parse({ "-s", "prog.bb", "-e", "RAM:prog", "-mode", "ab2" });
    CHECK(bad.result == AB3ARGS_FAIL);
    CHECK(bad.msg == "unknown mode \"ab2\" (ab3 or bb2)");
}

TEST_CASE("compile: the last value counts", "[ab3args]")
{
    auto p = parse({ "-s", "a", "-e", "x", "-s", "b", "-e", "y" });
    CHECK(p.result == AB3ARGS_COMPILE);
    CHECK(std::string(p.job.source) == "b");
    CHECK(std::string(p.job.exe) == "y");
}

TEST_CASE("missing arguments", "[ab3args]")
{
    auto noexe = parse({ "-s", "prog.ab3" });
    CHECK(noexe.result == AB3ARGS_FAIL);
    CHECK(ab3args_rc(noexe.result) == 20);
    CHECK(noexe.msg == "no executable given (-e)");
    auto nosrc = parse({ "-e", "RAM:prog" });
    CHECK(nosrc.result == AB3ARGS_FAIL);
    CHECK(nosrc.msg == "no source given (-s)");
    auto noval = parse({ "-e", "RAM:prog", "-s" });
    CHECK(noval.result == AB3ARGS_FAIL);
    CHECK(noval.msg == "option -s needs an argument");
    auto empty = parse({ "-s", "", "-e", "RAM:prog" });
    CHECK(empty.result == AB3ARGS_FAIL);
    CHECK(empty.msg == "option -s needs an argument");
    auto nomode = parse({ "-s", "a", "-e", "b", "-mode" });
    CHECK(nomode.result == AB3ARGS_FAIL);
    CHECK(nomode.msg == "option -mode needs an argument");
}

TEST_CASE("a file name that does not fit", "[ab3args]")
{
    std::string ok(AB3JOB_PATHLEN - 1, 'a'), longer(AB3JOB_PATHLEN, 'a');
    CHECK(parse({ "-s", ok, "-e", "x" }).result == AB3ARGS_COMPILE);
    auto p = parse({ "-s", longer, "-e", "x" });
    CHECK(p.result == AB3ARGS_FAIL);
    CHECK(p.msg.rfind("file name too long: aaa", 0) == 0);
}

TEST_CASE("unknown options and stray arguments", "[ab3args]")
{
    auto p = parse({ "-s", "a", "-e", "b", "-frobnicate" });
    CHECK(p.result == AB3ARGS_FAIL);
    CHECK(p.msg == "unknown option -frobnicate");
    auto q = parse({ "-s", "a", "-e", "b", "c" });
    CHECK(q.result == AB3ARGS_FAIL);
    CHECK(q.msg == "unexpected argument \"c\"");
}

TEST_CASE("AmiBlitz3's other options are refused", "[ab3args]")
{
    for (const char *o : { "-d", "-debug", "-run", "-dump", "-setcom", "-setdbg", "-strbufsize",
                           "-stacksize", "-maxerrors", "-1", "-1pass", "-dform", "-PED", "-ped",
                           "-save", "-v", "-verbose" }) {
        auto p = parse({ "-s", "prog.ab3", "-e", "RAM:prog", o });
        CHECK(p.result == AB3ARGS_UNSUPPORTED);
        CHECK(ab3args_rc(p.result) == 20);
        CHECK(p.msg == std::string("option ") + o + " is not supported by blitz2-ab3");
    }
    // the first problem decides: nothing behind it is looked at
    auto p = parse({ "-d", "-s" });
    CHECK(p.result == AB3ARGS_UNSUPPORTED);
}

TEST_CASE("usage", "[ab3args]")
{
    for (const char *h : { "-h", "-help", "-HELP" }) {
        auto p = parse({ h });
        CHECK(p.result == AB3ARGS_HELP);
        CHECK(ab3args_rc(p.result) == 0);
    }
    CHECK(parse({ "-s", "a", "-h", "-frobnicate" }).result == AB3ARGS_HELP);
    CHECK(parse({ "-frobnicate", "-h" }).result == AB3ARGS_FAIL);
    std::string u = ab3args_usage;
    for (const char *o : { "-s, -source", "-e, -exe", "-r, -release", "-mode ab3|bb2", "-h, -help" })
        CHECK(u.find(o) != std::string::npos);
}

TEST_CASE("no option: the editor", "[ab3args]")
{
    auto none = parse({});
    CHECK(none.result == AB3ARGS_EDITOR);
    CHECK(ab3args_rc(none.result) == -1);
    auto project = parse({ "work:prog.bb2" });
    CHECK(project.result == AB3ARGS_EDITOR);
    CHECK(project.msg.empty());
}

TEST_CASE("the message is cut to its buffer", "[ab3args]")
{
    char *argv[] = { (char *)"blitz2-ab3", (char *)"-frobnicate", nullptr };
    char msg[8];
    ab3job job;
    CHECK(ab3args_parse(2, argv, &job, msg, sizeof msg) == AB3ARGS_FAIL);
    CHECK(std::string(msg) == "unknown");
}

namespace {

// the set a list makes of the source's setting s: (s & keep) | add
uint32_t applied(uint32_t s, uint32_t keep, uint32_t add)
{
    return (s & keep) | add;
}

parsed compile_with(std::vector<std::string> opts)
{
    std::vector<std::string> args = { "-s", "a", "-e", "b" };
    args.insert(args.end(), opts.begin(), opts.end());
    return parse(args);
}

} // namespace

TEST_CASE("warnings: the names", "[ab3args]")
{
    CHECK(AB3JOB_W_COUNT == 1);
    CHECK(std::string(ab3args_warnings[AB3JOB_W_RESCLASH]) == "resclash");
    CHECK(ab3args_warnings[AB3JOB_W_COUNT] == nullptr);
    CHECK(AB3JOB_W_ALL == 1u);
}

TEST_CASE("warnings: without -w and -werr the source's setting", "[ab3args]")
{
    auto p = compile_with({});
    CHECK(p.result == AB3ARGS_COMPILE);
    CHECK(p.job.warn_keep == AB3JOB_W_ALL);
    CHECK(p.job.warn_add == 0u);
    CHECK(p.job.werr_keep == AB3JOB_W_ALL);
    CHECK(p.job.werr_add == 0u);
    CHECK(p.job.warnerrs == 0);
    CHECK(p.job.warnings == 0);
    for (uint32_t s : { 0u, AB3JOB_W_ALL }) {
        CHECK(applied(s, p.job.warn_keep, p.job.warn_add) == s);
        CHECK(applied(s, p.job.werr_keep, p.job.werr_add) == s);
    }
}

TEST_CASE("warnings: the items of a list", "[ab3args]")
{
    const uint32_t R = 1u << AB3JOB_W_RESCLASH;
    struct {
        const char *list;
        uint32_t from_none, from_all;   // the set from the setting 0 / all
    } cases[] = {
        { "all", AB3JOB_W_ALL, AB3JOB_W_ALL },
        { "none", 0, 0 },
        { "resclash", R, AB3JOB_W_ALL },
        { "-resclash", 0, AB3JOB_W_ALL & ~R },
        { "all,-resclash", 0, 0 },
        { "none,resclash", R, R },
        { "-resclash,resclash", R, AB3JOB_W_ALL },
        { "resclash,none", 0, 0 },
        { "ALL,-ResClash", 0, 0 },
    };
    for (auto &c : cases) {
        for (const char *o : { "-w", "-W", "-werr", "-WERR" }) {
            auto p = compile_with({ o, c.list });
            CAPTURE(o, c.list);
            REQUIRE(p.result == AB3ARGS_COMPILE);
            bool w = o[2] == 0;
            uint32_t keep = w ? p.job.warn_keep : p.job.werr_keep;
            uint32_t add = w ? p.job.warn_add : p.job.werr_add;
            CHECK(applied(0, keep, add) == c.from_none);
            CHECK(applied(AB3JOB_W_ALL, keep, add) == c.from_all);
            // the other list is untouched
            CHECK((w ? p.job.werr_keep : p.job.warn_keep) == AB3JOB_W_ALL);
            CHECK((w ? p.job.werr_add : p.job.warn_add) == 0u);
        }
    }
}

TEST_CASE("warnings: several options are applied in order", "[ab3args]")
{
    auto p = compile_with({ "-w", "none", "-w", "resclash", "-werr", "resclash", "-werr", "-resclash" });
    CHECK(p.result == AB3ARGS_COMPILE);
    CHECK(applied(0, p.job.warn_keep, p.job.warn_add) == AB3JOB_W_ALL);
    CHECK(applied(AB3JOB_W_ALL, p.job.werr_keep, p.job.werr_add) == 0u);
    // in any position, also before -s
    auto q = parse({ "-werr", "resclash", "-s", "a", "-e", "b" });
    CHECK(q.result == AB3ARGS_COMPILE);
    CHECK(applied(0, q.job.werr_keep, q.job.werr_add) == AB3JOB_W_ALL);
}

TEST_CASE("warnings: broken lists", "[ab3args]")
{
    struct {
        std::vector<std::string> opts;
        const char *msg;
    } cases[] = {
        { { "-w", "nosuch" }, "unknown warning \"nosuch\" in -w (known: all, none, resclash)" },
        { { "-werr", "all,resclashx" },
          "unknown warning \"resclashx\" in -werr (known: all, none, resclash)" },
        { { "-w", "-all" }, "unknown warning \"-all\" in -w (known: all, none, resclash)" },
        { { "-w", "res" }, "unknown warning \"res\" in -w (known: all, none, resclash)" },
        { { "-w", "all,,resclash" }, "empty item in -w \"all,,resclash\"" },
        { { "-w", "resclash," }, "empty item in -w \"resclash,\"" },
        { { "-werr", "," }, "empty item in -werr \",\"" },
        { { "-w", "-" }, "empty item in -w \"-\"" },
        { { "-w", "" }, "option -w needs an argument" },
        { { "-werr" }, "option -werr needs an argument" },
    };
    for (auto &c : cases) {
        auto p = compile_with(c.opts);
        CAPTURE(c.msg);
        CHECK(p.result == AB3ARGS_FAIL);
        CHECK(ab3args_rc(p.result) == 20);
        CHECK(p.msg == c.msg);
    }
    // -wx is not -w
    auto p = compile_with({ "-wx", "all" });
    CHECK(p.result == AB3ARGS_FAIL);
    CHECK(p.msg == "unknown option -wx");
}

TEST_CASE("warnings: the usage lists the options and the names", "[ab3args]")
{
    std::string u = ab3args_usage;
    for (const char *o : { "-w LIST", "-werr LIST", "all, none, NAME, -NAME" })
        CHECK(u.find(o) != std::string::npos);
    for (int k = 0; ab3args_warnings[k]; k++)
        CHECK(u.find(ab3args_warnings[k]) != std::string::npos);
}

TEST_CASE("return code: the end of a compile (ab3job_rc)", "[ab3args]")
{
    auto p = compile_with({});
    REQUIRE(p.result == AB3ARGS_COMPILE);
    ab3job &j = p.job;
    CHECK(AB3JOB_RC_WARN == 5);
    // no warnings: 0; written after warnings: 5 (RETURN_WARN)
    CHECK(ab3job_rc(&j, 0, 0, 1) == AB3JOB_RC_OK);
    j.warnings = 1;
    CHECK(ab3job_rc(&j, 0, 0, 1) == AB3JOB_RC_WARN);
    j.warnings = 255;
    CHECK(ab3job_rc(&j, 0, 0, 1) == AB3JOB_RC_WARN);
    // a warning and a compile error: 10; nothing written: 20
    CHECK(ab3job_rc(&j, 0, 1, 0) == AB3JOB_RC_ERROR);
    CHECK(ab3job_rc(&j, 0, 0, 0) == AB3JOB_RC_FAIL);
    // set-up errors: all warnings that are errors 10, else 20 (also
    // with a printed warning)
    j.warnerrs = 1;
    CHECK(ab3job_rc(&j, 1, 0, 0) == AB3JOB_RC_ERROR);
    CHECK(ab3job_rc(&j, 2, 0, 0) == AB3JOB_RC_FAIL);
    j.warnings = 0;
    j.warnerrs = 0;
    CHECK(ab3job_rc(&j, 1, 0, 0) == AB3JOB_RC_FAIL);
    CHECK(ab3job_rc(&j, 0, 1, 1) == AB3JOB_RC_ERROR);
}
