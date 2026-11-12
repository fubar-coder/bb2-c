/*
 * test_libscmp.c - the judge of the library tests (tools/lib/libscmp) with
 * files built in memory: equal files, a changed value, a case less and
 * more, a changed block and long buffer, the qualifiers per set, not
 * captured, the escaping, malformed, duplicate and ambiguous files, the
 * limit of the findings shown, the directory compare with --only, and two
 * files of 3 million cases.
 */
#define _DEFAULT_SOURCE
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "testutil.h"
#include "libscmp.h"

#define HDR "# capture prog 1\n"

static char *report;            /* the last compare's report */
static size_t reportlen;
static char err[700];           /* the last parse error */

/* parse text as kind into f; the result of libscmp_parse */
static int parse(struct libscmp_file *f, const char *text, enum libscmp_kind kind)
{
    return libscmp_parse(f, text, strlen(text), "f.txt", kind, err, sizeof err);
}

/* the parse error of an expected (or run) file, "" if it parses */
static const char *bad(const char *text, enum libscmp_kind kind)
{
    struct libscmp_file f;
    int r = parse(&f, text, kind);

    libscmp_free(&f);
    if (!r)
        return "";
    CHECK(r == -2);
    return err;
}

/* compare run with expected for set; the status, the report in report */
static int cmp(const char *run, const char *exp, enum libscmp_set set, struct libscmp_stats *st)
{
    struct libscmp_file r, e;
    FILE *m;
    int s;

    free(report);
    report = NULL;
    CHECK(parse(&r, run, LIBSCMP_RUN) == 0);
    CHECK(parse(&e, exp, LIBSCMP_EXPECTED) == 0);
    m = open_memstream(&report, &reportlen);
    s = libscmp_compare(&r, &e, set, m, st);
    fclose(m);
    libscmp_free(&r);
    libscmp_free(&e);
    return s;
}

#define RUN HDR "ldivlib|ldiv d0|$00000007 $00000002|1\t$00000003\n" \
    "ldivlib|ldiv d0|$00000007 $00000002|2\t$00000003\n" \
    "ldivlib|regs||1\tkept\n" \
    "|buf|big|1\tbytes 100 fnv 1A2B3C4D\n" \
    "BLOCK:17\t4096 0BADF00D first=ldiv d0|$00000001\n"

static void test_equal(void)
{
    struct libscmp_stats st;

    CHECK(cmp(RUN, RUN, LIBSCMP_OBJ, &st) == 0 && st.cases == 5 && st.equal == 5);
    CHECK(strstr(report, "prog: set obj: 5 cases, 5 equal, 0 differ, 0 missing, 0 added"));
    CHECK(cmp(RUN, RUN, LIBSCMP_ORIG, &st) == 0 && st.equal == 5);
    /* order does not count; the last line may lack its newline */
    CHECK(cmp(HDR "a|b|c|2\t$1\na|b|c|1\t$0", HDR "a|b|c|1\t$0\na|b|c|2\t$1\n", LIBSCMP_OBJ, NULL) == 0);
    /* a header without cases */
    CHECK(cmp(HDR, "# capture prog 1", LIBSCMP_OBJ, &st) == 0 && st.cases == 0);
}

static void test_changed(void)
{
    struct libscmp_stats st;

    CHECK(cmp(HDR "ldivlib|ldiv d0|$00000007 $00000002|1\t$00000004\n",
              HDR "ldivlib|ldiv d0|$00000007 $00000002|1\t$00000003\n", LIBSCMP_OBJ, &st) == 1);
    CHECK(st.differ == 1 && st.equal == 0);
    CHECK(strstr(report, "prog: ldivlib|ldiv d0|$00000007 $00000002|1: expected $00000003, got $00000004\n"));
    /* the second occurrence is its own case */
    CHECK(cmp(HDR "r|c|v|1\t$1\nr|c|v|2\t$3\n", HDR "r|c|v|1\t$1\nr|c|v|2\t$2\n", LIBSCMP_OBJ, &st) == 1);
    CHECK(st.differ == 1 && st.equal == 1 && strstr(report, "prog: r|c|v|2: expected $2, got $3\n"));
    /* a lost register */
    CHECK(cmp(HDR "r|regs||1\tlost d3 a1\n", HDR "r|regs||1\tkept\n", LIBSCMP_OBJ, NULL) == 1);
    CHECK(strstr(report, "r|regs||1: expected kept, got lost d3 a1\n"));
    /* a long buffer: key and both hashes */
    CHECK(cmp(HDR "|buf|big|1\tbytes 100 fnv 1A2B3C4E\n", HDR "|buf|big|1\tbytes 100 fnv 1A2B3C4D\n",
              LIBSCMP_OBJ, NULL) == 1);
    CHECK(strstr(report, "prog: |buf|big|1: expected bytes 100 fnv 1A2B3C4D, got bytes 100 fnv 1A2B3C4E\n"));
}

static void test_missing_added(void)
{
    struct libscmp_stats st;

    CHECK(cmp(HDR "a|b|c|1\t$0\n", HDR "a|b|c|1\t$0\na|b|c|2\t$1\n", LIBSCMP_OBJ, &st) == 1);
    CHECK(st.missing == 1 && st.equal == 1 && strstr(report, "prog: a|b|c|2: missing (expected $1)\n"));
    CHECK(cmp(HDR "a|b|c|1\t$0\na|b|c|2\t$1\n", HDR "a|b|c|1\t$0\n", LIBSCMP_OBJ, &st) == 1);
    CHECK(st.added == 1 && strstr(report, "prog: a|b|c|2: added (got $1)\n"));
}

static void test_block(void)
{
    /* the block and its observations 17*4096 .. */
    CHECK(cmp(HDR "BLOCK:17\t4096 0BADF00E first=ldiv d0|$00000001\n",
              HDR "BLOCK:17\t4096 0BADF00D first=ldiv d0|$00000001\n", LIBSCMP_OBJ, NULL) == 1);
    CHECK(strstr(report, "prog: BLOCK:17 (observations 69632..73727): expected 4096 0BADF00D "
                         "first=ldiv d0|$00000001, got 4096 0BADF00E first=ldiv d0|$00000001\n"));
    /* the last block, shorter */
    CHECK(cmp(HDR "BLOCK:0\t3 00000001 first=x\n", HDR "BLOCK:0\t3 00000002 first=x\n",
              LIBSCMP_OBJ, NULL) == 1 && strstr(report, "BLOCK:0 (observations 0..2): expected"));
    /* a block missing (the run has fewer observations) */
    CHECK(cmp(HDR "BLOCK:0\t4096 1 first=x\n", HDR "BLOCK:0\t4096 1 first=x\nBLOCK:1\t5 2 first=y\n",
              LIBSCMP_OBJ, NULL) == 1);
    CHECK(strstr(report, "prog: BLOCK:1 (observations 4096..4100): missing (expected 5 2 first=y)\n"));
    /* a value without the count: no range */
    CHECK(cmp(HDR "BLOCK:2\tx\n", HDR "BLOCK:2\ty\n", LIBSCMP_OBJ, NULL) == 1
          && strstr(report, "prog: BLOCK:2: expected y, got x\n"));
}

