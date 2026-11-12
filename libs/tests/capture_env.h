/*
 * capture_env.h - what capture.c needs of its system: files, a shell
 * variable, the memory of the test program.  capture_amiga.c implements it
 * with dos.library and exec.library for libs_tests; the host test
 * (libs/tests/host/test_capture_format.cpp) with memory.
 */
#ifndef CAPTURE_ENV_H
#define CAPTURE_ENV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* a new file PATH (replaced if there); NULL if it cannot be opened */
void *cap_env_create(const char *path);
/* make the directory PATH (an existing one is no error) */
void cap_env_mkdir(const char *path);
/* write N bytes; 0 on success */
int cap_env_write(void *f, const char *p, uint32_t n);
void cap_env_close(void *f);
/* the shell variable NAME into BUF (size SIZE, ended by 0): its length,
   or -1 if not set */
int cap_env_getvar(const char *name, char *buf, uint32_t size);
/* find the memory of the test program (its hunks and its stack) */
void cap_env_find_prog(void);
/* V is an address inside the test program's memory */
int cap_env_is_prog(uint32_t v);

/* a failed check (libs_tests.cpp: reported to snitch) */
void bl_test_check(const char *what, int ok);

#ifdef __cplusplus
}
#endif

#endif
