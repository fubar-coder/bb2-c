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
    { "compiler", TC_V_ENUM, TC_IN_BOTH, TC_RUN, "port|orig195|orig210|ab3|amiblitz3", 0, -1,
      "the compiler: port (blitz2-c), orig195 (blitz2 1.95, the build of blitz2.s), orig210 (the CD's blitz2 2.10), "
      "ab3 (blitz2-ab3 from the command line), amiblitz3 (AmiBlitz3 v3.14.0's compiler in CLI mode)", "port" },
    { "libs", TC_V_ENUM, TC_IN_BOTH, TC_RUN, "cd|clib", 0, -1,
      "the libraries: cd (the CD's acidlibs and deflibs), clib (the C libraries)", "clib" },
    { "source-mode", TC_V_ENUM, TC_IN_BOTH, TC_RUN, "ab3|bb2", 0, -1,
      "the mode of the driver of the compilers ab3 and amiblitz3: ab3 (ID.ab3, the default) or bb2 (ID.bb, "
      "blitz2-ab3's BB2 mode)", NULL },
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
    { "isolate", TC_V_ONOFF_OR_SELECTORS, TC_IN_BOTH, TC_RUN, "off|on", 0, -1,
      "on: every cell an emulator session of its own; a list of cell selectors: the cells they match", "off" },
    { "wrap", TC_V_STRING, TC_IN_BOTH, TC_RUN, NULL, 0, 0,
      "PATH[|ARGS]: the compiler is started through PATH", NULL },
    { "runwrap", TC_V_STRING, TC_IN_BOTH, TC_RUN, NULL, 0, 0,
      "PATH[|ARGS]: the program is started through PATH", NULL },
    { "runargs", TC_V_STRING, TC_IN_BOTH, TC_RUN, NULL, 0, -1,
      "the arguments of the program", NULL },
    { "launcherargs", TC_V_STRING, TC_IN_BOTH, TC_RUN, NULL, 0, -1,
      "the options of the launcher", NULL },
    { "editor-run", TC_V_ENUM, TC_IN_BOTH, TC_RUN, "off|on", 0, -1,
      "on: the program runs from the editor (COMPILE/RUN) instead of from the shell", "off" },
    { "defaultdbug", TC_V_STRING, TC_IN_BOTH, TC_RUN, NULL, 0, 0,
      "PATH: blitz2:defaultdbug, the debugger module of a run from the editor", NULL },
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
    { "extension", TC_V_INT, TC_IN_TEST, 0, NULL, 1, -1,
      "the issue of the extension of the port the test shows (the original does something else)", NULL },
    { "flaky", TC_V_STRING, TC_IN_TEST, TC_REG, NULL, 1, -1,
      "what is known of a test that fails in full runs but passes alone: its tests get the ctest label flaky", NULL },
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
    for (size_t i = 0; i < env->nrun; i++) {
        free(env->run[i].name);
        free(env->run[i].value);
    }
    free(env->run);
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
        } else if (e->ntable == 1 && !strcmp(e->table[0], "runner")) {
            struct tc_placeholder *rv;

            if (e->value.type != TT_STRING) {
                r = tc_err(err, errlen, "%s:%u: the runner's %s must be a string", file, e->line, e->key);
                break;
            }
            rv = realloc(env->run, (env->nrun + 1) * sizeof *rv);
            if (!rv) {
                r = -2;
                break;
            }
            env->run = rv;
            rv[env->nrun].name = tc_strdup(e->key);
            rv[env->nrun].value = tc_strdup(e->value.str);
            env->nrun++;
            if (!rv[env->nrun - 1].name || !rv[env->nrun - 1].value)
                r = -2;
        } else
            r = tc_err(err, errlen, "%s:%u: unknown key %s in the environment file", file, e->line, e->key);
    }
    for (size_t i = 0; r == 0 && i < d.ntables; i++)
        if (d.tables[i].npath != 1
            || (strcmp(d.tables[i].path[0], "placeholders") && strcmp(d.tables[i].path[0], "runner")))
            r = tc_err(err, errlen, "%s:%u: unknown table in the environment file", file, d.tables[i].line);
    tt_free(&d);
    if (r == -2)
        tc_err(err, errlen, "%s: out of memory", file);
    return r ? -1 : 0;
}

