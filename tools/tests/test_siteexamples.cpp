// test_siteexamples - the blocks of a site file and the manifest from
// memory: fences (indented, other languages, attributes, an unclosed
// block), manifest lines, and the check against each other - each broken
// input must fail with its reason.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <snitch/snitch.hpp>
extern "C" {
#include "siteexamples.h"
}

static char err[400];

static int scan(se_blocks *b, const char *file, const char *text)
{
    err[0] = 0;
    return se_scan_text(b, file, text, err, sizeof err);
}

static int parse(se_manifest *m, const char *text)
{
    err[0] = 0;
    return se_manifest_parse(m, text, err, sizeof err);
}

TEST_CASE("siteexamples blocks", "[siteexamples]")
{
    se_blocks b = {};

    /* blocks of the language only, counted per file, with the line of the fence */
    CHECK(scan(&b, "a.md", "x\n```blitzbasic\nNPrint 1\n```\n```c\nint x;\n```\n```blitzbasic\ny=2\nz=3\n```\n") == 0);
    REQUIRE(b.n == 2);
    CHECK(!strcmp(b.v[0].file, "a.md"));
    CHECK(b.v[0].index == 1);
    CHECK(b.v[0].line == 2);
    CHECK(!strcmp(b.v[0].text, "NPrint 1\n"));
    CHECK(b.v[1].index == 2);
    CHECK(b.v[1].line == 8);
    CHECK(!strcmp(b.v[1].text, "y=2\nz=3\n"));
    se_blocks_free(&b);

    /* the numbering starts again in the next file; blocks are added */
    CHECK(scan(&b, "a.md", "```blitzbasic\na\n```\n") == 0);
    CHECK(scan(&b, "b.md", "```blitzbasic\nb\n```\n") == 0);
    REQUIRE(b.n == 2);
    CHECK(b.v[0].index == 1);
    CHECK(b.v[1].index == 1);
    CHECK(!strcmp(b.v[1].file, "b.md"));
    se_blocks_free(&b);

    /* indented (YAML): the indentation of the fence off, less where a line has less;
       an empty block; a CR before the newline; an attribute behind the language */
    CHECK(scan(&b, "c.yaml", "intro: |\n  ```blitzbasic\n  a\n    b\n c\n\n  ```\n") == 0);
    REQUIRE(b.n == 1);
    CHECK(!strcmp(b.v[0].text, "a\n  b\nc\n\n"));
    se_blocks_free(&b);
    CHECK(scan(&b, "d.md", "```blitzbasic\n```\n```blitzbasic {x}\r\nq\r\n```\r\n") == 0);
    REQUIRE(b.n == 2);
    CHECK(!strcmp(b.v[0].text, ""));
    CHECK(!strcmp(b.v[1].text, "q\n"));
    se_blocks_free(&b);

    /* no match: another language that begins the same, text in the line, no fence */
    CHECK(scan(&b, "e.md", "```blitzbasic2\nx\n```\ntext ```blitzbasic\n``` blitzbasic\nBlitz\n") == 0);
    CHECK(b.n == 0);
    se_blocks_free(&b);
    CHECK(scan(&b, "e.md", "") == 0);
    CHECK(b.n == 0);
    se_blocks_free(&b);

    /* a block without its closing fence: the file and the line of its fence */
    CHECK(scan(&b, "f.md", "t\n\n```blitzbasic\nx=1\n") == -1);
    CHECK(!strcmp(err, "f.md: line 3: the block has no closing fence"));
    se_blocks_free(&b);
    /* the closing fence is only a fence: ``` followed by text is a line of the block */
    CHECK(scan(&b, "g.md", "```blitzbasic\n```x\n```\n") == 0);
    REQUIRE(b.n == 1);
    CHECK(!strcmp(b.v[0].text, "```x\n"));
    se_blocks_free(&b);
}

