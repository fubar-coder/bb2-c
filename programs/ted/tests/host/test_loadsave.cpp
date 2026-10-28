// test_loadsave - every CD source loaded with fileopen2 (ted.s:680: doopen,
// readline, allocline) and written back with realsave (ted.s:769),
// tokenise = 1, against the original TED (oracle.hpp: save/PATH): 715
// tokenised sources (round trip) and 9 text sources (tokenised on load).
#include <csetjmp>
#include <snitch/snitch.hpp>
#include "corpus.hpp"

TEST_CASE("fileopen2 + realsave of every CD source", "[loadsave][corpus]")
{
    corpus::env();
    if (oracle::sources().empty())
        SKIP("no CD (cmake --build --preset amiga --target fetch-originals)");
    REQUIRE(tokens != nullptr);

    corpus::tempdir dir;
    REQUIRE(!dir.path.empty());
    std::string out = dir.path + "/loadsave.out";
    int files = 0;
    for (const auto &s : oracle::sources()) {
        std::vector<unsigned char> name(s.path.begin(), s.path.end()), outname(out.begin(), out.end());
        std::vector<unsigned char> got;
        size_t size;
        std::jmp_buf jb;

        CAPTURE(s.rel);
        name.push_back(0);
        outname.push_back(0);
        hostenv_droplist();
        tokenise = 1;
        host_outmem = &jb;
        if (setjmp(jb)) {
            host_outmem = nullptr;
            FAIL_CHECK("outmem");
            continue;
        }
        fileopen2(name.data());
        CHECK(firstitem != nullptr);
        CHECK(numlines != 0);
        std::remove(out.c_str());
        realsave(outname.data());
        host_outmem = nullptr;
        REQUIRE(oracle::readfile(out, got, size));
        got.resize(size);
        CHECK(corpus::matches("save", s, got));
        files++;
    }
    std::remove(out.c_str());
    CHECK(files == 724);
}
