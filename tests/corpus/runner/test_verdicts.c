/*
 * test_verdicts.c - the verdict store of the bundles (verdicts.h) without
 * the emulator: an entry written and claimed, a claim of a missing entry,
 * two claimants at once (one gets it), an entry of other inputs, the
 * malformed entries (deliberately broken input: cut after its key line, a
 * wrong version, no status line: each named and not served), the cleanup
 * at a bundle's start that keeps the file of a live process, and the key
 * (it changes with one byte of the source, of a tests.toml, of a file
 * below ID.test/, of the environment file, of a [runner] file, of a staged
 * file or of the runner program, and not with an unrelated file).
 *   test_verdicts WORKDIR
 */
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "testconf.h"
#include "util.h"
#include "verdicts.h"

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%s:%d: CHECK failed: %s\n", \
                                          __FILE__, __LINE__, #c); failures++; } } while (0)

static char work[1024];

static char *at(const char *rel)
{
    return str_printf("%s/%s", work, rel);
}

static void put(const char *rel, const char *text)
{
    char *p = at(rel), *slash = p ? strrchr(p, '/') : NULL;

    if (slash) {
        *slash = 0;
        mkdir_p(p);
        *slash = '/';
    }
    CHECK(p && write_file(p, text, strlen(text)) == 0);
    free(p);
}

static int exists(const char *path)
{
    return access(path, F_OK) == 0;
}

#define KEY_A "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
#define KEY_B "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
#define BLOCK "== x.test.bas, case a, side port: port,clib (c0)\nx.test.bas: case a: PASSED\n"

static void test_round_trip(void)
{
    char *dir = at("store1"), *e = vstore_entry(KEY_A, "b_bundle_1", 0, BLOCK), *f = at("store1/t__a");
    char err[300];
    struct vstore_verdict v;

    CHECK(dir && e && f);
    CHECK(vstore_put(dir, "t__a", e, err, sizeof err) == 0);
    CHECK(exists(f));
    CHECK(vstore_claim(dir, "t__a", KEY_A, "b_bundle_1", &v, err, sizeof err) == VSTORE_SERVED);
    CHECK(v.status == 0 && v.block && !strcmp(v.block, BLOCK));
    vstore_verdict_free(&v);
    /* read once: gone, and the empty directory with it */
    CHECK(!exists(f) && !exists(dir));
    CHECK(vstore_claim(dir, "t__a", KEY_A, "b_bundle_1", &v, err, sizeof err) == VSTORE_NONE);
    /* REPRODUCED: its status */
    free(e);
    e = vstore_entry(KEY_A, "b_bundle_1", 1, BLOCK);
    CHECK(vstore_put(dir, "t__a", e, err, sizeof err) == 0);
    CHECK(vstore_claim(dir, "t__a", KEY_A, "b_bundle_1", &v, err, sizeof err) == VSTORE_SERVED && v.status == 1);
    vstore_verdict_free(&v);
    /* a name that is no case test's */
    CHECK(vstore_put(dir, "../x", e, err, sizeof err) == -1 && strstr(err, "no name"));
    CHECK(vstore_put(dir, ".hidden", e, err, sizeof err) == -1);
    free(e);
    free(f);
    free(dir);
}

static void test_missing(void)
{
    char *dir = at("store-none"), err[300];
    struct vstore_verdict v;

    CHECK(vstore_claim(dir, "t__a", KEY_A, "b", &v, err, sizeof err) == VSTORE_NONE);
    CHECK(!v.block);
    free(dir);
}

static void test_two_claimants(void)
{
    char *dir = at("store2"), *e = vstore_entry(KEY_A, "b", 0, BLOCK), err[300];
    int served = 0, none = 0;
    pid_t pid[2];
    int pfd[2];

    CHECK(vstore_put(dir, "t__a", e, err, sizeof err) == 0);
    CHECK(pipe(pfd) == 0);
    for (int i = 0; i < 2; i++)
        if ((pid[i] = fork()) == 0) {
            struct vstore_verdict v;
            char c;

            close(pfd[1]);
            /* both start when the pipe closes */
            (void)!read(pfd[0], &c, 1);
            _exit(vstore_claim(dir, "t__a", KEY_A, "b", &v, err, sizeof err));
        }
    close(pfd[0]);
    close(pfd[1]);
    for (int i = 0; i < 2; i++) {
        int st;

        CHECK(waitpid(pid[i], &st, 0) == pid[i] && WIFEXITED(st));
        served += WEXITSTATUS(st) == VSTORE_SERVED;
        none += WEXITSTATUS(st) == VSTORE_NONE;
    }
    CHECK(served == 1 && none == 1);
    free(e);
    free(dir);
}

