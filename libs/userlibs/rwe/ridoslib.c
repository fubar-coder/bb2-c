/*
 * ridoslib - RWE userlib 33: file notification, the DOS device list and
 * path splitting - AddNotify, RemNotify, CheckNotify, WaitNotify,
 * GetDeviceList, NextDevice, FreeDeviceList, NotifyPort, GetFileName,
 * GetPathName, DeviceEntryType.
 *
 * C translation of rwelibsrc/DOSLib/ridoslib2.bb; reference BSS2
 * RIDOSLIB.OBJ (the reference build of the source equals it byte for
 * byte, docs/rwelibs/README.md 3.1).  Notes: docs/rwelibs/port/ridoslib.md.
 *
 * init opens dos.library V36 (_dosbase) and creates one message port
 * for all notifications; finit ends the notifications still active,
 * closes dos.library and deletes the port.
 *   AddNotify id,file$   NotifyRequest (48 bytes, MEMF_CLEAR) with a copy
 *                        of the name, UserData = id, NRF_SEND_MESSAGE to
 *                        the port; kept in notifys[id] (64 entries) - -1
 *                        if StartNotify worked, 0 otherwise
 *   RemNotify id         EndNotify, free the request and the name copy
 *   CheckNotify          the id of the next message (replied), 0 if none
 *   WaitNotify           the same, waiting for a message first
 *   GetDeviceList flags  LockDosList(flags | LDF_READ) (dos base of the
 *                        program, !libs #doslib,#la6); -1 / 0
 *   NextDevice           the name of the next entry (NextDosEntry), ""
 *                        at the end
 *   DeviceEntryType      dol_Type of the current entry, -1 if none (noted
 *                        while the list is locked)
 *   FreeDeviceList       UnLockDosList
 *   GetFileName(p$)      the part behind the last / or :
 *   GetPathName(p$)      the part up to and including the last / or :
 * The two string functions give the argument's string space back (a3 -
 * length - 1) and copy the result to its start.
 *
 * No assembler besides the thunks.
 *
 * Fixed (intended deviation, #618): NextDevice and GetPathName copied
 * their result with its 0 and left a3 behind the 0, so a string
 * expression went on one byte too far; a3 now ends on the 0.
 * Fixed (intended deviation, #623): FreeDeviceList did not check that
 * the list is locked: a second call, or one without GetDeviceList, made
 * UnLockDosList release a semaphore once too often (exec's AN_SemCorrupt
 * alert) and call Permit once too often, so the task's Forbid count went
 * negative.  Without a locked list it now does nothing (d0 = 0).
 * Fixed (intended deviation, #625): NextDevice without a locked list
 * returned with d0 unchanged, so the result string's length was whatever
 * d0 held; it is "" now.
 * Fixed (intended deviation, #622): DeviceEntryType read dol_Type of the
 * current entry, also after FreeDeviceList, when the entry may have been
 * removed and freed; GetDeviceList and NextDevice now note the type while
 * the list is locked and DeviceEntryType returns that (the same value
 * while the entry exists; no allocation).
 * Fixed (intended deviation, #619): AddNotify used the results of its two
 * AllocMem calls (the name copy, the request) unchecked: without memory
 * it wrote the name or the request to address 0 on.  It now returns 0,
 * frees the name copy if only the request failed and leaves the table
 * alone.
 * Fixed (intended deviation, #621): a request whose StartNotify failed
 * stayed in the table, uncounted, and finit, which stopped when the
 * counter was 0, could stop before it: request and name copy stayed
 * allocated after the end.  AddNotify now takes it out and frees both;
 * finit looks at all 64 entries.
 * Fixed (intended deviation, #620): AddNotify with an id in use
 * overwrote the entry: the old request stayed active (its messages kept
 * coming) but could no longer be ended.  It is now ended and freed first,
 * as RemNotify does, once the new request has its memory.
 * Fixed (intended deviation, #626): AddNotify and RemNotify used the id
 * as the index of the two tables of 64 entries without a check.  The
 * original has no error check area; here the library has one: with
 * runtime error checking an id outside 0..63 is the runtime error
 * "Notify number must be 0 to 63" (notifyidchk); release code unchanged
 * (an invalid argument).
 * Fixed (intended deviation, #1081): the DOS list stayed locked after the
 * program when it ended without FreeDeviceList (the original's finit has
 * the call commented out) or after a second GetDeviceList (one
 * FreeDeviceList released one of two locks): every later writer of the
 * list (Assign, a disk change) waited for ever.  finit now unlocks a list
 * still locked, and GetDeviceList unlocks it before it locks again.
 * Fixed (intended deviation, #627): GetFileName and GetPathName built
 * their result in a 256 byte buffer (tbuf) without a limit: a longer file
 * or path part overwrote the code behind it (the C build other data).
 * The buffer is gone: the result is copied straight into the argument's
 * space, which it never outgrows.
 * Fixed (intended deviation, #628): GetFileName and GetPathName read the
 * length of their argument as a word (MOVE.w -(a2): the low word of the
 * long): for 65536 characters or more they gave the wrong amount of
 * string space back and GetPathName searched only the first (length AND
 * $FFFF) characters.  They read the long now.
 * Fixed (intended deviation, #624): GetPathName never looked at the first
 * character (the counter was tested before each read): "/abc" and ":x"
 * gave "" like a name without separator; they give "/" and ":" now.
 * Like the original, no defects: RemNotify leaves filelens[id] set (the
 * next AddNotify of the id sets it again) and sets d4 to $12345678 (a
 * debugging leftover, d4 is no result); finit deletes the port only if
 * dos.library was open (V36 is always there where init's CreateMsgPort,
 * exec V36, is); NextDevice's d0 is the BSTR's length byte (the length
 * of the string it copied); GetFileName writes no 0, a3 on the end of
 * the characters (string results need none, the compiler adds it, see
 * #257).  FreeDeviceList, like GetDeviceList and NextDevice, goes
 * through init's _dosbase (it also checks it); the C build uses a6, the
 * program's dos base (!libs #doslib,#la6): the same pointer.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/notify.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dos
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (thunks, below) */
void _addnotify(void), _remnotify(void), _checknotify(void), _waitnotify(void);
void _lockdoslist(void), ridoslib_nextlistitem(void), _unlockdoslist(void);
void _notifyport(void), _getfilename(void), _getpathname(void);
void _dosentrytype(void), ridoslib_libinit(void), ridoslib_libfinit(void);
void notifyidchk(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#ridoslib,init,0,finit,0}; error field: the error check area
   of notifyidchk (fixed, #626) */
BLITZ_LIBHEADER(LIB_ridoslib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_addnotify, notifyidchk, 0);    /* check: fixed #626 */
BLITZ_NAME("AddNotify", "idno,filespec$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_remnotify, notifyidchk, 0);    /* check: fixed #626 */
BLITZ_NAME("RemNotify", "notify#");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_checknotify, 0, 0);
BLITZ_NAME("CheckNotify", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_waitnotify, 0, 0);
BLITZ_NAME("WaitNotify", "");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_doslib, BL_LA6);
BLITZ_SUBS(_lockdoslist, 0, 0);
BLITZ_NAME("GetDeviceList", "flags");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_doslib, BL_LA6);
BLITZ_SUBS(ridoslib_nextlistitem, 0, 0);
BLITZ_NAME("NextDevice", "");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_doslib, BL_LA6);
BLITZ_SUBS(_unlockdoslist, 0, 0);
BLITZ_NAME("FreeDeviceList", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_notifyport, 0, 0);
BLITZ_NAME("NotifyPort", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_getfilename, 0, 0);
BLITZ_NAME("GetFileName", "filepath$");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_getpathname, 0, 0);
BLITZ_NAME("GetPathName", "filepath$");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_dosentrytype, 0, 0);
BLITZ_NAME("DeviceEntryType", "    -- dev=0,dir=1,vol=2,late=3,nonbind=4");

