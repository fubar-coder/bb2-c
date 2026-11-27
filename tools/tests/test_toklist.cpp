// test_toklist - the compiler's token list read as text (lib/toklist): the
// entries of a list from memory (kinds, M, escapes, comments, an asm entry
// over several lines), the JSON of the keywords, the comparison with a
// committed file, each broken list with its reason, and the compiler's own
// list programs/blitz2/src/toklist.h (TOKLIST_H): its TK names in the
// JSON, none of its TA mnemonics.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "toklist.h"
}

static char err[400];

static int parse(tl_list *l, const char *text)
{
    err[0] = 0;
    return tl_parse(l, text, err, sizeof err);
}

static const char *const sample =
    "/* the head\n"
    "   comment */\n"
    "TK(tok1, tok2, 1, NEWTYPE, \"NEWTYPE\", \".Typename\")\n"
    "TK(tok2, tok3, 2, CSIF, \"CSIF\", \"\\\"String\\\" Comparison\") // a comment\n"
    "TK(tok3, opabcd, 3, SYNTAX, M(\"Jimi\", \"SYNTAX\"), M(\"\", \"<Mode>\"))\n"
    "TA(opabcd, opadd, fnum+0, ABCD, \"ABCD\",\n"
    "   0xc100, dn_^pan_, (UWORD)((15<<12)+byte_), abcdchk, (WORD)0)\n"
    "TA(opmovep, x, fnum+67, MOVEP, M(\"MOVEP\", \"Chip\"),\n"
    "   0x108, f(1, 2), NULL, (WORD)0)\n"
    "TKLAST(tok4, tnum+32, RUNERRSOFF, M(\"Runerrsoff\", \"RunErrsOff\"), \"\")\n"
    "/* AmiBlitz3's */\n"
    "TK3(tok5, tok6, tnum+33, KEEP, \"KEEP\", \"\")\n"
    "TK3LAST(tok6, tnum+40, OPTIMIZE, \"OPTIMIZE\", \"Mode\")\n";

TEST_CASE("toklist entries", "[toklist]")
{
    tl_list l = {};

    REQUIRE(parse(&l, sample) == 0);
    REQUIRE(l.n == 8);
    CHECK(l.v[0].kind == TL_TK);
    CHECK(!strcmp(l.v[0].name, "NEWTYPE"));
    CHECK(!strcmp(l.v[0].help, ".Typename"));
    CHECK(l.v[0].line == 3);
    /* escapes decoded */
    CHECK(!strcmp(l.v[1].help, "\"String\" Comparison"));
    /* M: blitz2's name and help */
    CHECK(!strcmp(l.v[2].name, "Jimi"));
    CHECK(!strcmp(l.v[2].help, ""));
    /* an asm entry over two lines, brackets and calls in its data */
    CHECK(l.v[3].kind == TL_TA);
    CHECK(!strcmp(l.v[3].name, "ABCD"));
    CHECK(!strcmp(l.v[3].help, ""));
    CHECK(l.v[3].line == 6);
    CHECK(!strcmp(l.v[4].name, "MOVEP"));
    CHECK(l.v[4].line == 8);
    CHECK(l.v[5].kind == TL_TKLAST);
    CHECK(!strcmp(l.v[5].name, "Runerrsoff"));
    CHECK(l.v[5].line == 10);
    CHECK(l.v[6].kind == TL_TK3);
    CHECK(l.v[7].kind == TL_TK3LAST);
    CHECK(l.v[7].line == 13);
    CHECK(tl_count(&l, TL_TK) == 3);
    CHECK(tl_count(&l, TL_TA) == 2);
    tl_free(&l);

    /* adjacent literals are one string; a list without TK3 */
    REQUIRE(parse(&l, "TK(a,b,1,X,\"Ab\" \"cd\",\"\")\nTKLAST(c,2,Y,\"Y\",\"h\")") == 0);
    REQUIRE(l.n == 2);
    CHECK(!strcmp(l.v[0].name, "Abcd"));
    tl_free(&l);
}

