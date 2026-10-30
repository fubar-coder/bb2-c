/*
 * asllib - command library 80 (#myasllib): ASL requesters -
 * ASLFileRequest$, ASLFontRequest, ASLScreenRequest, ASLPathRequest$.
 *
 * C translation of bblibsrc/asllib (BUM 5/6; binary of the CD's acidlibs
 * checked: same as the source).
 *   init:  OpenLibrary("asl.library",0) -> aslbase (0 if not there)
 *   finit: CloseLibrary(aslbase)
 * The requesters open on the used screen (#screenslib,$1102: a1 = the
 * screen object, 0 = default public screen).  The tag lists are static
 * data of the library (values patched before every call, kept between
 * calls) - as in the original.
 *
 * ASLFileRequest$(title$,path$,file$[,pattern$][,x,y,w,h]): path$ and
 * file$ are string variables (#varptr): on OK the drawer and the file
 * name are copied into them (their length longs at -4 updated), the
 * result is drawer + "/" (unless empty or ending in ":") + file, written
 * at a3 = the title argument's place in the string work area.  Cancel:
 * "" (0 at a3).  ASLPathRequest$ the same with FRF_DRAWERSONLY.
 * ASLFontRequest(flags) returns the address of a static .fontinfo
 * {name$ (pointer to a BB2 string: length.l in front), ysize.w, style.b,
 * flags.b, pen1.b, pen2.b, drawmode.b, pad.b}, 0 on cancel.
 * ASLScreenRequest(flags) returns the ScreenModeRequester, 0 on cancel.
 *
 * Kept from the original (BUG(orig)):
 *   - ASLScreenRequest returns the requester AFTER freeing it (asl LVO
 *     -36 = FreeFileRequest, as FREEREQ below): freed memory. (#60)
 *   - ASLPathRequest$ copies fr_File into the address in d2 (no such
 *     argument: with the dimensions it is x, else whatever d2 holds) and
 *     writes its length to d2-4. (#61)
 *   - Without asl.library ASLFileRequest$/ASLPathRequest$ return with d0
 *     = the title's address as string length (the "defaults to freqlib"
 *     call is commented out in the source). (#62)
 *   - Title and pattern copies (maxslib allocst) are never freed. (#63)
 *   - FreeFileRequest(0) when AllocAslRequest fails. (#403)
 *   - ASLPathRequest$ without dimensions keeps the pattern flag of the
 *     previous ASLFileRequest$ call (flags is not reset). (#64)
 *   - ASLFontRequest: a font name of more than 95 characters overruns the
 *     static buffer (strng+4: 96 bytes including the 0; in the binary the
 *     code of doaslscreen follows at $3B8) (#65); an empty name counts as
 *     cancel (not a bug: the ASL font requester returns no empty name with
 *     OK).
 * The CD binary differs from the source: an extra font tag ASL_TB+17 =
 * 256 and an initial pattern "" instead of 0 (the binary is followed).
 * DIFF: the original stores maxslib's d1 (-1) into the title's length
 * slot on the compiler stack ("Restore Blitzstack"); here a2 and the
 * stack are left alone.
 */
#include "blitzlib.h"
#include "blitzexec.h"
#include <intuition/screens.h>
#include <libraries/asl.h>
#define __NOLIBBASE__
#define ASL_BASE_NAME aslbase
#include <proto/asl.h>
#undef __NOLIBBASE__

#define FRF_DRAWERSONLY_ 1
#define ASLTAG 0x80080000UL

