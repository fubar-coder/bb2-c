// test_testreg - the bundle tests of the registration (tools/lib/testreg,
// change basic-test-bundle-registration) on sample directories: the
// groups by session key, labels and fixtures, the chunks of 16, the
// commands and DEPENDS of the bundles and their members, the cases that
// are in no bundle, the registration without bundles, the COST of a
// bundle (the sum of its members', a measured entry, the order of the
// label bug-orig).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "testcosts.h"
#include "testreg.h"
}

namespace fs = std::filesystem;

static char err[800];

// a sample directory: a root tests.toml, the sources added by add(), an
// environment with the time limit 90
struct sample {
    fs::path root;
    tc_env env = {};
    std::vector<std::string> sources;
    treg_result r = {};
    treg_opts o = {};

    sample()
    {
        std::string tmp = (fs::temp_directory_path() / "testreg-XXXXXX").string();

        REQUIRE(mkdtemp(tmp.data()) != nullptr);
        root = tmp;
        std::ofstream(root / "tests.toml") << "root = true\n";
        std::string e = "top = \"" + root.string() + "\"\n[runner]\ntimeout = \"90\"\n";
        REQUIRE(tc_env_parse("env.toml", e.c_str(), &env, err, sizeof err) == 0);
        o.prefix = "p_";
        o.env = &env;
        o.timeout_default = 90;
        o.timeout_extra = 30;
        o.bundle = 1;
    }
    ~sample()
    {
        treg_free(&r);
        tc_env_free(&env);
        fs::remove_all(root);
    }
    // a source ID.test.bas with ncases plain cases c1.. (or the text), its
    // ID.test.toml (if any)
    void add(const std::string &id, int ncases, const std::string &toml = "", const std::string &text = "")
    {
        std::string t = text;

        if (t.empty())
            for (int i = 1; i <= ncases; i++)
                t += "!TestCase{c" + std::to_string(i) + "}\n  !Check{1,\"k\"}\n!EndTestCase\n";
        std::ofstream(root / (id + ".test.bas")) << t;
        if (!toml.empty())
            std::ofstream(root / (id + ".test.toml")) << toml;
        sources.push_back((root / (id + ".test.bas")).string());
    }
    // register every source (and the bundles)
    void reg()
    {
        for (const std::string &s : sources)
            REQUIRE(treg_add_source(s.c_str(), &o, &r) == 0);
        if (o.bundle)
            REQUIRE(treg_make_bundles(&r, &o) == 0);
        REQUIRE(treg_check_names(&r) == 0);
        for (size_t i = 0; i < r.errors.n; i++)
            FAIL(r.errors.v[i]);
    }
    const treg_test *test(const std::string &name) const
    {
        for (size_t i = 0; i < r.n; i++)
            if (name == r.t[i].name)
                return &r.t[i];
        return nullptr;
    }
    std::vector<const treg_test *> bundles() const
    {
        std::vector<const treg_test *> v;

        for (size_t i = 0; i < r.n; i++)
            if (r.t[i].bundle)
                v.push_back(&r.t[i]);
        return v;
    }
    // the member names of a bundle, in its order
    std::vector<std::string> members(const treg_test *b) const
    {
        std::vector<std::string> v;

        for (size_t k = 0; k < b->nmembers; k++)
            v.push_back(r.t[b->members[k]].name);
        return v;
    }
    // the include file, the brackets removed
    std::string include(const treg_entries *order = nullptr) const
    {
        static const char *const cmd[] = { "run", "case" };
        char *buf = nullptr;
        size_t len = 0;
        FILE *f = open_memstream(&buf, &len);

        REQUIRE(f != nullptr);
        REQUIRE(treg_write_include(f, &r, cmd, 2, order, nullptr) == 0);
        fclose(f);
        std::string s(buf, len), out;
        free(buf);
        for (size_t i = 0; i < s.size(); i++) {
            if (!s.compare(i, 4, "[==[") || !s.compare(i, 4, "]==]"))
                i += 3;
            else
                out += s[i];
        }
        return out;
    }
    std::string path(const std::string &id) const
    {
        return (root / (id + ".test.bas")).string();
    }
    // an order file of the old form (cmake/bug-orig-order.cmake)
    void order(const std::string &text, treg_entries *e) const
    {
        fs::path f = root / "order.cmake";

        std::ofstream(f) << text;
        REQUIRE(treg_entries_load(f.c_str(), e, err, sizeof err) == 0);
    }
};

