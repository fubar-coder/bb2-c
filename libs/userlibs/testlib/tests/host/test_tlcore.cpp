// test_tlcore - the logic of the Blitz test library (libs/userlibs/testlib/
// tlcore.c, spec testing/basic-testlib and testing/basic-test-interface)
// with the codec of shared/basictest: the entries of a run and their
// order, the verdicts of the checks, one write per entry, the entries
// kept while they cannot be written (Blitz mode, Forbid) and written at
// the next entry or at the end, the attribution of a procedure's check
// to the running block, preconditions in a case and in the batch init, a
// failed check in the batch init, the end of a run with a block open,
// calls out of order, a damaged state, the length and CRC32 of the whole
// strings of a string check (also of cut values), and the typed checks
// (quick, and float within a delta, here compared as doubles from the FFP
// bits).
// Every line written is read back with the codec's reader.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "basictest.h"
#include "tlcore.h"
}

namespace {

// what tl_os_write got, one string per call, whether it may write, and
// whether the floats can be compared (mathffp.library open)
std::vector<std::string> writes;
int can_write = 1;
int can_compare = 1;

// a Motorola FFP number: 24 bits of mantissa, the sign in bit 7, the
// exponent (excess 64) in bits 0-6; 0 is all zero bits
double ffp(uint32_t v)
{
    if (!v)
        return 0;
    double m = std::ldexp(double(v >> 8), -24);
    double r = std::ldexp(m, int(v & 0x7F) - 64);
    return v & 0x80 ? -r : r;
}

std::string all()
{
    std::string s;
    for (const auto &w : writes)
        s += w;
    return s;
}

// a fresh state (as the library's static one: zero) and a fresh file
struct run {
    tl_state st{};
    run()
    {
        writes.clear();
        can_write = 1;
        can_compare = 1;
    }
};

const uint8_t *u(const char *s)
{
    return reinterpret_cast<const uint8_t *>(s);
}

int block_case(tl_state &st, const char *name)
{
    return tl_block(&st, BASICTEST_BLOCK_CASE, u(name), std::strlen(name));
}

int block(tl_state &st, int b)
{
    return tl_block(&st, b, nullptr, 0);
}

void checkstr(tl_state &st, uint32_t k, const std::string &e, const std::string &a)
{
    tl_checkstr(&st, k, u(e.data()), e.size(), u(a.data()), a.size());
}

uint32_t crc(const std::string &s)
{
    return basictest_crc32(u(s.data()), s.size());
}

// one written line read back with the codec (the entry's strings point
// into line)
struct read_entry {
    std::string line;
    basictest_entry e{};
    std::string text_expected, text_actual;
};

read_entry entry_of(const std::string &written)
{
    read_entry r;
    REQUIRE(written.ends_with("\n"));
    r.line = written.substr(0, written.size() - 1);
    basictest_error err{};
    int rc = basictest_read(reinterpret_cast<uint8_t *>(r.line.data()), r.line.size(), 1, &r.e, &err);
    if (rc != BASICTEST_OK) {
        char msg[200];
        basictest_error_text(&err, msg, sizeof msg);
        FAIL(msg);
    }
    r.text_expected.assign(reinterpret_cast<const char *>(r.e.expected_str.data), r.e.expected_str.length);
    r.text_actual.assign(reinterpret_cast<const char *>(r.e.actual_str.data), r.e.actual_str.length);
    return r;
}

// every line of the file, read with the codec; the kind of each
std::vector<int> kinds(const std::string &file)
{
    std::vector<int> r;
    size_t at = 0;
    uint32_t number = 1;
    while (at < file.size()) {
        size_t lf = file.find('\n', at);
        REQUIRE(lf != std::string::npos);
        std::string line = file.substr(at, lf - at);
        basictest_entry e{};
        basictest_error err{};
        int rc = basictest_read(reinterpret_cast<uint8_t *>(line.data()), line.size(), number,
                                &e, &err);
        if (rc != BASICTEST_OK) {
            char msg[200];
            basictest_error_text(&err, msg, sizeof msg);
            FAIL(msg);
        }
        r.push_back(e.kind);
        at = lf + 1;
        number++;
    }
    return r;
}

} // namespace

extern "C" void tl_os_write(const uint8_t *data, size_t length)
{
    writes.emplace_back(reinterpret_cast<const char *>(data), length);
}