/* entry points (defined below) */
void doaslfreqa(void), doaslfreqb(void), doaslfreqc(void);
void doaslfreqd(void), doaslfreqe(void), doaslfreqf(void);
ULONG BLITZ_ROUTINE doaslfont(ULONG flags BL_REG(d0), struct Screen **scr BL_REG(a1));
ULONG BLITZ_ROUTINE doaslscreen(ULONG flags BL_REG(d0), struct Screen **scr BL_REG(a1));
void BLITZ_ROUTINE codeinit(void), codefinit(void);
void freqchk(void), freqchk2(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader{#myasllib,init,0,finit,errors} */
BLITZ_LIBHEADER(LIB_myasllib, init, BL_NORET, finit, BLITZ_ERROR_AREA);

#define SVAR (BL_VARPTR | BL_STRING)

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, SVAR, SVAR);
BLITZ_LIBS(LIB_screenslib, 0x1102);
BLITZ_SUBS(BLITZ_AMIGAONLY(doaslfreqa), freqchk, 0);
BLITZ_ARGS(BL_STRING, SVAR, SVAR, BL_STRING);
BLITZ_LIBS(LIB_screenslib, 0x1102);
BLITZ_SUBS(BLITZ_AMIGAONLY(doaslfreqb), freqchk, 0);
BLITZ_ARGS(BL_STRING, SVAR, SVAR, BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_screenslib, 0x1102);
BLITZ_SUBS(BLITZ_AMIGAONLY(doaslfreqc), freqchk, 0);
BLITZ_ARGS(BL_STRING, SVAR, SVAR, BL_STRING, BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_screenslib, 0x1102);
BLITZ_SUBS(BLITZ_AMIGAONLY(doaslfreqd), freqchk, 0);
BLITZ_NAME("ASLFileRequest$", "Title$,Pathname$,Filename$ [,Pattern$] [,x,y,w,h]");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_screenslib, 0x1102);
BLITZ_SUBS(BLITZ_AMIGAONLY(doaslfont), 0, 0);
BLITZ_NAME("ASLFontRequest", "enable flags (1=pen 2=bckgrnd 4=style 8=drawmode 16=fixedsize)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(LIB_screenslib, 0x1102);
BLITZ_SUBS(BLITZ_AMIGAONLY(doaslscreen), 0, 0);
BLITZ_NAME("ASLScreenRequest", "enable flags (1=width 2=height 4=depth 8=overscan 16=scroll)");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_STRING, SVAR);
BLITZ_LIBS(LIB_screenslib, 0x1102);
BLITZ_SUBS(BLITZ_AMIGAONLY(doaslfreqe), freqchk2, 0);
BLITZ_ARGS(BL_STRING, SVAR, BL_LONG, BL_LONG, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_screenslib, 0x1102);
BLITZ_SUBS(BLITZ_AMIGAONLY(doaslfreqf), freqchk2, 0);
BLITZ_NAME("ASLPathRequest$", "Title$,Pathname$ [,x,y,w,h]");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0);
BLITZ_NULLSUB_AT(finit, codefinit, 0, 0);

BLITZ_LIBFIN();

/* ----------------------------------------------------------------- */

static struct Library *aslbase;
static APTR aslstruct;

/* font requester tags and result (.fontinfo) */
/* the binary has a third tag (not in the source): ASL_TB+17, 256 */
static ULONG ftags[8] = { ASLTAG + 40, 0, ASLTAG + 20, 15, ASLTAG + 17, 256, 0, 0 };
#define fscreen ftags[1]
#define fflags  ftags[3]
static struct {
    char *name;                 /* strng+4 */
    UWORD size;
    UBYTE style[6];             /* style, flags, pen1, pen2, drawmode, pad */
    ULONG len;                  /* strng: length of the name */
    char str[96];
} BL_PACKED fstruct = { fstruct.str };

/* screen mode requester tags */
static ULONG stags[14] = { ASLTAG + 40, 0, ASLTAG + 109, 0, ASLTAG + 110, 0,
                           ASLTAG + 111, 0, ASLTAG + 112, 0, ASLTAG + 113, 0,
                           0, 0 };

/* an empty string (the binary points the pattern tag at a 0 word) */
static const UWORD nullstr[1] = { 0 };

/* file requester tags */
static ULONG Tags[26] = {
    ASLTAG + 40, 0,             /* ascreen */
    ASLTAG + 1, 0,              /* aname (TitleText) */
    ASLTAG + 9, 0,              /* apath (InitialDrawer) */
    ASLTAG + 8, 0,              /* afile (InitialFile) */
    ASLTAG + 3, 0,              /* ax */
    ASLTAG + 4, 0,              /* ay */
    ASLTAG + 5, 0,              /* aw */
    ASLTAG + 6, 0,              /* ah */
    ASLTAG + 20, 0,             /* flags (FRF_DOPATTERNS) */
    ASLTAG + 10, (ULONG)nullstr, /* pattern: binary "" (source 0) */
    ASLTAG + 60, (ULONG)-1,     /* noinfo */
    ASLTAG + 22, 0,             /* flags2 (FRF_DRAWERSONLY) */
    0, 0
};
#define T_SCREEN  1
#define T_NAME    3
#define T_PATH    5
#define T_FILE    7
#define T_X       9
#define T_Y       11
#define T_W       13
#define T_H       15
#define T_FLAGS   17
#define T_PATTERN 19
#define T_FLAGS2  23

/* the original frees with LVO -36 = FreeFileRequest (also for font and
   screen mode requesters; asl.library V38 treats it like FreeAslRequest) */
