/*
 * amiga_emulation_cov.c - collecting the coverage files of a session
 * (amiga_emulation.h, "Coverage"; the file layout in shared/cov/covfmt.h).
 */
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE         /* flock */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "amiga_emulation.h"
#include "covfmt.h"

static uint32_t be32(const unsigned char *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

/* a whole coverage file: header and bitmap of the length it gives */
static int cov_valid(const unsigned char *p, size_t n)
{
    uint32_t len;

    if (n < COVFMT_HEADER_SIZE || be32(p) != COVFMT_MAGIC
        || be32(p + 4) != COVFMT_VERSION)
        return 0;
    len = be32(p + 12);
    return len == COVFMT_BITMAP_LEN(be32(p + 8)) && n - COVFMT_HEADER_SIZE == len;
}

int amiga_emulation_cov_merge(unsigned char *cur, size_t curn, const unsigned char *old,
                              size_t oldn)
{
    if (!cov_valid(cur, curn))
        return -1;
    if (!old || !cov_valid(old, oldn) || oldn != curn || memcmp(old, cur, COVFMT_HEADER_SIZE))
        return 0;
    for (size_t i = COVFMT_HEADER_SIZE; i < curn; i++)
        cur[i] |= old[i];
    return 1;
}

int amiga_emulation_cov_prepare(const char *test_dir)
{
    char path[4096];

    if (snprintf(path, sizeof path, "%s/cov", test_dir) >= (int)sizeof path)
        return -1;
    return mkdir(path, 0777) && errno != EEXIST ? -1 : 0;
}

/* the whole file at fd (from offset 0); NULL on error, *n its size */
static unsigned char *read_fd(int fd, size_t *n)
{
    struct stat st;
    unsigned char *p;
    size_t got = 0;

    if (fstat(fd, &st) || st.st_size < 0)
        return NULL;
    if (!(p = malloc((size_t)st.st_size + 1)))
        return NULL;
    while (got < (size_t)st.st_size) {
        ssize_t r = pread(fd, p + got, (size_t)st.st_size - got, (off_t)got);

        if (r <= 0) {
            free(p);
            return NULL;
        }
        got += (size_t)r;
    }
    *n = got;
    return p;
}

static int mkdirs_p(const char *dir)
{
    char path[4096];
    size_t n = strlen(dir);

    if (n >= sizeof path)
        return -1;
    memcpy(path, dir, n + 1);
    for (char *s = path + 1; *s; s++)
        if (*s == '/') {
            *s = 0;
            if (mkdir(path, 0777) && errno != EEXIST)
                return -1;
            *s = '/';
        }
    return mkdir(path, 0777) && errno != EEXIST ? -1 : 0;
}

/* src (a coverage file) into dest: merged with what dest holds (locked,
   other runners may collect into the same directory); 0 = done, the source
   removed; 1 = src is no coverage file (left in place); -1 = error */
static int collect_one(const char *src, const char *dest, char *err, size_t errlen)
{
    unsigned char *cur = NULL, *old = NULL;
    size_t curn = 0, oldn = 0;
    int sfd, dfd = -1, r = -1;

    if ((sfd = open(src, O_RDONLY)) < 0 || !(cur = read_fd(sfd, &curn))) {
        snprintf(err, errlen, "cannot read %s: %s", src, strerror(errno));
        goto out;
    }
    if ((dfd = open(dest, O_RDWR | O_CREAT, 0666)) < 0 || flock(dfd, LOCK_EX)
        || !(old = read_fd(dfd, &oldn))) {
        snprintf(err, errlen, "cannot open %s: %s", dest, strerror(errno));
        goto out;
    }
    if (amiga_emulation_cov_merge(cur, curn, old, oldn) < 0) {
        r = 1;
        goto out;
    }
    if (ftruncate(dfd, 0) || pwrite(dfd, cur, curn, 0) != (ssize_t)curn) {
        snprintf(err, errlen, "cannot write %s: %s", dest, strerror(errno));
        goto out;
    }
    r = unlink(src) ? -1 : 0;
    if (r)
        snprintf(err, errlen, "cannot remove %s: %s", src, strerror(errno));
out:
    if (dfd >= 0) {
        /* a file made here for a source that is none: not left behind */
        if (r == 1 && oldn == 0)
            unlink(dest);
        close(dfd);
    }
    if (sfd >= 0)
        close(sfd);
    free(cur);
    free(old);
    return r;
}

int amiga_emulation_cov_collect(const char *test_dir, const char *dest_dir, char *err,
                                size_t errlen)
{
    char dir[4096];
    DIR *d;
    struct dirent *de;
    int count = 0, failed = 0;

    if (snprintf(dir, sizeof dir, "%s/cov", test_dir) >= (int)sizeof dir) {
        snprintf(err, errlen, "path too long: %s/cov", test_dir);
        return -1;
    }
    if (!(d = opendir(dir)))
        return 0;
    while ((de = readdir(d)) != NULL) {
        size_t l = strlen(de->d_name), sl = strlen(".cov");
        char src[4096], dest[4096];
        int r;

        if (l <= sl || strcmp(de->d_name + l - sl, ".cov"))
            continue;
        if (!count && !failed && mkdirs_p(dest_dir)) {
            snprintf(err, errlen, "cannot create %s: %s", dest_dir, strerror(errno));
            failed = 1;
            break;
        }
        if (snprintf(src, sizeof src, "%s/%s", dir, de->d_name) >= (int)sizeof src
            || snprintf(dest, sizeof dest, "%s/%s", dest_dir, de->d_name) >= (int)sizeof dest) {
            snprintf(err, errlen, "path too long: %s", de->d_name);
            failed = 1;
            continue;
        }
        if ((r = collect_one(src, dest, err, errlen)) < 0)
            failed = 1;
        else if (r == 0)
            count++;
    }
    closedir(d);
    rmdir(dir);                         /* only if it is empty now */
    return failed ? -1 : count;
}
