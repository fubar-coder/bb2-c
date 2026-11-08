/* cdtable.c - see cdtable.h */
#include "cdtable.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NFIELDS 8
#define F_PATH  0
#define F_SHA   5
#define F_NOTE  7

struct row {
    char *f[NFIELDS];           /* the fields, cut at the tabs */
};

struct table {
    char *text;
    char *header;
    struct row *rows;
    size_t n;
};

struct text {
    char *p;
    size_t n, cap;
};

static void text_raw(struct text *t, const char *s, size_t k)
{
    if (t->n + k + 1 > t->cap) {
        t->cap = (t->n + k + 1) * 2;
        t->p = realloc(t->p, t->cap);
        if (!t->p)
            abort();
    }
    memcpy(t->p + t->n, s, k);
    t->n += k;
    t->p[t->n] = 0;
}

static void text_add(struct text *t, const char *fmt, ...)
{
    va_list ap;
    char buf[2048];

    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    text_raw(t, buf, strlen(buf));
}

static int is_hex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

int cdtable_note_hash(const char *note, char hash[65])
{
    const char *p, *q;
    size_t i;

    if (!note || note[0] != '#' || !isdigit((unsigned char)note[1]))
        return 0;
    for (p = note + 1; isdigit((unsigned char)*p); p++)
        ;
    if (*p != ':')
        return 0;
    for (q = strstr(p, "(2.10: "); q; q = strstr(q + 1, "(2.10: ")) {
        const char *h = q + 7;

        for (i = 0; i < 64 && is_hex(h[i]); i++)
            ;
        if (i == 64 && h[64] == ')') {
            memcpy(hash, h, 64);
            hash[64] = 0;
            return 1;
        }
    }
    return -1;
}

/* split SRC into rows (the first line is the header); an error message
 * (malloc'd) or NULL */
static char *parse(const char *name, const char *src, struct table *t)
{
    char *l, *next, msg[512];
    size_t cap = 0, i, k;

    memset(t, 0, sizeof *t);
    t->text = strdup(src);
    if (!t->text)
        abort();
    for (l = t->text, i = 0; *l; l = next, i++) {
        char *e = strchr(l, '\n');
        struct row *r;
        char *p = l;

        if (e) {
            *e = 0;
            next = e + 1;
        } else {
            next = l + strlen(l);
        }
        if (i == 0) {
            t->header = l;
            continue;
        }
        if (t->n == cap) {
            cap = cap ? 2 * cap : 64;
            t->rows = realloc(t->rows, cap * sizeof *t->rows);
            if (!t->rows)
                abort();
        }
        r = &t->rows[t->n++];
        for (k = 0; k < NFIELDS - 1; k++) {
            r->f[k] = p;
            p = strchr(p, '\t');
            if (!p)
                break;
            *p++ = 0;
        }
        if (k == NFIELDS - 1)
            r->f[k] = p;
        if (k != NFIELDS - 1 || strchr(r->f[F_NOTE], '\t')) {
            snprintf(msg, sizeof msg, "%s: line %zu: not %d tab separated fields", name, i + 1,
                     NFIELDS);
            return strdup(msg);
        }
    }
    if (!t->header) {
        snprintf(msg, sizeof msg, "%s: empty", name);
        return strdup(msg);
    }
    for (i = 0; i < t->n; i++)
        for (k = 0; k < i; k++)
            if (!strcmp(t->rows[i].f[F_PATH], t->rows[k].f[F_PATH])) {
                snprintf(msg, sizeof msg, "%s: %s: twice", name, t->rows[i].f[F_PATH]);
                return strdup(msg);
            }
    return NULL;
}

static const struct row *find(const struct table *t, const char *path)
{
    size_t i;

    for (i = 0; i < t->n; i++)
        if (!strcmp(t->rows[i].f[F_PATH], path))
            return &t->rows[i];
    return NULL;
}