const char *tc_env_runner(const struct tc_env *env, const char *key)
{
    for (size_t i = 0; env && i < env->nrun; i++)
        if (!strcmp(env->run[i].name, key))
            return env->run[i].value;
    return NULL;
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

/* tc_placeholders; keep (NULL: none) a placeholder left as it is, and with
   keep (a jobline element) every @DATA:FILE@ too (checked against the data
   sources by check_jobline); without keep @DATA:FILE@ is refused */
static int placeholders(const struct tc_env *env, const char *text, const char *keep, char **out, char *err,
                        size_t errlen)
{
    size_t cap = strlen(text) + 1, n = 0;
    char *buf = malloc(cap);
    const char *p = text;

    if (!buf)
        return -2;
    while (*p) {
        const char *q;

        if (!strncmp(p, TC_DATA_PH, sizeof TC_DATA_PH - 1) && (q = strchr(p + sizeof TC_DATA_PH - 1, '@'))
            && q > p + sizeof TC_DATA_PH - 1) {
            if (!keep) {
                snprintf(err, errlen, "%.*s", (int)(q - p + 1), p);
                free(buf);
                return -1;
            }
            /* kept as it is */
            if (n + (size_t)(q - p + 1) + 1 > cap) {
                char *b = realloc(buf, cap = n + strlen(p) + 1);

                if (!b) {
                    free(buf);
                    return -2;
                }
                buf = b;
            }
            memcpy(buf + n, p, (size_t)(q - p + 1));
            n += (size_t)(q - p + 1);
            p = q + 1;
            continue;
        }
        if (*p == '@' && (q = strchr(p + 1, '@')) && ph_name_ok(p + 1, (size_t)(q - p - 1))) {
            const char *v = NULL;
            size_t vl;

            if (keep && strlen(keep) == (size_t)(q - p + 1) && !memcmp(keep, p, (size_t)(q - p + 1)))
                v = keep;
            for (size_t i = 0; env && !v && i < env->nph; i++)
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

int tc_placeholders(const struct tc_env *env, const char *text, char **out, char *err, size_t errlen)
{
    return placeholders(env, text, NULL, out, err, errlen);
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

    if (def->vkind == TC_V_ONOFF_OR_SELECTORS) {
        /* a string on or off, or the cell selectors of exclude */
        if (v->type == TT_STRING) {
            if (!tc_enum_ok(def, v->str))
                return tc_err(err, errlen, "%s:%u: %s must be one of %s or a list of cell selectors, not \"%s\"",
                              file, line, def->name, def->values, v->str);
            return 0;
        }
        if (v->type != TT_LIST)
            return tc_err(err, errlen, "%s:%u: %s must be a string or a list of cell selectors, not a%s %s", file,
                          line, def->name, v->type == TT_INTEGER ? "n" : "", tt_type_name(v->type));
        if (!v->nlist)
            return tc_err(err, errlen, "%s:%u: %s: an empty list of cell selectors (\"off\": no cell)", file,
                          line, def->name);
        for (size_t i = 0; i < v->nlist; i++) {
            struct tc_selector sel;
            char msg[300];
            int r = tc_selector_parse(v->list[i], 0, &sel, msg, sizeof msg);

            if (r == -2)
                return tc_err(err, errlen, "%s:%u: out of memory", file, line);
            if (r)
                return tc_err(err, errlen, "%s:%u: %s: %s", file, line, def->name, msg);
            tc_selector_free(&sel);
        }
        return 0;
    }
    if (v->type != want[def->vkind])
        return tc_err(err, errlen, "%s:%u: %s must be a%s %s, not a%s %s", file, line, def->name,
                      want[def->vkind] == TT_INTEGER ? "n" : "", tt_type_name(want[def->vkind]),
                      v->type == TT_INTEGER ? "n" : "", tt_type_name(v->type));
    if (def->vkind == TC_V_ENUM && !tc_enum_ok(def, v->str))
        return tc_err(err, errlen, "%s:%u: %s must be one of %s, not \"%s\"", file, line, def->name,
                      def->values, v->str);
    if (def->vkind == TC_V_STRING && def->min > 0 && !*v->str)
        return tc_err(err, errlen, "%s:%u: %s must be a non-empty string", file, line, def->name);
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
        if (!strcmp(name, "data")) {
            if (!f->own)
                return tc_err(err, errlen, "%s:%u: data is a table of a single test, valid only in its ID.test.toml "
                              "(the data belong to one ID.test.bas)", d->file, t->line);
            if (t->npath > 2)
                return tc_err(err, errlen, "%s:%u: the table [data] has no sub tables but one per file "
                              "([data.\"FILE\"])", d->file, t->line);
            continue;           /* the entries: collect_data */
        }
        if (!strcmp(name, "ends")) {
            int reason = 0;
            size_t alts = 0;

            if (!f->own)
                return tc_err(err, errlen, "%s:%u: ends is a key of a single test, valid only in its ID.test.toml",
                              d->file, t->line);
            if (t->npath != 2)
                return tc_err(err, errlen,
                              "%s:%u: alternative ends need the cell selector of the cells with the original's "
                              "faulty part ([ends.\"SELECTOR\"])", d->file, t->line);
            for (size_t k = 0; k < d->n; k++) {
                const struct tt_entry *e = &d->e[k];

                if (e->ntable != 2 || strcmp(e->table[0], "ends") || strcmp(e->table[1], t->path[1]))
                    continue;
                if (!strcmp(e->key, "reason"))
                    reason = 1;
                else
                    alts++;
            }
            if (!reason)
                return tc_err(err, errlen, "%s:%u: [ends.\"%s\"] has no reason (why the end of these cells varies)",
                              d->file, t->line, t->path[1]);
            if (!alts)
                return tc_err(err, errlen, "%s:%u: [ends.\"%s\"] declares no alternative end", d->file, t->line,
                              t->path[1]);
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
        if (!strcmp(e->table[0], "data"))
            continue;           /* collect_data */
        if (!strcmp(e->table[0], "ends")) {
            if (!strcmp(e->key, "reason")) {
                if (e->value.type != TT_STRING || !*e->value.str)
                    return tc_err(err, errlen, "%s:%u: the reason of [ends.\"%s\"] must be a non-empty string",
                                  d->file, e->line, e->table[1]);
                continue;
            }
            if (e->value.type != TT_LIST || !e->value.nlist)
                return tc_err(err, errlen, "%s:%u: the alternative end %s must be a non-empty list of \"QUERY "
                              "PREDICATE\"%s", d->file, e->line, e->key,
                              !strcmp(e->key, "abnormal") ? " (the general abnormal end: abnormal = [\"any\"])" : "");
            continue;
        }
        if (!strcmp(e->table[0], "deviations") && !strcmp(e->key, "variant")) {
            /* the row of docs/ab3/variant.md of a difference of AmiBlitz3 */
            if (e->value.type != TT_STRING || !*e->value.str)
                return tc_err(err, errlen, "%s:%u: variant of [deviations.\"%s\"] must be a non-empty string (the "
                              "row of docs/ab3/variant.md)", d->file, e->line, e->table[1]);
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
    for (size_t i = 0; i < c->nends; i++)
        tc_tabkey_free(&c->ends[i]);
    free(c->ends);
    for (size_t i = 0; i < c->nvariants; i++)
        tc_tabkey_free(&c->variants[i]);
    free(c->variants);
    for (size_t i = 0; i < c->ndata; i++) {
        free(c->data[i].file);
        for (int k = 0; k < TC_DO_N; k++)
            free(c->data[i].opt[k]);
    }
    free(c->data);
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

/* a string of a setting or an axis: placeholders (keep, NULL: none, left
   as it is), then the path field */
static int resolve_string(const struct tc_env *env, const char *file, unsigned line, int pathfield,
                          const char *dir, const char *keep, char **s, char *err, size_t errlen)
{
    char *out, *abs;
    char ph[128];
    int r = placeholders(env, *s, keep, &out, ph, sizeof ph);

    if (r == -1 && !strcmp(ph, TC_DRIVER))
        return tc_err(err, errlen, "%s:%u: the placeholder %s stands only in jobline (the driver's source)", file,
                      line, ph);
    if (r == -1 && !strncmp(ph, TC_DATA_PH, sizeof TC_DATA_PH - 1))
        return tc_err(err, errlen, "%s:%u: the placeholder %s stands only in jobline (the staged source of a data "
                      "source)", file, line, ph);
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
                     const char *dir, const char *keep, struct tt_value *v, char *err, size_t errlen)
{
    if (v->type == TT_STRING)
        return resolve_string(env, file, line, pathfield, dir, keep, &v->str, err, errlen);
    if (v->type == TT_LIST)
        for (size_t i = 0; i < v->nlist; i++)
            if (resolve_string(env, file, line, pathfield, dir, keep, &v->list[i], err, errlen))
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
                    if (tc_resolve_value(env, at.file, e->line, def->pathfield, f->dir,
                                         !strcmp(def->name, "jobline") ? TC_DRIVER : NULL, &s->value, err, errlen))
                        return -1;
                continue;
            }
            {
                const char *tab = e->table[0];
                const char *sel = e->ntable > 1 ? e->table[1] : NULL;
                struct tc_tabkey **v;
                size_t *n, j;

                if (!strcmp(tab, "data"))
                    continue;           /* collect_data */
                if (!strcmp(tab, "matrix")) {
                    v = &c->matrix;
                    n = &c->nmatrix;
                } else if (!strcmp(tab, "asserts")) {
                    v = &c->asserts;
                    n = &c->nasserts;
                } else if (!strcmp(tab, "ends")) {
                    v = &c->ends;
                    n = &c->nends;
                } else if (!strcmp(e->key, "variant")) {
                    /* deviations."SEL": its row of docs/ab3/variant.md */
                    v = &c->variants;
                    n = &c->nvariants;
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

                    if (tc_resolve_value(env, at.file, e->line, def ? def->pathfield : -1, f->dir, NULL,
                                         &(*v)[*n - 1].value, err, errlen))
                        return -1;
                }
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------ test-local data */

static const char *const dactions[] = { "basic-compile", "copy", "mktestdata" };
static const char *const dopts[TC_DO_N] = { "compiler", "libs", "runtime-errors" };
static const char *const dvalues[TC_DO_N] = { "same|port|orig195|orig210", "same|cd|clib", "same|on|off" };

const char *tc_daction_name(int action)
{
    return action >= 0 && action < (int)(sizeof dactions / sizeof dactions[0]) ? dactions[action] : "?";
}

const char *tc_dopt_name(int opt)
{
    return opt >= 0 && opt < TC_DO_N ? dopts[opt] : "?";
}

char *tc_data_result(const struct tc_data *d)
{
    const char *suffix = d->action == TC_DA_BASIC_COMPILE ? ".bas" : d->action == TC_DA_MKTESTDATA ? ".spec" : "";
    size_t n = strlen(d->file), sl = strlen(suffix);
    char *r;

    if (sl && n > sl && !strcmp(d->file + n - sl, suffix))
        n -= sl;
    if ((r = malloc(n + 1))) {
        memcpy(r, d->file, n);
        r[n] = 0;
    }
    return r;
}

static int in_values(const char *values, const char *v)
{
    struct tc_keydef def = { 0 };

    def.values = values;
    return tc_enum_ok(&def, v);
}

static int daction_of(const char *name)
{
    for (size_t i = 0; i < sizeof dactions / sizeof dactions[0]; i++)
        if (!strcmp(dactions[i], name))
            return (int)i;
    return -1;
}

/* a path relative to ID.test/: not empty, not absolute, no ":" or "\",
   no empty, "." or ".." part; msg the reason */
static int data_path_ok(const char *p, char *msg, size_t msglen)
{
    const char *s = p;

    if (!*p)
        return snprintf(msg, msglen, "an empty path"), 0;
    if (*p == '/')
        return snprintf(msg, msglen, "an absolute path (the path is relative to the test's directory ID.test/)"), 0;
    if (strchr(p, ':'))
        return snprintf(msg, msglen, "a \":\" in the path (an Amiga path)"), 0;
    if (strchr(p, '\\'))
        return snprintf(msg, msglen, "a \"\\\" in the path"), 0;
    for (;;) {
        const char *e = strchr(s, '/');
        size_t n = e ? (size_t)(e - s) : strlen(s);

        if (n == 0)
            return snprintf(msg, msglen, "an empty part of the path"), 0;
        if ((n == 1 && s[0] == '.') || (n == 2 && s[0] == '.' && s[1] == '.'))
            return snprintf(msg, msglen, "a part \"%.*s\" of the path (the data stay in ID.test/)", (int)n, s), 0;
        if (!e)
            return 1;
        s = e + 1;
    }
}

static struct tc_data *find_data(const struct tc_config *c, const char *file)
{
    for (size_t i = 0; i < c->ndata; i++)
        if (!strcmp(c->data[i].file, file))
            return &c->data[i];
    return NULL;
}

static int add_data(struct tc_config *c, const char *file, int action, unsigned line)
{
    struct tc_data *v = realloc(c->data, (c->ndata + 1) * sizeof *v);

    if (!v)
        return -1;
    c->data = v;
    v += c->ndata;
    memset(v, 0, sizeof *v);
    v->action = action;
    v->at.file = c->files[0];
    v->at.line = line;
    if (!(v->file = tc_strdup(file)))
        return -1;
    c->ndata++;
    return 0;
}

static int by_line(const void *a, const void *b)
{
    const struct tc_data *x = a, *y = b;

    return (x->at.line > y->at.line) - (x->at.line < y->at.line);
}

/* the table data of the test's own file f (check_file let it through) */
static int collect_data(struct tc_config *c, const struct cfile *f, char *err, size_t errlen)
{
    const struct tt_doc *d = &f->doc;
    char msg[200];

    /* the tables [data."FILE"] */
    for (size_t i = 0; i < d->ntables; i++) {
        const struct tt_table *t = &d->tables[i];

        if (strcmp(t->path[0], "data") || t->npath != 2)
            continue;
        if (!data_path_ok(t->path[1], msg, sizeof msg))
            return tc_err(err, errlen, "%s:%u: data \"%s\": %s", d->file, t->line, t->path[1], msg);
        if (add_data(c, t->path[1], -1, t->line))
            return tc_err(err, errlen, "out of memory");
    }
    /* the short form and the keys of the tables */
    for (size_t i = 0; i < d->n; i++) {
        const struct tt_entry *e = &d->e[i];
        struct tc_data *dt;
        int k;

        if (e->ntable == 0 || strcmp(e->table[0], "data"))
            continue;
        if (e->ntable == 1) {
            int a;

            if (!data_path_ok(e->key, msg, sizeof msg))
                return tc_err(err, errlen, "%s:%u: data \"%s\": %s", d->file, e->line, e->key, msg);
            if (e->value.type != TT_STRING)
                return tc_err(err, errlen, "%s:%u: data \"%s\": the action must be a string, not a%s %s", d->file,
                              e->line, e->key, e->value.type == TT_INTEGER ? "n" : "", tt_type_name(e->value.type));
            if ((a = daction_of(e->value.str)) < 0)
                return tc_err(err, errlen, "%s:%u: data \"%s\": unknown action \"%s\" (basic-compile, copy, "
                              "mktestdata)", d->file, e->line, e->key, e->value.str);
            if (add_data(c, e->key, a, e->line))
                return tc_err(err, errlen, "out of memory");
            continue;
        }
        dt = find_data(c, e->table[1]);
        if (!dt)
            continue;           /* not reached: every table was added */
        if (e->value.type != TT_STRING)
            return tc_err(err, errlen, "%s:%u: data \"%s\": %s must be a string, not a%s %s", d->file, e->line,
                          dt->file, e->key, e->value.type == TT_INTEGER ? "n" : "", tt_type_name(e->value.type));
        if (!strcmp(e->key, "action")) {
            if ((dt->action = daction_of(e->value.str)) < 0)
                return tc_err(err, errlen, "%s:%u: data \"%s\": unknown action \"%s\" (basic-compile, copy, "
                              "mktestdata)", d->file, e->line, dt->file, e->value.str);
            continue;
        }
        for (k = 0; k < TC_DO_N && strcmp(dopts[k], e->key); k++)
            ;
        if (k == TC_DO_N)
            return tc_err(err, errlen, "%s:%u: data \"%s\": unknown option %s (action, compiler, libs, "
                          "runtime-errors)", d->file, e->line, dt->file, e->key);
        if (!in_values(dvalues[k], e->value.str))
            return tc_err(err, errlen, "%s:%u: data \"%s\": %s must be one of %s, not \"%s\"", d->file, e->line,
                          dt->file, e->key, dvalues[k], e->value.str);
        free(dt->opt[k]);
        if (!(dt->opt[k] = tc_strdup(e->value.str)))
            return tc_err(err, errlen, "out of memory");
        dt->optline[k] = e->line;
    }
    /* every table has its action, and only basic-compile takes options */
    for (size_t i = 0; i < c->ndata; i++) {
        struct tc_data *dt = &c->data[i];

        if (dt->action < 0)
            return tc_err(err, errlen, "%s:%u: the table [data.\"%s\"] has no action (basic-compile, copy, "
                          "mktestdata)", d->file, dt->at.line, dt->file);
        for (int k = 0; k < TC_DO_N; k++) {
            if (dt->action != TC_DA_BASIC_COMPILE) {
                if (dt->opt[k])
                    return tc_err(err, errlen, "%s:%u: data \"%s\": the action %s takes no option %s", d->file,
                                  dt->optline[k], dt->file, dactions[dt->action], dopts[k]);
                continue;
            }
            if (!dt->opt[k] && !(dt->opt[k] = tc_strdup("same")))
                return tc_err(err, errlen, "out of memory");
        }
    }
    if (c->ndata)
        qsort(c->data, c->ndata, sizeof *c->data, by_line);
    return 0;
}

/* ------------------------------------------------------------ the compile jobs */

int tc_jobline_job(const char *elem, const char **id, size_t *idlen)
{
    static const struct {
        const char *prefix;
        int kind;
    } kinds[] = {
        { "job=", TC_JOB_JOB }, { "run=", TC_JOB_RUN }, { "menu=", TC_JOB_MENU }, { "resident=", TC_JOB_RESIDENT },
    };

    for (size_t k = 0; k < sizeof kinds / sizeof kinds[0]; k++) {
        size_t n = strlen(kinds[k].prefix);

        if (!strncmp(elem, kinds[k].prefix, n)) {
            *id = elem + n;
            *idlen = strcspn(*id, "|");
            return kinds[k].kind;
        }
    }
    *id = NULL;
    *idlen = 0;
    return TC_JOB_NONE;
}

/* a data source of basic-compile named name (len characters) */
static int is_data_source(const struct tc_config *c, const char *name, size_t len)
{
    for (size_t i = 0; i < c->ndata; i++)
        if (c->data[i].action == TC_DA_BASIC_COMPILE && strlen(c->data[i].file) == len
            && !memcmp(c->data[i].file, name, len))
            return 1;
    return 0;
}

/* a cell of the test may run from the editor */
static int editor_run_possible(const struct tc_config *c)
{
    const struct tc_setting *s = tc_get(c, "editor-run");

    for (size_t i = 0; i < c->nmatrix; i++)
        if (!strcmp(c->matrix[i].key, "editor-run") && c->matrix[i].value.type == TT_LIST)
            for (size_t k = 0; k < c->matrix[i].value.nlist; k++)
                if (!strcmp(c->matrix[i].value.list[k], "on"))
                    return 1;
    return s && s->value.type == TT_STRING && !strcmp(s->value.str, "on");
}

int tc_job_of(const struct tc_config *c, const char *name)
{
    const struct tc_setting *jl = tc_get(c, "jobline");

    for (size_t i = 0; jl && jl->value.type == TT_LIST && i < jl->value.nlist; i++) {
        const char *id;
        size_t n;
        int kind = tc_jobline_job(jl->value.list[i], &id, &n);

        if (kind != TC_JOB_NONE && strlen(name) == n && !memcmp(name, id, n))
            return kind;
    }
    if (!strcmp(name, "j000"))
        return TC_JOB_DRIVER;
    if (is_data_source(c, name, strlen(name)))
        return TC_JOB_DATA;
    if (!strcmp(name, "r001") && editor_run_possible(c))
        return TC_JOB_EDITOR_RUN;
    return TC_JOB_NONE;
}

/* the IDs of the jobline jobs and the data sources the placeholders
   @DATA:FILE@ name */
static int check_jobline(const struct tc_config *c, char *err, size_t errlen)
{
    const struct tc_setting *jl = tc_get(c, "jobline");

    if (!jl || jl->value.type != TT_LIST)
        return 0;
    for (size_t i = 0; i < jl->value.nlist; i++) {
        const char *e = jl->value.list[i], *id, *p;
        size_t n, k;
        int bad = 0;

        if (tc_jobline_job(e, &id, &n) != TC_JOB_NONE) {
            if (!n || n > TC_JOBID_MAX || !(id[0] >= 'a' && id[0] <= 'z'))
                bad = 1;
            for (k = 1; k < n && !bad; k++)
                if (!((id[k] >= 'a' && id[k] <= 'z') || (id[k] >= '0' && id[k] <= '9') || id[k] == '_'))
                    bad = 1;
            if (bad)
                return tc_err(err, errlen, "%s:%u: jobline: the job ID \"%.*s\" of \"%s\": [a-z][a-z0-9_]*, at most "
                              "%d characters", jl->at.file, jl->at.line, (int)n, id, e, TC_JOBID_MAX);
            if ((n == 4 && !memcmp(id, "j000", 4))
                || (n == 4 && id[0] == 'b' && id[1] >= '0' && id[1] <= '9' && id[2] >= '0' && id[2] <= '9'
                    && id[3] >= '0' && id[3] <= '9'))
                return tc_err(err, errlen, "%s:%u: jobline: the job ID %.*s is the runner's (j000 the driver, bNNN "
                              "a data source)", jl->at.file, jl->at.line, (int)n, id);
            if ((n == 3 && !memcmp(id, "log", 3)) || (n == 6 && !memcmp(id, "inject", 6)))
                return tc_err(err, errlen, "%s:%u: jobline: the job ID %.*s is a query (compile.%.*s)", jl->at.file,
                              jl->at.line, (int)n, id, (int)n, id);
            for (k = 0; k < i; k++) {
                const char *id2;
                size_t n2;

                if (tc_jobline_job(jl->value.list[k], &id2, &n2) != TC_JOB_NONE && n2 == n && !memcmp(id, id2, n))
                    return tc_err(err, errlen, "%s:%u: jobline: the job ID %.*s twice", jl->at.file, jl->at.line,
                                  (int)n, id);
            }
        }
        /* the data sources of the placeholders */
        for (p = e; (p = strstr(p, TC_DATA_PH));) {
            const char *f = p + sizeof TC_DATA_PH - 1, *q = strchr(f, '@');

            if (!q || q == f) {
                p = f;
                continue;
            }
            if (!is_data_source(c, f, (size_t)(q - f)))
                return tc_err(err, errlen, "%s:%u: jobline: the placeholder %.*s names no data source of the test "
                              "(the action basic-compile in its table data)", jl->at.file, jl->at.line,
                              (int)(q - p + 1), p);
            p = q + 1;
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
    if (c->has_own && collect_data(c, &files[0], err, errlen))
        goto out;
    if (check_jobline(c, err, errlen))
        goto out;
    {
        /* at most one of the keys of the fixed issue */
        static const char *const fixkeys[] = { "fixed-orig", "fixed", "extension" };
        const struct tc_setting *first = NULL, *s;
        const char *fk = NULL;
        int both = 0;

        for (size_t k = 0; k < sizeof fixkeys / sizeof fixkeys[0] && !both; k++) {
            if (!(s = tc_get(c, fixkeys[k])))
                continue;
            if (first) {
                tc_err(err, errlen, "%s:%u: test %s: %s and %s exclude each other", s->at.file, s->at.line,
                       c->id, fk, fixkeys[k]);
                both = 1;
            }
            first = s;
            fk = fixkeys[k];
        }
        if (both)
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
