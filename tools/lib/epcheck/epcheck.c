/*
 * epcheck.c - the check of the __entrypoint routines (epcheck.h).
 */
#include "epcheck.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

struct sym {
    unsigned long value;
    char *name;
};

struct insn {
    unsigned long off;
    char *text;                 /* "moveal d1,a0" */
};

struct member {
    char *name;
    struct sym *syms;           /* the symbols of .text */
    size_t nsyms;
    struct insn *ins;           /* the instructions of .text */
    size_t nins;
};

struct members {
    struct member *m;
    size_t n;
};

static char *dupn(const char *s, size_t n)
{
    char *p = malloc(n + 1);

    if (p) {
        memcpy(p, s, n);
        p[n] = 0;
    }
    return p;
}

static void free_members(struct members *ms)
{
    size_t i, j;

    for (i = 0; i < ms->n; i++) {
        for (j = 0; j < ms->m[i].nsyms; j++)
            free(ms->m[i].syms[j].name);
        for (j = 0; j < ms->m[i].nins; j++)
            free(ms->m[i].ins[j].text);
        free(ms->m[i].syms);
        free(ms->m[i].ins);
        free(ms->m[i].name);
    }
    free(ms->m);
}

/* the member a line "NAME:     file format ..." begins, or NULL */
static struct member *member_line(struct members *ms, const char *l, size_t n, int *oom)
{
    static const char ff[] = ":     file format ";
    const char *f = NULL;
    struct member *m;
    size_t i;

    for (i = 0; i + sizeof ff - 1 <= n; i++)
        if (!memcmp(l + i, ff, sizeof ff - 1)) {
            f = l + i;
            break;
        }
    if (!f)
        return NULL;
    for (i = 0; i < ms->n; i++)
        if (strlen(ms->m[i].name) == (size_t)(f - l) && !memcmp(ms->m[i].name, l, f - l))
            return &ms->m[i];
    m = realloc(ms->m, (ms->n + 1) * sizeof *m);
    if (!m) {
        *oom = 1;
        return NULL;
    }
    ms->m = m;
    m = &ms->m[ms->n];
    memset(m, 0, sizeof *m);
    if (!(m->name = dupn(l, f - l))) {
        *oom = 1;
        return NULL;
    }
    ms->n++;
    return m;
}

static int add_sym(struct member *m, unsigned long v, const char *name, size_t n)
{
    struct sym *s = realloc(m->syms, (m->nsyms + 1) * sizeof *s);

    if (!s)
        return -1;
    m->syms = s;
    s[m->nsyms].value = v;
    if (!(s[m->nsyms].name = dupn(name, n)))
        return -1;
    m->nsyms++;
    return 0;
}

static int add_insn(struct member *m, unsigned long off, const char *t, size_t n)
{
    struct insn *s = realloc(m->ins, (m->nins + 1) * sizeof *s);

    if (!s)
        return -1;
    m->ins = s;
    s[m->nins].off = off;
    if (!(s[m->nins].text = dupn(t, n)))
        return -1;
    m->nins++;
    return 0;
}

/* objdump -t: "0000005e g       .text      0000 01 _dopoke" */
static int read_symtab(struct members *ms, const char *t)
{
    struct member *cur = NULL;
    int oom = 0;

    while (*t) {
        const char *e = strchr(t, '\n');
        size_t n = e ? (size_t)(e - t) : strlen(t);
        struct member *m = member_line(ms, t, n, &oom);

        if (oom)
            return -1;
        if (m)
            cur = m;
        else if (cur && n > 9 && isxdigit((unsigned char)t[0])) {
            char *end;
            unsigned long v = strtoul(t, &end, 16);
            const char *p = end, *sec = NULL, *name = NULL;
            size_t secn = 0, namen = 0;

            while (p < t + n) {          /* the tokens: flags, section, ..., name */
                const char *q;

                while (p < t + n && isspace((unsigned char)*p))
                    p++;
                q = p;
                while (p < t + n && !isspace((unsigned char)*p))
                    p++;
                if (p == q)
                    break;
                if (!sec && (*q == '.' || *q == '*')) {
                    sec = q;
                    secn = (size_t)(p - q);
                }
                name = q;
                namen = (size_t)(p - q);
            }
            if (sec && secn == 5 && !memcmp(sec, ".text", 5) && name && name != sec &&
                add_sym(cur, v, name, namen))
                return -1;
        }
        t += n + (e != NULL);
    }
    return 0;
}

