/*
 * covmap.c - the coverage of each test as lcov tracefiles (covmap.h).
 */
#include "covmap.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "covfmt.h"
#include "json.h"

static int error(char *err, size_t errsz, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(err, errsz, fmt, ap);
    va_end(ap);
    return -1;
}

#define OOM() error(err, errsz, "out of memory")

void covmap_init(struct covmap *m, const char *root)
{
    memset(m, 0, sizeof *m);
    m->root = root ? strdup(root) : NULL;
}

void covmap_free(struct covmap *m)
{
    size_t i;

    for (i = 0; i < m->ndrops; i++)
        free(m->drops[i]);
    free(m->drops);
    free(m->funcs);
    free(m->hits);
    free(m->lines);
    free(m->branches);
    free(m->order);
    free(m->fslot);
    for (i = 0; i < m->nprogs; i++) {
        free(m->progs[i].objs);
        free(m->progs[i].raw);
        free(m->progs[i].syms);
    }
    free(m->progs);
    strtab_free(&m->strs);
    strtab_free(&m->tests);
    cov_deps_free(&m->deps);
    free(m->root);
    memset(m, 0, sizeof *m);
}

int covmap_drop(struct covmap *m, const char *prefix)
{
    char *p = cov_relpath(prefix, m->root), **v;

    if (!p)
        return 0;       /* outside the root: left out anyway */
    if (!(v = realloc(m->drops, (m->ndrops + 1) * sizeof *v))) {
        free(p);
        return -1;
    }
    m->drops = v;
    v[m->ndrops++] = p;
    return 0;
}

/* the id of path relative to the root (dir for a relative path), -1 if
   left out */
static int file_id(struct covmap *m, const char *dir, const char *path)
{
    char *full = cov_join(dir, path), *rel;
    size_t i, n;
    int id;

    if (!full)
        return -1;
    rel = cov_relpath(full, m->root);
    free(full);
    if (!rel)
        return -1;
    for (i = 0; i < m->ndrops; i++) {
        n = strlen(m->drops[i]);
        if (!n || (!strncmp(rel, m->drops[i], n) && (rel[n] == '/' || !rel[n]))) {
            free(rel);
            return -1;
        }
    }
    id = strtab_add(&m->strs, rel);
    free(rel);
    return id;
}

static size_t fhash(int file, int name)
{
    size_t h = (size_t)file * 2654435761u;

    h = (h ^ (size_t)name) * 3266489917u;
    return h ^ (h >> 15);
}

static int frehash(struct covmap *m)
{
    size_t n = m->nfslot ? 2 * m->nfslot : 1024, i, j;
    int *slot = malloc(n * sizeof *slot);

    if (!slot)
        return 0;
    for (i = 0; i < n; i++)
        slot[i] = -1;
    for (i = 0; i < m->nfuncs; i++) {
        const struct cov_func *f = &m->funcs[i];

        j = fhash(f->file, f->name) & (n - 1);
        while (slot[j] >= 0)
            j = (j + 1) & (n - 1);
        slot[j] = (int)i;
    }
    free(m->fslot);
    m->fslot = slot;
    m->nfslot = n;
    return 1;
}

/* the function (file, name), added if new; -1 if out of memory */
static int get_func(struct covmap *m, int file, int name)
{
    size_t j;
    struct cov_func *f;

    if (2 * (m->nfuncs + 1) > m->nfslot && !frehash(m))
        return -1;
    j = fhash(file, name) & (m->nfslot - 1);
    while (m->fslot[j] >= 0) {
        f = &m->funcs[m->fslot[j]];
        if (f->file == file && f->name == name)
            return m->fslot[j];
        j = (j + 1) & (m->nfslot - 1);
    }
    if (m->nfuncs == m->cap) {
        size_t cap = m->cap ? 2 * m->cap : 1024;
        struct cov_func *v = realloc(m->funcs, cap * sizeof *v);

        if (!v)
            return -1;
        m->funcs = v;
        m->cap = cap;
    }
    f = &m->funcs[m->nfuncs];
    memset(f, 0, sizeof *f);
    f->file = file;
    f->name = name;
    m->fslot[j] = (int)m->nfuncs;
    return (int)m->nfuncs++;
}

/* room for one more element at the end of *pv (n elements of size bytes,
   room for cap); NULL if out of memory */
