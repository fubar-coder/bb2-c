/*
 * blitzlib.c - reading and checking Blitz Basic 2 library files, see
 * blitzlib.h.
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "blitzlib.h"

/* ------------------------------------------------------------ reading */

/* reading the code with bounds: leaving the code or a broken structure
   jumps back to blitzlib_read */
struct cursor {
    const uint8_t *code;
    uint32_t size;
    jmp_buf fail;
    uint32_t at;                /* where it failed */
    const char *what;
    struct blitzlib *lib;
};

static void stop(struct cursor *c, uint32_t p, const char *what)
{
    c->at = p;
    c->what = what;
    longjmp(c->fail, 1);
}

static uint8_t B(struct cursor *c, uint32_t p)
{
    if (p >= c->size)
        stop(c, p, "runs past the end of the code");
    return c->code[p];
}

static uint16_t W(struct cursor *c, uint32_t p)
{
    if (p > c->size || c->size - p < 2)
        stop(c, p, "runs past the end of the code");
    return be16(c->code + p);
}

static uint32_t L(struct cursor *c, uint32_t p)
{
    if (p > c->size || c->size - p < 4)
        stop(c, p, "runs past the end of the code");
    return be32(c->code + p);
}

static uint32_t even(uint32_t p)
{
    return (p + 1) & ~1u;
}

/* marks n bytes from p in the mask (as far as they are inside the code) */
static void mark(struct cursor *c, uint32_t p, uint32_t n, uint8_t what)
{
    while (n-- && p < c->size)
        c->lib->mask[p++] = what;
}

/* padding up to the next even offset */
static uint32_t pad(struct cursor *c, uint32_t p)
{
    if (p & 1)
        mark(c, p, 1, BLITZLIB_IGNORE);
    return even(p);
}

/* {libnum.w, offset.w}* 0.w */
static uint32_t libs(struct cursor *c, uint32_t p)
{
    while (W(c, p))
        p += 4;
    return p + 2;
}

/* [$FFFF] next.l nummer.w "name",0 "help",0 [even]; sets the names of e */
static uint32_t namblock(struct cursor *c, uint32_t p, int lead, struct blitzlib_entry *e)
{
    if (lead) {
        if (W(c, p) != 0xFFFF)
            stop(c, p, "expected $FFFF in front of the name block");
        p += 2;
    }
    mark(c, p, 6, BLITZLIB_IGNORE);         /* next.l nummer.w: run time */
    p += 6;
    e->name = (const char *)c->code + p;
    while (B(c, p))
        p++;
    p++;
    e->help = (const char *)c->code + p;
    while (B(c, p))
        p++;
    return pad(c, p + 1);
}

static void add_form(struct cursor *c, uint32_t p, uint16_t head)
{
    struct blitzlib *lib = c->lib;

    if (lib->nforms % 32 == 0) {
        struct blitzlib_form *n = realloc(lib->forms, (lib->nforms + 32) * sizeof *n);

        if (!n)
            stop(c, p, "out of memory");
        lib->forms = n;
    }
    lib->forms[lib->nforms].head = head;
    lib->forms[lib->nforms].types = c->code + p + 2;
    lib->nforms++;
}

static void add(struct cursor *c, const struct blitzlib_entry *e)
{
    struct blitzlib *lib = c->lib;

    if (lib->nentries % 32 == 0) {
        struct blitzlib_entry *n = realloc(lib->entries, (lib->nentries + 32) * sizeof *n);

        if (!n)
            stop(c, e->offset, "out of memory");
        lib->entries = n;
    }
    lib->entries[lib->nentries++] = *e;
}

