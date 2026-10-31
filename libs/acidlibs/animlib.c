/*
 * animlib - command library 94: IFF ANIM objects (op 5, ByteVertical
 * delta compression) - LoadAnim, InitAnim, NextFrame, Frames.
 *
 * C translation of bblibsrc/animlib (bum8; binary checked: same as the
 * source).  Object library "Anim" (32 bytes, maximum 10, Free =
 * freeanim):
 *   +0  width.w (0 = not loaded)   +2  height.w   +4  depth.w
 *   +6  even byte width.w (ILBM words * 2)        +8  frames.w
 *   +10 BODY (memlib globalloc, MEMF_PUBLIC)       +14 its length.l
 *   +18 first delta node           +22 length of the first node
 *   +26 current delta node         +30 2 = EOR deltas, 0 = MOVE
 * Delta nodes: {next.l, length of the NEXT node.l, DLTA chunk}; the
 * length of a node is stored in its predecessor (+4; the anim's +22 for
 * the first).
 *   LoadAnim (Amiga mode only: code+1) opens the file with ilbmifflib
 *         ilbminit (+1: BMHD -> info block), reads BODY, then ANHD (the
 *         operation byte must be 5, else "Anim format not supported";
 *         19 bytes skipped, bit 1 of `bits` = EOR) and every following
 *         DLTA chunk into a node; frames = number of DLTAs - 1 (1 without
 *         ANHD).  The palette form first calls palettelib +3 (a0 = file
 *         name, a1 = palette).  The file is read through iffiolib.
 *   InitAnim decodes BODY (always as ByteRun1) into the used bitmap and
 *         sets the current delta to the first one; the second form then
 *         copies the planes to the second bitmap.
 *   NextFrame applies the current delta to the bitmap (per plane: column
 *         ops skip/uniq/same, MOVE or EOR) and goes to the next node;
 *         after the last one to the SECOND node.
 *   Frames: inline MOVE 8(a0),d0.
 *
 * Kept from the original (BUG(orig)):
 *   - the BODY is decoded as ByteRun1 whatever BMHD's compression says
 *     (#397); DBF counters: height/depth/width 0 run 65536 times (#402).
 *   - freeanim reads the next pointer of a delta node after freeing it. (#399)
 *   - LoadAnim: a missing BODY gives length -1 (memlib adds its 8 byte
 *     header, so AllocMem(7) succeeds, the read copies nothing and InitAnim
 *     decodes whatever follows the block) (#400); a single delta gives
 *     current = 0 (NextFrame would read address 0) (#401).  Not a bug:
 *     after the last delta the wrap goes to node 2 (the ANIM op-5
 *     double-buffer loop convention, the last two deltas lead back to
 *     frames 0/1).
 * Not a bug (statements, a3 has no meaning after them), reproduced: LoadAnim
 * leaves a3 on the last delta node (or anim+18), InitAnim behind the BODY
 * data / in the last plane of the second bitmap, NextFrame on the current
 * (or first) node.
 * Fixed (intended deviation, #398): the original's "Can't Open Anim File"
 * is `BEQ err7` into the error check area (behind the library without the
 * debugger), "Anim format not supported" (err6, in the normal code) loads
 * mess6 from the check area and runs on into err1 if the trap returns.
 * Here both traps and their texts are in the normal code: the same
 * runtime errors with the debugger, TRAP #0 with the right text without
 * it; if the trap returns, LoadAnim returns.
 * DIFF: a2 stays unchanged (port convention, notes/cmdlibs-port.md).
 */
#include "blitzlib.h"
#include "blitzobj.h"

/* entry points (thunks / C routines, defined below) */
struct anim;
void doframes(void);
void BLITZ_ROUTINE freeanim(void);
void BLITZ_ROUTINE doanimload(struct anim *anim BL_REG(a0), ULONG name BL_REG(d1));
void BLITZ_ROUTINE doanimload2(struct anim *anim BL_REG(a0), ULONG name BL_REG(d1),
                               ULONG pal BL_REG(a1));
