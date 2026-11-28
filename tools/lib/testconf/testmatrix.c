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
    if (def->vkind == TC_V_ONOFF_OR_SELECTORS) {
        tc_err(msg, msglen, "%s gives cells an emulator session of their own", key);
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

int tc_cell_isolated(const struct tc_config *conf, const struct tc_matrix *m, size_t cell)
{
    const struct tc_setting *s = tc_get(conf, "isolate");

    if (!s)
        return 0;
    if (s->value.type == TT_STRING)
        return !strcmp(s->value.str, "on");
    for (size_t i = 0; s->value.type == TT_LIST && i < s->value.nlist; i++) {
        struct tc_selector sel;
        char msg[300];
        int r;

        if (tc_selector_parse(s->value.list[i], 0, &sel, msg, sizeof msg))
            continue;           /* checked when the file was read */
        r = tc_selector_matches(&sel, conf, m, cell, NULL);
        tc_selector_free(&sel);
        if (r)
            return 1;
    }
    return 0;
}

/* ------------------------------------------------------------ the session key */

/* NAME=LEN:TEXT and a newline; no text as an unset value (gcc's
   -Wformat-overflow sees a null text on the inlined paths of a sanitizer
   build) */
static void key_add(FILE *f, const char *name, const char *text)
{
    if (!text)
        text = "";
    fprintf(f, "%s=%zu:%s\n", name, strlen(text), text);
}

char *tc_session_key(const struct tc_config *conf, const struct tc_matrix *m, size_t cell, const struct tc_env *env)
{
    static const char *const values[] = { "machine", "rtg", "timeout", "hang-after" };
    static const char *const lists[] = { "uae", "floppy", "mount" };
    char *text = NULL, buf[24];
    const char *dflt = NULL;
    size_t len = 0;
    FILE *f = open_memstream(&text, &len);
    int own;

    if (!f)
        return NULL;
    if (env)
        dflt = tc_env_runner(env, "timeout");
    for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
        const char *v = tc_cell_value(conf, m, cell, tc_key(values[i]), buf);

        /* a time limit left to the environment is the environment's */
        if (dflt && !*v && !strcmp(values[i], "timeout"))
            key_add(f, values[i], dflt);
        else
            key_add(f, values[i], v);
    }
    /* the lists are the same in every cell of a test (no list is an axis) */
    for (size_t k = 0; k < sizeof lists / sizeof lists[0]; k++) {
        const struct tc_setting *s = tc_get(conf, lists[k]);

        for (size_t i = 0; s && s->value.type == TT_LIST && i < s->value.nlist; i++)
            key_add(f, lists[k], s->value.list[i]);
    }
    /* the remote end of the serial port belongs to one program; a cell
       isolate names has its memory and RAM: of its own */
    own = !strcmp(tc_cell_value(conf, m, cell, tc_key("serial"), buf), "on") || tc_cell_isolated(conf, m, cell);
    if (own) {
        fprintf(f, "own=%zu:%s/%zu:%s|%zu\n", strlen(conf->dir ? conf->dir : ""), conf->dir ? conf->dir : "",
                strlen(conf->id ? conf->id : ""), conf->id ? conf->id : "", cell);
    }
    if (ferror(f)) {
        fclose(f);
        free(text);
        return NULL;
    }
    if (fclose(f)) {
        free(text);
        return NULL;
    }
    return text;
}

/* ------------------------------------------------------------ a run from the editor */

/* where a key of a cell is set: its axis, else its setting, else NULL */
static const struct tc_origin *key_at(const struct tc_config *conf, const struct tc_matrix *m, const char *key)
{
    const struct tc_keydef *def = tc_key(key);
    const struct tc_setting *s;

    for (size_t a = 0; a < m->naxes; a++)
        if (m->axes[a].def == def)
            return &m->axes[a].at;
    s = tc_get(conf, key);
    return s ? &s->at : NULL;
}

/* whether the axis terms of selector text (NULL: all cells) match cell, for
   some case; -1 a malformed selector (checked when the file was read), -2
   out of memory */
