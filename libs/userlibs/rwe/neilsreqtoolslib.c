/*
 * neilsreqtoolslib - userlib 54 (#neilsreqtoolslib): the requesters of
 * reqtools.library V38 - RTEZRequest, RTEZLoadFile, RTEZFontRequest,
 * RTEZScreenModeRequest, RTEZPaletteRequest, RTEZGetLong,
 * RTEZGetLongRange, RTEZGetString, RTEZSaveFile, RTEZSetDefaultDirectory,
 * RTEZPathRequest, RTLockWindow, RTUnlockWindow, RTEZSetPattern,
 * RTEZFreePattern, RTEZFlagsRequest, RTRequest, RTFileRequest,
 * RTEZMultiLoadFile, RTNextPathEntry, RTASyncRequest,
 * RTCheckASyncRequest, RTEndASyncRequest, RTVersion, RTRevision,
 * IsReqToolsActive, RTASyncPaletteRequest, RTCheckASyncPaletteRequest,
 * RTEndASyncPaletteRequest, RTReturnValue.
 *
 * C translation of rwelibsrc/RTLib.bb - "ReqTools V38 for Blitz Basic II
 * V1.70b (c) Neil O'Rourke 20/07/94", with the changes of 1996/97 by
 * Steve Matty (RWE): the autoscroll offset, a3 in RTEZGetString,
 * RTReturnValue, RTEZGetLong's default, the font name not copied.  The
 * source is newer than the CD's NEILSREQTOOLSLIB.OBJ (V1.70b without
 * those changes, docs/rwelibs/README.md 3.2); reference the reference
 * build of the source by blitz2 2.10 (refbuild).  Notes:
 * docs/rwelibs/port/neilsreqtoolslib.md.
 *
 * init opens reqtools.library V38 and allocates seven requesters (file
 * requesters for load, save, path, RTFileRequest and multi-load, a font
 * and a screen mode requester); IsReqToolsActive is -1 if all that
 * worked.  finit frees them, the file list of the last multi-load, and
 * closes the library.  The routines do not check IsReqToolsActive.
 *   file requesters   the result is directory + "/" (unless it ends in
 *                     ":") + file, built from the start of the title
 *                     string (the title argument's space); the file name
 *                     buffer is a 108 byte copy of FileName$ (maxslib)
 *   RTEZGetString     a buffer of MaxChars bytes (memlib), the result at
 *                     the title too
 *   async requesters  RT_ReqHandler: RTCheck... calls rtReqHandlerA (no
 *                     signals), RTEnd... ends it (RTRH_EndRequest)
 *   RTReturnValue     the result of the last reqtools call that stores it
 *
 * Everything is C; reqtools through BLITZ_BASEJSR or, where it takes a2-
 * a4, through RTJSR below (assembler: a3/a4 are the fixed registers of
 * the Blitz program, set for the call and restored after it, as the
 * original's MOVEM does), maxslib and memlib through BLITZ_ALIBJSR.
 *
 * BUG(orig), kept:
 *   - RTEZScreenModeRequest calls rtFontRequestA (-96) with the screen
 *     mode requester instead of rtScreenModeRequestA (-144).
 *   - RTEZFlagsRequest stores the title in RTEZRequest's tag list, not in
 *     its own: its requester always has reqtools' default title (there
 *     the title tag is off until RTEZRequest with a title sets it, and
 *     RTEZRequest resets the title after every call).
 *   - RTEZFreePattern 2 resets the multi-load requester's flags (the
 *     multi-load is 3 in RTEZSetPattern and RTEZSetDefaultDirectory).
 *   - RTEZMultiLoadFile tests only the low byte of the file list address
 *     (CMP.b #0,d0): a list at an address ending in $00 is returned as
 *     the address instead of -1.
 *   - RTEZGetLong returns the value also on cancel (TST.l without a
 *     branch); the value and the default stay for the next call.
 *   - RTEZGetString on cancel does not free its buffer (memlib frees it
 *     at the end of the program); the default string is copied without
 *     a limit into the buffer of MaxChars bytes; the result has no 0.
 *   - RTEZPathRequest writes no 0 behind the path; RTEZSaveFile builds
 *     its result at the title but restores a3 (MOVEM a3-a6), RTEZLoadFile/
 *     RTFileRequest leave a3 behind the 0.  FileName$ is copied with a
 *     length of 108, whatever its own (maxslib copies 108 bytes).
 *   - RTASyncRequest/RTASyncPaletteRequest read the "user port" at 86 of
 *     the window object (not of the Window) and keep its signal bit
 *     shifted by one (LSL.b #1) in a byte nobody reads; the handlers are
 *     called with no signals (sigs is never set).  RTASyncPaletteRequest
 *     and RTEZPaletteRequest do not store RTReturnValue.
 *   - init: when a requester cannot be allocated, the library and the
 *     requesters allocated so far stay (finit does nothing then); finit
 *     does not clear anything (a second finit frees again).  No routine
 *     checks the library: without reqtools.library they call through
 *     a6 = 0 (RTVersion/RTRevision read address $14/$16).
 *
 * DIFF: a2 stays unchanged (the original loads its arguments for
 * reqtools there; docs/libs/cmdlibs-port.md "General").
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks, below) */
void _rtrequest(void), _rtrequestPos(void), _rtrequestXtra(void);
void _rtfreq(void), _rtfontreq(void), _rtsmreq(void), _rtsmreqf(void), _rtpalreq(void);
void _rtgetlong(void), _rtgetlongDef(void), _rtgetlongrng(void), _rtgetlongrngDef(void);
void _rtgetstr(void), _rtgetstrDef(void), _rtsfreq(void), _rtsetdefdir(void);
void _rtpreq(void), _rtlockwin(void), _rtunlockwin(void), _rtsetpat(void);
void _rtfreepat(void), _rtflagsrequest(void), _rtflagsrequestPos(void);
void _rtTagrequest(void), _rtTagsfreq(void), _rtMfreq(void), _rtnxtentry(void);
void _rtasrequest(void), _rtcheckasreq(void), _rtendasreq(void), _rtver(void);
void _rtrev(void), _rtactive(void), _rtasPrequest(void), _rtcheckasPreq(void);
void _rtendasPreq(void), _rtreturnvalue(void), _initreqlib(void), _cleanupreqlib(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#neilsreqtoolslib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_neilsreqtoolslib, init, BL_NORET, finit, 0);

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_STRING, BL_UNKNOWN, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rtrequest, 0, 0);
BLITZ_ARGS(BL_STRING, BL_UNKNOWN, BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtrequestPos, 0, 0);
BLITZ_ARGS(BL_STRING, BL_UNKNOWN, BL_STRING, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtrequestXtra, 0, 0);
BLITZ_NAME("RTEZRequest",
           "(Title$,BodyText$,GadgetText$ [,ReqPosition [,DefaultResponse,Flags]])");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rtfreq, 0, 0);
