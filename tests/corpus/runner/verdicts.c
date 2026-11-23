/*
 * verdicts.c - the verdict store of the bundles (verdicts.h).
 */
#define _GNU_SOURCE
#include "verdicts.h"
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rundir.h"
#include "sha256.h"
#include "util.h"

/* ------------------------------------------------------------ the key */

void vstore_cache_free(struct vstore_cache *c)
{
    for (size_t i = 0; c && i < c->n; i++)
        free(c->path[i]);
    if (c) {
        free(c->path);
        free(c->hex);
        memset(c, 0, sizeof *c);
    }
}

static int digest_path(struct vstore_cache *c, const char *path, char hex[65]);

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

/* the digest of a directory: its names in order, each with its digest */
static int digest_dir(struct vstore_cache *c, const char *path, char hex[65])
{
    DIR *d = opendir(path);
    struct dirent *de;
    char **names = NULL;
    size_t n = 0, cap = 0;
    struct sha256 s;
    int r = 0;

    if (!d) {
        snprintf(hex, 65, "missing");
        return 0;
    }
    while ((de = readdir(d))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        if (n == cap) {
            char **v = realloc(names, (cap = cap ? 2 * cap : 16) * sizeof *v);

            if (!v) {
                r = -1;
                break;
            }
            names = v;
        }
        if (!(names[n] = strdup(de->d_name))) {
            r = -1;
            break;
        }
        n++;
    }
    closedir(d);
    if (!r)
        qsort(names, n, sizeof *names, cmp_str);
    sha256_init(&s);
    sha256_update(&s, "dir\n", 4);
    for (size_t i = 0; !r && i < n; i++) {
        char *p = str_printf("%s/%s", path, names[i]), h[65];

        if (!p || digest_path(c, p, h)) {
            free(p);
            r = -1;
            break;
        }
        free(p);
        sha256_update(&s, names[i], strlen(names[i]) + 1);
        sha256_update(&s, h, strlen(h));
        sha256_update(&s, "\n", 1);
    }
    for (size_t i = 0; i < n; i++)
        free(names[i]);
    free(names);
    if (!r)
        sha256_hex(&s, hex);
    return r;
}

/* the digest of a file (its bytes), of a directory, or "missing" */
static int digest_path(struct vstore_cache *c, const char *path, char hex[65])
{
    struct stat st;

    for (size_t i = 0; c && i < c->n; i++)
        if (!strcmp(c->path[i], path)) {
            memcpy(hex, c->hex[i], 65);
            return 0;
        }
    if (stat(path, &st))
        snprintf(hex, 65, "missing");
    else if (S_ISDIR(st.st_mode)) {
        if (digest_dir(c, path, hex))
            return -1;
    } else {
        size_t len;
        uint8_t *b = read_file(path, &len);

        if (b) {
            sha256_hex_of(b, len, hex);
            free(b);
        } else
            snprintf(hex, 65, "unreadable");
    }
    if (c) {
        if (c->n == c->cap) {
            size_t cap = c->cap ? 2 * c->cap : 64;
            char **p = realloc(c->path, cap * sizeof *p);

            if (!p)
                return -1;
            c->path = p;
            {
                char (*h)[65] = realloc(c->hex, cap * sizeof *h);

                if (!h)
                    return -1;
                c->hex = h;
            }
            c->cap = cap;
        }
        if (!(c->path[c->n] = strdup(path)))
            return -1;
        memcpy(c->hex[c->n++], hex, 65);
    }
    return 0;
}

static void line(struct text *t, const char *name, const char *value)
{
    text_add(t, "%s=%zu:%s\n", name, strlen(value), value);
}

static void file_line(struct vstore_cache *c, struct text *t, const char *what, const char *path)
{
    char hex[65], *name = str_printf("%s:%s", what, path);

    if (!name || digest_path(c, path, hex))
        t->bad = 1;
    else
        line(t, name, hex);
    free(name);
}

