// test_readline_long - readline (ted.s:3375) with lines wider than a
// row of the character map: such a line ends in front of the character,
// token pair or TAB that does not fit (at most 128 shown columns, a token
// pair counting with the length of its name, and at most 129 bytes and the
// 0), and the rest of the physical line follows as lines of their own,
// without a byte lost;
// Block Load goes on behind such a line (fixed #375: the original lost the
// 3 bytes behind the cut and Block Load stopped there).  Files loaded with
// fileopen2 and blokload.
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

// the text of a line as it is shown (token names for token pairs)
std::string shown(const struct item *it)
{
    static UBYTE buf[1024];

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

// write data to path and load it with fileopen2
void load(const std::string &path, const std::string &data, UWORD tok_on)
{
    writefile(path, data);
    std::string name = path;
    hostenv_droplist();
    tokenise = tok_on;
    fileopen2(reinterpret_cast<UBYTE *>(name.data()));
    std::remove(path.c_str());
}

// the shown text of every line, joined
std::string alltext()
{
    std::string s;

    for (const struct item *it = firstitem; it; it = it->nextitem)
        s += shown(it);
    return s;
}

std::string digits(size_t n)
{
    std::string s;

    for (size_t i = 0; i < n; i++)
        s += static_cast<char>('0' + i % 10);
    return s;
}

} // namespace

TEST_CASE("readline splits a long line without losing a byte", "[fileio][readline]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    std::string path = dir.path + "/test_readline_long.bas";
    const size_t room = static_cast<size_t>(charbuff3 - charbuff2) - 1;
    REQUIRE(room == 129);
    const UWORD keep = tabstop;
    tabstop = 8;

    // 128 characters and LF: one line, as before
    load(path, digits(128) + "\n", 0);
    REQUIRE(numlines == 1);
    CHECK(bytes(firstitem) == digits(128) + std::string(1, '\0'));
    // 129 characters (the original kept them in one line, wider than a
    // row of the character map): 128, then the last one
    load(path, digits(129) + "\n", 0);
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem) == digits(128) + std::string(1, '\0'));
    CHECK(bytes(enditem) == std::string("8", 2));

    // 200 characters: 128, then the other 72; nothing lost
    const std::string d200 = digits(200);
    load(path, d200 + "\nend\n", 0);
    REQUIRE(numlines == 3);
    CHECK(bytes(firstitem) == d200.substr(0, 128) + std::string(1, '\0'));
    CHECK(bytes(firstitem->nextitem) == d200.substr(128) + std::string(1, '\0'));
    CHECK(bytes(enditem) == std::string("end", 4));

    // over two buffers, and the last part without LF
    const std::string d300 = digits(300);
    load(path, d300, 0);
    REQUIRE(numlines == 3);
    CHECK(alltext() == d300);
    CHECK(firstitem->numchars == 129);
    CHECK(firstitem->nextitem->numchars == 129);

    // tokenise on: every part is tokenised, the text stays the same
    const std::string code = "NPrint \"" + digits(150) + "\" : NPrint \"a\"";
    load(path, code + "\nNPrint \"a\"\n", 1);
    REQUIRE(numlines == 3);
    CHECK(alltext() == code + "NPrint \"a\"");
    CHECK(firstitem->chars[0] >= 0x80);
    CHECK(firstitem->nextitem->chars[0] != 0);
    CHECK(bytes(enditem) == tok("NPrint \"a\""));

    // a TAB whose blanks do not fit starts the next line
    load(path, std::string(128, 'a') + "\tb\n", 0);
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem) == std::string(128, 'a') + std::string(1, '\0'));
    CHECK(bytes(enditem) == std::string("        b", 10));
    // ... one that fits up to column 128 stays, the character behind it
    // starts the next line
    load(path, std::string(120, 'a') + "\tb\n", 0);
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem) == std::string(120, 'a') + std::string(8, ' ') + std::string(1, '\0'));
    CHECK(bytes(enditem) == std::string("b", 2));
    load(path, std::string(119, 'a') + "\tb\n", 0);
    REQUIRE(numlines == 1);
    CHECK(bytes(firstitem) == std::string(119, 'a') + " b" + std::string(1, '\0'));

    tabstop = keep;
    tokenise = 1;
}

