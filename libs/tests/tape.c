/*
 * tape - the recorded results of the original libraries (sysload.h,
 * tests/README.md "Libraries").
 *
 * Every comparison of the original (sets[0]) with the C build (sets[1])
 * passes its two observations through tape_obs() or one of its siblings
 * (tape.h).  The mode comes from the shell variable LIBSTAPE
 * (emulator.cmake sets it):
 *
 *   (unset)   live: the original's observation is compared as it is
 *   record    live, and the original's observations are written to
 *             DIR/NAME.tape (libs_emulator_originals)
 *   replay    no original: sets[0] is the C build as well, the C build's
 *             observations are compared with DIR/NAME.tape
 *             (libs_emulator, needs no CD)
 *
 * A recording holds hashes (FNV-1a) of segments of TAPE_SEG observations
 * in the order of the checks and their number, no values of the
 * original except two kinds: the original's value of each check with an
 * exemption (expect_fixed), which the fixed check compares with, and the
 * registers the original keeps after a call (tape_kept) where the
 * prediction misses them.  Lines:
 *
 *   seg IDX HASH          segment IDX (observations IDX*TAPE_SEG ...)
 *   fixed SEQ VALUE       observation SEQ: the original's value
 *   k ENTRY...            kept registers: ENTRY is DELTA or DELTA/MASK
 *   end COUNT HASH        all observations; HASH of the last part segment
 *
 * The prediction of the kept registers (bit 0-7 d0-d7, 8-14 a0-a6) of a
 * call: the mask last seen for the C build's mask of the call and the
 * routine (the check's text up to its first blank), at first the C
 * build's mask.  A k ENTRY names a call it misses: the observation
 * DELTA behind the one of the previous ENTRY (the first: behind 0), the
 * original's MASK, or without /MASK the mask seen before the last one
 * (routines whose kept registers depend on the data alternate).
 *
 * In replay a check compares the C build's value with itself (it passes)
 * and the segment's hash is the verdict: a segment that differs fails
 * with the range of its observations and the first check's text (the
 * live comparison under the label originals shows the values).
 */
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <exec/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tape.h"

#define TAPE_SEG 1024

void bl_test_check(const char *what, int ok);

int tape_mode;
static int paused;                      /* tape_pause */
static char tname[64], tpath[256];
static ULONG seq, hash, segno;
static char first[100];
static int broken;                      /* the recording unusable */
static void find_hunks(void);

/* the recording: written (record) or read (replay); static, so that the
   memory balance of the tests stays as it is */
#define TAPE_MAX (512 * 1024)
static char buf[TAPE_MAX];
static ULONG len, pos;

/* the prediction of the kept registers, by a hash of the routine and the
   C build's mask: the last mask and the one before it */
#define NPREDICT 4096
static struct { ULONG key; UWORD mask, alt; } predict[NPREDICT];
static ULONG kseq;                      /* the observation of the last k ENTRY */
static char kline[96];                  /* record: the k line being written */
static const char *ktok;                /* replay: the next ENTRY of the k line */

static void put(const char *s)
{
    ULONG n = strlen(s);
    if (len + n + 1 > TAPE_MAX) {
        broken = 1;
        return;
    }
    memcpy(buf + len, s, n + 1);
    len += n;
}

/* record: the k line out, before any other line */
static void flush_k(void)
{
    if (kline[0]) {
        strcat(kline, "\n");
        put(kline);
        kline[0] = 0;
    }
}

static void put_line(const char *s)
{
    flush_k();
    put(s);
}

/* the next line of the recording (replay), NULL at its end */
static const char *next_line(void)
{
    static char line[100];
    ULONG n = 0;
    if (pos >= len)
        return NULL;
    while (pos < len && buf[pos] != '\n') {
        if (n + 1 < sizeof line)
            line[n++] = buf[pos];
        pos++;
    }
    pos++;
    line[n] = 0;
    return line;
}

/* the next line that is not a comment */
static const char *next_entry(void)
{
    const char *l;
    while ((l = next_line()) && l[0] == '#')
        ;
    return l;
}

/* replay: the next k ENTRY if it names observation seq (it is taken):
   1 with *mask, 2 without (the mask before the last one); else 0 */
