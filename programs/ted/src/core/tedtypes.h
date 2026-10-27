/*
 * tedtypes.h - basic types for the TED core (src/core).
 *
 * The core builds for AmigaOS (m68k-amigaos-gcc, exec types from the NDK)
 * and natively on the host (TED_HOST, set by CMake for the `host` preset).
 * It must not include any other AmigaOS header: everything the core needs
 * from the system goes through hooks.h.
 */
#ifndef TED_TEDTYPES_H
#define TED_TEDTYPES_H

#ifndef TED_HOST
#include <exec/types.h>
#else
#include <stdint.h>
typedef uint8_t   UBYTE;
typedef int8_t    BYTE;
typedef uint16_t  UWORD;
typedef int16_t   WORD;
typedef uint32_t  ULONG;
typedef int32_t   LONG;
typedef void     *APTR;
typedef char     *STRPTR;
typedef int16_t   BOOL;        /* exec: short */
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif
#endif

/* functions that never return (outmem: longjmp into mainedit) */
#define NORETURN __attribute__((noreturn))

/* AmigaDOS file handles/locks are BPTRs (LONG); the core only passes them
   on to the file hooks.  0 = no file. */
typedef LONG tedfh;

#endif
