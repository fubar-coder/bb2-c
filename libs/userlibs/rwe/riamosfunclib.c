/*
 * riamosfunclib - RWE userlib 75: "AMOS functions" - memory banks, file
 * and DOS helpers, caches, hardware registers.
 *   CachesOn/CachesOff bitmask   Reboot   Lisa   MemFree   Timer   KeyCode
 *   MakeDir(dir$)   Rename(old$,new$)   FileSize(name$)   ChangeDir dir$
 *   FindVolume(name)   DeviceName$(volumename$)
 *   GTTagValue([tag])   Min(a,b)   Max(a,b)
 *   CopyByte/CopyWord/CopyLong src,dest,n   FillMem address,length[,value]
 *   BlitterDone   WaitBlitter
 *   banks: Reserve(length)|(bank,length[,requirements])  Start(bank)
 *   Length(bank)  Erase bank  EraseAll  NextBank [start]  SwapBank(a,b)
 *   BankLimit [numbanks]  BLoad(name$[,address[,length[,offset[,memtype]]]])
 *   BSave(name$,address/bank[,length])   SaveExec name$,list()
 *
 * C translation of rwelibsrc/AMOSFunclib/func-library7.bb (1996-09-18) in
 * the state of the reference, AmiBlitz3's SVN RIAmosFuncLib.obj
 * (docs/rwelibs/README.md 3.2): the BSS2 binary, which is func-library7
 * with the UnLock in MakeDir that func-library4-6 have and 7 lost, plus
 * the changes of the AmiBlitz3 team - ChangeDir reads the old lock as a
 * long (MOVE.L, func-library7 and BSS2: MOVE.w), DeviceName$ rewritten
 * (registers kept, the task through mp_SigTask, UnLockDosList with the
 * flags of LockDosList, a terminating 0), the help text "(volumename$)".
 * The declarations equal the reference (ctest
 * rwe_declarations_riamosfunclib), the behaviour is compared with it in
 * the emulator (libs/tests/rwe/riamosfunclib.c).  Notes:
 * docs/rwelibs/port/riamosfunclib.md.
 *
 * Banks: two tables of maxbanks longs (default 50) in one memlib
 * globalloc block (MEMF_CLEAR): lenptr[bank] the length, startptr[bank]
 * the address (exec AllocMem).  A number below maxbanks (signed) is a
 * bank, anything else an address (FillMem, BLoad, BSave).  init opens
 * dos.library and allocates the tables; finit unlocks ChangeDir's lock,
 * frees every bank and the tables, closes dos.library.
 *
 * The routines are C behind thunks that keep d1/a0/a1 (statements also
 * d0) - at least what the original keeps.  The original calls its own
 * routines through ALibJsr (BLoad: FileSize, Reserve, Start, BLoad;
 * Reserve: NextBank, Reserve); here they are plain C calls.  Assembler:
 * the thunks, the two inline forms of GTTagValue (copied by the
 * compiler), and the CreateDir call of MakeDir (see makedir_c).
 *
 * BUG(orig), kept:
 *   - No range checks: a negative bank number counts as a bank and
 *     indexes before the tables (Start, Length, Erase, Reserve, BLoad,
 *     FillMem, SwapBank, NextBank); Reserve(bank,...) with bank >=
 *     maxbanks writes behind the start table (#484).  Reserve(length)
 *     checks the sign of the length, not NextBank's result: with every
 *     bank in use it reserves "bank -1" (the entry before the start
 *     table is the last length, the one before the length table memlib's
 *     header) (#498).
 *   - Reserve(length) returns 0 for a negative length - also a bank
 *     number (#500); -1 if AllocMem fails.  A bank in use is erased
 *     before the new allocation, also when that fails (#499).
 *   - NextBank without an argument starts at whatever d0 holds (#497).
 *   - EraseAll counts with DBRA on the low word, the bank number is the
 *     whole long: with more than 65536 banks only those from maxbanks-1
 *     down to the next multiple of 65536 are freed (the banks below it
 *     never), BankLimit 0 runs through 65536 negative banks (#495).
 *   - CopyByte/CopyWord/CopyLong and FillMem with a count of 0 copy or
 *     fill 2^32 units (SUBQ.l/BNE after the first) (#492).
 *   - BLoad(name$) returns the bank also when reading fails (the bank
 *     stays reserved), -1 for an empty or missing file and 0 when no
 *     bank is free (#486); the other forms leave a bank they reserved
 *     for a file that cannot be opened (#487), and treat Read's -1 as
 *     success (#488).  A bank given to BLoad is erased and reserved anew
 *     with the file's length (or the given one) (#499).  The error check
 *     (bloadchk) starts with an RTS: it never checks (#485).
 *   - BSave(name$,bank,length) writes that many bytes from the bank's
 *     start, whatever the bank's size (#489).
 *   - MakeDir unlocks whatever CreateDir leaves in d1, not the lock it
 *     returned in d0 (see makedir_c; with the dos.library of OS 3.2 d1
 *     holds the lock as well, the test sees it given back) (#496).
 *   - ChangeDir keeps no copy of the program's own current directory:
 *     finit (and BankLimit) unlock the last ChangeDir lock while it is
 *     still the current directory, without clearing oldlock (a second
 *     finit or BankLimit, or the next ChangeDir, unlocks it again)
 *     (#491).  If CurrentDir returns 0, the new lock is current but not
 *     remembered and the result is 0 (#490).
 *   - BankLimit returns the address of the new start table (init's d0)
 *     (#483).
 *   - DeviceName$ takes the name from the string space (a3), not from
 *     d0 (#493); the task is read through dol_Task without a check (a
 *     device without a handler task reads near address 0) (#494).
 *   - SaveExec: a file it cannot create returns the file name's address;
 *     an empty list (first pointer 0) leaves the file open with only the
 *     header in it (#501); the header gets one size slot (last hunk = 0),
 *     one HUNK_END behind all hunks, and each RELOC32 block names the
 *     index of its own hunk (#503); symbol hunks and other types are
 *     skipped but counted (#502).
 * Not defects: SaveExec stops at the node whose next is 0 - the list is
 * Exec-style, so that is the header's tail (lh_Tail), not an item, and
 * every item is written; init and BankLimit do not check globalloc's
 * result, but memlib raises "Out of Memory" itself.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/doshunks.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dos
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (thunks and asm, below) */
void _cacheon(void), _cacheoff(void), _reboot(void), _lisa(void), _makedir(void);
void _rename(void), _memfree(void), _gttags(void), _gttags2(void), _timer(void);
void _min(void), _max(void), _copybyte(void), _copyword(void), _copylong(void);
void _filesize(void), _bload4(void), _bload1(void), _bload0(void), _bload(void);
void _bload2(void), _bsave2(void), _bsave(void), _start(void), _length(void);
void _erase(void), _eraseall(void), _allocmem2(void), _allocmem1(void), _allocmem(void);
void _findfreebank(void), _fillmemzeros(void), _fillmem(void), _keycode(void);
void _blitdone(void), _blitwait(void), _finddev(void), _swapbank(void);
void _getdevname(void), _cd(void), _banklimit(void), _getbanklimit(void);
void _saveexec(void), _libinit(void), _libfinit(void);
static void bloadchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#riamosfunclib,init,0,finit,errors} */
BLITZ_LIBHEADER(LIB_riamosfunclib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_cacheon, 0, 0);
BLITZ_NAME("CachesOn", "bitmask");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_reboot, 0, 0);
BLITZ_NAME("Reboot", " -- performs a cold reboot");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_lisa, 0, 0);
BLITZ_NAME("Lisa", " -- returns Lisa chip version");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_makedir), 0, 0);
BLITZ_NAME("MakeDir", "(dirname$)");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_rename), 0, 0);
BLITZ_NAME("Rename", "(oldfilename$,newfilename$)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_memfree, 0, 0);
BLITZ_NAME("MemFree", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_gttags2, 0, 0);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_gttags, 0, 0);
BLITZ_NAME("GTTagValue", "([tagvalue])");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_timer, 0, 0);
BLITZ_NAME("Timer", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_min, 0, 0);
BLITZ_NAME("Min", "(a,b)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_copybyte, 0, 0);
BLITZ_NAME("CopyByte", "sourceaddr,destaddr,bytes");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_copyword, 0, 0);
BLITZ_NAME("CopyWord", "sourceaddr,destaddr,words");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_copylong, 0, 0);
BLITZ_NAME("CopyLong", "sourceaddr,destaddr,longs");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_max, 0, 0);
BLITZ_NAME("Max", "(a,b)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_filesize), 0, 0);
BLITZ_NAME("FileSize", "(filename$)");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_bload4), bloadchk, 0);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_bload1), bloadchk, 0);
BLITZ_ARGS(BL_STRING, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_bload0), bloadchk, 0);
BLITZ_ARGS(BL_STRING, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_bload), bloadchk, 0);
BLITZ_ARGS(BL_STRING, BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_bload2), bloadchk, 0);
BLITZ_NAME("BLoad", "(filename$,[address,[,length,[offset,[memtype]]]])");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_bsave2), 0, 0);
BLITZ_ARGS(BL_STRING, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_bsave), 0, 0);
BLITZ_NAME("BSave", "(filename$,address/bank[,length])");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_start, 0, 0);
BLITZ_NAME("Start", "(bank)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_length, 0, 0);
BLITZ_NAME("Length", "(bank)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_erase, 0, 0);
BLITZ_NAME("Erase", "bank");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_eraseall, 0, 0);
BLITZ_NAME("EraseAll", "");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_allocmem2, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_allocmem1, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_allocmem, 0, 0);
BLITZ_NAME("Reserve", "(length)|(bank,length[,requirements])");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_findfreebank, 0, 0);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_findfreebank, 0, 0);
BLITZ_NAME("NextBank", "[startbank]");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_cacheoff, 0, 0);
BLITZ_NAME("CachesOff", "bitmask");

