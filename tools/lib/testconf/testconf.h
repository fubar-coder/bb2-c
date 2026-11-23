/*
 * testconf.h - the run configuration of a Blitz test of the model of
 * change basic-test-runner (tests/README.md, "The run configuration"):
 * the files, their keys, the hierarchy and the merge.
 *
 * A test source `DIR/ID.test.bas` is configured by its own file
 * `DIR/ID.test.toml` (optional) and by the `tests.toml` files of DIR and
 * of the directories above it, up to and including the first one that
 * sets `root = true`.  The nearest file wins per key; the tables
 * (`matrix`, `asserts`, `asserts."SELECTOR"`) merge per key; a list value
 * replaces, never extends.  The files are in the TOML subset of
 * testtoml.h.
 *
 * The keys (tc_keys()): the run settings (compiler, libraries, machine,
 * runtime error checking, time limits, files, wrappers of the run, the
 * cells with an emulator session of their own, ...),
 * the keys of the registration (labels, cost, fixtures), `root` (only in
 * a `tests.toml`), the keys of a single test (`fixed`, `fixed-orig`,
 * `extension` - at most one of the three -, the table `deviations`; only
 * in an `ID.test.toml`), `exclude` and the
 * tables.  An unknown key or table is an error naming file, line and key.
 *
 * A relative path in a path setting (testconf keys with a path field) is
 * relative to the directory of the file that sets it; a placeholder
 * `@NAME@` in a string value or a list element of a setting or an axis is
 * replaced from the environment file, an unknown one is an error.
 */
#ifndef TESTCONF_H
#define TESTCONF_H

#include <stddef.h>
#include "testtoml.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The kind of a key's value. */
enum tc_vkind {
    TC_V_STRING,                /* a string */
    TC_V_ENUM,                  /* a string, one of tc_keydef.values */
    TC_V_INT,                   /* an integer >= tc_keydef.min */
    TC_V_BOOL,                  /* true or false */
    TC_V_LIST,                  /* a list of strings */
    TC_V_ONOFF_OR_SELECTORS     /* a string, one of tc_keydef.values, or a
                                   non-empty list of cell selectors (isolate) */
};

/** @brief Where a key is valid. */
enum tc_where {
    TC_IN_DIR = 1,              /* in a directory's tests.toml */
    TC_IN_TEST = 2,             /* in a test's own ID.test.toml */
    TC_IN_BOTH = 3
};

/** @brief What a key is. */
enum tc_keyflags {
    TC_RUN = 1,                 /* a run setting: may be an axis (if no list) */
    TC_REG = 2                  /* a setting of the registered ctest test */
};

/** @brief A key of the top table. */
struct tc_keydef {
    const char *name;
    int vkind;                  /* enum tc_vkind */
    unsigned where;             /* enum tc_where */
    unsigned flags;             /* enum tc_keyflags */
    const char *values;         /* TC_V_ENUM: the values, "|" between them */
    long long min;              /* TC_V_INT: the least value */
    int pathfield;              /* -1, or the field ("|" between the fields,
                                   from 0) of the string or of every list
                                   element that is a path */
    const char *doc;            /* one line for the documentation */
    const char *defval;         /* the value of a cell when no file sets the
                                   key, NULL: none */
};

/** @brief The keys of the top table, in their fixed order; *n their number. */
const struct tc_keydef *tc_keys(size_t *n);

/** @brief The key definition of name, or NULL. */
const struct tc_keydef *tc_key(const char *name);

/** @brief Whether value is one of def's values (TC_V_ENUM). */
int tc_enum_ok(const struct tc_keydef *def, const char *value);

/** @brief The origin of a key: file, line and the distance from the test. */
struct tc_origin {
    const char *file;           /* points into tc_config.files */
    unsigned line;
    unsigned depth;             /* 0 the test's own file, 1 the tests.toml of
                                   its directory or the nearest one, ... */
};

/** @brief An effective key of the top table. */
struct tc_setting {
    const struct tc_keydef *def;
    struct tt_value value;      /* placeholders replaced, paths made absolute */
    struct tc_origin at;
};

/** @brief A key of a table (matrix, asserts, deviations). */
struct tc_tabkey {
    char *sel;                  /* the selector of the table's name
                                   (asserts."SEL", deviations."SEL"), NULL none */
    char *key;
    struct tt_value value;      /* matrix: placeholders replaced */
    struct tc_origin at;
};

/** @brief A placeholder of the environment file. */
struct tc_placeholder {
    char *name;                 /* without the @ */
    char *value;
};

