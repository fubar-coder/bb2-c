/*
 * testconf.c - the files of the run configuration, their keys, the
 * hierarchy and the merge (testconf.h).
 */
#include "testconf.h"
#include "testconf_int.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* the keys of the top table, in the order of the documentation and of the
   axes of a cell's name */
static const struct tc_keydef keys[] = {
    { "compiler", TC_V_ENUM, TC_IN_BOTH, TC_RUN, "port|orig195|orig210", 0, -1,
      "the compiler: port (blitz2-c), orig195 (blitz2 1.95, the build of blitz2.s), orig210 (the CD's blitz2 2.10)", "port" },
    { "libs", TC_V_ENUM, TC_IN_BOTH, TC_RUN, "cd|clib", 0, -1,
      "the libraries: cd (the CD's acidlibs and deflibs), clib (the C libraries)", "clib" },
    { "machine", TC_V_STRING, TC_IN_BOTH, TC_RUN, NULL, 0, -1,
      "the emulated machine (a1200, a500, a500-68010, a500-204, a600)", "a1200" },
    { "runtime-errors", TC_V_ENUM, TC_IN_BOTH, TC_RUN, "off|on", 0, -1,
      "runtime error checking of the compiled program", "off" },
    { "rwe", TC_V_LIST, TC_IN_BOTH, TC_RUN, NULL, 0, -1,
      "the RWE user libraries the program needs", NULL },
    { "timeout", TC_V_INT, TC_IN_BOTH, TC_RUN, NULL, 1, -1,
      "the time limit of an emulator session (seconds)", NULL },
    { "hang-after", TC_V_INT, TC_IN_BOTH, TC_RUN, NULL, 1, -1,
      "a session silent for this many seconds ends as a hang", NULL },
    { "stack", TC_V_INT, TC_IN_BOTH, TC_RUN, NULL, 1, -1,
      "the stack of the compiler and the program (bytes)", NULL },
    { "serial", TC_V_ENUM, TC_IN_BOTH, TC_RUN, "off|on", 0, -1,
      "the remote end of the serial tests on the emulated serial port", "off" },
    { "rtg", TC_V_ENUM, TC_IN_BOTH, TC_RUN, "off|on", 0, -1,
      "the RTG variant of the emulator", "off" },
    { "pad", TC_V_ENUM, TC_IN_BOTH, TC_RUN, "off|on", 0, -1,
      "inject's pad: zeroed bytes behind every allocation of the program", "off" },
    { "wrap", TC_V_STRING, TC_IN_BOTH, TC_RUN, NULL, 0, 0,
      "PATH[|ARGS]: the compiler is started through PATH", NULL },
    { "runwrap", TC_V_STRING, TC_IN_BOTH, TC_RUN, NULL, 0, 0,
      "PATH[|ARGS]: the program is started through PATH", NULL },
    { "runargs", TC_V_STRING, TC_IN_BOTH, TC_RUN, NULL, 0, -1,
      "the arguments of the program", NULL },
    { "launcherargs", TC_V_STRING, TC_IN_BOTH, TC_RUN, NULL, 0, -1,
      "the options of the launcher", NULL },
    { "files", TC_V_LIST, TC_IN_BOTH, TC_RUN, NULL, 0, 1,
      "DEST|SRC[|TAG]: files staged as CORPUS:DEST", NULL },
    { "dirs", TC_V_LIST, TC_IN_BOTH, TC_RUN, NULL, 0, -1,
      "empty directories CORPUS:DEST", NULL },
    { "remove", TC_V_LIST, TC_IN_BOTH, TC_RUN, NULL, 0, -1,
      "CORPUS:DEST removed after staging", NULL },
    { "mount", TC_V_LIST, TC_IN_BOTH, TC_RUN, NULL, 0, 1,
      "NAME|DIR: read-only volumes of the program", NULL },
    { "floppy", TC_V_LIST, TC_IN_BOTH, TC_RUN, NULL, 0, 1,
      "N|SRC: the disk image SRC in drive DFN", NULL },
    { "uae", TC_V_LIST, TC_IN_BOTH, TC_RUN, NULL, 0, -1,
      "KEY=VALUE: changes of the emulator configuration", NULL },
    { "jobline", TC_V_LIST, TC_IN_BOTH, TC_RUN, NULL, 0, -1,
      "lines of tedstub's job list", NULL },
    { "labels", TC_V_LIST, TC_IN_BOTH, TC_REG, NULL, 0, -1,
      "further ctest labels of the registered tests", NULL },
    { "cost", TC_V_INT, TC_IN_BOTH, TC_REG, NULL, 0, -1,
      "the ctest COST of the registered tests", NULL },
    { "fixtures", TC_V_LIST, TC_IN_BOTH, TC_REG, NULL, 0, -1,
      "the ctest fixtures the registered tests require", NULL },
    { "exclude", TC_V_LIST, TC_IN_BOTH, 0, NULL, 0, -1,
      "cell selectors: the combinations of axis values that are no cell", NULL },
    { "root", TC_V_BOOL, TC_IN_DIR, 0, NULL, 0, -1,
      "the search for configuration files stops at this file", NULL },
    { "fixed-orig", TC_V_INT, TC_IN_TEST, 0, NULL, 1, -1,
      "the issue that fixed the original bug the test shows", NULL },
    { "fixed", TC_V_INT, TC_IN_TEST, 0, NULL, 1, -1,
      "the issue that fixed a bug that is not in the original", NULL },
};

