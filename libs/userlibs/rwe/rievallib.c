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
 *     end of the string its 0, and the scan goes on behind it (#631).  An
 *     operand character that is no digit, a-f, register, `$ % . { }`
 *     ends the operand and is dropped (#632).  A `-` is a sign only at the
 *     start of the string or right after an operator or `(` - not after
 *     a space ("3 -4" is 3-4) (#633).
 *   - PFEvaluate:
 *     - `<` is "less or same" unsigned (SLS), `>` signed (SGT) (#638).
 *     - `*` (lmult) adds the high words of the partial products into the
 *       low word (SWAP:ADD) - wrong only when the product overflows
 *       (#639).
 *     - `^`: x^0 and x^1 are x, 0^n is 1; the loop counts with DBRA on
 *       the low word of exponent-2 (#641).
 *     - The value count is a byte compared signed: 128 or more values on
 *       the stack make every operator fail with "Need 2 arguments"
 *       (#645).
 *     - Only the last value is the result; more values are no error
 *       (#635).
 *     - A string of spaces only: "No result value!" (error 4), and the
 *       result is the long above the empty stack, the caller's return
 *       address (MOVE.l (a7)+,d0) (#644).
 *     - A register operand (`d0`, `a7`, `pc`) takes the next character
 *       unchecked (`a` + 0 skips the string's end), `a`/`d` with any
 *       character index the register block (index byte * 4) (#642).  In
 *       `{...}` the close bracket check looks at d1, the last character a
 *       number read - for a register d1 is stale (the caller's d1, or what
 *       the last number, `*`, `/` or `^` left), so `{d0}` mostly fails
 *       with error 5; if it passes, the `}` is then read as a number 0
 *       (#637).
 *     - `-$10`, `-%1`: the sign is taken first, then `$`/`%` is read as
 *       a decimal digit (#643).  Digits are not checked (`12x` = 1, 2,
 *       'x'-'0') (#634).
 *     - The indirection reads a long at any address (an odd one is an
 *       address error on the 68000) (#636).
 *   - PFErrorText with no error returns no string length: d0's upper word
 *     is the caller's (MOVE.w) (#640).
 * Like the original, no defects: ConvToPostFix leaves a3 (the Blitz
 * string pointer) = the string (a statement: nothing uses a3 after it);
 * PFErrorType leaves the caller's upper word of d0 (MOVE.w; a word
 * result).
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
    UBYTE *pos, *out, *start;   /* a0, a1, a3: string, output, its start */
    ULONG acc, aux, prec;       /* d0, d1, d7: registers carrying state */
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
        if (s->pos == s->start)                 /* start of the string */
            goto notoperator;
        if (s->pos[1] == '(')
            goto checkoperator;
        prevch = s->pos[-1];
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
            c = *s->pos++;
            BL_SETB(s->acc, c);
        } else if (c != 'a' && c != 'd' && c != 'p' && c != 'c' && c != '$'
                   && c != '%' && c != '{' && c != '}'
                   && !((BYTE)c >= 'a' && (BYTE)c <= 'f')
                   && !((BYTE)c >= '0' && (BYTE)c <= '9'))
            break;                              /* 'endeval: c dropped */
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
 * after an operator or `(` - not after a space (BUG(orig) #633).
 * BUG(orig): a `.` copies the next character unchecked (#631); an operand
 * character that is no digit, a-f, register, `$ % . { }` ends the operand
 * and is dropped (#632).  a3 is left at the string (a statement: nothing
 * uses it after it).
 * @param str  d0: the expression (changed: lower case)
 * @param buf  d1: the output buffer
 * @return nothing
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
    s.ret = 0;
    blitz_string = str;                         /* MOVE.l d0,a3, not restored */
    ctp_loop(&s);
}

/* --------------------------------------------------------- PFEvaluate */

/* lmult: d0 = d0 * d1, d1 = |d1| swapped (the original's registers) */
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
    hi_lo = BL_SWAP(hi_lo);                     /* BUG(orig) (#639): its high word too */
    lhs += hi_lo;
    *rhs = BL_SWAP(*rhs);
    lo_hi = bl_mulu_w((UWORD)lo_hi, (UWORD)*rhs);
    lo_hi = BL_SWAP(lo_hi);
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

/* the indirection after a {number}: d0 = the long at d0 (lastaddr),
   .b/.w/.l behind the "}"; 0 = ok, else the error result */
static int ev_indirect(struct state *s)
{
    UBYTE c;

    if ((UBYTE)s->aux != '}') {
        ev_error(err_close, 5);
        return 1;
    }
    lastaddr = s->acc;
    s->acc = *(ULONG *)s->acc;
    BL_SETB(s->aux, *s->pos);
    if ((UBYTE)s->aux != '.')
        return 0;
    s->pos++;
    c = *s->pos++;
    BL_SETB(s->aux, c);
    if (c == 'b') {
        s->acc = (s->acc << 8 | s->acc >> 24) & 0xff;
        return 0;
    }
    if (c == 'w') {
        s->acc = BL_SWAP(s->acc) & 0xffff;
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
    const struct op *op_at;             /* a2 */
    ULONG rhs;                          /* the popped value */
    UBYTE ch;                           /* d0.b: the character */
    UWORD regoff;                       /* d0.w: the register's offset in regs */
    int op;
    UWORD cnt;                          /* d2: the exponent loop count */

evloop:
    if (*s->pos == 0) {                         /* ev_eol */
        if (st_empty(s)) {
            ev_error(err_noresult, 4);
            return s->ret;                      /* BUG(orig) (#644): MOVE.l (a7)+,d0 */
        }
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
    if ((BYTE)argc < 2)                         /* !argcheck */
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
    case 3:                                     /* < : SLS, unsigned <= */
        rhs = st_pop(s);
        s->acc = *st_top(s) <= rhs ? BB_TRUE : BB_FALSE;
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
    default:                                    /* ^ */
        rhs = st_pop(s);                        /* d2 */
        s->acc = *st_top(s);
        if (s->acc == 0)
            s->acc = 1;
        else if ((LONG)(rhs - 2) >= 0) {
            ULONG base = s->acc;
            cnt = (UWORD)(rhs - 2);
            do {                                /* DBRA d2 */
                s->aux = base;
                s->acc = lmult(s->acc, &s->aux);
            } while (cnt-- != 0);
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
    if (*s->pos == '$')
        goto evalhex;
    if (*s->pos == '%')
        goto evalbin;
    negative = *s->pos == '-' ? 0xff : 0;
    if (negative)
        s->pos++;
    if (*s->pos == 'a') {
        regoff = (UBYTE)(s->pos[1] - '0' + 8);
        s->pos += 2;
        goto getreg;
    }
    if (*s->pos == 'd') {
        regoff = (UBYTE)(s->pos[1] - '0');
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
        s->acc = (s->acc << 3) + s->acc + s->acc;
        BL_SETB(s->aux, s->aux - '0');
        s->acc += s->aux;
    }
    goto endofnum;

getreg:
    regoff = (UWORD)(regoff << 2);              /* ANDI.w #$ff:ASL.w #2 */
    if (!regs)
        return ev_error(err_reg, 2);
    s->acc = *(ULONG *)(regs + (WORD)regoff);

endofnum:
    if (negative)
        s->acc = -s->acc;
    if (indirect && ev_indirect(s))
        return 0;
    argc++;                                     /* after the push in the original */
    PUSH(s, s->acc);
    goto evloop;

evalhex:
    negative = 0;
    s->acc = 0;
    s->aux = 0;
    s->pos++;
    while (*s->pos != 0) {
        BL_SETB(s->aux, *s->pos++);
        if ((UBYTE)s->aux == ' ' || (UBYTE)s->aux == '}')
            break;
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
    negative = 0;
    s->acc = 0;
    s->aux = 0;
    s->pos++;
    while (*s->pos != 0) {
        BL_SETB(s->aux, *s->pos++);
        if ((UBYTE)s->aux == ' ' || (UBYTE)s->aux == '}')
            break;
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
 * address).  Operators: = & | < > + - * / ^ (comparisons give -1/0).
 * Errors set PFErrorType/PFErrorText and return 0: 1 "Need 2 arguments",
 * 2 "Register structure not found", 3 "Divide by 0", 4 "No result
 * value", 5 "No close bracket on indirect", 6 "Invalid size for
 * indirection".  An empty string returns 0, error 0.
 * BUG(orig): `<` is unsigned "less or same", `>` signed (#638); `*`
 * overflows wrongly (#639); `^` (#641); 128 or more values (#645); only
 * the last value counts (#635); a string of spaces returns the caller's
 * return address with error 4 (#644); register operands (#642), the `}`
 * check after a register (#637), `-$`/`-%` (#643), unchecked digits
 * (#634), indirection at any address (#636).
 * @param str        d0: the postfix string
 * @param caller_d1  d1: the caller's d1 (the stale d1 of a register in {})
 * @param ret        a1: the caller's return address (the thunk passes it)
 * @return d0: the value, 0 on an error
 */
static ULONG BLITZ_ROUTINE evaluate_c(UBYTE *str BL_REG(d0), ULONG caller_d1 BL_REG(d1),
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
    s.pos = s.out = s.start = str;
    s.acc = (ULONG)str;
    s.aux = caller_d1;
    s.prec = 0;
    s.ret = ret;
    return ev_loop(&s);
}

/* --------------------------------------------------------- the others */

/**
 * @brief PFRegisters *registerblock: the register block of PFEvaluate.
 *
 * Longs d0-d7, a0-a7, pc (index * 4); 0 = none (register operands fail
 * with error 2).
 * @param block  d0: the address of the block
 * @return nothing
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
 * The text at a3 (a3 ends at its 0), d0 = its length.  BUG(orig) (#640):
 * with no error d0.w = 0 and nothing is written, the upper word of d0 is
 * the caller's - no valid string length.
 * @param result  d0: the caller's d0
 * @return d0: the length of the text
 */
static ULONG BLITZ_ROUTINE errortext_c(ULONG result BL_REG(d0))
{
    const char *p;

    BL_SETW(result, errortype);
    if (!errortype)
        return result;
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
 * @return nothing
 */
void BLITZ_ROUTINE _libinit(void)
{
}

/**
 * @brief finit: nothing (RTS).
 * @return nothing
 */
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