/* the qualifiers: KEY [orig] for the original, KEY [fixed #7] for the port */
#define PAIR HDR "r|find d0||1\t$0\nr|find d0||2 [orig]\t$00000001\nr|find d0||2 [fixed #7]\t$00000000\n"

static void test_qualifiers(void)
{
    struct libscmp_stats st;

    /* the port: compared with [fixed #7], [orig] ignored */
    CHECK(cmp(HDR "r|find d0||1\t$0\nr|find d0||2\t$00000000\n", PAIR, LIBSCMP_OBJ, &st) == 0
          && st.equal == 2);
    /* ... a regressed port fails on the [fixed #7] case */
    CHECK(cmp(HDR "r|find d0||1\t$0\nr|find d0||2\t$00000001\n", PAIR, LIBSCMP_OBJ, NULL) == 1);
    CHECK(strstr(report, "prog: r|find d0||2 [fixed #7]: expected $00000000, got $00000001\n"));
    /* the original: compared with [orig], [fixed #7] ignored */
    CHECK(cmp(HDR "r|find d0||1\t$0\nr|find d0||2\t$00000001\n", PAIR, LIBSCMP_ORIG, &st) == 0
          && st.equal == 2);
    /* ... a changed original fails on the [orig] case */
    CHECK(cmp(HDR "r|find d0||1\t$0\nr|find d0||2\t$00000002\n", PAIR, LIBSCMP_ORIG, NULL) == 1);
    CHECK(strstr(report, "prog: r|find d0||2 [orig]: expected $00000001, got $00000002\n"));

    /* a case only in the port: [fixed #8] alone */
    {
        static const char exp[] = HDR "a|b||1\t$0\na|new||1 [fixed #8]\t$5\n";

        CHECK(cmp(HDR "a|b||1\t$0\na|new||1\t$5\n", exp, LIBSCMP_OBJ, NULL) == 0);
        CHECK(cmp(HDR "a|b||1\t$0\n", exp, LIBSCMP_OBJ, NULL) == 1
              && strstr(report, "prog: a|new||1 [fixed #8]: missing (expected $5)\n"));
        CHECK(cmp(HDR "a|b||1\t$0\n", exp, LIBSCMP_ORIG, NULL) == 0);
        CHECK(cmp(HDR "a|b||1\t$0\na|new||1\t$5\n", exp, LIBSCMP_ORIG, NULL) == 1
              && strstr(report, "prog: a|new||1: added (got $5)\n"));
    }
    /* a case only in the original: [orig] alone */
    {
        static const char exp[] = HDR "a|b||1\t$0\na|old||1 [orig]\t$5\n";

        CHECK(cmp(HDR "a|b||1\t$0\na|old||1\t$5\n", exp, LIBSCMP_ORIG, NULL) == 0);
        CHECK(cmp(HDR "a|b||1\t$0\n", exp, LIBSCMP_ORIG, NULL) == 1
              && strstr(report, "prog: a|old||1 [orig]: missing (expected $5)\n"));
        CHECK(cmp(HDR "a|b||1\t$0\n", exp, LIBSCMP_OBJ, NULL) == 0);
        CHECK(cmp(HDR "a|b||1\t$0\na|old||1\t$5\n", exp, LIBSCMP_OBJ, NULL) == 1
              && strstr(report, "prog: a|old||1: added (got $5)\n"));
    }
    /* qualified blocks */
    CHECK(cmp(HDR "BLOCK:3\t4096 2 first=x\n",
              HDR "BLOCK:3 [orig]\t4096 1 first=x\nBLOCK:3 [fixed #9]\t4096 2 first=x\n",
              LIBSCMP_OBJ, NULL) == 0);
    CHECK(cmp(HDR "BLOCK:3\t4096 2 first=x\n",
              HDR "BLOCK:3 [orig]\t4096 1 first=x\nBLOCK:3 [fixed #9]\t4096 2 first=x\n",
              LIBSCMP_ORIG, NULL) == 1);
    CHECK(strstr(report, "prog: BLOCK:3 [orig] (observations 12288..16383): expected 4096 1 first=x, "
                         "got 4096 2 first=x\n"));
}

static void test_not_captured(void)
{
    struct libscmp_stats st;

    /* skipped on both sides, whatever the reasons */
    CHECK(cmp(HDR "e|wheel||1\tnot captured: Rnd of vallib\n",
              HDR "e|wheel||1\tnot captured: Rnd of vallib takes the wheels from hardware timing\n",
              LIBSCMP_OBJ, &st) == 0 && st.skipped == 1 && st.equal == 0);
    CHECK(strstr(report, "0 added, 1 not captured\n"));
    CHECK(cmp(HDR "e|wheel||1\tnot captured: x\n", HDR "e|wheel||1\tnot captured: x\n", LIBSCMP_ORIG,
              &st) == 0 && st.skipped == 1);
    /* a value where the expected data has none, and the other way */
    CHECK(cmp(HDR "e|wheel||1\t$00000001\n", HDR "e|wheel||1\tnot captured: x\n", LIBSCMP_OBJ, NULL) == 1);
    CHECK(strstr(report, "prog: e|wheel||1: expected not captured: x, got $00000001\n"));
    CHECK(cmp(HDR "e|wheel||1\tnot captured: x\n", HDR "e|wheel||1\t$00000001\n", LIBSCMP_OBJ, NULL) == 1);
    /* the key still counts */
    CHECK(cmp(HDR, HDR "e|wheel||1\tnot captured: x\n", LIBSCMP_OBJ, NULL) == 1
          && strstr(report, "e|wheel||1: missing"));
    CHECK(strncmp("not captured:x", "not captured: ", 14));
    CHECK(cmp(HDR "e|w||1\tnot captured:x\n", HDR "e|w||1\tnot captured:y\n", LIBSCMP_OBJ, NULL) == 1);
}

