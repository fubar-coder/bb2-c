/*
 * printlib - command library 187: Print NPrint Str$ Format FloatMode UStr$.
 *
 * C translation of bblibsrc/printlib (binary of the CD's acidlibs checked:
 * same as the source).  The source INCLUDEs libincs:ffptoascii, which is
 * not on the CD; that routine (FFP -> ASCII with its table of powers of
 * ten) is translated from the binary (offsets $658-$9D3 of ref/printlib.obj).
 *
 * Output goes through inputoutputlib (164): every string is passed to
 * dooutstring1 (a0 = characters, d0 = length, token $5208), the newline
 * of NPrint to dooutsingle ($5207).  inputoutputlib calls the current
 * output handler (DefaultOutput, FileOutput, WindowOutput ...).  Str$ and
 * UStr$ install their own handler (strprint, below) that copies the
 * characters to a3, print the value as Print would, and restore the old
 * handler - with Format active, Str$ formats like Print.
 *
 * Arguments of #unknown type (!args{8}, !repargs{0,1,8}): the compiler
 * pushes the value and then its type as a word; the routine pops the type
 * (MOVE -(a2),d0) and the value (MOVE.l -(a2) for long/quick/float, MOVE
 * -(a2) for byte/word, length then address for a string).  Print/NPrint
 * get the number of arguments in d7 (move.w #numreps,d7).  Str$/UStr$
 * have the value in d0 and pop only the type.
 * DIFF: a2 is a register parameter (const UBYTE *args BL_REG(a2)), read
 * through a local cursor and left unchanged; the original returns with a2
 * decremented by everything it popped (see notes/cmdlibs-port.md).
 *
 * Number formats (Format string empty):
 *   byte/word   decimal, "-" for negative values (DIVU loop, 3/5 digits)
 *   long        decimal by repeated subtraction of powers of ten
 *   quick       integer part, "." and up to 4 decimals (fraction * 10000,
 *               rounded), trailing zeros and a leading "0" removed
 *   float       ffptoascii: 7 significant digits, FloatMode 0 = fixed
 *               point for 1E-9 <= |x| < 1E+7 (decimal exponent -9..6),
 *               else exponent form (1.234567E+7); 1 = always exponent
 *               form; -1 = always fixed point (also whenever a Format
 *               string is set)
 * Format: "#" = digit or space, "0" = digit or zero, "." decimal point,
 * "-" = "-" if negative else space, "+" = "+"/"-", "," = "," (space in
 * the leading pad), other characters literally.  Too many integer digits
 * give "?" and the lowest digits; rounding at the last decimal.
 *
 * BUG(orig), kept (for the comparison with AmiBlitz3):
 *   - Quick: a fraction that rounds to 10000 (x.99995 and more) loses the
 *     carry: Print 2.99999 -> "2." (four digits "0000" stripped to ".");
 *     a fraction that rounds to 0 leaves the point: 1+1/65536 -> "1.",
 *     1/65536 -> "."; numbers below 1 have no leading 0 (".5").
 *   - Float, fixed point: no leading 0 either (".5"); 0 is "0".
 *   - Format: rounding that carries out of the first digit is lost
 *     (999.96 with "###.#" -> "000.0"); numbers without an integer digit
 *     (|x| < 1, d3 = -(integer digits) = 0) are never rounded, whatever
 *     the format (0.567 with ".##" or "###.##" -> ".56"); the
 *     sign is lost when the format has no "-"/"+" (-7 with "###" ->
 *     "  7"); too many integer digits give "?" and then only padding for
 *     the whole integer part (1234.5 with "##" -> "?  "); the round-up
 *     loop starts at the last copied digit (d1; d1 = 0 with "address -1"
 *     would need a rounding without a copied integer digit, which does
 *     not occur: rounding needs d3 != 0 = at least one integer digit).
 *   - Format with more than 32 characters (no check without the
 *     debugger): the copy runs over formlen/formlead/formtrail/decpos and
 *     further into the conversion buffers (the layout of the variables
 *     below is that of the original so that this goes the same way).
 *   - ffptoascii, fixed point with FloatMode -1 (or a Format string) and
 *     |x| < 1E-17: the digits are written past the end of the 50 byte
 *     buffer into the first entry (1E19) of the table of powers of ten,
 *     which changes later conversions of numbers >= 1E18.
 *   - Str$ of a string is printed as a byte (the low byte of the address).
 *   - strprint, the handler of Str$, has the same code for single
 *     characters (JSR (a6)) as for strings (a0, d0) - inputoutputlib's
 *     dooutsingle would copy d0 characters from a0.  printlib never
 *     outputs single characters to it.
 *   - Str$ installs its handler with dosetout twice (install and
 *     restore), so every Str$ pushes two entries on inputoutputlib's
 *     output stack (a ring of 16, harmless).
 *
 * Registers: the original changes d0-d7/a0-a2 (Print), d0-d1/a0-a2/d4-d6
 * (Str$, d2-d3 kept), d1-d5/a0-a1 (Format, d0 kept); C keeps d2-d7/a2/a6,
 * the thunk of Format additionally d0, FloatMode and init keep everything
 * like the original.  a3 only changes where the original changes it
 * (string result of Str$/UStr$).
 *
 * Error check (debugger only): formatchk - Format string longer than 32
 * characters -> "Format String too long".
 */
