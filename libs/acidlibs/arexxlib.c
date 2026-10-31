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
 * Kept from the original (BUG(orig)):
 *   - ReplyRexxMsg creates the result ArgString (rexxsyslib, freed by
 *     ARexx) when Result1 = 0 and `BTST #17,28(a0)` - a memory BTST tests
 *     bit 17 mod 8 = bit 1 of the HIGH byte of rm_Action, i.e. RXFUNC
 *     ($02000000), not RXFF_RESULT ($00020000). (#48)
 *   - ClearRexxMsg (also RexxEvent on a reply) deletes rm_Args[] up to the
 *     first 0 without a limit of 16 (then rm_PassPort, rm_CommAddr ...);
 *     FillRexxMsg has no limit either.  An integer in a slot is "deleted"
 *     if the memory before it looks like an ArgString. (#49)
 *   - FillRexxMsg: a value of 0 ends the list (also an integer 0); flag
 *     bit 15 = slot 0 is an integer, bit 14 slot 1, ... (#50)
 *   - GetRexxResult/GetRexxCommand: index (n-1)*4 as word (MULU, d1.w),
 *     n = 0 reads rm_Action / rm_Result2. (#51)
 *   - GetRexxCommand/GetResultString copy ra_Length+1 bytes (the 0 too):
 *     a3 is left BEHIND the 0 (#52).  GetResultString deletes the ArgString
 *     but leaves rm_Result2 pointing to it (#53).
 *   - Not a bug (by design): RexxEvent removes the first message like exec
 *     GetMsg (own copy with INTENA and IDNestCnt), returns replies as well
 *     (after ClearRexxMsg).
 *   - SendRexxCommand: if there is no REXX port, the alert comes with
 *     interrupts still disabled (Disable without Enable) (#55).  Only
 *     creates the command ArgString if rm_Args[0] is 0 (not a bug: keeps
 *     an ArgString the user put into slot 0).
 *   - Not a bug (intended): CreateArgString, CreateRexxMsg, SendRexxCommand,
 *     ReplyRexxMsg set a3 to the (first) string argument ("preserve the BB2
 *     string work area"); SendRexxCommand only for a valid RexxMsg.
 *   - CreateArgString copies (length & $FFFF) characters (DBF) but
 *     allocates length+9 bytes. (#57)
 *   - Without rexxsyslib the internal RexxMsg check shows the alert and
 *     then treats the message as valid with address 0. (#56)
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
 * Registers: the thunks keep d1/a0/a1 for functions and d0/d1/a0/a1 for
 * statements, at least what the original keeps.
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

/* entry points (thunks, defined below) */
void _CreateRexxMsg(void), _DeleteRexxMsg(void), _FillRexxMsg(void);
void _ClearRexxMsg(void), _CreateArgString(void), _DeleteArgString(void);
void _SendRexxCommand(void), _ReplyRexxMsg(void), _GetRexxResult(void);
void _GetRexxCommand(void), _GetResultString(void), _RexxEvent(void);
void _IsRexxMsg(void), _RexxError(void), SetUpRoutine(void);
void CloseRoutine(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#arexxlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_arexxlib, init, BL_NORET, finit, 0);

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
BLITZ_SUBS(_GetRexxResult, 0, 0);
BLITZ_NAME("GetRexxResult", "Result=GetRexxResult(rexxmsg,ResultNum)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_GetRexxCommand, 0, 0);
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
BLITZ_NULLSUB_AT(finit, CloseRoutine, 0, 0);

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

/* TstRexxMsg - TRUE if *msg is a RexxMsg (rexxsyslib IsRexxMsg).  Without
   rexxsyslib: alert, then "valid" with *msg = 0 (ErrorRexxSysLib returns
   with d0 = 0 and Z set). */
static int tstrexxmsg(ULONG *msg)
{
    if (!RexxSysBase) {
        *msg = error_routine(ERR_REXXSYS);
        return 1;
    }
    return IsRexxMsg((struct RexxMsg *)*msg) != 0;
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
   characters at s; 0 if there is no memory */
static ULONG createargstring(const UBYTE *s, ULONG len)
{
    struct bl_regs r = { .d0 = len + 9, .d1 = MEMF_PUBLIC | MEMF_CLEAR };
    ULONG size = len + 9;
    struct RexxArg *mem;
    UBYTE *d;
    UBYTE hash = 0;
    UWORD n;

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

/* _ClearRexxMsg - delete rm_Args[] up to the first 0 (no limit) */
static void clearrexxmsg(ULONG msg)
{
    ULONG *slot;

    if (!tstrexxmsg(&msg))
        return;
    slot = (ULONG *)((struct RexxMsg *)msg)->rm_Args;
    while (*slot) {
        deleteargstring(*slot);
        *slot++ = 0;
    }
}

/* MULU #4 on (n-1).w, used as d1.w index */
static WORD argindex(ULONG n)
{
    return (WORD)(UWORD)((UWORD)(n - 1) * 4UL);
}

/* ------------------------------------------------------------ entries */

/* SetUpRoutine (init) */
static ULONG BLITZ_ROUTINE setup_c(void)
{
    RexxSysBase = (struct RxsLib *)OldOpenLibrary(RexxSysName);
    if (!RexxSysBase)
        return error_routine(ERR_REXXSYS);      /* Error_Routine1 */
    return (ULONG)RexxSysBase;
}
BLITZ_THUNK(SetUpRoutine, setup_c, "d1/a0-a1");

/* CloseRoutine (finit) */
static void BLITZ_ROUTINE close_c(void)
{
    if (RexxSysBase)
        CloseLibrary((struct Library *)RexxSysBase);
}
BLITZ_THUNK(CloseRoutine, close_c, "d0-d1/a0-a1");

/* _CreateRexxMsg - d0 = reply port, d1 = extension, d2 = host;
   -4(a2)/-8(a2) = their lengths; a3 = extension afterwards */
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
BLITZ_THUNK(_CreateRexxMsg, createrexxmsg_c, "d1/a0-a1");

/* _DeleteRexxMsg - both ArgStrings and the message (mn_Length bytes) */
static void BLITZ_ROUTINE deleterexxmsg_c(ULONG msg BL_REG(d0))
{
    struct RexxMsg *m;
    struct bl_regs r;

    if (!tstrexxmsg(&msg))
        return;
    m = (struct RexxMsg *)msg;
    deleteargstring((ULONG)m->rm_FileExt);
    deleteargstring((ULONG)m->rm_CommAddr);
    r = (struct bl_regs){ .d0 = m->rm_Node.mn_Length, .a1 = msg };
    BLITZ_ALIBJSR(TOK_globfree, r);
}
BLITZ_THUNK(_DeleteRexxMsg, deleterexxmsg_c, "d0-d1/a0-a1");

/* _ClearRexxMsg */
static void BLITZ_ROUTINE clearrexxmsg_c(ULONG msg BL_REG(d0))
{
    clearrexxmsg(msg);
}
BLITZ_THUNK(_ClearRexxMsg, clearrexxmsg_c, "d0-d1/a0-a1");

/* _FillRexxMsg - d1 = {flags.w, value.l, ..., 0}: flag bit 15-n set =
   value n is an integer, else a C string that becomes an ArgString */
static void BLITZ_ROUTINE fillrexxmsg_c(ULONG msg BL_REG(d0),
                                        const UBYTE *fill BL_REG(d1))
{
    const ULONG *p;
    ULONG flags, off = 0, v;

    if (!tstrexxmsg(&msg))
        return;
    flags = *(const UWORD *)fill;
    p = (const ULONG *)(fill + 2);
    for (;;) {
        off += 4;
        flags <<= 1;
        v = *p++;
        if (!v)
            return;                             /* FRMFinished */
        if (!(flags & 0x10000)) {
            /* FRMCallArgString: length up to the 0 */
            const UBYTE *s = (const UBYTE *)v;
            ULONG n = 0;
            while (*s++)
                n++;
            frmcstorage = n;
            v = createargstring((const UBYTE *)v, frmcstorage);
            if (!v) {
                error_routine(ERR_NOMEM);       /* FRMError */
                return;
            }
        }
        /* off = 4, 8, ...: rm_Args[0], [1], ... (no limit, #49) */
        *(ULONG *)((UBYTE *)&((struct RexxMsg *)msg)->rm_Result2 + (WORD)off) = v;
    }
}
BLITZ_THUNK(_FillRexxMsg, fillrexxmsg_c, "d0-d1/a0-a1");

/* _CreateArgString - d0 = string, -4(a2) = its length; a3 = d0 */
static ULONG BLITZ_ROUTINE createargstring_c(const UBYTE *s BL_REG(d0),
                                             const ULONG *lens BL_REG(a2))
{
    blitz_string = (UBYTE *)s;
    return createargstring(s, lens[-1]);
}
BLITZ_THUNK(_CreateArgString, createargstring_c, "d1/a0-a1");

/* _DeleteArgString */
static void BLITZ_ROUTINE deleteargstring_c(ULONG arg BL_REG(d0))
{
    deleteargstring(arg);
}
BLITZ_THUNK(_DeleteArgString, deleteargstring_c, "d0-d1/a0-a1");

/* _SendRexxCommand - d0 = msg, d1 = command, d2 = action, -4(a2) = the
   command's length: PutMsg to the public port "REXX" */
static void BLITZ_ROUTINE sendrexxcommand_c(ULONG msg BL_REG(d0),
                                            ULONG cmd BL_REG(d1),
                                            ULONG action BL_REG(d2),
                                            const ULONG *lens BL_REG(a2))
{
    struct RexxMsg *m;
    struct MsgPort *port;

    if (!tstrexxmsg(&msg))
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
        error_routine(ERR_NOPORT);              /* BUG(orig) (#55): no Enable */
        return;
    }
    m->rm_Node.mn_Node.ln_Type = NT_MESSAGE;
    PutMsg(port, &m->rm_Node);
    Enable();
}
BLITZ_THUNK(_SendRexxCommand, sendrexxcommand_c, "d0-d1/a0-a1");

/* _ReplyRexxMsg - d0 = msg, d1/d2 = results, d3 = result string,
   -4(a2) = its length; a3 = d3 */
static void BLITZ_ROUTINE replyrexxmsg_c(ULONG msg BL_REG(d0),
                                         ULONG res1 BL_REG(d1),
                                         ULONG res2 BL_REG(d2),
                                         ULONG str BL_REG(d3),
                                         const ULONG *lens BL_REG(a2))
{
    struct RexxMsg *m;

    blitz_string = (UBYTE *)str;
    if (!tstrexxmsg(&msg))
        return;                                 /* DIFF: original crashes */
    if (!msg)
        return;
    m = (struct RexxMsg *)msg;
    if (m->rm_Node.mn_Node.ln_Type == NT_REPLYMSG)
        return;
    m->rm_Result1 = res1;
    m->rm_Result2 = res2;
    /* BUG(orig) (#48): BTST #17,28(a0) = bit 1 of the high byte (RXFUNC) */
    if ((((const UBYTE *)&m->rm_Action)[0] & 2) && m->rm_Result1 == 0) {
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
BLITZ_THUNK(_ReplyRexxMsg, replyrexxmsg_c, "d0-d1/a0-a1");

/* _GetRexxResult - d1 = 1 (Result1) or 2 (Result2) */
static ULONG BLITZ_ROUTINE getrexxresult_c(ULONG msg BL_REG(d0),
                                           ULONG n BL_REG(d1))
{
    if (!tstrexxmsg(&msg))
        return 0;                               /* GRRError */
    /* BUG(orig) (#51): n = 0 reads rm_Action */
    return *(const ULONG *)((const UBYTE *)&((struct RexxMsg *)msg)->rm_Result1
                            + argindex(n));
}
BLITZ_THUNK(_GetRexxResult, getrexxresult_c, "d1/a0-a1");

/* copy an ArgString with its 0 to a3 (DBRA on ra_Length: length+1
   bytes, a3 behind the 0); d0 = length */
static ULONG copyarg(const UBYTE *s)
{
    UWORD len = ARGHEAD(s)->ra_Length, n = len;

    do
        *blitz_string++ = *s++;
    while (n-- != 0);
    return len;
}

/* _GetRexxCommand - d1 = 1..16: rm_Args[d1-1] as string */
static ULONG BLITZ_ROUTINE getrexxcommand_c(ULONG msg BL_REG(d0),
                                            ULONG n BL_REG(d1))
{
    ULONG arg;

    if (!tstrexxmsg(&msg))
        return 0;                               /* GRAError */
    /* BUG(orig) (#51): n = 0 reads rm_Result2 */
    arg = *(const ULONG *)((const UBYTE *)((struct RexxMsg *)msg)->rm_Args
                           + argindex(n));
    if (!tstargstring(arg))
        return 0;
    return copyarg((const UBYTE *)arg);
}
BLITZ_THUNK(_GetRexxCommand, getrexxcommand_c, "d1/a0-a1");

/* _GetResultString - rm_Result2 as string, then deleted */
static ULONG BLITZ_ROUTINE getresultstring_c(ULONG msg BL_REG(d0))
{
    ULONG arg, len;

    if (!tstrexxmsg(&msg))
        return 0;                               /* GRSError */
    arg = ((struct RexxMsg *)msg)->rm_Result2;
    if (!tstargstring(arg))
        return 0;
    len = copyarg((const UBYTE *)arg);
    deleteargstring(arg);
    return len;
}
BLITZ_THUNK(_GetResultString, getresultstring_c, "d1/a0-a1");

/* _RexxEvent - the first message of the port or 0; a reply gets its
   arguments deleted (ClearRexxMsg) */
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
BLITZ_THUNK(_RexxEvent, rexxevent_c, "d1/a0-a1");

/* _IsRexxMsg - 1 or 0 */
static ULONG BLITZ_ROUTINE isrexxmsg_c(ULONG msg BL_REG(d0))
{
    if (!RexxSysBase)
        return error_routine(ERR_REXXSYS);
    return IsRexxMsg((struct RexxMsg *)msg) ? 1 : 0;
}
BLITZ_THUNK(_IsRexxMsg, isrexxmsg_c, "d1/a0-a1");

/* _RexxError - rexxsyslib ErrorMsg (LVO -96: d0 = code -> d0 = found,
   a0 = NexxStr) as string */
static ULONG BLITZ_ROUTINE rexxerror_c(ULONG code BL_REG(d0))
{
    struct bl_regs r = { .d0 = code };
    const UBYTE *s;
    UWORD n = 0;

    if (!RexxSysBase)
        return error_routine(ERR_REXXSYS);
    BLITZ_BASEJSR(RexxSysBase, -96, r);
    s = (const UBYTE *)((struct NexxStr *)r.a0)->ns_Buff;
    if (!r.d0)
        s = (const UBYTE *)RENoError;
    do {
        *blitz_string++ = *s++;
        n++;
    } while (*s);
    return n;
}
BLITZ_THUNK(_RexxError, rexxerror_c, "d1/a0-a1");
