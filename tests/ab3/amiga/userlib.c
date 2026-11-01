/*
 * userlib - a Blitz library for the userlib test of blitz2-ab3's AB3 mode
 * (tests/ab3/cli/userlib.ab3): library number 152, the number of
 * AmiBlitz3 v3.14.0's freqlib in baseLibs, so a file in
 * Blitz3:BlitzLibs/UserLibs/ replaces it (AmiBlitz3's
 * read_userlibsfile).  One function AB3UserLibTest that returns 42 (token
 * $CC01 = 152 * 128 + 1, as FileRequest$ of the replaced library).  No
 * relocations: the file ends with HUNK_END behind the code.
 */
#include "blitzlib.h"

/* entry points (asm, defined below) */
void doab3userlib(void);

/* !libheader{152,0,0,0,0} */
BLITZ_LIBHEADER(152, 0, BL_NORET, 0, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doab3userlib, 0, 0);
BLITZ_NAME("AB3UserLibTest", "");

BLITZ_LIBFIN();

BLITZ_ASM_ROUTINE(doab3userlib,
"   moveq   #42,d0\n"
"   rts");
