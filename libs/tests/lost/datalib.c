/*
 * datalib with a broken thunk: the test of the kept-register cases
 * (ctest libs_emulator_lost, libs/tests/CMakeLists.txt).  The library is
 * libs/acidlibs/datalib.c, but every thunk (BLITZ_THUNK) changes d2 after
 * its C function, a register the original keeps in every call and systest
 * states (KEEPS); systest's cases of the calls then say "lost d2", and the
 * compare with the expected data fails on them.  Never part of acidlibs.
 */
#include "blitzlib.h"

#undef BL_THUNK_
#define BL_THUNK_(entry, cfunc, save, pre, tail)                            \
    void entry(void);                                                       \
    __asm__("   .text\n   .even\n   .globl  _" #entry "\n"                  \
            "_" #entry ":\n"                                                \
            "   movem.l " save ",-(sp)\n" pre                               \
            "   jsr     _" #cfunc "\n" tail                                 \
            "   moveq   #-1,d2\n"                                           \
            "   movem.l (sp)+," save "\n"                                   \
            "   rts\n")

#include "../../acidlibs/datalib.c"