const struct tc_keydef *tc_keys(size_t *n)
{
    *n = sizeof keys / sizeof keys[0];
    return keys;
}

const struct tc_keydef *tc_key(const char *name)
{
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
        if (!strcmp(keys[i].name, name))
            return &keys[i];
    return NULL;
}

int tc_enum_ok(const struct tc_keydef *def, const char *value)
{
    const char *p = def->values;
    size_t n = strlen(value);

    while (p && *p) {
        const char *e = strchr(p, '|');
        size_t len = e ? (size_t)(e - p) : strlen(p);

        if (len == n && !memcmp(p, value, n))
            return 1;
        p = e ? e + 1 : NULL;
    }
    return 0;
}

int tc_err(char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
    return -1;
}

char *tc_strdup(const char *s)
{
    size_t n = strlen(s);
    char *r = malloc(n + 1);

    if (r)
        memcpy(r, s, n + 1);
    return r;
}

/* ------------------------------------------------------------ the environment */

void tc_env_free(struct tc_env *env)
{
    for (size_t i = 0; i < env->nph; i++) {
        free(env->ph[i].name);
        free(env->ph[i].value);
    }
    free(env->ph);
    free(env->top);
    free(env->file);
    memset(env, 0, sizeof *env);
}

static int ph_name_ok(const char *s, size_t n)
{
    if (n == 0 || s[0] < 'A' || s[0] > 'Z')
        return 0;
    for (size_t i = 1; i < n; i++)
        if (!((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= '0' && s[i] <= '9') || s[i] == '_'))
            return 0;
    return 1;
}

int tc_env_parse(const char *file, const char *text, struct tc_env *env, char *err, size_t errlen)
{
    struct tt_doc d = {0};
    int r = tt_parse(file, text, strlen(text), &d, err, errlen);

    if (r == 0 && !(env->file = tc_strdup(file)))
        r = -2;
    for (size_t i = 0; r == 0 && i < d.n; i++) {
        const struct tt_entry *e = &d.e[i];

        if (e->ntable == 0 && !strcmp(e->key, "top")) {
            if (e->value.type != TT_STRING)
                r = tc_err(err, errlen, "%s:%u: top must be a string", file, e->line);
            else if (!(env->top = tc_strdup(e->value.str)))
                r = -2;
        } else if (e->ntable == 1 && !strcmp(e->table[0], "placeholders")) {
            struct tc_placeholder *ph;

            if (!ph_name_ok(e->key, strlen(e->key))) {
                r = tc_err(err, errlen, "%s:%u: %s is no placeholder name ([A-Z][A-Z0-9_]*)", file,
                           e->line, e->key);
                break;
            }
            if (e->value.type != TT_STRING) {
                r = tc_err(err, errlen, "%s:%u: the placeholder %s must be a string", file, e->line, e->key);
                break;
            }
            ph = realloc(env->ph, (env->nph + 1) * sizeof *ph);
            if (!ph) {
                r = -2;
                break;
            }
            env->ph = ph;
            ph[env->nph].name = tc_strdup(e->key);
            ph[env->nph].value = tc_strdup(e->value.str);
            env->nph++;
            if (!ph[env->nph - 1].name || !ph[env->nph - 1].value)
                r = -2;
        } else
            r = tc_err(err, errlen, "%s:%u: unknown key %s in the environment file", file, e->line, e->key);
    }
    for (size_t i = 0; r == 0 && i < d.ntables; i++)
        if (d.tables[i].npath != 1 || strcmp(d.tables[i].path[0], "placeholders"))
            r = tc_err(err, errlen, "%s:%u: unknown table in the environment file", file, d.tables[i].line);
    tt_free(&d);
    if (r == -2)
        tc_err(err, errlen, "%s: out of memory", file);
    return r ? -1 : 0;
}

