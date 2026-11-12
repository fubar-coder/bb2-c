/*
 * libscmp.c - the judge of the library tests: reading the case files and
 * comparing a run with the expected data (libscmp.h).
 */
#define _DEFAULT_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "libscmp.h"
#include "libscmp_int.h"

static int fail(char *err, size_t errlen, const char *path, uint32_t line, const char *fmt, ...)
{
    va_list ap;
    int n = line ? snprintf(err, errlen, "%s:%lu: ", path, (unsigned long)line)
                 : snprintf(err, errlen, "%s: ", path);

    if (n < 0 || (size_t)n >= errlen)
        return -2;
    va_start(ap, fmt);
    vsnprintf(err + n, errlen - (size_t)n, fmt, ap);
    va_end(ap);
    return -2;
}

static uint32_t hash_key(const char *k, size_t n)
{
    uint32_t h = 2166136261u;
    size_t i;

    for (i = 0; i < n; i++)
        h = (h ^ (uint8_t)k[i]) * 16777619u;
    return h;
}

/* the slot of key k (hash h): the entry's slot or a free one */
static size_t slot_of(const struct libscmp_file *f, const char *k, size_t n, uint32_t h)
{
    size_t mask = f->nslots - 1, s = h & mask;

    for (;; s = (s + 1) & mask) {
        uint32_t i = f->slots[s];
        const struct libscmp_entry *e;

        if (!i)
            return s;
        e = &f->entries[i - 1];
        if (e->hash == h && e->keylen == n && !memcmp(f->text + e->key, k, n))
            return s;
    }
}

static int grow_slots(struct libscmp_file *f)
{
    size_t n = f->nslots ? 2 * f->nslots : 1024, i;
    uint32_t *s = calloc(n, sizeof *s);

    if (!s)
        return -1;
    free(f->slots);
    f->slots = s;
    f->nslots = n;
    for (i = 0; i < f->nentries; i++) {
        const struct libscmp_entry *e = &f->entries[i];

        f->slots[slot_of(f, f->text + e->key, e->keylen, e->hash)] = (uint32_t)i + 1;
    }
    return 0;
}

const struct libscmp_entry *libscmp_find_h(const struct libscmp_file *f, const char *key,
                                           size_t keylen, uint32_t h)
{
    uint32_t i;

    if (!f->nslots)
        return NULL;
    i = f->slots[slot_of(f, key, keylen, h)];
    return i ? &f->entries[i - 1] : NULL;
}

const struct libscmp_entry *libscmp_find(const struct libscmp_file *f, const char *key,
                                         size_t keylen)
{
    return libscmp_find_h(f, key, keylen, hash_key(key, keylen));
}

int32_t libscmp_applies(const struct libscmp_entry *e, enum libscmp_set set)
{
    if (e->plain >= 0)
        return e->plain;
    return set == LIBSCMP_OBJ ? e->fixed : e->orig;
}

void libscmp_free(struct libscmp_file *f)
{
    free(f->text);
    free(f->cases);
    free(f->entries);
    free(f->slots);
    memset(f, 0, sizeof *f);
}

static int is_hex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

static int hexval(char c)
{
    return c <= '9' ? c - '0' : c - 'A' + 10;
}

/* a decimal number [1-9][0-9]* (or "0" if zero is allowed) at *p, at most
   9 digits; the end in *p; -1 = none */
static long decimal(const char **p, const char *end, int zero)
{
    const char *q = *p;
    long v = 0;

    if (q == end || *q < '0' || *q > '9' || (*q == '0' && !zero))
        return -1;
    if (*q == '0') {
        if (q + 1 < end && q[1] >= '0' && q[1] <= '9')
            return -1;
        *p = q + 1;
        return 0;
    }
    for (; q < end && *q >= '0' && *q <= '9'; q++) {
        if (q - *p == 9)
            return -1;
        v = 10 * v + (*q - '0');
    }
    *p = q;
    return v;
}

/* one part of a key at *p up to its '|': the end in *p (at the '|');
   NULL = ok, else the message */
