// test_lines_edit - line list operations of chunk 02 (ted.s 1041-1806):
// cursor up/down, insertline/deleteline/return, gotolined0 arithmetic,
// bloksave/blokload through the file hooks.  tokenise = 0.
#include <string>
#include <string_view>
#include <snitch/snitch.hpp>
#include "lines_helpers.hpp"
#include "tempdir.hpp"

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
    fontmul = 8;
    qualifier = 0;
}

} // namespace

TEST_CASE("cursor up and down", "[lines][cursor]")
{
    env();
    mklist(50, nullptr);
    place(0, 0, 0);
    cursdown();
    CHECK(cursy == 1);
    CHECK(curstop == 0);
    CHECK(std::string_view(chars(thisitem)) == "line1");

    // scrolling down at the margin: cursy + 1 + vscmarg >= edrows
    place(0, 13, 0);
    host_resetcalls();
    cursdown();
    CHECK(curstop == 1);
    CHECK(cursy == 13);
    CHECK(host_calls[HC_SCRUPALL] == 1);
    CHECK(std::string_view(chars(thisitem)) == "line14");

    // last line: beep
    place(30, 19, 0);
    host_resetcalls();
    cursdown();
    CHECK(host_calls[HC_BEEPEM] == 1);
    CHECK(cursy == 19);

    // up at the top margin with curstop != 0: scroll
    place(5, 6, 0);
    host_resetcalls();
    cursup();
    CHECK(curstop == 4);
    CHECK(cursy == 6);
    CHECK(host_calls[HC_SCRDOWNALL] == 1);
    place(5, 7, 0);
    cursup();
    CHECK(curstop == 5);
    CHECK(cursy == 6);
    place(0, 0, 0);
    host_resetcalls();
    cursup();
    CHECK(host_calls[HC_BEEPEM] == 1);

    // Shift: page down/up
    qualifier = 1;
    place(0, 3, 0);
    cursdown();
    CHECK(curstop + cursy == 23);
    place(25, 10, 0);
    cursdown();
    CHECK(curstop + cursy == 49);       // min(numlines - 1)
    place(0, 10, 0);
    cursup();
    CHECK(curstop + cursy == 0);
    place(20, 10, 0);
    cursup();
    CHECK(curstop + cursy == 10);
    qualifier = 0;
}

TEST_CASE("gotolined0, top, bottom", "[lines][cursor]")
{
    env();
    mklist(50, nullptr);
    place(0, 0, 5);
    host_resetcalls();
    CHECK(gotolined0(30) == thisitem);
    CHECK(curstop == 24);
    CHECK(cursy == 6);
    CHECK(cursx == 0);
    CHECK(std::string_view(chars(thisitem)) == "line30");
    CHECK(host_calls[HC_SHOWALL] == 1);
    CHECK(host_calls[HC_SHOWHPROP] == 1);
    gotolined0(45);
    CHECK(curstop == 30);
    CHECK(cursy == 15);
    host_resetcalls();
    gotolined02(44);                    // same curstop: no showall, no hprop
    CHECK(curstop == 30);
    CHECK(cursy == 14);
    CHECK(host_calls[HC_SHOWALL] == 0);
    CHECK(host_calls[HC_SHOWHPROP] == 0);
    gotolined0(3);
    CHECK(curstop == 0);
    CHECK(cursy == 3);
    top();
    CHECK(curstop + cursy == 0);
    CHECK(thisitem == firstitem);
    bottom();
    CHECK(curstop + cursy == 49);
    CHECK(thisitem == enditem);

    // fewer lines than rows
    mklist(5, nullptr);
    place(0, 0, 0);
    gotolined0(4);
    CHECK(curstop == 0);
    CHECK(cursy == 4);
}

