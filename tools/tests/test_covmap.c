/*
 * test_covmap.c - lib/covmap: the JSON reader, paths, ninja's dependency
 * lists, and the lcov tracefiles from host gcov JSON and from Amiga
 * coverage files with link map, nm and objdump line tables, checked
 * against the tracefile format of lcov; broken inputs of each kind.
 */
#include "testutil.h"
#include "covfmt.h"
#include "covmap.h"
#include "json.h"

#define ROOT "/src/bb2"

static char err[512];

/* the tracefile of test i as text, of all tests for -1 */
static char *trace_text(struct covmap *m, long i)
{
    char *s = NULL;
    size_t n = 0;
    FILE *f = open_memstream(&s, &n);

    CHECK(f && !(i < 0 ? covmap_write_all(m, f) : covmap_write_test(m, (size_t)i, f)));
    fclose(f);
    return s;
}

/*
 * The tracefile format of lcov 2.x (geninfo(1) "TRACEFILE FORMAT",
 * genhtml's reader): optional TN: with a name without ',' (lcov keeps
 * [A-Za-z0-9_] and reads every other character as '_', with a warning);
 * sections
 * SF: ... end_of_record; in a section FN:<line>[,<end line>],<name> with
 * lines above 0 and each name once, FNDA:<count>,<name> of a function of
 * an FN: before it, FNF/FNH the number of functions and of those with a
 * count above 0, BRDA:<line>,<block>,<branch>,<taken or -> with BRF/BRH,
 * DA:<line>,<count> with LF/LH; the count records after the records they
 * count.  NULL if text keeps to it, else why not.
 */
static const char *lcov_check(const char *text)
{
    static char why[700];
    char names[64][200];
    size_t pos = 0, len = strlen(text), n, nfn = 0, fnda = 0, fnh = 0, br = 0, brh = 0, da = 0,
           dah = 0, k, lineno = 0;
    const char *line;
    int section = 0;

#define NO(...) do { snprintf(why, sizeof why, __VA_ARGS__); return why; } while (0)
    while ((line = cov_next_line(text, len, &pos, &n))) {
        char buf[512], name[200], taken[32];
        unsigned long long a, b, c;
        int used;

        lineno++;
        if (n >= sizeof buf)
            NO("line %zu: too long", lineno);
        memcpy(buf, line, n);
        buf[n] = 0;
        if (!strncmp(buf, "TN:", 3)) {
            if (section || strchr(buf, ','))
                NO("line %zu: bad TN:", lineno);
        } else if (!strncmp(buf, "SF:", 3)) {
            if (section || n == 3)
                NO("line %zu: bad SF:", lineno);
            section = 1;
            nfn = fnda = fnh = br = brh = da = dah = 0;
        } else if (!section) {
            NO("line %zu: outside a section", lineno);
        } else if (!strcmp(buf, "end_of_record")) {
            section = 0;
        } else if (sscanf(buf, "FN:%llu,%llu,%199s%n", &a, &b, name, &used) == 3 && !buf[used]) {
            if (!a || b < a || fnda || nfn == 64)
                NO("line %zu: bad FN:", lineno);
            for (k = 0; k < nfn; k++)
                if (!strcmp(names[k], name))
                    NO("line %zu: FN: twice", lineno);
            snprintf(names[nfn++], sizeof *names, "%s", name);
        } else if (sscanf(buf, "FN:%llu,%199s%n", &a, name, &used) == 2 && !buf[used]) {
            if (!a || fnda || nfn == 64)
                NO("line %zu: bad FN:", lineno);
            snprintf(names[nfn++], sizeof *names, "%s", name);
        } else if (sscanf(buf, "FNDA:%llu,%199s%n", &a, name, &used) == 2 && !buf[used]) {
            for (k = 0; k < nfn && strcmp(names[k], name); k++)
                ;
            if (k == nfn)
                NO("line %zu: FNDA: without FN:", lineno);
            fnda++;
            fnh += a > 0;
        } else if (sscanf(buf, "FNF:%llu%n", &a, &used) == 1 && !buf[used]) {
            if (a != nfn)
                NO("line %zu: FNF:%llu, %zu functions", lineno, a, nfn);
        } else if (sscanf(buf, "FNH:%llu%n", &a, &used) == 1 && !buf[used]) {
            if (a != fnh || fnda != nfn)
                NO("line %zu: FNH:%llu, %zu hit", lineno, a, fnh);
        } else if (sscanf(buf, "BRDA:%llu,%llu,%llu,%31s%n", &a, &b, &c, taken, &used) == 4
                   && !buf[used]) {
            if (!a || (strcmp(taken, "-") && strspn(taken, "0123456789") != strlen(taken)))
                NO("line %zu: bad BRDA:", lineno);
            br++;
            brh += strcmp(taken, "-") && strcmp(taken, "0");
        } else if (sscanf(buf, "BRF:%llu%n", &a, &used) == 1 && !buf[used]) {
            if (a != br)
                NO("line %zu: BRF:%llu, %zu branches", lineno, a, br);
        } else if (sscanf(buf, "BRH:%llu%n", &a, &used) == 1 && !buf[used]) {
            if (a != brh)
                NO("line %zu: BRH:%llu, %zu taken", lineno, a, brh);
        } else if (sscanf(buf, "DA:%llu,%llu%n", &a, &b, &used) == 2 && !buf[used]) {
            if (!a)
                NO("line %zu: bad DA:", lineno);
            da++;
            dah += b > 0;
        } else if (sscanf(buf, "LF:%llu%n", &a, &used) == 1 && !buf[used]) {
            if (a != da)
                NO("line %zu: LF:%llu, %zu lines", lineno, a, da);
        } else if (sscanf(buf, "LH:%llu%n", &a, &used) == 1 && !buf[used]) {
            if (a != dah)
                NO("line %zu: LH:%llu, %zu hit", lineno, a, dah);
        } else
            NO("line %zu: not a record: %s", lineno, buf);
    }
    if (section)
        NO("no end_of_record");
#undef NO
    return NULL;
}