BLITZ_NAME("RTEZLoadFile", "(Title$,Filename$)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rtfontreq, 0, 0);
BLITZ_NAME("RTEZFontRequest", "(Title$)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rtsmreq, 0, 0);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtsmreqf, 0, 0);
BLITZ_NAME("RTEZScreenModeRequest", "(Title$ [,DisplayFlags])");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtpalreq, 0, 0);
BLITZ_NAME("RTEZPaletteRequest", "(Title$,FirstColour)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rtgetlong, 0, 0);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtgetlongDef, 0, 0);
BLITZ_NAME("RTEZGetLong", "(Title$,BodyText$ [,DefaultValue])");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtgetlongrng, 0, 0);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtgetlongrngDef, 0, 0);
BLITZ_NAME("RTEZGetLongRange", "(Title$,BodyText$,Min,Max [,DefaultValue])");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtgetstr, 0, 0);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rtgetstrDef, 0, 0);
BLITZ_NAME("RTEZGetString", "(Title$,BodyText$,MaxChars [,DefaultString])");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rtsfreq, 0, 0);
BLITZ_NAME("RTEZSaveFile", "(Title$,Filename$)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rtsetdefdir, 0, 0);
BLITZ_NAME("RTEZSetDefaultDirectory", "Requester#,Directory$");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rtpreq, 0, 0);
BLITZ_NAME("RTEZPathRequest", "(Title$)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_windowslib, BL_PD(0) + (BL_LA(1) | BL_ITEM));
BLITZ_SUBS(_rtlockwin, 0, 0);
BLITZ_NAME("RTLockWindow", "(Window#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_windowslib, BL_PD(0) + (BL_LA(1) | BL_ITEM));
BLITZ_SUBS(_rtunlockwin, 0, 0);
BLITZ_NAME("RTUnlockWindow", "Window#,Lock");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG, BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rtsetpat, 0, 0);
BLITZ_NAME("RTEZSetPattern", "Requester#,Pattern$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtfreepat, 0, 0);
BLITZ_NAME("RTEZFreePattern", "Requester#");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtflagsrequest, 0, 0);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_STRING, BL_LONG, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtflagsrequestPos, 0, 0);
BLITZ_NAME("RTEZFlagsRequest",
           "(Title$,BodyText$,GadgetText$,IDCMPFlags [,ReqPosition])");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtTagrequest, 0, 0);
