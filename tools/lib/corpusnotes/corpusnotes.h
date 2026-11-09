/*
 * corpusnotes.h - the intended deviations of the corpus programs from the
 * original (tests/corpus/notes.tsv).
 *
 * The corpus runs that compare with the original (ctest label originals:
 * the configurations ref, the original compiler with the CD's libraries,
 * and port, blitz2-c with the CD's libraries) judge a program against the
 * committed expected output and against ref's executable.  A fix of an
 * original bug makes the port differ from the original on purpose; the
 * fix commit lists the program here.  One row per program and pass:
 *
 *   PASS<TAB>NAME<TAB>#NNN: REASON
 *
 * PASS is a corpus pass (corpus, corpus_runerr, corpus_req, corpus_serial,
 * corpus_serial_runerr), NAME the source without .bb2, NNN the issue of
 * the fix, REASON a short text.  Empty lines and lines beginning with a
 * blank or "#" are comments.  The rule is that of the other lists of
 * intended differences (PREV_INTENDED, the note rows of
 * tests/data/blitz2-cd.tsv): a listed program may differ from the
 * original ("noted"), an unlisted difference is an error, and a listed
 * program that does not differ in any configuration is a stale note
 * (error).  A program is listed once per pass.
 */
#ifndef CORPUSNOTES_H
#define CORPUSNOTES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cnote {
    char *pass, *name, *text;           /* text: "#NNN: REASON" */
    unsigned issue;                     /* NNN */
    unsigned line;                      /* in the file */
    int differed;                       /* judged: it differed in a configuration */
    int seen;                           /* judged at all */
};

struct cnotes {
    struct cnote *e;
    size_t n;
};

/**
 * @brief Parse the text of a notes file, adding its rows to cn.
 * @return 0 = ok, -1 = malformed (a row not of the form, an unknown pass,
 *         a bad name, no issue number, no reason, a program twice) with the
 *         message "line N: ..." in err
 */
int cnotes_parse(struct cnotes *cn, const char *text, char *err, size_t errlen);
void cnotes_free(struct cnotes *cn);

/** @brief 1 = pass is the name of a corpus pass notes can name */
int cnotes_known_pass(const char *pass);

enum { CNOTE_SAME, CNOTE_NOTED, CNOTE_DIFFERS };

/**
 * @brief Judge one comparison (one configuration) of a program of pass.
 * @param differs  the configuration differs from what the original gives
 * @param note     out (may be NULL): the row, for CNOTE_NOTED
 * @return CNOTE_SAME (no difference), CNOTE_NOTED (differs, listed),
 *         CNOTE_DIFFERS (differs, not listed: an error)
 */
int cnotes_judge(struct cnotes *cn, const char *pass, const char *name, int differs,
                 const struct cnote **note);

/**
 * @brief The next stale note of pass: a program that never differed (or
 *        that was never judged: a typo, a program of another pass).
 * @param cursor  0 at the start
 * @return the row, NULL = no more (valid until cnotes_free)
 */
const struct cnote *cnotes_stale(const struct cnotes *cn, const char *pass, size_t *cursor);

#ifdef __cplusplus
}
#endif

#endif
