/*
 * tlcore.c - the logic of the Blitz test library (tlcore.h).  No libc,
 * no libgcc (no division, no 32-bit multiply), no allocation: compiled
 * by the library with the Blitz library conventions and by its host
 * tests.
 */
#include "tlcore.h"

/* the entries written as they are: no state is needed to make them */
static const char damaged[] = "broken\tstate damaged\n";
static const char lost[] = "broken\tentries lost while kept\n";

static void copy(uint8_t *to, const uint8_t *from, size_t n)
{
    while (n--)
        *to++ = *from++;
}

static struct basictest_str str(const char *s)
{
    struct basictest_str r;
    size_t n = 0;

    while (s[n])
        n++;
    r.data = (const uint8_t *)s;
    r.length = n;
    return r;
}

/* an empty entry of a kind */
static struct basictest_entry entry(int kind)
{
    struct basictest_entry e = { 0 };

    e.kind = kind;
    return e;
}

static void flush(struct tl_state *st);

/* the state before the first tl_begin (all zero) */
static int fresh(const struct tl_state *st)
{
    return !st->guard_lo && !st->guard_hi && !st->begun;
}

/*
 * 1 if the state can be used: tl_begin ran.  A damaged state is reported
 * once, as an entry made without the state (now, or with the next write:
 * the end's); the guards are set again and the state is dead: only
 * tl_finit writes, its end.
 */
static int alive(struct tl_state *st)
{
    if (fresh(st))
        return 0;
    if (st->guard_lo == TL_GUARD_LO && st->guard_hi == TL_GUARD_HI)
        return st->begun && !st->dead;
    st->guard_lo = TL_GUARD_LO;
    st->guard_hi = TL_GUARD_HI;
    st->begun = 1;
    st->dead = 1;
    st->open = TL_NONE;
    st->pending_length = 0;
    st->lost = 0;
    st->damaged = 1;
    if (tl_os_can_write())
        flush(st);
    return 0;
}

/* the note of a damaged state, the kept entries, the note that some were
   lost */
static void flush(struct tl_state *st)
{
    if (st->damaged) {
        st->damaged = 0;
        tl_os_write((const uint8_t *)damaged, sizeof damaged - 1);
    }
    if (st->pending_length) {
        tl_os_write(st->pending, st->pending_length);
        st->pending_length = 0;
    }
    if (st->lost) {
        st->lost = 0;
        tl_os_write((const uint8_t *)lost, sizeof lost - 1);
    }
}

/* writes or keeps the n bytes of a line; force: write in any case */
static void out(struct tl_state *st, const uint8_t *line, size_t n, int force)
{
    if (force || tl_os_can_write()) {
        flush(st);
        tl_os_write(line, n);
    } else if (n <= sizeof st->pending - st->pending_length) {
        copy(st->pending + st->pending_length, line, n);
        st->pending_length += n;
    } else {
        st->lost = 1;
    }
}

/*
 * Makes the line of an entry in st->line and writes it; returns its
 * length.  A string check that does not fit has its values cut until it
 * fits.
 */
static size_t emit(struct tl_state *st, struct basictest_entry *e, int force)
{
    size_t n = basictest_write(e, st->line, sizeof st->line);

    if (!n && e->kind == BASICTEST_CHECKSTR) {
        size_t cut = e->expected_str.length > e->actual_str.length ? e->expected_str.length
                                                                   : e->actual_str.length;

        e->cut = 1;
        while (!n && cut) {
            cut >>= 1;
            if (e->expected_str.length > cut)
                e->expected_str.length = cut;
            if (e->actual_str.length > cut)
                e->actual_str.length = cut;
            n = basictest_write(e, st->line, sizeof st->line);
        }
    }
    if (n)
        out(st, st->line, n, force);
    return n;
}

static void broken(struct tl_state *st, const char *reason)
{
    struct basictest_entry e = entry(BASICTEST_BROKEN);

    e.reason = str(reason);
    emit(st, &e, 0);
}

static void simple(struct tl_state *st, int kind)
{
    struct basictest_entry e = entry(kind);

    emit(st, &e, 0);
}

/* a check, precondition or announcement outside a block */
static void inside(struct tl_state *st)
{
    if (st->open == TL_NONE)
        broken(st, "check outside a block");
}

