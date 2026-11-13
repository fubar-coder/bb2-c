// test_literal_redraw - literal characters drawn as their glyphs in every
// row, also after a redraw (spec editor/literal-characters "Display, help
// and colour"; change ted-latin1-glyphs, design D4a: the literal map
// beside chmap).  With tokenise on and off.
//
// The redraws of the AmigaOS part (src/screen.c scrollrite, scrollleft;
// src/input.c refresh, reline, the last cell after a delete in char127)
// read the cells of chmap and draw each with print3at, which draws the
// glyph cellglyph (core/literal.c) gives; showa2item draws through the
// draw hook of chmap_showa2item; showmice (src/mouse.c) draws the
// characters litnext gives.  These tests check what they draw: the draw
// hook's characters, and cellglyph of the cells each redraw reads -
// horizontal scroll right and left (cursoff moved), refresh, reline,
// delete at the end of a row, the vertical scrolls, erase and cls - and
// that every literal cell of chmap has its character in the map after
// each operation.
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include <snitch/snitch.hpp>
#include "lines_helpers.hpp"
extern "C" {
#include "hostenv.h"
#include "fileio.h"
#include "lineedit.h"
#include "lines.h"
#include "tokit.h"
#include "keymap.h"
#include "literal.h"
}
#include "oracle.hpp"

namespace {

const std::string P = std::string("\x90", 1);
constexpr UWORD ROWS = 6;

void env(UWORD tok)
{
    static bool done;

    if (!done) {
        hostenv_init(oracle::tokens_file().c_str(), ROWS);
        done = true;
    }
    REQUIRE(tokens != nullptr);
    tokenise = tok;
    tabstop = 8;
    hscmarg = 12;
    edcols = 80;
    edrows = ROWS;
    bboty = static_cast<UWORD>(-1);
    normcol = 1;
    tokcol = 3;
    color = normcol;
    curstop = 0;
    cursx = cursy = 0;
    cursoff = 0;
}

void lines(const std::vector<std::string> &l)
{
    std::vector<const char *> p;

    for (auto &s : l)
        p.push_back(s.c_str());
    mklist(static_cast<int>(l.size()), p.data());
}

// a stored line: the text with each character >= $A0 (and $85) as the pair
std::string stored(const std::string &text)
{
    std::string s;

    for (unsigned char c : text) {
        if (c > 0x80)
            s += P;
        s += static_cast<char>(c);
    }
    return s;
}

// the glyphs the draw hook got, per screen cell
std::vector<std::vector<int>> screen;

void draw(UBYTE c)
{
    screen.at(cursy).at(cursx) = c;
}

void clearscreen()
{
    screen.assign(ROWS, std::vector<int>(edcols, ' '));
}

// showall from the first line, the cursor on row 0
void showall_()
{
    thisitem = firstitem;
    cursy = 0;
    cursx = 0;
    calcpos();
    clearscreen();
    chmap_showall(draw);
}

// the text a redraw shows of screen row r: cellglyph of the chmap cells
// cursoff + 0 .. cursoff + edcols - 1 (refresh, reline), as characters
std::string redrawn(UWORD r)
{
    std::string s;

    for (UWORD x = 0; x < edcols; x++) {
        BOOL tok;

        s += static_cast<char>(cellglyph(chmap + r * chcols + cursoff + x, &tok));
    }
    return s;
}

// the text of the stored line as columns (literal characters as theirs)
std::string columns(const std::string &text, UWORD from, UWORD n)
{
    std::string s = text.substr(std::min<size_t>(from, text.size()), n);

    s.resize(n, ' ');
    return s;
}

// the draw hook's characters of screen row r
std::string drawn(UWORD r)
{
    std::string s;

    for (int c : screen.at(r))
        s += static_cast<char>(c);
    return s;
}

// every literal cell of chmap has its character in the map, and the map
// has none elsewhere (design risk: the map out of step with chmap)
bool inStep()
{
    for (UWORD i = 0; i < ROWS * chcols; i++) {
        BOOL tok;
        UBYTE g = cellglyph(chmap + i, &tok);

        if (chmap[i] == TED_LITGLYPH && g < 0x81)
            return false;
        if (chmap[i] != TED_LITGLYPH && litat(chmap + i))
            return false;
        if (g >= 0x80 && tok)
            return false;               // a literal: the colour of the text
    }
    return true;
}

// all of $A0-$FF in three lines of 32 (each behind 50 x so that the line
// is wider than the screen: 82 columns), and a C1 control $85
const std::string X50(50, 'x');
std::string text(int i)
{
    std::string s = X50;

    for (int c = 0xA0 + 32 * i; c < 0xA0 + 32 * (i + 1); c++)
        s += static_cast<char>(c);
    return s;
}

const std::string C1 = "a\x85" "b";

void fourlines()
{
    lines({ stored(text(0)), stored(text(1)), stored(text(2)), stored(C1) });
}

} // namespace