static void *push(void *pv, size_t *n, size_t *cap, size_t size)
{
    void **v = pv;

    if (*n == *cap) {
        size_t c = *cap ? 2 * *cap : 1024;
        void *x = realloc(*v, c * size);

        if (!x)
            return NULL;
        *v = x;
        *cap = c;
    }
    return (char *)*v + (*n)++ * size;
}

static int add_hit(struct covmap *m, int test, int func, unsigned long long count, int bitmap)
{
    struct cov_hit *h = push(&m->hits, &m->nhits, &m->caphits, sizeof *h);

    if (!h)
        return -1;
    h->test = test;
    h->func = func;
    h->count = count;
    h->bitmap = bitmap;
    m->done = 0;
    return 0;
}

static void widen(struct cov_func *f, int first, int last)
{
    if (first <= 0 || last < first)
        return;
    if (!f->first || first < f->first)
        f->first = first;
    if (last > f->last)
        f->last = last;
}

int covmap_test_name_ok(const char *test)
{
    const char *c = test;

    if (!*test || *test == '/' || !strcmp(test, "all") || strpbrk(test, ",\t\n\r"))
        return 0;
    for (;;) {
        size_t n = strcspn(c, "/");

        if (!n || (n == 1 && c[0] == '.') || (n == 2 && c[0] == '.' && c[1] == '.'))
            return 0;
        if (!c[n])
            return 1;
        c += n + 1;
    }
}

/* the id of a test; -1 with a message if its name is not usable */
static int test_id(struct covmap *m, const char *test, char *err, size_t errsz)
{
    int t;

    if (!covmap_test_name_ok(test))
        return error(err, errsz, "\"%s\" is no usable test name", test);
    if ((t = strtab_add(&m->tests, test)) < 0)
        return OOM();
    return t;
}

int covmap_add_deps(struct covmap *m, const char *text, char *err, size_t errsz)
{
    return cov_deps_parse(&m->deps, text, err, errsz);
}

/* ---- host: gcov JSON ---- */

static unsigned long long count_of(const struct json *j)
{
    double d = j && j->type == JSON_NUMBER ? j->number : 0;

    return d > 0 ? (unsigned long long)d : 0;
}

/* the lines and branches of a file of the gcov JSON */
static int gcov_lines(struct covmap *m, int test, int file, const struct json *lines,
                      char *err, size_t errsz)
{
    size_t k, b, maxline = 0;
    int *seen;

    if (!lines)
        return 0;
    if (lines->type != JSON_ARRAY)
        return error(err, errsz, "not gcov JSON (\"lines\" is no array)");
    for (k = 0; k < lines->n; k++) {
        double ln = json_get_number(&lines->items[k], "line_number", 0);

        if (ln < 1 || ln > 1e8)
            return error(err, errsz, "not gcov JSON (a line without \"line_number\")");
        if ((size_t)ln > maxline)
            maxline = (size_t)ln;
    }
    m->done = 0;
    /* by line: its entries with branches so far (the block of the next) */
    if (!(seen = calloc(maxline + 1, sizeof *seen)))
        return OOM();
    for (k = 0; k < lines->n; k++) {
        const struct json *e = &lines->items[k], *br = json_get(e, "branches");
        int ln = (int)json_get_number(e, "line_number", 0);
        unsigned long long count = count_of(json_get(e, "count"));
        struct cov_line *l = push(&m->lines, &m->nlines, &m->caplines, sizeof *l);

        if (!l) {
            free(seen);
            return OOM();
        }
        l->test = test;
        l->file = file;
        l->line = ln;
        l->count = count;
        if (!br || br->type != JSON_ARRAY || !br->n)
            continue;
        for (b = 0; b < br->n; b++) {
            struct cov_branch *x = push(&m->branches, &m->nbranches, &m->capbranches, sizeof *x);

            if (!x) {
                free(seen);
                return OOM();
            }
            x->test = test;
            x->file = file;
            x->line = ln;
            x->block = seen[ln];
            x->branch = (int)b;
            x->taken = count ? (long long)count_of(json_get(&br->items[b], "count")) : -1;
        }
        seen[ln]++;
    }
    free(seen);
    return 0;
}

