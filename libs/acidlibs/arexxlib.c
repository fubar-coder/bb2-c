/*
 * arexxlib - command library 92: ARexx host interface - CreateRexxMsg,
 * DeleteRexxMsg, FillRexxMsg, ClearRexxMsg, CreateArgString,
 * DeleteArgString, SendRexxCommand, ReplyRexxMsg, GetRexxResult,
 * GetRexxCommand, GetResultString, RexxEvent, IsRexxMsg, RexxError.
 *
 * C translation of bblibsrc/arexxlib (binary of the CD's acidlibs
 * checked: same as the source).
 *   init:  OldOpenLibrary("rexxsyslib.library") -> RexxSysBase; on failure
 *          a recoverable alert ("SORRY CAN NOT OPEN REXXSYSLIB.LIBRARY").
 *   finit: CloseLibrary(RexxSysBase) if it was opened.
 *
 * BB2 ArgStrings (CreateArgString): a RexxArg in memory of memlib
 * (globalloc, MEMF_PUBLIC|MEMF_CLEAR): ra_Size = length+9 with bit 31 set
 * (= "allocated by Blitz", freed with globfree; ArgStrings of ARexx have
 * it clear and go to rexxsyslib DeleteArgstring), ra_Length (word),
 * ra_Flags $46, ra_Hash = byte sum of the characters, the characters and
 * a 0.  The result points to the characters (ra_Buff).  "Is an ArgString"
 * (internal check) = the string length up to the 0 equals ra_Length.
 * RexxMsgs (CreateRexxMsg): 128 bytes of memlib, ln_Name "REXX" (so that
 * rexxsyslib IsRexxMsg accepts them), mn_Length 128, rm_FileExt/
 * rm_CommAddr = ArgStrings of the extension and host strings.
 *
 * Errors (no memory, no REXX port, no rexxsyslib) open intuition and show
 * a RECOVERABLE alert (it waits for a mouse button), then the routine
 * returns with d0 = the error code (0/4/8) - e.g. CreateRexxMsg returns 4.
 *
 * Kept from the original, not bugs:
 *   - Not a bug (the library's ArgString test): an integer in a slot of
 *     rm_Args[] is "deleted" by ClearRexxMsg if the memory before it
 *     looks like an ArgString.
 *   - Not a bug (by design): RexxEvent removes the first message like exec
 *     GetMsg (own copy with INTENA and IDNestCnt), returns replies as well
 *     (after ClearRexxMsg).
 *   - Not a bug: SendRexxCommand only creates the command ArgString if
 *     rm_Args[0] is 0 (keeps an ArgString the user put into slot 0).
 *   - Not a bug (intended): CreateArgString, CreateRexxMsg, SendRexxCommand,
 *     ReplyRexxMsg set a3 to the (first) string argument ("preserve the BB2
 *     string work area"); SendRexxCommand only for a valid RexxMsg.
 * Fixed (intended deviation, #52): GetRexxCommand/GetResultString copied
 * ra_Length+1 bytes (the 0 too) and left a3 BEHIND the 0, so a string
 * expression went on one byte too far; a3 now ends on the 0.
 * Fixed (intended deviation, #49): ClearRexxMsg (also RexxEvent on a
 * reply) and FillRexxMsg had no limit of the 16 slots rm_Args[]: with all
 * slots used ClearRexxMsg went on into rm_PassPort, rm_CommAddr,
 * rm_FileExt ... (deleted and cleared), FillRexxMsg wrote a 17th value to
 * rm_PassPort; both now stop after 16 slots.
 * Fixed (intended deviation, #53): GetResultString deleted the ArgString
 * of rm_Result2 but left the field pointing to it (a second call, or a
 * DeleteArgString of the field, worked with freed memory); it clears the
 * field now.
 * Fixed (intended deviation, #55): SendRexxCommand without a public port
 * "REXX" showed its alert and returned with interrupts disabled (exec
 * Disable without the Enable); it enables them before the alert now.
 * Fixed (intended deviation, #56): without rexxsyslib the internal
 * RexxMsg check showed its alert and then took the message as valid with
 * the address 0 (DeleteRexxMsg, ClearRexxMsg, FillRexxMsg,
 * SendRexxCommand, GetRexxResult, GetRexxCommand, GetResultString read
 * and wrote low memory, FillRexxMsg the exception vectors); after the
 * alert the message now counts as no RexxMsg.
 * Fixed (intended deviation, #51): GetRexxResult and GetRexxCommand had
 * no check of n (index (n-1)*4 as a word, MULU and d1.w; n = 0 reads
 * rm_Action / rm_Result2).  The original has no error check area; here
 * the library has one: with runtime error checking an n other than 1 or
 * 2 (GetRexxResult) or outside 1 to 16 (GetRexxCommand) is the runtime
 * error "Illegal Result Number" / "Illegal Argument Number"; release
 * code unchanged (an invalid argument).
 * Fixed (intended deviation, #48): ReplyRexxMsg created the result
 * ArgString (Result1 = 0) when `BTST #17,28(a0)` was set - a memory BTST
 * tests bit 17 mod 8 = bit 1 of the HIGH byte of rm_Action, i.e. RXFUNC
 * ($02000000), not RXFF_RESULT ($00020000): a command with RXFF_RESULT
 * got no result string, a function call without it got one.  It tests
 * RXFF_RESULT now.
 * Fixed (intended deviation, #50): FillRexxMsg (flag bit 15 = slot 0 is
 * an integer, bit 14 slot 1, ...) tested every value for 0 before the
 * flag: an integer 0 ended the list, the arguments behind it were lost.
 * Only a string value of 0 ends it now; ClearRexxMsg, which stopped at
 * the first 0 slot, clears all 16 slots.
 * Fixed (intended deviation, #57): CreateArgString (and the ArgStrings of
 * CreateRexxMsg, FillRexxMsg, SendRexxCommand) allocated length+9 bytes
 * but copied (length & $FFFF) characters (DBF) and stored the low word as
 * ra_Length: of a string of 64 KB or more only length mod 65536
 * characters arrived.  ra_Length is a word, so an ArgString holds at most
 * 65535 characters: a longer string is cut to 65535, allocated and copied
 * with that length.
 * Kept from the original, not a bug (no visible effect):
 *   - RexxError copies the message up to (not including) its 0, at least
 *     one byte (rexxsyslib has no empty messages); no terminating 0 (the
 *     compiler appends clr.b (a3)+ itself where it passes a string on,
 *     blitz2.s:8767 sbtolong); d0 = word count (texts are far below 64K);
 *     unknown code: "Returned Error not known to Arexx".
 * DIFF:
 *   - ReplyRexxMsg with something that is not a RexxMsg returns; the
 *     original pulls 15 registers it never pushed (crash).
 *   - CreateRexxMsg does not copy the host length -8(a2) to -4(a2) on the
 *     compiler's stack (the original does, to call CreateArgString again);
 *     a2 is never changed.
 * Registers: the statements and functions are the C functions
 * (BLITZ_ALIAS) and keep what the register rule needs; init and finit
 * keep their thunks.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/execbase.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#define __NOLIBBASE__
#include <proto/rexxsyslib.h>
#include <proto/intuition.h>
#undef __NOLIBBASE__

/* entry points (aliases / thunks, defined below) */
void _CreateRexxMsg(void), _DeleteRexxMsg(void), _FillRexxMsg(void);
void _ClearRexxMsg(void), _CreateArgString(void), _DeleteArgString(void);
void _SendRexxCommand(void), _ReplyRexxMsg(void), _GetRexxResult(void);
void _GetRexxCommand(void), _GetResultString(void), _RexxEvent(void);
void _IsRexxMsg(void), _RexxError(void), SetUpRoutine(void);
void arexxlib_CloseRoutine(void);
void getrexxresultchk(void), getrexxcommandchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#arexxlib,init,0,finit,0}; error field: the error check area
   of getrexxresultchk/getrexxcommandchk (fixed, #51) */
BLITZ_LIBHEADER(LIB_arexxlib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_CreateRexxMsg, 0, 0);
BLITZ_NAME("CreateRexxMsg", "MsgPtr=CreateRexxMsg(ReplyPort,Exten,Host)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_DeleteRexxMsg, 0, 0);
BLITZ_NAME("DeleteRexxMsg", "rexxmsg.l");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_FillRexxMsg, 0, 0);
BLITZ_NAME("FillRexxMsg", "rexxmsg,&FillStruct");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_ClearRexxMsg, 0, 0);
BLITZ_NAME("ClearRexxMsg", "rexxmsg");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_CreateArgString, 0, 0);
BLITZ_NAME("CreateArgString", "ArgString=CreateArgString(String)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_DeleteArgString, 0, 0);
BLITZ_NAME("DeleteArgString", "argstring");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_SendRexxCommand, 0, 0);
BLITZ_NAME("SendRexxCommand", "rexxmsg,CommandString,ActionCodes");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG, BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_ReplyRexxMsg, 0, 0);
BLITZ_NAME("ReplyRexxMsg", "rexxmsg,Result1,Result2,ResultString");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_GetRexxResult, getrexxresultchk, 0);  /* check: fixed #51 */
BLITZ_NAME("GetRexxResult", "Result=GetRexxResult(rexxmsg,ResultNum)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_GetRexxCommand, getrexxcommandchk, 0);    /* check: fixed #51 */
BLITZ_NAME("GetRexxCommand", "String=GetRexxCommand(rexxmsg,ARGNum)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_GetResultString, 0, 0);
BLITZ_NAME("GetResultString", "String=GetResultString(rexxmsg)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_RexxEvent, 0, 0);
BLITZ_NAME("RexxEvent", "rexxmsg=RexxEvent(PortAddress)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_IsRexxMsg, 0, 0);
BLITZ_NAME("IsRexxMsg", "Boolean=IsRexxMsg(rexxmsg)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_RexxError, 0, 0);
BLITZ_NAME("RexxError", "ErrorString=RexxError(ErrorCode)");

BLITZ_NULLSUB_AT(init, SetUpRoutine, 0, 0);
BLITZ_NULLSUB_AT(finit, arexxlib_CloseRoutine, 0, 0);

BLITZ_LIBFIN();

static struct RxsLib *RexxSysBase;              /* RexxSysBase: Dc.l 0 */
static struct IntuitionBase *IntuitionBase;     /* IntBase: Dc.l 0 */

static const char RexxSysName[] = "rexxsyslib.library";
static const char IntName[] = "intuition.library";
static char REXXName[] = "REXX";
static const char RENoError[] = "Returned Error not known to Arexx";

/* alert texts: x (word), y (byte), text, 0, continue flag */
#define ALERT(text) "\0\226\017" text "\0\001\0\226\031" \
    "PRESS ANY MOUSE BUTTON TO CONTINUE\0"
static const char OpenRexx[] = ALERT("SORRY CAN NOT OPEN REXXSYSLIB.LIBRARY");
static const char NoMem[] = ALERT("SORRY CAN NOT ALLOCATE THE MEMORY");
static const char NoRexxPort[] = ALERT("SORRY CAN NOT FIND THE REXXPORT");
static const char NoSig[] = ALERT("SORRY CAN NOT ALLOCATE SIGNAL TO PORT");
static const char Unknown[] = ALERT("AN UNKNOWN ERROR HAS OCCURED");
static const char *const Error_Table[] = {      /* indexed by code = 0,4,8.. */
    OpenRexx, NoMem, NoRexxPort, NoSig, Unknown
};

#define ERR_REXXSYS 0
#define ERR_NOMEM   4
#define ERR_NOPORT  8

/* the argument slots of a RexxMsg, rm_Args[0..MAXRMARG] */
#define ARGSLOTS (MAXRMARG + 1)

/* the RexxArg in front of an ArgString's characters (ra_Buff) */
#define ARGHEAD(s) ((struct RexxArg *)((UBYTE *)(s) - 8))

/* the length used by FRMCallArgString (Dc.l 0 before FRMCStorage) */
static ULONG frmcstorage;

/* Error_Routine - recoverable alert with the message `code` (a byte
   offset into Error_Table); returns the code (the original restores all
   registers, d0 included) */
static ULONG error_routine(ULONG code)
{
    IntuitionBase = (struct IntuitionBase *)OldOpenLibrary(IntName);
    DisplayAlert(RECOVERY_ALERT,
                 *(const char *const *)((const UBYTE *)Error_Table + (WORD)code),
                 35);
    CloseLibrary((struct Library *)IntuitionBase);
    return code;
}

/* TstRexxMsg - TRUE if msg is a RexxMsg (rexxsyslib IsRexxMsg).  Without
   rexxsyslib: alert, then FALSE (fixed, #56: ErrorRexxSysLib returned
   with d0 = 0 and Z set, "valid" with the message address 0). */
static int tstrexxmsg(ULONG msg)
{
    if (!RexxSysBase) {
        error_routine(ERR_REXXSYS);
        return 0;
    }
    return IsRexxMsg((struct RexxMsg *)msg) != 0;
}

/* TstArgString - TRUE if the string length up to the 0 is ra_Length */
static int tstargstring(ULONG arg)
{
    const UBYTE *s = (const UBYTE *)arg;
    ULONG len, n = 0;

    if (!arg)
        return 0;
    len = ARGHEAD(s)->ra_Length;
    while (*s++)
        n++;
    return n == len;
}

/* _CreateArgString (without the a3 part) - BB2 ArgString of `len`
   characters at s, at most 65535 (ra_Length is a word; fixed, #57: the
   original allocated len+9 bytes but copied and stored len & $FFFF);
   0 if there is no memory */
static ULONG createargstring(const UBYTE *s, ULONG len)
{
    struct bl_regs r;
    ULONG size;
    struct RexxArg *mem;
    UBYTE *d;
    UBYTE hash = 0;
    UWORD n;

    if (len > 0xFFFF)
        len = 0xFFFF;                           /* fixed (#57) */
    size = len + 9;
    r = (struct bl_regs){ .d0 = size, .d1 = MEMF_PUBLIC | MEMF_CLEAR };
    mem = (struct RexxArg *)BLITZ_ALIBJSR(TOK_globalloc, r);
    if (!mem)
        return 0;                               /* CASError */
    mem->ra_Size = size;
    *(UBYTE *)&mem->ra_Size |= 0x80;            /* BSET #31,-4(a0) */
    mem->ra_Length = (UWORD)len;
    *(UWORD *)&mem->ra_Flags = 0x4600;          /* ra_Flags $46, ra_Hash 0 */
    /* CopyString: DBF on the length's low word, hash = byte sum */
    d = (UBYTE *)mem->ra_Buff;
    for (n = (UWORD)len; n != 0; n--) {
        *d = *s++;
        hash += *d++;
    }
    mem->ra_Hash = hash;
    return (ULONG)mem->ra_Buff;
}

/* DeleteArgString_rexx - rexxsyslib DeleteArgstring */
static void deleteargstring_rexx(ULONG arg)
{
    if (!RexxSysBase) {
        error_routine(ERR_REXXSYS);
        return;
    }
    DeleteArgstring((UBYTE *)arg);
}

/* CreateArgString_rexx - rexxsyslib CreateArgstring (freed by ARexx) */
static ULONG createargstring_rexx(ULONG str, ULONG len)
{
    if (!RexxSysBase)
        return error_routine(ERR_REXXSYS);
    return (ULONG)CreateArgstring((CONST_STRPTR)str, len);
}

/* _DeleteArgString - BB2 ArgStrings to memlib, others to rexxsyslib;
   anything that does not look like an ArgString is left alone */
static void deleteargstring(ULONG arg)
{
    struct RexxArg *h;

    if (!tstargstring(arg))
        return;
    h = ARGHEAD(arg);
    if (*(UBYTE *)&h->ra_Size & 0x80) {         /* BTST #31,(a1) */
        struct bl_regs r;
        *(UBYTE *)&h->ra_Size &= 0x7f;          /* BCLR #31,(a1) */
        r = (struct bl_regs){ .d0 = h->ra_Size, .a1 = (ULONG)h };
        BLITZ_ALIBJSR(TOK_globfree, r);
    } else
        deleteargstring_rexx(arg);
}

/* _ClearRexxMsg - delete the ArgStrings of the 16 slots rm_Args[] and
   clear them (fixed, #49: the original had no limit and stopped at the
   first 0 slot; #50: an integer 0 of FillRexxMsg may stand before more
   arguments) */
static void clearrexxmsg(ULONG msg)
{
    ULONG *slot;
    int n;

    if (!tstrexxmsg(msg))
        return;
    slot = (ULONG *)((struct RexxMsg *)msg)->rm_Args;
    for (n = ARGSLOTS; n != 0; n--) {
        deleteargstring(*slot);                 /* 0: not an ArgString */
        *slot++ = 0;
    }
}

/* MULU #4 on (n-1).w, used as d1.w index */
static WORD argindex(ULONG n)
{
    return (WORD)(UWORD)((UWORD)(n - 1) * 4UL);
}

/* ------------------------------------------------------------ entries */

/**
 * @brief SetUpRoutine (init): OldOpenLibrary("rexxsyslib.library").
 *
 * On failure a recoverable alert ("SORRY CAN NOT OPEN
 * REXXSYSLIB.LIBRARY", Error_Routine1).
 *
 * @return d0: RexxSysBase, or the error code 0 after the alert
 */
static ULONG BLITZ_ROUTINE setup_c(void)
{
    RexxSysBase = (struct RxsLib *)OldOpenLibrary(RexxSysName);
    if (!RexxSysBase)
        return error_routine(ERR_REXXSYS);      /* Error_Routine1 */
    return (ULONG)RexxSysBase;
}
BLITZ_THUNK(SetUpRoutine, setup_c, "d1/a0-a1");

/**
 * @brief CloseRoutine (finit): CloseLibrary(RexxSysBase) if it was opened.
 */
static void BLITZ_ROUTINE close_c(void)
{
    if (RexxSysBase)
        CloseLibrary((struct Library *)RexxSysBase);
}
BLITZ_THUNK(arexxlib_CloseRoutine, close_c, "d0-d1/a0-a1");

/**
 * @brief CreateRexxMsg(ReplyPort,Exten,Host): a RexxMsg of 128 bytes.
 *
 * memlib globalloc (MEMF_PUBLIC|MEMF_CLEAR); ln_Name "REXX" (so that
 * rexxsyslib IsRexxMsg accepts it), mn_ReplyPort, mn_Length 128,
 * rm_FileExt/rm_CommAddr = BB2 ArgStrings of the extension and host
 * strings.  a3 = the extension afterwards (not a bug, intended: "preserve
 * the BB2 string work area").  DIFF: the host length -8(a2) is not copied
 * to -4(a2) on the compiler's stack, a2 is never changed.
 * Errors: no memory - recoverable alert ("SORRY CAN NOT ALLOCATE THE
 * MEMORY"), d0 = the error code 4.
 *
 * @param port   d0: the reply port
 * @param exten  d1: the file extension string
 * @param host   d2: the host address string
 * @param lens   a2: the compiler's string stack, -4(a2)/-8(a2) = the
 *               lengths of exten/host
 * @return d0: the RexxMsg, or the error code (4) after the alert
 */
static ULONG BLITZ_ROUTINE createrexxmsg_c(ULONG port BL_REG(d0),
                                           ULONG exten BL_REG(d1),
                                           ULONG host BL_REG(d2),
                                           const ULONG *lens BL_REG(a2))
{
    struct bl_regs r = { .d0 = 0x80, .d1 = MEMF_PUBLIC | MEMF_CLEAR };
    struct RexxMsg *m;
    ULONG a;

    blitz_string = (UBYTE *)exten;
    m = (struct RexxMsg *)BLITZ_ALIBJSR(TOK_globalloc, r);
    if (!m)
        return error_routine(ERR_NOMEM);
    m->rm_Node.mn_Node.ln_Name = REXXName;
    m->rm_Node.mn_ReplyPort = (struct MsgPort *)port;
    m->rm_Node.mn_Length = 0x80;
    a = createargstring((const UBYTE *)exten, lens[-1]);
    m->rm_FileExt = (STRPTR)a;
    if (!a)
        return error_routine(ERR_NOMEM);
    a = createargstring((const UBYTE *)host, lens[-2]);
    m->rm_CommAddr = (STRPTR)a;
    if (!a)
        return error_routine(ERR_NOMEM);
    return (ULONG)m;
}
BLITZ_ALIAS(_CreateRexxMsg, createrexxmsg_c);

/**
 * @brief DeleteRexxMsg rexxmsg: delete rm_FileExt, rm_CommAddr and the
 *        message (mn_Length bytes, memlib globfree).
 *
 * Not a RexxMsg (rexxsyslib IsRexxMsg): nothing happens.  Without
 * rexxsyslib: its alert, then nothing happens (fixed, #56: the original
 * took the message as valid with address 0).
 *
 * @param msg  d0: the RexxMsg
 */
static void BLITZ_ROUTINE deleterexxmsg_c(ULONG msg BL_REG(d0))
{
    struct RexxMsg *m;
    struct bl_regs r;

    if (!tstrexxmsg(msg))
        return;
    m = (struct RexxMsg *)msg;
    deleteargstring((ULONG)m->rm_FileExt);
    deleteargstring((ULONG)m->rm_CommAddr);
    r = (struct bl_regs){ .d0 = m->rm_Node.mn_Length, .a1 = msg };
    BLITZ_ALIBJSR(TOK_globfree, r);
}
BLITZ_ALIAS(_DeleteRexxMsg, deleterexxmsg_c);

/**
 * @brief ClearRexxMsg rexxmsg: delete the ArgStrings of rm_Args[].
 *
 * All 16 slots, each slot cleared (the guide: "from slot number 1
 * forward to 16").  Fixed (#49): the original had no limit of 16 and
 * with all slots used went on into rm_PassPort, rm_CommAddr, rm_FileExt
 * ... (deleted and cleared).  Fixed (#50): the original stopped at the
 * first 0 slot; FillRexxMsg now stores an integer 0, with more arguments
 * behind it.  An integer in a slot is "deleted" if the memory before it
 * looks like an ArgString.
 * Not a RexxMsg (rexxsyslib IsRexxMsg): nothing happens.  Without
 * rexxsyslib: its alert, then nothing happens (fixed, #56: the original
 * took the message as valid with address 0).
 *
 * @param msg  d0: the RexxMsg
 */
static void BLITZ_ROUTINE clearrexxmsg_c(ULONG msg BL_REG(d0))
{
    clearrexxmsg(msg);
}
BLITZ_ALIAS(_ClearRexxMsg, clearrexxmsg_c);

/**
 * @brief FillRexxMsg rexxmsg,&FillStruct: fill rm_Args[] from a
 *        FillStruct.
 *
 * FillStruct = {flags.w, value.l, ..., 0}: flag bit 15-n set = value n is
 * an integer, else a C string that becomes a BB2 ArgString.  A string
 * value of 0 ends the list; an integer slot takes its value, 0 too
 * (fixed, #50: the original tested every value for 0 before the flag, an
 * integer 0 ended the list).  At most the 16 slots are filled (fixed, #49: the original had no limit, a 17th value
 * went to rm_PassPort, the next ones to rm_CommAddr ...).  Errors: no
 * memory - recoverable alert, the rest is not filled.
 * Not a RexxMsg (rexxsyslib IsRexxMsg): nothing happens.  Without
 * rexxsyslib: its alert, then nothing happens (fixed, #56: the original
 * took the message as valid with address 0).
 *
 * @param msg   d0: the RexxMsg
 * @param fill  d1: the FillStruct
 */
static void BLITZ_ROUTINE fillrexxmsg_c(ULONG msg BL_REG(d0),
                                        const UBYTE *fill BL_REG(d1))
{
    const ULONG *p;
    ULONG flags, off = 0, v;
    int n;

    if (!tstrexxmsg(msg))
        return;
    flags = *(const UWORD *)fill;
    p = (const ULONG *)(fill + 2);
    for (n = ARGSLOTS; n != 0; n--) {           /* fixed (#49): 16 slots */
        off += 4;
        flags <<= 1;
        v = *p++;
        if (!(flags & 0x10000)) {
            /* a string slot; 0 ends the list (fixed, #50: the original
               tested every value for 0 before the flag, an integer 0
               ended it too) */
            const UBYTE *s = (const UBYTE *)v;
            ULONG len = 0;
            if (!v)
                return;                         /* FRMFinished */
            /* FRMCallArgString: length up to the 0 */
            while (*s++)
                len++;
            frmcstorage = len;
            v = createargstring((const UBYTE *)v, frmcstorage);
            if (!v) {
                error_routine(ERR_NOMEM);       /* FRMError */
                return;
            }
        }
        /* off = 4, 8, ... 64: rm_Args[0], [1], ... [15] */
        *(ULONG *)((UBYTE *)&((struct RexxMsg *)msg)->rm_Result2 + (WORD)off) = v;
    }
}
BLITZ_ALIAS(_FillRexxMsg, fillrexxmsg_c);

/**
 * @brief CreateArgString(String): a BB2 ArgString.
 *
 * A RexxArg in memory of memlib (globalloc, MEMF_PUBLIC|MEMF_CLEAR):
 * ra_Size = length+9 with bit 31 set (allocated by Blitz), ra_Length,
 * ra_Flags $46, ra_Hash = byte sum of the characters, the characters and
 * a 0.  At most 65535 characters (ra_Length is a word): a longer string
 * is cut to 65535 (fixed, #57: the original allocated length+9 bytes but
 * copied and stored length & $FFFF).  a3 = the string afterwards (not a
 * bug, intended).
 *
 * @param s     d0: the string
 * @param lens  a2: the compiler's string stack, -4(a2) = its length
 * @return d0: the ArgString (its characters, ra_Buff), 0 if there is no
 *         memory
 */
static ULONG BLITZ_ROUTINE createargstring_c(const UBYTE *s BL_REG(d0),
                                             const ULONG *lens BL_REG(a2))
{
    blitz_string = (UBYTE *)s;
    return createargstring(s, lens[-1]);
}
BLITZ_ALIAS(_CreateArgString, createargstring_c);

/**
 * @brief DeleteArgString argstring: free an ArgString.
 *
 * BB2 ArgStrings (bit 31 of ra_Size) go to memlib globfree, others to
 * rexxsyslib DeleteArgstring; anything that does not look like an
 * ArgString (the string length up to the 0 is not ra_Length) is left
 * alone.
 *
 * @param arg  d0: the ArgString
 */
static void BLITZ_ROUTINE deleteargstring_c(ULONG arg BL_REG(d0))
{
    deleteargstring(arg);
}
BLITZ_ALIAS(_DeleteArgString, deleteargstring_c);

/**
 * @brief SendRexxCommand rexxmsg,CommandString,ActionCodes: send the
 *        message to the public port "REXX".
 *
 * Creates the command ArgString only if rm_Args[0] is 0 (not a bug: keeps
 * an ArgString the user put into slot 0), sets rm_Action and PutMsg()s
 * the message (FindPort under Disable).  a3 = the command afterwards
 * (valid RexxMsg only; not a bug, intended).  Errors: no memory -
 * recoverable alert; no REXX port - recoverable alert ("SORRY CAN NOT
 * FIND THE REXXPORT"), after the Enable (fixed, #55: the original showed
 * it and returned with interrupts still disabled).
 * Not a RexxMsg (rexxsyslib IsRexxMsg): nothing happens.  Without
 * rexxsyslib: its alert, then nothing happens (fixed, #56: the original
 * took the message as valid with address 0).
 *
 * @param msg     d0: the RexxMsg
 * @param cmd     d1: the command string
 * @param action  d2: rm_Action
 * @param lens    a2: the compiler's string stack, -4(a2) = the command's
 *                length
 */
static void BLITZ_ROUTINE sendrexxcommand_c(ULONG msg BL_REG(d0),
                                            ULONG cmd BL_REG(d1),
                                            ULONG action BL_REG(d2),
                                            const ULONG *lens BL_REG(a2))
{
    struct RexxMsg *m;
    struct MsgPort *port;

    if (!tstrexxmsg(msg))
        return;
    blitz_string = (UBYTE *)cmd;
    m = (struct RexxMsg *)msg;
    if (!m->rm_Args[0]) {
        ULONG a = createargstring((const UBYTE *)cmd, lens[-1]);
        if (!a) {
            error_routine(ERR_NOMEM);           /* SRCError */
            return;
        }
        m->rm_Args[0] = (STRPTR)a;
    }
    m->rm_Action = action;
    Disable();
    port = FindPort(REXXName);
    if (!port) {
        Enable();                               /* fixed (#55) */
        error_routine(ERR_NOPORT);
        return;
    }
    m->rm_Node.mn_Node.ln_Type = NT_MESSAGE;
    PutMsg(port, &m->rm_Node);
    Enable();
}
BLITZ_ALIAS(_SendRexxCommand, sendrexxcommand_c);

/**
 * @brief ReplyRexxMsg rexxmsg,Result1,Result2,ResultString: reply to a
 *        message from ARexx.
 *
 * Sets rm_Result1/rm_Result2 and ReplyMsg()s the message (not for one
 * that is already a reply).  The result ArgString (rexxsyslib, freed by
 * ARexx) is made when Result1 = 0 and the sender asked for a result
 * (RXFF_RESULT in rm_Action).  Fixed (#48): the original tested `BTST
 * #17,28(a0)` - bit 1 of the HIGH byte of rm_Action, i.e. RXFUNC, not
 * RXFF_RESULT.  a3 = the result string afterwards (not a bug, intended).
 * DIFF: with something that is not a RexxMsg it returns; the original
 * pulls 15 registers it never pushed (crash).  Errors: no memory -
 * recoverable alert, no reply.
 *
 * @param msg   d0: the RexxMsg
 * @param res1  d1: rm_Result1
 * @param res2  d2: rm_Result2
 * @param str   d3: the result string
 * @param lens  a2: the compiler's string stack, -4(a2) = the result
 *              string's length
 */
static void BLITZ_ROUTINE replyrexxmsg_c(ULONG msg BL_REG(d0),
                                         ULONG res1 BL_REG(d1),
                                         ULONG res2 BL_REG(d2),
                                         ULONG str BL_REG(d3),
                                         const ULONG *lens BL_REG(a2))
{
    struct RexxMsg *m;

    blitz_string = (UBYTE *)str;
    if (!tstrexxmsg(msg))
        return;                                 /* DIFF: original crashes */
    if (!msg)
        return;
    m = (struct RexxMsg *)msg;
    if (m->rm_Node.mn_Node.ln_Type == NT_REPLYMSG)
        return;
    m->rm_Result1 = res1;
    m->rm_Result2 = res2;
    /* fixed (#48): RXFF_RESULT, bit 17 of the long (the original's
       BTST #17,28(a0) tested bit 1 of the high byte, RXFUNC) */
    if ((m->rm_Action & RXFF_RESULT) && m->rm_Result1 == 0) {
        ULONG a = createargstring_rexx(str, lens[-1]);
        if (!a) {
            error_routine(ERR_NOMEM);           /* RRMError */
            return;
        }
        m->rm_Result2 = a;
    }
    m->rm_Node.mn_Node.ln_Type = NT_REPLYMSG;
    ReplyMsg(&m->rm_Node);
}
BLITZ_ALIAS(_ReplyRexxMsg, replyrexxmsg_c);

/**
 * @brief GetRexxResult(rexxmsg,ResultNum): rm_Result1 (1) or rm_Result2
 *        (2).
 *
 * Errors (debugger only, getrexxresultchk): "Illegal Result Number" for
 * an n other than 1 or 2 (fixed, #51: the original had no check; without
 * the debugger the index (n-1)*4 is a word (MULU, d1.w), n = 0 reads
 * rm_Action, an invalid argument).  Without rexxsyslib: the alert, then 0
 * (fixed, #56: the original read the field of a message at address 0).
 *
 * @param msg  d0: the RexxMsg
 * @param n    d1: 1 or 2
 * @return d0: the result, 0 if msg is no RexxMsg
 */
static ULONG BLITZ_ROUTINE getrexxresult_c(ULONG msg BL_REG(d0),
                                           ULONG n BL_REG(d1))
{
    if (!tstrexxmsg(msg))
        return 0;                               /* GRRError */
    /* n = 0 reads rm_Action (checked by getrexxresultchk, #51) */
    return *(const ULONG *)((const UBYTE *)&((struct RexxMsg *)msg)->rm_Result1
                            + argindex(n));
}
BLITZ_ALIAS(_GetRexxResult, getrexxresult_c);

/* copy an ArgString with its 0 to a3 (DBRA on ra_Length: length+1
   bytes); a3 ends on the 0 (fixed, #52: the original left it behind the
   0); d0 = length */
static ULONG copyarg(const UBYTE *s)
{
    UWORD len = ARGHEAD(s)->ra_Length, n = len;

    do
        *blitz_string++ = *s++;
    while (n-- != 0);
    blitz_string--;                             /* fixed (#52): a3 on the 0 */
    return len;
}

/**
 * @brief GetRexxCommand(rexxmsg,ARGNum): rm_Args[ARGNum-1] as string.
 *
 * Copies the ArgString to a3 (the string work area), a3 ends on the 0.
 * Fixed (#52): the original left a3 BEHIND the 0.
 * Errors (debugger only, getrexxcommandchk): "Illegal Argument Number"
 * for an n outside 1 to 16 (fixed, #51: the original had no check;
 * without the debugger the index is a word, n = 0 reads rm_Result2, an
 * invalid argument).
 *
 * @param msg  d0: the RexxMsg
 * @param n    d1: 1..16
 * @return d0: the string length; 0 if msg is no RexxMsg or the slot is no
 *         ArgString
 */
static ULONG BLITZ_ROUTINE getrexxcommand_c(ULONG msg BL_REG(d0),
                                            ULONG n BL_REG(d1))
{
    ULONG arg;

    if (!tstrexxmsg(msg))
        return 0;                               /* GRAError */
    /* n = 0 reads rm_Result2 (checked by getrexxcommandchk, #51) */
    arg = *(const ULONG *)((const UBYTE *)((struct RexxMsg *)msg)->rm_Args
                           + argindex(n));
    if (!tstargstring(arg))
        return 0;
    return copyarg((const UBYTE *)arg);
}
BLITZ_ALIAS(_GetRexxCommand, getrexxcommand_c);

/**
 * @brief GetResultString(rexxmsg): rm_Result2 as string, then the
 *        ArgString is deleted.
 *
 * Copied to a3 as GetRexxCommand (fixed, #52: a3 ends on the 0).
 * rm_Result2 is cleared then (fixed, #53: the original left it pointing
 * to the deleted ArgString, a second call worked with freed memory).
 *
 * @param msg  d0: the RexxMsg
 * @return d0: the string length; 0 if msg is no RexxMsg or rm_Result2 is
 *         no ArgString
 */
static ULONG BLITZ_ROUTINE getresultstring_c(ULONG msg BL_REG(d0))
{
    ULONG arg, len;

    if (!tstrexxmsg(msg))
        return 0;                               /* GRSError */
    arg = ((struct RexxMsg *)msg)->rm_Result2;
    if (!tstargstring(arg))
        return 0;
    len = copyarg((const UBYTE *)arg);
    deleteargstring(arg);
    ((struct RexxMsg *)msg)->rm_Result2 = 0;    /* fixed (#53) */
    return len;
}
BLITZ_ALIAS(_GetResultString, getresultstring_c);

/**
 * @brief RexxEvent(PortAddress): the first message of the port, removed.
 *
 * Not a bug (by design): removes the message like exec GetMsg (own copy
 * with INTENA and IDNestCnt) and returns replies as well; a reply first
 * gets its arguments deleted (ClearRexxMsg).
 *
 * @param port  d0: the message port
 * @return d0: the message, 0 if there is none (or port is 0)
 */
static ULONG BLITZ_ROUTINE rexxevent_c(struct MsgPort *port BL_REG(d0))
{
    struct List *l;
    struct Node *n, *next;

    if (!port)
        return 0;
    /* _GetMsg: exec GetMsg with "some error checking" (list at 0) */
    /* &port->mp_MsgList (+$14) as an integer sum: the original tests it
       for 0, GCC would drop the test for a member address */
    l = (struct List *)((ULONG)port + 0x14);
    if (!l)
        return 0;
    BL_INTS_OFF();                              /* interrupts off */
    SysBase->IDNestCnt++;
    n = l->lh_Head;
    next = n->ln_Succ;
    if (next) {
        l->lh_Head = next;
        next->ln_Pred = (struct Node *)l;
    }
    if (--SysBase->IDNestCnt < 0)
        BL_INTS_ON();                           /* interrupts on */
    if (!next)
        return 0;
    if (n->ln_Type == NT_REPLYMSG)              /* TstReplyMsg */
        clearrexxmsg((ULONG)n);
    return (ULONG)n;
}
BLITZ_ALIAS(_RexxEvent, rexxevent_c);

/**
 * @brief IsRexxMsg(rexxmsg): rexxsyslib IsRexxMsg.
 *
 * The result is 1 or 0, not the Blitz truth values -1/0.
 *
 * @param msg  d0: the message
 * @return d0: 1 if it is a RexxMsg, else 0; without rexxsyslib the error
 *         code 0 after the alert
 */
static ULONG BLITZ_ROUTINE isrexxmsg_c(ULONG msg BL_REG(d0))
{
    if (!RexxSysBase)
        return error_routine(ERR_REXXSYS);
    return IsRexxMsg((struct RexxMsg *)msg) ? 1 : 0;
}
BLITZ_ALIAS(_IsRexxMsg, isrexxmsg_c);

/**
 * @brief RexxError(ErrorCode): the ARexx error message as string.
 *
 * rexxsyslib ErrorMsg (LVO -96: d0 = code -> d0 = found, a0 = NexxStr),
 * copied to a3.  Not a bug (no visible effect): copied up to (not
 * including) its 0, at least one byte, no terminating 0 (the compiler
 * appends it where it passes a string on, blitz2.s:8767 sbtolong); unknown
 * code: "Returned Error not known to Arexx".
 *
 * @param code  d0: the error code
 * @return d0: the string length (word count); without rexxsyslib the
 *         error code 0 after the alert
 */
static ULONG BLITZ_ROUTINE rexxerror_c(ULONG code BL_REG(d0))
{
    struct bl_regs r = { .d0 = code };
    const UBYTE *s;
    UWORD n = 0;

    if (!RexxSysBase)
        return error_routine(ERR_REXXSYS);
    BLITZ_SYSJSR(RexxSysBase, -96, r);
    s = (const UBYTE *)((struct NexxStr *)r.a0)->ns_Buff;
    if (!r.d0)
        s = (const UBYTE *)RENoError;
    do {
        *blitz_string++ = *s++;
        n++;
    } while (*s);
    return n;
}
BLITZ_ALIAS(_RexxError, rexxerror_c);

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(IllResult, "Illegal Result Number");      /* fixed, #51 */
BLITZ_ERRCHK_MESSAGE(IllArg, "Illegal Argument Number");       /* fixed, #51 */

/**
 * @brief The error check of GetRexxResult (debugger only).
 *
 * Fixed (#51), not in the original (GetRexxResult did not check the
 * number): ResultNum must be 1 (rm_Result1) or 2 (rm_Result2), as the
 * guide says.
 *
 * Errors: "Illegal Result Number".
 *
 * @param n  d1: ResultNum
 */
static void BLITZ_ERRCHK getrexxresultchk_c(ULONG n BL_REG(d1))
{
    if (n - 1 >= 2)
        BLITZ_RUNERR(IllResult);
}

/**
 * @brief The error check of GetRexxCommand (debugger only).
 *
 * Fixed (#51), not in the original (GetRexxCommand did not check the
 * number): ARGNum must be a slot of rm_Args[], 1 to 16, as the guide
 * says.
 *
 * Errors: "Illegal Argument Number".
 *
 * @param n  d1: ARGNum
 */
static void BLITZ_ERRCHK getrexxcommandchk_c(ULONG n BL_REG(d1))
{
    if (n - 1 >= ARGSLOTS)
        BLITZ_RUNERR(IllArg);
}

/* the thunks of the check functions */
BLITZ_ERRCHK_THUNK(getrexxresultchk, getrexxresultchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(getrexxcommandchk, getrexxcommandchk_c, "d0-d1/a0-a1");
