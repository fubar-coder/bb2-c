/*
 * sysload.h - shared code of the systest* programs (header only):
 *   callregs()  call a routine with all registers (+ optional stack arg)
 *   loader      load NAME.orig / NAME.obj like the compiler does: relocate,
 *               resolve ALibJsr placeholders against the same set
 *   checks      check(), markregs(), kept(), cmp_preserve()
 *
 * Usage in a test program that compares two sets (not converted yet):
 *   static const char *names[] = { "memlib", "mylib", NULL };
 *   load_sets(dir, names) -> sets[0] (originals), sets[1] (C build)
 *   run the same scenario on both, record struct call / values, compare
 *   through tape.h.
 *
 * A program that observes one set (openspec change libs-tests-capture,
 * design D4, D7, "Converting a program") defines LIBS_ONE_SET before it
 * includes this header and loads the set with load_set(dir, names) ->
 * loaded: NAME.obj or NAME.orig, as the shell variable LIBSET of the run
 * says (bl_libset, libs_tests.cpp).  It writes every observation as a case
 * (capture.h) to captures/NAME.txt; the key is ROUT, the check text and
 * VEC (set by the program before the observations):
 *   OBS(what, v)                a number, written as it is ($XXXXXXXX)
 *   OBSPTR(what, v, name, base) an address: 0, prog (into the test
 *                               program), NAME+$OFF relative to base
 *   OBSBYTES(what, p, n)        n bytes (hex up to 64, longer: length, fnv)
 *   OBSPTRBYTES(what, p, n)     the same for a record holding pointers into
 *                               the test program (written as PROG)
 *   OBSSTR(what, s)             a string ("TEXT", NULL: null)
 *   NOT_CAPTURED(what, reason)  a part that cannot be reproduced
 *   OBS_BLOCKS(on)              block mode (only the block programs, D6)
 *   KEEPS(routine, d2|d3|a2)    the registers the routine keeps in every
 *                               call of the original (a4 and a5 always;
 *                               ranges like d2-d7; none); obs_kept(in, out)
 *                               after a call writes ROUT|kept|VEC: "kept"
 *                               or "lost d3 a1" for the stated set
 *   DONTCARE                    registers of results, exempt in the next
 *                               obs_kept (cleared by the program)
 *   kept_summary()              the kept registers of every routine in all
 *                               calls of this set, with the KEEPS line
 * check() stays for the program's own consistency checks.  The program
 * does not open its output: its first load_set() does (bl_prog_cases,
 * cap_begin), libs_tests.cpp ends it.
 */
#ifndef SYSLOAD_H
#define SYSLOAD_H

#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "capture.h"
#ifndef LIBS_ONE_SET
#include "tape.h"
#endif

/* the output of the program libs_tests.cpp runs (run_systest), begun by
   its first load_sets (its tape) or load_set (its cases), ended by
   run_systest after the program; nothing outside run_systest (bugorig.c,
   selftest_keeps.c) */
void bl_prog_tape(void);
void bl_prog_cases(void);

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

/* the library set of the run: "obj" (the C build) or "orig" (the
   original), from the shell variable LIBSET; NULL if it is not set
   (libs_tests.cpp) */
extern const char *bl_libset;

#ifndef LIBS_ONE_SET
static void cmp_preserve(const char *what, struct call *o, struct call *c)
{
    ULONG ko = kept(&o->in, &o->out), kc = kept(&c->in, &c->out);
    char buf[200];
    ko = tape_kept(what, ko, kc);
    snprintf(buf, sizeof buf, "%s: C build changes registers the original keeps (mask $%04lX)",
             what, ko & ~kc);
    check(buf, (ko & ~kc) == 0);
}


static struct set sets[2];

/* load and link both sets: sets[0] = NAME.orig, sets[1] = NAME.obj; the
   first call of a program libs_tests.cpp runs begins its tape */