static void test_foreign(void)
{
    char *dir = at("store3"), *e = vstore_entry(KEY_A, "b", 0, BLOCK), *f = at("store3/t__a"), err[300];
    struct vstore_verdict v;

    /* other inputs: stale, and removed */
    CHECK(vstore_put(dir, "t__a", e, err, sizeof err) == 0);
    CHECK(vstore_claim(dir, "t__a", KEY_B, "b", &v, err, sizeof err) == VSTORE_STALE && !v.block);
    CHECK(!exists(f));
    /* another bundle's */
    CHECK(vstore_put(dir, "t__a", e, err, sizeof err) == 0);
    CHECK(vstore_claim(dir, "t__a", KEY_A, "c", &v, err, sizeof err) == VSTORE_STALE);
    /* no key of the case test's inputs */
    CHECK(vstore_put(dir, "t__a", e, err, sizeof err) == 0);
    CHECK(vstore_claim(dir, "t__a", NULL, "b", &v, err, sizeof err) == VSTORE_STALE);
    CHECK(!exists(f));
    free(e);
    free(f);
    free(dir);
}

/* the broken-input test of the entry's check */
static void test_malformed(void)
{
    static const struct {
        const char *text, *why;
    } bad[] = {
        { VSTORE_MAGIC "\nkey=", "no key" },                                   /* cut after key= */
        { VSTORE_MAGIC "\nkey=" KEY_A "\n", "no bundle" },                     /* cut after the key line */
        { "bundle-verdict v=2\nkey=" KEY_A "\nbundle=b\nstatus=0\n" BLOCK, "version" },
        { VSTORE_MAGIC "\nkey=" KEY_A "\nbundle=b\n" BLOCK, "no status" },
        { VSTORE_MAGIC "\nkey=" KEY_A "\nbundle=b\nstatus=x\n" BLOCK, "no status" },
        { VSTORE_MAGIC "\nkey=" KEY_A "\nbundle=b\nstatus=0\n", "no report block" },
        { VSTORE_MAGIC "\nkey=" KEY_A "\nbundle=b\nstatus=0\n== cut", "no report block" },
        { VSTORE_MAGIC "\nkey=xyz\nbundle=b\nstatus=0\n" BLOCK, "no key" },
        { "something else\n", "not an entry" },
        { "", "not an entry" },
    };
    char *dir = at("store4"), *f = at("store4/t__a"), err[300];

    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        struct vstore_verdict v;

        mkdir_p(dir);
        CHECK(write_file(f, bad[i].text, strlen(bad[i].text)) == 0);
        err[0] = 0;
        CHECK(vstore_claim(dir, "t__a", KEY_A, "b", &v, err, sizeof err) == VSTORE_MALFORMED);
        if (!strstr(err, bad[i].why))
            fprintf(stderr, "entry %zu: \"%s\", wanted \"%s\"\n", i, err, bad[i].why);
        CHECK(strstr(err, bad[i].why) != NULL);
        CHECK(!v.block && !exists(f));
    }
    free(f);
    free(dir);
}