void doinitanim(void), doinitanim2(void), donextframe(void);
void animchk(void), animchk2(void), animchk3(void), animchk4(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#animlib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_animlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_DUMTOKE(_toke, "Anim", "- IFF Anim");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_animlib, 0x1080);
BLITZ_SUBS((char *)doanimload + 1, 0, 0);       /* +1: Amiga mode only */
BLITZ_ARGS(BL_WORD, BL_STRING, BL_WORD);
BLITZ_LIBS(LIB_animlib, 0x1080, LIB_palettelib, 0x1182);
BLITZ_SUBS((char *)doanimload2 + 1, 0, 0);
BLITZ_NAME("LoadAnim", "Anim#,Filename$[,Palette#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_animlib, 0x1080, LIB_bitmaplib, 0x1102);
BLITZ_SUBS(doinitanim, animchk, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_animlib, 0x1080, LIB_bitmaplib, 0x1102, LIB_bitmaplib, 0x1281);
BLITZ_SUBS(doinitanim2, animchk2, 0);
BLITZ_NAME("InitAnim", "Anim#[,BitMap#]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_animlib, 0x1080, LIB_bitmaplib, 0x1102);
BLITZ_SUBS(donextframe, animchk3, 0);
BLITZ_NAME("NextFrame", "Anim#");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_animlib, 0x1080);
BLITZ_SUBS(doframes, animchk4, 0);
BLITZ_NAME("Frames", "(Anim#)");

BLITZ_NULLSUB_AT(init, 0, 0, 0);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, freeanim, 0, 0);

BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 10, 5);

/* ******************* CODE BELOW **************** */

/* a delta node: {next.l, length of the NEXT node.l, DLTA chunk} */
struct delta {
    struct delta *next;         /* +0 */
    ULONG nextlen;              /* +4: the length of the next node */
    UBYTE dlta[];               /* +8: the DLTA chunk (16 plane offsets.l,
                                   then the ops) */
};

/* an anim object (2^5 bytes); first/firstlen are laid out like a delta
   node (the head of the list) */
struct anim {
    UWORD width;                /* +0: 0 = not loaded */
    UWORD height;               /* +2 */
    UWORD depth;                /* +4 */
    UWORD ebwidth;              /* +6: even byte width (ILBM words * 2) */
    UWORD frames;               /* +8 */
    UBYTE *body;                /* +10: BODY (memlib globalloc) */
    ULONG bodylen;              /* +14 */
    struct delta *first;        /* +18: the first delta node */
    ULONG firstlen;             /* +22: its length */
    struct delta *current;      /* +26 */
    UWORD eor;                  /* +30: 2 = EOR deltas, 0 = MOVE */
};
static_assert(sizeof(struct anim) == 32);

/* the head of the delta list: the anim's +18 as a node */
#define HEADNODE(a) ((struct delta *)&(a)->first)

/* ilbmifflib's iffstuff (the part read here) */
struct iffstuff {
    UWORD width;                /* +0 */
    UWORD height;               /* +2 */
    UWORD planes;               /* +4 */
    UWORD words;                /* +6: width in words */
};

/* a bitmap object: struct bb_bitmap (blitzobj.h); the planes are read as
   longs, Flags/Depth as one word (flagsdepth) as in the original */

#define TOK_ilbminit   ((LIB_ilbmifflib << 7) + 1)  /* a0 = name -> a0 = info, d0 = 0: failed */
#define TOK_getpal     ((LIB_palettelib << 7) + 3)  /* a0 = name, a1 = palette */
#define TOK_iffclose   ((LIB_iffiolib << 7) + 2)
#define TOK_iffbyte    ((LIB_iffiolib << 7) + 3)
#define TOK_ifflong    ((LIB_iffiolib << 7) + 5)
#define TOK_ifffind    ((LIB_iffiolib << 7) + 6)    /* d0 = id -> d0 = length, NE: not found */
#define TOK_iffskip    ((LIB_iffiolib << 7) + 7)    /* d0 = number of bytes */
#define TOK_iffread    ((LIB_iffiolib << 7) + 8)    /* a0 = buffer, d0 = length */

