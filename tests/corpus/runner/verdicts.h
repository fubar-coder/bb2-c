/*
 * verdicts.h - the verdict store of the bundles (change
 * basic-test-bundle-runner; caserun.h, "The bundles"): a bundle runs the
 * cases of several test sources in shared emulator sessions and hands
 * each clean PASSED or REPRODUCED to the case's own test through a file
 * of the store; the case test takes it if its inputs are still the same,
 * else it runs the case alone.
 *
 *   RUN/verdicts/TEST            the entry of the case test TEST
 *   RUN/verdicts/TEST.new-PID    an entry being written by process PID
 *   RUN/verdicts/TEST.claim-PID  an entry being read by process PID
 *
 * RUN is the run directory of the fixtures (rundir.h: $BB2C_TEST_RUN, else
 * the environment's `[runner] run-dir`).  An entry is written as
 * TEST.new-PID and renamed to TEST; a case test claims it by renaming it
 * to TEST.claim-PID, reads it and removes it (of two claimants one gets
 * the file, the other finds none), and removes the directory when it is
 * empty.  A bundle removes the entries of its members at its start
 * (vstore_clear), but not the files of a process that still lives.
 *
 * The entry, text lines with LF:
 *
 *   bundle-verdict v=1
 *   key=HEX64          the inputs the verdict was judged from (vstore_key)
 *   bundle=NAME        the bundle that stored it
 *   status=N           the case test's exit status
 *   ...                the case's report block, as the run of the case
 *                      alone prints it (the "==" line to the verdict line)
 */
#ifndef VERDICTS_H
#define VERDICTS_H

#include <stddef.h>
#include "testconf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VSTORE_DIR "verdicts"
#define VSTORE_MAGIC "bundle-verdict v=1"

/** @brief The digests of the files a process has read (a bundle reads the
 *         same runner files for every member). */
struct vstore_cache {
    char **path;
    char (*hex)[65];
    size_t n, cap;
};

void vstore_cache_free(struct vstore_cache *c);

/** @brief What a stored verdict is judged from (vstore_key). */
struct vstore_keyin {
    const char *test;               /* the case test's ctest name */
    const char *side;               /* "port" */
    const char *case_name;
    const struct tc_config *conf;   /* the source, the configuration files,
                                       the directory ID.test/ */
    const struct tc_env *env;       /* its file and the files of [runner] */
    const char *const *staged;      /* the host files the cell's pass stages
                                       or runs (its compiler, libraries,
                                       files=, programs) */
    size_t nstaged;
    const char *self;               /* the runner program; NULL: /proc/self/exe */
    const char *emulator;           /* the emulator's inputs as a text (the
                                       snapshot key of the session, the
                                       session key); NULL: none */
};

/**
 * @brief The key of a stored verdict: the sha256 (hex) of a text of
 *        NAME=LEN:VALUE lines: v=1, the test, the side, the case, the
 *        digest (sha256) of the source, of every configuration file
 *        tc_resolve read (in its order), of every file below ID.test/ (by
 *        its relative path, in name order), of the environment file, of
 *        every existing file a [runner] value names (a value of several
 *        paths separated by "|" names each), of every staged file, of the
 *        runner program, and the emulator's text.  A missing file has the
 *        digest "missing", a directory the digest of its names and the
 *        digests of its files.
 * @param c    the digests read before (NULL: none kept)
 * @param key  the key, 64 hex digits
 * @return 0, -1 = out of memory
 */
int vstore_key(struct vstore_cache *c, const struct vstore_keyin *in, char key[65]);

/**
 * @brief The store's directory: RUN/verdicts, RUN the run directory
 *        (rundir_path with the environment's `[runner] run-dir`).
 * @return malloc'd, NULL = no run directory (or out of memory)
 */
char *vstore_dir(const struct tc_env *env);

/**
 * @brief The text of an entry.
 * @return malloc'd, NULL = out of memory
 */
char *vstore_entry(const char *key, const char *bundle, int status, const char *block);

/**
 * @brief Write the entry of TEST: TEST.new-PID, then renamed to TEST (dir
 *        is made if needed).
 * @return 0, -1 (err: the file that cannot be written; a name with "/" or
 *         a leading ".")
 */
int vstore_put(const char *dir, const char *test, const char *text, char *err, size_t errlen);

/** @brief What a claim found (vstore_claim). */
enum vstore_got {
    VSTORE_NONE = 0,                /* no entry */
    VSTORE_SERVED = 1,              /* a fresh entry: status, block */
    VSTORE_STALE = 2,               /* an entry of other inputs or of
                                       another bundle (removed) */
    VSTORE_MALFORMED = 3            /* not an entry (removed; err names why) */
};

/** @brief A fresh entry. */
struct vstore_verdict {
    int status;
    char *block;                    /* malloc'd */
};

/**
 * @brief Claim the entry of TEST: rename it to TEST.claim-PID, read it,
 *        remove it (and dir, if that is empty now), check it.
 * @param key     the key of the case test's inputs (NULL: none can be made,
 *                every entry is stale)
 * @param bundle  the bundle the case test names
 * @param v       with VSTORE_SERVED the verdict (vstore_verdict_free)
 * @return enum vstore_got, -1 = out of memory or a read error (err)
 */
int vstore_claim(const char *dir, const char *test, const char *key, const char *bundle, struct vstore_verdict *v,
                 char *err, size_t errlen);

void vstore_verdict_free(struct vstore_verdict *v);

/**
 * @brief Check the text of an entry (vstore_claim's check).
 * @return 0 and the parts (pointers into text; block to its end), -1 =
 *         malformed (err: why: "not an entry", "version", "no key", "no
 *         bundle", "no status", "no report block")
 */
int vstore_parse(const char *text, size_t len, const char **key, const char **bundle, size_t *bundlelen,
                 int *status, const char **block, char *err, size_t errlen);

/**
 * @brief Remove the entry of TEST and its files TEST.new-PID and
 *        TEST.claim-PID whose process no longer lives.
 * @return 0, -1 = a file that cannot be removed
 */
int vstore_clear(const char *dir, const char *test);

#ifdef __cplusplus
}
#endif

#endif