TEST_CASE("siteexamples manifest", "[siteexamples]")
{
    se_manifest m = {};

    CHECK(parse(&m, "# comment\n\n  a-1\trun   x/y.md#1  \nb-2 compile x/y.md#12 opens a screen, waits\r\n") == 0);
    REQUIRE(m.n == 2);
    CHECK(!strcmp(m.v[0].name, "a-1"));
    CHECK(m.v[0].mode == SE_RUN);
    CHECK(!strcmp(m.v[0].file, "x/y.md"));
    CHECK(m.v[0].index == 1);
    CHECK(!strcmp(m.v[0].reason, ""));
    CHECK(m.v[0].line == 3);
    CHECK(m.v[1].mode == SE_COMPILE);
    CHECK(m.v[1].index == 12);
    CHECK(!strcmp(m.v[1].reason, "opens a screen, waits"));
    CHECK(se_count(&m, SE_RUN) == 1);
    CHECK(se_count(&m, SE_COMPILE) == 1);
    se_manifest_free(&m);

    /* rwe= words: libraries before the reason, none otherwise */
    CHECK(parse(&m, "a run x#1 rwe=risortlib rwe=ridoslib\nb compile x#2 rwe=rimisclib opens a screen\nc compile x#3 the rwe= in the reason\n") == 0);
    REQUIRE(m.n == 3);
    CHECK(!strcmp(m.v[0].libs, "risortlib,ridoslib"));
    CHECK(!strcmp(m.v[0].reason, ""));
    CHECK(!strcmp(m.v[1].libs, "rimisclib"));
    CHECK(!strcmp(m.v[1].reason, "opens a screen"));
    CHECK(!strcmp(m.v[2].libs, ""));
    CHECK(!strcmp(m.v[2].reason, "the rwe= in the reason"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a run x#1 rwe=\n") == -1);
    CHECK(strstr(err, "line 1: rwe= needs a library name"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a run x#1 rwe=Ab\n") == -1);
    CHECK(strstr(err, "rwe= needs a library name"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a compile x#1 rwe=risortlib\n") == -1);
    CHECK(strstr(err, "needs the reason"));
    se_manifest_free(&m);

    /* a # inside the file name: the last one separates the number */
    CHECK(parse(&m, "a run d/a#b.md#3\n") == 0);
    REQUIRE(m.n == 1);
    CHECK(!strcmp(m.v[0].file, "d/a#b.md"));
    CHECK(m.v[0].index == 3);
    se_manifest_free(&m);

    /* broken lines, each with its line number */
    CHECK(parse(&m, "\n\nA run x#1\n") == -1);
    CHECK(!strcmp(err, "line 3: the name must be lower case letters, digits and -"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a_b run x#1\n") == -1);
    CHECK(strstr(err, "line 1: the name"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a skip x#1\n") == -1);
    CHECK(!strcmp(err, "line 1: the mode is run or compile"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a runx x#1\n") == -1);
    CHECK(strstr(err, "the mode"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a compile x#1\n") == -1);
    CHECK(strstr(err, "needs the reason"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a run x#1 because\n") == -1);
    CHECK(strstr(err, "a run entry has no reason"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a run x\n") == -1);
    CHECK(strstr(err, "FILE#N expected"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a run #1\n") == -1);
    CHECK(strstr(err, "FILE#N expected"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a run x#0\n") == -1);
    CHECK(strstr(err, "the block number"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a run x#2y\n") == -1);
    CHECK(strstr(err, "the block number"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a run\n") == -1);
    CHECK(strstr(err, "NAME MODE FILE#N"));
    se_manifest_free(&m);
    CHECK(parse(&m, "a run x#1\nb run y#1\na run z#1\n") == -1);
    CHECK(!strcmp(err, "line 3: the name a is used in line 1"));
    se_manifest_free(&m);
}

TEST_CASE("siteexamples check", "[siteexamples]")
{
    se_blocks b = {};
    se_manifest m = {};

    REQUIRE(scan(&b, "a.md", "```blitzbasic\n1\n```\n\n```blitzbasic\n2\n```\n") == 0);
    REQUIRE(scan(&b, "b.md", "```blitzbasic\n3\n```\n") == 0);

    /* equal: also in another order */
    REQUIRE(parse(&m, "x run b.md#1\ny run a.md#2\nz compile a.md#1 why\n") == 0);
    CHECK(se_check(&m, &b, err, sizeof err) == 0);
    REQUIRE(se_find(&b, &m.v[0]));
    CHECK(!strcmp(se_find(&b, &m.v[0])->text, "3\n"));
    CHECK(!strcmp(se_find(&b, &m.v[1])->text, "2\n"));
    se_manifest_free(&m);

    /* a block without an entry (the line of its fence is in the message) */
    REQUIRE(parse(&m, "x run b.md#1\nz compile a.md#1 why\n") == 0);
    CHECK(se_check(&m, &b, err, sizeof err) == -1);
    CHECK(strstr(err, "a.md#2 (line 5): the example has no entry in the manifest"));
    se_manifest_free(&m);

    /* an entry without a block: the number past the file's blocks, another file */
    REQUIRE(parse(&m, "x run b.md#1\ny run a.md#2\nz compile a.md#1 why\nw run a.md#3\n") == 0);
    CHECK(se_check(&m, &b, err, sizeof err) == -1);
    CHECK(strstr(err, "manifest line 4: w: the site has no block a.md#3"));
    se_manifest_free(&m);
    REQUIRE(parse(&m, "x run c.md#1\n") == 0);
    CHECK(se_check(&m, &b, err, sizeof err) == -1);
    CHECK(strstr(err, "manifest line 1: x: the site has no block c.md#1"));
    CHECK(!se_find(&b, &m.v[0]));
    se_manifest_free(&m);

    /* a block listed twice */
    REQUIRE(parse(&m, "x run b.md#1\ny run a.md#2\nz compile a.md#1 why\nv run b.md#1\n") == 0);
    CHECK(se_check(&m, &b, err, sizeof err) == -1);
    CHECK(strstr(err, "manifest line 4: b.md#1 is already listed in line 1"));
    se_manifest_free(&m);

    /* nothing at all: every block lacks its entry */
    CHECK(se_check(&m, &b, err, sizeof err) == -1);
    CHECK(strstr(err, "a.md#1 (line 1)"));
    se_blocks_free(&b);
    CHECK(se_check(&m, &b, err, sizeof err) == 0);
}
