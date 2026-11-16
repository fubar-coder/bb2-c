/*
 * blitzexec.h - exec calls from library code.  There is no C startup in a
 * library, so no SysBase variable: like the originals (MOVE.l 4,a6) the
 * base is read from address 4 at every call.
 */
#ifndef BLITZEXEC_H
#define BLITZEXEC_H

#define __NOLIBBASE__
#include <proto/exec.h>
#undef __NOLIBBASE__
#define SysBase (*(struct ExecBase **)4L)

/* BL_FREESIGNAL(sig) - FreeSignal with the signal number zero extended,
   like the originals (MOVEQ #0,d0:MOVE.b sigbit,d0:JSR _FreeSignal(a6)).
   The NDK inline takes a BYTE and would pass a bit number $FF as -1.
   `sig` is passed as its (unsigned) value: a UBYTE mp_SigBit, or a UWORD
   for MOVE.w (brexxlib mybit = $FFFF).  Needs blitzlib.h (BLITZ_SYSJSR1). */
#define BL_FREESIGNAL(sig) ((void)BLITZ_SYSJSR1(SysBase, -336, (sig)))

#endif
