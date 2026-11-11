/*
 * systest_med - functional comparison of medlib (the MED player),
 * original (acidlibs) against the C build, with generated modules.
 *
 *   systest_med DIR
 *
 * Two modules are built here (fixed seed): an MMD0 and an MMD1 with 4
 * tracks, normal samples (with and without repeat), IFF octave samples
 * (types 1, 2 and 7 = clamped to 6), a synth and a hybrid instrument whose
 * tables use every table command, an InstrExt (hold, decay, finetune) and
 * pseudo random notes with all commands of the format.  Each set gets its
 * own copy; StartMedModule (the second by DecodeMedModule, copied into
 * chip memory), then PlayMed for every timing pulse, with JumpMed,
 * SetMedVolume, SetMedMask, GetMedNote/Instr/Volume and StopMed between.
 * The player writes Paula's registers through the audio address in its
 * track structures (trk_audioaddr): the test finds the 4 structures in
 * each library (the addresses $DFF0A0..$DFF0D0, 98 bytes apart) and points
 * them at a RAM copy of the registers of each set.  After every call the
 * registers, the 4 track structures (pointers as offsets into the module,
 * or by the bytes they point at), the play state of the module and the
 * player's variables behind the tracks (if both libraries have them at
 * the same place) are compared.
 *
 * The inputs avoid what reads or writes outside the module or the
 * library's tables (the result would depend on memory the two sets do not
 * share): at most 4 tracks (#209), finetunes -8..7 (#211), small notes and
 * transposes (period table rows), synth table jumps only to command
 * positions, small sample offsets (the Bxx word, #203), no MIDI dump
 * (#207).
 */
#include "sysload.h"

/* ------------------------------------------------------------ modules */

static ULONG seed = 0x13579bdf;
static ULONG rnd(ULONG n)
{
    seed = seed * 1103515245UL + 12345;
    return (seed >> 8) % n;
}

#define MODMAX 24000
static UBYTE mod[MODMAX];
static ULONG modlen;

static void p8(ULONG o, ULONG v) { mod[o] = (UBYTE)v; }
static void p16(ULONG o, ULONG v) { p8(o, v >> 8); p8(o + 1, v); }
static void p32(ULONG o, ULONG v) { p16(o, v >> 16); p16(o + 2, v); }
static ULONG align4(ULONG o) { return (o + 3) & ~3UL; }

#define NBLOCKS 6
#define NLINES  16
#define NSAMPLES 7

/* the synth tables (see the head: every command, jumps to command starts) */
static const UBYTE voltbl[] = {
    0xf0, 0x02, 0xf4, 0x01, 0x40, 0xf1, 0x03, 0x30, 0xf6, 0xf3, 0x02, 0xf1, 0x04, 0xf2,
    0x01, 0xf1, 0x02, 0xf5, 0x02, 0x20, 0xf1, 0x06, 0xfa, 0x0a, 0x28, 0xf1, 0x03, 0xfe,
    0x04,
};
static const UBYTE wftbl[] = {
    0xf0, 0x02, 0x00, 0xf1, 0x02, 0x01, 0xf2, 0x03, 0xf1, 0x03, 0x02, 0xf4, 0x08, 0xf5,
    0x03, 0xf7, 0x01, 0xf1, 0x04, 0xf6, 0xf3, 0x02, 0xf1, 0x02, 0xfc, 0x00, 0x04, 0x07,
    0xfd, 0xf1, 0x05, 0xf8, 0xfa, 0x05, 0x00, 0xf1, 0x03, 0xfe, 0x03,
};
/* the positions of wftbl where a command or a waveform starts (command 0E) */
static const UBYTE wfstarts[] = {
    0, 2, 3, 5, 6, 8, 10, 11, 13, 15, 17, 19, 20, 22, 24, 29, 31, 32, 34, 35, 37,
};