/**
 * @brief The environment of a build (one file per build, written by
 *        CMake): the placeholders and the directory the search for the
 *        root stops at.  The file, in the subset of testtoml.h:
 *
 *            top = "/path/of/the/repository"
 *            [placeholders]
 *            TESTDATA = "/path/of/the/test/data"
 *            [runner]
 *            blitz2 = "/path/of/the/build/blitz2/blitz2"
 *
 *        The table `runner` holds the paths and values only the runner
 *        uses (corpusrun case, tests/corpus/runner/caserun.h), strings
 *        named by any key; the configuration never sees them.
 */
struct tc_env {
    char *file;
    char *top;                  /* NULL: the search goes up to "/" */
    struct tc_placeholder *ph;
    size_t nph;
    struct tc_placeholder *run; /* the table runner: key and value */
    size_t nrun;
};

/** @brief The value of key in the table runner of env, or NULL. */
const char *tc_env_runner(const struct tc_env *env, const char *key);

/*
 * The test-local data (change basic-test-local-data, D1): the files of the
 * directory ID.test/ beside ID.test.bas, each declared in the table `data`
 * of the test's own file (refused in a tests.toml) by its path relative to
 * ID.test/ (no "..", no ":", no absolute path) with a build action:
 *
 *     [data]
 *     "cmap.spec" = "mktestdata"      # the short form: the action
 *     [data."child.bas"]              # a table per file: one option a line
 *     action = "basic-compile"
 *     compiler = "same"               # same | port | orig195 | orig210
 *     libs = "same"                   # same | cd | clib
 *     runtime-errors = "same"         # same | on | off
 *
 * basic-compile takes the three options (same: the cell's value), copy and
 * mktestdata none.  A file named in both forms, an unknown action or
 * option, an option the action does not take and a value that is none of
 * the option's are errors naming file, line and key.  A fixed option that
 * needs original material (a compiler other than port, libs = "cd") is
 * refused in a test with a cell of the side port (tc_matrix_build).
 */

/** @brief The build actions of test-local data. */
enum tc_daction {
    TC_DA_BASIC_COMPILE,        /* NAME.bas compiled in the emulator: NAME */
    TC_DA_COPY,                 /* the file as it is */
    TC_DA_MKTESTDATA            /* NAME.spec made into NAME by tools/mktestdata */
};

/** @brief The options of basic-compile, in this order. */
enum tc_dopt {
    TC_DO_COMPILER,
    TC_DO_LIBS,
    TC_DO_RUNERR,
    TC_DO_N
};

/** @brief A file of ID.test/. */
struct tc_data {
    char *file;                 /* the path relative to ID.test/ */
    int action;                 /* enum tc_daction */
    char *opt[TC_DO_N];         /* basic-compile's options ("same" if not
                                   set), NULL for the other actions */
    unsigned optline[TC_DO_N];  /* the line that sets an option, 0: the default */
    struct tc_origin at;        /* the entry of the short form, or the
                                   table header */
};

/** @brief The name of an action ("basic-compile"), of an option ("compiler"). */
const char *tc_daction_name(int action);
const char *tc_dopt_name(int opt);

/**
 * @brief The file name a data entry gives: basic-compile and mktestdata
 *        without their suffix (.bas, .spec), copy the file as declared.
 * @return malloc'd, NULL = out of memory
 */
char *tc_data_result(const struct tc_data *d);

/** @brief The resolved configuration of one test. */
struct tc_config {
    char *source;               /* the test source as given */
    char *id;                   /* ID of ID.test.bas */
    char *dir;                  /* its directory, absolute */
    char **files;               /* the files read, nearest first (the test's
                                   own one first if it exists) */
    size_t nfiles;
    int has_own;                /* files[0] is the test's ID.test.toml */
    struct tc_setting *set;     /* the effective keys of the top table */
    size_t nset;
    struct tc_tabkey *matrix;   /* the effective axes */
    size_t nmatrix;
    struct tc_tabkey *asserts;  /* the effective assertions, one per
                                   selector and query */
    size_t nasserts;
    struct tc_tabkey *deviations;   /* the test's own file only */
    size_t ndeviations;
    struct tc_tabkey *ends;     /* the alternative ends: per selector its
                                   reason (a string) and the alternatives
                                   (lists); the test's own file only */
    size_t nends;
    struct tc_data *data;       /* the files of ID.test/, in file order; the
                                   test's own file only */
    size_t ndata;
};

/**
 * @brief Read an environment file.
 * @return 0 = ok, -1 = error (err: "FILE:LINE: TEXT" or "FILE: TEXT")
 */
int tc_env_load(const char *file, struct tc_env *env, char *err, size_t errlen);