int vstore_key(struct vstore_cache *c, const struct vstore_keyin *in, char key[65])
{
    struct text t = { 0 };
    const struct tc_config *conf = in->conf;

    line(&t, "v", "1");
    line(&t, "test", in->test ? in->test : "");
    line(&t, "side", in->side ? in->side : "");
    line(&t, "case", in->case_name ? in->case_name : "");
    file_line(c, &t, "source", conf->source);
    for (size_t i = 0; i < conf->nfiles; i++)
        file_line(c, &t, "conf", conf->files[i]);
    {
        char *d = str_printf("%s/%s.test", conf->dir, conf->id);

        if (!d)
            t.bad = 1;
        else
            file_line(c, &t, "testdir", d);
        free(d);
    }
    if (in->env) {
        file_line(c, &t, "env", in->env->file ? in->env->file : "");
        for (size_t i = 0; i < in->env->nrun; i++) {
            const char *v = in->env->run[i].value;

            /* a value of several paths: "OBJ|REF|VOUT" */
            while (v && *v) {
                size_t n = strcspn(v, "|");
                char *p = strndup(v, n);
                struct stat st;

                if (!p)
                    t.bad = 1;
                else if (*p == '/' && !stat(p, &st) && S_ISREG(st.st_mode)) {
                    char *what = str_printf("runner.%s", in->env->run[i].name);

                    if (!what)
                        t.bad = 1;
                    else
                        file_line(c, &t, what, p);
                    free(what);
                }
                free(p);
                v += n + (v[n] == '|');
            }
        }
    }
    for (size_t i = 0; i < in->nstaged; i++)
        if (in->staged[i])
            file_line(c, &t, "staged", in->staged[i]);
    file_line(c, &t, "self", in->self ? in->self : "/proc/self/exe");
    line(&t, "emulator", in->emulator ? in->emulator : "");
    if (t.bad) {
        free(t.p);
        return -1;
    }
    sha256_hex_of(t.p, t.n, key);
    free(t.p);
    return 0;
}

/* ------------------------------------------------------------ the store */

char *vstore_dir(const struct tc_env *env)
{
    char *run = rundir_path(env ? tc_env_runner(env, "run-dir") : NULL), *d;

    if (!run)
        return NULL;
    d = str_printf("%s/" VSTORE_DIR, run);
    free(run);
    return d;
}

char *vstore_entry(const char *key, const char *bundle, int status, const char *block)
{
    return str_printf(VSTORE_MAGIC "\nkey=%s\nbundle=%s\nstatus=%d\n%s", key, bundle, status, block);
}

static int name_ok(const char *test, char *err, size_t errlen)
{
    if (!*test || *test == '.' || strchr(test, '/')) {
        snprintf(err, errlen, "\"%s\" is no name of a case test", test);
        return 0;
    }
    return 1;
}

int vstore_put(const char *dir, const char *test, const char *text, char *err, size_t errlen)
{
    char *tmp = NULL, *dst = NULL;
    int r = -1;

    if (!name_ok(test, err, errlen))
        return -1;
    if (!(tmp = str_printf("%s/%s.new-%ld", dir, test, (long)getpid())) || !(dst = str_printf("%s/%s", dir, test))) {
        snprintf(err, errlen, "out of memory");
        goto out;
    }
    if (mkdir_p(dir) || write_file(tmp, text, strlen(text))) {
        snprintf(err, errlen, "cannot write %s", tmp);
        unlink(tmp);
        goto out;
    }
    if (rename(tmp, dst)) {
        snprintf(err, errlen, "cannot rename %s to %s: %s", tmp, dst, strerror(errno));
        unlink(tmp);
        goto out;
    }
    r = 0;
out:
    free(tmp);
    free(dst);
    return r;
}

void vstore_verdict_free(struct vstore_verdict *v)
{
    free(v->block);
    v->block = NULL;
}

static int is_hex64(const char *s)
{
    for (int i = 0; i < 64; i++)
        if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f')))
            return 0;
    return 1;
}

