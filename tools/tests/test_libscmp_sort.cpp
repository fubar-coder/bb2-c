// test_libscmp_sort - the order in which the writers of the library tests'
// expected data put the cases (tools/lib/libscmp, libscmp_sorted): the
// header first, the keyed cases by ROUTINE, CHECK and VECTOR as text and
// OCC as a number, KEY [orig] directly before KEY [fixed #N], KEY [orig #N]
// alone where [orig] would be, the BLOCK:N cases last by N; a sorted file sorts to itself, every order of the same
// cases gives the same file, no case changes; libs-accept and libs-update
// write in that order; libs-sort on a directory.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>

extern "C" {
#include "libscmp.h"
}

namespace {

const std::string hdr = "# capture prog 1\n";

struct sorted_result {
    int status;
    std::string text, err;
};

sorted_result sort(const std::string &text)
{
    char *out = nullptr, err[700] = "";
    sorted_result r;

    r.status = libscmp_sort_text(text.data(), text.size(), "f.txt", &out, err, sizeof err);
    r.text = out ? out : "";
    r.err = err;
    free(out);
    return r;
}

// the lines of text after the header
std::vector<std::string> lines(const std::string &text)
{
    std::vector<std::string> v;
    std::istringstream in(text);
    std::string l;

    while (std::getline(in, l))
        v.push_back(l);
    if (!v.empty())
        v.erase(v.begin());
    return v;
}

std::string join(const std::vector<std::string> &v)
{
    std::string s = hdr;

    for (const std::string &l : v)
        s += l + "\n";
    return s;
}

// the cases in the writers' order
const std::vector<std::string> want = {
    "|routine count||1\t$00000002",
    "a|c|v|1\t$1",
    "a|c|v|2\t$2",
    "a|c|v|10\t$3",
    "a|c|w|1\t$4",
    "a|c\\||v|1\t$5",           // "c\|" after "c": a prefix first
    "a|cc|v|1\t$6",             // "c\|" before "cc": '\' < 'c'
    "a|d|v|1 [orig]\t$7",
    "a|d|v|1 [fixed #12]\t$8",
    "a|d|v|2 [fixed #3]\t$9",
    "a|d|v|3 [orig]\t$A",
    "a|d|v|4 [orig #469]\t$D",  // a case of the original only, alone
    "ab|c|v|1\t$B",
    "b|\\x01|v|1\t$C",
    "b|\\x01|w|1\tnull",
    // the blocks last, although "B" < "a"
    "BLOCK:1 [orig]\t4096 00000001 first=a|c|v|1",
    "BLOCK:1 [fixed #200]\t4096 00000002 first=a|c|v|1",
    "BLOCK:2\t4096 00000003 first=a|c|v|4097",
    "BLOCK:10\t12 00000004 first=a|c|v|36865",
};

} // namespace

TEST_CASE("the writers' order", "[libscmp][sort]")
{
    std::vector<std::string> in = want;

    std::reverse(in.begin(), in.end());
    sorted_result r = sort(join(in));
    REQUIRE(r.status == 0);
    CHECK(r.text == join(want));
}

TEST_CASE("OCC and BLOCK:N compare as numbers", "[libscmp][sort]")
{
    sorted_result r = sort(hdr + "r|c|v|10\t$0\nBLOCK:10\tx\nr|c|v|2\t$0\nBLOCK:2\tx\nr|c|v|1\t$0\n"
                                 "BLOCK:1\tx\n");
    REQUIRE(r.status == 0);
    CHECK(r.text == hdr + "r|c|v|1\t$0\nr|c|v|2\t$0\nr|c|v|10\t$0\nBLOCK:1\tx\nBLOCK:2\tx\n"
                          "BLOCK:10\tx\n");
}

TEST_CASE("[orig] directly before [fixed #N] of its key", "[libscmp][sort]")
{
    sorted_result r = sort(hdr + "r|c|v|2 [fixed #5]\t$2\nr|c|v|1 [fixed #900]\t$1\n"
                                 "r|c|v|2 [orig]\t$0\nr|c|v|1 [orig]\t$0\n"
                                 "BLOCK:3 [fixed #7]\tb\nBLOCK:3 [orig]\ta\n");
    REQUIRE(r.status == 0);
    CHECK(r.text == hdr + "r|c|v|1 [orig]\t$0\nr|c|v|1 [fixed #900]\t$1\n"
                          "r|c|v|2 [orig]\t$0\nr|c|v|2 [fixed #5]\t$2\n"
                          "BLOCK:3 [orig]\ta\nBLOCK:3 [fixed #7]\tb\n");
}

