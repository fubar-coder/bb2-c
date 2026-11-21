// test_basictest - the codec of the Blitz test library's result file
// (shared/basictest, spec testing/basic-test-interface): every entry kind
// written to its line and read back unchanged, the encodings of K, long
// and string values, a buffer too small, and every refused input of the
// reader (unknown entry, wrong number of fields, malformed value, unknown
// format version), each naming its line.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "basictest.h"
}

namespace {

basictest_str str(const std::string &s)
{
    return { reinterpret_cast<const uint8_t *>(s.data()), s.size() };
}

std::string text(const basictest_str &s)
{
    return std::string(reinterpret_cast<const char *>(s.data), s.length);
}

basictest_entry entry(int kind)
{
    basictest_entry e{};
    e.kind = kind;
    return e;
}

std::string write(const basictest_entry &e)
{
    std::vector<uint8_t> buf(4096);
    size_t n = basictest_write(&e, buf.data(), buf.size());
    return std::string(reinterpret_cast<const char *>(buf.data()), n);
}

// a read line and what came of it; the entry's strings point into line
struct read_result {
    std::string line;
    basictest_entry e{};
    basictest_error err{};
    int rc = -1;
    std::string message;
};

read_result read(const std::string &l, uint32_t number = 7)
{
    read_result r;
    r.line = l;
    r.rc = basictest_read(reinterpret_cast<uint8_t *>(r.line.data()), r.line.size(), number,
                          &r.e, &r.err);
    char msg[200];
    basictest_error_text(&r.err, msg, sizeof msg);
    r.message = msg;
    return r;
}

// a written line without its LF, read back
read_result roundtrip(const basictest_entry &e)
{
    std::string l = write(e);
    REQUIRE(!l.empty());
    REQUIRE(l.back() == '\n');
    l.pop_back();
    REQUIRE(l.find('\n') == std::string::npos);
    return read(l);
}

} // namespace

TEST_CASE("begin: the format version 1", "[basictest]")
{
    auto e = entry(BASICTEST_BEGIN);
    e.version = BASICTEST_VERSION;
    CHECK(write(e) == "begin\t1\n");
    auto r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_BEGIN);
    CHECK(r.e.version == 1u);
}

TEST_CASE("block: init, case NAME, final", "[basictest]")
{
    auto e = entry(BASICTEST_BLOCK);
    e.block = BASICTEST_BLOCK_INIT;
    CHECK(write(e) == "block\tinit\n");
    auto r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_BLOCK);
    CHECK(r.e.block == BASICTEST_BLOCK_INIT);

    e.block = BASICTEST_BLOCK_FINAL;
    CHECK(write(e) == "block\tfinal\n");
    r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.block == BASICTEST_BLOCK_FINAL);

    std::string name = "anim_frames_2";
    e.block = BASICTEST_BLOCK_CASE;
    e.name = str(name);
    CHECK(write(e) == "block\tcase anim_frames_2\n");
    r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.block == BASICTEST_BLOCK_CASE);
    CHECK(text(r.e.name) == name);
}

TEST_CASE("check: the spec's failed check 3 of case a", "[basictest]")
{
    auto e = entry(BASICTEST_CHECK);
    e.k = 3;
    e.ok = 0;
    e.expected = 16;
    e.actual = 0;
    CHECK(write(e) == "check\t3\tfail\t$00000010\t$00000000\n");
    auto r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_CHECK);
    CHECK(r.e.k == 3u);
    CHECK(r.e.ok == 0);
    CHECK(r.e.expected == 16u);
    CHECK(r.e.actual == 0u);
}

