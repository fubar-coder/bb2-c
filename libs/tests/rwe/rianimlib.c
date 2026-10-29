/*
 * rwe rianimlib - RIAnimLib: the C build against the CD binary BSS2
 * RIANIMLIB.OBJ (rianimlib.orig; = Anim-Library3 without RIDiskAnimInit),
 * method of rwecmp.h.
 *
 *   rianimlib DIR        (DIR with rianimlib.obj/.orig)
 *
 * The test builds small ANIMs in memory (64x8, 2 planes; frames with
 * ANHD operations 5, 65 and 7, a CMAP, an unknown operation) and gives
 * each set its own copy, bitmap object (ebwidth 8, plane pointers into
 * guarded buffers) and register markers.  RIPackLib is not loaded: its
 * UnpackIFF (form 1) and ILBMPalette, which RIAnimInit and
 * RINextAnimFrame call through ALibJsr, are stubs (sysload_stubs) that
 * log their arguments (the source address relative to the set's ANIM).
 * Compared after every call: d0, the stub log, the planes (with the
 * bytes around them) and every register the original keeps.
 *
 * Covered: RIAnimDepth/Width/Height (an ANIM, no FORM, a FORM ILBM, an
 * ANIM whose first chunk is no FORM, an ANIM without BMHD);
 * RIAnimFrameCount before and after RIAnimInit; RIAnimInit in all three
 * forms (offset 0, an offset, x,y with a marker in y's upper word, the
 * bitmap# * 64 of #pd1 and one whose DIVU overflows), failing (no ANIM,
 * no ILBM, no BMHD: BUG(orig), RIAnimFrameCount still counts), a BMHD
 * chunk of the ANIM behind the first frame (BUG(orig): found), one and
 * two frame anims (the safety nets); RINextAnimFrame through every
 * frame (ANIM5, ANIM5 with CMAP, ANIM7 long, operation 65, an unknown
 * operation), past the end with AnimLoop on (the third frame) and off,
 * before RIAnimInit;
 * AnimLoop with on, off and a value whose low byte is 0.
 * Not covered: a frame without DLTA (BUG(orig): RINextAnimFrame returns
 * through its saved d1 - a crash), a chunk with the ID at +8 that is no
 * FORM (BUG(orig): returns through a1; DIFF), odd chunk lengths, depth 0
 * (256 planes), runs of 0 (256 rows), widths below 8 or 32 (65536
 * columns).
 */
#include "rwecmp.h"

#define LIB "rianimlib"

enum { E_INIT, E_NEXT, E_LOOP, E_COUNT, E_DEPTH, E_WIDTH, E_HEIGHT };

/* ------------------------------------------------------------ RIPackLib */

#define TOK_unpackiff   ((73 << 7) + 3)
#define TOK_ilbmpalette ((73 << 7) + 4)

#define ANIMSZ 4096
static UBYTE anim[2][ANIMSZ];           /* the set's copy of the ANIM */

static struct { ULONG fn, d0, d1, d2, d3; } plog[2][16];
static int nplog[2], plogdone;

static ULONG rel(ULONG a)
{
    return a - (ULONG)anim[cur];
}

static void plogit(ULONG fn, ULONG d0, ULONG d1, ULONG d2, ULONG d3)
{
    if (nplog[cur] < 16) {
        plog[cur][nplog[cur]].fn = fn;
        plog[cur][nplog[cur]].d0 = rel(d0);
        plog[cur][nplog[cur]].d1 = d1;
        plog[cur][nplog[cur]].d2 = d2;
        plog[cur][nplog[cur]].d3 = d3;
    }
    nplog[cur]++;
}

static void __attribute__((used)) st_unpackiff_c(ULONG d0, ULONG d1, ULONG d2, ULONG d3)
{
    plogit(1, d0, d1, d2, d3);
}

static void __attribute__((used)) st_ilbmpalette_c(ULONG d0, ULONG d1)
{
    plogit(2, d0, d1, 0, 0);
}

/* the routines' register interface; they return d0 = -1 and keep the
   others (what they return is not used) */
void st_unpackiff(void), st_ilbmpalette(void);
__asm__(
"   .text\n"
"   .even\n"
"_st_unpackiff:\n"
"   movem.l d1-d7/a0-a6,-(sp)\n"
"   movem.l d0-d3,-(sp)\n"
"   jsr     _st_unpackiff_c\n"
"   lea     16(sp),sp\n"
"   movem.l (sp)+,d1-d7/a0-a6\n"
"   moveq   #-1,d0\n"
"   rts\n"
"_st_ilbmpalette:\n"
"   movem.l d1-d7/a0-a6,-(sp)\n"
"   movem.l d0-d1,-(sp)\n"
"   jsr     _st_ilbmpalette_c\n"
"   addq.l  #8,sp\n"
"   movem.l (sp)+,d1-d7/a0-a6\n"
"   moveq   #-1,d0\n"
"   rts\n");

