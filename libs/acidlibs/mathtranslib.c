/*
 * mathtranslib - command library 192: the transcendental FFP functions
 * (ACos ... HTan) and the ^ operator for floats (first nullsub, dopow,
 * called by the compiler).
 *
 * C translation of bblibsrc/mathtranslib (binary checked: same as the
 * source; the second `#mathtranslib=192` of libnums.bb is the one used).
 *   init:   open mathtrans.library (OldOpenLibrary); the base or 0 is the
 *           init value.  Fixed (#202): the original tries again forever if
 *           it cannot be opened (TST.l d0:BEQ initloop).
 *   finit:  close it if the base is not 0 (a1 = the base, !libs
 *           #mathtranslib,#la1).
 *   functions: the transcendental functions are inline routines
 *           (`Dc $a000,len`): the compiler copies `JSR lvo(a6)` to the
 *           call, a6 = the base (!libs #mathtranslib,#la6), d0 = argument.
 *           Inline code is assembler by nature (BLITZ_INLINE); the
 *           original has no RTS behind it.  Fixed (#1189): the ^ operator
 *           (dopow, d0 = x, d1 = y) is a routine around SPPow, no longer
 *           the inline SPPow call, which drops the sign of a negative base.
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (defined below) */
static ULONG codeinit(void);
static void codefinit(struct Library *lib BL_REG(a1));
void dopow(void), spacos(void), spasin(void), spatan(void), spcos(void);
void spcosh(void), spexp(void), splog10(void), splog(void), spsin(void);
void spsinh(void), spsqrt(void), sptan(void), sptanh(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#mathtranslib,init,1,finit,0} */
BLITZ_LIBHEADER(LIB_mathtranslib, init, BL_LONGRET, finit, 0);

BLITZ_NULLSUB(dopow, 0, 0, LIB_mathtranslib, BL_LA6);

#define MT_FUNC(code, name)                                 \
    BLITZ_AFUNCTION(BL_FLOAT);                              \
    BLITZ_ARGS(BL_FLOAT);                                   \
    BLITZ_LIBS(LIB_mathtranslib, BL_LA6);                   \
    BLITZ_SUBS(code, 0, 0);                                 \
    BLITZ_NAME(name, "(Float)")

MT_FUNC(spacos, "ACos");
MT_FUNC(spasin, "ASin");
MT_FUNC(spatan, "ATan");
MT_FUNC(spcos, "Cos");
MT_FUNC(spcosh, "HCos");
MT_FUNC(spexp, "Exp");
MT_FUNC(splog10, "Log10");
MT_FUNC(splog, "Log");
MT_FUNC(spsin, "Sin");
MT_FUNC(spsinh, "HSin");
MT_FUNC(spsqrt, "Sqr");
MT_FUNC(sptan, "Tan");
MT_FUNC(sptanh, "HTan");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0, LIB_mathtranslib, BL_LA1);

BLITZ_LIBFIN();

static const char libname[] = "mathtrans.library";

/**
 * @brief Library init - open mathtrans.library.
 *
 * OldOpenLibrary once: the base or 0.
 *
 * Fixed (#202): the original tries again forever if the library cannot
 * be opened (initloop: TST.l d0:BEQ initloop), so a program that uses a
 * transcendental function hung at startup without LIBS:mathtrans.library.
 * Now the program runs with base 0, as with a missing library of the
 * deflibs (#44): a call of a function of mathtranslib then crashes (JSR
 * to a negative offset from 0); a program that can do without them
 * checks for the library itself first.
 *
 * @return d0: the library base or 0, the init value (a6 of the functions).
 */
static ULONG BLITZ_ROUTINE codeinit(void)
{
    return (ULONG)OldOpenLibrary((CONST_STRPTR)libname);
}

/**
 * @brief Library finit - close mathtrans.library.
 *
 * MOVE.l 4,a6:JMP _CloseLibrary(a6); Fixed (#202): only if the base is
 * not 0 (with the init above it may be; CloseLibrary(0) crashes on
 * Kickstart 1.x).
 *
 * @param lib a1: the library base (!libs #mathtranslib,#la1).
 */
static void BLITZ_ROUTINE codefinit(struct Library *lib BL_REG(a1))
{
    if (lib)
        CloseLibrary(lib);
}

/* mathtrans.library LVOs */

