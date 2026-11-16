/*
 * strings2lib - system library 65135 ($C700-$C701): assign a string to a
 * string variable (tokens lastring/astring, blitz2.s equates 157-158:
 * local and global string assignment, also Shared/parameter strings).
 *
 * C translation of bblibsrc/strings2lib (binary checked: same as the
 * source).  From the original: "ass string at a3 to a2. len in 4(a7)".
 *   a2 = address of the string variable, a3 = the characters (the end of
 *   the string work area minus len), 4(a7) = len (removed by the routine),
 *   a6 = memlib's membase table (!libs #memlib,#la6).
 * String format as follows:
 *   -8...alloc'd len
 *   -4...string len
 *    0...characters,0.b
 * The variable holds the address of the characters (0 = no memory yet).
 * If the new string fits into the allocated length, the memory is reused;
 * otherwise it is freed and len+9 bytes allocated (memlib localloc /
 * locfree, or globalloc/globfree for gstringass: ADDQ #8,a6).
 *
 * Registers: a3 is advanced by len (past the copied characters); d1-d7
 * and a2 are kept (the contract the compiler uses), d0/a0/a1 changed;
 * gstringass leaves a6+8.
 */
#include "blitzlib.h"
#include "blitzobj.h"

/* entry points (the assembler front end, defined below) */
void stringass(void), gstringass(void);

/* !syslibheader{#strings2lib,$c700,$c702,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_strings2lib, 0xc700, 0xc702, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(stringass, 0, 0, LIB_memlib, BL_LA6);     /* $C700 lastring */
BLITZ_NULLSUB(gstringass, 0, 0, LIB_memlib, BL_LA6);    /* $C701 astring */
BLITZ_LIBFIN();

/* membase table of memlib (blitzhelp.h MEMBASE_xxx): JSR (a6) alloc,
   JSR 4(a6) free; the global versions are at +8.  By its tested contract
   (the KEEPS cases of libs/tests/systest.c) alloc keeps d2-d7/a0-a3/a6
   and free d1-d7/a0/a2/a3/a6. */
static_assert(MEMBASE_localloc == 0 && MEMBASE_locfree == 4,
              "stringass: JSR (a6) alloc, JSR 4(a6) free");

/* the header in front of a string's characters: struct bb_strmem
   (blitzobj.h): -8 alloc'd len, -4 string len */
static_assert(sizeof(struct bb_strmem) == 8, "stringass: -8/-4 header");

/**
 * @brief the copy of lastring/astring: the characters from a3 to the
 * variable's memory, then the terminating 0 ('nofree, BL_STRCOPY;
 * 'endass: CLR.b (a1)); a3 is advanced by len.
 *
 * Uses only d0, d1, a0, a1 and a3 and saves nothing (checked in the
 * disassembly), so the front end around it saves only d1.
 *
 * @param len d0: the length
 * @param d a0: the characters of the variable
 */
static void BLITZ_ROUTINE strcopy_c(ULONG len BL_REG(d0), UBYTE *d BL_REG(a0))
{
    BL_STRCOPY(d, blitz_string, len);
    *d = 0;
}

/**
 * @brief $C700 (lastring) / $C701 (astring) - assign the string at a3 to
 * a local / global string variable.
 *
 * The original's front end: if the new string fits into the allocated
 * length, the memory is reused and only its length set (CMP.l
 * -8(a1),d0:BHI 'lengthen); otherwise the old memory, if there is one, is
 * freed ('lengthen: alloc'd len + 9 bytes, JSR 4(a6) locfree), len + 9
 * bytes are allocated ('allocnews: JSR (a6) localloc, MEMF_PUBLIC), both
 * lengths set to len and the address of the characters stored in the
 * variable.  gstringass adds 8 to a6 first (globalloc/globfree) and falls
 * into stringass, as in the original: a6 stays +8.  Then strcopy_c copies
 * the characters (C, BL_STRCOPY).  The original skips the copy for a
 * string of length 0 that fits (BEQ 'endass); here strcopy_c writes only
 * the 0 then.
 *
 * Registers: the compiler relies on d1-d7 and a2 across the call (the
 * later string parameters of a procedure are still in d1-d5; a comma list
 * of assignments advances a2 after it, thunks.md 1.4); d1 is saved (the
 * alloc flags, BL_STRCOPY), memlib and strcopy_c keep the others.  The
 * length is removed from the stack (MOVE.l (a7)+,(a7):RTS).
 *
 * @param len 4(a7): the length, removed by the routine
 * @param var a2: the address of the string variable
 * @param mem a6: memlib's membase table
 */
BLITZ_ASM_BLOCK(stringass,
"   .globl  _gstringass\n"
"_gstringass:\n"
"   addq.w  #8,a6\n"
"   .globl  _stringass\n"
"_stringass:\n"
"   move.l  d1,-(sp)\n"
"   move.l  (a2),d0\n"
"   beq.s   2f\n"                       /* no memory yet: 'allocnews */
"   move.l  d0,a0\n"
"   move.l  8(sp),d0\n"                 /* len */
"   cmp.l   -8(a0),d0\n"
"   bhi.s   1f\n"                       /* does not fit: 'lengthen */
"   move.l  d0,-4(a0)\n"
"3: bsr.w   _strcopy_c\n"               /* 'nofree */
"   move.l  (sp)+,d1\n"
"   move.l  (sp)+,(sp)\n"
"   rts\n"
"1: moveq   #9,d0\n"                    /* 'lengthen */
"   lea     -8(a0),a1\n"
"   add.l   (a1),d0\n"
"   jsr     " BL_STR(MEMBASE_locfree) "(a6)\n"
"2: moveq   #9,d0\n"                    /* 'allocnews */
"   add.l   8(sp),d0\n"
"   moveq   #1,d1\n"                    /* MEMF_PUBLIC */
"   jsr     (a6)\n"                     /* MEMBASE_localloc */
"   move.l  d0,a0\n"
"   move.l  8(sp),d0\n"
"   move.l  d0,(a0)+\n"                 /* alloc'd len */
"   move.l  d0,(a0)+\n"                 /* string len */
"   move.l  a0,(a2)\n"
"   bra.s   3b");