/* the checker itself: broken tracefiles */
static void test_lcov_check(void)
{
    static const char *const bad[][2] = {
        { "TN:a,b\n", "bad TN:" },
        { "SF:a.c\nTN:a\nend_of_record\n", "bad TN:" },
        { "FN:1,f\n", "outside a section" },
        { "SF:a.c\nFN:1,f\n", "no end_of_record" },
        { "SF:a.c\nFN:0,f\nend_of_record\n", "bad FN:" },
        { "SF:a.c\nFN:5,4,f\nend_of_record\n", "bad FN:" },
        { "SF:a.c\nFN:1,2,f\nFN:3,4,f\nend_of_record\n", "FN: twice" },
        { "SF:a.c\nFN:1,2,f\nFNDA:1,g\nend_of_record\n", "without FN:" },
        { "SF:a.c\nFN:1,2,f\nFNDA:1,f\nFNF:2\nend_of_record\n", "FNF:2" },
        { "SF:a.c\nFN:1,2,f\nFNDA:0,f\nFNF:1\nFNH:1\nend_of_record\n", "FNH:1" },
        { "SF:a.c\nBRDA:1,0,0,x\nend_of_record\n", "bad BRDA:" },
        { "SF:a.c\nBRDA:1,0,0,-\nBRF:1\nBRH:1\nend_of_record\n", "BRH:1" },
        { "SF:a.c\nDA:0,1\nend_of_record\n", "bad DA:" },
        { "SF:a.c\nDA:1,0\nLF:1\nLH:1\nend_of_record\n", "LH:1" },
        { "SF:a.c\nDA:1\nend_of_record\n", "not a record" },
    };
    size_t i;
    const char *why;

    CHECK(!lcov_check("TN:t_1\nSF:/a.c\nFN:1,2,f\nFN:3,g\nFNDA:2,f\nFNDA:0,g\nFNF:2\nFNH:1\n"
                      "BRDA:1,0,0,-\nBRDA:1,0,1,3\nBRF:2\nBRH:1\nDA:1,2\nDA:2,0\nLF:2\nLH:1\n"
                      "end_of_record\nTN:\nSF:/b.c\nend_of_record\n"));
    for (i = 0; i < sizeof bad / sizeof *bad; i++)
        CHECK((why = lcov_check(bad[i][0])) && strstr(why, bad[i][1]));
}

