// test_basictest - the codec of the Blitz test library's result file
// (shared/basictest, spec testing/basic-test-interface): every entry kind
// written to its line and read back unchanged, the encodings of K, long
// and string values, lengths and CRC32s, the CRC32 against known vectors,
// a buffer too small, and every refused input of the reader (unknown
// entry, wrong number of fields, malformed value, a length or CRC32 that
// does not fit its value, unknown format version), each naming its line.
#include <algorithm>
#include <cstdint>
#include <cstdio>
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

uint32_t crc(const std::string &s)
{
    return basictest_crc32(reinterpret_cast<const uint8_t *>(s.data()), s.size());
}

std::string hex(uint32_t v)
{
    char b[12];
    snprintf(b, sizeof b, "$%08X", unsigned(v));
    return b;
}

// a checkstr entry of two whole strings: their lengths and CRC32s
basictest_entry checkstr(uint32_t k, int ok, const std::string &exp, const std::string &act)
{
    basictest_entry e = entry(BASICTEST_CHECKSTR);
    e.k = k;
    e.ok = ok;
    e.expected_str = str(exp);
    e.expected_length = uint32_t(exp.size());
    e.expected_crc = crc(exp);
    e.actual_str = str(act);
    e.actual_length = uint32_t(act.size());
    e.actual_crc = crc(act);
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
    CHECK(write(e) == "block\tcase\tanim_frames_2\n");
    r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.block == BASICTEST_BLOCK_CASE);
    CHECK(text(r.e.name) == name);

    // a name the library got from a program: encoded, still one field
    std::string odd = "a b\tc";
    e.name = str(odd);
    CHECK(write(e) == "block\tcase\ta b\\x09c\n");
    r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(text(r.e.name) == odd);
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

TEST_CASE("CRC32: the standard CRC-32 of IEEE 802.3", "[basictest]")
{
    // the check value of the CRC catalogues, and zlib's crc32 of the others
    CHECK(crc("123456789") == 0xCBF43926u);
    CHECK(crc("") == 0x00000000u);
    CHECK(crc("a") == 0xE8B7BE43u);
    CHECK(crc("abc") == 0x352441C2u);
    CHECK(crc("12345678") == 0x9AE0DAAFu);
    CHECK(crc("The quick brown fox jumps over the lazy dog") == 0x414FA339u);
    std::string all;
    for (int c = 0; c < 256; c++)
        all.push_back(char(c));
    CHECK(crc(all) == 0x29058C73u);
    CHECK(crc(std::string("a\0b", 3)) == 0x15E87871u);
    // no bytes, no pointer
    CHECK(basictest_crc32(nullptr, 0) == 0u);
}

TEST_CASE("checkstr: the spec's short string check", "[basictest]")
{
    std::string exp = "123456789", act = "12345678";
    auto e = checkstr(5, 0, exp, act);
    CHECK(write(e) == "checkstr\t5\tfail\t123456789\t9\t$CBF43926\t12345678\t8\t$9AE0DAAF\t-\n");
    auto r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.expected_length == 9u);
    CHECK(r.e.expected_crc == 0xCBF43926u);
    CHECK(r.e.actual_length == 8u);
    CHECK(r.e.actual_crc == 0x9AE0DAAFu);
    CHECK(r.e.cut == 0);
}

TEST_CASE("checkstr: the spec's string with a tab, and every byte", "[basictest]")
{
    std::string exp = "ab", act = "a\tb";
    auto e = checkstr(12, 0, exp, act);
    CHECK(write(e) == "checkstr\t12\tfail\tab\t2\t$9E83486D\ta\\x09b\t3\t$C42AC338\t-\n");
    auto r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_CHECKSTR);
    CHECK(r.e.k == 12u);
    CHECK(text(r.e.expected_str) == exp);
    CHECK(text(r.e.actual_str) == act);
    CHECK(r.e.expected_length == 2u);
    CHECK(r.e.actual_length == 3u);
    CHECK(r.e.expected_crc == 0x9E83486Du);
    CHECK(r.e.actual_crc == 0xC42AC338u);
    CHECK(r.e.cut == 0);

    std::string all;
    for (int c = 0; c < 256; c++)
        all.push_back(char(c));
    std::string empty;
    e = checkstr(12, 1, all, empty);
    std::string l = write(e);
    CHECK(l.find("\\x00\\x01") != std::string::npos);
    CHECK(l.find(" !\"#") != std::string::npos);
    CHECK(l.find("[\\x5C]") != std::string::npos);
    CHECK(l.find("}~\\x7F\\x80") != std::string::npos);
    CHECK(l.find("\\xFF\t256\t$29058C73\t\t0\t$00000000\t-\n") != std::string::npos);
    r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(text(r.e.expected_str) == all);
    CHECK(r.e.actual_str.length == 0u);
    CHECK(r.e.expected_length == 256u);
}