BLITZ_NULLSUB(0, 0, 0);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_fillmemzeros, 0, 0);
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_fillmem, 0, 0);
BLITZ_NAME("FillMem", "address,length[,value]");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_keycode, 0, 0);
BLITZ_NAME("KeyCode", "");

BLITZ_NULLSUB(0, 0, 0);

BLITZ_NULLSUB(0, 0, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_blitdone, 0, 0);
BLITZ_NAME("BlitterDone", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_blitwait, 0, 0);
BLITZ_NAME("WaitBlitter", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_finddev), 0, 0);
BLITZ_NAME("FindVolume", "(volumename)");

BLITZ_NULLSUB(0, 0, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_swapbank, 0, 0);
BLITZ_NAME("SwapBank", "(sourcebank,destbank)");

/* AmiBlitz3: "(volumename$)" (func-library7 and BSS2: "volumename$") */
BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_getdevname), 0, 0);
BLITZ_NAME("DeviceName$", "(volumename$)");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_cd), 0, 0);
BLITZ_NAME("ChangeDir", "dir$");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_banklimit, 0, 0);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_getbanklimit, 0, 0);
BLITZ_NAME("BankLimit", "[numbanks]");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_ARRAY);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_saveexec), 0, 0);
BLITZ_NAME("SaveExec", "filename$,list()");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