BLITZ_NULLSUB_AT(init, ridoslib_libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, ridoslib_libfinit, 0, 0);

BLITZ_LIBFIN();

#define NOTIFYSIZE 48                   /* sizeof(struct NotifyRequest) */
static_assert(sizeof(struct NotifyRequest) == NOTIFYSIZE);
#define NNOTIFY 64

static UWORD notifycnt;
static struct DosList *mydosentry, *mydoslist;
static UWORD doslistflags;
/* dol_Type of the current entry, -1 without one (fixed, #622: noted while
   the list is locked, DeviceEntryType reads no entry) */
static ULONG entrytype = (ULONG)-1;
static struct DosLibrary *dosbase_;                     /* _dosbase */
static const char dosname[] = "dos.library";
static STRPTR fileptr;
static ULONG filelen;
static UBYTE *filecop;
/* notifys / filelens: Dc.l 0 in front of each, as in the original (an id
   of -1 reads that 0) */
static ULONG notifys_[1 + NNOTIFY], filelens_[1 + NNOTIFY];
#define notifys (notifys_ + 1)
#define filelens (filelens_ + 1)
static struct NotifyRequest *notify;
static ULONG notifynum;
static struct MsgPort *msgport;

/* the table entry of id: offset id*4 (AddNotify: as a word, ASL.w/ADD.w) */
#define AT(tab, off) (*(ULONG *)((UBYTE *)(tab) + (off)))