TEST_CASE("checkstr: cut values keep the length and CRC32 of the whole", "[basictest]")
{
    std::string whole(400, 'x'), other(400, 'x'), ab = "ab";
    other[399] = 'y';
    auto e = checkstr(9, 0, whole, other);
    // the start of each
    e.expected_str.length = 100;
    e.actual_str.length = 100;
    e.cut = 1;
    std::string x100(100, 'x');
    CHECK(write(e) == "checkstr\t9\tfail\t" + x100 + "\t400\t" + hex(crc(whole)) + "\t" + x100 + "\t400\t"
                          + hex(crc(other)) + "\tcut\n");
    auto r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.cut == 1);
    CHECK(r.e.expected_str.length == 100u);
    CHECK(r.e.expected_length == 400u);
    CHECK(r.e.expected_crc == crc(whole));
    CHECK(r.e.actual_crc == crc(other));
    CHECK(r.e.expected_crc != r.e.actual_crc);

    // one value cut, the other complete: the complete one has its CRC32
    e = checkstr(9, 0, whole, ab);
    e.expected_str.length = 10;
    e.cut = 1;
    r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.cut == 1);
    CHECK(text(r.e.actual_str) == "ab");
    CHECK(r.e.actual_crc == 0x9E83486Du);

    // the bytes of a cut value cannot be checked against its CRC32: the
    // judge decides == by length and CRC32
    e = checkstr(9, 0, whole, ab);
    std::string changed = "zz";
    e.expected_str = str(changed);
    e.cut = 1;
    CHECK(roundtrip(e).rc == BASICTEST_OK);
}

TEST_CASE("reader: a length or CRC32 that does not fit its value", "[basictest]")
{
    const struct {
        const char *line;
        unsigned field;
    } cases[] = {
        // a complete value with another CRC32 (the spec's scenario)
        { "checkstr\t1\tok\tab\t2\t$9E83486C\tb\t1\t$71BEEFF9\t-", 6 },
        { "checkstr\t1\tok\tab\t2\t$9E83486D\tb\t1\t$00000000\t-", 9 },
        { "checkstr\t1\tok\t\t0\t$00000001\tb\t1\t$71BEEFF9\t-", 6 },
        // the CRC32 of a complete value is checked with CUT `cut` as well
        { "checkstr\t1\tok\tab\t2\t$00000000\tb\t2\t$71BEEFF9\tcut", 6 },
        // a value longer than its length
        { "checkstr\t1\tok\tab\t1\t$9E83486D\tb\t1\t$71BEEFF9\t-", 5 },
        { "checkstr\t1\tok\tab\t2\t$9E83486D\tb\t0\t$71BEEFF9\t-", 8 },
        // `-` with a cut value, `cut` with none
        { "checkstr\t1\tok\ta\t2\t$9E83486D\tb\t1\t$71BEEFF9\t-", 10 },
        { "checkstr\t1\tok\tab\t2\t$9E83486D\t\t1\t$71BEEFF9\t-", 10 },
        { "checkstr\t1\tok\tab\t2\t$9E83486D\tb\t1\t$71BEEFF9\tcut", 10 },
    };
    for (const auto &c : cases) {
        auto r = read(c.line, 30);
        CAPTURE(c.line);
        CHECK(r.rc == BASICTEST_E_VALUE);
        CHECK(r.err.line == 30u);
        CHECK(r.err.field == c.field);
        CHECK(r.message == "line 30: checkstr: malformed value in field " + std::to_string(c.field));
    }
    // the same lines made right read
    CHECK(read("checkstr\t1\tok\tab\t2\t$9E83486D\tb\t1\t$71BEEFF9\t-").rc == BASICTEST_OK);
    CHECK(read("checkstr\t1\tok\ta\t2\t$9E83486D\tb\t1\t$71BEEFF9\tcut").rc == BASICTEST_OK);
    CHECK(read("checkstr\t1\tok\t\t2\t$9E83486D\t\t0\t$00000000\tcut").rc == BASICTEST_OK);
}

