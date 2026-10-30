// test_xtra_blitz2 - the Blitz 2 .xtra of shared/xtra (xtra.h) with files
// built in memory: the defaults, both versions read field by field, the
// byte-exact round trip (garbage after the 0 bytes and in the resident
// pointers included), the writer, and broken files (each refused with its
// code and offset).
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "xtra.h"
}

namespace {

using bytes = std::vector<uint8_t>;

struct xtra x;  // 11 KB: not on the stack

void put16(bytes &b, size_t at, unsigned v)
{
    b[at] = uint8_t(v >> 8);
    b[at + 1] = uint8_t(v);
}

void put32(bytes &b, size_t at, uint32_t v)
{
    put16(b, at, v >> 16);
    put16(b, at + 2, v & 0xFFFF);
}

void putstr(bytes &b, size_t at, const char *s)
{
    std::memcpy(&b[at], s, std::strlen(s) + 1);
}

// a file: vers, the fixed part with recognisable values, n pairs
bytes file(unsigned vers, unsigned npairs)
{
    bytes b(size_t(vers ? XTRA_FIXED_V1 : XTRA_FIXED_V0) + 4 * npairs, 0);

    b[0] = uint8_t(vers);
    b[1] = 0x80;
    for (unsigned i = 0; i < XTRA_NSWITCH; i++)
        put16(b, 2 + 2 * i, (i & 1) ? 0x80 : 0);
    for (unsigned i = 0; i < XTRA_NBUFFER; i++)
        put32(b, 30 + 4 * i, 1000 + i);
    putstr(b, 54, "blitzlibs:amigalibs.res");
    put32(b, 54 + 64, 0x0012ABCD);          // garbage pointer
    putstr(b, 54 + 68, "blitzlibs:libmacs.res");
    putstr(b, 598, "ram:");
    putstr(b, 790, "ram:test");
    b[790 + 100] = 0x5A;                    // garbage behind the 0
    if (vers)
        putstr(b, 982, "setpa c:");
    for (unsigned i = 0; i < npairs; i++) {
        size_t at = b.size() - 4 * (npairs - i);
        put16(b, at, 4 + i * 7);
        put16(b, at + 2, 10 + i);
    }
    return b;
}

int read(const bytes &b, struct xtra_error *err = nullptr)
{
    return xtra_read_blitz2(&x, b.data(), b.size(), err);
}

bytes write()
{
    bytes out(xtra_blitz2_size(&x));
    size_t len = 99;
    unsigned skipped = 99;

    REQUIRE(xtra_write_blitz2(&x, out.data(), out.size(), &len, &skipped, nullptr) == XTRA_OK);
    REQUIRE(len == out.size());
    return out;
}

// refused with code at offset
void refused(const bytes &b, int code, size_t offset)
{
    struct xtra_error err;

    CHECK(read(b, &err) == code);
    CHECK(err.code == code);
    CHECK(err.offset == offset);
}

} // namespace

TEST_CASE("xtra_init: the defaults of blitz2's options requester", "[xtra]")
{
    xtra_init(&x);
    CHECK(x.source == XTRA_SRC_NONE);
    CHECK(x.vers == 1);
    CHECK(x.icon == XTRA_ON);
    const uint16_t sw[XTRA_NSWITCH] = {0x80, 0x80, 0x80, 0x80, 0, 0, 0, 0x80, 0x80, 0, 0, 0x80, 0x80, 0x80};
    for (unsigned i = 0; i < XTRA_NSWITCH; i++)
        CHECK(x.sw[i] == sw[i]);
    const uint32_t buf[XTRA_NBUFFER] = {16384, 32768, 4096, 4096, 8192, 10240};
    for (unsigned i = 0; i < XTRA_NBUFFER; i++)
        CHECK(x.buf[i] == buf[i]);
    CHECK(x.nmax == 0);
    CHECK(x.editor.flags == 0);
    CHECK(x.ab3.stack == 32768);
    CHECK(x.ab3.seen == 0);
    CHECK(x.res[0].name[0] == 0);
    CHECK(x.exe_path[0] == 0);
}