#define FREEREQ(r) FreeFileRequest((struct FileRequester *)(r))

static ULONG allocst(ULONG s, ULONG len)
{
    struct bl_regs r = { .d0 = s, .d1 = len };

    return BLITZ_ALIBJSR(TOK_allocst, r);
}

void BLITZ_ROUTINE codeinit(void)
{
    aslbase = OpenLibrary((CONST_STRPTR)"asl.library", 0);
}

void BLITZ_ROUTINE codefinit(void)
{
    if (aslbase) {
        CloseLibrary(aslbase);
        aslbase = NULL;
    }
}

/* ASLFontRequest(flags) */
ULONG BLITZ_ROUTINE doaslfont(ULONG flags BL_REG(d0), struct Screen **scr BL_REG(a1))
{
    ULONG d7;

    fscreen = (ULONG)*scr;
    fflags = flags;
    if (!aslbase)
        return 0;
    aslstruct = AllocAslRequest(ASL_FontRequest, (struct TagItem *)ftags);
    if (!aslstruct)
        return 0;
    d7 = AslRequest(aslstruct, (struct TagItem *)ftags);
    if (d7) {
        const UBYTE *a0 = (const UBYTE *)aslstruct + 8, *a1;
        char *d = fstruct.str;

        a1 = *(const UBYTE **)a0;                       /* ta_Name */
        *(ULONG *)&fstruct.size = *(const ULONG *)(a0 + 4);
        *(ULONG *)&fstruct.style[2] = *(const ULONG *)(a0 + 8);
        d7 = (ULONG)-1;
        do
            d7++;
        while ((*d++ = *a1++) != 0);
        fstruct.len = d7;
    }
    FREEREQ(aslstruct);
    if (!d7)
        return 0;
    return (ULONG)&fstruct;
}

/* ASLScreenRequest(flags) - BUG(orig) (#60): returns the freed requester */
ULONG BLITZ_ROUTINE doaslscreen(ULONG flags BL_REG(d0), struct Screen **scr BL_REG(a1))
{
    ULONG d7;
    int i;

    for (i = 0; i < 5; i++)
        stags[3 + 2 * i] = (flags & (1UL << i)) ? (ULONG)-1 : 0;
    stags[1] = (ULONG)*scr;
    if (!aslbase)
        return 0;
    aslstruct = AllocAslRequest(ASL_ScreenModeRequest, (struct TagItem *)stags);
    if (!aslstruct)
        return 0;
    d7 = AslRequest(aslstruct, (struct TagItem *)stags);
    FREEREQ(aslstruct);
    if (!d7)
        return 0;
    return (ULONG)aslstruct;
}

/* copy a string to the variable (length at -4) and to a3; returns its
   length (word count as the original) */
static UWORD copyout(const UBYTE *s, UBYTE *var)
{
    UWORD n = 0;

    do {
        n++;
        *var++ = *s;
    } while ((*blitz_string++ = *s++) != 0);
    blitz_string--;
    n--;
    return n;
}

/* defna/doaslreq: d0 = title, d1 = path var, d2 = file var */
static ULONG aslreq(ULONG title, ULONG path, ULONG file, struct Screen **scr,
                    const ULONG *lens)
{
    struct FileRequester *fr;
    ULONG d7, d6;

    if (!aslbase)
        return title;                   /* BUG(orig) (#62): d0 unchanged */
    Tags[T_SCREEN] = (ULONG)*scr;
    blitz_string = (UBYTE *)title;      /* the result goes here */
    Tags[T_NAME] = allocst(title, lens[-1]);
    Tags[T_PATH] = path;
    Tags[T_FILE] = file;
    aslstruct = AllocAslRequest(ASL_FileRequest, (struct TagItem *)Tags);
    if (!aslstruct || !AslRequest(aslstruct, (struct TagItem *)Tags)) {
        FREEREQ(aslstruct);      /* filefail (also with 0) */
        *blitz_string = 0;
        Tags[T_FLAGS2] = 0;
        return 0;
    }
    fr = aslstruct;
    d7 = copyout((const UBYTE *)fr->fr_Drawer, (UBYTE *)path);
    *(ULONG *)(path - 4) = d7;
    if (d7 && blitz_string[-1] != ':') {
        *blitz_string++ = '/';
        d7 = (UWORD)(d7 + 1);
    }
    d6 = copyout((const UBYTE *)fr->fr_File, (UBYTE *)file);
    *(ULONG *)(file - 4) = d6;
    d7 += d6;
    FREEREQ(aslstruct);
    Tags[T_FLAGS2] = 0;
    return d7;
}