/* ---------------------------------------------------------- the paths */

/**
 * @brief GetFileName(filepath$): the part behind the last / or :.
 *
 * The argument's string space is given back (a3 - length - 1) and the
 * result copied to its start, from the character behind the last
 * separator on (the result is never longer than the argument and lies
 * behind its start: a forward copy); no 0 written, a3 on the end of the
 * characters (the compiler adds it).  Fixed (#627): the original built
 * the result in a 256 byte buffer first, without a limit.  Fixed (#628):
 * the original took the length as a word.
 * @param s     d0: the path
 * @param lens  a2: the argument stack, the length (a long) at -4(a2)
 * @return d0: the length of the result
 */
static ULONG BLITZ_ROUTINE getfilename_c(const UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    const UBYTE *f = s;
    UBYTE c;
    ULONG n = 0;

    blitz_string -= lens[-1] + 1;               /* the argument's space (fixed, #628: a long) */
    while ((c = *s++) != 0)
        if (c == '/' || c == ':')
            f = s;
    while ((c = *f++) != 0) {                           /* no 0 at a3 */
        *blitz_string++ = c;
        n++;
    }
    return n;
}

/**
 * @brief GetPathName(filepath$): the part up to and including the last /
 * or :.
 *
 * Searched from the end; the argument's string space is given back and
 * the result copied to its start with its 0 (a forward copy: the result
 * is the argument's beginning); a3 ends on the 0 (fixed, #618: the
 * original left it behind the 0).  Fixed (#627): the original built the
 * result in a 256 byte buffer first, without a limit.  Fixed (#624): the
 * original never looked at the first character ("/abc" and ":x" gave "",
 * like a name without separator).  Fixed (#628): the original took the
 * length as a word.
 * @param s     d0: the path
 * @param lens  a2: the argument stack, the length (a long) at -4(a2)
 * @return d0: the length of the result, 0 without separator
 */
static ULONG BLITZ_ROUTINE getpathname_c(const UBYTE *s BL_REG(d0), const ULONG *lens BL_REG(a2))
{
    ULONG n = lens[-1] + 1, i;                  /* fixed (#628): a long */
    const UBYTE *p;
    UBYTE c;

    blitz_string -= n;
    p = s + n;
    do {
        if (n-- == 0)                   /* fixed (#624): s[0] is looked at */
            return 0;                                   /* 'oops */
        c = *--p;
    } while (c != '/' && c != ':');
    n++;                                    /* the characters up to the separator */
    for (i = 0; i < n; i++)
        *blitz_string++ = s[i];
    *blitz_string = 0;                          /* fixed (#618): a3 on the 0 */
    return n;
}

/* ------------------------------------------------------ notifications */

/**
 * @brief NotifyPort: the message port of the notifications.
 * @return d0: the port (init's CreateMsgPort), 0 if there is none
 */
static ULONG BLITZ_ROUTINE notifyport_c(void)
{
    return (ULONG)msgport;
}

/* _dochecknotify: the UserData (id) of the next message, replied */
static ULONG dochecknotify(void)
{
    struct NotifyMessage *m = (struct NotifyMessage *)GetMsg(msgport);
    ULONG id;

    if (!m)
        return 0;
    id = m->nm_NReq->nr_UserData;
    ReplyMsg(&m->nm_ExecMessage);
    return id;
}

/**
 * @brief CheckNotify: the id of the next notification message.
 *
 * The message is replied.
 * @return d0: the id (the request's UserData), 0 if there is none or no
 *         port
 */
