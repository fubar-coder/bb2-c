/*
 * filereq_env.h - what the shared file requester core
 * (../../shared/filereq/filereq_core.c) needs, for freqlib: the BlitzLib
 * environment (exec through blitzexec.h, SysBase read from address 4) and
 * DIVU.w as bl_divu_w.
 */
#ifndef FILEREQ_ENV_H
#define FILEREQ_ENV_H

#include "blitzlib.h"
#include "blitzexec.h"

#define fq_divu_w bl_divu_w

#endif