void tl_begin(struct tl_state *st)
{
    uint8_t *p = (uint8_t *)st;
    size_t i;
    int again = st->guard_lo == TL_GUARD_LO && st->guard_hi == TL_GUARD_HI && st->begun;
    struct basictest_entry e = entry(BASICTEST_BEGIN);

    if (again) {
        if (!st->dead)
            broken(st, "begin inside the run");
        return;
    }
    for (i = 0; i < sizeof *st; i++)
        p[i] = 0;
    st->guard_lo = TL_GUARD_LO;
    st->guard_hi = TL_GUARD_HI;
    st->begun = 1;
    e.version = BASICTEST_VERSION;
    emit(st, &e, 0);
}

int tl_block(struct tl_state *st, int block, const uint8_t *name, size_t length)
{
    struct basictest_entry e = entry(BASICTEST_BLOCK);
    int open = block + 1;

    if (!alive(st))
        return 0;
    if (st->open != TL_NONE)
        broken(st, "block inside a block");
    if (open == TL_INIT && st->phase != TL_NONE)
        broken(st, "batch init after a block");
    else if (open == TL_CASE && st->phase == TL_FINAL)
        broken(st, "case after the batch cleanup");
    else if (open == TL_FINAL && st->phase == TL_FINAL)
        broken(st, "second batch cleanup");
    if (open > st->phase)
        st->phase = (uint8_t)open;
    st->open = TL_NONE;
    if (open == TL_FINAL && st->init_broken)
        return 0;
    e.block = block;
    if (open == TL_CASE) {
        e.name.data = name;
        e.name.length = length;
    }
    if (!emit(st, &e, 0)) {
        /* a case without a name, or one too long for a line */
        e.name = str("?");
        emit(st, &e, 0);
        broken(st, length ? "case name too long" : "case without a name");
    }
    if (open == TL_CASE && st->init_broken) {
        struct basictest_entry r = entry(BASICTEST_REQUIRE);

        r.k = st->init_broken_k;
        emit(st, &r, 0);
        simple(st, BASICTEST_BLOCKEND);
        return 0;
    }
    if (open == TL_CASE && st->init_failed)
        out(st, st->init_line, st->init_length, 0);
    st->open = (uint8_t)open;
    return 1;
}

void tl_blockend(struct tl_state *st)
{
    if (!alive(st))
        return;
    if (st->open == TL_NONE)
        broken(st, "block end without a block");
    simple(st, BASICTEST_BLOCKEND);
    st->open = TL_NONE;
}

/* a failed check of the batch init: kept for every case */
static void init_check(struct tl_state *st, size_t n)
{
    if (st->open == TL_INIT && !st->init_failed && n) {
        copy(st->init_line, st->line, n);
        st->init_length = n;
        st->init_failed = 1;
    }
}

void tl_check(struct tl_state *st, uint32_t k, uint32_t expected, uint32_t actual)
{
    struct basictest_entry e = entry(BASICTEST_CHECK);

    if (!alive(st))
        return;
    inside(st);
    e.k = k;
    e.ok = expected == actual;
    e.expected = expected;
    e.actual = actual;
    if (e.ok)
        emit(st, &e, 0);
    else
        init_check(st, emit(st, &e, 0));
}

void tl_checkq(struct tl_state *st, uint32_t k, uint32_t expected, uint32_t actual)
{
    struct basictest_entry e = entry(BASICTEST_CHECKQ);

    if (!alive(st))
        return;
    inside(st);
    e.k = k;
    e.ok = expected == actual;
    e.expected = expected;
    e.actual = actual;
    if (e.ok)
        emit(st, &e, 0);
    else
        init_check(st, emit(st, &e, 0));
}

void tl_checkflt(struct tl_state *st, uint32_t k, uint32_t expected, uint32_t actual,
                 uint32_t delta)
{
    struct basictest_entry e = entry(BASICTEST_CHECKFLT);
    int near;

    if (!alive(st))
        return;
    inside(st);
    near = tl_os_fltnear(expected, actual, delta);
    if (near < 0)
        broken(st, "float compare not available");
    e.k = k;
    e.ok = near > 0;
    e.expected = expected;
    e.actual = actual;
    e.delta = delta;
    if (e.ok)
        emit(st, &e, 0);
    else
        init_check(st, emit(st, &e, 0));
}

