/*
 * blitzkeyslib - command library 98: keyboard in Blitz mode - BlitzKeys,
 * RawStatus, BlitzRepeat, BlitzQualifier, HandleRawKey, BlitzKey.
 *
 * C translation of bblibsrc/blitzkeyslib (binary checked: same as the
 * source; the binary has 6 more bytes behind the code, MOVEQ #0,d0:RTS
 * and a word $0794, referenced by nothing).
 *   init: !nullsub with !libs{#switchlib,#la0}: a0 = switchlib's init
 *         value = the keyboard.device base (switchlib walks the device
 *         list).  matoff = $136 (exec V36+) or $106: offset of the key
 *         matrix in keyboard.device.
 *   BlitzKeys On (Blitz mode only, err1 = 1): installs BlitzKey as input
 *         handler (inputoutputlib +3, TOK_setin), the old one is kept in
 *         oldin; its -4 entry (read a line) jumps to the -4 entry of the
 *         old handler.
 *   BlitzKey (also the input handler's single character entry): takes the
 *         next key down event from keyboard.device's buffer (+$70 read
 *         index, +$72 write index, entries {code.w, qualifier.w} at +$74,
 *         index + 4 & 127), skips key up codes (bit 7), $7D gives -1;
 *         the character comes from rawkeylib qconvertkey (+3) with the
 *         qualifier, which is also kept for BlitzQualifier.  -1 = no key.
 *   RawStatus(code): bit code&7 of byte code>>3 of the matrix: -1/0.
 *   BlitzRepeat: does nothing (RTS); HandleRawKey has no code.
 *
 *   BlitzKeys Off puts the old handler back (if BlitzKey is still the
 *         current one).
 *
 * Fixed (#408): BlitzKeys Off only cleared the flag; the input handler
 * stayed, and a second BlitzKeys On made oldin BlitzKey itself, so reading
 * a line jumped to itself for ever (bug-orig test blitzkeyslib-off-handler).
 * Not a bug (no observable effect): ison is toggled with NOT (On: $FFFF;
 * only tested for 0); the -8 entry of the input handler (string output) is
 * no BRA (an input handler is never called there); RawStatus,
 * BlitzQualifier: word results, the upper word of d0 is the caller's in
 * the original (here undefined, the register rule; the compiler uses only
 * d0.w); BlitzKey writes the read index of
 * keyboard.device for every entry it passes, also key up codes (needed to
 * get past them).
 * Registers: BlitzKey keeps d1/a0-a1 (MOVEM; the original's assembler,
 * like RawStatus).
 */
#include "blitzlib.h"
#include <exec/libraries.h>

/* entry points (thunks / C routines / assembler, defined below) */
void dorstat(void), doblitzqualifier(void), binkey(void);
void BLITZ_ROUTINE dobkeys(WORD on BL_REG(d0));
void BLITZ_ROUTINE doblitzrepeat(void);
static void BLITZ_ROUTINE codeinit(UBYTE *kd BL_REG(a0));

BLITZ_ENTRY_DECL(init);

/* !libheader{#blitzkeyslib,init,0,0,0} */
BLITZ_LIBHEADER(LIB_blitzkeyslib, init, BL_NORET, 0, 0);

