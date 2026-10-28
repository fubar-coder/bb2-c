// test_search - chunk 01 core (src/core/loadsave.c): search/replace
// (dosearch, next, previous, replace, replace2), blocktab/blockuntab,
// the printer line format, the .xtra/.info names and realsave's .info,
// plus the keymap data parts (rawspecial, germs_map, fontcalc).
// Expected values derived from the asm (ted.s 327-399, 570-676, 769-825,
// 926-1014; rawed.obj.dsm; ted.s 5214, 5976).
#include <csetjmp>
#include <cstdlib>
#include <string>
#include <string_view>
#include <snitch/snitch.hpp>
#include "lines_helpers.hpp"
extern "C" {
#include "loadsave.h"
#include "keymap.h"
}
#include "oracle.hpp"
#include "tempdir.hpp"

namespace {

void env()
{
    static bool done;

    if (!done) {
        hostenv_init(oracle::tokens_file().c_str(), 25);
        done = true;
    }
    tokenise = 0;
}

// the list, cursor at line 0 column 0, thisitem = first line
template <size_t N>
void list(const char *const (&txt)[N])
{
    mklist(N, txt);
    curstop = cursy = cursx = 0;
    cursoff = 0;
    calcpos();
    thisitem = firstitem;
}

struct item *nth(int i)
{
    struct item *it = firstitem;

    while (i-- > 0 && it)
        it = it->nextitem;
    return it;
}

void search(const char *find, const char *repl)
{
    std::strcpy(reinterpret_cast<char *>(findreqla16), find);
    std::strcpy(reinterpret_cast<char *>(findreqla17), repl);
}

std::string_view text(const struct item *it)
{
    return chars(it);
}

// an outmem would longjmp into nowhere: fail the test case instead
struct no_outmem {
    std::jmp_buf jb;

    no_outmem() { host_outmem = &jb; }
    ~no_outmem() { host_outmem = nullptr; }
};

} // namespace

TEST_CASE("replace2 and replace", "[search]")
{
    static const char *const l1[] = { "abc abc", "xyz", "aXa" };
    struct item *a4;
    no_outmem om;

    env();
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    host_findcase = FALSE;
    list(l1);
    search("abc", "x");
    allalt = 0;
    CHECK(replace2(thisitem, &a4));
    CHECK(text(a4) == "x x");
    CHECK(a4->numchars == 4);
    CHECK(a4 == thisitem);
    CHECK(a4 == firstitem);
    CHECK(allalt == 0xffff);

    // nothing found: Z=0, list unchanged
    thisitem = nth(1);
    CHECK(!replace2(thisitem, &a4));
    CHECK(a4 == nth(1));

    // replacement containing the search text: continues behind it
    search("a", "aa");
    thisitem = nth(2);
    CHECK(replace2(thisitem, &a4));
    CHECK(text(a4) == "aaXaa");

    // case insensitive unless findcase(): "ABC" matches "abc"
    static const char *const l2[] = { "ABC Abc" };
    list(l2);
    search("abc", "1");
    CHECK(replace2(thisitem, &a4));
    CHECK(text(a4) == "1 1");
    list(l2);
    host_findcase = TRUE;
    CHECK(!replace2(thisitem, &a4));
    host_findcase = FALSE;

    // replace (menu): empty search text -> beep; nothing -> beep
    list(l1);
    search("", "q");
    host_resetcalls();
    replace();
    CHECK(host_calls[HC_BEEPEM] == 1);
    search("nope", "q");
    replace();
    CHECK(host_calls[HC_BEEPEM] == 2);
    CHECK(host_calls[HC_SHOWALL] == 0);
    search("xyz", "q");
    thisitem = nth(1);
    replace();
    CHECK(host_calls[HC_SHOWALL] == 1);
    CHECK(text(nth(1)) == "q");
}

TEST_CASE("next and previous", "[search]")
{
    static const char *const l[] = { "one", "two", "three", "four two", "five" };
    no_outmem om;

    env();
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    list(l);
    search("two", "");
    // next from line 0 finds line 1 (never the rest of the current line)
    host_resetcalls();
    next();
    CHECK(host_calls[HC_BEEPEM] == 0);
    CHECK(host_calls[HC_POINTERON] == 1);
    CHECK(thisitem == nth(1));
    CHECK(curstop + cursy == 1);
    CHECK(cursoff + cursx == 0);
    next();
    CHECK(thisitem == nth(3));
    CHECK(curstop + cursy == 3);
    CHECK(cursx == 5);
    // nothing more: beep, cursor stays
    host_resetcalls();
    next();
    CHECK(host_calls[HC_BEEPEM] == 1);
    CHECK(thisitem == nth(3));
    // previous from line 3 finds line 1
    host_resetcalls();
    previous();
    CHECK(host_calls[HC_BEEPEM] == 0);
    CHECK(thisitem == nth(1));
    // previous at line 0: beep
    curstop = cursy = 0;
    thisitem = firstitem;
    host_resetcalls();
    previous();
    CHECK(host_calls[HC_BEEPEM] == 1);
    // next on the last line: beep
    thisitem = enditem;
    host_resetcalls();
    next();
    CHECK(host_calls[HC_BEEPEM] == 1);
}

