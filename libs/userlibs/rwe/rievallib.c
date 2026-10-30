/*
 * rievallib - RWE userlib 121 (Reflective Images, RI Eval Lib V2.5):
 * ConvToPostFix, PFEvaluate, PFRegisters, PFErrorType, PFErrorText,
 * PFIndirectAddr - integer expressions in strings.
 *
 * C translation of rwelibsrc/EvalLib4.bb brought to the state of the
 * reference, BSS2 RIEVALLIB.OBJ (newer: `{}` indirection with .b/.w/.l,
 * PFIndirectAddr, error 4 for "No result value!"; docs/rwelibs/README.md
 * 3.2).  Variant rwe-1999-eval: 121 is the acidlibs' polylib in the
 * other variants.  Notes: docs/rwelibs/port/rievallib.md.
 *
 * ConvToPostFix lower-cases the string in place, then converts the infix
 * expression to postfix in `buffer` (operands and operators separated
 * by spaces) with an operator stack; PFEvaluate evaluates such a string
 * with a value stack.  The original keeps both stacks on the 68000
 * stack (a7, reset from `stack` on errors) and counts the values in a
 * byte; the C build keeps them in chunks of CHUNK longs on the C stack,
 * a new chunk in a new frame when the ones there are full (grow()), so
 * the depth has no limit besides the stack, as in the original.
 * Registers that carry state from one operand to the next (d0's upper
 * bytes, d1 = the last character an operand read, d7 = the precedence
 * of the last operator) are fields of the state, at register level.
 *
 * Assembler: the thunks (PFEvaluate's passes the caller's return
 * address, see "No result value" below); everything else is C.
 *
 * BUG(orig), kept:
 *   - ConvToPostFix: a `.` copies the next character unchecked - at the
 *     end of the string its 0, and the scan goes on behind it.  An
 *     operand character that is no digit, a-f, register, `$ % . { }`
 *     ends the operand and is dropped.  A `-` is a sign only at the
 *     start of the string or right after an operator or `(` - not after
 *     a space ("3 -4" is 3-4).
 *   - ConvToPostFix leaves a3 (the Blitz string pointer) = the string.
 *   - PFEvaluate:
 *     - `<` is "less or same" unsigned (SLS), `>` signed (SGT).
 *     - `*` (lmult) adds the high words of the partial products into the
 *       low word (SWAP:ADD) - wrong only when the product overflows.
 *     - `^`: x^0 and x^1 are x, 0^n is 1; the loop counts with DBRA on
 *       the low word of exponent-2.
 *     - The value count is a byte compared signed: 128 or more values on
 *       the stack make every operator fail with "Need 2 arguments".
 *     - Only the last value is the result; more values are no error.
 *     - A string of spaces only: "No result value!" (error 4), and the
 *       result is the long above the empty stack, the caller's return
 *       address (MOVE.l (a7)+,d0).
 *     - A register operand (`d0`, `a7`, `pc`) takes the next character
 *       unchecked (`a` + 0 skips the string's end), `a`/`d` with any
 *       character index the register block (index byte * 4).  In `{...}`
 *       the close bracket check looks at d1, the last character a number
 *       read - for a register d1 is stale (the caller's d1, or what the
 *       last number, `*`, `/` or `^` left), so `{d0}` mostly fails with
 *       error 5; if it passes, the `}` is then read as a number 0.
 *     - `-$10`, `-%1`: the sign is taken first, then `$`/`%` is read as
 *       a decimal digit.  Digits are not checked (`12x` = 1, 2, 'x'-'0').
 *     - The indirection reads a long at any address (an odd one is an
 *       address error on the 68000).
 *   - PFErrorType/PFErrorText: d0's upper word is the caller's (MOVE.w);
 *     PFErrorText with no error returns no string length.
 */
#include "blitzlib.h"

