// test_calcrows - calcscreen's rows (ted.s:4644-4659, core calcrows):
// the screen (window + topskip) is never taller than the Workbench
// (fixed #351: the original's was one row taller when height - 20 is a
// multiple of fonthite), and for every other height the rows are the
// original's.
#include <snitch/snitch.hpp>
#include "lines_helpers.hpp"
extern "C" {
#include "tokit.h"
}

namespace {

// the original's computation (ted.s:4644-4659)
struct geometry {
    unsigned edrows, pixrows, window;
};

geometry original(unsigned height, unsigned font)
{
    unsigned rows = (height - 12 - 8) / font - 1;

    return {rows, rows * font, rows * font + font + 8 + 1};
}

} // namespace

TEST_CASE("calcrows: the screen fits the Workbench", "[tokit][screen]")
{
    UWORD oldtop = topskip, oldbot = botskip, oldfont = fonthite;

    topskip = 12;
    botskip = 8;
    for (UWORD font : {UWORD(6), UWORD(8)}) {
        fonthite = font;
        for (unsigned height = 100; height <= 1200; height++) {
            CAPTURE(font, height);
            UWORD window = calcrows((UWORD)height);
            geometry orig = original(height, font);

            REQUIRE(window + topskip <= height);
            REQUIRE(pixrows == edrows * font);
            REQUIRE(window == pixrows + font + botskip + 1);
            if ((height - 20) % font != 0) {
                // the original's rows
                REQUIRE(edrows == orig.edrows);
                REQUIRE(window == orig.window);
            } else {
                // there the original's screen was height + 1 rows
                REQUIRE(orig.window + 12 == height + 1);
                REQUIRE(edrows == orig.edrows - 1);
            }
        }
    }
    // the cases of the issue: font 6 on 512 rows, font 8 on 260 rows; the
    // sessions' Workbench (256 rows) as the original
    fonthite = 6;
    REQUIRE(calcrows(512) + topskip == 507);
    REQUIRE(edrows == 80);
    fonthite = 8;
    REQUIRE(calcrows(260) + topskip == 253);
    REQUIRE(edrows == 28);
    REQUIRE(calcrows(256) + topskip == 253);
    REQUIRE(edrows == 28);
    topskip = oldtop;
    botskip = oldbot;
    fonthite = oldfont;
}
