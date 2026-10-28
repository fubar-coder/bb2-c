/*
 * bugorig - tests of reproduced original bugs (BUG(orig)): they check the
 * CORRECT behaviour and therefore fail as long as the C build reproduces
 * the original.  Hidden snitch test cases (tag [.bug-orig], libs_tests.cpp)
 * run by the ctest tests bug_orig_* (label bug-orig, test preset
 * bug-orig), never by libs_emulator.  Only the C build (NAME.obj) is
 * loaded, the CD is not needed.
 *
 *   cliargs-parpath-cli       ParPath$ of a CLI argument (cliargslib)
 *   crunch-implode-overread   Implode reads behind its data (crunchlib)
 *   crunch-decrunch-corrupt   the decrunchers with corrupt data (crunchlib)
 *
 * crunch-implode-overread puts the data at the very end of the highest
 * memory region; what lies behind is not mapped, and with
 * log_illegal_mem Amiberry logs every access there.  The verdict is
 * emulator.cmake's (ILLEGAL_WATCH): this program prints the watched
 * address range and the code range of the library ("illegal-watch ...").
 */
/* not every helper of sysload.h is used here */
#pragma GCC diagnostic ignored "-Wunused-function"
#include "sysload.h"
#include <exec/execbase.h>
#include <exec/memory.h>

#define BLTZ 0x624c745aUL               /* d7 "bLtZ": started like from the editor */

static struct set cset;
static struct regs r;

/* load and link the C build of the named libraries */
static BOOL load_c(const char *dir, const char **names)
{
    char path[256];
    int i;

    cset.what = "C";
    for (i = 0; names[i]; i++) {
        sprintf(path, "%s/%s.obj", dir, names[i]);
        cset.lib[i].name = names[i];
        if (!loadfile(&cset.lib[i], path))
            return FALSE;
    }
    cset.n = i;
    return link_set(&cset);
}

/* code of form `form` of entry `idx` (a command / function, or a nullsub
   with form 0), walking the declarations like systest_cmdlibs' cmd_entry */
static UBYTE *cmd_form(struct lib *l, int idx, int form)
{
    UBYTE *b = l->code, *p = b + 0x30;
    int i = 0;
    while (!(W(p) & 0x8000)) {
        UWORD kind = W(p);
        p += 6;
        if ((kind & 0xf) == 0) {                /* nullsub */
            while (W(p)) p += 4;
            p += 2;
            if (i == idx && form == 0)
                return (UBYTE *)L(p + 4);
            p += 12;
        } else {
            int f = 0;
            if (kind != 8) {
                while (W(p) != 0xffff) {        /* forms */
                    p += 2 + (W(p) & 0xff);
                    p = b + ((p - b + 1) & ~1);
                    while (W(p)) p += 4;
                    p += 2;
                    if (i == idx && f == form)
                        return (UBYTE *)L(p + 4);
                    p += 12;
                    f++;
                }
                p += 2;
            }
            p += 6;                             /* next.l, num.w */
            while (*p++) ;                      /* name */
            while (*p++) ;                      /* help */
            p = b + ((p - b + 1) & ~1);
        }
        i++;
    }
    return NULL;
}

#define cmd_entry(l, idx) cmd_form(l, idx, 0)

static ULONG seed;
static ULONG rnd(void)
{
    seed = seed * 1103515245UL + 12345;
    return seed >> 8;
}

/* --------------------------------------------------- cliargs-parpath-cli */

/* ParPath$ of a CLI argument: the argument has no lock (0), the original
   hands it to NameFromLock without a check (error check off) and returns
   the name of lock 0 (the boot volume) + "/" + the argument.  Correct: an
   empty string (ParPath$ is for Workbench arguments only). */
