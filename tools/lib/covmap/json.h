/*
 * json.h - a small JSON reader (whole documents into a tree): the gcov
 * JSON of the host coverage and ctest's --show-only=json-v1.
 */
#ifndef COV_JSON_H
#define COV_JSON_H

#include <stddef.h>

enum json_type { JSON_NULL, JSON_FALSE, JSON_TRUE, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT };

struct json {
    enum json_type type;
    double number;              /* JSON_NUMBER */
    char *string;               /* JSON_STRING: the value, UTF-8, 0-terminated */
    size_t n;                   /* JSON_ARRAY, JSON_OBJECT: number of items */
    struct json *items;         /* the values */
    char **keys;                /* JSON_OBJECT: the keys of items */
};

/*
 * Parse one JSON value from text[*pos..len) (whitespace around it is
 * skipped; *pos is left after it).  Returns the tree, or NULL with a
 * message in err (also at the end of the text: then err is empty).
 */
struct json *json_parse(const char *text, size_t len, size_t *pos, char *err, size_t errsz);
void json_free(struct json *j);

/* the value of key in an object, NULL if j is no object or has no such key */
const struct json *json_get(const struct json *j, const char *key);
/* the string of key, NULL if missing or not a string */
const char *json_get_string(const struct json *j, const char *key);
/* the number of key, def if missing or not a number */
double json_get_number(const struct json *j, const char *key, double def);

#endif