/* ALibJsr that also returns the Z flag after the call (iffiolib reports
   "not found" only in the CCR) */
#define ALIBJSR_Z(tok, r) ({                                                \
    register ULONG _d0 __asm("d0") = (r).d0;                                \
    register ULONG _d1 __asm("d1") = (r).d1;                                \
    register ULONG _a0 __asm("a0") = (r).a0;                                \
    register ULONG _a1 __asm("a1") = (r).a1;                                \
    UBYTE _z;                                                               \
    __asm volatile("jsr _blitz_tok+%c5\n\tseq %4"                           \
        : "+d"(_d0), "+d"(_d1), "+a"(_a0), "+a"(_a1), "=m"(_z)             \
        : "i"(BL_TOKVAL(tok, 0))                                            \
        : "d2", "d3", "d4", "d5", "d6", "d7", "a2", "a6", "cc", "memory");  \
    (r).d0 = _d0; (r).d1 = _d1; (r).a0 = _a0; (r).a1 = _a1; _z; })

/* keeps GCC from replacing a pointer walk by a multiplication after the
   loop (__mulsi3, no libgcc in libraries) */
#define OPAQUE(x) __asm__("" : "+r"(x))

/* MULU.w as instruction (GCC would call __mulsi3) */
static inline ULONG mulu16(UWORD a, UWORD b)
{
    ULONG r = a;

    __asm__("mulu.w %1,%0" : "+d"(r) : "dmi"(b) : "cc");
    return r;
}

static ULONG iffcall(UWORD tok, ULONG count, ULONG buf)
{
    struct bl_regs r = { .d0 = count, .a0 = buf };

    switch (tok) {                      /* tokens must be constants */
    case 0: return BLITZ_ALIBJSR(TOK_iffbyte, r);
    case 1: return BLITZ_ALIBJSR(TOK_ifflong, r);
    case 2: return BLITZ_ALIBJSR(TOK_iffskip, r);
    case 3: return BLITZ_ALIBJSR(TOK_iffread, r);
    default: return BLITZ_ALIBJSR(TOK_iffclose, r);
    }
}
#define iffbyte()       iffcall(0, 0, 0)
#define ifflong()       iffcall(1, 0, 0)
#define iffskip(n)      iffcall(2, (n), 0)
#define iffread(a, n)   iffcall(3, (n), (ULONG)(a))
#define iffclose()      iffcall(4, 0, 0)

/* ifffind: d0 = length; *found = Z */
static ULONG ifffind(ULONG id, int *found)
{
    struct bl_regs r = { .d0 = id };

    *found = ALIBJSR_Z(TOK_ifffind, r);
    return r.d0;
}

