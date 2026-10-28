// test_tdscript - teddrive's script syntax (amiga/tdscript.h) on the host:
// numbers, gadget label matching, splitting a line into words (strings
// with escapes, comments).  From bb2 ted-c/tests/tools/tdscript_host.c.
#include <cstring>
#include <string_view>
#include <snitch/snitch.hpp>
extern "C" {
#include "tdscript.h"
}

TEST_CASE("td_number", "[tdscript]")
{
    CHECK(td_number("42", 10) == 42);
    CHECK(td_number("-7", 10) == -7);
    CHECK(td_number("45", 16) == 0x45);
    CHECK(td_number("$4C", 10) == 0x4c);
    CHECK(td_number("0x1f", 10) == 0x1f);
    CHECK(td_number("12ab", 10) == 12);
}

TEST_CASE("td_labelmatch", "[tdscript]")
{
    auto label = [](const char *s) { return reinterpret_cast<const unsigned char *>(s); };

    CHECK(td_labelmatch(label(" OKEE DOKEE "), "okee dokee"));
    CHECK(td_labelmatch(label("OK"), " OK "));
    CHECK(!td_labelmatch(label("OKAY"), "OK"));
    CHECK(!td_labelmatch(label("OK"), "OKAY"));
    CHECK(!td_labelmatch(nullptr, "OK"));
}

TEST_CASE("td_split", "[tdscript]")
{
    char l1[] = "  type \"a\\nb\\x41\\\"c\" next;x ; comment";
    char l2[] = "str 3 \"\"";
    char l3[] = "log \"\\x4\"";
    char *av[8];

    // a string with escapes, a word with ';' inside, the comment dropped
    REQUIRE(td_split(l1, av, 8) == 3);
    CHECK(std::string_view(av[0]) == "type");
    CHECK(std::string_view(av[1]) == "a\nbA\"c");
    CHECK(std::string_view(av[2]) == "next;x");
    // an empty string is a word
    REQUIRE(td_split(l2, av, 8) == 3);
    CHECK(av[2][0] == 0);
    // \x without two hex digits: a literal x
    REQUIRE(td_split(l3, av, 8) == 2);
    CHECK(std::string_view(av[1]) == "x4");
    // no room for words
    CHECK(td_split(l1, av, 0) == 0);
}
