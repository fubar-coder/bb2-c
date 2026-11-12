/*
 * libscmp_bin.c - the raw block observations of the library tests
 * (NAME.bin, libscmp.h): read, formatted and hashed as
 * libs/tests/capture.c writes plain cases, merged into a run's NAME.txt,
 * dumped and compared between two runs.
 */
#define _DEFAULT_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "libscmp.h"
#include "libscmp_int.h"

#define FNV_BASIS 0x811c9dc5u

static const char hexdig[] = "0123456789ABCDEF";

static uint32_t fnv(uint32_t h, const void *p, size_t n)
{
    const unsigned char *s = p;

    while (n--)
        h = (h ^ *s++) * 0x01000193u;
    return h;
}

static int bad(char *err, size_t errlen, const char *path, size_t at, const char *fmt, ...)
{
    va_list ap;
    int n = snprintf(err, errlen, "%s: offset %zu: ", path, at);

    if (n < 0 || (size_t)n >= errlen)
        return -2;
    va_start(ap, fmt);
    vsnprintf(err + n, errlen - (size_t)n, fmt, ap);
    va_end(ap);
    return -2;
}

/* --- reading ------------------------------------------------------------ */

int libscmp_obs_begin(struct libscmp_obs *o, const void *data, size_t len, const char *path,
                      char *err, size_t errlen)
{
    static const char tag[] = "# capture-bin ";
    const char *s = data, *n, *q, *e;

    memset(o, 0, sizeof *o);
    o->d = data;
    o->len = len;
    o->path = path;
    o->blkhash = o->hash = FNV_BASIS;
    if (len < sizeof tag - 1 || memcmp(s, tag, sizeof tag - 1)
        || !(e = memchr(s, '\n', len)))
        return bad(err, errlen, path, 0, "no header \"# capture-bin NAME 1\"");
    n = q = s + sizeof tag - 1;
    while (q < e && ((*q >= 'A' && *q <= 'Z') || (*q >= 'a' && *q <= 'z')
                     || (*q >= '0' && *q <= '9') || *q == '_' || *q == '.' || *q == '-'))
        q++;
    if (q == n || (size_t)(q - n) >= sizeof o->name || e - q != 2 || q[0] != ' ' || q[1] != '1')
        return bad(err, errlen, path, 0, "no header \"# capture-bin NAME 1\" (version 1)");
    memcpy(o->name, n, (size_t)(q - n));
    o->name[q - n] = 0;
    o->pos = (size_t)(e - s) + 1;
    return 0;
}

void libscmp_obs_free(struct libscmp_obs *o)
{
    free(o->val);
    o->val = NULL;
    o->valcap = 0;
}

/* the record being read: its start (for the messages) */
struct rd {
    struct libscmp_obs *o;
    size_t at;
    char *err;
    size_t errlen;
};

static int need(struct rd *r, size_t n)
{
    if (r->o->len - r->o->pos < n)
        return bad(r->err, r->errlen, r->o->path, r->at, "the record of type $%02X is cut off "
                   "(the file ends)", r->o->d[r->at]);
    return 0;
}

static int get_u(struct rd *r, int n, uint32_t *v)
{
    int i;

    if (need(r, (size_t)n))
        return -2;
    for (*v = 0, i = 0; i < n; i++)
        *v = *v << 8 | r->o->d[r->o->pos++];
    return 0;
}

static int get_s(struct rd *r, const char **s)
{
    struct libscmp_obs *o = r->o;
    const unsigned char *z = memchr(o->d + o->pos, 0, o->len - o->pos);

    if (!z)
        return need(r, o->len - o->pos + 1);
    *s = (const char *)o->d + o->pos;
    o->pos = (size_t)(z - o->d) + 1;
    return 0;
}

static int val_room(struct libscmp_obs *o, size_t n)
{
    if (o->vallen + n + 1 > o->valcap) {
        size_t cap = o->valcap ? o->valcap : 256;
        char *v;

        while (cap < o->vallen + n + 1)
            cap *= 2;
        if (!(v = realloc(o->val, cap)))
            return -1;
        o->val = v;
        o->valcap = cap;
    }
    return 0;
}

