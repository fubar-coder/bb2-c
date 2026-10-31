// test_xtra_mode - the mode rule of blitz2-ab3 (xtra_mode, shared/xtra):
// the scenarios of the spec ab3-compat/source-mode - AB3 text sources
// recognised (embedded header, *.ab3), tokenised sources by their
// companion .xtra, other sources BB2 unless the mode is forced (both
// ways) - and the rule applied to a tokenised include.
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

bytes b(const std::string &s)
{
    return bytes(s.begin(), s.end());
}

void put32(bytes &f, size_t off, uint32_t v)
{
    for (int i = 0; i < 4; i++)
        f[off + i] = uint8_t(v >> (24 - 8 * i));
}

const uint32_t buffers[6] = { 16384, 32768, 4096, 4096, 8192, 10240 };

// AmiBlitz3's .xtra (15 switch words, docs/formats.md 1.3), no maxima
bytes ab3_xtra()
{
    bytes f(XTRA_AB3_FIXED + 4, 0);

    f[0] = 1;
    for (int i = 0; i < 6; i++)
        put32(f, 32 + 4 * i, buffers[i]);
    return f;
}

// Blitz 2's .xtra (14 switch words), no maxima
bytes blitz2_xtra()
{
    bytes f(XTRA_FIXED_V1, 0);

    f[0] = 1;
    for (int i = 0; i < 6; i++)
        put32(f, 30 + 4 * i, buffers[i]);
    return f;
}

const bytes header_text = b("; XTRA\n; Embedded .xtra header\n; /XTRA\nNPrint \"hi\"\nEnd\n");
const bytes plain_text = b("NPrint \"hi\"\nEnd\n");
const bytes tokenised = b(std::string("\x9a\x80\"hi\"\0\x80\x33\0", 11));
const bytes bb22 = b(std::string("BB22\0\x0a\0\0\0\0\0\0\0\x02", 14));

int mode(const bytes &src, const char *path, const bytes *xtra = nullptr,
         int force = XTRA_MODE_DETECT)
{
    return xtra_mode(force, src.data(), src.size(), path, xtra ? xtra->data() : nullptr,
                     xtra ? xtra->size() : 0);
}

} // namespace

TEST_CASE("AB3 text sources are recognised", "[xtra][mode]")
{
    // text with the embedded header, whatever its name
    CHECK(mode(header_text, "prog.bb") == XTRA_MODE_AB3);
    CHECK(mode(header_text, nullptr) == XTRA_MODE_AB3);
    // a text named .ab3 without header, in any case
    CHECK(mode(plain_text, "prog.ab3") == XTRA_MODE_AB3);
    CHECK(mode(plain_text, "DH0:Src/PROG.AB3") == XTRA_MODE_AB3);
    CHECK(mode(plain_text, "prog.Ab3") == XTRA_MODE_AB3);
    // a text has no companion: an AmiBlitz3 .xtra next to it changes nothing
    bytes x = ab3_xtra();
    CHECK(mode(plain_text, "prog.bb", &x) == XTRA_MODE_BB2);
}

TEST_CASE("tokenised AB3 sources are recognised by their companion", "[xtra][mode]")
{
    bytes ab3 = ab3_xtra(), bb2 = blitz2_xtra();

    CHECK(mode(tokenised, "prog.bb2", &ab3) == XTRA_MODE_AB3);
    CHECK(mode(tokenised, "prog.bb2", &bb2) == XTRA_MODE_BB2);
    CHECK(mode(tokenised, "prog.bb2") == XTRA_MODE_BB2);          // no .xtra
    // the name does not count for a tokenised source
    CHECK(mode(tokenised, "prog.ab3") == XTRA_MODE_BB2);
    CHECK(mode(tokenised, "prog.ab3", &ab3) == XTRA_MODE_AB3);
    // a truncated AmiBlitz3 .xtra is not one
    bytes cut(ab3.begin(), ab3.begin() + XTRA_AB3_FIXED);
    CHECK(mode(tokenised, "prog.bb2", &cut) == XTRA_MODE_BB2);
    // RWE's BB22 sources are Blitz 2's
    CHECK(mode(bb22, "prog.bb", &ab3) == XTRA_MODE_BB2);
}

TEST_CASE("other sources are BB2 unless the mode is forced", "[xtra][mode]")
{
    bytes ab3 = ab3_xtra();

    CHECK(mode(plain_text, "prog.bb") == XTRA_MODE_BB2);
    CHECK(mode(plain_text, nullptr) == XTRA_MODE_BB2);
    CHECK(mode(bytes(), "empty.bb") == XTRA_MODE_BB2);
    // forced to AB3
    CHECK(mode(plain_text, "prog.bb", nullptr, XTRA_MODE_AB3) == XTRA_MODE_AB3);
    CHECK(mode(tokenised, "prog.bb2", nullptr, XTRA_MODE_AB3) == XTRA_MODE_AB3);
    // forced to BB2: also for what would be AB3
    CHECK(mode(plain_text, "prog.ab3", nullptr, XTRA_MODE_BB2) == XTRA_MODE_BB2);
    CHECK(mode(header_text, "prog.ab3", nullptr, XTRA_MODE_BB2) == XTRA_MODE_BB2);
    CHECK(mode(tokenised, "prog.bb2", &ab3, XTRA_MODE_BB2) == XTRA_MODE_BB2);
    // an unknown force value detects
    CHECK(mode(plain_text, "prog.ab3", nullptr, 7) == XTRA_MODE_AB3);
}

TEST_CASE("the rule for a tokenised include", "[xtra][mode]")
{
    // an include is judged like a main source without force; the caller
    // compares the result with the program's mode
    bytes ab3 = ab3_xtra(), bb2 = blitz2_xtra();

    CHECK(mode(tokenised, "inc.bb2") == XTRA_MODE_BB2);
    CHECK(mode(tokenised, "inc.bb2", &bb2) == XTRA_MODE_BB2);
    CHECK(mode(tokenised, "inc.bb2", &ab3) == XTRA_MODE_AB3);
}
