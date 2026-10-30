/*
 * banklib - command library 76: memory banks and plain memory - Bank,
 * InitBank, FreeBank, LoadBank, SaveBank, BankLoc, BankSize, AllocMem,
 * FreeMem.
 *
 * C translation of bblibsrc/banklib (binary checked: same as the source).
 * An object library (!libfin{_toke,...,5,4}): the Blitz object "Bank"
 * (the name of the first token, label _toke), default maximum 5, objects
 * of 2^4 = 16 bytes, zeroed at program start:
 *   +0 pos.l (memory, 0 = not in use), +4 size.l, +8 memtype.l
 * The compiler loads the object pointer from the bank number (!libs
 * {#banklib,$1380} = a3 from parameter 0, $1080 = a0).  There is no free
 * routine (_free is an empty nullsub): the banks' memory is memlib's global
 * memory and goes with the program.
 *
 * Kept from the original (BUG(orig)):
 *   - Not a bug (intended): InitBank with size 0 only frees the bank.
 *   - Not a bug (no visible effect: an unused bank has size 0 and is
 *     reallocated anyway): FreeBank frees only if pos and size are both
 *     non-zero and does not clear memtype.
 *   - LoadBank (the load rules are a description, not a bug; only the drawer
 *     case is one) asks fileiolib's Exists (ALibJsr $3E15) for the file size:
 *     0 (no file, or an empty file) = nothing happens.  The bank is
 *     (re)allocated with the file size if the memtype differs or if the
 *     file is not smaller than the bank (CMP.l/BMI: the same size is
 *     reallocated too, the sign of the difference decides); a bank larger
 *     than the file is kept and the old contents behind the file's data
 *     stay.  Then Read(file, pos, BANK size) - the bank size, not the file
 *     size.  LoadBank without memtype uses the bank's memtype if the bank
 *     is in use, otherwise 0.  A drawer (Exists = -1) with a different
 *     memtype "allocates" $FFFFFFFF bytes: memlib's localloc adds its
 *     8 byte header (ADDQ.l #8 -> 7), AllocMem(7) succeeds, pos points
 *     right behind that 7 byte block and size is -1 (the Open of the
 *     drawer then fails silently). (#72)
 *   - Not a bug (no error reporting anywhere in the library): SaveBank
 *     writes nothing for an unused bank, otherwise size bytes to a new
 *     file; errors of Open/Write are ignored.
 *   - Description, not a bug: the file handle and name are kept in two
 *     longs of the library (hand, fname) like the original's `Dc.l` variables.
 * Kept from the original, not a bug (no visible effect):
 *   - InitBank with a size stores size and memtype BEFORE allocating: a
 *     failed allocation would leave pos = 0 and BankSize = the requested
 *     size, but in the original memlib never returns then (err1, "Out of
 *     Memory!"); only the C memlib returns (K). (#71)
 */
#include "blitzlib.h"
#include <dos/dos.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dosbase
#include <proto/dos.h>
#undef __NOLIBBASE__

/* a bank object, 2^4 bytes */
struct bank {
    UBYTE *pos;                 /* +0 */
    ULONG size;                 /* +4 */
    ULONG memtype;              /* +8 */
} BL_PACKED;

/* entry points (thunks / C, defined below) */
void _bankloc(void), _banksize(void), _initbank(void), _freebank(void);
void _allocmem(void), _freemem(void);
void BLITZ_ROUTINE _bankload(const UBYTE *name BL_REG(d1),
                             struct Library *dosbase BL_REG(a6));
void BLITZ_ROUTINE _bankloadb(const UBYTE *name BL_REG(d1), ULONG memtype BL_REG(d2),
                              struct Library *dosbase BL_REG(a6));
void BLITZ_ROUTINE _banksave(const UBYTE *name BL_REG(d1),
                             struct Library *dosbase BL_REG(a6));