static int sel_cell(const char *text, const struct tc_config *conf, const struct tc_matrix *m, size_t cell)
{
    struct tc_selector sel;
    char msg[300];
    size_t j = 0;
    int r;

    if (!text)
        return 1;
    if ((r = tc_selector_parse(text, 1, &sel, msg, sizeof msg)))
        return r;
    /* the case terms left out */
    for (size_t i = 0; i < sel.n; i++)
        if (sel.t[i].def)
            sel.t[j++] = sel.t[i];
        else
            free(sel.t[i].value);
    sel.n = j;
    r = matches_vals(&sel, conf, m, m->cells[cell].values, NULL);
    tc_selector_free(&sel);
    return r;
}

/* the query text names the exit code or the requesters */
static int shell_query(const char *text)
{
    return !strcmp(text, "rc") || !strcmp(text, "requester");
}

/* the cells with editor-run = "on": no settings of the shell start, no
   assertion on what only a shell run captures; defaultdbug only there */
static int check_editor_run(const struct tc_config *conf, const struct tc_matrix *m, char *err, size_t errlen)
{
    static const char *const shell[] = { "runwrap", "runargs", "launcherargs", "pad" };

    for (size_t c = 0; c < m->ncells; c++) {
        char buf[24];
        int on = !strcmp(tc_cell_value(conf, m, c, tc_key("editor-run"), buf), "on");
        const struct tc_origin *er = key_at(conf, m, "editor-run");

        if (!on) {
            const struct tc_origin *at = key_at(conf, m, "defaultdbug");

            if (*tc_cell_value(conf, m, c, tc_key("defaultdbug"), buf))
                return tc_err(err, errlen, "%s:%u: defaultdbug without editor-run = \"on\" (cell %s): the debugger "
                              "module is used only by a run from the editor", at->file, at->line, m->cells[c].name);
            continue;
        }
        for (size_t k = 0; k < sizeof shell / sizeof shell[0]; k++) {
            const char *v = tc_cell_value(conf, m, c, tc_key(shell[k]), buf);
            const struct tc_origin *at = key_at(conf, m, shell[k]);

            if (*v && (k < 3 || !strcmp(v, "on")))
                return tc_err(err, errlen, "%s:%u: %s%s with editor-run = \"on\" (%s:%u, cell %s): the program "
                              "runs from the editor, not from the shell", at->file, at->line, shell[k],
                              k < 3 ? "" : " = \"on\"", er->file, er->line, m->cells[c].name);
        }
        /* r001 is the run from the editor: no jobline job of that ID */
        {
            const struct tc_setting *jl = tc_get(conf, "jobline");

            for (size_t i = 0; jl && jl->value.type == TT_LIST && i < jl->value.nlist; i++) {
                const char *id;
                size_t n;

                if (tc_jobline_job(jl->value.list[i], &id, &n) != TC_JOB_NONE && n == 4 && !memcmp(id, "r001", 4))
                    return tc_err(err, errlen, "%s:%u: jobline: the job ID r001 is the run from the editor in the "
                                  "cell %s with editor-run = \"on\" (%s:%u)", jl->at.file, jl->at.line,
                                  m->cells[c].name, er->file, er->line);
            }
        }
        /* the assertions, the deviations and the alternative ends on rc and
           requester whose selector matches the cell */
        for (size_t t = 0; t < 3; t++) {
            const struct tc_tabkey *v = t == 0 ? conf->asserts : t == 1 ? conf->deviations : conf->ends;
            size_t n = t == 0 ? conf->nasserts : t == 1 ? conf->ndeviations : conf->nends;

            for (size_t i = 0; i < n; i++) {
                const char *q = NULL;
                int r;

                if (t < 2 && shell_query(v[i].key))
                    q = v[i].key;
                for (size_t k = 0; t == 2 && !q && v[i].value.type == TT_LIST && k < v[i].value.nlist; k++) {
                    const char *a = v[i].value.list[k], *b = strchr(a, ' ');

                    if (b && ((b - a == 2 && !memcmp(a, "rc", 2)) || (b - a == 9 && !memcmp(a, "requester", 9))))
                        q = b - a == 2 ? "rc" : "requester";
                }
                if (!q)
                    continue;
                if ((r = sel_cell(v[i].sel, conf, m, c)) == -2)
                    return tc_err(err, errlen, "out of memory");
                if (r == 1)
                    return tc_err(err, errlen, "%s:%u: %s: %s on %s in the cell %s with editor-run = \"on\" (%s:%u): "
                                  "a run from the editor gives no exit code and no requesters", v[i].at.file,
                                  v[i].at.line, t == 2 ? v[i].key : q,
                                  t == 0 ? "an assertion" : t == 1 ? "a deviation" : "an alternative end", q,
                                  m->cells[c].name, er->file, er->line);
            }
        }
    }
    return 0;
}