extern "C" int tl_os_can_write(void)
{
    return can_write;
}

extern "C" int tl_os_fltnear(uint32_t expected, uint32_t actual, uint32_t delta)
{
    if (!can_compare)
        return -1;
    return std::fabs(ffp(actual) - ffp(expected)) <= ffp(delta);
}

TEST_CASE("a run: init, two cases, final, the end", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    CHECK(block(r.st, BASICTEST_BLOCK_INIT) == 1);
    tl_check(&r.st, 0, 1, 1);
    tl_blockend(&r.st);
    CHECK(block_case(r.st, "a") == 1);
    tl_check(&r.st, 1, 16, 0);
    tl_check(&r.st, 2, 0xFFFFFFFF, 0xFFFFFFFF);
    tl_blockend(&r.st);
    CHECK(block_case(r.st, "b_2") == 1);
    checkstr(r.st, 3, "ab", "ab");
    tl_expect_error(&r.st, 4);
    tl_blockend(&r.st);
    CHECK(block(r.st, BASICTEST_BLOCK_FINAL) == 1);
    tl_blockend(&r.st);
    tl_finit(&r.st);
    CHECK(all() == "begin\t1\n"
                   "block\tinit\n"
                   "check\t0\tok\t$00000001\t$00000001\n"
                   "blockend\n"
                   "block\tcase\ta\n"
                   "check\t1\tfail\t$00000010\t$00000000\n"
                   "check\t2\tok\t$FFFFFFFF\t$FFFFFFFF\n"
                   "blockend\n"
                   "block\tcase\tb_2\n"
                   "checkstr\t3\tok\tab\t2\t$9E83486D\tab\t2\t$9E83486D\t-\n"
                   "expect-error\t4\n"
                   "blockend\n"
                   "block\tfinal\n"
                   "blockend\n"
                   "end\n");
    kinds(all());
}

TEST_CASE("one write per entry in Amiga mode", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "a");
    tl_check(&r.st, 0, 1, 2);
    tl_blockend(&r.st);
    tl_finit(&r.st);
    REQUIRE(writes.size() == 5);
    for (const auto &w : writes) {
        CHECK(w.find('\n') == w.size() - 1);
    }
}

TEST_CASE("the verdicts of the checks", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "v");
    tl_check(&r.st, 0, 0xFFFFFFFF, 1);              // -1 against 1
    tl_check(&r.st, 1, 0x80000000, 0x80000000);
    checkstr(r.st, 2, "abc", "ab");                 // a prefix
    checkstr(r.st, 3, "", "");
    checkstr(r.st, 4, std::string("a\0b", 3), std::string("a\0c", 3));  // behind a 0 byte
    checkstr(r.st, 5, std::string("a\0b", 3), std::string("a\0b", 3));
    checkstr(r.st, 6, "x\ty\\", "x\ty\\");
    CHECK(writes.at(2) == "check\t0\tfail\t$FFFFFFFF\t$00000001\n");
    CHECK(writes.at(3) == "check\t1\tok\t$80000000\t$80000000\n");
    CHECK(writes.at(4) == "checkstr\t2\tfail\tabc\t3\t$352441C2\tab\t2\t$9E83486D\t-\n");
    CHECK(writes.at(5) == "checkstr\t3\tok\t\t0\t$00000000\t\t0\t$00000000\t-\n");
    CHECK(writes.at(6) == "checkstr\t4\tfail\ta\\x00b\t3\t$15E87871\ta\\x00c\t3\t$62EF48E7\t-\n");
    CHECK(writes.at(7) == "checkstr\t5\tok\ta\\x00b\t3\t$15E87871\ta\\x00b\t3\t$15E87871\t-\n");
    CHECK(writes.at(8) == "checkstr\t6\tok\tx\\x09y\\x5C\t4\t$1C3EB380\tx\\x09y\\x5C\t4\t$1C3EB380\t-\n");
    kinds(all());
}

