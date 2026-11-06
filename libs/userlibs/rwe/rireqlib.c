/*
 * rireqlib - RWE userlib 79 (Reflective Images): the file and colour
 * requesters of req.library - ReqFileRequest, ReqFileLoc, ReqOutput,
 * ReqFlags, ReqColour.
 *
 * C translation of rwelibsrc/reqlib/ReqLib2.bb (ReqLib, ReqLib2 and
 * ReqLib3 have the same declarations; the reference build of ReqLib2 is
 * the one that equals the binary, docs/rwelibs/README.md 3.1); reference
 * OL RIREQLIB.OBJ.  Notes: docs/rwelibs/port/rireqlib.md.
 *
 * init opens req.library (any version) and allocates one block of 940
 * bytes (MEMF_PUBLIC|MEMF_CLEAR) for the buffers title (320), directory
 * (150), file (150) and path (320); finit closes the library and frees
 * the block.  ReqFileRequest (Amiga mode only) calls FileRequester with
 * the structure FrStruct and copies the path buffer to the result;
 * ReqColour calls ColorRequester.  ReqOutput sets the window of both
 * (a0 = the window object, !libs #windowslib,#ia0|#pd0), ReqFlags the
 * flags of the file requester.
 *
 * No assembler besides the thunks.
 *
 * BUG(orig):
 *   - #frq_Flags = #FRQCACHINGB|#FRQINFOGADGETB|#FRQCACHEPURGEB ORs bit
 *     numbers (2|4|7 = 7): the default flags are bits 0-2 instead of
 *     caching, info gadget and cache purge (#787).
 *   - init stores the buffer addresses in its own four longs (frq_Title
 *     ... frq_PathName), never in FrStruct's Directory/File/PathName
 *     fields: the requester gets 0 for them, and ReqFileRequest copies
 *     the cleared path buffer, which nothing but the program (through
 *     ReqFileLoc) writes (#785).
 *   - No routine checks the library base: without req.library (or after
 *     init's AllocMem failed, which closes it again) ReqFileRequest and
 *     ReqColour call through a6 = 0, ReqFileLoc returns 0 (#788).
 *   - ReqFileRequest leaves a3 behind the terminating 0 of the result
 *     (d0 = length is right); on cancel d0 = 0 and a3 stays (with a
 *     title: at the start of the title string) (#784).
 *   - ReqColour ignores its argument: DefColour stays 0 (#786).
 *   - ReqFileRequest(title$,flags) changes the flags for good (like
 *     ReqFlags), and a title stays for later calls without one (#789).
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks, below) */
void _requestfile(void), _requestfiletitle(void), _requestfiletitleflags(void);
void _file(void), _reqoutput(void), _reqflags(void), _colourreq(void);
void rireqlib_libinit(void), rireqlib_libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#rireqlib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rireqlib, init, BL_NORET, finit, 0);

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_requestfile), 0, 0);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_requestfiletitle), 0, 0);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_requestfiletitleflags), 0, 0);
BLITZ_NAME("ReqFileRequest", "([title$][,flags])");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_file, 0, 0);
BLITZ_NAME("ReqFileLoc", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_windowslib, BL_LA(0) | BL_ITEM | BL_PD(0));
BLITZ_SUBS(_reqoutput, 0, 0);
BLITZ_NAME("ReqOutput", "windowptr");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_reqflags, 0, 0);
BLITZ_NAME("ReqFlags", "flags");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_colourreq, 0, 0);
BLITZ_NAME("ReqColour", "(colour)");

BLITZ_NULLSUB_AT(finit, rireqlib_libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, rireqlib_libinit, 0, 0);

BLITZ_LIBFIN();

/* req.library */
#define LVO_FileRequester  (-0x54)
#define LVO_ColorRequester (-0xc0)

#define BUFSIZE (320 + 320 + 150 + 150)

/* the FileRequester structure of req.library, as far as the original
   fills it (34 bytes, then 32 longs 0) */
