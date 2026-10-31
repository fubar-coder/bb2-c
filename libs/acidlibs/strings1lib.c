/*
 * strings1lib - system library 65235 ($C600-$C601): the string work
 * buffer and copying strings into it (token copstring, blitz2.s equate
 * 152; workstart $C601).
 *
 * C translation of bblibsrc/strings1lib.
 *   init:       allocate the string buffer (d0 = size from the compiler
 *               options, memlib globalloc); the init value is the buffer.
 *   $C601:      no code - loads this library's init value (the buffer)
 *               into a3 (!libs #strings1lib,#la3).
 *   stringmode: copy a string to the buffer at a3.
 */
#include "blitzlib.h"

/* entry points (thunks, defined below) */
void stringmode(void);
void codeinit(void);

BLITZ_ENTRY_DECL(init);

/* !syslibheader{#strings1lib,$c600,$c602,init,1,0,0} */
BLITZ_SYSLIBHEADER7(LIB_strings1lib, 0xc600, 0xc602, init, BL_LONGRET, 0, 0);
BLITZ_NULLSUB(stringmode, 0, 0);                        /* $C600 copstring */
BLITZ_NULLSUB(0, 0, 0, LIB_strings1lib, BL_LA(3));      /* $C601 workstart */
BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_LIBFIN();

/**
 * @brief init - allocate the string work buffer.
 *
 * MOVEQ #1,d1 (MEMF_PUBLIC): ALibJsr $c002 (memlib globalloc).  The
 * result is the library's init value, which $C601 (workstart) loads into
 * a3.
 *
 * @param size d0: the buffer size from the compiler options
 * @return d0: the buffer
 */
static ULONG BLITZ_ROUTINE codeinit_c(ULONG size BL_REG(d0))
{
    struct bl_regs r = { .d0 = size, .d1 = MEMF_PUBLIC };
    return BLITZ_ALIBJSR(TOK_globalloc, r);
}
BLITZ_THUNK(codeinit, codeinit_c, "a0-a1");

/**
 * @brief $C600 (copstring) - copy a string into the work buffer at a3.
 *
 * Copies `len` bytes from a0 to a3 and terminates with 0; a3 then points
 * to the 0.  The original copies in blocks of 8 bytes plus the rest; the
 * result is the same as this byte loop.  d0/d1 are preserved.
 *
 * @param src a0: the characters
 * @param len d0: the length - the caller's stack argument (the compiler
 *            pushes it with move.l (a0)+,-(a7) before the call and removes
 *            it later), loaded by the thunk
 */
static void BLITZ_ROUTINE stringmode_c(const UBYTE *src BL_REG(a0), ULONG len BL_REG(d0))
{
    while (len--)
        *blitz_string++ = *src++;
    *blitz_string = 0;                          /* CLR.b (a3) */
}
BLITZ_THUNK_PRE(stringmode, stringmode_c, "d0-d1", "move.l 4+8(sp),d0");