static int contains(const char *text, const char *line)
{
    const char *p = text;
    size_t n = strlen(line);

    while ((p = strstr(p, line))) {
        if ((p == text || p[-1] == '\n') && p[n] == '\n')
            return 1;
        p++;
    }
    return 0;
}

static void test_json(void)
{
    static const char doc[] = " {\"a\": [1, -2.5e1, true, false, null], \"s\": \"x\\n\\u00e9\\ud83d\\ude00\\\"\","
                              " \"o\": {}} {\"b\": 2}";
    size_t pos = 0;
    struct json *j = json_parse(doc, strlen(doc), &pos, err, sizeof err);
    const struct json *a;

    CHECK(j && j->type == JSON_OBJECT && j->n == 3);
    a = json_get(j, "a");
    CHECK(a && a->type == JSON_ARRAY && a->n == 5);
    CHECK(a && a->items[0].number == 1 && a->items[1].number == -25);
    CHECK(a && a->items[2].type == JSON_TRUE && a->items[3].type == JSON_FALSE
          && a->items[4].type == JSON_NULL);
    CHECK(!strcmp(json_get_string(j, "s"), "x\n\xc3\xa9\xf0\x9f\x98\x80\""));
    CHECK(json_get(j, "o") && json_get(j, "o")->type == JSON_OBJECT && !json_get(j, "o")->n);
    CHECK(!json_get(j, "missing") && json_get_number(j, "s", 7) == 7);
    json_free(j);
    j = json_parse(doc, strlen(doc), &pos, err, sizeof err);
    CHECK(j && json_get_number(j, "b", 0) == 2);
    json_free(j);
    CHECK(!json_parse(doc, strlen(doc), &pos, err, sizeof err) && !err[0]);   /* the end */

    /* broken */
    static const char *const bad[] = {
        "{\"a\" 1}", "{\"a\": 1,}", "[1 2]", "\"open", "{\"a\": tru}", "[\"\\x\"]", "{1: 2}",
        "[\"\\u12\"]", "[",
    };
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        pos = 0;
        CHECK(!json_parse(bad[i], strlen(bad[i]), &pos, err, sizeof err) && err[0]);
    }
    /* nested too deeply */
    char deep[2000];
    memset(deep, '[', sizeof deep - 1);
    deep[sizeof deep - 1] = 0;
    pos = 0;
    CHECK(!json_parse(deep, strlen(deep), &pos, err, sizeof err) && strstr(err, "deeply"));
}

static void test_paths(void)
{
    char *s;

    CHECK((s = cov_relpath(ROOT "/programs/blitz2/src/eval.c", ROOT)) && !strcmp(s, "programs/blitz2/src/eval.c"));
    free(s);
    CHECK((s = cov_relpath(ROOT "//programs/./ted/../blitz2/x.c", ROOT "/")) && !strcmp(s, "programs/blitz2/x.c"));
    free(s);
    CHECK(!cov_relpath("/opt/m68k/include/stdio.h", ROOT));
    CHECK(!cov_relpath(ROOT "2/x.c", ROOT));
    CHECK(!cov_relpath("../x.c", ROOT));
    CHECK((s = cov_relpath("programs/blitz2/../../shared/xtra/xtra.c", ROOT)) && !strcmp(s, "shared/xtra/xtra.c"));
    free(s);
    CHECK((s = cov_join("/b/host", "x/y.c")) && !strcmp(s, "/b/host/x/y.c"));
    free(s);
    CHECK((s = cov_join("/b/host", "/abs.c")) && !strcmp(s, "/abs.c"));
    free(s);

    CHECK((s = cov_object_source("programs/blitz2/CMakeFiles/blitz2-ab3.dir/__/__/shared/xtra/xtra.c.obj"))
          && !strcmp(s, "programs/blitz2/../../shared/xtra/xtra.c"));
    free(s);
    CHECK((s = cov_object_source("programs/blitz2/CMakeFiles/blitz2_core.dir/src/eval.c.obj"))
          && !strcmp(s, "programs/blitz2/src/eval.c"));
    free(s);
    CHECK((s = cov_object_source("CMakeFiles/top.dir/a.c.o")) && !strcmp(s, "a.c"));
    free(s);
    CHECK(!cov_object_source("/opt/libnix/lib/nbcrt0.o"));
    CHECK(!cov_object_source("programs/blitz2/CMakeFiles/x/eval.c.obj"));
}

