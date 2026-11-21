/*
 * testmatrix.c - the axes, cells and cell selectors of the run
 * configuration (testconf.h, "the matrix").
 */
#include "testconf.h"
#include "testconf_int.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* a value of an axis or a selector term against the key's kind */
static int value_ok(const struct tc_keydef *def, const char *v, char *msg, size_t msglen)
{
    if (!*v)
        return tc_err(msg, msglen, "an empty value of %s", def->name);
    if (def->vkind == TC_V_ENUM && !tc_enum_ok(def, v))
        return tc_err(msg, msglen, "%s must be one of %s, not \"%s\"", def->name, def->values, v);
    if (def->vkind == TC_V_INT) {
        long long n = 0;
        const char *p = v;

        if (*p == '0' && p[1])
            return tc_err(msg, msglen, "%s must be an integer, not \"%s\"", def->name, v);
        for (; *p; p++) {
            if (*p < '0' || *p > '9' || n > 100000000000LL)
                return tc_err(msg, msglen, "%s must be an integer, not \"%s\"", def->name, v);
            n = n * 10 + (*p - '0');
        }
        if (n < def->min)
            return tc_err(msg, msglen, "%s must be at least %lld, not %s", def->name, def->min, v);
    }
    return 0;
}

/* a key that can be an axis or a selector term: a run setting, no list */
static const struct tc_keydef *run_key(const char *key, char *msg, size_t msglen)
{
    const struct tc_keydef *def = tc_key(key);

    if (!def || !(def->flags & TC_RUN)) {
        tc_err(msg, msglen, "%s is no run setting", key);
        return NULL;
    }
    if (def->vkind == TC_V_LIST) {
        tc_err(msg, msglen, "%s is a list setting", key);
        return NULL;
    }
    return def;
}

int tc_axis_check(const char *file, unsigned line, const char *key, const struct tt_value *v,
                  char *err, size_t errlen)
{
    char msg[300];

    if (!run_key(key, msg, sizeof msg))
        return tc_err(err, errlen, "%s:%u: %s and cannot be an axis", file, line, msg);
    if (v->type != TT_LIST)
        return tc_err(err, errlen, "%s:%u: the axis %s must be a list of strings, not a%s %s", file, line,
                      key, v->type == TT_INTEGER ? "n" : "", tt_type_name(v->type));
    if (v->nlist == 0)
        return tc_err(err, errlen, "%s:%u: the axis %s has no value", file, line, key);
    for (size_t i = 0; i < v->nlist; i++) {
        if (value_ok(tc_key(key), v->list[i], msg, sizeof msg))
            return tc_err(err, errlen, "%s:%u: %s", file, line, msg);
        for (size_t j = 0; j < i; j++)
            if (!strcmp(v->list[i], v->list[j]))
                return tc_err(err, errlen, "%s:%u: the axis %s has the value \"%s\" twice", file, line, key,
                              v->list[i]);
    }
    return 0;
}

/* ------------------------------------------------------------ selectors */

void tc_selector_free(struct tc_selector *sel)
{
    for (size_t i = 0; i < sel->n; i++)
        free(sel->t[i].value);
    free(sel->t);
    memset(sel, 0, sizeof *sel);
}