int vstore_parse(const char *text, size_t len, const char **key, const char **bundle, size_t *bundlelen,
                 int *status, const char **block, char *err, size_t errlen)
{
    const char *p = text, *end = text + len, *nl;
    long st;
    char *e;

    if (len < strlen(VSTORE_MAGIC) + 1 || strncmp(p, "bundle-verdict ", 15)) {
        snprintf(err, errlen, "not an entry");
        return -1;
    }
    if (strncmp(p, VSTORE_MAGIC "\n", strlen(VSTORE_MAGIC) + 1)) {
        snprintf(err, errlen, "version: not v=1");
        return -1;
    }
    p += strlen(VSTORE_MAGIC) + 1;
    if (end - p < 69 || strncmp(p, "key=", 4) || !is_hex64(p + 4) || p[68] != '\n') {
        snprintf(err, errlen, "no key");
        return -1;
    }
    *key = p + 4;
    p += 69;
    if (end - p < 8 || strncmp(p, "bundle=", 7) || !(nl = memchr(p, '\n', (size_t)(end - p))) || nl == p + 7) {
        snprintf(err, errlen, "no bundle");
        return -1;
    }
    *bundle = p + 7;
    *bundlelen = (size_t)(nl - p - 7);
    p = nl + 1;
    if (end - p < 9 || strncmp(p, "status=", 7) || !(nl = memchr(p, '\n', (size_t)(end - p)))) {
        snprintf(err, errlen, "no status");
        return -1;
    }
    errno = 0;
    st = strtol(p + 7, &e, 10);
    if (e != nl || e == p + 7 || errno || st < 0 || st > 255) {
        snprintf(err, errlen, "no status");
        return -1;
    }
    *status = (int)st;
    p = nl + 1;
    if (p == end || end[-1] != '\n' || memchr(p, 0, (size_t)(end - p))) {
        snprintf(err, errlen, "no report block");
        return -1;
    }
    *block = p;
    return 0;
}

static void rmdir_empty(const char *dir)
{
    rmdir(dir);         /* fails while other entries are there */
}

int vstore_claim(const char *dir, const char *test, const char *key, const char *bundle, struct vstore_verdict *v,
                 char *err, size_t errlen)
{
    char *src = NULL, *own = NULL;
    uint8_t *text = NULL;
    size_t len;
    int r = -1;

    v->block = NULL;
    v->status = 0;
    if (!name_ok(test, err, errlen))
        return -1;
    if (!(src = str_printf("%s/%s", dir, test)) || !(own = str_printf("%s/%s.claim-%ld", dir, test, (long)getpid()))) {
        snprintf(err, errlen, "out of memory");
        goto out;
    }
    if (rename(src, own)) {
        if (errno == ENOENT || errno == ENOTDIR) {
            r = VSTORE_NONE;
            goto out;
        }
        snprintf(err, errlen, "cannot rename %s: %s", src, strerror(errno));
        goto out;
    }
    text = read_file(own, &len);
    unlink(own);
    rmdir_empty(dir);
    if (!text) {
        snprintf(err, errlen, "cannot read %s", own);
        goto out;
    }
    {
        const char *k, *b, *block;
        size_t blen;
        int st;

        if (vstore_parse((const char *)text, len, &k, &b, &blen, &st, &block, err, errlen)) {
            r = VSTORE_MALFORMED;
            goto out;
        }
        if (!key || strncmp(k, key, 64) || strlen(key) != 64 || blen != strlen(bundle) || strncmp(b, bundle, blen)) {
            r = VSTORE_STALE;
            goto out;
        }
        if (!(v->block = strdup(block))) {
            snprintf(err, errlen, "out of memory");
            goto out;
        }
        v->status = st;
        r = VSTORE_SERVED;
    }
out:
    free(text);
    free(src);
    free(own);
    return r;
}

/* the PID of a name TEST.new-PID or TEST.claim-PID, 0 = not one of test's */
static long file_pid(const char *name, const char *test)
{
    size_t n = strlen(test);
    const char *s;
    char *e;
    long pid;

    if (strncmp(name, test, n) || name[n] != '.')
        return 0;
    s = name + n + 1;
    if (!strncmp(s, "new-", 4))
        s += 4;
    else if (!strncmp(s, "claim-", 6))
        s += 6;
    else
        return 0;
    pid = strtol(s, &e, 10);
    return *e || e == s || pid <= 0 ? 0 : pid;
}

int vstore_clear(const char *dir, const char *test)
{
    DIR *d;
    struct dirent *de;
    char *p = str_printf("%s/%s", dir, test);
    int r = 0;

    if (!p)
        return -1;
    if (unlink(p) && errno != ENOENT)
        r = -1;
    free(p);
    if (!(d = opendir(dir)))
        return errno == ENOENT ? r : -1;
    while ((de = readdir(d))) {
        long pid = file_pid(de->d_name, test);

        /* a process that lives (or that we may not signal) still uses it */
        if (!pid || !kill((pid_t)pid, 0) || errno == EPERM)
            continue;
        if (!(p = str_printf("%s/%s", dir, de->d_name)) || (unlink(p) && errno != ENOENT))
            r = -1;
        free(p);
    }
    closedir(d);
    return r;
}
