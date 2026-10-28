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

/* normalised instruction text (malloc'd; NULL = out of memory); rel: the
   instruction contains a relocated long */
char *asmcmp_normalise(const char *text, int rel);

/* count instructions of code from start, normalised (relocations of h);
   *ins gets count strings (asmcmp_free_list).  0 = ok, -1 = error
   (message in err: objdump failed, fewer instructions) */
int asmcmp_disasm(const char *objdump, const struct hunk *h, uint32_t start, unsigned count,
                  char ***ins, char *err, size_t errlen);
void asmcmp_free_list(char **ins, unsigned count);

/* a routine: NAME:ORIGADDR:CSYMBOL[+HEX]:COUNT (ORIGADDR hex offset in the
   original; CSYMBOL a symbol of the C library without the leading _, or a
   hex offset; +HEX an offset from it, may be negative).  Compares and
   prints "NAME  N instructions identical" or "NAME DIFFERS" with both
   listings; returns 0 = same, 1 = differs, -1 = error (message to out) */
int asmcmp_routine(const char *objdump, const struct hunk *orig, const struct loadfile *c,
                   const char *spec, FILE *out);

#endif
