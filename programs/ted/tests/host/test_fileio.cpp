// test_fileio - hand cases for core helpers of chunks 04/05: divu_w (68000
// DIVU.W semantics), numtoascii2, the tokeniser quirks of survey-05 A
// (x1print, $ hex, _ . \, quote, remark, all spaces), alpha.
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <snitch/snitch.hpp>
extern "C" {
#include "hostenv.h"
#include "fileio.h"
#include "tokit.h"
}
#include "oracle.hpp"

namespace {

void env()
{
    static bool done;

    if (!done) {
        hostenv_init(oracle::tokens_file().c_str(), 25);
        done = true;
    }
}

// tokenise a text; out gets the tokenised line
UWORD tok(const char *text, UBYTE *out, UBYTE **end = nullptr)
{
    static UBYTE in[256];

    std::memset(in, 0, sizeof(in));
    std::memset(out, 0, 256);
    std::strcpy(reinterpret_cast<char *>(in), text);
    return tokit2(out, in, static_cast<WORD>(std::strlen(text)), end);
}

bool is(const UBYTE *p, const char *s, size_t n)
{
    return std::memcmp(p, s, n) == 0;
}

} // namespace

TEST_CASE("divu_w: DIVU.W of the 68000", "[fileio]")
{
    CHECK(divu_w(100, 7) == ((2u << 16) | 14));
    CHECK(divu_w(0x10000, 1) == 0x10000);          // overflow leaves d0
    CHECK(divu_w(0xEFFFF, 15) == ((0xEFFFFu % 15) << 16 | 0xEFFFF / 15));
    CHECK(divu_w(0xF0000, 15) == 0xF0000);          // overflows
    CHECK(divu_w(15u << 16, 15) == 15u << 16);      // v = 15: overflow (setprops)

    unsigned seed = 1;
    int bad = 0;
    for (ULONG i = 0; i < 100000; i++) {
        seed = seed * 1103515245 + 12345;
        ULONG d = seed;
        seed = seed * 1103515245 + 12345;
        UWORD s = static_cast<UWORD>(seed >> 8) | 1;
        ULONG want = d / s > 0xFFFF ? d : (d % s) << 16 | d / s;

        if (divu_w(d, s) != want && bad++ < 5) {
            CAPTURE(d, s);
            FAIL_CHECK("divu_w");
        }
    }
    CHECK(bad == 0);
}

TEST_CASE("numtoascii2", "[fileio]")
{
    for (UWORD v : { 0, 7, 10, 100, 1234, 65535 }) {
        UBYTE buf[8];
        char want[8];
        UBYTE *e = numtoascii2(v, buf);

        std::snprintf(want, sizeof(want), "%u", v);
        CAPTURE(v);
        CHECK(std::string_view(reinterpret_cast<char *>(buf)) == want);
        CHECK(*e == 0);
        CHECK(e == buf + std::strlen(want));
    }
}

TEST_CASE("tokit2 quirks", "[fileio][tokit]")
{
    env();
    REQUIRE(tokens != nullptr);
    UBYTE out[256], txt[512], *end;
    UWORD len;

    // all spaces: len = 1, a1 ON the 0
    len = tok("    ", out, &end);
    CHECK(len == 1);
    CHECK(end == out);
    CHECK(out[0] == 0);
    // x1print: x 1 Print (the digit ends the identifier)
    len = tok("x1print", out, &end);
    CHECK(out[0] == 'x');
    CHECK(out[1] == '1');
    CHECK((out[2] & 0x80) != 0);
    CHECK(len == 5);
    CHECK(end == out + len);            // a1 behind the 0
    // a token inside quotes stays text
    CHECK(tok("\"print\"", out) == 8);
    CHECK(is(out, "\"print\"", 8));
    // remark: the rest is copied
    CHECK(tok("; print", out) == 8);
    CHECK(is(out, "; print", 8));
    // $ hex: digits copied, the next char goes out unchecked (.nothex):
    // "$fprint" -> the 'p' is eaten, "rint" is no token
    CHECK(tok("$fprint", out) == 8);
    CHECK(is(out, "$fprint", 8));
    tok("$f print", out);
    CHECK((out[3] & 0x80) != 0);        // token after the space
    // _ . start an identifier: no token
    CHECK(tok("_print", out) == 7);
    CHECK(is(out, "_print", 7));
    CHECK(tok(".print", out) == 7);
    CHECK(is(out, ".print", 7));
    // letters behind a name: no token
    CHECK(tok("printx", out) == 7);
    CHECK(is(out, "printx", 7));
    // round trip through detok
    tok("NPrint \"a\":a=1", out);
    detok(out, txt, txt + sizeof(txt));
    CHECK(std::string_view(reinterpret_cast<char *>(txt)) == "NPrint \"a\":a=1");
}

TEST_CASE("no token list: findtoke and tokit match nothing", "[fileio][tokit]")
{
    UBYTE out[256];

    // Fixed (#359): with tokens == 0 (standalone TED) the original walked
    // a "node" at address 0; now every token is unknown and no word
    // becomes a token
    env();
    REQUIRE(tok("print a", out) == 5);
    UWORD num = static_cast<UWORD>((out[0] << 8) | out[1]);
    CHECK(std::string_view(findtoke(num)) == "Print");
    struct tokennode *keep = tokens;
    tokens = nullptr;
    host_resetcalls();
    CHECK(std::string_view(findtoke(num)) == tokeerr);
    CHECK(tok("print a", out) == 8);
    CHECK(is(out, "print a", 8));
    CHECK(host_hazards == 0);
    tokens = keep;
}

TEST_CASE("detok and detoka5 stop at the end of the buffer", "[fileio][detok]")
{
    UBYTE out[256];
    std::string text;

    // Fixed (#355): a line of 23 "NPrint:" (69 bytes) is 161 characters
    // as text; the original wrote it whole behind charbuff3, the end of
    // TED's buffer block
    env();
    for (int i = 0; i < 23; i++)
        text += "NPrint:";
    REQUIRE(tok(text.c_str(), out) == 23 * 3 + 1);
    auto *it = static_cast<struct item *>(ted_alloc(23 * 3 + 1 + lenitem, TED_MEMF_PUBLIC));
    it->numchars = 23 * 3 + 1;
    std::memcpy(it->chars, out, 23 * 3 + 1);

    // detoka5: chcols characters, the length byte chcols + 1
    detoka5(it);
    CHECK(charbuff3[0] == chcols + 1);
    CHECK(std::string_view(reinterpret_cast<char *>(charbuff3 + 1), chcols)
          == text.substr(0, chcols));

    // detok: the text stops in front of end, the 0 goes there
    std::memset(charbuff2, 'x', chcols + 2);
    UBYTE *e = detok(it->chars, charbuff, charbuff2);
    CHECK(e == charbuff2 - 1);
    CHECK(*e == 0);
    CHECK(std::string_view(reinterpret_cast<char *>(charbuff)) == text.substr(0, chcols + 1));
    CHECK(charbuff2[0] == 'x');
    // a short line is whole
    e = detok(it->chars, charbuff, charbuff4);
    CHECK(std::string_view(reinterpret_cast<char *>(charbuff)) == text);
    CHECK(e == charbuff + text.size());
    ted_free(it, it->numchars + lenitem);
}

TEST_CASE("alpha", "[fileio][tokit]")
{
    CHECK(alpha('a'));
    CHECK(alpha('Z'));
    CHECK(!alpha('['));
    CHECK(!alpha('`'));
    CHECK(!alpha('1'));
}
