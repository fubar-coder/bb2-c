/* The safe forms of <inline/gadtools.h> for the libraries
   (cmake/ndk-calls.cmake, libs/include/ndk/functions.list): the NDK's
   macro, the register variable on a3 replaced by an operand in a register
   gcc chooses, which "exg" swaps with a3 around the jsr.  a3 is back
   afterwards (the OS keeps d2-d7/a2-a6); 4 bytes per call, no stack, the
   condition codes untouched (the NDK's own technique for a5). */

#define __GT_SetGadgetAttrsA_base(__in_base, ___gad, ___win, ___req, ___taglist) ({\
  struct Gadget * __p____gad = (struct Gadget *)(___gad);\
  struct Window * __p____win = (struct Window *)(___win);\
  struct Requester * __p____req = (struct Requester *)(___req);\
  struct TagItem * __p____taglist = (struct TagItem *)(___taglist);\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register struct Gadget * __v0 __asm("a0") = __p____gad;\
  register struct Window * __v1 __asm("a1") = __p____win;\
  register struct Requester * __v2 __asm("a2") = __p____req;\
  __asm volatile (\
                   "exg %2,%/a3\n\tjsr %/a6@(-42:W)\n\texg %2,%/a3\n"\
                   : "+a"(__v0), "+a"(__v1), "+r"(__p____taglist)\
                   : "a"(__p__in_base), "a"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d0", "d1" );\
})

#define __GT_GetGadgetAttrsA_base(__in_base, ___gad, ___win, ___req, ___taglist) ({\
  struct Gadget * __p____gad = (struct Gadget *)(___gad);\
  struct Window * __p____win = (struct Window *)(___win);\
  struct Requester * __p____req = (struct Requester *)(___req);\
  struct TagItem * __p____taglist = (struct TagItem *)(___taglist);\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register LONG __v_ret __asm("d0");\
  register struct Gadget * __v0 __asm("a0") = __p____gad;\
  register struct Window * __v1 __asm("a1") = __p____win;\
  register struct Requester * __v2 __asm("a2") = __p____req;\
  __asm volatile (\
                   "exg %3,%/a3\n\tjsr %/a6@(-174:W)\n\texg %3,%/a3\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1), "+r"(__p____taglist)\
                   : "a"(__p__in_base), "a"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
  __v_ret;})