static void test_escaping(void)
{
    struct libscmp_file f;

    /* TAB, '|', '\' and non-printable bytes in the parts */
    CHECK(!*bad(HDR "r\\|x|c\\tTAB|v\\\\\\x00\\x1F\\x7F\\xFF|1\t$0\n", LIBSCMP_EXPECTED));
    /* an escaped '|' is not a separator: 4 parts, 2 cases */
    CHECK(parse(&f, HDR "a\\|b|c|d|1\t$0\na|b\\|c|d|1\t$1\n", LIBSCMP_RUN) == 0 && f.ncases == 2
          && f.nentries == 2);
    CHECK(libscmp_find(&f, "a\\|b|c|d|1", 10) && libscmp_find(&f, "a|b\\|c|d|1", 10)
          && !libscmp_find(&f, "a|b|c|d|1", 9));
    libscmp_free(&f);
    /* the escaped key appears in messages as it is written */
    CHECK(cmp(HDR "r\\|x|c\\t|\\x01|1\t$1\n", HDR "r\\|x|c\\t|\\x01|1\t$0\n", LIBSCMP_OBJ, NULL) == 1
          && strstr(report, "prog: r\\|x|c\\t|\\x01|1: expected $0, got $1\n"));
    /* spaces and other printables are written as they are */
    CHECK(!*bad(HDR "r|SAME(\"x\") [2]|VEC=a b #1 [orig]|1\t\"text \\x00\"\n", LIBSCMP_EXPECTED));
    /* the broken escapes */
    CHECK(strstr(bad(HDR "r|c\\n|v|1\t$0\n", LIBSCMP_RUN), "f.txt:2: unknown escape \\n"));
    CHECK(strstr(bad(HDR "r|c\\x41|v|1\t$0\n", LIBSCMP_RUN), "f.txt:2: needless escape \\x41"));
    CHECK(strstr(bad(HDR "r|c\\x09|v|1\t$0\n", LIBSCMP_RUN), "f.txt:2: \\x09 for a TAB"));
    CHECK(strstr(bad(HDR "r|c\\x1f|v|1\t$0\n", LIBSCMP_RUN), "f.txt:2: \\x needs two upper-case hex"));
    CHECK(strstr(bad(HDR "r|c\\x1|v|1\t$0\n", LIBSCMP_RUN), "f.txt:2: \\x needs two upper-case hex"));
    CHECK(strstr(bad(HDR "r|c|v\\", LIBSCMP_RUN), "f.txt:2: a '\\' at the end of the line"));
    /* raw bytes that need an escape */
    CHECK(strstr(bad(HDR "r|c\x01|v|1\t$0\n", LIBSCMP_RUN), "f.txt:2: byte $01 not escaped"));
    CHECK(strstr(bad(HDR "r|c\xC4|v|1\t$0\n", LIBSCMP_RUN), "f.txt:2: byte $C4 not escaped"));
    CHECK(strstr(bad(HDR "r|c\r|v|1\t$0\n", LIBSCMP_RUN), "f.txt:2: byte $0D not escaped"));
    CHECK(strstr(bad(HDR "r|c|v|1\t$0\r\n", LIBSCMP_RUN), "f.txt:2: byte $0D in the value"));
    CHECK(strstr(bad(HDR "r|c|v|1\t$0\t$1\n", LIBSCMP_RUN), "f.txt:2: byte $09 in the value"));
    CHECK(strstr(bad(HDR "r|c|v|1\t\x80\n", LIBSCMP_RUN), "f.txt:2: byte $80 in the value"));
    /* a TAB inside the key ends it too early */
    CHECK(strstr(bad(HDR "r|c\tx|v|1\t$0\n", LIBSCMP_RUN), "f.txt:2: the key needs ROUTINE|CHECK|VECTOR|OCC"));
}