int bugorig_cliargs_parpath_cli(const char *dir)
{
    static const char *names[] = { "memlib", "wbstartuplib", "cliargslib", NULL };
    static char line[] = "datei.txt\n";
    static UBYTE out[300];
    struct lib *wb, *cl;
    ULONG data;
    int type;

    failures = 0;
    if (!load_c(dir, names)) {
        check("load memlib, wbstartuplib, cliargslib (C build)", FALSE);
        return 20;
    }
    wb = getlib(&cset, "wbstartuplib");
    cl = getlib(&cset, "cliargslib");
    markregs(&r);
    callregs(entry_code(getlib(&cset, "memlib"), 7), &r, 0, 0);     /* memlib init */
    markregs(&r);
    r.a[0] = (ULONG)line;
    r.d[7] = BLTZ;
    callregs(entry_code(wb, 1), &r, 0, 0);                          /* wbstartuplib init */
    data = r.d[0];
    check("setup: one CLI argument (NumPars 1)", (LONG)L((UBYTE *)data + 2) == 1);
    for (type = 0; type <= 1; type++) {
        char buf[512];
        memset(out, 0xee, sizeof out);
        markregs(&r);
        r.a[0] = data;                  /* !libs {#wbstartuplib,#la0} */
        r.a[6] = (ULONG)DOSBase;        /* !libs {#doslib,#la6} */
        r.a[3] = (ULONG)out;            /* string buffer */
        r.d[0] = 1;                     /* Par# */
        r.d[1] = type;                  /* #type */
        callregs(cmd_entry(cl, 3), &r, 0, 0);                       /* ParPath$ */
        out[sizeof out - 1] = 0;
        if (r.d[0] < sizeof out - 1)
            out[r.d[0]] = 0;
        snprintf(buf, sizeof buf,
                 "ParPath$(1,%d) of the CLI argument \"datei.txt\" is empty "
                 "(BUG(orig): NameFromLock(0) - got length %ld \"%.200s\")",
                 type, (LONG)r.d[0], r.d[0] < sizeof out ? (char *)out : "?");
        check(buf, r.d[0] == 0);
    }
    markregs(&r);
    callregs(entry_code(wb, 2), &r, 0, 0);                          /* wbstartuplib finit */
    markregs(&r);
    callregs(entry_code(getlib(&cset, "memlib"), 8), &r, 0, 0);     /* memlib finit */
    return failures ? 5 : 0;
}

/* ---------------------------------------------- crunch-implode-overread */

#define IMPLEN 4096

static void fill(UBYTE *p, ULONG n)
{
    ULONG i;
    seed = 0x1234567;
    for (i = 0; i < n; i++)             /* t60_crunch kind 3: mixed */
        p[i] = (i % 97) < 50 ? (UBYTE)(rnd() & 3) : (UBYTE)(i * 7);
}

/* Implode on IMPLEN bytes that end exactly at the end of the highest
   memory region.  Correct: no read behind the data.  BUG(orig): the match
   search reads up to 254 bytes behind the data (emulator.cmake counts the
   logged illegal reads in the 256 bytes behind it); here only the round
   trip is checked. */
int bugorig_crunch_implode_overread(const char *dir)
{
    static const char *names[] = { "crunchlib", NULL };
    static UBYTE copy[IMPLEN];
    struct MemHeader *mh, *top = NULL;
    struct lib *l;
    UBYTE *buf, *end;
    LONG res;

    failures = 0;
    if (!load_c(dir, names)) {
        check("load crunchlib (C build)", FALSE);
        return 20;
    }
    l = getlib(&cset, "crunchlib");
    Forbid();
    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head; mh->mh_Node.ln_Succ;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ)
        if (!top || (ULONG)mh->mh_Upper > (ULONG)top->mh_Upper)
            top = mh;
    end = (UBYTE *)top->mh_Upper;
    buf = AllocAbs(IMPLEN, end - IMPLEN);
    Permit();
    if (!buf) {
        check("setup: IMPLEN bytes at the end of the highest memory region", FALSE);
        return 20;
    }
    fill(buf, IMPLEN);
    memcpy(copy, buf, IMPLEN);
    /* the probe: one read of the first byte behind the data from here
       (must be logged, else the range is mapped or the log is off) */
    (void)*(volatile UBYTE *)end;
    printf("illegal-watch %08lx %08lx %08lx %08lx\n", (ULONG)end, (ULONG)end + 256,
           (ULONG)l->code, (ULONG)l->code + l->size);
    printf("  Implode of %d bytes at $%08lX..$%08lX, crunchlib at $%08lX\n", IMPLEN,
           (ULONG)buf, (ULONG)end, (ULONG)l->code);
    fflush(stdout);
    markregs(&r);
    r.d[0] = (ULONG)buf;
    r.d[1] = IMPLEN;
    callregs(cmd_entry(l, 0), &r, 0, 0);                            /* Implode */
    res = (LONG)r.d[0];
    check("setup: Implode crunches the data", res > 0);
    markregs(&r);
    r.d[0] = (ULONG)buf;
    callregs(cmd_entry(l, 1), &r, 0, 0);                            /* Deplode */
    check("setup: Deplode gives the data back",
          r.d[0] == (ULONG)-1 && !memcmp(buf, copy, IMPLEN));
    FreeMem(buf, IMPLEN);
    return failures ? 5 : 0;
}

/* ---------------------------------------------- crunch-decrunch-corrupt */