TEST_CASE("toklist json", "[toklist]")
{
    tl_list l = {};

    REQUIRE(parse(&l, sample) == 0);
    char *j = tl_json(&l, "by a \"test\"", "toklist.h");
    REQUIRE(j);
    /* the TK and TKLAST entries only, in order; " and \ escaped, other bytes as \u00XX */
    CHECK(std::string(j) ==
          "{\n"
          "  \"compiler\": \"blitz2\",\n"
          "  \"generated\": \"by a \\\"test\\\"\",\n"
          "  \"source\": \"toklist.h\",\n"
          "  \"keywords\": [\n"
          "    {\"name\": \"NEWTYPE\", \"help\": \".Typename\"},\n"
          "    {\"name\": \"CSIF\", \"help\": \"\\\"String\\\" Comparison\"},\n"
          "    {\"name\": \"Jimi\", \"help\": \"\"},\n"
          "    {\"name\": \"Runerrsoff\", \"help\": \"\"}\n"
          "  ]\n"
          "}\n");
    std::free(j);
    tl_free(&l);

    REQUIRE(parse(&l, "TK(a,b,1,X,\"A\",\"tab\\there\\\\\")\nTKLAST(c,2,Y,\"Y\",\"h\")") == 0);
    j = tl_json(&l, "", "");
    REQUIRE(j);
    CHECK(strstr(j, "{\"name\": \"A\", \"help\": \"tab\\u0009here\\\\\"},\n") != nullptr);
    std::free(j);
    tl_free(&l);
}

TEST_CASE("toklist broken lists", "[toklist]")
{
    static const struct {
        const char *text, *msg;
    } cases[] = {
        { "TKLAST(c,2,Y,\"Y\",\"h\")\nTQ(a,b)\n", "line 2: not an entry: TQ" },
        { "TKLAST(c,2,Y,\"Y\",\"h\")\n;\n", "line 2: not an entry: ';'" },
        { "TK\n", "line 1: TK without its arguments" },
        { "TK(a,b,1,X,\"A\")\n", "line 1: TK with 5 arguments, 6 wanted" },
        { "TA(a,b,1,X,\"A\")\n", "line 1: TA with 5 arguments, at least 6 wanted" },
        { "\nTKLAST(c,2,Y,\"Y\",\"h\",\"\")\n", "line 2: TKLAST with 6 arguments, 5 wanted" },
        { "TK(a,b,1,X,NAME,\"\")\n", "line 1: name: no string literal or M(...)" },
        { "TK(a,b,1,X,\"A\" x,\"\")\n", "line 1: name: more than a string" },
        { "TK(a,b,1,X,\"A\",help)\n", "line 1: help: no string literal or M(...)" },
        { "TK(a,b,1,X,M(\"A\"),\"\")\n", "line 1: name: M needs two strings" },
        { "TK(a,b,1,X,M,\"\")\n", "line 1: name: M without its arguments" },
        { "TK(a,b,1,X,\"\",\"\")\n", "line 1: empty name" },
        { "TK(a,b,1,X,\"A\\x41\",\"\")\n", "line 1: name: escape \\x not supported" },
        { "/* open\n\n", "line 1: comment not closed" },
        { "TK(a,b,1,X,\"A,\n\")\n", "line 1: string not closed" },
        { "TK(a,b,1,X,\"A\",\"\"\n\n", "line 1: entry not closed" },
        { "TK(a,b,1,X,\"A\",\"\")\n", "line 2: no TKLAST" },
        { "TK3(a,b,1,X,\"A\",\"\")\n", "line 1: TK3 before TKLAST" },
        { "TKLAST(c,2,Y,\"Y\",\"h\")\nTK(a,b,1,X,\"A\",\"\")\n", "line 2: TK after TKLAST" },
        { "TKLAST(c,2,Y,\"Y\",\"h\")\nTKLAST(c,2,Y,\"Y\",\"h\")\n", "line 2: TKLAST after TKLAST" },
        { "TKLAST(c,2,Y,\"Y\",\"h\")\nTK3LAST(c,2,Y,\"Y\",\"h\")\nTK3(a,b,1,X,\"A\",\"\")\n",
          "line 3: TK3 after TK3LAST" },
    };

    for (const auto &c : cases) {
        tl_list l = {};

        CAPTURE(c.text);
        CHECK(parse(&l, c.text) == -1);
        CHECK(std::string(err) == c.msg);
        CHECK(l.n == 0);
        CHECK(l.v == nullptr);
    }
}

