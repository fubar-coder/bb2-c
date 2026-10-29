/*
 * sysload.h - shared code of the systest* programs (header only):
 *   callregs()  call a routine with all registers (+ optional stack arg)
 *   loader      load NAME.orig / NAME.obj like the compiler does: relocate,
 *               resolve ALibJsr placeholders against the same set
 *   checks      check(), markregs(), kept(), cmp_preserve()
 *
 * Usage in a test program:
 *   static const char *names[] = { "memlib", "mylib", NULL };
 *   load_sets(dir, names) -> sets[0] (originals), sets[1] (C build)
 *   run the same scenario on both, record struct call / values, compare.
 */
#ifndef SYSLOAD_H
#define SYSLOAD_H

#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ------------------------------------------------------------- register calls */

struct regs { ULONG d[8]; ULONG a[7]; };

/* callregs(fn, r, arg, pusharg): push `arg` (if pusharg), load d0-d7/a0-a6
   from r, call fn, store all registers back, pop arg.  Returns the CCR
   (N=8, Z=4, C=1).  Defined once in callregs.c. */
extern UWORD callregs(APTR fn, struct regs *r, ULONG arg, ULONG pusharg);
#define CCR_Z 4

/* ------------------------------------------------------------- loader */

#define MAXLIBS 16

struct lib {
    const char *name;
    UBYTE *code;                    /* relocated copy */
    ULONG size;
    ULONG *relocs;
    ULONG nrelocs;
    UWORD num, first, last;
};

struct set {
    const char *what;               /* "orig" / "C" */
    struct lib lib[MAXLIBS];
    int n;
};

static ULONG L(const UBYTE *p) { return (ULONG)p[0] << 24 | (ULONG)p[1] << 16 | p[2] << 8 | p[3]; }
static UWORD W(const UBYTE *p) { return (UWORD)(p[0] << 8 | p[1]); }

static BOOL loadfile(struct lib *l, const char *path)
{
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
    static UBYTE buf[65536];
    LONG len;
    ULONG words, p, i;

    if (!fh) {
        printf("cannot open %s\n", path);
        return FALSE;
    }
    len = Read(fh, buf, sizeof buf);
    Close(fh);
    /* fixed layout (loadalib): 32 byte header, code, HUNK_RELOC32 n 0 offs */
    words = L(buf + 28);
    l->size = words * 4;
    l->code = AllocMem(l->size, MEMF_PUBLIC);
    memcpy(l->code, buf + 32, l->size);
    p = 32 + l->size;
    l->nrelocs = (L(buf + p) == 0x3ec) ? L(buf + p + 4) : 0;
    l->relocs = malloc(4 * (l->nrelocs + 1));
    for (i = 0; i < l->nrelocs; i++)
        l->relocs[i] = L(buf + p + 12 + 4 * i);
    (void)len;
    l->num = W(l->code + 4);
    l->first = W(l->code + 14);
    l->last = W(l->code + 16);
    return TRUE;
}

/* address of the code of entry `idx`: a nullsub, or form `form` of a
   statement/function/command (ALibJsr placeholder $8000|form); NULL for
   a dumtoke, an AmigaOS call or a form the entry does not have */
static UBYTE *entry_form(struct lib *l, int idx, int form)
{
    UBYTE *b = l->code, *p = b + 0x30;
    int i = 0;
    while (!(W(p) & 0x8000)) {
        UWORD kind = W(p);
        UBYTE *code = NULL;
        p += 6;
        if (kind == 8) {                        /* dumtoke */
            p += 6;
            while (*p++) ;
            while (*p++) ;
            p = b + ((p - b + 1) & ~1);
        } else if (kind & 4) {                  /* AmigaOS call */
            p += 4;
            while (*p++ != 0xff) ;
            p = b + ((p - b + 1) & ~1);
            p += 6;
            while (*p++) ;
            while (*p++) ;
            p = b + ((p - b + 1) & ~1);
        } else if ((kind & 0xf) == 0) {         /* nullsub */
            while (W(p)) p += 4;
            p += 2;
            if (form == 0)
                code = (UBYTE *)L(p + 4);
            p += 12;
        } else {                                /* forms, name block */
            int f = 0;
            while (W(p) != 0xffff) {
                p += 2 + (W(p) & 0xff);
                p = b + ((p - b + 1) & ~1);
                while (W(p)) p += 4;
                p += 2;
                if (f++ == form)
                    code = (UBYTE *)L(p + 4);
                p += 12;
            }
            p += 2 + 6;                         /* $FFFF, next.l, num.w */
            while (*p++) ;
            while (*p++) ;
            p = b + ((p - b + 1) & ~1);
        }
        if (i == idx)
            return code;
        i++;
    }
    return NULL;
}

/* address of the code of entry `idx` (form 0; a nullsub) */
static __attribute__((unused)) UBYTE *entry_code(struct lib *l, int idx)
{
    return entry_form(l, idx, 0);
}

