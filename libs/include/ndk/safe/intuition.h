/* The safe forms of <inline/intuition.h> for the libraries
   (cmake/ndk-calls.cmake, libs/include/ndk/functions.list): the NDK's
   macro, the register variable on a3 replaced by an operand in a register
   gcc chooses, which "exg" swaps with a3 around the jsr.  a3 is back
   afterwards (the OS keeps d2-d7/a2-a6); 4 bytes per call, no stack, the
   condition codes untouched (the NDK's own technique for a5). */

#define __AutoRequest_base(__in_base, ___window, ___body, ___posText, ___negText, ___pFlag, ___nFlag, ___width, ___height) ({\
  struct Window * __p____window = (struct Window *)(___window);\
  struct IntuiText * __p____body = (struct IntuiText *)(___body);\
  struct IntuiText * __p____posText = (struct IntuiText *)(___posText);\
  struct IntuiText * __p____negText = (struct IntuiText *)(___negText);\
  ULONG __p____pFlag = (ULONG)(___pFlag);\
  ULONG __p____nFlag = (ULONG)(___nFlag);\
  UWORD __p____width = (UWORD)(___width);\
  UWORD __p____height = (UWORD)(___height);\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register BOOL __v_ret __asm("d0");\
  register struct Window * __v0 __asm("a0") = __p____window;\
  register struct IntuiText * __v1 __asm("a1") = __p____body;\
  register struct IntuiText * __v2 __asm("a2") = __p____posText;\
  register ULONG __v4 __asm("d0") = __p____pFlag;\
  register ULONG __v5 __asm("d1") = __p____nFlag;\
  register ULONG __v6 __asm("d2") = __p____width;\
  register ULONG __v7 __asm("d3") = __p____height;\
  __asm volatile (\
                   "exg %4,%/a3\n\tjsr %/a6@(-348:W)\n\texg %4,%/a3\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1), "+d"(__v5), "+r"(__p____negText)\
                   : "a"(__p__in_base), "a"(__v2), "d"(__v4), "d"(__v6), "d"(__v7)\
                   : "fp0", "fp1", "cc", "memory" );\
  __v_ret;})

#define __EasyRequestArgs_base(__in_base, ___window, ___easyStruct, ___idcmpPtr, ___args) ({\
  struct Window * __p____window = (struct Window *)(___window);\
  struct EasyStruct * __p____easyStruct = (struct EasyStruct *)(___easyStruct);\
  ULONG * __p____idcmpPtr = (ULONG *)(___idcmpPtr);\
  APTR __p____args = (APTR)(___args);\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register LONG __v_ret __asm("d0");\
  register struct Window * __v0 __asm("a0") = __p____window;\
  register struct EasyStruct * __v1 __asm("a1") = __p____easyStruct;\
  register ULONG * __v2 __asm("a2") = __p____idcmpPtr;\
  __asm volatile (\
                   "exg %3,%/a3\n\tjsr %/a6@(-588:W)\n\texg %3,%/a3\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1), "+r"(__p____args)\
                   : "a"(__p__in_base), "a"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
  __v_ret;})
