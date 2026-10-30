/* libnums.h of the variant rwe-1999-eval: rwe-1999 with 121 free for
   RIEvalLib (#evallib), which has the number of the acidlibs' polylib.
   polylib moves to 87, the number of the render library it is: in
   bblibsrc, polylib and renderlib are the same source - renderlib with
   #renderlib (87, "reserved" in the CD's libnums.bb) and
   "; #renderlib=121" commented out, polylib with #chunkylib=121.  RWE's
   "Official Blitz Library Number List" of 1997-02-02 has #renderlib=87
   and #evallib=121; AmiBlitz3's sorted_libnums.ab3 keeps renderlib at 87,
   "except his was called polylib.obj ... may also exist on some systems
   as libnum 121" (docs/rwelibs/port/rievallib.md). */
#ifndef VARIANT_RWE_1999_EVAL_LIBNUMS_H
#define VARIANT_RWE_1999_EVAL_LIBNUMS_H
#include "../rwe-1999/libnums.h"

#undef LIB_polylib
#define LIB_polylib                 87   /* = #renderlib */
#define LIB_evallib                121   /* BSS2 RIEVALLIB; RWE's list */
#endif
