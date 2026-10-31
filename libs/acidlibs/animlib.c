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
 *   - "Can't Open Anim File": BEQ err7 into the error check area, i.e.
 *     behind the library without the debugger (reproduced: JSR to err7
 *     in the check area); "Anim format not supported" (err6, in the
 *     normal code) loads mess6 from the check area and runs on into err1. (#398)
 * Not a bug (statements, a3 has no meaning after them), reproduced: LoadAnim
 * leaves a3 on the last delta node (or anim+18), InitAnim behind the BODY
 * data / in the last plane of the second bitmap, NextFrame on the current
 * (or first) node.
 * DIFF: a2 stays unchanged (port convention, notes/cmdlibs-port.md); err6/
 * err7 are reached by JSR (return address and the C function's saved
 * registers on the stack - GCC drops code behind an inline BRA).
 */
#include "blitzlib.h"

/* entry points (thunks / C routines, defined below) */
void doframes(void);
void BLITZ_ROUTINE freeanim(void);
void BLITZ_ROUTINE doanimload(UBYTE *anim BL_REG(a0), ULONG name BL_REG(d1));
void BLITZ_ROUTINE doanimload2(UBYTE *anim BL_REG(a0), ULONG name BL_REG(d1),
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

#define W(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))
#define L(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))

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
static void freeanim_c(UBYTE *anim)
{
    UBYTE *link;
    ULONG nodeval;                      /* d0: next pointer, then length */

    if (!W(anim, 0))
        return;
    W(anim, 0) = 0;
    globfree(L(anim, 10), L(anim, 14));
    link = anim + 18;
    while ((nodeval = L(link, 0)) != 0) {
        UBYTE *next = (UBYTE *)nodeval;

        nodeval = L(link, 4);
        link = next;
        globfree((ULONG)next, nodeval); /* BUG(orig) (#399): (a2) read after */
    }
}

/* _free nullsub: a3 = object */
void BLITZ_ROUTINE freeanim(void)
{
    freeanim_c(blitz_string);
}

/* Frames: MOVE 8(a0),d0 */
BLITZ_INLINE(doframes, "move.w 8(a0),d0");

/* err6 (normal code, its message in the error check area) and err7 (in
   the error check area, see below): reached by a branch, never left */
void err6(void) __attribute__((noreturn)), err7(void) __attribute__((noreturn));
#define GOTO(label) label()     /* JSR to the noreturn label, see displaylib.c */

/* LoadAnim - a0 = anim, d1 = file name */
void BLITZ_ROUTINE doanimload(UBYTE *anim BL_REG(a0), ULONG name BL_REG(d1))
{
    struct bl_regs r;
    UBYTE *info, *node;
    ULONG val, len;                     /* d0, d2 */
    int found;

    blitz_string = anim;                /* MOVE.l a0,a3:BSR freeanim */
    freeanim_c(anim);
    r.d0 = 0;
    r.d1 = name;
    r.a0 = name;
    r.a1 = 0;
    if (!BLITZ_ALIBJSR(TOK_ilbminit, r))
        GOTO(err7);                     /* BUG(orig) (#398): into the check area */
    info = (UBYTE *)r.a0;
    L(anim, 0) = L(info, 0);
    W(anim, 4) = W(info, 4);
    W(anim, 6) = (UWORD)(W(info, 6) << 1);
    W(anim, 8) = 1;
    val = ifffind(0x424f4459, &found);  /* "BODY" */
    L(anim, 14) = val;
    len = val;
    val = globalloc(val, 1);
    L(anim, 10) = val;
    iffread(val, len);
    L(anim, 26) = 0;
    W(anim, 30) = 0;
    node = anim + 18;
    blitz_string = node;
    L(node, 0) = 0;
    ifffind(0x414e4844, &found);        /* "ANHD" */
    if (found) {
        if ((UBYTE)iffbyte() != 5)
            GOTO(err6);
        iffskip(19);
        W(anim, 30) = (UWORD)(ifflong() & 2);
        W(anim, 8) = 0xffff;
        for (;;) {
            val = ifffind(0x444c5441, &found); /* "DLTA" */
            if (!found)
                break;
            len = val;
            W(anim, 8)++;
            val += 8;
            L(node, 4) = val;
            val = globalloc(val, 1);
            L(node, 0) = val;
            node = (UBYTE *)val;
            blitz_string = node;        /* a3 stays on the last node */
            L(node, 0) = 0;
            iffread(node + 8, len);
        }
    }
    iffclose();
}

/* LoadAnim with palette - d2 = palette (a1) */
void BLITZ_ROUTINE doanimload2(UBYTE *anim BL_REG(a0), ULONG name BL_REG(d1),
                               ULONG pal BL_REG(a1))
{
    struct bl_regs r = { .d1 = name, .a0 = name, .a1 = pal };

    BLITZ_ALIBJSR(TOK_getpal, r);
    doanimload(anim, name);
}

/* InitAnim - a0 = anim, a1 = bitmap: BODY (ByteRun1) into the bitmap */
static void initanim(UBYTE *anim, UBYTE *bm)
{
    UBYTE *src = (UBYTE *)L(anim, 10);
    ULONG rowoff = 0;
    UWORD rows = W(anim, 2) - 1;

    do {                                            /* 'loop2: rows */
        UWORD planes = W(anim, 4) - 1;
        ULONG *planep = (ULONG *)(bm + 8);

        do {                                        /* 'loop3: planes */
            WORD left = W(anim, 6) - 1;
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
        rowoff += (LONG)(WORD)W(bm, 0);             /* ADD (a1),a6 */
    } while ((WORD)--rows != -1);
    L(anim, 26) = L(anim, 18);
    blitz_string = src;                 /* a3 stays behind the BODY data */
}

static void BLITZ_ROUTINE doinitanim_c(UBYTE *anim BL_REG(a0),
                                       UBYTE *bm BL_REG(a1))
{
    initanim(anim, bm);
}
BLITZ_THUNK(doinitanim, doinitanim_c, "d0-d1/a0-a1");

/* InitAnim with second bitmap: a1 = used bitmap, a2 = second bitmap */
static void BLITZ_ROUTINE doinitanim2_c(UBYTE *anim BL_REG(a0),
                                        UBYTE *bm BL_REG(a1),
                                        UBYTE *bm2 BL_REG(a2))
{
    WORD bpr, bpr2, width;
    UWORD words, planes;
    ULONG *p1, *p2;

    initanim(anim, bm);
    bpr = W(bm, 0);
    p1 = (ULONG *)(bm + 8);
    bpr2 = W(bm2, 0);
    p2 = (ULONG *)(bm2 + 8);
    width = W(anim, 6);
    words = (UWORD)((UWORD)width >> 1) - 1;
    planes = W(anim, 4) - 1;
    do {
        UWORD *src = (UWORD *)*p1++;
        UWORD *dst = (UWORD *)*p2++;
        UWORD rows = W(anim, 2) - 1;

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

/* NextFrame - a0 = anim, a1 = bitmap */
static void BLITZ_ROUTINE donextframe_c(UBYTE *anim BL_REG(a0),
                                        UBYTE *bm BL_REG(a1))
{
    ULONG *planep = (ULONG *)(bm + 8);
    UWORD planes = W(anim, 4) - 1;
    UBYTE *node = (UBYTE *)L(anim, 26);
    ULONG *offs = (ULONG *)(node + 8);
    WORD bpr = W(bm, 0);
    UWORD cols = W(anim, 6) - 1;
    ULONG val;                          /* d0: delta offset, then next node */

    do {                                            /* 'loop: planes */
        UBYTE *column = (UBYTE *)*planep++;
        UBYTE *ops;
        UWORD colsleft;

        val = *offs++;
        if (!val)
            continue;                               /* 'more */
        ops = node + 8 + val;
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
                            if (W(anim, 30))
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

                            if (W(anim, 30))
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
    val = L(node, 0);
    if (!val) {
        node = (UBYTE *)L(anim, 18);
        val = L(node, 0);
    }
    L(anim, 26) = val;
    blitz_string = node;                /* a3 stays on the (first) node */
}
BLITZ_THUNK(donextframe, donextframe_c, "d0-d1/a0-a1");

/* err6:MOVE.l #mess6,d0:TRAP #0 - the last code before `errors`: after
   the trap the original runs on into err1 (the start of the check area,
   without the debugger whatever follows the library) */
__asm__("   .text\n   .even\n   .globl  _err6\n"
        "_err6:\n"
        "   move.l  #_mess6,d0\n"
        "   trap    #0\n"
        "   bra.w   _err1\n");

/* ****************** ERRORS BELOW **************** */

/* err1..err5, err7 as in the original (err7 is also the target of
   LoadAnim's BEQ, see doanimload) */
__asm__("   .section .blitzlib.errchk,\"ax\"\n   .even\n"
        "_err1:   move.l  #_mess1,d0\n   trap    #0\n"
        "_err2:   move.l  #_mess2,d0\n   trap    #0\n"
        "_err3:   move.l  #_mess3,d0\n   trap    #0\n"
        "_err4:   move.l  #_mess4,d0\n   trap    #0\n"
        "_err5:   move.l  #_mess5,d0\n   trap    #0\n"
        "   .globl  _err7\n"
        "_err7:   move.l  #_mess7,d0\n   trap    #0\n"
        "   .text\n");

BLITZ_ERRCHK_MESSAGE(mess1, "BitMap not large enough for ANIM");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess3, "Uninitialized Anim");
BLITZ_ERRCHK_MESSAGE(mess4, "Anim not Initialized");
BLITZ_ERRCHK_MESSAGE(mess5, "Anim has only 1 frame");
BLITZ_ERRCHK_MESSAGE(mess6, "Anim format not supported");
BLITZ_ERRCHK_MESSAGE(mess7, "Can't Open Anim File");

/* animchk: the anim at a0 fits into the bitmap at a1 (signed words) */
static void BLITZ_ERRCHK animchk_c(UBYTE *anim BL_REG(a0),
                                   UBYTE *bm BL_REG(a1))
{
    if (!W(anim, 0))
        BLITZ_RUNERR(mess3);
    else if (!W(bm, 0))
        BLITZ_RUNERR(mess2);
    else if ((WORD)W(anim, 6) > (WORD)W(bm, 0)
             || (WORD)W(anim, 2) > (WORD)W(bm, 2)
             || (WORD)W(anim, 4) > (WORD)W(bm, 4))
        BLITZ_RUNERR(mess1);
}

/* animchk2: EXG a1,a2:BSR animchk:EXG a1,a2 */
static void BLITZ_ERRCHK animchk2_c(UBYTE *anim BL_REG(a0),
                                    UBYTE *bm2 BL_REG(a2))
{
    animchk_c(anim, bm2);
}

static void BLITZ_ERRCHK animchk3_c(UBYTE *anim BL_REG(a0),
                                    UBYTE *bm BL_REG(a1))
{
    if ((WORD)W(anim, 8) < 2)
        BLITZ_RUNERR(mess5);
    else if (!L(anim, 26))
        BLITZ_RUNERR(mess4);
    else
        animchk_c(anim, bm);
}

static void BLITZ_ERRCHK animchk4_c(UBYTE *anim BL_REG(a0))
{
    if (!W(anim, 0))
        BLITZ_RUNERR(mess3);
}

/* the originals change d7 (and d0 on an error) */
BLITZ_ERRCHK_THUNK(animchk, animchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(animchk2, animchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(animchk3, animchk3_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(animchk4, animchk4_c, "d0-d1/a0-a1");
