/*
 * testlib - the Blitz test library, user library 81 (LIB_testlib): the
 * routines a test driver calls (spec testing/basic-test-interface, "The
 * library routines"; testing/basic-testlib).  The driver defines the
 * macros of shared/basictest/macros.bas, which call them.
 *
 *   TLBegin path$              opens the result file, writes `begin`
 *   r=TLInit / r=TLFinal       opens the batch init / cleanup: 1 if it runs
 *   r=TLCase(name$)            opens a case: 1 if it runs
 *   TLBlockEnd                 closes the open block
 *   TLCheck K,expected,actual  a check of two longs
 *   TLCheckQ K,exp.q,act.q     a check of two quicks (exactly)
 *   TLCheckFlt K,e.f,a.f,d.f   a check of two floats: |a-e| <= d, with
 *                              mathffp.library
 *   TLCheckStr K,exp$,act$     a check of two strings
 *   TLRequire K,condition      a precondition: if it is 0 the open
 *                              block's TLInit/TLCase/TLFinal call returns
 *                              a second time, with 0
 *   TLExpectError K            announces an expected runtime error
 *   finit                      `incomplete` if a block is open, `end`
 *
 * The entries and their order are tlcore.c's; this file is the Amiga
 * side: the result file (opened once by TLBegin, one dos Write per
 * entry, closed by the finit), whether an entry can be written now (not
 * while exec's TDNestCnt or IDNestCnt is 0 or more: Blitz mode, Forbid,
 * Disable), and the second return of the block calls.
 *
 * What the library keeps for the tested program: between TLBegin and the
 * finit it allocates and frees nothing (static state, the file opened in
 * TLBegin), and every routine keeps d2-d7 and a2-a6 besides the
 * registers of the register rule (the C routines by the calling
 * convention; TLInit, TLCase and TLFinal are assembler entries that save
 * the caller's registers for the second return, which restores them).
 * dos.library and mathffp.library are opened by TLBegin and closed by the
 * finit, so no routine calls a library before the start.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/execbase.h>
#include <dos/dos.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME tl_dosbase
#include <proto/dos.h>
#undef __NOLIBBASE__
#include "tlcore.h"

void TLBegin(void), TLInit(void), TLFinal(void), TLCase(void), TLBlockEnd(void);
void TLCheck(void), TLCheckStr(void), TLRequire(void), TLExpectError(void);
void TLCheckQ(void), TLCheckFlt(void);
static void BLITZ_ROUTINE codefinit(void);
BLITZ_ENTRY_DECL(finit);

BLITZ_LIBHEADER(LIB_testlib, 0, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(TLBegin, 0, 0);
BLITZ_NAME("TLBegin", "path$: start the test library, result file path$");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(TLInit, 0, 0);
BLITZ_NAME("TLInit", "run=TLInit: open the batch init (0: does not run)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(TLFinal, 0, 0);
BLITZ_NAME("TLFinal", "run=TLFinal: open the batch cleanup (0: does not run)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(TLCase, 0, 0);
BLITZ_NAME("TLCase", "run=TLCase(name$): open a case (0: does not run)");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(TLBlockEnd, 0, 0);
BLITZ_NAME("TLBlockEnd", "close the open block");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(TLCheck, 0, 0);
BLITZ_NAME("TLCheck", "K,expected,actual");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(TLCheckStr, 0, 0);
BLITZ_NAME("TLCheckStr", "K,expected$,actual$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(TLRequire, 0, 0);
BLITZ_NAME("TLRequire", "K,condition: 0 leaves the block");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(TLExpectError, 0, 0);
BLITZ_NAME("TLExpectError", "K: an expected runtime error follows");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_QUICK, BL_QUICK);
BLITZ_LIBS0();
BLITZ_SUBS(TLCheckQ, 0, 0);
BLITZ_NAME("TLCheckQ", "K,expected.q,actual.q");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_FLOAT, BL_FLOAT, BL_FLOAT);
BLITZ_LIBS0();
BLITZ_SUBS(TLCheckFlt, 0, 0);
BLITZ_NAME("TLCheckFlt", "K,expected.f,actual.f,delta.f: ok if |actual-expected|<=delta");

BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- state */

