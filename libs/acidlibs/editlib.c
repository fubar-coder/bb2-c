/*
 * editlib - command library 186: Edit$ Inkey$ Edit.
 *
 * C translation of bblibsrc/editlib ("LIBRARY 241 - EDIT LIB"; binary of
 * the CD's acidlibs checked: same as the source).
 *
 * All input goes through inputoutputlib (164): Inkey$ reads single
 * characters (dogetsingle, ALibJsr +5: d0 = character, negative = none),
 * Edit$/Edit a line (dogetstring, +6: a0 = buffer, d0 = maximum, returns
 * the number of characters).  What that means depends on the current
 * input handler: DefaultInput = dos Read(Input(), buffer, max+1) - 1
 * (single characters: runtime error), FileInput = one line of the file,
 * WindowInput = editing in the window ...
 *
 * Edit$([Default$,]n): the default string is padded with spaces to n
 * characters in the string buffer (for handlers that edit the buffer),
 * then the line is read over it; result = the characters read.
 * Edit([Default,]n): the same in an own buffer of 80 characters, the
 * default value converted with UStr$ (printlib, $5D86), the line
 * converted with Val (vallib, $3C01).  Both calls get their a2 stack from
 * a fake block `pars` {length.l, type.w = 5 (float)}: UStr$ pops the type
 * below parsf, Val the length below pars+4.
 *
 * DIFF: a2 is a register parameter of doeditstr (string length, popped
 * through a local cursor; the original returns with a2 decremented).
 *
 * BUG(orig), kept:
 *   - Inkey$(0) returns d0 = d6 of the caller (MOVEQ #0,d6 is skipped):
 *     a string of a random length; the C routine takes d6 as a register
 *     parameter to return the same.  Inkey$(n) counts n as unsigned long. (#128)
 *   - The padding starts with SUBQ #1,a3 when the default is not at a3 -
 *     meant for a string argument, where a3 points behind the terminating
 *     0 (so Edit$ is fine).  Edit(value,n): UStr$ leaves a3 right behind
 *     the last digit (no 0), so the first padding space overwrites the
 *     last digit of the default. (#129)
 *   - Edit calls UStr$ and Val with the a6 of the caller (Edit has no
 *     !libs): Val needs the ffplib base in a6, so Edit only works when the
 *     program has just used it (the compiler caches a6).  The C routine
 *     passes the caller's a6 on in the same way. (#130)
 *   - Edit with n = 80 reads up to 81 characters (Read d0+1) into the
 *     80 byte buffer: the 81st overwrites the high byte of pars.length,
 *     which is written again before it is read.  Without the debugger n
 *     is not checked at all. (#131)
 *   - At end of file DefaultInput returns -1 characters: Edit$ moves a3
 *     back by one and returns length -1. (#132)
 *
 * Registers: the original changes d0/d1/a0/a1 and whatever the handler
 * changes, Inkey$(n) d6/d7; Edit$ keeps d2, Edit a2/a3/a6.  C keeps
 * d2-d7/a2/a6, Edit restores a3.
 *
 * Error checks (debugger only): Edit$/Edit with 0 characters -> "Can't
 * Edit 0 Characters!", Edit with more than 80 -> "Can only Edit up to 80
 * characters".
 */
#include "blitzlib.h"

/* entry points (defined below) */
ULONG BLITZ_ROUTINE doeditstr2(ULONG n BL_REG(d0));
ULONG BLITZ_ROUTINE doeditstr(UBYTE *def BL_REG(d0), ULONG n BL_REG(d1),
                              const UBYTE *args BL_REG(a2));
ULONG BLITZ_ROUTINE doinkey(void);
ULONG BLITZ_ROUTINE doinkey2(ULONG n BL_REG(d0), ULONG count BL_REG(d6));
ULONG BLITZ_ROUTINE doedit2(ULONG n BL_REG(d0), const void *caller_a6 BL_REG(a6));
ULONG BLITZ_ROUTINE doedit(ULONG def BL_REG(d0), ULONG n BL_REG(d1),
                           const void *caller_a6 BL_REG(a6));
void editchk1(void), editchk2(void), editchkz(void), editchk(void);

