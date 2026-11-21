// test_testtoml - the strict TOML subset of the run configuration
// (tools/lib/testtoml) on files from memory: every construct of the subset
// with its file and line, and each refused construct of TOML (and broken
// input) with the file and line in its message.
#include <cstdio>
#include <cstring>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "testtoml.h"
}

static char err[400];

static int parse(tt_doc *d, const char *text)
{
    *d = {};
    return tt_parse("dir/tests.toml", text, strlen(text), d, err, sizeof err);
}

// the text must be refused with a message that starts with the file and
// LINE and holds WHAT
static void refused(const char *text, unsigned line, const char *what)
{
    tt_doc d;
    std::string want = "dir/tests.toml:" + std::to_string(line) + ": ";

    INFO(text);
    CHECK(parse(&d, text) == -1);
    CHECK(std::string(err).rfind(want, 0) == 0);
    CHECK(strstr(err, what) != nullptr);
    if (!strstr(err, what) || std::string(err).rfind(want, 0) != 0)
        fprintf(stderr, "message: %s\n", err);
    tt_free(&d);
}

TEST_CASE("testtoml: the subset", "[testtoml]")
{
    tt_doc d;

    REQUIRE(parse(&d,
                  "# the root\n"
                  "root = true\n"
                  "\n"
                  "timeout = 60   # seconds\n"
                  "neg = -5\n"
                  "plus = +7\n"
                  "zero = 0\n"
                  "off = false\n"
                  "name = \"a \\\"b\\\" \\\\ \\t\"\n"
                  "lit = 'C:\\x \"y\"'\n"
                  "libs = [\"cd\", 'clib', ]\n"
                  "none = []\n"
                  "  [matrix]  # axes\n"
                  "libs = [\"cd\"]\n"
                  "[asserts.\"libs=cd\"]\r\n"
                  "\"check.a.width\" = \"== fail\"\r\n"
                  "[ 'x y' . z ]\n"
                  "bare-key_1 = 1\n") == 0);
    CHECK(!strcmp(d.file, "dir/tests.toml"));
    REQUIRE(d.n == 13);
    CHECK(!strcmp(d.e[0].key, "root"));
    CHECK(d.e[0].ntable == 0);
    CHECK(d.e[0].line == 2);
    CHECK(d.e[0].value.type == TT_BOOLEAN);
    CHECK(d.e[0].value.boolean == 1);
    CHECK(d.e[1].value.type == TT_INTEGER);
    CHECK(d.e[1].value.integer == 60);
    CHECK(d.e[1].line == 4);
    CHECK(d.e[2].value.integer == -5);
    CHECK(d.e[3].value.integer == 7);
    CHECK(d.e[4].value.integer == 0);
    CHECK(d.e[5].value.type == TT_BOOLEAN);
    CHECK(d.e[5].value.boolean == 0);
    CHECK(d.e[6].value.type == TT_STRING);
    CHECK(!strcmp(d.e[6].value.str, "a \"b\" \\ \t"));
    CHECK(!strcmp(d.e[7].value.str, "C:\\x \"y\""));
    CHECK(d.e[8].value.type == TT_LIST);
    REQUIRE(d.e[8].value.nlist == 2);
    CHECK(!strcmp(d.e[8].value.list[0], "cd"));
    CHECK(!strcmp(d.e[8].value.list[1], "clib"));
    CHECK(d.e[9].value.type == TT_LIST);
    CHECK(d.e[9].value.nlist == 0);
    // the table of a key: the header above it
    CHECK(!strcmp(d.e[10].key, "libs"));
    REQUIRE(d.e[10].ntable == 1);
    CHECK(!strcmp(d.e[10].table[0], "matrix"));
    CHECK(d.e[10].line == 14);
    REQUIRE(d.e[11].ntable == 2);
    CHECK(!strcmp(d.e[11].table[0], "asserts"));
    CHECK(!strcmp(d.e[11].table[1], "libs=cd"));
    CHECK(!strcmp(d.e[11].key, "check.a.width"));
    CHECK(!strcmp(d.e[11].value.str, "== fail"));
    CHECK(d.e[11].line == 16);
    REQUIRE(d.e[12].ntable == 2);
    CHECK(!strcmp(d.e[12].table[0], "x y"));
    CHECK(!strcmp(d.e[12].table[1], "z"));
    CHECK(!strcmp(d.e[12].key, "bare-key_1"));
    REQUIRE(d.ntables == 3);
    CHECK(d.tables[0].line == 13);
    CHECK(d.tables[1].line == 15);
    CHECK(d.tables[2].line == 17);
    tt_free(&d);
    CHECK(d.n == 0);
}

