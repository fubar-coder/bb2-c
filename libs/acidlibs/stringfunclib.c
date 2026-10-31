/*
 * stringfunclib - command library 185: the string functions Left$ Right$
 * Mid$ Hex$ Bin$ Chr$ Asc String$ Instr Replace$ Mki$ Mkl$ Cvi Cvl Len
 * UnLeft$ UnRight$ StripLead$ StripTrail$ LSet$ RSet$ Centre$ LCase$
 * UCase$ CaseSense Mkq$ Cvq.
 *
 * C translation of bblibsrc/stringfunclib (binary of the CD's acidlibs
 * checked: same as the source).  From the original:
 *   "March 1994 changed to handle new null always upgrade
 *    June 1994  ignores illegal size/length parameters"
 *
 * String function convention of the Blitz 2 compiler:
 *   d0, d1, d2   arguments; a string argument is the address of its
 *                characters in the string work area,
 *   a2           stack of the string arguments' lengths (longs, the one
 *                of the first argument on top: MOVE.l -(a2),dN pops it),
 *   a3           string work pointer; on entry it points one byte past
 *                the end of the last string argument (LSet$/RSet$/
 *                Centre$/String$ write from -1(a3)),
 *   result       d0 = length of the string result, a3 = its end (the
 *                characters start at the first string argument, d0 - for
 *                functions without string argument at the entry a3);
 *                numeric results in d0.
 * DIFF: a2 is a register parameter of each C routine that reads lengths
 * (const ULONG *lens BL_REG(a2), read with negative offsets through a
 * local cursor) and is left unchanged; the original pops the lengths
 * (MOVE.l -(a2),dN) and returns with a2 decremented by a routine
 * dependent amount (String$ with count <= 0, Replace$ with an empty find
 * string and Cvi/Cvl/Cvq pop fewer or nothing).  The compiler sets a2
 * before every library call (preplibst: LEA N(a7),a2) and never reads it
 * afterwards; library code relies on a2 being preserved (GCC: a2 is
 * callee-saved).  The inline Len keeps the original MOVE.l -(a2),d0.
 *
 * Registers: the original changes d0-d7, a0, a1, a3 depending on the
 * routine; the thunks keep d1/a0/a1 for all of them (more than the
 * original, never less).  CaseSense keeps all registers.  The CCR is no
 * result.
 *
 * Inline routines (Compiler-Spec 9.3.2, copied into the program): Chr$,
 * Len - assembler text, as in the original.
 *
 * The init value (a long global of the program) is the address of
 * movestring, the copy routine of this library, as in the original
 * (no other library of the CD uses it): a0 = source, a3 = destination,
 * d1 = length -> d0 = length, a0/a3 advanced.
 */
#include "blitzlib.h"