static int val_put(struct libscmp_obs *o, const char *s, size_t n)
{
    if (val_room(o, n))
        return -1;
    memcpy(o->val + o->vallen, s, n);
    o->vallen += n;
    o->val[o->vallen] = 0;
    return 0;
}

static int val_printf(struct libscmp_obs *o, const char *fmt, ...)
{
    char b[160];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    return val_put(o, b, n < 0 ? 0 : (size_t)n >= sizeof b ? sizeof b - 1 : (size_t)n);
}

/* append the escaped key part s (capture.c esc_part); -2 = too long */
static int key_part(struct libscmp_obs *o, const char *s)
{
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        char e[4];
        size_t k = 0;

        if (c == '\t') {
            e[k++] = '\\';
            e[k++] = 't';
        } else if (c == '|' || c == '\\') {
            e[k++] = '\\';
            e[k++] = (char)c;
        } else if (c < 0x20 || c > 0x7e) {
            e[k++] = '\\';
            e[k++] = 'x';
            e[k++] = hexdig[c >> 4];
            e[k++] = hexdig[c & 15];
        } else
            e[k++] = (char)c;
        if (o->keylen + k > LIBSCMP_KEY_MAX)
            return -2;
        memcpy(o->key + o->keylen, e, k);
        o->keylen += k;
    }
    return 0;
}

static int key_add(struct libscmp_obs *o, const char *s, size_t n)
{
    if (o->keylen + n > LIBSCMP_KEY_MAX)
        return -2;
    memcpy(o->key + o->keylen, s, n);
    o->keylen += n;
    return 0;
}

/* the open block closes: its case in blk* */
static void close_block(struct libscmp_obs *o)
{
    o->blkno++;
    o->blkcount = o->count;
    o->blkhash = o->hash;
    memcpy(o->blkfirst, o->first, sizeof o->first);
    o->count = 0;
    o->hash = FNV_BASIS;
}

/* the value of an observation of type t into o->val */
static int value(struct rd *r, uint32_t t)
{
    struct libscmp_obs *o = r->o;
    uint32_t v, n, i;
    const char *s = NULL;

    switch (t) {
    case 0x10:
        if (get_u(r, 4, &v))
            return -2;
        return val_printf(o, "$%08lX", (unsigned long)v);
    case 0x11:
        if (get_u(r, 4, &n))
            return -2;
        if (o->len - o->pos < n)
            return bad(r->err, r->errlen, o->path, r->at, "bytes of length %lu, beyond the end "
                       "of the file", (unsigned long)n);
        o->bytes = o->d + o->pos;
        o->nbytes = n;
        o->pos += n;
        if (n > 64)
            return val_printf(o, "bytes %lu fnv %08lX", (unsigned long)n,
                              (unsigned long)fnv(FNV_BASIS, o->bytes, n));
        if (val_printf(o, n ? "bytes %lu " : "bytes %lu", (unsigned long)n) || val_room(o, 2 * n))
            return -1;
        for (i = 0; i < n; i++) {
            o->val[o->vallen++] = hexdig[o->bytes[i] >> 4];
            o->val[o->vallen++] = hexdig[o->bytes[i] & 15];
        }
        o->val[o->vallen] = 0;
        return 0;
    case 0x13:
        if (get_u(r, 1, &v))
            return -2;
        if (v > 1)
            return bad(r->err, r->errlen, o->path, r->at, "text flag %lu (0 or 1)",
                       (unsigned long)v);
        if (!v)
            return val_put(o, "null", 4);
        if (get_s(r, &s))
            return -2;
        if (val_room(o, 4 * strlen(s) + 2))
            return -1;
        o->val[o->vallen++] = '"';
        for (; *s; s++) {
            unsigned char c = (unsigned char)*s;

            if (c == '"' || c == '\\') {
                o->val[o->vallen++] = '\\';
                o->val[o->vallen++] = (char)c;
            } else if (c < 0x20 || c > 0x7e) {
                o->val[o->vallen++] = '\\';
                o->val[o->vallen++] = 'x';
                o->val[o->vallen++] = hexdig[c >> 4];
                o->val[o->vallen++] = hexdig[c & 15];
            } else
                o->val[o->vallen++] = (char)c;
        }
        o->val[o->vallen++] = '"';
        o->val[o->vallen] = 0;
        return 0;
    case 0x14:
        if (get_u(r, 1, &n))
            return -2;
        if (n == 0)
            return val_put(o, "prog", 4);
        if (n > 3)
            return bad(r->err, r->errlen, o->path, r->at, "address kind %lu (0-3)",
                       (unsigned long)n);
        if (get_u(r, 4, &v))
            return -2;
        if (n == 1)
            return val_printf(o, "$%08lX", (unsigned long)v);
        if (get_s(r, &s))
            return -2;
        return val_printf(o, "%.100s%c$%lX", s, n == 2 ? '+' : '-', (unsigned long)v);
    case 0x15:
        if (get_u(r, 2, &v))
            return -2;
        if (v & 0x8000)
            return bad(r->err, r->errlen, o->path, r->at, "register set $%04lX (bits 0-14)",
                       (unsigned long)v);
        o->lost = v;
        if (!v)
            return val_put(o, "kept", 4);
        if (val_put(o, "lost", 4))
            return -1;
        for (i = 0; i < 15; i++)
            if (v & (1u << i)) {
                char reg[3] = { ' ', i < 8 ? 'd' : 'a', (char)('0' + (i & 7)) };

                if (val_put(o, reg, 3))
                    return -1;
            }
        return 0;
    }
    return bad(r->err, r->errlen, o->path, r->at, "unknown record type $%02lX",
               (unsigned long)t);
}

