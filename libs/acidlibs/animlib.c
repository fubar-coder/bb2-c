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
void BLITZ_ROUTINE doanimload(UBYTE *a4 BL_REG(a0), ULONG name BL_REG(d1));
void BLITZ_ROUTINE doanimload2(UBYTE *a4 BL_REG(a0), ULONG name BL_REG(d1),
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

static ULONG iffcall(UWORD tok, ULONG d0, ULONG a0)
{
    struct bl_regs r = { .d0 = d0, .a0 = a0 };

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
static void freeanim_c(UBYTE *a3)
{
    UBYTE *a2;
    ULONG d0;

    if (!W(a3, 0))
        return;
    W(a3, 0) = 0;
    globfree(L(a3, 10), L(a3, 14));
    a2 = a3 + 18;
    while ((d0 = L(a2, 0)) != 0) {
        UBYTE *a1 = (UBYTE *)d0;

        d0 = L(a2, 4);
        a2 = a1;
        globfree((ULONG)a1, d0);        /* BUG(orig) (#399): (a2) read after this */
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
void BLITZ_ROUTINE doanimload(UBYTE *a4 BL_REG(a0), ULONG name BL_REG(d1))
{
    struct bl_regs r;
    UBYTE *info, *a3;
    ULONG d0, d2;
    int found;

    blitz_string = a4;                  /* MOVE.l a0,a3:BSR freeanim */
    freeanim_c(a4);
    r.d0 = 0;
    r.d1 = name;
    r.a0 = name;
    r.a1 = 0;
    if (!BLITZ_ALIBJSR(TOK_ilbminit, r))
        GOTO(err7);                     /* BUG(orig) (#398): into the check area */
    info = (UBYTE *)r.a0;
    L(a4, 0) = L(info, 0);
    W(a4, 4) = W(info, 4);
    W(a4, 6) = (UWORD)(W(info, 6) << 1);
    W(a4, 8) = 1;
    d0 = ifffind(0x424f4459, &found);   /* "BODY" */
    L(a4, 14) = d0;
    d2 = d0;
    d0 = globalloc(d0, 1);
    L(a4, 10) = d0;
    iffread(d0, d2);
    L(a4, 26) = 0;
    W(a4, 30) = 0;
    a3 = a4 + 18;
    blitz_string = a3;
    L(a3, 0) = 0;
    ifffind(0x414e4844, &found);        /* "ANHD" */
    if (found) {
        if ((UBYTE)iffbyte() != 5)
            GOTO(err6);
        iffskip(19);
        W(a4, 30) = (UWORD)(ifflong() & 2);
        W(a4, 8) = 0xffff;
        for (;;) {
            d0 = ifffind(0x444c5441, &found);  /* "DLTA" */
            if (!found)
                break;
            d2 = d0;
            W(a4, 8)++;
            d0 += 8;
            L(a3, 4) = d0;
            d0 = globalloc(d0, 1);
            L(a3, 0) = d0;
            a3 = (UBYTE *)d0;
            blitz_string = a3;          /* a3 stays on the last node */
            L(a3, 0) = 0;
            iffread(a3 + 8, d2);
        }
    }
    iffclose();
}

/* LoadAnim with palette - d2 = palette (a1) */
void BLITZ_ROUTINE doanimload2(UBYTE *a4 BL_REG(a0), ULONG name BL_REG(d1),
                               ULONG pal BL_REG(a1))
{
    struct bl_regs r = { .d1 = name, .a0 = name, .a1 = pal };

    BLITZ_ALIBJSR(TOK_getpal, r);
    doanimload(a4, name);
}

/* InitAnim - a0 = anim, a1 = bitmap: BODY (ByteRun1) into the bitmap */
static void initanim(UBYTE *a0, UBYTE *a1)
{
    UBYTE *a3 = (UBYTE *)L(a0, 10);
    ULONG a6 = 0;
    UWORD d7 = W(a0, 2) - 1;

    do {                                            /* 'loop2: rows */
        UWORD d6 = W(a0, 4) - 1;
        ULONG *a5 = (ULONG *)(a1 + 8);

        do {                                        /* 'loop3: planes */
            WORD d5 = W(a0, 6) - 1;
            UBYTE *a4 = (UBYTE *)(*a5++ + a6);

            do {                                    /* 'loop4 */
                UWORD d0 = *a3++;

                if (!(d0 & 0x80)) {
                    d5 -= d0;
                    d5 -= 1;
                    do
                        *a4++ = *a3++;
                    while ((WORD)--d0 != -1);
                } else {
                    UBYTE d1;

                    d0 = (UBYTE)-d0;                /* NEG.b */
                    if (d0 & 0x80)
                        continue;                   /* $80: nop */
                    d5 -= d0;
                    d5 -= 1;
                    d1 = *a3++;
                    do
                        *a4++ = d1;
                    while ((WORD)--d0 != -1);
                }
            } while (d5 >= 0);                      /* 'more: TST d5:BPL */
        } while ((WORD)--d6 != -1);
        a6 += (LONG)(WORD)W(a1, 0);                 /* ADD (a1),a6 */
    } while ((WORD)--d7 != -1);
    L(a0, 26) = L(a0, 18);
    blitz_string = a3;                  /* a3 stays behind the BODY data */
}

static void BLITZ_ROUTINE doinitanim_c(UBYTE *a0 BL_REG(a0), UBYTE *a1 BL_REG(a1))
{
    initanim(a0, a1);
}
BLITZ_THUNK(doinitanim, doinitanim_c, "d0-d1/a0-a1");

/* InitAnim with second bitmap: a1 = used bitmap, a2 = second bitmap */
static void BLITZ_ROUTINE doinitanim2_c(UBYTE *a0 BL_REG(a0), UBYTE *a1 BL_REG(a1),
                                        UBYTE *a2 BL_REG(a2))
{
    WORD d0, d1, d3;
    UWORD d2, d7;
    ULONG *p1, *p2;

    initanim(a0, a1);
    d0 = W(a1, 0);
    p1 = (ULONG *)(a1 + 8);
    d1 = W(a2, 0);
    p2 = (ULONG *)(a2 + 8);
    d3 = W(a0, 6);
    d2 = (UWORD)((UWORD)d3 >> 1) - 1;
    d7 = W(a0, 4) - 1;
    do {
        UWORD *a3 = (UWORD *)*p1++;
        UWORD *a4 = (UWORD *)*p2++;
        UWORD d6 = W(a0, 2) - 1;

        do {
            UWORD d5 = d2;

            do
                *a4++ = *a3++;
            while ((WORD)--d5 != -1);
            a3 = (UWORD *)((UBYTE *)a3 - d3 + d0);  /* SUB d3,a3:ADD d0,a3 */
            a4 = (UWORD *)((UBYTE *)a4 - d3 + d1);
        } while ((WORD)--d6 != -1);
        blitz_string = (UBYTE *)a3;     /* a3 stays in the last plane */
    } while ((WORD)--d7 != -1);
}
BLITZ_THUNK(doinitanim2, doinitanim2_c, "d0-d1/a0-a1");

/* NextFrame - a0 = anim, a1 = bitmap */
static void BLITZ_ROUTINE donextframe_c(UBYTE *a0 BL_REG(a0), UBYTE *a1 BL_REG(a1))
{
    ULONG *a2 = (ULONG *)(a1 + 8);
    UWORD d7 = W(a0, 4) - 1;
    UBYTE *a3 = (UBYTE *)L(a0, 26);
    ULONG *a4 = (ULONG *)(a3 + 8);
    WORD d4 = W(a1, 0);
    UWORD d3 = W(a0, 6) - 1;
    ULONG d0;

    do {                                            /* 'loop: planes */
        UBYTE *a6 = (UBYTE *)*a2++;
        UBYTE *a5;
        UWORD d6;

        d0 = *a4++;
        if (!d0)
            continue;                               /* 'more */
        a5 = a3 + 8 + d0;
        d6 = d3;
        do {                                        /* 'loop2: columns */
            UWORD d5 = *a5++;                       /* number of ops */

            if (d5) {
                UBYTE *col = a6;                    /* MOVE.l a6,-(a7) */

                d5 -= 1;
                do {
                    UWORD op = *a5++;

                    if (!op) {                      /* same: count, value */
                        UWORD d1 = *a5++ - 1;
                        UBYTE d2 = *a5++;

                        do {
                            if (W(a0, 30))
                                *col ^= d2;
                            else
                                *col = d2;
                            col += d4;
                            OPAQUE(col);
                        } while ((WORD)--d1 != -1);
                    } else if (op & 0x80) {         /* uniq: count & 127 bytes */
                        UWORD n = (op & 127) - 1;

                        do {
                            UBYTE d1 = *a5++;

                            if (W(a0, 30))
                                *col ^= d1;
                            else
                                *col = d1;
                            col += d4;
                            OPAQUE(col);
                        } while ((WORD)--n != -1);
                    } else                          /* skip: MULU d4,d0 */
                        col += mulu16(op, d4);
                } while ((WORD)--d5 != -1);
            }
            a6 += 1;                                /* 'more2 */
        } while ((WORD)--d6 != -1);
    } while ((WORD)--d7 != -1);
    d0 = L(a3, 0);
    if (!d0) {
        a3 = (UBYTE *)L(a0, 18);
        d0 = L(a3, 0);
    }
    L(a0, 26) = d0;
    blitz_string = a3;                  /* a3 stays on the (first) node */
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
static void BLITZ_ERRCHK animchk_c(UBYTE *a0 BL_REG(a0), UBYTE *a1 BL_REG(a1))
{
    if (!W(a0, 0))
        BLITZ_RUNERR(mess3);
    else if (!W(a1, 0))
        BLITZ_RUNERR(mess2);
    else if ((WORD)W(a0, 6) > (WORD)W(a1, 0) || (WORD)W(a0, 2) > (WORD)W(a1, 2)
             || (WORD)W(a0, 4) > (WORD)W(a1, 4))
        BLITZ_RUNERR(mess1);
}

/* animchk2: EXG a1,a2:BSR animchk:EXG a1,a2 */
static void BLITZ_ERRCHK animchk2_c(UBYTE *a0 BL_REG(a0), UBYTE *a2 BL_REG(a2))
{
    animchk_c(a0, a2);
}

static void BLITZ_ERRCHK animchk3_c(UBYTE *a0 BL_REG(a0), UBYTE *a1 BL_REG(a1))
{
    if ((WORD)W(a0, 8) < 2)
        BLITZ_RUNERR(mess5);
    else if (!L(a0, 26))
        BLITZ_RUNERR(mess4);
    else
        animchk_c(a0, a1);
}

static void BLITZ_ERRCHK animchk4_c(UBYTE *a0 BL_REG(a0))
{
    if (!W(a0, 0))
        BLITZ_RUNERR(mess3);
}

/* the originals change d7 (and d0 on an error) */
BLITZ_ERRCHK_THUNK(animchk, animchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(animchk2, animchk2_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(animchk3, animchk3_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(animchk4, animchk4_c, "d0-d1/a0-a1");