static const char deps_text[] =
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/eval.c.obj: #deps 3, deps mtime 1 (VALID)\n"
    "    " ROOT "/programs/blitz2/src/eval.c\n"
    "    " ROOT "/programs/blitz2/src/blitz2.h\n"
    "    /opt/m68k/include/exec/types.h\n"
    "\n"
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/main.c.obj: #deps 2, deps mtime 1 (STALE)\n"
    "    " ROOT "/programs/blitz2/src/blitz2.h\n"
    "    " ROOT "/programs/blitz2/src/main.c\n"
    "\n";

static void test_deps(void)
{
    struct cov_deps d = { 0 };
    const struct cov_dep *e;

    CHECK(!cov_deps_parse(&d, deps_text, err, sizeof err) && d.n == 2);
    CHECK((e = cov_deps_find(&d, "programs/blitz2/CMakeFiles/blitz2_core.dir/src/main.c.obj"))
          && e->ndeps == 2 && !strcmp(cov_dep_source(e), ROOT "/programs/blitz2/src/main.c"));
    CHECK(!cov_deps_find(&d, "x.o"));
    cov_deps_free(&d);
    CHECK(cov_deps_parse(&d, "    /a/b.h\n", err, sizeof err) && strstr(err, "without an object"));
    cov_deps_free(&d);
    CHECK(cov_deps_parse(&d, "this is no deps output\n", err, sizeof err) && strstr(err, "line 1"));
    cov_deps_free(&d);
}

/* gcov JSON of one object: tokit run, tokskip not, lines with branches
   (line 12 twice: two blocks; line 25 not run); a system header, a build
   directory file; a second object with alpha.c and tokit.c again (counts
   add up) */
static const char gcov_a[] =
    "{\"format_version\": \"2\", \"current_working_directory\": \"" ROOT "/build/host\","
    " \"data_file\": \"x.gcda\", \"files\": ["
    " {\"file\": \"" ROOT "/programs/ted/src/core/tokit.c\", \"functions\": ["
    "  {\"name\": \"tokit\", \"start_line\": 10, \"end_line\": 20, \"execution_count\": 3},"
    "  {\"name\": \"tokskip\", \"start_line\": 22, \"end_line\": 30, \"execution_count\": 0}],"
    "  \"lines\": [{\"line_number\": 10, \"count\": 3, \"branches\": []},"
    "   {\"line_number\": 12, \"count\": 3, \"unexecuted_block\": false, \"branches\": ["
    "     {\"count\": 2, \"throw\": false, \"fallthrough\": true}, {\"count\": 1}]},"
    "   {\"line_number\": 25, \"count\": 0, \"branches\": [{\"count\": 0}, {\"count\": 0}]},"
    "   {\"line_number\": 12, \"count\": 1, \"branches\": [{\"count\": 0}]}]},"
    " {\"file\": \"/usr/include/stdio.h\", \"functions\": ["
    "  {\"name\": \"printf\", \"start_line\": 1, \"end_line\": 2, \"execution_count\": 1}]},"
    " {\"file\": \"_deps/snitch-src/snitch.cpp\", \"functions\": ["
    "  {\"name\": \"run\", \"start_line\": 1, \"end_line\": 2, \"execution_count\": 1}]}]}\n"
    "{\"current_working_directory\": \"" ROOT "/build/host\", \"files\": ["
    " {\"file\": \"../../programs/ted/src/core/alpha.c\", \"functions\": ["
    "  {\"name\": \"alpha\", \"start_line\": 5, \"end_line\": 9, \"execution_count\": 1}]},"
    " {\"file\": \"" ROOT "/programs/ted/src/core/tokit.c\", \"functions\": ["
    "  {\"name\": \"tokit\", \"start_line\": 10, \"end_line\": 20, \"execution_count\": 2}],"
    "  \"lines\": [{\"line_number\": 10, \"count\": 1},"
    "   {\"line_number\": 12, \"count\": 2, \"branches\": [{\"count\": 1}, {\"count\": 1}]}]}]}\n";
