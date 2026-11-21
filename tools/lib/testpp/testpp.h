/*
 * testpp.h - the preprocessor of the Blitz test sources (`ID.test.bas`,
 * change basic-test-runner, design D6a; the contract: spec
 * testing/basic-test-interface, "The preprocessed form").
 *
 * It finds the calls of the check macros (`!Check`, `!CheckEq`,
 * `!CheckStrEq`, `!Require`, `!ExpectError`) the way the compiler finds
 * macro calls (blitz2's expandmax: a `!` outside strings and comments,
 * the macro name, blanks, `{`), splits their arguments by the compiler's
 * rules (findit, programs/blitz2/src/compile.c: a `,` or `}` outside
 * strings and outside `(` `{` nesting ends an argument; a byte with bit 7
 * set takes the next byte with it), and
 *
 *   - numbers them with K, from 0 over the whole source in source order,
 *     and writes K as the first argument, removing the name and the
 *     message (`!CheckEq{ACTUAL,EXPECTED,"name"}` -> `!CheckEq{K,ACTUAL,EXPECTED}`,
 *     `!ExpectError{"MESSAGE"}` -> `!ExpectError{K}`);
 *   - keeps the table of every check: K, kind, namespace (the case, the
 *     procedure, `init` for the batch init, `end` for the batch end), the
 *     number in the namespace (from 0), the name, the line and the text of
 *     the expressions (the message of `!ExpectError`) - none of that text
 *     reaches the program;
 *   - replaces the fixed placeholders @LINE@ (the source line), @CASE@
 *     (the case the line stands in), @TEST@ (the test id) and @TESTDATA@
 *     (the location of the test data, a parameter) in the code and the
 *     strings of the source, never in comments, and refuses any other
 *     @NAME@ (NAME [A-Z][A-Z0-9_]*).
 *
 * It refuses a check inside a macro definition of the source (`Macro M`
 * ... `End Macro`; a check as an argument of a macro call is allowed), a
 * check outside the blocks and procedures, a check inside the arguments of
 * a check, a call without `{`, a wrong number of arguments, a name or
 * message that is no string literal, an empty argument, the same check
 * name twice in a namespace, and a procedure holding checks whose name is
 * that of a case or `init` or `end`.  Every message names file and line.
 *
 * The preprocessed text has the lines of the source, so a compile error
 * of the driver keeps its line.
 */
#ifndef TESTPP_H
#define TESTPP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The check macros. */
enum tp_kind {
    TP_CHECK,                   /* !Check{COND} */
    TP_CHECKEQ,                 /* !CheckEq{ACTUAL,EXPECTED} */
    TP_CHECKSTREQ,              /* !CheckStrEq{ACTUAL$,EXPECTED$} */
    TP_REQUIRE,                 /* !Require{COND} */
    TP_EXPECTERROR              /* !ExpectError{"MESSAGE"} */
};

/** @brief The kinds of namespaces. */
enum tp_nskind {
    TP_NS_CASE,
    TP_NS_PROC,
    TP_NS_INIT,                 /* the batch init: "init" */
    TP_NS_END                   /* the batch end: "end" */
};

/** @brief A check of the table. */
struct tp_check {
    unsigned k;                 /* the number over the source */
    int kind;                   /* enum tp_kind */
    int nskind;                 /* enum tp_nskind */
    char *ns;                   /* the case, the procedure, "init", "end" */
    unsigned number;            /* the number in the namespace */
    char *name;                 /* the name, NULL: none */
    unsigned line;
    char *expr;                 /* COND or ACTUAL as written (trimmed); NULL
                                   for !ExpectError */
    char *expected;             /* EXPECTED (CheckEq, CheckStrEq), else NULL */
    char *message;              /* the message of !ExpectError, else NULL */
};

/** @brief A case of the source. */
struct tp_case {
    char *name;
    unsigned line;              /* the line of !TestCase */
    unsigned end_line;          /* of !EndTestCase, 0: none */
};

/** @brief A message: "FILE:LINE: TEXT". */
struct tp_errors {
    char **v;
    size_t n;
};

/** @brief The result of a source. */
struct tp_source {
    char *text;                 /* the preprocessed text */
    size_t len;
    struct tp_check *checks;
    size_t nchecks;
    struct tp_case *cases;
    size_t ncases;
    unsigned init_line;         /* the line of !TestBatchInit, 0: none */
    unsigned final_line;        /* of !TestBatchEnd, 0: none */
    unsigned nlines;
    struct tp_errors errors;
};

/** @brief The parameters of a run. */
struct tp_params {
    const char *file;           /* the source's name in the messages */
    const char *test_id;        /* @TEST@ */
    const char *testdata;       /* @TESTDATA@; NULL: refused as unknown */
    const char *const *tokens;  /* the names of the tokens of every library
                                   set (the form check, tp_check_form) */
    size_t ntokens;
};

/**
 * @brief Preprocess a source.
 * @param text  the source, text[0..len): lines ended by LF (a CR before
 *              the LF stays), a 0 byte refused
 * @return the number of messages in src->errors (0: src->text is the
 *         preprocessed text), -1 = out of memory
 */
int tp_process(const char *text, size_t len, const struct tp_params *par, struct tp_source *src);

/** @brief Free a result (zeroed). */
void tp_free(struct tp_source *src);

/** @brief The macro name of a kind ("CheckEq"). */
const char *tp_kind_name(int kind);

/**
 * @brief The check of namespace ns with name name, or NULL.
 */
const struct tp_check *tp_find(const struct tp_source *src, const char *ns, const char *name);

/** @brief The check with number k, or NULL. */
const struct tp_check *tp_by_k(const struct tp_source *src, unsigned k);

#ifdef __cplusplus
}
#endif

#endif