int libscmp_obs_next(struct libscmp_obs *o, char *err, size_t errlen)
{
    struct rd r = { o, 0, err, errlen };

    if (o->full) {
        o->full = 0;
        close_block(o);
        return LIBSCMP_OBS_BLOCK;
    }
    for (;;) {
        const char *check = NULL, *vec = NULL;
        uint32_t t;
        char num[16], *p;
        int k;

        r.at = o->pos;
        if (o->pos == o->len) {
            if (o->count)
                return bad(err, errlen, o->path, o->pos, "the file ends inside block %lu "
                           "(no end of block mode: the program stopped?)",
                           (unsigned long)o->blkno + 1);
            return LIBSCMP_OBS_END;
        }
        t = o->d[o->pos++];
        if (t == 0x01) {
            const char *s = NULL;

            if (get_s(&r, &s))
                return -2;
            if (strlen(s) > LIBSCMP_KEY_MAX)
                return bad(err, errlen, o->path, r.at, "a routine longer than %d bytes",
                           LIBSCMP_KEY_MAX);
            strcpy(o->routine, s);
            continue;
        }
        if (t == 0x02) {
            if (o->count) {
                close_block(o);
                return LIBSCMP_OBS_BLOCK;
            }
            continue;
        }
        if (t < 0x10 || t > 0x15 || t == 0x12)
            return bad(err, errlen, o->path, r.at, "unknown record type $%02lX",
                       (unsigned long)t);
        if (get_s(&r, &check) || get_s(&r, &vec))
            return -2;
        o->type = (uint8_t)t;
        o->vallen = 0;
        o->bytes = NULL;
        o->nbytes = o->lost = 0;
        if ((k = value(&r, t)))
            return k == -1 ? (snprintf(err, errlen, "%s: out of memory", o->path), -1) : -2;
        o->obs++;
        p = num + sizeof num - 1;
        *p = 0;
        {
            uint32_t v = o->obs;

            do
                *--p = (char)('0' + v % 10);
            while (v /= 10);
        }
        *--p = '|';
        o->keylen = 0;
        if (key_add(o, o->routine, strlen(o->routine)) || key_add(o, "|", 1)
            || key_part(o, check) || key_add(o, "|", 1) || key_part(o, vec)
            || key_add(o, p, strlen(p)))
            return bad(err, errlen, o->path, r.at, "a key longer than %d bytes",
                       LIBSCMP_KEY_MAX);
        o->key[o->keylen] = 0;
        if (!o->count)
            memcpy(o->first, o->key, o->keylen + 1);
        o->hash = fnv(o->hash, o->key, o->keylen);
        o->hash = fnv(o->hash, "\t", 1);
        o->hash = fnv(o->hash, o->val, o->vallen);
        o->hash = fnv(o->hash, "\n", 1);
        o->block = o->blkno + 1;
        if (++o->count == LIBSCMP_BLOCK)
            o->full = 1;
        return LIBSCMP_OBS;
    }
}

