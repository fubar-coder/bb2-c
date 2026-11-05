/*
 * json.c - a small JSON reader (json.h).
 */
#include "json.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DEPTH 256

struct parser {
    const char *s;
    size_t len, pos;
    char *err;
    size_t errsz;
    int failed;
};

static void fail(struct parser *p, const char *fmt, ...)
{
    va_list ap;
    int n;

    if (p->failed)
        return;
    p->failed = 1;
    n = snprintf(p->err, p->errsz, "offset %zu: ", p->pos);
    if (n >= 0 && (size_t)n < p->errsz) {
        va_start(ap, fmt);
        vsnprintf(p->err + n, p->errsz - (size_t)n, fmt, ap);
        va_end(ap);
    }
}

static void skip_ws(struct parser *p)
{
    while (p->pos < p->len && (p->s[p->pos] == ' ' || p->s[p->pos] == '\t'
                               || p->s[p->pos] == '\n' || p->s[p->pos] == '\r'))
        p->pos++;
}

static void free_value(struct json *j)
{
    size_t i;

    if (j->type == JSON_STRING)
        free(j->string);
    if (j->type == JSON_ARRAY || j->type == JSON_OBJECT) {
        for (i = 0; i < j->n; i++) {
            free_value(&j->items[i]);
            if (j->keys)
                free(j->keys[i]);
        }
        free(j->items);
        free(j->keys);
    }
}

void json_free(struct json *j)
{
    if (j) {
        free_value(j);
        free(j);
    }
}

static int hex4(struct parser *p, unsigned *v)
{
    unsigned i, c;

    *v = 0;
    if (p->len - p->pos < 4) {
        fail(p, "short \\u escape");
        return 0;
    }
    for (i = 0; i < 4; i++) {
        c = (unsigned char)p->s[p->pos++];
        *v <<= 4;
        if (c >= '0' && c <= '9')
            *v |= c - '0';
        else if (c >= 'a' && c <= 'f')
            *v |= c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            *v |= c - 'A' + 10;
        else {
            fail(p, "bad \\u escape");
            return 0;
        }
    }
    return 1;
}

