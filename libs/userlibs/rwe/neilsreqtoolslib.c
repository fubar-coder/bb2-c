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
 *     mode requester instead of rtScreenModeRequestA (-144) (#466).
 *   - RTEZFlagsRequest stores the title in RTEZRequest's tag list, not in
 *     its own: its requester always has reqtools' default title (there
 *     the title tag is off until RTEZRequest with a title sets it, and
 *     RTEZRequest resets the title after every call) (#455).
 *   - RTEZFreePattern 2 resets the multi-load requester's flags (the
 *     multi-load is 3 in RTEZSetPattern and RTEZSetDefaultDirectory)
 *     (#456).
 *   - RTEZMultiLoadFile tests only the low byte of the file list address
 *     (CMP.b #0,d0): a list at an address ending in $00 is returned as
 *     the address instead of -1 (#462).
 *   - RTEZGetLong returns the value also on cancel (TST.l without a
 *     branch); the value and the default stay for the next call (#457).
 *   - RTEZGetString on cancel does not free its buffer (memlib frees it
 *     at the end of the program) (#458); the default string is copied
 *     without a limit into the buffer of MaxChars bytes (#459).
 *   - RTEZSaveFile builds its result at the title but restores a3 (MOVEM
 *     a3-a6) (#465), RTEZLoadFile/RTFileRequest leave a3 behind the 0
 *     (#461).
 *   - RTASyncRequest/RTASyncPaletteRequest read the "user port" at 86 of
 *     the window object (not of the Window) and keep its signal bit
 *     shifted by one (LSL.b #1) in a byte nobody reads; the handlers are
 *     called with no signals (sigs is never set) (#454).
 *     RTASyncPaletteRequest and RTEZPaletteRequest do not store
 *     RTReturnValue (#464).
 *   - init: when a requester cannot be allocated, the library and the
 *     requesters allocated so far stay (finit does nothing then); finit
 *     does not clear anything (a second finit frees again) (#460).  No
 *     routine checks the library: without reqtools.library they call
 *     through a6 = 0 (RTVersion/RTRevision read address $14/$16) (#463).
 * Not defects: the string results (RTEZGetString, RTEZPathRequest, ...)
 * have no 0 behind them (a string function returns its length, as
 * usual); FileName$ is copied with a length of 108, whatever its own
 * (maxslib copies 108 bytes: reqtools' file name buffer size).
 *
 * DIFF: a2 stays unchanged (the original loads its arguments for
 * reqtools there; docs/libs/cmdlibs-port.md "General").
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <stddef.h>
#include <libraries/reqtools.h>

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

/* the tags, flags and requester structures: libraries/reqtools.h */
static_assert(offsetof(struct rtFileRequester, Dir) == 16);
static_assert(offsetof(struct rtFontRequester, Attr) == 16);
static_assert(offsetof(struct rtScreenModeRequester, DisplayID) == 16);
static_assert(offsetof(struct rtScreenModeRequester, DisplayDepth) == 52);
static_assert(offsetof(struct rtScreenModeRequester, AutoScroll) == 56);
static_assert(sizeof(struct rtFileList) == 12);

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
/* the requesters, in the original's order */
static struct rtFileRequester *filereq;
static struct rtFontRequester *fontreq;
static struct rtScreenModeRequester *smreq;
static struct rtFileRequester *Sfilereq, *preq, *Tfilereq, *Mfilereq;
static const char rtName[] = "reqtools.library";
static UBYTE fname[108];

#define TAG_LOCKWINDOW { RT_LockWindow, (ULONG)-1 }
#define TAG_CENTERSCR  { RT_ReqPos, REQPOS_CENTERSCR }

/* the tag lists of the original, each with a TAG_END pair at its end */

/* RT_LockWindow, one flags tag (load, font, screen mode, multi-load) */
struct lockflagstags {
    struct TagItem lockwin, flags, end;
};
static_assert(sizeof(struct lockflagstags) == 6 * 4);

/* one tag */
struct onetag {
    struct TagItem tag, end;
};
static_assert(sizeof(struct onetag) == 4 * 4);

/* rtGetLongA/rtGetStringA: window lock, body text, position, flags */
struct gltags {
    struct TagItem lockwin, textfmt, pos, flags, end;
};
static_assert(sizeof(struct gltags) == 10 * 4);

/* RTEZRequest: the title tag's id is 0 (TAG_END: the list ends there)
   until a call with a title sets RTEZ_ReqTitle */
static struct {
    struct TagItem pos, lockwin, underscore, flags, defres, title, end;
} RequestTagList = {
    TAG_CENTERSCR, TAG_LOCKWINDOW, { RT_Underscore, '_' },
    { RTEZ_Flags, 0 }, { RTEZ_DefaultResponse, 1 }, { 0, 0 }, { TAG_END, 0 }
};
static_assert(sizeof RequestTagList == 14 * 4);

/* RTEZLoadFile */
static STRPTR swsFreq;
static UBYTE *filenamestr;
static struct lockflagstags freqTagList = {
    TAG_LOCKWINDOW, { RTFI_Flags, 0 }, { TAG_END, 0 }
};

/* RTEZFontRequest: a TextAttr */
static struct lockflagstags fontTagList = {
    TAG_LOCKWINDOW, { RTFO_Flags, FREQF_STYLE }, { TAG_END, 0 }
};
static struct BL_PACKED {
    APTR name;
    UWORD ysize;
    UWORD styleflags;                                   /* style, flags */
} myfont;

/* RTEZScreenModeRequest */
static struct lockflagstags smTagList = {
    TAG_LOCKWINDOW, { RTSC_Flags, SCREQF_SIZEGADS | SCREQF_DEPTHGAD }, { TAG_END, 0 }
};
static struct BL_PACKED {
    ULONG DisplayID;
    UWORD DisplayWidth, DisplayHeight, DisplayDepth, OverscanType;
    ULONG Autoscroll;
} smRetType;

/* RTEZPaletteRequest */
static struct {
    struct TagItem color, lockwin, end;
} palreqTagList = { { RTPA_Color, 0 }, TAG_LOCKWINDOW, { TAG_END, 0 } };
static_assert(sizeof palreqTagList == 6 * 4);

/* RTEZGetLong, RTEZGetLongRange */
static ULONG glValue;
static struct gltags glTagList = {
    TAG_LOCKWINDOW, { RTGL_TextFmt, 0 }, TAG_CENTERSCR,
    { RTGL_Flags, GLREQF_CENTERTEXT }, { TAG_END, 0 }
};
static ULONG glrValue;
static struct {
    struct TagItem lockwin, textfmt, pos, flags, min, max, end;
} glrTagList = {
    TAG_LOCKWINDOW, { RTGL_TextFmt, 0 }, TAG_CENTERSCR,
    { RTGL_Flags, GLREQF_CENTERTEXT }, { RTGL_Min, 0 }, { RTGL_Max, 0 }, { TAG_END, 0 }
};
static_assert(sizeof glrTagList == 14 * 4);

/* RTEZGetString */
static UBYTE *callerstring;             /* the caller's a3 */
static UBYTE *swsPtr, *gsBuffer;
static ULONG gsTxtSize, gsActual;
static struct gltags gsTagList = {
    TAG_LOCKWINDOW, { RTGL_TextFmt, 0 }, TAG_CENTERSCR,
    { RTGL_Flags, GLREQF_CENTERTEXT }, { TAG_END, 0 }
};

/* RTEZSaveFile */
static const char SaveTxt[] = "Save";
static STRPTR swsSFreq;
static UBYTE *Sfilenamestr;
static struct {
    struct TagItem lockwin, oktext, flags, end;
} SfreqTagList = {
    TAG_LOCKWINDOW, { RTFI_OkText, (ULONG)SaveTxt }, { RTFI_Flags, FREQF_SAVE }, { TAG_END, 0 }
};
static_assert(sizeof SfreqTagList == 8 * 4);

/* RTEZSetDefaultDirectory, RTEZSetPattern */
static struct onetag sddTagList = { { RTFI_Dir, 0 }, { TAG_END, 0 } };
static struct onetag spTagList = { { RTFI_MatchPat, 0 }, { TAG_END, 0 } };

/* RTEZPathRequest */
static STRPTR swsPreq;
static struct {
    struct TagItem flags, lockwin, end;
} preqTagList = { { RTFI_Flags, FREQF_NOFILES }, TAG_LOCKWINDOW, { TAG_END, 0 } };
static_assert(sizeof preqTagList == 6 * 4);

/* RTEZFlagsRequest: its title is never set (BUG(orig), #455) */
static struct {
    struct TagItem title, pos, lockwin, underscore, idcmp, end;
} RequestFTagList = {
    { RTEZ_ReqTitle, 0 }, TAG_CENTERSCR, TAG_LOCKWINDOW,
    { RT_Underscore, '_' }, { RT_IDCMPFlags, 0 }, { TAG_END, 0 }
};
static_assert(sizeof RequestFTagList == 12 * 4);

/* RTFileRequest */
static STRPTR swsTFreq;
static UBYTE *Tfilenamestr;

/* RTEZMultiLoadFile, RTNextPathEntry */
static struct rtFileList *Mfilelist, *CurEntry;
static struct lockflagstags MfreqTagList = {
    TAG_LOCKWINDOW, { RTFI_Flags, FREQF_MULTISELECT }, { TAG_END, 0 }
};

/* the asynchronous requesters */
static UBYTE mymask, myPmask;
static APTR myhandler, myPhandler;
static ULONG sigs, Psigs;
static struct {
    struct TagItem pos, lockwin, title, handler, end;
} asRequestTagList = {
    TAG_CENTERSCR, TAG_LOCKWINDOW, { RTEZ_ReqTitle, 0 },
    { RT_ReqHandler, (ULONG)&myhandler }, { TAG_END, 0 }
};
static_assert(sizeof asRequestTagList == 10 * 4);
static struct {
    struct TagItem title, color, handler, end;
} asPRequestTagList = {
    { RTEZ_ReqTitle, 0 }, { RTPA_Color, 0 }, { RT_ReqHandler, (ULONG)&myPhandler },
    { TAG_END, 0 }
};
static_assert(sizeof asPRequestTagList == 8 * 4);
static const struct onetag endreqTL = { { RTRH_EndRequest, 0 }, { TAG_END, 0 } };
static const struct onetag endPreqTL = { { RTRH_EndRequest, 0 }, { TAG_END, 0 } };

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
static ULONG copydir(const struct rtFileRequester *req, ULONG n)
{
    n = copystring((const UBYTE *)req->Dir, n);
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
        var = (void *)r_;                                                   \
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
failed:                                 /* BUG(orig): nothing given back (#460) */
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
static ULONG BLITZ_ROUTINE rtver_c(ULONG callerd0 BL_REG(d0))
{
    return (callerd0 & 0xffff0000UL) | rtbase->lib_Version;
}

static ULONG BLITZ_ROUTINE rtrev_c(ULONG callerd0 BL_REG(d0))
{
    return (callerd0 & 0xffff0000UL) | rtbase->lib_Revision;
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
        RequestTagList.title.ti_Data = (ULONG)title;
        RequestTagList.title.ti_Tag = RTEZ_ReqTitle;
    }
    r = RTJSR4(LVO_rtEZRequestA, 0, &RequestTagList, body, gadget, 0, 0);
    returnvalue = r;
    RequestTagList.pos.ti_Data = REQPOS_CENTERSCR;
    RequestTagList.flags.ti_Data = 0;
    RequestTagList.defres.ti_Data = 1;
    RequestTagList.title.ti_Data = 0;
    RequestTagList.title.ti_Tag = 0;
    return r;
}

/* RTEZRequest(...,pos) (d3) */
static ULONG BLITZ_ROUTINE rtrequestPos_c(STRPTR title BL_REG(d0), APTR body BL_REG(d1),
                                          STRPTR gadget BL_REG(d2), ULONG pos BL_REG(d3),
                                          const ULONG *lens BL_REG(a2))
{
    RequestTagList.pos.ti_Data = pos;
    return rtrequest_c(title, body, gadget, lens);
}

/* RTEZRequest(...,pos,default,flags) (d3-d5) */
static ULONG BLITZ_ROUTINE rtrequestXtra_c(STRPTR title BL_REG(d0), APTR body BL_REG(d1),
                                           STRPTR gadget BL_REG(d2), ULONG pos BL_REG(d3),
                                           ULONG def BL_REG(d4), ULONG flags BL_REG(d5),
                                           const ULONG *lens BL_REG(a2))
{
    RequestTagList.defres.ti_Data = def;
    RequestTagList.flags.ti_Data = flags;
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
   length -12(a2): none = no gadgets); BUG(orig) (#455): the title goes to
   RTEZRequest's list */
static ULONG BLITZ_ROUTINE rtflagsrequest_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                            STRPTR gadget BL_REG(d2), ULONG idcmp BL_REG(d3),
                                            const ULONG *lens BL_REG(a2))
{
    ULONG r;

    if (!lens[-3])
        gadget = NULL;
    RequestFTagList.idcmp.ti_Data = idcmp;
    /* BUG(orig) (#455): RTEZRequest's title, not RequestFTagList.title */
    RequestTagList.title.ti_Data = (ULONG)title;
    r = RTJSR4(LVO_rtEZRequestA, 0, &RequestFTagList, body, gadget, 0, 0);
    returnvalue = r;
    RequestFTagList.pos.ti_Data = REQPOS_CENTERSCR;
    return r;
}

/* RTEZFlagsRequest(...,pos) (d4) */
static ULONG BLITZ_ROUTINE rtflagsrequestPos_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                               STRPTR gadget BL_REG(d2), ULONG idcmp BL_REG(d3),
                                               ULONG pos BL_REG(d4),
                                               const ULONG *lens BL_REG(a2))
{
    RequestFTagList.pos.ti_Data = pos;
    return rtflagsrequest_c(title, body, gadget, idcmp, lens);
}

/* ------------------------------------------------------ file requesters */

/* load/RTFileRequest: the path from the title on, a3 behind its 0; a3 =
   the title on cancel */
static ULONG filerequest(struct rtFileRequester *req, STRPTR title, UBYTE *name, APTR tags)
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
    r = filerequest(filereq, title, filenamestr, &freqTagList);
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
    UBYTE *savedstr = blitz_string;
    ULONG r;

    Sfilenamestr = allocst(file, 108);
    swsSFreq = title;
    r = filerequest(Sfilereq, title, Sfilenamestr, &SfreqTagList);
    freest(Sfilenamestr);
    blitz_string = savedstr;
    return r;
}

/* RTEZPathRequest(title$) (d0): the directory from the title on, with
   "/" but no 0 */
static ULONG BLITZ_ROUTINE rtpreq_c(STRPTR title BL_REG(d0))
{
    ULONG r;

    swsPreq = title;
    r = RTJSR(LVO_rtFileRequestA, 0, &preqTagList, preq, 0, title);
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
    struct rtFileRequester *req;

    sddTagList.tag.ti_Data = (ULONG)dir;
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
    RTJSR0(LVO_rtChangeReqAttrA, 0, &sddTagList, req);
    return 0;
}

/* RTEZSetPattern n,pat$ (d0, d1): 0 load, 1 save, 3 multi-load - with
   the pattern gadget */
static ULONG BLITZ_ROUTINE rtsetpat_c(ULONG n BL_REG(d0), STRPTR pat BL_REG(d1))
{
    struct rtFileRequester *req;

    spTagList.tag.ti_Data = (ULONG)pat;
    if (n == 0) {
        req = filereq;
        freqTagList.flags.ti_Data = FREQF_PATGAD;
    } else if (n == 1) {
        req = Sfilereq;
        SfreqTagList.flags.ti_Data = FREQF_PATGAD | FREQF_SAVE;
    } else if (n == 3) {
        MfreqTagList.flags.ti_Data = FREQF_MULTISELECT | FREQF_PATGAD;
        req = Mfilereq;
    } else
        return 0;
    RTJSR0(LVO_rtChangeReqAttrA, 0, &spTagList, req);
    return 0;
}

/* RTEZFreePattern n (d0): BUG(orig): 2 for the multi-load (#456) */
static ULONG BLITZ_ROUTINE rtfreepat_c(ULONG n BL_REG(d0))
{
    if (n == 0)
        freqTagList.flags.ti_Data = 0;
    else if (n == 1)
        SfreqTagList.flags.ti_Data = FREQF_SAVE;
    else if (n == 2)
        MfreqTagList.flags.ti_Data = FREQF_MULTISELECT;
    return 0;
}

/* RTEZMultiLoadFile(title$) (d0): the file list (the old one freed), -1
   unless its low byte is 0 */
static ULONG BLITZ_ROUTINE rtMfreq_c(STRPTR title BL_REG(d0))
{
    ULONG r;

    RTJSR0(LVO_rtFreeFileList, 0, Mfilelist, 0);
    r = RTJSR(LVO_rtFileRequestA, 0, &MfreqTagList, Mfilereq, fname, title);
    returnvalue = r;
    Mfilelist = (struct rtFileList *)r;
    CurEntry = (struct rtFileList *)r;
    if ((UBYTE)r == 0)                                  /* CMP.b #0,d0 */
        return r;
    return (ULONG)-1;
}

/* RTNextPathEntry: directory and the next name of the list, a3 behind
   the 0 */
static ULONG BLITZ_ROUTINE rtnxtentry_c(void)
{
    ULONG n;
    const struct rtFileList *e;

    if (!CurEntry)
        return 0;
    n = copydir(Mfilereq, 0);
    e = CurEntry;
    CurEntry = e->Next;
    n = copystring((const UBYTE *)e->Name, n);
    *blitz_string++ = 0;
    return n;
}

/* --------------------------------------------- font, screen mode, palette */

/* RTEZFontRequest(title$) (d0): the TextAttr (name, size, style, flags
   of the requester), 0 */
static ULONG BLITZ_ROUTINE rtfontreq_c(STRPTR title BL_REG(d0))
{
    ULONG r = RTJSR(LVO_rtFontRequestA, 0, &fontTagList, fontreq, 0, title);
    const struct TextAttr *ta;

    returnvalue = r;
    if (!r)
        return 0;
    ta = &fontreq->Attr;
    myfont.ysize = ta->ta_YSize;
    myfont.styleflags = *(const UWORD *)&ta->ta_Style;  /* ta_Style, ta_Flags */
    myfont.name = ta->ta_Name;
    return (ULONG)&myfont;
}

/* RTEZScreenModeRequest(title$) (d0): BUG(orig): rtFontRequestA (#466) */
static ULONG BLITZ_ROUTINE rtsmreq_c(STRPTR title BL_REG(d0))
{
    ULONG r = RTJSR(LVO_rtFontRequestA, 0, &smTagList, smreq, 0, title);

    returnvalue = r;
    if (!r)
        return 0;
    smRetType.DisplayID = smreq->DisplayID;
    smRetType.DisplayWidth = smreq->DisplayWidth;
    smRetType.DisplayHeight = smreq->DisplayHeight;
    smRetType.DisplayDepth = smreq->DisplayDepth;
    smRetType.OverscanType = smreq->OverscanType;
    smRetType.Autoscroll = smreq->AutoScroll;
    smTagList.flags.ti_Data = SCREQF_SIZEGADS | SCREQF_DEPTHGAD;
    return (ULONG)&smRetType;
}

/* RTEZScreenModeRequest(title$,flags) (d0, d1) */
static ULONG BLITZ_ROUTINE rtsmreqf_c(STRPTR title BL_REG(d0), ULONG flags BL_REG(d1))
{
    smTagList.flags.ti_Data = flags;
    return rtsmreq_c(title);
}

/* RTEZPaletteRequest(title$,first) (d0, d1) */
static ULONG BLITZ_ROUTINE rtpalreq_c(STRPTR title BL_REG(d0), ULONG first BL_REG(d1))
{
    palreqTagList.color.ti_Data = first;
    return RTJSR(LVO_rtPaletteRequestA, 0, &palreqTagList, 0, title, 0);
}

/* ------------------------------------------------------------ numbers */

/* RTEZGetLong(title$,body$) (d0, d1): the value, also on cancel */
static ULONG BLITZ_ROUTINE rtgetlong_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1))
{
    glTagList.textfmt.ti_Data = (ULONG)body;
    returnvalue = RTJSR(LVO_rtGetLongA, 0, &glTagList, &glValue, title, 0);
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

    glrTagList.textfmt.ti_Data = (ULONG)body;
    glrTagList.min.ti_Data = min;
    glrTagList.max.ti_Data = max;
    r = RTJSR(LVO_rtGetLongA, 0, &glrTagList, &glrValue, title, 0);
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
   stays, BUG(orig), #458) */
static ULONG getstring(STRPTR title, STRPTR body, ULONG max)
{
    ULONG r;

    swsPtr = (UBYTE *)title;
    gsTagList.textfmt.ti_Data = (ULONG)body;
    gsTxtSize = max;
    r = RTJSR(LVO_rtGetStringA, max, &gsTagList, gsBuffer, title, 0);
    returnvalue = r;
    blitz_string = callerstring;
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
    callerstring = blitz_string;
    gsBuffer = gsalloc(max);
    return getstring(title, body, max);
}

/* RTEZGetString(title$,body$,max,default$) (d3): the default copied into
   the buffer (BUG(orig): without a limit, #459) */
static ULONG BLITZ_ROUTINE rtgetstrDef_c(STRPTR title BL_REG(d0), STRPTR body BL_REG(d1),
                                         ULONG max BL_REG(d2), const UBYTE *def BL_REG(d3))
{
    UBYTE *savedstr = blitz_string;

    callerstring = blitz_string;
    gsBuffer = gsalloc(max);
    blitz_string = gsBuffer;
    copystring(def, 0);
    blitz_string = savedstr;
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
   object, not the Window: BUG(orig), #454), shifted by one */
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
    asRequestTagList.title.ti_Data = (ULONG)title;
    r = RTJSR4(LVO_rtEZRequestA, 0, &asRequestTagList, body, gadget, 0, 0);
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
    return RTJSR0(LVO_rtReqHandlerA, sigs, &endreqTL, myhandler);
}

/* RTASyncPaletteRequest(title$,first) (d0, d1, a1 as above) */
static ULONG BLITZ_ROUTINE rtasPrequest_c(STRPTR title BL_REG(d0), ULONG first BL_REG(d1),
                                          const UBYTE *obj BL_REG(a1))
{
    ULONG r;

    myPmask = portmask(obj);
    asPRequestTagList.color.ti_Data = first;
    r = RTJSR(LVO_rtPaletteRequestA, 0, &asPRequestTagList, 0, title, 0);
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
    return RTJSR0(LVO_rtReqHandlerA, Psigs, &endPreqTL, myPhandler);
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
