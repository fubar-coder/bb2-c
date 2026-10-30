// test_services_core - chunk 03 core (ted.s 2458-3036): F1/F2 block
// marks, the label list (makemice/freemice/remember2), a0toname with the
// d0 bug, tokone/tokall/detokallz, help_core, alloc2.
#include <string>
#include <string_view>
#include <snitch/snitch.hpp>
#include "lines_helpers.hpp"
extern "C" {
#include "services.h"
}
#include "oracle.hpp"

namespace {

void env()
{
    static bool done;

    if (!done) {
        hostenv_init(oracle::tokens_file().c_str(), 20);
        done = true;
    }
    tokenise = 1;
}

template <size_t N>
void list(const char *const (&txt)[N])
{
    mklist(N, txt);
    thisitem = firstitem;
}

} // namespace

TEST_CASE("block marks: blockstart, blockend", "[services][blocks]")
{
    static const char *const t[] = { "l0", "l1", "l2", "l3", "l4" };

    env();
    list(t);                            // showall needs a list
    bstx = -1;
    host_resetcalls();
    blockend();
    CHECK(host_calls[HC_BEEPEM] == 1);

    curstop = 0;
    cursoff = 0;
    cursy = 1;
    cursx = 3;
    blockstart();
    CHECK(bstx == 3);
    CHECK(bsty == 1);
    cursy = 2;
    cursx = 5;
    thisitem = firstitem->nextitem->nextitem;
    ev_code = 130;
    blockend();
    CHECK(btopx == 3);
    CHECK(btopy == 1);
    CHECK(bbotx == 4);
    CHECK(bboty == 2);

    // end at column 0: end of the previous line
    cursx = 0;
    blockend();
    CHECK(btopx == 3);
    CHECK(btopy == 1);
    CHECK(bbotx == static_cast<WORD>(chcols - 1));
    CHECK(bboty == 1);

    // end before start: sortblox swaps (the exclusive end becomes the start)
    cursy = 2;
    cursx = 5;
    blockstart();
    cursy = 1;
    cursx = 3;
    thisitem = firstitem->nextitem;
    blockend();
    CHECK(btopx == 2);
    CHECK(btopy == 1);
    CHECK(bbotx == 5);
    CHECK(bboty == 2);

    // start = end: blokdis, btopx keeps the undefined d0 (no effect)
    blockstart();
    blockend();
    CHECK(bboty == static_cast<UWORD>(-1));
    CHECK(bstx == -1);
    CHECK(btopx == 130);
}

TEST_CASE("label list: makemice, freemice, remember, mforget2", "[services][mice]")
{
    static const char *const t[] = { ".a", "x", "  .b:", "c", ".d" };

    env();
    list(t);
    nummice = 0;
    mousealt = 0;
    makemice();
    CHECK(nummice == 3);
    CHECK(mousealt == '.');
    struct micenode *m = mice;
    REQUIRE(m != nullptr);
    CHECK(m->line == firstitem);
    CHECK(m->next->line == firstitem->nextitem->nextitem);
    CHECK(m->next->next->line == enditem);
    freemice();
    CHECK(nummice == 0);
    CHECK(mousealt == 3);
    remember(nullptr);
    CHECK(mousealt == static_cast<UWORD>(-1));
    mousealt = 0;
    mforget2();
    CHECK(mousealt == static_cast<UWORD>(-1));
}

TEST_CASE("a0toname", "[services][names]")
{
    env();
    host_resetcalls();
    int hazards = host_hazards;
    a0toname(reinterpret_cast<const UBYTE *>("\"dh0:foo/bar\""), 0x12345600);
    CHECK(std::string_view(filepath) == "");
    CHECK(std::string_view(filename) == "dh0:foo/bar");

    // d0 low byte '/': the last character becomes the name
    a0toname(reinterpret_cast<const UBYTE *>("abc"), '/');
    CHECK(std::string_view(filepath) == "ab");
    CHECK(std::string_view(filename) == "c");

    // d0 low byte ':': empty name, -2(a1) read in front of filename
    a0toname(reinterpret_cast<const UBYTE *>("dh0:x"), 0x3A);
    CHECK(std::string_view(filepath) == "dh0:x");
    CHECK(std::string_view(filename) == "");
    CHECK(host_hazards == hazards + 1);
}

TEST_CASE("tokone, help_core, tokall, detokallz", "[services][tokit]")
{
    env();
    REQUIRE(tokens != nullptr);
    // the first token of the list (its name tokenises to itself)
    const char *name = tokens->name;
    std::string src = std::string(name) + " x";
    UBYTE line[200], ref[200];
    std::strcpy(reinterpret_cast<char *>(line), src.c_str());
    UWORD d5 = tokone(line);
    UWORD ref5 = tokit2(ref, reinterpret_cast<const UBYTE *>(src.c_str()),
                        static_cast<WORD>(src.size()), nullptr);
    CHECK(d5 == ref5);
    CHECK(std::memcmp(line, ref, d5) == 0);
    CHECK((line[0] & 0x80) != 0);

    // help_core: the token at column 0
    const char *t1[] = { src.c_str() };
    list(t1);
    struct item *it = firstitem;
    std::memcpy(it->chars, line, d5);   // same size or shorter
    it->numchars = static_cast<UBYTE>(d5);
    UWORD num = 0;
    const char *help = help_core(it, 0, &num);
    REQUIRE(help != nullptr);
    CHECK(std::string_view(help) == name);
    CHECK((num & 0x7FFF) == tokens->num);
    CHECK((num & 0x8000) != 0);
    // the help text follows the name (may be empty)
    CHECK(help + std::strlen(help) + 1 == tokens->name + std::strlen(tokens->name) + 1);
    // a column behind the token: still that token (I.13)
    CHECK(help_core(it, static_cast<UWORD>(std::strlen(name) + 1), &num) == help);

    // no token at all
    static const char *const t2[] = { "abc" };
    list(t2);
    num = 7;
    CHECK(help_core(firstitem, 2, &num) == nullptr);
    CHECK(num == 7);

    // tokall / detokallz round trip
    std::string txt = std::string("a=") + name;
    const char *t3[] = { "plain", txt.c_str(), "" };
    list(t3);
    curstop = 0;
    cursy = 0;
    tokenise = 0;
    host_resetcalls();
    tokall();
    CHECK(tokenise == 0xFFFF);
    CHECK(numlines == 3);
    CHECK((firstitem->nextitem->chars[2] & 0x80) != 0);
    CHECK(host_calls[HC_SHOWALL] == 1);
    CHECK(thisitem == firstitem);
    tokall();
    CHECK(host_calls[HC_BEEPEM] == 1);
    detokallz();
    CHECK(tokenise == 0);
    CHECK(std::string_view(chars(firstitem->nextitem)) == txt);
    CHECK(std::string_view(chars(firstitem)) == "plain");
    CHECK(thisitem == enditem);         // no calcitem (I.14)
    detokallz();
    CHECK(host_calls[HC_BEEPEM] == 2);
}

TEST_CASE("alloc2", "[services]")
{
    env();
    APTR p = alloc2(16, TED_MEMF_PUBLIC);
    CHECK(p != nullptr);
    CHECK(memalt == static_cast<UWORD>(-1));
    ted_free(p, 16);
}
