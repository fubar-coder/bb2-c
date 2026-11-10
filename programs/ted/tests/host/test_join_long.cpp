// test_join_long - Join (ted.s:1229, core/lines.c) of two lines whose
// text is longer than chcols together: refused with the beep before
// anything changes (#1005), with tokenise off and on (the next line's
// text length is that of its detokenised text).  Token list of the
// oracle (tests/data/ted-tokens.txt).
#include <string>
#include <string_view>
#include <snitch/snitch.hpp>
#include "lines_helpers.hpp"
#include "oracle.hpp"

namespace {

// the environment, set up once per program (the core keeps its globals)
void env()
{
    static bool done;

    if (!done) {
        hostenv_init(oracle::tokens_file().c_str(), 20);
        done = true;
    }
    tokenise = 0;
    bboty = static_cast<UWORD>(-1);
    bstx = -1;
}

// the text of line i (detokenised with tokenise on)
std::string text(int i)
{
    struct item *it = firstitem;

    while (i-- > 0 && it)
        it = it->nextitem;
    if (!it)
        return "<none>";
    if (!tokenise)
        return chars(it);
    detok(it->chars, charbuff, charbuff4);
    return reinterpret_cast<const char *>(charbuff);
}

// line 0 = prefix, line 1 = next (both as the list holds them), line 2
// "z"; cursor on line 0, the chmap row is the text of line 0
void lines(const std::string &prefix, const std::string &next)
{
    const char *const t[] = { prefix.c_str(), next.c_str(), "z" };

    mklist(3, t);
    place(0, 0, 0);
}

// line 1 tokenised: "NPrint a" (5 bytes, 8 characters as text)
std::string nprint()
{
    UBYTE in[16] = "NPrint a", out[16];

    tokit2(out, in, 8, nullptr);
    REQUIRE((out[0] & 0x80) != 0);
    return reinterpret_cast<const char *>(out);
}

} // namespace

TEST_CASE("join: a result of chcols characters fits", "[lines][join]")
{
    env();
    // tokenise off: 121 + 7 = 128
    lines(std::string(121, 'x'), "a=1:b=2");
    host_resetcalls();
    join();
    CHECK(host_calls[HC_BEEPEM] == 0);
    CHECK(numlines == 2);
    CHECK(text(0) == std::string(121, 'x') + "a=1:b=2");
    CHECK(text(1) == "z");

    // tokenise on: 120 + 8 ("NPrint a" as text, 5 bytes tokenised) = 128
    tokenise = 1;
    lines(std::string(119, 'x') + ":", nprint());
    host_resetcalls();
    join();
    CHECK(host_calls[HC_BEEPEM] == 0);
    CHECK(numlines == 2);
    CHECK(text(0) == std::string(119, 'x') + ":NPrint a");
    tokenise = 0;
}

TEST_CASE("join: a longer result is refused with the beep (#1005)", "[lines][join]")
{
    env();
    // Fixed (#1005): the original joined and dropped everything behind
    // column 128 ("b=2" of 122 + 7 characters).  Refused: both lines, the
    // number of lines and the block stay, nothing is redrawn
    const std::string x122(122, 'x');
    lines(x122, "a=1:b=2");
    btopx = 4;
    btopy = 1;
    bbotx = 6;
    bboty = 1;
    bstx = 1;
    allalt = 0;
    host_resetcalls();
    join();
    CHECK(host_calls[HC_BEEPEM] == 1);
    CHECK(host_calls[HC_SHOWALL] == 0);
    CHECK(numlines == 3);
    CHECK(listok());
    CHECK(text(0) == x122);
    CHECK(text(1) == "a=1:b=2");
    CHECK(btopx == 4);
    CHECK(btopy == 1);
    CHECK(bbotx == 6);
    CHECK(bboty == 1);
    CHECK(bstx == 1);
    CHECK(allalt == 0);
    CHECK(host_hazards == 0);

    // a prefix of chcols characters and any text
    lines(std::string(chcols, 'x'), "a");
    host_resetcalls();
    join();
    CHECK(host_calls[HC_BEEPEM] == 1);
    CHECK(numlines == 3);

    // tokenise on: the detokenised length counts, 121 + 8 = 129 (only
    // 121 + 5 bytes tokenised)
    tokenise = 1;
    const std::string pre = std::string(120, 'x') + ":";
    lines(pre, nprint());
    host_resetcalls();
    join();
    CHECK(host_calls[HC_BEEPEM] == 1);
    CHECK(numlines == 3);
    CHECK(text(0) == pre);
    CHECK(text(1) == "NPrint a");
    tokenise = 0;
}