static ULONG BLITZ_ROUTINE checknotify_c(void)
{
    if (!msgport)
        return 0;
    return dochecknotify();
}

/**
 * @brief WaitNotify: waits for a notification message and returns its id.
 *
 * WaitPort, then as CheckNotify.
 * @return d0: the id, 0 without a port
 */
static ULONG BLITZ_ROUTINE waitnotify_c(void)
{
    if (!msgport)
        return 0;
    WaitPort(msgport);
    return dochecknotify();
}

/**
 * @brief RemNotify notify#: ends a notification and frees its request and
 * the name copy.
 *
 * filelens[id] stays set and the original sets d4 to $12345678 (a
 * debugging leftover, no result): no defects.  An id outside 0..63 reads
 * outside the table (offset id*4): an invalid argument, refused by
 * notifyidchk with runtime error checking (#626).
 * @param id  d0: the id
 * @return d0: unchanged (id) without dos.library or port, id*4 if the
 *         entry is empty, else the address of the freed name (a
 *         statement: the value is not used)
 */
static ULONG BLITZ_ROUTINE remnotify_c(LONG id BL_REG(d0))
{
    struct DosLibrary *dos = dosbase_;
    LONG off = id << 2;                                 /* ASL.l #2 */
    ULONG r;

    if (!dosbase_ || !msgport)
        return (ULONG)id;                               /* oops */
    notify = (struct NotifyRequest *)AT(notifys, off);
    AT(notifys, off) = 0;
    filelen = AT(filelens, off);
    if (!notify)
        return (ULONG)off;
    EndNotify(notify);
    r = (ULONG)notify->nr_Name;
    FreeMem((APTR)r, filelen);
    FreeMem(notify, NOTIFYSIZE);
    notifycnt--;
    return r;
}

/**
 * @brief AddNotify(idno,filespec$): starts a notification for a file.
 *
 * A NotifyRequest (48 bytes, MEMF_CLEAR) with a copy of the name,
 * UserData = id, NRF_SEND_MESSAGE to the port; kept in notifys[id].
 * Without memory for the name copy or the request it returns 0 and
 * leaves the table alone (fixed, #619: the original used AllocMem's
 * results unchecked).  An id outside 0..63 writes outside the tables (64
 * entries, the offset id*4 as a word): an invalid argument, refused by
 * notifyidchk with runtime error checking (#626).  A request already in the entry of
 * the id is ended and freed first, as RemNotify does (fixed, #620: the
 * original overwrote the entry, the old request stayed active and could
 * no longer be ended).  A request whose StartNotify fails is taken out of the
 * table and freed with its name copy (fixed, #621: the original left it
 * there, uncounted).
 * @param id    d0: the id
 * @param file  d1: the file name
 * @param lens  a2: the argument stack, the name's length at -4(a2)
 * @return d0: BB_TRUE if StartNotify worked, BB_FALSE otherwise, without
 *         memory or without dos.library or port
 */
static BB_BOOL BLITZ_ROUTINE addnotify_c(ULONG id BL_REG(d0), STRPTR file BL_REG(d1),
                                         const ULONG *lens BL_REG(a2))
{
    struct DosLibrary *dos = dosbase_;
    struct NotifyRequest *old;
    const UBYTE *s;
    UBYTE *d;
    WORD off;

    fileptr = file;
    notifynum = id;
    if (!dosbase_ || !msgport)
        return BB_FALSE;                                /* whoops */
    filelen = lens[-1] + 1;
    filecop = AllocMem(filelen, MEMF_CLEAR);
    if (!filecop)
        return BB_FALSE;                                /* fixed (#619) */
    s = fileptr;
    d = filecop;
    while ((*d++ = *s++) != 0)
        ;
    notify = AllocMem(NOTIFYSIZE, MEMF_CLEAR);
    if (!notify) {                                      /* fixed (#619) */
        FreeMem(filecop, filelen);
        return BB_FALSE;
    }
    off = (WORD)(notifynum << 2);                       /* ASL.w #2 */
    old = (struct NotifyRequest *)AT(notifys, off);
    if (old) {                                  /* fixed (#620): as RemNotify */
        EndNotify(old);
        FreeMem(old->nr_Name, AT(filelens, off));
        FreeMem(old, NOTIFYSIZE);
        notifycnt--;
    }
    AT(filelens, off) = filelen;
    AT(notifys, off) = (ULONG)notify;
    notify->nr_Name = filecop;
    notify->nr_FullName = 0;
    notify->nr_UserData = notifynum;
    notify->nr_Flags = NRF_SEND_MESSAGE;
    notify->nr_stuff.nr_Msg.nr_Port = msgport;
    if (!StartNotify(notify)) {                         /* fixed (#621) */
        AT(notifys, off) = 0;
        FreeMem(filecop, filelen);
        FreeMem(notify, NOTIFYSIZE);
        return BB_FALSE;
    }
    notifycnt++;
    return BB_TRUE;
}