/* the next block of o (skipping its observations); the result of
   libscmp_obs_next: LIBSCMP_OBS_BLOCK, LIBSCMP_OBS_END or an error */
static int next_block(struct libscmp_obs *o, char *err, size_t errlen)
{
    int r;

    while ((r = libscmp_obs_next(o, err, errlen)) == LIBSCMP_OBS)
        ;
    return r;
}

/* --- the merge ---------------------------------------------------------- */

/* a block place "BLOCK:N<TAB>COUNT in NAME.bin" of the line p..e: N,
   COUNT; 0 = not one */
static int place(const char *p, const char *e, const char *name, unsigned long *n,
                 unsigned long *count)
{
    char tail[100];
    const char *q;
    char *x;

    if (e - p < 8 || memcmp(p, "BLOCK:", 6) || p[6] < '1' || p[6] > '9')
        return 0;
    *n = strtoul(p + 6, &x, 10);
    if (x + 1 >= e || *x != '\t' || x[1] < '1' || x[1] > '9')
        return 0;
    *count = strtoul(x + 1, &x, 10);
    snprintf(tail, sizeof tail, " in %s.bin", name);
    q = x;
    return (size_t)(e - q) == strlen(tail) && !memcmp(q, tail, (size_t)(e - q));
}

int libscmp_bin_merge(const char *text, size_t len, const char *path, const void *bin,
                      size_t binlen, const char *binpath, char **out, size_t *outlen, char *err,
                      size_t errlen)
{
    struct libscmp_obs o;
    const char *p = text, *end = text + len, *z;
    char name[64] = "";
    uint32_t line = 0;
    FILE *m;
    int r = 0, ev;

    *out = NULL;
    *outlen = 0;
    memset(&o, 0, sizeof o);
    /* the NAME of the header (malformed headers are the parser's) */
    if (len > 10 && !memcmp(text, "# capture ", 10)) {
        const char *n = text + 10, *q = n;

        while (q < end && *q != ' ' && *q != '\n' && q - n < 63)
            q++;
        memcpy(name, n, (size_t)(q - n));
        name[q - n] = 0;
    }
    if (bin && (r = libscmp_obs_begin(&o, bin, binlen, binpath, err, errlen)))
        return r;
    if (bin && strcmp(o.name, name)) {
        snprintf(err, errlen, "%s: the header names %s, the output %s", binpath, o.name,
                 name[0] ? name : "(none)");
        return -2;
    }
    if (!(m = open_memstream(out, outlen))) {
        snprintf(err, errlen, "%s: out of memory", path);
        return -1;
    }
    for (; p < end && !r; p = z + 1) {
        unsigned long n, count;
        const char *e;

        line++;
        if (!(z = memchr(p, '\n', (size_t)(end - p))))
            z = end;
        e = z;
        if (line > 1 && name[0] && place(p, e, name, &n, &count)) {
            if (!bin) {
                snprintf(err, errlen, "%s:%lu: a block case in %s.bin, which is missing", path,
                         (unsigned long)line, name);
                r = -2;
                break;
            }
            ev = next_block(&o, err, errlen);
            if (ev < 0) {
                r = ev;
                break;
            }
            if (ev == LIBSCMP_OBS_END || o.blkno != n || o.blkcount != count) {
                if (ev == LIBSCMP_OBS_END)
                    snprintf(err, errlen, "%s:%lu: BLOCK:%lu, but %s has only %lu blocks", path,
                             (unsigned long)line, n, binpath, (unsigned long)o.blkno);
                else
                    snprintf(err, errlen, "%s:%lu: BLOCK:%lu with %lu observations, but block "
                             "%lu of %s has %lu", path, (unsigned long)line, n, count,
                             (unsigned long)o.blkno, binpath, (unsigned long)o.blkcount);
                r = -2;
                break;
            }
            fprintf(m, "BLOCK:%lu\t%lu %08lX first=%s", n, count, (unsigned long)o.blkhash,
                    o.blkfirst);
        } else
            fwrite(p, 1, (size_t)(e - p), m);
        if (z == end)
            break;
        fputc('\n', m);
    }
    if (!r && bin) {
        ev = next_block(&o, err, errlen);
        if (ev < 0)
            r = ev;
        else if (ev == LIBSCMP_OBS_BLOCK) {
            snprintf(err, errlen, "%s: block %lu has no place in %s", binpath,
                     (unsigned long)o.blkno, path);
            r = -2;
        }
    }
    libscmp_obs_free(&o);
    if (fclose(m) && !r) {
        snprintf(err, errlen, "%s: out of memory", path);
        r = -1;
    }
    if (r) {
        free(*out);
        *out = NULL;
        *outlen = 0;
    }
    return r;
}