static const char *key_part(const char **p, const char *end, char *msg, size_t msglen)
{
    const char *q = *p;

    for (; q < end && *q != '|'; q++) {
        unsigned char c = (unsigned char)*q;

        if (c == '\\') {
            if (q + 1 == end)
                return "a '\\' at the end of the line";
            switch (q[1]) {
            case 't': case '|': case '\\':
                q++;
                continue;
            case 'x': {
                int v;

                if (end - q < 4 || !is_hex(q[2]) || !is_hex(q[3]))
                    return "\\x needs two upper-case hex digits";
                v = hexval(q[2]) * 16 + hexval(q[3]);
                if (v >= 0x20 && v < 0x7F) {
                    snprintf(msg, msglen, "needless escape \\x%c%c (a printable byte)", q[2], q[3]);
                    return msg;
                }
                if (v == '\t')
                    return "\\x09 for a TAB (it is \\t)";
                q += 3;
                continue;
            }
            default:
                snprintf(msg, msglen, "unknown escape \\%c (\\t, \\|, \\\\, \\xHH)", q[1]);
                return msg;
            }
        }
        if (c == '\t')
            return "the key needs ROUTINE|CHECK|VECTOR|OCC or BLOCK:N";
        if (c < 0x20 || c > 0x7E) {
            snprintf(msg, msglen, "byte $%02X not escaped in the key (\\xHH)", c);
            return msg;
        }
    }
    if (q == end)
        return "the key needs ROUTINE|CHECK|VECTOR|OCC or BLOCK:N";
    *p = q;
    return NULL;
}

static int add_case(struct libscmp_file *f, const struct libscmp_case *c)
{
    if (f->ncases == f->capcases) {
        size_t cap = f->capcases ? 2 * f->capcases : 256;
        struct libscmp_case *v = realloc(f->cases, cap * sizeof *v);

        if (!v)
            return -1;
        f->cases = v;
        f->capcases = cap;
    }
    f->cases[f->ncases++] = *c;
    return 0;
}

static const char *qual_text(const struct libscmp_case *c, char *buf, size_t len)
{
    if (c->qual == LIBSCMP_QORIG)
        return " [orig]";
    if (c->qual == LIBSCMP_QFIXED) {
        snprintf(buf, len, " [fixed #%lu]", (unsigned long)c->issue);
        return buf;
    }
    return "";
}

/* the case c into the index; 0 = ok, -1 = out of memory, -2 = duplicate
   or ambiguous (err) */
static int index_case(struct libscmp_file *f, struct libscmp_case *c, const char *path,
                      char *err, size_t errlen)
{
    const char *k = f->text + c->key;
    uint32_t h = hash_key(k, c->keylen);
    struct libscmp_entry *e;
    int32_t *slot, other;
    size_t s;
    char q[32];

    if (2 * (f->nentries + 1) > f->nslots && grow_slots(f))
        return -1;
    s = slot_of(f, k, c->keylen, h);
    if (f->slots[s])
        e = &f->entries[f->slots[s] - 1];
    else {
        if (f->nentries == f->capentries) {
            size_t cap = f->capentries ? 2 * f->capentries : 256;
            struct libscmp_entry *v = realloc(f->entries, cap * sizeof *v);

            if (!v)
                return -1;
            f->entries = v;
            f->capentries = cap;
        }
        e = &f->entries[f->nentries++];
        e->key = c->key;
        e->keylen = c->keylen;
        e->hash = h;
        e->plain = e->orig = e->fixed = -1;
        f->slots[s] = (uint32_t)f->nentries;
    }
    c->entry = (uint32_t)(e - f->entries);
    slot = c->qual == LIBSCMP_PLAIN ? &e->plain : c->qual == LIBSCMP_QORIG ? &e->orig : &e->fixed;
    if (*slot >= 0)
        return fail(err, errlen, path, c->line, "duplicate key %.*s%s (first at line %lu)",
                    (int)c->keylen, k, qual_text(&f->cases[*slot], q, sizeof q),
                    (unsigned long)f->cases[*slot].line);
    other = c->qual == LIBSCMP_PLAIN ? (e->orig >= 0 ? e->orig : e->fixed) : e->plain;
    if (other >= 0)
        return fail(err, errlen, path, c->line,
                    "%.*s is unqualified and qualified (the other at line %lu)", (int)c->keylen,
                    k, (unsigned long)f->cases[other].line);
    *slot = (int32_t)f->ncases;
    return add_case(f, c);
}

