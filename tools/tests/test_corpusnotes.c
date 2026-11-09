/*
 * test_corpusnotes.c - the notes of the corpus programs (lib/corpusnotes,
 * tests/corpus/notes.tsv): listed and different (noted), listed and the
 * same (stale), not listed and different, several configurations, comments,
 * and malformed rows.
 */
#include "testutil.h"
#include "corpusnotes.h"

static int parse(struct cnotes *cn, const char *text, char *err, size_t len)
{
    return cnotes_parse(cn, text, err, len);
}

/* the text is refused with msg in the message */
static int refused(const char *text, const char *msg)
{
    struct cnotes cn = { 0 };
    char err[300] = "";
    int r = parse(&cn, text, err, sizeof err);

    cnotes_free(&cn);
    if (r != -1 || !strstr(err, msg)) {
        fprintf(stderr, "text \"%s\": result %d, message \"%s\", wanted \"%s\"\n", text, r, err,
                msg);
        return 0;
    }
    return 1;
}

static void test_judge(void)
{
    struct cnotes cn = { 0 };
    char err[300];
    const struct cnote *n;
    size_t c = 0;

    CHECK(parse(&cn, "# the notes\n\n"
                "corpus\tt130_blit\t#46: BlitColl range check\n"
                "corpus_runerr\tt130_blit\t#46: BlitColl range check\n"
                "corpus_runerr\tt124_bitmaperr\t#6: error text\n"
                " indented comment\r\n"
                "corpus_serial\tt64_serialrx\t#99: gone\r\n", err, sizeof err) == 0);
    CHECK(cn.n == 4 && cn.e[0].issue == 46 && cn.e[0].line == 3 && cn.e[3].line == 7);
    CHECK(!strcmp(cn.e[3].text, "#99: gone"));
    /* listed and different: noted, with the row */
    CHECK(cnotes_judge(&cn, "corpus", "t130_blit", 1, &n) == CNOTE_NOTED && n == &cn.e[0]);
    /* a second configuration of the same program, the same */
    CHECK(cnotes_judge(&cn, "corpus", "t130_blit", 1, &n) == CNOTE_NOTED);
    /* listed, a configuration that does not differ: fine */
    CHECK(cnotes_judge(&cn, "corpus", "t130_blit", 0, &n) == CNOTE_SAME && !n);
    /* not listed and different: an error; not listed and the same: fine */
    CHECK(cnotes_judge(&cn, "corpus", "t01_int", 1, &n) == CNOTE_DIFFERS && !n);
    CHECK(cnotes_judge(&cn, "corpus", "t01_int", 0, NULL) == CNOTE_SAME);
    /* listed for another pass only */
    CHECK(cnotes_judge(&cn, "corpus_req", "t130_blit", 1, NULL) == CNOTE_DIFFERS);
    /* listed and the same: stale; never judged (t124_bitmaperr in corpus_runerr): stale too */
    CHECK(cnotes_judge(&cn, "corpus_runerr", "t130_blit", 0, NULL) == CNOTE_SAME);
    CHECK(cnotes_stale(&cn, "corpus", &c) == NULL);
    c = 0;
    CHECK(cnotes_stale(&cn, "corpus_runerr", &c) == &cn.e[1]);
    CHECK(cnotes_stale(&cn, "corpus_runerr", &c) == &cn.e[2]);
    CHECK(cnotes_stale(&cn, "corpus_runerr", &c) == NULL);
    c = 0;
    CHECK(cnotes_stale(&cn, "corpus_serial", &c) == &cn.e[3]);
    /* the same program differing in one pass only: the other pass is stale, not this one */
    CHECK(cnotes_judge(&cn, "corpus_runerr", "t124_bitmaperr", 1, NULL) == CNOTE_NOTED);
    c = 0;
    CHECK(cnotes_stale(&cn, "corpus_runerr", &c) == &cn.e[1]);
    CHECK(cnotes_stale(&cn, "corpus_runerr", &c) == NULL);
    cnotes_free(&cn);
}

static void test_empty(void)
{
    struct cnotes cn = { 0 };
    char err[300];
    size_t c = 0;

    /* an empty list (comments only): every difference is an error, nothing is stale */
    CHECK(parse(&cn, "# comments only\n", err, sizeof err) == 0 && cn.n == 0);
    CHECK(cnotes_judge(&cn, "corpus", "t01_int", 1, NULL) == CNOTE_DIFFERS);
    CHECK(cnotes_stale(&cn, "corpus", &c) == NULL);
    cnotes_free(&cn);
    CHECK(parse(&cn, "", err, sizeof err) == 0 && cn.n == 0);
    cnotes_free(&cn);
}

static void test_malformed(void)
{
    CHECK(refused("corpus\tt01_int\n", "line 1: PASS<TAB>NAME<TAB>"));
    CHECK(refused("corpus t01_int #6: text\n", "PASS<TAB>NAME<TAB>"));
    CHECK(refused("corpus\tt01_int\t#6: a\textra\n", "PASS<TAB>NAME<TAB>"));
    CHECK(refused("# c\ncorpuss\tt01_int\t#6: text\n", "line 2: unknown pass \"corpuss\""));
    CHECK(refused("ted_ab\tt01_int\t#6: text\n", "unknown pass \"ted_ab\""));
    CHECK(refused("corpus\t\t#6: text\n", "program name"));
    CHECK(refused("corpus\tt01 int\t#6: text\n", "program name"));
    CHECK(refused("corpus\tcorpus:t01\t#6: text\n", "program name"));
    CHECK(refused("corpus\tt01_int\tno issue\n", "#NNN: REASON"));
    CHECK(refused("corpus\tt01_int\t#: text\n", "#NNN: REASON"));
    CHECK(refused("corpus\tt01_int\t#0: text\n", "#NNN: REASON"));
    CHECK(refused("corpus\tt01_int\t#6:text\n", "#NNN: REASON"));
    CHECK(refused("corpus\tt01_int\t#6: \n", "#NNN: REASON"));
    CHECK(refused("corpus\tt01_int\t#6\n", "#NNN: REASON"));
    CHECK(refused("corpus\tt01_int\t#6: a\ncorpus\tt01_int\t#7: b\n",
                  "line 2: corpus:t01_int is listed twice (line 1)"));
    /* the same name in another pass is fine */
    {
        struct cnotes cn = { 0 };
        char err[300];

        CHECK(parse(&cn, "corpus\tt01_int\t#6: a\ncorpus_runerr\tt01_int\t#6: a\n", err,
                    sizeof err) == 0 && cn.n == 2);
        cnotes_free(&cn);
    }
}

int main(void)
{
    test_judge();
    test_empty();
    test_malformed();
    return failures ? 1 : 0;
}