static const struct stub stubs[] = {
    { TOK_unpackiff, (APTR)st_unpackiff },
    { TOK_ilbmpalette, (APTR)st_ilbmpalette },
    { 0, NULL }
};

/* ------------------------------------------------------------ the ANIM */

static UBYTE tpl[ANIMSZ];               /* built here, copied to the sets */
static ULONG tlen;

static void put1(ULONG v) { tpl[tlen++] = (UBYTE)v; }
static void put2(ULONG v) { put1(v >> 8); put1(v); }
static void put4(ULONG v) { put2(v >> 16); put2(v); }
static void putid(const char *id) { put1(id[0]); put1(id[1]); put1(id[2]); put1(id[3]); }

/* a chunk/FORM: its ID and the place of its length */
static ULONG begin(const char *id)
{
    putid(id);
    put4(0);
    return tlen - 4;
}

static void end(ULONG at)
{
    ULONG n = tlen - at - 4;

    tpl[at] = (UBYTE)(n >> 24);
    tpl[at + 1] = (UBYTE)(n >> 16);
    tpl[at + 2] = (UBYTE)(n >> 8);
    tpl[at + 3] = (UBYTE)n;
}

static void bmhd(UWORD w, UWORD h, UBYTE depth)
{
    ULONG c = begin("BMHD");
    put2(w); put2(h); put2(0); put2(0);
    put1(depth); put1(0); put1(0); put1(0);
    put2(0); put1(10); put1(11); put2(320); put2(200);
    end(c);
}

static void cmap(UBYTE seed)
{
    ULONG c = begin("CMAP"), i;
    for (i = 0; i < 12; i++)
        put1(seed + i * 17);
    end(c);
}

/* the first frame: BMHD, CMAP, BODY */
static void frame1(BOOL withbmhd)
{
    ULONG f = begin("FORM"), c, i;

    putid("ILBM");
    if (withbmhd)
        bmhd(64, 8, 2);
    cmap(0x10);
    c = begin("BODY");
    for (i = 0; i < 128; i++)
        put1(i * 3);
    end(c);
    end(f);
}

static void anhd(UBYTE op)
{
    ULONG c = begin("ANHD"), i;
    put1(op); put1(0); put2(64); put2(8); put2(0); put2(0);
    put4(0); put4(1); put1(0); put1(0);
    put4(op == 7 ? 1 : 0);              /* bits: long data */
    for (i = 0; i < 16; i++)
        put1(0);
    end(c);
}

/* ANIM5 DLTA: plane 0 and 1, 8 byte columns */
static void dlta5(UBYTE v)
{
    static const UBYTE p0[] = {
        2, 0x01, 0x82, 0xaa, 0xbb,      /* skip 1, copy 2 */
        1, 0x00, 3, 0x5a,               /* run 3 */
        0,
        1, 0x81, 0xcc,
        0, 0, 0, 2, 0x02, 0x00, 2, 0x77,
    };
    static const UBYTE p1[] = { 1, 0x00, 8, 0xff, 0, 0, 0, 0, 0, 0, 3, 0x81, 0x11, 0x03, 0x81, 0x22, 0x82, 0x33, 0x44 };
    ULONG c = begin("DLTA"), i;

    put4(64);
    put4(64 + sizeof p0);
    for (i = 2; i < 16; i++)
        put4(0);
    for (i = 0; i < sizeof p0; i++)
        put1(p0[i] == 0x5a ? v : p0[i]);
    for (i = 0; i < sizeof p1; i++)
        put1(p1[i]);
    end(c);
}

/* ANIM7 DLTA (long data): 2 long columns per plane */
static void dlta7(void)
{
    static const UBYTE o0[] = { 2, 0x00, 2, 0x01, 1, 0x82 };   /* run 2, skip 1 | copy 2 */
    static const UBYTE o1[] = { 0, 2, 0x02, 0x00, 1 };          /* - | skip 2, run 1 */
    ULONG c = begin("DLTA"), i;
    ULONG ops0 = 64, ops1 = ops0 + sizeof o0, dat0 = 76, dat1 = dat0 + 12;

    put4(ops0);
    put4(ops1);
    for (i = 2; i < 8; i++)
        put4(0);
    put4(dat0);
    put4(dat1);
    for (i = 10; i < 16; i++)
        put4(0);
    for (i = 0; i < sizeof o0; i++)
        put1(o0[i]);
    for (i = 0; i < sizeof o1; i++)
        put1(o1[i]);
    put1(0);                            /* to 76 */
    put4(0x11223344);
    put4(0x55667788);
    put4(0x99aabbcc);
    put4(0xdeadbeef);
    end(c);
}

