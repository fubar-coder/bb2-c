/*
 * filereq_core.h - the file requester of Blitz Basic 2 (filereq_core.c),
 * shared by freqlib (libs-c/libs/freqlib.c) and TED (ted-c/src/filereq.c).
 * Each program compiles filereq_core.c itself and provides filereq_env.h.
 */
#ifndef FILEREQ_CORE_H
#define FILEREQ_CORE_H

#include <exec/types.h>

/* parameters, set before the call (= the 4-long `reqstruct` of TED and the
   parameter block of freqlib: same order):
     [0] path buffer (160 bytes, in/out)
     [1] file buffer (64 bytes, in/out)
     [2] window title
     [3] screen (0 = Workbench) */
extern ULONG fq_args[4];

/* the requester; returns the address of the result "path/file" (inside the
   data block) or 0 (cancel, close gadget, no memory/window).  Original
   entry: code+4 ($CA in freqlib), the parameter block in a0. */
ULONG fq_request(void);

#endif