/* err1 = 1: Blitz mode only */
#define BLITZONLY ((void *)1)

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dobkeys, BLITZONLY, 0);
BLITZ_NAME("BlitzKeys", "On|Off");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dorstat, 0, 0);
BLITZ_NAME("RawStatus", "(Rawkey)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(doblitzrepeat, 0, 0);
BLITZ_NAME("BlitzRepeat", "Delay,Speed");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(doblitzqualifier, 0, 0);
BLITZ_NAME("BlitzQualifier", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(0, 0, 0);
BLITZ_NAME("HandleRawKey", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(binkey, 0, 0);
BLITZ_NAME("BlitzKey", ";same as inkey$ after a bitmapinput");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0, LIB_switchlib, BL_LA0);

BLITZ_LIBFIN();

/* ********************* CODE BELOW ********************** */

static UBYTE *key __attribute__((used)); /* keyboard.device (also read by RawStatus) */
static UWORD ison;                      /* flag for if blitzkeys is already on */
static UWORD bqual __attribute__((used)); /* written by binkey */
UBYTE *oldin;                           /* old input handler (dobinkey2) */
static WORD matoff __attribute__((used));

/**
 * @brief BlitzQualifier: the qualifier of the last key BlitzKey read.
 *
 * MOVE bqual(pc),d0: a word function; the upper word of d0 is the
 * caller's in the original and undefined here (the register rule; the
 * compiler uses only d0.w).
 *
 * @return d0.w: the qualifier word of the key event
 */
static UWORD BLITZ_ROUTINE doblitzqualifier_c(void)
{
    return bqual;
}
BLITZ_ALIAS(doblitzqualifier, doblitzqualifier_c);

/**
 * @brief The init routine: remember keyboard.device and its key matrix.
 *
 * Clears the BlitzKeys flag and the qualifier; matoff = $136 (exec V36+)
 * or $106 (CMP #36,20(a0):BCC): the offset of the key matrix in
 * keyboard.device.
 *
 * @param kd  a0: switchlib's init value, the keyboard.device base
 *            (!libs{#switchlib,#la0}; switchlib walks the device list)
 */
static void BLITZ_ROUTINE codeinit(UBYTE *kd BL_REG(a0))
{
    key = kd;
    ison = 0;
    bqual = 0;
    matoff = 0x136;
    if ((*(struct Library **)4)->lib_Version < 36)  /* CMP #36,20(a0):BCC */
        matoff = 0x106;
}

/**
 * @brief BlitzRepeat Delay,Speed: does nothing (RTS).
 */
void BLITZ_ROUTINE doblitzrepeat(void)
{
}

/**
 * @brief BlitzKeys On|Off: BlitzKey as the input handler (Blitz mode only).
 *
 * On installs binkey as input handler (inputoutputlib +3, TOK_setin) and
 * keeps the old one in oldin; the handler's -4 entry (read a line) jumps
 * to the -4 entry of the old handler.  ison is toggled with NOT (On:
 * $FFFF, only tested for 0; not a bug).  Errors: Blitz mode only (err1 =
 * 1).
 * Off puts oldin back as the input handler (inputoutputlib +3) if BlitzKey
 * is still the current one; an input the program chose after On (e.g.
 * FileInput) stays.  Fixed (#408): the original's Off only cleared the
 * flag, the handler stayed, and a second On made oldin BlitzKey itself (a
 * line read through the -4 entry then jumped to itself for ever).
 *
 * @param on  d0.w: 0 = Off, otherwise On
 */
void BLITZ_ROUTINE dobkeys(WORD on BL_REG(d0))
{
    struct bl_regs r = { 0 };

    if (on) {
        if (!ison) {
            r.a0 = (ULONG)binkey;
            ison = ~ison;
            oldin = (UBYTE *)BLITZ_ALIBJSR(TOK_setin, r);
        }
    } else if (ison) {
        ison = ~ison;
        r.a0 = (ULONG)oldin;                    /* Fixed (#408) */
        r.a0 = BLITZ_ALIBJSR(TOK_setin, r);     /* the handler it replaced */
        if (r.a0 != (ULONG)binkey)              /* not BlitzKey: put it back */
            BLITZ_ALIBJSR(TOK_setin, r);
    }
}

/**
 * @brief BlitzKey: the next key from keyboard.device's buffer, -1 if none.
 *
 * Also the input handler's single character entry.  Takes the next key
 * down event from keyboard.device's buffer (+$70 read index, +$72 write
 * index, entries {code.w, qualifier.w} at +$74, index + 4 & 127) and
 * writes the read index for every entry it passes, also key up codes
 * (bit 7, skipped; needed to get past them); $7D gives -1.  The character
 * comes from rawkeylib qconvertkey (+3) with the qualifier, which is also
 * kept for BlitzQualifier.  Keeps d1/a0-a1 (MOVEM).
 *
 * The original's code, an assembler entry: Inkey$ calls it through the
 * input handler, and as a C function behind a thunk it cost about 90
 * cycles more for "no key" (the call of the C function, the save of
 * d2/d3).  The upper words of d0 and d1 are the caller's (word loads);
 * qconvertkey gets d0 = the code, d1 = the qualifier, a0 = the key data
 * and the caller's a1 (it does not read a1).
 *
 * The handler: -4 = BRA dobinkey2 (read a line through the old handler),
 * 0 = binkey (one character).
 *
 * Out: d0 the character, -1 = no key
 */
BLITZ_ASM_BLOCK(binkey,
        "   bra.w   _dobinkey2\n"
        "   .globl  _binkey\n"
        "_binkey:\n"
        "   movem.l d1/a0-a1,-(sp)\n"
        "   move.l  _key(pc),a0\n"
        "   move.w  0x70(a0),d1\n"
        "0: addq.w  #4,d1\n"                    /* 'retry */
        "   and.w   #127,d1\n"
        "   cmp.w   0x72(a0),d1\n"
        "   beq.s   1f\n"
        "   move.w  d1,0x70(a0)\n"
        "   move.w  0x74(a0,d1.w),d0\n"
        "   btst    #7,d0\n"                    /* key up: retry */
        "   bne.s   0b\n"
        "   cmp.w   #0x7d,d0\n"
        "   beq.s   1f\n"
        "   move.w  0x76(a0,d1.w),d1\n"
        "   move.w  d1,_bqual\n"
        "   jsr     _blitz_tok+" BL_ASMTOK(LIB_rawkeylib, 3) "\n" /* qconvertkey */
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n"
        "1: moveq   #-1,d0\n"                   /* 'nokey */
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n"
        "_dobinkey2:\n"
        "   move.l  _oldin,a6\n"
        "   jmp     -4(a6)\n");

/**
 * @brief RawStatus(Rawkey): whether a key is down.
 *
 * Bit code&7 of byte code>>3 of keyboard.device's key matrix: the
 * original's code (MOVE.l key(pc),a0 ... BTST d1,0(a0,d0.w):SNE d0:EXT
 * d0), an assembler entry, as a C function it cost 100 cycles more.  A
 * word function: the upper word of d0 is the caller's (undefined by the
 * register rule; the compiler uses only d0.w).  Changes d0, d1 and a0 and
 * keeps a1, which RILesDebugLib's SearchMem holds across it (KEEPS a1 in
 * libs/tests/systest_cmdlibs.c).
 *
 * In:  d0.w the raw key code
 * Out: d0.w -1 = down, 0 = up
 */
BLITZ_ASM_ROUTINE(dorstat,
"   move.l  _key(pc),a0\n"
"   move.w  d0,d1\n"
"   and.w   #7,d1\n"
"   lsr.w   #3,d0\n"
"   add.w   _matoff(pc),a0\n"           /* ADDA.w: matoff sign extended */
"   btst    d1,0(a0,d0.w)\n"
"   sne     d0\n"
"   ext.w   d0\n"
"   rts");