/* optional stubs for tokens of libraries the test does not load (e.g. a
   fake runerrlib getvbr): { token, code }, ended by { 0, NULL }.  Set
   sysload_stubs before load_sets(); the same stub is used for both sets. */
struct stub { UWORD tok; APTR code; };
static const struct stub *sysload_stubs;

static UBYTE *find_token(struct set *s, UWORD tok, int form)
{
    const struct stub *st;
    int i;
    for (i = 0; i < s->n; i++) {
        struct lib *l = &s->lib[i];
        UWORD first = l->first, last = l->last;
        if (!(l->num & 0x8000)) {               /* user library: libnum*128+1 */
            first = (UWORD)((l->num << 7) + 1);
            last = (UWORD)(first + 127);
        }
        if (first <= tok && tok < last)
            return entry_form(l, tok - first, form);
    }
    for (st = sysload_stubs; st && st->code; st++)
        if (st->tok == tok)
            return st->code;
    return NULL;
}

/* relocate like blitz2.s uselib: add the base; placeholders with bit 15
   ($8000|form, token) are resolved to the routine (handlejsrs/tokecode) */
static BOOL link_set(struct set *s)
{
    int i;
    ULONG r;
    for (i = 0; i < s->n; i++) {
        struct lib *l = &s->lib[i];
        for (r = 0; r < l->nrelocs; r++) {
            ULONG *at = (ULONG *)(l->code + l->relocs[r]);
            if (*at & 0x80000000)
                continue;
            *at += (ULONG)l->code;
        }
    }
    for (i = 0; i < s->n; i++) {
        struct lib *l = &s->lib[i];
        for (r = 0; r < l->nrelocs; r++) {
            ULONG *at = (ULONG *)(l->code + l->relocs[r]);
            if (*at & 0x80000000) {
                UBYTE *to = find_token(s, (UWORD)*at, (int)((*at >> 16) & 0x3fff));
                if (!to) {
                    printf("%s %s: token $%04lX not found\n", s->what, l->name,
                           (ULONG)(UWORD)*at);
                    return FALSE;
                }
                /* bit 0 of a routine's address: Amiga mode only
                   (`!subs {code+1,...}`, blitz2.s); a program never jumps
                   there directly, the compiler makes a stub that calls
                   the even address (tokecode/cutejsr) - so the even one */
                *at = (ULONG)to & ~1UL;
            }
        }
    }
    CacheClearU();
    return TRUE;
}

static struct lib *getlib(struct set *s, const char *name)
{
    int i;
    for (i = 0; i < s->n; i++)
        if (!strcmp(s->lib[i].name, name))
            return &s->lib[i];
    return NULL;
}

/* ------------------------------------------------------------- checks */

static int failures, checks;

#ifdef BL_SNITCH
/* every check is also reported to snitch (libs_tests.cpp) */
void bl_test_check(const char *what, int ok);
#endif

static void check(const char *what, BOOL ok)
{
    checks++;
    if (!ok) {
        failures++;
        printf("FAIL %s\n", what);
    }
#ifdef BL_SNITCH
    bl_test_check(what, ok);
#endif
}

#define MARK(n) (0x5A000000UL | ((n) << 16) | 0x1234)

/* fill all registers with markers, then set inputs */
static void markregs(struct regs *r)
{
    int i;
    for (i = 0; i < 8; i++) r->d[i] = MARK(i);
    for (i = 0; i < 7; i++) r->a[i] = MARK(8 + i);
}

/* bit n set = register n (d0..d7 = 0..7, a0..a6 = 8..14) unchanged */
static ULONG kept(const struct regs *before, const struct regs *after)
{
    ULONG m = 0;
    int i;
    for (i = 0; i < 8; i++) if (before->d[i] == after->d[i]) m |= 1UL << i;
    for (i = 0; i < 7; i++) if (before->a[i] == after->a[i]) m |= 1UL << (8 + i);
    return m;
}

/* one routine call record per set, compared afterwards */
struct call {
    struct regs in, out;
    UWORD ccr;
};

static void cmp_preserve(const char *what, struct call *o, struct call *c)
{
    ULONG ko = kept(&o->in, &o->out), kc = kept(&c->in, &c->out);
    char buf[200];
    snprintf(buf, sizeof buf, "%s: C build changes registers the original keeps (mask $%04lX)",
             what, ko & ~kc);
    check(buf, (ko & ~kc) == 0);
}


static struct set sets[2];

/* load and link both sets: sets[0] = NAME.orig, sets[1] = NAME.obj */
static BOOL load_sets(const char *dir, const char **names)
{
    char path[256];
    int s, i;
    for (s = 0; s < 2; s++) {
        sets[s].what = s ? "C" : "orig";
        for (i = 0; names[i]; i++) {
            sprintf(path, "%s/%s.%s", dir, names[i], s ? "obj" : "orig");
            sets[s].lib[i].name = names[i];
            if (!loadfile(&sets[s].lib[i], path))
                return FALSE;
        }
        sets[s].n = i;
        if (!link_set(&sets[s]))
            return FALSE;
    }
    return TRUE;
}

#endif