TEST_CASE("blocktab and blockuntab", "[search][blocks]")
{
    static const char *const l[] = { "a", "    b", "c", "xy" };
    no_outmem om;

    env();
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    tabstop = 2;
    list(l);
    btopx = 0;
    btopy = 0;
    bbotx = 0;
    bboty = 2;
    blocktab();
    CHECK(text(nth(0)) == "  a");
    // BUG(orig): the 0 byte of the copied ASCII line stays in the chmap
    // style buffer - every tabbed line gets one more 0 byte
    CHECK(nth(0)->numchars == 5);
    CHECK(text(nth(1)) == "      b");
    CHECK(nth(1)->numchars == 9);
    CHECK(text(nth(3)) == "xy");

    list(l);
    bboty = 3;
    blockuntab();
    // line shorter than tabstop: everything becomes spaces (BUG(orig))
    CHECK(text(nth(0)) == "");
    CHECK(nth(0)->numchars == 1);
    CHECK(text(nth(1)) == "  b");
    CHECK(nth(1)->numchars == 4);
    CHECK(text(nth(3)) == "");
    // tabstop 0: the whole line is cleared (survey-01 I.11)
    list(l);
    tabstop = 0;
    blockuntab();
    CHECK(text(nth(1)) == "");
    tabstop = 2;

    // no block: beep
    bboty = static_cast<UWORD>(-1);
    host_resetcalls();
    blocktab();
    blockuntab();
    CHECK(host_calls[HC_BEEPEM] == 2);
}

TEST_CASE("printer line format", "[search][print]")
{
    static const char *const l[] = { "hi" };
    UBYTE tok[64];

    env();
    list(l);
    UWORD n = prtfile_line(firstitem, charbuff);
    CHECK(n == 3);
    CHECK(std::memcmp(charbuff, "hi\n", 3) == 0);

    // a token -> ESC[1m name ESC[22m
    REQUIRE(tokens != nullptr);
    std::memset(charbuff2, ' ', chcols);
    std::memcpy(charbuff2, "Print 1", 7);
    n = tokit2(tok, charbuff2, 7, nullptr);
    auto *it = static_cast<struct item *>(ted_alloc(n + lenitem, TED_MEMF_PUBLIC));
    it->numchars = static_cast<UBYTE>(n);
    std::memcpy(it->chars, tok, n);
    CHECK((it->chars[0] & 0x80) != 0);
    n = prtfile_line(it, charbuff);
    CHECK(n == 4 + 5 + 5 + 2 + 1);
    CHECK(std::memcmp(charbuff, "\033[1mPrint\033[22m 1\n", n) == 0);
    ted_free(it, it->numchars + lenitem);
}

TEST_CASE(".xtra name and realsave's .info", "[search][files]")
{
    static const char *const l[] = { "line", "two" };
    UBYTE buf[1240];
    long len;
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    std::strcpy(reinterpret_cast<char *>(buf), "file");
    xtraat = buf;
    CHECK(openmore(xtratxt) == buf);
    CHECK(std::string_view(reinterpret_cast<char *>(buf)) == "file.xtra");
    closextra();
    CHECK(std::string_view(reinterpret_cast<char *>(buf)) == "file");

    // realsave: ASCII lines with LF, .info written once (never replaced)
    list(l);
    std::string name = dir.path + "/test_search.out", info = name + ".info";
    std::strcpy(reinterpret_cast<char *>(buf), name.c_str());
    xtraat = buf;                       // filesave sets it; realsave does not
    host_icongad = TRUE;
    allalt = 1;
    host_resetcalls();
    realsave(buf);
    CHECK(allalt == 0);
    CHECK(host_calls[HC_SHOWFNAME] == 1);
    CHECK(std::string_view(reinterpret_cast<char *>(buf)) == name);    // extension removed again
    UBYTE *b = host_readfile(name.c_str(), &len);
    REQUIRE(b != nullptr);
    CHECK(len == 9);
    CHECK(std::memcmp(b, "line\ntwo\n", 9) == 0);
    std::free(b);
    b = host_readfile(info.c_str(), &len);
    REQUIRE(b != nullptr);
    CHECK(len == static_cast<long>(sizeof(srcinfo)));
    CHECK(std::memcmp(b, srcinfo, sizeof(srcinfo)) == 0);
    std::free(b);
    // existing icon: not overwritten
    FILE *f = std::fopen(info.c_str(), "wb");
    REQUIRE(f != nullptr);
    std::fputs("x", f);
    std::fclose(f);
    realsave(buf);
    b = host_readfile(info.c_str(), &len);
    REQUIRE(b != nullptr);
    CHECK(len == 1);
    std::free(b);
    host_icongad = FALSE;
    std::remove(name.c_str());
    std::remove(info.c_str());
}

TEST_CASE("keymap data: rawspecial, germs_map, fontcalc", "[keymap]")
{
    env();
    int bad = 0;
    for (UWORD c = 0; c < 0x200; c++) {
        UWORD want = 0;

        if (c >= 0x50 && c <= 0x59)
            want = c + 0x31;
        else if (c >= 0x4c && c <= 0x4f)
            want = c - 0x30;
        else if (c == 0x5f)
            want = 0x8b;
        if (rawspecial(c) != want && bad++ < 5) {
            CAPTURE(c, want);
            FAIL_CHECK("rawspecial");
        }
    }
    CHECK(bad == 0);
    CHECK(germs_map(228) == (26 | (1UL << 30)));
    CHECK(germs_map(220) == (31 | (1UL << 30)));
    CHECK(germs_map('a') == 'a');
    CHECK(germs_map(26) == 26);
    fonthite = 8;
    CHECK(fontcalc(0) == font8);
    CHECK(fontcalc(0x7f) == font8 + 15 + 7 * 128);
    fonthite = 6;
    CHECK(fontcalc(0x21) == font6 + 1 + 2 * 96);
    fonthite = 8;
}