/* entry points (thunks, below) */
void _ConvToPostFix(void), _evaluate(void), _registers(void), _PFErrorType(void);
void _PFErrorText(void), _PFIndirectAddr(void), _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#evallib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_evallib, init, BL_NORET, finit, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_ConvToPostFix, 0, 0);
BLITZ_NAME("ConvToPostFix", "string,buffer");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_evaluate, 0, 0);
BLITZ_NAME("PFEvaluate", "(string)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_registers, 0, 0);
BLITZ_NAME("PFRegisters", "*registerblock - pointer to registers");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_PFErrorType, 0, 0);
BLITZ_NAME("PFErrorType", "- returns error number or 0 for none");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_PFErrorText, 0, 0);
BLITZ_NAME("PFErrorText", "- returns error text corresponding to error");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_PFIndirectAddr, 0, 0);
BLITZ_NAME("PFIndirectAddr", "- returns last indirect lookup");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------ the data */

/* operators: the character and its precedence; "(" first (ConvToPostFix
   only - PFEvaluate searches from operators+2), Dc.b 0,0 at the end */
static const UBYTE operators[] = {
    '(', 0, '=', 1, '&', 1, '|', 1, '<', 2, '>', 2,
    '+', 3, '-', 3, '*', 4, '/', 4, '^', 5, 0, 0,
};

static const char err_arg[] = "Need 2 arguments for expression!";
static const char err_reg[] = "Register structure not found!";
static const char err_noresult[] = "No result value!";
static const char err_divide0[] = "Divide by 0 error!";
static const char err_close[] = "No close bracket on indirect!";
static const char err_size[] = "Invalid size for indirection!";

static ULONG lastaddr;                  /* PFIndirectAddr */
static UBYTE negative, argc, indirect;  /* argc: compared signed (CMPI.b) */
static ULONG regs;                      /* PFRegisters */
static UWORD errortype;
static const char *errortext;

/* --------------------------------------------------------- the stacks */

/* The original's a7: values (longs) pushed and popped.  Chunks of CHUNK
   values; a chunk lives in the frame of grow() that made it (the first
   in the routine's), and stays until the routine returns, so pops can go
   down into the chunks below and pushes up again into those above. */
#define CHUNK 32

struct chunk {
    ULONG v[CHUNK];
    struct chunk *below, *above;
};

struct state {
    struct chunk *c, *bottom;   /* the chunk of the top value, the first */
    int n;                      /* values in c */
    ULONG (*loop)(struct state *);
    UBYTE *a0, *a1, *a3;        /* string, output, string start */
    ULONG d0, d1, d7;           /* registers carrying state */
    ULONG ret;                  /* the caller's return address */
};

static void st_init(struct state *s, struct chunk *k)
{
    k->below = k->above = 0;
    s->c = s->bottom = k;
    s->n = 0;
}

/* CMP.l stack,a7 / CMP.l a7,a2: nothing on the stack */
static int st_empty(const struct state *s)
{
    return s->c == s->bottom && s->n == 0;
}

/* MOVE.l x,-(a7) within the chunks there; 0: a new one is needed */
static int st_push(struct state *s, ULONG x)
{
    if (s->n == CHUNK) {
        if (!s->c->above)
            return 0;
        s->c = s->c->above;
        s->n = 0;
    }
    s->c->v[s->n++] = x;
    return 1;
}

/* (a7), the stack not empty */
static ULONG *st_top(struct state *s)
{
    if (s->n == 0) {
        s->c = s->c->below;
        s->n = CHUNK;
    }
    return &s->c->v[s->n - 1];
}

/* MOVE.l (a7)+,x, the stack not empty */
static ULONG st_pop(struct state *s)
{
    ULONG x = *st_top(s);
    s->n--;
    return x;
}

/* the push of x with a new chunk in this frame, then the rest of the
   routine (every push of the original is followed by BRA to the main
   loop); no sibling call: the chunk must outlive it */
static ULONG __attribute__((noinline, optimize("no-optimize-sibling-calls")))
grow(struct state *s, ULONG x)
{
    struct chunk k;
    ULONG r;

    k.below = s->c;
    k.above = 0;
    s->c->above = &k;
    s->c = &k;
    s->n = 0;
    k.v[s->n++] = x;
    r = s->loop(s);
    return r;
}

#define PUSH(s, x) do { if (!st_push((s), (x))) return grow((s), (x)); } while (0)

/* ------------------------------------------------------ ConvToPostFix */

/* is c (a byte) an operator of the table, "(" included */
static int is_operator(UBYTE c)
{
    const UBYTE *p;

    for (p = operators; *p; p += 2)
        if (*p == c)
            return 1;
    return 0;
}