struct BL_PACKED frstruct {
    UWORD version;                                      /* 0 */
    STRPTR title, dir, file, pathname;                  /* 2 6 10 14 */
    APTR window;                                        /* 18 */
    UWORD maxextsel;                                    /* 22 */
    UWORD numlines, numcolumns, devcolumns;             /* 24 26 28 */
    ULONG flags;                                        /* 30 */
    ULONG rest[32];                                     /* 34 */
};

/* #frq_VersionNumber=2, #frq_Window=0, #frq_numlines=20,
   #frq_numcolumns=28, #frq_devcolumns=12, #frq_Flags (BUG(orig): 7, #787) */
static struct frstruct FrStruct = {
    2, 0, 0, 0, 0, 0, 0, 20, 28, 12, 2 | 4 | 7, { 0 }
};

/* frq_Title, frq_Dir, frq_File, frq_PathName: the buffers of the block */
static UBYTE *frq_Title, *frq_Dir, *frq_File, *frq_PathName;

static APTR routput;                                    /* the window */

/* the colour requester structure: DefColour, WinPtr, 6 longs RFU */
static struct BL_PACKED {
    UWORD defcolour;
    APTR winptr;
    ULONG rfu[6];
} ecr;

static struct Library *reqbase;
static const char reqname[] = "req.library";

/**
 * @brief init: open req.library (any version) and allocate the buffers.
 *
 * One block of 940 bytes (MEMF_PUBLIC|MEMF_CLEAR): title (320),
 * directory (150), file (150) and path (320); their addresses go to
 * frq_Title ... frq_PathName, never into FrStruct (BUG(orig) #785).  If
 * the AllocMem fails, the library is closed again (_oops2).
 * @param incoming  d0: returned unchanged if the library is already open
 * @return d0: the path buffer; 0 if the library or the memory is missing
 */
static ULONG BLITZ_ROUTINE libinit_c(ULONG incoming BL_REG(d0))
{
    UBYTE *mem;

    if (reqbase)
        return incoming;                                /* oops */
    reqbase = OpenLibrary((CONST_STRPTR)reqname, 0);
    if (!reqbase)
        return 0;
    mem = AllocMem(BUFSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!mem) {                                         /* _oops2 */
        CloseLibrary(reqbase);
        reqbase = NULL;
        return 0;
    }
    frq_Title = mem;
    frq_Dir = mem + 320;
    frq_File = mem + 320 + 150;
    frq_PathName = mem + 320 + 150 + 150;
    routput = NULL;
    return (ULONG)frq_PathName;
}

/**
 * @brief finit: close req.library and free the buffers (if it was open).
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    if (!reqbase)
        return;
    CloseLibrary(reqbase);
    reqbase = NULL;
    FreeMem(frq_Title, BUFSIZE);
    frq_Title = NULL;
}

/**
 * @brief ReqFlags flags: the flags of the file requester (FrStruct's
 * flags).
 *
 * The default is BUG(orig) (#787): bit numbers ORed, 7.
 * @param flags  d0: the req.library file requester flags
 */
static void BLITZ_ROUTINE reqflags_c(ULONG flags BL_REG(d0))
{
    FrStruct.flags = flags;
}

/**
 * @brief ReqFileLoc: the address of the path buffer.
 *
 * The program may write the path there (it is what ReqFileRequest
 * returns, BUG(orig) #785).  0 without the buffers (#788).
 * @return d0 (and a0, the thunk): the path buffer
 */
static ULONG BLITZ_ROUTINE file_c(void)
{
    return (ULONG)frq_PathName;
}

/**
 * @brief ReqFileRequest: the req.library file requester, the path as the
 * result.
 *
 * FileRequester(FrStruct); the path buffer is copied to a3.  BUG(orig):
 * a3 stays behind the terminating 0 (#784); FrStruct has no buffers, so
 * the path is what the program wrote with ReqFileLoc (#785); req.library
 * is not checked (#788).
 * @return d0: the length of the path; 0 on cancel (a3 unchanged)
 */