/* ---------------------------------------------------------- init/finit */

/**
 * @brief init: opens dos.library V36 (_dosbase) and creates the message
 * port of all notifications.
 * @return d0: the port (init's result is not used: BL_NORET)
 */
static ULONG BLITZ_ROUTINE libinit_c(void)
{
    notifycnt = 0;
    dosbase_ = (struct DosLibrary *)OpenLibrary((CONST_STRPTR)dosname, 36);
    msgport = CreateMsgPort();
    return (ULONG)msgport;
}

/**
 * @brief finit: ends the notifications still active, closes dos.library
 * and deletes the port.
 *
 * All 64 entries are looked at, from id 63 down (fixed, #621: the
 * original stopped when the counter was 0).  The port is deleted only if
 * dos.library was open (always, where CreateMsgPort exists: no defect).
 * A DOS list GetDeviceList still holds is unlocked before dos.library is
 * closed (fixed, #1081: the original had the call commented out, the list
 * stayed locked after the program).
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    if (msgport) {
        LONG id = 63;
        do
            remnotify_c(id);
        while (--id >= 0);                              /* DBRA */
    }
    if (!dosbase_)
        return;
    if (mydoslist) {                                    /* fixed (#1081) */
        struct DosLibrary *dos = dosbase_;
        UnLockDosList(doslistflags);
        mydoslist = NULL;
    }
    CloseLibrary(&dosbase_->dl_lib);
    dosbase_ = NULL;
    if (msgport)
        DeleteMsgPort(msgport);
}

/* ------------------------------------------------------ the device list */

/**
 * @brief GetDeviceList(flags): locks the DOS device list
 * (LockDosList(flags | LDF_READ)).
 *
 * The current entry is the list itself until NextDevice; its dol_Type is
 * noted for DeviceEntryType (#622).  A list still locked by an earlier
 * GetDeviceList is unlocked first (fixed, #1081: the original locked it a
 * second time, and one FreeDeviceList released only one of the locks).
 * @param flags  d0.w: the LDF_ flags
 * @param dos    a6: the program's dos base (!libs #doslib,#la6)
 * @return d0: BB_TRUE, BB_FALSE if LockDosList fails
 */
static BB_BOOL BLITZ_ROUTINE lockdoslist_c(UWORD flags BL_REG(d0),
                                           struct DosLibrary *dos BL_REG(a6))
{
    if (mydoslist)                                      /* fixed (#1081) */
        UnLockDosList(doslistflags);
    doslistflags = flags | LDF_READ;                    /* BSET #0 */
    mydoslist = LockDosList(doslistflags);
    if (!mydoslist)
        return BB_FALSE;
    mydosentry = mydoslist;
    entrytype = (ULONG)mydosentry->dol_Type;            /* fixed (#622) */
    return BB_TRUE;
}

/**
 * @brief NextDevice: the name of the next entry of the locked list
 * (NextDosEntry), "" at the end.
 *
 * The name is copied to a3 up to its 0 (DOS BSTRs have one); a3 ends on
 * the 0 (fixed, #618: the original left it behind the 0).  Without a
 * locked list the result is "" (fixed, #625: the original returned with
 * d0 unchanged, the string's length whatever d0 was).
 * @param dos     a6: the program's dos base
 * @return d0: the length byte of the name, 0 at the end or without a
 *         list
 */
