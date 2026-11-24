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
static unsigned long usedbase = 0;              /* the bytes of the blocks before it */
static char *freep = (char *)arena;             /* its first free byte */
static char *endp = (char *)(arena + EXPRTREE_NODES);   /* its end */
static struct exprnode *pend = NULL;            /* the pending node on top */
static unsigned npend = 0;                      /* the pending nodes */
static struct exprunit unit = { NULL, NULL, 0 }; /* the unit being built */
static int replaying = 0;                       /* its actions are run */

/* no asm counterpart; replaces blitz2.s:2824 prepstack's move.l
   #precstack,precsp - every node free, none pending, the further blocks
   given back, no unit being built */
void exprtree_reset(void)
{
    while (blocks) {
        struct block *b = blocks;

        blocks = b->next;
        exprtree_freeblock(b);
    }
    nblocks = 0;
    cur = NULL;
    usedbase = 0;
    freep = (char *)arena;
    endp = (char *)(arena + EXPRTREE_NODES);
    pend = NULL;
    npend = 0;
    unit.first = NULL;
    unit.tail = &unit.first;
    unit.building = 0;
    replaying = 0;
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
        usedbase += cur ? EXPRTREE_BLOCK - BLOCKSPACE : sizeof arena;
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


/* no asm counterpart - the block b (NULL: the static one) in use from p
   on, the further ones after it kept for the next nodes */
static void usefrom(struct block *b, char *p)
{
    struct block *k;

    cur = b;
    usedbase = 0;
    if (b) {
        usedbase = sizeof arena;
        for (k = blocks; k != b; k = k->next)
            usedbase += EXPRTREE_BLOCK - BLOCKSPACE;
    }
    endp = b ? (char *)b + EXPRTREE_BLOCK : (char *)(arena + EXPRTREE_NODES);
    freep = p;
}

/* no asm counterpart - the arena free from p on (p in the static block or
   in one of the further ones, which stay with the arena) */
static void freefrom(void *p)
{
    struct block *b;

    if (inblock(p, (const char *)arena, (const char *)(arena + EXPRTREE_NODES))) {
        usefrom(NULL, p);
        return;
    }
    for (b = blocks; b && !inblock(p, (const char *)b, (const char *)b + EXPRTREE_BLOCK);
         b = b->next)
        ;
    if (b)                          /* else not an allocation of the arena */
        usefrom(b, p);
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
   #6,precsp (blitz2.s:8979) - n done: no longer pending, and outside a
   unit free with what follows it */
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
    if (!unit.building)
        freefrom(n);
}

/* no asm counterpart - the first free byte of the arena */
struct exprmark exprtree_mark(void)
{
    struct exprmark m;

    m.block = cur;
    m.at = freep;
    return m;
}

/* no asm counterpart - the arena free from the mark m on */
void exprtree_unwind(struct exprmark m)
{
    usefrom(m.block, m.at);
}

/* no asm counterpart - the bytes in use, the nodes in use, the pending
   ones, the further blocks */
unsigned long exprtree_used(void)
{
    return usedbase + (unsigned long)(freep - (cur ? (char *)cur + BLOCKSPACE : (char *)arena));
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

/* no asm counterpart - a unit begins (the one being built saved) */
void exprtree_begin(struct exprunit *outer)
{
    *outer = unit;
    unit.first = NULL;
    unit.tail = &unit.first;
    unit.building = 1;
}

/* no asm counterpart - the unit being built set aside, none built */
void exprtree_suspend(struct exprunit *outer)
{
    exprtree_begin(outer);
    unit.building = 0;
}

/* no asm counterpart - the unit's actions; the saved one back */
struct expract *exprtree_end(struct exprunit *outer)
{
    struct expract *a = unit.first;

    unit = *outer;
    if (!unit.tail)
        unit.tail = &unit.first;
    return a;
}

int exprtree_building(void)
{
    return unit.building;
}

/* no asm counterpart - the last action of the unit (next is its first
   member: tail points into it) */
struct expract *exprtree_lastact(void)
{
    return unit.tail == &unit.first ? NULL : (struct expract *)(void *)unit.tail;
}

/* no asm counterpart - the code of a unit written: its actions run */
void exprtree_run(struct expract *a)
{
    int s = replaying;

    replaying = 1;
    for (; a; a = a->next)
        exprtree_runact(a);
    replaying = s;
}

int exprtree_replaying(void)
{
    return replaying;
}

/* no asm counterpart - an action appended to the unit */
struct expract *exprtree_act(exprtree_actfn *fn)
{
    struct expract *a = exprtree_alloc(sizeof *a);

    memset(a, 0, sizeof *a);
    a->fn = fn;
    *unit.tail = a;
    unit.tail = &a->next;
    return a;
}

/* no asm counterpart; replaces blitz2.s:8983-9006 eval5's control (bsr
   eval2; .more: cmp -(a1),d1 / bhi .higher; .higher: bsr pushprec, bsr
   eval5 for the right operand, popprec, bsr doop) - one level of the
   precedence climbing: an operand, then each operator that binds tighter
   than the pending one with its right operand (one level deeper: the
   recursion of the original, once per higher precedence), after which it
   is typed and gets its code; a chain of one precedence is the loop.  The
   hooks do the rest of eval5 at the moments the original did.  The
   operator's result is a node of its own (RESULT), the left operand of
   the next operator. */
unsigned exprtree_climb(void *ctx, unsigned *op, struct exprnode **last)
{
    struct exprnode *l = exprtree_new(EXN_LEAF);
    unsigned prec = exprtree_operand(ctx, l, op);

    exprtree_left(ctx, l, prec > pend->prec ? *op : pend->op);
    while (prec > pend->prec) {
        struct exprnode *n = exprtree_new(EXN_BINOP), *r;
        unsigned char flags;
        unsigned short t;
        unsigned long start;
        unsigned nop;

        n->left = l;
        n->op = (unsigned short)*op;
        n->prec = (unsigned short)prec;
        exprtree_pre(ctx, n);
        exprtree_push(n, *op, prec);
        exprtree_enter(ctx, n);
        prec = exprtree_climb(ctx, &nop, &n->right);
        exprtree_emit(ctx, n);
        flags = (unsigned char)(l->flags & EXF_CMP);
        t = n->tafter;
        start = l->value;
        exprtree_release(n);
        r = exprtree_new(EXN_RESULT);
        r->flags = flags;
        r->tbefore = t;
        r->value = start;           /* the caller's: where its left operand began */
        l = r;
        *op = nop;
    }
    exprtree_done(ctx, l);
    *last = l;
    return prec;
}
