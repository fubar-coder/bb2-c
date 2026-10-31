/*
 * riapplib - userlib 61 (RWE): Workbench App windows, App icons and App
 * menu items (workbench.library V37) with one message port.
 *   AddAppWindow window#    DelAppWindow window#
 *   AddAppIcon id,text$,filename$[,x,y]   DelAppIcon id
 *   AddAppMenu id,text$     DelAppMenu id
 *   AppEvent  (next message, replies to the previous one)
 *   AppEventType  1 window, 2 icon, 3 menu      AppEventID
 *   NextAppFile / AppFile(n) / AppNumFiles / AppLock
 *   AppMessage / AppMsgPort / AppEventIDCMP ($800000)
 *
 * C translation of rwelibsrc/AppLib/applib2.bb; the declarations equal
 * BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS/RIAPPLIB.OBJ (ctest
 * rwe_declarations_riapplib), the behaviour is compared with it in the
 * emulator (libs/tests/rwe/riapplib.c).  Notes: docs/rwelibs/port/riapplib.md.
 *
 * init opens a message port, one block of 768 bytes (MEMF_CLEAR) and
 * workbench, dos and icon.library V37.  The block: 16 longs each for the
 * AppMenuItems (+0), AppWindows (+64) and AppIcons (+128), the path buffer
 * of NextAppFile (+192, 512 bytes) and the DiskObjects of the App icons
 * (+704).  The ids index these tables unchecked (long index).
 *
 * All routines are C; every entry keeps d1/a0/a1 (thunks), init and finit
 * all registers, as the original (MOVEM) - more than the original keeps
 * elsewhere.  a2 stays unchanged (AddAppWindow's original leaves it
 * decremented and AddAppMenu/AddAppWindow/AddAppIcon set it to 0; DIFF,
 * see docs/libs/cmdlibs-port.md "General").  Assembler: the inline routine
 * AppEventIDCMP (Dc $a000), and the AddAppIconA call, which needs a3/a4
 * (the fixed registers of the Blitz program, see below).
 *
 * BUG(orig), kept:
 *   - AddAppIcon passes the DiskObject in a3 and the tag list (0) in a4
 *     to AddAppIconA and restores neither: after it a3 (string pointer) =
 *     the DiskObject and a4 (locals of the Blitz program) = 0 (#516).
 *   - finit clears the table slot at byte offset index*4/4 instead of
 *     index*4 (ASR.l #2 before the store): a long store at an odd address
 *     for the slots 1, 3 ... (an address error on a 68000) (#524).
 *   - AddAppMenu returns the AppMenuItem address, not -1 (the MOVEQ #-1
 *     behind the RTS is never reached) (#518).
 *   - DelAppWindow calls RemoveAppWindow also for an empty slot (NULL);
 *     DelAppIcon calls FreeDiskObject also for an empty DiskObject slot
 *     (#523).
 *   - AddAppIcon on a used id removes the old AppIcon and frees its
 *     DiskObject but leaves the old AppIcon in the slot if AddAppIconA
 *     fails (a later DelAppIcon removes it again); the new DiskObject is
 *     stored before AddAppIconA and stays on failure (#517).
 *   - NextAppFile/AppFile unlock the lock of the WBArg (ChangeMode +
 *     UnLock - Workbench's own lock) and clear it in the message, unless
 *     the path ends with ':'.  The ':' test reads the byte before the
 *     string if NameFromLock gives an empty path (#521).  a3 ends BEHIND
 *     the 0 of the result (d0 = the length is right) (#519).  AppFile(n)
 *     with n <= 0 reads the WBArg before the list (#520); AppLock reads
 *     the first WBArg without looking at am_NumArgs (#522).
 *   - finit replies to the messages queued at the port only if a message
 *     is current (the last AppEvent) - the flag `msgportflag` for the
 *     other case is never set; otherwise they stay unreplied when the
 *     port is deleted (#525).
 *   - init: if a library does not open, the port is deleted and the block
 *     freed, but the libraries already opened stay open; AppMsgPort then
 *     still returns the deleted port (#526).
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <exec/ports.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dosbase
#define WB_BASE_NAME wbbase
#define ICON_BASE_NAME iconbase
#include <proto/dos.h>
#include <proto/wb.h>
#include <proto/icon.h>
#undef __NOLIBBASE__

/* entry points (thunks, defined below) */
void addwindow(void), delwindow(void), addmenu(void), delmenu(void);
void addicon(void), addicon3(void), delicon(void), appevent(void);
void appeventcode(void), appeventid(void), appfile(void), appfilemine(void);
void appfiles(void), applock(void), appmessage(void), appmsgport(void);
void appeventidcmp(void), libinit(void), libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

