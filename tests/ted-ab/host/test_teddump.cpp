// test_teddump - screen dumps of teddrive (runner/teddump.h) with dumps
// built in memory: header and windows, broken dumps, geometry, decoding
// with TED's fonts, comparing with the field "Free" masked.  From bb2
// ted-c/tests/tools/test_teddump.py.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "teddump.h"
}

namespace {

constexpr int W = 640, H = 509, FH = 8;   // TED on a 640x512 Workbench: edrows 60
constexpr int RB = W / 8;

const teddump_fonts &fonts()
{
    static teddump_fonts f;
    static bool loaded = teddump_load_fonts(TED_FONTS_DIR, &f) == 0;

    REQUIRE(loaded);
    return f;
}

using Planes = std::vector<std::vector<uint8_t>>;

Planes blank(int depth = 2, int rows = H, int rowbytes = RB)
{
    return Planes(depth, std::vector<uint8_t>(static_cast<size_t>(rowbytes * rows)));
}

const uint8_t *font8() { return fonts().f8; }

void glyph(int c, uint8_t *g)
{
    for (int r = 0; r < FH; r++)
        g[r] = font8()[(c & 15) + (c >> 4) * 16 * FH + r * 16];
}

// print4: glyph & mask(plane) ^ inverse into both planes
void put(Planes &p, int row_px, int col, std::string_view text, int colour = 1, bool inverse = false)
{
    uint8_t inv = inverse ? 0xFF : 0, g[8];

    for (size_t i = 0; i < text.size(); i++) {
        glyph(static_cast<unsigned char>(text[i]), g);
        for (int r = 0; r < FH; r++) {
            size_t o = static_cast<size_t>((row_px + r) * RB + col) + i;

            p[0][o] = static_cast<uint8_t>(((colour & 1) ? g[r] : 0) ^ inv);
            p[1][o] = static_cast<uint8_t>(((colour & 2) ? g[r] : 0) ^ inv);
        }
    }
}

// qprint: raw glyph into plane 1 (status line)
void qput(Planes &p, int row_px, int col, std::string_view text)
{
    uint8_t g[8];

    for (size_t i = 0; i < text.size(); i++) {
        glyph(static_cast<unsigned char>(text[i]), g);
        for (int r = 0; r < FH; r++)
            p[1][static_cast<size_t>((row_px + r) * RB + col) + i] = g[r];
    }
}

Planes ted_screen(std::string_view free = "12345", std::string_view line2 = "a=1")
{
    Planes p = blank();
    int status = 12 + 60 * FH + 9;

    put(p, 12, 0, "NPrint \"hi\"");
    put(p, 12 + FH, 0, line2, 3);
    put(p, 12 + 2 * FH, 4, "X", 1, true);
    qput(p, status, 0, "Line:   1  Column:   1");
    qput(p, status, 38, free);
    return p;
}

// the file teddrive writes
std::string build(const Planes &p, int width = W, int height = H, int rowbytes = RB,
                  const char *magic = "TEDSCR1", const char *extra = "")
{
    char head[512];

    std::snprintf(head, sizeof head,
                  "magic %s\nwidth %d\nheight %d\ndepth %zu\nbytesperrow %d\nrows %zu\n"
                  "rowbytes %d\ninterleaved 0\nmodeid $00029004\nmousex 0\nmousey 0\n"
                  "memtype $00000303\ntitle File: t01_int.bb2\n%s",
                  magic, width, height, p.size(), rowbytes, p[0].size() / rowbytes, rowbytes, extra);
    std::string s = std::string(head) + "data\n";
    for (const auto &plane : p)
        s.append(reinterpret_cast<const char *>(plane.data()), plane.size());
    return s;
}

struct dump {
    teddump d{};
    ~dump() { teddump_free(&d); }
};

bool parse(const std::string &s, dump &out)
{
    char err[200];

    return teddump_parse(reinterpret_cast<const uint8_t *>(s.data()), s.size(), &out.d, err,
                         sizeof err) == 0;
}

std::string compare(const teddump &a, const teddump &b, bool no_mask = false, int *rc = nullptr)
{
    char *buf = nullptr;
    size_t len = 0;
    FILE *f = open_memstream(&buf, &len);
    int r = teddump_compare(&a, &b, &fonts(), FH, no_mask, f);

    std::fclose(f);
    std::string s(buf ? buf : "", len);
    std::free(buf);
    if (rc)
        *rc = r;
    return s;
}

} // namespace