/* the queries of a compile's warnings and return code against the cells
   (spec testing/basic-test-runner): compile.rc only in a cell that compiles
   from the command line (ab3, amiblitz3); compile.warnings and
   compile.warnings.K not in a cell of amiblitz3, whose warnings have
   another form and other names.  An alternative end names no such query
   (end_query refuses them). */
static int check_warning_queries(const struct tc_config *conf, const struct tc_matrix *m, char *err, size_t errlen)
{
    for (size_t t = 0; t < 2; t++) {
        const struct tc_tabkey *v = t == 0 ? conf->asserts : conf->deviations;
        size_t n = t == 0 ? conf->nasserts : conf->ndeviations;

        for (size_t i = 0; i < n; i++) {
            const char *k = v[i].key;
            int rc = !strcmp(k, "compile.rc");
            int warn = !strncmp(k, "compile.warnings", 16) && (!k[16] || k[16] == '.');

            if (!rc && !warn)
                continue;
            for (size_t c = 0; c < m->ncells; c++) {
                char b[24];
                const char *comp = tc_cell_value(conf, m, c, tc_key("compiler"), b);
                int bad = rc ? strcmp(comp, "ab3") && strcmp(comp, "amiblitz3") : !strcmp(comp, "amiblitz3");
                int r;

                if (!bad)
                    continue;
                if ((r = sel_cell(v[i].sel, conf, m, c)) == -2)
                    return tc_err(err, errlen, "out of memory");
                if (r == 1)
                    return tc_err(err, errlen, "%s:%u: %s: %s on %s in the cell %s of compiler = \"%s\": %s",
                                  v[i].at.file, v[i].at.line, k, t == 0 ? "an assertion" : "a deviation", k,
                                  m->cells[c].name, comp,
                                  rc ? "only a compile from the command line (compiler = \"ab3\" or "
                                       "\"amiblitz3\") has a return code"
                                     : "the warnings of AmiBlitz3 have another form and other names (exclude the "
                                       "cell or select compiler=ab3)");
            }
        }
    }
    return 0;
}

/* the cells of AB3 mode (tc_matrix_build in testconf.h): source-mode only
   with the compilers ab3 and amiblitz3, bb2 not with amiblitz3; the
   deviations of amiblitz3 cells with their variant, no variant elsewhere */