/* entry points (thunks / inline code, defined below) */
void leftstring(void), rightstring(void), midstring(void), midstring2(void);
void hexstring(void), binstring(void), chrstring(void), ascii(void);
void stringstr(void), instrstring(void), instrstring2(void), replace(void);
void mkistring(void), mklstring(void), cvistring(void), cvlstring(void);
void lenstring(void), unleftstring(void), unrightstring(void);
void striplead(void), striptrail(void), lsetstring(void), rsetstring(void);
void centrestring(void), lcasestring(void), ucasestring(void);
void casesensestring(void), codeinit(void), movestring(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#stringfunclib,init,1,0,0} */
BLITZ_LIBHEADER(LIB_stringfunclib, init, BL_LONGRET, 0, 0);

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(leftstring, 0, 0);
BLITZ_NAME("Left$", "(String$,Length)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(rightstring, 0, 0);
BLITZ_NAME("Right$", "(String$,Length)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(midstring, 0, 0);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(midstring2, 0, 0);
BLITZ_NAME("Mid$", "(String$,Startchar[,Length])");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(hexstring, 0, 0);
BLITZ_NAME("Hex$", "(Expression)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(binstring, 0, 0);
BLITZ_NAME("Bin$", "(Expression)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(chrstring, 0, 0);
BLITZ_NAME("Chr$", "(Expression)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(ascii, 0, 0);
BLITZ_NAME("Asc", "(String$)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(stringstr, 0, 0);
BLITZ_NAME("String$", "(String$,Repeats)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(instrstring, 0, 0);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(instrstring2, 0, 0);
BLITZ_NAME("Instr", "(String$,Findstring$[,Startpos])");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(replace, 0, 0);
BLITZ_NAME("Replace$", "(String$,Findstring$,Replacestring$)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(mkistring, 0, 0);
BLITZ_NAME("Mki$", "(Integer)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(mklstring, 0, 0);
BLITZ_NAME("Mkl$", "(Long)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(cvistring, 0, 0);
BLITZ_NAME("Cvi", "(String$)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(cvlstring, 0, 0);
BLITZ_NAME("Cvl", "(String$)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(lenstring, 0, 0);
BLITZ_NAME("Len", "(String$)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(unleftstring, 0, 0);
BLITZ_NAME("UnLeft$", "(String$,Length)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(unrightstring, 0, 0);
BLITZ_NAME("UnRight$", "(String$,Length)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(striplead, 0, 0);
BLITZ_NAME("StripLead$", "(String$,Ascii)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(striptrail, 0, 0);
BLITZ_NAME("StripTrail$", "(String$,Ascii)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(lsetstring, 0, 0);
BLITZ_NAME("LSet$", "(String$,Characters)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(rsetstring, 0, 0);
BLITZ_NAME("RSet$", "(String$,Characters)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(centrestring, 0, 0);
BLITZ_NAME("Centre$", "(String$,Characters)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(lcasestring, 0, 0);
BLITZ_NAME("LCase$", "(String$)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(ucasestring, 0, 0);
BLITZ_NAME("UCase$", "(String$)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(casesensestring, 0, 0);
BLITZ_NAME("CaseSense", "On|Off");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_QUICK);
BLITZ_LIBS0();
BLITZ_SUBS(mklstring, 0, 0);
BLITZ_NAME("Mkq$", "(Quick)");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(cvlstring, 0, 0);
BLITZ_NAME("Cvq", "(String$)");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_LIBFIN();

/* ------------------------------------------------------------- helpers */

static UWORD csense;                    /* 0 = case insensitive (CaseSense Off) */

/* The length of a string argument is popped from the local cursor `sp`
   (a copy of a2) with BL_POPL(sp): MOVE.l -(a2),dN - a2 itself is not
   changed */

/* movestring - copy `len` bytes from *src to a3, both advanced; result
   len.  The original copies blocks of 8 bytes plus the rest (DBF on the
   low word of len&7); for every len (unsigned) that is the same as this
   byte loop, forward, which the overlapping moves below rely on. */
static inline ULONG movestr(const UBYTE **src, ULONG len)
{
    const UBYTE *s = *src;
    ULONG n = len;

    while (n--)
        *blitz_string++ = *s++;
    *src = s;
    return len;
}

/* CaseSense Off compare of Instr/Replace: 'a'..'z' -> AND.b #255-32 */
static inline UBYTE upcase(UBYTE c)
{
    if (c > 'z' || c < 'a')                     /* BHI 'hi / BCS 'hi */
        return c;
    return c & (255 - 32);
}

/* -------------------------------------------------------- init, export */

/* movestring as exported routine (the init value): a0 = source, d1 =
   length; a3 and a0 advanced, d0 = length.  The thunk adds d0 to the
   saved a0 (at 0(sp)) - a0 is the second result. */
static ULONG BLITZ_ROUTINE movestring_c(const UBYTE *src BL_REG(a0), ULONG len BL_REG(d1))
{
    return movestr(&src, len);
}
BLITZ_THUNK_POST(movestring, movestring_c, "a0-a1", "add.l   d0,(sp)");

/* codeinit - MOVE #-1,csense:LEA movestring(pc),a0:MOVE.l a0,d0:RTS */
static ULONG BLITZ_ROUTINE codeinit_c(void)
{
    csense = 0xffff;
    return (ULONG)movestring;
}
BLITZ_THUNK(codeinit, codeinit_c, "d1/a0-a1");

/**
 * @brief CaseSense On|Off - case sensitive or insensitive Instr and
 * Replace$.
 *
 * MOVE d0,csense:RTS.  The init sets On (-1).  Keeps all registers.
 *
 * @param on d0.w: 0 (Off) = 'a'..'z' compare as upper case, else On
 */
static void BLITZ_ROUTINE casesensestring_c(UWORD on BL_REG(d0))
{
    csense = on;
}
BLITZ_THUNK(casesensestring, casesensestring_c, "d0-d1/a0-a1");

/* ------------------------------------------------------ string results */

/**
 * @brief StripTrail$(String$,Ascii) - the string without the trailing
 * characters Ascii.
 *
 * In place: a3 is moved back over the trailing characters d1.b.
 *
 * @param s    d0: the string
 * @param ch   d1.w: the character to strip (low byte)
 * @param lens a2: the length stack (the string's length)
 * @return d0: the length of the result; a3 = its end
 */
static ULONG BLITZ_ROUTINE striptrail_c(UBYTE *s BL_REG(d0), UWORD ch BL_REG(d1),
                                        const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    LONG n = BL_POPL(sp);

    blitz_string = s + n;
    for (;;) {
        if (--n < 0)                            /* SUBQ.l #1,d0:BMI 'done */
            break;
        if (*--blitz_string != (UBYTE)ch) {     /* CMP.b -(a3),d1:BEQ 'loop */
            blitz_string++;
            break;
        }
    }
    return n + 1;
}
BLITZ_THUNK(striptrail, striptrail_c, "d1/a0-a1");

/* Left$(d0$,d1) - perform left$(d0,d1).  A negative length counts as 0
   ("simon was here"). */
static ULONG left(UBYTE *s, LONG n, const ULONG *sp)
{
    LONG len;

    if (n < 0)
        n = 0;
    blitz_string = s;
    len = BL_POPL(sp);
    if (!(n < len))                             /* CMP.l -(a2),d1:BLT 'skip */
        n = len;                                /* new length */
    blitz_string += n;                          /* new end of string */
    return n;
}

/**
 * @brief Left$(String$,Length) - the first Length characters.
 *
 * In place.  A negative length counts as 0 ("simon was here"); a length
 * beyond the string gives the whole string.
 *
 * @param s    d0: the string
 * @param n    d1: the length
 * @param lens a2: the length stack (the string's length)
 * @return d0: the length of the result; a3 = its end
 */
static ULONG BLITZ_ROUTINE leftstring_c(UBYTE *s BL_REG(d0), LONG n BL_REG(d1),
                                        const ULONG *lens BL_REG(a2))
{
    return left(s, n, lens);
}
BLITZ_THUNK(leftstring, leftstring_c, "d1/a0-a1");

/**
 * @brief UnLeft$(String$,Length) - the string without its last Length
 * characters.
 *
 * Left$(d0$,Len(d0$)-d1); the length is read with -4(a2).
 *
 * @param s    d0: the string
 * @param n    d1: the number of characters to remove
 * @param lens a2: the length stack (the string's length)
 * @return d0: the length of the result; a3 = its end
 */
static ULONG BLITZ_ROUTINE unleftstring_c(UBYTE *s BL_REG(d0), LONG n BL_REG(d1),
                                          const ULONG *lens BL_REG(a2))
{
    return left(s, lens[-1] - n, lens);
}
BLITZ_THUNK(unleftstring, unleftstring_c, "d1/a0-a1");

/**
 * @brief LSet$(String$,Characters) - the string left justified in
 * Characters characters.
 *
 * Pads with spaces from the end of the argument (-1(a3)) or cuts.  The
 * length is d1 (negative: 0).  For d1 = Len(d0$) a3 is left at its entry
 * value - 1 (not a bug: the compiler always leaves a3 one past the
 * argument's 0, so that is the right end).
 *
 * @param s    d0: the string
 * @param n    d1: the width
 * @param lens a2: the length stack (the string's length)
 * @return d0: the width; a3 = the end of the result
 */
static ULONG BLITZ_ROUTINE lsetstring_c(UBYTE *s BL_REG(d0), LONG n BL_REG(d1),
                                        const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    LONG pad;

    if (n < 0)
        n = 0;
    blitz_string--;                             /* SUBQ #1,a3 */
    pad = (ULONG)n - BL_POPL(sp);               /* how much to clear */
    if (pad == 0)                               /* BEQ 'done2 */
        return n;
    if (pad > 0) {                              /* BMI 'done */
        do
            *blitz_string++ = ' ';
        while (--pad);
    }
    blitz_string = s + n;
    return n;
}
BLITZ_THUNK(lsetstring, lsetstring_c, "d1/a0-a1");

/**
 * @brief RSet$(String$,Characters) - the string right justified in
 * Characters characters.
 *
 * The characters are moved to the end of d1 characters (backwards from
 * -1(a3)), spaces in front; a longer string keeps its last d1 characters
 * (movestring).  A negative width counts as 0.
 *
 * @param s    d0: the string
 * @param n    d1: the width
 * @param lens a2: the length stack (the string's length)
 * @return d0: the width; a3 = the end of the result
 */
static ULONG BLITZ_ROUTINE rsetstring_c(UBYTE *s BL_REG(d0), LONG n BL_REG(d1),
                                        const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    ULONG len;
    LONG pad;
    UBYTE *d;
    const UBYTE *src;

    if (n < 0)
        n = 0;
    blitz_string--;
    len = BL_POPL(sp);
    pad = (ULONG)n - len;                       /* how much to clear */
    if (pad == 0)                               /* BEQ 'done */
        return n;
    if (pad < 0) {                              /* 'back: too many characters */
        src = s + len - n;
        blitz_string = s;
        return movestr(&src, n);
    }
    d = s + n;
    if (len) {
        do
            *--d = *--blitz_string;
        while (--len);
    }
    do
        *--d = ' ';
    while (--pad);
    blitz_string = s + n;
    return n;
}
BLITZ_THUNK(rsetstring, rsetstring_c, "d1/a0-a1");

/**
 * @brief Centre$(String$,Characters) - the string centred in Characters
 * characters.
 *
 * End of the moved characters: s + d1/2 + len/2 (+1 if both d1 and len
 * are odd: the LSR #1 of d1&len sets the carry, SCS/EXT/SUBA); spaces:
 * (d1-len)/2 in front (written backwards), the rest behind.  A longer
 * string keeps the middle d1 characters: s + len/2 - d1/2 (LSR.l).  A
 * negative width counts as 0.
 *
 * @param s    d0: the string
 * @param n    d1: the width
 * @param lens a2: the length stack (the string's length)
 * @return d0: the width; a3 = the end of the result
 */
static ULONG BLITZ_ROUTINE centrestring_c(UBYTE *s BL_REG(d0), LONG n BL_REG(d1),
                                          const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    ULONG len, front;
    LONG pad;
    UBYTE *d, *mid;
    const UBYTE *src;

    if (n < 0)
        n = 0;
    blitz_string--;
    len = BL_POPL(sp);
    pad = (ULONG)n - len;                       /* how much to clear */
    if (pad < 0) {                              /* 'back: string is too big! */
        src = s + (len >> 1) - ((ULONG)n >> 1);
        blitz_string = s;
        return movestr(&src, n);
    }
    if (pad == 0)                               /* 'done */
        return n;
    d = s + ((ULONG)n >> 1) + (len >> 1);
    if ((UWORD)n & (UWORD)len & 1)              /* MOVE d1,d4:AND d2,d4:LSR #1,d4:SCS d4 */
        d++;                                    /* EXT d4:SUB d4,a0 */
    mid = d;                                    /* MOVE.l a0,-(a7) */
    if (len) {
        do
            *--d = *--blitz_string;
        while (--len);
    }
    front = (ULONG)pad >> 1;
    if (front) {
        do {
            *--d = ' ';
            pad--;
        } while (--front);
    }
    d = mid;
    do
        *d++ = ' ';
    while (--pad);
    blitz_string = s + n;
    return n;
}
BLITZ_THUNK(centrestring, centrestring_c, "d1/a0-a1");

/**
 * @brief LCase$(String$) - the string in lower case ('A'..'Z' only).
 *
 * In place.  An empty string leaves a3 at its entry value (MOVE.l
 * -(a2),d1:BEQ 'done before MOVE.l d0,a3).
 *
 * @param s    d0: the string
 * @param lens a2: the length stack (the string's length)
 * @return d0: the length; a3 = the end of the result
 */
static ULONG BLITZ_ROUTINE lcasestring_c(UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    ULONG len = BL_POPL(sp), n;

    if (len) {
        blitz_string = s;
        n = len;
        do {
            UBYTE c = *blitz_string++;
            if (c >= 'A' && c <= 'Z')           /* BCS 'skip / BHI 'skip */
                blitz_string[-1] = c | 32;
        } while (--n);
    }
    return len;
}
BLITZ_THUNK(lcasestring, lcasestring_c, "d1/a0-a1");

/**
 * @brief UCase$(String$) - the string in upper case ('a'..'z' only).
 *
 * In place.  An empty string leaves a3 at its entry value, as LCase$.
 *
 * @param s    d0: the string
 * @param lens a2: the length stack (the string's length)
 * @return d0: the length; a3 = the end of the result
 */
static ULONG BLITZ_ROUTINE ucasestring_c(UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    ULONG len = BL_POPL(sp), n;

    if (len) {
        blitz_string = s;
        n = len;
        do {
            UBYTE c = *blitz_string++;
            if (c >= 'a' && c <= 'z')
                blitz_string[-1] = c & (255 - 32);
        } while (--n);
    }
    return len;
}
BLITZ_THUNK(ucasestring, ucasestring_c, "d1/a0-a1");

/**
 * @brief StripLead$(String$,Ascii) - the string without the leading
 * characters Ascii.
 *
 * Strips the leading characters d1.b; the rest is moved to the start
 * (movestring); all stripped: d0 = 0, a3 = d0$.
 *
 * @param s    d0: the string
 * @param ch   d1.w: the character to strip (low byte)
 * @param lens a2: the length stack (the string's length)
 * @return d0: the length of the result; a3 = its end
 */
static ULONG BLITZ_ROUTINE striplead_c(UBYTE *s BL_REG(d0), UWORD ch BL_REG(d1),
                                       const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    const UBYTE *src = s;
    LONG n;

    blitz_string = s;                           /* to..... */
    n = BL_POPL(sp);                            /* length */
    for (;;) {
        if (--n < 0)                            /* SUBQ.l #1,d1:BMI 'done */
            return 0;
        if (*src++ != (UBYTE)ch)                /* CMP.b (a0)+,d2:BEQ 'loop */
            break;
    }
    src--;
    return movestr(&src, n + 1);
}
BLITZ_THUNK(striplead, striplead_c, "d1/a0-a1");

/* Right$(d0$,d1) - perform right$(d0,d1).
   BUG(orig) (#285): the source address is LEA 0(a3,d0.w),a0 - the offset
   Len-d1 is used as a sign extended WORD, so for Len-d1 >= 32768 the
   characters are taken from before the string. */
static ULONG right(UBYTE *s, LONG n, const ULONG *sp)
{
    LONG len;
    const UBYTE *src;

    if (n < 0)
        n = 0;
    blitz_string = s;
    len = BL_POPL(sp);
    if (n >= len) {                             /* CMP.l -(a2),d1:BGE 'skip */
        blitz_string += len;
        return len;
    }
    src = blitz_string + (WORD)(len - n);
    return movestr(&src, n);
}

/**
 * @brief Right$(String$,Length) - the last Length characters.
 *
 * Moved to the start of the argument (movestring).  A negative length
 * counts as 0; a length beyond the string gives the whole string.
 *
 * BUG(orig) (#285): the source address is LEA 0(a3,d0.w),a0 - for
 * Len-Length >= 32768 the characters are taken from before the string.
 *
 * @param s    d0: the string
 * @param n    d1: the length
 * @param lens a2: the length stack (the string's length)
 * @return d0: the length of the result; a3 = its end
 */
static ULONG BLITZ_ROUTINE rightstring_c(UBYTE *s BL_REG(d0), LONG n BL_REG(d1),
                                         const ULONG *lens BL_REG(a2))
{
    return right(s, n, lens);
}
BLITZ_THUNK(rightstring, rightstring_c, "d1/a0-a1");

/**
 * @brief UnRight$(String$,Length) - the string without its first Length
 * characters.
 *
 * Right$(d0$,Len(d0$)-d1); the length is read with -4(a2).
 *
 * BUG(orig) (#285): as Right$, a word offset into the string.
 *
 * @param s    d0: the string
 * @param n    d1: the number of characters to remove
 * @param lens a2: the length stack (the string's length)
 * @return d0: the length of the result; a3 = its end
 */
static ULONG BLITZ_ROUTINE unrightstring_c(UBYTE *s BL_REG(d0), LONG n BL_REG(d1),
                                           const ULONG *lens BL_REG(a2))
{
    return right(s, lens[-1] - n, lens);
}
BLITZ_THUNK(unrightstring, unrightstring_c, "d1/a0-a1");

/* Mid$(d0$,d1,d2) - perform mid$(d0,d1,d2); first position = 0 after
   SUBQ.l #1,d1, then negative start/length count as 0.  Start beyond the
   end: empty.  BUG(orig) (#286): start+length is compared signed after the
   addition, so a huge length (start-1+length > $7FFFFFFF) copies
   `length` bytes. */
static ULONG mid(UBYTE *s, LONG start, LONG n, const ULONG *sp)
{
    LONG len, end;
    const UBYTE *src;

    blitz_string = s;
    start--;                                    /* first position=0 */
    if (start < 0)
        start = 0;
    if (n < 0)
        n = 0;
    len = BL_POPL(sp);
    if (start > len)                            /* CMP.l -(a2),d1:BGT 'skip */
        return 0;
    end = (ULONG)start + (ULONG)n;
    if (end > len)                              /* CMP.l (a2),d3:BGT 'skip2 (= len) */
        n = len - start;
    src = blitz_string + start;                 /* LEA 0(a3,d1.l),a0 */
    return movestr(&src, n);
}

/**
 * @brief Mid$(String$,Startchar,Length) - Length characters from
 * Startchar (1 = the first).
 *
 * First position = 0 after SUBQ.l #1,d1, then negative start/length count
 * as 0.  Start beyond the end: empty; the length is cut at the end.
 *
 * BUG(orig) (#286): start+length is compared signed after the addition,
 * so a huge length (start-1+length > $7FFFFFFF) copies `length` bytes.
 *
 * @param s     d0: the string
 * @param start d1: the first character (1-based)
 * @param n     d2: the length
 * @param lens  a2: the length stack (the string's length)
 * @return d0: the length of the result; a3 = its end
 */
static ULONG BLITZ_ROUTINE midstring_c(UBYTE *s BL_REG(d0), LONG start BL_REG(d1),
                                       LONG n BL_REG(d2), const ULONG *lens BL_REG(a2))
{
    return mid(s, start, n, lens);
}
BLITZ_THUNK(midstring, midstring_c, "d1/a0-a1");

/**
 * @brief Mid$(String$,Startchar) - the rest of the string from Startchar.
 *
 * Mid$ with the length Len(d0$)-d1+1 (read from -4(a2)).
 *
 * BUG(orig) (#286): see Mid$ with three arguments.
 *
 * @param s     d0: the string
 * @param start d1: the first character (1-based)
 * @param lens  a2: the length stack (the string's length)
 * @return d0: the length of the result; a3 = its end
 */
static ULONG BLITZ_ROUTINE midstring2_c(UBYTE *s BL_REG(d0), LONG start BL_REG(d1),
                                        const ULONG *lens BL_REG(a2))
{
    return mid(s, start, lens[-1] - start + 1, lens);
}
BLITZ_THUNK(midstring2, midstring2_c, "d1/a0-a1");

/**
 * @brief Chr$(Expression) - a string of one character.
 *
 * Inline code (copied into the program): MOVE.b d0,(a3)+:MOVEQ #1,d0.
 * Parameter d0.w: the character code (low byte).
 * Result d0: 1, the length; a3 behind the character.
 */
BLITZ_INLINE(chrstring,
    "   move.b  d0,(a3)+\n"
    "   moveq   #1,d0");

/**
 * @brief Bin$(Expression) - the value as 32 binary digits.
 *
 * From bit 31 down, written at a3.
 *
 * @param v d0: the value
 * @return d0: 32, the length; a3 = the end of the result
 */
static ULONG BLITZ_ROUTINE binstring_c(ULONG v BL_REG(d0))
{
    UWORD i;

    for (i = 0; i < 32; i++) {
        *blitz_string++ = '0' + (UBYTE)(v >> 31);   /* LSL.l #1,d0:ADDX d3,d1 */
        v <<= 1;
    }
    return 32;
}
BLITZ_THUNK(binstring, binstring_c, "d1/a0-a1");

/**
 * @brief Hex$(Expression) - the value as 8 hexadecimal digits.
 *
 * Upper case, written at a3.
 *
 * @param v d0: the value
 * @return d0: 8, the length; a3 = the end of the result
 */
static ULONG BLITZ_ROUTINE hexstring_c(ULONG v BL_REG(d0))
{
    UWORD i, c;

    for (i = 0; i < 8; i++) {
        c = (UWORD)(v >> 28) + 48;
        v <<= 4;
        if (c > '9')                            /* CMP #"9",d1:BLS 'skip */
            c += 7;
        *blitz_string++ = (UBYTE)c;
    }
    return 8;
}
BLITZ_THUNK(hexstring, hexstring_c, "d1/a0-a1");

/**
 * @brief String$(String$,Repeats) - Repeats copies of the string.
 *
 * movestring copies the argument behind itself (from -1(a3) on) with a0
 * trailing by one length, then the whole block is moved to the start.  A
 * count <= 0 gives the empty string.
 * DIFF: for a count <= 0 the original returns the empty string without
 * popping the argument's length (MOVE.l d1,d3:BLE 'skip comes first), so
 * a2 is left at another position than for count > 0; the C version
 * leaves a2 unchanged in both cases (same results).
 *
 * @param s     d0: the string
 * @param count d1: the number of copies
 * @param lens  a2: the length stack (the string's length)
 * @return d0: the length of the result; a3 = its end
 */
static ULONG BLITZ_ROUTINE stringstr_c(UBYTE *s BL_REG(d0), LONG count BL_REG(d1),
                                       const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    const UBYTE *src;
    UBYTE *start;
    ULONG len, total;

    blitz_string--;
    if (count <= 0) {                           /* MOVE.l d1,d3:BLE 'skip */
        blitz_string = s;
        return 0;
    }
    src = s;
    start = blitz_string;                       /* MOVE.l a3,-(a7) */
    len = BL_POPL(sp);
    total = 0;
    do {
        len = movestr(&src, len);               /* MOVE.l d0,d1 */
        total += len;
    } while (--count);
    src = start;                                /* MOVE.l (a7)+,a0 */
    blitz_string = s;                           /* MOVE.l (a7)+,a3 */
    return movestr(&src, total);
}
BLITZ_THUNK(stringstr, stringstr_c, "d1/a0-a1");

/**
 * @brief Asc(String$) - the code of the first character.
 *
 * a3 = d0$; the empty string leaves a3 unchanged.
 *
 * @param s    d0: the string
 * @param lens a2: the length stack (the string's length)
 * @return d0: the first character (0-255), -1 for the empty string
 */
static ULONG BLITZ_ROUTINE ascii_c(UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;

    if (!BL_POPL(sp))                           /* TST.l -(a2):BEQ 'skip */
        return (ULONG)-1;
    blitz_string = s;
    return *s;
}
BLITZ_THUNK(ascii, ascii_c, "d1/a0-a1");

/* Instr - find f$ in s$ from 0-based position pos+1; result 1-based
   position or 0.  CaseSense Off compares 'a'..'z' as upper case.
   pos is d4: -1 for Instr(s$,f$), Startpos-2 for Instr(s$,f$,Startpos).
   BUG(orig) (#287): Startpos <= 0 is not checked - the compare starts before
   the string (Instr(s$,"",0) = 0, Instr(s$,"",-5) = -5). */
static ULONG instr(UBYTE *s, const UBYTE *f, LONG pos, const ULONG *sp)
{
    LONG slen, flen, n;
    const UBYTE *scan, *fp;

    blitz_string = s;                           /* fix string pointer */
    slen = BL_POPL(sp);                         /* length of string 1 */
    flen = BL_POPL(sp);                         /* length of string 2 */
    for (;;) {                                  /* instrl */
        pos++;
        if ((LONG)((ULONG)pos + (ULONG)flen) > slen)    /* CMP.l d2,d5:BGT 'done */
            return 0;                           /* not there */
        scan = s + pos;
        fp = f;
        for (n = flen; n; n--) {                /* 'loop2: BEQ 'found */
            if (csense) {                       /* 'norm: CMPM.b (a0)+,(a1)+ */
                if (*scan++ != *fp++)
                    break;
            } else {
                UBYTE sc = upcase(*scan++), fc = upcase(*fp++);
                if (sc != fc)
                    break;
            }
        }
        if (!n)
            return pos + 1;                     /* 'found */
    }
}

/**
 * @brief Instr(String$,Findstring$) - the position of Findstring$ in
 * String$.
 *
 * Searches from the first character (d4 = -1 in the original);
 * CaseSense Off compares 'a'..'z' as upper case.  a3 = String$.
 *
 * @param s    d0: the string to search in
 * @param f    d1: the string to find
 * @param lens a2: the length stack (String$'s length on top, then
 *             Findstring$'s)
 * @return d0: the 1-based position, 0 if not found
 */
static ULONG BLITZ_ROUTINE instrstring_c(UBYTE *s BL_REG(d0), const UBYTE *f BL_REG(d1),
                                         const ULONG *lens BL_REG(a2))
{
    return instr(s, f, -1, lens);               /* char search pos. */
}
BLITZ_THUNK(instrstring, instrstring_c, "d1/a0-a1");

/**
 * @brief Instr(String$,Findstring$,Startpos) - the position of
 * Findstring$ in String$, searching from Startpos.
 *
 * d4 = Startpos-2 in the original; CaseSense Off compares 'a'..'z' as
 * upper case.  a3 = String$.
 *
 * BUG(orig) (#287): Startpos <= 0 is not checked - the compare starts
 * before the string (Instr(s$,"",0) = 0, Instr(s$,"",-5) = -5).
 *
 * @param s     d0: the string to search in
 * @param f     d1: the string to find
 * @param start d2: the 1-based start position
 * @param lens  a2: the length stack (String$'s length on top, then
 *              Findstring$'s)
 * @return d0: the 1-based position, 0 if not found
 */
static ULONG BLITZ_ROUTINE instrstring2_c(UBYTE *s BL_REG(d0), const UBYTE *f BL_REG(d1),
                                          LONG start BL_REG(d2), const ULONG *lens BL_REG(a2))
{
    return instr(s, f, start - 2, lens);
}
BLITZ_THUNK(instrstring2, instrstring2_c, "d1/a0-a1");

/**
 * @brief Replace$(String$,Findstring$,Replacestring$) - String$ with
 * every Findstring$ replaced by Replacestring$.
 *
 * The result is built at a3 (behind the arguments), then moved to d0$.
 * End test: a0+Len(f$) > address of f$ (BHI) - the find string is
 * expected right behind s$ in the work area.  CaseSense Off compares
 * 'a'..'z' as upper case.
 * DIFF: with an empty find string the original pops only two lengths
 * (two MOVE.l -(a2) before BEQ 'skip), otherwise three (SUBQ #4,a2), so
 * a2 is left at different positions; the C version leaves a2 unchanged.
 *
 * BUG(orig) (#424): an empty find string returns s$ unchanged with a3
 * left at its entry value.
 *
 * @param s    d0: the string
 * @param f    d1: the string to find
 * @param r    d2: the replacement
 * @param lens a2: the length stack (the three lengths, String$'s on top)
 * @return d0: the length of the result; a3 = its end
 */
static ULONG BLITZ_ROUTINE replace_c(UBYTE *s BL_REG(d0), const UBYTE *f BL_REG(d1),
                                     const UBYTE *r BL_REG(d2), const ULONG *lens BL_REG(a2))
{
    const ULONG *sp = lens;
    ULONG slen, flen, newlen, left, n;
    UBYTE *out;
    const UBYTE *at, *scan, *src;
    const UBYTE *find;                          /* a1: the find string */
    const UBYTE *repl;                          /* a1: the replacement string */

    slen = BL_POPL(sp);                         /* len 1 */
    flen = BL_POPL(sp);                         /* len 2 */
    if (!flen)                                  /* BEQ 'skip */
        return slen;
    sp--;                                       /* SUBQ #4,a2: (a2) = len 3 */
    out = blitz_string;                         /* MOVE.l a3,-(a7) */
    at = s;
    newlen = 0;
    left = slen;
    while (left) {                              /* 'loop: BEQ 'done */
        if ((ULONG)at + flen > (ULONG)f) {      /* CMP.l d1,d7:BHI 'done2 */
            do {
                *blitz_string++ = *at++;
                newlen++;
            } while (--left);
            break;
        }
        scan = at;                              /* MOVE.l a0,-(a7) */
        find = f;
        n = flen;
        if (csense) {
            do {
                if (*scan++ != *find++)         /* 'norm: CMPM.b */
                    break;
            } while (--n);
        } else {
            do {
                UBYTE sc = upcase(*scan++), fc = upcase(*find++);
                if (sc != fc)                   /* BNE '_next2 */
                    break;
            } while (--n);
        }
        if (n) {                                /* '_next: copy one character */
            *blitz_string++ = *at++;
            newlen++;
            left--;
            continue;
        }
        repl = r;                               /* 'match */
        for (n = *sp; n; n--)                   /* 'loop3 */
            *blitz_string++ = *repl++;
        newlen += *sp;                          /* 'more: ADD.l (a2),d5 */
        at += flen;
        left -= flen;
    }
    src = out;                                  /* 'done: MOVE.l (a7)+,a3 */
    blitz_string = s;                           /* MOVE.l (a7)+,a0 */
    return movestr(&src, newlen);
}
BLITZ_THUNK(replace, replace_c, "d1/a0-a1");

/**
 * @brief Mki$(Integer) - the low word as a 2 byte string (big endian).
 *
 * @param v d0: the value
 * @return d0: 2, the length; a3 = the end of the result
 */
static ULONG BLITZ_ROUTINE mkistring_c(ULONG v BL_REG(d0))
{
    blitz_string[1] = (UBYTE)v;
    blitz_string[0] = (UBYTE)(v >> 8);
    blitz_string += 2;
    return 2;
}
BLITZ_THUNK(mkistring, mkistring_c, "d1/a0-a1");

/**
 * @brief Mkl$(Long), Mkq$(Quick) - the value as a 4 byte string (big
 * endian).
 *
 * @param v d0: the value
 * @return d0: 4, the length; a3 = the end of the result
 */
static ULONG BLITZ_ROUTINE mklstring_c(ULONG v BL_REG(d0))
{
    blitz_string[3] = (UBYTE)v;
    blitz_string[2] = (UBYTE)(v >> 8);
    blitz_string[1] = (UBYTE)(v >> 16);
    blitz_string[0] = (UBYTE)(v >> 24);
    blitz_string += 4;
    return 4;
}
BLITZ_THUNK(mklstring, mklstring_c, "d1/a0-a1");

/**
 * @brief Cvi(String$) - the first 2 bytes of the string as a word.
 *
 * a3 = d0$.  Cvi leaves the upper word of the string address in the
 * upper word of d0 (MOVE.b/LSL #8 on the low word only; Cvi is a word
 * function).  DIFF: the original does not pop the length (a2 unchanged,
 * unlike the other string functions); the C version leaves a2 unchanged
 * as everywhere (same result).
 *
 * BUG(orig) (#288): the argument's length is not checked (shorter strings
 * read the bytes behind them).
 *
 * @param s d0: the string
 * @return d0.w: the word (big endian)
 */
static ULONG BLITZ_ROUTINE cvistring_c(const UBYTE *s BL_REG(d0))
{
    blitz_string = (UBYTE *)s;
    return ((ULONG)s & 0xffff0000) | (UWORD)(s[0] << 8 | s[1]);
}
BLITZ_THUNK(cvistring, cvistring_c, "d1/a0-a1");

/**
 * @brief Cvl(String$), Cvq(String$) - the first 4 bytes of the string as
 * a long (Cvq: as a Quick).
 *
 * a3 = d0$.  DIFF: the length is not popped, as in Cvi.
 *
 * BUG(orig) (#288): the argument's length is not checked.
 *
 * @param s d0: the string
 * @return d0: the long (big endian)
 */
static ULONG BLITZ_ROUTINE cvlstring_c(const UBYTE *s BL_REG(d0))
{
    blitz_string = (UBYTE *)s;
    return (ULONG)s[0] << 24 | (ULONG)s[1] << 16 | (ULONG)s[2] << 8 | s[3];
}
BLITZ_THUNK(cvlstring, cvlstring_c, "d1/a0-a1");

/**
 * @brief Len(String$) - the length of the string.
 *
 * Inline code (copied into the program): MOVE.l d0,a3:MOVE.l -(a2),d0 -
 * pops the length (a2 changes, unlike the C routines).
 * Parameter d0: the string; a2: the length stack.
 * Result d0: the length; a3 = the string.
 */
BLITZ_INLINE(lenstring,
    "   move.l  d0,a3\n"
    "   move.l  -(a2),d0");
