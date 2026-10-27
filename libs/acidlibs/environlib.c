/*
 * environlib - command library 193: facts about the environment, read
 * once at program start - WBWidth, WBHeight, WBDepth, WBViewMode,
 * Processor, cr_vbr, ExecVersion, WBExtViewMode.
 *
 * C translation of bblibsrc/environlib (binary checked: same as the
 * source).
 *   init (a6 = intuition base, !libs #intuitionlib,#la6):
 *     - ExecVersion = SysBase->LibNode.lib_Version
 *     - GetScreenData(scdata, 346, WBENCHSCREEN, 0): copy of the
 *       Workbench screen; width (+12), height (+14), depth of its bitmap
 *       (RastPort.BitMap at +88, Depth at +5), the long at +76 =
 *       ViewPort.Modes (WBViewMode, high word) and SpritePriority/
 *       ExtendedModes (WBExtViewMode, low word).  The result of
 *       GetScreenData is not checked.
 *     - Processor = number of shifts until AttnFlags & 15 is 0
 *       (0 = 68000, 1 = 68010, 2 = 68020, 3 = 68030, 4 = 68040).
 *     - cr_vbr = the VBR (MOVEC in supervisor state, SuperState/
 *       UserState), only for Processor >= 2 (BUG(orig): not on a 68010,
 *       which has a VBR too); 0 otherwise.
 * The functions only return the stored values.  The word functions load
 * the word into d0 with MOVE.w, the upper word of d0 stays as it was
 * (thunks with "move.w d0,2(sp)").
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks, defined below) */
void codeinit(void);
void dowbwidth(void), dowbhite(void), dowbdepth(void), dowbviewmode(void);
void doprocessor(void), docr_vbr(void), doexecversion(void), doextwbviewmode(void);

BLITZ_ENTRY_DECL(init);

/* !libheader{#environlib,init,0,0,0} */
BLITZ_LIBHEADER(LIB_environlib, init, BL_NORET, 0, 0);

#define ENV_FUNC(type, code, name)                          \
    BLITZ_AFUNCTION(type);                                  \
    BLITZ_ARGS0();                                          \
    BLITZ_LIBS0();                                          \
    BLITZ_SUBS(code, 0, 0);                                 \
    BLITZ_NAME(name, "")

ENV_FUNC(BL_LONG, dowbwidth, "WBWidth");
ENV_FUNC(BL_LONG, dowbhite, "WBHeight");
ENV_FUNC(BL_WORD, dowbdepth, "WBDepth");
ENV_FUNC(BL_WORD, dowbviewmode, "WBViewMode");
ENV_FUNC(BL_WORD, doprocessor, "Processor");
ENV_FUNC(BL_LONG, docr_vbr, "cr_vbr");
ENV_FUNC(BL_WORD, doexecversion, "ExecVersion");
ENV_FUNC(BL_WORD, doextwbviewmode, "WBExtViewMode");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0, LIB_intuitionlib, BL_LA(6));

BLITZ_LIBFIN();

#define SCDATA_SIZE 346                                 /* new screen size */
#define GetScreenData (-426)                            /* intuition */

static UBYTE scdata[SCDATA_SIZE] __attribute__((aligned(2)));
static ULONG wbwidth_, wbhite_, wbview_, vbr_;
static UWORD wbdepth_, cpu_, execver_;

/* MOVEC VBR,D2 (Dc $4e7a,$2801) - only in supervisor state; there is no
   C for a control register */
static ULONG getvbr(void)
{
    register ULONG v __asm("d2");
    __asm__ volatile(".word 0x4e7a,0x2801" : "=d"(v));   /* 68000 assembler */
    return v;
}

static void BLITZ_ROUTINE codeinit_c(const void *intuition BL_REG(a6))
{
    struct bl_regs r;
    UWORD flags;
    UWORD cpu = 0;

    execver_ = SysBase->LibNode.lib_Version;            /* MOVE 20(a0),execver_ */
    r = (struct bl_regs){ .d0 = SCDATA_SIZE, .d1 = 1 /* WBENCHSCREEN */,
                          .a0 = (ULONG)scdata, .a1 = 0 };
    BLITZ_BASEJSR(intuition, GetScreenData, r);
    wbwidth_ = (wbwidth_ & 0xffff0000) | *(UWORD *)(scdata + 12);  /* MOVE 12(a0),wbwidth_+2 */
    wbhite_ = (wbhite_ & 0xffff0000) | *(UWORD *)(scdata + 14);
    wbdepth_ = (wbdepth_ & 0xff00) | (*(UBYTE **)(scdata + 88))[5];  /* MOVE.b 5(a1),wbdepth_+1 */
    wbview_ = *(ULONG *)(scdata + 44 + 32);

    flags = SysBase->AttnFlags & 15;                    /* AND #15,d0 */
    while (flags) {                                     /* 'loop */
        cpu++;
        flags >>= 1;
    }
    cpu_ = cpu;

    {
        ULONG vbr = 0;                                  /* MOVEQ #0,d2 */
        if (cpu >= 2) {                                 /* CMP #2,d1:BCS 'skip */
            APTR ssp = SuperState();                    /* supervisor state! */
            vbr = getvbr();
            UserState(ssp);
        }
        vbr_ = vbr;
    }
}
BLITZ_THUNK(codeinit, codeinit_c, "a0-a1");

static ULONG BLITZ_ROUTINE wbwidth_c(void) { return wbwidth_; }
static ULONG BLITZ_ROUTINE wbhite_c(void) { return wbhite_; }
static ULONG BLITZ_ROUTINE cr_vbr_c(void) { return vbr_; }
static ULONG BLITZ_ROUTINE wbdepth_c(void) { return wbdepth_; }
static ULONG BLITZ_ROUTINE wbviewmode_c(void) { return wbview_ >> 16; }    /* MOVE.w wbview_ */
static ULONG BLITZ_ROUTINE extwbviewmode_c(void) { return (UWORD)wbview_; } /* MOVE.w wbview_+2 */
static ULONG BLITZ_ROUTINE processor_c(void) { return cpu_; }
static ULONG BLITZ_ROUTINE execversion_c(void) { return execver_; }

/* long functions: MOVE.l x,d0 - every other register kept */
BLITZ_THUNK(dowbwidth, wbwidth_c, "d1/a0-a1");
BLITZ_THUNK(dowbhite, wbhite_c, "d1/a0-a1");
BLITZ_THUNK(docr_vbr, cr_vbr_c, "d1/a0-a1");
/* word functions: MOVE.w x,d0 - the upper word of d0 kept */
BLITZ_THUNK_POST(dowbdepth, wbdepth_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK_POST(dowbviewmode, wbviewmode_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK_POST(doextwbviewmode, extwbviewmode_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK_POST(doprocessor, processor_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
BLITZ_THUNK_POST(doexecversion, execversion_c, "d0-d1/a0-a1", "move.w d0,2(sp)");