TEST_CASE("[orig #N] stands alone at its key's place", "[libscmp][sort]")
{
    sorted_result r = sort(hdr + "r|c|v|3\t$3\nr|c|v|2 [orig #469]\t$2\nr|c|v|1 [orig #7]\t$1\n"
                                 "BLOCK:2 [orig #12]\tb\nBLOCK:1\ta\n");
    REQUIRE(r.status == 0);
    CHECK(r.text == hdr + "r|c|v|1 [orig #7]\t$1\nr|c|v|2 [orig #469]\t$2\nr|c|v|3\t$3\n"
                          "BLOCK:1\ta\nBLOCK:2 [orig #12]\tb\n");
    // a key with [orig] and [orig #N], or [orig #N] and [fixed #N]: malformed
    r = sort(hdr + "r|c|v|1 [orig]\t$0\nr|c|v|1 [orig #7]\t$1\n");
    CHECK(r.status == -2);
    CHECK(r.err.find("f.txt:3: r|c|v|1 has [orig] and [orig #N]") != std::string::npos);
    r = sort(hdr + "r|c|v|1 [orig #7]\t$0\nr|c|v|1 [fixed #7]\t$1\n");
    CHECK(r.status == -2);
    CHECK(r.err.find("f.txt:3: r|c|v|1 has [orig #N] and [fixed #N]") != std::string::npos);
}

TEST_CASE("parts compare one by one, as escaped text", "[libscmp][sort]")
{
    // as whole lines "a|z" would sort after "ab|a"; by parts "a" < "ab"
    sorted_result r = sort(hdr + "ab|a|v|1\t$0\na|z|v|1\t$0\na\\|b|a|v|1\t$0\n");
    REQUIRE(r.status == 0);
    CHECK(r.text == hdr + "a|z|v|1\t$0\na\\|b|a|v|1\t$0\nab|a|v|1\t$0\n");
}

TEST_CASE("sorting is idempotent and independent of the input order", "[libscmp][sort]")
{
    sorted_result first = sort(join(want));
    REQUIRE(first.status == 0);
    sorted_result again = sort(first.text);
    REQUIRE(again.status == 0);
    CHECK(again.text == first.text);
    // a few permutations: the same file
    for (unsigned seed = 1; seed <= 20; seed++) {
        std::vector<std::string> p = want;

        for (size_t i = p.size() - 1; i > 0; i--)
            std::swap(p[i], p[(seed * 7919u + i * 104729u) % (i + 1)]);
        sorted_result r = sort(join(p));
        REQUIRE(r.status == 0);
        CHECK(r.text == first.text);
    }
}

TEST_CASE("no case changes: the same lines", "[libscmp][sort]")
{
    std::vector<std::string> in = want;

    std::reverse(in.begin(), in.end());
    sorted_result r = sort(join(in));
    REQUIRE(r.status == 0);
    std::vector<std::string> a = lines(join(in)), b = lines(r.text);
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    CHECK(a == b);
}

TEST_CASE("the last line gets its newline; a malformed file is refused", "[libscmp][sort]")
{
    sorted_result r = sort(hdr + "r|b||1\t$1\nr|a||1\t$0");
    REQUIRE(r.status == 0);
    CHECK(r.text == hdr + "r|a||1\t$0\nr|b||1\t$1\n");
    r = sort(hdr + "r|a||1\t$0\nr|a||1\t$1\n");
    CHECK(r.status == -2);
    CHECK(r.err.find("f.txt:3:") != std::string::npos);
    r = sort(hdr);
    REQUIRE(r.status == 0);
    CHECK(r.text == hdr);
}

namespace {

struct written {
    int status;
    std::string text, report;
};

written accept(const std::string &run, const std::string *old)
{
    libscmp_file a, o;
    char err[700], *out = nullptr, *rep = nullptr;
    size_t replen = 0;
    written w;

    REQUIRE(libscmp_parse(&a, run.data(), run.size(), "run", LIBSCMP_RUN, err, sizeof err) == 0);
    if (old)
        REQUIRE(libscmp_parse(&o, old->data(), old->size(), "old", LIBSCMP_EXPECTED, err,
                              sizeof err) == 0);
    FILE *m = open_memstream(&rep, &replen);
    w.status = libscmp_accept(&a, &a, old ? &o : nullptr, &out, m);
    fclose(m);
    w.text = out ? out : "";
    w.report = rep ? rep : "";
    free(out);
    free(rep);
    libscmp_free(&a);
    if (old)
        libscmp_free(&o);
    return w;
}

} // namespace

TEST_CASE("libs-accept writes in the writers' order", "[libscmp][sort]")
{
    const std::string run = hdr + "r|c|v|10\t$3\nBLOCK:1\tb\nr|c|v|2\t$2\nr|b||1\t$1\n";
    const std::string old = hdr + "r|c|v|2 [fixed #9]\t$0\nr|c|v|2 [orig]\t$7\nr|a||1 [fixed #4]\t$5\n";
    written w = accept(run, &old);

    REQUIRE(w.status == 0);
    CHECK(w.text == hdr + "r|a||1 [fixed #4]\t$5\nr|b||1\t$1\nr|c|v|2 [orig]\t$2\n"
                          "r|c|v|2 [fixed #9]\t$0\nr|c|v|10\t$3\nBLOCK:1\tb\n");
    // accepted again on its own output: the same file
    written again = accept(run, &w.text);
    REQUIRE(again.status == 0);
    CHECK(again.text == w.text);
}