TEST_CASE("checkq: the bits of two quicks", "[basictest]")
{
    auto e = entry(BASICTEST_CHECKQ);
    e.k = 7;
    e.ok = 0;
    e.expected = 0x00018000u;                   // 1.5
    e.actual = 0x00010000u;                     // 1.0
    CHECK(write(e) == "checkq\t7\tfail\t$00018000\t$00010000\n");
    auto r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_CHECKQ);
    CHECK(r.e.k == 7u);
    CHECK(r.e.ok == 0);
    CHECK(r.e.expected == 0x00018000u);
    CHECK(r.e.actual == 0x00010000u);
}

TEST_CASE("checkflt: the FFP bits of two floats and the delta", "[basictest]")
{
    auto e = entry(BASICTEST_CHECKFLT);
    e.k = 4294967295u;
    e.ok = 1;
    e.expected = 0xA0000042u;                   // 2.5
    e.actual = 0xA0000042u;
    e.delta = 0xA3D70A3Au;                      // 0.01
    CHECK(write(e) == "checkflt\t4294967295\tok\t$A0000042\t$A0000042\t$A3D70A3A\n");
    auto r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_CHECKFLT);
    CHECK(r.e.k == 4294967295u);
    CHECK(r.e.ok == 1);
    CHECK(r.e.expected == 0xA0000042u);
    CHECK(r.e.actual == 0xA0000042u);
    CHECK(r.e.delta == 0xA3D70A3Au);
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

// the entries of a started program (change basic-test-local-data): run K,
// runend K with the return code or - where the system gives none
TEST_CASE("run, runend", "[basictest]")
{
    auto e = entry(BASICTEST_RUN);
    e.k = 4;
    CHECK(write(e) == "run\t4\n");
    auto r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_RUN);
    CHECK(r.e.k == 4u);

    e = entry(BASICTEST_RUNEND);
    e.k = 4;
    e.ok = 1;
    e.actual = 20;
    CHECK(write(e) == "runend\t4\t$00000014\n");
    r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.kind == BASICTEST_RUNEND);
    CHECK(r.e.k == 4u);
    CHECK(r.e.ok == 1);
    CHECK(r.e.actual == 20u);

    e.ok = 0;
    e.actual = 0;
    CHECK(write(e) == "runend\t4\t-\n");
    r = roundtrip(e);
    REQUIRE(r.rc == BASICTEST_OK);
    CHECK(r.e.ok == 0);
    CHECK(r.e.actual == 0u);

    e.ok = 1;
    e.actual = 0xFFFFFFFFu;
    CHECK(write(e) == "runend\t4\t$FFFFFFFF\n");
}

