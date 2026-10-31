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
 * Registers: a3 is advanced by len (past the copied characters); d1 and
 * a0 are kept (memlib keeps a0), d0/a1 changed; gstringass leaves a6+8.
 */
#include "blitzlib.h"
#include "blitzobj.h"

/* entry points (thunks, defined below) */
void stringass(void), gstringass(void);

/* !syslibheader{#strings2lib,$c700,$c702,0,0,0,0} */
BLITZ_SYSLIBHEADER7(LIB_strings2lib, 0xc700, 0xc702, 0, BL_NORET, 0, 0);
BLITZ_NULLSUB(stringass, 0, 0, LIB_memlib, BL_LA(6));   /* $C700 lastring */
BLITZ_NULLSUB(gstringass, 0, 0, LIB_memlib, BL_LA(6));  /* $C701 astring */
BLITZ_LIBFIN();

/* membase table of memlib (blitzhelp.h MEMBASE_xxx): JSR (a6) alloc,
   JSR 4(a6) free; the global versions are at +8 */

/* the header in front of a string's characters: struct bb_strmem
   (blitzobj.h) */

static void assign(ULONG len, UBYTE **var, const UBYTE *mem)
{
    UBYTE *s = *var;
    struct bb_strmem *h;

    if (s) {
        h = (struct bb_strmem *)s - 1;
        if (len <= h->alloced) {               /* CMP.l -8(a1),d0:BHI 'lengthen */
            h->len = len;                       /* MOVE.l d0,-4(a1) */
            goto copy;                          /* (BEQ 'endass: copies nothing) */
        }
        {                                       /* 'lengthen */
            struct bl_regs r = { .d0 = h->alloced + 9, .a1 = (ULONG)h };
            BLITZ_BASEJSR(mem, MEMBASE_locfree, r);
        }
    }
    {                                           /* 'allocnews */
        struct bl_regs r = { .d0 = len + 9, .d1 = MEMF_PUBLIC };
        h = (struct bb_strmem *)BLITZ_BASEJSR(mem, MEMBASE_localloc, r);
    }
    h->alloced = len;
    h->len = len;
    s = (UBYTE *)(h + 1);
    *var = s;                                   /* MOVE.l a1,(a2) */
copy:
    /* 'nofree: the original copies 8 bytes per loop plus the rest; the
       result is the same as this byte loop */
    while (len--)
        *s++ = *blitz_string++;
    *s = 0;                                     /* 'endass: CLR.b (a1) */
}

/**
 * @brief $C700 (lastring) / $C701 (astring) - assign the string at a3 to
 * a local / global string variable.
 *
 * If the new string fits into the allocated length, the memory is reused;
 * otherwise it is freed and len+9 bytes are allocated (memlib localloc /
 * locfree; gstringass: ADDQ #8,a6 first for globalloc/globfree, in the
 * thunk as in the original: a6 stays +8).  a3 is advanced by len; d1 and
 * a0 are kept, d0/a1 changed.
 *
 * @param len d0: the length - 4(a7), removed by the routine (MOVE.l
 *            (a7)+,(a7):RTS), loaded by the thunk
 * @param var a2: the address of the string variable
 * @param mem a6: memlib's membase table
 */
static void BLITZ_ROUTINE stringass_c(ULONG len BL_REG(d0), UBYTE **var BL_REG(a2),
                                      const UBYTE *mem BL_REG(a6))
{
    assign(len, var, mem);
}

/* saved d1/a0 (8 bytes) + return address: len at 12(sp); the routine
   removes it (MOVE.l (a7)+,(a7):RTS) */
BLITZ_THUNK_POP(stringass, stringass_c, "d1/a0", "move.l 12(sp),d0", "", 4);
BLITZ_THUNK_POP(gstringass, stringass_c, "d1/a0", "addq.w #8,a6\n   move.l 12(sp),d0", "", 4);