#define FIBSIZE 264             /* AllocMem'd for Examine (no AllocDosObject:
                                   OS 1.3, see the source's header) */

static ULONG *lenptr, *startptr;        /* the bank tables */
static LONG maxbanks = 50;
static BPTR oldlock;                    /* ChangeDir's current directory */
static struct DosLibrary *dosbase_;     /* _dosbase */
static const char dosname[] = "dos.library";
static BPTR fh;                         /* SaveExec's file */
static ULONG hunkbuff[8];

/* the table entry of bank n: offset n*4 (ASL.l #2, a long addition) */
#define ENTRY(tab, n) (*(ULONG *)((ULONG)(tab) + ((ULONG)(n) << 2)))

/* ------------------------------------------------------------- banks */

/* Start(bank): 0 for bank >= maxbanks */
static ULONG BLITZ_ROUTINE start_c(LONG bank BL_REG(d0))
{
    if (bank >= maxbanks)
        return 0;
    return ENTRY(startptr, bank);
}

/* Length(bank) */
static ULONG BLITZ_ROUTINE length_c(LONG bank BL_REG(d0))
{
    if (bank >= maxbanks)
        return 0;
    return ENTRY(lenptr, bank);
}

/* eraseit: free the bank, clear both entries */
static void eraseit(LONG bank)
{
    APTR mem = (APTR)ENTRY(startptr, bank);
    ULONG len;

    ENTRY(startptr, bank) = 0;
    len = ENTRY(lenptr, bank);
    ENTRY(lenptr, bank) = 0;
    FreeMem(mem, len);
}

/* empty: AllocMem(len, flags) into the bank, -1 / 0 */
static ULONG newbank(LONG bank, ULONG len, ULONG flags)
{
    APTR mem = AllocMem(len, flags);

    if (!mem)
        return 0;
    ENTRY(startptr, bank) = (ULONG)mem;
    ENTRY(lenptr, bank) = len;
    return (ULONG)-1;
}

/* Erase bank */
static void BLITZ_ROUTINE erase_c(LONG bank BL_REG(d0))
{
    if (start_c(bank))
        eraseit(bank);
}

/* EraseAll: from maxbanks-1 down, DBRA on the low word (BUG(orig), #495) */
static void BLITZ_ROUTINE eraseall_c(void)
{
    ULONG n = (ULONG)maxbanks - 1;

    do {
        if (start_c((LONG)n))
            eraseit((LONG)n);
        BL_SETW(n, (UWORD)n - 1);
    } while ((UWORD)n != 0xffff);
}

/* Reserve(bank,length,requirements): the bank erased first if in use */
static ULONG BLITZ_ROUTINE allocmem_c(LONG bank BL_REG(d0), ULONG len BL_REG(d1),
                                      ULONG flags BL_REG(d2))
{
    if (start_c(bank))
        eraseit(bank);
    return newbank(bank, len, flags);
}

/* Reserve(bank,length): MEMF_PUBLIC */
static ULONG BLITZ_ROUTINE allocmem1_c(LONG bank BL_REG(d0), ULONG len BL_REG(d1))
{
    return allocmem_c(bank, len, MEMF_PUBLIC);
}