static void test_clear(void)
{
    char *dir = at("store5"), name[300], err[300];
    char *e = vstore_entry(KEY_A, "b", 0, BLOCK);
    pid_t dead = fork();
    int st;

    if (dead == 0)
        _exit(0);
    CHECK(waitpid(dead, &st, 0) == dead);
    CHECK(vstore_put(dir, "t__a", e, err, sizeof err) == 0);
    /* a claim of this (living) process, the files of a process that ended */
    snprintf(name, sizeof name, "t__a.claim-%ld", (long)getpid());
    put("store5/x", "");
    {
        char *live = at("store5/x"), *to = str_printf("%s/%s", dir, name);

        CHECK(rename(live, to) == 0);
        free(live);
        free(to);
    }
    snprintf(name, sizeof name, "store5/t__a.new-%ld", (long)dead);
    put(name, e);
    snprintf(name, sizeof name, "store5/t__a.claim-%ld", (long)dead);
    put(name, e);
    put("store5/t__ab", e);         /* another test's entry */
    put("store5/t__a.other", e);    /* no file of the store's */
    CHECK(vstore_clear(dir, "t__a") == 0);
    {
        char *p = at("store5/t__a");

        CHECK(!exists(p));
        free(p);
        snprintf(name, sizeof name, "store5/t__a.claim-%ld", (long)getpid());
        p = at(name);
        CHECK(exists(p));
        free(p);
        snprintf(name, sizeof name, "store5/t__a.new-%ld", (long)dead);
        p = at(name);
        CHECK(!exists(p));
        free(p);
        snprintf(name, sizeof name, "store5/t__a.claim-%ld", (long)dead);
        p = at(name);
        CHECK(!exists(p));
        free(p);
        p = at("store5/t__ab");
        CHECK(exists(p));
        free(p);
        p = at("store5/t__a.other");
        CHECK(exists(p));
        free(p);
    }
    /* nothing there */
    {
        char *none = at("store-none5");

        CHECK(vstore_clear(none, "t__a") == 0);
        free(none);
    }
    free(e);
    free(dir);
}

/* the store's place: $BB2C_TEST_RUN, else run-dir */
static void test_dir(void)
{
    struct tc_env env = { 0 };
    char err[300], *d;

    CHECK(tc_env_parse("env.toml", "[runner]\nrun-dir = \"/r/default\"\n", &env, err, sizeof err) == 0);
    unsetenv("BB2C_TEST_RUN");
    d = vstore_dir(&env);
    CHECK(d && !strcmp(d, "/r/default/verdicts"));
    free(d);
    setenv("BB2C_TEST_RUN", "/r/own", 1);
    d = vstore_dir(&env);
    CHECK(d && !strcmp(d, "/r/own/verdicts"));
    free(d);
    unsetenv("BB2C_TEST_RUN");
    tc_env_free(&env);
    CHECK(tc_env_parse("env.toml", "[runner]\nwork = \"/w\"\n", &env, err, sizeof err) == 0);
    CHECK(vstore_dir(&env) == NULL);
    tc_env_free(&env);
}

/* ------------------------------------------------------------ the key */

static char *key_of(void)
{
    char *envf = at("k/env.toml"), *src = at("k/t/x.test.bas"), err[600];
    char *self = at("k/runner"), *staged = at("k/staged.txt");
    const char *st[] = { staged };
    struct tc_env env = { 0 };
    struct tc_config conf = { 0 };
    struct vstore_keyin in = { 0 };
    char key[65], *r = NULL;

    if (tc_env_load(envf, &env, err, sizeof err) || tc_resolve(src, &env, &conf, err, sizeof err))
        fprintf(stderr, "key: %s\n", err);
    else {
        in.test = "t_x__a";
        in.side = "port";
        in.case_name = "a";
        in.conf = &conf;
        in.env = &env;
        in.staged = st;
        in.nstaged = 1;
        in.self = self;
        in.emulator = "emu";
        if (!vstore_key(NULL, &in, key))
            r = strdup(key);
    }
    tc_free(&conf);
    tc_env_free(&env);
    free(envf);
    free(src);
    free(self);
    free(staged);
    return r;
}