#include "blitzlib.h"

/* entry points (thunks / C routines, defined below) */
void doformat(void), dofloatmode(void), codeinit(void), formatchk(void);
void BLITZ_ROUTINE doprint(UWORD n BL_REG(d7), const UBYTE *args BL_REG(a2));
void BLITZ_ROUTINE donprint(UWORD n BL_REG(d7), const UBYTE *args BL_REG(a2));
ULONG BLITZ_ROUTINE dostrstr(ULONG v BL_REG(d0), const UBYTE *args BL_REG(a2));
ULONG BLITZ_ROUTINE doustrstr(ULONG v BL_REG(d0), const UBYTE *args BL_REG(a2));

BLITZ_ENTRY_DECL(init);

/* !libheader{#printlib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_printlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_REPARGS(0, 1, BL_UNKNOWN);
BLITZ_LIBS(LIB_ffplib, BL_LA(6));
BLITZ_SUBS(doprint, 0, 0);
BLITZ_NAME("Print", "Expression[,Expression...]");

BLITZ_ASTATEMENT();
BLITZ_REPARGS(0, 1, BL_UNKNOWN);
BLITZ_LIBS(LIB_ffplib, BL_LA(6));
BLITZ_SUBS(donprint, 0, 0);
BLITZ_NAME("NPrint", "Expression[,Expression...]");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_UNKNOWN);
BLITZ_LIBS(LIB_ffplib, BL_LA(6));
BLITZ_SUBS(dostrstr, 0, 0);
BLITZ_NAME("Str$", "(Expression)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(doformat, formatchk, 0);
BLITZ_NAME("Format", "FormatString");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dofloatmode, 0, 0);
BLITZ_NAME("FloatMode", "Mode : -1=standard, 0=guess, 1=exp");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_UNKNOWN);
BLITZ_LIBS(LIB_ffplib, BL_LA(6));
BLITZ_SUBS(doustrstr, 0, 0);
BLITZ_NAME("UStr$", "(Expression)");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------ variables */

/* The variables in the order of the original (offsets $1C6-$233 of the
   binary): a Format string longer than 32 characters overwrites the ones
   behind `form` (BUG(orig), see above), and ezchar reads the byte before
   `chars` ($20) for a "," at the start.  All accesses that may leave a
   member go through pointers into the whole struct. */
static struct BL_PACKED {
    UBYTE form[32];         /* the Format string */
    UWORD formlen;          /* its length (0 = no Format) */
    UWORD formlead;         /* number of digit places before the point */
    UWORD formtrail;        /* ... after the point */
    UWORD decpos;           /* pos of decimal point (always 0/-1, unused) */
    UWORD space2;           /* Dc $2020 */
    UBYTE chars[32];        /* converted format string */
    UBYTE numbuff[32];      /* makelong; makeval/quick write backwards */
    UWORD numbufff;         /*   from here (and the 0 here ends them) */
    UWORD floatmode;        /* 0 = guess, + = never kludge, - = always kludge */
} pl = { .space2 = 0x2020 };

#define PL_BASE   ((UBYTE *)&pl)
#define PL_FORM   (PL_BASE + 0)
#define PL_CHARS  (PL_BASE + 42)
#define PL_NUMBUFF (PL_BASE + 74)
#define PL_NUMBUFFF (PL_BASE + 106)

/* ffptoascii's buffers and table, in the order of the binary ($872-$9D3):
   buf1 = sign, digits and exponent form, buf2 = fixed point form (the
   point at buf2[24]), then the 48 powers of ten {binary exponent,
   mantissa} 1E19 ... 1E-28 - writable, see BUG(orig) above. */