/* NextBank [start]: the first free bank from start on, -1 if none */
static ULONG BLITZ_ROUTINE findfreebank_c(LONG bank BL_REG(d0))
{
    for (; bank < maxbanks; bank++)
        if (!start_c(bank))
            return (ULONG)bank;
    return (ULONG)-1;
}

/* Reserve(length): the bank number (NextBank from 0), -1 if AllocMem
   fails, 0 for a negative length (BUG(orig): the sign of the length is
   tested, not NextBank's -1; #500 #498) */
static ULONG BLITZ_ROUTINE allocmem2_c(LONG len BL_REG(d0))
{
    ULONG bank = findfreebank_c(0);

    if (len < 0)
        return 0;
    if (!allocmem1_c((LONG)bank, (ULONG)len))
        return (ULONG)-1;
    return bank;
}

/* SwapBank(a,b): both must have a length; -1 / 0 */
static ULONG BLITZ_ROUTINE swapbank_c(LONG a BL_REG(d0), LONG b BL_REG(d1))
{
    ULONG t;

    if (!length_c(a) || !length_c(b))
        return 0;
    t = ENTRY(lenptr, a);
    ENTRY(lenptr, a) = ENTRY(lenptr, b);
    ENTRY(lenptr, b) = t;
    t = ENTRY(startptr, a);
    ENTRY(startptr, a) = ENTRY(startptr, b);
    ENTRY(startptr, b) = t;
    return (ULONG)-1;
}

/* FillMem address/bank,length[,value]: nothing for a free bank */
static void BLITZ_ROUTINE fillmem_c(ULONG adr BL_REG(d0), ULONG len BL_REG(d1),
                                    UBYTE v BL_REG(d2))
{
    UBYTE *p;

    if ((LONG)adr < maxbanks) {
        adr = start_c((LONG)adr);
        if (!adr)
            return;
    }
    p = (UBYTE *)adr;
    do                                          /* BUG(orig): 0 = 2^32 (#492) */
        *p++ = v;
    while (--len);
}

static void BLITZ_ROUTINE fillmemzeros_c(ULONG adr BL_REG(d0), ULONG len BL_REG(d1))
{
    fillmem_c(adr, len, 0);
}

/* ---------------------------------------------------------- init/finit */

static ULONG BLITZ_ROUTINE libinit_c(void)
{
    struct bl_regs r = { .d0 = (ULONG)maxbanks << 3, .d1 = MEMF_CLEAR };

    dosbase_ = (struct DosLibrary *)OpenLibrary((CONST_STRPTR)dosname, 0);
    lenptr = (ULONG *)BLITZ_ALIBJSR(TOK_globalloc, r);  /* unchecked: memlib raises Out of Memory */
    startptr = (ULONG *)((ULONG)lenptr + ((ULONG)maxbanks << 2));
    return (ULONG)startptr;
}

static void BLITZ_ROUTINE libfinit_c(void)
{
    struct DosLibrary *dos = dosbase_;
    struct bl_regs r = { 0 };

    if (oldlock)
        UnLock(oldlock);                        /* BUG(orig): still current, kept (#491) */
    eraseall_c();
    r.a1 = (ULONG)lenptr;
    r.d0 = (ULONG)maxbanks << 3;
    BLITZ_ALIBJSR(TOK_globfree, r);
    CloseLibrary(&dosbase_->dl_lib);
}

/* BankLimit numbanks: finit, the new number, init - d0 = init's */
static ULONG BLITZ_ROUTINE banklimit_c(LONG n BL_REG(d0))
{
    libfinit_c();
    maxbanks = n;
    return libinit_c();
}

static ULONG BLITZ_ROUTINE getbanklimit_c(void)
{
    return (ULONG)maxbanks;
}

/* -------------------------------------------------------------- files */

/* FileSize(name$): fib_Size of Examine (unchecked), 0 without a lock */
static ULONG BLITZ_ROUTINE filesize_c(CONST_STRPTR name BL_REG(d0))
{
    struct DosLibrary *dos = dosbase_;
    struct FileInfoBlock *fib = AllocMem(FIBSIZE, MEMF_CLEAR);
    BPTR lock;
    ULONG size = 0;

    if (!fib)
        return 0;
    lock = Lock(name, ACCESS_READ);
    if (lock) {
        Examine(lock, fib);
        UnLock(lock);
        size = (ULONG)fib->fib_Size;
    }
    FreeMem(fib, FIBSIZE);
    return size;
}

/* BLoad(name$,address/bank,length,offset,memtype): length 0 = the file's
   size; a bank is (re)reserved with that length; -1 / 0 */
