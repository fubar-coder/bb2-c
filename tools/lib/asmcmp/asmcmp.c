/*
 * asmcmp.c - assembler routines against the original, see asmcmp.h.
 */
#define _XOPEN_SOURCE 700       /* popen, mkstemp, strdup */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "asmcmp.h"

/* growing string */
struct str {
    char *p;
    size_t n, cap;
    int bad;
};

static void add(struct str *s, const char *p, size_t n)
{
    if (s->bad)
        return;
    if (s->n + n + 1 > s->cap) {
        size_t cap = (s->n + n + 1) * 2;
        char *q = realloc(s->p, cap);

        if (!q) {
            s->bad = 1;
            return;
        }
        s->p = q;
        s->cap = cap;
    }
    memcpy(s->p + s->n, p, n);
    s->n += n;
    s->p[s->n] = 0;
}

static int is_hexdigit(int c)
{
    return isdigit(c) || (c >= 'a' && c <= 'f');
}

char *asmcmp_normalise(const char *text, int rel)
{
    struct str a = { 0 }, b = { 0 };
    const char *p;

    add(&a, "", 0);
    add(&b, "", 0);
    /* %pc@(0xHEX) and %pc@(HEX) -> %pc@(REL) */
    for (p = text; *p; ) {
        if (!strncmp(p, "%pc@(", 5)) {
            const char *q = p + 5, *h;

            if (!strncmp(q, "0x", 2) && is_hexdigit((unsigned char)q[2]))
                q += 2;
            for (h = q; is_hexdigit((unsigned char)*h); h++)
                ;
            if (h > q && *h == ')') {
                add(&a, "%pc@(REL)", 9);
                p = h + 1;
                continue;
            }
        }
        add(&a, p++, 1);
    }
    /* 0xHEX -> A, except ALibJsr placeholders and custom chip addresses */
    for (p = a.p; *p; ) {
        if (p[0] == '0' && p[1] == 'x' && is_hexdigit((unsigned char)p[2])) {
            const char *h = p + 2;
            unsigned long long v = 0;

            for (; is_hexdigit((unsigned char)*h); h++)
                v = v * 16 + (unsigned)(isdigit((unsigned char)*h) ? *h - '0' : *h - 'a' + 10);
            if (v >= 0x80000000ULL || (v >= 0xdff000 && v < 0xdff200))
                add(&b, p, (size_t)(h - p));
            else
                add(&b, "A", 1);
            p = h;
            continue;
        }
        add(&b, p++, 1);
    }
    free(a.p);
    if (!rel || b.bad)
        return b.bad ? (free(b.p), NULL) : b.p;
    /* relocated: #DEC beyond +-255 -> #A */
    a.p = NULL;
    a.n = a.cap = 0;
    a.bad = 0;
    add(&a, "", 0);
    for (p = b.p; *p; ) {
        const char *d = p + 1 + (p[1] == '-');

        if (*p == '#' && isdigit((unsigned char)*d)) {
            const char *e = d;

            while (isdigit((unsigned char)*e))
                e++;
            if (llabs(strtoll(p + 1, NULL, 10)) > 255)
                add(&a, "#A", 2);
            else
                add(&a, p, (size_t)(e - p));
            p = e;
            continue;
        }
        add(&a, p++, 1);
    }
    free(b.p);
    return a.bad ? (free(a.p), NULL) : a.p;
}

void asmcmp_free_list(char **ins, unsigned count)
{
    unsigned i;

    for (i = 0; ins && i < count; i++)
        free(ins[i]);
    free(ins);
}

/* "  ADDR:\tHEX \tTEXT": address and the trimmed text (0 = no instruction) */
static int parse_line(char *line, uint32_t *addr, char **text)
{
    char *p = line, *e;

    while (*p == ' ' || *p == '\t')
        p++;
    if (!is_hexdigit((unsigned char)*p))
        return 0;
    *addr = (uint32_t)strtoul(p, &e, 16);
    if (e[0] != ':' || e[1] != '\t')
        return 0;
    p = e + 2;
    if (!*p || !(is_hexdigit((unsigned char)*p) || *p == ' '))
        return 0;
    while (is_hexdigit((unsigned char)*p) || *p == ' ')
        p++;
    if (*p != '\t')
        return 0;
    p++;
    while (isspace((unsigned char)*p))
        p++;
    e = p + strlen(p);
    while (e > p && isspace((unsigned char)e[-1]))
        *--e = 0;
    *text = p;
    return *p != 0;
}