/* a synth (-1) or hybrid (-2) instrument at o; returns its end */
static ULONG synth(ULONG o, int hybrid)
{
    ULONG i, w, wf = o + 278 + 3 * 4;

    p32(o, 0);
    p16(o + 4, hybrid ? 0xfffe : 0xffff);
    p8(o + 6, 2);
    p16(o + 14, 128);
    p16(o + 16, 128);
    p8(o + 18, 2);                              /* volume table speed */
    p8(o + 19, 3);                              /* waveform table speed */
    p16(o + 20, 3);                             /* waveforms */
    for (i = 0; i < sizeof voltbl; i++)
        p8(o + 22 + i, voltbl[i]);
    for (i = 0; i < sizeof wftbl; i++)
        p8(o + 150 + i, wftbl[i]);
    for (w = 0; w < 3; w++) {
        p32(o + 278 + 4 * w, wf - o);           /* relative to the instrument */
        if (hybrid && w == 0) {                 /* the sample of the hybrid */
            p32(wf, 256);
            p16(wf + 4, 0);
            for (i = 0; i < 256; i++)
                p8(wf + 6 + i, i * 3);
            wf += 6 + 256;
            continue;
        }
        p16(wf, 64);                            /* triangle, saw, square */
        for (i = 0; i < 128; i++)
            p8(wf + 2 + i, w == 0 ? (i < 64 ? i * 2 : 255 - i * 2)
                           : w == 1 ? i * 2 : (i & 32 ? 0x7f : 0x80));
        wf += 130;
    }
    return align4(wf);
}

/* a sample: length, type, data */
static ULONG sample(ULONG o, ULONG len, UWORD type)
{
    ULONG i;

    p32(o, len);
    p16(o + 4, type);
    for (i = 0; i < len; i++)
        p8(o + 6 + i, i * 7 + type);
    return align4(o + 6 + len);
}

