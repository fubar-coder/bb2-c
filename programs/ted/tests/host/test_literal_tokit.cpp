// test_literal_tokit - the tokeniser with literal characters (spec
// editor/literal-characters, "Which characters become a literal
// character"; design D4, D4a): a marked character >= $80 of a column row
// (literal.h) is written as the pair $90 cc in strings, in comments and
// outside them; it never takes part in a keyword; the umlauts stay the
// codes 26-31; the line keeps at most 129 bytes and a pair is never cut.
// The rows go through charstobuff (chmap and line buffer rows: the scratch
// row, literal.h), as the editor tokenises them.
#include <cstdlib>
#include <cstring>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "hostenv.h"
#include "fileio.h"
#include "tokit.h"
#include "literal.h"
}
#include "oracle.hpp"

namespace {

const std::string P = std::string("\x90", 1);   // the first byte of the pair

// the environment with the original's token list, set up once per program
// (the core keeps its globals)
void env()
{
    static struct tokennode *list;

    if (!list) {
        hostenv_init(oracle::tokens_file().c_str(), 4);
        list = tokens;
    }
    tokens = list;
    REQUIRE(tokens != nullptr);
}

// the text as a column row in charbuff2 (blanks up to chcols); every byte
// >= $80 of text is a literal character (marked), tokenised by charstobuff
std::string tokrow(const std::string &text)
{
    litclear();
    REQUIRE(text.size() <= chcols);
    std::memset(charbuff2, ' ', chcols);
    for (size_t i = 0; i < text.size(); i++) {
        UBYTE c = static_cast<UBYTE>(text[i]);

        if (c & 0x80)
            litput(charbuff2 + i, c);
        else
            charbuff2[i] = c;
    }
    UWORD n = charstobuff(charbuff2);
    return std::string(reinterpret_cast<const char *>(charbuff), n);
}

// the same text with every byte >= $80 as '~' (no letter, no quote),
// tokenised, then each '~' replaced by the pair: what the line must be
std::string expect(const std::string &text)
{
    std::string t = text, lits;

    for (auto &c : t)
        if (static_cast<UBYTE>(c) & 0x80) {
            lits += c;
            c = '~';
        }
    std::string out = tokrow(t), r;
    size_t k = 0;

    for (char c : out) {
        if (c == '~' && k < lits.size())
            r += P + lits[k++];
        else
            r += c;
    }
    REQUIRE(k == lits.size());
    return r;
}

std::string z(const std::string &s)
{
    return s + std::string(1, '\0');
}

struct tokennode *litlist[127];

// literallib's 127 entries at the end of the list (test_literal_names)
void appendlib()
{
    struct tokennode *last = tokens;

    while (last->next)
        last = last->next;
    for (int n = 0; n < 127; n++) {
        auto *node = static_cast<struct tokennode *>(std::calloc(1, sizeof(struct tokennode) + 4));

        node->num = static_cast<UWORD>(0x1081 + n);
        node->name[0] = static_cast<char>(0x81 + n);
        litlist[n] = node;
        last->next = node;
        last = node;
    }
}

// the entries of appendlib taken off the list again
void droplib()
{
    struct tokennode *p = tokens;

    while (p->next && p->next != litlist[0])
        p = p->next;
    p->next = nullptr;
    for (auto *node : litlist)
        std::free(node);
}

} // namespace