static void test_malformed(void)
{
    struct libscmp_file f;
    char big[200];

    /* the header */
    CHECK(strstr(bad("", LIBSCMP_RUN), "f.txt: empty"));
    CHECK(strstr(bad("prog|a|b|1\t$0\n", LIBSCMP_RUN), "f.txt:1: no header"));
    CHECK(strstr(bad("# capture prog 2\n", LIBSCMP_RUN), "f.txt:1: format version 2"));
    CHECK(strstr(bad("# capture prog\n", LIBSCMP_RUN), "f.txt:1: the header needs a NAME"));
    CHECK(strstr(bad("# capture  1\n", LIBSCMP_RUN), "f.txt:1: the header needs a NAME"));
    CHECK(strstr(bad("# capture pr/og 1\n", LIBSCMP_RUN), "f.txt:1: the header needs a NAME"));
    snprintf(big, sizeof big, "# capture %070d 1\n", 0);
    CHECK(strstr(bad(big, LIBSCMP_RUN), "f.txt:1: the header needs a NAME"));
    CHECK(parse(&f, "# capture libtest_ldivlib 1\n", LIBSCMP_RUN) == 0 && !strcmp(f.name, "libtest_ldivlib"));
    libscmp_free(&f);
    CHECK(strstr(bad(HDR "a|b|c|1\t$0\n" HDR, LIBSCMP_RUN), "f.txt:3: a second header"));
    /* lines */
    CHECK(strstr(bad(HDR "a|b|c|1\t$0\n\na|b|c|2\t$0\n", LIBSCMP_RUN), "f.txt:3: empty line"));
    CHECK(strstr(bad(HDR "a|b|c|1\t$0\n\n", LIBSCMP_RUN), "f.txt:3: empty line"));
    CHECK(strstr(bad(HDR "a|b|1\t$0\n", LIBSCMP_RUN), "f.txt:2: the key needs"));
    CHECK(strstr(bad(HDR "a|b|c|0\t$0\n", LIBSCMP_RUN), "f.txt:2: the occurrence number"));
    CHECK(strstr(bad(HDR "a|b|c|01\t$0\n", LIBSCMP_RUN), "f.txt:2: the occurrence number"));
    CHECK(strstr(bad(HDR "a|b|c|\t$0\n", LIBSCMP_RUN), "f.txt:2: the occurrence number"));
    CHECK(strstr(bad(HDR "a|b|c|1x\t$0\n", LIBSCMP_RUN), "f.txt:2: no TAB after the key"));
    CHECK(strstr(bad(HDR "a|b|c|d|1\t$0\n", LIBSCMP_RUN), "f.txt:2: the occurrence number"));
    CHECK(strstr(bad(HDR "a|b|c|1\n", LIBSCMP_RUN), "f.txt:2: no TAB after the key"));
    CHECK(strstr(bad(HDR "a|b|c|1\t\n", LIBSCMP_RUN), "f.txt:2: no value"));
    CHECK(strstr(bad(HDR "BLOCK:01\t1 x\n", LIBSCMP_RUN), "f.txt:2: BLOCK:N needs a decimal N"));
    CHECK(libscmp_parse(&f, HDR "a|b|c|1\t$0\na\0|b|c|1\t$0\n", sizeof HDR + 22, "f.txt",
                        LIBSCMP_RUN, err, sizeof err) == -2 && strstr(err, "f.txt:3: a 0 byte"));
    libscmp_free(&f);
    /* the qualifiers */
    CHECK(strstr(bad(HDR "a|b|c|1 [orig]\t$0\n", LIBSCMP_RUN), "f.txt:2: a qualifier in a run's output"));
    CHECK(strstr(bad(HDR "a|b|c|1 [new]\t$0\n", LIBSCMP_EXPECTED), "f.txt:2: unknown qualifier"));
    CHECK(strstr(bad(HDR "a|b|c|1 [Orig]\t$0\n", LIBSCMP_EXPECTED), "f.txt:2: unknown qualifier"));
    CHECK(strstr(bad(HDR "a|b|c|1 [fixed #]\t$0\n", LIBSCMP_EXPECTED), "f.txt:2: [fixed #N] needs"));
    CHECK(strstr(bad(HDR "a|b|c|1 [fixed #07]\t$0\n", LIBSCMP_EXPECTED), "f.txt:2: [fixed #N] needs"));
    CHECK(strstr(bad(HDR "a|b|c|1 [fixed 7]\t$0\n", LIBSCMP_EXPECTED), "f.txt:2: unknown qualifier"));
    CHECK(strstr(bad(HDR "a|b|c|1 [fixed #7] \t$0\n", LIBSCMP_EXPECTED), "f.txt:2: no TAB after the key"));
    CHECK(strstr(bad(HDR "a|b|c|1  [orig]\t$0\n", LIBSCMP_EXPECTED), "f.txt:2: unknown qualifier"));
    CHECK(!*bad(HDR "a|b|c|1 [orig]\t$0\na|b|c|1 [fixed #12345]\t$1\n", LIBSCMP_EXPECTED));
    /* duplicate keys */
    CHECK(strstr(bad(HDR "a|b|c|1\t$0\nx|y|z|1\t$0\na|b|c|1\t$1\n", LIBSCMP_RUN),
                 "f.txt:4: duplicate key a|b|c|1 (first at line 2)"));
    CHECK(strstr(bad(HDR "a|b|c|1 [orig]\t$0\na|b|c|1 [orig]\t$1\n", LIBSCMP_EXPECTED),
                 "f.txt:3: duplicate key a|b|c|1 [orig] (first at line 2)"));
    CHECK(strstr(bad(HDR "a|b|c|1 [fixed #1]\t$0\na|b|c|1 [fixed #2]\t$1\n", LIBSCMP_EXPECTED),
                 "f.txt:3: duplicate key a|b|c|1 [fixed #1] (first at line 2)"));
    CHECK(strstr(bad(HDR "BLOCK:4\t1 x\nBLOCK:4\t1 y\n", LIBSCMP_RUN), "f.txt:3: duplicate key BLOCK:4"));
    /* ambiguous: unqualified and qualified */
    CHECK(strstr(bad(HDR "a|b|c|1\t$0\na|b|c|1 [orig]\t$1\n", LIBSCMP_EXPECTED),
                 "f.txt:3: a|b|c|1 is unqualified and qualified (the other at line 2)"));
    CHECK(strstr(bad(HDR "a|b|c|1 [fixed #3]\t$0\na|b|c|1\t$1\n", LIBSCMP_EXPECTED),
                 "f.txt:3: a|b|c|1 is unqualified and qualified (the other at line 2)"));
}

/* more than LIBSCMP_DETAILS findings: the first ones, then the count */
static void test_details(void)
{
    char run[2000] = HDR, exp[2000] = HDR;
    struct libscmp_stats st;
    int i;

    for (i = 1; i <= 20; i++) {
        snprintf(run + strlen(run), sizeof run - strlen(run), "r|c|v|%d\t$1\n", i);
        snprintf(exp + strlen(exp), sizeof exp - strlen(exp), "r|c|v|%d\t$0\n", i);
    }
    CHECK(cmp(run, exp, LIBSCMP_OBJ, &st) == 1 && st.differ == 20);
    CHECK(strstr(report, "prog: r|c|v|1: expected $0, got $1\n") && strstr(report, "r|c|v|11:")
          && !strstr(report, "r|c|v|12:") && strstr(report, "prog: ... 9 more\n"));
}

/* a file in dir as NAME.txt */
static void put_file(const char *dir, const char *name, const char *text)
{
    char path[300];
    FILE *f;

    snprintf(path, sizeof path, "%s/%s.txt", dir, name);
    f = fopen(path, "wb");
    CHECK(f && fwrite(text, 1, strlen(text), f) == strlen(text));
    if (f)
        fclose(f);
}

static int cmp_dirs(const char *only1, const char *only2, char **errtext)
{
    const char *only[2] = { only1, only2 };
    size_t nonly = only2 ? 2 : only1 ? 1 : 0, errlen;
    FILE *m, *e;
    int s;

    free(report);
    report = NULL;
    m = open_memstream(&report, &reportlen);
    e = open_memstream(errtext, &errlen);
    s = libscmp_compare_dirs("libscmp-dirs/run", "libscmp-dirs/exp", LIBSCMP_OBJ, only, nonly, m, e);
    fclose(m);
    fclose(e);
    return s;
}

/* directories: the programs of both, --only, files in one directory only,
   a header naming another program (in the build directory of the test) */
