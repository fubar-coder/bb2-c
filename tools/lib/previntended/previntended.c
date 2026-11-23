#include "previntended.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const tests[] = { "corpus", "corpus_runerr", "corpus_req", "corpus_serial",
                                     "corpus_serial_runerr", "ted_ab" };
static const char *const pairs[] = { "C5-P5", "Cc-Pc" };

/* the names ending in _prev (the compile-only comparisons: corpus_cd_prev,
   compiler_prev, bug_orig_ID_prev, ab3_cli_prev ...), the passes of the
   site examples and the corpus passes of the expression matrix
   (corpus_exprmatrix_NAME, corpus_exprmatrix_runerr_NAME) are tests too */
int prevint_known_test(const char *test)
{
    size_t i, n = strlen(test);

    for (i = 0; i < sizeof tests / sizeof tests[0]; i++)
        if (!strcmp(test, tests[i]))
            return 1;
    if (!strncmp(test, "site_examples_", 14) || !strncmp(test, "corpus_exprmatrix_", 18))
        return 1;
    return n > 5 && !strcmp(test + n - 5, "_prev");
}

/* the comparison of a test of the BASIC test model (blitz2_ID_prev,
   bug_orig_ID_prev, regression_ID_prev): its data sources are named
   ID/FILE (change basic-test-local-data) */
static int model_prev(const char *test)
{
    size_t n = strlen(test);

    return n > 5 && !strcmp(test + n - 5, "_prev")
           && (!strncmp(test, "blitz2_", 7) || !strncmp(test, "bug_orig_", 9) || !strncmp(test, "regression_", 11));
}

static int bad(char *err, size_t errlen, const char *entry, const char *why)
{
    snprintf(err, errlen, "PREV_INTENDED entry \"%s\": %s", entry, why);
    return -1;
}

/* a name: not empty, printable, none of the separators; a path (the CD's
   sources) has slashes */
static int name_ok(const char *s, int path)
{
    size_t i, n = strlen(s);

    for (i = 0; i < n; i++)
        if ((unsigned char)s[i] <= ' ' || strchr(path ? ":;|\\=" : ":;|/\\=", s[i]))
            return 0;
    return n > 0;
}

int prevint_add(struct prevint *pi, const char *entry, char *err, size_t errlen)
{
    const char *c = strchr(entry, ':'), *c2, *why = NULL;
    struct prevint_entry *v = NULL, x = { 0 };
    size_t i;

    if (!strcmp(entry, "ON")) {
        pi->all = 1;
        return 0;
    }
    if (!c)
        return 1;
    c2 = strchr(c + 1, ':');
    x.test = strndup(entry, (size_t)(c - entry));
    x.name = strndup(c + 1, c2 ? (size_t)(c2 - c - 1) : strlen(c + 1));
    x.pair = c2 ? strdup(c2 + 1) : NULL;
    x.text = strdup(entry);
    if (!x.test || !x.name || !x.text || (c2 && !x.pair))
        why = "out of memory";
    else if (!prevint_known_test(x.test))
        why = "unknown test (corpus, corpus_runerr, corpus_req, corpus_serial, "
              "corpus_serial_runerr, ted_ab, site_examples_*, corpus_exprmatrix_* or one ending in _prev)";
    else if (!name_ok(x.name, !strncmp(x.test, "corpus_cd", 9) || model_prev(x.test)))
        why = "the source or case name is empty or has a character of : ; | / \\ = or a blank";
    else if (x.pair && strcmp(x.test, "ted_ab"))
        why = "only ted_ab entries have a pair";
    else if (x.pair) {
        why = "the pair is C5-P5 or Cc-Pc";
        for (i = 0; i < sizeof pairs / sizeof pairs[0]; i++)
            if (!strcmp(x.pair, pairs[i]))
                why = NULL;
    }
    for (i = 0; !why && i < pi->n; i++)
        if (!strcmp(pi->e[i].text, entry))
            why = "named twice";
    if (!why && !(v = realloc(pi->e, (pi->n + 1) * sizeof *v)))
        why = "out of memory";
    if (why) {
        free(x.test);
        free(x.name);
        free(x.pair);
        free(x.text);
        return bad(err, errlen, entry, why);
    }
    pi->e = v;
    pi->e[pi->n++] = x;
    return 0;
}

void prevint_free(struct prevint *pi)
{
    size_t i;

    for (i = 0; i < pi->n; i++) {
        free(pi->e[i].text);
        free(pi->e[i].test);
        free(pi->e[i].name);
        free(pi->e[i].pair);
    }
    free(pi->e);
    memset(pi, 0, sizeof *pi);
}

int prevint_judge(struct prevint *pi, const char *test, const char *name, const char *pair,
                  int differs)
{
    int listed = 0;
    size_t i;

    for (i = 0; i < pi->n; i++) {
        struct prevint_entry *e = &pi->e[i];

        if (strcmp(e->test, test) || strcmp(e->name, name)
            || (e->pair && (!pair || strcmp(e->pair, pair))))
            continue;
        e->seen = 1;
        if (differs) {
            e->differed = 1;
            listed = 1;
        }
    }
    if (!differs)
        return PREVINT_SAME;
    return listed || pi->all ? PREVINT_INTENDED : PREVINT_DIFFERS;
}

const char *prevint_stale(const struct prevint *pi, const char *test, int partial, size_t *cursor)
{
    while (*cursor < pi->n) {
        const struct prevint_entry *e = &pi->e[(*cursor)++];

        if (!strcmp(e->test, test) && !e->differed && (e->seen || !partial))
            return e->text;
    }
    return NULL;
}
