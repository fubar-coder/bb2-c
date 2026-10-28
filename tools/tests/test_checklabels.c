/*
 * test_checklabels.c - code_labels_parse (which labels are code) and
 * labels_check (missing, twice, ok, markers of other labels or files).
 */
#include "testutil.h"
#include "checklabels.h"

static const char asm_text[] =
    "start\tmoveq\t#0,d0\n"             /* 1  code */
    "\trts\n"                           /* 2 */
    "alone\n"                           /* 3  code (next statement) */
    "alias\t;comment only\n"            /* 4  code: alias of alone */
    "\tmove.l\td0,d1\n"                 /* 5 */
    ".local\tbra\tstart\n"              /* 6  local: not counted */
    "data\tdc.w\t1,2\n"                 /* 7  data */
    "table\tDCB.B\t4,0\n"               /* 8  data (any case, size) */
    "len\tequ\t4\n"                     /* 9  equ */
    "val\t=\t5\n"                       /* 10 equ */
    "* a comment line\n"                /* 11 */
    "finish\tmoveq\t#1,d0\n"            /* 12 end of data: not code */
    "colon:\tnop\n"                     /* 13 code, with colon */
    "inc\tinclude\tmyoffs\n"            /* 14 data (include) */
    "x=1\n"                             /* 15 no label syntax */
    "last\tmove\t(a0)+,d0 ; ; two\n";   /* 16 code */

static long check(const char *const *texts, size_t n, char *msg, size_t len)
{
    struct code_labels l;
    struct label_source src[4];
    FILE *out = tmpfile();
    long r;
    size_t i;

    if (code_labels_parse(asm_text, sizeof asm_text - 1, &l))
        abort();
    for (i = 0; i < n; i++) {
        src[i].name = i ? "b.c" : "a.c";
        src[i].text = texts[i];
        src[i].len = strlen(texts[i]);
    }
    r = labels_check(&l, "ted.s", src, n, 0, out);
    rewind(out);
    msg[fread(msg, 1, len - 1, out)] = 0;
    fclose(out);
    code_labels_free(&l);
    return r;
}

int main(void)
{
    static const char *const want[] = { "start", "alone", "alias", "colon", "last" };
    static const unsigned lines[] = { 1, 3, 4, 13, 16 };
    struct code_labels l;
    char msg[2000];
    size_t i;

    CHECK(code_labels_parse(asm_text, sizeof asm_text - 1, &l) == 0);
    CHECK(l.n == 5);
    for (i = 0; i < l.n && i < 5; i++) {
        CHECK(!strcmp(l.v[i].name, want[i]));
        CHECK(l.v[i].line == lines[i]);
    }
    code_labels_free(&l);

    /* all there, in two files; markers of other labels are ignored */
    {
        const char *t[] = { "/* ted.s:1 start */ x; /* ted.s:3 alone ted.s:4 alias */",
                            "// ted.s:13 colon\n// ted.s:16 last\n// ted.s:7 data ted.s:99 start" };

        CHECK(check(t, 2, msg, sizeof msg) == 0);
        CHECK(strstr(msg, "5 code labels, 0 missing, 0 more than once") != NULL);
    }
    /* broken: one missing, one twice (other file), wrong line, other prefix */
    {
        const char *t[] = { "ted.s:1 start ted.s:3 alone ted.s:5 alias blitz2.s:13 colon",
                            "ted.s:16 last ted.s:1 start ted.s:13 colonx" };

        CHECK(check(t, 2, msg, sizeof msg) == 3);
        CHECK(strstr(msg, "MISSING ted.s:4 alias\n") != NULL);
        CHECK(strstr(msg, "MISSING ted.s:13 colon\n") != NULL);
        CHECK(strstr(msg, "TWICE   ted.s:1 start: a.c, b.c\n") != NULL);
        CHECK(strstr(msg, "5 code labels, 2 missing, 1 more than once") != NULL);
    }
    /* twice in the same file */
    {
        const char *t[] = { "ted.s:1 start ted.s:3 alone ted.s:4 alias ted.s:13 colon "
                            "ted.s:16 last ted.s:16 last" };

        CHECK(check(t, 1, msg, sizeof msg) == 1);
        CHECK(strstr(msg, "TWICE   ted.s:16 last: a.c, a.c\n") != NULL);
    }
    return failures != 0;
}
