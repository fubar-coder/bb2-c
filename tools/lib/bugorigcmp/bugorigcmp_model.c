/*
 * bugorigcmp_model.c - a bug-orig test of the BASIC test model (ID.test.bas
 * and its ID.test.toml) and a directory's tests.toml as cases of the
 * shared test model, for fix-scope (bugorigcmp.h).
 */
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>
#include "bugorigcmp.h"
#include "bugorigcmp_int.h"
#include "testpp.h"
#include "testtoml.h"

/* a part of a key (libscmp.h): printable ASCII, \| \\ \t and \xHH */
static void put_part(FILE *out, const char *s)
{
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;

        if (c == '|' || c == '\\')
            fprintf(out, "\\%c", c);
        else if (c == '\t')
            fputs("\\t", out);
        else if (c < 0x20 || c > 0x7e)
            fprintf(out, "\\x%02X", c);
        else
            fputc(c, out);
    }
}

/* a value of the configuration as text: "string", 12, true, ["a", "b"] */
static char *value_text(const struct tt_value *v)
{
    struct bstr s = { 0 };
    char buf[40];

    switch (v->type) {
    case TT_STRING:
        bstr_add(&s, "\"", 1);
        bstr_add(&s, v->str, strlen(v->str));
        bstr_add(&s, "\"", 1);
        break;
    case TT_INTEGER:
        snprintf(buf, sizeof buf, "%lld", v->integer);
        bstr_add(&s, buf, strlen(buf));
        break;
    case TT_BOOLEAN:
        bstr_add(&s, v->boolean ? "true" : "false", v->boolean ? 4 : 5);
        break;
    default:
        bstr_add(&s, "[", 1);
        for (size_t i = 0; i < v->nlist; i++) {
            if (i)
                bstr_add(&s, ", ", 2);
            bstr_add(&s, "\"", 1);
            bstr_add(&s, v->list[i], strlen(v->list[i]));
            bstr_add(&s, "\"", 1);
        }
        bstr_add(&s, "]", 1);
    }
    return bstr_take(&s);
}

/* the entries of a configuration file as cases ROUTINE|toml:TABLE.KEY||1;
   with own, the keys fixed-orig, fixed and extension are the case
   ROUTINE|KEY||1 [fixed #N] (the fix's own) */
static int put_toml(FILE *out, const char *routine, const char *text, const char *path, int own, char *err,
                    size_t errlen)
{
    struct tt_doc d = { 0 };
    int r = tt_parse(path, text, strlen(text), &d, err, errlen);

    if (r == -2)
        snprintf(err, errlen, "out of memory");
    if (r) {
        tt_free(&d);
        return r == -2 ? -1 : -2;
    }
    for (size_t i = 0; i < d.n; i++) {
        const struct tt_entry *e = &d.e[i];
        char *v;

        if (own && !e->ntable && (!strcmp(e->key, "fixed-orig") || !strcmp(e->key, "fixed")
                    || !strcmp(e->key, "extension"))
            && e->value.type == TT_INTEGER && e->value.integer > 0) {
            put_part(out, routine);
            fprintf(out, "|%s||1 [fixed #%lld]\t%s %lld\n", e->key, e->value.integer, e->key, e->value.integer);
            continue;
        }
        if (!(v = value_text(&e->value))) {
            tt_free(&d);
            snprintf(err, errlen, "out of memory");
            return -1;
        }
        put_part(out, routine);
        fputs("|toml:", out);
        for (size_t k = 0; k < e->ntable; k++) {
            put_part(out, e->table[k]);
            fputc('.', out);
        }
        put_part(out, e->key);
        fputs("||1\t", out);
        bugorigcmp_put_value(out, v);
        fputc('\n', out);
        free(v);
    }
    tt_free(&d);
    return 0;
}

int bugorigcmp_project_model(FILE *out, const char *id, const char *source, const char *own, const char *path,
                             char *err, size_t errlen)
{
    struct tp_source src = { 0 };
    struct tp_params par = { 0 };
    char file[600], testdir[300];
    int n;

    snprintf(file, sizeof file, "%s.test.bas", path);
    snprintf(testdir, sizeof testdir, "CORPUS:%s.test", id);
    par.file = file;
    par.test_id = id;
    par.testdata = "CORPUS:data";
    par.testdir = testdir;
    if ((n = tp_process(source, strlen(source), &par, &src)) != 0) {
        if (n < 0)
            snprintf(err, errlen, "out of memory");
        else
            snprintf(err, errlen, "%s", src.errors.v[0]);
        tp_free(&src);
        return n < 0 ? -1 : -2;
    }
    for (size_t i = 0; i < src.nchecks; i++) {
        const struct tp_check *c = &src.checks[i];
        struct bstr v = { 0 };
        const char *kind = tp_kind_name(c->kind);
        char *t;

        put_part(out, id);
        fputs("|check.", out);
        put_part(out, c->ns);
        fputc('.', out);
        if (c->name)
            put_part(out, c->name);
        else
            fprintf(out, "#%u", c->number);
        fputs("||1\t", out);
        /* what the check checks: its macro and arguments, not its line or number */
        bstr_add(&v, kind, strlen(kind));
        if (c->expr)
            bstr_add(&v, "{", 1), bstr_add(&v, c->expr, strlen(c->expr));
        if (c->expected)
            bstr_add(&v, ",", 1), bstr_add(&v, c->expected, strlen(c->expected));
        if (c->delta)
            bstr_add(&v, ",", 1), bstr_add(&v, c->delta, strlen(c->delta));
        if (c->message)
            bstr_add(&v, "{\"", 2), bstr_add(&v, c->message, strlen(c->message)), bstr_add(&v, "\"", 1);
        bstr_add(&v, "}", 1);
        if (!(t = bstr_take(&v))) {
            tp_free(&src);
            snprintf(err, errlen, "out of memory");
            return -1;
        }
        bugorigcmp_put_value(out, t);
        fputc('\n', out);
        free(t);
    }
    tp_free(&src);
    if (own) {
        snprintf(file, sizeof file, "%s.test.toml", path);
        return put_toml(out, id, own, file, 1, err, errlen);
    }
    return 0;
}

int bugorigcmp_project_conf(FILE *out, const char *name, const char *text, const char *path, char *err,
                            size_t errlen)
{
    return put_toml(out, name, text, path, 0, err, errlen);
}
