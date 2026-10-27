/*
 * blitzlib.c - checks of a Blitz Basic 2 library file, see blitzlib.h.
 */
#include <setjmp.h>
#include <string.h>
#include "blitzlib.h"

/* reading the code with bounds: leaving the code or a broken structure
   jumps back to blitzlib_check */
struct cursor {
    const uint8_t *code;
    uint32_t size;
    jmp_buf fail;
    uint32_t at;                /* where it failed */
    const char *what;
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

/* {libnum.w, offset.w}* 0.w */
static uint32_t libs(struct cursor *c, uint32_t p)
{
    while (W(c, p))
        p += 4;
    return p + 2;
}

/* [$FFFF] next.l nummer.w "name",0 "help",0 [even] */
static uint32_t namblock(struct cursor *c, uint32_t p, int lead)
{
    if (lead) {
        if (W(c, p) != 0xFFFF)
            stop(c, p, "expected $FFFF in front of the name block");
        p += 2;
    }
    p += 6;
    while (B(c, p))             /* name */
        p++;
    p++;
    while (B(c, p))             /* help text */
        p++;
    return even(p + 1);
}

/* the token table from $30; returns the offset behind it (start of the
   code) and the number of entries */
static uint32_t parse(struct cursor *c, unsigned *entries)
{
    uint32_t p = 0x30;

    while (!(W(c, p) & 0x8000)) {
        uint16_t kind = W(c, p);

        p += 6;
        if (kind == 8) {                        /* dumtoke */
            p = namblock(c, p, 0);
        } else if (kind & 4) {                  /* AmigaOS call */
            p += 4;                             /* library, LVO */
            while (B(c, p) != 0xFF)             /* registers */
                p++;
            p = namblock(c, even(p + 1), 0);
        } else if ((kind & 0xF) == 0) {         /* nullsub */
            p = libs(c, p) + 12;
        } else {                                /* statement/function/command */
            while (W(c, p) != 0xFFFF) {
                uint16_t head = W(c, p);

                p = even(p + 2 + (head & 0xFF));        /* argument types */
                p = libs(c, p) + 12;                    /* libs, subs */
            }
            p = namblock(c, p, 1);
        }
        (*entries)++;
    }
    if (W(c, p) != 0xFFFF)
        stop(c, p, "expected $FFFF behind the entries");
    p += 2;
    if (L(c, p)) {                              /* !libfin */
        W(c, p + 24);
        p += 26;
    } else {
        p += 4;
    }
    if (p > c->size)
        stop(c, p, "runs past the end of the code");
    return p;
}

int blitzlib_check(const struct loadfile *lf, const char *name, FILE *out)
{
    static const uint8_t moveq_rts[4] = { 0x70, 0x00, 0x4E, 0x75 };
    static const uint8_t zero[4];
    const struct hunk *h;
    /* static: longjmp leaves changed automatic variables indeterminate */
    static struct cursor c;
    unsigned entries = 0;
    uint32_t i, codestart, error;

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
    if (h->size < 0x30
        || (memcmp(h->data, moveq_rts, 4) && memcmp(h->data, zero, 4))) {
        fprintf(out, "%s: code does not start with the library header (MOVEQ #0,d0:RTS)\n", name);
        return 1;
    }

    c.code = h->data;
    c.size = h->size;
    if (setjmp(c.fail)) {
        fprintf(out, "%s: token table does not parse: %s at $%X\n", name, c.what, (unsigned)c.at);
        return 1;
    }
    codestart = parse(&c, &entries);

    error = be32(h->data + 0x2C);
    if (error) {
        for (i = 0; i < h->nrelocs; i++) {
            uint32_t r = h->reloc_offset[i];
            uint32_t target = be32(h->data + r);

            if (target & 0x80000000)
                continue;               /* ALibJsr placeholder, not an address */
            if (codestart <= r && r < error && target >= error)
                fprintf(out, "%s: warning: code at $%X refers to $%X in the error check area "
                        "(from $%X), dangling without debugger\n",
                        name, (unsigned)r, (unsigned)target, (unsigned)error);
        }
    }
    return 0;
}