static struct BL_PACKED {
    UBYTE buf1[16];
    UBYTE buf2[50];
    UWORD tab[48][3];
} fa = {
    .tab = {
#define T(e, m) { (e), (UWORD)((m) >> 16), (UWORD)(m) }
        T(0x0040, 0x8ac72305), T(0x003c, 0xde0b6b3a), T(0x0039, 0xb1a2bc2f),
        T(0x0036, 0x8e1bc9bf), T(0x0032, 0xe35fa932), T(0x002f, 0xb5e620f5),
        T(0x002c, 0x9184e72a), T(0x0028, 0xe8d4a510), T(0x0025, 0xba43b740),
        T(0x0022, 0x9502f900), T(0x001e, 0xee6b2800), T(0x001b, 0xbebc2000),
        T(0x0018, 0x98968000), T(0x0014, 0xf4240000), T(0x0011, 0xc3500000),
        T(0x000e, 0x9c400000), T(0x000a, 0xfa000000), T(0x0007, 0xc8000000),
        T(0x0004, 0xa0000000), T(0x0001, 0x80000000), T(0xfffd, 0xcccccccd),
        T(0xfffa, 0xa3d70a3d), T(0xfff7, 0x83126e98), T(0xfff3, 0xd1b71759),
        T(0xfff0, 0xa7c5ac47), T(0xffed, 0x8637bd06), T(0xffe9, 0xd6bf94d6),
        T(0xffe6, 0xabcc7712), T(0xffe3, 0x89705f41), T(0xffdf, 0xdbe6fecf),
        T(0xffdc, 0xafebff0c), T(0xffd9, 0x8cbccc09), T(0xffd5, 0xe12e1342),
        T(0xffd2, 0xb424dc35), T(0xffcf, 0x901d7cf7), T(0xffcb, 0xe69594bf),
        T(0xffc8, 0xb877aa32), T(0xffc5, 0x9392ee8f), T(0xffc1, 0xec1e4a7e),
        T(0xffbe, 0xbce50865), T(0xffbb, 0x971da050), T(0xffb7, 0xf1c90081),
        T(0xffb4, 0xc16d9a01), T(0xffb1, 0x9abe14cd), T(0xffad, 0xf79687ae),
        T(0xffaa, 0xc6120625), T(0xffa7, 0x9e74d1b8), T(0xffa3, 0xfd87b5f3),
#undef T
    },
};

#define FA_BASE  ((UBYTE *)&fa)
#define FA_BUF1  (FA_BASE + 0)
#define FA_BUF2  (FA_BASE + 16)
#define FA_ONE   (FA_BASE + 66 + 19 * 6)    /* table entry 1E0 */

/* word / long at a (word aligned) address */
#define RDW(p) (*(const UWORD *)(p))
#define RDL(p) (*(const ULONG *)(p))

/* ------------------------------------------------------------- output */

/* ALibJsr #inputoutputlib LSL 7+8: string to the current output, d0 =
   what the handler returns (the length for strprint) */
static ULONG output(const UBYTE *s, ULONG len)
{
    struct bl_regs r = { .d0 = len, .a0 = (ULONG)s };

    return BLITZ_ALIBJSR(TOK_iooutstring1, r);
}

/* ------------------------------------------------------------- Format */

/* ezchar - copy the literal characters of the format string up to the
   next digit place ("#" or "0", not consumed): returns 0 (Z, "BEQ") at a
   digit place, 1 (NE) when the format string is used up (d6 < 0).
   d2 = minus flag (0 = number negative, -1 = not). */
struct ez {
    UBYTE *a1;          /* output (chars) */
    UBYTE *a2;          /* format string */
    UWORD d6;           /* characters left - 1 */
    UWORD d2;
};

static int ezchar(struct ez *z)
{
    for (;;) {
        if ((WORD)--z->d6 < 0)                  /* SUBQ #1,d6:BMI 'noch */
            return 1;
        switch (*z->a2) {
        case '#':
        case '0':
            return 0;
        case '-':                               /* 'dominus */
            *z->a1++ = (WORD)z->d2 >= 0 ? '-' : ' ';
            z->a2++;                            /* ezchar2 */
            break;
        case '+':                               /* 'doplus */
            *z->a1 = '+';
            if ((WORD)z->d2 >= 0)
                *z->a1 = '-';
            z->a1++;                            /* ezchar3 / MOVE.b (a1)+ */
            z->a2++;
            break;
        case '.':                               /* 'dopoint */
            *z->a1++ = '.';
            z->a2++;
            break;
        case ',':                               /* 'docomma */
            if (z->a1[-1] == ' ') {             /* 'ezspc */
                *z->a1++ = ' ';
                z->a2++;
                break;
            }
            /* fall through */
        default:
            *z->a1++ = *z->a2++;
            break;
        }
    }
}