/* the header "# capture NAME 1" */
static int header(struct libscmp_file *f, const char *p, const char *end, const char *path,
                  char *err, size_t errlen)
{
    static const char tag[] = "# capture ";
    const char *n = p + sizeof tag - 1, *q = n;

    if ((size_t)(end - p) < sizeof tag - 1 || memcmp(p, tag, sizeof tag - 1))
        return fail(err, errlen, path, 1, "no header \"# capture NAME 1\"");
    while (q < end && ((*q >= 'A' && *q <= 'Z') || (*q >= 'a' && *q <= 'z')
                       || (*q >= '0' && *q <= '9') || *q == '_' || *q == '.' || *q == '-'))
        q++;
    if (q == n || q == end || *q != ' ' || (size_t)(q - n) >= sizeof f->name)
        return fail(err, errlen, path, 1, "the header needs a NAME of [A-Za-z0-9_.-] "
                    "(at most %zu characters)", sizeof f->name - 1);
    if (end - q != 2 || q[1] != '1')
        return fail(err, errlen, path, 1, "format version %.*s, the judge reads 1",
                    (int)(end - q - 1), q + 1);
    memcpy(f->name, n, (size_t)(q - n));
    f->name[q - n] = 0;
    return 0;
}

/* one case line */
static int case_line(struct libscmp_file *f, const char *p, const char *end, uint32_t line,
                     enum libscmp_kind kind, const char *path, char *err, size_t errlen)
{
    struct libscmp_case c = { 0 };
    const char *q = p, *m;
    char msg[96];
    int i;

    if (p == end)
        return fail(err, errlen, path, line, "empty line");
    if (end - p >= 10 && !memcmp(p, "# capture ", 10))
        return fail(err, errlen, path, line, "a second header");
    c.line = line;
    c.key = (uint32_t)(p - f->text);
    if (end - q > 6 && !memcmp(q, "BLOCK:", 6) && q[6] >= '0' && q[6] <= '9') {
        q += 6;
        if (decimal(&q, end, 0) < 0)
            return fail(err, errlen, path, line,
                        "BLOCK:N needs a decimal N from 1 without leading zero");
    } else {
        for (i = 0; i < 3; i++) {
            if ((m = key_part(&q, end, msg, sizeof msg)))
                return fail(err, errlen, path, line, "%s", m);
            q++;
        }
        if (decimal(&q, end, 0) < 0)
            return fail(err, errlen, path, line, "the occurrence number (after the third '|') "
                        "needs decimal digits from 1, no leading zero");
    }
    c.keylen = (uint32_t)(q - p);
    if (q < end && *q == ' ') {
        if (kind == LIBSCMP_RUN)
            return fail(err, errlen, path, line, "a qualifier in a run's output");
        q++;
        if (end - q >= 6 && !memcmp(q, "[orig]", 6)) {
            c.qual = LIBSCMP_QORIG;
            q += 6;
        } else if (end - q >= 8 && !memcmp(q, "[fixed #", 8)) {
            long n;

            q += 8;
            if ((n = decimal(&q, end, 0)) < 0 || q == end || *q != ']')
                return fail(err, errlen, path, line, "[fixed #N] needs an issue number N");
            c.qual = LIBSCMP_QFIXED;
            c.issue = (uint32_t)n;
            q++;
        } else
            return fail(err, errlen, path, line,
                        "unknown qualifier (the qualifiers are [orig] and [fixed #N])");
    }
    if (q == end || *q != '\t')
        return fail(err, errlen, path, line, "no TAB after the key");
    q++;
    if (q == end)
        return fail(err, errlen, path, line, "no value");
    c.val = (uint32_t)(q - f->text);
    c.vallen = (uint32_t)(end - q);
    for (; q < end; q++)
        if ((unsigned char)*q < 0x20 || (unsigned char)*q > 0x7E)
            return fail(err, errlen, path, line, "byte $%02X in the value (printable ASCII only)",
                        (unsigned char)*q);
    return index_case(f, &c, path, err, errlen);
}

int libscmp_parse(struct libscmp_file *f, const char *text, size_t len, const char *path,
                  enum libscmp_kind kind, char *err, size_t errlen)
{
    const char *p, *end, *z;
    uint32_t line = 0;
    int r;