static BOOL load_sets(const char *dir, const char **names)
{
    char path[256];
    int s, i;
    bl_prog_tape();
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

#else /* LIBS_ONE_SET */

/* the one set of the run (load_set) */
static struct set loaded;

/* load and link the one set of the run: NAME.<LIBSET> into `loaded`; the
   first call of a program libs_tests.cpp runs begins its cases
   (captures/NAME.txt) */
static BOOL load_set(const char *dir, const char **names)
{
    char path[256];
    int i;
    if (!bl_libset) {
        printf("LIBSET is not set (obj or orig)\n");
        return FALSE;
    }
    bl_prog_cases();
    loaded.what = !strcmp(bl_libset, "obj") ? "C" : "orig";
    for (i = 0; names[i]; i++) {
        sprintf(path, "%s/%s.%s", dir, names[i], bl_libset);
        loaded.lib[i].name = names[i];
        if (!loadfile(&loaded.lib[i], path))
            return FALSE;
    }
    loaded.n = i;
    return link_set(&loaded);
}

/* ------------------------------------------------------------- observations */

/* the key of the cases: the routine (part 1) and the vector (part 3, the
   inputs of the case); the check text is the `what` of each observation.
   Neither may hold a value or an address (that is the value's part). */
static const char *ROUT = "";
static char VEC[100];

#define OBS(what, v) do {                                                   \
        cap_routine(ROUT); cap_num((what), VEC, (ULONG)(v)); checks++;      \
    } while (0)
#define OBSPTR(what, v, name, base) do {                                    \
        cap_routine(ROUT);                                                  \
        cap_ptr((what), VEC, (ULONG)(v), (name), (ULONG)(base)); checks++;  \
    } while (0)
#define OBSBYTES(what, p, n) do {                                           \
        cap_routine(ROUT); cap_bytes((what), VEC, (p), (n)); checks++;      \
    } while (0)
#define OBSPTRBYTES(what, p, n) do {                                        \
        cap_routine(ROUT); cap_ptrbytes((what), VEC, (p), (n)); checks++;   \
    } while (0)
#define OBSSTR(what, s) do {                                                \
        cap_routine(ROUT); cap_str((what), VEC, (s)); checks++;             \
    } while (0)
#define NOT_CAPTURED(what, reason) do {                                     \
        cap_routine(ROUT); cap_not_captured((what), VEC, (reason));         \
    } while (0)
#define OBS_BLOCKS(on) cap_blocks(on)

/* ------------------------------------------------------------- kept registers */

/* KEEPS(routine, regs): the registers `routine` (a ROUT) keeps in every
   call of the scenario with the original (design D7: the intersection
   over the calls, from kept_summary's listing; never less than the
   convention's a4 and a5, which are always added).  regs is written as
   C text, registers d0-d7/a0-a6 joined by |, ranges like d2-d7, or none:
     KEEPS("FillArray", d2-d7|a2|a3|a6);
   obs_kept() writes for every call of a routine one case ROUT|kept|VEC:
   "kept", or "lost" with the stated registers the call changed. */
#define KEEPS(routine, regs) keeps_state((routine), #regs)

#define KEEPS_MAX 64
#define KEEPS_CONV ((1UL << 12) | (1UL << 13))  /* a4, a5 */
#define KEEPS_BAD 0x80000000UL

static struct keeps {
    const char *rout;
    int has;                            /* KEEPS stated */
    ULONG stated;                       /* the stated set, a4/a5 added */
    ULONG all;                          /* kept in all calls of this set */
    ULONG n;                            /* calls */
} keeps[KEEPS_MAX];
static int nkeeps;

/* registers bit 0-7 d0-d7, 8-14 a0-a6, set by DONTCARE: results of the
   next call, exempt from its kept case (cleared by the program) */
static ULONG DONTCARE;

static struct keeps *keeps_find(const char *rout)
{
    int i;
    for (i = 0; i < nkeeps; i++)
        if (!strcmp(keeps[i].rout, rout))
            return &keeps[i];
    if (nkeeps == KEEPS_MAX) {
        check("KEEPS: more than KEEPS_MAX routines", FALSE);
        return NULL;
    }
    keeps[nkeeps].rout = rout;
    keeps[nkeeps].has = 0;
    keeps[nkeeps].stated = 0;
    keeps[nkeeps].all = 0x7fff;
    keeps[nkeeps].n = 0;
    return &keeps[nkeeps++];
}