/* the fields before the note equal */
static int same_result(const struct row *a, const struct row *b)
{
    int i;

    for (i = 0; i < F_NOTE; i++)
        if (strcmp(a->f[i], b->f[i]))
            return 0;
    return 1;
}

static void put_row(struct text *o, const struct row *r, const char *note)
{
    int i;

    for (i = 0; i < F_NOTE; i++)
        text_add(o, "%s\t", r->f[i]);
    text_add(o, "%s\n", note);
}

static void free_table(struct table *t)
{
    free(t->rows);
    free(t->text);
}

static int fail(struct table *a, struct table *b, struct text *rep, char *err, char **report)
{
    text_add(rep, "%s\n", err);
    free(err);
    free_table(a);
    free_table(b);
    *report = rep->p;
    return 1;
}

int cdtable_merge(enum cdtable_mode mode, const char *fresh, const char *old,
                  char **out, char **report)
{
    struct table nt, ot;
    struct text o = { 0 }, rep = { 0 };
    char *err;
    size_t i;
    int bad = 0;

    *out = NULL;
    *report = NULL;
    text_raw(&rep, "", 0);
    memset(&ot, 0, sizeof ot);
    if ((err = parse("new table", fresh, &nt)))
        return fail(&nt, &ot, &rep, err, report);
    if ((err = parse("old table", old, &ot)))
        return fail(&nt, &ot, &rep, err, report);
    if (strcmp(nt.header, ot.header)) {
        text_add(&rep, "the headers differ\n");
        bad = 1;
    }
    text_add(&o, "%s\n", nt.header);

    for (i = 0; i < nt.n && !bad; i++) {
        const struct row *n = &nt.rows[i], *r = find(&ot, n->f[F_PATH]);
        char hash[65];
        int kind = r ? cdtable_note_hash(r->f[F_NOTE], hash) : 0;

        if (kind < 0) {
            text_add(&rep, "%s: note row without the CD's hash: %s\n", n->f[F_PATH],
                     r->f[F_NOTE]);
            bad = 1;
        } else if (kind == 0) {
            put_row(&o, n, n->f[F_NOTE]);
            if (!r)
                text_add(&rep, "%s: new row\n", n->f[F_PATH]);
            else if (mode == CDTABLE_UPDATE && !same_result(n, r))
                text_add(&rep, "%s: changed without a note (a fix needs \"#NNN: ... (2.10: %s)\")\n",
                         n->f[F_PATH], r->f[F_SHA]);
        } else if (mode == CDTABLE_REFERENCE) {
            if (strcmp(n->f[F_SHA], hash)) {
                text_add(&rep, "%s: the note names %s, the original's hash is %s (stale note)\n",
                         n->f[F_PATH], hash, n->f[F_SHA]);
                bad = 1;
            } else {
                put_row(&o, r, r->f[F_NOTE]);
                text_add(&rep, "%s: note kept\n", n->f[F_PATH]);
            }
        } else if (!strcmp(n->f[F_SHA], hash)) {
            put_row(&o, n, n->f[F_NOTE]);
            text_add(&rep, "%s: note dropped (the port's executable is the CD's again)\n",
                     n->f[F_PATH]);
        } else {
            put_row(&o, n, r->f[F_NOTE]);
            text_add(&rep, "%s: note kept\n", n->f[F_PATH]);
        }
    }
    for (i = 0; i < ot.n && !bad; i++) {
        char hash[65];

        if (!find(&nt, ot.rows[i].f[F_PATH])) {
            if (cdtable_note_hash(ot.rows[i].f[F_NOTE], hash)) {
                text_add(&rep, "%s: note row not in the new table\n", ot.rows[i].f[F_PATH]);
                bad = 1;
            } else {
                text_add(&rep, "%s: row removed\n", ot.rows[i].f[F_PATH]);
            }
        }
    }
    free_table(&nt);
    free_table(&ot);
    *report = rep.p;
    if (bad) {
        free(o.p);
        return 1;
    }
    *out = o.p;
    return 0;
}