static ULONG BLITZ_ROUTINE nextlistitem_c(struct DosLibrary *dos BL_REG(a6))
{
    const UBYTE *name;
    ULONG result;

    if (!mydoslist)
        return 0;                                       /* fixed (#625) */
    mydosentry = NextDosEntry(mydosentry, doslistflags);
    if (!mydosentry) {
        entrytype = (ULONG)-1;                          /* fixed (#622) */
        return 0;
    }
    entrytype = (ULONG)mydosentry->dol_Type;            /* fixed (#622) */
    name = (const UBYTE *)BADDR(mydosentry->dol_Name);
    result = *name++;
    while ((*blitz_string++ = *name++) != 0)
        ;
    blitz_string--;                             /* fixed (#618): a3 on the 0 */
    return result;
}

/**
 * @brief DeviceEntryType: dol_Type of the current entry (dev 0, dir 1,
 * vol 2, late 3, nonbind 4).
 *
 * The type noted by GetDeviceList and NextDevice while the list was
 * locked; no entry is read, so the result stays the same after
 * FreeDeviceList (fixed, #622: the original read dol_Type of the current
 * entry also after the unlock, when it may have been freed).
 * @return d0: the type, -1 if there is no entry
 */
static ULONG BLITZ_ROUTINE dosentrytype_c(void)
{
    return entrytype;
}

/**
 * @brief FreeDeviceList: unlocks the DOS device list (UnLockDosList).
 *
 * Only a list that GetDeviceList locked is unlocked (fixed, #623: the
 * original did not check it, and UnLockDosList released the semaphore
 * once too often and called Permit once too often).  The current entry
 * stays; DeviceEntryType does not read it (#622).
 * @param incoming  d0: the caller's d0
 * @param dos       a6: the program's dos base (the original goes through
 *                  init's _dosbase: the same pointer)
 * @return d0: -1, 0 without a locked list, the caller's d0 without
 *         _dosbase
 */
static ULONG BLITZ_ROUTINE unlockdoslist_c(ULONG incoming BL_REG(d0),
                                           struct DosLibrary *dos BL_REG(a6))
{
    if (!dosbase_)
        return incoming;
    if (!mydoslist)
        return 0;                                       /* fixed (#623) */
    UnLockDosList(doslistflags);
    mydoslist = NULL;
    return (ULONG)-1;
}

/* Registers: the original changes d0, a0/a1, d4/d5 (the string
   functions, RemNotify), d1/d2 and a6 (the system calls); the thunks
   keep d1/a0/a1, the C code d2-d7/a2-a6. */
BLITZ_THUNK(_getfilename, getfilename_c, "d1/a0-a1");
BLITZ_THUNK(_getpathname, getpathname_c, "d1/a0-a1");
BLITZ_THUNK(_notifyport, notifyport_c, "d1/a0-a1");
BLITZ_THUNK(_checknotify, checknotify_c, "d1/a0-a1");
BLITZ_THUNK(_waitnotify, waitnotify_c, "d1/a0-a1");
BLITZ_THUNK(_remnotify, remnotify_c, "d1/a0-a1");
BLITZ_THUNK(_addnotify, addnotify_c, "d1/a0-a1");
BLITZ_THUNK(ridoslib_libinit, libinit_c, "d1/a0-a1");
BLITZ_THUNK(ridoslib_libfinit, libfinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_lockdoslist, lockdoslist_c, "d1/a0-a1");
BLITZ_THUNK(ridoslib_nextlistitem, nextlistitem_c, "d1/a0-a1");
BLITZ_THUNK(_dosentrytype, dosentrytype_c, "d1/a0-a1");
BLITZ_THUNK(_unlockdoslist, unlockdoslist_c, "d1/a0-a1");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(badid, "Notify number must be 0 to 63");  /* fixed, #626 */

/**
 * @brief The error check of AddNotify and RemNotify (debugger only).
 *
 * Fixed (#626), not in the original (ridoslib had no error check area):
 * the id indexes the two tables of 64 entries without a check, so an id
 * outside 0..63 made AddNotify write and RemNotify read, EndNotify and
 * free the library's data around the tables.  The release code is
 * unchanged (an invalid argument).
 *
 * Errors: "Notify number must be 0 to 63".
 * @param id  d0: the id
 */
static void BLITZ_ERRCHK notifyidchk_c(ULONG id BL_REG(d0))
{
    if (id >= NNOTIFY)
        BLITZ_RUNERR(badid);
}

/* the thunk of the check function */
BLITZ_ERRCHK_THUNK(notifyidchk, notifyidchk_c, "d0-d1/a0-a1");