/* prtype1 - output the number a0 (d0 characters), formatted with the
   Format string if there is one.  Word arithmetic as in the original. */
static ULONG prtype1(const UBYTE *a0, ULONG len)
{
    struct ez z;
    UWORD d0, d1w, d3, d4, d5, d6;
    ULONG d1 = 0;                               /* after the last copied digit */
    UBYTE *a1;
    const UBYTE *p;

    if (!pl.formlen)                            /* MOVE formlen(pc),d1:BEQ 'done */
        return output(a0, len);

    d0 = len;
    d1w = 0xffff;                               /* pos of point */
    z.d2 = 0xffff;                              /* pos of minus */
    if (*a0 == '-') {
        z.d2 = 0;
        a0++;
        d0--;
    }
    d3 = 0;                                     /* leading digits */
    d4 = 0;                                     /* trailing digits */
    d5 = 0;                                     /* pos counter */
    d6 = d0 - 1;
    p = a0;
    do {
        if (*p == '.')
            d1w = d5;
        else {
            d3++;
            if ((WORD)d1w >= 0) {
                d3--;
                d4++;
            }
        }
        p++;
        d5++;
    } while (--d6 != 0xffff);                   /* DBF d6,'loop */

    /* OK, now we copy to 'chars' buffer. */
    a1 = PL_CHARS;
    z.a2 = PL_FORM;
    z.d6 = pl.formlen;

    /* Leading chars first.... */
    d5 = pl.formlead - d3;
    if ((WORD)d5 < 0) {                         /* Overflow! */
        *a1++ = '?';
        d5 = -d5;
        a0 += (WORD)d5;                         /* ADD d5,a0 (sign extended) */
        d0 -= d5;
        d5 = pl.formlead;
    }
    d3 = d5;
    d5 = pl.formlead;
    if (d5) {
        d5--;
        do {                                    /* 'leadloop */
            z.a1 = a1;
            if (ezchar(&z))
                goto alldone_z;
            a1 = z.a1;
            if (*z.a2++ == '#') {
                if ((WORD)--d3 >= 0 || (WORD)--d0 < 0)
                    *a1++ = ' ';                /* 'putspc */
                else {
                    *a1++ = *a0++;
                    d1 = (ULONG)a1;
                }
            } else {                            /* 'iszero */
                if ((WORD)--d3 >= 0)
                    *a1++ = '0';                /* 'putzero */
                else if ((WORD)--d0 < 0)
                    *a1++ = ' ';                /* -> 'putspc */
                else {
                    *a1++ = *a0++;
                    d1 = (ULONG)a1;
                }
            }
        } while (--d5 != 0xffff);               /* DBF d5,'leadloop */
    }

    /* 'dotrail */
    if ((WORD)--d0 >= 0) {
        if (*a0++ == '.')
            goto askip;
        a0--;
    }
    d0++;                                       /* 'askip2 */
askip:
    d5 = pl.formtrail;
    z.a1 = a1;
    if (d5) {
        d5--;
        do {                                    /* 'trailloop */
            if (ezchar(&z))
                goto alldone2;
            if (*z.a2++ == '#') {
                if ((WORD)--d4 < 0 || (WORD)--d0 < 0)
                    *z.a1++ = ' ';              /* 'putspc2 */
                else {
                    *z.a1++ = *a0++;
                    d1 = (ULONG)z.a1;
                }
            } else {
                if ((WORD)--d4 < 0 || (WORD)--d0 < 0)
                    *z.a1++ = '0';              /* 'putzero2 */
                else {
                    *z.a1++ = *a0++;
                    d1 = (ULONG)z.a1;
                }
            }
        } while (--d5 != 0xffff);
    }
    while (!ezchar(&z))                         /* 'doexcess: BSR ezchar:BEQ */
        ;
alldone2:
    /* round up if the next digit is >= 5 (TST.l d3: the upper word of d3
       is always 0) */
    if ((WORD)--d0 >= 0 && d3 && *a0 >= '5') {
        UBYTE *q = (UBYTE *)d1;                 /* 0 if no digit was copied */

        for (;;) {                              /* 'sssl */
            q = (UBYTE *)((ULONG)q - 1);
            if ((ULONG)q < (ULONG)PL_CHARS)     /* CMP.l #chars,a0:BCS */
                break;
            if (*q < '0' || *q > '9')
                continue;
            if (++*q <= '9')
                break;
            *q = '0';
        }
    }
alldone_z:
    a1 = z.a1;
    return output(PL_CHARS, a1 - PL_CHARS);     /* SUB.l a0,a1:MOVE.l a1,d0 */
}

