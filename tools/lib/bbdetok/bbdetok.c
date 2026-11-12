/*
 * bbdetok.c - a tokenised source as plain text, see bbdetok.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bbdetok.h"

int bbdetok_init(struct bbdetok *d, const struct blitztokens *list)
{
    size_t i;

    d->name = calloc(32768, sizeof *d->name);
    if (!d->name)
        return -1;
    for (i = 0; i < list->n; i++) {
        unsigned w = list->t[i].num & 0x7FFF;

        if (!d->name[w])
            d->name[w] = list->t[i].name;
    }
    return 0;
}

void bbdetok_free(struct bbdetok *d)
{
    free((void *)d->name);
    d->name = NULL;
}

/* the growing output */
struct text {
    char *p;
    size_t n, cap;
    int bad;
};

static void put(struct text *t, const void *s, size_t n)
{
    if (t->bad)
        return;
    if (t->cap - t->n < n) {
        size_t cap = t->cap ? t->cap : 65536;
        char *p;

        while (cap - t->n < n)
            cap *= 2;
        if (!(p = realloc(t->p, cap))) {
            t->bad = 1;
            return;
        }
        t->p = p;
        t->cap = cap;
    }
    memcpy(t->p + t->n, s, n);
    t->n += n;
}

int bbdetok_text(const struct bbdetok *d, int rules, const uint8_t *src, size_t len,
                 char **out, size_t *outlen, struct bbdetok_result *r, char *err,
                 size_t errlen)
{
    struct text t = { 0 };
    size_t i = 0, line = 1;
    int ended = 1;                      /* the last byte read ended a line */
    enum { CODE, STRING, COMMENT } in = CODE;

    memset(r, 0, sizeof *r);
    *out = NULL;
    *outlen = 0;
    if (rules != BBDETOK_AB && rules != BBDETOK_TED) {
        snprintf(err, errlen, "unknown rules %d", rules);
        return -1;
    }
    while (i < len) {
        uint8_t c = src[i];

        if (c == 0) {                   /* the end of the line */
            put(&t, "\n", 1);
            r->lines++;
            line++;
            in = CODE;
            ended = 1;
            i++;
            continue;
        }
        ended = 0;
        if (c & 0x80 && rules == BBDETOK_AB && in == COMMENT && i + 1 < len && src[i + 1]
            && d->name[(c << 8 | src[i + 1]) & 0x7FFF]) {
            const char *name = d->name[(c << 8 | src[i + 1]) & 0x7FFF];

            put(&t, name, strlen(name));        /* a line commented out */
            r->tokens++;
            r->comment++;
            i += 2;
            continue;
        }
        if (c & 0x80 && (rules == BBDETOK_TED || in == CODE)) {
            unsigned w;
            const char *name;

            if (i + 1 >= len) {
                snprintf(err, errlen, "line %zu: token byte $%02X at the end of the file",
                         line, (unsigned)c);
                free(t.p);
                return -1;
            }
            if (c == 0x90 && (src[i + 1] & 0x80)) {
                /* a literal character (docs/formats.md 3.2): its byte */
                put(&t, &src[i + 1], 1);
                r->literals++;
                i += 2;
                continue;
            }
            w = (unsigned)(c << 8 | src[i + 1]) & 0x7FFF;
            r->tokens++;
            if ((name = d->name[w]))
                put(&t, name, strlen(name));
            else {
                char u[16];
                int n = snprintf(u, sizeof u, "{%u:%u}", w >> 7, w & 0x7F);

                put(&t, u, (size_t)n);
                r->unknown++;
                if (!r->count[w]++)
                    r->kinds++;
            }
            i += 2;
            continue;
        }
        put(&t, &c, 1);
        i++;
        if (rules == BBDETOK_TED)
            continue;
        if (in == CODE && c == '"')
            in = STRING;
        else if (in == STRING && c == '"')
            in = CODE;
        else if (in == CODE && c == ';')
            in = COMMENT;
    }
    if (!ended) {                       /* a last line without its 0 */
        put(&t, "\n", 1);
        r->lines++;
        r->open = 1;
    }
    if (!t.p)
        t.p = malloc(1);                /* an empty file: an empty text */
    if (t.bad || !t.p) {
        snprintf(err, errlen, "out of memory");
        free(t.p);
        return -1;
    }
    *out = t.p;
    *outlen = t.n;
    return 0;
}