static void dims(ULONG x, ULONG y, ULONG w, ULONG h)
{
    Tags[T_X] = x;
    Tags[T_Y] = y;
    Tags[T_W] = w;
    Tags[T_H] = h;
}

/* the C entry functions get all registers they may use as parameters;
   the token table points to them (+1: Amiga mode only) */
#define FREQ_ARGS ULONG d0 BL_REG(d0), ULONG d1 BL_REG(d1), ULONG d2 BL_REG(d2), \
    ULONG d3 BL_REG(d3), ULONG d4 BL_REG(d4), ULONG d5 BL_REG(d5),               \
    struct Screen **scr BL_REG(a1), const ULONG *lens BL_REG(a2)

/* title, path, file */
static ULONG BLITZ_ROUTINE freqa_c(FREQ_ARGS)
{
    Tags[T_FLAGS] = 0;
    dims(20, 20, 280, 160);
    return aslreq(d0, d1, d2, scr, lens);
}

/* title, path, file, pattern (-8(a2) = its length) */
static ULONG BLITZ_ROUTINE freqb_c(FREQ_ARGS)
{
    Tags[T_PATTERN] = allocst(d3, lens[-2]);
    Tags[T_FLAGS] = 1;                  /* FRF_DOPATTERNS */
    dims(20, 20, 280, 160);
    return aslreq(d0, d1, d2, scr, lens);
}

/* title, path, file, x, y, w, h (-8(a2)) */
static ULONG BLITZ_ROUTINE freqc_c(FREQ_ARGS)
{
    Tags[T_FLAGS] = 0;
    dims(d3, d4, d5, lens[-2]);
    return aslreq(d0, d1, d2, scr, lens);
}

/* title, path, file, pattern, x, y, w (-12(a2)), h (-16(a2)) */
static ULONG BLITZ_ROUTINE freqd_c(FREQ_ARGS)
{
    Tags[T_PATTERN] = allocst(d3, lens[-2]);
    Tags[T_FLAGS] = 1;
    dims(d4, d5, lens[-3], lens[-4]);
    return aslreq(d0, d1, d2, scr, lens);
}

/* ASLPathRequest$ title, path - d2 is no argument (BUG(orig), #61) */
static ULONG BLITZ_ROUTINE freqe_c(FREQ_ARGS)
{
    Tags[T_FLAGS2] = FRF_DRAWERSONLY_;
    dims(20, 20, 280, 160);             /* flags stay from the last call */
    return aslreq(d0, d1, d2, scr, lens);
}

/* ASLPathRequest$ title, path, x, y, w, h */
static ULONG BLITZ_ROUTINE freqf_c(FREQ_ARGS)
{
    Tags[T_FLAGS] = 0;
    dims(d2, d3, d4, d5);
    Tags[T_FLAGS2] = FRF_DRAWERSONLY_;
    return aslreq(d0, d1, d2, scr, lens);
}

/* entry labels = the C functions */
__asm__("   .globl _doaslfreqa,_doaslfreqb,_doaslfreqc,_doaslfreqd,_doaslfreqe,_doaslfreqf\n"
        "   .set _doaslfreqa,_freqa_c\n   .set _doaslfreqb,_freqb_c\n"
        "   .set _doaslfreqc,_freqc_c\n   .set _doaslfreqd,_freqd_c\n"
        "   .set _doaslfreqe,_freqe_c\n   .set _doaslfreqf,_freqf_c\n");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess2, "Not enough space in PATH string");
BLITZ_ERRCHK_MESSAGE(mess3, "Not enough space in NAME string");

/* freqchk - path$/file$ must be variables with room for 160/64 chars
   (-8 = the buffer size); err1 = runtime error number 1 */
static void BLITZ_ERRCHK freqchk_c(ULONG path BL_REG(d1), ULONG file BL_REG(d2))
{
    if (!path || !file)
        BLITZ_RUNERR((const char *)1);
    else if (*(const ULONG *)(path - 8) < 160)
        BLITZ_RUNERR(mess2);
    else if (*(const ULONG *)(file - 8) < 64)
        BLITZ_RUNERR(mess3);
}

static void BLITZ_ERRCHK freqchk2_c(ULONG path BL_REG(d1))
{
    if (!path)
        BLITZ_RUNERR((const char *)1);
    else if (*(const ULONG *)(path - 8) < 160)
        BLITZ_RUNERR(mess2);
}

BLITZ_ERRCHK_THUNK(freqchk, freqchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(freqchk2, freqchk2_c, "d0-d1/a0-a1");
