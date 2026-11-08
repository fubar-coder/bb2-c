/*
 * test_cdcache.c - the cache of a corpusrun cd pass (cdcache.h): a run with the
 * same inputs and unchanged outputs is skipped; a changed input, a changed
 * or missing output, a missing or broken record means a full run.
 */
#define _XOPEN_SOURCE 700       /* mkdtemp */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cdcache.h"
#include "util.h"

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%s:%d: CHECK failed: %s\n", \
                                          __FILE__, __LINE__, #c); failures++; } } while (0)

static char base[] = "/tmp/cdcache-test-XXXXXX";

static char *at(const char *rel)
{
    static char paths[16][512];
    static unsigned n;
    char *p = paths[n++ % 16];

    snprintf(p, sizeof paths[0], "%s/%s", base, rel);
    return p;
}

static void put(const char *rel, const char *text)
{
    char *p = at(rel), *slash = strrchr(p, '/');

    *slash = 0;
    mkdir_p(p);
    *slash = '/';
    CHECK(write_file(p, text, strlen(text)) == 0);
}

#define PASS "work=/w\nsource=RWE/a.bb|/o/a.obj\n"

/* the digest of the test's inputs: the pass text, two files, the "CD" */
static int digest(const char *text, char hex[65])
{
    const char *files[] = { at("blitz2"), at("deflibs") };
    const char *dirs[] = { at("cd") };

    return cdcache_digest(text, files, 2, dirs, 1, hex);
}

int main(void)
{
    char d0[65], d[65];
    const char *outs[2], *record;

    if (!mkdtemp(base)) {
        perror("mkdtemp");
        return 1;
    }
    /* (at() reuses its buffers) */
    outs[0] = strdup(at("out/table.tsv"));
    outs[1] = strdup(at("out/a.obj"));
    record = strdup(at("work/cache"));
    put("blitz2", "compiler");
    put("deflibs", "libs");
    put("cd/RWE/a.bb", "source a");
    put("cd/RWE/sub/b.bb", "source b");
    put("cd/BLITZLIBS/LIBNUMS.BB", "numbers");
    put("out/table.tsv", "table");
    put("out/a.obj", "object");
    mkdir_p(at("work"));

    CHECK(digest(PASS, d0) == 0 && strlen(d0) == 64);
    /* no record yet: a full run, then the record */
    CHECK(!cdcache_hit(record, d0, outs, 2));
    CHECK(cdcache_write(record, d0, outs, 2) == 0);
    /* the second run: the same inputs, the outputs there - skipped */
    CHECK(digest(PASS, d) == 0 && !strcmp(d, d0));
    CHECK(cdcache_hit(record, d, outs, 2));

    /* a changed input: a file below the CD, one of the files, the pass */
    put("cd/RWE/sub/b.bb", "source b, changed");
    CHECK(digest(PASS, d) == 0 && strcmp(d, d0));
    CHECK(!cdcache_hit(record, d, outs, 2));
    put("cd/RWE/sub/b.bb", "source b");
    CHECK(digest(PASS, d) == 0 && !strcmp(d, d0));
    put("deflibs", "libs2");
    CHECK(digest(PASS, d) == 0 && strcmp(d, d0));
    put("deflibs", "libs");
    CHECK(digest(PASS "timeout=9\n", d) == 0 && strcmp(d, d0));
    /* a new file below the CD, a renamed one */
    put("cd/RWE/c.bb", "");
    CHECK(digest(PASS, d) == 0 && strcmp(d, d0));
    CHECK(rename(at("cd/RWE/c.bb"), at("cd/RWE/d.bb")) == 0);
    {
        char d2[65];

        CHECK(digest(PASS, d2) == 0 && strcmp(d2, d0) && strcmp(d2, d));
    }
    CHECK(remove(at("cd/RWE/d.bb")) == 0);
    CHECK(digest(PASS, d) == 0 && !strcmp(d, d0));
    /* an input missing: no digest (a full run without a record) */
    CHECK(remove(at("blitz2")) == 0);
    CHECK(digest(PASS, d) != 0);
    put("blitz2", "compiler");
    {
        const char *dirs[] = { at("nocd") };

        CHECK(cdcache_digest(PASS, NULL, 0, dirs, 1, d) != 0);
    }
    CHECK(cdcache_hit(record, d0, outs, 2));

    /* an output changed or missing */
    put("out/a.obj", "object, changed");
    CHECK(!cdcache_hit(record, d0, outs, 2));
    put("out/a.obj", "object");
    CHECK(cdcache_hit(record, d0, outs, 2));
    CHECK(remove(at("out/table.tsv")) == 0);
    CHECK(!cdcache_hit(record, d0, outs, 2));
    /* (and no record for missing outputs) */
    CHECK(cdcache_write(at("work/cache2"), d0, outs, 2) != 0);
    CHECK(access(at("work/cache2"), F_OK) != 0);
    put("out/table.tsv", "table");
    CHECK(cdcache_hit(record, d0, outs, 2));
    /* other outputs than the record's */
    CHECK(!cdcache_hit(record, d0, outs, 1));

    /* broken records: garbage, empty, cut off, one byte changed, a 0
       byte, a line more */
    {
        size_t len;
        char *good = (char *)read_file(record, &len), *t;

        CHECK(good != NULL);
        put("work/cache", "garbage\n");
        CHECK(!cdcache_hit(record, d0, outs, 2));
        put("work/cache", "");
        CHECK(!cdcache_hit(record, d0, outs, 2));
        CHECK(write_file(record, good, len - 1) == 0);
        CHECK(!cdcache_hit(record, d0, outs, 2));
        t = strdup(good);
        t[20] ^= 1;
        CHECK(write_file(record, t, len) == 0);
        CHECK(!cdcache_hit(record, d0, outs, 2));
        memcpy(t, good, len);
        t[len - 1] = 0;
        CHECK(write_file(record, t, len) == 0);
        CHECK(!cdcache_hit(record, d0, outs, 2));
        free(t);
        CHECK((t = str_printf("%sx y\n", good)) != NULL);
        put("work/cache", t);
        CHECK(!cdcache_hit(record, d0, outs, 2));
        free(t);
        CHECK(write_file(record, good, len) == 0);
        CHECK(cdcache_hit(record, d0, outs, 2));
        free(good);
    }
    /* the record missing */
    CHECK(remove(record) == 0);
    CHECK(!cdcache_hit(record, d0, outs, 2));

    rm_tree(base);
    free((char *)outs[0]);
    free((char *)outs[1]);
    free((char *)record);
    if (failures)
        fprintf(stderr, "%d failure(s)\n", failures);
    return failures != 0;
}