int libscmp_parse_run(struct libscmp_file *f, const char *text, size_t len, const char *path,
                      const void *bin, size_t binlen, const char *binpath, char *err,
                      size_t errlen)
{
    char *merged;
    size_t mlen;
    int r = libscmp_bin_merge(text, len, path, bin, binlen, binpath, &merged, &mlen, err, errlen);

    if (r) {
        memset(f, 0, sizeof *f);
        return r;
    }
    r = libscmp_parse(f, merged, mlen, path, LIBSCMP_RUN, err, errlen);
    free(merged);
    if (!r && bin) {
        if (!(f->bin = malloc(binlen ? binlen : 1)) || !(f->binpath = strdup(binpath))) {
            snprintf(err, errlen, "%s: out of memory", binpath);
            return -1;
        }
        memcpy(f->bin, bin, binlen);
        f->binlen = binlen;
    }
    return r;
}

/* --- the dump and the comparison of two runs ---------------------------- */

long libscmp_bin_dump(const void *bin, size_t binlen, const char *binpath, const char *what,
                      FILE *out, char *err, size_t errlen)
{
    struct libscmp_obs o;
    unsigned long block = 0;
    long lines = 0;
    int ev;

    if (!strncmp(what, "BLOCK:", 6)) {
        char *x;

        block = strtoul(what + 6, &x, 10);
        if (*x || !block)
            block = 0;
    }
    if ((ev = libscmp_obs_begin(&o, bin, binlen, binpath, err, errlen)))
        return ev;
    while ((ev = libscmp_obs_next(&o, err, errlen)) > 0) {
        if (ev != LIBSCMP_OBS)
            continue;
        if (block) {
            if (o.block == block) {
                fprintf(out, "%s\t%s\n", o.key, o.val);
                lines++;
            } else if (o.block > block)
                break;
        } else if (o.type == 0x11 && !strcmp(o.key, what)) {
            uint32_t at, k;

            for (at = 0; at < o.nbytes; at += 32) {
                fprintf(out, "%s\tat $%04lX ", o.key, (unsigned long)at);
                for (k = at; k < o.nbytes && k < at + 32; k++)
                    fprintf(out, "%c%c", hexdig[o.bytes[k] >> 4], hexdig[o.bytes[k] & 15]);
                fputc('\n', out);
                lines++;
            }
        }
    }
    libscmp_obs_free(&o);
    return ev < 0 ? ev : lines;
}

