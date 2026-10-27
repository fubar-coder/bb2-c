// test_lines_blocks - block coordinate rules of chunk 02 (survey-02 H,
// derived from ted.s 1153-1974) as table tests: blokadj through
// insertline/return, deleteline, join, split, and blokcopy's block move.
// No token list: tokenise = 0, chmap rows written by the test.
#include <string_view>
#include <snitch/snitch.hpp>
#include "lines_helpers.hpp"

namespace {

// the environment, set up once per program (the core keeps its globals)
void env()
{
    static bool done;

    if (!done) {
        hostenv_init(nullptr, 20);
        done = true;
    }
    tokenise = 0;
}

// cursor on line `line` (curstop 0), column col
void cursorto(UWORD line, UWORD col)
{
    place(0, line, col);
}

void setblock(WORD tx, UWORD ty, WORD bx, UWORD by)
{
    btopx = tx;
    btopy = ty;
    bbotx = bx;
    bboty = by;
}

bool block(WORD tx, int ty, WORD bx, int by)
{
    return btopx == tx && btopy == static_cast<UWORD>(ty) && bbotx == bx
           && bboty == static_cast<UWORD>(by);
}

// ------------------------------------------------------------ tables

struct blkcase {
    UWORD line, col;
    WORD tx; UWORD ty; WORD bx; UWORD by;       // before
    WORD etx; UWORD ety; WORD ebx; UWORD eby;   // expected after
};

template <size_t N>
void run(const char *name, void (*fn)(void), const blkcase (&c)[N], int lines)
{
    for (size_t i = 0; i < N; i++) {
        mklist(lines, nullptr);
        cursorto(c[i].line, c[i].col);
        setblock(c[i].tx, c[i].ty, c[i].bx, c[i].by);
        qualifier = 0;
        fn();
        CAPTURE(name, i, btopx, btopy, bbotx, bboty);
        CHECK(block(c[i].etx, c[i].ety, c[i].ebx, c[i].eby));
    }
}

// insertline: blokadj(cur)
const blkcase ins[] = {
    { 3, 0, 1, 3, 2, 5,   1, 4, 2, 6 },         // cur = btopy: both +1
    { 5, 0, 1, 3, 2, 5,   1, 3, 2, 6 },         // cur = bboty: bboty +1
    { 6, 0, 1, 3, 2, 5,   1, 3, 2, 5 },         // behind: nothing
    { 1, 0, 1, 3, 2, (UWORD)-1,  1, 3, 2, (UWORD)-1 },  // no block
};
// return: blokadj(cur + 1)
const blkcase ret[] = {
    { 2, 0, 1, 3, 2, 5,   1, 4, 2, 6 },
    { 4, 0, 1, 3, 2, 5,   1, 3, 2, 6 },
    { 5, 0, 1, 3, 2, 5,   1, 3, 2, 5 },
};
// deleteline
const blkcase del[] = {
    { 1, 0, 1, 3, 2, 5,   1, 2, 2, 4 },         // before: both -1
    { 4, 0, 1, 3, 2, 5,   1, 3, 2, 4 },         // inside: bboty -1
    { 3, 0, 1, 3, 2, 5,   1, 3, 2, 4 },         // first line: bboty -1
    { 3, 0, 1, 3, 4, 3,   1, 2, 4, (UWORD)-1 }, // one line block: bboty = -1
                                                // without blokdis (I.6)
    { 7, 0, 1, 3, 2, 5,   1, 3, 2, 5 },         // behind: nothing
};
// join: line `line` has the text "lineN" (length 5)
const blkcase jn[] = {
    { 1, 0, 1, 3, 2, 5,   1, 2, 2, 4 },         // next line before block
    { 2, 0, 2, 3, 5, 5,   7, 2, 5, 4 },         // next = btopy < bboty: btopx += 5
    { 2, 0, 1, 3, 2, 3,   6, 2, 7, 2 },         // next = btopy = bboty: both x += 5
    { 4, 0, 1, 3, 2, 5,   1, 3, 7, 4 },         // next = bboty: bbotx += 5
    { 3, 0, 1, 2, 2, 5,   1, 2, 2, 4 },         // inside
    { 5, 0, 1, 3, 2, 5,   1, 3, 2, 5 },         // cur = bboty: nothing
};
// split at column col
const blkcase sp[] = {
    { 7, 2, 1, 3, 2, 5,   1, 3, 2, 5 },         // behind
    { 1, 2, 1, 3, 2, 5,   1, 4, 2, 6 },         // before
    { 4, 2, 1, 3, 2, 5,   1, 3, 2, 6 },         // inside
    { 3, 2, 5, 3, 2, 5,   3, 4, 2, 6 },         // top line, col <= btopx
    { 3, 7, 5, 3, 2, 5,   5, 3, 2, 6 },         // top line, col > btopx
    { 5, 2, 1, 3, 6, 5,   1, 3, 4, 6 },         // bottom line, col <= bbotx
    { 5, 8, 1, 3, 6, 5,   1, 3, 6, 5 },         // bottom line, col > bbotx
    { 3, 10, 4, 3, 8, 3,  4, 3, 8, 3 },         // one line, col > bbotx
    { 3, 6, 4, 3, 8, 3,   4, 3, 2, 4 },         // one line, btopx < col <= bbotx
    { 3, 2, 4, 3, 8, 3,   2, 4, 6, 4 },         // one line, col <= btopx
};

} // namespace

