// test_readline_latin1 - readline (ted.s:3375) with bytes >= $80 in a text
// file (its first buffer has a LF and no 0 byte): each is one character,
// never the start of a token pair, so a LF, TAB or 0 behind it stays what
// it is (fixed #374: the original took the next byte as the pair's second
// byte); the umlauts of the germs table are stored as typing stores them
// (26-31), every other Latin-1 character as a literal character (the pair
// $90 cc, spec editor/literal-characters), with tokenise on and off; a
// text save writes both back as the bytes, so the file is saved as it was
// loaded.  Files loaded with fileopen2 and saved with realsave.  (More in
// test_literal_files.)
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
#include "literal.h"
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

// text tokenised as the editor tokenises a row (charstobuff, with its 0
// byte); a byte >= $80 of text is a literal character
std::string tok(const std::string &text)
{
    const UWORD keep = tokenise;

    tokenise = 1;
    litclear();
    std::memset(charbuff2, ' ', chcols);
    for (size_t i = 0; i < text.size(); i++) {
        UBYTE c = static_cast<UBYTE>(text[i]);

        if (c & 0x80)
            litput(charbuff2 + i, c);
        else
            charbuff2[i] = c;
    }
    UWORD n = charstobuff(charbuff2);
    tokenise = keep;
    return std::string(reinterpret_cast<const char *>(charbuff), n);
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
    CHECK(bytes(0) == z("x\x1a"));
    CHECK(bytes(1) == z("yy"));

    // a TAB behind it is expanded, from the column of one character
    load(path, "\xe4\tb\n", 0);
    REQUIRE(numlines == 1);
    CHECK(bytes(0) == z("\x1a       b"));
    load(path, "\xdf\tb\n", 0);
    REQUIRE(numlines == 1);
    CHECK(bytes(0) == z("\x90\xdf       b"));

    // the last byte of the file stays (#379 drops a half token of a
    // tokenised file only)
    load(path, "x\n\xe4", 0);
    REQUIRE(numlines == 2);
    CHECK(bytes(1) == z("\x1a"));

    // a line of 128 Latin-1 characters is 128 columns, but 256 bytes as
    // literal characters: two lines of 64 (design D4a)
    load(path, std::string(128, '\xdf') + "\n", 0);
    REQUIRE(numlines == 2);
    std::string p64;
    for (int i = 0; i < 64; i++)
        p64 += "\x90\xdf";
    CHECK(bytes(0) == z(p64));
    CHECK(bytes(1) == z(p64));

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
    CHECK(bytes(3) == z("; \x1a"));
    CHECK(bytes(2) == z("b$=\"\x90\xfd\x90\xfe\x90\xff\""));
    CHECK(save(path) == text);
    tokenise = 1;
}

TEST_CASE("readline: with tokenise on the germs umlauts are stored as typed, the rest as literal characters", "[fileio][readline]")
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
    // Latin-1 characters are literal characters (the original's tokeniser
    // cleared bit 7: ß became _, é i)
    load(path, "a$=\"\xe4\xf6\xfc\xc4\xd6\xdc\xdf\xe9\" : NPrint a$\n; \xe4\nEnd", 1);
    REQUIRE(numlines == 3);
    CHECK(bytes(0) == tok("a$=\"\x1a\x1b\x1c\x1d\x1e\x1f\xdf\xe9\" : NPrint a$"));
    CHECK(bytes(0).find("\x90\xdf\x90\xe9") != std::string::npos);
    CHECK(bytes(1) == tok("; \x1a"));
    CHECK(bytes(2) == tok("End"));
    tokenise = 1;
}