static bool contains(const std::string &s, const std::string &part)
{
    if (s.find(part) != std::string::npos)
        return true;
    INFO("lacks ", part);
    return false;
}

TEST_CASE("testreg: forty plain cases give bundles of 16, 16 and 8", "[testreg]")
{
    sample s;

    for (int i = 0; i < 4; i++)
        s.add("t" + std::to_string(i), 10);
    s.reg();
    auto b = s.bundles();
    REQUIRE(b.size() == 3);
    CHECK(std::string(b[0]->name) == "p_bundle_1");
    CHECK(std::string(b[1]->name) == "p_bundle_2");
    CHECK(std::string(b[2]->name) == "p_bundle_3");
    CHECK(b[0]->nmembers == 16);
    CHECK(b[1]->nmembers == 16);
    CHECK(b[2]->nmembers == 8);
    // the order of the sources and cases
    std::vector<std::string> all;
    for (const treg_test *x : b)
        for (const std::string &m : s.members(x))
            all.push_back(m);
    REQUIRE(all.size() == 40);
    for (int i = 0; i < 40; i++)
        CHECK(all[i] == "p_t" + std::to_string(i / 10) + "__c" + std::to_string(i % 10 + 1));
    // the members name their bundle
    CHECK(std::string(s.test("p_t0__c1")->from_bundle) == "p_bundle_1");
    CHECK(std::string(s.test("p_t1__c6")->from_bundle) == "p_bundle_1");
    CHECK(std::string(s.test("p_t1__c7")->from_bundle) == "p_bundle_2");
    CHECK(std::string(s.test("p_t3__c3")->from_bundle) == "p_bundle_3");
    CHECK(std::string(s.test("p_t3__c10")->from_bundle) == "p_bundle_3");

    std::string inc = s.include();
    // the bundle: I1, its labels, 3 x 90 + 30, no COST (no member has one)
    CHECK(contains(inc, "--member p_t2__c10 " + s.path("t2") + " c10 --member p_t3__c1 " + s.path("t3")
                            + " c1 --member p_t3__c2 " + s.path("t3") + " c2)\n"));
    CHECK(contains(inc, "add_test( p_bundle_3 run case --bundle p_bundle_3 --member p_t3__c3 " + s.path("t3")
                            + " c3 --member p_t3__c4 "));
    CHECK(contains(inc, "--member p_t3__c10 " + s.path("t3") + " c10)\n"));
    CHECK(contains(inc, "set_tests_properties( p_bundle_1 PROPERTIES LABELS emulator TIMEOUT 300 "
                        "SKIP_RETURN_CODE 77)\n"));
    // a member: I2 and DEPENDS; name, labels, TIMEOUT stay
    CHECK(contains(inc, "add_test( p_t1__c7 run case --source " + s.path("t1")
                            + " --case c7 --side port --test p_t1__c7 --from-bundle p_bundle_2)\n"));
    CHECK(contains(inc, "set_tests_properties( p_t1__c7 PROPERTIES LABELS emulator TIMEOUT 120 SKIP_RETURN_CODE 77 "
                        "DEPENDS p_bundle_2)\n"));
}

TEST_CASE("testreg: seventeen plain cases give bundles of 16 and 1", "[testreg]")
{
    sample s;

    s.add("a", 9);
    s.add("b", 8);
    s.reg();
    auto b = s.bundles();
    REQUIRE(b.size() == 2);
    CHECK(b[0]->nmembers == 16);
    CHECK(b[1]->nmembers == 1);
    CHECK(s.members(b[1]) == std::vector<std::string>{ "p_b__c8" });
    std::string inc = s.include();
    CHECK(contains(inc, "add_test( p_bundle_2 run case --bundle p_bundle_2 --member p_b__c8 " + s.path("b")
                            + " c8)\n"));
}