static void put_utf8(char *out, size_t *n, unsigned cp)
{
    if (cp < 0x80)
        out[(*n)++] = (char)cp;
    else if (cp < 0x800) {
        out[(*n)++] = (char)(0xC0 | cp >> 6);
        out[(*n)++] = (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out[(*n)++] = (char)(0xE0 | cp >> 12);
        out[(*n)++] = (char)(0x80 | (cp >> 6 & 0x3F));
        out[(*n)++] = (char)(0x80 | (cp & 0x3F));
    } else {
        out[(*n)++] = (char)(0xF0 | cp >> 18);
        out[(*n)++] = (char)(0x80 | (cp >> 12 & 0x3F));
        out[(*n)++] = (char)(0x80 | (cp >> 6 & 0x3F));
        out[(*n)++] = (char)(0x80 | (cp & 0x3F));
    }
}

/* a string; p->pos at the opening quote */
static char *parse_string(struct parser *p)
{
    size_t start = ++p->pos, end, n = 0;
    char *out;
    unsigned cp, lo;

    /* the end, for the size (escapes only shrink the text: \uXXXX is 6
       bytes for at most 3, a surrogate pair 12 for 4) */
    for (end = start; end < p->len && p->s[end] != '"'; end++)
        if (p->s[end] == '\\')
            end++;
    if (end >= p->len) {
        fail(p, "unterminated string");
        return NULL;
    }
    if (!(out = malloc(end - start + 1))) {
        fail(p, "out of memory");
        return NULL;
    }
    while (p->s[p->pos] != '"') {
        char c = p->s[p->pos++];

        if ((unsigned char)c < 0x20) {
            fail(p, "control character in string");
            free(out);
            return NULL;
        }
        if (c != '\\') {
            out[n++] = c;
            continue;
        }
        c = p->s[p->pos++];
        switch (c) {
        case '"': case '\\': case '/':
            out[n++] = c;
            break;
        case 'b': out[n++] = '\b'; break;
        case 'f': out[n++] = '\f'; break;
        case 'n': out[n++] = '\n'; break;
        case 'r': out[n++] = '\r'; break;
        case 't': out[n++] = '\t'; break;
        case 'u':
            if (!hex4(p, &cp)) {
                free(out);
                return NULL;
            }
            if (cp >= 0xD800 && cp < 0xDC00 && p->len - p->pos >= 6 && p->s[p->pos] == '\\'
                && p->s[p->pos + 1] == 'u') {
                p->pos += 2;
                if (!hex4(p, &lo)) {
                    free(out);
                    return NULL;
                }
                if (lo >= 0xDC00 && lo < 0xE000)
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                else
                    p->pos -= 6;    /* not a pair: the second escape on its own */
            }
            put_utf8(out, &n, cp);
            break;
        default:
            fail(p, "bad escape \\%c", c);
            free(out);
            return NULL;
        }
    }
    p->pos++;
    out[n] = 0;
    return out;
}

static int parse_value(struct parser *p, struct json *j, int depth);

static int grow(struct parser *p, struct json *j, size_t *cap, int with_keys)
{
    void *t;

    if (j->n < *cap)
        return 1;
    *cap = *cap ? 2 * *cap : 8;
    if (!(t = realloc(j->items, *cap * sizeof *j->items))) {
        fail(p, "out of memory");
        return 0;
    }
    j->items = t;
    if (with_keys) {
        if (!(t = realloc(j->keys, *cap * sizeof *j->keys))) {
            fail(p, "out of memory");
            return 0;
        }
        j->keys = t;
    }
    return 1;
}

static int parse_container(struct parser *p, struct json *j, int depth)
{
    int object = p->s[p->pos] == '{';
    char close = object ? '}' : ']';
    size_t cap = 0;

    j->type = object ? JSON_OBJECT : JSON_ARRAY;
    p->pos++;
    skip_ws(p);
    if (p->pos < p->len && p->s[p->pos] == close) {
        p->pos++;
        return 1;
    }
    for (;;) {
        char *key = NULL;

        skip_ws(p);
        if (object) {
            if (p->pos >= p->len || p->s[p->pos] != '"') {
                fail(p, "object key expected");
                return 0;
            }
            if (!(key = parse_string(p)))
                return 0;
            skip_ws(p);
            if (p->pos >= p->len || p->s[p->pos] != ':') {
                free(key);
                fail(p, "':' expected");
                return 0;
            }
            p->pos++;
        }
        if (!grow(p, j, &cap, object)) {
            free(key);
            return 0;
        }
        memset(&j->items[j->n], 0, sizeof j->items[j->n]);
        if (object)
            j->keys[j->n] = key;
        j->n++;
        if (!parse_value(p, &j->items[j->n - 1], depth + 1))
            return 0;
        skip_ws(p);
        if (p->pos < p->len && p->s[p->pos] == ',') {
            p->pos++;
            continue;
        }
        if (p->pos < p->len && p->s[p->pos] == close) {
            p->pos++;
            return 1;
        }
        fail(p, "',' or '%c' expected", close);
        return 0;
    }
}

static int parse_value(struct parser *p, struct json *j, int depth)
{
    const char *s;
    char *end;

    if (depth > MAX_DEPTH) {
        fail(p, "nested too deeply");
        return 0;
    }
    skip_ws(p);
    if (p->pos >= p->len) {
        fail(p, "value expected at the end");
        return 0;
    }
    s = p->s + p->pos;
    switch (*s) {
    case '{': case '[':
        return parse_container(p, j, depth);
    case '"':
        j->type = JSON_STRING;
        return (j->string = parse_string(p)) != NULL;
    case 't':
        if (p->len - p->pos >= 4 && !memcmp(s, "true", 4)) {
            j->type = JSON_TRUE;
            p->pos += 4;
            return 1;
        }
        break;
    case 'f':
        if (p->len - p->pos >= 5 && !memcmp(s, "false", 5)) {
            j->type = JSON_FALSE;
            p->pos += 5;
            return 1;
        }
        break;
    case 'n':
        if (p->len - p->pos >= 4 && !memcmp(s, "null", 4)) {
            j->type = JSON_NULL;
            p->pos += 4;
            return 1;
        }
        break;
    default:
        if (*s == '-' || (*s >= '0' && *s <= '9')) {
            j->type = JSON_NUMBER;
            j->number = strtod(s, &end);
            if (end > s && (size_t)(end - p->s) <= p->len) {
                p->pos = (size_t)(end - p->s);
                return 1;
            }
        }
        break;
    }
    fail(p, "unexpected character '%c'", *s);
    return 0;
}

struct json *json_parse(const char *text, size_t len, size_t *pos, char *err, size_t errsz)
{
    struct parser p = { text, len, *pos, err, errsz, 0 };
    struct json *j;

    if (errsz)
        err[0] = 0;
    skip_ws(&p);
    if (p.pos >= len) {
        *pos = p.pos;
        return NULL;
    }
    if (!(j = calloc(1, sizeof *j))) {
        fail(&p, "out of memory");
        return NULL;
    }
    if (!parse_value(&p, j, 0)) {
        json_free(j);
        return NULL;
    }
    skip_ws(&p);
    *pos = p.pos;
    return j;
}

const struct json *json_get(const struct json *j, const char *key)
{
    size_t i;

    if (!j || j->type != JSON_OBJECT)
        return NULL;
    for (i = 0; i < j->n; i++)
        if (!strcmp(j->keys[i], key))
            return &j->items[i];
    return NULL;
}

const char *json_get_string(const struct json *j, const char *key)
{
    const struct json *v = json_get(j, key);

    return v && v->type == JSON_STRING ? v->string : NULL;
}

double json_get_number(const struct json *j, const char *key, double def)
{
    const struct json *v = json_get(j, key);

    return v && v->type == JSON_NUMBER ? v->number : def;
}