/* one note: MMD0 3 bytes (instrument bits 4/5 in the note byte), MMD1 4 */
static ULONG note(ULONG o, int mmd1)
{
    static const UBYTE cmds0[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
    static const UBYTE cmds1[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x11, 0x12, 0x13, 0x14,
        0x15, 0x16, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    };
    static const UBYTE fq[] = { 0x00, 0xf1, 0xf2, 0xf3, 0xf8, 0xf9, 0xfd, 0xff, 0x10, 0x80 };
    UWORD n = rnd(10) < 4 ? 1 + rnd(48) : 0, ins = 0, cmd = 0, q = 0;

    if (n || rnd(4) == 0)
        ins = rnd(4) ? 1 + rnd(NSAMPLES) : 0;
    if (rnd(2)) {
        cmd = mmd1 ? cmds1[rnd(sizeof cmds1)] : cmds0[rnd(sizeof cmds0)];
        /* the jumps, breaks, loops and delays rarely: else the song keeps
           to a few lines */
        if ((cmd == 0x0b || cmd == 0x0f || cmd == 0x16 || cmd == 0x1d || cmd == 0x1e)
            && rnd(16))
            cmd = 0x0a;
        switch (cmd) {
        case 0x01: case 0x02: case 0x11: case 0x12: q = rnd(24); break;
        case 0x03: case 0x13: q = rnd(40); break;
        case 0x09: q = 1 + rnd(7); break;
        case 0x0b: q = rnd(12); break;
        case 0x0c: q = rnd(3) ? rnd(65) : 0x80 | rnd(65); break;
        case 0x0e: q = wfstarts[rnd(sizeof wfstarts)]; break;
        case 0x0f: q = fq[rnd(sizeof fq)]; break;
        case 0x15: q = (UBYTE)(rnd(16) - 8); break;
        case 0x16: q = rnd(4); break;
        case 0x18: q = rnd(6); break;
        case 0x19: q = rnd(5); break;
        case 0x1a: case 0x1b: q = rnd(16); break;
        case 0x1d: q = rnd(NLINES); break;
        case 0x1e: q = rnd(3); break;
        default: q = rnd(256); break;
        }
    }
    if (!mmd1) {
        p8(o, n | (ins & 0x30) << 2);
        p8(o + 1, (ins & 15) << 4 | cmd);
        p8(o + 2, q);
        return o + 3;
    }
    p8(o, n);
    p8(o + 1, ins);
    p8(o + 2, cmd);
    p8(o + 3, q);
    return o + 4;
}

/* the module (MMD0 or MMD1) in mod[], modlen */
static void build(int mmd1)
{
    ULONG song = 52, blockarr, smplarr, o, b, l, t, i, exp, iext;
    static const ULONG lens[NSAMPLES] = { 512, 600, 31 * 32, 7 * 64, 0, 0, 127 * 8 };
    static const UWORD types[NSAMPLES] = { 0, 0, 1, 2, 0xffff, 0xfffe, 7 };

    memset(mod, 0, sizeof mod);
    p32(0, mmd1 ? 0x4d4d4431 : 0x4d4d4430);    /* "MMD0" / "MMD1" */
    p32(8, song);
    /* song: instruments, blocks, sequence, tempo, flags, track volumes */
    for (i = 0; i < NSAMPLES; i++) {
        p16(song + 8 * i, i == 1 ? 100 : 0);    /* repeat (words) */
        p16(song + 8 * i + 2, i == 1 ? 150 : i == 2 ? 200 : 0);
        p8(song + 8 * i + 6, 30 + rnd(35));     /* svol */
        p8(song + 8 * i + 7, rnd(3));           /* strans */
    }
    p16(song + 504, NBLOCKS);
    p16(song + 506, 10);
    for (i = 0; i < 10; i++)
        p8(song + 508 + i, i % NBLOCKS);
    p16(song + 764, 125);
    p8(song + 766, mmd1 ? 2 : 0);               /* transpose */
    p8(song + 767, mmd1 ? 0x31 : 0x00);         /* flags */
    p8(song + 768, mmd1 ? 0x20 : 0x00);         /* flags2 (BPM: every 4th pulse) */
    p8(song + 769, mmd1 ? 3 : 4);               /* ticks per line */
    for (i = 0; i < 16; i++)
        p8(song + 770 + i, 40 + rnd(25));
    p8(song + 786, 50 + rnd(15));               /* master volume */
    p8(song + 787, NSAMPLES);
    o = align4(song + 788);
    /* blocks */
    blockarr = o;
    p32(16, blockarr);
    o = align4(blockarr + 4 * NBLOCKS);
    for (b = 0; b < NBLOCKS; b++) {
        p32(blockarr + 4 * b, o);
        if (mmd1) {
            p16(o, 4);
            p16(o + 2, NLINES - 1);
            p32(o + 4, 0);
            o += 8;
        } else {
            p8(o, 4);
            p8(o + 1, NLINES - 1);
            o += 2;
        }
        for (l = 0; l < NLINES; l++)
            for (t = 0; t < 4; t++)
                o = note(o, mmd1);
        o = align4(o);
    }
    /* samples */
    smplarr = o;
    p32(24, smplarr);
    o = align4(smplarr + 4 * NSAMPLES);
    for (i = 0; i < NSAMPLES; i++) {
        p32(smplarr + 4 * i, o);
        if ((WORD)types[i] < 0)
            o = synth(o, types[i] == 0xfffe);
        else
            o = sample(o, lens[i], types[i]);
    }
    /* expansion data: only the InstrExt (hold, decay, -, finetune) */
    exp = o;
    p32(32, exp);
    iext = exp + 64;
    p32(exp + 4, iext);
    p16(exp + 8, NSAMPLES);
    p16(exp + 10, 4);
    for (i = 0; i < NSAMPLES; i++) {
        p8(iext + 4 * i, rnd(4));
        p8(iext + 4 * i + 1, rnd(16));
        p8(iext + 4 * i + 3, rnd(16) - 8);
    }
    modlen = align4(iext + 4 * NSAMPLES);
    p32(4, modlen);
}

/* ------------------------------------------------------------ the sets */

#define T03SZ 98
static UBYTE *tracks[2];                /* the 4 track structures */
static UBYTE audio[2][4][16];           /* the audio registers */
static UBYTE *module[2];                /* the module the set plays */
static ULONG obj[2][8];                 /* MedModule object (32 bytes) */
static int dbsame;                      /* player variables at the same place */
static struct regs R[2], IN[2];
static int cur, prints, ticks;
static const char *what;

static UBYTE *find_tracks(struct lib *l)
{
    ULONG o;

    for (o = 22; o + 300 < l->size; o += 2) {
        const UBYTE *p = l->code + o;

        if (L(p) == 0xdff0a0 && L(p + 98) == 0xdff0b0 && L(p + 196) == 0xdff0c0
            && L(p + 294) == 0xdff0d0)
            return l->code + o - 22;
    }
    return NULL;
}

/* the offset of the first period table row (3424, 3232, 3048) from the
   tracks, or -1 */
static LONG periods_at(struct lib *l, UBYTE *t)
{
    ULONG o;

    for (o = 0; o + 6 < l->size; o += 2) {
        const UBYTE *p = l->code + o;

        if (W(p) == 3424 && W(p + 2) == 3232 && W(p + 4) == 3048)
            return (LONG)(p - t);
    }
    return -1;
}

static UBYTE *entry(int i)
{
    return entry_form(getlib(&sets[cur], "medlib"), i, 0);
}

/* call medlib entry i on both sets (registers: markers + setup) */
#define CALL2(i, setup) do {                                                \
        for (cur = 0; cur < 2; cur++) {                                     \
            markregs(&R[cur]);                                              \
            setup;                                                          \
            IN[cur] = R[cur];                                               \
            callregs(entry(i), &R[cur], 0, 0);                              \
        }                                                                   \
        preserve(i);                                                        \
    } while (0)

