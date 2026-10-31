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
 * give "?" and padding (see below); rounding at the last decimal.
 *
 * BUG(orig), kept (for the comparison with AmiBlitz3):
 *   - Quick: a fraction that rounds to 10000 (x.99995 and more) loses the
 *     carry: Print 2.99999 -> "2." (four digits "0000" stripped to ".")
 *     (#233); a fraction that rounds to 0 leaves the point: 1+1/65536 ->
 *     "1.", 1/65536 -> "." (#234).  Not a bug (the output style of Blitz 2,
 *     the same for Quick and Float): numbers below 1 have no leading 0
 *     (".5").
 *   - Not a bug (as above): Float, fixed point: no leading 0 either
 *     (".5"); 0 is "0".
 *   - Format: rounding that carries out of the first digit is lost
 *     (999.96 with "###.#" -> "000.0") (#235); numbers without an integer
 *     digit (|x| < 1, d3 = -(integer digits) = 0) are never rounded,
 *     whatever the format (0.567 with ".##" or "###.##" -> ".56") (#236);
 *     the sign is lost when the format has no "-"/"+" (-7 with "###" ->
 *     "  7") (#237); too many integer digits give "?" and then only
 *     padding for the whole integer part (1234.5 with "##" -> "?  ")
 *     (#238).
 *   - Format with more than 32 characters (no check without the
 *     debugger): the copy runs over formlen/formlead/formtrail/decpos and
 *     further into the conversion buffers (the layout of the variables
 *     below is that of the original so that this goes the same way). (#240)
 *   - ffptoascii, fixed point with FloatMode -1 (or a Format string) and
 *     |x| < 1E-17: the digits are written past the end of the 50 byte
 *     buffer into the first entry (1E19) of the table of powers of ten,
 *     which changes later conversions of numbers >= 1E18. (#241)
 *   - Str$ of a string is printed as a byte (the low byte of the address). (#242)
 * Kept from the original, not a bug (no effect):
 *   - strprint, the handler of Str$, has the same code for single
 *     characters (JSR (a6)) as for strings (a0, d0) - inputoutputlib's
 *     dooutsingle would copy d0 characters from a0.  printlib never
 *     outputs single characters to it.
 *   - Str$ installs its handler with dosetout twice (install and
 *     restore), so every Str$ pushes two entries on inputoutputlib's
 *     output stack (a ring of 16, harmless).
 *   - the Format round-up loop starts at the last copied digit (d1); d1 = 0
 *     ("address -1") would need a rounding without a copied integer digit,
 *     which does not occur (rounding needs d3 != 0 = at least one integer
 *     digit). (#239)
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
#include <stddef.h>

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
   behind `form` (BUG(orig) (#240), see above), and ezchar reads the byte before
   `chars` ($20) for a "," at the start.  All accesses that may leave a
   member go through pointers into the whole struct. */
static struct BL_PACKED printvars {
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
static_assert(sizeof pl == 0x234 - 0x1c6);

#define PL_BASE   ((UBYTE *)&pl)
#define PL_FORM   (PL_BASE + offsetof(struct printvars, form))
#define PL_CHARS  (PL_BASE + offsetof(struct printvars, chars))
#define PL_NUMBUFF (PL_BASE + offsetof(struct printvars, numbuff))
#define PL_NUMBUFFF (PL_BASE + offsetof(struct printvars, numbufff))

/* ffptoascii's buffers and table, in the order of the binary ($872-$9D3):
   buf1 = sign, digits and exponent form, buf2 = fixed point form (the
   point at buf2[24]), then the 48 powers of ten {binary exponent,
   mantissa} 1E19 ... 1E-28 - writable, see BUG(orig) (#241) above. */
static struct BL_PACKED ffpvars {
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

static_assert(sizeof fa == 0x9d4 - 0x872);

#define FA_BASE  ((UBYTE *)&fa)
#define FA_BUF1  (FA_BASE + offsetof(struct ffpvars, buf1))
#define FA_BUF2  (FA_BASE + offsetof(struct ffpvars, buf2))
#define FA_ONE   (FA_BASE + offsetof(struct ffpvars, tab[19]))  /* table entry 1E0 */

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
    UBYTE *out;         /* a1: output (chars) */
    UBYTE *form;        /* a2: format string */
    UWORD left;         /* d6: characters left - 1 */
    UWORD minus;        /* d2 */
};

static int ezchar(struct ez *z)
{
    for (;;) {
        if ((WORD)--z->left < 0)                /* SUBQ #1,d6:BMI 'noch */
            return 1;
        switch (*z->form) {
        case '#':
        case '0':
            return 0;
        case '-':                               /* 'dominus */
            *z->out++ = (WORD)z->minus >= 0 ? '-' : ' ';
            z->form++;                          /* ezchar2 */
            break;
        case '+':                               /* 'doplus */
            *z->out = '+';
            if ((WORD)z->minus >= 0)
                *z->out = '-';
            z->out++;                           /* ezchar3 / MOVE.b (a1)+ */
            z->form++;
            break;
        case '.':                               /* 'dopoint */
            *z->out++ = '.';
            z->form++;
            break;
        case ',':                               /* 'docomma */
            if (z->out[-1] == ' ') {            /* 'ezspc */
                *z->out++ = ' ';
                z->form++;
                break;
            }
            /* fall through */
        default:
            *z->out++ = *z->form++;
            break;
        }
    }
}

/* prtype1 - output the number a0 (d0 characters), formatted with the
   Format string if there is one.  Word arithmetic as in the original. */
static ULONG prtype1(const UBYTE *numtext, ULONG len)
{
    struct ez z;
    UWORD nchars, point, trail, n;
    UWORD ldigits;                              /* d3: the leading digits */
    UWORD lpad;                                 /* d3: leading spaces/zeros to write */
    UWORD pos;                                  /* d5: the position counter */
    UWORD over;                                 /* d5: the overflow */
    UWORD count;                                /* d5: the lead/trail loop counter */
    ULONG last = 0;                             /* after the last copied digit */
    UBYTE *out;
    const UBYTE *p;

    if (!pl.formlen)                            /* MOVE formlen(pc),d1:BEQ 'done */
        return output(numtext, len);

    nchars = len;
    point = 0xffff;                             /* pos of point */
    z.minus = 0xffff;                           /* pos of minus */
    if (*numtext == '-') {
        z.minus = 0;
        numtext++;
        nchars--;
    }
    ldigits = 0;                                /* leading digits */
    trail = 0;                                  /* trailing digits */
    pos = 0;                                    /* pos counter */
    n = nchars - 1;
    p = numtext;
    do {
        if (*p == '.')
            point = pos;
        else {
            ldigits++;
            if ((WORD)point >= 0) {
                ldigits--;
                trail++;
            }
        }
        p++;
        pos++;
    } while (--n != 0xffff);                    /* DBF d6,'loop */

    /* OK, now we copy to 'chars' buffer. */
    out = PL_CHARS;
    z.form = PL_FORM;
    z.left = pl.formlen;

    /* Leading chars first.... */
    lpad = pl.formlead - ldigits;
    if ((WORD)lpad < 0) {                       /* Overflow! */
        *out++ = '?';
        over = -lpad;
        numtext += (WORD)over;                  /* ADD d5,a0 (sign extended) */
        nchars -= over;
        lpad = pl.formlead;
    }
    count = pl.formlead;
    if (count) {
        count--;
        do {                                    /* 'leadloop */
            z.out = out;
            if (ezchar(&z))
                goto alldone_z;
            out = z.out;
            if (*z.form++ == '#') {
                if ((WORD)--lpad >= 0 || (WORD)--nchars < 0)
                    *out++ = ' ';               /* 'putspc */
                else {
                    *out++ = *numtext++;
                    last = (ULONG)out;
                }
            } else {                            /* 'iszero */
                if ((WORD)--lpad >= 0)
                    *out++ = '0';               /* 'putzero */
                else if ((WORD)--nchars < 0)
                    *out++ = ' ';               /* -> 'putspc */
                else {
                    *out++ = *numtext++;
                    last = (ULONG)out;
                }
            }
        } while (--count != 0xffff);            /* DBF d5,'leadloop */
    }

    /* 'dotrail */
    if ((WORD)--nchars >= 0) {
        if (*numtext++ == '.')
            goto askip;
        numtext--;
    }
    nchars++;                                   /* 'askip2 */
askip:
    count = pl.formtrail;
    z.out = out;
    if (count) {
        count--;
        do {                                    /* 'trailloop */
            if (ezchar(&z))
                goto alldone2;
            if (*z.form++ == '#') {
                if ((WORD)--trail < 0 || (WORD)--nchars < 0)
                    *z.out++ = ' ';             /* 'putspc2 */
                else {
                    *z.out++ = *numtext++;
                    last = (ULONG)z.out;
                }
            } else {
                if ((WORD)--trail < 0 || (WORD)--nchars < 0)
                    *z.out++ = '0';             /* 'putzero2 */
                else {
                    *z.out++ = *numtext++;
                    last = (ULONG)z.out;
                }
            }
        } while (--count != 0xffff);
    }
    while (!ezchar(&z))                         /* 'doexcess: BSR ezchar:BEQ */
        ;
alldone2:
    /* round up if the next digit is >= 5 (TST.l d3: the upper word of d3
       is always 0) */
    if ((WORD)--nchars >= 0 && lpad && *numtext >= '5') {
        UBYTE *q = (UBYTE *)last;               /* 0 if no digit was copied */

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
    out = z.out;
    return output(PL_CHARS, out - PL_CHARS);    /* SUB.l a0,a1:MOVE.l a1,d0 */
}

/* prtype2 - a0 = buffer, null terminated */
static ULONG prtype2(const UBYTE *buf)
{
    const UBYTE *p = buf;
    ULONG n = (ULONG)-1;

    do
        n++;
    while (*p++);
    return prtype1(buf, n);
}

/* doformat - Format FormatString: copy the string to form, count the
   digit places before and after the point.  Length from the a2 stack
   (MOVE.l -(a2),d1), only its low word is used. */
static void BLITZ_ROUTINE doformat_c(const UBYTE *s BL_REG(d0),
                                     const UBYTE *args BL_REG(a2))
{
    const UBYTE *sp = args;
    UBYTE *out = PL_FORM;
    UWORD n = BL_POPL(sp), point, lead, trail, count;

    pl.formlen = n;
    if (!n)
        return;
    n--;
    point = 0xffff;                             /* pos of decimal point */
    lead = 0;                                   /* number of leading chars */
    trail = 0;                                  /* number of trailing... */
    count = 0;                                  /* counter (never counted) */
    do {
        UBYTE c = *s++;

        *out++ = c;                             /* may run over formlen ... */
        if (c == '.')
            point = count;
        else if (c == '#' || c == '0') {
            lead++;
            if ((WORD)point >= 0) {
                lead--;
                trail++;
            }
        }
    } while (--n != 0xffff);
    /* MOVE d2,decpos:MOVE d3,formlead:MOVE d4,formtrail (through the
       struct: the loop may have written over them) */
    *(volatile UWORD *)&pl.decpos = point;
    *(volatile UWORD *)&pl.formlead = lead;
    *(volatile UWORD *)&pl.formtrail = trail;
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
static UBYTE *makeval3(UBYTE *dst, ULONG *value, UWORD n)
{
    ULONG v = *value;

    do {
        v = bl_divu_w(v, 10);                   /* DIVU #10,d0 */
        v = BL_SWAP(v);
        BL_SETW(v, (UWORD)v | 48);              /* OR #48,d0 */
        *--dst = (UBYTE)v;
        v = BL_SWAP(v & 0xffff0000);            /* CLR d0:SWAP d0 */
    } while (--n != 0xffff);
    *value = v;
    return dst;
}

/* makeval2 - makeval3 without leading zeros ("0" if all are zero; the
   string behind the digits ends the scan) */
static UBYTE *makeval2(UBYTE *dst, ULONG value, UWORD n)
{
    dst = makeval3(dst, &value, n);
    while (*dst++ == '0')
        ;
    if (!*--dst)
        *--dst = '0';
    return dst;
}

/* makeval - signed d0 with at most d1+1 digits, then output */
static ULONG makeval(ULONG value, UWORD n)
{
    UBYTE *dst = PL_NUMBUFFF;

    if ((LONG)value < 0) {
        dst = makeval2(dst, -value, n);
        *--dst = '-';
        return prtype2(dst);
    }
    return prtype2(makeval2(dst, value, n));
}

static const ULONG divisors[10] = {
    1, 10, 100, 1000, 10000, 100000,
    1000000, 10000000, 100000000, 1000000000,
};

/* makelong - decimal of a long to numbuff (-$80000000 stays negative
   after NEG and is printed unsigned: "-2147483648") */
static void makelong(ULONG value)
{
    UBYTE *dst = PL_NUMBUFF;
    int started = 0;
    WORD i;

    if ((LONG)value < 0) {
        *dst++ = '-';
        value = -value;
    }
    for (i = 9; i >= 0; i--) {
        UBYTE ch = '0';

        while (value >= divisors[i]) {          /* CMP.l:BCS (unsigned) */
            value -= divisors[i];
            ch++;
        }
        if (ch != '0' || started) {
            *dst++ = ch;
            started = 1;
        }
    }
    if (!started)
        *dst++ = '0';
    *dst = 0;
}

static ULONG plong2(ULONG value)
{
    makelong(value);
    return prtype2(PL_NUMBUFF);
}

static ULONG pbyte2(ULONG value)
{
    return makeval((LONG)(BYTE)value, 2);       /* EXT d0:EXT.l d0 */
}

static ULONG pword2(ULONG value)
{
    return makeval((LONG)(WORD)value, 4);       /* EXT.l d0 */
}

/* 'quick3 - d2 = what to print (not negative, except $80000000): fraction
   "." + 4 digits with trailing zeros removed, then the integer part */
static UBYTE *quick3(ULONG value)
{
    UBYTE *dst = PL_NUMBUFFF;
    ULONG frac = (UWORD)value;

    if (frac) {
        UBYTE *p;

        frac = (ULONG)(UWORD)frac * 10000;      /* MULU #10000,d0 */
        if ((UWORD)frac >= 0x8000)              /* CMP #$8000,d1:BCS */
            frac = (frac >> 16) + 1;
        else
            frac >>= 16;
        dst = makeval3(PL_NUMBUFFF, &frac, 3);
        *--dst = '.';
        p = PL_NUMBUFFF;
        /* 'f0l: the "." test compares the same '0' again, so it never
           stops at the point (BUG(orig) (#234): "x." for .0000) */
        while (p[-1] == '0' && *--p != '.')
            ;
        *p = 0;                                 /* 'skip2 */
    }
    return makeval2(dst, value >> 16, 4);       /* 'skip3 */
}

static ULONG pquick2(ULONG value)
{
    ULONG quick = value;
    UBYTE *str;

    if ((LONG)quick < 0) {
        str = quick3(-quick);
        *--str = '-';
    } else
        str = quick3(quick);
    return prtype2(str);
}

/* ------------------------------------------------------- ffptoascii */

/* ffptoascii (libincs:ffptoascii, from the binary) - FFP number d0 to a
   null terminated string; mode = FloatMode (<0 fixed point, 0 guess, >0
   exponent form).  Integer arithmetic only (no mathffp): 7 digits from
   the table of powers of ten, rounding with $80 below the mantissa. */
static UBYTE *ffptoascii(ULONG value, UWORD mode)
{
    UBYTE *buf = FA_BUF1, *out, *start;
    UBYTE *ten;
    UBYTE b;
    WORD bexp;
    UWORD dexp;
    UWORD eabs;                                 /* d2: |exponent| for "E+nn" */
    UWORD etens;                                /* d2: the exponent's tens digit */
    ULONG mant;                                 /* d0: the mantissa being reduced */
    int i;

    if (!(UBYTE)value)                          /* TST.b d0:BNE */
        value = 65;                             /* MOVEQ #65,d0 */
    buf[0] = '+';                               /* MOVE #$2b2e,0(a0) */
    buf[1] = '.';
    b = (UBYTE)value;
    if (b & 0x80)
        buf[0] += 2;                            /* '-' */
    b = (UBYTE)(b + b) ^ 0x80;                  /* ADD.b d1,d1:EOR.b #$80 */
    mant = (value & ~0xffUL) | 0x80;            /* MOVE.b #$80,d0 */
    bexp = (WORD)(BYTE)b >> 1;                  /* EXT d1:ASR #1,d1 */

    /* find the power of ten <= the number: a3 -> entry, d3 = exponent+1 */
    dexp = 1;
    ten = FA_ONE;
    if (bexp < (WORD)RDW(ten)) {
        do {
            ten += 6;
            dexp--;
        } while (bexp < (WORD)RDW(ten));
        if (bexp != (WORD)RDW(ten))
            goto found;
    } else if (bexp > (WORD)RDW(ten)) {
        do {
            ten -= 6;
            dexp++;
        } while (bexp > (WORD)RDW(ten));
        if (bexp != (WORD)RDW(ten))
            goto next;
    }
    if (mant >= RDL(ten + 2))                   /* CMP.l 2(a3),d0:BCC */
        goto found;
next:
    ten += 6;
    dexp--;
found:
    /* "E+00" (computed, never used) */
    buf[10] = 'E';
    buf[11] = '+';
    buf[12] = '0';
    buf[13] = '0';
    eabs = dexp;
    if ((WORD)eabs < 0) {
        eabs = -eabs;
        buf[11] += 2;
    }
    if (eabs >= 10) {
        buf[12] += 1;
        eabs -= 10;
    }
    buf[13] |= (UBYTE)eabs;

    /* 7 digits: subtract the power of ten, shifted to the number's
       exponent, until it borrows */
    out = buf + 2;
    if ((LONG)mant >= 0 || !ten[5])             /* exact power: no rounding */
        mant &= ~0xffUL;
    for (i = 0; i < 7; i++) {
        UWORD sh = bexp - RDW(ten);
        ULONG power = RDL(ten + 2);
        UWORD count = 9;
        int borrow;

        ten += 6;
        sh &= 63;                               /* LSR.l d4,d5: count mod 64 */
        power = sh >= 32 ? 0 : power >> sh;
        for (;;) {                              /* SUB.l d5,d0:DBCS d4 */
            borrow = mant < power;
            mant -= power;
            if (borrow || --count == 0xffff)
                break;
        }
        if (!borrow)
            count &= 0xff00;                    /* CLR.b d4 */
        mant += power;
        *out++ = (UBYTE)-(UBYTE)((UBYTE)count - 9) | '0';
    }
    *out++ = '0';

    if ((WORD)mode < 0
        || (mode == 0 && (dexp < 8 || dexp >= 0xfff8))) {
        /* fixed point */
        UBYTE *fixbuf = FA_BUF2;

        fixbuf[25] = '0';
        for (i = 0; i < 24; i++) {
            fixbuf[i] = '0';
            fixbuf[26 + i] = '0';
        }
        fixbuf[24] = '.';
        buf = fixbuf + 24;
        if ((WORD)dexp < 0)
            buf++;
        buf -= (WORD)dexp;                      /* SUBA.w d3,a0 */
        out = FA_BUF1 + 2;
        start = buf;
        for (i = 0; i < 8; i++) {
            if (*buf == '.')
                buf++;
            *buf++ = *out++;
        }
        if (buf >= fixbuf + 25) {               /* CMPA.l #$89b,a0:BCS */
            for (;;) {                          /* strip trailing zeros */
                if (*--buf != '0')
                    break;
                *buf = 0;
            }
            if (*buf != '.') {
                *++buf = 0;
                goto sign;
            }
            if (!(buf > start)) {               /* 0: "0" without sign */
                buf[0] = '0';
                buf[1] = 0;
                return buf;
            }
        }
        fixbuf[24] = 0;                         /* integer only */
sign:
        buf = start;
        if (buf >= fixbuf + 24)                 /* nothing before the point */
            buf = fixbuf + 24;
        *--buf = FA_BUF1[0];
        if (*buf != '-')
            buf++;
        return buf;
    }

    /* exponent form: d.dddddd without trailing zeros, E+-n */
    buf = FA_BUF1;
    buf[1] = buf[2];
    buf[2] = '.';
    while (*--out == '0')
        ;
    if (*out++ == '.') {
        out--;
        if (buf[1] == '0')
            goto term;
    }
    dexp--;
    *out++ = 'E';
    *out++ = '+';
    if ((WORD)dexp < 0) {
        out[-1] = '-';
        dexp = -dexp;
    }
    {
        ULONG q = bl_divu_w(dexp, 10);          /* MOVEQ #0,d2:MOVE d3,d2:DIVU */

        etens = (UWORD)q;
        if (etens)
            *out++ = etens + '0';
        dexp -= etens * 10;
        *out++ = dexp + '0';
    }
term:
    *out = 0;
    if (*buf != '-')
        buf++;
    return buf;
}

/* pfloat2 - FloatMode, or always fixed point with a Format string */
static ULONG pfloat2(ULONG value)
{
    UWORD mode = pl.floatmode;

    if (pl.formlen)
        mode = 0xffff;
    return prtype2(ffptoascii(value, mode));
}

/* ------------------------------------------------------- Print, NPrint */

/* ptype - one argument of type t, value(s) from the a2 stack */
static ULONG ptype(UWORD type, const UBYTE **sp)
{
    switch (type) {
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
        UWORD type = BL_POPW(sp);

        ptype(type, &sp);
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
   character - in the original the same code as stprint2; not a bug, never
   used for single characters).
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
static ULONG wstr(ULONG value, const UBYTE *args)
{
    const UBYTE *sp = args;

    switch (BL_POPW(sp)) {
    case 4:
        return pquick2(value);
    case 3:
    case 6:
        return plong2(value);
    case 2:
        return pword2(value);
    case 5:
        return pfloat2(value);
    default:                                    /* also strings */
        return pbyte2(value);
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
