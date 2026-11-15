/*
 * epcheck.h - the check of the __entrypoint routines of a Blitz library
 * (BLITZ_ENTRYPOINT, libs/include/blitzlib.h): a C function that GCC
 * compiles without saving the callee-saved registers d2-d7/a2/a6.  Such
 * a routine is a token table entry only (BLITZ_ALIAS_EP); the check reads
 * the objects of the library as the toolchain's objdump prints them and
 * refuses:
 *   - a routine that writes a6 without saving it at its start (the
 *     register rule needs a6 kept, GCC gives it free in __entrypoint);
 *   - a call or jump into a routine from other code of the object: GCC
 *     does not know at the call site that the routine keeps nothing, and
 *     identical code folding turns a plain function with the same body
 *     into a jump to it (without the routine's no_icf);
 *   - a routine that begins with a jump into another routine (folded the
 *     other way round).
 * A routine is marked by the global symbol ___blitz_entrypoint_NAME at its
 * address (BLITZ_ALIAS_EP); it ends at the next symbol of the section.
 * Data in the code (texts, tables) is disassembled too: what objdump
 * shows at an odd offset is skipped; a branch that data at an even
 * offset happens to encode into a routine would be reported (accepted
 * limit, never seen).
 */
#ifndef EPCHECK_H
#define EPCHECK_H

#include <stdio.h>

/* the prefix of the marker symbol of an __entrypoint routine */
#define EPCHECK_MARKER "___blitz_entrypoint_"

/**
 * @brief Check the __entrypoint routines of an archive or object.
 * @param symtab  the output of objdump -t of the file
 * @param disasm  the output of objdump -d --no-show-raw-insn of the file
 * @param report  the findings, one line each, prefixed with "OBJECT: "
 * @param nroutines  out (may be NULL): the routines found
 * @return the number of findings; -1 = out of memory
 */
long epcheck_text(const char *symtab, const char *disasm, FILE *report, long *nroutines);

#endif
