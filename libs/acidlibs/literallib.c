/*
 * literallib - library 33 ($9081-$90FF): the names of the literal
 * characters (issue #1018, spec libs/literal-library, docs/formats.md 3.2).
 *
 * TED-C and the compilers of this repository store a Latin-1 character
 * other than the six umlauts as the token pair $90 cc, the word $9000 + cc
 * of library 33.  The library names those words with their characters, so
 * every editor that shows the compiler's token list (also the original
 * TED, with the userlib literallib.obj) shows the character.  blitz2
 * numbers a user library's tokens from libnum * 128 + 1 (fixlibs): entry n
 * (from 1) is the word $9080 + n, named by the character $80 + n; $9080 is
 * no literal character (decision of 2026-11-13).
 *
 * The entries are dummy tokens (!dumtoke, kind 8): neither the statement
 * nor the function flag, no code, so a literal character outside a string
 * is a "Syntax Error" (handlelib, function).  No init, no finit, no help.
 * Not in the RWE variants, whose RIDosLib has the number 33
 * (ACIDLIBS_WITHOUT_LITERAL, CMakeLists.txt).
 */
#include "blitzlib.h"

/* !libheader{#literallib,0,0,0,0} */
BLITZ_LIBHEADER(LIB_literallib, 0, BL_NORET, 0, 0);

