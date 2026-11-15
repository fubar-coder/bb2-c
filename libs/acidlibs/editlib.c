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
 * Fixed (intended deviations):
 *   - #128: Inkey$(0) returned d0 = d6 of the caller (MOVEQ #0,d6 was
 *     skipped): a string of a random length.  Here it is empty.
 *   - #129: the padding started with SUBQ #1,a3 when the default was not
 *     at a3 - meant for a string argument, where a3 points behind the
 *     terminating 0 (so Edit$ was fine).  Edit(value,n): UStr$ leaves a3
 *     right behind the last digit (no 0), so the first padding space
 *     overwrote the last character of the default (Edit(9.5,5) offered
 *     "9."); Edit(n)'s default "0" was overwritten for n >= 2.  Here the
 *     padding starts behind the default, and Edit(n) has no default (the
 *     buffer starts with spaces, as the original showed it for n >= 2).
 *   - #132: at the end of the file DefaultInput returns -1 characters (-2
 *     on a read error): Edit$ moved a3 back by one and returned the
 *     length -1, Edit gave Val the length -1.  Here nothing is read.
 *   - #1031: Inkey$(n) counts n as an unsigned long: a negative n makes
 *     about 4 billion attempts and appends every character that comes
 *     past the string buffer.  The original had no error check for it;
 *     here inkeychk (error check area only) refuses a negative n with
 *     "Can't Inkey$ a negative number of Characters!".  Release code
 *     unchanged (an invalid argument).
 *
 * BUG(orig), kept:
 *   - Edit with n = 80 reads up to 81 characters (Read d0+1) into the
 *     80 byte buffer: the 81st overwrites the high byte of pars.length,
 *     which is written again before it is read.  Without the debugger n
 *     is not checked at all. (#131)
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
ULONG BLITZ_ROUTINE doinkey2(ULONG n BL_REG(d0));
ULONG BLITZ_ROUTINE doedit2(ULONG n BL_REG(d0), const void *caller_a6 BL_REG(a6));
ULONG BLITZ_ROUTINE doedit(ULONG def BL_REG(d0), ULONG n BL_REG(d1),
                           const void *caller_a6 BL_REG(a6));
void editchk1(void), editchk2(void), editchkz(void), editchk(void);
void inkeychk(void);

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
BLITZ_SUBS(doinkey2, inkeychk, 0);
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

/**
 * @brief Inkey$: one character from the current input, "" if none came.
 *
 * Reads one character with inputoutputlib's dogetsingle (d0 negative =
 * none) and appends it to the string at a3.
 *
 * @return d0: the string length, 1 or 0
 */
ULONG BLITZ_ROUTINE doinkey(void)
{
    ULONG c = getsingle();

    if ((WORD)c < 0)                            /* TST d0:BMI 'null */
        return 0;
    *blitz_string++ = (UBYTE)c;
    return 1;
}

/**
 * @brief Inkey$(Characters): n attempts to read a character, the
 *        characters that came.
 *
 * Calls dogetsingle n times and appends every character it returns to
 * the string at a3.  n counts as an unsigned long.
 * Fixed (#128): Inkey$(0) is the empty string (the original skipped
 * MOVEQ #0,d6 and returned the caller's d6 as the length).
 * Errors (debugger, inkeychk): "Can't Inkey$ a negative number of
 * Characters!" (fixed, #1031: the original had no check; a negative n
 * makes about 4 billion attempts).
 *
 * @param n      d0: the number of attempts
 * @return d0: the string length (the characters that came)
 */
ULONG BLITZ_ROUTINE doinkey2(ULONG n BL_REG(d0))
{
    ULONG count = 0;                            /* fixed (#128): MOVEQ #0,d6 first */

    if (!n)                                     /* BEQ 'none */
        return 0;
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
   line at the default (a3 = behind it), d0 = number of characters.
   Fixed (#129): the padding starts behind the default (d0+d2); the
   original started at a3-1 when the default was not at a3 (CMP.l a3,d0:
   BEQ 'noclu:SUBQ #1,a3), right for a string argument (a3 behind its 0)
   but one character early for Edit's default from UStr$.
   Fixed (#132): a negative count (DefaultInput at the end of the file -1,
   on a read error -2) is 0 characters; the original added it to a3. */
static ULONG editstr(UBYTE *def, ULONG max, ULONG deflen)
{
    struct bl_regs r = { 0 };
    LONG pad = max - deflen;                    /* d1 */
    ULONG n;

    if (pad >= 0 && --pad >= 0) {               /* SUB.l d2,d1:BMI:SUBQ.l #1,d1:BMI */
        blitz_string = def + deflen;            /* fixed (#129): behind the default */
        do
            *blitz_string++ = ' ';
        while (--pad >= 0);                     /* SUBQ.l #1,d1:BPL 'loop */
    }
    blitz_string = def;
    r.d0 = max;
    r.a0 = (ULONG)def;
    n = BLITZ_ALIBJSR(TOK_iogetstring, r);         /* input a string */
    if ((LONG)n < 0)                            /* fixed (#132): end of file */
        n = 0;                                  /* or error: nothing read */
    blitz_string += n;                          /* ADD.l d0,a3 */
    return n;
}

/**
 * @brief Edit$(Characters): reads a line of up to n characters, without
 *        a default.
 *
 * The buffer at a3 is padded with n spaces (for input handlers that edit
 * the buffer), then the line is read over it with inputoutputlib's
 * dogetstring (see editstr).
 * Fixed (#132): at the end of the file (DefaultInput returns -1
 * characters) the result is empty (the original moved a3 back by one and
 * returned the length -1).
 * Errors (debugger only): n = 0 -> "Can't Edit 0 Characters!".
 *
 * @param n  d0: the maximum number of characters
 * @return d0: the number of characters read (the string length)
 */
ULONG BLITZ_ROUTINE doeditstr2(ULONG n BL_REG(d0))
{
    return editstr(blitz_string, n, 0);
}

/**
 * @brief Edit$(DefaultString$,Characters): reads a line of up to n
 *        characters over a default string.
 *
 * The default is padded with spaces to n characters in the string
 * buffer, then the line is read over it (see editstr).  The padding
 * starts one byte before a3 when the default is not at a3 (a3 points
 * behind the string argument's terminating 0).
 * Fixed (#132): at the end of the file the result is empty (the original
 * returned the length -1).
 * Errors (debugger only): n = 0 -> "Can't Edit 0 Characters!".
 * DIFF: a2 is a register parameter here (the length is popped through a
 * local cursor; the original returns with a2 decremented).
 *
 * @param def   d0: the default string
 * @param n     d1: the maximum number of characters
 * @param args  a2: the argument stack, the default's length below it
 * @return d0: the number of characters read (the string length)
 */
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

/**
 * @brief Edit(Characters): reads a number of up to n characters, without
 *        a default.
 *
 * The line is read in the library's 80 byte buffer and converted with
 * vallib's Val (see doeditz).
 * Fixed (#129): the buffer starts empty (padded with n spaces).  The
 * original put a default "0" there, which its padding overwrote for
 * n >= 2 (#129) and showed only for n = 1; an empty line still gives 0.
 * Val gets the ffplib base although Edit has no !libs: the compiler links
 * the ALibJsr call through a stub that loads the callee's !libs (not a
 * bug, #130).
 * By design (#131): n = 80 reads up to 81 characters into the 80 byte
 * buffer.
 * Errors (debugger only): n = 0 -> "Can't Edit 0 Characters!", n > 80 ->
 * "Can only Edit up to 80 characters".
 *
 * @param n          d0: the maximum number of characters
 * @param caller_a6  a6: the caller's a6, passed on to Val
 * @return d0: the value (FFP float)
 */
ULONG BLITZ_ROUTINE doedit2(ULONG n BL_REG(d0), const void *caller_a6 BL_REG(a6))
{
    UBYTE *saved_string = blitz_string;

    blitz_string = ed.buff;                     /* fixed (#129): no default */
    ed.len = 0;
    return doeditz(n, caller_a6, saved_string);
}

/**
 * @brief Edit(DefaultValue,Characters): reads a number of up to n
 *        characters over a default value.
 *
 * The default is converted with printlib's UStr$ into the library's 80
 * byte buffer, the line is read over it and converted with Val (see
 * doeditz).
 * Fixed (#129): the padding starts behind the default (the original's
 * first padding space overwrote its last character: UStr$ leaves a3 right
 * behind it, without a 0).
 * UStr$ and Val get their !libs bases from the ALibJsr stubs (#130).
 * By design (#131): n = 80 reads up to 81 characters into the buffer.
 * Errors (debugger only): n = 0 -> "Can't Edit 0 Characters!", n > 80 ->
 * "Can only Edit up to 80 characters".
 *
 * @param def        d0: the default value (FFP float)
 * @param n          d1: the maximum number of characters
 * @param caller_a6  a6: the caller's a6, passed on to UStr$ and Val
 * @return d0: the value (FFP float)
 */
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
BLITZ_ERRCHK_MESSAGE(mess3, "Can't Inkey$ a negative number of Characters!");

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

/* inkeychk - fixed (#1031), not in the original (Inkey$(n) had no
   check): a negative n */
static void BLITZ_ERRCHK inkeychk_c(ULONG n BL_REG(d0))
{
    if ((LONG)n < 0)
        BLITZ_RUNERR(mess3);
}

/* editchkz - MOVE.l d0,d1, then editchk (the original leaves d1 = d0;
   doedit2 sets d1 itself) */
static void BLITZ_ERRCHK editchkz_c(ULONG n BL_REG(d0))
{
    editchk_c(n);
}

/* the thunks of the check functions */
BLITZ_ERRCHK_THUNK(editchk1, editchk1_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(editchk2, editchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(editchk, editchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(editchkz, editchkz_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(inkeychk, inkeychk_c, "d0-d1/a0-a1");