#define IA(n) (BL_LA(n) | BL_ITEM | BL_PD(n))   /* #ia<n>|#pd<n> */

/* !libheader {#riapplib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_riapplib, init, BL_NORET, finit, 0);

/* all code addresses odd (`label+1`): Amiga mode only */
BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_PUSH | BL_LONG);
BLITZ_LIBS(LIB_windowslib, IA(0));
BLITZ_SUBS(BLITZ_AMIGAONLY(addwindow), 0, 0);
BLITZ_NAME("AddAppWindow", "(windownumber)");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(delwindow), 0, 0);
BLITZ_NAME("DelAppWindow", "(windownumber)");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(addmenu), 0, 0);
BLITZ_NAME("AddAppMenu", "(id,text$)");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(delmenu), 0, 0);
BLITZ_NAME("DelAppMenu", "(id)");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG, BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(addicon), 0, 0);
BLITZ_ARGS(BL_LONG, BL_STRING, BL_STRING, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(addicon3), 0, 0);
BLITZ_NAME("AddAppIcon", "(id,text$,filename$[,x,y])");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(delicon), 0, 0);
BLITZ_NAME("DelAppIcon", "(id)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(appevent), 0, 0);
BLITZ_NAME("AppEvent", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(appeventcode), 0, 0);
BLITZ_NAME("AppEventType", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(appeventid), 0, 0);
BLITZ_NAME("AppEventID", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(appfile), 0, 0);
BLITZ_NAME("NextAppFile", "");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(appfilemine), 0, 0);
BLITZ_NAME("AppFile", "(num)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(appfiles), 0, 0);
BLITZ_NAME("AppNumFiles", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(applock), 0, 0);
BLITZ_NAME("AppLock", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(appmessage), 0, 0);
BLITZ_NAME("AppMessage", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(appmsgport), 0, 0);
BLITZ_NAME("AppMsgPort", "");

/* the only entry that also works in Blitz mode: an inline routine */
BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(appeventidcmp, 0, 0);
BLITZ_NAME("AppEventIDCMP", "");

BLITZ_NULLSUB_AT(finit, libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, libinit, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

#define BLOCKSIZE 768                   /* 192 + 512 + 64 */

/* the block of init: the three tables of 16 objects, the path buffer
   of NextAppFile and the DiskObjects of the App icons; the original's
   variables below point into it */
struct appblock {
    ULONG menus[16];                    /* +0 the AppMenuItems */
    ULONG windows[16];                  /* +64 the AppWindows */
    ULONG icons[16];                    /* +128 the AppIcons */
    UBYTE path[512];                    /* +192 */
    struct DiskObject *disks[16];       /* +704 */
};
static_assert(sizeof(struct appblock) == BLOCKSIZE);
#define NO_POS 0x80000000UL             /* NO_ICON_POSITION */

static struct Library *wbbase, *iconbase;
static struct DosLibrary *dosbase;
static ULONG msgportflag;               /* never set (see finit) */
static ULONG *appmenus, *appwindows, *appicons;
static struct DiskObject **disklist;
static UBYTE *arglist;                  /* path buffer of NextAppFile */
static struct MsgPort *msgport;
static UWORD msgtype;
static ULONG msgid;
static LONG numargs;
static LONG libflag;                    /* -1 = all opened */
static struct AppMessage *msg;          /* current message */
static LONG argnum;                     /* next argument of NextAppFile */
static ULONG iconx, icony;              /* position of the next App icon */

/* AppEventIDCMP: MOVE.l #$800000,d0 (inline) */
BLITZ_INLINE(appeventidcmp, "move.l #0x800000,d0");

/* ---------------------------------------------------------------- init */

static void BLITZ_ROUTINE libinit_c(void)
{
    struct appblock *block;

    msgportflag = 0;
    msgport = CreateMsgPort();
    if (!msgport) {
        libflag = 0;
        return;
    }
    block = AllocMem(BLOCKSIZE, MEMF_CLEAR);
    if (!block) {
        DeleteMsgPort(msgport);
        libflag = 0;
        return;
    }
    appmenus = block->menus;
    appwindows = block->windows;
    appicons = block->icons;
    arglist = block->path;
    disklist = block->disks;
    /* BUG(orig): the libraries opened before a failing one stay open (#526) */
    if (!(wbbase = OpenLibrary((STRPTR)"workbench.library", 37))
        || !(dosbase = (struct DosLibrary *)OpenLibrary((STRPTR)"dos.library", 37))
        || !(iconbase = OpenLibrary((STRPTR)"icon.library", 37))) {
        DeleteMsgPort(msgport);
        libflag = 0;
        FreeMem(appmenus, BLOCKSIZE);
        return;
    }
    libflag = -1;
}
BLITZ_THUNK(libinit, libinit_c, "d0-d1/a0-a1");

/* delapp: remove every object of a table of 16; BUG(orig) (#524): the slot that
   is cleared is the long at byte offset index*4/4 (an odd address for
   odd indexes: GCC writes it with one MOVE.l, as the original) */
static void delapp(ULONG *table, int kind)
{
    ULONG off;

    for (off = 0;; off += 4) {
        ULONG obj = *(ULONG *)((UBYTE *)table + off);
        if (obj) {
            switch (kind) {
            case 0: RemoveAppMenuItem((struct AppMenuItem *)obj); break;
            case 1: RemoveAppWindow((struct AppWindow *)obj); break;
            case 2: RemoveAppIcon((struct AppIcon *)obj); break;
            default: FreeDiskObject((struct DiskObject *)obj); break;
            }
            *(volatile ULONG *)((UBYTE *)table + (off >> 2)) = 0;
        }
        if (off == 60)
            break;
    }
}

static void BLITZ_ROUTINE libfinit_c(void)
{
    struct Message *m;

    if (!libflag)
        return;
    /* BUG(orig): the queue is replied only if a message is current (#525) */
    m = (struct Message *)msg;
    if (m || (msgportflag && (m = GetMsg(msgport)) != NULL)) {
        do
            ReplyMsg(m);
        while ((m = GetMsg(msgport)) != NULL);
    }
    delapp(appmenus, 0);
    delapp(appwindows, 1);
    delapp(appicons, 2);
    delapp((ULONG *)disklist, 3);
    DeleteMsgPort(msgport);
    FreeMem(appmenus, BLOCKSIZE);
    CloseLibrary((struct Library *)dosbase);
    CloseLibrary(wbbase);
    CloseLibrary(iconbase);
}
BLITZ_THUNK(libfinit, libfinit_c, "d0-d1/a0-a1");

/* ----------------------------------------------------- message functions */

static ULONG BLITZ_ROUTINE appmsgport_c(void)
{
    return (ULONG)msgport;
}
BLITZ_THUNK(appmsgport, appmsgport_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE appmessage_c(void)
{
    if (!libflag || !msg)
        return 0;
    return (ULONG)msg;
}
BLITZ_THUNK(appmessage, appmessage_c, "d1/a0-a1");

/* AppLock: the lock of the first argument */
static ULONG BLITZ_ROUTINE applock_c(void)
{
    if (!libflag || !msg)
        return 0;
    return (ULONG)msg->am_ArgList->wa_Lock;
}
BLITZ_THUNK(applock, applock_c, "d1/a0-a1");

/* AppEvent: reply to the current message, get the next one */
static ULONG BLITZ_ROUTINE appevent_c(void)
{
    argnum = 0;
    if (!libflag)
        return 0;
    if (msg) {
        ReplyMsg(&msg->am_Message);
        msg = NULL;
    }
    msg = (struct AppMessage *)GetMsg(msgport);
    if (!msg)
        return 0;
    msgid = msg->am_ID;
    numargs = msg->am_NumArgs;
    msgtype = msg->am_Type;
    return 0x800000;
}
BLITZ_THUNK(appevent, appevent_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE appfiles_c(void)
{
    if (!libflag || !msg)
        return 0;
    return numargs;
}
BLITZ_THUNK(appfiles, appfiles_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE appeventid_c(void)
{
    if (!libflag || !msg)
        return 0;
    return msgid;
}
BLITZ_THUNK(appeventid, appeventid_c, "d1/a0-a1");

/* AppEventType: 1 AppWindow (am_Type 7), 2 AppIcon (8), 3 AppMenuItem (9) */
static ULONG BLITZ_ROUTINE appeventcode_c(void)
{
    if (!libflag || !msg)
        return 0;
    switch (msgtype) {
    case AMTYPE_APPWINDOW: return 1;
    case AMTYPE_APPICON: return 2;
    case AMTYPE_APPMENUITEM: return 3;
    }
    return 0;
}
BLITZ_THUNK(appeventcode, appeventcode_c, "d1/a0-a1");

/* NextAppFile: the full path of the next argument at a3; a3 ends behind
   the 0 (BUG(orig), #519), d0 = the length */
static ULONG BLITZ_ROUTINE appfile_c(void)
{
    struct WBArg *arg;
    UBYTE *start, *s;
    BPTR lock;

    if (!libflag || !msg)
        return 0;
    if (argnum >= numargs)
        return 0;
    arg = msg->am_ArgList + argnum++;
    lock = arg->wa_Lock;
    if (!NameFromLock(lock, (STRPTR)arglist, 255))
        return 0;
    start = blitz_string;
    s = arglist;
    while ((*blitz_string++ = *s++) != 0)
        ;
    blitz_string--;
    /* the byte before the 0: before the string if the path is empty */
    if (blitz_string[-1] != ':') {
        ChangeMode(CHANGE_LOCK, lock, SHARED_LOCK);     /* "Let us Unlock() it!" */
        arg->wa_Lock = 0;
        UnLock(lock);
        *blitz_string++ = '/';
    }
    s = (UBYTE *)arg->wa_Name;
    while ((*blitz_string++ = *s++) != 0)
        ;
    return blitz_string - start - 1;
}
BLITZ_THUNK(appfile, appfile_c, "d1/a0-a1");

/* AppFile(num): argument num (1 = the first), argnum stays */
static ULONG BLITZ_ROUTINE appfilemine_c(LONG num BL_REG(d0))
{
    LONG keep = argnum;
    ULONG r;

    argnum = num - 1;
    r = appfile_c();
    argnum = keep;
    return r;
}
BLITZ_THUNK(appfilemine, appfilemine_c, "d1/a0-a1");

/* ----------------------------------------------------------- App menus */

static BB_BOOL BLITZ_ROUTINE delmenu_c(LONG id BL_REG(d0))
{
    ULONG *slot;

    if (!libflag)
        return BB_FALSE;
    slot = &appmenus[id];
    if (!*slot)
        return BB_FALSE;
    RemoveAppMenuItem((struct AppMenuItem *)*slot);
    appmenus[id] = 0;
    return BB_TRUE;
}
BLITZ_THUNK(delmenu, delmenu_c, "d1/a0-a1");

/* AddAppMenu id,text$ - BUG(orig): returns the AppMenuItem, not -1 (#518) */
static ULONG BLITZ_ROUTINE addmenu_c(LONG id BL_REG(d0), STRPTR text BL_REG(d1))
{
    struct AppMenuItem *item;

    if (!libflag)
        return 0;
    if (appmenus[id])
        delmenu_c(id);
    item = AddAppMenuItemA(id, 0, text, msgport, NULL);
    if (!item)
        return 0;
    appmenus[id] = (ULONG)item;
    return (ULONG)item;
}
BLITZ_THUNK(addmenu, addmenu_c, "d1/a0-a1");

/* ----------------------------------------------------------- App icons */

/* AddAppIcon id,text$,filename$[,x,y] (after the libflag test) - the
   icon from GetDiskObject */
static BB_BOOL BLITZ_ROUTINE addicon_c(LONG id BL_REG(d0), STRPTR text BL_REG(d1),
                                       STRPTR name BL_REG(d2))
{
    struct DiskObject *dobj = GetDiskObject(name);
    struct AppIcon *icon;

    if (!dobj)
        return BB_FALSE;
    dobj->do_CurrentX = iconx;
    dobj->do_CurrentY = icony;
    if (appicons[id]) {
        RemoveAppIcon((struct AppIcon *)appicons[id]);
        if (disklist[id]) {
            FreeDiskObject(disklist[id]);
            disklist[id] = NULL;
        }
    }
    disklist[id] = dobj;
    /* AddAppIconA(id,userdata,text,msgport,lock,diskobj,taglist) takes
       the DiskObject in a3 and the tag list in a4, the registers the Blitz
       program keeps fixed (-ffixed-a3/a4): set in assembler.  BUG(orig) (#516):
       neither is restored - a3 = the DiskObject, a4 = 0 afterwards. */
    {
        register ULONG d0 __asm("d0") = id;
        register ULONG d1 __asm("d1") = 0;
        register STRPTR a0 __asm("a0") = text;
        register struct MsgPort *a1 __asm("a1") = msgport;
        register ULONG a2 __asm("a2") = 0;
        register struct Library *a6 __asm("a6") = wbbase;
        __asm volatile("move.l  %6,a3\n\t"
                       "suba.l  a4,a4\n\t"
                       "jsr     -60(a6)"
                       : "+d"(d0), "+d"(d1), "+a"(a0), "+a"(a1), "+a"(a2), "+a"(a6)
                       : "r"(dobj)
                       : "cc", "memory");
        icon = (struct AppIcon *)d0;
    }
    if (!icon)
        return BB_FALSE;
    appicons[id] = (ULONG)icon;
    return BB_TRUE;
}

static BB_BOOL BLITZ_ROUTINE addicon2_c(LONG id BL_REG(d0), STRPTR text BL_REG(d1),
                                        STRPTR name BL_REG(d2))
{
    iconx = icony = NO_POS;
    if (!libflag)
        return BB_FALSE;
    return addicon_c(id, text, name);
}
BLITZ_THUNK(addicon, addicon2_c, "d1/a0-a1");

static BB_BOOL BLITZ_ROUTINE addicon3_c(LONG id BL_REG(d0), STRPTR text BL_REG(d1),
                                        STRPTR name BL_REG(d2), ULONG x BL_REG(d3),
                                        ULONG y BL_REG(d4))
{
    iconx = x;
    icony = y;
    if (!libflag)
        return BB_FALSE;
    return addicon_c(id, text, name);
}
BLITZ_THUNK(addicon3, addicon3_c, "d1/a0-a1");

static BB_BOOL BLITZ_ROUTINE delicon_c(LONG id BL_REG(d0))
{
    ULONG *slot;

    if (!libflag)
        return BB_FALSE;
    slot = &appicons[id];
    if (!*slot)
        return BB_FALSE;
    RemoveAppIcon((struct AppIcon *)*slot);
    *slot = 0;
    FreeDiskObject(disklist[id]);       /* BUG(orig): also NULL (#523) */
    disklist[id] = NULL;
    return BB_TRUE;
}
BLITZ_THUNK(delicon, delicon_c, "d1/a0-a1");

/* --------------------------------------------------------- App windows */

/* AddAppWindow window# - a0 = the windowslib object (first long = the
   Intuition window), the number on the compiler's stack (#push) */
static BB_BOOL BLITZ_ROUTINE addwindow_c(struct Window **wobj BL_REG(a0),
                                         const ULONG *args BL_REG(a2))
{
    LONG n = args[-1];                  /* MOVE.l -(a2),d0 */
    struct Window *win = *wobj;
    struct AppWindow *aw;

    if (!libflag)
        return BB_FALSE;
    if (appwindows[n]) {
        RemoveAppWindow((struct AppWindow *)appwindows[n]);
        appwindows[n] = 0;
    }
    aw = AddAppWindowA(n, 0, win, msgport, NULL);
    if (!aw)
        return BB_FALSE;
    appwindows[n] = (ULONG)aw;
    return BB_TRUE;
}
BLITZ_THUNK(addwindow, addwindow_c, "d1/a0-a1");

static BB_BOOL BLITZ_ROUTINE delwindow_c(LONG n BL_REG(d0))
{
    if (!libflag)
        return BB_FALSE;
    RemoveAppWindow((struct AppWindow *)appwindows[n]);    /* BUG(orig): also NULL (#523) */
    appwindows[n] = 0;
    return BB_TRUE;
}
BLITZ_THUNK(delwindow, delwindow_c, "d1/a0-a1");
