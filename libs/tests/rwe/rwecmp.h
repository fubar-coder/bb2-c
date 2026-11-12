/*
 * rwecmp.h - the framework of the RWE userlib tests (libs/tests/rwe/NAME.c,
 * header only; the method of systest_cmdlibs.c): every routine of the one
 * library set of the run (`loaded`, sysload.h: NAME.obj, the C build, or
 * NAME.orig, the CD original, as LIBSET says) is called with the inputs of
 * the case, the other registers markers, and its observations are written
 * as cases (capture.h), compared with libs/tests/expected/NAME.txt:
 *
 *   ENT(lib, idx, form)   code of form `form` of entry `idx` of `lib` (0: none)
 *   ERR(lib, idx, form)   its error check routine (0: none)
 *   DO1(fn, setup)        call fn; `setup` sets R
 *   DO1S(fn, setup)       the same with stack arguments ARGS[0..NARGS-1]
 *   preserve()            after every DO1: the kept case (KEEPS of ROUT,
 *                         DONTCARE exempt) and OBS("popped", POP)
 *   OBS, OBSBYTES, ...    the observations (sysload.h)
 *   BOTH(what, expr)      OBS of the truth value (counted twice)
 *   fake_library() ...    a fake system library (requesters, storage)
 *   rwe_done()            summary, the kept registers with their KEEPS
 *                         lines (kept_summary), the exit status
 */
#ifndef RWECMP_H
#define RWECMP_H

#include "../sysload.h"
#include <exec/execbase.h>
#include "rwemem.h"

/* callstk() is defined in systest_mathstr.c (same executable) */
extern UWORD callstk(APTR fn, struct regs *r, const ULONG *args, ULONG nargs, ULONG *popped);

/* the registers of the call (R after it, IN before it), its stack
   arguments ARGS[0..NARGS-1], the bytes it popped and its CCR; ROUT and
   VEC (the key of the cases) are sysload.h's */
static struct regs R, IN;
static ULONG ARGS[6], POP;
static int NARGS;
static UWORD CCR;

/* bit 0 of the address = Amiga mode only (blitz2.s), not part of it */
#define ENT(lib, idx, form) \
    ((UBYTE *)((ULONG)entry_form(getlib(&loaded, lib), idx, form) & ~1UL))

/* entry_err(lib, idx, form): the error check routine (first !subs field)
   of form `form` of entry `idx` (NULL: none) - entry_form's walk */
static __attribute__((unused)) UBYTE *entry_err(struct lib *l, int idx, int form)
{
    UBYTE *b = l->code, *p = b + 0x30;
    int i = 0;
    while (!(W(p) & 0x8000)) {
        UWORD kind = W(p);
        UBYTE *err = NULL;
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
                err = (UBYTE *)L(p);
            p += 12;
        } else {                                /* forms, name block */
            int f = 0;
            while (W(p) != 0xffff) {
                p += 2 + (W(p) & 0xff);
                p = b + ((p - b + 1) & ~1);
                while (W(p)) p += 4;
                p += 2;
                if (f++ == form)
                    err = (UBYTE *)L(p);
                p += 12;
            }
            p += 2 + 6;
            while (*p++) ;
            while (*p++) ;
            p = b + ((p - b + 1) & ~1);
        }
        if (i == idx)
            return err;
        i++;
    }
    return NULL;
}
#define ERR(lib, idx, form) entry_err(getlib(&loaded, lib), idx, form)

/* a condition on the set's result: its truth value is written as a case,
   counted as two checks */
#define BOTH(what, expr) do { OBS((what), (expr) != 0); checks++; } while (0)

/* after a call: its kept case (the KEEPS of ROUT, DONTCARE exempt) and the
   bytes it popped */
static void preserve(void)
{
    obs_kept(&IN, &R);
    OBS("popped", POP);
}

/* call fn of the set: markers in all registers, `setup` sets R (and
   ARGS[0..], NARGS for stack arguments), then preserve() */
#define DO1S(fn, setup) do {                                                \
        APTR fn_;                                                           \
        markregs(&R);                                                       \
        NARGS = 0;                                                          \
        setup;                                                              \
        IN = R;                                                             \
        fn_ = (fn);                                                         \
        if (!fn_) {                                                         \
            check("entry missing", FALSE);                                  \
            break;                                                          \
        }                                                                   \
        CCR = callstk(fn_, &R, ARGS, NARGS, &POP);                          \
        preserve();                                                         \
    } while (0)
#define DO1(fn, setup) DO1S(fn, setup)

static ULONG rwe_seed = 0x13579bdf;
static __attribute__((unused)) ULONG rnd(void)
{
    rwe_seed = rwe_seed * 1103515245UL + 12345;
    return (rwe_seed >> 16) | ((rwe_seed * 69069UL + 1) & 0xffff0000UL);
}

/* fake_library(name, pri, vec, n): a library in the system list (AddLibrary,
   priority `pri` - before a real one of the same name with a higher one)
   with the functions vec[0..n-1] (vec[0] Open, [1] Close, ..., [k] = LVO
   -6*(k+1)); fake_open/fake_close count the opens.  fake_remove takes it
   out again. */
static __attribute__((unused)) struct Library *fake_open(struct Library *base __asm("a6"))
{
    base->lib_OpenCnt++;
    return base;
}

static __attribute__((unused)) ULONG fake_close(struct Library *base __asm("a6"))
{
    base->lib_OpenCnt--;
    return 0;
}

static __attribute__((unused)) ULONG fake_null(void)
{
    return 0;
}

static __attribute__((unused)) struct Library *fake_library(const char *name, int pri,
                                                             APTR *vec, int n)
{
    struct Library *l;
    int i;
    vec[n] = (APTR)-1;
    for (i = 0; i < n; i++)
        if (!vec[i])
            vec[i] = i == 0 ? (APTR)fake_open : i == 1 ? (APTR)fake_close : (APTR)fake_null;
    l = MakeLibrary(vec, NULL, NULL, sizeof(struct Library), 0);
    l->lib_Node.ln_Name = (char *)name;
    l->lib_Node.ln_Type = NT_LIBRARY;
    l->lib_Node.ln_Pri = pri;
    l->lib_Version = 99;
    AddLibrary(l);
    return l;
}

static __attribute__((unused)) void fake_remove(struct Library *l)
{
    Forbid();
    Remove(&l->lib_Node);
    Permit();
    FreeMem((UBYTE *)l - l->lib_NegSize, l->lib_NegSize + l->lib_PosSize);
}

/* a system call replaced by `moveq #0,d0:rts` (SetFunction), e.g. to let
   an init's OpenLibrary or AllocMem fail */
static const UWORD rwe_fail0[] = { 0x7000, 0x4e75 };

/* the summary, the kept registers of every routine with its KEEPS line
   (kept_summary), the exit status */
static int rwe_done(const char *name)
{
    rwe_mem_stop();                     /* the patches out on every way out */
    printf("%s: %d checks, %d failures\n", name, checks, failures);
    kept_summary();
    return failures ? 5 : 0;
}

#endif
