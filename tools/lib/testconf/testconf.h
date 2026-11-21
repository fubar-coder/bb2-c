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
 * runtime error checking, time limits, files, wrappers of the run, ...),
 * the keys of the registration (labels, cost, fixtures), `root` (only in
 * a `tests.toml`), the keys of a single test (`fixed`, `fixed-orig`, the
 * table `deviations`; only in an `ID.test.toml`), `exclude` and the
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
    TC_V_LIST                   /* a list of strings */
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
 */
struct tc_env {
    char *file;
    char *top;                  /* NULL: the search goes up to "/" */
    struct tc_placeholder *ph;
    size_t nph;
};

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
 * named by its axis values, "," between them ("orig,cd,on"); the only cell
 * of a test without axes is "default".  A cell needs original material
 * (the side "originals") if its compiler is not port or its libraries are
 * the CD's.
 *
 * A cell selector is TERM[,TERM...] without blanks, a TERM KEY=VALUE: KEY
 * a run setting that is no list setting, VALUE one of its values; in an
 * assertion's selector also case=NAME.  A cell matches a selector if its
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

#ifdef __cplusplus
}
#endif

#endif