static void test_dirs(void)
{
    static const char *const files[] = { "run/a", "run/b", "run/d", "exp/a", "exp/b", "exp/c", "exp/d" };
    char *e = NULL, path[100];
    size_t i;

    for (i = 0; i < sizeof files / sizeof *files; i++) {    /* a run before */
        snprintf(path, sizeof path, "libscmp-dirs/%s.txt", files[i]);
        unlink(path);
    }
    mkdir("libscmp-dirs", 0777);
    mkdir("libscmp-dirs/run", 0777);
    mkdir("libscmp-dirs/exp", 0777);
    put_file("libscmp-dirs/run", "a", "# capture a 1\nr|c|v|1\t$0\n");
    put_file("libscmp-dirs/exp", "a", "# capture a 1\nr|c|v|1\t$0\n");
    put_file("libscmp-dirs/run", "b", "# capture b 1\nr|c|v|1\t$1\n");
    put_file("libscmp-dirs/exp", "b", "# capture b 1\nr|c|v|1\t$0\n");
    put_file("libscmp-dirs/exp", "c", "# capture c 1\nr|c|v|1\t$0\n");
    CHECK(cmp_dirs(NULL, NULL, &e) == 1);
    CHECK(strstr(report, "a: set obj: 1 cases, 1 equal") && strstr(report, "b: r|c|v|1: expected $0, got $1\n")
          && strstr(report, "c: no output of the run (libscmp-dirs/run/c.txt)\n")
          && strstr(report, "libs-compare: 3 programs, 1 equal, 2 differ, 0 not judged (set obj)\n"));
    free(e);
    /* --only: the others are neither compared nor missing */
    CHECK(cmp_dirs("a", NULL, &e) == 0 && !strstr(report, "b:") && !strstr(report, "c:")
          && strstr(report, "libs-compare: 1 programs, 1 equal, 0 differ"));
    free(e);
    CHECK(cmp_dirs("b", NULL, &e) == 1 && strstr(report, "b: r|c|v|1"));
    free(e);
    /* a named program without files, a bad name */
    CHECK(cmp_dirs("a", "zz", &e) == 2 && strstr(e, "zz: no output (libscmp-dirs/run/zz.txt) and no expected data"));
    free(e);
    CHECK(cmp_dirs("../a", NULL, &e) == 2 && strstr(e, "--only ../a: not a program name"));
    free(e);
    /* output without expected data */
    put_file("libscmp-dirs/run", "d", "# capture d 1\nr|c|v|1\t$0\n");
    CHECK(cmp_dirs("d", NULL, &e) == 1 && strstr(report, "d: no expected data (libscmp-dirs/exp/d.txt)\n"));
    free(e);
    /* a header naming another program, a malformed file */
    put_file("libscmp-dirs/exp", "d", "# capture a 1\nr|c|v|1\t$0\n");
    CHECK(cmp_dirs("d", NULL, &e) == 2
          && strstr(e, "libscmp-dirs/exp/d.txt:1: the header names a, the file is d.txt\n"));
    free(e);
    put_file("libscmp-dirs/exp", "d", "# capture d 1\nr|c|v|1\t$0\nr|c|v|1\t$0\n");
    CHECK(cmp_dirs("d", "a", &e) == 2 && strstr(e, "libscmp-dirs/exp/d.txt:3: duplicate key r|c|v|1")
          && strstr(report, "a: set obj: 1 cases, 1 equal"));
    free(e);
    /* a directory that cannot be read */
    {
        size_t elen;
        FILE *m, *x;

        free(report);
        m = open_memstream(&report, &reportlen);
        x = open_memstream(&e, &elen);

        CHECK(libscmp_compare_dirs("libscmp-dirs/none", "libscmp-dirs/exp", LIBSCMP_OBJ, NULL, 0, m, x) == 2);
        fclose(m);
        fclose(x);
        CHECK(strstr(e, "libscmp-dirs/none: cannot read the directory"));
        free(e);
    }
}

/* two files of 3 million cases each, one value changed: one pass, a few
   seconds */
static void test_large(void)
{
    enum { N = 3000000 };
    size_t cap = (size_t)N * 56 + 64, n;
    char *a = malloc(cap), *b = malloc(cap);
    struct libscmp_file r, e;
    struct libscmp_stats st;
    struct timespec t0, t1;
    double secs;
    FILE *m;
    int i;

    CHECK(a && b);
    if (!a || !b)
        return;
    n = (size_t)snprintf(a, cap, HDR);
    for (i = 0; i < N; i++)
        n += (size_t)snprintf(a + n, cap - n, "ridisasmlib|DisAssemble$ d0|$%08X|1\t$%08X\n",
                              (unsigned)i, (unsigned)(i * 7));
    memcpy(b, a, n + 1);
    b[n - 2] = b[n - 2] == '0' ? '1' : '0';     /* the last value */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    CHECK(libscmp_parse(&r, b, n, "run", LIBSCMP_RUN, err, sizeof err) == 0);
    free(b);
    CHECK(libscmp_parse(&e, a, n, "exp", LIBSCMP_EXPECTED, err, sizeof err) == 0);
    free(a);
    free(report);
    report = NULL;
    m = open_memstream(&report, &reportlen);
    CHECK(libscmp_compare(&r, &e, LIBSCMP_OBJ, m, &st) == 1);
    fclose(m);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    secs = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
    CHECK(st.cases == N && st.differ == 1 && st.equal == N - 1);
    CHECK(strstr(report, "|DisAssemble$ d0|$002DC6BF|1: expected"));
    printf("test_libscmp: two files of %d cases parsed and compared in %.2f s\n", N, secs);
    CHECK(secs < 30);           /* a few seconds; generous for the sanitizers */
    libscmp_free(&r);
    libscmp_free(&e);
}

/* --- libs-accept and libs-update --------------------------------------- */

static char *result;            /* the file written, NULL = refused */

/* accept two runs (old: the expected data so far, NULL = none) */
static int accept2(const char *run1, const char *run2, const char *old)
{
    struct libscmp_file a, b, o;
    FILE *m;
    int s;

    free(report);
    free(result);
    report = result = NULL;
    CHECK(parse(&a, run1, LIBSCMP_RUN) == 0);
    CHECK(parse(&b, run2, LIBSCMP_RUN) == 0);
    if (old)
        CHECK(parse(&o, old, LIBSCMP_EXPECTED) == 0);
    m = open_memstream(&report, &reportlen);
    s = libscmp_accept(&a, &b, old ? &o : NULL, &result, m);
    fclose(m);
    CHECK((s == 0) == (result != NULL));
    libscmp_free(&a);
    libscmp_free(&b);
    if (old)
        libscmp_free(&o);
    return s;
}