/* !libheader{#banklib,0,0,0,0} */
BLITZ_LIBHEADER(LIB_banklib, 0, BL_NORET, 0, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_banklib, 0x1080);
BLITZ_SUBS(_bankloc, 0, 0);
BLITZ_NAME_AT(_toke, "Bank", "(bank#)  ;returns location of bank, 0 if not in use");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_banklib, 0x1380);
BLITZ_SUBS(_initbank, 0, 0);
BLITZ_NAME("InitBank", "bank#,size,memtype ;0=fast 2=chip 65536=clrmem");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_banklib, 0x1380);
BLITZ_SUBS(_freebank, 0, 0);
BLITZ_NAME("FreeBank", "bank#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_doslib, BL_LA(6), LIB_banklib, 0x1380);
BLITZ_SUBS(_bankload, 0, 0);
BLITZ_ARGS(BL_WORD, BL_STRING, BL_LONG);
BLITZ_LIBS(LIB_doslib, BL_LA(6), LIB_banklib, 0x1380);
BLITZ_SUBS(_bankloadb, 0, 0);
BLITZ_NAME("LoadBank", "bank#,filename$[,memtype]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_doslib, BL_LA(6), LIB_banklib, 0x1380);
BLITZ_SUBS(_banksave, 0, 0);
BLITZ_NAME("SaveBank", "bank#,filename$");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_banklib, 0x1080);
BLITZ_SUBS(_bankloc, 0, 0);
BLITZ_NAME("BankLoc", "(bank#)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_banklib, 0x1080);
BLITZ_SUBS(_banksize, 0, 0);
BLITZ_NAME("BankSize", "(bank#)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_allocmem, 0, 0);
BLITZ_NAME("AllocMem", "(size,memtype) memtype: 0=fast 2=chip 65536=clrmem");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_freemem, 0, 0);
BLITZ_NAME("FreeMem", "address,size");

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, 0, 0, 0);

/* !libfin{_toke,_load,_save,_use,_free,5,4} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 4);

static BPTR hand;                                       /* hand: Dc.l 0 */
static const UBYTE *fname;                              /* fname: Dc.l 0 */

/* the bank: a3 (!libs {#banklib,$1380}) */
#define BANK ((struct bank *)blitz_string)

/* _freebank - free the memory if pos and size are set, clear both */
static void freebank(struct bank *b)
{
    struct bl_regs r;

    if (!b->pos || !b->size)
        return;
    r = (struct bl_regs){ .d0 = b->size, .a1 = (ULONG)b->pos };
    BLITZ_ALIBJSR(TOK_globfree, r);
    b->pos = NULL;
    b->size = 0;
}

static void BLITZ_ROUTINE freebank_c(void)
{
    freebank(BANK);
}
BLITZ_THUNK(_freebank, freebank_c, "a0");

/* _initbank - d1 = size, d2 = memtype */
static void initbank(struct bank *b, ULONG size, ULONG memtype)
{
    struct bl_regs r;

    freebank(b);
    if (!size)
        return;
    b->size = size;                                     /* MOVEM.l d0-d1,4(a3) */
    b->memtype = memtype;
    r = (struct bl_regs){ .d0 = size, .d1 = memtype };
    b->pos = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
}

static void BLITZ_ROUTINE initbank_c(ULONG size BL_REG(d1), ULONG memtype BL_REG(d2))
{
    initbank(BANK, size, memtype);
}
BLITZ_THUNK(_initbank, initbank_c, "a0");

/* _bankloadb - d1 = name, d2 = memtype */
void BLITZ_ROUTINE _bankloadb(const UBYTE *name BL_REG(d1), ULONG memtype BL_REG(d2),
                              struct Library *dosbase BL_REG(a6))
{
    struct bank *b = BANK;
    LONG size;

    fname = name;
    size = BLITZ_ALIBJSR_A6(TOK_exists, dosbase, (ULONG)name);
    if (!size)
        return;
    /* CMP.l 8(a3),d2:BNE yoloc:CMP.l 4(a3),d0:BMI noloc - the N flag of
       the difference (wraps like the original) */
    if (memtype != b->memtype || (LONG)((ULONG)size - b->size) >= 0)
        initbank(b, size, memtype);
    hand = Open((STRPTR)fname, MODE_OLDFILE);
    if (!hand)
        return;
    Read(hand, b->pos, b->size);
    Close(hand);
}

/* _bankload - memtype = the bank's memtype if in use, else 0 */
void BLITZ_ROUTINE _bankload(const UBYTE *name BL_REG(d1),
                             struct Library *dosbase BL_REG(a6))
{
    struct bank *b = BANK;

    _bankloadb(name, b->pos ? b->memtype : 0, dosbase);
}

/* _banksave - d1 = name; Write(pos, size) to a new file */
void BLITZ_ROUTINE _banksave(const UBYTE *name BL_REG(d1),
                             struct Library *dosbase BL_REG(a6))
{
    struct bank *b = BANK;

    if (!b->pos)
        return;
    hand = Open((STRPTR)name, MODE_NEWFILE);
    if (!hand)
        return;
    Write(hand, b->pos, b->size);
    Close(hand);
}

/* _bankloc / _banksize - a0 = the bank (!libs {#banklib,$1080}) */
static ULONG BLITZ_ROUTINE bankloc_c(const struct bank *b BL_REG(a0))
{
    return (ULONG)b->pos;
}
BLITZ_THUNK(_bankloc, bankloc_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE banksize_c(const struct bank *b BL_REG(a0))
{
    return b->size;
}
BLITZ_THUNK(_banksize, banksize_c, "d1/a0-a1");

/* _allocmem - memlib globalloc (d0 = size, d1 = memtype); keeps a0/a1
   like globalloc */
static ULONG BLITZ_ROUTINE allocmem_c(ULONG size BL_REG(d0), ULONG memtype BL_REG(d1))
{
    struct bl_regs r = { .d0 = size, .d1 = memtype };

    return BLITZ_ALIBJSR(TOK_globalloc, r);
}
BLITZ_THUNK(_allocmem, allocmem_c, "a0-a1");

/* _freemem - memlib globfree (a1 = address, d0 = size); keeps d1/a0 */
static void BLITZ_ROUTINE freemem_c(ULONG mem BL_REG(d0), ULONG size BL_REG(d1))
{
    struct bl_regs r = { .d0 = size, .a1 = mem };

    BLITZ_ALIBJSR(TOK_globfree, r);
}
BLITZ_THUNK(_freemem, freemem_c, "d1/a0");