TEST_CASE("block coordinates: insertline, return, deleteline, join, split", "[lines][blocks]")
{
    env();
    run("insertline", insertline, ins, 10);
    run("return", return_, ret, 10);
    run("deleteline", deleteline, del, 10);
    run("join", join, jn, 10);
    run("split", split, sp, 10);
}

TEST_CASE("join and split: line contents", "[lines]")
{
    static const char *const t[] = { "abc", "defg", "hi" };

    env();
    // join: "abc" + "defg", FreeMem with the wrong size (BUG(orig))
    mklist(3, t);
    cursorto(0, 0);
    bboty = (UWORD)-1;
    host_resetcalls();
    join();
    CHECK(numlines == 2);
    CHECK(std::string_view(linetext(0)) == "abcdefg");
    CHECK(std::string_view(linetext(1)) == "hi");
    CHECK(firstitem->nextitem == enditem);
    CHECK(enditem->lastitem == firstitem);
    CHECK(host_hazards == 1);           // FreeMem(numchars) instead of +lenitem
    CHECK(memalt == 5);                 // numchars of "defg"
    CHECK(host_calls[HC_SHOWALL] == 1);

    // join on the last line: beep
    mklist(3, t);
    cursorto(2, 0);
    host_resetcalls();
    join();
    CHECK(host_calls[HC_BEEPEM] == 1);
    CHECK(numlines == 3);

    // split "defg" at column 1
    mklist(3, t);
    cursorto(1, 1);
    bboty = (UWORD)-1;
    qualifier = 0;
    split();
    CHECK(numlines == 4);
    CHECK(std::string_view(linetext(1)) == "d");
    CHECK(std::string_view(linetext(2)) == "efg");
    CHECK(std::string_view(linetext(3)) == "hi");
    CHECK(chmap[chloc + 1] == ' ');
    CHECK(chmap[chloc] == 'd');
    CHECK(thisitem == firstitem->nextitem);

    // split the last line: enditem
    mklist(3, t);
    cursorto(2, 1);
    split();
    CHECK(numlines == 4);
    CHECK(std::string_view(chars(enditem)) == "i");
    CHECK(enditem->nextitem == nullptr);
    CHECK(enditem->lastitem == thisitem);
}