BLITZ_NAME("RTRequest", "(BodyText$,GadgetText$,TagList)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_rtTagsfreq, 0, 0);
BLITZ_NAME("RTFileRequest", "(Title$,FileName$,TagList)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(_rtMfreq, 0, 0);
BLITZ_NAME("RTEZMultiLoadFile", "(Title$)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_rtnxtentry, 0, 0);
BLITZ_NAME("RTNextPathEntry", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_STRING, BL_STRING, BL_STRING);
BLITZ_LIBS(LIB_windowslib, BL_LA(1) | BL_USED);
BLITZ_SUBS(_rtasrequest, 0, 0);
BLITZ_NAME("RTASyncRequest", "(Title$,BodyText$,GadgetText$)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_rtcheckasreq, 0, 0);
BLITZ_NAME("RTCheckASyncRequest", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_rtendasreq, 0, 0);
BLITZ_NAME("RTEndASyncRequest", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_rtver, 0, 0);
BLITZ_NAME("RTVersion", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_rtrev, 0, 0);
BLITZ_NAME("RTRevision", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_rtactive, 0, 0);
BLITZ_NAME("IsReqToolsActive", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_STRING, BL_LONG);
BLITZ_LIBS(LIB_windowslib, BL_LA(1) | BL_USED);
BLITZ_SUBS(_rtasPrequest, 0, 0);
BLITZ_NAME("RTASyncPaletteRequest", "(Title$,FirstColour)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_rtcheckasPreq, 0, 0);
BLITZ_NAME("RTCheckASyncPaletteRequest", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_rtendasPreq, 0, 0);
BLITZ_NAME("RTEndASyncPaletteRequest", "");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_rtreturnvalue, 0, 0);
BLITZ_NAME("RTReturnValue", "-- return value of last RT command");

BLITZ_NULLSUB_AT(init, _initreqlib, 0, 0);
BLITZ_NULLSUB_AT(finit, _cleanupreqlib, 0, 0);

BLITZ_LIBFIN();

static const char version[] __attribute__((used)) =
    "ReqTools V38 for Blitz Basic II V1.70b (c) Neil O'Rourke 20/07/94";

/* ------------------------------------------------------------ reqtools */

#define LVO_rtAllocRequestA      (-30)
#define LVO_rtFreeRequest        (-36)
#define LVO_rtChangeReqAttrA     (-48)
#define LVO_rtFileRequestA       (-54)
#define LVO_rtFreeFileList       (-60)
#define LVO_rtEZRequestA         (-66)
#define LVO_rtGetStringA         (-72)
#define LVO_rtGetLongA           (-78)
#define LVO_rtFontRequestA       (-96)
#define LVO_rtPaletteRequestA    (-102)
#define LVO_rtReqHandlerA        (-108)
#define LVO_rtLockWindow         (-156)
#define LVO_rtUnlockWindow       (-162)

#define RT_TagBase           0x80000000UL
#define RT_IDCMPFlags        (RT_TagBase + 2)
#define RT_ReqPos            (RT_TagBase + 3)
#define RT_ReqHandler        (RT_TagBase + 8)
#define RT_Underscore        (RT_TagBase + 11)
#define RT_LockWindow        (RT_TagBase + 13)
#define RTEZ_ReqTitle        (RT_TagBase + 20)
#define RTEZ_Flags           (RT_TagBase + 22)
#define RTEZ_DefaultResponse (RT_TagBase + 23)
#define RTGL_Min             (RT_TagBase + 30)
#define RTGL_Max             (RT_TagBase + 31)
#define RTGL_TextFmt         (RT_TagBase + 38)
#define RTGL_Flags           RTEZ_Flags
#define RTFI_Flags           (RT_TagBase + 40)
#define RTFI_OkText          (RT_TagBase + 42)
#define RTFI_Dir             (RT_TagBase + 50)
#define RTFI_MatchPat        (RT_TagBase + 51)
#define RTFO_Flags           RTFI_Flags
#define RTSC_Flags           RTFI_Flags
#define RTPA_Color           (RT_TagBase + 70)
#define RTRH_EndRequest      (RT_TagBase + 60)

#define REQPOS_CENTERSCR     2
#define RT_FILEREQ           0
#define RT_FONTREQ           2
#define RT_SCREENMODEREQ     3
#define FREQF_MULTISELECT    1
#define FREQF_SAVE           2
#define FREQF_NOFILES        8
#define FREQF_PATGAD         16
#define FREQF_STYLE          (1 << 10)
#define SCREQF_SIZEGADS      (1 << 13)
#define SCREQF_DEPTHGAD      (1 << 14)
#define GLREQF_CENTERTEXT    4
#define CALL_HANDLER         0x80000000UL

#define RTFR_DIR 16                     /* rtFileRequester.Dir */

/* RTJSR(lvo, d0, a0, a1, a2, a3) / RTJSR4(..., a3, a4): a reqtools call
   that takes a2/a3 (and a4).  a3/a4 are the fixed registers of the Blitz
   program (-ffixed-a3/a4): saved, loaded, restored around the JSR in
   assembler (the original keeps them with MOVEM).  Result d0; reqtools
   may change d0/d1/a0/a1. */
#ifndef BL_CODE_MODEL
#define RTJSR_(lvo, d0v, a0v, a1v, a2v, a3v, a4v, seta4) ({                \
    register ULONG _d0 __asm("d0") = (ULONG)(d0v);                          \
    register ULONG _a0 __asm("a0") = (ULONG)(a0v);                          \
    register ULONG _a1 __asm("a1") = (ULONG)(a1v);                          \
    register ULONG _a2 __asm("a2") = (ULONG)(a2v);                          \
    register struct Library *_a6 __asm("a6") = rtbase;                      \
    ULONG _a3 = (ULONG)(a3v), _a4 = (ULONG)(a4v);                           \
    __asm volatile("movem.l a3-a4,-(sp)\n\t"                                \
                   "move.l  %5,a3\n\t"                                      \
                   seta4                                                    \
                   "jsr     %c7(a6)\n\t"                                    \
                   "movem.l (sp)+,a3-a4"                                    \
        : "+d"(_d0), "+a"(_a0), "+a"(_a1), "+a"(_a2), "+a"(_a6)             \
        : "d"(_a3), "d"(_a4), "i"(lvo)                                      \
        : "d1", "cc", "memory");                                            \
    _d0; })
#define RTJSR(lvo, d0v, a0v, a1v, a2v, a3v) \
    RTJSR_(lvo, d0v, a0v, a1v, a2v, a3v, 0, "")
#define RTJSR4(lvo, d0v, a0v, a1v, a2v, a3v, a4v) \
    RTJSR_(lvo, d0v, a0v, a1v, a2v, a3v, a4v, "move.l  %6,a4\n\t")
#else
#define RTJSR(lvo, d0v, a0v, a1v, a2v, a3v) \
    ((void)(a0v), (void)(a1v), (void)(a2v), (void)(a3v), (ULONG)(d0v))
#define RTJSR4(lvo, d0v, a0v, a1v, a2v, a3v, a4v) \
    ((void)(a4v), RTJSR(lvo, d0v, a0v, a1v, a2v, a3v))
#endif

/* a call with d0/a0/a1 only */
#define RTJSR0(lvo, d0v, a0v, a1v) ({                                       \
    struct bl_regs r_ = { .d0 = (ULONG)(d0v), .a0 = (ULONG)(a0v),          \
                          .a1 = (ULONG)(a1v) };                             \
    BLITZ_BASEJSR(rtbase, lvo, r_); })

/* ---------------------------------------------------------------- data */

static struct Library *rtbase;
static ULONG ReqToolsActive, returnvalue;
static UBYTE *filereq, *fontreq, *smreq, *Sfilereq, *preq, *Tfilereq, *Mfilereq;
static const char rtName[] = "reqtools.library";
static UBYTE fname[108];

/* RTEZRequest */
static ULONG RequestTagList[] = {
    RT_ReqPos, REQPOS_CENTERSCR, RT_LockWindow, (ULONG)-1, RT_Underscore, '_',
    RTEZ_Flags, 0, RTEZ_DefaultResponse, 1, 0, 0, 0, 0
};
#define RequestPos    RequestTagList[1]
#define RequestFlags  RequestTagList[7]
#define RequestDefRes RequestTagList[9]
#define reqTTag       RequestTagList[10]
#define RequestTitle  RequestTagList[11]

/* RTEZLoadFile */
static STRPTR swsFreq;
static UBYTE *filenamestr;
static ULONG freqTagList[] = { RT_LockWindow, (ULONG)-1, RTFI_Flags, 0, 0, 0 };
#define freqFlags freqTagList[3]

/* RTEZFontRequest: a TextAttr */
static ULONG fontTagList[] = { RT_LockWindow, (ULONG)-1, RTFO_Flags, FREQF_STYLE, 0, 0 };
static struct BL_PACKED {
    APTR name;
    UWORD ysize;
    UWORD styleflags;                                   /* style, flags */
} myfont;

/* RTEZScreenModeRequest */
static ULONG smTagList[] = {
    RT_LockWindow, (ULONG)-1, RTSC_Flags, SCREQF_SIZEGADS | SCREQF_DEPTHGAD, 0, 0
};
#define smFlags smTagList[3]
static struct BL_PACKED {
    ULONG DisplayID;
    UWORD DisplayWidth, DisplayHeight, DisplayDepth, OverscanType;
    ULONG Autoscroll;
} smRetType;

