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
 * Assembler: the thunks; everything else is C.
 *
 * Fixed (intended deviations, docs/libs/deviations.md):
 *   - PFEvaluate counts its values in a long: the original's byte,
 *     compared signed, made every operator fail with "Need 2 arguments"
 *     from 128 values on (#645).
 *   - PFEvaluate of a string of spaces only returns 0 with error 4 "No
 *     result value!"; the original returned the long above its empty
 *     stack, the caller's return address (MOVE.l (a7)+,d0) (#644).
 *   - PFErrorText without an error returns the length 0; the original set
 *     only d0.w (MOVE.w), the upper word was the caller's (#640).
 *   - PFEvaluate's `<` is a signed "less than" (the manual: "Test for
 *     less than"), the mirror image of `>`; the original's was unsigned
 *     "lower or same" (SLS) (#638).
 *   - PFEvaluate takes the sign before `$` and `%`: `-$10` is -16, `-%1`
 *     -1; the original tested `$`/`%` first and read them behind a `-` as
 *     decimal digits (-24410, -2451) (#643).
 *   - ConvToPostFix skips spaces in front of a `-` before it decides: a
 *     sign behind an operator, `(` or only spaces ("4* -2", " -5"), a
 *     binary minus behind an operand ("3 -4" is 3-4).  The original
 *     looked at the character right before it only, so a space made it a
 *     binary minus without an operand (#633).
 *   - ConvToPostFix copies a `.` with the character behind it, but not
 *     the string's 0: a `.` at the end ends the operand.  The original
 *     copied the 0 too and went on behind the string (#631).
 *   - PFEvaluate's `*` (lmult, also for `^`) gives the low 32 bits of the
 *     product: the cross products' low words are added into the high
 *     word only.  The original's SWAP:ADD added their high words into the
 *     low word too, wrong whenever the product overflows (#639).
 *   - PFEvaluate: a register in `{...}` (`{d0}`) is followed by its `}`,
 *     which the register path checks and consumes itself.  The original
 *     checked d1, the last character a number loop read - for a register
 *     stale (the caller's d1, or what the last number, `*`, `/` or `^`
 *     left): `{d0}` mostly failed with error 5, and if it passed, the `}`
 *     was then read as a number 0 (#637).
 *   - PFEvaluate: the indirection reads the size behind the `}` first and
 *     then that many bytes one by one (`.b` a byte, `.w` a word, `.l` or
 *     none a long, the same values): no word or long access at an odd
 *     address.  The original always read a long and took its top byte
 *     or word - an address error on the 68000 at an odd address (#636).
 *   - PFEvaluate checks the digits of a number: a character that is no
 *     digit of its base (decimal 0-9, `$` hex 0-9 a-f, `%` binary 0 1)
 *     is error 4 "No result value!" with the result 0.  The original
 *     took every character up to the next space as a digit (`12x` = 192,
 *     `%2` = 2) (#634).
 *   - PFEvaluate: a register operand is `a` or `d` with a digit 0-7, or
 *     `pc`, followed by a space or the end (in `{...}` by its `}`); any
 *     other is error 4 "No result value!" with the result 0.  The
 *     original took the character behind `a`/`d` unchecked as the index
 *     into the register block (`d9`, `dx`; behind a final `d` the
 *     string's 0, and it went on behind the string) and read a digit
 *     behind a register as a number of its own (`d01` = d0, 1) (#642).
 *   - PFEvaluate: more than one value left at the end of the string is
 *     error 4 "No result value!" with the result 0.  The original
 *     returned the top value without an error (`1 2` = 2, `1 2 + 3` = 3)
 *     (#635).
 *   - ConvToPostFix keeps every character of an operand up to a space,
 *     an operator, `)` or the end, and PFEvaluate rejects one that is no
 *     part of a number or register (error 4).  The original ended the
 *     operand at a character that is no digit, a-f, register,
 *     `$ % . { }` and dropped it: `1,2` became `1 2 `, `12x+3`
 *     `12 3 + ` (15) (#632).
 *   - PFEvaluate's `^` is the integer power: x^0 = 1, 0^n = 0 for n > 0,
 *     the low 32 bits of the power by square and multiply over all 32
 *     bits of the exponent; a negative exponent gives the truncated value
 *     (0; 1 for the base 1; 1 or -1 for the base -1, by the exponent's
 *     parity), 0^-n is error 3 "Divide by 0 error!".  The original
 *     returned 1 for the base 0, the base for the exponents 1, 0 and
 *     every negative one, and counted with DBRA on the low word of
 *     exponent-2 (`3 65538 ^` = 9) (#641).
 * Like the original, no defects: ConvToPostFix leaves a3 (the Blitz
 * string pointer) = the string (a statement: nothing uses a3 after it);
 * PFErrorType leaves the caller's upper word of d0 (MOVE.w; a word
 * result).
 */
#include "blitzlib.h"

/* entry points (thunks, below) */
void _ConvToPostFix(void), _evaluate(void), _registers(void);
void _PFErrorType(void), _PFErrorText(void), _PFIndirectAddr(void);
static void _libinit(void);
static void _libfinit(void);

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
   only - PFEvaluate searches from operators+2, the entry after it), Dc.b
   0,0 at the end */
struct op {
    UBYTE ch, prec;
};
static_assert(sizeof(struct op) == 2);
static const struct op operators[] = {
    { '(', 0 }, { '=', 1 }, { '&', 1 }, { '|', 1 }, { '<', 2 }, { '>', 2 },
    { '+', 3 }, { '-', 3 }, { '*', 4 }, { '/', 4 }, { '^', 5 }, { 0, 0 },
};

static const char err_arg[] = "Need 2 arguments for expression!";
static const char err_reg[] = "Register structure not found!";
static const char err_noresult[] = "No result value!";
static const char err_divide0[] = "Divide by 0 error!";
static const char err_close[] = "No close bracket on indirect!";
static const char err_size[] = "Invalid size for indirection!";

static ULONG lastaddr;                  /* PFIndirectAddr */
static UBYTE negative, indirect;
static ULONG argc;                      /* the values on the stack */
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
    UBYTE *pos, *out, *start;   /* a0, a1, a3: string, output, its start */
    ULONG acc, aux, prec;       /* d0, d1, d7: registers carrying state */
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
    const struct op *p;

    for (p = operators; p->ch; p++)
        if (p->ch == c)
            return 1;
    return 0;
}

/* the loop of ConvToPostFix from 'loop: a0 = the string, a1 the output;
   the operator stack holds d0 with the character in its low byte */
static ULONG ctp_loop(struct state *s)
{
    const struct op *op;                /* a2: the table entry */
    UBYTE opprec;                       /* d2: its precedence */
    UBYTE prevch;                       /* d1.b: the character before a "-" */
    UBYTE top;                          /* d1.b: the operator on the stack top */

loop:
    if (*s->pos == 0)
        goto endthis;
    if (*s->pos++ == ' ')
        goto loop;
    s->pos--;
    BL_SETB(s->acc, *s->pos);

    /* check the special operators - ( ) */
    if ((UBYTE)s->acc == '-') {
        UBYTE *before = s->pos;

        while (before != s->start && before[-1] == ' ')
            before--;                           /* fixed (#633): spaces skipped */
        if (before == s->start)                 /* start of the string */
            goto notoperator;
        if (s->pos[1] == '(')
            goto checkoperator;
        prevch = before[-1];
        if (is_operator(prevch))                /* - is for negative */
            goto notoperator;
        goto checkoperator;
    }
    if ((UBYTE)s->acc == '(') {
        s->pos++;
        PUSH(s, s->acc);
        goto loop;
    }
    if ((UBYTE)s->acc == ')') {
        s->pos++;
        while (!st_empty(s)) {                  /* 'unstackloop */
            s->acc = st_pop(s);
            if ((UBYTE)s->acc == '(')
                break;
            *s->out++ = (UBYTE)s->acc;
            *s->out++ = ' ';
        }
        goto loop;
    }

checkoperator:
    for (op = operators; op->ch; op++)
        if (op->ch == (UBYTE)s->acc) {
            s->pos++;
            BL_SETB(s->prec, op->prec);
            goto operator;
        }

notoperator:                                    /* a number or register */
    if ((UBYTE)s->acc == '-') {
        *s->out++ = (UBYTE)s->acc;
        s->pos++;
    }
    for (;;) {                                  /* 'evall */
        UBYTE c = *s->pos++;

        BL_SETB(s->acc, c);
        if (c == 0 || is_operator(c) || c == ')') {
            s->pos--;                           /* 'endeval2 */
            break;
        }
        if (c == ' ')
            break;
        if (c == '.') {                         /* and the next one, unchecked */
            *s->out++ = c;
            if (*s->pos == 0)                   /* fixed (#631): but not the 0 */
                break;
            c = *s->pos++;
            BL_SETB(s->acc, c);
        }
        /* fixed (#632): any other character is copied too (the original
           ended the operand at one that is no digit, a-f, register,
           `$ % { }` and dropped it); PFEvaluate rejects it (#634, #642) */
        *s->out++ = c;                          /* 'putchar */
    }
    *s->out++ = ' ';                            /* 'endeval */
    goto loop;

operator:
    /* unstack the operators of higher or equal precedence */
    while (!st_empty(s)) {
        top = (UBYTE)*st_top(s);
        for (op = operators; op->ch != top; op++)
            ;
        opprec = op->prec;
        if ((BYTE)s->prec > (BYTE)opprec)
            break;
        st_pop(s);
        if (top == '(')                         /* never: "(" has 0 */
            goto loop;
        *s->out++ = top;
        *s->out++ = ' ';
    }
    PUSH(s, s->acc);                            /* 'stacktoken */
    goto loop;

endthis:
    while (!st_empty(s)) {
        s->acc = st_pop(s);
        if ((UBYTE)s->acc == '(')
            continue;
        *s->out++ = (UBYTE)s->acc;
        *s->out++ = ' ';
    }
    *s->out = 0;
    return 0;
}

/**
 * @brief ConvToPostFix string,buffer: converts an infix expression to postfix.
 *
 * Lower-cases the string in place, then writes the operands and
 * operators separated by spaces to the buffer (0 terminated), with an
 * operator stack; "(" ... ")" group.  A `-` is a sign at the start or
 * after an operator or `(`, spaces between skipped (fixed, #633).
 * A `.` copies the character behind it, not the string's 0 (fixed, #631).
 * Every other character of an operand is kept, for PFEvaluate to reject
 * (fixed, #632).  a3 is left at the
 * string (a statement: nothing uses it after it).
 * @param str  d0: the expression (changed: lower case)
 * @param buf  d1: the output buffer
 */
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
    s.pos = s.start = str;
    s.out = buf;
    s.acc = (ULONG)str;
    s.aux = (ULONG)buf;
    s.prec = 0;                                 /* set by every operator first */
    blitz_string = str;                         /* MOVE.l d0,a3, not restored */
    ctp_loop(&s);
}

/* --------------------------------------------------------- PFEvaluate */

/* lmult: d0 = the low 32 bits of d0 * d1, d1 = |d1| swapped (the
   original's registers) */
static ULONG lmult(ULONG lhs, ULONG *rhs)
{
    ULONG sign = 0, hi_lo, lo_hi;       /* d2-d4 */

    if ((LONG)lhs < 0) {
        lhs = -lhs;
        sign = 0xffffffffUL;
    }
    if ((LONG)*rhs < 0) {
        *rhs = -*rhs;
        sign ^= 0xffff;                         /* NOT d2 (.w) */
    }
    hi_lo = lo_hi = lhs;
    lhs = bl_mulu_w((UWORD)lhs, (UWORD)*rhs);   /* MULU d1,d0 */
    hi_lo = BL_SWAP(hi_lo);
    hi_lo = bl_mulu_w((UWORD)hi_lo, (UWORD)*rhs);
    hi_lo = BL_SWAP(hi_lo) & 0xffff0000UL;      /* fixed (#639): CLR.w, not its high word */
    lhs += hi_lo;
    *rhs = BL_SWAP(*rhs);
    lo_hi = bl_mulu_w((UWORD)lo_hi, (UWORD)*rhs);
    lo_hi = BL_SWAP(lo_hi) & 0xffff0000UL;      /* fixed (#639) */
    lhs += lo_hi;
    if ((UWORD)sign)
        lhs = -lhs;
    return lhs;
}

/* ldivide: d0 = d0 / d1 (d1 not 0 here), bit by bit as the original;
   d1 = what its shifts leave */
static ULONG ldivide(ULONG dividend, ULONG *divisor)
{
    ULONG sign = 0, shifted = 0, quotient = 0, carry; /* d2-d4, X */
    int bit;                            /* d5 */

    if (*divisor == 0)
        return 0x7fffffffUL;                    /* 'infinity */
    if ((LONG)*divisor < 0) {
        *divisor = -*divisor;
        sign = 0xffffffffUL;
    }
    if ((LONG)dividend < 0) {
        dividend = -dividend;
        sign ^= 0xffff;
    }
    for (bit = 31; bit >= 0; bit--) {           /* DBF d5 */
        quotient <<= 1;
        carry = *divisor & 1;                   /* LSR.l #1,d1: X */
        *divisor >>= 1;
        shifted = shifted >> 1 | carry << 31;   /* ROXR.l #1,d3 */
        if (*divisor != 0)
            continue;
        if (shifted != 0 && dividend >= shifted) { /* 'skip6 */
            dividend -= shifted;
            quotient |= 1;                      /* ADDQ #1,d4 on LSL'd d4 */
        }
    }
    if ((UWORD)sign)
        quotient = -quotient;
    return quotient;
}

/* the error exits: errortext, errortype, d0 = 0 */
static ULONG ev_error(const char *text, UWORD type)
{
    errortext = text;
    errortype = type;
    return 0;
}

/* the indirection after a {number}: d0 = the byte, word or long at d0
   (lastaddr), .b/.w/.l behind the "}" (none: a long); 0 = ok, else the
   error result.  Fixed (#636): the size is read first, then that many
   bytes one by one - no word or long access at an odd address (an
   address error on the 68000); the original read a long and took its
   top byte or word */
static int ev_indirect(struct state *s)
{
    const UBYTE *at;
    ULONG value = 0;
    int size = 4;
    UBYTE c;

    if ((UBYTE)s->aux != '}') {
        ev_error(err_close, 5);
        return 1;
    }
    lastaddr = s->acc;
    at = (const UBYTE *)s->acc;
    BL_SETB(s->aux, *s->pos);
    if ((UBYTE)s->aux == '.') {
        s->pos++;
        c = *s->pos++;
        BL_SETB(s->aux, c);
        if (c == 'b')
            size = 1;
        else if (c == 'w')
            size = 2;
        else if (c != 'l') {
            ev_error(err_size, 6);
            return 1;
        }
    }
    while (size--)
        value = value << 8 | *at++;
    s->acc = value;
    return 0;
}

/* the loop of PFEvaluate from evloop */
static ULONG ev_loop(struct state *s)
{
    const struct op *op_at;             /* a2 */
    ULONG rhs;                          /* the popped value */
    UBYTE ch;                           /* d0.b: the character */
    UWORD regoff;                       /* d0.w: the register's offset in regs */
    int op;
    ULONG base;                         /* ^: the base, squared */

evloop:
    if (*s->pos == 0) {                         /* ev_eol */
        if (st_empty(s))                        /* fixed (#644): 0, not (a7)+ */
            return ev_error(err_noresult, 4);
        if (argc > 1)                           /* fixed (#635): values left */
            return ev_error(err_noresult, 4);
        return st_pop(s);
    }
    if (*s->pos++ == ' ')
        goto evloop;
    s->pos--;
    ch = *s->pos;
    if (ch == '-' && s->pos[1] != ' ' && s->pos[1] != 0)
        goto evall;
    for (op_at = operators + 1, op = 0; op_at->ch; op_at++, op++)
        if (op_at->ch == ch)
            goto gotop;
    goto evall;

gotop:
    if (argc < 2)                               /* !argcheck; fixed (#645) */
        return ev_error(err_arg, 1);
    switch (op) {
    case 0:                                     /* = */
        rhs = st_pop(s);
        s->acc = rhs == *st_top(s) ? BB_TRUE : BB_FALSE;
        break;
    case 1:                                     /* & */
        s->acc = st_pop(s) & *st_top(s);
        break;
    case 2:                                     /* | */
        s->acc = st_pop(s) | *st_top(s);
        break;
    case 3:                                     /* < : signed; fixed (#638) */
        rhs = st_pop(s);
        s->acc = (LONG)*st_top(s) < (LONG)rhs ? BB_TRUE : BB_FALSE;
        break;
    case 4:                                     /* > : SGT */
        rhs = st_pop(s);
        s->acc = (LONG)*st_top(s) > (LONG)rhs ? BB_TRUE : BB_FALSE;
        break;
    case 5:                                     /* + */
        s->acc = st_pop(s) + *st_top(s);
        break;
    case 6:                                     /* - */
        rhs = st_pop(s);
        s->acc = *st_top(s) - rhs;
        break;
    case 7:                                     /* * */
        s->acc = st_pop(s);
        s->aux = *st_top(s);
        s->acc = lmult(s->acc, &s->aux);
        break;
    case 8:                                     /* / */
        s->aux = st_pop(s);
        if (s->aux == 0)
            return ev_error(err_divide0, 3);
        s->acc = ldivide(*st_top(s), &s->aux);
        break;
    default:                                    /* ^, fixed (#641) */
        rhs = st_pop(s);                        /* the exponent */
        base = *st_top(s);
        if ((LONG)rhs < 0) {                    /* the truncated value */
            if (base == 0)
                return ev_error(err_divide0, 3);
            if (base == 1 || base == (ULONG)-1)
                s->acc = (rhs & 1) ? base : 1;
            else
                s->acc = 0;
            break;
        }
        s->acc = 1;                             /* x^0 = 1 */
        while (rhs) {                           /* square and multiply */
            if (rhs & 1) {
                s->aux = base;
                s->acc = lmult(s->acc, &s->aux);
            }
            rhs >>= 1;
            if (rhs) {
                s->aux = base;
                base = lmult(base, &s->aux);
            }
        }
        break;
    }
    *st_top(s) = s->acc;                        /* !putarg */
    s->pos++;
    argc--;
    goto evloop;

evall:
    indirect = 0;
    if (*s->pos == '{') {
        indirect = 0xff;
        s->pos++;
    }
    negative = *s->pos == '-' ? 0xff : 0;       /* fixed (#643): the sign first */
    if (negative)
        s->pos++;
    if (*s->pos == '$')
        goto evalhex;
    if (*s->pos == '%')
        goto evalbin;
    if (*s->pos == 'a' || *s->pos == 'd') {     /* fixed (#642): 0-7 only */
        if (s->pos[1] < '0' || s->pos[1] > '7')
            return ev_error(err_noresult, 4);
        regoff = (UBYTE)(s->pos[1] - '0' + (*s->pos == 'a' ? 8 : 0));
        s->pos += 2;
        goto getreg;
    }
    if (*s->pos == 'p' && s->pos[1] == 'c') {
        regoff = 16;
        s->pos += 2;
        goto getreg;
    }
    /* the number, decimal */
    s->acc = 0;
    s->aux = 0;
    while (*s->pos != 0) {
        BL_SETB(s->aux, *s->pos++);
        if ((UBYTE)s->aux == ' ' || (UBYTE)s->aux == '}')
            break;
        if ((UBYTE)s->aux < '0' || (UBYTE)s->aux > '9')
            return ev_error(err_noresult, 4);   /* fixed (#634): no digit */
        s->acc = (s->acc << 3) + s->acc + s->acc;
        BL_SETB(s->aux, s->aux - '0');
        s->acc += s->aux;
    }
    goto endofnum;

getreg:
    if (!indirect && *s->pos != ' ' && *s->pos != 0)
        return ev_error(err_noresult, 4);       /* fixed (#642): "d01" */
    regoff = (UWORD)(regoff << 2);              /* ANDI.w #$ff:ASL.w #2 */
    if (!regs)
        return ev_error(err_reg, 2);
    s->acc = *(ULONG *)(regs + (WORD)regoff);
    if (indirect) {                             /* fixed (#637): the "}" behind it */
        BL_SETB(s->aux, *s->pos);
        if (*s->pos == '}')
            s->pos++;
    }

endofnum:
    if (negative)
        s->acc = -s->acc;
    if (indirect && ev_indirect(s))
        return 0;
    argc++;                                     /* after the push in the original */
    PUSH(s, s->acc);
    goto evloop;

evalhex:
    s->acc = 0;
    s->aux = 0;
    s->pos++;
    while (*s->pos != 0) {
        BL_SETB(s->aux, *s->pos++);
        if ((UBYTE)s->aux == ' ' || (UBYTE)s->aux == '}')
            break;
        if (!((UBYTE)s->aux >= '0' && (UBYTE)s->aux <= '9')
            && !((UBYTE)s->aux >= 'a' && (UBYTE)s->aux <= 'f'))
            return ev_error(err_noresult, 4);   /* fixed (#634): no hex digit */
        s->acc <<= 4;
        if ((BYTE)s->aux < 'a') {
            BL_SETB(s->aux, s->aux - '0');
            s->acc += s->aux;
        } else {
            BL_SETB(s->aux, s->aux - 'a' + 10);
            s->acc |= (UBYTE)s->aux;
        }
    }
    goto endofnum;

evalbin:
    s->acc = 0;
    s->aux = 0;
    s->pos++;
    while (*s->pos != 0) {
        BL_SETB(s->aux, *s->pos++);
        if ((UBYTE)s->aux == ' ' || (UBYTE)s->aux == '}')
            break;
        if ((UBYTE)s->aux != '0' && (UBYTE)s->aux != '1')
            return ev_error(err_noresult, 4);   /* fixed (#634): no binary digit */
        s->acc <<= 1;
        BL_SETB(s->aux, s->aux - '0');
        s->acc |= (UBYTE)s->aux;
    }
    goto endofnum;
}

/**
 * @brief PFEvaluate(string): evaluates a postfix expression of ConvToPostFix.
 *
 * Operands: decimal, `$` hex, `%` binary numbers, `-` signs, registers
 * `d0`-`d7`, `a0`-`a7`, `pc` of the PFRegisters block, `{...}` an
 * indirection (a long, `.b`/`.w`/`.l` behind it; PFIndirectAddr keeps the
 * address).  Operators: = & | < > + - * / ^ (comparisons give -1/0;
 * `<` and `>` signed, `<` fixed, #638).
 * Errors set PFErrorType/PFErrorText and return 0: 1 "Need 2 arguments",
 * 2 "Register structure not found", 3 "Divide by 0", 4 "No result
 * value", 5 "No close bracket on indirect", 6 "Invalid size for
 * indirection".  An empty string returns 0, error 0; a string of spaces
 * only 0, error 4 (fixed, #644).
 * `*` gives the low 32 bits of the product (fixed, #639); the `}` of a
 * register in `{...}` is checked at the string (fixed, #637); the
 * indirection reads the bytes of its size one by one (fixed, #636).
 * A character that is no digit of the number's base is error 4 (fixed,
 * #634); so is a register other than `a0`-`a7`, `d0`-`d7`, `pc`, or one
 * not followed by a space, the end or its `}` (fixed, #642).
 * More than one value left at the end is error 4 (fixed, #635).
 * `^` is the integer power, a negative exponent the truncated value,
 * 0^-n error 3 (fixed, #641).
 * @param str  d0: the postfix string
 * @return d0: the value, 0 on an error
 */
static ULONG BLITZ_ROUTINE evaluate_c(UBYTE *str BL_REG(d0))
{
    struct state s;
    struct chunk k;

    argc = 0;
    errortype = 0;
    if (*str == 0)
        return 0;
    st_init(&s, &k);
    s.loop = ev_loop;
    s.pos = s.out = s.start = str;
    s.acc = (ULONG)str;
    s.aux = 0;                                  /* read only after it is set */
    s.prec = 0;
    return ev_loop(&s);
}

/* --------------------------------------------------------- the others */

/**
 * @brief PFRegisters *registerblock: the register block of PFEvaluate.
 *
 * Longs d0-d7, a0-a7, pc (index * 4); 0 = none (register operands fail
 * with error 2).
 * @param block  d0: the address of the block
 */
static void BLITZ_ROUTINE registers_c(ULONG block BL_REG(d0))
{
    regs = block;
}

/**
 * @brief PFErrorType: the number of the last PFEvaluate error, 0 for none.
 *
 * MOVE.w errortype,d0: the caller's upper word of d0 stays (a word
 * result, no defect).
 * @param result  d0: the caller's d0
 * @return d0.w: the error number (1-6) or 0
 */
static ULONG BLITZ_ROUTINE errortype_c(ULONG result BL_REG(d0))
{
    return BL_SETW(result, errortype);
}

/**
 * @brief PFErrorText: the text of the last PFEvaluate error.
 *
 * The text at a3 (a3 ends at its 0), d0 = its length.  With no error
 * nothing is written and the length is 0.  Fixed (#640): the original
 * set only d0.w (MOVE.w errortype,d0), the upper word of the length was
 * the caller's.
 * @return d0: the length of the text, 0 without an error
 */
static ULONG BLITZ_ROUTINE errortext_c(void)
{
    const char *p;
    ULONG result;

    if (!errortype)
        return 0;                               /* fixed (#640) */
    p = errortext;
    result = (ULONG)-1;
    do
        result++;
    while ((*blitz_string++ = *p++) != 0);
    blitz_string--;
    return result;
}

/**
 * @brief PFIndirectAddr: the address of the last `{...}` indirection.
 * @return d0: the address
 */
static ULONG BLITZ_ROUTINE indirectaddr_c(void)
{
    return lastaddr;
}

/**
 * @brief init: nothing (RTS).
 */
static void BLITZ_ROUTINE _libinit(void)
{
}

/**
 * @brief finit: nothing (RTS).
 */
static void BLITZ_ROUTINE _libfinit(void)
{
}

/* the thunks keep d1/a0/a1 (and d0 where the original does) */
BLITZ_THUNK(_ConvToPostFix, convtopostfix_c, "d0-d1/a0-a1");
BLITZ_THUNK(_evaluate, evaluate_c, "d1/a0-a1");
BLITZ_THUNK(_registers, registers_c, "d0-d1/a0-a1");
BLITZ_THUNK(_PFErrorType, errortype_c, "d1/a0-a1");
BLITZ_THUNK(_PFErrorText, errortext_c, "d1/a0-a1");
BLITZ_THUNK(_PFIndirectAddr, indirectaddr_c, "d1/a0-a1");
