/*
 * ricopperfxlib - RWE userlib 11 (#ricopperfx): custom copper lists for
 * the Blitz mode display library (displaylib 143) - colour splits with
 * AGA 24 bit colours (DoColSplit, RedoColSplit), single copper
 * instructions (CopperMove, CopperWait, CopperSkip, CopperCommand,
 * CopperAGACol, CopperEnd) written into the custom copper space of a
 * coplist, and their bookkeeping (CopperReset, CopperResetAdr,
 * CopperInfoBlock, GetCCOffset, ColSplitBplcon3).
 *
 * C translation of rwelibsrc/Copperlib/CopperFX3.bb (Reflective Images,
 * 8/3/95); the declarations equal the CD's BSS2 RICOPPERFXLIB.OBJ
 * (ctest rwe_declarations_ricopperfxlib), the behaviour is compared with
 * it in libs/tests/rwe/ricopperfxlib.c.  Notes:
 * docs/rwelibs/port/ricopperfxlib.md.
 *
 * Nothing here touches the hardware: the routines only write copper
 * instructions into memory (the custom copper space), so everything is C.
 * The word arithmetic of the original (ASR.w, ROL.b, ADD.w, DBRA) is kept
 * with UWORD/UBYTE operands, DIVS/MULS through bl_divs_w/bl_muls_w.
 *
 * State (struct cfx, CopperInfoBlock = its address; the layout is the
 * original's, a program may read it):
 *   coplistaddr  coplist of CopperReset (0 after CopperResetAdr)
 *   customaddr   start of the custom copper space (CopperReset: the long
 *                at coplist+28), 0 = none: then every command does nothing
 *   startline    line of the next colour split
 *   ccoffset     byte offset of the next instruction in the custom space
 *                (the arguments ccoffset of CopperReset and RedoColSplit
 *                and GetCCOffset count instructions = 4 bytes)
 *   bplcon3LO/HI BPLCON3 selecting the low/high nibbles of the colour
 *                bank of the last DoColSplit (| lo_bplcon3)
 *   colreg       colour register of the last DoColSplit
 *   lo_bplcon3   low byte of BPLCON3 (ColSplitBplcon3); the original
 *                writes and reads it as the first byte of a word
 *                (MOVE.b d0,lo_bplcon3 with lo_bplcon3: Dc.w 0)
 *
 * BUG(orig):
 *   - GetCCOffset tests coplistaddr, not customaddr: after CopperResetAdr
 *     (coplistaddr 0) it returns -1 although the commands work (#593).
 *   - DoColSplit is a command with a word result, but d0 is the red step
 *     value at the end (its cols_adr argument without a custom space);
 *     CopperAGACol (long result) returns 1234 without a custom space,
 *     otherwise its col# argument (#592, #590).
 *   - DoColSplit/RedoColSplit: numlines is not checked (0: DIVS by zero,
 *     the divide-by-zero exception as in the original; negative: the DBRA
 *     counter numlines-1 as a word, up to 65536 lines); the step
 *     (1 << 14) / numlines is a word quotient; the $FFE1FFFE wait (line
 *     256) is written whenever the low byte of the line is 0, also for
 *     line 0 (#589, #588).
 *   - RedoColSplit: ccoffset * 4 is ASL.w (only the low word is
 *     shifted), the upper word of the argument is added unchanged (#594).
 *   - CopperWait: the x position is ((x >> 1) & $FE) + 6 as a word, so the
 *     +6 can carry into the line bits (x >= 500) (#591).
 * Registers: the original changes d6/d7/a0 (and more) in every command,
 * but keeps others depending on the routine (a1 nearly always, d0/d1 in
 * the statements); the thunks keep all of d0/d1/a0/a1 that are not the
 * result, GCC keeps d2-d7/a2-a6.
 */
#include "blitzlib.h"

/* custom chip registers (custom.res) */
#define DMACON  0x096
#define BPLCON3 0x106
#define COLOR00 0x180
/* #bplspr_dmaoff: switch off bitplane and sprite DMA; #AGAhighnibble /
   #AGAlownibble: BPLCON3 with LOCT clear / set (high / low nibbles) */