int tc_read_file(const char *file, char **text, size_t *len)
{
    FILE *f = fopen(file, "rb");
    char *buf = NULL;
    size_t n = 0, cap = 0;

    if (!f)
        return -1;
    for (;;) {
        size_t got;

        if (cap - n < 4096) {
            char *b = realloc(buf, cap + 65536 + 1);

            if (!b) {
                free(buf);
                fclose(f);
                return -1;
            }
            buf = b;
            cap += 65536;
        }
        got = fread(buf + n, 1, cap - n, f);
        n += got;
        if (got == 0)
            break;
    }
    if (ferror(f)) {
        free(buf);
        fclose(f);
        return -1;
    }
    fclose(f);
    buf[n] = 0;
    *text = buf;
    *len = n;
    return 0;
}

int tc_env_load(const char *file, struct tc_env *env, char *err, size_t errlen)
{
    char *text;
    size_t len;
    int r;

    if (tc_read_file(file, &text, &len))
        return tc_err(err, errlen, "%s: cannot read: %s", file, strerror(errno));
    if (strlen(text) != len) {
        free(text);
        return tc_err(err, errlen, "%s: a 0 byte in the file", file);
    }
    r = tc_env_parse(file, text, env, err, errlen);
    free(text);
    return r;
}

int tc_placeholders(const struct tc_env *env, const char *text, char **out, char *err, size_t errlen)
{
    size_t cap = strlen(text) + 1, n = 0;
    char *buf = malloc(cap);
    const char *p = text;

    if (!buf)
        return -2;
    while (*p) {
        const char *q;

        if (*p == '@' && (q = strchr(p + 1, '@')) && ph_name_ok(p + 1, (size_t)(q - p - 1))) {
            const char *v = NULL;
            size_t vl;

            for (size_t i = 0; env && i < env->nph; i++)
                if (strlen(env->ph[i].name) == (size_t)(q - p - 1)
                    && !memcmp(env->ph[i].name, p + 1, (size_t)(q - p - 1)))
                    v = env->ph[i].value;
            if (!v) {
                snprintf(err, errlen, "%.*s", (int)(q - p + 1), p);
                free(buf);
                return -1;
            }
            vl = strlen(v);
            if (n + vl + 1 > cap) {
                char *b = realloc(buf, cap = n + vl + strlen(q) + 1);

                if (!b) {
                    free(buf);
                    return -2;
                }
                buf = b;
            }
            memcpy(buf + n, v, vl);
            n += vl;
            p = q + 1;
            continue;
        }
        if (n + 2 > cap) {
            char *b = realloc(buf, cap *= 2);

            if (!b) {
                free(buf);
                return -2;
            }
            buf = b;
        }
        buf[n++] = *p++;
    }
    buf[n] = 0;
    *out = buf;
    return 0;
}

/* ------------------------------------------------------------ paths */

/* a/b + c, lexically normalised ("." and ".." parts, double "/") */
char *tc_path_join(const char *dir, const char *rel)
{
    size_t n = strlen(dir) + strlen(rel) + 2;
    char *all = malloc(n), *out = malloc(n + 1);
    char *save, *part;
    size_t o = 0;

    if (!all || !out) {
        free(all);
        free(out);
        return NULL;
    }
    if (rel[0] == '/')
        snprintf(all, n, "%s", rel);
    else
        snprintf(all, n, "%s/%s", dir, rel);
    out[0] = 0;
    for (part = strtok_r(all, "/", &save); part; part = strtok_r(NULL, "/", &save)) {
        if (!strcmp(part, "."))
            continue;
        if (!strcmp(part, "..")) {
            while (o > 0 && out[o - 1] != '/')
                o--;
            if (o > 0)
                o--;
            out[o] = 0;
            continue;
        }
        out[o++] = '/';
        strcpy(out + o, part);
        o += strlen(part);
    }
    if (o == 0)
        strcpy(out, "/");
    free(all);
    return out;
}