/* the token table from $30 */
static void parse(struct cursor *c)
{
    uint32_t p = 0x30;

    while (!(W(c, p) & 0x8000)) {
        struct blitzlib_entry e = { .offset = p, .kind = W(c, p), .form = c->lib->nforms };

        mark(c, p + 2, 4, BLITZLIB_IGNORE);
        p += 6;
        if (e.kind == 8) {                      /* dumtoke */
            p = namblock(c, p, 0, &e);
        } else if (e.kind & 4) {                /* AmigaOS call */
            e.oslib = W(c, p);                  /* library, LVO */
            e.lvo = (int16_t)W(c, p + 2);
            p += 4;
            e.regs = c->code + p;
            while (B(c, p) != 0xFF)             /* registers */
                p++;
            e.nregs = (unsigned)(p - (uint32_t)(e.regs - c->code));
            p = namblock(c, pad(c, p + 1), 0, &e);
        } else if ((e.kind & 0xF) == 0) {       /* nullsub */
            p = libs(c, p);
            L(c, p + 8);
            mark(c, p, 12, BLITZLIB_CODE);
            p += 12;
        } else {                                /* statement/function/command */
            while (W(c, p) != 0xFFFF) {
                uint16_t head = W(c, p);

                add_form(c, p, head);
                e.nforms++;
                p = pad(c, p + 2 + (head & 0xFF));      /* argument types */
                p = libs(c, p);
                L(c, p + 8);
                mark(c, p, 12, BLITZLIB_CODE);          /* subs */
                p += 12;
            }
            p = namblock(c, p, 1, &e);
        }
        add(c, &e);
    }
    if (W(c, p) != 0xFFFF)
        stop(c, p, "expected $FFFF behind the entries");
    p += 2;
    if (L(c, p)) {                              /* !libfin */
        uint32_t toke = L(c, p), q;

        c->lib->libfin = p;
        W(c, p + 24);
        c->lib->defmax = W(c, p + 22);
        for (q = toke; q < c->size && c->code[q]; q++)
            ;
        if (q < c->size && q > toke)
            c->lib->object = (const char *)c->code + toke;
        mark(c, p, 20, BLITZLIB_CODE);          /* toke load save use free */
        mark(c, p + 20, 2, BLITZLIB_IGNORE);
        p += 26;
    } else {
        p += 4;
    }
    if (p > c->size)
        stop(c, p, "runs past the end of the code");
    c->lib->codestart = p;
}

/* the code offsets of the header: init, finit, error */
static void mark_header(struct blitzlib *lib)
{
    static const uint32_t code[] = { 0x16, 0x1C, 0x2C };
    unsigned i;

    for (i = 0; i < 3; i++)
        memset(lib->mask + code[i], BLITZLIB_CODE, 4);
}

int blitzlib_read(const struct hunk *h, struct blitzlib *lib, char *err, size_t errlen)
{
    /* static: longjmp leaves changed automatic variables indeterminate */
    static struct cursor c;

    memset(lib, 0, sizeof *lib);
    lib->code = h->data;
    lib->size = h->size;
    if (!h->data || h->size < 0x30) {
        snprintf(err, errlen, "no library header (code shorter than $30 bytes)");
        return -1;
    }
    if (!(lib->mask = calloc(h->size, 1))) {
        snprintf(err, errlen, "out of memory");
        return -1;
    }
    mark_header(lib);
    lib->libnum = be16(h->data + 4);
    lib->error = be32(h->data + 0x2C);
    c.code = h->data;
    c.size = h->size;
    c.lib = lib;
    if (setjmp(c.fail)) {
        snprintf(err, errlen, "token table does not parse: %s at $%X", c.what, (unsigned)c.at);
        blitzlib_free(lib);
        return -1;
    }
    parse(&c);
    return 0;
}

void blitzlib_free(struct blitzlib *lib)
{
    free(lib->entries);
    free(lib->forms);
    free(lib->mask);
    memset(lib, 0, sizeof *lib);
}

/* ------------------------------------------------------------- checks */

