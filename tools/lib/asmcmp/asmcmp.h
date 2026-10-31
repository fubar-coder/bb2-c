/*
 * asmcmp.h - assembler routines of a C library against the original,
 * instruction by instruction (for routines that stay assembler and cannot
 * run in a test; bb2 libs-c/tools/asmcmp.py).
 *
 * Disassembly with the toolchain's objdump (-b binary -m m68k:68020).
 * Addresses differ between the two files, so operands are normalised: PC
 * relative displacements become %pc@(REL), hex values become A (except
 * ALibJsr placeholders >= $80000000 and custom chip addresses $DFF000-
 * $DFF1FF), and in an instruction with a relocated long, immediates
 * beyond +-255 become #A.
 */
#ifndef ASMCMP_H
#define ASMCMP_H

#include <stdio.h>
#include "hunk.h"

/**
 * @brief The normalised text of an instruction (see the top).
 * @param text  objdump's text of the instruction
 * @param rel   nonzero: the instruction contains a relocated long
 * @return the text (malloc'd), NULL = out of memory
 */
char *asmcmp_normalise(const char *text, int rel);

/**
 * @brief Disassemble count instructions of a hunk from start, normalised.
 * @param objdump  the toolchain's objdump
 * @param h        the hunk (its relocations mark the relocated longs)
 * @param start    the offset of the first instruction
 * @param count    the number of instructions
 * @param ins      out: count strings (asmcmp_free_list)
 * @param err      the message of an error
 * @param errlen   the size of err
 * @return 0 = ok, -1 = error (objdump failed, fewer instructions)
 */
int asmcmp_disasm(const char *objdump, const struct hunk *h, uint32_t start, unsigned count,
                  char ***ins, char *err, size_t errlen);

/** @brief Free the list of asmcmp_disasm (count strings and the list). */
void asmcmp_free_list(char **ins, unsigned count);

/**
 * @brief Compare a routine of the C library with the original.
 *
 * Prints "NAME  N instructions identical" or "NAME DIFFERS" with both
 * listings.
 *
 * @param objdump  the toolchain's objdump
 * @param orig     the original's code hunk
 * @param c        the C library
 * @param spec     NAME:ORIGADDR:CSYMBOL[+HEX]:COUNT (ORIGADDR hex offset
 *                 in the original; CSYMBOL a symbol of the C library
 *                 without the leading _, or a hex offset; +HEX an offset
 *                 from it, may be negative)
 * @param out      the result and the messages
 * @return 0 = same, 1 = differs, -1 = error
 */
int asmcmp_routine(const char *objdump, const struct hunk *orig, const struct loadfile *c,
                   const char *spec, FILE *out);

#endif