static int take_kept(ULONG *mask)
{
    ULONG d;
    int n;

    if (!ktok || !*ktok) {
        ULONG save = pos;
        const char *e = next_entry();
        if (!e || e[0] != 'k' || e[1] != ' ') {
            pos = save;
            ktok = NULL;
            return 0;
        }
        ktok = e + 1;
    }
    if (sscanf(ktok, " %lu%n", &d, &n) != 1 || kseq + d != seq)
        return 0;
    kseq = seq;
    ktok += n;
    if (*ktok == '/') {
        sscanf(ktok + 1, "%lx%n", mask, &n);
        ktok += 1 + n;
        return 1;
    }
    return 2;
}

static void fault(const char *why)
{
    char m[200];
    snprintf(m, sizeof m, "tape %s: %s", tname, why);
    printf("FAIL %s\n", m);
    bl_test_check(m, 0);
}

static void reset_seg(void)
{
    hash = 0x811c9dc5UL;
    first[0] = 0;
}

static void mix(const UBYTE *p, ULONG n)
{
    while (n--) {
        hash ^= *p++;
        hash *= 0x01000193UL;
    }
}

static void differs(void)
{
    char m[200];
    snprintf(m, sizeof m, "observations %lu..%lu differ from the original's"
             " recording (first: %s)", segno * TAPE_SEG, seq - 1, first);
    fault(m);
}

/* the end of a segment: write its hash or compare it */
static void close_seg(int last)
{
    char l[80];
    ULONG idx, h, n;
    const char *e;

    if (tape_mode == TAPE_RECORD) {
        if (last)
            snprintf(l, sizeof l, "end %lu %08lx\n", seq, hash);
        else
            snprintf(l, sizeof l, "seg %lu %08lx\n", segno, hash);
        put_line(l);
    } else if (tape_mode == TAPE_REPLAY && !broken) {
        if (ktok && *ktok) {
            fault("the kept registers differ from the original's recording");
            broken = 1;
        } else if (!(e = next_entry())) {
            fault("more observations than the recording has");
            broken = 1;
        } else if (last) {
            if (sscanf(e, "end %lu %lx", &n, &h) != 2) {
                snprintf(l, sizeof l, "%lu observations, the recording has more", seq);
                fault(l);
                broken = 1;
            } else if (n != seq) {
                snprintf(l, sizeof l, "%lu observations, the recording has %lu", seq, n);
                fault(l);
            } else if (h != hash)
                differs();
        } else if (sscanf(e, "seg %lu %lx", &idx, &h) != 2 || idx != segno) {
            fault("more observations than the recording has, or other kept registers");
            broken = 1;
        } else if (h != hash)
            differs();
        ktok = NULL;
    }
    segno++;
    reset_seg();
}

/* LIBSTAPEDUMP set: the first DUMP_MAX observations are written to
   DIR/NAME.dump as well ("SEQ VALUE TEXT", bytes as their hash), to find
   the check of a segment that differs (compare a recording run's and a
   replay's dump) */
#define DUMP_MAX 20000
static char dump[DUMP_MAX * 48];
static ULONG dlen;
static int dumping;

static void count(const char *what, ULONG v)
{
    if (!first[0])
        snprintf(first, sizeof first, "%s", what ? what : "?");
    if (dumping && seq < DUMP_MAX && dlen + 48 < sizeof dump)
        dlen += snprintf(dump + dlen, 48, "%lu %08lx %.28s\n", seq, v, what ? what : "?");
    seq++;
    if (seq % TAPE_SEG == 0)
        close_seg(0);
}

void tape_begin(const char *dir, const char *name)
{
    char mode[16];
    BPTR fh;

    tape_mode = TAPE_LIVE;
    if (GetVar((STRPTR)"LIBSTAPE", (STRPTR)mode, sizeof mode, 0) > 0) {
        if (!strcmp(mode, "record"))
            tape_mode = TAPE_RECORD;
        else if (!strcmp(mode, "replay"))
            tape_mode = TAPE_REPLAY;
    }
    dumping = tape_mode != TAPE_LIVE
              && GetVar((STRPTR)"LIBSTAPEDUMP", (STRPTR)mode, sizeof mode, 0) >= 0;
    dlen = 0;
    snprintf(tname, sizeof tname, "%s", name);
    snprintf(tpath, sizeof tpath, "%s/%s.tape", dir, name);
    seq = segno = len = pos = kseq = 0;
    paused = 0;
    broken = 0;
    kline[0] = 0;
    ktok = NULL;
    reset_seg();
    memset(predict, 0, sizeof predict);
    find_hunks();
    if (tape_mode == TAPE_RECORD) {
        char l[200];
        snprintf(l, sizeof l, "# %s: the original libraries' observations (libs/tests/tape.c)\n",
                 name);
        put(l);
    } else if (tape_mode == TAPE_REPLAY) {
        LONG got;
        fh = Open((STRPTR)tpath, MODE_OLDFILE);
        if (!fh) {
            fault("no recording");
            broken = 1;
            return;
        }
        while (len < TAPE_MAX && (got = Read(fh, buf + len, TAPE_MAX - len)) > 0)
            len += got;
        Close(fh);
        if (len == TAPE_MAX) {
            fault("the recording is too long");
            broken = 1;
        }
    }
}