TEST_CASE("insertline, deleteline, return", "[lines]")
{
    env();
    mklist(3, nullptr);
    place(0, 1, 0);
    insertline();
    CHECK(numlines == 4);
    CHECK(listok());
    CHECK(std::string_view(linetext(1)) == "");
    CHECK(thisitem == firstitem->nextitem);
    CHECK(allalt == (UWORD)-1);

    place(0, 0, 0);
    insertline();                       // before the first line
    CHECK(numlines == 5);
    CHECK(listok());
    CHECK(thisitem == firstitem);

    // deleteline: middle, last, only
    mklist(3, nullptr);
    place(0, 1, 0);
    deleteline();
    CHECK(numlines == 2);
    CHECK(listok());
    CHECK(std::string_view(chars(thisitem)) == "line2");
    place(0, 1, 0);
    deleteline();                       // freelast
    CHECK(numlines == 1);
    CHECK(listok());
    CHECK(thisitem == firstitem);
    CHECK(cursy == 0);
    place(0, 0, 0);
    deleteline();                       // freeonly
    CHECK(numlines == 1);
    CHECK(listok());
    CHECK(firstitem->chars[0] == 0);
    CHECK(firstitem == thisitem);
    CHECK(firstitem == enditem);

    // return: new line after a4, cursx = 0, cursoff reset
    mklist(3, nullptr);
    place(0, 0, 7);
    cursoff = 4;
    host_resetcalls();
    return_();
    CHECK(numlines == 4);
    CHECK(listok());
    CHECK(std::string_view(linetext(1)) == "");
    CHECK(cursx == 0);
    CHECK(cursoff == 0);
    CHECK(host_calls[HC_REFRESH] == 1);
    CHECK(cursy == 1);
    CHECK(thisitem == firstitem->nextitem);
    mklist(3, nullptr);
    place(0, 2, 0);
    return_();
    CHECK(numlines == 4);
    CHECK(listok());
    CHECK(thisitem == enditem);

    // return at the bottom margin: curstop++, scrupall with edrows
    // temporarily cursy + 1, restored
    mklist(30, nullptr);
    place(0, 13, 0);
    host_resetcalls();
    return_();
    CHECK(curstop == 1);
    CHECK(cursy == 13);
    CHECK(edrows == 20);
    CHECK(host_calls[HC_SCRUPALL] == 1);
    CHECK(host_calls[HC_SCROLLDOWN] == 0);
}

TEST_CASE("bloksave and blokload through the file hooks", "[lines][blocks][files]")
{
    static const char *const t[] = { "abcdef", "ghij", "klm" };
    tempdir dir;

    env();
    REQUIRE(!dir.path.empty());
    std::string path = dir.path + "/test_lines_edit.blk";
    host_bname = reinterpret_cast<UBYTE *>(path.data());

    mklist(3, t);
    place(0, 0, 0);
    btopx = 1;
    btopy = 0;
    bbotx = 2;
    bboty = 1;
    bloksave();
    long len = 0;
    UBYTE *b = host_readfile(path.c_str(), &len);
    REQUIRE(b != nullptr);
    CHECK(len == 9);
    CHECK(std::memcmp(b, "bcdef\nghi", 9) == 0);
    std::free(b);
    CHECK(filehand == 0);

    // blokload "bcdef\nghi" (last line without LF) at line 2, column 1
    mklist(3, t);
    place(0, 2, 1);
    bboty = 1;
    blokload();
    CHECK(numlines == 4);
    CHECK(listok());
    CHECK(std::string_view(linetext(2)) == "kbcdef");
    CHECK(std::string_view(linetext(3)) == "ghilm");
    CHECK(bboty == (UWORD)-1);

    // a file ending with LF: an extra line for the rest
    FILE *f = std::fopen(path.c_str(), "wb");
    REQUIRE(f != nullptr);
    std::fputs("XX\nYY\n", f);
    std::fclose(f);
    mklist(3, t);
    place(0, 0, 2);
    blokload();
    CHECK(numlines == 5);
    CHECK(listok());
    CHECK(std::string_view(linetext(0)) == "abXX");
    CHECK(std::string_view(linetext(1)) == "YY");
    CHECK(std::string_view(linetext(2)) == "cdef");
    CHECK(std::string_view(linetext(3)) == "ghij");

    // bytes >= $80 (Latin-1) are characters, not the block end (#362:
    // getbchar2 returned them negative without tokenising)
    static const char *const t2[] = { "x", "a$=\"Gr\xf6\xdf" "e\"", "b" };
    mklist(3, t2);
    place(0, 0, 0);
    btopx = 0;
    btopy = 0;
    bbotx = static_cast<WORD>(chcols - 1);
    bboty = 1;
    bloksave();
    b = host_readfile(path.c_str(), &len);
    REQUIRE(b != nullptr);
    CHECK(std::string_view(reinterpret_cast<const char *>(b), static_cast<size_t>(len))
          == "x\na$=\"Gr\xf6\xdf" "e\"\n");
    std::free(b);

    // cancelled requester
    host_bname = nullptr;
    host_resetcalls();
    bboty = 0;
    bloksave();
    CHECK(host_calls[HC_GETBNAME] == 1);
    CHECK(host_calls[HC_ERROR] == 0);
    std::remove(path.c_str());
}
