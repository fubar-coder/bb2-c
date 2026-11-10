// test_blokload - Block Load (blokload, ted.s:1738) at a cursor column:
// the cursor line's text in front of the cursor, the lines of the block
// file, the text behind the cursor (the tail) appended to the last line;
// with tokenise on and off.
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
#include "lines.h"
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
    fontmul = 8;
    qualifier = 0;
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
    static UBYTE in[512], out[512];

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
    return std::string(reinterpret_cast<const char *>(it->chars), std::strlen(chars(it)) + 1);
}

// the text of line i as it is shown (token names for token pairs)
std::string shown(int i)
{
    static UBYTE buf[1024];
    const struct item *it = firstitem;

    while (i-- > 0 && it)
        it = it->nextitem;
    REQUIRE(it != nullptr);
    detok(it->chars, buf, buf + sizeof(buf));
    return std::string(reinterpret_cast<const char *>(buf));
}

void writefile(const std::string &path, const std::string &data)
{
    FILE *f = std::fopen(path.c_str(), "wb");

    REQUIRE(f != nullptr);
    std::fwrite(data.data(), 1, data.size(), f);
    std::fclose(f);
}

// line 1 of "abcdef", line, "klm" with the cursor at column col; the
// block file data loaded there
void loadat(const std::string &path, const char *line, UWORD col, const std::string &data)
{
    const char *const t[] = { "abcdef", line, "klm" };

    writefile(path, data);
    host_bname = reinterpret_cast<UBYTE *>(const_cast<char *>(path.c_str()));
    mklist(3, t);
    place(0, 1, col);
    host_resetcalls();
    blokload();
    CHECK(listok());
    CHECK(filehand == 0);
}

} // namespace

TEST_CASE("blokload tokenises the last line without LF once", "[lines][blocks][files]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(tokens != nullptr);
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    const std::string path = dir.path + "/test_blokload.blk";
    tokenise = 1;

    // a last line without LF behind a line with LF (a text file, #376:
    // readline tokenises such a line in fileopen2): Block Load keeps it
    // as text until it has the tail, then tokenises it once (#1013: it
    // was tokenised twice and lost)
    loadat(path, "0123456789TAIL", 10, "XX\nIf a Then NPrint b");
    REQUIRE(numlines == 4);
    CHECK(shown(1) == "0123456789XX");
    CHECK(bytes(2) == tok("If a Then NPrint bTAIL"));
    CHECK(shown(2) == "If a Then NPrint bTAIL");
    CHECK(shown(3) == "klm");

    // a file of one line without LF, inserted into the cursor line
    loadat(path, "0123456789TAIL", 10, "If a Then NPrint b");
    REQUIRE(numlines == 3);
    CHECK(bytes(1) == tok("0123456789If a Then NPrint bTAIL"));
    CHECK(shown(2) == "klm");

    // the lines with LF are tokenised by readline, as before
    loadat(path, "0123456789TAIL", 10, "NPrint a\nIf b Then End\n");
    REQUIRE(numlines == 5);
    CHECK(shown(1) == "0123456789NPrint a");
    CHECK(bytes(2) == tok("If b Then End"));
    CHECK(shown(3) == "TAIL");

    std::remove(path.c_str());
}

TEST_CASE("blokload of a text file with Latin-1", "[lines][blocks][files]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    const std::string path = dir.path + "/test_blokload.blk";

    // a first line ending in a byte >= $80 (#374: one character of a text
    // file): the copy behind the cursor's prefix ends at its 0
    tokenise = 0;
    loadat(path, "0123456789", 2, "\xe4\nYY\n");
    REQUIRE(numlines == 5);
    CHECK(bytes(1) == std::string("01\xe4", 4));
    CHECK(bytes(2) == std::string("YY", 3));
    CHECK(bytes(3) == std::string("23456789", 9));
    // the same with tokenise on: the umlaut as typed (26)
    tokenise = 1;
    loadat(path, "0123456789", 2, "\xe4\nYY\n");
    REQUIRE(numlines == 5);
    CHECK(bytes(1) == std::string("01\x1a", 4));
    CHECK(bytes(2) == tok("YY"));
    CHECK(bytes(3) == tok("23456789"));
    // the last line without LF (left as text for the tail, #1013)
    loadat(path, "0123456789", 2, "x\n\xf6");
    REQUIRE(numlines == 4);
    CHECK(bytes(2) == tok("\x1b" "23456789"));

    std::remove(path.c_str());
}