TEST_CASE("testtoml: empty files and the last line", "[testtoml]")
{
    tt_doc d;

    CHECK(parse(&d, "") == 0);
    CHECK(d.n == 0);
    tt_free(&d);
    CHECK(parse(&d, "# only a comment") == 0);
    tt_free(&d);
    // no LF at the end
    REQUIRE(parse(&d, "a = 1\nb = \"x\"") == 0);
    REQUIRE(d.n == 2);
    CHECK(d.e[1].line == 2);
    tt_free(&d);
    // the same key in two tables
    CHECK(parse(&d, "a = 1\n[t]\na = 2\n[u]\na = 3\n") == 0);
    tt_free(&d);
    // a super table after its sub table is valid TOML
    CHECK(parse(&d, "[a.b]\nx = 1\n[a]\ny = 2\n") == 0);
    tt_free(&d);
    // the largest integers
    REQUIRE(parse(&d, "a = 9223372036854775807\nb = -9223372036854775808\n") == 0);
    CHECK(d.e[0].value.integer == 9223372036854775807LL);
    tt_free(&d);
}

TEST_CASE("testtoml: copies of values", "[testtoml]")
{
    tt_doc d;
    tt_value c;

    REQUIRE(parse(&d, "l = [\"a\", \"b\"]\ns = \"x\"\n") == 0);
    REQUIRE(tt_value_copy(&c, &d.e[0].value) == 0);
    tt_free(&d);
    REQUIRE(c.nlist == 2);
    CHECK(!strcmp(c.list[1], "b"));
    tt_value_free(&c);
    CHECK(!strcmp(tt_type_name(TT_LIST), "list of strings"));
}

TEST_CASE("testtoml: refused constructs", "[testtoml]")
{
    // values outside the subset
    refused("a = 1.5\n", 1, "no value of the subset");
    refused("a = 1_000\n", 1, "no value of the subset");
    refused("a = 0x10\n", 1, "no value of the subset");
    refused("a = 1979-05-27\n", 1, "no value of the subset");
    refused("a = 012\n", 1, "leading zero");
    refused("a = 99999999999999999999\n", 1, "too large");
    refused("a = True\n", 1, "no value of the subset");
    refused("a = { b = 1 }\n", 1, "inline tables");
    refused("a = \"\"\"x\"\"\"\n", 1, "multi-line strings");
    refused("a = '''x'''\n", 1, "multi-line strings");
    refused("x = 1\na = [1, 2]\n", 2, "strings only");
    refused("a = [\"x\",\n\"y\"]\n", 1, "one line");
    refused("a = [\"x\" \"y\"]\n", 1, "',' or ']'");
    refused("a = [\"x\", # c\n", 1, "one line");
    refused("a =\n", 1, "a value expected");
    refused("a = # c\n", 1, "a value expected");
    // strings
    refused("a = \"x\n", 1, "without its closing");
    refused("a = \"x\\u0041\"\n", 1, "escape \\u");
    refused("a = \"x\\\n", 1, "without its closing");
    refused("a = \"x\x01\"\n", 1, "control character");
    refused("a = 'x\x7f'\n", 1, "control character");
    refused("# a\x01\n", 1, "control character");
    // keys
    refused("a.b = 1\n", 1, "dotted keys");
    refused("a 1\n", 1, "'=' expected");
    refused("= 1\n", 1, "a key expected");
    refused("a = 1 b = 2\n", 1, "unexpected text after the value");
    refused("a = 1\nb = 2\na = 3\n", 3, "given twice in its table (line 1)");
    refused("[t]\na = 1\n[u]\n[t]\n", 4, "the table [t] is given twice (line 1)");
    refused("[[t]]\na = 1\n", 1, "arrays of tables");
    refused("[t\n", 1, "'.' or ']' expected");
    refused("[t] x\n", 1, "unexpected text after the table header");
    refused("[]\n", 1, "a key expected");
    refused("[a.]\n", 1, "a key expected");
    // a key that is also a table
    refused("a = 1\n[a]\nb = 2\n", 2, "a is a key (line 1) and a table (line 2)");
    refused("[t]\na = 1\n[t.a.b]\n", 3, "a is a key (line 2) and a table (line 3)");
    // a 0 byte
    {
        tt_doc d = {};
        const char text[] = "a = 1\nb = \"x\0\"\n";

        CHECK(tt_parse("dir/tests.toml", text, sizeof text - 1, &d, err, sizeof err) == -1);
        CHECK(!strcmp(err, "dir/tests.toml:2: a 0 byte in the file"));
        tt_free(&d);
    }
}
