/*
 * exprtree.c - the nodes of an expression and the walk over them
 * (exprtree.h; issue #1180).  The arena replaces the original's
 * precedence stack (blitz2.s:9171 precsp, 9173 precstack, 9153 pushprec,
 * 9161 popprec); the walk is eval5's control (blitz2.s:8983-9083); which
 * routine went where: docs/blitz2/expression-tree.md.
 */
#include <stddef.h>
#include <string.h>
#include "exprtree.h"

static struct exprnode arena[EXPRTREE_NODES];
static struct exprnode *freep = arena;          /* the first free node */
static struct exprnode *pend = NULL;            /* the pending node on top */
static unsigned npend = 0;                      /* the pending nodes */

/* no asm counterpart; replaces blitz2.s:2824 prepstack's move.l
   #precstack,precsp - every node free, none pending */
void exprtree_reset(void)
{
    freep = arena;
    pend = NULL;
    npend = 0;
}

/* no asm counterpart - a new node */
struct exprnode *exprtree_new(unsigned kind)
{
    struct exprnode *n = freep;

    if (n == arena + EXPRTREE_NODES)
        exprtree_full();
    freep = n + 1;
    memset(n, 0, sizeof *n);
    n->kind = (unsigned char)kind;
    return n;
}

/* no asm counterpart; replaces blitz2.s:9153 pushprec - n pending.  The
   original checked the limit only in popprec, after the write past the
   end of precstack; this stops before (as the port's pushprec did) */
void exprtree_push(struct exprnode *n, unsigned op, unsigned prec)
{
    if (npend >= EXPRTREE_PENDING)
        exprtree_overflow();
    n->op = (unsigned short)op;
    n->prec = (unsigned short)prec;
    n->down = pend;
    pend = n;
    npend++;
}

/* no asm counterpart; replaces the reads of precsp[-1] (blitz2.s:8986
   eval5 .more: cmp -(a1),d1) */
struct exprnode *exprtree_top(void)
{
    return pend;
}

/* no asm counterpart; replaces blitz2.s:9161 popprec and eval4's subq.l
   #6,precsp (blitz2.s:8979) - n and the nodes after it free */
void exprtree_release(struct exprnode *n)
{
    while (pend && pend >= n) {
        pend = pend->down;
        npend--;
    }
    freep = n;
}

/* no asm counterpart - the node after n in use */
struct exprnode *exprtree_after(struct exprnode *n)
{
    return n + 1 < freep ? n + 1 : NULL;
}

/* no asm counterpart - the nodes in use, the pending ones */
unsigned exprtree_live(void)
{
    return (unsigned)(freep - arena);
}

unsigned exprtree_pending(void)
{
    return npend;
}

/* no asm counterpart; replaces blitz2.s:8983-9006 eval5's control (bsr
   eval2; .more: cmp -(a1),d1 / bhi .higher; .higher: bsr pushprec, bsr
   eval5 for the right operand, popprec, bsr doop) - one level of the
   precedence climbing: an operand, then each operator that binds tighter
   than the pending one with its right operand (one level deeper: the
   recursion of the original, once per higher precedence) and its
   emission; a chain of one precedence is the loop.  The hooks do the
   rest of eval5 at the moments it did it. */
unsigned exprtree_climb(void *ctx, unsigned *op)
{
    unsigned prec = exprtree_operand(ctx, op);
    struct exprnode *l = exprtree_new(EXN_LEAF);

    exprtree_left(ctx, l, prec > pend->prec ? *op : pend->op);
    while (prec > pend->prec) {
        struct exprnode *n = exprtree_new(EXN_BINOP);
        unsigned nop;

        n->left = l;
        n->op = (unsigned short)*op;
        n->prec = (unsigned short)prec;
        exprtree_pre(ctx, n);
        exprtree_push(n, *op, prec);
        exprtree_enter(ctx, n);
        prec = exprtree_climb(ctx, &nop);
        n->right = exprtree_after(n);
        exprtree_emit(ctx, n);
        l = n->left;                /* the left operand is a result now */
        l->kind = EXN_RESULT;
        l->start = l->end = 0;
        l->flags &= (unsigned char)~EXF_RAW;
        l->tbefore = n->tafter;
        exprtree_release(n);
        *op = nop;
    }
    exprtree_done(ctx, l);
    return prec;
}