static ULONG BLITZ_ROUTINE bload2_c(CONST_STRPTR name BL_REG(d0), ULONG adr BL_REG(d1),
                                    ULONG len BL_REG(d2), LONG offset BL_REG(d3),
                                    ULONG memtype BL_REG(d4))
{
    struct DosLibrary *dos = dosbase_;
    BPTR file;
    LONG got;

    if (!len) {
        len = filesize_c(name);
        if (!len)
            return 0;
    }
    if ((LONG)adr < maxbanks) {
        LONG bank = (LONG)adr;
        if (length_c(bank))
            eraseit(bank);
        if (!newbank(bank, len, memtype))
            return 0;
        adr = start_c(bank);
        if (!adr)
            return 0;
    }
    file = Open(name, MODE_OLDFILE);
    if (!file)
        return 0;                               /* BUG(orig): the bank stays (#487) */
    if (offset)
        Seek(file, offset, OFFSET_BEGINNING);
    got = Read(file, (APTR)adr, (LONG)len);     /* BUG(orig): -1 counts as read (#488) */
    Close(file);
    return got ? (ULONG)-1 : 0;
}

/* BLoad(name$,address/bank,length,offset) */
static ULONG BLITZ_ROUTINE bload_c(CONST_STRPTR name BL_REG(d0), ULONG adr BL_REG(d1),
                                   ULONG len BL_REG(d2), LONG offset BL_REG(d3))
{
    return bload2_c(name, adr, len, offset, 0);
}

/* BLoad(name$,address/bank,length) */
static ULONG BLITZ_ROUTINE bload0_c(CONST_STRPTR name BL_REG(d0), ULONG adr BL_REG(d1),
                                    ULONG len BL_REG(d2))
{
    return bload2_c(name, adr, len, 0, 0);
}

/* BLoad(name$,address/bank) */
static ULONG BLITZ_ROUTINE bload1_c(CONST_STRPTR name BL_REG(d0), ULONG adr BL_REG(d1))
{
    return bload2_c(name, adr, 0, 0, 0);
}

/* BLoad(name$): into a new bank (Reserve(length)), its number; -1 for an
   empty or missing file, 0 if Reserve gives a negative number */
static ULONG BLITZ_ROUTINE bload4_c(CONST_STRPTR name BL_REG(d0))
{
    ULONG size = filesize_c(name), bank;

    if (!size)
        return (ULONG)-1;
    bank = allocmem2_c((LONG)size);
    if ((LONG)bank < 0)
        return 0;
    bload1_c(name, start_c((LONG)bank));        /* BUG(orig): result ignored (#486) */
    return bank;
}

/* BSave(name$,address/bank,length): length 0 = the bank's length (an
   address without a length: 0); -1 / 0 */
static ULONG BLITZ_ROUTINE bsave_c(CONST_STRPTR name BL_REG(d0), ULONG adr BL_REG(d1),
                                   ULONG len BL_REG(d2))
{
    struct DosLibrary *dos = dosbase_;
    BPTR file;
    LONG put;

    if (!len) {
        if ((LONG)adr >= maxbanks)
            return 0;
        len = length_c((LONG)adr);
    }
    if ((LONG)adr < maxbanks) {
        adr = start_c((LONG)adr);
        if (!adr)
            return 0;
    }
    file = Open(name, MODE_NEWFILE);
    if (!file)
        return 0;
    put = Write(file, (APTR)adr, (LONG)len);
    Close(file);
    return put < 0 ? 0 : (ULONG)-1;
}

static ULONG BLITZ_ROUTINE bsave2_c(CONST_STRPTR name BL_REG(d0), ULONG adr BL_REG(d1))
{
    return bsave_c(name, adr, 0);
}

/* MakeDir(dir$): -1 / 0.  The original unlocks with whatever CreateDir
   leaves in d1 (JSR _CreateDir:TST.l d0:BEQ oops:JSR _UnLock -
   BUG(orig): not the lock in d0, #496).  The NDK inline of CreateDir gives no
   access to d1 after the call, so the call is inline assembler. */
static ULONG BLITZ_ROUTINE makedir_c(CONST_STRPTR name BL_REG(d0))
{
    struct DosLibrary *dos = dosbase_;
    register ULONG d0 __asm("d0");
    register ULONG d1 __asm("d1") = (ULONG)name;
    register struct DosLibrary *a6 __asm("a6") = dos;

    __asm volatile("jsr -120(a6)"
                   : "=d"(d0), "+d"(d1)
                   : "a"(a6)
                   : "a0", "a1", "cc", "memory");
    if (!d0)
        return 0;
    UnLock((BPTR)d1);
    return (ULONG)-1;
}

/* Rename(old$,new$): dos.library's result */
static ULONG BLITZ_ROUTINE rename_c(CONST_STRPTR from BL_REG(d0), CONST_STRPTR to BL_REG(d1))
{
    struct DosLibrary *dos = dosbase_;

    return (ULONG)Rename(from, to);
}