void tl_checkstr(struct tl_state *st, uint32_t k, const uint8_t *expected, size_t expected_length,
                 const uint8_t *actual, size_t actual_length)
{
    struct basictest_entry e = entry(BASICTEST_CHECKSTR);
    size_t i;

    if (!alive(st))
        return;
    inside(st);
    e.k = k;
    e.ok = expected_length == actual_length;
    for (i = 0; e.ok && i < expected_length; i++)
        e.ok = expected[i] == actual[i];
    e.expected_str.data = expected;
    e.expected_str.length = expected_length;
    e.actual_str.data = actual;
    e.actual_str.length = actual_length;
    /* of the whole strings: emit may cut the values, not these */
    e.expected_length = (uint32_t)expected_length;
    e.expected_crc = basictest_crc32(expected, expected_length);
    e.actual_length = (uint32_t)actual_length;
    e.actual_crc = basictest_crc32(actual, actual_length);
    if (e.ok)
        emit(st, &e, 0);
    else
        init_check(st, emit(st, &e, 0));
}

int tl_require(struct tl_state *st, uint32_t k, uint32_t condition)
{
    struct basictest_entry e = entry(BASICTEST_REQUIRE);

    if (!alive(st) || condition)
        return 0;
    inside(st);
    e.k = k;
    emit(st, &e, 0);
    if (st->open == TL_NONE)
        return 0;
    if (st->open == TL_INIT) {
        st->init_broken = 1;
        st->init_broken_k = k;
    }
    simple(st, BASICTEST_BLOCKEND);
    st->open = TL_NONE;
    return 1;
}

void tl_expect_error(struct tl_state *st, uint32_t k)
{
    struct basictest_entry e = entry(BASICTEST_EXPECT_ERROR);

    if (!alive(st))
        return;
    inside(st);
    e.k = k;
    emit(st, &e, 0);
}

void tl_finit(struct tl_state *st)
{
    struct basictest_entry e = entry(BASICTEST_END);

    if (fresh(st) || (st->guard_lo == TL_GUARD_LO && st->guard_hi == TL_GUARD_HI && !st->begun))
        return;
    if (alive(st) && st->open != TL_NONE) {
        struct basictest_entry i = entry(BASICTEST_INCOMPLETE);

        emit(st, &i, 1);
    }
    emit(st, &e, 1);
    st->begun = 0;
    st->open = TL_NONE;
}

/* the result of a run: nothing printed, the return code unknown */
static void run_clear(struct tl_run *r, struct tl_run_text *t)
{
    r->rc = 0;
    r->rcknown = 0;
    r->cut = 0;
    r->size = 0;
    t->length = 0;
    t->chars[0] = 0;
    r->printed = t->chars;
}

int tl_run_start(struct tl_state *st, uint32_t k, struct tl_run *r, struct tl_run_text *t)
{
    struct basictest_entry e = entry(BASICTEST_RUN);

    run_clear(r, t);
    if (!alive(st))
        return 0;
    if (st->open == TL_NONE) {
        broken(st, "run outside a block");
        return 0;
    }
    if (!tl_os_can_write()) {
        broken(st, "run in Blitz mode");
        return 0;
    }
    e.k = k;
    emit(st, &e, 0);
    return 1;
}

void tl_run_end(struct tl_state *st, uint32_t k, struct tl_run *r, struct tl_run_text *t, int known, int32_t rc,
                size_t n, uint32_t size)
{
    struct basictest_entry e = entry(BASICTEST_RUNEND);

    if (n > TL_RUN_TEXT)
        n = TL_RUN_TEXT;
    if (size < n)
        size = (uint32_t)n;
    t->length = (uint32_t)n;
    t->chars[n] = 0;
    r->printed = t->chars;
    r->size = size;
    r->cut = size > n ? -1 : 0;
    r->rcknown = known ? -1 : 0;
    r->rc = known ? rc : 0;
    if (!alive(st))
        return;
    e.k = k;
    e.ok = known != 0;
    e.actual = (uint32_t)r->rc;
    emit(st, &e, 0);
}