BLITZ_DUMTOKE(lit_81, "\x81", "");
BLITZ_DUMTOKE(lit_82, "\x82", "");
BLITZ_DUMTOKE(lit_83, "\x83", "");
BLITZ_DUMTOKE(lit_84, "\x84", "");
BLITZ_DUMTOKE(lit_85, "\x85", "");
BLITZ_DUMTOKE(lit_86, "\x86", "");
BLITZ_DUMTOKE(lit_87, "\x87", "");
BLITZ_DUMTOKE(lit_88, "\x88", "");
BLITZ_DUMTOKE(lit_89, "\x89", "");
BLITZ_DUMTOKE(lit_8A, "\x8A", "");
BLITZ_DUMTOKE(lit_8B, "\x8B", "");
BLITZ_DUMTOKE(lit_8C, "\x8C", "");
BLITZ_DUMTOKE(lit_8D, "\x8D", "");
BLITZ_DUMTOKE(lit_8E, "\x8E", "");
BLITZ_DUMTOKE(lit_8F, "\x8F", "");
BLITZ_DUMTOKE(lit_90, "\x90", "");
BLITZ_DUMTOKE(lit_91, "\x91", "");
BLITZ_DUMTOKE(lit_92, "\x92", "");
BLITZ_DUMTOKE(lit_93, "\x93", "");
BLITZ_DUMTOKE(lit_94, "\x94", "");
BLITZ_DUMTOKE(lit_95, "\x95", "");
BLITZ_DUMTOKE(lit_96, "\x96", "");
BLITZ_DUMTOKE(lit_97, "\x97", "");
BLITZ_DUMTOKE(lit_98, "\x98", "");
BLITZ_DUMTOKE(lit_99, "\x99", "");
BLITZ_DUMTOKE(lit_9A, "\x9A", "");
BLITZ_DUMTOKE(lit_9B, "\x9B", "");
BLITZ_DUMTOKE(lit_9C, "\x9C", "");
BLITZ_DUMTOKE(lit_9D, "\x9D", "");
BLITZ_DUMTOKE(lit_9E, "\x9E", "");
BLITZ_DUMTOKE(lit_9F, "\x9F", "");
BLITZ_DUMTOKE(lit_A0, "\xA0", "");
BLITZ_DUMTOKE(lit_A1, "\xA1", "");
BLITZ_DUMTOKE(lit_A2, "\xA2", "");
BLITZ_DUMTOKE(lit_A3, "\xA3", "");
BLITZ_DUMTOKE(lit_A4, "\xA4", "");
BLITZ_DUMTOKE(lit_A5, "\xA5", "");
BLITZ_DUMTOKE(lit_A6, "\xA6", "");
BLITZ_DUMTOKE(lit_A7, "\xA7", "");
BLITZ_DUMTOKE(lit_A8, "\xA8", "");
BLITZ_DUMTOKE(lit_A9, "\xA9", "");
BLITZ_DUMTOKE(lit_AA, "\xAA", "");
BLITZ_DUMTOKE(lit_AB, "\xAB", "");
BLITZ_DUMTOKE(lit_AC, "\xAC", "");
BLITZ_DUMTOKE(lit_AD, "\xAD", "");
BLITZ_DUMTOKE(lit_AE, "\xAE", "");
BLITZ_DUMTOKE(lit_AF, "\xAF", "");
BLITZ_DUMTOKE(lit_B0, "\xB0", "");
BLITZ_DUMTOKE(lit_B1, "\xB1", "");
BLITZ_DUMTOKE(lit_B2, "\xB2", "");
BLITZ_DUMTOKE(lit_B3, "\xB3", "");
BLITZ_DUMTOKE(lit_B4, "\xB4", "");
BLITZ_DUMTOKE(lit_B5, "\xB5", "");
BLITZ_DUMTOKE(lit_B6, "\xB6", "");
BLITZ_DUMTOKE(lit_B7, "\xB7", "");
BLITZ_DUMTOKE(lit_B8, "\xB8", "");
BLITZ_DUMTOKE(lit_B9, "\xB9", "");
BLITZ_DUMTOKE(lit_BA, "\xBA", "");
BLITZ_DUMTOKE(lit_BB, "\xBB", "");
BLITZ_DUMTOKE(lit_BC, "\xBC", "");
BLITZ_DUMTOKE(lit_BD, "\xBD", "");
BLITZ_DUMTOKE(lit_BE, "\xBE", "");
BLITZ_DUMTOKE(lit_BF, "\xBF", "");
BLITZ_DUMTOKE(lit_C0, "\xC0", "");
BLITZ_DUMTOKE(lit_C1, "\xC1", "");
BLITZ_DUMTOKE(lit_C2, "\xC2", "");
BLITZ_DUMTOKE(lit_C3, "\xC3", "");
BLITZ_DUMTOKE(lit_C4, "\xC4", "");
BLITZ_DUMTOKE(lit_C5, "\xC5", "");
BLITZ_DUMTOKE(lit_C6, "\xC6", "");
BLITZ_DUMTOKE(lit_C7, "\xC7", "");
BLITZ_DUMTOKE(lit_C8, "\xC8", "");
BLITZ_DUMTOKE(lit_C9, "\xC9", "");
BLITZ_DUMTOKE(lit_CA, "\xCA", "");
BLITZ_DUMTOKE(lit_CB, "\xCB", "");
BLITZ_DUMTOKE(lit_CC, "\xCC", "");
BLITZ_DUMTOKE(lit_CD, "\xCD", "");
BLITZ_DUMTOKE(lit_CE, "\xCE", "");
BLITZ_DUMTOKE(lit_CF, "\xCF", "");
BLITZ_DUMTOKE(lit_D0, "\xD0", "");
BLITZ_DUMTOKE(lit_D1, "\xD1", "");
BLITZ_DUMTOKE(lit_D2, "\xD2", "");
BLITZ_DUMTOKE(lit_D3, "\xD3", "");
BLITZ_DUMTOKE(lit_D4, "\xD4", "");
BLITZ_DUMTOKE(lit_D5, "\xD5", "");
BLITZ_DUMTOKE(lit_D6, "\xD6", "");
BLITZ_DUMTOKE(lit_D7, "\xD7", "");
BLITZ_DUMTOKE(lit_D8, "\xD8", "");
BLITZ_DUMTOKE(lit_D9, "\xD9", "");
BLITZ_DUMTOKE(lit_DA, "\xDA", "");
BLITZ_DUMTOKE(lit_DB, "\xDB", "");
BLITZ_DUMTOKE(lit_DC, "\xDC", "");
BLITZ_DUMTOKE(lit_DD, "\xDD", "");
BLITZ_DUMTOKE(lit_DE, "\xDE", "");
BLITZ_DUMTOKE(lit_DF, "\xDF", "");
BLITZ_DUMTOKE(lit_E0, "\xE0", "");
BLITZ_DUMTOKE(lit_E1, "\xE1", "");
BLITZ_DUMTOKE(lit_E2, "\xE2", "");
BLITZ_DUMTOKE(lit_E3, "\xE3", "");
BLITZ_DUMTOKE(lit_E4, "\xE4", "");
BLITZ_DUMTOKE(lit_E5, "\xE5", "");
BLITZ_DUMTOKE(lit_E6, "\xE6", "");
BLITZ_DUMTOKE(lit_E7, "\xE7", "");
BLITZ_DUMTOKE(lit_E8, "\xE8", "");
BLITZ_DUMTOKE(lit_E9, "\xE9", "");
BLITZ_DUMTOKE(lit_EA, "\xEA", "");
BLITZ_DUMTOKE(lit_EB, "\xEB", "");
BLITZ_DUMTOKE(lit_EC, "\xEC", "");
BLITZ_DUMTOKE(lit_ED, "\xED", "");
BLITZ_DUMTOKE(lit_EE, "\xEE", "");
BLITZ_DUMTOKE(lit_EF, "\xEF", "");
BLITZ_DUMTOKE(lit_F0, "\xF0", "");
BLITZ_DUMTOKE(lit_F1, "\xF1", "");
BLITZ_DUMTOKE(lit_F2, "\xF2", "");
BLITZ_DUMTOKE(lit_F3, "\xF3", "");
BLITZ_DUMTOKE(lit_F4, "\xF4", "");
BLITZ_DUMTOKE(lit_F5, "\xF5", "");
BLITZ_DUMTOKE(lit_F6, "\xF6", "");
BLITZ_DUMTOKE(lit_F7, "\xF7", "");
BLITZ_DUMTOKE(lit_F8, "\xF8", "");
BLITZ_DUMTOKE(lit_F9, "\xF9", "");
BLITZ_DUMTOKE(lit_FA, "\xFA", "");
BLITZ_DUMTOKE(lit_FB, "\xFB", "");
BLITZ_DUMTOKE(lit_FC, "\xFC", "");
BLITZ_DUMTOKE(lit_FD, "\xFD", "");
BLITZ_DUMTOKE(lit_FE, "\xFE", "");
BLITZ_DUMTOKE(lit_FF, "\xFF", "");

BLITZ_LIBFIN();