TEST_CASE("toklist compare", "[toklist]")
{
    const char *got = "a\nb\nc\n";
    char *r = nullptr;

    CHECK(tl_compare("a\nb\nc\n", 6, got, "f.json", &r) == 0);
    CHECK(r == nullptr);

    REQUIRE(tl_compare("a\nB\nc\n", 6, got, "f.json", &r) == 1);
    CHECK(std::string(r) == "f.json:2: differs from the generated file: B\n  generated: b\n"
                            "f.json: lines that differ from the generated file: 1\n");
    std::free(r);

    REQUIRE(tl_compare("a\nb\n", 4, got, "f.json", &r) == 1);
    CHECK(std::string(r) == "f.json:3: missing: c\nf.json: lines that differ from the generated file: 1\n");
    std::free(r);

    REQUIRE(tl_compare("a\nb\nc\nd\n", 8, got, "f.json", &r) == 1);
    CHECK(std::string(r) ==
          "f.json:4: not in the generated file: d\nf.json: lines that differ from the generated file: 1\n");
    std::free(r);

    REQUIRE(tl_compare("a\nb\nc", 5, got, "f.json", &r) == 1);
    CHECK(std::string(r) == "f.json:3: differs from the generated file (the newline at its end): c\n"
                            "  generated: c\nf.json: lines that differ from the generated file: 1\n");
    std::free(r);

    /* at most ten lines named, all counted */
    std::string many, mine;
    for (int i = 0; i < 12; i++) {
        many += "x\n";
        mine += "y\n";
    }
    REQUIRE(tl_compare(mine.data(), mine.size(), many.c_str(), "f.json", &r) == 1);
    std::string rep(r);
    CHECK(rep.find("f.json:10: differs") != std::string::npos);
    CHECK(rep.find("f.json:11:") == std::string::npos);
    CHECK(rep.find("f.json: lines that differ from the generated file: 12\n") != std::string::npos);
    std::free(r);
}

TEST_CASE("toklist the compiler's list", "[toklist]")
{
    std::ifstream f(TOKLIST_H, std::ios::binary);
    std::ostringstream ss;
    tl_list l = {};

    REQUIRE(f);
    ss << f.rdbuf();
    REQUIRE(parse(&l, ss.str().c_str()) == 0);
    /* the 210 nodes of blitz2's list (92 keywords and 118 mnemonics) and
       AmiBlitz3's 8 more */
    CHECK(tl_count(&l, TL_TK) + tl_count(&l, TL_TKLAST) == 92);
    CHECK(tl_count(&l, TL_TA) == 118);
    CHECK(tl_count(&l, TL_TK3) + tl_count(&l, TL_TK3LAST) == 8);

    char *j = tl_json(&l, "", "");
    REQUIRE(j);
    std::string json(j);
    std::free(j);
    std::set<std::string> keywords;
    for (size_t i = 0; i < l.n; i++) {
        std::string entry = "{\"name\": \"" + std::string(l.v[i].name) + "\",";

        CAPTURE(l.v[i].name);
        if (l.v[i].kind == TL_TK || l.v[i].kind == TL_TKLAST) {
            CHECK(json.find(entry) != std::string::npos);
            keywords.insert(l.v[i].name);
        }
    }
    for (size_t i = 0; i < l.n; i++) {
        std::string entry = "{\"name\": \"" + std::string(l.v[i].name) + "\",";

        CAPTURE(l.v[i].name);
        if (l.v[i].kind != TL_TK && l.v[i].kind != TL_TKLAST && !keywords.count(l.v[i].name))
            CHECK(json.find(entry) == std::string::npos);
    }
    CHECK(keywords.count("If") == 1);
    CHECK(keywords.count("NEWTYPE") == 1);
    CHECK(keywords.count("ADD") == 0);
    tl_free(&l);
}