    memset(f, 0, sizeof *f);
    if (len >= UINT32_MAX)
        return fail(err, errlen, path, 0, "larger than 4 GB");
    if (!(f->text = malloc(len + 1))) {
        snprintf(err, errlen, "%s: out of memory", path);
        return -1;
    }
    memcpy(f->text, text, len);
    f->text[len] = 0;
    f->len = len;
    end = f->text + len;
    if ((z = memchr(f->text, 0, len))) {
        for (p = f->text, line = 1; p < z; p++)
            line += *p == '\n';
        return fail(err, errlen, path, line, "a 0 byte");
    }
    for (p = f->text; p < end; p = z + 1) {
        const char *e;

        if (!(z = memchr(p, '\n', (size_t)(end - p))))
            z = end;
        e = z;
        line++;
        r = line == 1 ? header(f, p, e, path, err, errlen)
                      : case_line(f, p, e, line, kind, path, err, errlen);
        if (r == -1)
            snprintf(err, errlen, "%s: out of memory", path);
        if (r)
            return r;
        if (z == end)
            break;
    }
    if (!line)
        return fail(err, errlen, path, 0, "empty (no header \"# capture NAME 1\")");
    return 0;
}

char *libscmp_read_file(const char *path, size_t *len)
{
    FILE *fp = fopen(path, "rb");
    char *s = NULL;
    long n = 0;

    if (!fp)
        return NULL;
    if (!fseek(fp, 0, SEEK_END) && (n = ftell(fp)) >= 0 && !fseek(fp, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, fp) == (size_t)n)
        s[n] = 0;
    else {
        free(s);
        s = NULL;
    }
    fclose(fp);
    *len = (size_t)n;
    return s;
}

int libscmp_load(struct libscmp_file *f, const char *path, enum libscmp_kind kind, char *err,
                 size_t errlen)
{
    size_t len;
    char *s = libscmp_read_file(path, &len);
    int r;

    if (!s) {
        memset(f, 0, sizeof *f);
        snprintf(err, errlen, "%s: cannot read it", path);
        return -3;
    }
    r = libscmp_parse(f, s, len, path, kind, err, errlen);
    free(s);
    return r;
}

static int not_captured(const char *v, uint32_t n)
{
    return n >= 14 && !memcmp(v, "not captured: ", 14);
}

/* the key of case c for messages: KEY[ QUAL][ (observations A..B)] */
void libscmp_key_text(const struct libscmp_file *f, const struct libscmp_case *c,
                      const struct libscmp_file *other, const struct libscmp_case *oc, char *buf,
                      size_t len)
{
    const char *k = f->text + c->key, *v = f->text + c->val, *ve = v + c->vallen;
    char q[32];
    int n = snprintf(buf, len, "%.*s%s", (int)c->keylen, k, qual_text(c, q, sizeof q));

    if (n < 0 || (size_t)n >= len)
        return;
    if (c->keylen > 6 && !memcmp(k, "BLOCK:", 6)) {
        const char *p = k + 6;
        long b = decimal(&p, k + c->keylen, 0), cnt;

        p = v;
        cnt = decimal(&p, ve, 0);
        if ((cnt < 0 || (p < ve && *p != ' ')) && oc) {
            v = other->text + oc->val;
            ve = v + oc->vallen;
            p = v;
            cnt = decimal(&p, ve, 0);
        }
        if (b > 0 && cnt > 0 && cnt <= LIBSCMP_BLOCK && (p == ve || *p == ' '))
            snprintf(buf + n, len - (size_t)n, " (observations %ld..%ld)",
                     (b - 1) * LIBSCMP_BLOCK + 1, (b - 1) * LIBSCMP_BLOCK + cnt);
    }
}