/* objdump -d: "   2a:\tmoveb a0@(0,a1:l),d1" in "Disassembly of section .text:" */
static int read_disasm(struct members *ms, const char *t)
{
    struct member *cur = NULL;
    int text = 0, oom = 0;

    while (*t) {
        const char *e = strchr(t, '\n');
        size_t n = e ? (size_t)(e - t) : strlen(t);
        struct member *m = member_line(ms, t, n, &oom);

        if (oom)
            return -1;
        if (m) {
            cur = m;
            text = 0;
        } else if (n > 23 && !memcmp(t, "Disassembly of section ", 23))
            text = n == 29 && !memcmp(t + 23, ".text:", 6);
        else if (cur && text) {
            const char *p = t, *tab;
            unsigned long off;
            char *end;

            while (p < t + n && *p == ' ')
                p++;
            off = strtoul(p, &end, 16);
            tab = memchr(t, '\t', n);
            if (end > p && *end == ':' && tab) {
                const char *q = tab + 1, *z = t + n;

                while (z > q && isspace((unsigned char)z[-1]))
                    z--;
                if (add_insn(cur, off, q, (size_t)(z - q)))
                    return -1;
            }
        }
        t += n + (e != NULL);
    }
    return 0;
}

/* the mnemonic of an instruction (up to the first blank) */
static size_t mnemonic(const char *t, char *buf, size_t size)
{
    size_t n = 0;

    while (t[n] && t[n] != ' ' && n + 1 < size) {
        buf[n] = t[n];
        n++;
    }
    buf[n] = 0;
    return n;
}

/* a branch, call or jump: bCC/bra/bsr (with size), dbCC, jsr, jmp, jbsr, jra, jCC */
static int is_branch(const char *mn)
{
    static const char *const cc[] = { "ra", "sr", "hi", "ls", "cc", "cs", "ne", "eq", "vc",
                                      "vs", "pl", "mi", "ge", "lt", "gt", "le", "hs", "lo" };
    size_t i, n = strlen(mn);

    if (!strcmp(mn, "jsr") || !strcmp(mn, "jmp") || !strcmp(mn, "jbsr") || !strcmp(mn, "jbra"))
        return 1;
    if (n >= 3 && mn[0] == 'd' && mn[1] == 'b')
        return 1;
    if (n < 3 || (mn[0] != 'b' && mn[0] != 'j'))
        return 0;
    for (i = 0; i < sizeof cc / sizeof cc[0]; i++)
        if (mn[1] == cc[i][0] && mn[2] == cc[i][1] &&
            (n == 3 || (n == 4 && mn[0] == 'b' && strchr("swl", mn[3]))))
            return 1;
    return 0;
}

/* an unconditional jump (no call) */
static int is_jump(const char *mn)
{
    return !strncmp(mn, "bra", 3) || !strcmp(mn, "jmp") || !strcmp(mn, "jra") ||
           !strcmp(mn, "jbra");
}

static const struct sym *find_sym(const struct member *m, const char *name, size_t n)
{
    size_t i;

    for (i = 0; i < m->nsyms; i++)
        if (strlen(m->syms[i].name) == n && !memcmp(m->syms[i].name, name, n))
            return &m->syms[i];
    return NULL;
}

/* the target of a branch "bras 36 <_name+0x4>": the address of the symbol
   of this member plus the offset; 0 = no target in this member's .text */
