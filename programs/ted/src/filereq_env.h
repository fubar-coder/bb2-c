/*
 * filereq_env.h - what the shared file requester core
 * (../../shared/filereq/filereq_core.c) needs, for TED: exec through
 * <proto/exec.h> (SysBase: main.c) and DIVU.w as TED's divu_w (glue.c),
 * which executes the instruction itself - no libnix __udivsi3/__umodsi3
 * (they need UtilityBase, which TED never opens).  Same semantics as
 * freqlib's bl_divu_w.
 */
#ifndef FILEREQ_ENV_H
#define FILEREQ_ENV_H

#include <exec/types.h>
#include <proto/exec.h>
#include "glue.h"

#define fq_divu_w divu_w

#endif
