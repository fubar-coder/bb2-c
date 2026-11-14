// test_fontlit - TED-C's screen fonts of 256 glyphs (spec
// editor/screen-font; change ted-latin1-glyphs, design D1, D2, D5), as
// res.c embeds them: the strips 0-7 are TED 1.1's chars8.b / chars.b
// (except the redrawn 6 px glyph 26), every code $A1-$FF has a glyph,
// $A0 is blank, $81-$9F (and $80) are the hollow box, the umlauts are
// TED's glyphs 26-31, the 6 px glyph 26 is the dots on TED's a, fontcalc
// serves every code 0-255 inside the font, and the digests of both fonts
// (an accidental change of a glyph fails here).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "hostenv.h"
#include "keymap.h"
#include "sha256.h"
}

namespace {

// the archive font file of height h (chars8.b, chars.b)
std::vector<UBYTE> archive(int h)
{
    std::string path = std::string(TED_ARCHIVE_DIR) + (h == 8 ? "/chars8.b" : "/chars.b");
    long len = 0;
    UBYTE *data = host_readfile(path.c_str(), &len);

    REQUIRE(data != nullptr);
    std::vector<UBYTE> v(data, data + len);
    std::free(data);
    return v;
}

const UBYTE *font(int h)
{
    return h == 8 ? font8 : font6;
}

// the rows of glyph c of a font of height h in the strip layout
std::vector<UBYTE> glyph(const UBYTE *f, int h, unsigned c)
{
    std::vector<UBYTE> g;

    for (int r = 0; r < h; r++)
        g.push_back(f[(c & 15) + (c >> 4) * 16 * h + r * 16]);
    return g;
}

// rows as text, "#" set and "." blank (for the messages)
std::string pic(const std::vector<UBYTE> &g)
{
    std::string s;

    for (UBYTE b : g) {
        for (int i = 7; i >= 0; i--)
            s += (b >> i) & 1 ? '#' : '.';
        s += ' ';
    }
    return s;
}

std::vector<UBYTE> box(int h)
{
    std::vector<UBYTE> g(static_cast<size_t>(h), 0x42);     // .#....#.

    g[0] = g[static_cast<size_t>(h - 2)] = 0x7E;            // .######.
    g[static_cast<size_t>(h - 1)] = 0;
    return g;
}

} // namespace

TEST_CASE("the fonts have 256 glyphs, glyphs 0-127 the original's", "[font]")
{
    CHECK(sizeof(font8) == 2048u);
    CHECK(sizeof(font6) == 1536u);
    for (int h : { 8, 6 }) {
        CAPTURE(h);
        std::vector<UBYTE> a = archive(h);

        REQUIRE(a.size() == static_cast<size_t>(128 * h));
        for (unsigned c = 0; c < 128; c++) {
            CAPTURE(c);
            if (h == 6 && c == 26)
                CHECK(glyph(font(h), h, c) != glyph(a.data(), h, c));   // redrawn
            else
                CHECK(glyph(font(h), h, c) == glyph(a.data(), h, c));
        }
    }
    // the 8 px font begins with chars8.b unchanged
    std::vector<UBYTE> a8 = archive(8);
    CHECK(std::memcmp(font8, a8.data(), a8.size()) == 0);
}

TEST_CASE("every code $A0-$FF has a glyph, $81-$9F the box", "[font]")
{
    for (int h : { 8, 6 }) {
        CAPTURE(h);
        const std::vector<UBYTE> blank(static_cast<size_t>(h), 0);

        CHECK(glyph(font(h), h, 0xA0) == blank);        // no-break space
        for (unsigned c = 0xA1; c <= 0xFF; c++) {
            CAPTURE(c);
            CHECK(glyph(font(h), h, c) != blank);
            CHECK(glyph(font(h), h, c) != box(h));
        }
        for (unsigned c = 0x80; c <= 0x9F; c++) {
            CAPTURE(c);
            CHECK(glyph(font(h), h, c) == box(h));
        }
        // $AD (soft hyphen) is a dash: one row
        int rows = 0;
        for (UBYTE b : glyph(font(h), h, 0xAD))
            rows += b != 0;
        CHECK(rows == 1);
        // the box is none of TED's O, 0, D
        for (unsigned c : { 'O', '0', 'D' })
            CHECK(glyph(font(h), h, c) != box(h));
    }
}