static void deltaframe(UBYTE op, BOOL withcmap, UBYTE v)
{
    ULONG f = begin("FORM");

    putid("ILBM");
    anhd(op);
    if (withcmap)
        cmap(0x40);
    if (op == 7)
        dlta7();
    else
        dlta5(v);
    end(f);
}

/* the ANIMs the test uses (offsets in tpl) */
static ULONG a_full, a_one, a_two, a_nobmhd, a_late, a_noform, a_ilbm, a_noilbm;

static void build(void)
{
    ULONG a, c;

    tlen = 0;
    a_full = tlen;                      /* six frames */
    a = begin("FORM");
    putid("ANIM");
    frame1(TRUE);
    deltaframe(5, FALSE, 0x5a);
    deltaframe(5, TRUE, 0x6b);
    deltaframe(7, FALSE, 0);
    deltaframe(65, FALSE, 0x7c);
    deltaframe(3, FALSE, 0);
    end(a);

    a_one = tlen;                       /* one frame */
    a = begin("FORM");
    putid("ANIM");
    frame1(TRUE);
    end(a);

    a_two = tlen;                       /* two frames */
    a = begin("FORM");
    putid("ANIM");
    frame1(TRUE);
    deltaframe(5, FALSE, 0x3c);
    end(a);

    a_nobmhd = tlen;                    /* no BMHD anywhere */
    a = begin("FORM");
    putid("ANIM");
    frame1(FALSE);
    deltaframe(5, FALSE, 0x5a);
    end(a);

    a_late = tlen;                      /* a BMHD in the ANIM behind frame 1 */
    a = begin("FORM");
    putid("ANIM");
    frame1(FALSE);
    bmhd(48, 6, 1);
    deltaframe(5, FALSE, 0x2d);
    end(a);

    a_noilbm = tlen;                    /* the first chunk no FORM */
    a = begin("FORM");
    putid("ANIM");
    c = begin("ANNO");
    putid("text");
    putid("XXXX");
    end(c);
    frame1(TRUE);
    end(a);

    a_ilbm = tlen;                      /* a FORM ILBM, not ANIM */
    frame1(TRUE);

    a_noform = tlen;                    /* no FORM, no ID at +8 */
    putid("JUNK");
    put4(8);
    putid("NONE");
    put4(0);
}

/* ------------------------------------------------------------ bitmaps */

#define PLANESZ 256
#define PLANEAT 32                      /* the plane in its buffer */
static UBYTE planes[2][2][PLANESZ];
static union { ULONG l[16]; UWORD w[32]; } bmobj[2];   /* the bitmap object */

static void resetbitmap(void)
{
    int s, p, i;

    for (s = 0; s < 2; s++) {
        for (p = 0; p < 2; p++)
            for (i = 0; i < PLANESZ; i++)
                planes[s][p][i] = (UBYTE)(0x80 ^ (i * 7 + p));
        memset(&bmobj[s], 0, sizeof bmobj[s]);
        bmobj[s].w[0] = 8;    /* ebwidth */
        bmobj[s].w[1] = 8;    /* height */
        bmobj[s].w[2] = 2;    /* depth */
        bmobj[s].l[2] = (ULONG)&planes[s][0][PLANEAT];
        bmobj[s].l[3] = (ULONG)&planes[s][1][PLANEAT];
    }
}

static void same_state(void)
{
    int i;
    char buf[30];
    UBYTE *x[2] = { &planes[0][0][0], &planes[1][0][0] };

    SAMEBYTES("planes", x, sizeof planes[0]);
    SAME("RIPackLib calls", nplog[cur]);
    for (i = plogdone; i < nplog[0] && i < nplog[1] && i < 16; i++) {
        sprintf(buf, "RIPackLib call %d", i);
        SAME(buf, plog[cur][i].fn);
        SAME(buf, plog[cur][i].d0);
        SAME(buf, plog[cur][i].d1);
        SAME(buf, plog[cur][i].d2);
        SAME(buf, plog[cur][i].d3);
    }
    plogdone = nplog[0] > nplog[1] ? nplog[0] : nplog[1];
}

/* ------------------------------------------------------------ calls */

static ULONG at(ULONG off)
{
    return (ULONG)anim[cur] + off;
}

static void info(const char *what, ULONG off)
{
    static const char *const r[] = { "RIAnimDepth", "RIAnimWidth", "RIAnimHeight" };
    int i;

    for (i = 0; i < 3; i++) {
        ROUT = r[i];
        sprintf(VEC, "%s", what);
        DO2(ENT(LIB, E_DEPTH + i, 0), R[cur].d[0] = at(off));
        SAME("d0", R[cur].d[0]);
        same_state();
    }
}