TEST_CASE("vers 1: every field, the pairs in file order", "[xtra]")
{
    bytes b = file(1, 3);

    REQUIRE(read(b) == XTRA_OK);
    CHECK(x.source == XTRA_SRC_BLITZ2);
    CHECK(x.vers == 1);
    CHECK(x.icon == 0x80);
    for (unsigned i = 0; i < XTRA_NSWITCH; i++)
        CHECK(x.sw[i] == ((i & 1) ? 0x80 : 0));
    for (unsigned i = 0; i < XTRA_NBUFFER; i++)
        CHECK(x.buf[i] == 1000 + i);
    CHECK(std::string((char *)x.res[0].name) == "blitzlibs:amigalibs.res");
    CHECK(x.res[0].ptr == 0x0012ABCD);
    CHECK(std::string((char *)x.res[1].name) == "blitzlibs:libmacs.res");
    CHECK(x.res[1].ptr == 0);
    CHECK(x.res[2].name[0] == 0);
    CHECK(std::string((char *)x.exe_path) == "ram:");
    CHECK(std::string((char *)x.exe_name) == "ram:test");
    CHECK(x.exe_name[100] == 0x5A);
    CHECK(std::string((char *)x.cli) == "setpa c:");
    REQUIRE(x.nmax == 3);
    for (unsigned i = 0; i < 3; i++) {
        CHECK(x.max[i].lib == 4 + i * 7);
        CHECK(x.max[i].max == 10 + i);
        CHECK(x.max[i].flags == XTRA_MAX_LIB);
    }
    CHECK(x.editor.flags == 0);
    CHECK(write() == b);
}

TEST_CASE("vers 0: no CLI block; xtra_apply_vers0 as loadxtra", "[xtra]")
{
    bytes b = file(0, 2);

    REQUIRE(b.size() == 982 + 8);
    REQUIRE(read(b) == XTRA_OK);
    CHECK(x.vers == 0);
    CHECK(x.cli[0] == 0);
    CHECK(x.nmax == 2);
    CHECK(x.max[0].lib == 4);
    CHECK(write() == b);

    x.sw[XTRA_SW_AUTO_RUN] = 0;
    x.sw[XTRA_SW_ASSEMBLER_CHECK] = 0x80;
    x.sw[XTRA_SW_DEFAULT_INTERRUPT] = 0;
    x.sw[XTRA_SW_DEFAULT_OVERFLOW] = 0x80;
    xtra_apply_vers0(&x);
    CHECK(x.sw[XTRA_SW_AUTO_RUN] == 0x80);
    CHECK(x.sw[XTRA_SW_INTERRUPT_CHECK] == 0);
    CHECK(x.sw[XTRA_SW_ASSEMBLER_CHECK] == 0);
    CHECK(x.sw[XTRA_SW_OVERFLOW_CHECK] == 0x80);
}

TEST_CASE("round trip: odd values stay as they are", "[xtra]")
{
    bytes b = file(1, 0);

    b[1] = 0x7F;                    // icon byte other than $00/$80
    put16(b, 2, 0x1234);            // switch word other than $00/$80
    put32(b, 30, 0xFFFFFFFF);
    for (unsigned i = 0; i < XTRA_NRES; i++)
        put32(b, 54 + 68 * i + 64, 0xDEAD0000 + i);
    b[982 + 120] = 0x11;            // behind the CLI argument's 0
    REQUIRE(read(b) == XTRA_OK);
    CHECK(x.icon == 0x7F);
    CHECK(x.sw[0] == 0x1234);
    CHECK(x.res[7].ptr == 0xDEAD0007);
    CHECK(write() == b);
}

TEST_CASE("the most maxima: XTRA_MAXMAXIMA pairs", "[xtra]")
{
    bytes b = file(1, XTRA_MAXMAXIMA);

    REQUIRE(read(b) == XTRA_OK);
    CHECK(x.nmax == XTRA_MAXMAXIMA);
    CHECK(write() == b);
}

TEST_CASE("writer: maxima without a library number are skipped", "[xtra]")
{
    xtra_init(&x);
    REQUIRE(xtra_add_max(&x, 5, nullptr, 20) == XTRA_OK);
    REQUIRE(xtra_add_max(&x, -1, "Screen", 7) == XTRA_OK);
    REQUIRE(xtra_add_max(&x, 9, "Window", 8) == XTRA_OK);
    CHECK(x.max[1].flags == XTRA_MAX_NAME);
    CHECK(x.max[2].flags == (XTRA_MAX_LIB | XTRA_MAX_NAME));
    CHECK(xtra_blitz2_size(&x) == 1110 + 8);

    bytes out(2000, 0xEE);
    size_t len = 0;
    unsigned skipped = 0;
    REQUIRE(xtra_write_blitz2(&x, out.data(), out.size(), &len, &skipped, nullptr) == XTRA_OK);
    CHECK(len == 1118);
    CHECK(skipped == 1);
    CHECK(out[0] == 1);
    CHECK(out[1] == 0x80);
    CHECK(out[1110] == 0);
    CHECK(out[1111] == 5);
    CHECK(out[1113] == 20);
    CHECK(out[1115] == 9);
    CHECK(out[1117] == 8);
    CHECK(out[1118] == 0xEE);       // nothing behind
    out.resize(len);
    REQUIRE(read(out) == XTRA_OK);
    CHECK(x.nmax == 2);
}