static void preserve(int i)
{
    static int done[32];
    struct call c[2];
    char buf[100];

    if (done[i & 31]++ >= 20)
        return;
    c[0].in = IN[0];
    c[0].out = R[0];
    c[1].in = IN[1];
    c[1].out = R[1];
    sprintf(buf, "medlib %s (tick %d)", what, ticks);
    cmp_preserve(buf, &c[0], &c[1]);
}

/* ---------------------------------------------------------- the state */

#define MAXD 700
static ULONG dig[2][MAXD];
static int nd[2];
static const char *dname[2][MAXD];

static void put(const char *name, ULONG v)
{
    if (nd[cur] < MAXD) {
        dname[cur][nd[cur]] = name;
        dig[cur][nd[cur]++] = v;
    }
}

/* a pointer: 0, an offset into the module, or the 8 bytes it points at
   (period tables, sine table, the zero sample of each library) */
static void putptr(const char *name, ULONG v)
{
    const UBYTE *p = (const UBYTE *)v;

    if (!v) {
        put(name, 0);
        put(name, 0);
    } else if (v >= (ULONG)module[cur] && v < (ULONG)module[cur] + modlen + 256) {
        put(name, 1);
        put(name, v - (ULONG)module[cur]);
    } else {
        put(name, (ULONG)p[0] << 24 | (ULONG)p[1] << 16 | p[2] << 8 | p[3]);
        put(name, (ULONG)p[4] << 24 | (ULONG)p[5] << 16 | p[6] << 8 | p[7]);
    }
}

static int isptr(int off)
{
    return off == 14 || off == 22 || off == 26 || off == 38 || off == 60 || off == 66
        || off == 78 || off == 86;
}

static void digest(void)
{
    static const char *trk[4] = { "track 0", "track 1", "track 2", "track 3" };
    static const char *reg[4] = { "audio 0", "audio 1", "audio 2", "audio 3" };
    int ch, i;

    nd[cur] = 0;
    for (ch = 0; ch < 4; ch++) {
        UBYTE *t = tracks[cur] + ch * T03SZ, *a = audio[cur][ch];

        putptr(reg[ch], L(a));
        for (i = 4; i < 16; i++)
            put(reg[ch], a[i]);
        for (i = 0; i < T03SZ; i++) {
            if (isptr(i)) {
                if (i != 22)                    /* trk_audioaddr: ours */
                    putptr(trk[ch], L(t + i));
                i += 3;
            } else
                put(trk[ch], t[i] | (ULONG)i << 16);
        }
    }
    for (i = 40; i < 52; i++)
        put("module state", module[cur][i] | (ULONG)i << 16);
    if (dbsame)
        for (i = 696; i < 716; i++)
            put("player variables", tracks[cur][i] | (ULONG)i << 16);
}

/* what the run went through (set 0): ticks with the module playing, audio
   register changes, ticks with a synth or hybrid note, commands seen */