/* the second test runs tokskip */
static const char gcov_b[] =
    "{\"current_working_directory\": \"" ROOT "/build/host\", \"files\": ["
    " {\"file\": \"" ROOT "/programs/ted/src/core/tokit.c\", \"functions\": ["
    "  {\"name\": \"tokit\", \"start_line\": 10, \"end_line\": 20, \"execution_count\": 1},"
    "  {\"name\": \"tokskip\", \"start_line\": 22, \"end_line\": 30, \"execution_count\": 2}]}]}";

static void test_gcov(void)
{
    struct covmap m;
    char *t;
    const char *why;

    covmap_init(&m, ROOT);
    CHECK(!covmap_drop(&m, ROOT "/build"));
    CHECK(!covmap_add_gcov(&m, "host/ted_core_b", gcov_b, strlen(gcov_b), err, sizeof err));
    CHECK(!covmap_add_gcov(&m, "host/ted_core_a", gcov_a, strlen(gcov_a), err, sizeof err));
    CHECK(covmap_tests(&m) == 2);
    CHECK(!strcmp(covmap_test(&m, 0), "host/ted_core_a") && !strcmp(covmap_test(&m, 1), "host/ted_core_b"));
    CHECK(!covmap_test(&m, 2));
    t = trace_text(&m, 0);
    CHECK(!strcmp(t, "TN:host/ted_core_a\n"
                     "SF:" ROOT "/programs/ted/src/core/alpha.c\n"
                     "FN:5,9,alpha\nFNDA:1,alpha\nFNF:1\nFNH:1\nend_of_record\n"
                     "SF:" ROOT "/programs/ted/src/core/tokit.c\n"
                     "FN:10,20,tokit\nFN:22,30,tokskip\nFNDA:5,tokit\nFNDA:0,tokskip\nFNF:2\nFNH:1\n"
                     "BRDA:12,0,0,3\nBRDA:12,0,1,2\nBRDA:12,1,0,0\nBRDA:25,0,0,-\nBRDA:25,0,1,-\n"
                     "BRF:5\nBRH:2\n"
                     "DA:10,4\nDA:12,6\nDA:25,0\nLF:3\nLH:2\nend_of_record\n"));
    CHECK(!(why = lcov_check(t)));
    free(t);
    t = trace_text(&m, 1);
    CHECK(!strcmp(t, "TN:host/ted_core_b\n"
                     "SF:" ROOT "/programs/ted/src/core/tokit.c\n"
                     "FN:10,20,tokit\nFN:22,30,tokskip\nFNDA:1,tokit\nFNDA:2,tokskip\nFNF:2\nFNH:2\n"
                     "end_of_record\n"));
    free(t);
    /* all.info: both, in the order of their names */
    t = trace_text(&m, -1);
    CHECK(!(why = lcov_check(t)));
    CHECK(strstr(t, "TN:host/ted_core_a\n") == t && strstr(t, "\nTN:host/ted_core_b\n"));
    CHECK(!strstr(t, "stdio.h") && !strstr(t, "snitch"));
    free(t);

    /* broken: no JSON, JSON that is no gcov output, a function without
       name, a line without number; names no tracefile can have */
    static const char *const bad[][2] = {
        { "{\"files\": [", "gcov JSON" },
        { "{\"a\": 1}", "no \"files\"" },
        { "{\"files\": [{\"file\": \"a.c\", \"functions\": [{}]}]}", "without \"name\"" },
        { "{\"files\": [{\"file\": \"a.c\"}]}", "without \"file\" or \"functions\"" },
        { "{\"files\": [{\"file\": \"a.c\", \"functions\": [], \"lines\": [{\"count\": 1}]}]}",
          "without \"line_number\"" },
        { "{\"files\": [{\"file\": \"a.c\", \"functions\": [], \"lines\": {}}]}", "no array" },
    };
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++)
        CHECK(covmap_add_gcov(&m, "host/x", bad[i][0], strlen(bad[i][0]), err, sizeof err)
              && strstr(err, bad[i][1]));
    static const char *const bad_names[] = { "", "all", "/abs", "a//b", "a/../b", "./a", "a/", "a\tb", "a,b" };
    for (size_t i = 0; i < sizeof bad_names / sizeof *bad_names; i++)
        CHECK(covmap_add_gcov(&m, bad_names[i], gcov_b, strlen(gcov_b), err, sizeof err)
              && strstr(err, "no usable test name"));
    CHECK(covmap_test_name_ok("host/ted_core_a") && covmap_test_name_ok("all2")
          && covmap_test_name_ok("a/all") && covmap_test_name_ok("x.y-z"));
    covmap_free(&m);
}