/* prtype2 - a0 = buffer, null terminated */
static ULONG prtype2(const UBYTE *a0)
{
    const UBYTE *a1 = a0;
    ULONG n = (ULONG)-1;

    do
        n++;
    while (*a1++);
    return prtype1(a0, n);
}

/* doformat - Format FormatString: copy the string to form, count the
   digit places before and after the point.  Length from the a2 stack
   (MOVE.l -(a2),d1), only its low word is used. */
static void BLITZ_ROUTINE doformat_c(const UBYTE *s BL_REG(d0),
                                     const UBYTE *args BL_REG(a2))
{
    const UBYTE *sp = args;
    UBYTE *a1 = PL_FORM;
    UWORD d1 = BL_POPL(sp), d2, d3, d4, d5;

    pl.formlen = d1;
    if (!d1)
        return;
    d1--;
    d2 = 0xffff;                                /* pos of decimal point */
    d3 = 0;                                     /* number of leading chars */
    d4 = 0;                                     /* number of trailing... */
    d5 = 0;                                     /* counter (never counted) */
    do {
        UBYTE c = *s++;

        *a1++ = c;                              /* may run over formlen ... */
        if (c == '.')
            d2 = d5;
        else if (c == '#' || c == '0') {
            d3++;
            if ((WORD)d2 >= 0) {
                d3--;
                d4++;
            }
        }
    } while (--d1 != 0xffff);
    /* MOVE d2,decpos:MOVE d3,formlead:MOVE d4,formtrail (through the
       struct: the loop may have written over them) */
    *(volatile UWORD *)&pl.decpos = d2;
    *(volatile UWORD *)&pl.formlead = d3;
    *(volatile UWORD *)&pl.formtrail = d4;
}
/* the original keeps d0 (MOVE.l d0,a0) */
BLITZ_THUNK(doformat, doformat_c, "d0");