TEST_CASE("writer: a buffer too small, an unknown kind, a case without name", "[basictest]")
{
    std::string exp = "x\ty", act = "\\";
    auto e = checkstr(123, 0, exp, act);
    std::string want = "checkstr\t123\tfail\tx\\x09y\t3\t$5DB8922B\t\\x5C\t1\t$B0DFF252\t-\n";
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
    e = entry(BASICTEST_RUNEND + 1);
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
        // a case without a name, init and final with one, a case with two
        "block\tcase", "block\tinit\t", "block\tfinal\ta", "block\tcase\ta\tb",
        "check\t1\tok\t$00000000",     "check\t1\tok\t$00000000\t$00000000\t$00000000",
        "checkstr\t1\tok\ta",          "checkstr\t1\tok\ta\tb",
        "checkstr\t1\tok\ta\tb\t-",     "checkstr\t1\tok\ta\tb\t-\tc",
        // one field short, one too many
        "checkstr\t1\tok\ta\t1\t$E8B7BE43\tb\t1\t$71BEEFF9",
        "checkstr\t1\tok\ta\t1\t$E8B7BE43\tb\t1\t$71BEEFF9\t-\t-",
        "checkq\t1\tok\t$00000000",    "checkq\t1\tok\t$00000000\t$00000000\t$00000000",
        "checkflt\t1\tok\t$00000000\t$00000000",
        "checkflt\t1\tok\t$00000000\t$00000000\t$00000000\t$00000000",
        "require\t1",  "require\t1\tbroken\tx",  "expect-error", "expect-error\t1\t2",
        "blockend\tx", "broken",       "broken\ta\tb", "incomplete\t", "end\t",
        "check\t1\tok\t$00000000\t$00000000\t\t\t\t",
        "run", "run\t1\t2", "runend\t1", "runend\t1\t$00000000\t-",
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
        { "checkstr\t1\tok\ta\\x9\t1\t$E8B7BE43\tb\t1\t$71BEEFF9\t-", 4 },
        { "checkstr\t1\tok\ta\t1\t$E8B7BE43\tb\\\t1\t$71BEEFF9\t-", 7 },
        { "checkstr\t1\tok\ta\\t\t1\t$E8B7BE43\tb\t1\t$71BEEFF9\t-", 4 },
        { "checkstr\t1\tok\ta\\x0d\t1\t$E8B7BE43\tb\t1\t$71BEEFF9\t-", 4 },
        { "checkstr\t1\tok\ta\\x41\t1\t$E8B7BE43\tb\t1\t$71BEEFF9\t-", 4 },      // A stands as it is
        { "checkstr\t1\tok\ta\t1\t$E8B7BE43\tb\\x20\t1\t$71BEEFF9\t-", 7 },      // so does the blank
        { "checkstr\t1\tok\ta\t1\t$E8B7BE43\tb\r\t1\t$71BEEFF9\t-", 7 },
        { "checkstr\t1\tok\ta\x7F\t1\t$E8B7BE43\tb\t1\t$71BEEFF9\t-", 4 },
        { "checkstr\t1\tok\ta\xC4\t1\t$E8B7BE43\tb\t1\t$71BEEFF9\t-", 4 },
        { "checkstr\t1\tok\ta\t1\t$E8B7BE43\tb\t1\t$71BEEFF9\t", 10 },
        { "checkstr\t1\tok\ta\t1\t$E8B7BE43\tb\t1\t$71BEEFF9\tCUT", 10 },
        { "checkstr\t1\tok\ta\t1\t$E8B7BE43\tb\t1\t$71BEEFF9\t--", 10 },
        // a length or CRC32 in another form
        { "checkstr\t1\tok\ta\t01\t$E8B7BE43\tb\t1\t$71BEEFF9\t-", 5 },
        { "checkstr\t1\tok\ta\t+1\t$E8B7BE43\tb\t1\t$71BEEFF9\t-", 5 },
        { "checkstr\t1\tok\ta\t\t$E8B7BE43\tb\t1\t$71BEEFF9\t-", 5 },
        { "checkstr\t1\tok\ta\t4294967296\t$E8B7BE43\tb\t1\t$71BEEFF9\t-", 5 },
        { "checkstr\t1\tok\ta\t1\t$e8b7be43\tb\t1\t$71BEEFF9\t-", 6 },
        { "checkstr\t1\tok\ta\t1\tE8B7BE43\tb\t1\t$71BEEFF9\t-", 6 },
        { "checkstr\t1\tok\ta\t1\t$E8B7BE4\tb\t1\t$71BEEFF9\t-", 6 },
        { "checkstr\t1\tok\ta\t1\t$E8B7BE43\tb\t1 \t$71BEEFF9\t-", 8 },
        { "checkstr\t1\tok\ta\t1\t$E8B7BE43\tb\t1\t$71BEEFF9 \t-", 9 },
        { "checkq\t1\tok\t$0001800\t$00018000", 4 },
        { "checkq\t1\tok\t$00018000\t$0001800g", 5 },
        { "checkq\t1\tmaybe\t$00018000\t$00018000", 3 },
        { "checkflt\t1\tok\t$A0000042\t$A0000042\t$a0000042", 6 },
        { "checkflt\t1\tok\t$A0000042\t$A000004\t$A0000042", 5 },
        { "checkflt\t01\tok\t$A0000042\t$A0000042\t$A0000042", 2 },
        { "require\t1\tok", 3 },
        { "require\tx\tbroken", 2 },
        { "expect-error\t1x", 2 },
        { "broken\tbad\\", 2 },
        { "block\tcase\t", 3 },                  // an empty name
        { "block\tcase a", 2 },                  // name and word in one field
        { "block\tcase ", 2 },
        { "block\tcase_a", 2 },
        { "block\tInit", 2 },
        { "block\tCase\ta", 2 },
        { "block\t\ta", 2 },
        { "block\tcase\ta\\x5c", 3 },
        { "block\tcase\ta b\x01", 3 },
        { "begin\t", 2 },
        { "begin\t01", 2 },
        { "begin\tone", 2 },
        // the spec's scenario: a runend with a malformed code
        { "runend\t4\t20", 3 },
        { "runend\t4\t$0000014", 3 },
        { "runend\t4\t$0000001e", 3 },
        { "runend\t4\t--", 3 },
        { "runend\t4\t", 3 },
        { "runend\t04\t$00000014", 2 },
        { "run\tx", 2 },
        { "run\t", 2 },
    };
    for (const auto &c : cases) {
        auto r = read(c.line, 21);
        CAPTURE(c.line);
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