TEST_CASE("a string check too long for a line keeps its verdict", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "long");
    std::string a(3000, '\x01'), b = a;
    checkstr(r.st, 7, a, b);
    b.back() = '\x02';
    checkstr(r.st, 8, a, b);
    REQUIRE(writes.size() == 4);
    CHECK(writes[2].rfind("checkstr\t7\tok\t\\x01", 0) == 0);
    CHECK(writes[3].rfind("checkstr\t8\tfail\t\\x01", 0) == 0);
    CHECK(writes[2].size() <= TL_LINE);
    CHECK(writes[3].size() <= TL_LINE);
    CHECK(writes[2].ends_with("\tcut\n"));
    CHECK(writes[3].ends_with("\tcut\n"));
    kinds(all());
    // the lengths and CRC32s of the whole strings: equal values, equal CRC32s
    auto e7 = entry_of(writes[2]), e8 = entry_of(writes[3]);
    CHECK(e7.e.expected_length == 3000u);
    CHECK(e7.e.actual_length == 3000u);
    CHECK(e7.e.expected_crc == crc(a));
    CHECK(e7.e.actual_crc == crc(a));
    CHECK(e8.e.actual_crc == crc(b));
    CHECK(e8.e.expected_crc != e8.e.actual_crc);
}

TEST_CASE("string checks: the length and CRC32 of the whole strings", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "crc");
    // the spec's short string check: nothing cut
    checkstr(r.st, 5, "123456789", "12345678");
    CHECK(writes.at(2) == "checkstr\t5\tfail\t123456789\t9\t$CBF43926\t12345678\t8\t$9AE0DAAF\t-\n");

    // a pair of 400 bytes that differ in the last byte: both cut, each with
    // its whole length and CRC32, the start of each as its value
    std::string x(400, 'x'), y = x;
    y.back() = 'y';
    checkstr(r.st, 6, x, y);
    auto c = entry_of(writes.at(3));
    CHECK(c.e.ok == 0);
    CHECK(c.e.cut == 1);
    CHECK(writes.at(3).size() <= TL_LINE);
    CHECK(c.e.expected_length == 400u);
    CHECK(c.e.actual_length == 400u);
    CHECK(c.e.expected_crc == crc(x));
    CHECK(c.e.actual_crc == crc(y));
    CHECK(c.e.expected_crc != c.e.actual_crc);
    CHECK(c.e.expected_str.length < 400u);
    CHECK(c.e.actual_str.length < 400u);
    CHECK(c.text_expected == x.substr(0, c.e.expected_str.length));
    CHECK(c.text_actual == y.substr(0, c.e.actual_str.length));

    // equal values of 400 bytes: ok, equal CRC32s
    checkstr(r.st, 7, x, x);
    auto d = entry_of(writes.at(4));
    CHECK(d.e.ok == 1);
    CHECK(d.e.cut == 1);
    CHECK(d.e.expected_crc == d.e.actual_crc);
    CHECK(d.e.expected_crc == crc(x));

    // a long value and a short one: only the long one is cut, the short
    // one stays whole with its CRC32
    checkstr(r.st, 8, std::string(600, 'a'), "ab");
    auto s = entry_of(writes.at(5));
    CHECK(s.e.cut == 1);
    CHECK(s.e.expected_str.length < 600u);
    CHECK(s.e.expected_length == 600u);
    CHECK(s.text_actual == "ab");
    CHECK(s.e.actual_length == 2u);
    CHECK(s.e.actual_crc == 0x9E83486Du);
    kinds(all());
}

TEST_CASE("entries are kept while they cannot be written", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "blitz");
    can_write = 0;                                  // BLITZ
    tl_check(&r.st, 0, 1, 1);
    tl_check(&r.st, 1, 2, 3);
    CHECK(writes.size() == 2);
    can_write = 1;                                  // QAMIGA
    tl_check(&r.st, 2, 4, 4);
    CHECK(writes.size() == 4);                      // the kept ones, then the entry
    CHECK(writes[2] == "check\t0\tok\t$00000001\t$00000001\n"
                       "check\t1\tfail\t$00000002\t$00000003\n");
    CHECK(writes[3] == "check\t2\tok\t$00000004\t$00000004\n");
    tl_blockend(&r.st);
    tl_finit(&r.st);
    kinds(all());
}

TEST_CASE("the end writes the kept entries", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "blitz");
    can_write = 0;
    tl_check(&r.st, 0, 1, 1);
    tl_blockend(&r.st);
    tl_finit(&r.st);                                // the program ends in Blitz mode
    CHECK(all() == "begin\t1\n"
                   "block\tcase\tblitz\n"
                   "check\t0\tok\t$00000001\t$00000001\n"
                   "blockend\n"
                   "end\n");
}

