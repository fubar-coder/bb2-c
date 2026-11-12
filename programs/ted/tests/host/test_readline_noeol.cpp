// test_readline_noeol - readline (ted.s:3375) tokenises a line without
// end (the last line without LF, the first part of a line cut at the
// buffer end) like the lines ended by LF when the file is a text file (an
// earlier line ended by LF) or the line has no byte >= $80 (fixed #376);
// token pairs of a tokenised file's line without end stay as they are.
// Files loaded with fileopen2, tokenise = 1.
#include <csetjmp>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
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
std::string tok(const char *text)
{
    static UBYTE in[256], out[256];

    std::memset(in, 0, sizeof(in));
    std::memset(out, 0, sizeof(out));
    std::strcpy(reinterpret_cast<char *>(in), text);
    UWORD n = tokit2(out, in, static_cast<WORD>(std::strlen(text)), nullptr);
    return std::string(reinterpret_cast<const char *>(out), n);
}

// the bytes of a line with its 0 byte
std::string bytes(const struct item *it)
{
    return std::string(reinterpret_cast<const char *>(it->chars), it->numchars);
}

// write data to path and load it with fileopen2
void load(const std::string &path, const std::string &data, UWORD tok_on = 1)
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

} // namespace

TEST_CASE("readline tokenises a text file's line without end", "[fileio][readline]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(tokens != nullptr);
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    std::string path = dir.path + "/test_readline_noeol.bas";
    const std::string a = tok("NPrint \"a\""), b = tok("NPrint \"b\"");
    REQUIRE(static_cast<UBYTE>(a[0]) >= 0x80);      // NPrint is a token
    REQUIRE(b.size() < std::strlen("NPrint \"b\"") + 1);

    // the last line without LF: tokenised as the first one
    load(path, "NPrint \"a\"\nNPrint \"b\"");
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem) == a);
    CHECK(bytes(firstitem->nextitem) == b);

    // a file of one line without LF
    load(path, "NPrint \"b\"");
    REQUIRE(numlines == 1);
    CHECK(bytes(firstitem) == b);

    // a line cut at the buffer end: its first part is tokenised as well
    std::string longline = "NPrint \"" + std::string(140, '0') + "\"";
    load(path, longline + "\nNPrint \"a\"\n");
    REQUIRE(numlines >= 2);
    CHECK(firstitem->chars[0] == static_cast<UBYTE>(a[0]));
    CHECK(firstitem->chars[1] == static_cast<UBYTE>(a[1]));
    CHECK(bytes(enditem) == a);

    // a tokenised file (lines end in 0 bytes): a line without end with
    // token pairs stays as it is (they are not tokenised again) ...
    std::string tail = b.substr(0, b.size() - 1);  // without its 0 byte
    load(path, a + tail);
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem) == a);
    CHECK(bytes(firstitem->nextitem) == b);
    // ... a half token at the end is dropped (fixed #379: the original
    // kept it in front of the 0)
    load(path, a + "a\xDD");
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem->nextitem) == std::string("a", 2));
    // ... also as the only byte of the line; in a text file the byte is
    // a character (#374), a literal character (spec
    // editor/literal-characters: the pair $90 cc)
    load(path, "NPrint \"a\"\na\xDD");
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem->nextitem) == std::string("a\x90\xDD", 4));
    load(path, a + "\xDD");
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem->nextitem) == std::string("", 1));
    // ... one of plain characters is tokenised, as an edit would do
    load(path, a + "NPrint \"b\"");
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem->nextitem) == b);

    // tokenise off: nothing is tokenised
    load(path, "NPrint \"a\"\nNPrint \"b\"", 0);
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem->nextitem) == std::string("NPrint \"b\"", 11));
    tokenise = 1;
}
