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

/* entry points (the thunk, defined below) */
void stringass(void), gstringass(void);

/* !syslibheader{#strings2lib,$c700,$c702,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_strings2lib, 0xc700, 0xc702, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(stringass, 0, 0, LIB_memlib, BL_LA6);     /* $C700 lastring */
BLITZ_NULLSUB(gstringass, 0, 0, LIB_memlib, BL_LA6);    /* $C701 astring */
BLITZ_LIBFIN();

/* membase table of memlib (blitzhelp.h MEMBASE_xxx): JSR (a6) alloc,
   JSR 4(a6) free; the global versions are at +8.  Called with
   BLITZ_SYSJSR/BLITZ_SYSJSR2: by its tested contract (the KEEPS cases of
   libs/tests/systest.c) alloc changes d0/d1 only and free d0/a1 only, so
   GCC saves no register for the call. */

/* the header in front of a string's characters: struct bb_strmem
   (blitzobj.h) */

/**
 * @brief the allocation path of lastring/astring: the string does not
 * fit, or the variable has no memory yet.
 *
 * Frees the old memory if there is one ('lengthen: alloc'd len + 9
 * bytes, memlib locfree), allocates len + 9 bytes ('allocnews: memlib
 * localloc), sets both lengths to len and stores the address of the
 * characters in the variable.  Out of line (`noinline`), so that the
 * common path in stringass_c (the string fits) saves no register.
 *
 * @param len d0: the length
 * @param var a2: the address of the string variable
 * @param mem a6: memlib's membase table (+8 for astring)
 * @return d0: the characters (the new value of the variable)
 */
static __attribute__((noinline)) UBYTE *BLITZ_ROUTINE
stringass_new(ULONG len BL_REG(d0), UBYTE **var BL_REG(a2), const UBYTE *mem BL_REG(a6))
{
    UBYTE *s = *var;
    struct bb_strmem *h;

    if (s) {                                    /* 'lengthen */
        struct bl_regs r;
        h = (struct bb_strmem *)s - 1;
        r = (struct bl_regs){ .d0 = h->alloced + 9, .a1 = (ULONG)h };
        BLITZ_SYSJSR(mem, MEMBASE_locfree, r);
    }
    /* 'allocnews */
    h = (struct bb_strmem *)BLITZ_SYSJSR2(mem, MEMBASE_localloc,
                                          len + 9, MEMF_PUBLIC);
    h->alloced = len;
    h->len = len;
    s = (UBYTE *)(h + 1);
    *var = s;                                   /* MOVE.l a1,(a2) */
    return s;
}

/**
 * @brief $C700 (lastring) / $C701 (astring) - assign the string at a3 to
 * a local / global string variable.
 *
 * If the new string fits into the allocated length, the memory is reused
 * and only its length set (CMP.l -8(a1),d0:BHI 'lengthen); otherwise
 * stringass_new frees it and allocates len+9 bytes (gstringass: ADDQ
 * #8,a6 first for globalloc/globfree, in the thunk as in the original: a6
 * stays +8).  Then the characters are copied ('nofree, BL_STRCOPY) and
 * terminated with 0 ('endass: CLR.b (a1)); a3 is advanced by len.  The
 * length after stringass_new is read back from the header, so that it
 * is not kept in a register across the call: the function uses only d0,
 * d1, a0, a1 and a3 and saves nothing (checked in the disassembly).
 *
 * @param len d0: the length - 4(a7), removed by the routine (MOVE.l
 *            (a7)+,(a7):RTS), loaded by the thunk
 * @param var a2: the address of the string variable
 * @param mem a6: memlib's membase table
 */
static void BLITZ_ROUTINE stringass_c(ULONG len BL_REG(d0), UBYTE **var BL_REG(a2),
                                      const UBYTE *mem BL_REG(a6))
{
    UBYTE *d = *var;

    if (d && len <= ((struct bb_strmem *)d)[-1].alloced) {
        ((struct bb_strmem *)d)[-1].len = len;  /* MOVE.l d0,-4(a1) */
    } else {
        d = stringass_new(len, var, mem);
        len = ((struct bb_strmem *)d)[-1].len;
    }
    BL_STRCOPY(d, blitz_string, len);
    *d = 0;
}

/* The thunk: the compiler relies on d1-d7 and a2 across the call (the
   later string parameters of a procedure are still in d1-d5; a comma
   list of assignments advances a2 after it, thunks.md 1.4); GCC keeps
   d2-d7/a2 itself, so only d1 is saved.  Then the length is removed from
   the stack (MOVE.l (a7)+,(a7):RTS).  gstringass adds 8 to a6 and falls
   into stringass, as in the original. */
BLITZ_ASM_BLOCK(stringass,
"   .globl  _gstringass\n"
"_gstringass:\n"
"   addq.w  #8,a6\n"
"   .globl  _stringass\n"
"_stringass:\n"
"   move.l  d1,-(sp)\n"
"   move.l  8(sp),d0\n"
"   bsr.w   _stringass_c\n"
"   move.l  (sp)+,d1\n"
"   move.l  (sp)+,(sp)\n"
"   rts");