static int gcov_doc(struct covmap *m, int test, const struct json *doc, char *err, size_t errsz)
{
    const struct json *files = json_get(doc, "files");
    const char *cwd = json_get_string(doc, "current_working_directory");
    size_t i, k;

    if (!files || files->type != JSON_ARRAY)
        return error(err, errsz, "not gcov JSON (no \"files\")");
    for (i = 0; i < files->n; i++) {
        const struct json *fe = &files->items[i], *funcs = json_get(fe, "functions");
        const char *path = json_get_string(fe, "file");
        int file;

        if (!path || !funcs || funcs->type != JSON_ARRAY)
            return error(err, errsz, "not gcov JSON (a file without \"file\" or \"functions\")");
        if ((file = file_id(m, cwd, path)) < 0)
            continue;
        for (k = 0; k < funcs->n; k++) {
            const struct json *fn = &funcs->items[k];
            const char *name = json_get_string(fn, "name");
            int id, f;

            if (!name)
                return error(err, errsz, "not gcov JSON (a function without \"name\")");
            if ((id = strtab_add(&m->strs, name)) < 0 || (f = get_func(m, file, id)) < 0)
                return OOM();
            widen(&m->funcs[f], (int)json_get_number(fn, "start_line", 0),
                  (int)json_get_number(fn, "end_line", 0));
            if (add_hit(m, test, f, count_of(json_get(fn, "execution_count")), 0))
                return OOM();
        }
        if (gcov_lines(m, test, file, json_get(fe, "lines"), err, errsz))
            return -1;
    }
    return 0;
}

int covmap_add_gcov(struct covmap *m, const char *test, const char *text, size_t len,
                    char *err, size_t errsz)
{
    size_t pos = 0;
    int t = test_id(m, test, err, errsz);
    char jerr[200];
    struct json *doc;

    if (t < 0)
        return -1;
    while ((doc = json_parse(text, len, &pos, jerr, sizeof jerr))) {
        int r = gcov_doc(m, t, doc, err, errsz);

        json_free(doc);
        if (r)
            return r;
    }
    if (jerr[0])
        return error(err, errsz, "gcov JSON: %s", jerr);
    return 0;
}

/* ---- Amiga: link map, nm, objdump, coverage files ---- */

static struct cov_prog *find_prog(struct covmap *m, const char *name)
{
    int id = strtab_find(&m->strs, name);
    size_t i;

    for (i = 0; id >= 0 && i < m->nprogs; i++)
        if (m->progs[i].name == id)
            return &m->progs[i];
    return NULL;
}

static int add_raw(struct cov_prog *p, uint32_t addr, size_t obj, int name, int from_nm)
{
    struct cov_rawsym *v;

    if (p->nraw == p->capraw) {
        size_t cap = p->capraw ? 2 * p->capraw : 64;

        if (!(v = realloc(p->raw, cap * sizeof *v)))
            return -1;
        p->raw = v;
        p->capraw = cap;
    }
    p->raw[p->nraw].addr = addr;
    p->raw[p->nraw].obj = obj;
    p->raw[p->nraw].name = name;
    p->raw[p->nraw].from_nm = from_nm;
    p->nraw++;
    return 0;
}

/* the source id of an object path of the build, -1 if left out */
static int object_file(struct covmap *m, const char *object)
{
    const struct cov_dep *d = cov_deps_find(&m->deps, object);
    const char *src = d ? cov_dep_source(d) : NULL;
    char *derived;
    int id;

    if (src)
        return file_id(m, NULL, src);
    if (object[0] == '/' || !(derived = cov_object_source(object)))
        return -1;
    id = file_id(m, NULL, derived);
    free(derived);
    return id;
}

static int parse_hex(const char **s, uint32_t *v)
{
    const char *p = *s;
    char *end;
    unsigned long x;

    while (*p == ' ' || *p == '\t')
        p++;
    if (p[0] != '0' || (p[1] != 'x' && p[1] != 'X'))
        return 0;
    x = strtoul(p + 2, &end, 16);
    if (end == p + 2)
        return 0;
    *v = (uint32_t)x;
    *s = end;
    return 1;
}

static const char *skip_space(const char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    return s;
}

