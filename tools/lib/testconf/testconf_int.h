/*
 * testconf_int.h - the helpers the files of lib_testconf share (not part
 * of its interface).
 */
#ifndef TESTCONF_INT_H
#define TESTCONF_INT_H

#include <stddef.h>
#include "testconf.h"

/* writes the message (printf format) to err; returns -1 */
int tc_err(char *err, size_t errlen, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
char *tc_strdup(const char *s);
/* a whole file, 0-terminated (*len without the 0); 0 = ok, -1 = errno */
int tc_read_file(const char *file, char **text, size_t *len);
/* dir + "/" + rel (rel absolute: rel), lexically normalised */
char *tc_path_join(const char *dir, const char *rel);
/* the TOML type and the value of a key against its definition */
int tc_check_type(const char *file, unsigned line, const struct tc_keydef *def,
                  const struct tt_value *v, char *err, size_t errlen);
/* placeholders and the path field of a string or of every list element;
   keep (NULL: none) a placeholder left as it is (TC_DRIVER in jobline) */
int tc_resolve_value(const struct tc_env *env, const char *file, unsigned line, int pathfield,
                     const char *dir, const char *keep, struct tt_value *v, char *err, size_t errlen);
void tc_tabkey_free(struct tc_tabkey *t);
/* testmatrix.c: a key of the table matrix and its values */
int tc_axis_check(const char *file, unsigned line, const char *key, const struct tt_value *v,
                  char *err, size_t errlen);
/* testmatrix.c: an axis against a setting of the same key (the nearer
   file wins, one file both: error), the exclude selectors */
int tc_matrix_merge(struct tc_config *c, char *err, size_t errlen);
/* testassert.c: the selectors, queries and predicates of the tables
   asserts and deviations; fixed-orig and fixed against the deviations */
int tc_assert_validate(const struct tc_config *c, char *err, size_t errlen);

#endif
