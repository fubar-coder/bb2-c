/*
 * chipbaselib - system library 64100: base address of the custom chips.
 * Its init value ($DFF000) is the global the compiler uses for hardware
 * access in Blitz mode.
 *
 * C translation of bblibsrc/chipbaselib.  The init routine is an inline
 * routine (`Dc $a000,len`): the compiler copies `move.l #$dff000,d0`
 * into the program start instead of calling it.
 */
#include "blitzlib.h"

/* entry points (thunks / asm, defined below) */
void _codeinit(void);

BLITZ_ENTRY_DECL(_init);

/* !syslibheader{#chipbaselib,0,0,_init,1,0,0} */
BLITZ_SYSLIBHEADER7(LIB_chipbaselib, 0, 0, _init, BL_LONGRET, 0, 0);
BLITZ_NULLSUB_AT(_init, _codeinit, 0, 0);
BLITZ_LIBFIN();

/**
 * @brief The init routine: the base address of the custom chips.
 *
 * An inline routine (`Dc $a000,len`): the compiler copies its code
 * (`move.l #$dff000,d0`) into the program start instead of calling it;
 * the init value is the global the compiler uses for hardware access in
 * Blitz mode.
 *
 * @return d0: $DFF000
 */
BLITZ_INLINE(_codeinit, "move.l #0xdff000,d0");