TEST_CASE("testreg: different labels, fixtures or keys give different bundles", "[testreg]")
{
    sample s;

    s.add("plain-a", 1);
    s.add("flaky-a", 1, "flaky = \"x\"\n");
    s.add("lab-a", 1, "labels = [\"y\"]\n");
    s.add("fix-a", 1, "fixtures = [\"f\"]\n");
    s.add("t30-a", 1, "timeout = 30\n");
    s.add("t60-a", 1, "timeout = 60\n");
    // the same key as plain: the environment's time limit set
    s.add("t90-a", 1, "timeout = 90\n");
    // the labels as a set: the same group as lab-a
    s.add("lab-b", 1, "labels = [\"y\", \"emulator\"]\n");
    s.add("t30-b", 1, "timeout = 30\n");
    s.reg();
    auto b = s.bundles();
    REQUIRE(b.size() == 6);
    CHECK(s.members(b[0]) == std::vector<std::string>{ "p_plain_a__c1", "p_t90_a__c1" });
    CHECK(s.members(b[1]) == std::vector<std::string>{ "p_flaky_a__c1" });
    CHECK(s.members(b[2]) == std::vector<std::string>{ "p_lab_a__c1", "p_lab_b__c1" });
    CHECK(s.members(b[3]) == std::vector<std::string>{ "p_fix_a__c1" });
    CHECK(s.members(b[4]) == std::vector<std::string>{ "p_t30_a__c1", "p_t30_b__c1" });
    CHECK(s.members(b[5]) == std::vector<std::string>{ "p_t60_a__c1" });
    std::string inc = s.include();
    // the group's labels and fixtures; the key's limit
    CHECK(contains(inc, "set_tests_properties( p_bundle_2 PROPERTIES LABELS emulator;flaky TIMEOUT 300 "));
    CHECK(contains(inc, "set_tests_properties( p_bundle_3 PROPERTIES LABELS emulator;y TIMEOUT 300 "));
    CHECK(contains(inc, "set_tests_properties( p_bundle_4 PROPERTIES LABELS emulator TIMEOUT 300 SKIP_RETURN_CODE 77 "
                        "FIXTURES_REQUIRED f)\n"));
    CHECK(contains(inc, "set_tests_properties( p_bundle_5 PROPERTIES LABELS emulator TIMEOUT 120 "));
    CHECK(contains(inc, "set_tests_properties( p_bundle_6 PROPERTIES LABELS emulator TIMEOUT 210 "));
    CHECK(contains(inc, "set_tests_properties( p_fix_a__c1 PROPERTIES LABELS emulator TIMEOUT 120 SKIP_RETURN_CODE 77 "
                        "FIXTURES_REQUIRED f DEPENDS p_bundle_4)\n"));
}

TEST_CASE("testreg: the cases in no bundle", "[testreg]")
{
    sample s;

    // a case that asserts the output; the other case of the source is bundled
    s.add("out", 2, "[asserts.\"case=c1\"]\noutput = \"contains \\\"x\\\"\"\n");
    // two cells on the side port
    s.add("two", 1, "[matrix]\nruntime-errors = [\"off\", \"on\"]\n");
    // an isolated cell
    s.add("iso", 1, "isolate = \"on\"\n");
    // a cell of the side originals beside the port cell: the port side only
    s.add("orig", 1, "[matrix]\nlibs = [\"cd\", \"clib\"]\n");
    // a time limit of no plain key
    s.add("t45", 1, "timeout = 45\n");
    s.reg();
    auto b = s.bundles();
    REQUIRE(b.size() == 1);
    CHECK(s.members(b[0]) == std::vector<std::string>{ "p_out__c2", "p_orig__c1" });
    for (const char *n : { "p_out__c1", "p_two__c1", "p_iso__c1", "p_orig__c1_originals", "p_t45__c1" }) {
        const treg_test *t = s.test(n);

        REQUIRE(t != nullptr);
        CHECK(t->from_bundle == nullptr);
        CHECK(t->key == nullptr);
    }
    std::string inc = s.include();
    CHECK(contains(inc, "add_test( p_out__c1 run case --source " + s.path("out") + " --case c1 --side port)\n"));
    CHECK(contains(inc, "add_test( p_orig__c1_originals run case --source " + s.path("orig")
                            + " --case c1 --side originals)\n"));
    CHECK(contains(inc, "set_tests_properties( p_iso__c1 PROPERTIES LABELS emulator TIMEOUT 120 "
                        "SKIP_RETURN_CODE 77)\n"));
}

