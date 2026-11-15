/* cmake/check-asm-sections.cmake, test blitzlib_asm_sections_broken: a
   library's own section switches, which the check must refuse with file
   and line (lines 6, 9, 12 and 14).  Only read, never compiled. */

/* an error check in top-level asm: into the error check area and back */
__asm__("   .section .blitzlib.errchk,\"ax\"\n   .even\n"
        "_divchk:\n"
        "   rts\n"
        "   .text\n");

/* data in the code hunk, and a directive in the middle of a literal */
__asm__("   .text\n"
        "_table: .long 0\n"
        "   .even\n   .data\n"
        "_var: .long 0\n");