TEST_CASE("blokcopy: content and block move", "[blocks]")
{
    static const char *const t1[] = { "abcdef", "x", "xyz" };
    static const char *const t2[] = { "abcdef", "ghijk", "lmn", "xyz" };
    static const char *const t3[] = { "q", "r", "abcdefgh" };

    env();
    // one line block copied to another line
    mklist(3, t1);
    cursorto(2, 0);
    setblock(1, 0, 2, 0);
    blokcopy();
    CHECK(numlines == 3);
    CHECK(std::string_view(linetext(2)) == "bcxyz");
    CHECK(block(1, 0, 2, 0));

    // two line block copied into the middle of a line
    mklist(4, t2);
    cursorto(3, 1);
    setblock(2, 0, 1, 1);
    blokcopy();
    CHECK(numlines == 5);
    CHECK(std::string_view(linetext(3)) == "xcdef");
    CHECK(std::string_view(linetext(4)) == "ghyz");
    CHECK(block(2, 0, 1, 1));

    // copy in front of the block on its own line: block moves right
    // (.shit5: both x + width)
    mklist(3, t3);
    cursorto(2, 1);
    setblock(3, 2, 4, 2);
    blokcopy();
    CHECK(std::string_view(linetext(2)) == "adebcdefgh");
    CHECK(block(5, 2, 6, 2));

    // cursor inside the block: beep, nothing changed
    mklist(3, t3);
    cursorto(2, 4);
    setblock(3, 2, 4, 2);
    host_resetcalls();
    blokcopy();
    CHECK(host_calls[HC_BEEPEM] == 1);
    CHECK(std::string_view(linetext(2)) == "abcdefgh");

    // blokcopy2 = blokcopy + showvprop
    mklist(3, t1);
    cursorto(2, 0);
    setblock(1, 0, 2, 0);
    host_resetcalls();
    blokcopy2();
    CHECK(host_calls[HC_SHOWVPROP] == 1);
}

TEST_CASE("blokkill and blokforget", "[blocks]")
{
    static const char *const t[] = { "abcdef", "ghijkl", "mnop", "rest" };

    env();
    // three line block
    mklist(4, t);
    cursorto(0, 0);
    setblock(2, 0, 1, 2);
    host_resetcalls();
    blokkill();
    CHECK(numlines == 2);
    CHECK(std::string_view(linetext(0)) == "abop");
    CHECK(std::string_view(linetext(1)) == "rest");
    CHECK(bboty == (UWORD)-1);
    CHECK(bstx == -1);
    CHECK(host_hazards == 1);           // detoka5 without tokenise (I.8)
    CHECK(thisitem == firstitem);

    // whole lines (btopx = 0, bbotx = chcols-1)
    mklist(4, t);
    cursorto(3, 0);
    setblock(0, 1, static_cast<WORD>(chcols - 1), 2);
    blokkill();
    CHECK(numlines == 2);
    CHECK(std::string_view(linetext(0)) == "abcdef");
    CHECK(std::string_view(linetext(1)) == "rest");
    // cursor line 3 >= numlines: bottom line, gotolined02
    CHECK(curstop + cursy == 1);
    CHECK(thisitem == enditem);

    // one line part
    mklist(4, t);
    cursorto(1, 0);
    setblock(1, 1, 3, 1);
    blokkill();
    CHECK(numlines == 4);
    CHECK(std::string_view(linetext(1)) == "gkl");

    // no block: beep
    host_resetcalls();
    blokkill();
    CHECK(host_calls[HC_BEEPEM] == 1);

    // blokforget
    setblock(1, 1, 3, 1);
    host_resetcalls();
    blokforget();
    CHECK(bboty == (UWORD)-1);
    CHECK(host_calls[HC_SHOWALL] == 1);
}

TEST_CASE("markindent", "[blocks]")
{
    static const char *const t[] = { "top", "  a", "", "    b", "  c", "d" };

    env();
    mklist(6, t);
    cursorto(1, 0);
    markindent();
    CHECK(block(0, 1, static_cast<WORD>(chcols - 1), 4));
    cursorto(2, 0);
    host_resetcalls();
    markindent();                       // empty line: beep
    CHECK(host_calls[HC_BEEPEM] == 1);
}