static int branch_target(const struct member *m, const char *t, unsigned long *to)
{
    const char *lt = strchr(t, '<'), *gt, *plus;
    const struct sym *s;
    unsigned long off = 0;

    if (!lt || !(gt = strchr(lt, '>')))
        return 0;
    plus = memchr(lt, '+', (size_t)(gt - lt));
    s = find_sym(m, lt + 1, (size_t)((plus ? plus : gt) - lt - 1));
    if (!s)
        return 0;
    if (plus)
        off = strtoul(plus + 1, NULL, 16);
    *to = s->value + off;
    return 1;
}

/* the register set of a register list operand ("d2-d7/a2/a6"): bit 0-7
   d0-d7, 8-15 a0-a7; 0 if the operand is no register list */
static unsigned reglist(const char *s, size_t n)
{
    unsigned m = 0;
    size_t i = 0;

    while (i < n) {
        int lo, hi;

        if (i + 1 >= n || (s[i] != 'd' && s[i] != 'a') || s[i + 1] < '0' || s[i + 1] > '7')
            return 0;
        lo = (s[i] == 'a' ? 8 : 0) + (s[i + 1] - '0');
        hi = lo;
        i += 2;
        if (i < n && s[i] == '-') {
            if (i + 2 >= n + 0 || (s[i + 1] != 'd' && s[i + 1] != 'a') || s[i + 2] < '0' ||
                s[i + 2] > '7')
                return 0;
            hi = (s[i + 1] == 'a' ? 8 : 0) + (s[i + 2] - '0');
            i += 3;
        }
        if (hi < lo)
            return 0;
        for (; lo <= hi; lo++)
            m |= 1u << lo;
        if (i < n) {
            if (s[i] != '/')
                return 0;
            i++;
        }
    }
    return m;
}

#define A6 (1u << 14)

/* the operands of an instruction: the text behind the mnemonic, split at
   its last comma outside parentheses into source and destination */
static void operands(const char *t, const char **src, size_t *srcn, const char **dst,
                     size_t *dstn)
{
    const char *p = strchr(t, ' '), *comma = NULL;
    int depth = 0;
    const char *q;

    *src = *dst = "";
    *srcn = *dstn = 0;
    if (!p)
        return;
    p++;
    for (q = p; *q; q++) {
        if (*q == '(')
            depth++;
        else if (*q == ')')
            depth--;
        else if (*q == ',' && !depth)
            comma = q;
    }
    if (!comma) {
        *dst = p;
        *dstn = strlen(p);
        return;
    }
    *src = p;
    *srcn = (size_t)(comma - p);
    *dst = comma + 1;
    *dstn = strlen(comma + 1);
}

/* whether the instruction writes a6 */
static int writes_a6(const char *t)
{
    char mn[16];
    const char *src, *dst;
    size_t srcn, dstn;

    mnemonic(t, mn, sizeof mn);
    if (strstr(t, "a6@+") || strstr(t, "a6@-"))
        return 1;
    operands(t, &src, &srcn, &dst, &dstn);
    if (!strncmp(mn, "link", 4) || !strncmp(mn, "unlk", 4))
        return strstr(t, "a6") != NULL;
    if (!strcmp(mn, "exg"))
        return (srcn == 2 && !memcmp(src, "a6", 2)) || (dstn == 2 && !memcmp(dst, "a6", 2));
    if (!strncmp(mn, "movem", 5))
        return (reglist(dst, dstn) & A6) != 0;
    if (!strncmp(mn, "pea", 3) || !strncmp(mn, "tst", 3) || !strncmp(mn, "cmp", 3) ||
        !strncmp(mn, "btst", 4) || is_branch(mn))
        return 0;
    return dstn == 2 && !memcmp(dst, "a6", 2);
}

/* whether the instruction saves a6 on the stack (movel a6,sp@- or a
   moveml with a6 to sp@-) */