int covmap_add_linkmap(struct covmap *m, const char *program, const char *text,
                       char *err, size_t errsz)
{
    size_t pos = 0, len = strlen(text), n, lineno = 0;
    const char *line;
    struct cov_prog *p;
    int in_text = 0, have_text = 0, name, wrapped = 0;
    long cur = -1;                  /* the object of the current input section */
    uint32_t start = 0;
    char buf[4096], secname[256];

    if (find_prog(m, program))
        return error(err, errsz, "%s: a second link map", program);
    if ((name = strtab_add(&m->strs, program)) < 0)
        return OOM();
    if (!(p = realloc(m->progs, (m->nprogs + 1) * sizeof *p)))
        return OOM();
    m->progs = p;
    p = &m->progs[m->nprogs++];
    memset(p, 0, sizeof *p);
    p->name = name;
    while ((line = cov_next_line(text, len, &pos, &n))) {
        const char *s;
        uint32_t addr, size;

        lineno++;
        if (n >= sizeof buf)
            n = sizeof buf - 1;
        memcpy(buf, line, n);
        buf[n] = 0;
        s = buf;
        if (!in_text) {
            if (!have_text && !strncmp(buf, ".text", 5) && (buf[5] == ' ' || buf[5] == '\t')) {
                s = buf + 5;
                if (!parse_hex(&s, &start) || !parse_hex(&s, &size))
                    return error(err, errsz, "link map line %zu: .text without address and size",
                                 lineno);
                p->text_size = size;
                in_text = have_text = 1;
            }
            continue;
        }
        if (n && buf[0] != ' ' && buf[0] != '\t') {
            in_text = 0;            /* the next output section */
            continue;
        }
        if (wrapped || (buf[0] == ' ' && buf[1] == '.')) {
            /* an input section: " .text  0xADDR  0xSIZE  OBJECT", the name
               maybe on a line of its own */
            if (!wrapped) {
                size_t k = 0;

                s = buf + 1;
                while (*s && *s != ' ' && *s != '\t' && k < sizeof secname - 1)
                    secname[k++] = *s++;
                secname[k] = 0;
                if (!*skip_space(s)) {
                    wrapped = 1;
                    continue;
                }
            }
            wrapped = 0;
            cur = -1;
            if (!parse_hex(&s, &addr) || !parse_hex(&s, &size))
                continue;
            s = skip_space(s);
            if (size && *s && (!strcmp(secname, ".text") || !strncmp(secname, ".text.", 6))) {
                struct cov_obj *o = realloc(p->objs, (p->nobjs + 1) * sizeof *o);
                int path;

                if (!o || (path = strtab_add(&m->strs, s)) < 0)
                    return OOM();
                p->objs = o;
                o = &p->objs[p->nobjs];
                o->base = addr - start;
                o->size = size;
                o->path = path;
                o->file = object_file(m, s);
                cur = (long)p->nobjs++;
            }
            continue;
        }
        /* a symbol: "                0xADDR                NAME" */
        if (cur >= 0 && parse_hex(&s, &addr)) {
            const char *nm = skip_space(s);
            int id;

            if (!*nm || strchr(nm, '=') || strchr(nm, ' ') || strchr(nm, '('))
                continue;
            if ((id = strtab_add(&m->strs, nm)) < 0 || add_raw(p, addr - start, (size_t)cur, id, 0))
                return OOM();
            continue;
        }
        if (buf[0] == ' ' && buf[1] == '*')
            cur = -1;           /* " *(.text)", " *fill*": no object */
    }
    if (!have_text)
        return error(err, errsz, "%s: no .text section in the link map", program);
    return 0;
}

static long find_obj(struct covmap *m, const struct cov_prog *p, const char *path, size_t n)
{
    int id = strtab_addn(&m->strs, path, n);
    size_t i;

    for (i = 0; id >= 0 && i < p->nobjs; i++)
        if (p->objs[i].path == id)
            return (long)i;
    return -1;
}

