// test_tokit - tokit2 (ted.s:4195) on every line of the detok text of the
// 715 tokenised CD sources against the original TED (oracle.hpp:
// tokall/PATH).  The detok text is the core's; a file whose detok text
// differs from the oracle's is a detok failure (test_detok) and counted
// here, not compared.
#include <cstring>
#include <snitch/snitch.hpp>
#include "corpus.hpp"

TEST_CASE("tokit2 on the detok text of every tokenised CD source", "[tokit][corpus]")
{
    corpus::env();
    if (oracle::sources().empty())
        SKIP("no CD (cmake --build --preset amiga --target fetch-originals)");
    REQUIRE(tokens != nullptr);

    std::vector<unsigned char> in(corpus::bufsize), out(corpus::bufsize);
    size_t used = 0;                    // bytes of in that may be non-zero
    int files = 0, nodetok = 0;
    for (const auto &s : oracle::sources()) {
        if (!s.tok)
            continue;
        std::vector<unsigned char> text, tok;

        CAPTURE(s.rel);
        REQUIRE(corpus::detok_text(s, text));
        if (!corpus::matches("detok", s, text)) {
            nodetok++;
            continue;
        }
        // every LF line, in a buffer that is 0 behind it (as oracle.c: tokit
        // looks at the byte behind the line)
        for (size_t t = 0, q = 0; q < text.size(); q++) {
            if (text[q] != '\n')
                continue;
            size_t tl = q - t;

            REQUIRE(tl < in.size() - 16);
            std::memcpy(in.data(), text.data() + t, tl);
            if (used > tl)
                std::memset(in.data() + tl, 0, used - tl);
            used = tl + 16;
            UWORD d5 = tokit2(out.data(), in.data(), static_cast<WORD>(tl), nullptr);
            tok.insert(tok.end(), out.begin(), out.begin() + d5);
            t = q + 1;
        }
        CHECK(corpus::matches("tokall", s, tok));
        files++;
    }
    CHECK(nodetok == 0);
    CHECK(files == 715);
}