/* RTEZPaletteRequest */
static ULONG palreqTagList[] = { RTPA_Color, 0, RT_LockWindow, (ULONG)-1, 0, 0 };
#define FirstCol palreqTagList[1]

/* RTEZGetLong, RTEZGetLongRange */
static ULONG glValue;
static ULONG glTagList[] = {
    RT_LockWindow, (ULONG)-1, RTGL_TextFmt, 0, RT_ReqPos, REQPOS_CENTERSCR,
    RTGL_Flags, GLREQF_CENTERTEXT, 0, 0
};
#define gltxt glTagList[3]
static ULONG glrValue;
static ULONG glrTagList[] = {
    RT_LockWindow, (ULONG)-1, RTGL_TextFmt, 0, RT_ReqPos, REQPOS_CENTERSCR,
    RTGL_Flags, GLREQF_CENTERTEXT, RTGL_Min, 0, RTGL_Max, 0, 0, 0
};
#define glrtxt glrTagList[3]
#define glrMin glrTagList[9]
#define glrMax glrTagList[11]

/* RTEZGetString */
static UBYTE *a3_, *swsPtr, *gsBuffer;
static ULONG gsTxtSize, gsActual;
static ULONG gsTagList[] = {
    RT_LockWindow, (ULONG)-1, RTGL_TextFmt, 0, RT_ReqPos, REQPOS_CENTERSCR,
    RTGL_Flags, GLREQF_CENTERTEXT, 0, 0
};
#define gstxt gsTagList[3]

/* RTEZSaveFile */
static const char SaveTxt[] = "Save";
static STRPTR swsSFreq;
static UBYTE *Sfilenamestr;
static ULONG SfreqTagList[] = {
    RT_LockWindow, (ULONG)-1, RTFI_OkText, (ULONG)SaveTxt, RTFI_Flags, FREQF_SAVE, 0, 0
};
#define SfreqFlags SfreqTagList[5]

/* RTEZSetDefaultDirectory, RTEZSetPattern */
static ULONG sddTagList[] = { RTFI_Dir, 0, 0, 0 };
static ULONG spTagList[] = { RTFI_MatchPat, 0, 0, 0 };

/* RTEZPathRequest */
static STRPTR swsPreq;
static ULONG preqTagList[] = { RTFI_Flags, FREQF_NOFILES, RT_LockWindow, (ULONG)-1, 0, 0 };

/* RTEZFlagsRequest: RequestFTitle is never set (BUG(orig)) */
static ULONG RequestFTagList[] = {
    RTEZ_ReqTitle, 0, RT_ReqPos, REQPOS_CENTERSCR, RT_LockWindow, (ULONG)-1,
    RT_Underscore, '_', RT_IDCMPFlags, 0, 0, 0
};
#define reqFPos RequestFTagList[3]
#define rfflags RequestFTagList[9]

/* RTFileRequest */
static STRPTR swsTFreq;
static UBYTE *Tfilenamestr;

/* RTEZMultiLoadFile, RTNextPathEntry: an rtFileList (Next, StrLen,
   Name) */
static ULONG *Mfilelist, *CurEntry;
static ULONG MfreqTagList[] = { RT_LockWindow, (ULONG)-1, RTFI_Flags, FREQF_MULTISELECT, 0, 0 };
#define MfreqFlags MfreqTagList[3]

/* the asynchronous requesters */
static UBYTE mymask, myPmask;
static APTR myhandler, myPhandler;
static ULONG sigs, Psigs;
static ULONG asRequestTagList[] = {
    RT_ReqPos, REQPOS_CENTERSCR, RT_LockWindow, (ULONG)-1, RTEZ_ReqTitle, 0,
    RT_ReqHandler, (ULONG)&myhandler, 0, 0
};
#define asRequestTitle asRequestTagList[5]
static ULONG asPRequestTagList[] = {
    RTEZ_ReqTitle, 0, RTPA_Color, 0, RT_ReqHandler, (ULONG)&myPhandler, 0, 0
};
#define FirstPCol asPRequestTagList[3]
static const ULONG endreqTL[] = { RTRH_EndRequest, 0, 0, 0 };
static const ULONG endPreqTL[] = { RTRH_EndRequest, 0, 0, 0 };

/* ------------------------------------------------------------- helpers */

/* _CopyString: s to a3 without the 0; n + its length */
static ULONG copystring(const UBYTE *s, ULONG n)
{
    UBYTE c;

    while ((c = *s++) != 0) {
        *blitz_string++ = c;
        n++;
    }
    return n;
}

/* the directory of a file requester to a3 with a "/" unless it ends in
   ':' (or is empty); n + the characters written */
static ULONG copydir(const UBYTE *req, ULONG n)
{
    n = copystring(*(const UBYTE *const *)(req + RTFR_DIR), n);
    if (n && blitz_string[-1] != ':') {
        *blitz_string++ = '/';
        n++;
    }
    return n;
}

/* maxslib: a copy of len bytes of s / free it */
static UBYTE *allocst(const UBYTE *s, ULONG len)
{
    struct bl_regs r = { .d0 = (ULONG)s, .d1 = len };

    return (UBYTE *)BLITZ_ALIBJSR(TOK_allocst, r);
}

static void freest(UBYTE *p)
{
    struct bl_regs r = { .d0 = (ULONG)p };

    BLITZ_ALIBJSR(TOK_freest, r);
}

/* ------------------------------------------------------------ init/finit */

static ULONG allocreq(ULONG type)
{
    return RTJSR0(LVO_rtAllocRequestA, type, 0, 0);
}

/* each result is stored only if it is not 0 (the variable keeps its old
   value otherwise, as in the original) */
#define ALLOC(var, type) do {                                               \
        ULONG r_ = allocreq(type);                                          \
        if (!r_)                                                            \
            goto failed;                                                    \
        var = (UBYTE *)r_;                                                  \
    } while (0)