int covmap_add_objsyms(struct covmap *m, const char *program, const char *text,
                       char *err, size_t errsz)
{
    struct cov_prog *p = find_prog(m, program);
    size_t pos = 0, len = strlen(text), n, lineno = 0;
    const char *line;

    if (!p)
        return error(err, errsz, "%s: symbols before the link map", program);
    if (p->ready)
        return error(err, errsz, "%s: symbols after the coverage", program);
    while ((line = cov_next_line(text, len, &pos, &n))) {
        /* "OBJECT:XXXXXXXX T NAME" or "OBJECT:         U NAME" */
        const char *colon = NULL;
        size_t i, k, namelen;
        long obj;
        uint32_t off = 0;
        char type;
        int id;

        lineno++;
        if (!n)
            continue;
        for (i = n; i-- > 0;)
            if (line[i] == ':' && n - i > 11) {
                for (k = 1; k <= 8; k++)
                    if (!isxdigit((unsigned char)line[i + k]) && line[i + k] != ' ')
                        break;
                if (k == 9 && line[i + 9] == ' ' && line[i + 11] == ' ') {
                    colon = line + i;
                    break;
                }
            }
        if (!colon)
            return error(err, errsz, "%s: symbols line %zu is no line of nm -A", program, lineno);
        type = colon[10];
        if (type != 'T' && type != 't')
            continue;
        for (k = 1; k <= 8; k++)
            off = off << 4 | (uint32_t)(isdigit((unsigned char)colon[k]) ? colon[k] - '0'
                                        : (tolower((unsigned char)colon[k]) - 'a' + 10));
        if ((obj = find_obj(m, p, line, (size_t)(colon - line))) < 0)
            continue;           /* an object without code in this program */
        if (off >= p->objs[obj].size)
            return error(err, errsz, "%s: symbols line %zu: offset 0x%x outside the object's .text",
                         program, lineno, (unsigned)off);
        namelen = n - (size_t)(colon + 12 - line);
        if (colon[12] == '_')
            namelen--;
        if ((id = strtab_addn(&m->strs, colon + 12 + (colon[12] == '_'), namelen)) < 0
            || add_raw(p, p->objs[obj].base + off, (size_t)obj, id, 1))
            return OOM();
        p->have_nm = 1;
    }
    return 0;
}

static int cmp_raw(const void *a, const void *b)
{
    const struct cov_rawsym *x = a, *y = b;

    if (x->addr != y->addr)
        return x->addr < y->addr ? -1 : 1;
    return x->name - y->name;
}

/* the program's functions from its symbols (once) */
static int prog_ready(struct covmap *m, struct cov_prog *p)
{
    char *has_nm;
    size_t i;

    if (p->ready)
        return 0;
    if (!(has_nm = calloc(p->nobjs + 1, 1)))
        return -1;
    for (i = 0; i < p->nraw; i++)
        if (p->raw[i].from_nm)
            has_nm[p->raw[i].obj] = 1;
    if (p->nraw)
        qsort(p->raw, p->nraw, sizeof *p->raw, cmp_raw);
    if (!(p->syms = malloc((p->nraw + 1) * sizeof *p->syms))) {
        free(has_nm);
        return -1;
    }
    for (i = 0; i < p->nraw; i++) {
        const struct cov_rawsym *r = &p->raw[i];
        const struct cov_obj *o = &p->objs[r->obj];
        int f;

        if (o->file < 0 || r->from_nm != has_nm[r->obj])
            continue;
        if (p->nsyms && p->syms[p->nsyms - 1].addr == r->addr)
            continue;           /* a second name of the same function */
        if ((f = get_func(m, o->file, r->name)) < 0) {
            free(has_nm);
            return -1;
        }
        p->syms[p->nsyms].addr = r->addr;
        p->syms[p->nsyms].end = o->base + o->size;
        p->syms[p->nsyms].func = f;
        p->syms[p->nsyms].obj = r->obj;
        if (p->nsyms && p->syms[p->nsyms - 1].end > r->addr)
            p->syms[p->nsyms - 1].end = r->addr;
        p->nsyms++;
    }
    free(has_nm);
    p->ready = 1;
    return 0;
}