static void test_accept(void)
{
    static const char run[] = HDR "r|a||1\t$0\nr|find||1\t$1\nr|c||1\tkept\n";

    /* equal runs: the run's cases, unqualified */
    CHECK(accept2(run, run, NULL) == 0 && !strcmp(result, run));
    CHECK(strstr(report, "prog: 3 cases of the original, 0 [fixed #N] cases kept\n"));
    CHECK(accept2(run, run, HDR "r|a||1\t$5\nr|gone||1\t$5\n") == 0 && !strcmp(result, run));
    CHECK(strstr(report, "prog: r|gone||1: not in the original's run any more, removed\n"));
    /* a [fixed #N] case is kept, the original's value becomes [orig] */
    {
        static const char old[] = HDR "r|a||1\t$0\nr|find||1 [orig]\t$9\nr|find||1 [fixed #7]\t$0\n"
                                  "r|c||1\tkept\n";
        static const char want[] = HDR "r|a||1\t$0\nr|find||1 [orig]\t$1\nr|find||1 [fixed #7]\t$0\n"
                                   "r|c||1\tkept\n";

        CHECK(accept2(run, run, old) == 0 && !strcmp(result, want));
        CHECK(strstr(report, "1 [fixed #N] cases kept"));
        /* again on its own output: nothing changes */
        CHECK(accept2(run, run, want) == 0 && !strcmp(result, want));
    }
    /* a case of the port only stays after the case it followed; an [orig]
       case alone stays [orig] or goes when the run lacks it */
    {
        static const char old[] = HDR "r|new0||1 [fixed #3]\t$3\nr|a||1\t$0\nr|new||1 [fixed #8]\t$5\n"
                                  "r|find||1 [orig]\t$1\nr|old||1 [orig]\t$4\nr|c||1\tkept\n";
        static const char want[] = HDR "r|new0||1 [fixed #3]\t$3\nr|a||1\t$0\nr|new||1 [fixed #8]\t$5\n"
                                   "r|find||1 [orig]\t$1\nr|c||1\tkept\n";

        CHECK(accept2(run, run, old) == 0 && !strcmp(result, want));
        CHECK(strstr(report, "prog: r|new||1 [fixed #8]: not in the original's run, kept as a case "
                             "of the port only"));
        CHECK(strstr(report, "prog: r|old||1 [orig]: not in the original's run any more, removed\n"));
        CHECK(strstr(report, "2 [fixed #N] cases kept"));
    }
    /* the runs differ: nothing, and the differing cases */
    CHECK(accept2(run, HDR "r|a||1\t$0\nr|find||1\t$2\nr|c||1\tkept\n", NULL) == 1 && !result);
    CHECK(strstr(report, "test not deterministic: prog\n")
          && strstr(report, "prog: r|find||1: run 1 $1, run 2 $2\n") && !strstr(report, "r|a||1"));
    CHECK(accept2(run, HDR "r|a||1\t$0\nr|c||1\tkept\nr|x||1\t$0\n", NULL) == 1);
    CHECK(strstr(report, "prog: r|find||1: only in run 1 ($1)\n")
          && strstr(report, "prog: r|x||1: only in run 2 ($0)\n"));
    CHECK(accept2(run, HDR "r|find||1\t$1\nr|a||1\t$0\nr|c||1\tkept\n", NULL) == 1);
    CHECK(strstr(report, "test not deterministic: prog\nprog: the same cases in another order\n"));
    /* a lost register: refused */
    CHECK(accept2(HDR "r|a||1\t$0\nr|c||1\tlost d3 a1\n", HDR "r|a||1\t$0\nr|c||1\tlost d3 a1\n",
                  NULL) == 1);
    CHECK(strstr(report, "prog: r|c||1: lost d3 a1: the original does not keep a stated register")
          && strstr(report, "prog: refused: 1 cases lost\n"));
    CHECK(accept2(HDR "r|c||1\tlost\n", HDR "r|c||1\tlost\n", NULL) == 1);
    CHECK(accept2(HDR "r|c||1\tlostx\n", HDR "r|c||1\tlostx\n", NULL) == 0);
}

static struct libscmp_fix fixes[4];

/* update exp from run with the fixes "KEY#N" (NULL-terminated) */
static int update(const char *run, const char *exp, const char *const *fix)
{
    struct libscmp_file r, e;
    size_t n = 0;
    FILE *m;
    int s;

    free(report);
    free(result);
    report = result = NULL;
    for (n = 0; n < 4; n++)
        libscmp_fix_free(&fixes[n]);
    for (n = 0; fix && fix[n]; n++)
        CHECK(libscmp_parse_fix(fix[n], &fixes[n], err, sizeof err) == 0);
    CHECK(parse(&r, run, LIBSCMP_RUN) == 0);
    CHECK(parse(&e, exp, LIBSCMP_EXPECTED) == 0);
    m = open_memstream(&report, &reportlen);
    s = libscmp_update(&r, &e, fixes, n, &result, m);
    fclose(m);
    libscmp_free(&r);
    libscmp_free(&e);
    return s;
}