/* the packed data of t60_crunch (made by a test encoder for TEXT) */
static const ULONG pp20[22] = {
    0x50503230, 0x090A0B0C, 0xDFFF8E08, 0x1FFFC740, 0x1BFFF8F0, 0x85FFFC64,
    0x42FFFE3A, 0x217FFF1B, 0x10BFFF8F, 0x885FFFC6, 0x242FFFE3, 0x92749402,
    0x900B9781, 0x10A1B3A5, 0xB1811301, 0x3193AB9D, 0xB1858DA5, 0x91810BA9,
    0xB38B8114, 0x9D012FFF, 0xFC000000, 0x00014019
};
static const ULONG crm1[25] = {
    0x43724D21, 0x00640000, 0x01400000, 0x00540001, 0xB9060003, 0xF0D00004,
    0x721C0008, 0xE4480011, 0xC8B00023, 0x91A00047, 0x23C0008E, 0x488018CC,
    0x6D0D8D2C, 0x440E8CAE, 0x6E840925, 0xC4084D8D, 0x2E8F4408, 0x4C2E6D2C,
    0x6406440C, 0x6E4EADCC, 0x6D0D8D2C, 0x440E8CAE, 0x6E840945, 0xC41FFB00,
    0x00090000
};
static const ULONG crm2[23] = {
    0x43724D32, 0x005C0000, 0x01400000, 0x004C7829, 0xF02F0CF8, 0x17C2BE0D,
    0xF0EF857E, 0x0DC610DB, 0xBF843CDA, 0xA172AF7C, 0xFADC6D3C, 0xE5E7D3B8,
    0xC003348D, 0xEAEB72B7, 0x5A2CB62B, 0x0D2A9348, 0x995CED94, 0x7A351689,
    0x43A0CB83, 0x574B4D8E, 0x85200E18, 0xA4418000, 0x00020000
};

#define TEXTLEN  320                    /* 10 x "Blitz Basic 2 crunchlib test X. " */
#define IMPSRC   2000                   /* Implode input for the IMP! data */
#define DESTMAX  IMPSRC
#define GUARD    4096                   /* checked bytes in front of / behind dest */
#define ABSORB   (1024 * 1024)          /* unchecked room for runaway writes */
#define SRCAREA  (DESTMAX + 64)
#define VARIANTS 100                    /* corrupt variants per format */
#define GUARDVAL 0xA5
/* the arena: [source][ABSORB][GUARD][dest][GUARD][ABSORB].  The source
   lies below the destination: CrMDecrunch copies a source that is not
   at least 14 + MinSecDist bytes below the destination to
   dest - MinSecDist first (in front of the destination, an issue of its
   own).  Wild writes land in the absorbers, not in the system, as long
   as they stay within ABSORB bytes. */
#define ARENA    (SRCAREA + ABSORB + GUARD + DESTMAX + GUARD + ABSORB)

enum { F_IMP, F_CRM, F_CRM2, F_PP20, NFORMATS };
static const char *const fname[NFORMATS] = { "Deplode (IMP!)", "CrMDecrunch (CrM!)",
                                             "CrMDecrunch (CrM2)", "PPDecrunch (PP20)" };

static UBYTE packed[NFORMATS][DESTMAX + 64];
static ULONG plen[NFORMATS], olen[NFORMATS], first[NFORMATS], last[NFORMATS];
static UBYTE text[TEXTLEN], impsrc[IMPSRC];

/* number of guard bytes in front of / behind the destination that changed */
static ULONG guards_changed(const UBYTE *dest, ULONG dlen)
{
    ULONG i, n = 0;
    for (i = 0; i < GUARD; i++)
        n += dest[-1 - (LONG)i] != GUARDVAL;
    for (i = 0; i < GUARD; i++)
        n += dest[dlen + i] != GUARDVAL;
    return n;
}

/* Deplode, CrMDecrunch and PPDecrunch with data whose id is right but
   whose contents are corrupt (VARIANTS deterministic variants per format,
   one to three bytes behind the header changed; the header with the
   lengths stays - CrMDecrunch's overlap copy with a crunched length < 4
   and its in-place form are issues of their own).  Correct: nothing is
   written outside the destination (GUARD bytes in front of and behind
   it).  BUG(orig): the decrunchers check nothing and write in front of or
   behind the destination.  The first such variant ends the format: what
   it wrote beyond the guards is not known, the next one could run into
   it (each format reports the number of the variant). */