void tape_end(void)
{
    BPTR fh;

    if (tape_mode == TAPE_LIVE)
        return;
    close_seg(1);
    if (tape_mode == TAPE_RECORD) {
        fh = broken ? 0 : Open((STRPTR)tpath, MODE_NEWFILE);
        if (!fh || Write(fh, buf, len) != (LONG)len)
            fault("cannot write the recording");
        if (fh)
            Close(fh);
    } else if (!broken && next_entry())
        fault("fewer observations than the recording has");
    if (dumping) {
        char p[260];
        snprintf(p, sizeof p, "%.250s", tpath);
        strcpy(p + strlen(p) - 4, "dump");
        if ((fh = Open((STRPTR)p, MODE_NEWFILE))) {
            Write(fh, dump, dlen);
            Close(fh);
        }
    }
    tape_mode = TAPE_LIVE;
    len = pos = 0;
}

/* the memory of the test program itself: its hunks and its stack.  A
   value there is an address of the program's data (a string, a buffer, a
   return address); the program's layout changes with every change of a
   test, so such a value is hashed as PROGADDR (the live comparison still
   compares it) */
#define MAXHUNKS 16
#define PROGADDR 0x50524f47UL           /* "PROG" */
static ULONG hunk[MAXHUNKS][2];
static int nhunks;

static void find_hunks(void)
{
    struct Process *pr = (struct Process *)FindTask(NULL);
    struct CommandLineInterface *cli = BADDR(pr->pr_CLI);
    BPTR seg = cli ? cli->cli_Module : pr->pr_SegList;
    nhunks = 0;
    if (!cli && seg)                    /* pr_SegList: the 4th entry */
        seg = ((BPTR *)BADDR(seg))[3];
    for (; seg && nhunks < MAXHUNKS - 1; seg = *(BPTR *)BADDR(seg)) {
        ULONG *p = BADDR(seg);
        hunk[nhunks][0] = (ULONG)p;
        hunk[nhunks][1] = (ULONG)p + p[-1];
        nhunks++;
    }
    hunk[nhunks][0] = (ULONG)pr->pr_Task.tc_SPLower;
    hunk[nhunks][1] = (ULONG)pr->pr_Task.tc_SPUpper;
    nhunks++;
}

static ULONG progaddr(ULONG v)
{
    int i;
    for (i = 0; i < nhunks; i++)
        if (v >= hunk[i][0] && v < hunk[i][1])
            return PROGADDR;
    return v;
}

static int rawval;

ULONG tape_val(const char *what, ULONG o, ULONG c)
{
    ULONG v;
    rawval = 1;
    v = tape_obs(what, o, c);
    rawval = 0;
    return v;
}

ULONG tape_obs(const char *what, ULONG o, ULONG c)
{
    ULONG v;
    UBYTE b[4];

    if (tape_mode == TAPE_LIVE || paused)
        return o;
    v = tape_mode == TAPE_RECORD ? o : c;
    if (!rawval)
        v = progaddr(v);
    b[0] = (UBYTE)(v >> 24);
    b[1] = (UBYTE)(v >> 16);
    b[2] = (UBYTE)(v >> 8);
    b[3] = (UBYTE)v;
    mix(b, 4);
    count(what, v);
    return tape_mode == TAPE_RECORD ? o : c;
}

int tape_same(const char *what, ULONG o, ULONG c)
{
    return tape_obs(what, o, c) == c;
}