static void count(void)
{
    ROUT = "RIAnimFrameCount";
    DO2(ENT(LIB, E_COUNT, 0), (void)0);
    SAME("d0", R[cur].d[0]);
}

static void next(void)
{
    ROUT = "RINextAnimFrame";
    DO2(ENT(LIB, E_NEXT, 0), R[cur].d[0] = 0x5a5a5a03; R[cur].a[0] = (ULONG)bmobj[cur].l);
    SAME("d0", R[cur].d[0]);
    same_state();
}

static void loop(ULONG on)
{
    ROUT = "AnimLoop";
    sprintf(VEC, "$%08lX", on);
    DO2(ENT(LIB, E_LOOP, 0), R[cur].d[0] = on);
}

/* RIAnimInit form 0 (address,bitmap#,palette#) */
static void init0(const char *what, ULONG off)
{
    ROUT = "RIAnimInit";
    sprintf(VEC, "%s", what);
    DO2(ENT(LIB, E_INIT, 0), R[cur].d[0] = at(off); R[cur].d[1] = 3; R[cur].d[2] = 4);
    SAME("d0", R[cur].d[0]);
    same_state();
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    int s, i;

    if (argc < 2) {
        printf("usage: rianimlib DIR\n");
        return 20;
    }
    sysload_stubs = stubs;
    if (!load_sets(argv[1], names)) {
        sysload_stubs = NULL;
        return 20;
    }
    sysload_stubs = NULL;
    build();
    if (tlen > ANIMSZ) {
        check("rianimlib: the ANIMs fit", FALSE);
        return 20;
    }
    for (s = 0; s < 2; s++)
        memcpy(anim[s], tpl, tlen);
    resetbitmap();
    DONTCARE = 1;                       /* d0: the result, compared */
    strcpy(VEC, "");

    /* before RIAnimInit */
    count();
    next();
    info("ANIM", a_full);
    info("no FORM", a_noform);
    info("FORM ILBM", a_ilbm);
    info("first chunk no FORM", a_noilbm);
    info("no BMHD", a_nobmhd);
    info("BMHD behind frame 1", a_late);

    /* failing */
    init0("no FORM", a_noform);
    count();
    init0("FORM ILBM", a_ilbm);
    count();
    init0("first chunk no FORM", a_noilbm);
    count();                            /* BUG(orig): the ANIM is kept */
    init0("no BMHD", a_nobmhd);
    count();
    init0("BMHD behind frame 1", a_late);
    count();
    next();

    /* the six frames, AnimLoop on (the default) */
    resetbitmap();
    init0("six frames", a_full);
    count();
    for (i = 0; i < 9; i++)
        next();
    loop(0x5a5a5a00);                   /* off: the low byte */
    for (i = 0; i < 5; i++)
        next();
    loop(0x00000101);                   /* on */
    next();
    loop(0);
    next();

    /* an offset; x,y */
    resetbitmap();
    ROUT = "RIAnimInit offset";
    strcpy(VEC, "offset 9");
    DO2(ENT(LIB, E_INIT, 1), R[cur].d[0] = at(a_full); R[cur].d[1] = 0x5a5a5a01;
        R[cur].d[2] = 0x5a5a5a02; R[cur].d[3] = 9);
    SAME("d0", R[cur].d[0]);
    same_state();
    loop(1);
    for (i = 0; i < 6; i++)
        next();
    resetbitmap();
    ROUT = "RIAnimInit x,y";
    strcpy(VEC, "x 17 y 1");
    DO2(ENT(LIB, E_INIT, 2), R[cur].d[0] = at(a_full); R[cur].d[1] = 5 * 64;
        R[cur].d[2] = 6; R[cur].d[3] = 17; R[cur].d[4] = 0x5a5a0001;
        R[cur].a[0] = (ULONG)bmobj[cur].l);
    SAME("d0", R[cur].d[0]);
    same_state();
    for (i = 0; i < 6; i++)
        next();
    ROUT = "RIAnimInit x,y";
    strcpy(VEC, "DIVU overflow, x -24");
    DO2(ENT(LIB, E_INIT, 2), R[cur].d[0] = at(a_full); R[cur].d[1] = 0x01000007;
        R[cur].d[2] = 6; R[cur].d[3] = (ULONG)-24; R[cur].d[4] = 3;
        R[cur].a[0] = (ULONG)bmobj[cur].l);
    SAME("d0", R[cur].d[0]);
    same_state();

    /* short anims: the safety nets */
    resetbitmap();
    init0("one frame", a_one);
    count();
    next();
    next();
    init0("two frames", a_two);
    count();
    for (i = 0; i < 4; i++)
        next();

    return rwe_done("rianimlib");
}
