/*
 * test_covmap.c - lib/covmap: the JSON reader, paths, ninja's dependency
 * lists, and the map from host gcov JSON and from Amiga coverage files
 * with link map, nm and objdump line tables; broken inputs of each kind.
 */
#include "testutil.h"
#include "covfmt.h"
#include "covmap.h"
#include "json.h"

#define ROOT "/src/bb2"

static char err[512];

/* the map as text */
static char *map_text(struct covmap *m)
{
    char *s = NULL;
    size_t n = 0;
    FILE *f = open_memstream(&s, &n);

    CHECK(f && !covmap_write(m, "abc123", f));
    fclose(f);
    return s;
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

/* gcov JSON of one object: f1 run, f2 not; a system header, a build
   directory file */
static const char gcov_a[] =
    "{\"format_version\": \"2\", \"current_working_directory\": \"" ROOT "/build/host\","
    " \"data_file\": \"x.gcda\", \"files\": ["
    " {\"file\": \"" ROOT "/programs/ted/src/core/tokit.c\", \"functions\": ["
    "  {\"name\": \"tokit\", \"start_line\": 10, \"end_line\": 20, \"execution_count\": 3},"
    "  {\"name\": \"tokskip\", \"start_line\": 22, \"end_line\": 30, \"execution_count\": 0}],"
    "  \"lines\": [{\"line_number\": 10, \"count\": 3}]},"
    " {\"file\": \"/usr/include/stdio.h\", \"functions\": ["
    "  {\"name\": \"printf\", \"start_line\": 1, \"end_line\": 2, \"execution_count\": 1}]},"
    " {\"file\": \"_deps/snitch-src/snitch.cpp\", \"functions\": ["
    "  {\"name\": \"run\", \"start_line\": 1, \"end_line\": 2, \"execution_count\": 1}]}]}\n"
    "{\"current_working_directory\": \"" ROOT "/build/host\", \"files\": ["
    " {\"file\": \"../../programs/ted/src/core/alpha.c\", \"functions\": ["
    "  {\"name\": \"alpha\", \"start_line\": 5, \"end_line\": 9, \"execution_count\": 1}]}]}\n";
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

    covmap_init(&m, ROOT);
    CHECK(!covmap_drop(&m, ROOT "/build"));
    CHECK(!covmap_add_gcov(&m, "host/ted_core_b", gcov_b, strlen(gcov_b), err, sizeof err));
    CHECK(!covmap_add_gcov(&m, "host/ted_core_a", gcov_a, strlen(gcov_a), err, sizeof err));
    t = map_text(&m);
    CHECK(!strncmp(t, "# commit abc123\n", 16));
    CHECK(contains(t, "host\tprograms/ted/src/core/tokit.c\ttokit\t10\t20\thost/ted_core_a host/ted_core_b"));
    CHECK(contains(t, "host\tprograms/ted/src/core/tokit.c\ttokskip\t22\t30\thost/ted_core_b"));
    CHECK(contains(t, "host\tprograms/ted/src/core/tokit.c\t\t\t\thost/ted_core_a host/ted_core_b"));
    CHECK(contains(t, "host\tprograms/ted/src/core/alpha.c\talpha\t5\t9\thost/ted_core_a"));
    CHECK(!strstr(t, "stdio.h") && !strstr(t, "snitch"));
    /* the object line follows the file's functions */
    CHECK(strstr(t, "tokskip") < strstr(t, "tokit.c\t\t\t\t"));
    free(t);

    /* broken: no JSON, JSON that is no gcov output, a function without name */
    static const char *const bad[][2] = {
        { "{\"files\": [", "gcov JSON" },
        { "{\"a\": 1}", "no \"files\"" },
        { "{\"files\": [{\"file\": \"a.c\", \"functions\": [{}]}]}", "without \"name\"" },
        { "{\"files\": [{\"file\": \"a.c\"}]}", "without \"file\" or \"functions\"" },
    };
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++)
        CHECK(covmap_add_gcov(&m, "host/x", bad[i][0], strlen(bad[i][0]), err, sizeof err)
              && strstr(err, bad[i][1]));
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
    t = map_text(&m);
    /* main.c: source from the deps, no line ranges (no -g object) */
    CHECK(contains(t, "blitz2\tprograms/blitz2/src/main.c\tmain\t\t\tcorpus"));
    CHECK(contains(t, "blitz2\tprograms/blitz2/src/main.c\tcompile\t\t\tab3_cli"));
    CHECK(contains(t, "blitz2\tprograms/blitz2/src/main.c\t\t\t\tab3_cli corpus"));
    /* eval.c: the static function from nm, its range from the line table
       (the header's rows do not count) */
    CHECK(contains(t, "blitz2\tprograms/blitz2/src/eval.c\teval\t100\t120\tcorpus"));
    CHECK(contains(t, "blitz2\tprograms/blitz2/src/eval.c\tevalsub\t200\t230\tab3_cli"));
    CHECK(contains(t, "blitz2\tprograms/blitz2/src/eval.c\t\t\t\tab3_cli corpus"));
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
    CHECK(covmap_add_cov(&m, "x", "blitz2-ab3", NULL, 0, err, sizeof err) && strstr(err, "no link map"));
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
    t = map_text(&m);
    /* 0x120 is in the static function, which only the map's eval holds */
    CHECK(contains(t, "blitz2\tprograms/blitz2/src/eval.c\teval\t\t\tab3_cli"));
    CHECK(contains(t, "blitz2\tprograms/blitz2/src/main.c\tmain\t\t\t"));
    free(t);
    covmap_free(&m);
}

int main(void)
{
    test_json();
    test_paths();
    test_deps();
    test_gcov();
    test_amiga();
    if (failures)
        fprintf(stderr, "%d failure(s)\n", failures);
    return failures != 0;
}