/* !libheader{#editlib,0,0,0,errors} */
BLITZ_LIBHEADER(LIB_editlib, 0, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(doeditstr2, editchk1, 0);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(doeditstr, editchk2, 0);
BLITZ_NAME("Edit$", "([DefaultString$,]Characters)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doinkey, 0, 0);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(doinkey2, 0, 0);
BLITZ_NAME("Inkey$", "[(Characters)]");

BLITZ_AFUNCTION(BL_FLOAT);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(doedit2, editchkz, 0);
BLITZ_ARGS(BL_FLOAT, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(doedit, editchk, 0);
BLITZ_NAME("Edit", "([DefaultValue,]Characters)");

BLITZ_LIBFIN();

/* ------------------------------------------------------------- Inkey$ */

static ULONG getsingle(void)
{
    struct bl_regs r = { 0 };

    return BLITZ_ALIBJSR(TOK_iogetsingle, r);
}

/* Inkey$ - one character to a3, or "" if there is none */
ULONG BLITZ_ROUTINE doinkey(void)
{
    ULONG c = getsingle();

    if ((WORD)c < 0)                            /* TST d0:BMI 'null */
        return 0;
    *blitz_string++ = (UBYTE)c;
    return 1;
}

/* Inkey$(n) - n attempts, the characters that came */
ULONG BLITZ_ROUTINE doinkey2(ULONG n BL_REG(d0), ULONG count BL_REG(d6))
{
    if (!n)                                     /* BEQ 'none: d6 not set */
        return count;
    count = 0;
    do {
        ULONG c = getsingle();

        if ((WORD)c >= 0) {
            *blitz_string++ = (UBYTE)c;
            count++;
        }
    } while (--n);                              /* SUBQ.l #1,d7:BNE 'loop */
    return count;
}

/* -------------------------------------------------------------- Edit$ */

/* editstr - d0 = default string, d1 = maxchars, a3 = string destination,
   d2 = the default's length: pad to maxchars with spaces, then read the
   line at the default (a3 = behind it), d0 = number of characters */
static ULONG editstr(UBYTE *def, ULONG max, ULONG deflen)
{
    struct bl_regs r = { 0 };
    LONG pad = max - deflen;                    /* d1 */
    ULONG n;

    if (pad >= 0 && --pad >= 0) {               /* SUB.l d2,d1:BMI:SUBQ.l #1,d1:BMI */
        if ((ULONG)def != (ULONG)blitz_string)  /* CMP.l a3,d0:BEQ 'noclu */
            blitz_string--;
        do
            *blitz_string++ = ' ';
        while (--pad >= 0);                     /* SUBQ.l #1,d1:BPL 'loop */
    }
    blitz_string = def;
    r.d0 = max;
    r.a0 = (ULONG)def;
    n = BLITZ_ALIBJSR(TOK_iogetstring, r);         /* input a string */
    blitz_string += n;                          /* ADD.l d0,a3 */
    return n;
}

/* Edit$(n) - no default string (it would be at a3) */
ULONG BLITZ_ROUTINE doeditstr2(ULONG n BL_REG(d0))
{
    return editstr(blitz_string, n, 0);
}

/* Edit$(Default$,n) - length of the default from the a2 stack */
ULONG BLITZ_ROUTINE doeditstr(UBYTE *def BL_REG(d0), ULONG n BL_REG(d1),
                              const UBYTE *args BL_REG(a2))
{
    const UBYTE *sp = args;

    return editstr(def, n, BL_POPL(sp));
}

/* --------------------------------------------------------------- Edit */

/* editbuff and the fake a2 block behind it, as in the original:
   pars: Dc.l 0 (length for Val / doeditstr), Dc 5 (type float for
   UStr$), parsf: */
static struct BL_PACKED {
    UBYTE buff[80];             /* editbuff */
    ULONG len;                  /* pars */
    UWORD type;                 /* pars+4 */
} ed = { .type = 5 };
static_assert(sizeof ed == 86);

#define ED_PARS4 ((const UBYTE *)&ed.type)      /* pars+4: a2 for len */
#define ED_PARSF ((const UBYTE *)(&ed + 1))     /* parsf: a2 for type */

/* doeditz - read the line over the default (length in pars), then Val */
static ULONG doeditz(ULONG n, const void *caller_a6, UBYTE *saved_string)
{
    ULONG v;

    v = editstr(ed.buff, n, ed.len);            /* BSR doeditstr, a2 = pars+4 */
    *blitz_string = 0;                          /* CLR.b (a3) */
    ed.len = v;                                 /* MOVE.l d0,(a2)+ */
    v = BLITZ_ALIBJSR_A2A6(TOK_val, ed.buff, ED_PARS4, caller_a6); /* val... */
    blitz_string = saved_string;                /* MOVEM.l (a7)+,a2-a3/a6 */
    return v;
}

/* Edit(n) - no default: "0" */
ULONG BLITZ_ROUTINE doedit2(ULONG n BL_REG(d0), const void *caller_a6 BL_REG(a6))
{
    UBYTE *saved_string = blitz_string;

    ed.buff[0] = '0';
    blitz_string = ed.buff + 1;
    ed.len = 1;
    return doeditz(n, caller_a6, saved_string);
}

/* Edit(Default,n) - default with UStr$ to editbuff */
ULONG BLITZ_ROUTINE doedit(ULONG def BL_REG(d0), ULONG n BL_REG(d1),
                           const void *caller_a6 BL_REG(a6))
{
    UBYTE *saved_string = blitz_string;

    blitz_string = ed.buff;
    ed.len = BLITZ_ALIBJSR_A2A6(TOK_ustr, def, ED_PARSF, caller_a6); /* ustr$ */
    return doeditz(n, caller_a6, saved_string);
}

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess1, "Can't Edit 0 Characters!");
BLITZ_ERRCHK_MESSAGE(mess2, "Can only Edit up to 80 characters");

/* editchk1 - TST.l d0:BEQ err1 */
static void BLITZ_ERRCHK editchk1_c(ULONG n BL_REG(d0))
{
    if (!n)
        BLITZ_RUNERR(mess1);
}

/* editchk2 - TST.l d1:BEQ err1 */
static void BLITZ_ERRCHK editchk2_c(ULONG n BL_REG(d1))
{
    if (!n)
        BLITZ_RUNERR(mess1);
}

/* editchk - TST.l d1:BEQ err1:CMP.l #80,d1:BHI err2 */
static void BLITZ_ERRCHK editchk_c(ULONG n BL_REG(d1))
{
    if (!n)
        BLITZ_RUNERR(mess1);
    if (n > 80)
        BLITZ_RUNERR(mess2);
}

/* editchkz - MOVE.l d0,d1, then editchk (the original leaves d1 = d0;
   doedit2 sets d1 itself) */
static void BLITZ_ERRCHK editchkz_c(ULONG n BL_REG(d0))
{
    editchk_c(n);
}

/* the thunks after all check functions: they leave the assembler in .text
   while GCC still assumes the error check section */
BLITZ_ERRCHK_THUNK(editchk1, editchk1_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(editchk2, editchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(editchk, editchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(editchkz, editchkz_c, "d0-d1/a0-a1");