TEST_CASE("every glyph $A1-$FF is its own: a capital differs from its small letter", "[font]")
{
    // user decision 2026-11-14: no two of $A1-$FF share a glyph (the
    // capitals with a mark included), except the umlauts, which are TED's
    // 26-31; and the 6 px inverted exclamation mark is not TED's i
    for (int h : { 8, 6 }) {
        for (unsigned a = 0xA1; a <= 0xFF; a++)
            for (unsigned b = a + 1; b <= 0xFF; b++) {
                CAPTURE(h, a, b);
                CHECK(glyph(font(h), h, a) != glyph(font(h), h, b));
            }
        for (unsigned a = 0xA1; a <= 0xFF; a++)
            for (unsigned c = 0x21; c < 0x7F; c++) {
                CAPTURE(h, a, c);
                CHECK(glyph(font(h), h, a) != glyph(font(h), h, c));
            }
    }
}

TEST_CASE("the umlauts are TED's glyphs 26-31", "[font]")
{
    const unsigned latin1[] = { 0xE4, 0xF6, 0xFC, 0xC4, 0xD6, 0xDC };

    for (int h : { 8, 6 })
        for (unsigned i = 0; i < 6; i++) {
            CAPTURE(h, i);
            CHECK(glyph(font(h), h, latin1[i]) == glyph(font(h), h, 26 + i));
            // and germs maps the Latin-1 umlaut to that code
            CHECK(germs_map(static_cast<UBYTE>(latin1[i])) == ((1ul << 30) | (26 + i)));
        }
}

TEST_CASE("the 6 px glyph 26: the dots in row 0 on TED's a", "[font]")
{
    std::vector<UBYTE> g = glyph(font6, 6, 26), a = glyph(font6, 6, 'a');

    INFO(pic(g));
    CHECK(g[0] != 0);                   // the dots
    CHECK(a[0] == 0);                   // TED's a: rows 1-4
    for (int r = 1; r < 6; r++)
        CHECK(g[static_cast<size_t>(r)] == a[static_cast<size_t>(r)]);
    // the 8 px glyph 26 stays TED's (dots row 0, gap, a in rows 2-6)
    std::vector<UBYTE> a8 = archive(8);
    CHECK(glyph(font8, 8, 26) == glyph(a8.data(), 8, 26));
}

TEST_CASE("fontcalc serves every code 0-255 inside the font", "[font]")
{
    UWORD keep = fonthite;

    for (int h : { 8, 6 }) {
        fonthite = static_cast<UWORD>(h);
        for (unsigned c = 0; c < 256; c++) {
            CAPTURE(h, c);
            const UBYTE *p = fontcalc(static_cast<UBYTE>(c));
            size_t at = (c & 15) + (c >> 4) * 16 * static_cast<size_t>(h);

            CHECK(p == font(h) + at);
            // the last row of the glyph is inside the font
            CHECK(at + static_cast<size_t>(h - 1) * 16 < static_cast<size_t>(256 * h));
        }
    }
    fonthite = keep;
    CHECK(host_hazards == 0);
}

TEST_CASE("the digests of the fonts", "[font]")
{
    char hex[65];

    // a changed glyph source or archive font changes them: update after
    // looking at the change (tedfont builds the fonts, fonts.cmake)
    sha256_hex_of(font8, sizeof font8, hex);
    CHECK(std::string(hex) == "4e0e45047a35512923dc792a46a5473f4afec7cfc5dde108f64ca0138365ea39");
    sha256_hex_of(font6, sizeof font6, hex);
    CHECK(std::string(hex) == "e6780df6107767b034ed0f63a137a077e1cb2f192967214df87207bf06064ede");
}