TEST_CASE("libs-update writes in the writers' order", "[libscmp][sort]")
{
    const std::string run = hdr + "r|new||1\t$9\nr|c|v|10\t$3\nr|c|v|2\t$4\nr|b||1\t$1\n";
    const std::string exp = hdr + "r|c|v|10\t$3\nr|c|v|2\t$2\nr|b||1\t$1\n";
    libscmp_file r, e;
    libscmp_fix fx[2];
    char err[700], *out = nullptr, *rep = nullptr;
    size_t replen = 0;

    REQUIRE(libscmp_parse(&r, run.data(), run.size(), "run", LIBSCMP_RUN, err, sizeof err) == 0);
    REQUIRE(libscmp_parse(&e, exp.data(), exp.size(), "exp", LIBSCMP_EXPECTED, err, sizeof err) == 0);
    REQUIRE(libscmp_parse_fix("r|c|v|2#5", &fx[0], err, sizeof err) == 0);
    REQUIRE(libscmp_parse_fix("r|new||1#6", &fx[1], err, sizeof err) == 0);
    FILE *m = open_memstream(&rep, &replen);
    int s = libscmp_update(&r, &e, fx, 2, &out, m);
    fclose(m);
    REQUIRE(s == 0);
    CHECK(std::string(out) == hdr + "r|b||1\t$1\nr|c|v|2 [orig]\t$2\nr|c|v|2 [fixed #5]\t$4\n"
                                    "r|c|v|10\t$3\nr|new||1 [fixed #6]\t$9\n");
    free(out);
    free(rep);
    libscmp_fix_free(&fx[0]);
    libscmp_fix_free(&fx[1]);
    libscmp_free(&r);
    libscmp_free(&e);
}

namespace {

std::string slurp(const std::filesystem::path &p)
{
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;

    s << f.rdbuf();
    return s.str();
}

void spit(const std::filesystem::path &p, const std::string &text)
{
    std::ofstream(p, std::ios::binary) << text;
}

struct dir_result {
    int status;
    std::string out, err;
};

dir_result sort_dir(const std::string &dir, std::vector<const char *> only = {})
{
    char *o = nullptr, *e = nullptr;
    size_t ol = 0, el = 0;
    FILE *out = open_memstream(&o, &ol), *err = open_memstream(&e, &el);
    dir_result r;

    r.status = libscmp_sort_dirs(dir.c_str(), only.data(), only.size(), out, err);
    fclose(out);
    fclose(err);
    r.out = o ? o : "";
    r.err = e ? e : "";
    free(o);
    free(e);
    return r;
}

} // namespace

TEST_CASE("libs-sort on a directory", "[libscmp][sort]")
{
    std::string tmpl = (std::filesystem::temp_directory_path() / "libscmp-sort-XXXXXX").string();
    REQUIRE(mkdtemp(tmpl.data()));
    const std::filesystem::path d = tmpl;
    const std::string a = "# capture a 1\nr|b||1\t$1\nr|a||1\t$0\n";
    const std::string b = "# capture b 1\nr|a||1\t$0\n";

    spit(d / "a.txt", a);
    spit(d / "b.txt", b);
    dir_result r = sort_dir(d.string());
    CHECK(r.status == 0);
    CHECK(r.out == "written: " + (d / "a.txt").string() + "\n1 files written, 1 unchanged\n");
    CHECK(slurp(d / "a.txt") == "# capture a 1\nr|a||1\t$0\nr|b||1\t$1\n");
    CHECK(slurp(d / "b.txt") == b);
    // again: nothing to write
    r = sort_dir(d.string());
    CHECK(r.status == 0);
    CHECK(r.out == "0 files written, 2 unchanged\n");
    // --only: the other files stay as they are
    spit(d / "a.txt", a);
    spit(d / "b.txt", "# capture b 1\nr|b||1\t$1\nr|a||1\t$0\n");
    r = sort_dir(d.string(), {"b"});
    CHECK(r.status == 0);
    CHECK(slurp(d / "a.txt") == a);
    CHECK(slurp(d / "b.txt") == "# capture b 1\nr|a||1\t$0\nr|b||1\t$1\n");
    // a malformed file: an error, the file untouched, the others sorted
    spit(d / "b.txt", "# capture b 1\nr|a||1\t$0\nr|a||1\t$1\n");
    r = sort_dir(d.string());
    CHECK(r.status == 2);
    CHECK(r.err.find("b.txt:3: duplicate") != std::string::npos);
    CHECK(slurp(d / "a.txt") == "# capture a 1\nr|a||1\t$0\nr|b||1\t$1\n");
    CHECK(slurp(d / "b.txt") == "# capture b 1\nr|a||1\t$0\nr|a||1\t$1\n");
    std::filesystem::remove_all(d);
}
