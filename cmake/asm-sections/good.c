/* cmake/check-asm-sections.cmake, test blitzlib_asm_sections_good: what
   the check must accept.  Only read, never compiled. */
#include "blitzlib.h"

/* the same routines through blitzlib.h: GCC places them (.text and
   .data named in comments and prose are no directives) */
BLITZ_ERRCHK_ASM(divchk,
        "   .globl  _divchk\n"
        "_divchk:\n"
        "   rts");
BLITZ_ASM_BLOCK(table,
        "_table: .long 0\n"
        "   .even\n");

/* the header section (dhandlerlib) */
__asm__("   .section .blitzlib.head\ndh_hunk0:\n");

/* names that only begin like a directive */
__asm__("   .textlen = 4\n   .database = 0\n");
