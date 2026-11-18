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
 * AppEventIDCMP (Dc $a000).  AddAppIconA takes a3/a4 (the fixed registers
 * of the Blitz program): the libraries' safe form of the NDK macro keeps
 * them (cmake/ndk-calls.cmake).
 *
 * BUG(orig), kept:
 *   - init: if a library does not open, the port is deleted and the block
 *     freed, but the libraries already opened stay open; AppMsgPort then
 *     still returns the deleted port (#526).
 *
 * Fixed (intended deviation, #518): AddAppMenu returned the AppMenuItem
 * address, not -1 (the MOVEQ #-1 behind the RTS was never reached); it
 * returns -1 now, as AddAppWindow and AddAppIcon.
 * Fixed (intended deviation, #517): AddAppIcon on a used id removed the
 * old AppIcon and freed its DiskObject but left the old AppIcon in its
 * slot if AddAppIconA failed (a later DelAppIcon or finit removed it a
 * second time), and the new DiskObject stayed in its slot on failure;
 * the slot is cleared after RemoveAppIcon now, and on failure the new
 * DiskObject is freed and its slot cleared.
 * Fixed (intended deviation, #524): finit cleared the table slot at byte
 * offset index*4/4 instead of index*4 (ASR.l #2 before the store): a long
 * store at an odd address for the slots 1, 3 ... (an address error on a
 * 68000); it clears the slot itself now.
 * Fixed (intended deviation, #525): finit replied to the messages queued
 * at the port only if a message was current (the last AppEvent) - the
 * flag for the other case was never set - so they stayed unreplied when
 * the port was deleted; it replies to the current message and the whole
 * queue now.
 * Fixed (intended deviation, #521): NextAppFile/AppFile unlocked the lock
 * of the WBArg (ChangeMode + UnLock - Workbench's own lock) and cleared it
 * in the message unless the path ended with ':', and the ':' test read
 * the byte before the string for an empty path; the lock stays now, and
 * '/' is appended only behind a path that is not empty and does not end
 * with ':'.
 * Fixed (intended deviation, #523): DelAppWindow called RemoveAppWindow
 * also for an empty slot (NULL) and returned -1; it returns 0 now without
 * a call, as DelAppMenu and DelAppIcon.  DelAppIcon calls FreeDiskObject
 * only for a DiskObject (the original also for an empty slot).
 * Fixed (intended deviation, #520): AppFile(n) with n <= 0 read the WBArg
 * before the list (the index was checked against its upper bound only);
 * it returns an empty string now.
 * Fixed (intended deviation, #522): AppLock read the first WBArg without
 * looking at am_NumArgs; a message without arguments now has lock 0.
 * Fixed (intended deviation, #519): NextAppFile/AppFile copied the file
 * name with its 0 and left a3 behind the 0, so a string expression went
 * on one byte too far; a3 now ends on the 0.
 * Fixed (intended deviation, #516): AddAppIcon passed the DiskObject in a3
 * and the tag list (0) in a4 to AddAppIconA and restored neither (a3 =
 * the DiskObject, a4, the locals base of the Blitz program, = 0); a4 now
 * stays, and a3 ends on text$, its first string argument, giving back the
 * space of the arguments, which the compiler does not take back after a
 * function with a numeric result (#846).
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
void appeventidcmp(void), riapplib_libinit(void), riapplib_libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#riapplib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_riapplib, init, BL_NORET, finit, 0);

/* all code addresses odd (`label+1`): Amiga mode only */
BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(BL_PUSH | BL_LONG);
BLITZ_LIBS(LIB_windowslib, BL_IA0|BL_PD0);
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

BLITZ_NULLSUB_AT(finit, riapplib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, riapplib_libinit, 0, 0);

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

/**
 * @brief AppEventIDCMP: the value AppEvent returns for a message,
 * $800000 (inline: MOVE.l #$800000,d0; also in Blitz mode).
 */
BLITZ_INLINE(appeventidcmp, "move.l #0x800000,d0");

/* ---------------------------------------------------------------- init */

/**
 * @brief init: the message port, the block (768 bytes, MEMF_CLEAR) and
 * workbench, dos and icon.library V37.
 *
 * Sets libflag -1 when all is there, 0 otherwise (the commands then do
 * nothing).  BUG(orig) (#526): the libraries opened before a failing one
 * stay open, and AppMsgPort still returns the deleted port.
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    struct appblock *block;

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
BLITZ_THUNK(riapplib_libinit, libinit_c, "d0-d1/a0-a1");

/* delapp: remove every object of a table of 16 and clear its slot;
   fixed (#524): the original cleared the long at byte offset index*4/4
   (an odd address for odd indexes) */
static void delapp(ULONG *table, int kind)
{
    int i;

    for (i = 0; i < 16; i++) {
        ULONG obj = table[i];
        if (obj) {
            switch (kind) {
            case 0: RemoveAppMenuItem((struct AppMenuItem *)obj); break;
            case 1: RemoveAppWindow((struct AppWindow *)obj); break;
            case 2: RemoveAppIcon((struct AppIcon *)obj); break;
            default: FreeDiskObject((struct DiskObject *)obj); break;
            }
            table[i] = 0;               /* fixed (#524): the slot */
        }
    }
}

/**
 * @brief finit: replies the messages, removes every App object, deletes
 * the port, frees the block and closes the libraries.
 *
 * Fixed (#525): the current message and then the whole queue are
 * replied (the original replied to the queue only if a message was
 * current).  Fixed (#524): the slot of each object is cleared (the
 * original cleared the long at byte offset index*4/4).
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    struct Message *m;

    if (!libflag)
        return;
    /* fixed (#525): the queue also without a current message */
    if (msg)
        ReplyMsg(&msg->am_Message);
    while ((m = GetMsg(msgport)) != NULL)
        ReplyMsg(m);
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
BLITZ_THUNK(riapplib_libfinit, libfinit_c, "d0-d1/a0-a1");

/* ----------------------------------------------------- message functions */

/**
 * @brief AppMsgPort: the library's message port.
 * @return d0: the MsgPort (also after a failed init, BUG(orig) (#526))
 */
static ULONG BLITZ_ROUTINE appmsgport_c(void)
{
    return (ULONG)msgport;
}
BLITZ_THUNK(appmsgport, appmsgport_c, "d1/a0-a1");

/**
 * @brief AppMessage: the current AppMessage (of the last AppEvent).
 * @return d0: the message, 0 if none
 */
static ULONG BLITZ_ROUTINE appmessage_c(void)
{
    if (!libflag || !msg)
        return 0;
    return (ULONG)msg;
}
BLITZ_THUNK(appmessage, appmessage_c, "d1/a0-a1");

/**
 * @brief AppLock: the lock of the current message's first argument.
 *
 * Fixed (#522): a message without arguments has no lock (the original
 * read the first WBArg without looking at am_NumArgs).
 * @return d0: the lock, 0 if there is no message or it has no arguments
 */
static ULONG BLITZ_ROUTINE applock_c(void)
{
    if (!libflag || !msg || numargs <= 0)       /* fixed (#522) */
        return 0;
    return (ULONG)msg->am_ArgList->wa_Lock;
}
BLITZ_THUNK(applock, applock_c, "d1/a0-a1");

/**
 * @brief AppEvent: replies to the current message and gets the next one.
 *
 * Its id, number of arguments and type are kept for AppEventID,
 * AppNumFiles and AppEventType; NextAppFile starts again at the first
 * argument.
 * @return d0: $800000 (AppEventIDCMP) if there is a message, else 0
 */
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

/**
 * @brief AppNumFiles: the number of arguments (am_NumArgs) of the current
 * message.
 * @return d0: the number, 0 if there is no message
 */
static ULONG BLITZ_ROUTINE appfiles_c(void)
{
    if (!libflag || !msg)
        return 0;
    return numargs;
}
BLITZ_THUNK(appfiles, appfiles_c, "d1/a0-a1");

/**
 * @brief AppEventID: the id (am_ID) of the current message.
 * @return d0: the id, 0 if there is no message
 */
static ULONG BLITZ_ROUTINE appeventid_c(void)
{
    if (!libflag || !msg)
        return 0;
    return msgid;
}
BLITZ_THUNK(appeventid, appeventid_c, "d1/a0-a1");

/**
 * @brief AppEventType: the kind of the current message.
 * @return d0: 1 AppWindow (am_Type 7), 2 AppIcon (8), 3 AppMenuItem (9),
 *         0 otherwise or if there is no message
 */
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

/**
 * @brief NextAppFile: the full path of the current message's next
 * argument, at a3.
 *
 * NameFromLock of the argument's lock, '/' unless the path ends with
 * ':', then the name.  Fixed (#521): the lock (Workbench's) stays in the
 * message (the original unlocked and cleared it unless the path ended
 * with ':'), and an empty path gets no '/' (the original's test read the
 * byte before the string).  a3 ends on the 0 (fixed, #519: the original
 * left it behind the 0).
 * @return d0: the length, 0 if there is no message, no argument left or
 *         NameFromLock fails
 */
static ULONG BLITZ_ROUTINE appfile_c(void)
{
    struct WBArg *arg;
    UBYTE *start, *s;

    if (!libflag || !msg)
        return 0;
    if (argnum < 0 || argnum >= numargs)        /* fixed (#520): argnum < 0 */
        return 0;
    arg = msg->am_ArgList + argnum++;
    if (!NameFromLock(arg->wa_Lock, (STRPTR)arglist, 255))
        return 0;
    start = blitz_string;
    s = arglist;
    while ((*blitz_string++ = *s++) != 0)
        ;
    blitz_string--;
    /* fixed (#521): the lock stays; no '/' behind an empty path */
    if (blitz_string != start && blitz_string[-1] != ':')
        *blitz_string++ = '/';
    s = (UBYTE *)arg->wa_Name;
    while ((*blitz_string++ = *s++) != 0)
        ;
    blitz_string--;                             /* fixed (#519): a3 on the 0 */
    return blitz_string - start;
}
BLITZ_THUNK(appfile, appfile_c, "d1/a0-a1");

/**
 * @brief AppFile(num): the full path of argument num (1 = the first), as
 * NextAppFile; NextAppFile's position stays.
 *
 * Fixed (#520): num <= 0 is out of range, an empty string (the original
 * read the WBArg before the list).
 * @param num  d0: the argument number
 * @return d0: the length, as NextAppFile
 */
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

/**
 * @brief DelAppMenu(id): removes the App menu item with this id.
 *
 * The id indexes the table unchecked.
 * @param id  d0: the id
 * @return d0: BB_TRUE, BB_FALSE if there is none (or init failed)
 */
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

/**
 * @brief AddAppMenu(id,text$): adds an item to Workbench's Tools menu.
 *
 * An item with the same id is removed first.  The id indexes the table
 * unchecked.
 * @param id    d0: the id
 * @param text  d1: the item's text
 * @return d0: BB_TRUE, BB_FALSE on failure (fixed, #518: the original
 *         returned the AppMenuItem)
 */
static BB_BOOL BLITZ_ROUTINE addmenu_c(LONG id BL_REG(d0), STRPTR text BL_REG(d1))
{
    struct AppMenuItem *item;

    if (!libflag)
        return BB_FALSE;
    if (appmenus[id])
        delmenu_c(id);
    item = AddAppMenuItemA(id, 0, text, msgport, NULL);
    if (!item)
        return BB_FALSE;
    appmenus[id] = (ULONG)item;
    return BB_TRUE;                     /* fixed (#518): not the item */
}
BLITZ_THUNK(addmenu, addmenu_c, "d1/a0-a1");

/* ----------------------------------------------------------- App icons */

/**
 * @brief AddAppIcon (after the libflag test): the icon from
 * GetDiskObject(filename$) at iconx/icony, added as an App icon.
 *
 * Fixed (#517): the slot of the old AppIcon is cleared once it is
 * removed, and a new DiskObject that AddAppIconA did not take is freed
 * (the original left both in their slots).
 * Fixed (#516): a3 and a4 are kept around AddAppIconA.
 * @param id    d0: the id
 * @param text  d1: the icon's text
 * @param name  d2: the file name of the icon (without .info)
 * @return d0: BB_TRUE, BB_FALSE on failure
 */
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
        appicons[id] = 0;               /* fixed (#517) */
        if (disklist[id]) {
            FreeDiskObject(disklist[id]);
            disklist[id] = NULL;
        }
    }
    disklist[id] = dobj;
    /* AddAppIconA takes the DiskObject in a3 and the tag list in a4, the
       Blitz program's string pointer and locals base.  Fixed (#516): both
       come back as they were (the libraries' safe form of the NDK macro,
       cmake/ndk-calls.cmake); the original left a3 = the DiskObject and
       a4 = 0. */
    icon = AddAppIconA(id, 0, text, msgport, 0, dobj, NULL);
    if (!icon) {
        FreeDiskObject(dobj);           /* fixed (#517) */
        disklist[id] = NULL;
        return BB_FALSE;
    }
    appicons[id] = (ULONG)icon;
    return BB_TRUE;
}

/**
 * @brief AddAppIcon(id,text$,filename$): an App icon at a position of
 * Workbench's choice (NO_ICON_POSITION).
 *
 * Fixed (#516): a3 = text$ afterwards on every path (the original: the
 * DiskObject, or behind the arguments).
 * @param id    d0: the id
 * @param text  d1: the icon's text
 * @param name  d2: the file name of the icon
 * @return d0: BB_TRUE, BB_FALSE on failure
 */
static BB_BOOL BLITZ_ROUTINE addicon2_c(LONG id BL_REG(d0), STRPTR text BL_REG(d1),
                                        STRPTR name BL_REG(d2))
{
    BB_BOOL r;

    iconx = icony = NO_POS;
    r = libflag ? addicon_c(id, text, name) : BB_FALSE;
    blitz_string = (UBYTE *)text;       /* fixed (#516): a3 = text$ */
    return r;
}
BLITZ_THUNK(addicon, addicon2_c, "d1/a0-a1");

/**
 * @brief AddAppIcon(id,text$,filename$,x,y): an App icon at x,y.
 *
 * Fixed (#516): a3 = text$ afterwards, as above.
 * @param id    d0: the id
 * @param text  d1: the icon's text
 * @param name  d2: the file name of the icon
 * @param x     d3: x
 * @param y     d4: y
 * @return d0: BB_TRUE, BB_FALSE on failure
 */
static BB_BOOL BLITZ_ROUTINE addicon3_c(LONG id BL_REG(d0), STRPTR text BL_REG(d1),
                                        STRPTR name BL_REG(d2), ULONG x BL_REG(d3),
                                        ULONG y BL_REG(d4))
{
    BB_BOOL r;

    iconx = x;
    icony = y;
    r = libflag ? addicon_c(id, text, name) : BB_FALSE;
    blitz_string = (UBYTE *)text;       /* fixed (#516): a3 = text$ */
    return r;
}
BLITZ_THUNK(addicon3, addicon3_c, "d1/a0-a1");

/**
 * @brief DelAppIcon(id): removes the App icon and frees its DiskObject.
 *
 * Fixed (#523): FreeDiskObject only for a DiskObject.
 * @param id  d0: the id
 * @return d0: BB_TRUE, BB_FALSE if there is none (or init failed)
 */
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
    if (disklist[id]) {                 /* fixed (#523): not for NULL */
        FreeDiskObject(disklist[id]);
        disklist[id] = NULL;
    }
    return BB_TRUE;
}
BLITZ_THUNK(delicon, delicon_c, "d1/a0-a1");

/* --------------------------------------------------------- App windows */

/**
 * @brief AddAppWindow(windownumber): makes a window an App window (id =
 * the window number).
 *
 * An App window with the same number is removed first.  DIFF: a2 stays
 * unchanged.
 * @param wobj  a0: the windowslib object (first long = the Intuition
 *              window)
 * @param args  a2: the argument stack, -4(a2) = the number (#push)
 * @return d0: BB_TRUE, BB_FALSE on failure
 */
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

/**
 * @brief DelAppWindow(windownumber): removes the App window.
 *
 * Fixed (#523): an empty slot is BB_FALSE without a call (the original
 * called RemoveAppWindow(NULL) and returned BB_TRUE).
 * @param n  d0: the window number
 * @return d0: BB_TRUE, BB_FALSE if there is none (or init failed)
 */
static BB_BOOL BLITZ_ROUTINE delwindow_c(LONG n BL_REG(d0))
{
    if (!libflag || !appwindows[n])     /* fixed (#523): the empty slot */
        return BB_FALSE;
    RemoveAppWindow((struct AppWindow *)appwindows[n]);
    appwindows[n] = 0;
    return BB_TRUE;
}
BLITZ_THUNK(delwindow, delwindow_c, "d1/a0-a1");