/* the symbol whose function holds addr, NULL if none */
static const struct cov_sym *find_sym(const struct cov_prog *p, uint32_t addr)
{
    size_t lo = 0, hi = p->nsyms;

    while (lo < hi) {
        size_t mid = (lo + hi) / 2;

        if (p->syms[mid].addr <= addr)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (!lo || addr >= p->syms[lo - 1].end)
        return NULL;
    return &p->syms[lo - 1];
}

int covmap_add_lines(struct covmap *m, const char *program, const char *text,
                     char *err, size_t errsz)
{
    struct cov_prog *p = find_prog(m, program);
    size_t pos = 0, len = strlen(text), n;
    const char *line;
    long obj = -1;
    int match = 0, headers = 0;
    char buf[4096];

    if (!p)
        return error(err, errsz, "%s: lines before the link map", program);
    if (prog_ready(m, p))
        return OOM();
    while ((line = cov_next_line(text, len, &pos, &n))) {
        const char *ff;
        char *s, *end;
        long ln;
        unsigned long addr;
        const struct cov_sym *sym;

        if (n >= sizeof buf)
            n = sizeof buf - 1;
        memcpy(buf, line, n);
        buf[n] = 0;
        if (!n)
            continue;
        if ((ff = strstr(buf, ":     file format "))) {
            /* "OBJECT:     file format amiga" */
            obj = find_obj(m, p, buf, (size_t)(ff - buf));
            match = 0;
            headers++;
            continue;
        }
        if (obj < 0)
            continue;
        if (buf[0] != ' ' && buf[n - 1] == ':' && strncmp(buf, "Contents of", 11)) {
            /* the file of the rows below: "PATH:" or "CU: PATH:" */
            const char *path = !strncmp(buf, "CU: ", 4) ? buf + 4 : buf;
            const char *src;
            size_t sl, pl;

            buf[n - 1] = 0;
            if (p->objs[obj].file < 0) {
                match = 0;
                continue;
            }
            src = m->strs.str[p->objs[obj].file];
            if (path[0] == '/') {
                char *rel = cov_relpath(path, m->root);

                match = rel && !strcmp(rel, src);
                free(rel);
            } else {
                sl = strlen(src);
                pl = strlen(path);
                match = !strcmp(src, path)
                        || (sl > pl && src[sl - pl - 1] == '/' && !strcmp(src + sl - pl, path));
            }
            continue;
        }
        if (!match)
            continue;
        /* a row: "NAME  LINE  ADDRESS  [VIEW]  [x]" */
        s = buf;
        while (*s && *s != ' ')
            s++;
        ln = strtol(s, &end, 10);
        if (end == s || ln <= 0 || (*end != ' ' && *end != '\t'))
            continue;
        addr = strtoul(end, &s, 16);
        if (s == end)
            continue;
        if ((sym = find_sym(p, p->objs[obj].base + (uint32_t)addr)) && sym->obj == (size_t)obj)
            widen(&m->funcs[sym->func], (int)ln, (int)ln);
    }
    if (!headers && len)
        return error(err, errsz, "%s: lines without an object header (not objdump --dwarf=decodedline)",
                     program);
    return 0;
}

static uint32_t be32(const unsigned char *b)
{
    return (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3];
}

int covmap_add_cov(struct covmap *m, const char *test, const char *program,
                   const unsigned char *data, size_t len, char *err, size_t errsz)
{
    struct cov_prog *p = find_prog(m, program);
    uint32_t code, bytes, i;
    char *entered;
    int t;

    if (!p)
        return error(err, errsz, "%s: no link map for the coverage of %s", program, test);
    if (len < COVFMT_HEADER_SIZE || be32(data) != COVFMT_MAGIC)
        return error(err, errsz, "%s: %s: not a coverage file", test, program);
    if (be32(data + 4) != COVFMT_VERSION)
        return error(err, errsz, "%s: %s: coverage file version %u, not %u", test, program,
                     (unsigned)be32(data + 4), COVFMT_VERSION);
    code = be32(data + 8);
    bytes = be32(data + 12);
    if (code != p->text_size)
        return error(err, errsz,
                     "%s: %s: the coverage is of a code hunk of %u bytes, the link map has %u",
                     test, program, (unsigned)code, (unsigned)p->text_size);
    if (bytes < COVFMT_BITMAP_LEN(code) || len - COVFMT_HEADER_SIZE != bytes)
        return error(err, errsz, "%s: %s: bitmap of %zu bytes, the header says %u, %u needed",
                     test, program, len - COVFMT_HEADER_SIZE, (unsigned)bytes,
                     (unsigned)COVFMT_BITMAP_LEN(code));
    data += COVFMT_HEADER_SIZE;
    for (i = (code + 1) / 2; i < 8 * bytes; i++)
        if (data[i >> 3] & 1 << (i & 7))
            return error(err, errsz, "%s: %s: a bit beyond the code hunk (offset 0x%x)", test,
                         program, (unsigned)(2 * i));
    if ((t = test_id(m, test, err, errsz)) < 0)
        return -1;
    if (prog_ready(m, p) || !(entered = calloc(p->nsyms + 1, 1)))
        return OOM();
    for (i = 0; i < (code + 1) / 2; i++) {
        const struct cov_sym *sym;

        if ((data[i >> 3] & 1 << (i & 7)) && (sym = find_sym(p, 2 * i)))
            entered[sym - p->syms] = 1;
    }
    /* every function of the program, entered or not */
    for (i = 0; i < p->nsyms; i++)
        if (add_hit(m, t, p->syms[i].func, entered[i], 1)) {
            free(entered);
            return OOM();
        }
    free(entered);
    return 0;
}

/* ---- the tracefiles ---- */

static const struct covmap *sort_map;

static int cmp_strid(int a, int b)
{
    return a == b ? 0 : strcmp(sort_map->strs.str[a], sort_map->strs.str[b]);
}

static int cmp_int(int a, int b)
{
    return a < b ? -1 : a > b;
}

/* by test, file, first line, name */
static int cmp_hit(const void *a, const void *b)
{
    const struct cov_hit *x = a, *y = b;
    const struct cov_func *f = &sort_map->funcs[x->func], *g = &sort_map->funcs[y->func];
    int c;

    if ((c = cmp_int(x->test, y->test)) || (c = cmp_strid(f->file, g->file))
        || (c = cmp_int(f->first, g->first)))
        return c;
    return cmp_strid(f->name, g->name);
}

/* by test, file, line */
static int cmp_line(const void *a, const void *b)
{
    const struct cov_line *x = a, *y = b;
    int c;

    if ((c = cmp_int(x->test, y->test)) || (c = cmp_strid(x->file, y->file)))
        return c;
    return cmp_int(x->line, y->line);
}

/* by test, file, line, block, branch */
static int cmp_branch(const void *a, const void *b)
{
    const struct cov_branch *x = a, *y = b;
    int c;

    if ((c = cmp_int(x->test, y->test)) || (c = cmp_strid(x->file, y->file))
        || (c = cmp_int(x->line, y->line)) || (c = cmp_int(x->block, y->block)))
        return c;
    return cmp_int(x->branch, y->branch);
}

static int cmp_test(const void *a, const void *b)
{
    return strcmp(sort_map->tests.str[*(const int *)a], sort_map->tests.str[*(const int *)b]);
}

/* sort the hits, lines and branches, and merge those of the same test
   and function, line or branch */
static int prepare(struct covmap *m)
{
    size_t i, n;

    if (m->done)
        return 0;
    sort_map = m;
    if (m->nhits)
        qsort(m->hits, m->nhits, sizeof *m->hits, cmp_hit);
    for (i = n = 0; i < m->nhits; i++) {
        struct cov_hit *h = &m->hits[i], *p = n ? &m->hits[n - 1] : NULL;

        if (p && p->test == h->test && p->func == h->func) {
            if (p->bitmap && h->bitmap)
                p->count = p->count > h->count ? p->count : h->count;
            else
                p->count += h->count;
            p->bitmap &= h->bitmap;
        } else
            m->hits[n++] = *h;
    }
    m->nhits = n;
    if (m->nlines)
        qsort(m->lines, m->nlines, sizeof *m->lines, cmp_line);
    for (i = n = 0; i < m->nlines; i++) {
        struct cov_line *l = &m->lines[i];

        if (n && !cmp_line(&m->lines[n - 1], l))
            m->lines[n - 1].count += l->count;
        else
            m->lines[n++] = *l;
    }
    m->nlines = n;
    if (m->nbranches)
        qsort(m->branches, m->nbranches, sizeof *m->branches, cmp_branch);
    for (i = n = 0; i < m->nbranches; i++) {
        struct cov_branch *b = &m->branches[i], *p = n ? &m->branches[n - 1] : NULL;

        if (p && !cmp_branch(p, b)) {
            if (b->taken >= 0)
                p->taken = (p->taken < 0 ? 0 : p->taken) + b->taken;
        } else
            m->branches[n++] = *b;
    }
    m->nbranches = n;
    free(m->order);
    if (!(m->order = malloc((m->tests.n + 1) * sizeof *m->order)))
        return -1;
    for (i = 0; i < m->tests.n; i++)
        m->order[i] = (int)i;
    if (m->tests.n)
        qsort(m->order, m->tests.n, sizeof *m->order, cmp_test);
    m->done = 1;
    return 0;
}

long covmap_tests(struct covmap *m)
{
    return prepare(m) ? -1 : (long)m->tests.n;
}

const char *covmap_test(const struct covmap *m, size_t i)
{
    return m->done && i < m->tests.n ? m->tests.str[m->order[i]] : NULL;
}

/* the first element of a sorted array whose test is t: elements of size
   bytes, the test an int at offset 0 */
static size_t first_of(const void *v, size_t n, size_t size, int t)
{
    size_t lo = 0, hi = n;

    while (lo < hi) {
        size_t mid = (lo + hi) / 2;

        if (*(const int *)(const void *)((const char *)v + mid * size) < t)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

static int write_test(struct covmap *m, int t, FILE *out)
{
    size_t h = first_of(m->hits, m->nhits, sizeof *m->hits, t);
    size_t l = first_of(m->lines, m->nlines, sizeof *m->lines, t);
    size_t b = first_of(m->branches, m->nbranches, sizeof *m->branches, t);
    char *const *s = m->strs.str;

    fprintf(out, "TN:%s\n", m->tests.str[t]);
    for (;;) {
        int file = -1, hf = -1, lf = -1, bf = -1;
        size_t k, n, nhit;

        /* the next file: the first of the three in the order of paths */
        if (h < m->nhits && m->hits[h].test == t)
            file = hf = m->funcs[m->hits[h].func].file;
        if (l < m->nlines && m->lines[l].test == t) {
            lf = m->lines[l].file;
            if (file < 0 || strcmp(s[lf], s[file]) < 0)
                file = lf;
        }
        if (b < m->nbranches && m->branches[b].test == t) {
            bf = m->branches[b].file;
            if (file < 0 || strcmp(s[bf], s[file]) < 0)
                file = bf;
        }
        if (file < 0)
            break;
        if (m->root)
            fprintf(out, "SF:%s%s%s\n", m->root, m->root[strlen(m->root) - 1] == '/' ? "" : "/",
                    s[file]);
        else
            fprintf(out, "SF:%s\n", s[file]);
        for (k = h; hf == file && k < m->nhits && m->hits[k].test == t
                    && m->funcs[m->hits[k].func].file == file; k++) {
            const struct cov_func *f = &m->funcs[m->hits[k].func];

            if (f->first)
                fprintf(out, "FN:%d,%d,%s\n", f->first, f->last, s[f->name]);
            else
                fprintf(out, "FN:1,%s\n", s[f->name]);
        }
        for (n = k - h, nhit = 0; h < k; h++) {
            fprintf(out, "FNDA:%llu,%s\n", m->hits[h].count, s[m->funcs[m->hits[h].func].name]);
            nhit += m->hits[h].count > 0;
        }
        if (n)
            fprintf(out, "FNF:%zu\nFNH:%zu\n", n, nhit);
        for (n = nhit = 0; bf == file && b < m->nbranches && m->branches[b].test == t
                           && m->branches[b].file == file; b++, n++) {
            const struct cov_branch *x = &m->branches[b];

            if (x->taken < 0)
                fprintf(out, "BRDA:%d,%d,%d,-\n", x->line, x->block, x->branch);
            else
                fprintf(out, "BRDA:%d,%d,%d,%lld\n", x->line, x->block, x->branch, x->taken);
            nhit += x->taken > 0;
        }
        if (n)
            fprintf(out, "BRF:%zu\nBRH:%zu\n", n, nhit);
        for (n = nhit = 0; lf == file && l < m->nlines && m->lines[l].test == t
                           && m->lines[l].file == file; l++, n++) {
            fprintf(out, "DA:%d,%llu\n", m->lines[l].line, m->lines[l].count);
            nhit += m->lines[l].count > 0;
        }
        if (n)
            fprintf(out, "LF:%zu\nLH:%zu\n", n, nhit);
        fputs("end_of_record\n", out);
    }
    return ferror(out) ? -1 : 0;
}

int covmap_write_test(struct covmap *m, size_t i, FILE *out)
{
    if (prepare(m) || i >= m->tests.n)
        return -1;
    return write_test(m, m->order[i], out);
}

int covmap_write_all(struct covmap *m, FILE *out)
{
    size_t i;

    if (prepare(m))
        return -1;
    for (i = 0; i < m->tests.n; i++)
        if (write_test(m, m->order[i], out))
            return -1;
    return 0;
}
