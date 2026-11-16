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

    return failures ? 1 : 0;
}
