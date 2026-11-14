/* The safe forms of <inline/wb.h> for the libraries
   (cmake/ndk-calls.cmake, libs/include/ndk/functions.list): the NDK's
   macro, the register variables on a3 and a4 replaced by operands in
   registers gcc chooses, which "exg" swaps with a3 and a4 around the jsr.
   Both are back afterwards (the OS keeps d2-d7/a2-a6); 4 bytes per call
   and register, no stack, the condition codes untouched (the NDK's own
   technique for a5, and for a4 with -fbaserel, which no library uses). */

#ifdef __baserel__
#error "the safe AddAppIconA is the form without -fbaserel (the NDK's own saves a4 there)"
#endif

#define __AddAppIconA_base(__in_base, ___id, ___userdata, ___text, ___msgport, ___lock, ___diskobj, ___taglist) ({\
  ULONG __p____id = (ULONG)(___id);\
  ULONG __p____userdata = (ULONG)(___userdata);\
  STRPTR __p____text = (STRPTR)(___text);\
  struct MsgPort * __p____msgport = (struct MsgPort *)(___msgport);\
  BPTR __p____lock = (BPTR)(___lock);\
  struct DiskObject * __p____diskobj = (struct DiskObject *)(___diskobj);\
  struct TagItem * __p____taglist = (struct TagItem *)(___taglist);\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register struct AppIcon * __v_ret __asm("d0");\
  register ULONG __v0 __asm("d0") = __p____id;\
  register ULONG __v1 __asm("d1") = __p____userdata;\
  register STRPTR __v2 __asm("a0") = __p____text;\
  register struct MsgPort * __v3 __asm("a1") = __p____msgport;\
  register BPTR __v4 __asm("a2") = __p____lock;\
  __asm volatile (\
                   "exg %4,%/a3\n\texg %5,%/a4\n\tjsr %/a6@(-60:W)\n\texg %5,%/a4\n\texg %4,%/a3\n"\
                   : "=d"(__v_ret), "+d"(__v1), "+a"(__v2), "+a"(__v3), "+r"(__p____diskobj), "+r"(__p____taglist)\
                   : "a"(__p__in_base), "d"(__v0), "a"(__v4)\
                   : "fp0", "fp1", "cc", "memory" );\
  __v_ret;})