TEST_CASE("testreg: without bundles, no bundle test and no DEPENDS", "[testreg]")
{
    sample s, w;

    s.o.bundle = 0;
    s.add("a", 20);
    s.reg();
    CHECK(s.bundles().empty());
    for (size_t i = 0; i < s.r.n; i++) {
        CHECK(s.r.t[i].key == nullptr);
        CHECK(s.r.t[i].from_bundle == nullptr);
    }
    std::string inc = s.include();
    CHECK(inc.find("bundle") == std::string::npos);
    CHECK(inc.find("DEPENDS") == std::string::npos);
    CHECK(inc.find("--test") == std::string::npos);
    CHECK(contains(inc, "add_test( p_a__c20 run case --source " + s.path("a") + " --case c20 --side port)\n"));
    // a registration with keys but no treg_make_bundles: the same file
    w.add("a", 20);
    for (const std::string &src : w.sources)
        REQUIRE(treg_add_source(src.c_str(), &w.o, &w.r) == 0);
    std::string winc = w.include();
    // the paths differ by the sample directory
    size_t at;
    while ((at = winc.find(w.root.string())) != std::string::npos)
        winc.replace(at, w.root.string().size(), s.root.string());
    CHECK(winc == inc);
}

TEST_CASE("testreg: the COST of a bundle", "[testreg]")
{
    // open bugs: each member's COST is its session limit (90); the bundle's
    // the sum, ordered with the tests of the label bug-orig by it
    sample s;

    for (int i = 0; i < 3; i++)
        s.add("open" + std::to_string(i), 1, "[deviations.\"compiler=port\"]\n\"check.c1.k\" = \"== fail\"\n");
    s.add("plain", 2);
    s.add("cost", 1, "cost = 7\n");
    s.reg();
    auto b = s.bundles();
    REQUIRE(b.size() == 2);
    CHECK(s.members(b[0]) == std::vector<std::string>{ "p_open0__c1", "p_open1__c1", "p_open2__c1" });
    CHECK(b[0]->has_cost);
    CHECK(b[0]->cost == 270);
    // a configured cost is no key: one group; the sum of the COSTs there are
    CHECK(s.members(b[1]) == std::vector<std::string>{ "p_plain__c1", "p_plain__c2", "p_cost__c1" });
    CHECK(b[1]->has_cost);
    CHECK(b[1]->cost == 7);

    treg_entries order = {};
    s.order("# @test p_old_big 300\nadd_test(p_old_big run)\n# @test p_old_mid 100\nadd_test(p_old_mid run)\n",
            &order);
    std::string inc = s.include(&order);
    size_t sect = inc.find("# the tests of the label bug-orig"), big = inc.find("add_test(p_old_big"),
           bun = inc.find("add_test( p_bundle_1 "), mid = inc.find("add_test(p_old_mid"),
           mem = inc.find("add_test( p_open0__c1 ");
    REQUIRE(sect != std::string::npos);
    CHECK(sect < big);
    CHECK(big < bun);
    CHECK(bun < mid);
    CHECK(mid < mem);
    CHECK(contains(inc, "set_tests_properties( p_bundle_1 PROPERTIES LABELS bug-orig;emulator TIMEOUT 300 "
                        "SKIP_RETURN_CODE 77 COST 270)\n"));
    CHECK(contains(inc, "set_tests_properties( p_open1__c1 PROPERTIES LABELS bug-orig;emulator TIMEOUT 120 "
                        "SKIP_RETURN_CODE 77 COST 90 DEPENDS p_bundle_1)\n"));
    treg_entries_free(&order);

    // a measured entry of a bundle is applied; without one, the sum of the
    // members' measured COSTs
    tcosts c = {};
    REQUIRE(tcosts_parse("costs.txt", "p_bundle_1 12\np_open0__c1 5\np_plain__c1 4\np_plain__c2 6\n", 1, &c, err,
                         sizeof err)
            == 0);
    treg_apply_costs(&s.r, &c);
    tcosts_free(&c);
    b = s.bundles();
    CHECK(b[0]->cost == 12);
    CHECK(b[1]->cost == 17);
    CHECK(s.test("p_open0__c1")->cost == 5);
    inc = s.include();
    CHECK(contains(inc, "set_tests_properties( p_bundle_2 PROPERTIES LABELS emulator TIMEOUT 300 SKIP_RETURN_CODE 77 "
                        "COST 17)\n"));
}

TEST_CASE("testreg: a bundle's name is checked against the tests of the old form", "[testreg]")
{
    sample s;
    treg_entries order = {};

    s.add("a", 1);
    s.reg();
    s.order("# @test p_bundle_1 5\nadd_test(p_bundle_1 run)\n", &order);
    CHECK(treg_check_entries(&s.r, &order) == 1);
    REQUIRE(s.r.errors.n == 1);
    CHECK(contains(s.r.errors.v[0], "the test p_bundle_1 is also a test of the old form"));
    treg_entries_free(&order);
}