static void BLITZ_ROUTINE dofloatmode_c(UWORD m BL_REG(d0))
{
    pl.floatmode = m;                           /* MOVE d0,floatmode */
}
BLITZ_THUNK(dofloatmode, dofloatmode_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE codeinit_c(void)
{
    pl.formlen = 0;
    pl.floatmode = 0;
}
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");

/* --------------------------------------------------- integer -> ASCII */

/* makeval3 - d1+1 digits of d0 backwards before a0 (DIVU #10 per digit;
   d0 keeps the quotient) */
static UBYTE *makeval3(UBYTE *a0, ULONG *d0, UWORD d1)
{
    ULONG v = *d0;

    do {
        v = bl_divu_w(v, 10);                   /* DIVU #10,d0 */
        v = BL_SWAP(v);
        BL_SETW(v, (UWORD)v | 48);              /* OR #48,d0 */
        *--a0 = (UBYTE)v;
        v = BL_SWAP(v & 0xffff0000);            /* CLR d0:SWAP d0 */
    } while (--d1 != 0xffff);
    *d0 = v;
    return a0;
}

/* makeval2 - makeval3 without leading zeros ("0" if all are zero; the
   string behind the digits ends the scan) */
static UBYTE *makeval2(UBYTE *a0, ULONG d0, UWORD d1)
{
    a0 = makeval3(a0, &d0, d1);
    while (*a0++ == '0')
        ;
    if (!*--a0)
        *--a0 = '0';
    return a0;
}

/* makeval - signed d0 with at most d1+1 digits, then output */
static ULONG makeval(ULONG d0, UWORD d1)
{
    UBYTE *a0 = PL_NUMBUFFF;

    if ((LONG)d0 < 0) {
        a0 = makeval2(a0, -d0, d1);
        *--a0 = '-';
        return prtype2(a0);
    }
    return prtype2(makeval2(a0, d0, d1));
}

static const ULONG divisors[10] = {
    1, 10, 100, 1000, 10000, 100000,
    1000000, 10000000, 100000000, 1000000000,
};

/* makelong - decimal of a long to numbuff (-$80000000 stays negative
   after NEG and is printed unsigned: "-2147483648") */
static void makelong(ULONG d0)
{
    UBYTE *a0 = PL_NUMBUFF;
    int d1 = 0;
    WORD i;

    if ((LONG)d0 < 0) {
        *a0++ = '-';
        d0 = -d0;
    }
    for (i = 9; i >= 0; i--) {
        UBYTE d3 = '0';

        while (d0 >= divisors[i]) {             /* CMP.l:BCS (unsigned) */
            d0 -= divisors[i];
            d3++;
        }
        if (d3 != '0' || d1) {
            *a0++ = d3;
            d1 = 1;
        }
    }
    if (!d1)
        *a0++ = '0';
    *a0 = 0;
}

static ULONG plong2(ULONG d0)
{
    makelong(d0);
    return prtype2(PL_NUMBUFF);
}

static ULONG pbyte2(ULONG d0)
{
    return makeval((LONG)(BYTE)d0, 2);          /* EXT d0:EXT.l d0 */
}

static ULONG pword2(ULONG d0)
{
    return makeval((LONG)(WORD)d0, 4);          /* EXT.l d0 */
}

/* 'quick3 - d2 = what to print (not negative, except $80000000): fraction
   "." + 4 digits with trailing zeros removed, then the integer part */
static UBYTE *quick3(ULONG d2)
{
    UBYTE *a0 = PL_NUMBUFFF;
    ULONG d0 = (UWORD)d2;

    if (d0) {
        UBYTE *a1;

        d0 = (ULONG)(UWORD)d0 * 10000;          /* MULU #10000,d0 */
        if ((UWORD)d0 >= 0x8000)                /* CMP #$8000,d1:BCS */
            d0 = (d0 >> 16) + 1;
        else
            d0 >>= 16;
        a0 = makeval3(PL_NUMBUFFF, &d0, 3);
        *--a0 = '.';
        a1 = PL_NUMBUFFF;
        /* 'f0l: the "." test compares the same '0' again, so it never
           stops at the point (BUG(orig): "x." for .0000) */
        while (a1[-1] == '0' && *--a1 != '.')
            ;
        *a1 = 0;                                /* 'skip2 */
    }
    return makeval2(a0, d2 >> 16, 4);           /* 'skip3 */
}

static ULONG pquick2(ULONG d0)
{
    ULONG d2 = d0;
    UBYTE *a0;

    if ((LONG)d2 < 0) {
        a0 = quick3(-d2);
        *--a0 = '-';
    } else
        a0 = quick3(d2);
    return prtype2(a0);
}

/* ------------------------------------------------------- ffptoascii */

/* ffptoascii (libincs:ffptoascii, from the binary) - FFP number d0 to a
   null terminated string; mode = FloatMode (<0 fixed point, 0 guess, >0
   exponent form).  Integer arithmetic only (no mathffp): 7 digits from
   the table of powers of ten, rounding with $80 below the mantissa. */
static UBYTE *ffptoascii(ULONG d0, UWORD mode)
{
    UBYTE *a0 = FA_BUF1, *a1, *start;
    UBYTE *a3;
    UBYTE b;
    WORD d1;
    UWORD d2, d3;
    int i;

    if (!(UBYTE)d0)                             /* TST.b d0:BNE */
        d0 = 65;                                /* MOVEQ #65,d0 */
    a0[0] = '+';                                /* MOVE #$2b2e,0(a0) */
    a0[1] = '.';
    b = (UBYTE)d0;
    if (b & 0x80)
        a0[0] += 2;                             /* '-' */
    b = (UBYTE)(b + b) ^ 0x80;                  /* ADD.b d1,d1:EOR.b #$80 */
    d0 = (d0 & ~0xffUL) | 0x80;                 /* MOVE.b #$80,d0 */
    d1 = (WORD)(BYTE)b >> 1;                    /* EXT d1:ASR #1,d1 */

    /* find the power of ten <= the number: a3 -> entry, d3 = exponent+1 */
    d3 = 1;
    a3 = FA_ONE;
    if (d1 < (WORD)RDW(a3)) {
        do {
            a3 += 6;
            d3--;
        } while (d1 < (WORD)RDW(a3));
        if (d1 != (WORD)RDW(a3))
            goto found;
    } else if (d1 > (WORD)RDW(a3)) {
        do {
            a3 -= 6;
            d3++;
        } while (d1 > (WORD)RDW(a3));
        if (d1 != (WORD)RDW(a3))
            goto next;
    }
    if (d0 >= RDL(a3 + 2))                      /* CMP.l 2(a3),d0:BCC */
        goto found;
next:
    a3 += 6;
    d3--;
found:
    /* "E+00" (computed, never used) */
    a0[10] = 'E';
    a0[11] = '+';
    a0[12] = '0';
    a0[13] = '0';
    d2 = d3;
    if ((WORD)d2 < 0) {
        d2 = -d2;
        a0[11] += 2;
    }
    if (d2 >= 10) {
        a0[12] += 1;
        d2 -= 10;
    }
    a0[13] |= (UBYTE)d2;

    /* 7 digits: subtract the power of ten, shifted to the number's
       exponent, until it borrows */
    a1 = a0 + 2;
    if ((LONG)d0 >= 0 || !a3[5])                /* exact power: no rounding */
        d0 &= ~0xffUL;
    for (i = 0; i < 7; i++) {
        UWORD sh = d1 - RDW(a3);
        ULONG d5 = RDL(a3 + 2);
        UWORD d4 = 9;
        int borrow;

        a3 += 6;
        sh &= 63;                               /* LSR.l d4,d5: count mod 64 */
        d5 = sh >= 32 ? 0 : d5 >> sh;
        for (;;) {                              /* SUB.l d5,d0:DBCS d4 */
            borrow = d0 < d5;
            d0 -= d5;
            if (borrow || --d4 == 0xffff)
                break;
        }
        if (!borrow)
            d4 &= 0xff00;                       /* CLR.b d4 */
        d0 += d5;
        *a1++ = (UBYTE)-(UBYTE)((UBYTE)d4 - 9) | '0';
    }
    *a1++ = '0';

    if ((WORD)mode < 0
        || (mode == 0 && (d3 < 8 || d3 >= 0xfff8))) {
        /* fixed point */
        UBYTE *b2 = FA_BUF2;

        b2[25] = '0';
        for (i = 0; i < 24; i++) {
            b2[i] = '0';
            b2[26 + i] = '0';
        }
        b2[24] = '.';
        a0 = b2 + 24;
        if ((WORD)d3 < 0)
            a0++;
        a0 -= (WORD)d3;                         /* SUBA.w d3,a0 */
        a1 = FA_BUF1 + 2;
        start = a0;
        for (i = 0; i < 8; i++) {
            if (*a0 == '.')
                a0++;
            *a0++ = *a1++;
        }
        if (a0 >= b2 + 25) {                    /* CMPA.l #$89b,a0:BCS */
            for (;;) {                          /* strip trailing zeros */
                if (*--a0 != '0')
                    break;
                *a0 = 0;
            }
            if (*a0 != '.') {
                *++a0 = 0;
                goto sign;
            }
            if (!(a0 > start)) {                /* 0: "0" without sign */
                a0[0] = '0';
                a0[1] = 0;
                return a0;
            }
        }
        b2[24] = 0;                             /* integer only */
sign:
        a0 = start;
        if (a0 >= b2 + 24)                      /* nothing before the point */
            a0 = b2 + 24;
        *--a0 = FA_BUF1[0];
        if (*a0 != '-')
            a0++;
        return a0;
    }

    /* exponent form: d.dddddd without trailing zeros, E+-n */
    a0 = FA_BUF1;
    a0[1] = a0[2];
    a0[2] = '.';
    while (*--a1 == '0')
        ;
    if (*a1++ == '.') {
        a1--;
        if (a0[1] == '0')
            goto term;
    }
    d3--;
    *a1++ = 'E';
    *a1++ = '+';
    if ((WORD)d3 < 0) {
        a1[-1] = '-';
        d3 = -d3;
    }
    {
        ULONG q = bl_divu_w(d3, 10);            /* MOVEQ #0,d2:MOVE d3,d2:DIVU */

        d2 = (UWORD)q;
        if (d2)
            *a1++ = d2 + '0';
        d3 -= d2 * 10;
        *a1++ = d3 + '0';
    }
term:
    *a1 = 0;
    if (*a0 != '-')
        a0++;
    return a0;
}

/* pfloat2 - FloatMode, or always fixed point with a Format string */
static ULONG pfloat2(ULONG d0)
{
    UWORD mode = pl.floatmode;

    if (pl.formlen)
        mode = 0xffff;
    return prtype2(ffptoascii(d0, mode));
}

/* ------------------------------------------------------- Print, NPrint */

/* ptype - one argument of type t, value(s) from the a2 stack */
static ULONG ptype(UWORD t, const UBYTE **sp)
{
    switch (t) {
    case 4:
        return pquick2(BL_POPL(*sp));
    case 3:
    case 6:
        return plong2(BL_POPL(*sp));
    case 7: {                                   /* 'pstring */
        ULONG len = BL_POPL(*sp);
        const UBYTE *s = (const UBYTE *)BL_POPL(*sp);

        return output(s, len);
    }
    case 2:
        return pword2(BL_POPW(*sp));
    case 5:
        return pfloat2(BL_POPL(*sp));
    default:                                    /* 'pbyte */
        return pbyte2(BL_POPW(*sp));
    }
}

void BLITZ_ROUTINE doprint(UWORD n BL_REG(d7), const UBYTE *args BL_REG(a2))
{
    const UBYTE *sp = args;

    n--;                                        /* SUBQ #1,d7 */
    do {
        UWORD t = BL_POPW(sp);

        ptype(t, &sp);
    } while (--n != 0xffff);                    /* DBF d7,'loop */
}

void BLITZ_ROUTINE donprint(UWORD n BL_REG(d7), const UBYTE *args BL_REG(a2))
{
    struct bl_regs r = { .d0 = 10 };

    doprint(n, args);
    BLITZ_ALIBJSR(TOK_iooutsingle, r);
}

/* ---------------------------------------------------------- Str$, UStr$ */

/* strprint - output handler of Str$: entries -8 (null terminated string,
   stprint3), -4 (string a0, d0 = length, stprint2) and 0 (single
   character - in the original the same code as stprint2, BUG(orig)).
   The characters go to a3, d0 = length. */
static ULONG BLITZ_ROUTINE stprint2_c(ULONG len BL_REG(d0), const UBYTE *s BL_REG(a0))
{
    ULONG n = len;

    while (n) {
        *blitz_string++ = *s++;
        n--;
    }
    return len;
}

static ULONG BLITZ_ROUTINE stprint3_c(const UBYTE *s BL_REG(a0))
{
    ULONG n = (ULONG)-1;

    do
        n++;
    while ((*blitz_string++ = *s++));
    blitz_string--;                             /* SUBQ #1,a3 */
    return n;
}

void strprint(void);
__asm__("   .text\n   .even\n"
        "   bra.w   1f\n"                       /* -8: stprint3 */
        "   bra.w   _strprint\n"                /* -4: stprint2 */
        "   .globl  _strprint\n"
        "_strprint:\n"                          /*  0: = stprint2 */
        "   movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _stprint2_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n"
        "1: movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _stprint3_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n");

/* wstr - the value d2 of type (popped) as Print would output it */
static ULONG wstr(ULONG d2, const UBYTE *args)
{
    const UBYTE *sp = args;

    switch (BL_POPW(sp)) {
    case 4:
        return pquick2(d2);
    case 3:
    case 6:
        return plong2(d2);
    case 2:
        return pword2(d2);
    case 5:
        return pfloat2(d2);
    default:                                    /* also strings */
        return pbyte2(d2);
    }
}

/* Str$(any) - redirect the output to strprint, print, restore */
ULONG BLITZ_ROUTINE dostrstr(ULONG v BL_REG(d0), const UBYTE *args BL_REG(a2))
{
    struct bl_regs r = { .a0 = (ULONG)strprint };
    ULONG old, len;

    old = BLITZ_ALIBJSR(TOK_setout, r);
    len = wstr(v, args);
    r.a0 = old;
    BLITZ_ALIBJSR(TOK_setout, r);
    return len;
}

/* UStr$(any) - Str$ without Format */
ULONG BLITZ_ROUTINE doustrstr(ULONG v BL_REG(d0), const UBYTE *args BL_REG(a2))
{
    UWORD save = pl.formlen;
    ULONG len;

    pl.formlen = 0;
    len = dostrstr(v, args);
    pl.formlen = save;
    return len;
}

/* ------------------------------------------- error check (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess1, "Format String too long");

/* formatchk - CMP.l #32,-4(a2):BHI err1 */
static void BLITZ_ERRCHK formatchk_c(const ULONG *lens BL_REG(a2))
{
    if (lens[-1] > 32)
        BLITZ_RUNERR(mess1);
}
BLITZ_ERRCHK_THUNK(formatchk, formatchk_c, "d0-d1/a0-a1");
