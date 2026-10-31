// test_xtra_ab3_binary - recognising AmiBlitz3's binary .xtra with 15
// switch words (xtra_is_ab3_xtra, shared/xtra): the file as AmiBlitz3's
// write_xtrafile writes it (with and without PED's XtraExtend), Blitz 2's
// .xtra, an older AmiBlitz3 .xtra with 14 switch words, broken and
// truncated input.
#include <cstdint>
#include <cstring>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "xtra.h"
}

namespace {

using bytes = std::vector<uint8_t>;

void put16(bytes &f, size_t off, uint16_t v)
{
    f[off] = uint8_t(v >> 8);
    f[off + 1] = uint8_t(v);
}

void put32(bytes &f, size_t off, uint32_t v)
{
    put16(f, off, uint16_t(v >> 16));
    put16(f, off + 2, uint16_t(v));
}

const uint32_t buffers[6] = { 16384, 32768, 4096, 4096, 8192, 10240 };

// AmiBlitz3 @58a11658 write_xtrafile (AmiBlitz3.ab3 25089): vers 1, icon,
// 15 words, 6 longs, residents, exe path and name, 116 bytes CLI argument,
// stack, version, builds, the pairs, 4 bytes (maxerrorcount), then PED's
// XtraExtend (44 bytes) if PED saved it
bytes ab3_xtra(unsigned pairs = 2, bool extend = false)
{
    bytes f(XTRA_AB3_FIXED + 4 * pairs + 4 + (extend ? 44 : 0), 0);

    f[0] = 1;
    f[1] = 0x80;
    for (unsigned i = 0; i < 15; i += 2)
        put16(f, 2 + 2 * i, XTRA_ON);
    for (unsigned i = 0; i < 6; i++)
        put32(f, 32 + 4 * i, buffers[i]);
    std::memcpy(&f[56], "all.res", 7);
    std::memcpy(&f[56 + 68], "amigalibs.res", 13);
    std::memcpy(&f[600], "ram:", 4);
    std::memcpy(&f[792], "prog", 4);
    std::memcpy(&f[984], "-v", 2);
    put32(f, 1100, 32768);              // stack
    put32(f, 1104, 0x030e0000);         // version
    put32(f, 1108, 7);                  // builds
    for (unsigned i = 0; i < pairs; i++) {
        put16(f, XTRA_AB3_FIXED + 4 * i, uint16_t(40 + i));
        put16(f, XTRA_AB3_FIXED + 4 * i + 2, 20);
    }
    put32(f, XTRA_AB3_FIXED + 4 * pairs, 100);  // maxerrorcount
    return f;
}

// Blitz 2's .xtra (docs/formats.md 1.1): vers 1 or 0, 14 words, buffers at 30
bytes blitz2_xtra(int vers = 1, unsigned pairs = 2)
{
    bytes f((vers ? XTRA_FIXED_V1 : XTRA_FIXED_V0) + 4 * pairs, 0);

    f[0] = uint8_t(vers);
    f[1] = 0x80;
    for (unsigned i = 0; i < 14; i += 2)
        put16(f, 2 + 2 * i, XTRA_ON);
    for (unsigned i = 0; i < 6; i++)
        put32(f, 30 + 4 * i, buffers[i]);
    std::memcpy(&f[54], "blitzlibs:amigalibs.res", 23);
    std::memcpy(&f[790], "prog", 4);
    return f;
}

bool is_ab3(const bytes &f)
{
    return xtra_is_ab3_xtra(f.data(), f.size()) != 0;
}

} // namespace

TEST_CASE("AmiBlitz3's .xtra is recognised", "[xtra][ab3]")
{
    CHECK(is_ab3(ab3_xtra()));
    CHECK(is_ab3(ab3_xtra(0)));                 // no maxima
    CHECK(is_ab3(ab3_xtra(30)));
    CHECK(is_ab3(ab3_xtra(2, true)));           // with PED's XtraExtend
    bytes f = ab3_xtra();
    f[1] = 0;                                   // no icon
    put16(f, 2 + 2 * 14, XTRA_ON);              // the 15th word
    CHECK(is_ab3(f));
    bytes g = ab3_xtra();
    std::memset(&g[56], 'r', 63);               // a resident of 63 characters
    CHECK(is_ab3(g));
}

TEST_CASE("Blitz 2's .xtra is not AmiBlitz3's", "[xtra][ab3]")
{
    CHECK_FALSE(is_ab3(blitz2_xtra(1)));
    CHECK_FALSE(is_ab3(blitz2_xtra(1, 0)));
    CHECK_FALSE(is_ab3(blitz2_xtra(1, 1)));
    CHECK_FALSE(is_ab3(blitz2_xtra(0)));
    CHECK_FALSE(is_ab3(blitz2_xtra(0, 41)));
    // a Blitz 2 layout of a multiple of 4 (an older AmiBlitz3 .xtra with
    // 14 switch words, e.g. Tests/68k_crash_newtype.xtra, 1296 bytes):
    // the buffers in the 15-word place are $40000000 ...
    bytes f = blitz2_xtra(1, 0);
    f.resize(1296, 0);
    CHECK_FALSE(is_ab3(f));
}

TEST_CASE("broken and truncated AmiBlitz3 .xtra", "[xtra][ab3]")
{
    CHECK_FALSE(xtra_is_ab3_xtra(nullptr, 0));
    CHECK_FALSE(xtra_is_ab3_xtra(nullptr, 1200));
    bytes f = ab3_xtra(0);
    CHECK(xtra_is_ab3_xtra(f.data(), XTRA_AB3_FIXED + 4));
    CHECK_FALSE(xtra_is_ab3_xtra(f.data(), XTRA_AB3_FIXED));     // without the 4 bytes
    CHECK_FALSE(xtra_is_ab3_xtra(f.data(), XTRA_AB3_FIXED + 2));
    CHECK_FALSE(xtra_is_ab3_xtra(f.data(), 984));
    CHECK_FALSE(xtra_is_ab3_xtra(f.data(), 0));
    bytes g = ab3_xtra(3);
    CHECK_FALSE(xtra_is_ab3_xtra(g.data(), g.size() - 1));     // cut in a pair
    CHECK_FALSE(xtra_is_ab3_xtra(g.data(), g.size() - 2));

    auto broken = [](auto change) {
        bytes b = ab3_xtra();
        change(b);
        return is_ab3(b);
    };
    CHECK_FALSE(broken([](bytes &b) { b[0] = 0; }));            // vers 0
    CHECK_FALSE(broken([](bytes &b) { b[0] = 2; }));
    CHECK_FALSE(broken([](bytes &b) { b[1] = 1; }));            // icon
    CHECK_FALSE(broken([](bytes &b) { put16(b, 2 + 2 * 3, 0x0081); }));
    CHECK_FALSE(broken([](bytes &b) { put16(b, 2 + 2 * 14, 0x8000); }));
    CHECK_FALSE(broken([](bytes &b) { put32(b, 32 + 4 * 5, 0x01000000); }));
    CHECK_FALSE(broken([](bytes &b) { std::memset(&b[56 + 68 * 7], 'r', 64); }));
    CHECK_FALSE(broken([](bytes &b) { std::memset(&b[600], 'p', 192); }));
    CHECK_FALSE(broken([](bytes &b) { std::memset(&b[792], 'p', 192); }));
    CHECK_FALSE(broken([](bytes &b) { std::memset(&b[984], 'c', 116); }));
    CHECK(broken([](bytes &b) { std::memset(&b[984], 'c', 115); }));
}