TEST_CASE("tokeniser: a literal character is the pair, inside and outside strings", "[literal][tokit]")
{
    env();
    const UWORD keep = tokenise;
    tokenise = 1;

    // in a string: the bytes as they are, the pair for ß (ö: the code 27)
    CHECK(tokrow("a$=\"Gr\x1b\xdf" "e\"") == z("a$=\"Gr\x1b" + P + "\xdf" "e\""));
    // the rest of the line is tokenised as without the character
    CHECK(tokrow("a$=\"Gr\x1b\xdf" "e\" : NPrint a$") == expect("a$=\"Gr\x1b\xdf" "e\" : NPrint a$"));
    // outside a string (not guaranteed, design D4): the pair, not "i"
    CHECK(tokrow("NPrint \xe9") == expect("NPrint \xe9"));
    CHECK(tokrow("NPrint \xe9").find(P + "\xe9") != std::string::npos);
    CHECK(tokrow("NPrint \xe9").find('i') == std::string::npos);
    // in a comment, behind a statement and as the whole line
    CHECK(tokrow("; \xe9\xdf") == z("; " + P + "\xe9" + P + "\xdf"));
    CHECK(tokrow("NPrint 1 ; caf\xe9") == expect("NPrint 1 ; caf\xe9"));
    // at the end of the line and as the only character
    CHECK(tokrow("a=1 ; \xe9") == expect("a=1 ; \xe9"));
    CHECK(tokrow("\xe9") == z(P + "\xe9"));
    // no keyword across a literal character: "Pri" é "nt" is no Print
    CHECK(tokrow("Pri\xe9nt") == expect("Pri\xe9nt"));
    CHECK(tokrow("Pri\xe9nt").find("Pri") == 0);
    // a keyword in front of a literal character is one
    CHECK(tokrow("NPrint\xe9") == expect("NPrint\xe9"));
    CHECK(tokrow("NPrint\xe9").size() == 5);   // the token, the pair, the 0
    // the umlauts stay the codes 26-31; every character $80-$FF a pair
    CHECK(tokrow("\"\x1a\x1b\x1c\x1d\x1e\x1f\"") == z("\"\x1a\x1b\x1c\x1d\x1e\x1f\""));
    for (int cc = 0x81; cc <= 0xFF; cc++) {
        CAPTURE(cc);
        CHECK(tokrow(std::string("\"") + static_cast<char>(cc) + "\"")
              == z("\"" + P + static_cast<char>(cc) + "\""));
    }
    tokenise = keep;
}

TEST_CASE("tokeniser: the line limit counts bytes, a pair is never cut", "[literal][tokit]")
{
    env();
    const UWORD keep = tokenise;
    tokenise = 1;
    std::string pairs64;

    for (int i = 0; i < 64; i++)
        pairs64 += P + "\xdf";
    // 64 literal characters: 128 bytes and the 0
    CHECK(tokrow(std::string(64, '\xdf')) == z(pairs64));
    // 65: the 65th does not fit (130 bytes), the line ends in front of it
    CHECK(tokrow(std::string(65, '\xdf')) == z(pairs64));
    // a character and 64 literal characters: 129 bytes, they fit
    CHECK(tokrow(";" + std::string(64, '\xdf')) == z(";" + pairs64));
    // two characters and 64: the last pair would be the 130th byte
    CHECK(tokrow(";;" + std::string(64, '\xdf'))
          == z(";;" + pairs64.substr(0, 126)));
    // a full row of characters without a literal one: as before (128)
    CHECK(tokrow(";" + std::string(127, 'x')) == z(";" + std::string(127, 'x')));
    tokenise = keep;
}

TEST_CASE("tokeniser: the library's names in the list match no keyword", "[literal][tokit]")
{
    env();
    const UWORD keep = tokenise;
    tokenise = 1;
    const std::string lines[] = {
        "a$=\"Gr\x1b\xdf" "e\" : NPrint a$",
        "NPrint \xe9",
        "Pri\xe9nt",
        "\xdf" "a",
        "If a=\xe9 Then End",
        "; \xe9 Print",
    };
    std::string before[6];

    for (int i = 0; i < 6; i++)
        before[i] = tokrow(lines[i]);
    appendlib();
    for (int i = 0; i < 6; i++) {
        CAPTURE(i);
        CHECK(tokrow(lines[i]) == before[i]);
    }
    tokenise = keep;
    droplib();
}

TEST_CASE("charstobuff without tokenise: the pair as well (one stored form)", "[literal][tokit]")
{
    env();
    const UWORD keep = tokenise;
    tokenise = 0;
    CHECK(tokrow("a$=\"Gr\x1b\xdf" "e\"") == z("a$=\"Gr\x1b" + P + "\xdf" "e\""));
    CHECK(tokrow("print \xe9") == z("print " + P + "\xe9"));
    std::string pairs64;
    for (int i = 0; i < 64; i++)
        pairs64 += P + "\xdf";
    CHECK(tokrow(std::string(65, '\xdf')) == z(pairs64));
    CHECK(tokrow(";" + std::string(64, '\xdf')) == z(";" + pairs64));
    // a byte >= $80 that is no literal character (a token pair shown raw
    // with tokenise off) stays as it is
    litclear();
    std::memset(charbuff2, ' ', chcols);
    std::memcpy(charbuff2, "a\x8a\x41" "b", 4);
    UWORD n = charstobuff(charbuff2);
    CHECK(std::string(reinterpret_cast<const char *>(charbuff), n) == z("a\x8a\x41" "b"));
    tokenise = keep;
}