/* the loop of ConvToPostFix from 'loop: a0 = the string, a1 the output;
   the operator stack holds d0 with the character in its low byte */
static ULONG ctp_loop(struct state *s)
{
    const UBYTE *a2;
    UBYTE d2;

loop:
    if (*s->a0 == 0)
        goto endthis;
    if (*s->a0++ == ' ')
        goto loop;
    s->a0--;
    BL_SETB(s->d0, *s->a0);

    /* check the special operators - ( ) */
    if ((UBYTE)s->d0 == '-') {
        if (s->a0 == s->a3)                     /* start of the string */
            goto notoperator;
        if (s->a0[1] == '(')
            goto checkoperator;
        BL_SETB(s->d1, s->a0[-1]);
        if (is_operator((UBYTE)s->d1))          /* - is for negative */
            goto notoperator;
        goto checkoperator;
    }
    if ((UBYTE)s->d0 == '(') {
        s->a0++;
        PUSH(s, s->d0);
        goto loop;
    }
    if ((UBYTE)s->d0 == ')') {
        s->a0++;
        while (!st_empty(s)) {                  /* 'unstackloop */
            s->d0 = st_pop(s);
            if ((UBYTE)s->d0 == '(')
                break;
            *s->a1++ = (UBYTE)s->d0;
            *s->a1++ = ' ';
        }
        goto loop;
    }

checkoperator:
    for (a2 = operators; *a2; a2 += 2)
        if (*a2 == (UBYTE)s->d0) {
            s->a0++;
            BL_SETB(s->d7, a2[1]);
            goto operator;
        }

notoperator:                                    /* a number or register */
    if ((UBYTE)s->d0 == '-') {
        *s->a1++ = (UBYTE)s->d0;
        s->a0++;
    }
    for (;;) {                                  /* 'evall */
        UBYTE c = *s->a0++;

        BL_SETB(s->d0, c);
        if (c == 0 || is_operator(c) || c == ')') {
            s->a0--;                            /* 'endeval2 */
            break;
        }
        if (c == ' ')
            break;
        if (c == '.') {                         /* and the next one, unchecked */
            *s->a1++ = c;
            c = *s->a0++;
            BL_SETB(s->d0, c);
        } else if (c != 'a' && c != 'd' && c != 'p' && c != 'c' && c != '$'
                   && c != '%' && c != '{' && c != '}'
                   && !((BYTE)c >= 'a' && (BYTE)c <= 'f')
                   && !((BYTE)c >= '0' && (BYTE)c <= '9'))
            break;                              /* 'endeval: c dropped */
        *s->a1++ = c;                           /* 'putchar */
    }
    *s->a1++ = ' ';                             /* 'endeval */
    goto loop;

operator:
    /* unstack the operators of higher or equal precedence */
    while (!st_empty(s)) {
        s->d1 = *st_top(s);
        for (a2 = operators; *a2++ != (UBYTE)s->d1; a2++)
            ;
        d2 = *a2;
        if ((BYTE)s->d7 > (BYTE)d2)
            break;
        st_pop(s);
        if ((UBYTE)s->d1 == '(')                /* never: "(" has 0 */
            goto loop;
        *s->a1++ = (UBYTE)s->d1;
        *s->a1++ = ' ';
    }
    PUSH(s, s->d0);                             /* 'stacktoken */
    goto loop;

endthis:
    while (!st_empty(s)) {
        s->d0 = st_pop(s);
        if ((UBYTE)s->d0 == '(')
            continue;
        *s->a1++ = (UBYTE)s->d0;
        *s->a1++ = ' ';
    }
    *s->a1 = 0;
    return 0;
}

/* ConvToPostFix string,buffer (d0, d1) */
static void BLITZ_ROUTINE convtopostfix_c(UBYTE *str BL_REG(d0), UBYTE *buf BL_REG(d1))
{
    struct state s;
    struct chunk k;
    UBYTE *p = str, c;

    while ((c = *p++) != 0)                     /* lower case, in place */
        if ((BYTE)c >= 'A' && (BYTE)c <= 'Z')
            p[-1] = c | 32;
    st_init(&s, &k);
    s.loop = ctp_loop;
    s.a0 = s.a3 = str;
    s.a1 = buf;
    s.d0 = (ULONG)str;
    s.d1 = (ULONG)buf;
    s.d7 = 0;                                   /* set by every operator first */
    s.ret = 0;
    blitz_string = str;                         /* MOVE.l d0,a3, not restored */
    ctp_loop(&s);
}