/* the context of the last TLInit/TLCase/TLFinal call for the second
   return: d2-d7/a2-a6 (44 bytes), the return address, the stack pointer
   behind it (read and written by the assembler below) */
ULONG tl_ctx[13];

static struct tl_state tl_st;
static struct Library *tl_dosbase;
static struct Library *tl_ffpbase;      /* mathffp.library */
/* the result file and its complement: a file handle a wild write changed
   is not used */
static BPTR tl_fh, tl_fhcheck;

static int file_ok(void)
{
    return tl_fh && tl_fhcheck == ~tl_fh;
}

void tl_os_write(const uint8_t *data, size_t length)
{
    if (file_ok() && length)
        Write(tl_fh, (APTR)data, length);
}

int tl_os_can_write(void)
{
    return SysBase->TDNestCnt < 0 && SysBase->IDNestCnt < 0;
}

/* |actual-expected| <= delta with mathffp.library: SPSub (d0 = d0 - d1),
   SPAbs, and the sign of delta - |actual-expected| (bit 7 of an FFP
   number; 0 is all zero bits) */
int tl_os_fltnear(uint32_t expected, uint32_t actual, uint32_t delta)
{
    ULONG diff;

    if (!tl_ffpbase)
        return -1;
    diff = BLITZ_SYSJSR2(tl_ffpbase, LVO_SPSub, actual, expected);
    diff = BLITZ_SYSJSR1(tl_ffpbase, LVO_SPAbs, diff);
    return !(BLITZ_SYSJSR2(tl_ffpbase, LVO_SPSub, delta, diff) & 0x80);
}

/* ------------------------------------------------------------- routines */

/**
 * @brief TLBegin path$: opens dos.library, mathffp.library and the result file path$
 *   (MODE_NEWFILE), writes `begin`.  A second TLBegin writes `broken` and
 *   changes nothing.
 * @param path  d0: the path's characters
 * @param lens  a2: the compiler's string stack, -4(a2) = its length
 */
static void BLITZ_ROUTINE tlbegin_c(const UBYTE *path BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    UBYTE name[256];
    ULONG n = lens[-1], i;

    if (tl_st.begun && tl_st.guard_lo == TL_GUARD_LO && tl_st.guard_hi == TL_GUARD_HI) {
        tl_begin(&tl_st);
        return;
    }
    tl_fh = tl_fhcheck = 0;
    if (n > sizeof name - 1)
        n = sizeof name - 1;
    for (i = 0; i < n; i++)
        name[i] = path[i];
    name[n] = 0;
    if (!tl_dosbase)
        tl_dosbase = OpenLibrary((CONST_STRPTR)"dos.library", 33);
    if (tl_dosbase) {
        tl_fh = Open((CONST_STRPTR)name, MODE_NEWFILE);
        tl_fhcheck = ~tl_fh;
    }
    if (!tl_ffpbase)
        tl_ffpbase = OpenLibrary((CONST_STRPTR)"mathffp.library", 0);
    tl_begin(&tl_st);
}
BLITZ_ALIAS(TLBegin, tlbegin_c);

/* the C parts of the assembler entries TLInit, TLFinal, TLCase */
ULONG BLITZ_ROUTINE tl_init_c(void);
ULONG BLITZ_ROUTINE tl_init_c(void)
{
    return tl_block(&tl_st, BASICTEST_BLOCK_INIT, 0, 0);
}

ULONG BLITZ_ROUTINE tl_final_c(void);
ULONG BLITZ_ROUTINE tl_final_c(void)
{
    return tl_block(&tl_st, BASICTEST_BLOCK_FINAL, 0, 0);
}

ULONG BLITZ_ROUTINE tl_case_c(const UBYTE *name BL_REG(d0), const ULONG *lens BL_REG(a2));
ULONG BLITZ_ROUTINE tl_case_c(const UBYTE *name BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    return tl_block(&tl_st, BASICTEST_BLOCK_CASE, name, lens[-1]);
}