int libscmp_bin_first_diff(const struct libscmp_file *a, const struct libscmp_file *b,
                           uint32_t block, const char *la, const char *lb, FILE *out, char *err,
                           size_t errlen)
{
    struct libscmp_obs x, y;
    int r = 0, ea, eb;

    if ((ea = libscmp_obs_begin(&x, a->bin, a->binlen, a->binpath, err, errlen)))
        return ea;
    if ((eb = libscmp_obs_begin(&y, b->bin, b->binlen, b->binpath, err, errlen)))
        return eb;
    for (;;) {
        do
            ea = libscmp_obs_next(&x, err, errlen);
        while (ea == LIBSCMP_OBS_BLOCK);
        do
            eb = libscmp_obs_next(&y, err, errlen);
        while (eb == LIBSCMP_OBS_BLOCK);
        if (ea < 0 || eb < 0) {
            r = ea < 0 ? ea : eb;
            break;
        }
        if (ea == LIBSCMP_OBS_END && eb == LIBSCMP_OBS_END)
            break;
        if (ea == LIBSCMP_OBS_END || eb == LIBSCMP_OBS_END) {
            const struct libscmp_obs *h = ea == LIBSCMP_OBS ? &x : &y;

            if (h->block > block)
                break;
            fprintf(out, "%s: BLOCK:%lu: first differing observation %s: only in %s (%s)\n",
                    a->name, (unsigned long)block, h->key, h == &x ? la : lb, h->val);
            r = 1;
            break;
        }
        if (x.block > block && y.block > block)
            break;
        if (x.block < block && y.block < block)
            continue;
        if (x.keylen != y.keylen || memcmp(x.key, y.key, x.keylen)) {
            fprintf(out, "%s: BLOCK:%lu: first differing observation: %s %s (%s), %s %s (%s)\n",
                    a->name, (unsigned long)block, la, x.key, x.val, lb, y.key, y.val);
            r = 1;
            break;
        }
        if (x.vallen != y.vallen || memcmp(x.val, y.val, x.vallen)) {
            fprintf(out, "%s: BLOCK:%lu: first differing observation %s: %s %s, %s %s\n",
                    a->name, (unsigned long)block, x.key, la, x.val, lb, y.val);
            r = 1;
            break;
        }
    }
    if (!r)
        fprintf(out, "%s: BLOCK:%lu: no differing observation (%s and %s)\n", a->name,
                (unsigned long)block, la, lb);
    libscmp_obs_free(&x);
    libscmp_obs_free(&y);
    return r;
}

/* the NAMEs of DIR/NAME.bin into l; -1 = error (on err) */
static int bin_names(struct libscmp_names *l, const char *dir, FILE *err)
{
    DIR *d = opendir(dir);
    struct dirent *de;

    if (!d) {
        fprintf(err, "%s: cannot read the directory (%s)\n", dir, strerror(errno));
        return -1;
    }
    while ((de = readdir(d))) {
        size_t n = strlen(de->d_name);

        if (n > 4 && !strcmp(de->d_name + n - 4, ".bin")
            && libscmp_names_add(l, de->d_name, n - 4)) {
            closedir(d);
            fprintf(err, "out of memory\n");
            return -1;
        }
    }
    closedir(d);
    libscmp_names_sort(l);
    return 0;
}

int libscmp_dump_dirs(const char *run_dir, const char *what, const char *const *only,
                      size_t nonly, FILE *out, FILE *err)
{
    struct libscmp_names names = { 0 };
    size_t i, nwritten = 0;
    int status = 0;

    if (nonly ? libscmp_program_names(&names, run_dir, NULL, only, nonly, err)
              : bin_names(&names, run_dir, err)) {
        libscmp_names_free(&names);
        return 2;
    }
    for (i = 0; i < names.n && status != 2; i++) {
        char *bp = libscmp_path_ext(run_dir, names.v[i], ".bin");
        char *dp = libscmp_path_ext(run_dir, names.v[i], ".dump");
        char *text = NULL, msg[700];
        size_t len = 0, blen;
        unsigned char *bin = NULL;
        FILE *m;
        long n = 0;

        if (!bp || !dp || !(m = open_memstream(&text, &len))) {
            fprintf(err, "out of memory\n");
            status = 2;
        } else {
            if (!(bin = (unsigned char *)libscmp_read_file(bp, &blen))) {
                if (nonly)
                    fprintf(err, "%s: cannot read it\n", bp);
                n = nonly ? -3 : 0;
            } else
                n = libscmp_bin_dump(bin, blen, bp, what, m, msg, sizeof msg);
            fclose(m);
            if (n == -3)
                status = 2;
            else if (n < 0) {
                fprintf(err, "%s\n", n == -1 ? "out of memory" : msg);
                status = 2;
            } else if (n) {
                FILE *f = fopen(dp, "ab");

                if (!f || fwrite(text, 1, len, f) != len || fclose(f)) {
                    fprintf(err, "%s: cannot write it (%s)\n", dp, strerror(errno));
                    status = 2;
                } else {
                    fprintf(out, "%s: %ld lines of %s\n", dp, n, what);
                    nwritten++;
                }
            }
        }
        free(bin);
        free(text);
        free(bp);
        free(dp);
    }
    if (!status && !nwritten) {
        fprintf(out, "libs-dump %s: in no NAME.bin of %s\n", what, run_dir);
        status = 1;
    }
    libscmp_names_free(&names);
    return status;
}