/* --------------------------------------------------------- PFEvaluate */

/* lmult: d0 = d0 * d1, d1 = |d1| swapped (the original's registers) */
static ULONG lmult(ULONG d0, ULONG *d1)
{
    ULONG d2 = 0, d3, d4;

    if ((LONG)d0 < 0) {
        d0 = -d0;
        d2 = 0xffffffffUL;
    }
    if ((LONG)*d1 < 0) {
        *d1 = -*d1;
        d2 ^= 0xffff;                           /* NOT d2 (.w) */
    }
    d3 = d4 = d0;
    d0 = bl_mulu_w((UWORD)d0, (UWORD)*d1);      /* MULU d1,d0 */
    d3 = BL_SWAP(d3);
    d3 = bl_mulu_w((UWORD)d3, (UWORD)*d1);
    d3 = BL_SWAP(d3);                           /* BUG(orig): its high word too */
    d0 += d3;
    *d1 = BL_SWAP(*d1);
    d4 = bl_mulu_w((UWORD)d4, (UWORD)*d1);
    d4 = BL_SWAP(d4);
    d0 += d4;
    if ((UWORD)d2)
        d0 = -d0;
    return d0;
}

/* ldivide: d0 = d0 / d1 (d1 not 0 here), bit by bit as the original;
   d1 = what its shifts leave */
static ULONG ldivide(ULONG d0, ULONG *d1)
{
    ULONG d2 = 0, d3 = 0, d4 = 0, x;
    int d5;

    if (*d1 == 0)
        return 0x7fffffffUL;                    /* 'infinity */
    if ((LONG)*d1 < 0) {
        *d1 = -*d1;
        d2 = 0xffffffffUL;
    }
    if ((LONG)d0 < 0) {
        d0 = -d0;
        d2 ^= 0xffff;
    }
    for (d5 = 31; d5 >= 0; d5--) {              /* DBF d5 */
        d4 <<= 1;
        x = *d1 & 1;                            /* LSR.l #1,d1: X */
        *d1 >>= 1;
        d3 = d3 >> 1 | x << 31;                 /* ROXR.l #1,d3 */
        if (*d1 != 0)
            continue;
        if (d3 != 0 && d0 >= d3) {              /* 'skip6 */
            d0 -= d3;
            d4 |= 1;                            /* ADDQ #1,d4 on LSL'd d4 */
        }
    }
    if ((UWORD)d2)
        d4 = -d4;
    return d4;
}

/* the error exits: errortext, errortype, d0 = 0 */
static ULONG ev_error(const char *text, UWORD type)
{
    errortext = text;
    errortype = type;
    return 0;
}

/* the indirection after a {number}: d0 = the long at d0 (lastaddr),
   .b/.w/.l behind the "}"; 0 = ok, else the error result */
static int ev_indirect(struct state *s)
{
    UBYTE c;

    if ((UBYTE)s->d1 != '}') {
        ev_error(err_close, 5);
        return 1;
    }
    lastaddr = s->d0;
    s->d0 = *(ULONG *)s->d0;
    BL_SETB(s->d1, *s->a0);
    if ((UBYTE)s->d1 != '.')
        return 0;
    s->a0++;
    c = *s->a0++;
    BL_SETB(s->d1, c);
    if (c == 'b') {
        s->d0 = (s->d0 << 8 | s->d0 >> 24) & 0xff;
        return 0;
    }
    if (c == 'w') {
        s->d0 = BL_SWAP(s->d0) & 0xffff;
        return 0;
    }
    if (c == 'l')
        return 0;
    ev_error(err_size, 6);
    return 1;
}