TEST_CASE("header and windows", "[teddump]")
{
    dump d;
    REQUIRE(parse(build(ted_screen(), W, H, RB, "TEDSCR1",
                        "windows 2\nwindow 0 0 640 509 $00001900 \n"
                        "window 100 200 300 80 $0000100A Name to save\n"), d));
    CHECK(d.d.width == W);
    CHECK(d.d.height == H);
    CHECK(d.d.depth == 2);
    CHECK(d.d.rowbytes == RB);
    CHECK(d.d.modeid == 0x29004);
    CHECK(std::string_view(d.d.title) == "File: t01_int.bb2");
    REQUIRE(d.d.nwindows == 2);
    CHECK(d.d.windows[1].left == 100);
    CHECK(d.d.windows[1].height == 80);
    CHECK(d.d.windows[1].flags == 0x100A);
    CHECK(std::string_view(d.d.windows[1].title) == "Name to save");
    char hex[65];
    teddump_digest(&d.d, hex);
    CHECK(std::strlen(hex) == 64);
}

TEST_CASE("broken dumps", "[teddump]")
{
    std::string good = build(ted_screen());
    dump a, b, c;

    CHECK(!parse(build(ted_screen(), W, H, RB, "TEDSCRX"), a));
    CHECK(!parse(good.substr(0, good.size() - 1), b));
    CHECK(!parse("magic TEDSCR1\nwidth 640\n", c));
}

TEST_CASE("geometry", "[teddump]")
{
    dump d, d6;
    struct teddump_geometry g;

    REQUIRE(parse(build(ted_screen()), d));
    teddump_geometry(&d.d, 8, &g);
    CHECK(g.edrows == 60);
    CHECK(g.status == 12 + 480 + 9);
    REQUIRE(parse(build(blank(2, 27 + 6 * 80), W, 27 + 6 * 80), d6));
    teddump_geometry(&d6.d, 6, &g);
    CHECK(g.edrows == 80);
}

TEST_CASE("decoding", "[teddump]")
{
    dump d;
    REQUIRE(parse(build(ted_screen()), d));
    char *t = teddump_text(&d.d, &fonts(), FH);
    REQUIRE(t != nullptr);
    std::string text(t);
    std::free(t);

    CHECK(text.find("  0 |NPrint \"hi\"\n") != std::string::npos);
    CHECK(text.find("  1 |a=1\n    @333\n") != std::string::npos);
    CHECK(text.find("  2 |    X\n    @    a\n") != std::string::npos);
    CHECK(text.find("  3 |\n") != std::string::npos);
    // blank cells of a colour-1 row are ' ': no attribute line for row 0
    CHECK(text.find("  0 |NPrint \"hi\"\n    @") == std::string::npos);
    CHECK(text.find("status |Line:   1  Column:   1") != std::string::npos);
    CHECK(text.find("12345") != std::string::npos);
}

TEST_CASE("comparing masks the field Free", "[teddump]")
{
    dump a, b;
    int rc;

    REQUIRE(parse(build(ted_screen("12345")), a));
    REQUIRE(parse(build(ted_screen("99999")), b));
    CHECK(compare(a.d, b.d, false, &rc).empty());
    CHECK(rc == 0);
    compare(a.d, b.d, true, &rc);
    CHECK(rc == 1);
}

TEST_CASE("comparing shows the differing text", "[teddump]")
{
    dump a, b, c;
    int rc;

    REQUIRE(parse(build(ted_screen("12345", "a=1")), a));
    REQUIRE(parse(build(ted_screen("12345", "a=2")), b));
    std::string detail = compare(a.d, b.d, false, &rc);
    CHECK(rc == 1);
    CHECK(detail.find("-  1 |a=1") != std::string::npos);
    CHECK(detail.find("+  1 |a=2") != std::string::npos);
    REQUIRE(parse(build(blank(2, H, 40), 320, H, 40), c));
    CHECK(compare(a.d, c.d, false, &rc).find("not comparable") != std::string::npos);
    CHECK(rc == 1);
}

TEST_CASE("pixels", "[teddump]")
{
    Planes p = blank();
    p[0][32 * RB + 2] = 0xFF;           // x 16..23, y 32: colour 1
    p[1][32 * RB + 2] = 0x0F;           // x 20..23: colour 3
    dump d;
    REQUIRE(parse(build(p), d));
    CHECK(teddump_pixel(&d.d, 15, 32) == 0);
    CHECK(teddump_pixel(&d.d, 16, 32) == 1);
    CHECK(teddump_pixel(&d.d, 19, 32) == 1);
    CHECK(teddump_pixel(&d.d, 20, 32) == 3);
    CHECK(teddump_pixel(&d.d, 23, 32) == 3);
    CHECK(teddump_pixel(&d.d, 24, 32) == 0);
}