int blitzlib_check(const struct loadfile *lf, const char *name, FILE *out)
{
    static const uint8_t moveq_rts[4] = { 0x70, 0x00, 0x4E, 0x75 };
    static const uint8_t zero[4];
    const struct hunk *h;
    struct blitzlib lib;
    char err[200];
    uint32_t i;
    int bad;

    if (lf->table_size != 1 || lf->nhunks != 1) {
        fprintf(out, "%s: %u hunks, a library needs exactly one (check the linker script)\n",
                name, (unsigned)lf->table_size);
        return 1;
    }
    h = &lf->hunks[0];
    if (h->type != HUNK_CODE) {
        fprintf(out, "%s: the hunk is $%X, not code\n", name, (unsigned)h->type);
        return 1;
    }
    for (i = 0; i < h->nrelocs; i++) {
        if (h->reloc_target[i] != 0) {
            fprintf(out, "%s: relocation at $%X into hunk %u, only hunk 0 is allowed\n",
                    name, (unsigned)h->reloc_offset[i], (unsigned)h->reloc_target[i]);
            return 1;
        }
        if (h->reloc_offset[i] > h->size || h->size - h->reloc_offset[i] < 4) {
            fprintf(out, "%s: relocation at $%X outside the code\n",
                    name, (unsigned)h->reloc_offset[i]);
            return 1;
        }
    }
    if (h->size < 4 || (memcmp(h->data, moveq_rts, 4) && memcmp(h->data, zero, 4))) {
        fprintf(out, "%s: code does not start with the library header (MOVEQ #0,d0:RTS)\n", name);
        return 1;
    }
    if (blitzlib_read(h, &lib, err, sizeof err)) {
        fprintf(out, "%s: %s\n", name, err);
        return 1;
    }

    bad = 0;
    /* a user library numbers its entries from libnum*128+1, nullsubs
       included (blitz2.s fixlibs); a token behind the 127 numbers of the
       library's block is the next library's number, and the compiler ends
       the range where the next loaded library's begins (#960): such an
       entry is found only while that number is free.  Nullsubs there are
       reached through the header (init, finit, the object handlers of
       !libfin) */
    if (!(lib.libnum & 0x8000))
        for (i = 127; i < lib.nentries; i++) {
            const struct blitzlib_entry *e = &lib.entries[i];
            unsigned first = ((unsigned)lib.libnum << 7) + 1;

            if ((e->kind & 0xF) == 0)
                continue;               /* nullsub: no token */
            fprintf(out, "%s: library %u: entry %u \"%s\" gets the token $%04X, behind "
                    "the 127 of the library ($%04X-$%04X)\n",
                    name, (unsigned)lib.libnum, (unsigned)i + 1, e->name ? e->name : "",
                    ((first + i) | 0x8000) & 0xFFFF, first | 0x8000, (first + 126) | 0x8000);
            bad = 1;
        }
    if (lib.error) {
        for (i = 0; i < h->nrelocs; i++) {
            uint32_t r = h->reloc_offset[i];
            uint32_t target = be32(h->data + r);

            if (target & 0x80000000)
                continue;               /* ALibJsr placeholder, not an address */
            if (lib.codestart <= r && r < lib.error && target >= lib.error) {
                fprintf(out, "%s: code at $%X refers to $%X in the error check area "
                        "(from $%X), dangling without debugger\n",
                        name, (unsigned)r, (unsigned)target, (unsigned)lib.error);
                bad = 1;
            }
        }
    }
    blitzlib_free(&lib);
    return bad;
}

/* ------------------------------------------------- reserved numbers */

int blitzlib_is_literallib(const struct blitzlib *lib)
{
    unsigned i;

    if (lib->libnum != 33 || lib->nentries != 127)
        return 0;
    for (i = 0; i < lib->nentries; i++) {
        const struct blitzlib_entry *e = &lib->entries[i];

        if (e->kind != 8 || !e->name || (uint8_t)e->name[0] != 0x81 + i || e->name[1])
            return 0;
    }
    return 1;
}

int blitzlib_reserved(const struct blitzlib *lib, int allow33)
{
    if (lib->libnum == 32)
        return BLITZLIB_RESERVE_32;
    if (lib->libnum != 33 || blitzlib_is_literallib(lib))
        return BLITZLIB_RESERVE_OK;
    return allow33 ? BLITZLIB_RESERVE_ALLOWED : BLITZLIB_RESERVE_33;
}