TEST_CASE("check: K and long values at their limits", "[basictest]")
{
    const uint32_t ks[] = { 0, 9, 10, 99, 100, 1000000000, 4294967295u };
    const char *kt[] = { "0", "9", "10", "99", "100", "1000000000", "4294967295" };
    for (size_t i = 0; i < sizeof ks / sizeof ks[0]; i++) {
        auto e = entry(BASICTEST_CHECK);
        e.k = ks[i];
        e.ok = 1;
        e.expected = 0xFFFFFFFFu;
        e.actual = 0x89ABCDEFu;
        CHECK(write(e) == std::string("check\t") + kt[i] + "\tok\t$FFFFFFFF\t$89ABCDEF\n");
        auto r = roundtrip(e);
        REQUIRE(r.rc == BASICTEST_OK);
        CHECK(r.e.k == ks[i]);
        CHECK(r.e.ok == 1);
        CHECK(r.e.expected == 0xFFFFFFFFu);
        CHECK(r.e.actual == 0x89ABCDEFu);
    }
}

TEST_CASE("checkstr: the spec's string with a tab, and every byte", "[basictest]")
{
    std::string exp = "ab", act = "a\tb";
    auto e = entry(BASICTEST_CHECKSTR);
    e.k = 12;
    e.ok = 0;
    e.expected_str = str(exp);
    e.actual_str = str(act);
    CHECK(write(e) == "checkstr\t12\tfail\tab\ta\\x09b\n");
    auto r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_CHECKSTR);
    CHECK(r.e.k == 12u);
    CHECK(text(r.e.expected_str) == exp);
    CHECK(text(r.e.actual_str) == act);

    std::string all;
    for (int c = 0; c < 256; c++)
        all.push_back(char(c));
    std::string empty;
    e.ok = 1;
    e.expected_str = str(all);
    e.actual_str = str(empty);
    std::string l = write(e);
    CHECK(l.find("\\x00\\x01") != std::string::npos);
    CHECK(l.find(" !\"#") != std::string::npos);
    CHECK(l.find("[\\x5C]") != std::string::npos);
    CHECK(l.find("}~\\x7F\\x80") != std::string::npos);
    CHECK(l.find("\\xFF\t\n") != std::string::npos);
    r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(text(r.e.expected_str) == all);
    CHECK(r.e.actual_str.length == 0u);
}

TEST_CASE("require, expect-error, blockend, broken, incomplete, end", "[basictest]")
{
    auto e = entry(BASICTEST_REQUIRE);
    e.k = 5;
    CHECK(write(e) == "require\t5\tbroken\n");
    auto r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_REQUIRE);
    CHECK(r.e.k == 5u);

    e = entry(BASICTEST_EXPECT_ERROR);
    e.k = 41;
    CHECK(write(e) == "expect-error\t41\n");
    r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_EXPECT_ERROR);
    CHECK(r.e.k == 41u);

    std::string why = "state damaged\\guard";
    e = entry(BASICTEST_BROKEN);
    e.reason = str(why);
    CHECK(write(e) == "broken\tstate damaged\\x5Cguard\n");
    r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_BROKEN);
    CHECK(text(r.e.reason) == why);

    const struct {
        int kind;
        const char *line;
    } plain[] = { { BASICTEST_BLOCKEND, "blockend\n" },
                  { BASICTEST_INCOMPLETE, "incomplete\n" },
                  { BASICTEST_END, "end\n" } };
    for (const auto &p : plain) {
        e = entry(p.kind);
        CHECK(write(e) == p.line);
        r = roundtrip(e);
        REQUIRE(r.rc == BASICTEST_OK);
        CHECK(r.e.kind == p.kind);
    }
}

