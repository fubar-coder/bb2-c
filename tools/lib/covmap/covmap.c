/*
 * covmap.c - the map from functions to the tests that executed them
 * (covmap.h).
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
    for (i = 0; i < m->nfuncs; i++)
        free(m->funcs[i].tests);
    free(m->funcs);
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

static size_t fhash(int prog, int file, int name)
{
    size_t h = (size_t)prog * 2654435761u;

    h = (h ^ (size_t)file) * 2246822519u;
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

        j = fhash(f->prog, f->file, f->name) & (n - 1);
        while (slot[j] >= 0)
            j = (j + 1) & (n - 1);
        slot[j] = (int)i;
    }
    free(m->fslot);
    m->fslot = slot;
    m->nfslot = n;
    return 1;
}

/* the function (prog, file, name), added if new; -1 if out of memory */
static int get_func(struct covmap *m, int prog, int file, int name)
{
    size_t j;
    struct cov_func *f;

    if (2 * (m->nfuncs + 1) > m->nfslot && !frehash(m))
        return -1;
    j = fhash(prog, file, name) & (m->nfslot - 1);
    while (m->fslot[j] >= 0) {
        f = &m->funcs[m->fslot[j]];
        if (f->prog == prog && f->file == file && f->name == name)
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
    f->prog = prog;
    f->file = file;
    f->name = name;
    m->fslot[j] = (int)m->nfuncs;
    return (int)m->nfuncs++;
}

static int add_test(struct cov_func *f, int test)
{
    if (f->ntests && f->tests[f->ntests - 1] == test)
        return 0;
    if (f->ntests == f->cap) {
        size_t cap = f->cap ? 2 * f->cap : 4;
        int *v = realloc(f->tests, cap * sizeof *v);

        if (!v)
            return -1;
        f->tests = v;
        f->cap = cap;
    }
    f->tests[f->ntests++] = test;
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

int covmap_add_deps(struct covmap *m, const char *text, char *err, size_t errsz)
{
    return cov_deps_parse(&m->deps, text, err, errsz);
}

/* ---- host: gcov JSON ---- */

static int gcov_doc(struct covmap *m, int prog, int test, const struct json *doc,
                    char *err, size_t errsz)
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
            if ((id = strtab_add(&m->strs, name)) < 0 || (f = get_func(m, prog, file, id)) < 0)
                return OOM();
            widen(&m->funcs[f], (int)json_get_number(fn, "start_line", 0),
                  (int)json_get_number(fn, "end_line", 0));
            if (json_get_number(fn, "execution_count", 0) > 0 && add_test(&m->funcs[f], test))
                return OOM();
        }
    }
    return 0;
}

int covmap_add_gcov(struct covmap *m, const char *test, const char *text, size_t len,
                    char *err, size_t errsz)
{
    size_t pos = 0;
    int prog = strtab_add(&m->strs, "host"), t = strtab_add(&m->tests, test);
    char jerr[200];
    struct json *doc;

    if (prog < 0 || t < 0)
        return OOM();
    while ((doc = json_parse(text, len, &pos, jerr, sizeof jerr))) {
        int r = gcov_doc(m, prog, t, doc, err, errsz);

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
        if ((f = get_func(m, p->name, o->file, r->name)) < 0) {
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
    if ((t = strtab_add(&m->tests, test)) < 0 || prog_ready(m, p))
        return OOM();
    data += COVFMT_HEADER_SIZE;
    for (i = 0; i < 8 * bytes; i++) {
        const struct cov_sym *sym;

        if (!(data[i >> 3] & 1 << (i & 7)))
            continue;
        if (2 * i >= code)
            return error(err, errsz, "%s: %s: a bit beyond the code hunk (offset 0x%x)", test,
                         program, (unsigned)(2 * i));
        if ((sym = find_sym(p, 2 * i)) && add_test(&m->funcs[sym->func], t))
            return OOM();
    }
    return 0;
}

/* ---- map.tsv ---- */

static const struct covmap *sort_map;

static int cmp_func(const void *a, const void *b)
{
    const struct cov_func *x = &sort_map->funcs[*(const int *)a];
    const struct cov_func *y = &sort_map->funcs[*(const int *)b];
    char *const *s = sort_map->strs.str;
    int c;

    if ((c = strcmp(s[x->prog], s[y->prog])) || (c = strcmp(s[x->file], s[y->file])))
        return c;
    if (x->first != y->first)
        return x->first < y->first ? -1 : 1;
    return strcmp(s[x->name], s[y->name]);
}

static int cmp_test(const void *a, const void *b)
{
    const int x = *(const int *)a, y = *(const int *)b;

    return strcmp(sort_map->tests.str[x], sort_map->tests.str[y]);
}

static void write_tests(const struct covmap *m, int *ids, size_t n, FILE *out)
{
    size_t i;
    int last = -1;

    qsort(ids, n, sizeof *ids, cmp_test);
    for (i = 0; i < n; i++) {
        if (ids[i] == last)
            continue;
        fprintf(out, "%s%s", last < 0 ? "" : " ", m->tests.str[ids[i]]);
        last = ids[i];
    }
    fputc('\n', out);
}

int covmap_write(struct covmap *m, const char *commit, FILE *out)
{
    int *order, *all = NULL;
    size_t i, j, nall = 0, capall = 0;

    for (i = 0; i < m->nprogs; i++)
        if (prog_ready(m, &m->progs[i]))
            return -1;
    if (!(order = malloc((m->nfuncs + 1) * sizeof *order)))
        return -1;
    for (i = 0; i < m->nfuncs; i++)
        order[i] = (int)i;
    sort_map = m;
    qsort(order, m->nfuncs, sizeof *order, cmp_func);
    fprintf(out, "# commit %s\n# program\tfile\tfunction\tfirst\tlast\ttests\n", commit);
    for (i = 0; i < m->nfuncs; i++) {
        struct cov_func *f = &m->funcs[order[i]];
        const struct cov_func *next = i + 1 < m->nfuncs ? &m->funcs[order[i + 1]] : NULL;

        fprintf(out, "%s\t%s\t%s\t", m->strs.str[f->prog], m->strs.str[f->file], m->strs.str[f->name]);
        if (f->first)
            fprintf(out, "%d\t%d\t", f->first, f->last);
        else
            fputs("\t\t", out);
        write_tests(m, f->tests, f->ntests, out);
        for (j = 0; j < f->ntests; j++) {
            if (nall == capall) {
                int *v;

                capall = capall ? 2 * capall : 256;
                if (!(v = realloc(all, capall * sizeof *v))) {
                    free(all);
                    free(order);
                    return -1;
                }
                all = v;
            }
            all[nall++] = f->tests[j];
        }
        if (!next || next->prog != f->prog || next->file != f->file) {
            fprintf(out, "%s\t%s\t\t\t\t", m->strs.str[f->prog], m->strs.str[f->file]);
            write_tests(m, all, nall, out);
            nall = 0;
        }
    }
    free(all);
    free(order);
    return ferror(out) ? -1 : 0;
}
