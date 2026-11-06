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
 *   DeviceEntryType      dol_Type of the current entry, -1 if none
 *   FreeDeviceList       UnLockDosList
 *   GetFileName(p$)      the part behind the last / or :
 *   GetPathName(p$)      the part up to and including the last / or :
 * The two string functions give the argument's string space back (a3 -
 * length - 1) and build the result in the 256 byte buffer tbuf first.
 *
 * No assembler besides the thunks.
 *
 * BUG(orig):
 *   - AddNotify: no range check of the id (64 entries; the offset is
 *     id*4 as a word), AllocMem's results are not checked, an id used
 *     twice loses the first request; a request whose StartNotify failed
 *     stays in the table (RemNotify / finit would EndNotify it) and is
 *     not counted, so finit may stop before it (#626, #619, #620, #621).
 *   - RemNotify: no range check either (#626).
 *   - finit removes the requests from id 63 down only while the counter
 *     is not 0 (#621).
 *   - FreeDeviceList does not check that the list is locked (#623):
 *     UnLockDosList releases a semaphore once too often (exec's
 *     AN_SemCorrupt alert) and always calls Permit, so the task's Forbid
 *     count goes negative (also without GetDeviceList); and it
 *     leaves the current entry: DeviceEntryType then reads the entry of
 *     the unlocked list (#622).
 *   - NextDevice without GetDeviceList returns with d0 unchanged (the
 *     result string's length is whatever d0 was) (#625); it copies the
 *     name up to its 0 (DOS BSTRs have one) and leaves a3 behind the 0
 *     (#618).
 *   - GetPathName never looks at the first character: "/abc" and ":x"
 *     give "", like a name without separator (#624); the result has a3
 *     behind its 0 (#618).  GetFileName and GetPathName take the length
 *     as a word (MOVE.w -(a2): the low word of the long) (#628), and
 *     nothing limits tbuf to 256 characters (the original overwrites the
 *     code behind it, the C build other data) (#627).
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

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#ridoslib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_ridoslib, init, BL_NORET, finit, 0);

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_addnotify, 0, 0);
BLITZ_NAME("AddNotify", "idno,filespec$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_remnotify, 0, 0);
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
BLITZ_LIBS(LIB_doslib, BL_LA(6));
BLITZ_SUBS(_lockdoslist, 0, 0);
BLITZ_NAME("GetDeviceList", "flags");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_doslib, BL_LA(6));
BLITZ_SUBS(ridoslib_nextlistitem, 0, 0);
BLITZ_NAME("NextDevice", "");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_doslib, BL_LA(6));
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

static UBYTE tbuf[256];                                 /* Dcb.b 256,0 */
static UWORD notifycnt;
static struct DosList *mydosentry, *mydoslist;
static UWORD doslistflags;
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
 * The argument's string space is given back (a3 - length - 1), the
 * result built in tbuf and then copied to a3 (no 0 written, a3 on the end
 * of the characters: the compiler adds it).  BUG(orig): the length is
 * taken as a word (#628); nothing limits tbuf to 256 characters (#627).
 * @param s     d0: the path
 * @param lens  a2: the argument stack, the length's low word at -2(a2)
 * @return d0: the length of the result
 */
static ULONG BLITZ_ROUTINE getfilename_c(const UBYTE *s BL_REG(d0), const UWORD *lens BL_REG(a2))
{
    UBYTE *t = tbuf, c;
    ULONG n = 0;

    blitz_string -= (ULONG)lens[-1] + 1;                /* the argument's space */
    while ((c = *s++) != 0) {
        if (c == '/' || c == ':')
            t = tbuf;
        else
            *t++ = c;                                   /* BUG(orig) (#627): no limit */
    }
    *t = 0;
    for (t = tbuf; (c = *t++) != 0; n++)                /* no 0 at a3 */
        *blitz_string++ = c;
    return n;
}

/**
 * @brief GetPathName(filepath$): the part up to and including the last /
 * or :.
 *
 * Searched from the end; the argument's string space is given back and
 * the result built in tbuf, then copied to a3 with its 0.  BUG(orig): the
 * first character is never looked at ("/abc" and ":x" give "", like a
 * name without separator, #624); a3 ends behind the 0 (#618); the length
 * is taken as a word (#628); tbuf is not limited (#627).
 * @param s     d0: the path
 * @param lens  a2: the argument stack, the length's low word at -2(a2)
 * @return d0: the length of the result, 0 without separator
 */
static ULONG BLITZ_ROUTINE getpathname_c(const UBYTE *s BL_REG(d0), const UWORD *lens BL_REG(a2))
{
    ULONG n = (ULONG)lens[-1] + 1, i;
    const UBYTE *p;
    UBYTE c;

    blitz_string -= n;
    p = s + n;
    do {
        if (--n == 0)
            return 0;                                   /* 'oops */
        c = *--p;
    } while (c != '/' && c != ':');
    n++;                                    /* the characters up to the separator */
    tbuf[n] = 0;
    for (i = n; i-- > 0;)
        tbuf[i] = s[i];
    i = (ULONG)-1;
    p = tbuf;
    do                                          /* with the 0: a3 behind it */
        i++;
    while ((*blitz_string++ = *p++) != 0);
    return i;
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
 * debugging leftover, no result): no defects.  BUG(orig) (#626): no range
 * check of the id (offset id*4).
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
 * BUG(orig): no range check of the id (64 entries, the offset id*4 as a
 * word, #626); AllocMem's results are not checked (#619); an id used
 * twice loses the first request (#620); a request whose StartNotify
 * failed stays in the table and is not counted (#621).
 * @param id    d0: the id
 * @param file  d1: the file name
 * @param lens  a2: the argument stack, the name's length at -4(a2)
 * @return d0: BB_TRUE if StartNotify worked, BB_FALSE otherwise or
 *         without dos.library or port
 */
static BB_BOOL BLITZ_ROUTINE addnotify_c(ULONG id BL_REG(d0), STRPTR file BL_REG(d1),
                                         const ULONG *lens BL_REG(a2))
{
    struct DosLibrary *dos = dosbase_;
    const UBYTE *s;
    UBYTE *d;
    WORD off;

    fileptr = file;
    notifynum = id;
    if (!dosbase_ || !msgport)
        return BB_FALSE;                                /* whoops */
    filelen = lens[-1] + 1;
    filecop = AllocMem(filelen, MEMF_CLEAR);            /* BUG(orig) (#619): unchecked */
    s = fileptr;
    d = filecop;
    while ((*d++ = *s++) != 0)
        ;
    notify = AllocMem(NOTIFYSIZE, MEMF_CLEAR);          /* BUG(orig) (#619): unchecked */
    off = (WORD)(notifynum << 2);                       /* ASL.w #2 */
    AT(filelens, off) = filelen;
    AT(notifys, off) = (ULONG)notify;
    notify->nr_Name = filecop;
    notify->nr_FullName = 0;
    notify->nr_UserData = notifynum;
    notify->nr_Flags = NRF_SEND_MESSAGE;
    notify->nr_stuff.nr_Msg.nr_Port = msgport;
    if (!StartNotify(notify))
        return BB_FALSE;
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
 * BUG(orig) (#621): the requests are removed from id 63 down only while
 * the counter is not 0.  The port is deleted only if dos.library was
 * open (always, where CreateMsgPort exists: no defect).
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    if (msgport) {
        LONG id = 63;
        do {
            if (!notifycnt)
                break;
            remnotify_c(id);
        } while (--id >= 0);                            /* DBRA */
    }
    if (!dosbase_)
        return;
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
 * The current entry is the list itself until NextDevice.
 * @param flags  d0.w: the LDF_ flags
 * @param dos    a6: the program's dos base (!libs #doslib,#la6)
 * @return d0: BB_TRUE, BB_FALSE if LockDosList fails
 */
static BB_BOOL BLITZ_ROUTINE lockdoslist_c(UWORD flags BL_REG(d0),
                                           struct DosLibrary *dos BL_REG(a6))
{
    doslistflags = flags | LDF_READ;                    /* BSET #0 */
    mydoslist = LockDosList(doslistflags);
    if (!mydoslist)
        return BB_FALSE;
    mydosentry = mydoslist;
    return BB_TRUE;
}

/**
 * @brief NextDevice: the name of the next entry of the locked list
 * (NextDosEntry), "" at the end.
 *
 * The name is copied to a3 up to its 0 (DOS BSTRs have one).  BUG(orig):
 * without GetDeviceList d0 comes back unchanged as the string's length
 * (#625); a3 ends behind the 0 (#618).
 * @param result  d0: the caller's d0
 * @param dos     a6: the program's dos base
 * @return d0: the length byte of the name, 0 at the end, the caller's d0
 *         without a list
 */
static ULONG BLITZ_ROUTINE nextlistitem_c(ULONG result BL_REG(d0),
                                          struct DosLibrary *dos BL_REG(a6))
{
    const UBYTE *name;

    if (!mydoslist)
        return result;
    mydosentry = NextDosEntry(mydosentry, doslistflags);
    if (!mydosentry)
        return 0;
    name = (const UBYTE *)BADDR(mydosentry->dol_Name);
    result = *name++;
    while ((*blitz_string++ = *name++) != 0)
        ;
    return result;
}

/**
 * @brief DeviceEntryType: dol_Type of the current entry (dev 0, dir 1,
 * vol 2, late 3, nonbind 4).
 *
 * BUG(orig) (#622): after FreeDeviceList it reads the entry of the
 * unlocked list.
 * @return d0: the type, -1 if there is no entry
 */
static ULONG BLITZ_ROUTINE dosentrytype_c(void)
{
    if (!mydosentry)
        return (ULONG)-1;
    return (ULONG)mydosentry->dol_Type;
}

/**
 * @brief FreeDeviceList: unlocks the DOS device list (UnLockDosList).
 *
 * BUG(orig) (#623): it does not check that the list is locked
 * (UnLockDosList releases the semaphore once too often and always calls
 * Permit); the current entry stays (#622).
 * @param incoming  d0: the caller's d0
 * @param dos       a6: the program's dos base (the original goes through
 *                  init's _dosbase: the same pointer)
 * @return d0: -1, the caller's d0 without _dosbase
 */
static ULONG BLITZ_ROUTINE unlockdoslist_c(ULONG incoming BL_REG(d0),
                                           struct DosLibrary *dos BL_REG(a6))
{
    if (!dosbase_)
        return incoming;
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