static void test_update(void)
{
    static const char exp[] = HDR "r|a||1\t$0\nr|find||1 [orig]\t$1\nr|find||1 [fixed #7]\t$0\n"
                              "r|old||1 [orig]\t$4\nr|c||1\tkept\n";

    /* the [fixed #N] values from the port, nothing else */
    CHECK(update(HDR "r|a||1\t$0\nr|find||1\t$2\nr|c||1\tkept\n", exp, NULL) == 0);
    CHECK(!strcmp(result, HDR "r|a||1\t$0\nr|find||1 [orig]\t$1\nr|find||1 [fixed #7]\t$2\n"
                          "r|old||1 [orig]\t$4\nr|c||1\tkept\n"));
    CHECK(strstr(report, "prog: r|find||1 [fixed #7]: $2, was $0\n") && strstr(report, "1 cases changed"));
    /* an unqualified case the port gives otherwise: reported, not touched */
    CHECK(update(HDR "r|a||1\t$3\nr|find||1\t$0\nr|c||1\tkept\n", exp, NULL) == 0 && !strcmp(result, exp));
    CHECK(strstr(report, "prog: r|a||1: the port gives $3, expected $0 (not touched"));
    CHECK(strstr(report, "0 cases changed"));
    /* --fix: the pair, at the place of the case */
    {
        static const char *const fix[] = { "r|a||1#12", NULL };

        CHECK(update(HDR "r|a||1\t$3\nr|find||1\t$0\nr|c||1\tkept\n", exp, fix) == 0);
        CHECK(!strcmp(result, HDR "r|a||1 [orig]\t$0\nr|a||1 [fixed #12]\t$3\nr|find||1 [orig]\t$1\n"
                              "r|find||1 [fixed #7]\t$0\nr|old||1 [orig]\t$4\nr|c||1\tkept\n"));
        CHECK(fixes[0].applied == 1 && strstr(report, "prog: r|a||1: the pair [orig] $0, [fixed #12] $3\n"));
        /* ... the port gives the expected value: nothing to fix */
        CHECK(update(HDR "r|a||1\t$0\nr|find||1\t$0\nr|c||1\tkept\n", exp, fix) == 0 && !strcmp(result, exp));
        CHECK(fixes[0].applied == 0 && fixes[0].same == 1);
        /* ... the port lacks the case: [orig] alone */
        CHECK(update(HDR "r|find||1\t$0\nr|c||1\tkept\n", exp, fix) == 0);
        CHECK(!strcmp(result, HDR "r|a||1 [orig]\t$0\nr|find||1 [orig]\t$1\nr|find||1 [fixed #7]\t$0\n"
                              "r|old||1 [orig]\t$4\nr|c||1\tkept\n"));
        CHECK(fixes[0].applied == 1 && strstr(report, "r|a||1 [orig]: not in the port's run, a case of the original only (#12)"));
    }
    /* --fix of a case only the port has: [fixed #N] after the case before it */
    {
        static const char *const fix[] = { "r|new||1#9", "r|new0||1#9", NULL };

        CHECK(update(HDR "r|new0||1\t$8\nr|a||1\t$0\nr|find||1\t$0\nr|new||1\t$5\nr|c||1\tkept\n", exp, fix) == 0);
        CHECK(!strcmp(result, HDR "r|new0||1 [fixed #9]\t$8\nr|a||1\t$0\nr|find||1 [orig]\t$1\n"
                              "r|find||1 [fixed #7]\t$0\nr|new||1 [fixed #9]\t$5\nr|old||1 [orig]\t$4\n"
                              "r|c||1\tkept\n"));
        CHECK(fixes[0].applied == 1 && fixes[1].applied == 1);
        CHECK(strstr(report, "prog: r|new||1 [fixed #9]: added ($5), a case of the port only\n"));
    }
    /* --fix of a qualified key */
    {
        static const char *const fix[] = { "r|find||1#9", "r|old||1#9", NULL };

        CHECK(update(HDR "r|a||1\t$0\nr|find||1\t$0\nr|c||1\tkept\n", exp, fix) == 0);
        CHECK(fixes[0].qualified == 1 && fixes[0].applied == 0 && fixes[1].qualified == 1);
    }
    /* a [fixed #N] case the port's run lacks: refused */
    CHECK(update(HDR "r|a||1\t$0\nr|c||1\tkept\n", exp, NULL) == 1 && !result);
    CHECK(strstr(report, "prog: r|find||1 [fixed #7]: not in the port's run\n")
          && strstr(report, "prog: refused: 1 [fixed #N] cases not in the port's run\n"));
    /* the syntax of --fix */
    {
        struct libscmp_fix f;

        CHECK(libscmp_parse_fix("r|c #1|v|2#15", &f, err, sizeof err) == 0 && f.issue == 15
              && f.keylen == 10 && !memcmp(f.key, "r|c #1|v|2", 10));
        libscmp_fix_free(&f);
        CHECK(libscmp_parse_fix("BLOCK:3#4", &f, err, sizeof err) == 0 && f.issue == 4);
        libscmp_fix_free(&f);
        CHECK(libscmp_parse_fix("r|c|v|1", &f, err, sizeof err) == -1 && strstr(err, "--fix r|c|v|1: not KEY#N"));
        CHECK(libscmp_parse_fix("r|c|v|1#0", &f, err, sizeof err) == -1);
        CHECK(libscmp_parse_fix("r|c|v|1#", &f, err, sizeof err) == -1);
        CHECK(libscmp_parse_fix("#5", &f, err, sizeof err) == -1);
        CHECK(libscmp_parse_fix("r|c|v|1#5x", &f, err, sizeof err) == -1);
        CHECK(libscmp_parse_fix("r|c|1#5", &f, err, sizeof err) == -1 && strstr(err, "--fix r|c|1#5: the key needs"));
        CHECK(libscmp_parse_fix("r|c|v|1 [orig]#5", &f, err, sizeof err) == -1 && strstr(err, "a qualifier"));
    }
    for (size_t n = 0; n < 4; n++)
        libscmp_fix_free(&fixes[n]);
}

/* the file dir/name.txt holds text */
static int holds(const char *dir, const char *name, const char *text)
{
    char path[300], *s;
    size_t len;
    FILE *f;
    int r;

    snprintf(path, sizeof path, "%s/%s.txt", dir, name);
    if (!(f = fopen(path, "rb")))
        return 0;
    s = malloc(strlen(text) + 2);
    len = fread(s, 1, strlen(text) + 1, f);
    fclose(f);
    r = len == strlen(text) && !memcmp(s, text, len);
    free(s);
    return r;
}

