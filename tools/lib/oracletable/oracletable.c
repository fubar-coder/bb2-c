/* oracletable.c - see oracletable.h */
#include "oracletable.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOTE_TAG "(TED 1.1: "

struct row {
    char *hash;                 /* 64 hex digits */
    char *key;                  /* KIND/PATH */
    char *note;                 /* behind the tab, or NULL */
};

struct table {
    char *text;
    struct row *rows;
    size_t n;
};

struct text {
    char *p;
    size_t n, cap;
};

static void text_add(struct text *t, const char *fmt, ...)
{
    va_list ap;
    int k;

    va_start(ap, fmt);
    k = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (k < 0)
        abort();
    if (t->n + (size_t)k + 1 > t->cap) {
        t->cap = (t->n + (size_t)k + 1) * 2;
        t->p = realloc(t->p, t->cap);
        if (!t->p)
            abort();
    }
    va_start(ap, fmt);
    vsnprintf(t->p + t->n, (size_t)k + 1, fmt, ap);
    va_end(ap);
    t->n += (size_t)k;
}

static int is_hex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

static int is_hash(const char *s)
{
    size_t i;

    for (i = 0; i < 64; i++)
        if (!is_hex(s[i]))
            return 0;
    return 1;
}

int oracletable_note_hash(const char *note, char hash[65])
{
    const char *p, *q;

    if (!note || note[0] != '#' || !isdigit((unsigned char)note[1]))
        return -1;
    for (p = note + 1; isdigit((unsigned char)*p); p++)
        ;
    if (*p != ':')
        return -1;
    for (q = strstr(p, NOTE_TAG); q; q = strstr(q + 1, NOTE_TAG)) {
        const char *h = q + strlen(NOTE_TAG);

        if (strlen(h) >= 65 && is_hash(h) && h[64] == ')') {
            memcpy(hash, h, 64);
            hash[64] = 0;
            return 1;
        }
    }
    return -1;
}

static int key_ok(const char *k)
{
    return (!strncmp(k, "detok/", 6) && k[6]) || (!strncmp(k, "tokall/", 7) && k[7])
           || (!strncmp(k, "save/", 5) && k[5]);
}

/* split SRC into rows; an error message (malloc'd) or NULL */
static char *parse(const char *name, const char *src, struct table *t, int notes)
{
    char *l, *next, msg[512];
    size_t cap = 0, i, k;

    memset(t, 0, sizeof *t);
    t->text = strdup(src);
    if (!t->text)
        abort();
    for (l = t->text, i = 1; *l; l = next, i++) {
        char *e = strchr(l, '\n'), *tab;
        struct row *r;

        if (!e) {
            snprintf(msg, sizeof msg, "%s: line %zu: no LF at the end", name, i);
            return strdup(msg);
        }
        *e = 0;
        next = e + 1;
        if (t->n == cap) {
            cap = cap ? 2 * cap : 256;
            t->rows = realloc(t->rows, cap * sizeof *t->rows);
            if (!t->rows)
                abort();
        }
        r = &t->rows[t->n++];
        r->hash = l;
        r->note = NULL;
        if ((tab = strchr(l, '\t')) != NULL) {
            *tab = 0;
            r->note = tab + 1;
        }
        if (strlen(l) < 67 || !is_hash(l) || l[64] != ' ' || l[65] != ' '
            || !key_ok(l + 66)) {
            snprintf(msg, sizeof msg, "%s: line %zu: not \"HASH  KIND/PATH\"", name, i);
            return strdup(msg);
        }
        l[64] = 0;
        r->key = l + 66;
        if (r->note && !notes) {
            snprintf(msg, sizeof msg, "%s: line %zu: %s: a note in the fresh run", name, i,
                     r->key);
            return strdup(msg);
        }
        if (r->note && (!*r->note || strchr(r->note, '\t'))) {
            snprintf(msg, sizeof msg, "%s: line %zu: %s: an empty note or a second tab", name,
                     i, r->key);
            return strdup(msg);
        }
    }
    for (i = 0; i < t->n; i++)
        for (k = 0; k < i; k++)
            if (!strcmp(t->rows[i].key, t->rows[k].key)) {
                snprintf(msg, sizeof msg, "%s: %s: twice", name, t->rows[i].key);
                return strdup(msg);
            }
    return NULL;
}

static const struct row *find(const struct table *t, const char *key)
{
    size_t i;

    for (i = 0; i < t->n; i++)
        if (!strcmp(t->rows[i].key, key))
            return &t->rows[i];
    return NULL;
}

int oracletable_merge(const char *fresh, const char *old, char **out, char **report)
{
    struct table f = { 0 }, o = { 0 };
    struct text rep = { 0 }, res = { 0 };
    char *err;
    size_t i, errors = 0, kept = 0;

    *out = NULL;
    text_add(&rep, "%s", "");
    if ((err = parse("NEW", fresh, &f, 0)) != NULL || (err = parse("OLD", old, &o, 1)) != NULL) {
        text_add(&rep, "%s\n", err);
        free(err);
        free(f.rows);
        free(f.text);
        free(o.rows);
        free(o.text);
        *report = rep.p;
        return 1;
    }
    /* every note row of OLD: well formed, in FRESH, naming FRESH's hash */
    for (i = 0; i < o.n; i++) {
        const struct row *r = &o.rows[i], *n;
        char h[65];

        if (!r->note)
            continue;
        if (oracletable_note_hash(r->note, h) != 1) {
            text_add(&rep, "%s: the note is not \"#NNN: TEXT (TED 1.1: HASH)\": %s\n", r->key,
                     r->note);
            errors++;
        } else if (!strcmp(h, r->hash)) {
            text_add(&rep, "%s: the note names the row's own hash (no difference)\n", r->key);
            errors++;
        } else if (!(n = find(&f, r->key))) {
            text_add(&rep, "%s: note row, but not in the fresh run\n", r->key);
            errors++;
        } else if (strcmp(n->hash, h)) {
            text_add(&rep, "%s: stale note: the original gives %s, the note names %s\n", r->key,
                     n->hash, h);
            errors++;
        }
    }
    for (i = 0; !errors && i < f.n; i++) {
        const struct row *r = find(&o, f.rows[i].key);

        if (r && r->note) {
            text_add(&res, "%s  %s\t%s\n", r->hash, r->key, r->note);
            text_add(&rep, "%s: note kept (%.*s)\n", r->key, (int)strcspn(r->note, ":"),
                     r->note);
            kept++;
        } else
            text_add(&res, "%s  %s\n", f.rows[i].hash, f.rows[i].key);
    }
    if (!errors)
        text_add(&rep, "%zu rows, %zu note rows kept\n", f.n, kept);
    free(f.rows);
    free(f.text);
    free(o.rows);
    free(o.text);
    *report = rep.p;
    if (errors) {
        free(res.p);
        return 1;
    }
    if (!res.p)
        text_add(&res, "%s", "");
    *out = res.p;
    return 0;
}