static ULONG BLITZ_ROUTINE initreqlib_c(void)
{
    struct Library *l = OpenLibrary((CONST_STRPTR)rtName, 38);

    if (!l)
        goto failed;
    rtbase = l;
    ALLOC(filereq, RT_FILEREQ);
    ALLOC(fontreq, RT_FONTREQ);
    ALLOC(smreq, RT_SCREENMODEREQ);
    ALLOC(Sfilereq, RT_FILEREQ);
    ALLOC(preq, RT_FILEREQ);
    ALLOC(Tfilereq, RT_FILEREQ);
    ALLOC(Mfilereq, RT_FILEREQ);
    ReqToolsActive = (ULONG)-1;
    return (ULONG)Mfilereq;
failed:                                 /* BUG(orig): nothing given back */
    ReqToolsActive = 0;
    return 0;
}

static void BLITZ_ROUTINE cleanupreqlib_c(void)
{
    if (!ReqToolsActive)
        return;
    RTJSR0(LVO_rtFreeRequest, 0, 0, filereq);
    RTJSR0(LVO_rtFreeRequest, 0, 0, fontreq);
    RTJSR0(LVO_rtFreeRequest, 0, 0, smreq);
    RTJSR0(LVO_rtFreeRequest, 0, 0, Sfilereq);
    RTJSR0(LVO_rtFreeRequest, 0, 0, Tfilereq);
    RTJSR0(LVO_rtFreeRequest, 0, 0, Mfilereq);
    RTJSR0(LVO_rtFreeRequest, 0, 0, preq);
    RTJSR0(LVO_rtFreeFileList, 0, Mfilelist, 0);
    CloseLibrary(rtbase);
}

/* IsReqToolsActive, RTReturnValue */
static ULONG BLITZ_ROUTINE rtactive_c(void)
{
    return ReqToolsActive;
}

static ULONG BLITZ_ROUTINE rtreturnvalue_c(void)
{
    return returnvalue;
}

/* RTVersion / RTRevision: lib_Version / lib_Revision (d0's high word
   kept) */
static ULONG BLITZ_ROUTINE rtver_c(ULONG d0 BL_REG(d0))
{
    return (d0 & 0xffff0000UL) | rtbase->lib_Version;
}

static ULONG BLITZ_ROUTINE rtrev_c(ULONG d0 BL_REG(d0))
{
    return (d0 & 0xffff0000UL) | rtbase->lib_Revision;
}

/* --------------------------------------------------------- EZ requests */

/* RTEZRequest(title$,body,gadget$) (d0-d2; the title's length -4(a2)):
   the title tag only with a title; position, flags, default response
   and title back to their defaults afterwards */
static ULONG BLITZ_ROUTINE rtrequest_c(STRPTR title BL_REG(d0), APTR body BL_REG(d1),
                                       STRPTR gadget BL_REG(d2),
                                       const ULONG *lens BL_REG(a2))
{
    ULONG r;

    if (lens[-1]) {
        RequestTitle = (ULONG)title;
        reqTTag = RTEZ_ReqTitle;
    }
    r = RTJSR4(LVO_rtEZRequestA, 0, RequestTagList, body, gadget, 0, 0);
    returnvalue = r;
    RequestPos = REQPOS_CENTERSCR;
    RequestFlags = 0;
    RequestDefRes = 1;
    RequestTitle = 0;
    reqTTag = 0;
    return r;
}

/* RTEZRequest(...,pos) (d3) */
static ULONG BLITZ_ROUTINE rtrequestPos_c(STRPTR title BL_REG(d0), APTR body BL_REG(d1),
                                          STRPTR gadget BL_REG(d2), ULONG pos BL_REG(d3),
                                          const ULONG *lens BL_REG(a2))
{
    RequestPos = pos;
    return rtrequest_c(title, body, gadget, lens);
}

/* RTEZRequest(...,pos,default,flags) (d3-d5) */
static ULONG BLITZ_ROUTINE rtrequestXtra_c(STRPTR title BL_REG(d0), APTR body BL_REG(d1),
                                           STRPTR gadget BL_REG(d2), ULONG pos BL_REG(d3),
                                           ULONG def BL_REG(d4), ULONG flags BL_REG(d5),
                                           const ULONG *lens BL_REG(a2))
{
    RequestDefRes = def;
    RequestFlags = flags;
    return rtrequestPos_c(title, body, gadget, pos, lens);
}

/* RTRequest(body$,gadget$,taglist) (d0-d2) */
static ULONG BLITZ_ROUTINE rtTagrequest_c(STRPTR body BL_REG(d0), STRPTR gadget BL_REG(d1),
                                          APTR tags BL_REG(d2))
{
    ULONG r = RTJSR4(LVO_rtEZRequestA, 0, tags, body, gadget, 0, 0);

    returnvalue = r;
    return r;
}

/* RTEZFlagsRequest(title$,body$,gadget$,idcmp) (d0-d3; the gadget's
   length -12(a2): none = no gadgets); BUG(orig): the title goes to
   RTEZRequest's list */
static ULONG BLITZ_ROUTINE rtflagsrequest_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                            STRPTR gadget BL_REG(d2), ULONG idcmp BL_REG(d3),
                                            const ULONG *lens BL_REG(a2))
{
    ULONG r;

    if (!lens[-3])
        gadget = NULL;
    rfflags = idcmp;
    RequestTitle = (ULONG)title;
    r = RTJSR4(LVO_rtEZRequestA, 0, RequestFTagList, body, gadget, 0, 0);
    returnvalue = r;
    reqFPos = REQPOS_CENTERSCR;
    return r;
}

/* RTEZFlagsRequest(...,pos) (d4) */
static ULONG BLITZ_ROUTINE rtflagsrequestPos_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                               STRPTR gadget BL_REG(d2), ULONG idcmp BL_REG(d3),
                                               ULONG pos BL_REG(d4),
                                               const ULONG *lens BL_REG(a2))
{
    reqFPos = pos;
    return rtflagsrequest_c(title, body, gadget, idcmp, lens);
}

/* ------------------------------------------------------ file requesters */

/* load/RTFileRequest: the path from the title on, a3 behind its 0; a3 =
   the title on cancel */
static ULONG filerequest(UBYTE *req, STRPTR title, UBYTE *name, APTR tags)
{
    ULONG r = RTJSR(LVO_rtFileRequestA, 0, tags, req, name, title);

    returnvalue = r;
    blitz_string = (UBYTE *)title;                      /* MOVE.l d0,a3 */
    if (r) {
        r = copydir(req, 0);
        r = copystring(name, r);
        *blitz_string++ = 0;
    }
    return r;
}

