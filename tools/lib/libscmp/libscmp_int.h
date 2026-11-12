/*
 * libscmp_int.h - the parts of the judge shared by its source files
 * (libscmp.h is the interface).
 */
#ifndef LIBSCMP_INT_H
#define LIBSCMP_INT_H

#include "libscmp.h"

/* a sorted list of program names */
struct libscmp_names {
    char **v;
    size_t n, cap;
};

const struct libscmp_entry *libscmp_find_h(const struct libscmp_file *f, const char *key,
                                           size_t keylen, uint32_t h);
/* the key of case c of f for messages: KEY[ QUAL][ (observations A..B)];
   the block's count from c's value, else from oc of other (may be NULL) */
void libscmp_key_text(const struct libscmp_file *f, const struct libscmp_case *c,
                      const struct libscmp_file *other, const struct libscmp_case *oc, char *buf,
                      size_t len);
char *libscmp_read_file(const char *path, size_t *len);

int libscmp_valid_name(const char *s);
int libscmp_names_add(struct libscmp_names *l, const char *s, size_t n);
void libscmp_names_sort(struct libscmp_names *l);
void libscmp_names_free(struct libscmp_names *l);
/* the NAMEs of DIR/NAME.txt into l; -1 = error (on err) */
int libscmp_names_dir(struct libscmp_names *l, const char *dir, FILE *err);
/* the programs: the only list (checked), else the files of dir1 and dir2
   (dir2 may be NULL); sorted; -1 = error (on err) */
int libscmp_program_names(struct libscmp_names *names, const char *dir1, const char *dir2,
                          const char *const *only, size_t nonly, FILE *err);
/* DIR/NAME.txt (malloc'd) */
char *libscmp_path(const char *dir, const char *name);
int libscmp_exists(const char *path);
/* DIR/NAME.txt parsed, its header naming NAME; 0 = ok, else the message
   on err */
int libscmp_load_named(struct libscmp_file *f, const char *dir, const char *name,
                       enum libscmp_kind kind, FILE *err);

#endif