int libscmp_list_blocks(const struct libscmp_file *run, const struct libscmp_file *exp,
                        enum libscmp_set set, const char *run_dir, const char *other_dir,
                        FILE *out, FILE *err)
{
    uint32_t blocks[LIBSCMP_LISTED];
    FILE *files[LIBSCMP_LISTED] = { NULL };
    char *paths[LIBSCMP_LISTED] = { NULL };
    unsigned shown[LIBSCMP_LISTED] = { 0 };
    struct libscmp_obs o;
    size_t i, nb = 0, k;
    char msg[700];
    int status = 0, ev;

    for (i = 0; i < run->ncases && nb < LIBSCMP_LISTED; i++) {
        const struct libscmp_case *c = &run->cases[i];
        const char *key = run->text + c->key;
        const struct libscmp_entry *e;
        int32_t j;

        if (c->keylen <= 6 || memcmp(key, "BLOCK:", 6))
            continue;
        e = libscmp_find(exp, key, c->keylen);
        if (e && (j = libscmp_applies(e, set)) >= 0 && exp->cases[j].vallen == c->vallen
            && !memcmp(exp->text + exp->cases[j].val, run->text + c->val, c->vallen))
            continue;
        blocks[nb++] = (uint32_t)strtoul(key + 6, NULL, 10);
    }
    if (!nb)
        return 0;
    for (k = 0; k < nb && !status; k++) {
        char ext[32];

        snprintf(ext, sizeof ext, ".block-%lu.obs", (unsigned long)blocks[k]);
        if (!(paths[k] = libscmp_path_ext(run_dir, run->name, ext))
            || !(files[k] = fopen(paths[k], "wb"))) {
            fprintf(err, "%s: cannot write it (%s)\n", paths[k] ? paths[k] : run->name,
                    strerror(errno));
            status = -1;
        }
    }
    if (!status && libscmp_obs_begin(&o, run->bin, run->binlen, run->binpath, msg, sizeof msg)) {
        fprintf(err, "%s\n", msg);
        status = -1;
    }
    if (!status) {
        /* the observation lines of the blocks, the first three also on out */
        while ((ev = libscmp_obs_next(&o, msg, sizeof msg)) > 0) {
            if (ev != LIBSCMP_OBS)
                continue;
            for (k = 0; k < nb; k++)
                if (o.block == blocks[k]) {
                    fprintf(files[k], "%s\t%s\n", o.key, o.val);
                    if (!shown[k]++)
                        fprintf(out, "%s: BLOCK:%lu: its observations in %s, the first:\n",
                                run->name, (unsigned long)blocks[k], paths[k]);
                    if (shown[k] <= 3)
                        fprintf(out, "%s:   %s\t%s\n", run->name, o.key, o.val);
                }
        }
        if (ev < 0) {
            fprintf(err, "%s\n", ev == -1 ? "out of memory" : msg);
            status = -1;
        }
        libscmp_obs_free(&o);
    }
    for (k = 0; k < nb; k++)
        if (files[k] && fclose(files[k]) && !status) {
            fprintf(err, "%s: cannot write it (%s)\n", paths[k], strerror(errno));
            status = -1;
        }
    if (!status && other_dir) {
        char *bp = libscmp_path_ext(other_dir, run->name, ".bin");
        struct libscmp_file other;

        memset(&other, 0, sizeof other);
        memcpy(other.name, run->name, sizeof other.name);
        if (!bp) {
            fprintf(err, "out of memory\n");
            status = -1;
        } else if (!(other.bin = (unsigned char *)libscmp_read_file(bp, &other.binlen)))
            fprintf(out, "%s: no %s, no comparison with the other run\n", run->name, bp);
        else {
            other.binpath = bp;
            bp = NULL;
            for (k = 0; k < nb && !status; k++) {
                int r = libscmp_bin_first_diff(run, &other, blocks[k], "this run",
                                               "the other run", out, msg, sizeof msg);

                if (r < 0) {
                    fprintf(err, "%s\n", r == -1 ? "out of memory" : msg);
                    status = -1;
                }
            }
        }
        free(bp);
        libscmp_free(&other);
    }
    for (k = 0; k < nb; k++)
        free(paths[k]);
    return status;
}