/* RTEZLoadFile(title$,file$) (d0, d1) */
static ULONG BLITZ_ROUTINE rtfreq_c(STRPTR title BL_REG(d0), const UBYTE *file BL_REG(d1))
{
    ULONG r;

    filenamestr = allocst(file, 108);
    swsFreq = title;
    r = filerequest(filereq, title, filenamestr, freqTagList);
    freest(filenamestr);
    return r;
}

/* RTFileRequest(title$,file$,taglist) (d0-d2) */
static ULONG BLITZ_ROUTINE rtTagsfreq_c(STRPTR title BL_REG(d0), const UBYTE *file BL_REG(d1),
                                        APTR tags BL_REG(d2))
{
    ULONG r;

    Tfilenamestr = allocst(file, 108);
    swsTFreq = title;
    r = filerequest(Tfilereq, title, Tfilenamestr, tags);
    freest(Tfilenamestr);
    return r;
}

/* RTEZSaveFile(title$,file$) (d0, d1): like RTEZLoadFile, but a3 comes
   back unchanged (MOVEM a3-a6) */
static ULONG BLITZ_ROUTINE rtsfreq_c(STRPTR title BL_REG(d0), const UBYTE *file BL_REG(d1))
{
    UBYTE *a3 = blitz_string;
    ULONG r;

    Sfilenamestr = allocst(file, 108);
    swsSFreq = title;
    r = filerequest(Sfilereq, title, Sfilenamestr, SfreqTagList);
    freest(Sfilenamestr);
    blitz_string = a3;
    return r;
}

/* RTEZPathRequest(title$) (d0): the directory from the title on, with
   "/" but no 0 */
static ULONG BLITZ_ROUTINE rtpreq_c(STRPTR title BL_REG(d0))
{
    ULONG r;

    swsPreq = title;
    r = RTJSR(LVO_rtFileRequestA, 0, preqTagList, preq, 0, title);
    returnvalue = r;
    blitz_string = (UBYTE *)title;
    if (r)
        r = copydir(preq, 0);
    return r;
}

/* RTEZSetDefaultDirectory n,dir$ (d0, d1): 0 load, 1 save, 2 path,
   3 multi-load */
static ULONG BLITZ_ROUTINE rtsetdefdir_c(ULONG n BL_REG(d0), STRPTR dir BL_REG(d1))
{
    UBYTE *req;

    sddTagList[1] = (ULONG)dir;
    if (n == 0)
        req = filereq;
    else if (n == 1)
        req = Sfilereq;
    else if (n == 2)
        req = preq;
    else if (n == 3)
        req = Mfilereq;
    else
        return 0;
    RTJSR0(LVO_rtChangeReqAttrA, 0, sddTagList, req);
    return 0;
}

/* RTEZSetPattern n,pat$ (d0, d1): 0 load, 1 save, 3 multi-load - with
   the pattern gadget */
static ULONG BLITZ_ROUTINE rtsetpat_c(ULONG n BL_REG(d0), STRPTR pat BL_REG(d1))
{
    UBYTE *req;

    spTagList[1] = (ULONG)pat;
    if (n == 0) {
        req = filereq;
        freqFlags = FREQF_PATGAD;
    } else if (n == 1) {
        req = Sfilereq;
        SfreqFlags = FREQF_PATGAD | FREQF_SAVE;
    } else if (n == 3) {
        MfreqFlags = FREQF_MULTISELECT | FREQF_PATGAD;
        req = Mfilereq;
    } else
        return 0;
    RTJSR0(LVO_rtChangeReqAttrA, 0, spTagList, req);
    return 0;
}

/* RTEZFreePattern n (d0): BUG(orig): 2 for the multi-load */
static ULONG BLITZ_ROUTINE rtfreepat_c(ULONG n BL_REG(d0))
{
    if (n == 0)
        freqFlags = 0;
    else if (n == 1)
        SfreqFlags = FREQF_SAVE;
    else if (n == 2)
        MfreqFlags = FREQF_MULTISELECT;
    return 0;
}

/* RTEZMultiLoadFile(title$) (d0): the file list (the old one freed), -1
   unless its low byte is 0 */
static ULONG BLITZ_ROUTINE rtMfreq_c(STRPTR title BL_REG(d0))
{
    ULONG r;

    RTJSR0(LVO_rtFreeFileList, 0, Mfilelist, 0);
    r = RTJSR(LVO_rtFileRequestA, 0, MfreqTagList, Mfilereq, fname, title);
    returnvalue = r;
    Mfilelist = (ULONG *)r;
    CurEntry = (ULONG *)r;
    if ((UBYTE)r == 0)                                  /* CMP.b #0,d0 */
        return r;
    return (ULONG)-1;
}

/* RTNextPathEntry: directory and the next name of the list, a3 behind
   the 0 */
static ULONG BLITZ_ROUTINE rtnxtentry_c(void)
{
    ULONG n;
    const ULONG *e;

    if (!CurEntry)
        return 0;
    n = copydir(Mfilereq, 0);
    e = CurEntry;
    CurEntry = (ULONG *)e[0];
    n = copystring((const UBYTE *)e[2], n);
    *blitz_string++ = 0;
    return n;
}

/* --------------------------------------------- font, screen mode, palette */

/* RTEZFontRequest(title$) (d0): the TextAttr (name, size, style, flags
   of the requester), 0 */
static ULONG BLITZ_ROUTINE rtfontreq_c(STRPTR title BL_REG(d0))
{
    ULONG r = RTJSR(LVO_rtFontRequestA, 0, fontTagList, fontreq, 0, title);
    const UBYTE *ta;

    returnvalue = r;
    if (!r)
        return 0;
    ta = fontreq + 16;
    myfont.ysize = *(const UWORD *)(ta + 4);
    myfont.styleflags = *(const UWORD *)(ta + 6);
    myfont.name = *(const APTR *)ta;
    return (ULONG)&myfont;
}