TEST_CASE("writer: broken structure, too small buffer", "[xtra][broken]")
{
    struct xtra_error err;
    uint8_t out[1200];
    size_t len = 7;

    xtra_init(&x);
    CHECK(xtra_write_blitz2(&x, out, 1109, &len, nullptr, &err) == XTRA_E_SPACE);
    CHECK(err.code == XTRA_E_SPACE);
    CHECK(len == 0);
    x.vers = 2;
    CHECK(xtra_write_blitz2(&x, out, sizeof(out), &len, nullptr, &err) == XTRA_E_VERS);
    x.vers = 0;
    CHECK(xtra_write_blitz2(&x, out, 982, &len, nullptr, &err) == XTRA_OK);
    CHECK(len == 982);
}

TEST_CASE("broken: short files", "[xtra][broken]")
{
    refused(bytes{}, XTRA_E_SHORT, 0);
    refused(bytes(981, 0), XTRA_E_SHORT, 981);
    bytes b = file(1, 0);
    b.resize(1109);
    refused(b, XTRA_E_SHORT, 1109);
    b.resize(982);                  // a complete vers 0 size, but vers 1
    refused(b, XTRA_E_SHORT, 982);
}

TEST_CASE("broken: size not fixed part + 4n", "[xtra][broken]")
{
    for (size_t extra = 1; extra < 4; extra++) {
        bytes b = file(1, 1);
        b.resize(b.size() + extra);
        refused(b, XTRA_E_SIZE, b.size());
        bytes c = file(0, 0);
        c.resize(c.size() + extra);
        refused(c, XTRA_E_SIZE, c.size());
    }
}

TEST_CASE("broken: vers other than 0 and 1", "[xtra][broken]")
{
    for (unsigned v : {2u, 0x80u, 0xFFu}) {
        bytes b = file(1, 0);
        b[0] = uint8_t(v);
        refused(b, XTRA_E_VERS, 0);
    }
}

TEST_CASE("broken: text fields without their 0 byte", "[xtra][broken]")
{
    for (unsigned i = 0; i < XTRA_NRES; i++) {
        bytes b = file(1, 0);
        std::memset(&b[54 + 68 * i], 'r', 64);
        refused(b, XTRA_E_UNTERMINATED, 54 + 68 * i);
    }
    bytes b = file(1, 0);
    std::memset(&b[598], 'p', 192);
    refused(b, XTRA_E_UNTERMINATED, 598);
    b = file(1, 0);
    std::memset(&b[790], 'n', 192);
    refused(b, XTRA_E_UNTERMINATED, 790);
    b = file(1, 0);
    std::memset(&b[982], 'c', 128);
    refused(b, XTRA_E_UNTERMINATED, 982);
    // vers 0: the same bytes are the first pairs, not a CLI argument
    b = file(0, 32);
    std::memset(&b[982], 'c', 128);
    CHECK(read(b) == XTRA_OK);
}

TEST_CASE("broken: more maxima than XTRA_MAXMAXIMA", "[xtra][broken]")
{
    refused(file(1, XTRA_MAXMAXIMA + 1), XTRA_E_TOO_MANY, 1110 + 4 * XTRA_MAXMAXIMA);
    refused(file(0, XTRA_MAXMAXIMA + 5), XTRA_E_TOO_MANY, 982 + 4 * XTRA_MAXMAXIMA);
}

TEST_CASE("xtra_add_max: full", "[xtra][broken]")
{
    xtra_init(&x);
    for (unsigned i = 0; i < XTRA_MAXMAXIMA; i++)
        REQUIRE(xtra_add_max(&x, int(i), nullptr, 1) == XTRA_OK);
    CHECK(xtra_add_max(&x, 1, nullptr, 1) == XTRA_E_TOO_MANY);
    CHECK(x.nmax == XTRA_MAXMAXIMA);
}

TEST_CASE("xtra_strerror: a message for every code", "[xtra]")
{
    for (int c = 0; c < XTRA_E_COUNT; c++)
        CHECK(std::string(xtra_strerror(c)) != "unknown error");
    CHECK(std::string(xtra_strerror(XTRA_E_COUNT)) == "unknown error");
}