static ULONG globalloc(ULONG size, ULONG flags)
{
    struct bl_regs r = { .d0 = size, .d1 = flags };

    return BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void globfree(ULONG mem, ULONG size)
{
    struct bl_regs r = { .d0 = size, .a1 = mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}

/* freeanim with a3 = the anim */
static void freeanim_c(struct anim *anim)
{
    struct delta *link;
    struct delta *next;                 /* d0: the next pointer */

    if (!anim->width)
        return;
    anim->width = 0;
    globfree((ULONG)anim->body, anim->bodylen);
    link = HEADNODE(anim);
    while ((next = link->next) != 0) {
        ULONG len = link->nextlen;      /* d0: the node length */

        link = next;
        globfree((ULONG)next, len);     /* BUG(orig) (#399): (a2) read after */
    }
}

/**
 * @brief Free Anim (the object's _free nullsub): free BODY and the delta
 *        nodes of the anim in a3.
 *
 * Only a loaded anim (width not 0); width is cleared.  BUG(orig) (#399):
 * the next pointer of a delta node is read after the node was freed.
 */
void BLITZ_ROUTINE freeanim(void)
{
    freeanim_c((struct anim *)blitz_string);
}

/**
 * @brief Frames(Anim#): the number of frames (inline MOVE 8(a0),d0).
 *
 * a0 = the anim (!libs {#animlib,$1080}); d0.w = the number of DLTA
 * chunks - 1 (1 without ANHD).  Error (debugger, animchk4): "Uninitialized
 * Anim".
 */
BLITZ_INLINE(doframes, "move.w 8(a0),d0");

/* the messages of err6/err7 in the normal code (fixed, #398: in the
   original they lie in the error check area) */
static const char mess6[] = "Anim format not supported";
static const char mess7[] = "Can't Open Anim File";

/**
 * @brief LoadAnim Anim#,Filename$: load an IFF ANIM (op 5) into the anim.
 *
 * Amiga mode only (code+1).  Frees the anim, opens the file with
 * ilbmifflib ilbminit (BMHD -> width, height, depth, byte width), reads
 * BODY, then ANHD (the operation byte must be 5; 19 bytes skipped, bit 1
 * of `bits` = EOR) and every following DLTA chunk into a delta node;
 * frames = number of DLTAs - 1 (1 without ANHD).  The file is read through
 * iffiolib.  a3 is left on the last delta node (or anim+18; not a bug).
 * Errors: "Can't Open Anim File" (err7), "Anim format not supported"
 * (err6), both in the normal code (fixed, #398: the original's err7 is in
 * the error check area, err6's text too).  BUG(orig): a missing BODY
 * gives length -1 (#400); a single delta gives current = 0 (#401).
 *
 * @param anim  a0: the anim (!libs {#animlib,$1080}, from Anim# in d0)
 * @param name  d1: the file name
 */
void BLITZ_ROUTINE doanimload(struct anim *anim BL_REG(a0), ULONG name BL_REG(d1))
{
    struct bl_regs r;
    const struct iffstuff *info;
    struct delta *node;
    ULONG bodylen;                      /* d0 */
    ULONG bodymem;                      /* d0 */
    ULONG dltalen;                      /* d0 */
    ULONG nodelen;                      /* d0: the DLTA length + 8 */
    struct delta *next;                 /* d0: the new node */
    ULONG len;                          /* d2 */
    int found;

    blitz_string = (UBYTE *)anim;       /* MOVE.l a0,a3:BSR freeanim */
    freeanim_c(anim);
    r.d0 = 0;
    r.d1 = name;
    r.a0 = name;
    r.a1 = 0;
    if (!BLITZ_ALIBJSR(TOK_ilbminit, r)) {
        BLITZ_RUNERR(mess7);            /* err7, fixed (#398) */
        return;
    }
    info = (const struct iffstuff *)r.a0;
    /* width and height with one MOVE.l */
    *(ULONG *)&anim->width = *(const ULONG *)&info->width;
    anim->depth = info->planes;
    anim->ebwidth = (UWORD)(info->words << 1);
    anim->frames = 1;
    bodylen = ifffind(0x424f4459, &found); /* "BODY" */
    anim->bodylen = bodylen;
    len = bodylen;
    bodymem = globalloc(bodylen, 1);
    anim->body = (UBYTE *)bodymem;
    iffread(bodymem, len);
    anim->current = 0;
    anim->eor = 0;
    node = HEADNODE(anim);
    blitz_string = (UBYTE *)node;
    node->next = 0;
    ifffind(0x414e4844, &found);        /* "ANHD" */
    if (found) {
        if ((UBYTE)iffbyte() != 5) {
            BLITZ_RUNERR(mess6);        /* err6, fixed (#398) */
            return;
        }
        iffskip(19);
        anim->eor = (UWORD)(ifflong() & 2);
        anim->frames = 0xffff;
        for (;;) {
            dltalen = ifffind(0x444c5441, &found); /* "DLTA" */
            if (!found)
                break;
            len = dltalen;
            anim->frames++;
            nodelen = dltalen + 8;
            node->nextlen = nodelen;
            next = (struct delta *)globalloc(nodelen, 1);
            node->next = next;
            node = next;
            blitz_string = (UBYTE *)node; /* a3 stays on the last node */
            node->next = 0;
            iffread(node->dlta, len);
        }
    }
    iffclose();
}

/**
 * @brief LoadAnim Anim#,Filename$,Palette#: LoadAnim, first the palette.
 *
 * Calls palettelib +3 (a0 = file name, a1 = palette), then LoadAnim.
 *
 * @param anim  a0: the anim (!libs {#animlib,$1080}, from Anim# in d0)
 * @param name  d1: the file name
 * @param pal   a1: the palette (!libs {#palettelib,$1182}, from Palette#
 *              in d2)
 */
void BLITZ_ROUTINE doanimload2(struct anim *anim BL_REG(a0), ULONG name BL_REG(d1),
                               ULONG pal BL_REG(a1))
{
    struct bl_regs r = { .d1 = name, .a0 = name, .a1 = pal };

    BLITZ_ALIBJSR(TOK_getpal, r);
    doanimload(anim, name);
}

/* InitAnim - a0 = anim, a1 = bitmap: BODY (ByteRun1) into the bitmap */
static void initanim(struct anim *anim, struct bb_bitmap *bm)
{
    UBYTE *src = anim->body;
    ULONG rowoff = 0;
    UWORD rows = anim->height - 1;

    do {                                            /* 'loop2: rows */
        UWORD planes = anim->depth - 1;
        ULONG *planep = (ULONG *)bm->planes;

        do {                                        /* 'loop3: planes */
            WORD left = anim->ebwidth - 1;
            UBYTE *dst = (UBYTE *)(*planep++ + rowoff);

            do {                                    /* 'loop4 */
                UWORD n = *src++;

                if (!(n & 0x80)) {
                    left -= n;
                    left -= 1;
                    do
                        *dst++ = *src++;
                    while ((WORD)--n != -1);
                } else {
                    UBYTE fill;

                    n = (UBYTE)-n;                  /* NEG.b */
                    if (n & 0x80)
                        continue;                   /* $80: nop */
                    left -= n;
                    left -= 1;
                    fill = *src++;
                    do
                        *dst++ = fill;
                    while ((WORD)--n != -1);
                }
            } while (left >= 0);                    /* 'more: TST d5:BPL */
        } while ((WORD)--planes != -1);
        rowoff += (LONG)(WORD)bm->bpr;              /* ADD (a1),a6 */
    } while ((WORD)--rows != -1);
    anim->current = anim->first;
    blitz_string = src;                 /* a3 stays behind the BODY data */
}

/**
 * @brief InitAnim Anim#: decode BODY into the used bitmap.
 *
 * BODY is decoded as ByteRun1 whatever BMHD's compression says (BUG(orig),
 * #397; DBF counters: height/depth/width 0 run 65536 times, #402); the
 * current delta becomes the first one.  a3 is left behind the BODY data
 * (not a bug).  Errors (debugger, animchk): "Uninitialized Anim",
 * "Uninitialized BitMap", "BitMap not large enough for ANIM".
 *
 * @param anim  a0: the anim (!libs {#animlib,$1080}, from Anim# in d0)
 * @param bm    a1: the used bitmap (!libs {#bitmaplib,$1102})
 */
static void BLITZ_ROUTINE doinitanim_c(struct anim *anim BL_REG(a0),
                                       struct bb_bitmap *bm BL_REG(a1))
{
    initanim(anim, bm);
}
BLITZ_THUNK(doinitanim, doinitanim_c, "d0-d1/a0-a1");

/**
 * @brief InitAnim Anim#,BitMap#: InitAnim, then the planes copied to the
 *        second bitmap (double buffering).
 *
 * a3 is left in the last plane of the second bitmap (not a bug).  Errors
 * (debugger, animchk2): as InitAnim, checked for the second bitmap.
 *
 * @param anim  a0: the anim (!libs {#animlib,$1080}, from Anim# in d0)
 * @param bm    a1: the used bitmap (!libs {#bitmaplib,$1102})
 * @param bm2   a2: the second bitmap (!libs {#bitmaplib,$1281}, from
 *              BitMap# in d1)
 */
static void BLITZ_ROUTINE doinitanim2_c(struct anim *anim BL_REG(a0),
                                        struct bb_bitmap *bm BL_REG(a1),
                                        struct bb_bitmap *bm2 BL_REG(a2))
{
    WORD bpr, bpr2, width;
    UWORD words, planes;
    ULONG *p1, *p2;

    initanim(anim, bm);
    bpr = bm->bpr;
    p1 = (ULONG *)bm->planes;
    bpr2 = bm2->bpr;
    p2 = (ULONG *)bm2->planes;
    width = anim->ebwidth;
    words = (UWORD)((UWORD)width >> 1) - 1;
    planes = anim->depth - 1;
    do {
        UWORD *src = (UWORD *)*p1++;
        UWORD *dst = (UWORD *)*p2++;
        UWORD rows = anim->height - 1;

        do {
            UWORD n = words;

            do
                *dst++ = *src++;
            while ((WORD)--n != -1);
            src = (UWORD *)((UBYTE *)src - width + bpr); /* SUB d3,a3:ADD d0,a3 */
            dst = (UWORD *)((UBYTE *)dst - width + bpr2);
        } while ((WORD)--rows != -1);
        blitz_string = (UBYTE *)src;    /* a3 stays in the last plane */
    } while ((WORD)--planes != -1);
}
BLITZ_THUNK(doinitanim2, doinitanim2_c, "d0-d1/a0-a1");

/**
 * @brief NextFrame Anim#: apply the current delta to the used bitmap.
 *
 * Per plane the column ops skip/uniq/same, written (MOVE) or EORed; then
 * the next node, after the last one the SECOND node (not a bug: the ANIM
 * op-5 double-buffer loop, the last two deltas lead back to frames 0/1).
 * a3 is left on the current (or first) node (not a bug).  Errors
 * (debugger, animchk3): "Anim has only 1 frame", "Anim not Initialized",
 * then as InitAnim.
 *
 * @param anim  a0: the anim (!libs {#animlib,$1080}, from Anim# in d0)
 * @param bm    a1: the used bitmap (!libs {#bitmaplib,$1102})
 */
static void BLITZ_ROUTINE donextframe_c(struct anim *anim BL_REG(a0),
                                        struct bb_bitmap *bm BL_REG(a1))
{
    ULONG *planep = (ULONG *)bm->planes;
    UWORD planes = anim->depth - 1;
    /* the current node read as a long: as a pointer member GCC gives
       other code */
    struct delta *node = (struct delta *)*(ULONG *)&anim->current;
    ULONG *offs = (ULONG *)node->dlta;
    WORD bpr = bm->bpr;
    UWORD cols = anim->ebwidth - 1;
    ULONG delta;                        /* d0: the delta offset */
    struct delta *next;                 /* d0: the next node */

    do {                                            /* 'loop: planes */
        UBYTE *column = (UBYTE *)*planep++;
        UBYTE *ops;
        UWORD colsleft;

        delta = *offs++;
        if (!delta)
            continue;                               /* 'more */
        ops = node->dlta + delta;
        colsleft = cols;
        do {                                        /* 'loop2: columns */
            UWORD nops = *ops++;                    /* number of ops */

            if (nops) {
                UBYTE *col = column;                /* MOVE.l a6,-(a7) */

                nops -= 1;
                do {
                    UWORD op = *ops++;

                    if (!op) {                      /* same: count, value */
                        UWORD count = *ops++ - 1;
                        UBYTE value = *ops++;

                        do {
                            if (anim->eor)
                                *col ^= value;
                            else
                                *col = value;
                            col += bpr;
                            OPAQUE(col);
                        } while ((WORD)--count != -1);
                    } else if (op & 0x80) {         /* uniq: count & 127 bytes */
                        UWORD n = (op & 127) - 1;

                        do {
                            UBYTE value = *ops++;

                            if (anim->eor)
                                *col ^= value;
                            else
                                *col = value;
                            col += bpr;
                            OPAQUE(col);
                        } while ((WORD)--n != -1);
                    } else                          /* skip: MULU d4,d0 */
                        col += mulu16(op, bpr);
                } while ((WORD)--nops != -1);
            }
            column += 1;                            /* 'more2 */
        } while ((WORD)--colsleft != -1);
    } while ((WORD)--planes != -1);
    next = node->next;
    if (!next) {
        node = anim->first;
        next = node->next;
    }
    anim->current = next;
    blitz_string = (UBYTE *)node;       /* a3 stays on the (first) node */
}
BLITZ_THUNK(donextframe, donextframe_c, "d0-d1/a0-a1");

/* ****************** ERRORS BELOW **************** */

/* err1..err5 as in the original (err7 moved into doanimload, #398) */
__asm__("   .section .blitzlib.errchk,\"ax\"\n   .even\n"
        "_err1:   move.l  #_mess1,d0\n   trap    #0\n"
        "_err2:   move.l  #_mess2,d0\n   trap    #0\n"
        "_err3:   move.l  #_mess3,d0\n   trap    #0\n"
        "_err4:   move.l  #_mess4,d0\n   trap    #0\n"
        "_err5:   move.l  #_mess5,d0\n   trap    #0\n"
        "   .text\n");

BLITZ_ERRCHK_MESSAGE(mess1, "BitMap not large enough for ANIM");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess3, "Uninitialized Anim");
BLITZ_ERRCHK_MESSAGE(mess4, "Anim not Initialized");
BLITZ_ERRCHK_MESSAGE(mess5, "Anim has only 1 frame");

/* animchk: the anim at a0 fits into the bitmap at a1 (signed words) */
static void BLITZ_ERRCHK animchk_c(struct anim *anim BL_REG(a0),
                                   struct bb_bitmap *bm BL_REG(a1))
{
    if (!anim->width)
        BLITZ_RUNERR(mess3);
    else if (!bm->bpr)
        BLITZ_RUNERR(mess2);
    else if ((WORD)anim->ebwidth > (WORD)bm->bpr
             || (WORD)anim->height > (WORD)bm->rows
             || (WORD)anim->depth > (WORD)bm->flagsdepth)
        BLITZ_RUNERR(mess1);
}

/* animchk2: EXG a1,a2:BSR animchk:EXG a1,a2 */
static void BLITZ_ERRCHK animchk2_c(struct anim *anim BL_REG(a0),
                                    struct bb_bitmap *bm2 BL_REG(a2))
{
    animchk_c(anim, bm2);
}

static void BLITZ_ERRCHK animchk3_c(struct anim *anim BL_REG(a0),
                                    struct bb_bitmap *bm BL_REG(a1))
{
    if ((WORD)anim->frames < 2)
        BLITZ_RUNERR(mess5);
    else if (!anim->current)
        BLITZ_RUNERR(mess4);
    else
        animchk_c(anim, bm);
}

static void BLITZ_ERRCHK animchk4_c(struct anim *anim BL_REG(a0))
{
    if (!anim->width)
        BLITZ_RUNERR(mess3);
}

/* the originals change d7 (and d0 on an error) */
BLITZ_ERRCHK_THUNK(animchk, animchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(animchk2, animchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(animchk3, animchk3_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(animchk4, animchk4_c, "d0-d1/a0-a1");