static void test_key(void)
{
    static const char *const touch[] = {
        "k/t/x.test.bas", "k/t/tests.toml", "k/tests.toml", "k/t/x.test.toml", "k/t/x.test/sub/data.txt",
        "k/env.toml", "k/b/blitz2", "k/b/rwe-ref.obj", "k/staged.txt", "k/runner",
    };
    char *k0, *k1;
    struct vstore_cache cache = { 0 };

    {
        char *envtext = str_printf("top = \"%s/k\"\n[runner]\nblitz2 = \"%s/k/b/blitz2\"\n"
                                   "rwe-x = \"%s/k/b/rwe.obj|%s/k/b/rwe-ref.obj|%s/k/b\"\nwork = \"%s/k/w\"\n"
                                   "timeout = \"90\"\n", work, work, work, work, work, work);

        put("k/env.toml", envtext);
        free(envtext);
    }
    put("k/tests.toml", "root = true\n");
    put("k/t/tests.toml", "timeout = 30\n");
    put("k/t/x.test.toml", "timeout = 60\n");
    put("k/t/x.test.bas", "!Case a\n!EndCase\n");
    put("k/t/x.test/sub/data.txt", "data\n");
    put("k/t/unrelated.txt", "u\n");
    put("k/t/y.test.bas", "!Case a\n!EndCase\n");
    put("k/b/blitz2", "B2");
    put("k/b/rwe.obj", "R");
    put("k/b/rwe-ref.obj", "RR");
    put("k/b/other", "O");
    put("k/staged.txt", "S");
    put("k/runner", "RUNNER");
    k0 = key_of();
    CHECK(k0 && strlen(k0) == 64);
    /* the same inputs, the same key; with a cache too */
    k1 = key_of();
    CHECK(k1 && k0 && !strcmp(k0, k1));
    free(k1);
    for (size_t i = 0; i < sizeof touch / sizeof touch[0]; i++) {
        char *p = at(touch[i]);
        size_t len;
        char *old = p ? (char *)read_file(p, &len) : NULL, *nw = old ? str_printf("%s\n# one byte more\n", old) : NULL;

        CHECK(nw && write_file(p, nw, strlen(nw)) == 0);
        k1 = key_of();
        if (k1 && k0 && !strcmp(k0, k1))
            fprintf(stderr, "key: unchanged by %s\n", touch[i]);
        CHECK(k1 && k0 && strcmp(k0, k1));
        free(k1);
        CHECK(old && write_file(p, old, len) == 0);
        k1 = key_of();
        CHECK(k1 && k0 && !strcmp(k0, k1));
        free(k1);
        free(old);
        free(nw);
        free(p);
    }
    /* unrelated files: another source, a file of the directory, a file
       below bin/ no value names */
    put("k/t/unrelated.txt", "changed\n");
    put("k/t/y.test.bas", "!Case b\n!EndCase\n");
    put("k/b/other", "changed");
    k1 = key_of();
    CHECK(k1 && k0 && !strcmp(k0, k1));
    free(k1);
    /* a file below ID.test/ added */
    put("k/t/x.test/new.txt", "n");
    k1 = key_of();
    CHECK(k1 && k0 && strcmp(k0, k1));
    free(k1);
    {
        char *p = at("k/t/x.test/new.txt");

        unlink(p);
        free(p);
    }
    /* a cache gives the same key */
    {
        char *envf = at("k/env.toml"), *src = at("k/t/x.test.bas"), err[600], key[65], key2[65];
        char *self = at("k/runner"), *staged = at("k/staged.txt");
        const char *st[] = { staged };
        struct tc_env env = { 0 };
        struct tc_config conf = { 0 };
        struct vstore_keyin in = { "t_x__a", "port", "a", &conf, &env, st, 1, self, "emu" };

        CHECK(tc_env_load(envf, &env, err, sizeof err) == 0 && tc_resolve(src, &env, &conf, err, sizeof err) == 0);
        CHECK(vstore_key(&cache, &in, key) == 0 && vstore_key(&cache, &in, key2) == 0);
        CHECK(!strcmp(key, k0) && !strcmp(key2, k0));
        /* the test, the case, the emulator */
        in.test = "t_x__b";
        CHECK(vstore_key(&cache, &in, key) == 0 && strcmp(key, k0));
        in.test = "t_x__a";
        in.case_name = "b";
        CHECK(vstore_key(&cache, &in, key) == 0 && strcmp(key, k0));
        in.case_name = "a";
        in.emulator = "emu2";
        CHECK(vstore_key(&cache, &in, key) == 0 && strcmp(key, k0));
        tc_free(&conf);
        tc_env_free(&env);
        free(envf);
        free(src);
        free(self);
        free(staged);
    }
    vstore_cache_free(&cache);
    free(k0);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: test_verdicts WORKDIR\n");
        return 2;
    }
    snprintf(work, sizeof work, "%s", argv[1]);
    rm_tree(work);
    if (mkdir_p(work)) {
        fprintf(stderr, "cannot create %s\n", work);
        return 2;
    }
    test_round_trip();
    test_missing();
    test_two_claimants();
    test_foreign();
    test_malformed();
    test_clear();
    test_dir();
    test_key();
    if (!failures)
        rm_tree(work);
    printf("test_verdicts: %d failures\n", failures);
    return failures != 0;
}