/* "d2|d3-d5|a2" -> mask; KEEPS_BAD for anything else */
static ULONG keeps_parse(const char *s)
{
    ULONG m = 0;
    while (*s) {
        int base, lo, hi;
        if (*s == ' ' || *s == '|') {
            s++;
            continue;
        }
        if (!strncmp(s, "none", 4)) {
            s += 4;
            continue;
        }
        if ((*s != 'd' && *s != 'a') || s[1] < '0' || s[1] > '7')
            return KEEPS_BAD;
        base = *s == 'a' ? 8 : 0;
        lo = hi = s[1] - '0';
        s += 2;
        while (*s == ' ')
            s++;
        if (*s == '-') {
            s++;
            while (*s == ' ')
                s++;
            if (*s != (base ? 'a' : 'd') || s[1] < '0' || s[1] > '7' || s[1] - '0' < lo)
                return KEEPS_BAD;
            hi = s[1] - '0';
            s += 2;
        }
        if (base && hi > 6)
            return KEEPS_BAD;
        for (; lo <= hi; lo++)
            m |= 1UL << (base + lo);
    }
    return m;
}

static __attribute__((unused)) void keeps_state(const char *rout, const char *regs)
{
    struct keeps *k = keeps_find(rout);
    ULONG m = keeps_parse(regs);
    char buf[160];
    if (!k)
        return;
    if (m == KEEPS_BAD || k->has) {
        snprintf(buf, sizeof buf, "KEEPS(\"%.40s\", %.60s): %s", rout, regs,
                 m == KEEPS_BAD ? "not a register set" : "stated twice");
        check(buf, FALSE);
        return;
    }
    k->has = 1;
    k->stated = m | KEEPS_CONV;
}

/* the registers of a mask as KEEPS text (d2-d7|a2) */
static void keeps_text(char *s, ULONG m)
{
    int r, n = 0;
    *s = 0;
    for (r = 0; r < 15; r++) {
        int e = r;
        if (!(m & (1UL << r)))
            continue;
        while (e + 1 < (r < 8 ? 8 : 15) && (m & (1UL << (e + 1))))
            e++;
        n += sprintf(s + n, "%s%c%d", n ? "|" : "", r < 8 ? 'd' : 'a', r & 7);
        if (e > r)
            n += sprintf(s + n, "%s%c%d", e > r + 1 ? "-" : "|", r < 8 ? 'd' : 'a', e & 7);
        r = e;
    }
    if (!n)
        strcpy(s, "none");
}

/* after a call of ROUT with the registers `in`: the case ROUT|kept|VEC
   for its stated set; a routine without KEEPS fails once */
static __attribute__((unused)) void obs_kept(const struct regs *in, const struct regs *out)
{
    struct keeps *k = keeps_find(ROUT);
    ULONG m = kept(in, out);
    char buf[120];
    if (!k)
        return;
    k->all &= m;
    k->n++;
    if (!k->has) {
        if (k->n == 1) {
            snprintf(buf, sizeof buf, "KEEPS: no kept set stated for \"%.60s\"", ROUT);
            check(buf, FALSE);
        }
        return;
    }
    cap_routine(ROUT);
    cap_regs("kept", VEC, k->stated & ~(m | DONTCARE));
    checks++;
}

/* the kept registers of every routine in all its calls with this set,
   the KEEPS line to state them (the set of the original's run is the one
   to state), and the stated set */
static __attribute__((unused)) void kept_summary(void)
{
    char t[64], st[64];
    int i;
    printf("kept registers in all calls (set %s), the KEEPS of the original's run:\n",
           loaded.what ? loaded.what : "?");
    for (i = 0; i < nkeeps; i++) {
        struct keeps *k = &keeps[i];
        keeps_text(t, k->all & ~KEEPS_CONV);
        if (k->has)
            keeps_text(st, k->stated & ~KEEPS_CONV);
        printf("  %5lu calls  $%04lX  KEEPS(\"%s\", %s);%s%s%s\n", k->n, k->all, k->rout, t,
               k->has ? "  stated: " : "  not stated", k->has ? st : "",
               (k->all & KEEPS_CONV) != KEEPS_CONV ? "  (a4/a5 changed: a result? DONTCARE)" : "");
    }
}

#endif /* LIBS_ONE_SET */

#endif