TEST_CASE("every row draws its literal characters as themselves", "[literal][redraw]")
{
    for (UWORD tok = 0; tok < 2; tok++) {
        CAPTURE(tok);
        env(tok);
        fourlines();
        showall_();
        for (UWORD r = 0; r < 3; r++) {
            CAPTURE(r);
            CHECK(drawn(r) == columns(text(r), 0, edcols));
            CHECK(redrawn(r) == columns(text(r), 0, edcols));    // refresh
        }
        CHECK(drawn(3) == columns(C1, 0, edcols));              // the box
        CHECK(redrawn(3) == columns(C1, 0, edcols));
        CHECK(chmap[3 * chcols + 1] == TED_LITGLYPH);
        CHECK(inStep());
    }
}

TEST_CASE("horizontal scroll right and left redraws the literal characters", "[literal][redraw]")
{
    for (UWORD tok = 0; tok < 2; tok++) {
        CAPTURE(tok);
        env(tok);
        fourlines();
        showall_();
        // scrollrite (screen.c): cursoff + 16, the two left columns of
        // every row from chmap + cursoff; the other columns are shifted
        // bitmap - in sum the window from cursoff, as refresh draws it
        for (UWORD off : { 16, 2, 0 }) {
            cursoff = off;
            CAPTURE(off);
            for (UWORD r = 0; r < 4; r++) {
                std::string want = columns(r < 3 ? text(r) : C1, off, edcols);
                BOOL tok_;

                CHECK(redrawn(r) == want);
                // scrollrite's two cells, scrollleft's two cells
                CHECK(cellglyph(chmap + r * chcols + off, &tok_) == static_cast<UBYTE>(want[0]));
                CHECK(cellglyph(chmap + r * chcols + off + 1, &tok_) == static_cast<UBYTE>(want[1]));
                CHECK(cellglyph(chmap + r * chcols + off + edcols - 2, &tok_)
                      == static_cast<UBYTE>(want[edcols - 2]));
                CHECK(cellglyph(chmap + r * chcols + off + edcols - 1, &tok_)
                      == static_cast<UBYTE>(want[edcols - 1]));
            }
        }
        // the cursor on row 1 does not change what the other rows show
        cursoff = 16;
        cursy = 1;
        calcpos();
        calcitem();                     // thisitem: the cursor's line
        litrow_sync();
        CHECK(redrawn(0) == columns(text(0), 16, edcols));
        CHECK(redrawn(2) == columns(text(2), 16, edcols));
        CHECK(inStep());
    }
}