/** @brief An environment from memory (tests): text in the format above. */
int tc_env_parse(const char *file, const char *text, struct tc_env *env, char *err, size_t errlen);

/** @brief Free an environment (zeroed). */
void tc_env_free(struct tc_env *env);

/**
 * @brief Resolve the configuration of a test source DIR/ID.test.bas.
 * @param source  the path of the source (it need not exist; its name must
 *                end in ".test.bas")
 * @param env     the environment (placeholders, top); NULL = none
 * @param conf    the result (zeroed by the caller; tc_free after use, also
 *                after an error)
 * @return 0 = ok, -1 = error (err names file, line and key where there are
 *         such: "FILE:LINE: TEXT"; a missing root names the test)
 */
int tc_resolve(const char *source, const struct tc_env *env, struct tc_config *conf,
               char *err, size_t errlen);

/** @brief Free a configuration (zeroed). */
void tc_free(struct tc_config *conf);

/** @brief The effective setting of a key, or NULL if no file sets it. */
const struct tc_setting *tc_get(const struct tc_config *conf, const char *key);

/**
 * @brief Replace the placeholders @NAME@ (NAME: [A-Z][A-Z0-9_]*) of text
 *        from env.
 * @param out  the result (malloc'd)
 * @return 0 = ok, -1 = an unknown placeholder (err: "@NAME@"), -2 = out of
 *         memory
 */
int tc_placeholders(const struct tc_env *env, const char *text, char **out,
                    char *err, size_t errlen);

/* ------------------------------------------------------------ the matrix
 *
 * The axes of a test are the keys of its effective table `matrix`: each a
 * run setting (not a list setting) with a non-empty list of values, given
 * as strings (an integer setting's values as decimal strings).  A key that
 * a file sets in the top table and a nearer one makes an axis is an axis,
 * and the other way round a setting; both in the same file is an error.
 * The cells are the product of the values of the axes, in the order of
 * tc_keys(), minus the cells an `exclude` selector matches.  A cell is
 * named by its axis values, "," between them ("orig195,cd,on"); the only cell
 * of a test without axes is "default".  A cell needs original material
 * (the side "originals") if its compiler is not port or its libraries are
 * the CD's.
 *
 * A cell selector is TERM[,TERM...] without blanks, a TERM KEY=VALUE: KEY
 * a run setting that is no list setting (nor isolate), VALUE one of its
 * values; in an assertion's selector also case=NAME.  A cell matches a
 * selector if its
 * value of every KEY (its axis value, else the effective setting, else the
 * key's default) is VALUE; case=NAME matches the case NAME only.  The more
 * terms, the more specific.
 */

/** @brief A term of a cell selector. */
struct tc_term {
    const struct tc_keydef *def;    /* NULL: case=NAME */
    char *value;
};

/** @brief A parsed cell selector. */
struct tc_selector {
    struct tc_term *t;
    size_t n;
};

/**
 * @brief Parse a cell selector.
 * @param allow_case  1: case=NAME allowed (assertions), 0: not (exclude)
 * @param msg         the reason of a refusal, without file and line
 * @return 0 = ok, -1 = refused (msg), -2 = out of memory
 */
int tc_selector_parse(const char *text, int allow_case, struct tc_selector *sel,
                      char *msg, size_t msglen);

/** @brief Free a selector (zeroed). */
void tc_selector_free(struct tc_selector *sel);

/** @brief An axis. */
struct tc_axis {
    const struct tc_keydef *def;
    char **values;              /* point into the configuration */
    size_t nvalues;
    struct tc_origin at;
};

/** @brief A cell. */
struct tc_cell {
    char *name;
    const char **values;        /* one per axis */
    int originals;              /* needs original material */
};

/** @brief The axes and cells of a test. */
struct tc_matrix {
    struct tc_axis *axes;
    size_t naxes;
    struct tc_cell *cells;
    size_t ncells;
};

/**
 * @brief The axes and cells of a resolved configuration.
 * @return 0 = ok, -1 = error (err: an exclusion naming no run setting, no
 *         cell left, with file and line)
 */
int tc_matrix_build(const struct tc_config *conf, struct tc_matrix *m, char *err, size_t errlen);

/** @brief Free a matrix (zeroed). */
void tc_matrix_free(struct tc_matrix *m);

/**
 * @brief The value of a key in a cell: its axis value, else the effective
 *        setting (an integer in decimal), else the key's default, else "".
 * @param buf  room for an integer's text (24 bytes)
 */
const char *tc_cell_value(const struct tc_config *conf, const struct tc_matrix *m, size_t cell,
                          const struct tc_keydef *def, char buf[24]);

/**
 * @brief Whether cell (of case case_name, NULL: no case) matches sel.
 */