static ULONG BLITZ_ROUTINE requestfile_c(void)
{
    struct bl_regs r = { .a0 = (ULONG)&FrStruct };
    const UBYTE *s;
    UWORD n = 0;

    if (!BLITZ_BASEJSR(reqbase, LVO_FileRequester, r))
        return 0;
    s = frq_PathName;
    do
        n++;                                            /* ADDQ.w #1,d0 */
    while ((*blitz_string++ = *s++) != 0);
    return (ULONG)n - 1;
}

/**
 * @brief ReqFileRequest(title$): the file requester with a title.
 *
 * The title string is given up (a3 back by its length + 1), then as
 * requestfile_c.  BUG(orig) (#789): the title stays for later calls
 * without one; on cancel a3 stays at the start of the title string
 * (#784).
 * @param title  d0: the title
 * @param lens   a2: the argument stack, -4(a2) the length of title$
 * @return d0: the length of the path; 0 on cancel
 */
static ULONG BLITZ_ROUTINE requestfiletitle_c(STRPTR title BL_REG(d0),
                                              const ULONG *lens BL_REG(a2))
{
    blitz_string -= lens[-1] + 1;
    FrStruct.title = title;
    return requestfile_c();
}

/**
 * @brief ReqFileRequest(title$,flags): the file requester with a title
 * and flags.
 *
 * BUG(orig) (#789): the flags are set for good, like ReqFlags.
 * @param title  d0: the title
 * @param flags  d1: the requester flags
 * @param lens   a2: the argument stack, -4(a2) the length of title$
 * @return d0: the length of the path; 0 on cancel
 */
static ULONG BLITZ_ROUTINE requestfiletitleflags_c(STRPTR title BL_REG(d0),
                                                   ULONG flags BL_REG(d1),
                                                   const ULONG *lens BL_REG(a2))
{
    reqflags_c(flags);
    return requestfiletitle_c(title, lens);
}

/**
 * @brief ReqColour(colour): the req.library colour requester on the
 * ReqOutput window.
 *
 * BUG(orig): the argument (d0) is ignored, DefColour stays 0 (#786);
 * req.library is not checked (#788).
 * @return d0: what ColorRequester returns
 */
static ULONG BLITZ_ROUTINE colourreq_c(void)
{
    struct bl_regs r = { .a0 = (ULONG)&ecr };

    ecr.winptr = routput;
    return BLITZ_BASEJSR(reqbase, LVO_ColorRequester, r);
}

/**
 * @brief ReqOutput window#: the window of both requesters.
 * @param win  a0: the window object (windowslib, #ia0|#pd0), its first
 *             long the Window
 */
static void BLITZ_ROUTINE reqoutput_c(APTR *win BL_REG(a0))
{
    APTR w = *win;

    FrStruct.window = w;
    routput = w;
}

/* Registers: the original changes d0/a0 (ReqFlags, ReqFileLoc,
   ReqOutput only a0/d0), init/finit d0/d1/a0/a1/a6 (system calls), the
   requesters what req.library changes (and d3, a6); the C code keeps
   a2-a6, the thunks d1/a0/a1 where the original keeps them. */
BLITZ_THUNK(rireqlib_libinit, libinit_c, "d1/a0-a1");
BLITZ_THUNK(rireqlib_libfinit, libfinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_reqflags, reqflags_c, "d0-d1/a1");
BLITZ_THUNK_POST(_file, file_c, "d1/a1", "move.l d0,a0");
BLITZ_THUNK(_reqoutput, reqoutput_c, "d1/a1");
BLITZ_THUNK(_requestfile, requestfile_c, "d1/a0-a1");
BLITZ_THUNK(_requestfiletitle, requestfiletitle_c, "d1/a0-a1");
BLITZ_THUNK(_requestfiletitleflags, requestfiletitleflags_c, "d1/a0-a1");
BLITZ_THUNK(_colourreq, colourreq_c, "d1/a0-a1");
