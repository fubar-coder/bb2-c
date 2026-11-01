/*
 * test_outmatch.c - outputs against expect files built in memory: literal
 * lines, '~' and '+' patterns, escapes, sections, line ends, and outputs
 * that must not match (a changed, missing or extra line) and broken
 * expect files.
 */
#include "testutil.h"
#include "outmatch.h"

static char msg[512];

static int m(const char *expect, const char *output)
{
    return outmatch(expect, output, NULL, msg, sizeof msg);
}

static int ms(const char *expect, const char *output, const char *section)
{
    return outmatch(expect, output, section, msg, sizeof msg);
}

#define L(p, s) outmatch_line(p, s, strlen(s))

static void test_patterns(void)
{
    CHECK(L("abc", "abc") && !L("abc", "abcd") && !L("abc", "ab"));
    CHECK(L("a*c", "ac") && L("a*c", "abbbc") && !L("a*c", "abcd"));
    CHECK(L("*", "") && L("*", "anything"));
    CHECK(L("freemem: #", "freemem: 136736472") && L("x #", "x -12"));
    CHECK(!L("freemem: #", "freemem: ") && !L("freemem: #", "freemem: 12a")
          && !L("x #", "x -"));
    CHECK(L("#.#", "1.25") && L("##", "12") && !L("##", "1"));
    CHECK(L("a\\*b", "a*b") && !L("a\\*b", "axb") && L("\\#", "#"));
    CHECK(!L("a\\", "a"));
}

static void test_match(void)
{
    /* literal lines; CR LF; a missing last LF */
    CHECK(m("a\nb\n", "a\nb\n") == 0 && msg[0] == 0);
    CHECK(m("a\nb\n", "a\r\nb") == 0);
    CHECK(m("", "") == 0);
    /* an empty line is a line */
    CHECK(m("a\n\nb\n", "a\n\nb\n") == 0);
    CHECK(m("a\n\nb\n", "a\nb\n") == 1);
    /* ~: one line; +: one or more */
    CHECK(m("start\n~freemem: #\nend\n", "start\nfreemem: 4711\nend\n") == 0);
    CHECK(m("+#\nrc=0\n", "0\n0\n1\n17\nrc=0\n") == 0);
    CHECK(m("+#\nrc=0\n", "rc=0\n") == 1);
    CHECK(m("+#\n", "1\n2\nx\n") == 1);
    /* escapes of the first character */
    CHECK(m("\\~x\n\\+y\n\\\\z\n", "~x\n+y\n\\z\n") == 0);
    CHECK(m("\\~x\n", "~xx\n") == 1);
}

static void test_mismatch(void)
{
    /* a changed line: both line numbers in the message */
    CHECK(m("a\nb\nc\n", "a\nB\nc\n") == 1);
    CHECK(strstr(msg, "expect line 2: \"b\"") && strstr(msg, "output line 2: \"B\""));
    /* a missing and an extra line */
    CHECK(m("a\nb\n", "a\n") == 1 && strstr(msg, "output line 2: (none)"));
    CHECK(m("a\n", "a\nb\n") == 1 && strstr(msg, "expect line 2: (none)")
          && strstr(msg, "output line 2: \"b\""));
    /* the pattern of a + line does not match the first line */
    CHECK(m("x\n+#\n", "x\ny\n") == 1 && strstr(msg, "expect line 2"));
    /* control characters are shown as '?' */
    CHECK(m("a\n", "\x1b[1m\n") == 1 && strstr(msg, "\"?[1m\""));
}

static void test_sections(void)
{
    static const char expect[] = "[compile]\nrc=0\n[run]\n10\n~rc=#\n";

    CHECK(ms(expect, "[compile]\nrc=0\n[run]\n10\nrc=0\n", NULL) == 0);
    /* only [run]: the compile part of the output may be anything */
    CHECK(ms(expect, "[compile]\nlots of\nlog lines\nrc=5\n[run]\n10\nrc=0\n", "run") == 0);
    CHECK(ms(expect, "[compile]\nrc=0\n[run]\n11\nrc=0\n", "run") == 1
          && strstr(msg, "expect line 4: \"10\"") && strstr(msg, "output line 4: \"11\""));
    CHECK(ms(expect, "[compile]\nrc=0\n[run]\n10\nrc=0\n", "compile") == 0);
    CHECK(ms(expect, "[compile]\nrc=10\n[run]\n10\nrc=0\n", "compile") == 1);
    /* the section is missing in the output: no match; in the expect file:
       broken */
    CHECK(ms(expect, "[compile]\nrc=0\n", "run") == 1 && strstr(msg, "no section [run] in the output"));
    CHECK(ms(expect, "[link]\n", "link") == -1 && strstr(msg, "expect file"));
    /* "[a b]" is no section line */
    CHECK(ms("[run]\n[a b]\n", "[run]\n[a b]\n", "run") == 0);
}

static void test_broken(void)
{
    CHECK(m("~\n", "x\n") == -1 && strstr(msg, "without a pattern"));
    CHECK(m("a\n+\n", "a\nb\n") == -1 && strstr(msg, "expect line 2"));
}

int main(void)
{
    test_patterns();
    test_match();
    test_mismatch();
    test_sections();
    test_broken();
    if (failures)
        fprintf(stderr, "%d check(s) failed\n", failures);
    return failures != 0;
}
