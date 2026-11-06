/*
 * switchlib - system library 65510 ($C200-$C207): switching a Blitz
 * program between Amiga mode (OS running), QAmiga mode (OS running, the
 * program's own display) and Blitz mode (OS display off, Forbid, blitter
 * owned) - the commands BLITZ, AMIGA, QAMIGA (tokens $C200-$C202, blitz2.s
 * equates goblitz/goamiga/goqamiga) and OwnBlitter/DisownBlitter/
 * coplist handling for the display libraries ($C203-$C205).  $C206/$C207
 * (BlitzS/QAmigaS) are used by the debugger: they switch the mode without
 * touching the display.
 *
 * C translation of bblibsrc/switchlib V1.1 (14/1/95).  ref/switchlib.obj
 * differs from that source in doinit, the BINARY is the reference:
 *   - the keyboard.device base (`key`, also the init value) is found by
 *     walking Exec's device list for a node named "keyboard." (9
 *     characters compared, unrolled CMP.b in the binary) - the source
 *     reads it from the CIA-A resource / lowlevel.library instead;
 *   - the ciaa.resource base is still opened and stored, but not used.
 * Each mode switch returns the previous mode in d0.w (-1 Blitz, 0 Amiga,
 * 1 QAmiga); the upper word of d0 is the caller's (MOVE mymode,d0).
 *
 * Everything is C.  Custom chip registers are written with MOVE (inline
 * assembler helper), as in the original: GCC would write 0 with CLR,
 * which on a 68000 reads the register first (COPJMP1 is a strobe).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <graphics/gfxbase.h>
#include <graphics/monitor.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gr
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks, defined below) */
void switchlib_goblitz(void), switchlib_goamiga(void), goqamiga(void);
void doownblit(void), dodisownblit(void), setcoplist(void), goblitzS(void);
void goqamigaS(void);
static ULONG doinit(void);
void dofinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !syslibheader{#switchlib,$c200,$c208,init,1,finit,0} */
BLITZ_SYSLIBHEADER7(LIB_switchlib, 0xc200, 0xc208, init, BL_LONGRET, finit, 0);
BLITZ_NULLSUB(switchlib_goblitz, 0, 0);         /* $C200 */
BLITZ_NULLSUB(switchlib_goamiga, 0, 0);         /* $C201 */
BLITZ_NULLSUB(goqamiga, 0, 0);                  /* $C202 */
BLITZ_NULLSUB(doownblit, 0, 0);                 /* $C203 */
BLITZ_NULLSUB(dodisownblit, 0, 0);              /* $C204 */
BLITZ_NULLSUB(setcoplist, 0, 0);                /* $C205 */
BLITZ_NULLSUB(goblitzS, 0, 0);                  /* $C206 */
BLITZ_NULLSUB(goqamigaS, 0, 0);                 /* $C207 */
BLITZ_NULLSUB_AT(init, doinit, 0, 0);
BLITZ_NULLSUB_AT(finit, dofinit, 0, 0);
BLITZ_LIBFIN();

#define intgoblitz 0xc106                       /* intlib */
#define intgoamiga 0xc107

static WORD mymode;                             /* -1 Blitz, 0 Amiga, 1 QAmiga */
static UWORD agamode;                           /* $FFFF: exec V39+ */
static UWORD oldpoke;                           /* BEAMCON0 of the OS (unused) */
static UWORD monpoke;                           /* BEAMCON0 for Blitz mode */
static UWORD forbiden;                          /* non zero if a 'forbid' is in effect */
static UWORD blitowned;                         /* non zero if we've got the blitter */
static struct View *oldview;                    /* non zero if view disabled! */
static struct GfxBase *gr;
static APTR cia;                                /* ciaa.resource (unused) */
static UBYTE *key;                              /* keyboard.device */
static ULONG oldcoplist;
static UWORD oldviewS;                          /* view saved without LoadView */

/* custom chip registers: MOVE.w / MOVE.l, never CLR */
#define CUSTOM 0xdff000
static inline void custom_w(ULONG reg, UWORD v)
{
    __asm volatile("move.w %1,%0" : "=m"(*(volatile UWORD *)(CUSTOM + reg)) : "dn"(v));
}
static inline void custom_l(ULONG reg, ULONG v)
{
    __asm volatile("move.l %1,%0" : "=m"(*(volatile ULONG *)(CUSTOM + reg)) : "dn"(v));
}
#define COP1LC   0x080
#define COPJMP1  0x088
#define DMACON   0x096
#define BPLCON3  0x106
#define BEAMCON0 0x1dc
#define FMODE    0x1fc

/* doforbid - Forbid once; tell keyboard.device (private fields: unit
   flag bit 0 at $3D, $70 = 4) that the OS is not reading the keyboard */
static void doforbid(void)
{
    if (forbiden)
        return;
    forbiden = 0xff00;                  /* ST */
    Forbid();
    key[0x3d] |= 1;
    *(ULONG *)(key + 0x70) = 4;
}

static void dopermit(void)
{
    if (!forbiden)
        return;
    forbiden = 0;                       /* SF */
    Permit();
    key[0x3d] &= ~1;
    *(ULONG *)(key + 0x70) = 0;
}

/* doviewoff - switch the OS display off (LoadView(NULL), two frames),
   start our own copper list if there is one */
static void doviewoff(void)
{
    oldviewS = 0;
    if (oldview)
        return;
    oldview = gr->ActiView;
    LoadView(NULL);
    WaitTOF();
    WaitTOF();
    if (oldcoplist) {
        custom_l(COP1LC, oldcoplist);
        custom_w(COPJMP1, 0);
    }
    custom_w(FMODE, 0);                 /* simon was here (hires sprite kludge) */
    custom_w(BPLCON3, 0xc00);
    if (agamode)
        custom_w(BEAMCON0, monpoke);
}

/* doviewoffS - only remember the view (debugger: don't redraw) */
static void doviewoffS(void)
{
    oldview = gr->ActiView;
    oldviewS = 0xffff;
}

/* doviewon - back to the OS display: copper to the system list, DMA on,
   LoadView(oldview).  oldview/oldviewS are cleared in any case. */
static void doviewon(void)
{
    if (oldview && !oldviewS) {
        custom_w(DMACON, 0x01a0);
        WaitTOF();
        WaitTOF();
        custom_l(COP1LC, (ULONG)gr->copinit);
        custom_w(COPJMP1, 0);
        custom_w(DMACON, 0x81a0);
        LoadView(oldview);
    }
    oldview = NULL;
    oldviewS = 0;
}

static void ownblit(void)
{
    if (blitowned)
        return;
    blitowned = 0xff00;                 /* ST */
    OwnBlitter();
}

static void disownblit(void)
{
    if (!blitowned)
        return;
    blitowned = 0;                      /* SF */
    DisownBlitter();
}

/**
 * @brief $C203 - OwnBlitter, once (for the display libraries).
 *
 * Keeps all registers.
 */
static void BLITZ_ROUTINE doownblit_c(void) { ownblit(); }
BLITZ_THUNK(doownblit, doownblit_c, "d0-d1/a0-a1");
/**
 * @brief $C204 - DisownBlitter, if owned.
 *
 * Keeps all registers.
 */
static void BLITZ_ROUTINE dodisownblit_c(void) { disownblit(); }
BLITZ_THUNK(dodisownblit, dodisownblit_c, "d0-d1/a0-a1");

static void intjsr(UWORD tok)
{
    struct bl_regs r = { 0 };

    if (tok == intgoblitz)
        BLITZ_ALIBJSR(intgoblitz, r);
    else
        BLITZ_ALIBJSR(intgoamiga, r);
}

/* setmode - new mode, returns the previous one (d0.w) */
static UWORD setmode(WORD mode)
{
    UWORD old = mymode;

    mymode = mode;
    return old;
}

/* The mode switches keep every register but d0.w (the thunks write the
   result into the low word of the saved d0). */

/**
 * @brief $C207 (QAmigaS) - QAmiga mode for the debugger, without touching
 * the display.
 *
 * Only remembers the view, intlib goamiga, Permit, DisownBlitter.
 *
 * @return d0.w: the previous mode (-1 Blitz, 0 Amiga, 1 QAmiga); the
 *         upper word of d0 is the caller's
 */
static UWORD BLITZ_ROUTINE goqamigaS_c(void)
{
    doviewoffS();
    intjsr(intgoamiga);
    dopermit();
    disownblit();
    return setmode(1);
}
BLITZ_THUNK_POST(goqamigaS, goqamigaS_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

/**
 * @brief $C206 (BlitzS) - Blitz mode for the debugger, without touching
 * the display.
 *
 * Only remembers the view, OwnBlitter, Forbid (keyboard.device told the
 * OS is not reading the keyboard), intlib goblitz.
 *
 * @return d0.w: the previous mode (-1 Blitz, 0 Amiga, 1 QAmiga)
 */
static UWORD BLITZ_ROUTINE goblitzS_c(void)
{
    doviewoffS();
    ownblit();
    doforbid();
    intjsr(intgoblitz);
    return setmode(-1);
}
BLITZ_THUNK_POST(goblitzS, goblitzS_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

/**
 * @brief $C200 BLITZ - switch to Blitz mode.
 *
 * OS display off (LoadView(NULL), two frames; our own copper list if one
 * is set; FMODE 0, BPLCON3 $C00, on V39+ BEAMCON0), OwnBlitter, Forbid
 * (keyboard.device told the OS is not reading the keyboard), intlib
 * goblitz.
 *
 * @return d0.w: the previous mode (-1 Blitz, 0 Amiga, 1 QAmiga)
 */
static UWORD BLITZ_ROUTINE goblitz_c(void)
{
    doviewoff();
    ownblit();
    doforbid();
    intjsr(intgoblitz);
    return setmode(-1);
}
BLITZ_THUNK_POST(switchlib_goblitz, goblitz_c, "d0-d1/a0-a1",
                 "move.w d0,2(sp)");

/**
 * @brief $C201 AMIGA - switch to Amiga mode.
 *
 * intlib goamiga, Permit, DisownBlitter, back to the OS display (copper
 * to the system list, DMA on, LoadView(oldview)).
 *
 * @return d0.w: the previous mode (-1 Blitz, 0 Amiga, 1 QAmiga)
 */
static UWORD BLITZ_ROUTINE goamiga_c(void)
{
    intjsr(intgoamiga);
    dopermit();
    disownblit();
    doviewon();
    return setmode(0);
}
BLITZ_THUNK_POST(switchlib_goamiga, goamiga_c, "d0-d1/a0-a1",
                 "move.w d0,2(sp)");

/**
 * @brief $C202 QAMIGA - switch to QAmiga mode (OS running, the program's
 * own display).
 *
 * intlib goamiga, Permit, DisownBlitter, OS display off as for BLITZ.
 *
 * @return d0.w: the previous mode (-1 Blitz, 0 Amiga, 1 QAmiga)
 */
static UWORD BLITZ_ROUTINE goqamiga_c(void)
{
    intjsr(intgoamiga);
    dopermit();
    disownblit();
    doviewoff();
    return setmode(1);
}
BLITZ_THUNK_POST(goqamiga, goqamiga_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

/**
 * @brief init - open graphics, find keyboard.device.
 *
 * Also notes the exec version (V39+: BEAMCON0 in Blitz mode), BEAMCON0
 * for Blitz mode from the NTSC flag, and opens ciaa.resource (unused).
 * The device list walk has no end check (not a bug: keyboard.device is
 * always there).
 *
 * @par Result
 * d0: the keyboard.device base, the library's init value
 */
static ULONG BLITZ_ROUTINE doinit(void)
{
    static const char kbd[] = "keyboard.";
    struct Node *n;
    int i;

    forbiden = 0;
    oldview = NULL;
    blitowned = 0;
    oldcoplist = 0;
    agamode = SysBase->LibNode.lib_Version >= 39 ? 0xffff : 0;  /* SGE, EXT */
    gr = (struct GfxBase *)OldOpenLibrary((STRPTR)"graphics.library");
    /* SEQ d0:AND #$20,d0 - bit 0 of byte 207, the low byte of DisplayFlags
       (NTSC), read as a byte as in the original */
    monpoke = oldpoke = (((UBYTE *)&gr->DisplayFlags)[1] & NTSC) ? 0 : 0x20;
    if (agamode)                        /* $17C(a6) = current_monitor, $28 = BeamCon0 */
        oldpoke = gr->current_monitor->BeamCon0;
    cia = OpenResource((STRPTR)"ciaa.resource");
    n = SysBase->DeviceList.lh_Head;
    for (;;) {
        for (i = 0; i < 9; i++)
            if ((UBYTE)n->ln_Name[i] != (UBYTE)kbd[i])
                break;
        if (i == 9)
            break;
        n = n->ln_Succ;
    }
    key = (UBYTE *)n;
    return (ULONG)n;
}

/* dofinit - close graphics */
void BLITZ_ROUTINE dofinit(void)
{
    CloseLibrary((struct Library *)gr);
}

/**
 * @brief $C205 (setcoplist) - set the copper list for Blitz/QAmiga mode.
 *
 * Started when the OS display is switched off.  Keeps all registers.
 *
 * @param cop a0: the copper list (0 = none)
 */
static void BLITZ_ROUTINE setcoplist_c(ULONG cop BL_REG(a0))
{
    oldviewS = 0;
    oldcoplist = cop;
}
BLITZ_THUNK(setcoplist, setcoplist_c, "d0-d1/a0-a1");
