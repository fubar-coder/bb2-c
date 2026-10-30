// test_xtra_load - loading a source with shared/xtra (xtra.h): the type
// detection, the companion's name, xtra_load for every type (with and
// without .xtra, a type given by the caller), and broken calls.
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

struct xtra x;

bytes b(const std::string &s)
{
    return bytes(s.begin(), s.end());
}

int detect(const bytes &v, const char *path = nullptr)
{
    return xtra_detect(v.data(), v.size(), path);
}

bytes xtra_file()
{
    bytes f(1110 + 4, 0);

    f[0] = 1;
    f[1] = 0x80;
    std::memcpy(&f[790], "test", 4);
    f[1111] = 3;
    f[1113] = 9;
    return f;
}

} // namespace

TEST_CASE("the structure is small", "[xtra]")
{
    // m68k: 300 bytes (docs/formats.md 1.4); 64-bit hosts: more for the
    // pointers
    CHECK(sizeof(struct xtra) <= 400);
}

TEST_CASE("xtra_detect: the type of a source", "[xtra]")
{
    CHECK(detect(b(std::string("BB22\0\x0a", 6))) == XTRA_T_BB22);
    CHECK(detect(b("BB22")) == XTRA_T_BB22);       // too short: xtra_load says so
    CHECK(detect(b(std::string(";\0NPrint 1\0", 11))) == XTRA_T_TED);
    CHECK(detect(b(std::string("\0", 1))) == XTRA_T_TED);
    CHECK(detect(b(std::string("; XTRA\n; /XTRA\n\0", 16))) == XTRA_T_TED);   // a $00: tokenised
    CHECK(detect(b("; XTRA\n; /XTRA\nNPrint 1\n")) == XTRA_T_AB3);
    CHECK(detect(b("  ;xtra \r\nEnd\n"), "a.bb2") == XTRA_T_AB3);
    CHECK(detect(b("NPrint 1\n")) == XTRA_T_TEXT);
    CHECK(detect(b("NPrint 1\n"), "dh0:x.bb2") == XTRA_T_TEXT);
    CHECK(detect(b("NPrint 1\n"), "dh0:x.ab3") == XTRA_T_AB3);
    CHECK(detect(b("NPrint 1\n"), "DH0:X.AB3") == XTRA_T_AB3);
    CHECK(detect(b("NPrint 1\n"), "ab3") == XTRA_T_TEXT);
    CHECK(detect(b("NPrint 1\n"), "x.ab3.bak") == XTRA_T_TEXT);
    CHECK(detect(bytes{}) == XTRA_T_TEXT);
    CHECK(detect(bytes{}, "new.ab3") == XTRA_T_AB3);
    CHECK(xtra_detect(nullptr, 0, nullptr) == XTRA_T_TEXT);
}

TEST_CASE("the companion: NAME.xtra for TED, BB22 and plain text", "[xtra]")
{
    char out[32];

    CHECK(xtra_has_companion(XTRA_T_TED));
    CHECK(xtra_has_companion(XTRA_T_BB22));
    CHECK(xtra_has_companion(XTRA_T_TEXT));
    CHECK(!xtra_has_companion(XTRA_T_AB3));
    CHECK(!xtra_has_companion(XTRA_T_COUNT));
    REQUIRE(xtra_companion_name("dh0:src/x.bb2", out, sizeof(out)) == XTRA_OK);
    CHECK(std::string(out) == "dh0:src/x.bb2.xtra");
    REQUIRE(xtra_companion_name("", out, 6) == XTRA_OK);
    CHECK(std::string(out) == ".xtra");
    std::memset(out, 'Q', sizeof(out));
    CHECK(xtra_companion_name("abc", out, 8) == XTRA_E_SPACE);
    CHECK(out[0] == 'Q');                   // nothing written
    CHECK(xtra_companion_name("abc", out, 9) == XTRA_OK);
}

TEST_CASE("xtra_load: TED and text sources with and without .xtra", "[xtra]")
{
    bytes f = xtra_file();
    bytes ted = b(std::string(";\0NPrint 1\0", 11));
    struct xtra_base base = {f.data(), ted.data()};
    size_t n;

    for (int t : {XTRA_T_TED, XTRA_T_TEXT}) {
        REQUIRE(xtra_load(&x, t, ted.data(), ted.size(), f.data(), f.size(), nullptr) == XTRA_OK);
        CHECK(x.type == t);
        CHECK(x.hdr == 0);
        CHECK(x.source == XTRA_SRC_BLITZ2);
        const uint8_t *p = xtra_text(&x, &base, XTRA_F_EXE_NAME, &n);
        CHECK(std::string(reinterpret_cast<const char *>(p), n) == "test");
        CHECK(x.max.n == 1);
        REQUIRE(xtra_load(&x, t, ted.data(), ted.size(), nullptr, 0, nullptr) == XTRA_OK);
        CHECK(x.source == XTRA_SRC_NONE);
        CHECK(x.icon == XTRA_ON);
        CHECK(x.type == t);
    }
    // the caller's type: a text source loaded as AmiBlitz3 (no header)
    bytes txt = b("NPrint 1\n");
    REQUIRE(xtra_load(&x, XTRA_T_AB3, txt.data(), txt.size(), f.data(), f.size(), nullptr) == XTRA_OK);
    CHECK(x.source == XTRA_SRC_AB3_TEXT);
    CHECK(x.hdr == 0);
    CHECK(x.icon == 0);
}

TEST_CASE("xtra_take_blitz2: what a .xtra holds, nothing else", "[xtra]")
{
    static struct xtra y;
    bytes f = xtra_file();

    REQUIRE(xtra_load(&y, XTRA_T_TED, nullptr, 0, f.data(), f.size(), nullptr) == XTRA_OK);
    xtra_init(&x);
    x.editor.flags = XTRA_ED_CURSOR;
    x.editor.line = 7;
    x.ab3.stack = 1234;
    x.icon = 0;
    x.sw[XTRA_SW_DEBUGGER] = 0;
    x.vers = 0;
    xtra_take_blitz2(&x, &y);
    CHECK(x.icon == 0x80);
    CHECK(x.vers == 1);
    CHECK(x.sw[XTRA_SW_DEBUGGER] == 0);     // the file's word
    CHECK(x.text[XTRA_F_EXE_NAME].in == XTRA_IN_XTRA);
    CHECK(x.text[XTRA_F_EXE_NAME].off == 790);
    CHECK(x.max.in == XTRA_IN_NONE);        // not taken
    CHECK(x.editor.line == 7);
    CHECK(x.ab3.stack == 1234);
}

TEST_CASE("broken: unknown type", "[xtra][broken]")
{
    struct xtra_error err;

    CHECK(xtra_load(&x, XTRA_T_COUNT, nullptr, 0, nullptr, 0, &err) == XTRA_E_TYPE);
    CHECK(err.code == XTRA_E_TYPE);
    CHECK(xtra_load(&x, -1, nullptr, 0, nullptr, 0, nullptr) == XTRA_E_TYPE);
    CHECK(x.source == XTRA_SRC_NONE);
}