TEST_CASE("the vertical scrolls, erase and cls move the literal characters with the rows", "[literal][redraw]")
{
    for (UWORD tok = 0; tok < 2; tok++) {
        CAPTURE(tok);
        env(tok);
        fourlines();
        showall_();
        chmap_scrollup(0);              // rows 1.. one up, the last row blank
        CHECK(redrawn(0) == columns(text(1), 0, edcols));
        CHECK(redrawn(1) == columns(text(2), 0, edcols));
        CHECK(redrawn(2) == columns(C1, 0, edcols));
        CHECK(redrawn(ROWS - 1) == std::string(edcols, ' '));
        CHECK(inStep());
        chmap_scrolldown(0);            // rows 0.. one down, row 0 blank
        CHECK(redrawn(0) == std::string(edcols, ' '));
        CHECK(redrawn(1) == columns(text(1), 0, edcols));
        CHECK(redrawn(3) == columns(C1, 0, edcols));
        CHECK(inStep());
        chmap_scrolldown(2);            // from row 2: row 2 blank
        CHECK(redrawn(1) == columns(text(1), 0, edcols));
        CHECK(redrawn(2) == std::string(edcols, ' '));
        CHECK(redrawn(3) == columns(text(2), 0, edcols));
        CHECK(redrawn(4) == columns(C1, 0, edcols));
        CHECK(inStep());
        chmap_eraseline(1);
        CHECK(redrawn(1) == std::string(edcols, ' '));
        CHECK(litat(chmap + chcols + 60) == 0);
        CHECK(inStep());
        chmap_cls();
        for (UWORD r = 0; r < ROWS; r++)
            CHECK(redrawn(r) == std::string(edcols, ' '));
        CHECK(inStep());
    }
}

TEST_CASE("typing, deleting and reline on a row draw the characters", "[literal][redraw]")
{
    for (UWORD tok = 0; tok < 2; tok++) {
        CAPTURE(tok);
        env(tok);
        fourlines();
        showall_();
        // the cursor on row 2, column 0: getline's entry
        cursy = 2;
        cursx = 0;
        calcpos();
        calcitem();                     // thisitem: the cursor's line
        litrow_sync();
        // type a literal character (input.c islit: gl_islit, printlit's
        // chmap and map write)
        gl_islit(0xE9);
        chmap[chloc + cursoff + cursx] = TED_LITGLYPH;
        litmap_put(chmap + chloc + cursoff + cursx, 0xE9);
        cursrite();
        std::string t = "\xe9" + text(2);
        CHECK(redrawn(2) == columns(t, 0, edcols));             // reline
        CHECK(inStep());
        // a plain character in front of it (.isasc)
        cursx = 0;
        gl_isasc('q');
        chmap[chloc + cursoff + cursx] = 'q';
        t = "q" + t;
        CHECK(redrawn(2) == columns(t, 0, edcols));
        CHECK(inStep());
        // delete at the cursor (char127): the row one to the left, the
        // last visible cell redrawn from chmap holds what was behind it
        for (int i = 0; i < 4; i++) {
            gl_char127(127);
            t.erase(0, 1);
            BOOL tok_;
            CHECK(cellglyph(chmap + chloc + edcols + cursoff - 1, &tok_)
                  == static_cast<UBYTE>(columns(t, 0, edcols)[edcols - 1]));
            CHECK(redrawn(2) == columns(t, 0, edcols));
        }
        CHECK(inStep());
        // delete to the end of the line, then reline
        cursx = 60;
        gl_delright();
        CHECK(redrawn(2) == columns(t.substr(0, 60), 0, edcols));
        CHECK(inStep());
        // the other rows are untouched
        CHECK(redrawn(1) == columns(text(1), 0, edcols));
        CHECK(redrawn(3) == columns(C1, 0, edcols));
    }
}

TEST_CASE("showmice: litnext gives the character of a pair or of a detok'd row", "[literal][redraw]")
{
    env(1);
    // a stored line (tokenise off: showmice reads the line itself)
    std::string line = stored(".l\xe9\xdf" "b:");
    const UBYTE *src = reinterpret_cast<const UBYTE *>(line.c_str());
    std::string got;
    UBYTE c;

    while ((c = litnext(&src, TRUE)) != 0)
        got += static_cast<char>(c);
    CHECK(got == ".l\xe9\xdf" "b:");
    // a detok'd row (tokenise on: charbuff, the characters as themselves,
    // a literal $90 among them stays one character)
    std::string row = ".a\x90\xe9:";
    src = reinterpret_cast<const UBYTE *>(row.c_str());
    got.clear();
    while ((c = litnext(&src, FALSE)) != 0)
        got += static_cast<char>(c);
    CHECK(got == row);
}