/* the loop of PFEvaluate from evloop */
static ULONG ev_loop(struct state *s)
{
    const UBYTE *a2;
    ULONG x;
    int op;
    UWORD cnt;

evloop:
    if (*s->a0 == 0) {                          /* ev_eol */
        if (st_empty(s)) {
            ev_error(err_noresult, 4);
            return s->ret;                      /* BUG(orig): MOVE.l (a7)+,d0 */
        }
        return st_pop(s);
    }
    if (*s->a0++ == ' ')
        goto evloop;
    s->a0--;
    BL_SETB(s->d0, *s->a0);
    if ((UBYTE)s->d0 == '-' && s->a0[1] != ' ' && s->a0[1] != 0)
        goto evall;
    for (a2 = operators + 2, op = 0; *a2; a2 += 2, op++)
        if (*a2 == (UBYTE)s->d0)
            goto gotop;
    goto evall;

gotop:
    if ((BYTE)argc < 2)                         /* !argcheck */
        return ev_error(err_arg, 1);
    switch (op) {
    case 0:                                     /* = */
        x = st_pop(s);
        s->d0 = x == *st_top(s) ? 0xffffffffUL : 0;
        break;
    case 1:                                     /* & */
        s->d0 = st_pop(s) & *st_top(s);
        break;
    case 2:                                     /* | */
        s->d0 = st_pop(s) | *st_top(s);
        break;
    case 3:                                     /* < : SLS, unsigned <= */
        x = st_pop(s);
        s->d0 = *st_top(s) <= x ? 0xffffffffUL : 0;
        break;
    case 4:                                     /* > : SGT */
        x = st_pop(s);
        s->d0 = (LONG)*st_top(s) > (LONG)x ? 0xffffffffUL : 0;
        break;
    case 5:                                     /* + */
        s->d0 = st_pop(s) + *st_top(s);
        break;
    case 6:                                     /* - */
        x = st_pop(s);
        s->d0 = *st_top(s) - x;
        break;
    case 7:                                     /* * */
        s->d0 = st_pop(s);
        s->d1 = *st_top(s);
        s->d0 = lmult(s->d0, &s->d1);
        break;
    case 8:                                     /* / */
        s->d1 = st_pop(s);
        if (s->d1 == 0)
            return ev_error(err_divide0, 3);
        s->d0 = ldivide(*st_top(s), &s->d1);
        break;
    default:                                    /* ^ */
        x = st_pop(s);                          /* d2 */
        s->d0 = *st_top(s);
        if (s->d0 == 0)
            s->d0 = 1;
        else if ((LONG)(x -= 2) >= 0) {
            ULONG d3 = s->d0;
            cnt = (UWORD)x;
            do {                                /* DBRA d2 */
                s->d1 = d3;
                s->d0 = lmult(s->d0, &s->d1);
            } while (cnt-- != 0);
        }
        break;
    }
    *st_top(s) = s->d0;                         /* !putarg */
    s->a0++;
    argc--;
    goto evloop;

evall:
    indirect = 0;
    if (*s->a0 == '{') {
        indirect = 0xff;
        s->a0++;
    }
    if (*s->a0 == '$')
        goto evalhex;
    if (*s->a0 == '%')
        goto evalbin;
    negative = *s->a0 == '-' ? 0xff : 0;
    if (negative)
        s->a0++;
    if (*s->a0 == 'a') {
        BL_SETB(s->d0, s->a0[1] - '0' + 8);
        s->a0 += 2;
        goto getreg;
    }
    if (*s->a0 == 'd') {
        BL_SETB(s->d0, s->a0[1] - '0');
        s->a0 += 2;
        goto getreg;
    }
    if (*s->a0 == 'p' && s->a0[1] == 'c') {
        s->d0 = 16;
        s->a0 += 2;
        goto getreg;
    }
    /* the number, decimal */
    s->d0 = 0;
    s->d1 = 0;
    while (*s->a0 != 0) {
        BL_SETB(s->d1, *s->a0++);
        if ((UBYTE)s->d1 == ' ' || (UBYTE)s->d1 == '}')
            break;
        s->d0 = (s->d0 << 3) + s->d0 + s->d0;
        BL_SETB(s->d1, s->d1 - '0');
        s->d0 += s->d1;
    }
    goto endofnum;

getreg:
    BL_SETW(s->d0, ((UBYTE)s->d0) << 2);        /* ANDI.w #$ff:ASL.w #2 */
    if (!regs)
        return ev_error(err_reg, 2);
    s->d0 = *(ULONG *)(regs + (WORD)s->d0);

endofnum:
    if (negative)
        s->d0 = -s->d0;
    if (indirect && ev_indirect(s))
        return 0;
    argc++;                                     /* after the push in the original */
    PUSH(s, s->d0);
    goto evloop;

evalhex:
    negative = 0;
    s->d0 = 0;
    s->d1 = 0;
    s->a0++;
    while (*s->a0 != 0) {
        BL_SETB(s->d1, *s->a0++);
        if ((UBYTE)s->d1 == ' ' || (UBYTE)s->d1 == '}')
            break;
        s->d0 <<= 4;
        if ((BYTE)s->d1 < 'a') {
            BL_SETB(s->d1, s->d1 - '0');
            s->d0 += s->d1;
        } else {
            BL_SETB(s->d1, s->d1 - 'a' + 10);
            s->d0 |= (UBYTE)s->d1;
        }
    }
    goto endofnum;

evalbin:
    negative = 0;
    s->d0 = 0;
    s->d1 = 0;
    s->a0++;
    while (*s->a0 != 0) {
        BL_SETB(s->d1, *s->a0++);
        if ((UBYTE)s->d1 == ' ' || (UBYTE)s->d1 == '}')
            break;
        s->d0 <<= 1;
        BL_SETB(s->d1, s->d1 - '0');
        s->d0 |= (UBYTE)s->d1;
    }
    goto endofnum;
}

