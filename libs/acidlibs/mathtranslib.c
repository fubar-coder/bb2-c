/*
 * mathtranslib - command library 192: the transcendental FFP functions
 * (ACos ... HTan) and the ^ operator for floats (first nullsub, dopow,
 * called by the compiler).
 *
 * C translation of bblibsrc/mathtranslib (binary checked: same as the
 * source; the second `#mathtranslib=192` of libnums.bb is the one used).
 *   init:   open mathtrans.library (OldOpenLibrary); the base is the init
 *           value.  BUG(orig): if it cannot be opened, init tries again
 *           forever (TST.l d0:BEQ initloop).
 *   finit:  close it (a1 = the base, !libs #mathtranslib,#la1).
 *   functions: every one is an inline routine (`Dc $a000,len`): the
 *           compiler copies `JSR lvo(a6)` to the call, a6 = the base
 *           (!libs #mathtranslib,#la6), d0 = argument, d1 = exponent for
 *           dopow (SPPow).  Inline code is assembler by nature
 *           (BLITZ_INLINE); the original has no RTS behind it.
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (defined below) */
ULONG codeinit(void);
void codefinit(struct Library *lib BL_REG(a1));
void dopow(void), spacos(void), spasin(void), spatan(void), spcos(void);
void spcosh(void), spexp(void), splog10(void), splog(void), spsin(void);
void spsinh(void), spsqrt(void), sptan(void), sptanh(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#mathtranslib,init,1,finit,0} */
BLITZ_LIBHEADER(LIB_mathtranslib, init, BL_LONGRET, finit, 0);

BLITZ_NULLSUB(dopow, 0, 0, LIB_mathtranslib, BL_LA(6));

#define MT_FUNC(code, name)                                 \
    BLITZ_AFUNCTION(BL_FLOAT);                              \
    BLITZ_ARGS(BL_FLOAT);                                   \
    BLITZ_LIBS(LIB_mathtranslib, BL_LA(6));                 \
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
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0, LIB_mathtranslib, BL_LA(1));

BLITZ_LIBFIN();

static const char libname[] = "mathtrans.library";

/* codeinit - initloop: OldOpenLibrary until it succeeds */
ULONG BLITZ_ROUTINE codeinit(void)
{
    struct Library *base;

    do
        base = OldOpenLibrary((CONST_STRPTR)libname);
    while (!base);
    return (ULONG)base;
}

/* codefinit - MOVE.l 4,a6:JMP _CloseLibrary(a6) */
void BLITZ_ROUTINE codefinit(struct Library *lib BL_REG(a1))
{
    CloseLibrary(lib);
}

/* mathtrans.library LVOs */
BLITZ_INLINE(dopow,   "jsr -90(a6)");                   /* SPPow: d0^d1 */
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