/* accept and update on directories: all or nothing, the filter */
static void test_accept_dirs(void)
{
    static const char *const files[] = { "r1/a", "r1/b", "r2/a", "r2/b", "exp/a", "exp/b", "exp/c" };
    static const char a_exp[] = "# capture a 1\nr|c||1 [orig]\t$1\nr|c||1 [fixed #7]\t$0\n";
    static const char b_old[] = "# capture b 1\nr|c||1\t$0\n";
    char path[100], *e = NULL;
    size_t i, elen;
    FILE *m, *x;
    const char *only_b[] = { "b" };

    for (i = 0; i < sizeof files / sizeof *files; i++) {
        snprintf(path, sizeof path, "libscmp-acc/%s.txt", files[i]);
        unlink(path);
    }
    mkdir("libscmp-acc", 0777);
    mkdir("libscmp-acc/r1", 0777);
    mkdir("libscmp-acc/r2", 0777);
    mkdir("libscmp-acc/exp", 0777);
    put_file("libscmp-acc/r1", "a", "# capture a 1\nr|c||1\t$2\n");
    put_file("libscmp-acc/r2", "a", "# capture a 1\nr|c||1\t$2\n");
    put_file("libscmp-acc/r1", "b", "# capture b 1\nr|c||1\t$1\n");
    put_file("libscmp-acc/r2", "b", "# capture b 1\nr|c||1\t$2\n");
    put_file("libscmp-acc/exp", "a", a_exp);
    put_file("libscmp-acc/exp", "b", b_old);
#define ACCEPT(only, n) do { \
        free(report); \
        m = open_memstream(&report, &reportlen); \
        x = open_memstream(&e, &elen); \
        status = libscmp_accept_dirs("libscmp-acc/r1", "libscmp-acc/r2", "libscmp-acc/exp", only, n, m, x); \
        fclose(m); \
        fclose(x); \
    } while (0)
    {
        int status;

        /* b is not deterministic: nothing written, a stays too */
        ACCEPT(NULL, 0);
        CHECK(status == 1 && strstr(report, "test not deterministic: b\nb: r|c||1: run 1 $1, run 2 $2\n")
              && strstr(report, "libs-accept: nothing written\n"));
        CHECK(holds("libscmp-acc/exp", "a", a_exp) && holds("libscmp-acc/exp", "b", b_old));
        free(e);
        /* the filter: only b, refused; then b deterministic, only b written */
        ACCEPT(only_b, 1);
        CHECK(status == 1 && holds("libscmp-acc/exp", "b", b_old));
        free(e);
        put_file("libscmp-acc/r2", "b", "# capture b 1\nr|c||1\t$1\n");
        put_file("libscmp-acc/r1", "a", "# capture a 1\nr|c||1\t$3\n");
        ACCEPT(only_b, 1);
        CHECK(status == 0 && holds("libscmp-acc/exp", "b", "# capture b 1\nr|c||1\t$1\n")
              && holds("libscmp-acc/exp", "a", a_exp));
        CHECK(strstr(report, "written: libscmp-acc/exp/b.txt\n1 files written, 0 unchanged\n"));
        free(e);
        /* all: a's runs differ now */
        ACCEPT(NULL, 0);
        CHECK(status == 1 && strstr(report, "test not deterministic: a\n"));
        free(e);
        put_file("libscmp-acc/r1", "a", "# capture a 1\nr|c||1\t$2\n");
        ACCEPT(NULL, 0);
        CHECK(status == 0 && holds("libscmp-acc/exp", "a", "# capture a 1\nr|c||1 [orig]\t$2\nr|c||1 [fixed #7]\t$0\n"));
        CHECK(strstr(report, "1 files written, 1 unchanged\n"));
        free(e);
        /* expected data without runs */
        put_file("libscmp-acc/exp", "c", "# capture c 1\nr|c||1\t$0\n");
        ACCEPT(NULL, 0);
        CHECK(status == 1 && strstr(report, "c: expected data, but no output of the runs"));
        free(e);
        /* a run of one program in one directory only */
        put_file("libscmp-acc/r1", "c", "# capture c 1\nr|c||1\t$0\n");
        ACCEPT(NULL, 0);
        CHECK(status == 1 && strstr(report, "test not deterministic: c\nc: no output of run 2"));
        free(e);
        unlink("libscmp-acc/r1/c.txt");
        unlink("libscmp-acc/exp/c.txt");
        /* a malformed run */
        put_file("libscmp-acc/r2", "b", "# capture b 1\nr|c||1 [orig]\t$1\n");
        ACCEPT(NULL, 0);
        CHECK(status == 2 && strstr(e, "libscmp-acc/r2/b.txt:2: a qualifier in a run's output"));
        free(e);
    }
#undef ACCEPT
    /* update: a --fix that applies nowhere refuses; then it applies */
    {
        struct libscmp_fix fx[1];
        int status;

        put_file("libscmp-acc/r1", "b", "# capture b 1\nr|c||1\t$5\n");
        CHECK(libscmp_parse_fix("r|x||1#4", &fx[0], err, sizeof err) == 0);
        free(report);
        m = open_memstream(&report, &reportlen);
        x = open_memstream(&e, &elen);
        status = libscmp_update_dirs("libscmp-acc/r1", "libscmp-acc/exp", NULL, 0, fx, 1, m, x);
        fclose(m);
        fclose(x);
        CHECK(status == 1 && strstr(report, "--fix r|x||1#4: the key is in no program's expected data or run\n")
              && strstr(report, "libs-update: nothing written"));
        free(e);
        libscmp_fix_free(&fx[0]);
        CHECK(libscmp_parse_fix("r|c||1#4", &fx[0], err, sizeof err) == 0);
        free(report);
        m = open_memstream(&report, &reportlen);
        x = open_memstream(&e, &elen);
        status = libscmp_update_dirs("libscmp-acc/r1", "libscmp-acc/exp", NULL, 0, fx, 1, m, x);
        fclose(m);
        fclose(x);
        /* a: [fixed #7] from the port's $2 ... and b's pair */
        CHECK(status == 0 && holds("libscmp-acc/exp", "a", "# capture a 1\nr|c||1 [orig]\t$2\nr|c||1 [fixed #7]\t$2\n"));
        CHECK(holds("libscmp-acc/exp", "b", "# capture b 1\nr|c||1 [orig]\t$1\nr|c||1 [fixed #4]\t$5\n"));
        free(e);
        libscmp_fix_free(&fx[0]);
        /* in two programs: name one with --only */
        put_file("libscmp-acc/exp", "b", "# capture b 1\nr|c||1\t$1\n");
        put_file("libscmp-acc/exp", "a", "# capture a 1\nr|c||1\t$1\n");
        CHECK(libscmp_parse_fix("r|c||1#4", &fx[0], err, sizeof err) == 0);
        free(report);
        m = open_memstream(&report, &reportlen);
        x = open_memstream(&e, &elen);
        status = libscmp_update_dirs("libscmp-acc/r1", "libscmp-acc/exp", NULL, 0, fx, 1, m, x);
        fclose(m);
        fclose(x);
        CHECK(status == 1 && strstr(report, "the key is in more than one program (name it with --only)"));
        free(e);
        fx[0].applied = 0;
        free(report);
        m = open_memstream(&report, &reportlen);
        x = open_memstream(&e, &elen);
        status = libscmp_update_dirs("libscmp-acc/r1", "libscmp-acc/exp", only_b, 1, fx, 1, m, x);
        fclose(m);
        fclose(x);
        CHECK(status == 0 && holds("libscmp-acc/exp", "a", "# capture a 1\nr|c||1\t$1\n")
              && holds("libscmp-acc/exp", "b", "# capture b 1\nr|c||1 [orig]\t$1\nr|c||1 [fixed #4]\t$5\n"));
        free(e);
        libscmp_fix_free(&fx[0]);
    }
}

int main(void)
{
    test_equal();
    test_changed();
    test_missing_added();
    test_block();
    test_qualifiers();
    test_not_captured();
    test_escaping();
    test_malformed();
    test_details();
    test_dirs();
    test_accept();
    test_update();
    test_accept_dirs();
    free(result);
    test_large();
    free(report);
    if (failures)
        fprintf(stderr, "test_libscmp: %d failures\n", failures);
    return failures != 0;
}