int libscmp_compare(const struct libscmp_file *run, const struct libscmp_file *exp,
                    enum libscmp_set set, FILE *out, struct libscmp_stats *st)
{
    struct libscmp_stats s = { 0 };
    size_t i, shown = 0;
    char k[600];

#define FINDING(...) do { \
        if (++shown <= LIBSCMP_DETAILS) \
            fprintf(out, __VA_ARGS__); \
    } while (0)

    s.cases = run->ncases;
    for (i = 0; i < run->ncases; i++) {
        const struct libscmp_case *c = &run->cases[i];
        const struct libscmp_entry *re = &run->entries[c->entry];
        const struct libscmp_entry *e = libscmp_find_h(exp, run->text + c->key, c->keylen,
                                                       re->hash);
        const struct libscmp_case *x;
        int32_t j;

        if (!e || (j = libscmp_applies(e, set)) < 0) {
            s.added++;
            libscmp_key_text(run, c, NULL, NULL, k, sizeof k);
            FINDING("%s: %s: added (got %.*s)\n", run->name, k, (int)c->vallen,
                    run->text + c->val);
            continue;
        }
        x = &exp->cases[j];
        if (not_captured(exp->text + x->val, x->vallen)
            && not_captured(run->text + c->val, c->vallen))
            s.skipped++;
        else if (x->vallen == c->vallen && !memcmp(exp->text + x->val, run->text + c->val, c->vallen))
            s.equal++;
        else {
            s.differ++;
            libscmp_key_text(exp, x, run, c, k, sizeof k);
            FINDING("%s: %s: expected %.*s, got %.*s\n", run->name, k, (int)x->vallen,
                    exp->text + x->val, (int)c->vallen, run->text + c->val);
        }
    }
    for (i = 0; i < exp->nentries; i++) {
        const struct libscmp_entry *e = &exp->entries[i];
        int32_t j = libscmp_applies(e, set);
        const struct libscmp_case *x;

        if (j < 0 || libscmp_find_h(run, exp->text + e->key, e->keylen, e->hash))
            continue;
        x = &exp->cases[j];
        s.missing++;
        libscmp_key_text(exp, x, NULL, NULL, k, sizeof k);
        FINDING("%s: %s: missing (expected %.*s)\n", run->name, k, (int)x->vallen,
                exp->text + x->val);
    }
#undef FINDING
    if (shown > LIBSCMP_DETAILS)
        fprintf(out, "%s: ... %zu more\n", run->name, shown - LIBSCMP_DETAILS);
    fprintf(out, "%s: set %s: %zu cases, %zu equal, %zu differ, %zu missing, %zu added, "
            "%zu not captured\n", run->name, set == LIBSCMP_OBJ ? "obj" : "orig", s.cases, s.equal,
            s.differ, s.missing, s.added, s.skipped);
    if (st)
        *st = s;
    return shown ? 1 : 0;
}

/* --- directories ------------------------------------------------------ */

