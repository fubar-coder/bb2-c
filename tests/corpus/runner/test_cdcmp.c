/*
 * test_cdcmp.c - the comparison of a CD result table with the reference
 * (cdcmp.h): a different compile result (result, line, message) or a
 * missing row fails; another size or hash of an ok row is only reported.
 * The tables are built in memory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdcmp.h"

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%s:%d: CHECK failed: %s\n", \
                                          __FILE__, __LINE__, #c); failures++; } } while (0)

#define H1 "1111111111111111111111111111111111111111111111111111111111111111"
#define H2 "2222222222222222222222222222222222222222222222222222222222222222"
#define HEAD "path\tresult\tline\tmessage\tsize\tsha256_masked\txtra\tnote\n"

/* the reference: an ok row, an error row, a third ok row */
#define REF HEAD \
    "A/ONE.BB2\tok\t\t\t100\t" H1 "\town\t\n" \
    "A/TWO.BB2\terror\t7\tSyntax Error\t\t\town\t\n" \
    "A/THREE.BB2\tok\t\t\t300\t" H1 "\town\t#940: a note\n"

static int cmp(const char *out, struct cdcmp_stats *st, char **rep)
{
    return cdcmp_compare("ref.tsv", "out.tsv", REF, out, rep, st);
}

int main(void)
{
    struct cdcmp_stats st;
    char *rep;
    int rc;

    /* the same table */
    rc = cmp(REF, &st, &rep);
    CHECK(rc == 0 && st.sources == 3 && st.equal == 3 && !st.different && !st.missing
          && !st.exe_differ);
    CHECK(strstr(rep, "3 sources, 3 equal, 0 different, 0 missing\n"));
    free(rep);

    /* another executable only (size and hash): information, no difference */
    rc = cmp(HEAD
             "A/ONE.BB2\tok\t\t\t96\t" H2 "\town\t\n"
             "A/TWO.BB2\terror\t7\tSyntax Error\t\t\town\t\n"
             "A/THREE.BB2\tok\t\t\t300\t" H2 "\town\t#940: a note\n",
             &st, &rep);
    CHECK(rc == 0 && st.equal == 3 && st.exe_differ == 2 && !st.different);
    CHECK(strstr(rep, "A/ONE.BB2: executable differs (informational): size 100 -> 96, "
                      "sha256_masked 111111111111 -> 222222222222"));
    CHECK(strstr(rep, "2 of the equal with another executable: information"));
    free(rep);

    /* the compile result differs: an error line, a message, ok -> error,
       error -> ok: each one fails and names the fields */
    rc = cmp(HEAD
             "A/ONE.BB2\tok\t\t\t100\t" H1 "\town\t\n"
             "A/TWO.BB2\terror\t8\tSyntax Error\t\t\town\t\n"
             "A/THREE.BB2\tok\t\t\t300\t" H1 "\town\t\n",
             &st, &rep);
    CHECK(rc == 1 && st.different == 1 && st.equal == 2);
    CHECK(strstr(rep, "A/TWO.BB2: line 7 -> 8"));
    free(rep);
    rc = cmp(HEAD
             "A/ONE.BB2\tok\t\t\t100\t" H1 "\town\t\n"
             "A/TWO.BB2\terror\t7\tIllegal Function\t\t\town\t\n"
             "A/THREE.BB2\tok\t\t\t300\t" H1 "\town\t\n",
             &st, &rep);
    CHECK(rc == 1 && strstr(rep, "message Syntax Error -> Illegal Function"));
    free(rep);
    rc = cmp(HEAD
             "A/ONE.BB2\terror\t3\tSyntax Error\t\t\town\t\n"
             "A/TWO.BB2\tok\t\t\t10\t" H1 "\town\t\n"
             "A/THREE.BB2\tok\t\t\t300\t" H1 "\town\t\n",
             &st, &rep);
    CHECK(rc == 1 && st.different == 2
          && strstr(rep, "A/ONE.BB2: result ok -> error line  -> 3 message  -> Syntax Error")
          && strstr(rep, "A/TWO.BB2: result error -> ok line 7 ->  message Syntax Error -> "));
    free(rep);

    /* a missing row and an extra row fail */
    rc = cmp(HEAD
             "A/ONE.BB2\tok\t\t\t100\t" H1 "\town\t\n"
             "A/TWO.BB2\terror\t7\tSyntax Error\t\t\town\t\n"
             "A/FOUR.BB2\tok\t\t\t1\t" H1 "\town\t\n",
             &st, &rep);
    CHECK(rc == 1 && st.missing == 2 && strstr(rep, "A/THREE.BB2: not in out.tsv")
          && strstr(rep, "A/FOUR.BB2: not in the reference"));
    free(rep);

    /* a malformed table */
    rc = cmp(HEAD "A/ONE.BB2\tok\n", &st, &rep);
    CHECK(rc == 2 && strstr(rep, "out.tsv: line 2: not 8 tab separated fields"));
    free(rep);

    /* the comparison with the previous commit (cdcmp_prev): every field,
       the executable too, must be the same */
    {
        struct prevint pi = { 0 };
        char msg[300];
        const char *same = REF;
        const char *exe = HEAD
            "A/ONE.BB2\tok\t\t\t96\t" H2 "\town\t\n"
            "A/TWO.BB2\terror\t7\tSyntax Error\t\t\town\t\n"
            "A/THREE.BB2\tok\t\t\t300\t" H1 "\town\t#940: a note\n";

        /* the same: no difference */
        rc = cdcmp_prev("corpus_cd_prev", "cur.tsv", "prev.tsv", REF, same, &pi, &rep, &st);
        CHECK(rc == 0 && st.sources == 3 && st.equal == 3 && !st.different);
        CHECK(strstr(rep, "3 sources, 3 the same as prev, 0 different (0 intended), 0 not in both"));
        CHECK(!strstr(rep, "prev differs in"));
        free(rep);
        /* another executable (size and hash): a difference, unlike in the
           comparison with the original; unlisted it fails and names the entry */
        rc = cdcmp_prev("corpus_cd_prev", "cur.tsv", "prev.tsv", REF, exe, &pi, &rep, &st);
        CHECK(rc == 1 && st.different == 1
              && strstr(rep, "A/ONE.BB2: DIFFERS (prev -> now): size 96 -> 100; sha256_masked "
                          "2222222222222222222222222222222222222222 -> "
                          "1111111111111111111111111111111111111111;"));
        CHECK(!strstr(rep, "(intended)")
              && strstr(rep, "prev differs in: PREV_INTENDED=\"corpus_cd_prev:A/ONE.BB2\""));
        free(rep);
        /* listed: intended, passes (the row is reported) */
        CHECK(prevint_add(&pi, "corpus_cd_prev:A/ONE.BB2", msg, sizeof msg) == 0);
        rc = cdcmp_prev("corpus_cd_prev", "cur.tsv", "prev.tsv", REF, exe, &pi, &rep, &st);
        CHECK(rc == 0 && strstr(rep, "(intended)") && strstr(rep, "1 different (1 intended)"));
        free(rep);
        prevint_free(&pi);
        CHECK(prevint_add(&pi, "corpus_cd_prev:A/ONE.BB2", msg, sizeof msg) == 0);
        /* listed, but the same: stale */
        rc = cdcmp_prev("corpus_cd_prev", "cur.tsv", "prev.tsv", REF, same, &pi, &rep, &st);
        CHECK(rc == 1 && strstr(rep, "STALE corpus_cd_prev:A/ONE.BB2"));
        free(rep);
        /* the entry of another test does not count; a row that is in one table
           only is a difference, a result that changed too */
        rc = cdcmp_prev("corpus_cd_ab3_prev", "cur.tsv", "prev.tsv", REF, HEAD
                        "A/ONE.BB2\terror\t3\tSyntax Error\t\t\town\t\n"
                        "A/TWO.BB2\terror\t7\tSyntax Error\t\t\town\t\n"
                        "A/FIVE.BB2\tok\t\t\t1\t" H1 "\town\t\n", &pi, &rep, &st);
        CHECK(rc == 1 && st.missing == 2 && st.different == 3);
        CHECK(strstr(rep, "A/ONE.BB2: DIFFERS (prev -> now): result error -> ok;")
              && strstr(rep, "A/THREE.BB2: DIFFERS (prev -> now): not in the prev table")
              && strstr(rep, "A/FIVE.BB2: DIFFERS (prev -> now): only in the prev table"));
        free(rep);
        prevint_free(&pi);
        /* ON: every difference is intended */
        CHECK(prevint_add(&pi, "ON", msg, sizeof msg) == 0);
        rc = cdcmp_prev("corpus_cd_prev", "cur.tsv", "prev.tsv", REF, exe, &pi, &rep, &st);
        CHECK(rc == 0 && st.different == 1);
        free(rep);
        prevint_free(&pi);
        /* a malformed table */
        rc = cdcmp_prev("corpus_cd_prev", "cur.tsv", "prev.tsv", REF, HEAD "A/ONE.BB2\tok\n", &pi, &rep,
                        &st);
        CHECK(rc == 2 && strstr(rep, "prev.tsv: line 2: not 8 tab separated fields"));
        free(rep);
        prevint_free(&pi);
    }

    return failures ? 1 : 0;
}
