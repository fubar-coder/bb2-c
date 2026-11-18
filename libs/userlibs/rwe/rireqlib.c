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
 * No assembler besides the thunks.  ReqFileRequest(title$,flags) sets
 * the flags for later calls too, like ReqFlags: documented in
 * RIReqLib.guide, kept (#789).
 *
 * Fixed (intended deviations):
 *   - ReqFileRequest copied the path with its 0 and left a3 behind the
 *     0, so a string expression went on one byte too far; a3 now ends on
 *     the 0.  On cancel d0 = 0 and a3 stays (with a title: at the start
 *     of the title string), as before (#784).
 *   - ReqColour passes its argument as DefColour; the original ignored
 *     it, so the requester always started on colour 0 (#786).
 *   - The colour requester structure is req.library's
 *     ExtendedColorRequester (REQBASE.BB2): DefColour a long at 0, the
 *     window at 4, 5 longs RFU.  The original's had DefColour as a word
 *     at 0 and the window at 2, so the requester read the colour shifted
 *     into the high word (with the window's high word below it) and a
 *     window pointer made of the window's low word and 0; #786 takes
 *     effect only with this (#1091).
 *   - The default flags are caching, info gadget and cache purge ($94,
 *     the masks RIReqLib.guide lists); the original ORed the bit numbers
 *     (#FRQCACHINGB|#FRQINFOGADGETB|#FRQCACHEPURGEB = 2|4|7 = 7: bits
 *     0-2, extended select and caching) (#787).
 *   - Without req.library (not installed, or init's AllocMem failed,
 *     which closes it again) ReqFileRequest returns "" as on cancel and
 *     ReqColour 65535 ($FFFF, a word), what req.library's ColorRequester
 *     returns on cancel; the original checked no base and called through
 *     a6 = 0 (#788).  ReqFileLoc returns 0, as before.
 *   - init puts the four buffers into FrStruct's title, directory, file
 *     and path fields (finit clears them), as the first version ReqLib.bb
 *     had them: the original stored them only in its own four longs
 *     (frq_Title ... frq_PathName), so the requester got 0 for them and
 *     ReqFileRequest returned the path buffer that only the program
 *     (through ReqFileLoc) wrote, never the user's choice (#785).
 *   - FrStruct is req.library's whole file requester structure, 356
 *     bytes (frq_SIZEOF); the original's ended at 162, so the fields the
 *     requester keeps behind that (frq_Show from 152, the cursor
 *     positions, frq_Memory, frq_Memory2, frq_Lock, its directory
 *     buffer) were written over the library's data behind it: in the
 *     original the buffer pointers, the colour requester structure and
 *     the library base (#1088).
 *   - A title of ReqFileRequest(title$[,flags]) is for that call only;
 *     a later call without a title has the empty title buffer again.
 *     The original kept the pointer to the title string, which it gives
 *     up to the string work area, for later calls without one (#789).
 *   - init clears FrStruct's window together with the colour requester's
 *     (routput).  The original cleared only routput: FrStruct is library
 *     data, so a second run of the program with the library still loaded
 *     (from the editor) gave the file requester the first run's window,
 *     closed at its end, until its own ReqOutput (#1090).
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
BLITZ_LIBS(LIB_windowslib, BL_IA0|BL_PD0);
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

/* the FileRequester structure of req.library (frq_SIZEOF 356 bytes,
   RIReqLib.guide "STRUCTURE AFileRequester" with REQBASE.BB2's DSIZE,
   FCHARS, WILDLENGTH): the fields the library fills, then the rest 0 -
   frq_Hide at 120, frq_Show at 152, the cursor positions at 184,
   frq_Memory, frq_Memory2, frq_Lock at 200, 204, 208 and the private
   directory buffer at 212 are the requester's own, kept over calls.
   Fixed (#1088): the original's structure ended at 162 (34 bytes and 32
   longs), so the requester wrote those fields over the data behind it. */
#define FRQ_SIZEOF 356
struct BL_PACKED frstruct {
    UWORD version;                                      /* 0 */
    STRPTR title, dir, file, pathname;                  /* 2 6 10 14 */
    APTR window;                                        /* 18 */
    UWORD maxextsel;                                    /* 22 */
    UWORD numlines, numcolumns, devcolumns;             /* 24 26 28 */
    ULONG flags;                                        /* 30 */
    UBYTE rest[FRQ_SIZEOF - 34];                        /* 34 */
};
_Static_assert(sizeof(struct frstruct) == FRQ_SIZEOF, "frq_SIZEOF");

/* #frq_VersionNumber=2, #frq_Window=0, #frq_numlines=20,
   #frq_numcolumns=28, #frq_devcolumns=12, #frq_Flags: caching, info
   gadget and cache purge (fixed, #787: the original ORed the bit
   numbers, 7) */
static struct frstruct FrStruct = {
    2, 0, 0, 0, 0, 0, 0, 20, 28, 12, (1 << 2) | (1 << 4) | (1 << 7), { 0 }
};

/* frq_Title, frq_Dir, frq_File, frq_PathName: the buffers of the block */
static UBYTE *frq_Title, *frq_Dir, *frq_File, *frq_PathName;

static APTR routput;                                    /* the window */

/* the colour requester structure of req.library (REQBASE.BB2
   ExtendedColorRequester, 28 bytes): DefColour a long at 0, the window
   at 4, 5 longs RFU.  Fixed (#1091): the original's had DefColour as a
   word and the window at 2 (and 6 longs RFU), so the requester read the
   colour and the window pointer's high word as DefColour and a mangled
   window pointer. */
static struct BL_PACKED {
    ULONG defcolour;                                    /* 0 */
    APTR winptr;                                        /* 4 */
    ULONG rfu[5];                                       /* 8 */
} ecr;
_Static_assert(sizeof ecr == 28, "ExtendedColorRequester");

static struct Library *reqbase;
static const char reqname[] = "req.library";

/**
 * @brief init: open req.library (any version) and allocate the buffers.
 *
 * One block of 940 bytes (MEMF_PUBLIC|MEMF_CLEAR): title (320),
 * directory (150), file (150) and path (320); their addresses go to
 * frq_Title ... frq_PathName and into FrStruct's title, directory, file
 * and path fields (fixed, #785: the original never set the fields; its
 * first version, ReqLib.bb, had them).  No output window: FrStruct's
 * window and the colour requester's are cleared (fixed, #1090: the
 * original cleared only the colour requester's, so a second run from the
 * editor gave the file requester the first run's closed window until its
 * own ReqOutput).  If the AllocMem fails, the library is closed again
 * (_oops2).
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
    FrStruct.title = frq_Title;                         /* fixed (#785) */
    FrStruct.dir = frq_Dir;
    FrStruct.file = frq_File;
    FrStruct.pathname = frq_PathName;
    FrStruct.window = NULL;                             /* fixed (#1090) */
    routput = NULL;
    return (ULONG)frq_PathName;
}

/**
 * @brief finit: close req.library and free the buffers (if it was open).
 *
 * FrStruct's buffer fields are cleared again (#785).
 */
static void BLITZ_ROUTINE libfinit_c(void)
{
    if (!reqbase)
        return;
    CloseLibrary(reqbase);
    reqbase = NULL;
    FreeMem(frq_Title, BUFSIZE);
    frq_Title = NULL;
    FrStruct.title = FrStruct.dir = FrStruct.file = FrStruct.pathname = NULL;
}

/**
 * @brief ReqFlags flags: the flags of the file requester (FrStruct's
 * flags).
 *
 * The default is $94: caching, info gadget and cache purge (fixed, #787).
 * @param flags  d0: the req.library file requester flags
 */
static void BLITZ_ROUTINE reqflags_c(ULONG flags BL_REG(d0))
{
    FrStruct.flags = flags;
}

/**
 * @brief ReqFileLoc: the address of the path buffer.
 *
 * The requester writes the chosen path there, which ReqFileRequest
 * returns; a program may write a default path there before the call.
 * 0 without the buffers (#788).
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
 * FileRequester(FrStruct); the path buffer is copied to a3, a3 ends on
 * the terminating 0 (fixed, #784: the original left it behind the 0).
 * FrStruct holds the buffers (fixed, #785: the original's had none, so
 * the path was what the program wrote with ReqFileLoc, not the
 * requester's).  Fixed (#788): without req.library it
 * returns "" as on cancel; the original called through a6 = 0.
 * @return d0: the length of the path; 0 on cancel (a3 unchanged)
 */
static ULONG BLITZ_ROUTINE requestfile_c(void)
{
    struct bl_regs r = { .a0 = (ULONG)&FrStruct };
    const UBYTE *s;
    UWORD n = 0;

    if (!reqbase)                                       /* fixed (#788) */
        return 0;
    if (!BLITZ_BASEJSR(reqbase, LVO_FileRequester, r))
        return 0;
    s = frq_PathName;
    do
        n++;                                            /* ADDQ.w #1,d0 */
    while ((*blitz_string++ = *s++) != 0);
    blitz_string--;                                     /* fixed (#784): a3 on the 0 */
    return (ULONG)n - 1;
}

/**
 * @brief ReqFileRequest(title$): the file requester with a title.
 *
 * The title string is given up (a3 back by its length + 1), then as
 * requestfile_c.  The title is for this call only: afterwards the
 * title field points to the library's (empty) title buffer again
 * (fixed, #789: the original kept the pointer to the given-up string for
 * later calls without a title).  On cancel a3 stays at the start of the
 * title string.
 * @param title  d0: the title
 * @param lens   a2: the argument stack, -4(a2) the length of title$
 * @return d0: the length of the path; 0 on cancel
 */
static ULONG BLITZ_ROUTINE requestfiletitle_c(STRPTR title BL_REG(d0),
                                              const ULONG *lens BL_REG(a2))
{
    ULONG n;

    blitz_string -= lens[-1] + 1;
    FrStruct.title = title;
    n = requestfile_c();
    FrStruct.title = frq_Title;                         /* fixed (#789) */
    return n;
}

/**
 * @brief ReqFileRequest(title$,flags): the file requester with a title
 * and flags.
 *
 * The flags are set for later calls too, like ReqFlags, as
 * RIReqLib.guide documents it ("If this is omitted then the last flag
 * setting is used"; not a defect, #789).
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
 * Fixed (#786): the argument is the default colour (DefColour); the
 * original ignored it.  Fixed (#1091): DefColour is a long at 0 and the
 * window at 4, as req.library reads them; the original's word at 0 and
 * window at 2 gave the requester another colour and a mangled window
 * (so #786 alone had no effect with the real library).  Fixed (#788):
 * without req.library it returns $FFFF, what ColorRequester returns on
 * cancel; the original called through a6 = 0.
 * @param colour  d0: the colour the requester starts on
 * @return d0: what ColorRequester returns ($FFFF on cancel); without
 *         req.library $FFFF
 */
static ULONG BLITZ_ROUTINE colourreq_c(ULONG colour BL_REG(d0))
{
    struct bl_regs r = { .a0 = (ULONG)&ecr };

    if (!reqbase)                                       /* fixed (#788) */
        return 0xffff;                                  /* as on cancel */
    ecr.defcolour = colour;                             /* fixed (#786, #1091) */
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
