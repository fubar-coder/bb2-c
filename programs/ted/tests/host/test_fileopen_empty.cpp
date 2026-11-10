// test_fileopen_empty - fileopen2 (ted.s:680) of an empty file starts a new
// text and frees the lines of the old one (fixed #358: the original went
// to donew without freeall, the old text stayed allocated); out of memory
// at its first line leaves no line list (fixed #1009).
#include <csetjmp>
#include <cstdio>
#include <string>
#include <snitch/snitch.hpp>
#include "tempdir.hpp"
extern "C" {
#include "hostenv.h"
#include "loadsave.h"
}

namespace {

// an outmem would longjmp into nowhere: fail the test case instead
struct no_outmem {
    std::jmp_buf jb;

    no_outmem() { host_outmem = &jb; }
    ~no_outmem() { host_outmem = nullptr; }
};

void writefile(const std::string &path, const std::string &data)
{
    FILE *f = std::fopen(path.c_str(), "wb");

    REQUIRE(f != nullptr);
    std::fwrite(data.data(), 1, data.size(), f);
    std::fclose(f);
}

void open(std::string name)
{
    fileopen2(reinterpret_cast<UBYTE *>(name.data()));
}

} // namespace

TEST_CASE("fileopen2 of an empty file frees the old text", "[loadsave]")
{
    tempdir dir;
    no_outmem om;

    hostenv_init(nullptr, 20);
    REQUIRE(!dir.path.empty());
    if (setjmp(om.jb))
        FAIL("unexpected outmem");
    const std::string text = dir.path + "/text.bas", empty = dir.path + "/empty.bas";
    writefile(text, "abc\ndef\nghi\n");
    writefile(empty, "");

    hostenv_droplist();
    const long base = host_live;        // the buffers of hostenv_init
    open(text);
    REQUIRE(numlines == 3);
    CHECK(host_live == base + 3);
    open(empty);
    // a new text of one empty line, the three old lines freed
    REQUIRE(numlines == 1);
    REQUIRE(firstitem != nullptr);
    CHECK(firstitem->chars[0] == 0);
    CHECK(firstitem->nextitem == nullptr);
    CHECK(host_live == base + 1);
    hostenv_droplist();
    std::remove(text.c_str());
    std::remove(empty.c_str());
}

TEST_CASE("fileopen2 out of memory at the first line leaves no line list", "[loadsave]")
{
    // #1009: fileopen2 frees the old text; when the first line of the new
    // one (or the empty line of an empty file) cannot be allocated,
    // outmem must not find the list on the freed text: firstitem = 0
    // (outmem then leaves an empty text, src/mouse.c)
    tempdir dir;
    std::jmp_buf jb;

    hostenv_init(nullptr, 20);
    REQUIRE(!dir.path.empty());
    const std::string text = dir.path + "/text.bas", other = dir.path + "/other.bas",
                      empty = dir.path + "/empty.bas";
    writefile(text, "abc\ndef\nghi\n");
    writefile(other, "jkl\nmno\n");
    writefile(empty, "");

    for (const std::string *name : { &other, &empty }) {
        hostenv_droplist();
        const long base = host_live;

        host_outmem = &jb;
        if (setjmp(jb))
            FAIL("unexpected outmem");
        open(text);
        REQUIRE(numlines == 3);
        // fileopen2 directly: outmem's longjmp skips no destructor
        std::string buf = *name;

        host_resetcalls();
        host_allocfail = 1;
        if (!setjmp(jb))
            fileopen2(reinterpret_cast<UBYTE *>(buf.data()));
        host_outmem = nullptr;
        host_allocfail = 0;
        CHECK(host_calls[HC_OUTMEM] == 1);
        CHECK(firstitem == nullptr);
        CHECK(thisitem == nullptr);
        CHECK(enditem == nullptr);
        CHECK(host_live == base);       // the old text freed, nothing new
        CHECK(host_hazards == 0);
        numlines = 0;
    }
    std::remove(text.c_str());
    std::remove(other.c_str());
    std::remove(empty.c_str());
}