static int check_ab3(const struct tc_config *conf, const struct tc_matrix *m, char *err, size_t errlen)
{
    for (size_t c = 0; c < m->ncells; c++) {
        char b1[24], b2[24];
        const char *comp = tc_cell_value(conf, m, c, tc_key("compiler"), b1);
        const char *mode = tc_cell_value(conf, m, c, tc_key("source-mode"), b2);
        const struct tc_origin *at = key_at(conf, m, "source-mode");

        if (!*mode)
            continue;
        if (strcmp(comp, "ab3") && strcmp(comp, "amiblitz3"))
            return tc_err(err, errlen, "%s:%u: source-mode in the cell %s of compiler = \"%s\": only the compilers "
                          "ab3 and amiblitz3 read a source mode", at->file, at->line, m->cells[c].name, comp);
        if (!strcmp(comp, "amiblitz3") && !strcmp(mode, "bb2"))
            return tc_err(err, errlen, "%s:%u: source-mode = \"bb2\" in the cell %s of compiler = \"amiblitz3\": "
                          "AmiBlitz3 has no BB2 mode", at->file, at->line, m->cells[c].name);
    }
    for (size_t i = 0; i < conf->ndeviations; i++) {
        const struct tc_tabkey *d = &conf->deviations[i];
        const struct tc_tabkey *v = tc_variant_of(conf, d->sel);
        size_t oracle = 0, other = 0;
        const char *other_name = NULL;

        for (size_t c = 0; c < m->ncells; c++) {
            int r = sel_cell(d->sel, conf, m, c);

            if (r == -2)
                return tc_err(err, errlen, "out of memory");
            if (r != 1)
                continue;
            if (m->cells[c].oracle)
                oracle++;
            else if (!other++)
                other_name = m->cells[c].name;
        }
        if (oracle && other)
            return tc_err(err, errlen, "%s:%u: [deviations.\"%s\"] matches the cells of amiblitz3 and the cell %s: "
                          "a difference of AmiBlitz3 is declared for its cells alone", d->at.file, d->at.line, d->sel,
                          other_name);
        if (oracle && !v)
            return tc_err(err, errlen, "%s:%u: [deviations.\"%s\"] of the cells of amiblitz3 has no variant (the "
                          "row of docs/ab3/variant.md that records the difference)", d->at.file, d->at.line, d->sel);
        if (!oracle && v)
            return tc_err(err, errlen, "%s:%u: variant in [deviations.\"%s\"], which matches no cell of "
                          "amiblitz3: a variant names a difference of AmiBlitz3", v->at.file, v->at.line, d->sel);
    }
    return 0;
}

int tc_cell_ab3(const struct tc_config *conf, const struct tc_matrix *m, size_t cell)
{
    char b1[24], b2[24];
    const char *comp = tc_cell_value(conf, m, cell, tc_key("compiler"), b1);

    return (!strcmp(comp, "ab3") || !strcmp(comp, "amiblitz3"))
           && strcmp(tc_cell_value(conf, m, cell, tc_key("source-mode"), b2), "bb2");
}

int tc_expect_error_ok(const struct tc_matrix *m, const char *file, unsigned line, char *err, size_t errlen)
{
    for (size_t c = 0; c < m->ncells; c++)
        if (m->cells[c].oracle)
            return tc_err(err, errlen, "%s:%u: !ExpectError in a test with the cell %s of compiler = \"amiblitz3\": "
                          "AmiBlitz3 runs without its debugger and reports no runtime error (exclude the cell)", file,
                          line, m->cells[c].name);
    return 0;
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

            cell->oracle = !strcmp(comp, "amiblitz3");
            cell->originals = !cell->oracle && (!strcmp(comp, "orig195") || !strcmp(comp, "orig210")
                                                || !strcmp(libs, "cd"));
        }
    }
    if (m->ncells == 0) {
        tc_err(err, errlen, "%s:%u: exclude leaves %s no cell", ex ? ex->at.file : conf->source,
               ex ? ex->at.line : 0, conf->id);
        goto out;
    }
    /* a data file whose fixed option needs original material, in a test
       with a cell of the side port (basic-test-local-data D4) */
    for (size_t i = 0; i < conf->ndata; i++) {
        const struct tc_data *d = &conf->data[i];
        int k = -1;

        if (d->action != TC_DA_BASIC_COMPILE)
            continue;
        if (strcmp(d->opt[TC_DO_COMPILER], "same") && strcmp(d->opt[TC_DO_COMPILER], "port"))
            k = TC_DO_COMPILER;
        else if (!strcmp(d->opt[TC_DO_LIBS], "cd"))
            k = TC_DO_LIBS;
        if (k < 0)
            continue;
        for (size_t c = 0; c < m->ncells; c++)
            if (!m->cells[c].originals) {
                tc_err(err, errlen, "%s:%u: data \"%s\": %s = \"%s\" needs original material, but the test has "
                       "the cell %s of the side port", d->at.file, d->optline[k], d->file, tc_dopt_name(k),
                       d->opt[k], m->cells[c].name);
                goto out;
            }
    }
    if (check_editor_run(conf, m, err, errlen) || check_ab3(conf, m, err, errlen)
        || check_warning_queries(conf, m, err, errlen))
        goto out;
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