int bugorig_crunch_decrunch_corrupt(const char *dir)
{
    static const char *names[] = { "crunchlib", NULL };
    struct lib *l;
    UBYTE *arena, *work, *dest;
    const UBYTE *expect;
    ULONG wrongok = 0;
    int f, v, i, bad;
    char buf[200];

    failures = 0;
    if (!load_c(dir, names)) {
        check("load crunchlib (C build)", FALSE);
        return 20;
    }
    l = getlib(&cset, "crunchlib");
    arena = AllocMem(ARENA, MEMF_PUBLIC);
    if (!arena) {
        check("setup: arena", FALSE);
        return 20;
    }
    work = arena;
    dest = arena + SRCAREA + ABSORB + GUARD;

    /* the formats: IMP! from Implode, the others from t60_crunch */
    for (i = 0; i < TEXTLEN; i++)
        text[i] = "Blitz Basic 2 crunchlib test "[i % 32 < 29 ? i % 32 : 0];
    for (i = 0; i < 10; i++) {
        text[i * 32 + 29] = (UBYTE)(65 + i);
        text[i * 32 + 30] = '.';
        text[i * 32 + 31] = ' ';
    }
    memcpy(packed[F_PP20], pp20, plen[F_PP20] = sizeof pp20);
    memcpy(packed[F_CRM], crm1, plen[F_CRM] = sizeof crm1);
    memcpy(packed[F_CRM2], crm2, plen[F_CRM2] = sizeof crm2);
    olen[F_PP20] = olen[F_CRM] = olen[F_CRM2] = TEXTLEN;
    first[F_PP20] = 8;                  /* id, efficiency */
    last[F_PP20] = plen[F_PP20] - 4;    /* the last long: length, skip bits */
    first[F_CRM] = first[F_CRM2] = 14;  /* id, MinSecDist, OrigLen, CrLen */
    last[F_CRM] = plen[F_CRM];
    last[F_CRM2] = plen[F_CRM2];
    fill(work, IMPSRC);
    memcpy(impsrc, work, IMPSRC);
    markregs(&r);
    r.d[0] = (ULONG)work;
    r.d[1] = IMPSRC;
    callregs(cmd_entry(l, 0), &r, 0, 0);                            /* Implode */
    plen[F_IMP] = r.d[0];
    check("setup: Implode for the IMP! data", (LONG)plen[F_IMP] > 0);
    if ((LONG)plen[F_IMP] <= 0)
        plen[F_IMP] = 12;
    memcpy(packed[F_IMP], work, plen[F_IMP]);
    olen[F_IMP] = IMPSRC;
    first[F_IMP] = 12;                  /* id, length, crunched length */
    last[F_IMP] = plen[F_IMP];

    for (f = 0; f < NFORMATS; f++) {
        printf("  %s: %d corrupt variants\n", fname[f], VARIANTS);
        fflush(stdout);                 /* a crash leaves this line */
        expect = f == F_IMP ? impsrc : text;
        seed = 0x5eed0000 + f;
        bad = -1;
        for (v = -1; v < VARIANTS && bad < 0; v++) {
            ULONG changed;
            memset(dest - GUARD, GUARDVAL, GUARD + DESTMAX + GUARD);
            memcpy(work, packed[f], plen[f]);
            if (v >= 0) {               /* variant -1: the intact data */
                int n = 1 + rnd() % 3;
                while (n--)
                    work[first[f] + rnd() % (last[f] - first[f])] ^= (UBYTE)(1 + rnd() % 255);
            }
            markregs(&r);
            switch (f) {
            case F_IMP:                 /* Deplode(buffer): in place */
                memcpy(dest, work, plen[f]);
                r.d[0] = (ULONG)dest;
                callregs(cmd_entry(l, 1), &r, 0, 0);
                if (v >= 0 && r.d[0] && memcmp(dest, expect, olen[f]))
                    wrongok++;
                break;
            case F_CRM:                 /* CrMDecrunch source,dest */
            case F_CRM2:
                r.d[0] = (ULONG)work;
                r.d[1] = (ULONG)dest;
                callregs(cmd_form(l, 2, 1), &r, 0, 0);
                break;
            case F_PP20:                /* PPDecrunch start,end,dest */
                r.d[0] = (ULONG)work;
                r.d[1] = (ULONG)work + plen[f];
                r.d[2] = (ULONG)dest;
                callregs(cmd_entry(l, 3), &r, 0, 0);
                break;
            }
            changed = guards_changed(dest, olen[f]);
            if (v < 0) {
                snprintf(buf, sizeof buf, "setup: %s of the intact data", fname[f]);
                check(buf, changed == 0 && !memcmp(dest, expect, olen[f]));
            } else if (changed) {
                bad = v;
                snprintf(buf, sizeof buf,
                         "%s writes nothing outside the destination (BUG(orig): corrupt "
                         "variant %d changed %lu of the %d guard bytes)",
                         fname[f], v, changed, 2 * GUARD);
                check(buf, FALSE);
            }
        }
        if (bad < 0)
            check("corrupt variants write nothing outside the destination", TRUE);
    }
    printf("  Deplode: %lu corrupt variants returned success with wrong data (no "
           "checksum in the format; not checked)\n", wrongok);
    FreeMem(arena, ARENA);
    return failures ? 5 : 0;
}
