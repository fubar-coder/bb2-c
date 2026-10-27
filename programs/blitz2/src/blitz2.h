/*
 * blitz2.h - umbrella header of the C translation of the Blitz Basic 2
 * v1.95 compiler (blitz2.s, Acid Software 1995).
 *
 * Every chunk file NAME.c has a header NAME.h that declares everything it
 * defines for the other chunks; all of them are included here, so a chunk
 * file only needs #include "blitz2.h".
 */
#ifndef BLITZ2_H
#define BLITZ2_H

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <setjmp.h>

#define NORETURN __attribute__((noreturn))

#include "structs.h"
#include "glue.h"

/* library bases (asm names: dos, int, maths; exec via 4.w) */
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct IntuitionBase *IntuitionBase;
extern struct Library *MathBase;

/* chunk headers, in source order of blitz2.s */
#include "main.h"       /* 01: 1-1911      startup, editor interface, menus, run */
#include "compile.h"    /* 02: 1912-3290   compile driver, macros, statement dispatch */
#include "stmts.h"      /* 03: 3291-4999   statements, direct mode, residents, asm EA */
#include "asmlibs.h"    /* 04: 5000-6739   inline asm, parameters, lib loading */
#include "parse.h"      /* 05: 6740-8450   getchar, errors, names, variables */
#include "eval.h"       /* 06: 8451-10683  code emitters, expression evaluator */
#include "libcall.h"    /* 07: 10684-12135 numbers, lib calls, includes */
#include "tokens1.h"    /* 08: 12136-13326 built-in tokens part 1 */
#include "tokens2.h"    /* 09: 13327-14655 built-in tokens part 2 */
#include "data.h"       /* 10: 14656-end   data, requesters, options, .xtra */

#endif