int tape_bytes(const char *what, const void *o, const void *c, ULONG n)
{
    if (tape_mode == TAPE_LIVE || paused)
        return memcmp(o, c, n);
    {
        const UBYTE *p = tape_mode == TAPE_RECORD ? o : c;
        ULONG h = 0x811c9dc5UL, k;
        mix(p, n);
        for (k = 0; dumping && k < n; k++)
            h = (h ^ p[k]) * 0x01000193UL;
        count(what, h);
    }
    return tape_mode == TAPE_RECORD ? memcmp(o, c, n) : 0;
}

int tape_ptrbytes(const char *what, const void *o, const void *c, ULONG n)
{
    static UBYTE copy[4096];
    const UBYTE *p;
    ULONG i, v;

    if (tape_mode == TAPE_LIVE || paused || n > sizeof copy)
        return tape_bytes(what, o, c, n);
    p = tape_mode == TAPE_RECORD ? o : c;
    memcpy(copy, p, n);
    for (i = 0; i + 4 <= n; i++) {
        v = (ULONG)p[i] << 24 | (ULONG)p[i + 1] << 16 | (ULONG)p[i + 2] << 8 | p[i + 3];
        if (progaddr(v) == PROGADDR) {
            copy[i] = 'P';
            copy[i + 1] = 'R';
            copy[i + 2] = 'O';
            copy[i + 3] = 'G';
        }
    }
    if (tape_mode == TAPE_RECORD) {
        tape_bytes(what, copy, copy, n);
        return memcmp(o, c, n);
    }
    tape_bytes(what, copy, copy, n);
    return 0;
}

int tape_strsame(const char *what, const char *o, const char *c)
{
    ULONG lo = strlen(o), lc = strlen(c);
    if (tape_mode == TAPE_LIVE || paused)
        return !strcmp(o, c);
    tape_obs(what, lo, lc);
    if (tape_mode == TAPE_RECORD) {
        tape_bytes(what, o, o, lo);
        return !strcmp(o, c);
    }
    tape_bytes(what, c, c, lc);
    return 1;
}

ULONG tape_fixed(const char *what, ULONG o, ULONG c)
{
    char l[80];
    const char *e;
    ULONG s, v;

    if (tape_mode == TAPE_LIVE || paused)
        return o;
    if (tape_mode == TAPE_RECORD) {
        snprintf(l, sizeof l, "fixed %lu %08lx\n", seq, o);
        put_line(l);
    } else {
        e = broken || (ktok && *ktok) ? NULL : next_entry();
        if (!e || sscanf(e, "fixed %lu %lx", &s, &v) != 2 || s != seq) {
            snprintf(l, sizeof l, "observation %lu has no recorded value", seq);
            fault(l);
            broken = 1;
            o = c;
        } else
            o = v;
    }
    count(what, o);
    return o;
}

ULONG tape_kept(const char *what, ULONG o, ULONG c)
{
    char l[24];
    UBYTE b[2];
    ULONG key = 0x811c9dc5UL, p, alt, v;
    const char *w;
    int i;

    if (tape_mode == TAPE_LIVE || paused)
        return o;
    c &= 0x7fff;
    for (w = what; w && *w && *w != ' '; w++) {
        key ^= (UBYTE)*w;
        key *= 0x01000193UL;
    }
    key ^= c;
    i = (int)((key ^ key >> 12 ^ key >> 24) % NPREDICT);
    if (predict[i].key == key) {
        p = predict[i].mask;
        alt = predict[i].alt;
    } else
        p = alt = c;
    if (tape_mode == TAPE_RECORD) {
        if (o != p) {
            if (o == alt)
                snprintf(l, sizeof l, " %lu", seq - kseq);
            else
                snprintf(l, sizeof l, " %lu/%04lx", seq - kseq, o);
            kseq = seq;
            if (strlen(kline) + strlen(l) > 72)
                flush_k();
            if (!kline[0])
                strcpy(kline, "k");
            strcat(kline, l);
        }
    } else {
        switch (broken ? 0 : take_kept(&v)) {
        case 1: o = v; break;
        case 2: o = alt; break;
        default: o = p; break;
        }
    }
    if (o != p || predict[i].key != key) {
        predict[i].alt = predict[i].key == key ? (UWORD)p : (UWORD)o;
        predict[i].key = key;
        predict[i].mask = (UWORD)o;
    }
    b[0] = (UBYTE)(o >> 8);
    b[1] = (UBYTE)o;
    mix(b, 2);
    count(what, o);
    return o;
}

void tape_pause(int on)
{
    paused = on;
}

int tape_orig(void)
{
    return tape_mode != TAPE_REPLAY;
}