TEST_CASE("kept entries that do not fit are lost and reported", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "full");
    can_write = 0;
    for (uint32_t i = 0; i < 100; i++)
        tl_check(&r.st, i, i, i);
    can_write = 1;
    tl_blockend(&r.st);
    std::string f = all();
    CHECK(f.find("check\t0\tok") != std::string::npos);
    CHECK(f.find("check\t99\tok") == std::string::npos);
    size_t lost = f.find("broken\tentries lost while kept\n");
    REQUIRE(lost != std::string::npos);
    CHECK(f.substr(lost) == "broken\tentries lost while kept\nblockend\n");
    kinds(f);
}

TEST_CASE("a check of a procedure belongs to the running block", "[tlcore]")
{
    // the procedure's check has one number K (the runner's table names
    // its namespace); the entry stands in the block that called it
    run r;
    auto procedure = [&] { tl_check(&r.st, 9, 5, 5); };
    tl_begin(&r.st);
    block_case(r.st, "a");
    procedure();
    tl_blockend(&r.st);
    block_case(r.st, "b");
    procedure();
    procedure();
    tl_blockend(&r.st);
    tl_finit(&r.st);
    CHECK(all() == "begin\t1\n"
                   "block\tcase\ta\n"
                   "check\t9\tok\t$00000005\t$00000005\n"
                   "blockend\n"
                   "block\tcase\tb\n"
                   "check\t9\tok\t$00000005\t$00000005\n"
                   "check\t9\tok\t$00000005\t$00000005\n"
                   "blockend\n"
                   "end\n");
}

TEST_CASE("a failed precondition leaves its case", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "a");
    CHECK(tl_require(&r.st, 0, 1) == 0);            // holds: nothing written
    CHECK(tl_require(&r.st, 1, 0) == 1);            // the case returns
    CHECK(block_case(r.st, "b") == 1);              // the next case runs
    tl_blockend(&r.st);
    tl_finit(&r.st);
    CHECK(all() == "begin\t1\n"
                   "block\tcase\ta\n"
                   "require\t1\tbroken\n"
                   "blockend\n"
                   "block\tcase\tb\n"
                   "blockend\n"
                   "end\n");
}

TEST_CASE("a failed precondition of the batch init breaks every case", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block(r.st, BASICTEST_BLOCK_INIT);
    CHECK(tl_require(&r.st, 0, 0) == 1);
    CHECK(block_case(r.st, "a") == 0);              // does not run
    CHECK(block_case(r.st, "b") == 0);
    CHECK(block(r.st, BASICTEST_BLOCK_FINAL) == 0); // nor the batch cleanup
    tl_finit(&r.st);
    CHECK(all() == "begin\t1\n"
                   "block\tinit\n"
                   "require\t0\tbroken\n"
                   "blockend\n"
                   "block\tcase\ta\n"
                   "require\t0\tbroken\n"
                   "blockend\n"
                   "block\tcase\tb\n"
                   "require\t0\tbroken\n"
                   "blockend\n"
                   "end\n");
}

TEST_CASE("a failed check of the batch init fails every case", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block(r.st, BASICTEST_BLOCK_INIT);
    tl_check(&r.st, 0, 1, 1);
    checkstr(r.st, 1, "x", "y");                    // the first failure
    tl_check(&r.st, 2, 3, 4);
    tl_blockend(&r.st);
    CHECK(block_case(r.st, "a") == 1);              // runs
    tl_check(&r.st, 3, 5, 5);
    tl_blockend(&r.st);
    CHECK(block_case(r.st, "b") == 1);
    tl_blockend(&r.st);
    CHECK(block(r.st, BASICTEST_BLOCK_FINAL) == 1);
    tl_blockend(&r.st);
    tl_finit(&r.st);
    CHECK(all() == "begin\t1\n"
                   "block\tinit\n"
                   "check\t0\tok\t$00000001\t$00000001\n"
                   "checkstr\t1\tfail\tx\t1\t$8CDC1683\ty\t1\t$FBDB2615\t-\n"
                   "check\t2\tfail\t$00000003\t$00000004\n"
                   "blockend\n"
                   "block\tcase\ta\n"
                   "checkstr\t1\tfail\tx\t1\t$8CDC1683\ty\t1\t$FBDB2615\t-\n"
                   "check\t3\tok\t$00000005\t$00000005\n"
                   "blockend\n"
                   "block\tcase\tb\n"
                   "checkstr\t1\tfail\tx\t1\t$8CDC1683\ty\t1\t$FBDB2615\t-\n"
                   "blockend\n"
                   "block\tfinal\n"
                   "blockend\n"
                   "end\n");
}