static int saves_a6(const char *t)
{
    char mn[16];
    const char *src, *dst;
    size_t srcn, dstn;

    mnemonic(t, mn, sizeof mn);
    operands(t, &src, &srcn, &dst, &dstn);
    if (dstn != 4 || memcmp(dst, "sp@-", 4))
        return 0;
    if (!strncmp(mn, "movem", 5))
        return (reglist(src, srcn) & A6) != 0;
    return !strncmp(mn, "move", 4) && srcn == 2 && !memcmp(src, "a6", 2);
}

/* the end of the routine at start: the next symbol of .text above it, or
   behind the last instruction */
static unsigned long routine_end(const struct member *m, unsigned long start)
{
    unsigned long end = (unsigned long)-1;
    size_t i;

    for (i = 0; i < m->nsyms; i++)
        if (m->syms[i].value > start && m->syms[i].value < end)
            end = m->syms[i].value;
    if (end == (unsigned long)-1)
        end = m->nins ? m->ins[m->nins - 1].off + 1 : start + 1;
    return end;
}

static long check_member(const struct member *m, FILE *report, long *nroutines)
{
    size_t lm = strlen(EPCHECK_MARKER), i, j;
    long found = 0;

    for (i = 0; i < m->nsyms; i++) {
        const char *name = m->syms[i].name;
        unsigned long start = m->syms[i].value, end;
        int first = 1, saved = 0;

        if (strncmp(name, EPCHECK_MARKER, lm))
            continue;
        name += lm;
        end = routine_end(m, start);
        if (nroutines)
            (*nroutines)++;
        for (j = 0; j < m->nins; j++) {
            const struct insn *in = &m->ins[j];
            char mn[16];
            unsigned long to;

            mnemonic(in->text, mn, sizeof mn);
            if (in->off & 1)
                continue;               /* data in the code (texts): no instruction */
            if (in->off < start || in->off >= end) {
                /* rule 2: a call or jump into the routine from elsewhere */
                if (is_branch(mn) && branch_target(m, in->text, &to) && to >= start &&
                    to < end) {
                    fprintf(report, "%s: %lx: %s: a call or jump into the __entrypoint "
                            "routine %s, which keeps no d2-d7/a2 for a caller (a C call, or "
                            "identical code folding of a function with the same body: "
                            "BLITZ_ENTRYPOINT's no_icf missing?)\n",
                            m->name, in->off, in->text, name);
                    found++;
                }
                continue;
            }
            if (first) {
                saved = saves_a6(in->text);
                /* rule 3: folded into another routine */
                if (is_jump(mn) && branch_target(m, in->text, &to) && (to < start || to >= end)) {
                    fprintf(report, "%s: %s: the __entrypoint routine begins with a jump "
                            "into another routine (%lx: %s): identical code folding\n",
                            m->name, name, in->off, in->text);
                    found++;
                }
                first = 0;
            }
            /* rule 1: a6 written without a save at the start */
            if (!saved && writes_a6(in->text)) {
                fprintf(report, "%s: %s: the __entrypoint routine writes a6 without saving "
                        "it at its start (%lx: %s); the register rule needs a6 kept\n",
                        m->name, name, in->off, in->text);
                found++;
            }
        }
        if (first) {
            fprintf(report, "%s: %s: no code of the __entrypoint routine found\n", m->name,
                    name);
            found++;
        }
    }
    return found;
}

long epcheck_text(const char *symtab, const char *disasm, FILE *report, long *nroutines)
{
    struct members ms = { NULL, 0 };
    long found = 0;
    size_t i;

    if (nroutines)
        *nroutines = 0;
    if (read_symtab(&ms, symtab) || read_disasm(&ms, disasm)) {
        free_members(&ms);
        return -1;
    }
    for (i = 0; i < ms.n; i++)
        found += check_member(&ms.m[i], report, nroutines);
    free_members(&ms);
    return found;
}