/* RTEZScreenModeRequest(title$) (d0): BUG(orig): rtFontRequestA */
static ULONG BLITZ_ROUTINE rtsmreq_c(STRPTR title BL_REG(d0))
{
    ULONG r = RTJSR(LVO_rtFontRequestA, 0, smTagList, smreq, 0, title);

    returnvalue = r;
    if (!r)
        return 0;
    smRetType.DisplayID = *(const ULONG *)(smreq + 16);
    smRetType.DisplayWidth = *(const UWORD *)(smreq + 20);
    smRetType.DisplayHeight = *(const UWORD *)(smreq + 22);
    smRetType.DisplayDepth = *(const UWORD *)(smreq + 52);
    smRetType.OverscanType = *(const UWORD *)(smreq + 54);
    smRetType.Autoscroll = *(const ULONG *)(smreq + 56);
    smFlags = SCREQF_SIZEGADS | SCREQF_DEPTHGAD;
    return (ULONG)&smRetType;
}

/* RTEZScreenModeRequest(title$,flags) (d0, d1) */
static ULONG BLITZ_ROUTINE rtsmreqf_c(STRPTR title BL_REG(d0), ULONG flags BL_REG(d1))
{
    smFlags = flags;
    return rtsmreq_c(title);
}

/* RTEZPaletteRequest(title$,first) (d0, d1) */
static ULONG BLITZ_ROUTINE rtpalreq_c(STRPTR title BL_REG(d0), ULONG first BL_REG(d1))
{
    FirstCol = first;
    return RTJSR(LVO_rtPaletteRequestA, 0, palreqTagList, 0, title, 0);
}

/* ------------------------------------------------------------ numbers */

/* RTEZGetLong(title$,body$) (d0, d1): the value, also on cancel */
static ULONG BLITZ_ROUTINE rtgetlong_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1))
{
    gltxt = (ULONG)body;
    returnvalue = RTJSR(LVO_rtGetLongA, 0, glTagList, &glValue, title, 0);
    return glValue;
}

/* RTEZGetLong(title$,body$,default) (d2) */
static ULONG BLITZ_ROUTINE rtgetlongDef_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                          ULONG def BL_REG(d2))
{
    glValue = def;
    return rtgetlong_c(title, body);
}

/* RTEZGetLongRange(title$,body$,min,max) (d0-d3): the value, 0 */
static ULONG BLITZ_ROUTINE rtgetlongrng_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                          ULONG min BL_REG(d2), ULONG max BL_REG(d3))
{
    ULONG r;

    glrtxt = (ULONG)body;
    glrMin = min;
    glrMax = max;
    r = RTJSR(LVO_rtGetLongA, 0, glrTagList, &glrValue, title, 0);
    returnvalue = r;
    if (r)
        r = glrValue;
    return r;
}

/* RTEZGetLongRange(...,default) (d4) */
static ULONG BLITZ_ROUTINE rtgetlongrngDef_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                             ULONG min BL_REG(d2), ULONG max BL_REG(d3),
                                             ULONG def BL_REG(d4))
{
    glrValue = def;
    return rtgetlongrng_c(title, body, min, max);
}

/* ------------------------------------------------------------ strings */

/* memlib globalloc (MEMF_CLEAR) / globfree */
static UBYTE *gsalloc(ULONG size)
{
    struct bl_regs r = { .d0 = size, .d1 = MEMF_CLEAR };

    return (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
}

/* gsCont: the requester on gsBuffer; the string from the title on (no 0,
   a3 behind it), its length; 0 and a3 unchanged on cancel (the buffer
   stays, BUG(orig)) */
static ULONG getstring(STRPTR title, STRPTR body, ULONG max)
{
    ULONG r;

    swsPtr = (UBYTE *)title;
    gstxt = (ULONG)body;
    gsTxtSize = max;
    r = RTJSR(LVO_rtGetStringA, max, gsTagList, gsBuffer, title, 0);
    returnvalue = r;
    blitz_string = a3_;
    if (!r)
        return 0;
    blitz_string = swsPtr;
    gsActual = copystring(gsBuffer, 0);
    {
        struct bl_regs f = { .d0 = gsTxtSize, .a1 = (ULONG)gsBuffer };

        BLITZ_ALIBJSR(TOK_globfree, f);
    }
    return gsActual;
}

/* RTEZGetString(title$,body$,max) (d0-d2) */
static ULONG BLITZ_ROUTINE rtgetstr_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                      ULONG max BL_REG(d2))
{
    a3_ = blitz_string;
    gsBuffer = gsalloc(max);
    return getstring(title, body, max);
}

/* RTEZGetString(title$,body$,max,default$) (d3): the default copied into
   the buffer (BUG(orig): without a limit) */
static ULONG BLITZ_ROUTINE rtgetstrDef_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                         ULONG max BL_REG(d2), const UBYTE *def BL_REG(d3))
{
    UBYTE *a3 = blitz_string;

    a3_ = blitz_string;
    gsBuffer = gsalloc(max);
    blitz_string = gsBuffer;
    copystring(def, 0);
    blitz_string = a3;
    return getstring(title, body, max);
}

/* ------------------------------------------------------------ windows */

/* RTLockWindow(win#) (a1 = the window object, its first long the
   Window) */
static ULONG BLITZ_ROUTINE rtlockwin_c(APTR *win BL_REG(a1))
{
    return RTJSR0(LVO_rtLockWindow, 0, *win, 0);
}

/* RTUnlockWindow win#,lock (a1, d1): d0 what reqtools leaves */
static ULONG BLITZ_ROUTINE rtunlockwin_c(ULONG lock BL_REG(d1), APTR *win BL_REG(a1))
{
    return RTJSR0(LVO_rtUnlockWindow, 0, *win, lock);
}

/* ---------------------------------------------------- async requesters */

/* the signal bit of the "user port" at 86 of the window object (the
   object, not the Window: BUG(orig)), shifted by one */
static UBYTE portmask(const UBYTE *obj)
{
    const struct MsgPort *port = *(const struct MsgPort *const *)(obj + 86);

    return (UBYTE)(port->mp_SigBit << 1);
}

/* RTASyncRequest(title$,body$,gadget$) (d0-d2, a1 = the used window
   object): -1 if the requester is up (CALL_HANDLER) */
static ULONG BLITZ_ROUTINE rtasrequest_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                         STRPTR gadget BL_REG(d2), const UBYTE *obj BL_REG(a1))
{
    ULONG r;

    mymask = portmask(obj);
    asRequestTitle = (ULONG)title;
    r = RTJSR4(LVO_rtEZRequestA, 0, asRequestTagList, body, gadget, 0, 0);
    returnvalue = r;
    if (r == CALL_HANDLER)
        return (ULONG)-1;
    return r;
}

