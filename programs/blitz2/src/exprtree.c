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

/* the alignment of a node: every allocation is a multiple of it */
#define ALIGN (offsetof(struct { char c; struct exprnode n; }, n))
#define ROUND(size) (((size) + ALIGN - 1) & ~(unsigned long)(ALIGN - 1))

/* a further block: the link to the next one, then its space */
struct block {
    struct block *next;
};
#define BLOCKSPACE (ROUND(sizeof(struct block)))

static struct exprnode arena[EXPRTREE_NODES];   /* the static block */
static struct block *blocks = NULL;             /* the further blocks, in order */
static unsigned nblocks = 0;                    /* how many */
static struct block *cur = NULL;                /* the block in use (NULL: the static one) */
static char *freep = (char *)arena;             /* its first free byte */
static char *endp = (char *)(arena + EXPRTREE_NODES);   /* its end */
static struct exprnode *pend = NULL;            /* the pending node on top */
static unsigned npend = 0;                      /* the pending nodes */

/* no asm counterpart; replaces blitz2.s:2824 prepstack's move.l
   #precstack,precsp - every node free, none pending, the further blocks
   given back */
void exprtree_reset(void)
{
    while (blocks) {
        struct block *b = blocks;

        blocks = b->next;
        exprtree_freeblock(b);
    }
    nblocks = 0;
    cur = NULL;
    freep = (char *)arena;
    endp = (char *)(arena + EXPRTREE_NODES);
    pend = NULL;
    npend = 0;
}

/* no asm counterpart - size bytes of the arena: from the block in use, or
   from the next one (one the arena holds already, else a new one from the
   system: none is "Out of Memory", without a retry) */
void *exprtree_alloc(unsigned size)
{
    unsigned long n = ROUND(size);
    char *p;

    if ((unsigned long)(endp - freep) < n) {
        struct block *b = cur ? cur->next : blocks;

        if (!b) {
            struct block **link = &blocks;

            b = exprtree_getblock();
            if (!b)
                exprtree_nomem();
            b->next = NULL;
            while (*link)
                link = &(*link)->next;
            *link = b;
            nblocks++;
        }
        cur = b;
        freep = (char *)b + BLOCKSPACE;
        endp = (char *)b + EXPRTREE_BLOCK;
    }
    p = freep;
    freep += n;
    return p;
}

/* no asm counterpart - a new node */
struct exprnode *exprtree_new(unsigned kind)
{
    struct exprnode *n = exprtree_alloc(sizeof *n);

    memset(n, 0, sizeof *n);
    n->kind = (unsigned char)kind;
    return n;
}

/* no asm counterpart - is p in the block that begins at b and ends at e? */
static int inblock(const void *p, const char *b, const char *e)
{
    return (const char *)p >= b && (const char *)p < e;
}

/* no asm counterpart - the arena free from p on (p in the static block or
   in one of the further ones, which stay with the arena) */
static void freefrom(void *p)
{
    struct block *b;

    if (inblock(p, (const char *)arena, (const char *)(arena + EXPRTREE_NODES))) {
        cur = NULL;
        endp = (char *)(arena + EXPRTREE_NODES);
    } else {
        for (b = blocks; b && !inblock(p, (const char *)b, (const char *)b + EXPRTREE_BLOCK);
             b = b->next)
            ;
        if (!b)
            return;                 /* not an allocation of the arena */
        cur = b;
        endp = (char *)b + EXPRTREE_BLOCK;
    }
    freep = p;
}

/* no asm counterpart; replaces blitz2.s:9153 pushprec - n pending.  The
   original checked the limit only in popprec, after the write past the
   end of precstack; this stops before it */
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
    struct exprnode *p;

    /* a counted n is on the pending list, the nodes above it were made
       after it */
    for (p = pend; p && p != n; p = p->down)
        ;
    if (p) {
        while (pend != n) {
            pend = pend->down;
            npend--;
        }
        pend = n->down;
        npend--;
    }
    freefrom(n);
}

/* no asm counterpart - the node after n in use */
struct exprnode *exprtree_after(struct exprnode *n)
{
    return (char *)(n + 1) < freep && inblock(n + 1, (char *)n, freep) ? n + 1 : NULL;
}

/* no asm counterpart - the bytes in use, the nodes in use, the pending
   ones, the further blocks */
unsigned long exprtree_used(void)
{
    struct block *b;
    unsigned long used = sizeof arena;

    if (!cur)
        return (unsigned long)(freep - (char *)arena);
    for (b = blocks; b != cur; b = b->next)
        used += EXPRTREE_BLOCK - BLOCKSPACE;
    return used + (unsigned long)(freep - ((char *)cur + BLOCKSPACE));
}

unsigned exprtree_live(void)
{
    return (unsigned)(exprtree_used() / sizeof(struct exprnode));
}

unsigned exprtree_pending(void)
{
    return npend;
}

unsigned exprtree_blocks(void)
{
    return nblocks;
}

/* no asm counterpart; replaces blitz2.s:8983-9006 eval5's control (bsr
   eval2; .more: cmp -(a1),d1 / bhi .higher; .higher: bsr pushprec, bsr
   eval5 for the right operand, popprec, bsr doop) - one level of the
   precedence climbing: an operand, then each operator that binds tighter
   than the pending one with its right operand (one level deeper: the
   recursion of the original, once per higher precedence) and its
   emission; a chain of one precedence is the loop.  The hooks do the
   rest of eval5 at the moments the original did. */
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
