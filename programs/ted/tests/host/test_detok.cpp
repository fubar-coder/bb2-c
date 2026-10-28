// test_detok - detok (ted.s:3455) on every line of the 715 tokenised CD
// sources against the original TED (oracle.hpp: detok/PATH).  Exact: the
// oracle is TED itself, so there are no exceptions (bb2tok's "<$XXXX>" for
// unknown tokens, raw bytes in quotes were its own deviations).
#include <cstring>
#include <snitch/snitch.hpp>
#include "corpus.hpp"

TEST_CASE("detok of every tokenised CD source", "[detok][corpus]")
{
    corpus::env();
    if (oracle::sources().empty())
        SKIP("no CD (cmake --build --preset amiga --target fetch-originals)");
    REQUIRE(tokens != nullptr);

    int files = 0;
    for (const auto &s : oracle::sources()) {
        if (!s.tok)
            continue;
        std::vector<unsigned char> text;

        CAPTURE(s.rel);
        REQUIRE(corpus::detok_text(s, text));
        CHECK(corpus::matches("detok", s, text));
        files++;
    }
    CHECK(files == 715);
}