/* RTCheckASyncRequest: -1 while it is up */
static ULONG BLITZ_ROUTINE rtcheckasreq_c(void)
{
    ULONG r = RTJSR0(LVO_rtReqHandlerA, sigs, 0, myhandler);

    if (r == CALL_HANDLER)
        return (ULONG)-1;
    return r;
}

/* RTEndASyncRequest */
static ULONG BLITZ_ROUTINE rtendasreq_c(void)
{
    return RTJSR0(LVO_rtReqHandlerA, sigs, endreqTL, myhandler);
}

/* RTASyncPaletteRequest(title$,first) (d0, d1, a1 as above) */
static ULONG BLITZ_ROUTINE rtasPrequest_c(STRPTR title BL_REG(d0), ULONG first BL_REG(d1),
                                          const UBYTE *obj BL_REG(a1))
{
    ULONG r;

    myPmask = portmask(obj);
    FirstPCol = first;
    r = RTJSR(LVO_rtPaletteRequestA, 0, asPRequestTagList, 0, title, 0);
    if (r == CALL_HANDLER)
        return (ULONG)-1;
    return r;
}

/* RTCheckASyncPaletteRequest: -1 while it is up, else 0 */
static ULONG BLITZ_ROUTINE rtcheckasPreq_c(void)
{
    ULONG r = RTJSR0(LVO_rtReqHandlerA, Psigs, 0, myPhandler);

    return r == CALL_HANDLER ? (ULONG)-1 : 0;
}

/* RTEndASyncPaletteRequest */
static ULONG BLITZ_ROUTINE rtendasPreq_c(void)
{
    return RTJSR0(LVO_rtReqHandlerA, Psigs, endPreqTL, myPhandler);
}

/* Registers: the original keeps a3-a6 with MOVEM in most routines (a4-a6
   in the file requesters with a string result in a3), changes d0/d1/a0/
   a1 (and what reqtools changes), a2 (loaded for reqtools), d2 in
   RTNextPathEntry; RTEZFreePattern changes d0 only.  The thunks keep
   d1/a0/a1 (finit also d0), the C code d2-d7/a2-a6. */
BLITZ_THUNK(_initreqlib, initreqlib_c, "d1/a0-a1");
BLITZ_THUNK(_cleanupreqlib, cleanupreqlib_c, "d0-d1/a0-a1");
BLITZ_THUNK(_rtactive, rtactive_c, "d1/a0-a1");
BLITZ_THUNK(_rtreturnvalue, rtreturnvalue_c, "d1/a0-a1");
BLITZ_THUNK(_rtver, rtver_c, "d1/a0-a1");
BLITZ_THUNK(_rtrev, rtrev_c, "d1/a0-a1");
BLITZ_THUNK(_rtrequest, rtrequest_c, "d1/a0-a1");
BLITZ_THUNK(_rtrequestPos, rtrequestPos_c, "d1/a0-a1");
BLITZ_THUNK(_rtrequestXtra, rtrequestXtra_c, "d1/a0-a1");
BLITZ_THUNK(_rtTagrequest, rtTagrequest_c, "d1/a0-a1");
BLITZ_THUNK(_rtflagsrequest, rtflagsrequest_c, "d1/a0-a1");
BLITZ_THUNK(_rtflagsrequestPos, rtflagsrequestPos_c, "d1/a0-a1");
BLITZ_THUNK(_rtfreq, rtfreq_c, "d1/a0-a1");
BLITZ_THUNK(_rtTagsfreq, rtTagsfreq_c, "d1/a0-a1");
BLITZ_THUNK(_rtsfreq, rtsfreq_c, "d1/a0-a1");
BLITZ_THUNK(_rtpreq, rtpreq_c, "d1/a0-a1");
BLITZ_THUNK(_rtsetdefdir, rtsetdefdir_c, "d1/a0-a1");
BLITZ_THUNK(_rtsetpat, rtsetpat_c, "d1/a0-a1");
BLITZ_THUNK(_rtfreepat, rtfreepat_c, "d1/a0-a1");
BLITZ_THUNK(_rtMfreq, rtMfreq_c, "d1/a0-a1");
BLITZ_THUNK(_rtnxtentry, rtnxtentry_c, "d1/a0-a1");
BLITZ_THUNK(_rtfontreq, rtfontreq_c, "d1/a0-a1");
BLITZ_THUNK(_rtsmreq, rtsmreq_c, "d1/a0-a1");
BLITZ_THUNK(_rtsmreqf, rtsmreqf_c, "d1/a0-a1");
BLITZ_THUNK(_rtpalreq, rtpalreq_c, "d1/a0-a1");
BLITZ_THUNK(_rtgetlong, rtgetlong_c, "d1/a0-a1");
BLITZ_THUNK(_rtgetlongDef, rtgetlongDef_c, "d1/a0-a1");
BLITZ_THUNK(_rtgetlongrng, rtgetlongrng_c, "d1/a0-a1");
BLITZ_THUNK(_rtgetlongrngDef, rtgetlongrngDef_c, "d1/a0-a1");
BLITZ_THUNK(_rtgetstr, rtgetstr_c, "d1/a0-a1");
BLITZ_THUNK(_rtgetstrDef, rtgetstrDef_c, "d1/a0-a1");
BLITZ_THUNK(_rtlockwin, rtlockwin_c, "d1/a0-a1");
BLITZ_THUNK(_rtunlockwin, rtunlockwin_c, "d1/a0-a1");
BLITZ_THUNK(_rtasrequest, rtasrequest_c, "d1/a0-a1");
BLITZ_THUNK(_rtcheckasreq, rtcheckasreq_c, "d1/a0-a1");
BLITZ_THUNK(_rtendasreq, rtendasreq_c, "d1/a0-a1");
BLITZ_THUNK(_rtasPrequest, rtasPrequest_c, "d1/a0-a1");
BLITZ_THUNK(_rtcheckasPreq, rtcheckasPreq_c, "d1/a0-a1");
BLITZ_THUNK(_rtendasPreq, rtendasPreq_c, "d1/a0-a1");