TEST_CASE("the end inside a block: incomplete, end", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "a");
    tl_check(&r.st, 0, 1, 1);
    tl_expect_error(&r.st, 1);
    tl_finit(&r.st);                                // End or a runtime error
    CHECK(all() == "begin\t1\n"
                   "block\tcase\ta\n"
                   "check\t0\tok\t$00000001\t$00000001\n"
                   "expect-error\t1\n"
                   "incomplete\n"
                   "end\n");
    writes.clear();
    tl_finit(&r.st);                                // once only
    CHECK(writes.empty());
}

TEST_CASE("without tl_begin nothing is written and no block runs", "[tlcore]")
{
    run r;
    CHECK(block_case(r.st, "a") == 0);
    tl_check(&r.st, 0, 1, 1);
    CHECK(tl_require(&r.st, 1, 0) == 0);
    tl_finit(&r.st);
    CHECK(writes.empty());
}

TEST_CASE("calls out of order are reported", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    tl_check(&r.st, 0, 1, 1);                       // outside a block
    tl_blockend(&r.st);                             // without a block
    block_case(r.st, "a");
    block_case(r.st, "b");                          // inside a block
    tl_blockend(&r.st);
    block(r.st, BASICTEST_BLOCK_INIT);              // after a case
    tl_blockend(&r.st);
    block(r.st, BASICTEST_BLOCK_FINAL);
    tl_blockend(&r.st);
    block_case(r.st, "c");                          // after the batch cleanup
    tl_blockend(&r.st);
    block(r.st, BASICTEST_BLOCK_FINAL);             // twice
    tl_blockend(&r.st);
    CHECK(tl_require(&r.st, 1, 0) == 0);            // outside a block: no return
    tl_begin(&r.st);                                // twice
    block_case(r.st, "");                           // no name, after the batch cleanup
    tl_blockend(&r.st);
    tl_finit(&r.st);
    CHECK(all() == "begin\t1\n"
                   "broken\tcheck outside a block\n"
                   "check\t0\tok\t$00000001\t$00000001\n"
                   "broken\tblock end without a block\n"
                   "blockend\n"
                   "block\tcase\ta\n"
                   "broken\tblock inside a block\n"
                   "block\tcase\tb\n"
                   "blockend\n"
                   "broken\tbatch init after a block\n"
                   "block\tinit\n"
                   "blockend\n"
                   "block\tfinal\n"
                   "blockend\n"
                   "broken\tcase after the batch cleanup\n"
                   "block\tcase\tc\n"
                   "blockend\n"
                   "broken\tsecond batch cleanup\n"
                   "block\tfinal\n"
                   "blockend\n"
                   "broken\tcheck outside a block\n"
                   "require\t1\tbroken\n"
                   "broken\tbegin inside the run\n"
                   "broken\tcase after the batch cleanup\n"
                   "block\tcase\t?\n"
                   "broken\tcase without a name\n"
                   "blockend\n"
                   "end\n");
    kinds(all());
}

TEST_CASE("a damaged state is reported once", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "a");
    r.st.guard_hi ^= 1;                             // a wild write
    tl_check(&r.st, 0, 1, 1);
    tl_check(&r.st, 1, 1, 1);
    CHECK(block_case(r.st, "b") == 0);
    tl_finit(&r.st);
    CHECK(all() == "begin\t1\n"
                   "block\tcase\ta\n"
                   "broken\tstate damaged\n"
                   "end\n");
    kinds(all());
}

TEST_CASE("a damaged state at the end", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "a");
    r.st.guard_lo = 0;
    tl_finit(&r.st);
    CHECK(all() == "begin\t1\n"
                   "block\tcase\ta\n"
                   "broken\tstate damaged\n"
                   "end\n");
}

TEST_CASE("a damaged state found in Blitz mode is reported at the end", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "a");
    can_write = 0;
    r.st.guard_lo = 0;
    tl_check(&r.st, 0, 1, 1);
    CHECK(writes.size() == 2);
    tl_finit(&r.st);
    CHECK(all() == "begin\t1\n"
                   "block\tcase\ta\n"
                   "broken\tstate damaged\n"
                   "end\n");
}