static int case_name_ok(const char *s)
{
    if (*s < 'a' || *s > 'z')
        return 0;
    for (; *s; s++)
        if (!((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '_'))
            return 0;
    return 1;
}

int tc_selector_parse(const char *text, int allow_case, struct tc_selector *sel, char *msg, size_t msglen)
{
    const char *p = text;

    memset(sel, 0, sizeof *sel);
    if (!*text)
        return tc_err(msg, msglen, "an empty cell selector");
    for (;;) {
        const char *e = strchr(p, ','), *eq;
        size_t len = e ? (size_t)(e - p) : strlen(p);
        char key[64], *value;
        const struct tc_keydef *def = NULL;
        struct tc_term *t;

        if (memchr(p, ' ', len) || memchr(p, '\t', len)) {
            tc_selector_free(sel);
            return tc_err(msg, msglen, "a blank in the cell selector \"%s\"", text);
        }
        eq = memchr(p, '=', len);
        if (!eq || eq == p || (size_t)(eq - p) >= sizeof key) {
            tc_selector_free(sel);
            return tc_err(msg, msglen, "the cell selector \"%s\": KEY=VALUE expected, not \"%.*s\"", text,
                          (int)len, p);
        }
        memcpy(key, p, (size_t)(eq - p));
        key[eq - p] = 0;
        if (!(value = malloc(len - (size_t)(eq - p))))
            return tc_selector_free(sel), -2;
        memcpy(value, eq + 1, len - (size_t)(eq - p) - 1);
        value[len - (size_t)(eq - p) - 1] = 0;
        if (!strcmp(key, "case")) {
            if (!allow_case) {
                free(value);
                tc_selector_free(sel);
                return tc_err(msg, msglen, "the cell selector \"%s\": case= is not allowed here", text);
            }
            if (!case_name_ok(value)) {
                tc_selector_free(sel);
                tc_err(msg, msglen, "the cell selector \"%s\": \"%s\" is no case name", text, value);
                free(value);
                return -1;
            }
        } else {
            char why[200];

            if (!(def = run_key(key, why, sizeof why)) || value_ok(def, value, why, sizeof why)) {
                free(value);
                tc_selector_free(sel);
                return tc_err(msg, msglen, "the cell selector \"%s\": %s", text, why);
            }
        }
        for (size_t i = 0; i < sel->n; i++)
            if (sel->t[i].def == def) {
                free(value);
                tc_selector_free(sel);
                return tc_err(msg, msglen, "the cell selector \"%s\" names %s twice", text, key);
            }
        if (!(t = realloc(sel->t, (sel->n + 1) * sizeof *t))) {
            free(value);
            tc_selector_free(sel);
            return -2;
        }
        sel->t = t;
        t[sel->n].def = def;
        t[sel->n].value = value;
        sel->n++;
        if (!e)
            return 0;
        p = e + 1;
    }
}

/* ------------------------------------------------------------ axes against settings */

int tc_matrix_merge(struct tc_config *c, char *err, size_t errlen)
{
    for (size_t i = 0; i < c->nmatrix;) {
        const struct tc_setting *s = tc_get(c, c->matrix[i].key);
        const struct tc_tabkey *a = &c->matrix[i];

        if (s && s->at.depth == a->at.depth)
            return tc_err(err, errlen, "%s:%u: %s is a setting (line %u) and an axis (line %u) in one file",
                          a->at.file, a->at.line, a->key, s->at.line, a->at.line);
        if (s && s->at.depth < a->at.depth) {
            /* the nearer file sets the key: no axis */
            tc_tabkey_free(&c->matrix[i]);
            memmove(&c->matrix[i], &c->matrix[i + 1], (c->nmatrix - i - 1) * sizeof c->matrix[0]);
            c->nmatrix--;
            continue;
        }
        if (s) {
            /* the nearer file makes it an axis: no setting */
            size_t k = (size_t)(s - c->set);

            tt_value_free(&c->set[k].value);
            memmove(&c->set[k], &c->set[k + 1], (c->nset - k - 1) * sizeof c->set[0]);
            c->nset--;
        }
        i++;
    }
    {
        const struct tc_setting *ex = tc_get(c, "exclude");

        for (size_t i = 0; ex && i < ex->value.nlist; i++) {
            struct tc_selector sel;
            char msg[300];
            int r = tc_selector_parse(ex->value.list[i], 0, &sel, msg, sizeof msg);

            if (r == -2)
                return tc_err(err, errlen, "out of memory");
            if (r)
                return tc_err(err, errlen, "%s:%u: exclude: %s", ex->at.file, ex->at.line, msg);
            tc_selector_free(&sel);
        }
    }
    return 0;
}

/* ------------------------------------------------------------ cells */

void tc_matrix_free(struct tc_matrix *m)
{
    free(m->axes);
    for (size_t i = 0; i < m->ncells; i++) {
        free(m->cells[i].name);
        free(m->cells[i].values);
    }
    free(m->cells);
    memset(m, 0, sizeof *m);
}

const char *tc_cell_value(const struct tc_config *conf, const struct tc_matrix *m, size_t cell,
                          const struct tc_keydef *def, char buf[24])
{
    const struct tc_setting *s;

    for (size_t i = 0; i < m->naxes; i++)
        if (m->axes[i].def == def)
            return m->cells[cell].values[i];
    if ((s = tc_get(conf, def->name))) {
        if (s->value.type == TT_INTEGER) {
            snprintf(buf, 24, "%lld", s->value.integer);
            return buf;
        }
        if (s->value.type == TT_STRING)
            return s->value.str;
        return "";
    }
    return def->defval ? def->defval : "";
}

/* the selector against a cell whose values are vals (one per axis) */
static int matches_vals(const struct tc_selector *sel, const struct tc_config *conf,
                        const struct tc_matrix *m, const char **vals, const char *case_name)
{
    for (size_t i = 0; i < sel->n; i++) {
        const struct tc_term *t = &sel->t[i];
        const char *v = NULL;
        char buf[24];

        if (!t->def) {
            if (!case_name || strcmp(case_name, t->value))
                return 0;
            continue;
        }
        for (size_t a = 0; a < m->naxes; a++)
            if (m->axes[a].def == t->def)
                v = vals[a];
        if (!v) {
            const struct tc_setting *s = tc_get(conf, t->def->name);

            if (s && s->value.type == TT_INTEGER) {
                snprintf(buf, sizeof buf, "%lld", s->value.integer);
                v = buf;
            } else if (s && s->value.type == TT_STRING)
                v = s->value.str;
            else
                v = t->def->defval ? t->def->defval : "";
        }
        if (strcmp(v, t->value))
            return 0;
    }
    return 1;
}

int tc_selector_matches(const struct tc_selector *sel, const struct tc_config *conf,
                        const struct tc_matrix *m, size_t cell, const char *case_name)
{
    return matches_vals(sel, conf, m, m->cells[cell].values, case_name);
}

int tc_matrix_build(const struct tc_config *conf, struct tc_matrix *m, char *err, size_t errlen)
{
    size_t nkeys, total = 1;
    const struct tc_keydef *keys = tc_keys(&nkeys);
    const struct tc_setting *ex = tc_get(conf, "exclude");
    struct tc_selector *excl = NULL;
    size_t nexcl = 0, *idx = NULL;
    const char **vals = NULL;
    int r = -1;

    memset(m, 0, sizeof *m);
    /* the axes in the order of the keys */
    for (size_t k = 0; k < nkeys; k++)
        for (size_t i = 0; i < conf->nmatrix; i++)
            if (!strcmp(conf->matrix[i].key, keys[k].name)) {
                struct tc_axis *a = realloc(m->axes, (m->naxes + 1) * sizeof *a);

                if (!a)
                    goto oom;
                m->axes = a;
                a[m->naxes].def = &keys[k];
                a[m->naxes].values = conf->matrix[i].value.list;
                a[m->naxes].nvalues = conf->matrix[i].value.nlist;
                a[m->naxes].at = conf->matrix[i].at;
                total *= conf->matrix[i].value.nlist;
                m->naxes++;
            }
    if (total > 4096)
        return tc_err(err, errlen, "%s: %zu cells, more than 4096", conf->source, total);
    if (ex) {
        if (!(excl = calloc(ex->value.nlist + 1, sizeof *excl)))
            goto oom;
        for (; nexcl < ex->value.nlist; nexcl++) {
            char msg[300];
            int pr = tc_selector_parse(ex->value.list[nexcl], 0, &excl[nexcl], msg, sizeof msg);

            if (pr == -2)
                goto oom;
            if (pr) {
                tc_err(err, errlen, "%s:%u: exclude: %s", ex->at.file, ex->at.line, msg);
                goto out;
            }
        }
    }
    if (!(idx = calloc(m->naxes + 1, sizeof *idx)) || !(vals = calloc(m->naxes + 1, sizeof *vals))
        || !(m->cells = calloc(total, sizeof *m->cells)))
        goto oom;
    for (size_t c = 0; c < total; c++) {
        size_t rest = c, len = 1;
        int excluded = 0;
        struct tc_cell *cell;
        char buf[24];

        /* the first axis varies slowest */
        for (size_t a = m->naxes; a-- > 0;) {
            idx[a] = rest % m->axes[a].nvalues;
            rest /= m->axes[a].nvalues;
            vals[a] = m->axes[a].values[idx[a]];
            len += strlen(vals[a]) + 1;
        }
        for (size_t e = 0; e < nexcl && !excluded; e++)
            excluded = matches_vals(&excl[e], conf, m, vals, NULL);
        if (excluded)
            continue;
        cell = &m->cells[m->ncells];
        if (!(cell->values = malloc((m->naxes + 1) * sizeof *cell->values)) || !(cell->name = malloc(len + 8))) {
            free(cell->values);
            cell->values = NULL;
            goto oom;
        }
        m->ncells++;
        cell->name[0] = 0;
        for (size_t a = 0; a < m->naxes; a++) {
            cell->values[a] = vals[a];
            if (a)
                strcat(cell->name, ",");
            strcat(cell->name, vals[a]);
        }
        if (m->naxes == 0)
            strcpy(cell->name, "default");
        {
            const char *comp = tc_cell_value(conf, m, m->ncells - 1, tc_key("compiler"), buf);
            const char *libs = tc_cell_value(conf, m, m->ncells - 1, tc_key("libs"), buf);

            cell->originals = strcmp(comp, "port") != 0 || strcmp(libs, "cd") == 0;
        }
    }
    if (m->ncells == 0) {
        tc_err(err, errlen, "%s:%u: exclude leaves %s no cell", ex ? ex->at.file : conf->source,
               ex ? ex->at.line : 0, conf->id);
        goto out;
    }
    r = 0;
    goto out;
oom:
    tc_err(err, errlen, "out of memory");
out:
    for (size_t e = 0; e < nexcl; e++)
        tc_selector_free(&excl[e]);
    free(excl);
    free(idx);
    free(vals);
    if (r)
        tc_matrix_free(m);
    return r;
}
