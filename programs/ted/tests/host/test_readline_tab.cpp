// test_readline_tab - the TAB expansion of readline (ted.s:3375): it stays
// inside the line buffer charbuff2 whatever the tab size (fixed #378), and
// it starts from the column the line shows, a token counting with the
// length of its name (fixed #377).  Files loaded with fileopen2.
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

TEST_CASE("readline: the TAB expansion stays in charbuff2", "[fileio][readline]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    std::string path = dir.path + "/test_readline_tab.bas";
    const UWORD keep = tabstop;
    const size_t buflen = static_cast<size_t>(charbuff3 - charbuff2);

    // a tab size above the buffer (only an options file could set it, the
    // original wrote behind TED's buffer block): the expansion ends at
    // column 128, in front of the end of charbuff2, charbuff3 and what
    // follows stay untouched; the TAB does not fit behind the "a" and
    // starts the next line (#375), which it fills
    std::memset(charbuff3, 0x5A, buflen);
    tabstop = 300;
    load(path, "a\tb\n", 0);
    REQUIRE(numlines == 3);
    bool untouched = true;
    for (size_t i = 0; i < buflen; i++)
        if (charbuff3[i] != 0x5A)
            untouched = false;
    CHECK(untouched);
    CHECK(bytes(firstitem) == std::string("a", 2));
    CHECK(bytes(firstitem->nextitem) == std::string(chcols, ' ') + std::string(1, '\0'));
    CHECK(bytes(enditem) == std::string("b", 2));

    // an ordinary tab size: unchanged
    tabstop = 8;
    load(path, "a\tb\n", 0);
    REQUIRE(numlines == 1);
    CHECK(bytes(firstitem) == std::string("a       b", 10));
    tabstop = keep;
    tokenise = 1;
}

TEST_CASE("readline: a TAB after a token goes to the shown tab position", "[fileio][readline]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(tokens != nullptr);
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    std::string path = dir.path + "/test_readline_tab.bb2";
    const UWORD keep = tabstop;
    REQUIRE(std::string(findtoke(0xDD82)) == "NPrint");

    // a tokenised file, line 2 NPrint<TAB>a: NPrint shows 6 columns, the
    // TAB goes to column 8 (2 blanks; the original counted the 2 bytes of
    // the token and wrote 6)
    tabstop = 8;
    load(path, std::string("x\0\xDD\x82\ta\0", 7));
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem->nextitem) == std::string("\xDD\x82  a", 6));
    // two tokens and text in front of the TAB: NPrint NPrint = 13 columns
    load(path, std::string("\xDD\x82 \xDD\x82\ta\0", 8));
    REQUIRE(numlines == 1);
    CHECK(bytes(firstitem) == std::string("\xDD\x82 \xDD\x82   a", 10));
    // text only: as before
    load(path, std::string("NPrint\ta\n"), 0);
    REQUIRE(numlines == 1);
    CHECK(bytes(firstitem) == std::string("NPrint  a", 10));
    tabstop = keep;
    tokenise = 1;
}