static ULONG playing, changes, synths, cmdseen, visited[NBLOCKS];
static UBYTE lastaudio[4][16];

static void stats(void)
{
    int ch;

    if (W(module[0] + 40))
        playing++;
    if (W(module[0] + 42) < NBLOCKS && W(module[0] + 44) < NLINES)
        visited[W(module[0] + 42)] |= 1UL << W(module[0] + 44);
    for (ch = 0; ch < 4; ch++) {
        UBYTE *t = tracks[0] + ch * T03SZ;

        if (memcmp(lastaudio[ch], audio[0][ch], 16)) {
            changes++;
            memcpy(lastaudio[ch], audio[0][ch], 16);
        }
        if (L(t + 38))
            synths++;
        cmdseen |= 1UL << (t[4] & 31);
    }
}

static void compare(void)
{
    char buf[120];
    int i, bad = 0;

    stats();
    for (cur = 0; cur < 2; cur++)
        digest();
    for (i = 0; i < nd[0] && i < nd[1]; i++)
        if (!tape_same(dname[1][i], dig[0][i], dig[1][i])) {
            if (prints++ < 40)
                printf("FAIL medlib %s tick %d: %s [%d] orig $%08lX, C $%08lX\n", what, ticks,
                       dname[0][i], i, dig[0][i], dig[1][i]);
            bad = 1;
        }
    sprintf(buf, "medlib %s tick %d: player state", what, ticks);
    check(buf, !bad && tape_same(buf, nd[0], nd[1]));
}

static void same_d0(const char *name)
{
    char buf[100];
    ULONG o;

    sprintf(buf, "medlib %s tick %d: %s d0", what, ticks, name);
    o = tape_obs(buf, R[0].d[0], R[1].d[0]);
    if (o != R[1].d[0] && prints++ < 40)
        printf("FAIL %s: orig $%08lX, C $%08lX\n", buf, o, R[1].d[0]);
    check(buf, o == R[1].d[0]);
}

/* the medlib entries */
enum { E_START = 1, E_PLAY, E_STOP, E_JUMP, E_SETVOL, E_GETVOL, E_GETNOTE, E_GETINSTR,
       E_SETMASK, E_DECODE, E_FREE = 15, E_INIT, E_FINIT };

static void play(int n)
{
    int k;

    for (k = 0; k < n; k++) {
        ticks++;
        CALL2(E_PLAY, (void)0);
        compare();
        if (ticks % 5 == 0) {
            CALL2(E_GETNOTE, R[cur].d[0] = ticks % 4);
            same_d0("GetMedNote");
        }
        if (ticks % 7 == 0) {
            CALL2(E_GETINSTR, R[cur].d[0] = ticks % 4);
            same_d0("GetMedInstr");
        }
        if (ticks % 97 == 0) {                  /* clears trk_prevvol (#212) */
            CALL2(E_GETVOL, R[cur].d[0] = ticks % 4);
            same_d0("GetMedVolume");
        }
    }
}

static void song(const char *name, int pulses, int mincmds)
{
    char buf[160];
    int n, cmds = 0, lines = 0;

    ticks = 0;
    playing = changes = synths = cmdseen = 0;
    memset(visited, 0, sizeof visited);
    play(pulses);
    what = "JumpMed";
    CALL2(E_JUMP, R[cur].d[0] = 4);
    compare();
    play(150);
    what = "SetMedVolume";
    CALL2(E_SETVOL, R[cur].d[0] = 33);
    compare();
    play(150);
    what = "SetMedMask";
    CALL2(E_SETMASK, R[cur].d[0] = 5);
    play(200);
    CALL2(E_SETMASK, R[cur].d[0] = 15);
    play(100);
    what = "StopMed";
    CALL2(E_STOP, (void)0);
    compare();
    play(3);
    for (n = 0; n < 32; n++)
        cmds += (cmdseen >> n) & 1;
    for (n = 0; n < NBLOCKS * NLINES; n++)
        lines += (visited[n / NLINES] >> (n % NLINES)) & 1;
    printf("medlib %s: %lu ticks playing, %lu audio register changes, %lu synth ticks,"
           " %d commands ($%08lX), %d lines\n", name, playing, changes, synths, cmds, cmdseen,
           lines);
    sprintf(buf, "medlib %s: the module plays notes, synth notes and most commands", name);
    check(buf, playing > 500 && changes > 500 && synths > 100 && cmds >= mincmds
          && lines >= 40);
}