/* ChangeDir dir$: CurrentDir(Lock(dir$)), the previous ChangeDir lock
   unlocked (AmiBlitz3: MOVE.L oldlock - func-library7 and BSS2 tested
   and unlocked the word MOVE oldlock,d1 left in d1); -1 / 0 */
static ULONG BLITZ_ROUTINE cd_c(CONST_STRPTR name BL_REG(d0))
{
    struct DosLibrary *dos = dosbase_;
    BPTR lock = Lock(name, ACCESS_READ);

    if (!lock)
        return 0;
    if (!CurrentDir(lock))
        return 0;                               /* BUG(orig), see the head (#490) */
    if (oldlock)
        UnLock(oldlock);
    oldlock = lock;
    return (ULONG)-1;
}

#define LDF_FINDVOLUME (LDF_READ | LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS | LDF_ENTRY)

/* FindVolume(name): the DosList entry (devices, volumes, assigns), 0 */
static ULONG BLITZ_ROUTINE finddev_c(CONST_STRPTR name BL_REG(d0))
{
    struct DosLibrary *dos = dosbase_;
    struct DosList *dl = LockDosList(LDF_FINDVOLUME);

    if (!dl)
        return 0;
    dl = FindDosEntry(dl, name, LDF_FINDVOLUME);
    UnLockDosList(LDF_FINDVOLUME);
    return (ULONG)dl;
}

/* DeviceName$(volumename$): the name of the handler task of a device or
   volume, with ":" (AmiBlitz3's version).  A ":" at the end of the
   argument is cut; the result is written over the argument (a3 = its
   start, the length below a2); "" if the name is not found. */
static ULONG BLITZ_ROUTINE getdevname_c(const ULONG *lens BL_REG(a2))
{
    struct DosLibrary *dos = dosbase_;
    struct DosList *dl;
    const UBYTE *s;
    UBYTE *end = blitz_string - 2;      /* a3 */
    ULONG n = 0;

    if (*end++ == ':')
        end[-1] = 0;
    blitz_string = end - lens[-1];
    dl = LockDosList(LDF_READ | LDF_DEVICES | LDF_VOLUMES);
    if (!dl)
        return 0;
    dl = FindDosEntry(dl, blitz_string, LDF_DEVICES | LDF_VOLUMES);
    UnLockDosList(LDF_READ | LDF_DEVICES | LDF_VOLUMES);
    if (!dl)
        return 0;
    s = (const UBYTE *)((struct Task *)dl->dol_Task->mp_SigTask)->tc_Node.ln_Name;
    do
        n++;
    while ((*blitz_string++ = *s++) != 0);
    blitz_string[-1] = ':';
    *blitz_string = 0;
    return n;
}

/* ------------------------------------------------------------ SaveExec */

/* NEWTYPE .hunk, behind the list node (next.l, prev.l) */
struct hunkitem {
    struct hunkitem *next, *prev;
    UWORD hunktype;
    ULONG hunkloc, hunklen, reloc, reloclen, flags;
} BL_PACKED;

#define LISTMAGIC 0x6c497354                    /* "lIsT" */

/* the 32 bytes in front of a Blitz list array (linklistlib): the
   current item, the lists of the used and the free items, "lIsT" */
struct listhead {
    struct hunkitem *current;                   /* -32 */
    struct MinList used;                        /* -28 */
    struct MinList free;                        /* -16 */
    ULONG magic;                                /* -4: LISTMAGIC */
};
static_assert(sizeof(struct listhead) == 32);

/* execinit: the file and a HUNK_HEADER of 6 longs; -1 / 0 */
static ULONG execinit(CONST_STRPTR name)
{
    struct DosLibrary *dos = dosbase_;
    BPTR file = Open(name, MODE_NEWFILE);
    int i;

    if (!file)
        return 0;
    fh = file;
    hunkbuff[0] = HUNK_HEADER;
    for (i = 1; i < 6; i++)
        hunkbuff[i] = 0;
    Write(fh, hunkbuff, 24);
    return (ULONG)-1;
}

/* docode: HUNK_CODE (with the memory flags), longs, the data rounded up
   to longs, RELOC32 if there are relocations; the size in longs */
static ULONG docode(const struct hunkitem *h, ULONG num)
{
    struct DosLibrary *dos = dosbase_;
    ULONG bytes = h->hunklen, longs, n;

    if (bytes & 3)
        bytes = (bytes & ~3UL) + 4;
    longs = (ULONG)((LONG)bytes >> 2);
    hunkbuff[0] = HUNK_CODE | h->flags;
    hunkbuff[1] = longs;
    Write(fh, hunkbuff, 8);
    Write(fh, (APTR)h->hunkloc, (LONG)bytes);
    if (h->reloc) {
        n = (ULONG)((LONG)h->reloclen >> 2);
        hunkbuff[0] = HUNK_RELOC32;
        hunkbuff[1] = n;
        hunkbuff[2] = num;                      /* BUG(orig): its own index (#502 #503) */
        Write(fh, hunkbuff, 12);
        Write(fh, (APTR)h->reloc, (LONG)(n << 2));
        hunkbuff[0] = 0;
        Write(fh, hunkbuff, 4);
    }
    return longs;
}