TEST_CASE("writer: a buffer too small, an unknown kind, a case without name", "[basictest]")
{
    std::string exp = "x\ty", act = "\\";
    auto e = entry(BASICTEST_CHECKSTR);
    e.k = 123;
    e.expected_str = str(exp);
    e.actual_str = str(act);
    std::string want = "checkstr\t123\tfail\tx\\x09y\t\\x5C\n";
    REQUIRE(write(e) == want);
    for (size_t size = 0; size <= want.size(); size++) {
        std::vector<uint8_t> buf(size + 8, 0xAA);
        size_t n = basictest_write(&e, buf.data(), size);
        if (size < want.size()) {
            CHECK(n == 0u);
        } else {
            CHECK(n == want.size());
        }
        for (size_t i = size; i < buf.size(); i++)
            CHECK(buf[i] == 0xAA);              // nothing behind the buffer
    }

    std::vector<uint8_t> buf(64);
    e = entry(BASICTEST_END + 1);
    CHECK(basictest_write(&e, buf.data(), buf.size()) == 0u);
    e = entry(-1);
    CHECK(basictest_write(&e, buf.data(), buf.size()) == 0u);
    e = entry(BASICTEST_BLOCK);
    e.block = BASICTEST_BLOCK_CASE;
    CHECK(basictest_write(&e, buf.data(), buf.size()) == 0u);
    e.block = 7;
    CHECK(basictest_write(&e, buf.data(), buf.size()) == 0u);
}

TEST_CASE("reader: an unknown entry names the line", "[basictest]")
{
    const char *lines[] = { "frob\t1", "", "Begin\t1", "begin ", "check ", "caseend\ta\tok",
                            "end\r" };
    for (const char *l : lines) {
        auto r = read(l, 12);
        CAPTURE(l);
        CHECK(r.rc == BASICTEST_E_KIND);
        CHECK(r.err.line == 12u);
        CHECK(r.message.starts_with("line 12: "));
        CHECK(r.message.ends_with("unknown entry"));
    }
    CHECK(read("frob\t1", 3).message == "line 3: frob: unknown entry");
    CHECK(read("end\r", 3).message == "line 3: end\\x0D: unknown entry");
}

TEST_CASE("reader: a wrong number of fields names the line", "[basictest]")
{
    const char *lines[] = {
        "begin",       "begin\t1\t1",  "block",        "block\tinit\tx",
        "check\t1\tok\t$00000000",     "check\t1\tok\t$00000000\t$00000000\t$00000000",
        "checkstr\t1\tok\ta",          "checkstr\t1\tok\ta\tb\tc",
        "require\t1",  "require\t1\tbroken\tx",  "expect-error", "expect-error\t1\t2",
        "blockend\tx", "broken",       "broken\ta\tb", "incomplete\t", "end\t",
        "check\t1\tok\t$00000000\t$00000000\t\t\t\t",
    };
    for (const char *l : lines) {
        auto r = read(l, 99);
        CAPTURE(l);
        CHECK(r.rc == BASICTEST_E_FIELDS);
        CHECK(r.err.line == 99u);
        CHECK(r.message.starts_with("line 99: "));
        CHECK(r.message.ends_with("wrong number of fields"));
    }
    CHECK(read("end\t", 4).message == "line 4: end: wrong number of fields");
}

