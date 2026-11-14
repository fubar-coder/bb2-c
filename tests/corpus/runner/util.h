/*
 * util.h - strings and files for the corpus runners (corpusrun, tedab).
 */
#ifndef CORPUS_UTIL_H
#define CORPUS_UTIL_H

#include <stddef.h>
#include <stdint.h>

/* malloc'd formatted string (NULL = out of memory) */
char *str_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* growing text; bad = out of memory (then nothing more is added) */
struct text {
    char *p;
    size_t n, cap;
    int bad;
};
void text_add(struct text *t, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/* removes a directory tree (no error if it is not there) */
void rm_tree(const char *dir);
/* creates a directory and its parents; 0 = ok */
int mkdir_p(const char *path);
/* the whole file, 0-terminated (malloc'd, *len without the 0); NULL = missing */
uint8_t *read_file(const char *path, size_t *len);
/* 0 = ok */
int write_file(const char *path, const void *data, size_t len);
int copy_file(const char *from, const char *to);
/* copies a directory tree (files with their mode, links as links); 0 = ok */
int copy_tree(const char *from, const char *to);

#endif