/* execfinit: HUNK_END, the header's table size, first/last 0 and one
   size, close */
static void execfinit(ULONG nhunks, ULONG size)
{
    struct DosLibrary *dos = dosbase_;

    hunkbuff[0] = HUNK_END;
    Write(fh, hunkbuff, 4);
    Seek(fh, 8, OFFSET_BEGINNING);
    hunkbuff[0] = nhunks;
    hunkbuff[1] = 0;
    hunkbuff[2] = 0;
    hunkbuff[3] = size;
    Write(fh, hunkbuff, 16);
    Close(fh);
}

/* SaveExec name$,list(): -1; 0 if list() is no list; the name's address
   if the file cannot be made or the list is empty (BUG(orig): the file
   stays open, #501) */
static ULONG BLITZ_ROUTINE saveexec_c(CONST_STRPTR name BL_REG(d0), const UBYTE *list BL_REG(d1))
{
    const struct listhead *lh = (const struct listhead *)(list - sizeof(struct listhead));
    const struct hunkitem *h;
    ULONG num = 0, size = 0;

    if (lh->magic != LISTMAGIC)
        return 0;
    if (!execinit(name))
        return (ULONG)name;
    h = (const struct hunkitem *)lh->used.mlh_Head;
    if (!h)
        return (ULONG)name;
    do {
        if (h->hunktype == HUNK_CODE)
            size += docode(h, num);
        num++;                                  /* HUNK_SYMBOL, others: skipped */
        h = h->next;
    } while (h->next);
    execfinit(num, size);
    return (ULONG)-1;
}

/* ------------------------------------------------- exec and hardware */

/* CachesOn bitmask: CacheControl(bitmask,bitmask) - the old bits */
static ULONG BLITZ_ROUTINE cacheon_c(ULONG mask BL_REG(d0))
{
    return CacheControl(mask, mask);
}

/* CachesOff bitmask: CacheControl(0,bitmask) */
static ULONG BLITZ_ROUTINE cacheoff_c(ULONG mask BL_REG(d0))
{
    return CacheControl(0, mask);
}

static void BLITZ_ROUTINE reboot_c(void)
{
    ColdReboot();
}

/* MemFree: AvailMem(MEMF_PUBLIC) */
static ULONG BLITZ_ROUTINE memfree_c(void)
{
    return AvailMem(MEMF_PUBLIC);
}

/* Lisa: DENISEID ($DFF07C) into the low word of d0 (MOVE.w) */
static ULONG BLITZ_ROUTINE lisa_c(ULONG result BL_REG(d0))
{
    BL_SETW(result, *(volatile UWORD *)0xdff07c);
    return result;
}

/* Timer: CIA-A TOD high, mid, low (the read of high latches it) */
static ULONG BLITZ_ROUTINE timer_c(void)
{
    ULONG t = *(volatile UBYTE *)0xbfea01;

    t = t << 8 | *(volatile UBYTE *)0xbfe901;
    return t << 8 | *(volatile UBYTE *)0xbfe801;
}

/* KeyCode: CIA-A SDR, the raw keyboard byte */
static ULONG BLITZ_ROUTINE keycode_c(void)
{
    return *(volatile UBYTE *)0xbfec01;
}

/* BlitterDone: -1 if the blitter is idle (one BTST, no Agnus wait) */
static ULONG BLITZ_ROUTINE blitdone_c(void)
{
    return BL_BLITBUSY(BL_CUSTOM) ? 0 : (ULONG)-1;
}

static void BLITZ_ROUTINE blitwait_c(void)
{
    while (BL_BLITBUSY(BL_CUSTOM))
        ;
}

/* ------------------------------------------------------ small helpers */

static LONG BLITZ_ROUTINE max_c(LONG a BL_REG(d0), LONG b BL_REG(d1))
{
    return b < a ? a : b;
}

static LONG BLITZ_ROUTINE min_c(LONG a BL_REG(d0), LONG b BL_REG(d1))
{
    return b > a ? a : b;
}

/* CopyByte/CopyWord/CopyLong src,dest,n: nothing for n < 0 (d0 = 0
   then, the source otherwise, like the original); n = 0 is 2^32 units
   (BUG(orig), #492) */
static ULONG BLITZ_ROUTINE copybyte_c(const UBYTE *src BL_REG(d0), UBYTE *d BL_REG(d1),
                                      LONG n BL_REG(d2))
{
    const UBYTE *s = src;

    if (n < 0)
        return 0;
    do
        *d++ = *s++;
    while (--n);
    return (ULONG)src;
}