int tc_selector_matches(const struct tc_selector *sel, const struct tc_config *conf,
                        const struct tc_matrix *m, size_t cell, const char *case_name);

/**
 * @brief Whether cell has an emulator session of its own: the run setting
 *        `isolate` is "on", or one of its cell selectors matches the cell
 *        (a string "on"/"off" or a non-empty list of selectors in the
 *        grammar of `exclude`; no axis, and no term of a selector).
 */
int tc_cell_isolated(const struct tc_config *conf, const struct tc_matrix *m, size_t cell);

/* ------------------------------------------------------------ assertions
 *
 * An assertion is a key QUERY = "PREDICATE" of the table `asserts` (all
 * cells) or `asserts."SELECTOR"` (the cells the selector matches), or of
 * the table `deviations."SELECTOR"` of a test's own file (the deviations
 * the original's faulty part causes in the cells the selector matches).
 * The grammar is in tests/README.md, "Assertions"; tc_query_parse and
 * tc_pred_parse implement it.
 *
 * Replacement per query (tc_assertions_for): for one query of a cell and
 * case only one assertion applies.  A deviation (when the cell is judged
 * with the deviations) beats every assertion; among deviations the most
 * specific selector wins.  Among assertions the nearest file wins, and in
 * it the most specific selector; then the default assertions
 * (tc_defaults).  Two different assertions of the same rank for the same
 * query are an error naming both.
 */

/** @brief The queries. */
enum tc_qkind {
    TC_Q_RC,                    /* rc: the exit code of the program */
    TC_Q_REQUESTER,             /* requester: the requesters shown, but the
                                   runtime errors the case announced */
    TC_Q_COMPILE,               /* compile: ok, error LINE MESSAGE, none */
    TC_Q_COMPILE_LOG,           /* compile.log: the compiler's log */
    TC_Q_OUTPUT,                /* output: the printed output */
    TC_Q_SERIAL,                /* serial: the bytes sent over the serial port */
    TC_Q_INJECT,                /* inject: the report of inject */
    TC_Q_SESSION,               /* session: done, timeout, halt */
    TC_Q_END_CASE,              /* end.case: the end entry of the case */
    TC_Q_END_RUN,               /* end.run: the end entry of the run */
    TC_Q_CHECK,                 /* check.NS.NAME[.expected|.actual] */
    TC_Q_DATA                   /* data.FILE: the compile of a data source of
                                   ID.test/ (ok, error LINE MESSAGE, none) */
};

/** @brief A parsed query. */
struct tc_query {
    int kind;                   /* enum tc_qkind */
    char *ns;                   /* TC_Q_CHECK: the namespace */
    char *rest;                 /* TC_Q_CHECK: NAME, NAME.expected or
                                   NAME.actual (the judge splits it against
                                   the names of the checks); TC_Q_DATA: FILE */
};

/** @brief The operators of a predicate. */
enum tc_pop {
    TC_P_EQ, TC_P_NE, TC_P_LT, TC_P_LE, TC_P_GT, TC_P_GE,
    TC_P_CONTAINS, TC_P_NOT_CONTAINS, TC_P_MATCHES,
    TC_P_EXISTS, TC_P_NOT_EXISTS,
    TC_P_COUNT                  /* count CMP N: cmp one of TC_P_EQ..TC_P_GE */
};

/** @brief The kind of a predicate's value. */
enum tc_pvtype {
    TC_PV_NONE,                 /* exists, not exists */
    TC_PV_INT,                  /* -12, 16, $FFFFFFFF (32 bits, signed) */
    TC_PV_STR,                  /* "text" */
    TC_PV_WORD                  /* ok, fail, not-run, done, timeout, halt, none */
};

/** @brief A parsed predicate. */
struct tc_pred {
    int op;                     /* enum tc_pop */
    int cmp;                    /* TC_P_COUNT: the comparison */
    int vtype;                  /* enum tc_pvtype */
    long long num;              /* TC_PV_INT */
    char *str;                  /* TC_PV_STR, TC_PV_WORD */
};

/**
 * @brief Parse a query.
 * @return 0 = ok, -1 = refused (msg), -2 = out of memory
 */
int tc_query_parse(const char *text, struct tc_query *q, char *msg, size_t msglen);
void tc_query_free(struct tc_query *q);

/**
 * @brief Parse a predicate for a query of kind qkind (the operators and
 *        values the query allows).
 * @return 0 = ok, -1 = refused (msg), -2 = out of memory
 */
int tc_pred_parse(const char *text, int qkind, struct tc_pred *p, char *msg, size_t msglen);
void tc_pred_free(struct tc_pred *p);

