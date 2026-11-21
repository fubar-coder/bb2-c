/*
 * testtoml.h - the strict subset of TOML of the run configuration of the
 * Blitz tests (`tests.toml`, `ID.test.toml`; change basic-test-runner,
 * design D8, tests/README.md "The run configuration").
 *
 * Every file of the subset is valid TOML; the parser refuses everything
 * else of TOML, naming file and line.  A line is one of:
 *
 *   (blank)                     also only blanks and tabs
 *   # comment
 *   [TABLE.PATH]                a table header; the parts bare keys or
 *                               quoted keys, "." between them
 *   KEY = VALUE                 one key per line; KEY a bare key
 *                               ([A-Za-z0-9_-]+) or a quoted key ("..."
 *                               or '...'), no dotted keys
 *
 * each optionally followed by blanks and a "# comment".  VALUE is
 *
 *   "basic string"              escapes \" \\ \t \n \r only
 *   'literal string'            no escapes
 *   123, -5, +7                 a decimal integer (no "_", no leading 0)
 *   true, false
 *   ["a", 'b', ]                a list of strings on one line (a comma
 *                               after the last one allowed), [] empty
 *
 * Refused: multi-line strings, floats, dates, inline tables, arrays of
 * tables, lists of other values or over several lines, a key defined twice
 * in a table, a table header given twice, a key that is also a table, any
 * other escape, a control character in a string.  Lines end with LF; a CR
 * before it is ignored.
 */
#ifndef TESTTOML_H
#define TESTTOML_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The type of a value. */
enum tt_type {
    TT_STRING,
    TT_INTEGER,
    TT_BOOLEAN,
    TT_LIST                     /* a list of strings */
};

/** @brief A value; only the members of its type count. */
struct tt_value {
    int type;                   /* enum tt_type */
    char *str;                  /* TT_STRING */
    long long integer;          /* TT_INTEGER */
    int boolean;                /* TT_BOOLEAN: 0 or 1 */
    char **list;                /* TT_LIST: nlist strings */
    size_t nlist;
};

/** @brief A key with its value, in the table of the header above it. */
struct tt_entry {
    char **table;               /* the parts of the table's name; none: the top */
    size_t ntable;
    char *key;
    struct tt_value value;
    unsigned line;              /* the line of the key */
};

/** @brief A table header. */
struct tt_table {
    char **path;
    size_t npath;
    unsigned line;
};

/** @brief A parsed file: its keys and table headers in file order. */
struct tt_doc {
    char *file;                 /* the name given to tt_parse */
    struct tt_entry *e;
    size_t n, cap;
    struct tt_table *tables;
    size_t ntables, captables;
};

/**
 * @brief Parse the text of a file.
 * @param file    the file's name, for the messages and doc->file
 * @param text    its text, text[0..len)
 * @param len     its length (a 0 byte in it is refused)
 * @param doc     the result (zeroed by the caller, tt_free after use, also
 *                after an error)
 * @param err     the message of an error: "FILE:LINE: TEXT"
 * @param errlen  the size of err (> 0)
 * @return 0 = ok, -1 = refused (err), -2 = out of memory
 */
int tt_parse(const char *file, const char *text, size_t len, struct tt_doc *doc,
             char *err, size_t errlen);

/** @brief Free what tt_parse allocated; doc is zeroed. */
void tt_free(struct tt_doc *doc);

/** @brief Free a value's memory (the value is zeroed). */
void tt_value_free(struct tt_value *v);

/** @brief A deep copy of v into out; 0 = ok, -1 = out of memory. */
int tt_value_copy(struct tt_value *out, const struct tt_value *v);

/** @brief The name of a type, for messages ("string", "integer", ...). */
const char *tt_type_name(int type);

#ifdef __cplusplus
}
#endif

#endif