/* PFEvaluate(string): d0 = the string, d1 = the caller's d1 (the stale
   d1 of a register in {}), ret = the caller's return address (thunk) */
static ULONG BLITZ_ROUTINE evaluate_c(UBYTE *str BL_REG(d0), ULONG d1 BL_REG(d1),
                                      ULONG ret BL_REG(a1))
{
    struct state s;
    struct chunk k;

    argc = 0;
    errortype = 0;
    if (*str == 0)
        return 0;
    st_init(&s, &k);
    s.loop = ev_loop;
    s.a0 = s.a1 = s.a3 = str;
    s.d0 = (ULONG)str;
    s.d1 = d1;
    s.d7 = 0;
    s.ret = ret;
    return ev_loop(&s);
}

/* --------------------------------------------------------- the others */

/* PFRegisters *registerblock */
static void BLITZ_ROUTINE registers_c(ULONG block BL_REG(d0))
{
    regs = block;
}

/* PFErrorType: MOVE.w errortype,d0 */
static ULONG BLITZ_ROUTINE errortype_c(ULONG d0 BL_REG(d0))
{
    return BL_SETW(d0, errortype);
}

/* PFErrorText: the text at a3 (a3 ends at its 0), d0 = its length; no
   error: d0.w = 0, nothing written */
static ULONG BLITZ_ROUTINE errortext_c(ULONG d0 BL_REG(d0))
{
    const char *p;

    BL_SETW(d0, errortype);
    if (!errortype)
        return d0;
    p = errortext;
    d0 = (ULONG)-1;
    do
        d0++;
    while ((*blitz_string++ = *p++) != 0);
    blitz_string--;
    return d0;
}

/* PFIndirectAddr: the address of the last indirection */
static ULONG BLITZ_ROUTINE indirectaddr_c(void)
{
    return lastaddr;
}

/* _libinit, _libfinit: RTS */
void BLITZ_ROUTINE _libinit(void)
{
}

void BLITZ_ROUTINE _libfinit(void)
{
}

/* the thunks keep d1/a0/a1 (and d0 where the original does); PFEvaluate
   passes the caller's return address in a1 (above the 12 saved bytes) */
BLITZ_THUNK(_ConvToPostFix, convtopostfix_c, "d0-d1/a0-a1");
BLITZ_THUNK_PRE(_evaluate, evaluate_c, "d1/a0-a1", "move.l  12(sp),a1");
BLITZ_THUNK(_registers, registers_c, "d0-d1/a0-a1");
BLITZ_THUNK(_PFErrorType, errortype_c, "d1/a0-a1");
BLITZ_THUNK(_PFErrorText, errortext_c, "d1/a0-a1");
BLITZ_THUNK(_PFIndirectAddr, indirectaddr_c, "d1/a0-a1");