#define BPLSPR_DMAOFF (((ULONG)DMACON << 16) + 0x120)
#define AGAHIGHNIBBLE (((ULONG)BPLCON3 << 16) + 0xc00)
#define AGALOWNIBBLE  (((ULONG)BPLCON3 << 16) + 0xe00)
/* #calcfactor = 1 ASL #rolfactor; the colour values are shifted back by
   #rolfactor1 + #rolfactor2 (8 + 6) */
#define ROLFACTOR  14
#define CALCFACTOR (1 << ROLFACTOR)

/* entries (thunks below) */
void libinit(void), libfinit(void);
void docolsplit(void), redocolsplit(void), splitreset2(void), splitreset(void);
void EndSplit(void), InfoBlock(void), CustomCommand(void), CustomMove(void);
void CustomWait(void), CustomSkip(void), getccoffset(void), CustomAGACol(void);
void splitresethard(void), splitbplcon3(void);

BLITZ_ENTRY_DECL(finit);
BLITZ_ENTRY_DECL(init);

/* !libheader {#ricopperfx,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ricopperfx, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS(BL_LONG, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(docolsplit, 0, 0);
BLITZ_NAME("DoColSplit", "cols_adr,numlines,colour_register");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(redocolsplit, 0, 0);
BLITZ_NAME("RedoColSplit", "cols_adr,numlines,ccoffset");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, BL_LA(0) | BL_ITEM | BL_PD(0));   /* #ia0|#pd0 */
BLITZ_SUBS(splitreset2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_displaylib, BL_LA(0) | BL_ITEM | BL_PD(0));
BLITZ_SUBS(splitreset, 0, 0);
BLITZ_NAME("CopperReset", "coplist#,startline[,ccoffset]");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(EndSplit, 0, 0);
BLITZ_NAME("CopperEnd", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(InfoBlock, 0, 0);
BLITZ_NAME("CopperInfoBlock", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(CustomCommand, 0, 0);
BLITZ_NAME("CopperCommand", "copins1,copins2");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(CustomMove, 0, 0);
BLITZ_NAME("CopperMove", "register,value");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(CustomWait, 0, 0);
BLITZ_NAME("CopperWait", "x,y");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(CustomSkip, 0, 0);
BLITZ_NAME("CopperSkip", "x,y");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(getccoffset, 0, 0);
BLITZ_NAME("GetCCOffset", "- get current custom copper offset");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(CustomAGACol, 0, 0);
BLITZ_NAME("CopperAGACol", "col#,r,g,b");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(splitresethard, 0, 0);
BLITZ_NAME("CopperResetAdr", "coplist#,startline");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(splitbplcon3, 0, 0);
BLITZ_NAME("ColSplitBplcon3", "bplcon3 - low byte of bplcon 3");

BLITZ_NULLSUB_AT(finit, libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, libinit, 0, 0);

BLITZ_LIBFIN();

/* the state block (CopperInfoBlock) */
static struct cfx {
    UBYTE *coplistaddr;                 /* +0  */
    UBYTE *customaddr;                  /* +4  */
    UWORD startline;                    /* +8  */
    UWORD ccoffset;                     /* +10 */
    UWORD bplcon3LO;                    /* +12 */
    UWORD bplcon3HI;                    /* +14 */
    UWORD colreg;                       /* +16 */
    UBYTE lo_bplcon3;                   /* +18 (first byte of a word) */
    UBYTE lo_pad;
} cfx;
_Static_assert(sizeof(struct cfx) == 20, "cfx layout");

/* a coplist object of displaylib (NEWTYPE .mycop, 128 bytes): the
   pointers into the copper list in front, +28 the custom space */
struct mycop {
    ULONG size;                         /* +0: 0 = not initialised */
    UBYTE *coppos, *colors, *sprites;   /* +4 +8 +12 */
    UBYTE *bpcons, *bplanes, *dot;      /* +16 +20 +24 */
    UBYTE *customs;                     /* +28 */
    UBYTE rest[96];                     /* +32 dob, bot, ... */
};
static_assert(sizeof(struct mycop) == 128);

/* the colours of a colour split (cols_adr): the r, g, b of the first
   line, then of the last, 8 bit values as longs */
struct colsplit {
    LONG start[3], end[3];
};
static_assert(sizeof(struct colsplit) == 24);

/* _getcustom: the next free place in the custom space, NULL if there is
   none */
static UWORD *getcustom(void)
{
    if (!cfx.customaddr)
        return NULL;
    return (UWORD *)(cfx.customaddr + cfx.ccoffset);
}

/* colour bank of a colour register (0-255): bits 13-15 of BPLCON3
   (MOVEQ #0,d6:MOVE.b reg,d6:ASR.w #5,d6:ROR.w #3,d6) */
static UWORD bank(UBYTE reg)
{
    return (UWORD)((reg >> 5) << 13);
}

/* COLORxx of a register in its bank (ANDI.w #%11111:ROL.b #1:ADD.w
   #color00) */
static UWORD colourreg(UBYTE reg)
{
    return (UWORD)(COLOR00 + ((reg & 31) << 1));
}

/* the high and low nibbles of three 8 bit components (word values) as
   the 12 bit colours of the two BPLCON3 halves */
static UWORD highnibbles(UWORD r, UWORD g, UWORD b)
{
    return (UWORD)(((r & 0xf0) << 4) | (g & 0xf0) | ((b & 0xf0) >> 4));
}

static UWORD lownibbles(UWORD r, UWORD g, UWORD b)
{
    return (UWORD)(((r & 0xf) << 8) | ((g & 0xf) << 4) | (b & 0xf));
}

/* the gradient of DoColSplit/RedoColSplit: start values (start << 14)
   and steps ((end - start) * (16384 / numlines), MULS.w of the low
   words) */
struct grad {
    LONG v[3], step[3];
};

static void gradient(struct grad *g, const struct colsplit *cols, UWORD numlines)
{
    WORD f = (WORD)bl_divs_w(CALCFACTOR, (WORD)numlines);   /* DIVS d7,d3 */
    /* the six longs: c[i] = start[i], c[3 + i] = end[i] (indexed like
       this, the code stays byte-identical) */
    const LONG *c = (const LONG *)cols;
    int i;

    for (i = 0; i < 3; i++) {
        g->step[i] = bl_muls_w((WORD)(c[3 + i] - c[i]), f);
        g->v[i] = bl_muls_w((WORD)c[i], CALCFACTOR);
    }
}

/* DoColSplit cols_adr,numlines,colour_register: per line a WAIT, BPLCON3
   (high nibbles), the colour, BPLCON3 (low nibbles), the colour */
ULONG BLITZ_ROUTINE c_docolsplit(const struct colsplit *cols BL_REG(d0),
                                        UWORD numlines BL_REG(d1),
                                        UBYTE reg BL_REG(d2))
{
    struct grad g;
    UWORD *p, n, hi, lo;
    UBYTE y;

    cfx.bplcon3HI = (UWORD)(0x0c00 | cfx.lo_bplcon3 | bank(reg));
    cfx.bplcon3LO = (UWORD)(0x0e00 | cfx.lo_bplcon3 | bank(reg));
    cfx.colreg = colourreg(reg);
    p = getcustom();
    if (!p)
        return (ULONG)cols;
    y = (UBYTE)cfx.startline;
    gradient(&g, cols, numlines);
    n = (UWORD)(numlines - 1);
    do {
        WORD r = (WORD)(g.v[0] >> 14), gr = (WORD)(g.v[1] >> 14), b = (WORD)(g.v[2] >> 14);

        if (!y) {
            *p++ = 0xffe1;                  /* wait for $e0,$ff */
            *p++ = 0xfffe;
        }
        *(UBYTE *)p = y;                    /* y position */
        ((UBYTE *)p)[1] = 1;                /* x position and command */
        p[1] = 0xffee;
        p += 2;
        hi = highnibbles(r, gr, b);
        lo = lownibbles(r, gr, b);
        *p++ = BPLCON3;
        *p++ = cfx.bplcon3HI;
        *p++ = cfx.colreg;
        *p++ = hi;
        *p++ = BPLCON3;
        *p++ = cfx.bplcon3LO;
        *p++ = cfx.colreg;
        *p++ = lo;
        g.v[0] += g.step[0];
        g.v[1] += g.step[1];
        g.v[2] += g.step[2];
        y++;
        cfx.startline++;
    } while (n-- != 0);                     /* DBRA */
    cfx.ccoffset = (UWORD)((UBYTE *)p - cfx.customaddr);
    return (ULONG)g.step[0];
}

/* RedoColSplit cols_adr,numlines,ccoffset: new colours for a split made
   by DoColSplit at instruction ccoffset (skips a line 256 WAIT, writes
   only the two colour values per line) */
void BLITZ_ROUTINE c_redocolsplit(const struct colsplit *cols BL_REG(d0),
                                         UWORD numlines BL_REG(d1),
                                         ULONG ccoffset BL_REG(d2))
{
    struct grad g;
    UWORD *p, n;

    if (!getcustom())
        return;
    /* ASL.w #2,d2:ADD.l d2,a0 (BUG(orig) (#594): only the low word shifted) */
    p = (UWORD *)(cfx.customaddr
                  + ((ccoffset & 0xffff0000UL) | (UWORD)(ccoffset << 2)));
    gradient(&g, cols, numlines);
    n = (UWORD)(numlines - 1);
    do {
        WORD r = (WORD)(g.v[0] >> 14), gr = (WORD)(g.v[1] >> 14), b = (WORD)(g.v[2] >> 14);

        if (*p == 0xffe1)
            p += 2;
        p += 4;                             /* WAIT, BPLCON3 */
        p[1] = highnibbles(r, gr, b);
        p += 4;                             /* colour, BPLCON3 */
        p[1] = lownibbles(r, gr, b);
        p += 2;
        g.v[0] += g.step[0];
        g.v[1] += g.step[1];
        g.v[2] += g.step[2];
    } while (n-- != 0);
}

/* CopperReset coplist#,startline[,ccoffset]: a0 = the coplist object */
void BLITZ_ROUTINE c_splitreset(UWORD startline BL_REG(d1),
                                       UWORD ccoffset BL_REG(d2),
                                       struct mycop *coplist BL_REG(a0))
{
    cfx.coplistaddr = (UBYTE *)coplist;
    cfx.customaddr = coplist->customs;
    cfx.startline = startline;
    cfx.ccoffset = (UWORD)(ccoffset << 2);
}

void BLITZ_ROUTINE c_splitreset2(UWORD startline BL_REG(d1),
                                        struct mycop *coplist BL_REG(a0))
{
    c_splitreset(startline, 0, coplist);
}

/* CopperResetAdr address,startline */
void BLITZ_ROUTINE c_splitresethard(UBYTE *custom BL_REG(d0),
                                           UWORD startline BL_REG(d1))
{
    cfx.coplistaddr = NULL;
    cfx.customaddr = custom;
    cfx.startline = startline;
    cfx.ccoffset = 0;
}

/* GetCCOffset: instructions, -1 without coplist (BUG(orig) (#593), see above);
   ASR.w on a d0 cleared by MOVEQ: the upper word is 0 */
ULONG BLITZ_ROUTINE c_getccoffset(void)
{
    if (!cfx.coplistaddr)
        return (ULONG)-1;
    return (UWORD)((WORD)cfx.ccoffset >> 2);
}

void BLITZ_ROUTINE c_splitbplcon3(UBYTE lo BL_REG(d0))
{
    cfx.lo_bplcon3 = lo;
}

/* CopperCommand copins1,copins2 (and the end of Move/Wait/Skip) */
void BLITZ_ROUTINE c_CustomCommand(UWORD ins1 BL_REG(d0),
                                          UWORD ins2 BL_REG(d1))
{
    UWORD *p = getcustom();

    if (!p)
        return;
    p[0] = ins1;
    p[1] = ins2;
    cfx.ccoffset += 4;
}

/* CopperMove register,value (BCLR #0) */
void BLITZ_ROUTINE c_CustomMove(UWORD reg BL_REG(d0), UWORD value BL_REG(d1))
{
    c_CustomCommand((UWORD)(reg & ~1), value);
}

/* CopperWait x,y */
void BLITZ_ROUTINE c_CustomWait(UWORD x BL_REG(d0), UWORD y BL_REG(d1))
{
    UWORD w = (UWORD)((UWORD)((((WORD)x >> 1) & 0xfe) + 6) | (UWORD)(y << 8));

    c_CustomCommand((UWORD)(w | 1), 0xffee);
}

/* CopperSkip x,y */
void BLITZ_ROUTINE c_CustomSkip(UWORD x BL_REG(d0), UWORD y BL_REG(d1))
{
    c_CustomCommand((UWORD)((x & 0xfe) | (UWORD)(y << 8) | 1), 0xffef);
}

/* CopperEnd: WAIT line $2c, DMA off, BPLCON3 bank 0, COLOR00 = 0 */
void BLITZ_ROUTINE c_EndSplit(void)
{
    ULONG *p = (ULONG *)getcustom();

    if (!p)
        return;
    p[0] = 0x2c01ffee;
    p[1] = BPLSPR_DMAOFF;
    p[2] = AGAHIGHNIBBLE;
    p[3] = (ULONG)COLOR00 << 16;
    cfx.ccoffset += 16;
}

/* CopperAGACol col#,r,g,b: one 24 bit colour (4 instructions) */
ULONG BLITZ_ROUTINE c_CustomAGACol(ULONG col BL_REG(d0), UWORD r BL_REG(d1),
                                          UWORD g BL_REG(d2), UWORD b BL_REG(d3))
{
    UWORD *p = getcustom(), reg;

    if (!p)
        return 1234;
    reg = colourreg((UBYTE)col);
    *(ULONG *)p = AGAHIGHNIBBLE | bank((UBYTE)col);
    p[2] = reg;
    p[3] = highnibbles(r, g, b);
    *(ULONG *)(p + 4) = AGALOWNIBBLE | bank((UBYTE)col);
    p[6] = reg;
    p[7] = lownibbles(r, g, b);
    cfx.ccoffset += 16;
    return col;
}

/* CopperInfoBlock: address of the state block */
ULONG BLITZ_ROUTINE c_InfoBlock(void)
{
    return (ULONG)&cfx;
}

void BLITZ_ROUTINE c_libinit(void)
{
}

/* entries: keep every one of d0/d1/a0/a1 that is not the result */
BLITZ_THUNK(docolsplit, c_docolsplit, "d1/a0-a1");
BLITZ_THUNK(redocolsplit, c_redocolsplit, "d0-d1/a0-a1");
BLITZ_THUNK(splitreset2, c_splitreset2, "d0-d1/a0-a1");
BLITZ_THUNK(splitreset, c_splitreset, "d0-d1/a0-a1");
BLITZ_THUNK(EndSplit, c_EndSplit, "d0-d1/a0-a1");
BLITZ_THUNK(InfoBlock, c_InfoBlock, "d1/a0-a1");
BLITZ_THUNK(CustomCommand, c_CustomCommand, "d0-d1/a0-a1");
BLITZ_THUNK(CustomMove, c_CustomMove, "d0-d1/a0-a1");
BLITZ_THUNK(CustomWait, c_CustomWait, "d0-d1/a0-a1");
BLITZ_THUNK(CustomSkip, c_CustomSkip, "d0-d1/a0-a1");
BLITZ_THUNK(getccoffset, c_getccoffset, "d1/a0-a1");
BLITZ_THUNK(CustomAGACol, c_CustomAGACol, "d1/a0-a1");
BLITZ_THUNK(splitresethard, c_splitresethard, "d0-d1/a0-a1");
BLITZ_THUNK(splitbplcon3, c_splitbplcon3, "d0-d1/a0-a1");
/* _libinit/_libfinit: RTS (nothing to set up) */
BLITZ_THUNK(libinit, c_libinit, "d0-d1/a0-a1");
BLITZ_THUNK(libfinit, c_libinit, "d0-d1/a0-a1");