int libscmp_valid_name(const char *s)
{
    size_t n = 0;

    for (; s[n]; n++)
        if (!((s[n] >= 'A' && s[n] <= 'Z') || (s[n] >= 'a' && s[n] <= 'z')
              || (s[n] >= '0' && s[n] <= '9') || s[n] == '_' || s[n] == '.' || s[n] == '-'))
            return 0;
    return n > 0 && n < sizeof ((struct libscmp_file *)0)->name;
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

void libscmp_names_free(struct libscmp_names *l)
{
    size_t i;

    for (i = 0; i < l->n; i++)
        free(l->v[i]);
    free(l->v);
    memset(l, 0, sizeof *l);
}

int libscmp_names_add(struct libscmp_names *l, const char *s, size_t n)
{
    size_t i;

    for (i = 0; i < l->n; i++)
        if (strlen(l->v[i]) == n && !memcmp(l->v[i], s, n))
            return 0;
    if (l->n == l->cap) {
        size_t cap = l->cap ? 2 * l->cap : 64;
        char **v = realloc(l->v, cap * sizeof *v);

        if (!v)
            return -1;
        l->v = v;
        l->cap = cap;
    }
    if (!(l->v[l->n] = strndup(s, n)))
        return -1;
    l->n++;
    return 0;
}

void libscmp_names_sort(struct libscmp_names *l)
{
    if (l->n)
        qsort(l->v, l->n, sizeof *l->v, cmp_str);
}

int libscmp_names_dir(struct libscmp_names *l, const char *dir, FILE *err)
{
    DIR *d = opendir(dir);
    struct dirent *de;

    if (!d) {
        fprintf(err, "%s: cannot read the directory (%s)\n", dir, strerror(errno));
        return -1;
    }
    while ((de = readdir(d))) {
        size_t n = strlen(de->d_name);

        if (n > 4 && !strcmp(de->d_name + n - 4, ".txt") && libscmp_names_add(l, de->d_name, n - 4)) {
            closedir(d);
            fprintf(err, "out of memory\n");
            return -1;
        }
    }
    closedir(d);
    return 0;
}

char *libscmp_path(const char *dir, const char *name)
{
    /* "DIR/NAME.txt" in a buffer of exactly its size.  Copied with the
     * lengths already taken: with -fsanitize=undefined, gcc keeps a path
     * on which strlen was handed NULL (the check reports and goes on) and
     * warns about a null %s argument of snprintf there. */
    size_t nd = strlen(dir), nn = strlen(name);
    char *p = malloc(nd + 1 + nn + sizeof ".txt");

    if (p) {
        memcpy(p, dir, nd);
        p[nd] = '/';
        memcpy(p + nd + 1, name, nn);
        memcpy(p + nd + 1 + nn, ".txt", sizeof ".txt");
    }
    return p;
}

int libscmp_exists(const char *path)
{
    FILE *f = fopen(path, "rb");

    if (f)
        fclose(f);
    return f != NULL;
}

int libscmp_program_names(struct libscmp_names *names, const char *dir1, const char *dir2,
                          const char *const *only, size_t nonly, FILE *err)
{
    size_t i;

    for (i = 0; i < nonly; i++) {
        if (!libscmp_valid_name(only[i])) {
            fprintf(err, "--only %s: not a program name ([A-Za-z0-9_.-])\n", only[i]);
            return -1;
        }
        if (libscmp_names_add(names, only[i], strlen(only[i]))) {
            fprintf(err, "out of memory\n");
            return -1;
        }
    }
    if (!nonly && (libscmp_names_dir(names, dir1, err) || (dir2 && libscmp_names_dir(names, dir2, err))))
        return -1;
    libscmp_names_sort(names);
    return 0;
}

int libscmp_load_named(struct libscmp_file *f, const char *dir, const char *name,
                       enum libscmp_kind kind, FILE *err)
{
    char *path = libscmp_path(dir, name);
    char msg[700];
    int r;

    if (!path) {
        fprintf(err, "out of memory\n");
        return -1;
    }
    r = libscmp_load(f, path, kind, msg, sizeof msg);
    if (r)
        fprintf(err, "%s\n", msg);
    else if (strcmp(f->name, name)) {
        fprintf(err, "%s:1: the header names %s, the file is %s.txt\n", path, f->name, name);
        r = -2;
    }
    free(path);
    return r;
}

int libscmp_compare_dirs(const char *run_dir, const char *exp_dir, enum libscmp_set set,
                         const char *const *only, size_t nonly, FILE *out, FILE *err)
{
    struct libscmp_names names = { 0 };
    size_t i, nequal = 0, nfail = 0, nbad = 0;
    int status = 0;

    if (libscmp_program_names(&names, run_dir, exp_dir, only, nonly, err)) {
        libscmp_names_free(&names);
        return 2;
    }
    for (i = 0; i < names.n; i++) {
        const char *name = names.v[i];
        char *rp = libscmp_path(run_dir, name), *ep = libscmp_path(exp_dir, name);
        int hr, he;

        if (!rp || !ep) {
            fprintf(err, "out of memory\n");
            free(rp);
            free(ep);
            status = 2;
            break;
        }
        hr = libscmp_exists(rp);
        he = libscmp_exists(ep);
        if (!hr && !he) {
            fprintf(err, "%s: no output (%s) and no expected data (%s)\n", name, rp, ep);
            status = 2;
            nbad++;
        } else if (!hr || !he) {
            fprintf(out, hr ? "%s: no expected data (%s)\n" : "%s: no output of the run (%s)\n",
                    name, hr ? ep : rp);
            nfail++;
            if (!status)
                status = 1;
        } else {
            struct libscmp_file run, exp;
            int r1 = libscmp_load_named(&run, run_dir, name, LIBSCMP_RUN, err);
            int r2 = libscmp_load_named(&exp, exp_dir, name, LIBSCMP_EXPECTED, err);

            if (r1 || r2) {
                status = 2;
                nbad++;
            }
            else {
                int r = libscmp_compare(&run, &exp, set, out, NULL);

                if (r < 0) {
                    fprintf(err, "out of memory\n");
                    status = 2;
                } else if (r) {
                    nfail++;
                    if (!status)
                        status = 1;
                } else
                    nequal++;
            }
            libscmp_free(&run);
            libscmp_free(&exp);
        }
        free(rp);
        free(ep);
    }
    fprintf(out, "libs-compare: %zu programs, %zu equal, %zu differ, %zu not judged (set %s)\n",
            names.n, nequal, nfail, nbad, set == LIBSCMP_OBJ ? "obj" : "orig");
    libscmp_names_free(&names);
    return status;
}
