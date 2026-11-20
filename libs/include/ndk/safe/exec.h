/* The safe forms of <inline/exec.h> for the libraries
   (cmake/ndk-calls.cmake, libs/include/ndk/functions.list): the NDK's
   macro, the register variable on a3 replaced by an operand in a register
   gcc chooses, which "exg" swaps with a3 around the jsr.  a3 is back
   afterwards (the OS keeps d2-d7/a2-a6); 4 bytes per call, no stack, the
   condition codes untouched (the NDK's own technique for a5).  RawDoFmt's
   PutChProc gets PutChData in a3 and may move it (move.b d0,(a3)+): the
   value it leaves stays in the operand, a3 is the caller's again. */

#define __RawDoFmt_base(__in_base, ___formatString, ___dataStream, ___putChProc, ___putChData) ({\
  STRPTR __p____formatString = (STRPTR)(___formatString);\
  APTR __p____dataStream = (APTR)(___dataStream);\
  void *__p____putChProc = (void *)(___putChProc);\
  APTR __p____putChData = (APTR)(___putChData);\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register APTR __v_ret __asm("d0");\
  register STRPTR __v0 __asm("a0") = __p____formatString;\
  register APTR __v1 __asm("a1") = __p____dataStream;\
  register VOID (*__v2)() __asm("a2") = __p____putChProc;\
  __asm volatile (\
                   "exg %3,%/a3\n\tjsr %/a6@(-522:W)\n\texg %3,%/a3\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1), "+r"(__p____putChData)\
                   : "a"(__p__in_base), "a"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
  __v_ret;})
