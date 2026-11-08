/*
 * previntended.h - the intended differences of the corpus and the TED A/B
 * tests against the prev build (the list PREV_INTENDED of CMakeLists.txt,
 * tests/README.md "The prev build").
 *
 * An entry names one comparison of one test:
 *
 *   TEST:NAME          the source NAME (without .bb2) of the corpus pass
 *                      TEST (corpus, corpus_runerr, corpus_req,
 *                      corpus_serial, corpus_serial_runerr): the prev
 *                      configuration may differ from clib in it
 *   ted_ab:CASE        the test CASE of ted_ab: the pairs C5-P5 and Cc-Pc
 *                      may differ (at least one must)
 *   ted_ab:CASE:PAIR   only that pair (C5-P5 or Cc-Pc) may differ
 *
 * (An entry without a colon is a library file path, prevcmp's.)  The rule
 * is that of the library files: a listed comparison must differ (a stale
 * entry fails), an unlisted difference fails, ON allows every difference.
 * TEST in the entry is the ctest name, so the same source in corpus and
 * corpus_runerr is listed twice, and a typo in a name is stale.
 */
#ifndef PREVINTENDED_H
#define PREVINTENDED_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct prevint_entry {
    char *text;                         /* as given */
    char *test, *name, *pair;           /* pair: NULL = not given */
    int differed;                       /* judged: it differed */
    int seen;                           /* judged at all */
};

struct prevint {
    struct prevint_entry *e;
    size_t n;
    int all;                            /* ON */
};

/**
 * @brief Add an entry (or ON: every difference is intended).
 * @return 0 = added, 1 = not an entry of this kind (no colon: a library
 *         path), -1 = malformed (message in err) or out of memory
 */
int prevint_add(struct prevint *pi, const char *entry, char *err, size_t errlen);
void prevint_free(struct prevint *pi);

enum { PREVINT_SAME, PREVINT_INTENDED, PREVINT_DIFFERS };

/**
 * @brief Judge one comparison of test (the ctest name).
 * @param name     the source or case
 * @param pair     the pair of a ted_ab case, else NULL
 * @param differs  the comparison found a difference
 * @return PREVINT_SAME (no difference), PREVINT_INTENDED (differs, listed or
 *         ON), PREVINT_DIFFERS (differs, not listed: an error)
 */
int prevint_judge(struct prevint *pi, const char *test, const char *name, const char *pair,
                  int differs);

/**
 * @brief The next stale entry of test: one that never differed.  Unless
 *        partial (not every item was run: ted_ab with a filter) this
 *        includes one never judged (a typo).
 * @param cursor  0 at the start
 * @return the entry text, NULL = no more (valid until prevint_free)
 */
const char *prevint_stale(const struct prevint *pi, const char *test, int partial,
                          size_t *cursor);

/** @brief 1 = test is the ctest name of a test an entry can name */
int prevint_known_test(const char *test);

#ifdef __cplusplus
}
#endif

#endif