static void BLITZ_ROUTINE tlblockend_c(void)
{
    tl_blockend(&tl_st);
}
BLITZ_ALIAS(TLBlockEnd, tlblockend_c);

static void BLITZ_ROUTINE tlcheck_c(ULONG k BL_REG(d0), ULONG expected BL_REG(d1),
                                    ULONG actual BL_REG(d2))
{
    tl_check(&tl_st, k, expected, actual);
}
BLITZ_ALIAS(TLCheck, tlcheck_c);

static void BLITZ_ROUTINE tlcheckq_c(ULONG k BL_REG(d0), ULONG expected BL_REG(d1),
                                     ULONG actual BL_REG(d2))
{
    tl_checkq(&tl_st, k, expected, actual);
}
BLITZ_ALIAS(TLCheckQ, tlcheckq_c);

static void BLITZ_ROUTINE tlcheckflt_c(ULONG k BL_REG(d0), ULONG expected BL_REG(d1),
                                       ULONG actual BL_REG(d2), ULONG delta BL_REG(d3))
{
    tl_checkflt(&tl_st, k, expected, actual, delta);
}
BLITZ_ALIAS(TLCheckFlt, tlcheckflt_c);

/**
 * @brief TLCheckStr K,expected$,actual$.
 * @param lens  a2: -4(a2) = the length of expected$, -8(a2) = of actual$
 */
static void BLITZ_ROUTINE tlcheckstr_c(ULONG k BL_REG(d0), const UBYTE *expected BL_REG(d1),
                                       const UBYTE *actual BL_REG(d2),
                                       const ULONG *lens BL_REG(a2))
{
    tl_checkstr(&tl_st, k, expected, lens[-1], actual, lens[-2]);
}
BLITZ_ALIAS(TLCheckStr, tlcheckstr_c);

/* leaves the open block: its TLInit/TLCase/TLFinal call returns again,
   with d0 = 0 (assembler, below) */
void tl_resume(void);

static void BLITZ_ROUTINE tlrequire_c(ULONG k BL_REG(d0), ULONG condition BL_REG(d1))
{
    if (tl_require(&tl_st, k, condition))
        tl_resume();
}
BLITZ_ALIAS(TLRequire, tlrequire_c);

static void BLITZ_ROUTINE tlexpecterror_c(ULONG k BL_REG(d0))
{
    tl_expect_error(&tl_st, k);
}
BLITZ_ALIAS(TLExpectError, tlexpecterror_c);

/* TLInit, TLFinal, TLCase: save the caller's context for the second
   return, then the C part (which keeps d2-d7/a2-a6) */
#define TL_SAVE                                                             \
    "   movem.l d2-d7/a2-a6,_tl_ctx\n"                                      \
    "   move.l  (sp),_tl_ctx+44\n"                                          \
    "   lea     4(sp),a0\n"                                                 \
    "   move.l  a0,_tl_ctx+48\n"

BLITZ_ASM_ROUTINE(TLInit, TL_SAVE "   jbra    _tl_init_c");
BLITZ_ASM_ROUTINE(TLFinal, TL_SAVE "   jbra    _tl_final_c");
BLITZ_ASM_ROUTINE(TLCase, TL_SAVE "   jbra    _tl_case_c");

BLITZ_ASM_ROUTINE(tl_resume,
"   movem.l _tl_ctx,d2-d7/a2-a6\n"
"   move.l  _tl_ctx+48,sp\n"
"   move.l  _tl_ctx+44,a0\n"
"   moveq   #0,d0\n"
"   jmp     (a0)");

/* finit: the last entries, the file closed, the libraries closed */
static void BLITZ_ROUTINE codefinit(void)
{
    tl_finit(&tl_st);
    if (tl_dosbase) {
        if (file_ok())
            Close(tl_fh);
        CloseLibrary(tl_dosbase);
    }
    if (tl_ffpbase)
        CloseLibrary(tl_ffpbase);
    tl_fh = tl_fhcheck = 0;
    tl_dosbase = 0;
    tl_ffpbase = 0;
}
