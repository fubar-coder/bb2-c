// test_literal_files - loading and saving literal characters (spec
// editor/literal-characters, "Which characters become a literal character"
// (text file), "Saving and loading a file"; design D4, D6, D7): readline
// stores a byte >= $80 of a text file as the pair $90 cc, the six umlauts
// of germs as 26-31, with tokenise on and off; LF and TAB behind such a
// byte keep their meaning, a pair is one column for the TAB and for the
// line limit; a tokenised file is loaded as stored.  realsave writes a
// tokenised source as before (no header), a text save only text: a
// literal character as cc, 26-31 as the umlauts, tokens as their names.
#include <csetjmp>
#include <cstdio>
#include <cstdlib>
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

const std::string P = std::string("\x90", 1);

void env()
{
    static bool done;

    if (!done) {
        hostenv_init(oracle::tokens_file().c_str(), 20);
        done = true;
    }
    REQUIRE(tokens != nullptr);
    tabstop = 8;
}

struct no_outmem {
    std::jmp_buf jb;

    no_outmem() { host_outmem = &jb; }
    ~no_outmem() { host_outmem = nullptr; }
};

std::string z(const std::string &s)
{
    return s + std::string(1, '\0');
}

std::string bytes(int i)
{
    const struct item *it = firstitem;

    while (i-- > 0 && it)
        it = it->nextitem;
    REQUIRE(it != nullptr);
    return std::string(reinterpret_cast<const char *>(it->chars), it->numchars);
}

// text tokenised as the editor tokenises a row (charstobuff), every byte
// >= $80 of text a literal character; without the 0
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

std::string save(const std::string &path, UWORD tok_on)
{
    std::string name = path;
    long len = 0;

    tokenise = tok_on;
    realsave(reinterpret_cast<UBYTE *>(name.data()));
    UBYTE *b = host_readfile(path.c_str(), &len);
    REQUIRE(b != nullptr);
    std::string s(reinterpret_cast<const char *>(b), static_cast<size_t>(len));
    std::free(b);
    std::remove(path.c_str());
    std::string bak = path + ".bak";
    std::remove(bak.c_str());
    return s;
}

// the lines of the TED A/B source long_umlaut.bas (tests/ted-ab/src):
// umlauts and ß in a comment and a string, $FD-$FF in a string
const std::string longumlaut =
    "; Latin-1 umlauts \xe4\xf6\xfc \xc4\xd6\xdc \xdf in a comment and in strings, $FD-$FF\n"
    "a$=\"\xe4\xf6\xfc\xdf\" : NPrint a$\n"
    "b$=\"\xfd\xfe\xff\" : NPrint Len(b$)\n"
    "End\n";

} // namespace

TEST_CASE("readline: a Latin-1 byte of a text file is a literal character or an umlaut", "[literal][readline]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    const std::string path = dir.path + "/test_literal_files.bas";

    for (UWORD t = 0; t < 2; t++) {
        CAPTURE(t);
        // Größe: ö the code 27, ß the pair; the next line stays a line
        load(path, "a$=\"Gr\xf6\xdf" "e\"\nb\n", t);
        REQUIRE(numlines == 2);
        if (t)
            CHECK(bytes(0) == tok("a$=\"Gr\x1b\xdf" "e\""));
        else
            CHECK(bytes(0) == z("a$=\"Gr\x1b" + P + "\xdf" "e\""));
        CHECK(bytes(0).find(P + "\xdf") != std::string::npos);
        // an umlaut and a literal character in front of the LF
        load(path, "x\xe4\ny\xe9\nz\n", t);
        REQUIRE(numlines == 3);
        CHECK(bytes(0) == (t ? tok("x\x1a") : z("x\x1a")));
        CHECK(bytes(1) == (t ? tok("y\xe9") : z("y" + P + "\xe9")));
        // a TAB behind it: the pair is one column
        load(path, "\xe9\tb\n", t);
        REQUIRE(numlines == 1);
        CHECK(bytes(0) == (t ? tok("\xe9       b") : z(P + "\xe9" "       b")));
        // a Latin-1 byte right before the end of the file stays
        load(path, "x\n\xdf", t);
        REQUIRE(numlines == 2);
        CHECK(bytes(1) == (t ? tok("\xdf") : z(P + "\xdf")));
        // 128 literal characters: 64 to a line (129 bytes, design D4a),
        // nothing lost
        load(path, std::string(128, '\xdf') + "\n", t);
        REQUIRE(numlines == 2);
        std::string p64;
        for (int i = 0; i < 64; i++)
            p64 += P + "\xdf";
        CHECK(bytes(0) == z(p64));
        CHECK(bytes(1) == z(p64));
        // the A/B source long_umlaut.bas
        load(path, longumlaut, t);
        REQUIRE(numlines == 4);
        CHECK(bytes(1) == (t ? tok("a$=\"\x1a\x1b\x1c\xdf\" : NPrint a$")
                             : z("a$=\"\x1a\x1b\x1c" + P + "\xdf\" : NPrint a$")));
        CHECK(bytes(2).find(P + "\xfd" + P + "\xfe" + P + "\xff") != std::string::npos);
    }
    // a tokenised file is loaded as stored: a pair $90 E4 stays a pair
    load(path, z("a" + P + "\xe4" "b") + z("c"), 1);
    REQUIRE(numlines == 2);
    CHECK(bytes(0) == z("a" + P + "\xe4" "b"));
    tokenise = 1;
}

TEST_CASE("realsave: a tokenised source as before, load and save keep it", "[literal][save]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    const std::string path = dir.path + "/test_literal_files.bb2";
    const std::string file = tok("a$=\"Gr\x1b\xdf" "e\" : NPrint a$") + tok("; caf\xe9")
                             + tok("End");

    // no header, each line with its 0, the pair in the line
    load(path, file, 1);
    REQUIRE(numlines == 3);
    CHECK(save(path, 1) == file);
    CHECK(file.find(P + "\xdf") != std::string::npos);
    CHECK(file.find(P + "\xe9") != std::string::npos);
    // without a literal character: the bytes as before (no change at all)
    const std::string plain = tok("a$=\"x\" : NPrint a$") + tok("End");
    load(path, plain, 1);
    CHECK(save(path, 1) == plain);
    tokenise = 1;
}

TEST_CASE("realsave without tokenise writes only text", "[literal][save]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    const std::string path = dir.path + "/test_literal_files.bas";

    // the text round trip of a file with ß, é and umlauts
    load(path, longumlaut, 0);
    CHECK(save(path, 0) == longumlaut);
    load(path, "a$=\"Gr\xf6\xdf" "e\"\n", 0);
    CHECK(save(path, 0) == "a$=\"Gr\xf6\xdf" "e\"\n");
    // a line with the code 26 (a typed ä) and the token of NPrint: $E4
    // and the name, no byte of a token pair
    const std::string np = tok("NPrint");
    REQUIRE(np.size() == 3);
    mklist(1, nullptr);
    hostenv_droplist();
    {
        std::string l = "\x1a " + np.substr(0, 2) + " " + P + "\xe9";
        static const char *t[1];
        t[0] = l.c_str();
        mklist(1, t);
    }
    std::string s = save(path, 0);
    CHECK(s == "\xe4 NPrint \xe9\n");
    // a tokenised file saved as text: the names
    load(path, tok("NPrint \"\xdf\"") + tok("End"), 1);
    CHECK(save(path, 0) == "NPrint \"\xdf\"\nEnd\n");
    tokenise = 1;
}
