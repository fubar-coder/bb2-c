// test_readline_latin1 - readline (ted.s:3375) with bytes >= $80 in a text
// file (its first buffer has a LF and no 0 byte): each is one character,
// never the start of a token pair, so a LF, TAB or 0 behind it stays what
// it is (fixed #374: the original took the next byte as the pair's second
// byte); with tokenise on the umlauts of the germs table are stored as
// typing stores them (26-31), the other Latin-1 characters lose bit 7 in
// the tokeniser as in the original; with tokenise off the bytes stay, and
// the file is saved as it was loaded.  Files loaded with fileopen2 and
// saved with realsave.
#include <csetjmp>
#include <cstdio>
#include <cstring>
#include <string>
#include <snitch/snitch.hpp>
#include "lines_helpers.hpp"
#include "tempdir.hpp"
extern "C" {
#include "fileio.h"
#include "loadsave.h"
#include "tokit.h"
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
}

// an outmem would longjmp into nowhere: fail the test case instead
struct no_outmem {
    std::jmp_buf jb;

    no_outmem() { host_outmem = &jb; }
    ~no_outmem() { host_outmem = nullptr; }
};

// text tokenised by tokit2 (with its 0 byte)
std::string tok(const std::string &text)
{
    static UBYTE in[256], out[256];

    std::memset(in, 0, sizeof(in));
    std::memset(out, 0, sizeof(out));
    std::memcpy(in, text.data(), text.size());
    UWORD n = tokit2(out, in, static_cast<WORD>(text.size()), nullptr);
    return std::string(reinterpret_cast<const char *>(out), n);
}

// the bytes of line i with its 0 byte
std::string bytes(int i)
{
    const struct item *it = firstitem;

    while (i-- > 0 && it)
        it = it->nextitem;
    REQUIRE(it != nullptr);
    return std::string(reinterpret_cast<const char *>(it->chars), it->numchars);
}

std::string z(const std::string &s)
{
    return s + std::string(1, '\0');
}

// write data to path and load it with fileopen2
void load(const std::string &path, const std::string &data, UWORD tok_on)
{
    FILE *f = std::fopen(path.c_str(), "wb");

    REQUIRE(f != nullptr);
    std::fwrite(data.data(), 1, data.size(), f);
    std::fclose(f);
    std::string name = path;
    hostenv_droplist();
    tokenise = tok_on;
    fileopen2(reinterpret_cast<UBYTE *>(name.data()));
    std::remove(path.c_str());
}

// the file realsave writes
std::string save(const std::string &path)
{
    std::string name = path;
    long len = 0;

    realsave(reinterpret_cast<UBYTE *>(name.data()));
    UBYTE *b = host_readfile(path.c_str(), &len);
    REQUIRE(b != nullptr);
    std::string s(reinterpret_cast<const char *>(b), static_cast<size_t>(len));
    std::free(b);
    std::remove(path.c_str());
    return s;
}

} // namespace

TEST_CASE("readline: a byte >= $80 of a text file is one character", "[fileio][readline]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    const std::string path = dir.path + "/test_readline_latin1.bas";
    const UWORD keep = tabstop;
    tabstop = 8;

    // an umlaut in front of the LF: the LF ends the line (the original
    // took it as the low byte of a token and joined the lines)
    load(path, "x\xe4\nyy\n", 0);
    REQUIRE(numlines == 2);
    CHECK(bytes(0) == z("x\xe4"));
    CHECK(bytes(1) == z("yy"));

    // a TAB behind it is expanded, from the column of one character
    load(path, "\xe4\tb\n", 0);
    REQUIRE(numlines == 1);
    CHECK(bytes(0) == z("\xe4       b"));

    // the last byte of the file stays (#379 drops a half token of a
    // tokenised file only)
    load(path, "x\n\xe4", 0);
    REQUIRE(numlines == 2);
    CHECK(bytes(1) == z("\xe4"));

    // a line of 128 Latin-1 characters is 128 columns: one line
    load(path, std::string(128, '\xdf') + "\n", 0);
    REQUIRE(numlines == 1);
    CHECK(bytes(0) == z(std::string(128, '\xdf')));

    // a tokenised file (lines end in 0) keeps its token pairs, also with
    // a LF as the low byte
    load(path, std::string("a\x80\x0a" "b", 4) + std::string(1, '\0') + "c" + std::string(1, '\0'), 0);
    REQUIRE(numlines == 2);
    CHECK(bytes(0) == z("a\x80\x0a" "b"));
    CHECK(bytes(1) == z("c"));

    tabstop = keep;
    tokenise = 1;
}

TEST_CASE("readline: a text file with Latin-1 saved with tokenise off is unchanged", "[fileio][readline]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    const std::string path = dir.path + "/test_readline_latin1.bas";
    const std::string text = "; Gr\xf6\xdf" "e \xe4\xf6\xfc \xc4\xd6\xdc \xdf \xe9\n"
                             "a$=\"\xe4\xf6\xfc\xdf\" : NPrint a$\n"
                             "b$=\"\xfd\xfe\xff\"\n"
                             "; \xe4\n"
                             "End\n";

    load(path, text, 0);
    REQUIRE(numlines == 5);
    CHECK(bytes(3) == z("; \xe4"));
    CHECK(save(path) == text);
    tokenise = 1;
}

TEST_CASE("readline: with tokenise on the germs umlauts are stored as typed", "[fileio][readline]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(tokens != nullptr);
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    const std::string path = dir.path + "/test_readline_latin1.bas";

    // ä ö ü Ä Ö Ü -> 26..31 (the germs table, ted.s:6022, as typing maps
    // them; blitz2 maps them back to Latin-1 in strings); ß and the other
    // Latin-1 characters lose bit 7 in the tokeniser (as in the original)
    load(path, "a$=\"\xe4\xf6\xfc\xc4\xd6\xdc\xdf\xe9\" : NPrint a$\n; \xe4\nEnd", 1);
    REQUIRE(numlines == 3);
    CHECK(bytes(0) == tok("a$=\"\x1a\x1b\x1c\x1d\x1e\x1f\x5f\x69\" : NPrint a$"));
    CHECK(bytes(1) == tok("; \x1a"));
    CHECK(bytes(2) == tok("End"));
    tokenise = 1;
}