/**
 * @brief The ^ operator for floats (the first nullsub, called by the
 * compiler) - x^y with SPPow, the sign of a negative base kept.
 *
 * Fixed (#1189): the original is the inline `JSR -90(a6)` (SPPow), which
 * gives |x|^y for a negative x, also for a whole odd exponent: (-2.5)^3
 * gave 15.625.  Now a base that is not negative jumps to SPPow as before;
 * a negative one is passed to SPPow as |x|, and the result is negative
 * when y is a whole odd number: (-2.5)^3 is -15.625, (-2)^2 4, (-2)^-1
 * -0.5.  A result 0 stays 0.  With a fractional exponent there is no real
 * power and the value stays |x|^y, as in the original (whether that
 * becomes a runtime error is open in #1189).
 *
 * Whole and odd from the FFP bits: the mantissa M in bits 31-8
 * (normalised), the sign in bit 7, the exponent in bits 6-0 (excess 64),
 * the value M * 2^(e-24) with e = exponent - 64.  e <= 0: 0 or below 1
 * in magnitude; e > 24: whole and even; otherwise y is a whole odd number
 * if bit 24-e of M is set and the 24-e bits below it are 0 (e = 24: the
 * value is M itself, 16777215 the largest odd one).  `LSL.l e` of the
 * mantissa (the low byte cleared) shifts bit 24-e of M into C and leaves
 * the bits below it, the fraction: C set and the rest 0.
 *
 * Assembler by nature: the tail jump into SPPow with the caller's
 * registers (the inline call it replaces, the cost of the common case
 * one test) and the bit test on the condition codes of the shift.  The
 * registers: those of the SPPow call (d0/d1/a0/a1 changed, a6 kept).
 *
 * @arg d0 x, the base (FFP).
 * @arg d1 y, the exponent (FFP).
 * @arg a6 the mathtrans.library base (!libs #mathtranslib,#la6).
 * @par Result
 * d0: x^y (FFP).
 */
BLITZ_ASM_ROUTINE(dopow,
        "   tst.b   d0\n"
        "   bmi.s   1f\n"
        "   jmp     -90(a6)\n"                 /* SPPow: x not negative */
        "1: and.b   #0x7f,d0\n"                /* |x| */
        "   move.l  d1,-(sp)\n"
        "   jsr     -90(a6)\n"                 /* SPPow: |x|^y */
        "   move.l  (sp)+,d1\n"
        "   tst.l   d0\n"
        "   beq.s   2f\n"                      /* 0 stays 0 */
        "   move.l  d0,a0\n"
        "   moveq   #0x7f,d0\n"
        "   and.b   d1,d0\n"
        "   sub.w   #64,d0\n"                  /* e */
        "   ble.s   3f\n"                      /* below 1: not whole odd */
        "   cmp.w   #24,d0\n"
        "   bgt.s   3f\n"                      /* from 2^24: even */
        "   clr.b   d1\n"                      /* the mantissa */
        "   lsl.l   d0,d1\n"                   /* C: bit 24-e of M */
        "   bcc.s   3f\n"                      /* even */
        "   tst.l   d1\n"
        "   bne.s   3f\n"                      /* a fraction */
        "   move.l  a0,d0\n"
        "   or.b    #0x80,d0\n"                /* odd: negative */
        "2: rts\n"
        "3: move.l  a0,d0\n"
        "   rts");

/**
 * @brief ACos, ASin, ATan, Cos, HCos, Exp, Log, Log10, Sin, HSin, Sqr,
 * Tan, HTan (Float) - the mathtrans.library function named in the comment
 * of each line.
 *
 * Inline code (`Dc $a000,len`): the compiler copies the `JSR lvo(a6)` to
 * the call; the original has no RTS behind it.
 *
 * @arg d0 the argument (FFP).
 * @arg a6 the mathtrans.library base (!libs #mathtranslib,#la6).
 * @par Result
 * d0: the result (FFP).
 */
BLITZ_INLINE(spacos,  "jsr -120(a6)");                  /* SPAcos */
BLITZ_INLINE(spasin,  "jsr -114(a6)");                  /* SPAsin */
BLITZ_INLINE(spatan,  "jsr -30(a6)");                   /* SPAtan */
BLITZ_INLINE(spcos,   "jsr -42(a6)");                   /* SPCos */
BLITZ_INLINE(spcosh,  "jsr -66(a6)");                   /* SPCosh */
BLITZ_INLINE(spexp,   "jsr -78(a6)");                   /* SPExp */
BLITZ_INLINE(splog,   "jsr -84(a6)");                   /* SPLog */
BLITZ_INLINE(splog10, "jsr -126(a6)");                  /* SPLog10 */
BLITZ_INLINE(spsin,   "jsr -36(a6)");                   /* SPSin */
BLITZ_INLINE(spsinh,  "jsr -60(a6)");                   /* SPSinh */
BLITZ_INLINE(spsqrt,  "jsr -96(a6)");                   /* SPSqrt */
BLITZ_INLINE(sptan,   "jsr -48(a6)");                   /* SPTan */
BLITZ_INLINE(sptanh,  "jsr -72(a6)");                   /* SPTanh */
