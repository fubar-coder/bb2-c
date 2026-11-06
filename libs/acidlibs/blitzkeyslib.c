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
 * Kept from the original (BUG(orig)):
 *   - BlitzKeys Off only clears the flag; the input handler stays. (#408)
 * Not a bug (no observable effect): ison is toggled with NOT (On: $FFFF;
 * only tested for 0); the -8 entry of the input handler (string output) is
 * no BRA (an input handler is never called there); RawStatus,
 * BlitzQualifier: word results, the upper word of d0 is the caller's
 * (thunks; the compiler uses only d0.w); BlitzKey writes the read index of
 * keyboard.device for every entry it passes, also key up codes (needed to
 * get past them).
 * Registers: BlitzKey keeps d1/a0-a1 (MOVEM in the original, here the
 * thunk; GCC keeps d2-d7/a2-a6 around the ALibJsr).
 */
#include "blitzlib.h"
#include <exec/libraries.h>

/* entry points (thunks / C routines, defined below) */
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

BLITZ_NULLSUB_AT(init, codeinit, 0, 0, LIB_switchlib, BL_LA(0));

BLITZ_LIBFIN();

/* ********************* CODE BELOW ********************** */

static UBYTE *key;                      /* keyboard.device */
static UWORD ison;                      /* flag for if blitzkeys is already on */
static UWORD bqual;
UBYTE *oldin;                           /* old input handler (dobinkey2) */
static WORD matoff;

/* rawkeylib qconvertkey: d0 = raw code, d1 = qualifier -> d0 = character */
#define TOK_qconvertkey ((LIB_rawkeylib << 7) + 3)

/**
 * @brief BlitzQualifier: the qualifier of the last key BlitzKey read.
 *
 * MOVE bqual(pc),d0: a word function, the upper word of d0 is the
 * caller's (thunk; the compiler uses only d0.w).
 *
 * @return d0.w: the qualifier word of the key event
 */
static UWORD BLITZ_ROUTINE doblitzqualifier_c(void)
{
    return bqual;
}
BLITZ_THUNK_POST(doblitzqualifier, doblitzqualifier_c, "d0-d1/a0-a1", "move.w d0,2(sp)");

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
 * BUG(orig) (#408): Off only clears the flag; the input handler stays.
 *
 * @param on  d0.w: 0 = Off, otherwise On
 */
void BLITZ_ROUTINE dobkeys(WORD on BL_REG(d0))
{
    if (on) {
        if (!ison) {
            struct bl_regs r = { .a0 = (ULONG)binkey };

            ison = ~ison;
            oldin = (UBYTE *)BLITZ_ALIBJSR(TOK_setin, r);
        }
    } else if (ison)
        ison = ~ison;                   /* BUG(orig) (#408): handler stays */
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
 * kept for BlitzQualifier.  Keeps d1/a0-a1 (MOVEM in the original, here
 * the thunk; GCC keeps d2-d7/a2-a6 around the ALibJsr).
 *
 * @param code  d0: the caller's register; its low word gets the raw code
 *              (the upper word survives the word loads of the original)
 * @param callerd1  d1: the caller's register; first the buffer position,
 *                  then its low word gets the qualifier
 * @return d0: the character, -1 = no key
 */
static ULONG BLITZ_ROUTINE binkey_c(ULONG code BL_REG(d0), ULONG callerd1 BL_REG(d1))
{
    UBYTE *keydata = key;
    struct bl_regs r;
    UWORD pos;                          /* d1.w: the buffer position */
    ULONG qual = callerd1;              /* d1: the qualifier */

    pos = *(UWORD *)(keydata + 0x70);
    for (;;) {
        pos = (pos + 4) & 127;
        if (pos == *(UWORD *)(keydata + 0x72))
            return (ULONG)-1;           /* nokey */
        *(UWORD *)(keydata + 0x70) = pos;
        BL_SETW(code, *(UWORD *)(keydata + 0x74 + (WORD)pos));
        if (code & 0x80)
            continue;                   /* key up: retry */
        if ((UWORD)code == 0x7d)
            return (ULONG)-1;
        break;
    }
    BL_SETW(qual, *(UWORD *)(keydata + 0x76 + (WORD)pos));
    bqual = (UWORD)qual;
    r.d0 = code;
    r.d1 = qual;
    r.a0 = (ULONG)keydata;
    r.a1 = 0;
    return BLITZ_ALIBJSR(TOK_qconvertkey, r);
}

/* the handler: -4 = BRA dobinkey2 (read a line through the old handler),
   0 = binkey (one character, d1/a0-a1 kept) */
__asm__("   .text\n   .even\n"
        "   bra.w   _dobinkey2\n"
        "   .globl  _binkey\n"
        "_binkey:\n"
        "   movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _binkey_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n"
        "_dobinkey2:\n"
        "   move.l  _oldin,a6\n"
        "   jmp     -4(a6)\n");

/**
 * @brief RawStatus(Rawkey): whether a key is down.
 *
 * Bit code&7 of byte code>>3 of keyboard.device's key matrix
 * (BTST d1,0(a0,d0.w):SNE d0:EXT d0).  A word function: the upper word of
 * d0 is the caller's (thunk; the compiler uses only d0.w).
 *
 * @param code  d0.w: the raw key code
 * @return d0.w: -1 = down, 0 = up
 */
static UWORD BLITZ_ROUTINE dorstat_c(UWORD code BL_REG(d0))
{
    UBYTE *matrix = key + matoff;       /* ADD matoff(pc),a0 (ADDA.w) */

    return (matrix[(WORD)(code >> 3)] & (1 << (code & 7))) ? 0xffff : 0;
}
BLITZ_THUNK_POST(dorstat, dorstat_c, "d0/a1", "move.w d0,2(sp)");