/* a link map: the startup code (outside the root), main.c (two global
   functions), eval.c (one global, a static one only nm knows, the section
   name on a line of its own); code hunk 0x200 bytes */
static const char linkmap[] =
    "Archive member included to satisfy reference by file (symbol)\n"
    "\n"
    "Linker script and memory map\n"
    "\n"
    "LOAD /opt/libnix/lib/nbcrt0.o\n"
    "LOAD programs/blitz2/CMakeFiles/blitz2_core.dir/src/main.c.obj\n"
    "                0x00000000                        . = 0x0\n"
    "\n"
    ".text           0x00000000      0x200\n"
    "                0x00000000                        __stext = .\n"
    " *(.text)\n"
    " .text          0x00000000       0x40 /opt/libnix/lib/nbcrt0.o\n"
    "                0x00000020                exit\n"
    " .text          0x00000040       0x80 programs/blitz2/CMakeFiles/blitz2_core.dir/src/main.c.obj\n"
    "                0x00000040                main\n"
    "                0x00000080                compile\n"
    " .text.unlikely\n"
    "                0x000000c0      0x100 programs/blitz2/CMakeFiles/blitz2_core.dir/src/eval.c.obj\n"
    "                0x000000c0                eval\n"
    " *fill*         0x000001c0       0x40 \n"
    "                0x00000200                        __etext = .\n"
    "\n"
    ".data           0x00000200      0x100\n"
    " .data          0x00000200      0x100 programs/blitz2/CMakeFiles/blitz2_core.dir/src/main.c.obj\n"
    "                0x00000200                globalvar\n";

static const char objsyms[] =
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/eval.c.obj:00000000 T _eval\n"
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/eval.c.obj:00000060 t _evalsub\n"
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/eval.c.obj:00000010 d _table\n"
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/eval.c.obj:         U _compile\n"
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/other.c.obj:00000000 T _notlinked\n";

static const char lines[] =
    "\n"
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/eval.c.obj:     file format amiga\n"
    "\n"
    "Contents of the .debug_line section:\n"
    "\n"
    ROOT "/programs/blitz2/src/eval.c:\n"
    "File name                        Line number    Starting address    View    Stmt\n"
    "eval.c                                   100                   0               x\n"
    "eval.c                                   120                0x40               x\n"
    "eval.c                                   104                0x58\n"
    "eval.c                                   200                0x60               x\n"
    "eval.c                                   230                0xfe       1\n"
    "\n"
    ROOT "/programs/blitz2/src/blitz2.h:\n"
    "blitz2.h                                  10                0x70               x\n"
    "\n"
    "programs/blitz2/CMakeFiles/blitz2_core.dir/src/main.c.obj:     file format amiga\n"
    "\n";

/* a coverage file for a code hunk of code bytes with the functions at
   the given offsets called */
