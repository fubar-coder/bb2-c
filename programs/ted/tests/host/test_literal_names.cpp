// test_literal_names - the name of a literal character (spec
// editor/literal-characters, "The name of a literal character"; design D3):
// findtoke answers a word $9080-$90FF with the one character, from the
// token list when it holds the entry (the library literallib), from the
// built-in table when it does not (no list: standalone TED, the host
// tests; a list without the library); a word $9000-$907F stays unknown.
// detok and detoka5 give the character.
#include <cstdlib>
#include <cstring>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "hostenv.h"
#include "fileio.h"
#include "literal.h"
}
#include "oracle.hpp"

namespace {

// a line item holding the bytes s (with its 0)
struct item *mkline(const std::string &s)
{
    auto *it = static_cast<struct item *>(ted_alloc(s.size() + 1 + lenitem, TED_MEMF_PUBLIC));

    it->nextitem = it->lastitem = nullptr;
    it->numchars = static_cast<UBYTE>(s.size() + 1);
    std::memcpy(it->chars, s.data(), s.size());
    it->chars[s.size()] = 0;
    return it;
}

void freeline(struct item *it)
{
    ted_free(it, static_cast<UWORD>(it->numchars + lenitem));
}

// the 128 entries of literallib appended to the list: num = $1080 + n
// (bit 15 cleared, as blitz2 hands it to the editor), name = the character
// with the code $80 + n, marked with '!' behind it so that the test can
// tell the list's entry from the built-in one
struct tokennode *litlist[128];

void appendlib(char mark)
{
    struct tokennode *last = tokens;

    while (last && last->next)
        last = last->next;
    for (int n = 0; n < 128; n++) {
        auto *node = static_cast<struct tokennode *>(std::calloc(1, sizeof(struct tokennode) + 4));

        node->num = static_cast<UWORD>(0x1080 + n);
        node->name[0] = static_cast<char>(0x80 + n);
        node->name[1] = mark;
        litlist[n] = node;
        if (last)
            last->next = node;
        else
            tokens = node;
        last = node;
    }
}

void droplib()
{
    struct tokennode **p = &tokens;

    while (*p && *p != litlist[0])
        p = &(*p)->next;
    *p = nullptr;
    for (auto *node : litlist)
        std::free(node);
}

} // namespace

TEST_CASE("findtoke: no token list - the built-in names", "[literal][findtoke]")
{
    hostenv_init(nullptr, 4);
    REQUIRE(tokens == nullptr);
    for (int cc = 0x80; cc <= 0xFF; cc++) {
        CAPTURE(cc);
        const char *name = findtoke(static_cast<UWORD>(0x9000 + cc));

        CHECK(static_cast<UBYTE>(name[0]) == cc);
        CHECK(name[1] == 0);
        // bit 15 of the word does not matter (findtoke clears it)
        CHECK(findtoke(static_cast<UWORD>(0x1000 + cc)) == name);
    }
    // library 32 and the words $9000-$907F: unknown
    for (int lo = 0; lo < 0x80; lo++) {
        CAPTURE(lo);
        CHECK(findtoke(static_cast<UWORD>(0x9000 + lo)) == tokeerr);
    }
    CHECK(findtoke(0x9100) == tokeerr);
    CHECK(findtoke(0x8F80) == tokeerr);
}

TEST_CASE("findtoke: the list's entry wins, a list without them gives the built-in one", "[literal][findtoke]")
{
    hostenv_init(oracle::tokens_file().c_str(), 4);
    REQUIRE(tokens != nullptr);
    // the original's list has no word $90xx
    for (int cc = 0x80; cc <= 0xFF; cc++) {
        const char *name = findtoke(static_cast<UWORD>(0x9000 + cc));

        CAPTURE(cc);
        CHECK(static_cast<UBYTE>(name[0]) == cc);
        CHECK(name[1] == 0);
    }
    CHECK(findtoke(0x9001) == tokeerr);
    appendlib('!');
    for (int cc = 0x80; cc <= 0xFF; cc++) {
        const char *name = findtoke(static_cast<UWORD>(0x9000 + cc));

        CAPTURE(cc);
        CHECK(name == litlist[cc - 0x80]->name);
        CHECK(std::string(name) == std::string(1, static_cast<char>(cc)) + "!");
    }
    droplib();
    hostenv_init(nullptr, 4);
}

TEST_CASE("detok and detoka5 give the character", "[literal][detok]")
{
    for (int withlist = 0; withlist < 2; withlist++) {
        CAPTURE(withlist);
        hostenv_init(withlist ? oracle::tokens_file().c_str() : nullptr, 4);
        if (withlist)
            appendlib('\0');
        for (int cc = 0x80; cc <= 0xFF; cc++) {
            CAPTURE(cc);
            std::string line = std::string("a\x90", 2) + static_cast<char>(cc) + "b";
            struct item *it = mkline(line);
            UBYTE *end = detok(it->chars, charbuff, charbuff4);

            CHECK(end - charbuff == 3);
            CHECK(std::string(reinterpret_cast<char *>(charbuff), 3)
                  == std::string("a") + static_cast<char>(cc) + "b");
            CHECK(litat(charbuff + 1) == cc);           // marked
            CHECK(litat(charbuff) == 0);
            detoka5(it);
            CHECK(charbuff3[0] == 4);                   // length + 1
            CHECK(std::string(reinterpret_cast<char *>(charbuff3 + 1), 3)
                  == std::string("a") + static_cast<char>(cc) + "b");
            CHECK(litat(charbuff3 + 2) == cc);
            freeline(it);
        }
        if (withlist)
            droplib();
    }
    hostenv_init(nullptr, 4);
}