TEST_CASE("reader: a malformed value names the line and the field", "[basictest]")
{
    const struct {
        const char *line;
        unsigned field;
    } cases[] = {
        // the spec's scenario: an actual value of 4 hex digits
        { "check\t3\tfail\t$00000010\t$1234", 5 },
        { "check\t3\tfail\t$0000001\t$00000000", 4 },
        { "check\t3\tfail\t$000000100\t$00000000", 4 },
        { "check\t3\tfail\t00000010\t$00000000", 4 },
        { "check\t3\tfail\t$0000001a\t$00000000", 4 },
        { "check\t3\tfail\t$0000001G\t$00000000", 4 },
        { "check\t3\tfail\t$00000010\t", 5 },
        { "check\t3\tFAIL\t$00000010\t$00000000", 3 },
        { "check\t3\t\t$00000010\t$00000000", 3 },
        { "check\t03\tok\t$00000010\t$00000010", 2 },
        { "check\t-3\tok\t$00000010\t$00000010", 2 },
        { "check\t+3\tok\t$00000010\t$00000010", 2 },
        { "check\t\tok\t$00000010\t$00000010", 2 },
        { "check\t3 \tok\t$00000010\t$00000010", 2 },
        { "check\t4294967296\tok\t$00000010\t$00000010", 2 },
        { "check\t9999999999\tok\t$00000010\t$00000010", 2 },
        { "check\t12345678901\tok\t$00000010\t$00000010", 2 },
        { "checkstr\t1\tok\ta\\x9\tb", 4 },
        { "checkstr\t1\tok\ta\\x09\tb\\", 5 },
        { "checkstr\t1\tok\ta\\t\tb", 4 },
        { "checkstr\t1\tok\ta\\x0d\tb", 4 },
        { "checkstr\t1\tok\ta\\x41\tb", 4 },      // A stands as it is
        { "checkstr\t1\tok\ta\tb\\x20", 5 },      // so does the blank
        { "checkstr\t1\tok\ta\tb\r", 5 },
        { "checkstr\t1\tok\ta\x7F\tb", 4 },
        { "checkstr\t1\tok\ta\xC4\tb", 4 },
        { "require\t1\tok", 3 },
        { "require\tx\tbroken", 2 },
        { "expect-error\t1x", 2 },
        { "broken\tbad\\", 2 },
        { "block\tcase", 2 },
        { "block\tcase ", 2 },
        { "block\tcase\ta", 0 },                  // three fields: see below
        { "block\tcase_a", 2 },
        { "block\tInit", 2 },
        { "block\tcase a\\x5c", 2 },
        { "begin\t", 2 },
        { "begin\t01", 2 },
        { "begin\tone", 2 },
    };
    for (const auto &c : cases) {
        auto r = read(c.line, 21);
        CAPTURE(c.line);
        if (c.field == 0) {
            CHECK(r.rc == BASICTEST_E_FIELDS);
            continue;
        }
        CHECK(r.rc == BASICTEST_E_VALUE);
        CHECK(r.err.line == 21u);
        CHECK(r.err.field == c.field);
        CHECK(r.message.starts_with("line 21: "));
        CHECK(r.message.ends_with("malformed value in field " + std::to_string(c.field)));
    }
    CHECK(read("check\t3\tfail\t$00000010\t$1234", 8).message
          == "line 8: check: malformed value in field 5");
}

TEST_CASE("reader: an unknown format version names the line", "[basictest]")
{
    const char *lines[] = { "begin\t0", "begin\t2", "begin\t10", "begin\t4294967295" };
    for (const char *l : lines) {
        auto r = read(l, 1);
        CAPTURE(l);
        CHECK(r.rc == BASICTEST_E_VERSION);
        CHECK(r.err.line == 1u);
        CHECK(r.message == "line 1: begin: unknown format version");
    }
}

TEST_CASE("reader: without an error structure", "[basictest]")
{
    std::string l = "check\t3\tfail\t$00000010\t$1234";
    basictest_entry e{};
    CHECK(basictest_read(reinterpret_cast<uint8_t *>(l.data()), l.size(), 1, &e, nullptr)
          == BASICTEST_E_VALUE);
    std::string ok = "end";
    CHECK(basictest_read(reinterpret_cast<uint8_t *>(ok.data()), ok.size(), 1, &e, nullptr)
          == BASICTEST_OK);
}

TEST_CASE("error text: cut to the buffer", "[basictest]")
{
    auto r = read("check\t3\tfail\t$00000010\t$1234", 4000000000u);
    std::string full = "line 4000000000: check: malformed value in field 5";
    CHECK(r.message == full);
    for (size_t size = 1; size <= full.size() + 1; size++) {
        std::vector<char> buf(size + 4, 'Z');
        size_t n = basictest_error_text(&r.err, buf.data(), size);
        CHECK(n == std::min(size - 1, full.size()));
        CHECK(std::string(buf.data()) == full.substr(0, n));
        for (size_t i = size; i < buf.size(); i++)
            CHECK(buf[i] == 'Z');
    }
}