static unsigned char *cov_file(uint32_t code, uint32_t bytes, const uint32_t *offsets, size_t n,
                               size_t *len)
{
    unsigned char *b = calloc(1, COVFMT_HEADER_SIZE + bytes);
    uint32_t v[4] = { COVFMT_MAGIC, COVFMT_VERSION, code, bytes };

    for (int i = 0; i < 4; i++) {
        b[4 * i] = (unsigned char)(v[i] >> 24);
        b[4 * i + 1] = (unsigned char)(v[i] >> 16);
        b[4 * i + 2] = (unsigned char)(v[i] >> 8);
        b[4 * i + 3] = (unsigned char)v[i];
    }
    for (size_t i = 0; i < n; i++)
        b[COVFMT_HEADER_SIZE + offsets[i] / 16] |= (unsigned char)(1 << (offsets[i] / 2 % 8));
    *len = COVFMT_HEADER_SIZE + bytes;
    return b;
}

static void test_amiga(void)
{
    struct covmap m;
    const uint32_t run1[] = { 0x0, 0x40, 0xc0 }, run2[] = { 0x80, 0x120 };
    unsigned char *c;
    size_t len;
    char *t;

    covmap_init(&m, ROOT);
    CHECK(!covmap_add_deps(&m, deps_text, err, sizeof err));
    CHECK(!covmap_add_linkmap(&m, "blitz2", linkmap, err, sizeof err));
    CHECK(m.nprogs == 1 && m.progs[0].text_size == 0x200 && m.progs[0].nobjs == 3);
    CHECK(!covmap_add_objsyms(&m, "blitz2", objsyms, err, sizeof err));
    CHECK(!covmap_add_lines(&m, "blitz2", lines, err, sizeof err));
    c = cov_file(0x200, COVFMT_BITMAP_LEN(0x200), run1, 3, &len);
    CHECK(!covmap_add_cov(&m, "corpus", "blitz2", c, len, err, sizeof err));
    free(c);
    c = cov_file(0x200, COVFMT_BITMAP_LEN(0x200), run2, 2, &len);
    CHECK(!covmap_add_cov(&m, "ab3_cli", "blitz2", c, len, err, sizeof err));
    free(c);
    /* a second program of the same sources run by corpus: a function
       entered in both counts 1 */
    CHECK(!covmap_add_linkmap(&m, "blitz2-ab3", linkmap, err, sizeof err));
    c = cov_file(0x200, COVFMT_BITMAP_LEN(0x200), run1, 2, &len);
    CHECK(!covmap_add_cov(&m, "corpus", "blitz2-ab3", c, len, err, sizeof err));
    free(c);
    CHECK(covmap_tests(&m) == 2 && !strcmp(covmap_test(&m, 0), "ab3_cli"));
    /* eval.c: the static function from nm, its range from the line table
       (the header's rows do not count); main.c: source from the deps, no
       line ranges (no -g object): line 1 without a last line; every
       function of the program, run or not */
    t = trace_text(&m, 1);
    CHECK(!strcmp(t, "TN:corpus\n"
                     "SF:" ROOT "/programs/blitz2/src/eval.c\n"
                     "FN:100,120,eval\nFN:200,230,evalsub\nFNDA:1,eval\nFNDA:0,evalsub\nFNF:2\nFNH:1\n"
                     "end_of_record\n"
                     "SF:" ROOT "/programs/blitz2/src/main.c\n"
                     "FN:1,compile\nFN:1,main\nFNDA:0,compile\nFNDA:1,main\nFNF:2\nFNH:1\n"
                     "end_of_record\n"));
    free(t);
    t = trace_text(&m, 0);
    CHECK(strstr(t, "TN:ab3_cli\n") == t && strstr(t, "\nFNDA:0,eval\nFNDA:1,evalsub\n")
          && strstr(t, "\nFNDA:1,compile\nFNDA:0,main\n"));
    free(t);
    t = trace_text(&m, -1);
    CHECK(!lcov_check(t));
    /* the startup code is outside the root: left out; no data symbols */
    CHECK(!strstr(t, "exit") && !strstr(t, "table") && !strstr(t, "globalvar") && !strstr(t, "notlinked"));
    free(t);

    /* broken coverage files */
    c = cov_file(0x1fe, COVFMT_BITMAP_LEN(0x1fe), run1, 1, &len);
    CHECK(covmap_add_cov(&m, "x", "blitz2", c, len, err, sizeof err) && strstr(err, "the link map has 512"));
    free(c);
    c = cov_file(0x200, COVFMT_BITMAP_LEN(0x200) - 1, run1, 1, &len);
    CHECK(covmap_add_cov(&m, "x", "blitz2", c, len, err, sizeof err) && strstr(err, "bitmap"));
    CHECK(covmap_add_cov(&m, "x", "blitz2", c, len - 3, err, sizeof err));
    c[0] = 'X';
    CHECK(covmap_add_cov(&m, "x", "blitz2", c, len, err, sizeof err) && strstr(err, "not a coverage file"));
    CHECK(covmap_add_cov(&m, "x", "blitz2", c, 8, err, sizeof err) && strstr(err, "not a coverage file"));
    free(c);
    c = cov_file(0x200, COVFMT_BITMAP_LEN(0x200) + 4, run1, 1, &len);
    c[len - 1] = 1;
    CHECK(covmap_add_cov(&m, "x", "blitz2", c, len, err, sizeof err) && strstr(err, "beyond"));
    c[7] = 2;
    CHECK(covmap_add_cov(&m, "x", "blitz2", c, len, err, sizeof err) && strstr(err, "version 2"));
    free(c);
    CHECK(covmap_add_cov(&m, "x", "ted", NULL, 0, err, sizeof err) && strstr(err, "no link map"));
    /* broken link maps, symbols, line tables */
    CHECK(covmap_add_linkmap(&m, "blitz2", linkmap, err, sizeof err) && strstr(err, "second"));
    CHECK(covmap_add_linkmap(&m, "ted", "nothing here\n", err, sizeof err) && strstr(err, "no .text"));
    CHECK(covmap_add_linkmap(&m, "ted2", ".text   nonsense\n", err, sizeof err) && strstr(err, "without address"));
    CHECK(covmap_add_objsyms(&m, "blitz2", objsyms, err, sizeof err) && strstr(err, "after the coverage"));
    CHECK(covmap_add_objsyms(&m, "nope", objsyms, err, sizeof err) && strstr(err, "before the link map"));
    CHECK(covmap_add_lines(&m, "blitz2", "garbage\n", err, sizeof err) && strstr(err, "decodedline"));
    covmap_free(&m);

    covmap_init(&m, ROOT);
    CHECK(!covmap_add_linkmap(&m, "blitz2", linkmap, err, sizeof err));
    CHECK(covmap_add_objsyms(&m, "blitz2", "eval.c.obj 0000 T x\n", err, sizeof err) && strstr(err, "nm -A"));
    CHECK(covmap_add_objsyms(&m, "blitz2",
                             "programs/blitz2/CMakeFiles/blitz2_core.dir/src/eval.c.obj:00000100 T _x\n",
                             err, sizeof err) && strstr(err, "outside"));
    covmap_free(&m);

    /* without nm and deps: the map's globals, the source from the object
       path */
    covmap_init(&m, ROOT);
    CHECK(!covmap_add_linkmap(&m, "blitz2", linkmap, err, sizeof err));
    c = cov_file(0x200, COVFMT_BITMAP_LEN(0x200), run2, 2, &len);
    CHECK(!covmap_add_cov(&m, "ab3_cli", "blitz2", c, len, err, sizeof err));
    free(c);
    t = trace_text(&m, 0);
    /* 0x120 is in the static function, which only the map's eval holds */
    CHECK(contains(t, "FN:1,eval") && contains(t, "FNDA:1,eval"));
    CHECK(contains(t, "FNDA:1,compile") && contains(t, "FNDA:0,main"));
    CHECK(!lcov_check(t));
    free(t);
    /* a test name no tracefile can have */
    c = cov_file(0x200, COVFMT_BITMAP_LEN(0x200), run2, 2, &len);
    CHECK(covmap_add_cov(&m, "../x", "blitz2", c, len, err, sizeof err) && strstr(err, "no usable"));
    free(c);
    covmap_free(&m);
}

int main(void)
{
    test_json();
    test_paths();
    test_deps();
    test_lcov_check();
    test_gcov();
    test_amiga();
    if (failures)
        fprintf(stderr, "%d failure(s)\n", failures);
    return failures != 0;
}