TEST_CASE("readline does not split a token pair at the buffer end", "[fileio][readline]")
{
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(tokens != nullptr);
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    std::string path = dir.path + "/test_readline_long.bb2";
    const std::string np = tok("NPrint");
    REQUIRE(np.size() == 3);                        // pair and 0
    const std::string pair = np.substr(0, 2);

    // a tokenised line (0 at its end) of 128 characters and a pair: the
    // pair does not fit behind them and starts the next line
    const std::string line = std::string(128, 'x') + pair + "1";
    load(path, line + std::string(1, '\0') + tok("End"), 1);
    REQUIRE(numlines == 3);
    CHECK(bytes(firstitem) == std::string(128, 'x') + std::string(1, '\0'));
    CHECK(bytes(firstitem->nextitem) == pair + "1" + std::string(1, '\0'));
    CHECK(bytes(enditem) == tok("End"));
    // a pair counts with the width of its name: 122 characters and
    // NPrint fit exactly into 128 columns, 123 do not
    const std::string line2 = std::string(122, 'x') + pair;
    load(path, line2 + std::string(1, '\0') + tok("End"), 1);
    REQUIRE(numlines == 2);
    CHECK(bytes(firstitem) == line2 + std::string(1, '\0'));
    const std::string line3 = std::string(123, 'x') + pair;
    load(path, line3 + std::string(1, '\0') + tok("End"), 1);
    REQUIRE(numlines == 3);
    CHECK(bytes(firstitem) == std::string(123, 'x') + std::string(1, '\0'));
    CHECK(bytes(firstitem->nextitem) == pair + std::string(1, '\0'));
    // 30 pairs (60 bytes, 180 columns): 21 pairs (126 columns), then 9
    std::string pairs30;
    for (int i = 0; i < 30; i++)
        pairs30 += pair;
    load(path, pairs30 + std::string(1, '\0') + tok("End"), 1);
    REQUIRE(numlines == 3);
    CHECK(bytes(firstitem) == pairs30.substr(0, 42) + std::string(1, '\0'));
    CHECK(bytes(firstitem->nextitem) == pairs30.substr(42) + std::string(1, '\0'));
    CHECK(shown(firstitem).size() == 126);
    tokenise = 1;
}

TEST_CASE("blokload goes on behind a long line", "[fileio][readline][blocks]")
{
    static const char *const t[] = { "abcdef", "ghij", "klm" };
    tempdir dir;
    no_outmem om;

    env();
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    std::string path = dir.path + "/test_readline_long.blk";
    host_bname = reinterpret_cast<UBYTE *>(path.data());
    tokenise = 0;
    fontmul = 8;
    qualifier = 0;

    // a block file of a 200 character line and two more lines, loaded
    // at line 1, column 0: the original stopped at the long line
    const std::string d200 = digits(200);
    writefile(path, d200 + "\nYY\nZZ\n");
    mklist(3, t);
    place(0, 1, 0);
    blokload();
    CHECK(listok());
    REQUIRE(numlines == 7);
    CHECK(std::string_view(linetext(0)) == "abcdef");
    CHECK(std::string(linetext(1)) == d200.substr(0, 128));
    CHECK(std::string(linetext(2)) == d200.substr(128));
    CHECK(std::string_view(linetext(3)) == "YY");
    CHECK(std::string_view(linetext(4)) == "ZZ");
    CHECK(std::string_view(linetext(5)) == "ghij");
    CHECK(std::string_view(linetext(6)) == "klm");
    std::remove(path.c_str());
    tokenise = 1;
}