static ULONG BLITZ_ROUTINE copyword_c(const UWORD *src BL_REG(d0), UWORD *d BL_REG(d1),
                                      LONG n BL_REG(d2))
{
    const UWORD *s = src;

    if (n < 0)
        return 0;
    do
        *d++ = *s++;
    while (--n);
    return (ULONG)src;
}

static ULONG BLITZ_ROUTINE copylong_c(const ULONG *src BL_REG(d0), ULONG *d BL_REG(d1),
                                      LONG n BL_REG(d2))
{
    const ULONG *s = src;

    if (n < 0)
        return 0;
    do
        *d++ = *s++;
    while (--n);
    return (ULONG)src;
}

/* GTTagValue([tag]): TAG_USER|$80000 (+ tag) - inline, copied by the
   compiler */
BLITZ_INLINE(_gttags, "addi.l #0x80080000,d0");
BLITZ_INLINE(_gttags2, "move.l #0x80080000,d0");

/* the error check of BLoad: BUG(orig) (#485) the original starts with an RTS -
   its Lock test of the file name and the runtime error "Unable to load
   file!" behind it are never reached (not ported) */
static void BLITZ_ERRCHK bloadchk(void)
{
}

/* Registers: the original changes d0-d7, a0-a2 and a6 in places; the
   thunks keep d1/a0/a1 (statements d0 too), the C code d2-d7/a2-a6. */
BLITZ_THUNK(_cacheon, cacheon_c, "d1/a0-a1");
BLITZ_THUNK(_cacheoff, cacheoff_c, "d1/a0-a1");
BLITZ_THUNK(_reboot, reboot_c, "d0-d1/a0-a1");
BLITZ_THUNK(_lisa, lisa_c, "d1/a0-a1");
BLITZ_THUNK(_makedir, makedir_c, "d1/a0-a1");
BLITZ_THUNK(_rename, rename_c, "d1/a0-a1");
BLITZ_THUNK(_memfree, memfree_c, "d1/a0-a1");
BLITZ_THUNK(_timer, timer_c, "d1/a0-a1");
BLITZ_THUNK(_min, min_c, "d1/a0-a1");
BLITZ_THUNK(_max, max_c, "d1/a0-a1");
BLITZ_THUNK(_copybyte, copybyte_c, "d1/a0-a1");
BLITZ_THUNK(_copyword, copyword_c, "d1/a0-a1");
BLITZ_THUNK(_copylong, copylong_c, "d1/a0-a1");
BLITZ_THUNK(_filesize, filesize_c, "d1/a0-a1");
BLITZ_THUNK(_bload4, bload4_c, "d1/a0-a1");
BLITZ_THUNK(_bload1, bload1_c, "d1/a0-a1");
BLITZ_THUNK(_bload0, bload0_c, "d1/a0-a1");
BLITZ_THUNK(_bload, bload_c, "d1/a0-a1");
BLITZ_THUNK(_bload2, bload2_c, "d1/a0-a1");
BLITZ_THUNK(_bsave2, bsave2_c, "d1/a0-a1");
BLITZ_THUNK(_bsave, bsave_c, "d1/a0-a1");
BLITZ_THUNK(_start, start_c, "d1/a0-a1");
BLITZ_THUNK(_length, length_c, "d1/a0-a1");
BLITZ_THUNK(_erase, erase_c, "d0-d1/a0-a1");
BLITZ_THUNK(_eraseall, eraseall_c, "d0-d1/a0-a1");
BLITZ_THUNK(_allocmem2, allocmem2_c, "d1/a0-a1");
BLITZ_THUNK(_allocmem1, allocmem1_c, "d1/a0-a1");
BLITZ_THUNK(_allocmem, allocmem_c, "d1/a0-a1");
BLITZ_THUNK(_findfreebank, findfreebank_c, "d1/a0-a1");
BLITZ_THUNK(_fillmemzeros, fillmemzeros_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fillmem, fillmem_c, "d0-d1/a0-a1");
BLITZ_THUNK(_keycode, keycode_c, "d1/a0-a1");
BLITZ_THUNK(_blitdone, blitdone_c, "d1/a0-a1");
BLITZ_THUNK(_blitwait, blitwait_c, "d0-d1/a0-a1");
BLITZ_THUNK(_finddev, finddev_c, "d1/a0-a1");
BLITZ_THUNK(_swapbank, swapbank_c, "d1/a0-a1");
BLITZ_THUNK(_getdevname, getdevname_c, "d1/a0-a1");
BLITZ_THUNK(_cd, cd_c, "d1/a0-a1");
BLITZ_THUNK(_banklimit, banklimit_c, "d1/a0-a1");
BLITZ_THUNK(_getbanklimit, getbanklimit_c, "d1/a0-a1");
BLITZ_THUNK(_saveexec, saveexec_c, "d1/a0-a1");
BLITZ_THUNK(_libinit, libinit_c, "d1/a0-a1");
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");
