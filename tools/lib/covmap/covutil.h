/*
 * covutil.h - helpers of covmap and covsel: interned strings, paths
 * relative to the repository, ninja's dependency lists and the source of
 * an object file.
 */
#ifndef COVUTIL_H
#define COVUTIL_H

#include <stddef.h>

/* interned strings: each distinct string has one id (0, 1, ...) */
struct strtab {
    char **str;         /* by id */
    size_t n, cap;
    int *slot;          /* hash table of ids, -1 empty */
    size_t nslot;
};

void strtab_free(struct strtab *t);
/* the id of s, added if new; -1 if out of memory */
int strtab_add(struct strtab *t, const char *s);
/* the id of s, -1 if not there */
int strtab_find(const struct strtab *t, const char *s);
/* the id of the first n bytes of s, added if new */
int strtab_addn(struct strtab *t, const char *s, size_t n);

/*
 * path normalised (".", "..", "//" removed) and relative to root: an
 * absolute path below root without root and its '/', a relative one as
 * it is (relative to root).  NULL for a path outside root (absolute
 * elsewhere, or leading ".." after normalising).  Allocated.
 */
char *cov_relpath(const char *path, const char *root);
/* dir + '/' + path, or path if it is absolute (allocated) */
char *cov_join(const char *dir, const char *path);

/* ninja -t deps: per object (path as ninja has it) its dependencies */
struct cov_dep {
    char *object;
    char **deps;
    size_t ndeps;
};
struct cov_deps {
    struct cov_dep *v;
    size_t n, cap;
};

/* add the objects of one `ninja -t deps` output; 0, or -1 with err */
int cov_deps_parse(struct cov_deps *d, const char *text, char *err, size_t errsz);
void cov_deps_free(struct cov_deps *d);
/* the object's source: its first dependency with a source extension
   (.c .cc .cpp .cxx .s .S .asm), NULL if none */
const char *cov_dep_source(const struct cov_dep *dep);
/* the entry of object, NULL if none */
const struct cov_dep *cov_deps_find(const struct cov_deps *d, const char *object);

/*
 * The source of an object file of CMake's Ninja generator, from its path
 * alone: DIR/CMakeFiles/TARGET.dir/REST.o (or .obj) is DIR/REST with
 * every "__" component as "..".  Relative to the source tree mirrored by
 * the build directory; NULL if the path has no such form.  Allocated.
 */
char *cov_object_source(const char *object);

/* the next line of text from *pos (without '\n' and a '\r' before it);
   NULL at the end */
const char *cov_next_line(const char *text, size_t len, size_t *pos, size_t *linelen);

/* the TN: of a test in an lcov tracefile: its name with each character but
   [A-Za-z0-9_] as '_' (lcov's test names); allocated */
char *cov_lcov_test_name(const char *test);

#endif