/** @brief The name of an operator ("==", "contains", "count", ...). */
const char *tc_pop_name(int op);

/** @brief One applying assertion. */
struct tc_assertion {
    const char *query;          /* the query's text */
    const char *pred;           /* the predicate's text */
    struct tc_query q;
    struct tc_pred p;
    const char *sel;            /* the selector, NULL: all cells */
    struct tc_origin at;        /* file "(default)" for a default */
    int deviation;              /* a declared deviation */
};

/** @brief The assertions of a cell and case (tc_assertions_free). */
struct tc_assertions {
    struct tc_assertion *a;
    size_t n;
};

/** @brief How a cell is judged. */
enum tc_judge_mode {
    TC_PLAIN,                   /* the assertions only */
    TC_DEVIATED,                /* with the deviations whose selector
                                   matches the cell */
    TC_DEVIATED_ANY             /* with the deviations as if their axis
                                   terms matched (the cells without the
                                   faulty part of an open bug) */
};

/**
 * @brief The default assertions: the queries and predicates that apply
 *        when no file names the query (rc == 0, requester count == 0,
 *        end.case exists, end.run exists); *n their number.  Every check
 *        with entries must have passed unless an assertion names it (the
 *        judge's default, not in this list).
 */
struct tc_default {
    const char *query, *pred;
};
const struct tc_default *tc_defaults(size_t *n);

/**
 * @brief The assertions that apply to cell of m for case case_name, one
 *        per query, defaults included.
 * @return 0 = ok, -1 = two different assertions of the same rank (err),
 *         -2 = out of memory
 */
int tc_assertions_for(const struct tc_config *conf, const struct tc_matrix *m, size_t cell,
                      const char *case_name, int mode, struct tc_assertions *out,
                      char *err, size_t errlen);
void tc_assertions_free(struct tc_assertions *a);

/**
 * @brief Whether cell holds the original's faulty part for case_name: a
 *        selector of the table deviations or ends matches it.
 */
int tc_cell_faulty(const struct tc_config *conf, const struct tc_matrix *m, size_t cell,
                   const char *case_name);

/* ------------------------------------------------------------ alternative ends
 *
 * A test's own file may declare, for the cells with the original's faulty
 * part, the ends the original's run takes depending on things the test
 * cannot fix (the memory behind a table): the table ends."SELECTOR" with
 * `reason` (a non-empty string, required) and under every other key an
 * alternative, a non-empty list of "QUERY PREDICATE" (the grammar of the
 * assertions, a blank between the two; the queries session, rc, requester,
 * end.case, end.run, output and check.*).  The table is a declared
 * deviation: it applies where the deviations apply (judged with
 * TC_DEVIATED where its selector matches, with TC_DEVIATED_ANY as if its
 * axis terms matched), counts for fixed-orig and extension and is refused
 * with fixed and in a tests.toml.  Where it applies, the queries its
 * alternatives name leave the other assertions and the defaults (a check
 * an alternative names is out of the default "every check passed"), and the
 * cell passes that point if every assertion of one alternative holds.
 */

/** @brief An alternative end. */
struct tc_end_alt {
    const char *name;           /* its key */
    struct tc_assertions as;    /* its assertions (deviations, at its key) */
    char **texts;               /* the queries and predicates they point to */
    size_t ntexts;
};

/** @brief The alternative ends of one selector. */
struct tc_end {
    const char *sel;
    const char *reason;
    struct tc_origin at;        /* of the reason */
    struct tc_end_alt *alt;
    size_t nalt;
};

/** @brief The alternative ends that apply to a cell (tc_ends_free). */
struct tc_ends {
    struct tc_end *e;
    size_t n;
};

/**
 * @brief Split "QUERY PREDICATE" at the first blank.
 * @return 0 = ok (*query, *pred malloc'd), -1 = no blank or no predicate,
 *         -2 = out of memory
 */
int tc_end_split(const char *text, char **query, char **pred);

/**
 * @brief The alternative ends that apply to cell of m for case_name in the
 *        judging mode (none with TC_PLAIN).
 * @return 0 = ok, -1 = a malformed alternative (err), -2 = out of memory
 */
int tc_ends_for(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const char *case_name,
                int mode, struct tc_ends *out, char *err, size_t errlen);
void tc_ends_free(struct tc_ends *e);

/** @brief Whether an alternative of e names the query q. */
int tc_ends_name(const struct tc_ends *e, const struct tc_query *q);

/** @brief Whether two parsed queries are the same query. */
int tc_query_same(const struct tc_query *a, const struct tc_query *b);

#ifdef __cplusplus
}
#endif

#endif
