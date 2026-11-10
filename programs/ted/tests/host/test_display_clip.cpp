// test_display_clip - the line display (showa2item, ted.s:4104: normprint,
// tokprint) ends a line at column 128 (chcols), the end of its row of the
// character map, also in the middle of a keyword name (fixed #1011: the
// original stored every column, a wider line ran on into the next row and
// from the last row behind the map).  A character map of the test's own
// with a guard area behind its last row.
#include <cstdlib>
#include <cstring>
#include <string>
#include <snitch/snitch.hpp>
#include "lines_helpers.hpp"
extern "C" {
#include "fileio.h"
#include "tokit.h"
}
#include "oracle.hpp"

namespace {

const int ROWS = 4;
const size_t GUARD = 2048;
UBYTE *mymap;

void env()
{
    static bool done;

    if (!done) {
        hostenv_init(oracle::tokens_file().c_str(), ROWS);
        mymap = static_cast<UBYTE *>(std::malloc(ROWS * chcols + GUARD));
        done = true;
    }
    chmap = mymap;
    std::memset(mymap, ' ', ROWS * chcols);
    std::memset(mymap + ROWS * chcols, 0xEE, GUARD);
}

bool guardok()
{
    for (size_t i = 0; i < GUARD; i++)
        if (mymap[ROWS * chcols + i] != 0xEE)
            return false;
    return true;
}

std::string row(int r)
{
    return std::string(reinterpret_cast<const char *>(mymap + r * chcols), chcols);
}

std::string digits(size_t n)
{
    std::string s;

    for (size_t i = 0; i < n; i++)
        s += static_cast<char>('0' + i % 10);
    return s;
}

// the first token pair of text tokenised by tokit2
std::string pairof(const char *text)
{
    static UBYTE in[64], out[64];

    std::memset(in, 0, sizeof(in));
    std::memset(out, 0, sizeof(out));
    std::strcpy(reinterpret_cast<char *>(in), text);
    tokit2(out, in, static_cast<WORD>(std::strlen(text)), nullptr);
    REQUIRE(out[0] >= 0x80);
    return std::string(reinterpret_cast<const char *>(out), 2);
}

std::string drawn;

void draw(UBYTE c)
{
    drawn += static_cast<char>(c);
}

} // namespace

TEST_CASE("the line display ends at column 128", "[fileio][display]")
{
    env();
    tokenise = 0;

    // a line of 129 characters in row 0 (a file the original loaded so,
    // a line of the original's Block Load at a column): row 1 stays blank,
    // its empty line stays empty when it is edited
    const std::string d129 = digits(129), q129(129, 'q');
    const char *const t[] = { d129.c_str(), "", "abc", q129.c_str() };
    mklist(4, t);
    place(0, 0, 0);
    env();
    chmap_showall(nullptr);
    CHECK(row(0) == digits(128));
    CHECK(row(1) == std::string(chcols, ' '));
    CHECK(charstobuff(mymap + chcols) == 1);
    CHECK(row(2) == "abc" + std::string(chcols - 3, ' '));
    // ... and in the last row nothing is written behind the map
    CHECK(row(3) == std::string(chcols, 'q'));
    CHECK(guardok());

    // a redraw of row 0 alone (scrolling): the next row keeps its line
    const char *const t2[] = { d129.c_str(), "abc", "def", "ghi" };
    mklist(4, t2);
    place(0, 0, 0);
    env();
    chmap_showall(nullptr);
    thisitem = firstitem;
    cursy = 0;
    chmap_showtopitem(nullptr);
    CHECK(row(1) == "abc" + std::string(chcols - 3, ' '));
    tokenise = 1;
}

TEST_CASE("a line of keywords ends at column 128", "[fileio][display]")
{
    env();
    REQUIRE(tokens != nullptr);
    tokenise = 1;
    const std::string np = pairof("NPrint");
    std::string l;
    for (int i = 0; i < 64; i++)
        l += np;
    l += "x";                                       // 129 bytes, 385 columns

    // in row 0: the rows below hold only their own lines; the row holds
    // the first 128 columns, the 22nd name cut after its 2nd character,
    // every character with bit 7
    const char *const t[] = { l.c_str(), "b", "c", "d" };
    mklist(4, t);
    place(0, 0, 0);
    env();
    chmap_showall(nullptr);
    std::string want;
    for (int i = 0; i < 22; i++)
        want += "NPrint";
    want.resize(chcols);
    for (auto &ch : want)
        ch = static_cast<char>(static_cast<UBYTE>(ch) | 0x80);
    CHECK(row(0) == want);
    CHECK(row(1) == "b" + std::string(chcols - 1, ' '));
    CHECK(row(2) == "c" + std::string(chcols - 1, ' '));
    CHECK(row(3) == "d" + std::string(chcols - 1, ' '));
    CHECK(guardok());

    // in the last row: nothing behind the map
    const char *const t2[] = { "a", "b", "c", l.c_str() };
    mklist(4, t2);
    place(0, 0, 0);
    env();
    chmap_showall(nullptr);
    CHECK(row(3) == want);
    CHECK(guardok());
}

TEST_CASE("a line of 128 columns is shown as before", "[fileio][display]")
{
    env();
    tokenise = 1;
    const std::string np = pairof("NPrint");

    // 120 characters and a keyword of 6 and 2 characters: 128 columns
    const std::string l = std::string(120, 'y') + np + "ab";
    const char *const t[] = { l.c_str(), "b", "c", "d" };
    mklist(4, t);
    place(0, 0, 0);
    env();
    drawn.clear();
    chmap_showall(draw);
    std::string want = std::string(120, 'y');
    for (char ch : std::string("NPrint"))
        want += static_cast<char>(static_cast<UBYTE>(ch) | 0x80);
    want += "ab";
    CHECK(row(0) == want);
    CHECK(row(1) == "b" + std::string(chcols - 1, ' '));
    // drawn: the first edcols (80) columns of row 0, then the other rows
    CHECK(drawn.substr(0, edcols) == std::string(edcols, 'y'));
    CHECK(drawn.substr(edcols) == "bcd");
    CHECK(guardok());
}