/* the directory of a path (malloc'd) */
static char *dir_of(const char *path)
{
    const char *s = strrchr(path, '/');

    if (!s)
        return tc_strdup(".");
    if (s == path)
        return tc_strdup("/");
    {
        char *r = malloc((size_t)(s - path) + 1);

        if (r) {
            memcpy(r, path, (size_t)(s - path));
            r[s - path] = 0;
        }
        return r;
    }
}

static int file_exists(const char *path)
{
    struct stat st;

    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

/* the field f ("|" between the fields) of s made absolute relative to dir */
static char *resolve_path_field(const char *s, int f, const char *dir)
{
    const char *start = s, *end;
    size_t pre, len;
    char *field, *abs, *r;

    for (int i = 0; i < f; i++) {
        start = strchr(start, '|');
        if (!start)
            return tc_strdup(s);
        start++;
    }
    end = strchr(start, '|');
    if (!end)
        end = start + strlen(start);
    len = (size_t)(end - start);
    if (len == 0 || start[0] == '/' || memchr(start, ':', len))
        return tc_strdup(s);
    field = malloc(len + 1);
    if (!field)
        return NULL;
    memcpy(field, start, len);
    field[len] = 0;
    abs = tc_path_join(dir, field);
    free(field);
    if (!abs)
        return NULL;
    pre = (size_t)(start - s);
    r = malloc(pre + strlen(abs) + strlen(end) + 1);
    if (r) {
        memcpy(r, s, pre);
        strcpy(r + pre, abs);
        strcat(r, end);
    }
    free(abs);
    return r;
}

/* ------------------------------------------------------------ one file */

/* a parsed file of the hierarchy */
struct cfile {
    struct tt_doc doc;
    char *dir;                  /* its directory */
    unsigned depth;
    int own;                    /* the test's own file */
};

static int where_err(const struct cfile *f, const struct tt_entry *e, const struct tc_keydef *def,
                     char *err, size_t errlen)
{
    if (def->where == TC_IN_TEST)
        return tc_err(err, errlen,
                      "%s:%u: %s is a key of a single test, valid only in its ID.test.toml",
                      f->doc.file, e->line, e->key);
    return tc_err(err, errlen, "%s:%u: %s is valid only in a directory's tests.toml",
                  f->doc.file, e->line, e->key);
}

int tc_check_type(const char *file, unsigned line, const struct tc_keydef *def,
                  const struct tt_value *v, char *err, size_t errlen)
{
    static const int want[] = { TT_STRING, TT_STRING, TT_INTEGER, TT_BOOLEAN, TT_LIST };

    if (v->type != want[def->vkind])
        return tc_err(err, errlen, "%s:%u: %s must be a%s %s, not a%s %s", file, line, def->name,
                      want[def->vkind] == TT_INTEGER ? "n" : "", tt_type_name(want[def->vkind]),
                      v->type == TT_INTEGER ? "n" : "", tt_type_name(v->type));
    if (def->vkind == TC_V_ENUM && !tc_enum_ok(def, v->str))
        return tc_err(err, errlen, "%s:%u: %s must be one of %s, not \"%s\"", file, line, def->name,
                      def->values, v->str);
    if (def->vkind == TC_V_INT && v->integer < def->min)
        return tc_err(err, errlen, "%s:%u: %s must be at least %lld, not %lld", file, line, def->name,
                      def->min, v->integer);
    return 0;
}

static int check_file(const struct cfile *f, char *err, size_t errlen)
{
    const struct tt_doc *d = &f->doc;

    for (size_t i = 0; i < d->ntables; i++) {
        const struct tt_table *t = &d->tables[i];
        const char *name = t->path[0];

        if (!strcmp(name, "matrix") || !strcmp(name, "asserts") || !strcmp(name, "deviations")) {
            if (t->npath > (strcmp(name, "matrix") ? 2u : 1u))
                return tc_err(err, errlen, "%s:%u: the table [%s] has no sub tables but by a cell selector",
                              d->file, t->line, name);
            if (!strcmp(name, "deviations") && !f->own)
                return tc_err(err, errlen,
                              "%s:%u: deviations is a key of a single test, valid only in its ID.test.toml",
                              d->file, t->line);
            continue;
        }
        return tc_err(err, errlen, "%s:%u: unknown table [%s]", d->file, t->line, name);
    }
    for (size_t i = 0; i < d->n; i++) {
        const struct tt_entry *e = &d->e[i];
        const struct tc_keydef *def;

        if (e->ntable == 0) {
            if (!(def = tc_key(e->key)))
                return tc_err(err, errlen, "%s:%u: unknown key %s", d->file, e->line, e->key);
            if (!(def->where & (f->own ? TC_IN_TEST : TC_IN_DIR)))
                return where_err(f, e, def, err, errlen);
            if (tc_check_type(d->file, e->line, def, &e->value, err, errlen))
                return -1;
            continue;
        }
        if (!strcmp(e->table[0], "deviations") && e->ntable == 1)
            return tc_err(err, errlen,
                          "%s:%u: a deviation needs the cell selector of the cells with the original's "
                          "faulty part ([deviations.\"SELECTOR\"])", d->file, e->line);
        if (!strcmp(e->table[0], "matrix")) {
            if (tc_axis_check(d->file, e->line, e->key, &e->value, err, errlen))
                return -1;
            continue;
        }
        if (e->value.type != TT_STRING)
            return tc_err(err, errlen, "%s:%u: the assertion %s must be a string", d->file, e->line, e->key);
    }
    return 0;
}

/* ------------------------------------------------------------ the merge */

void tc_tabkey_free(struct tc_tabkey *t)
{
    free(t->sel);
    free(t->key);
    tt_value_free(&t->value);
}

void tc_free(struct tc_config *c)
{
    for (size_t i = 0; i < c->nset; i++)
        tt_value_free(&c->set[i].value);
    free(c->set);
    for (size_t i = 0; i < c->nmatrix; i++)
        tc_tabkey_free(&c->matrix[i]);
    free(c->matrix);
    for (size_t i = 0; i < c->nasserts; i++)
        tc_tabkey_free(&c->asserts[i]);
    free(c->asserts);
    for (size_t i = 0; i < c->ndeviations; i++)
        tc_tabkey_free(&c->deviations[i]);
    free(c->deviations);
    for (size_t i = 0; i < c->nfiles; i++)
        free(c->files[i]);
    free(c->files);
    free(c->source);
    free(c->id);
    free(c->dir);
    memset(c, 0, sizeof *c);
}

const struct tc_setting *tc_get(const struct tc_config *conf, const char *key)
{
    for (size_t i = 0; i < conf->nset; i++)
        if (!strcmp(conf->set[i].def->name, key))
            return &conf->set[i];
    return NULL;
}

/* a string of a setting or an axis: placeholders, then the path field */
static int resolve_string(const struct tc_env *env, const char *file, unsigned line, int pathfield,
                          const char *dir, char **s, char *err, size_t errlen)
{
    char *out, *abs;
    char ph[128];
    int r = tc_placeholders(env, *s, &out, ph, sizeof ph);

    if (r == -1)
        return tc_err(err, errlen, "%s:%u: unknown placeholder %s", file, line, ph);
    if (r)
        return tc_err(err, errlen, "%s:%u: out of memory", file, line);
    free(*s);
    *s = out;
    if (pathfield < 0)
        return 0;
    if (!(abs = resolve_path_field(out, pathfield, dir)))
        return tc_err(err, errlen, "%s:%u: out of memory", file, line);
    free(*s);
    *s = abs;
    return 0;
}

int tc_resolve_value(const struct tc_env *env, const char *file, unsigned line, int pathfield,
                     const char *dir, struct tt_value *v, char *err, size_t errlen)
{
    if (v->type == TT_STRING)
        return resolve_string(env, file, line, pathfield, dir, &v->str, err, errlen);
    if (v->type == TT_LIST)
        for (size_t i = 0; i < v->nlist; i++)
            if (resolve_string(env, file, line, pathfield, dir, &v->list[i], err, errlen))
                return -1;
    return 0;
}

static int add_tabkey(struct tc_tabkey **v, size_t *n, const struct tt_entry *e, const char *sel,
                      const struct tc_origin *at)
{
    struct tc_tabkey *nv = realloc(*v, (*n + 1) * sizeof *nv);
    struct tc_tabkey *t;

    if (!nv)
        return -1;
    *v = nv;
    t = &nv[*n];
    memset(t, 0, sizeof *t);
    t->at = *at;
    if ((sel && !(t->sel = tc_strdup(sel))) || !(t->key = tc_strdup(e->key))
        || tt_value_copy(&t->value, &e->value)) {
        tc_tabkey_free(t);
        return -1;
    }
    (*n)++;
    return 0;
}

static int same_sel(const char *a, const char *b)
{
    return (!a && !b) || (a && b && !strcmp(a, b));
}

static int merge(struct tc_config *c, struct cfile *files, size_t nfiles, const struct tc_env *env,
                 char *err, size_t errlen)
{
    for (size_t fi = 0; fi < nfiles; fi++) {
        const struct cfile *f = &files[fi];

        for (size_t i = 0; i < f->doc.n; i++) {
            const struct tt_entry *e = &f->doc.e[i];
            struct tc_origin at = { c->files[fi], e->line, f->depth };

            if (e->ntable == 0) {
                const struct tc_keydef *def = tc_key(e->key);
                struct tc_setting *s;

                if (tc_get(c, def->name))
                    continue;           /* a nearer file set it */
                s = realloc(c->set, (c->nset + 1) * sizeof *s);
                if (!s)
                    return tc_err(err, errlen, "out of memory");
                c->set = s;
                s = &s[c->nset];
                s->def = def;
                s->at = at;
                if (tt_value_copy(&s->value, &e->value))
                    return tc_err(err, errlen, "out of memory");
                c->nset++;
                if (def->vkind == TC_V_STRING || def->vkind == TC_V_LIST)
                    if (tc_resolve_value(env, at.file, e->line, def->pathfield, f->dir, &s->value, err,
                                         errlen))
                        return -1;
                continue;
            }
            {
                const char *tab = e->table[0];
                const char *sel = e->ntable > 1 ? e->table[1] : NULL;
                struct tc_tabkey **v;
                size_t *n, j;

                if (!strcmp(tab, "matrix")) {
                    v = &c->matrix;
                    n = &c->nmatrix;
                } else if (!strcmp(tab, "asserts")) {
                    v = &c->asserts;
                    n = &c->nasserts;
                } else {
                    v = &c->deviations;
                    n = &c->ndeviations;
                }
                for (j = 0; j < *n; j++)
                    if (same_sel((*v)[j].sel, sel) && !strcmp((*v)[j].key, e->key))
                        break;
                if (j < *n)
                    continue;           /* a nearer file set it */
                if (add_tabkey(v, n, e, sel, &at))
                    return tc_err(err, errlen, "out of memory");
                if (v == &c->matrix && e->value.type == TT_LIST) {
                    const struct tc_keydef *def = tc_key(e->key);

                    if (tc_resolve_value(env, at.file, e->line, def ? def->pathfield : -1, f->dir,
                                         &(*v)[*n - 1].value, err, errlen))
                        return -1;
                }
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------ the hierarchy */

static int load(struct cfile *f, const char *path, unsigned depth, int own, char *err, size_t errlen)
{
    char *text;
    size_t len;
    int r;

    memset(f, 0, sizeof *f);
    f->depth = depth;
    f->own = own;
    if (!(f->dir = dir_of(path)))
        return tc_err(err, errlen, "out of memory");
    if (tc_read_file(path, &text, &len))
        return tc_err(err, errlen, "%s: cannot read: %s", path, strerror(errno));
    r = tt_parse(path, text, len, &f->doc, err, errlen);
    free(text);
    if (r)
        return -1;
    return check_file(f, err, errlen);
}

static int is_root(const struct cfile *f)
{
    for (size_t i = 0; i < f->doc.n; i++)
        if (f->doc.e[i].ntable == 0 && !strcmp(f->doc.e[i].key, "root"))
            return f->doc.e[i].value.boolean;
    return 0;
}

static int push_file(struct tc_config *c, const char *path)
{
    char **v = realloc(c->files, (c->nfiles + 1) * sizeof *v);

    if (!v)
        return -1;
    c->files = v;
    if (!(v[c->nfiles] = tc_strdup(path)))
        return -1;
    c->nfiles++;
    return 0;
}

static char *absolute(const char *path)
{
    char cwd[PATH_MAX];

    if (path[0] == '/')
        return tc_path_join("/", path);
    if (!getcwd(cwd, sizeof cwd))
        return NULL;
    return tc_path_join(cwd, path);
}

int tc_resolve(const char *source, const struct tc_env *env, struct tc_config *c,
               char *err, size_t errlen)
{
    static const char suffix[] = ".test.bas";
    struct cfile files[64];
    size_t nfiles = 0, slen = strlen(source);
    const char *base = strrchr(source, '/');
    char *abs = NULL, *d = NULL, *path = NULL, *top = NULL;
    int r = -1, rooted = 0;

    err[0] = 0;
    base = base ? base + 1 : source;
    if (slen < sizeof suffix || strcmp(source + slen - (sizeof suffix - 1), suffix)
        || strlen(base) == sizeof suffix - 1)
        return tc_err(err, errlen, "%s: a test source of the model is named ID.test.bas", source);
    if (!(c->source = tc_strdup(source)) || !(abs = absolute(source)) || !(c->dir = dir_of(abs))
        || !(c->id = malloc(strlen(base) - (sizeof suffix - 1) + 1))) {
        free(abs);
        return tc_err(err, errlen, "out of memory");
    }
    memcpy(c->id, base, strlen(base) - (sizeof suffix - 1));
    c->id[strlen(base) - (sizeof suffix - 1)] = 0;
    if (env && env->top && !(top = absolute(env->top)))
        goto oom;

    /* the test's own file */
    if (!(path = malloc(strlen(c->dir) + strlen(c->id) + 16)))
        goto oom;
    sprintf(path, "%s/%s.test.toml", c->dir, c->id);
    if (file_exists(path)) {
        if (load(&files[nfiles++], path, 0, 1, err, errlen))
            goto out;
        if (push_file(c, path))
            goto oom;
        c->has_own = 1;
    }
    free(path);
    path = NULL;

    /* the directories upwards, up to root = true */
    if (!(d = tc_strdup(c->dir)))
        goto oom;
    for (unsigned depth = 1;;) {
        char *up;

        if (!(path = tc_path_join(d, "tests.toml")))
            goto oom;
        if (file_exists(path)) {
            if (nfiles == sizeof files / sizeof files[0]) {
                tc_err(err, errlen, "%s: more than %zu configuration files above it", source,
                       sizeof files / sizeof files[0]);
                goto out;
            }
            if (load(&files[nfiles++], path, depth++, 0, err, errlen))
                goto out;
            if (push_file(c, path))
                goto oom;
            if (is_root(&files[nfiles - 1])) {
                rooted = 1;
                break;
            }
        }
        free(path);
        path = NULL;
        if (!strcmp(d, "/") || (top && !strcmp(d, top)))
            break;
        if (!(up = tc_path_join(d, "..")))
            goto oom;
        free(d);
        d = up;
    }
    if (!rooted) {
        tc_err(err, errlen, "%s: no tests.toml with root = true in its directory or above it (up to %s)",
               source, top ? top : "/");
        goto out;
    }
    if (merge(c, files, nfiles, env, err, errlen) || tc_matrix_merge(c, err, errlen))
        goto out;
    if (tc_get(c, "fixed") && tc_get(c, "fixed-orig")) {
        const struct tc_setting *s = tc_get(c, "fixed");

        tc_err(err, errlen, "%s:%u: test %s: fixed-orig and fixed exclude each other", s->at.file,
               s->at.line, c->id);
        goto out;
    }
    if (tc_assert_validate(c, err, errlen))
        goto out;
    r = 0;
    goto out;
oom:
    tc_err(err, errlen, "out of memory");
    r = -1;
out:
    for (size_t i = 0; i < nfiles; i++) {
        tt_free(&files[i].doc);
        free(files[i].dir);
    }
    free(path);
    free(d);
    free(abs);
    free(top);
    return r;
}