TEST_CASE("the FFP numbers of the tests", "[tlcore]")
{
    CHECK(ffp(0xA0000042u) == 2.5);
    CHECK(ffp(0xC0000045u) == 24.0);
    CHECK(ffp(0x80000041u) == 1.0);
    CHECK(ffp(0x800000C1u) == -1.0);
    CHECK(std::fabs(ffp(0xA3D70A3Au) - 0.01) < 1e-8);
}

TEST_CASE("quick checks: the bits equal", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "q");
    tl_checkq(&r.st, 0, 0x00018000u, 0x00018000u);  // 1.5, 1.5
    tl_checkq(&r.st, 1, 0x00018000u, 0x00010000u);  // 1.5, 1.0
    tl_checkq(&r.st, 2, 0xFFFF8000u, 0xFFFF8000u);  // -0.5
    CHECK(writes.at(2) == "checkq\t0\tok\t$00018000\t$00018000\n");
    CHECK(writes.at(3) == "checkq\t1\tfail\t$00018000\t$00010000\n");
    CHECK(writes.at(4) == "checkq\t2\tok\t$FFFF8000\t$FFFF8000\n");
    kinds(all());
}

TEST_CASE("float checks: within and outside the delta", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "f");
    const uint32_t f25 = 0xA0000042u, f24 = 0xC0000045u, d001 = 0xA3D70A3Au, f1 = 0x80000041u;
    tl_checkflt(&r.st, 0, f25, f25, d001);          // equal
    tl_checkflt(&r.st, 1, f24, f25, d001);          // 24 against 2.5
    tl_checkflt(&r.st, 2, f25, 0x00000000u, f25);   // |0-2.5| = 2.5 <= 2.5
    tl_checkflt(&r.st, 3, f1, 0x800000C1u, f1);     // |-1-1| = 2 > 1
    tl_checkflt(&r.st, 4, f1, f1, 0);               // delta 0, equal
    CHECK(writes.at(2) == "checkflt\t0\tok\t$A0000042\t$A0000042\t$A3D70A3A\n");
    CHECK(writes.at(3) == "checkflt\t1\tfail\t$C0000045\t$A0000042\t$A3D70A3A\n");
    CHECK(writes.at(4) == "checkflt\t2\tok\t$A0000042\t$00000000\t$A0000042\n");
    CHECK(writes.at(5) == "checkflt\t3\tfail\t$80000041\t$800000C1\t$80000041\n");
    CHECK(writes.at(6) == "checkflt\t4\tok\t$80000041\t$80000041\t$00000000\n");
    kinds(all());
}

TEST_CASE("float checks without mathffp.library fail as broken", "[tlcore]")
{
    run r;
    tl_begin(&r.st);
    block_case(r.st, "f");
    can_compare = 0;
    tl_checkflt(&r.st, 0, 0xA0000042u, 0xA0000042u, 0xA3D70A3Au);
    CHECK(writes.at(2) == "broken\tfloat compare not available\n");
    CHECK(writes.at(3) == "checkflt\t0\tfail\t$A0000042\t$A0000042\t$A3D70A3A\n");
}

TEST_CASE("typed checks: no compare before the start, init failures repeated", "[tlcore]")
{
    run r;
    can_compare = 0;
    tl_checkflt(&r.st, 0, 1, 1, 1);                 // before tl_begin: nothing
    CHECK(writes.empty());
    can_compare = 1;
    tl_begin(&r.st);
    block(r.st, BASICTEST_BLOCK_INIT);
    tl_checkq(&r.st, 1, 0x00018000u, 0x00010000u);
    tl_checkflt(&r.st, 2, 0xA0000042u, 0xC0000045u, 0xA3D70A3Au);
    tl_blockend(&r.st);
    block_case(r.st, "a");
    tl_blockend(&r.st);
    tl_finit(&r.st);
    CHECK(all() == "begin\t1\n"
                   "block\tinit\n"
                   "checkq\t1\tfail\t$00018000\t$00010000\n"
                   "checkflt\t2\tfail\t$A0000042\t$C0000045\t$A3D70A3A\n"
                   "blockend\n"
                   "block\tcase\ta\n"
                   "checkq\t1\tfail\t$00018000\t$00010000\n"
                   "blockend\n"
                   "end\n");
}