int asmcmp_disasm(const char *objdump, const struct hunk *h, uint32_t start, unsigned count,
                  char ***ins, char *err, size_t errlen)
{
    const char *tmp = getenv("TMPDIR");
    char path[4096], cmd[8400], line[1024];
    uint32_t *addr = calloc((size_t)count + 1, sizeof *addr);
    char **text = calloc((size_t)count + 1, sizeof *text);
    unsigned n = 0, i;
    FILE *p;
    int fd, ok;

    *ins = NULL;
    snprintf(path, sizeof path, "%s/asmcmp-XXXXXX", tmp && *tmp ? tmp : "/tmp");
    if (!addr || !text || (fd = mkstemp(path)) < 0) {
        snprintf(err, errlen, "cannot create a temporary file");
        free(addr);
        free(text);
        return -1;
    }
    ok = write(fd, h->data, h->size) == (ssize_t)h->size;
    close(fd);
    snprintf(cmd, sizeof cmd, "'%s' -b binary -m m68k:68020 --start-address=0x%x -D '%s'",
             objdump, (unsigned)start, path);
    if (!ok || !(p = popen(cmd, "r"))) {
        snprintf(err, errlen, "cannot run %s", objdump);
        unlink(path);
        free(addr);
        free(text);
        return -1;
    }
    /* count + 1 instructions: the next address bounds the last one */
    while (n <= count && fgets(line, sizeof line, p)) {
        char *t;

        line[strcspn(line, "\n")] = 0;
        if (parse_line(line, &addr[n], &t) && !(text[n] = strdup(t)))
            break;
        if (text[n])
            n++;
    }
    while (fgets(line, sizeof line, p))
        ;
    pclose(p);
    unlink(path);
    if (n < count) {
        snprintf(err, errlen, "objdump gave %u of %u instructions from $%X", n, count,
                 (unsigned)start);
        asmcmp_free_list(text, n);
        free(addr);
        return -1;
    }
    for (i = 0; i < count; i++) {
        uint32_t next = i + 1 < n ? addr[i + 1] : addr[i] + 10, r;
        int rel = 0;
        char *norm;

        for (r = 0; r < h->nrelocs && !rel; r++)
            rel = addr[i] <= h->reloc_offset[r] && h->reloc_offset[r] < next;
        if (!(norm = asmcmp_normalise(text[i], rel))) {
            snprintf(err, errlen, "out of memory");
            asmcmp_free_list(text, n);
            free(addr);
            return -1;
        }
        free(text[i]);
        text[i] = norm;
    }
    for (i = count; i < n; i++)
        free(text[i]);
    free(addr);
    *ins = text;
    return 0;
}

/* symbol value; names compared without their leading underscores (the
   last definition wins); -1 = not there */
static int csymbol(const struct loadfile *c, const char *name, uint32_t *value)
{
    size_t n = strlen(name);
    uint32_t i, k;
    int found = -1;

    for (i = 0; i < c->nhunks; i++)
        for (k = 0; k < c->hunks[i].nsyms; k++) {
            const struct hunk_symbol *s = &c->hunks[i].syms[k];
            uint32_t skip = 0;

            while (skip < s->len && s->name[skip] == '_')
                skip++;
            if (s->len - skip == n && !memcmp(s->name + skip, name, n)) {
                *value = s->value;
                found = 0;
            }
        }
    return found;
}

int asmcmp_routine(const char *objdump, const struct hunk *orig, const struct loadfile *c,
                   const char *spec, FILE *out)
{
    char buf[256], *f[4], *plus, *end, err[300], **a = NULL, **b = NULL;
    uint32_t oaddr, caddr;
    unsigned count, i, k = 0;
    long off = 0;
    int same, ok;

    snprintf(buf, sizeof buf, "%s", spec);
    f[0] = buf;
    for (i = 1; i < 4; i++) {
        char *colon = strchr(f[i - 1], ':');

        if (!colon) {
            fprintf(out, "asmcmp: %s: expected NAME:ORIGADDR:CSYMBOL:COUNT\n", spec);
            return -1;
        }
        *colon = 0;
        f[i] = colon + 1;
    }
    oaddr = (uint32_t)strtoul(f[1], &end, 16);
    ok = *f[1] && !*end;
    count = (unsigned)strtoul(f[3], &end, 10);
    ok &= *f[3] && !*end && count > 0;
    if ((plus = strchr(f[2], '+'))) {
        *plus = 0;
        off = strtol(plus + 1, &end, 16);
        ok &= plus[1] && !*end;
    }
    if (csymbol(c, f[2], &caddr)) {
        caddr = (uint32_t)strtoul(f[2], &end, 16);
        if (!*f[2] || *end) {
            fprintf(out, "asmcmp: %s: no symbol %s\n", spec, f[2]);
            return -1;
        }
    }
    if (!ok) {
        fprintf(out, "asmcmp: %s: expected NAME:ORIGADDR:CSYMBOL[+HEX]:COUNT\n", spec);
        return -1;
    }
    caddr += (uint32_t)off;
    if (!c->nhunks || asmcmp_disasm(objdump, orig, oaddr, count, &a, err, sizeof err)
        || asmcmp_disasm(objdump, &c->hunks[0], caddr, count, &b, err, sizeof err)) {
        fprintf(out, "asmcmp: %s: %s\n", f[0], c->nhunks ? err : "no hunk");
        asmcmp_free_list(a, count);
        return -1;
    }
    for (i = 0, same = 1; i < count; i++)
        same &= !strcmp(a[i], b[i]);
    if (same)
        fprintf(out, "%-14s %3u instructions identical\n", f[0], count);
    else {
        fprintf(out, "%-14s DIFFERS\n", f[0]);
        for (k = 0; k < count; k++)
            fprintf(out, "   %s %-40s %s\n", strcmp(a[k], b[k]) ? "*" : " ", a[k], b[k]);
    }
    asmcmp_free_list(a, count);
    asmcmp_free_list(b, count);
    return !same;
}