static const char *names[] = { "memlib", "medlib", NULL };

int main(int argc, char **argv)
{
    UBYTE *copy[2], *mmd0[2];
    ULONG len0;
    LONG per[2];
    int ch, e;

    if (argc < 2) {
        printf("usage: systest_med DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    what = "init";
    for (cur = 0; cur < 2; cur++)
        for (e = E_START; e <= E_FINIT; e++)
            if ((e <= E_DECODE || e >= E_INIT) && !entry(e)) {
                printf("medlib %s: no routine for entry %d\n", sets[cur].what, e);
                return 20;
            }
    for (cur = 0; cur < 2; cur++) {
        markregs(&R[cur]);
        callregs(entry_code(getlib(&sets[cur], "memlib"), 7), &R[cur], 0, 0);
        tracks[cur] = find_tracks(getlib(&sets[cur], "medlib"));
        if (!tracks[cur]) {
            printf("medlib %s: no track structures found\n", sets[cur].what);
            return 20;
        }
        per[cur] = periods_at(getlib(&sets[cur], "medlib"), tracks[cur]);
        for (ch = 0; ch < 4; ch++)
            *(ULONG *)(tracks[cur] + ch * T03SZ + 22) = (ULONG)audio[cur][ch];
    }
    dbsame = tape_same("medlib: period tables", per[0], per[1]) && per[1] == 906;
    printf("medlib: period tables %ld / %ld bytes behind the tracks%s\n", per[0], per[1],
           dbsame ? "" : " - player variables not compared");
    CALL2(E_INIT, (void)0);

    /* MMD0: the object as LoadMedModule leaves it (id, length, module) */
    build(0);
    what = "MMD0 StartMedModule";
    len0 = modlen;
    for (cur = 0; cur < 2; cur++) {
        mmd0[cur] = module[cur] = AllocMem(modlen + 256, MEMF_CHIP | MEMF_CLEAR);
        memcpy(module[cur], mod, modlen);
        obj[cur][0] = L(mod);
        obj[cur][1] = modlen;
        obj[cur][2] = (ULONG)module[cur];
    }
    CALL2(E_START, R[cur].a[3] = (ULONG)obj[cur]);
    compare();
    what = "MMD0";
    song(what, what[3] == '1' ? 6000 : 1500, what[3] == '1' ? 22 : 14);

    /* MMD1 by DecodeMedModule from public memory (copied into chip) */
    build(1);
    what = "MMD1 DecodeMedModule";
    for (cur = 0; cur < 2; cur++) {
        copy[cur] = AllocMem(modlen + 256, MEMF_PUBLIC | MEMF_CLEAR);
        memcpy(copy[cur], mod, modlen);
    }
    CALL2(E_DECODE, R[cur].d[1] = (ULONG)copy[cur]; R[cur].a[3] = (ULONG)&obj[cur][4]);
    for (cur = 0; cur < 2; cur++)
        module[cur] = (UBYTE *)obj[cur][6];
    check("medlib DecodeMedModule: module in place or copied alike",
          tape_same("medlib DecodeMedModule: in place", module[0] == copy[0],
                    module[1] == copy[1]));
    what = "MMD1 StartMedModule";
    CALL2(E_START, R[cur].a[3] = (ULONG)&obj[cur][4]);
    compare();
    what = "MMD1";
    song(what, what[3] == '1' ? 6000 : 1500, what[3] == '1' ? 22 : 14);

    what = "finit";
    CALL2(E_FINIT, (void)0);
    for (cur = 0; cur < 2; cur++) {             /* memlib frees the chip copy */
        markregs(&R[cur]);
        callregs(entry_code(getlib(&sets[cur], "memlib"), 8), &R[cur], 0, 0);
        FreeMem(mmd0[cur], len0 + 256);
        FreeMem(copy[cur], modlen + 256);
    }
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 5 : 0;
}
