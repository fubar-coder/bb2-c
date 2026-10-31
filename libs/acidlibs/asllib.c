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
static struct {
    struct TagItem screen;      /* ASLFO_Screen */
    struct TagItem flags;       /* ASLFO_Flags */
    struct TagItem maxheight;   /* ASLFO_MaxHeight (binary only) */
    struct TagItem end;         /* TAG_END */
} ftags = {
    { ASLTAG + 40, 0 }, { ASLTAG + 20, 15 }, { ASLTAG + 17, 256 }, { 0, 0 }
};
static_assert(sizeof ftags == 8 * 4);
static struct {
    char *name;                 /* strng+4 */
    UWORD size;                 /* ta_YSize */
    UBYTE style;                /* ta_Style */
    UBYTE flags;                /* ta_Flags */
    UBYTE pen1;                 /* fo_FrontPen */
    UBYTE pen2;                 /* fo_BackPen */
    UBYTE drawmode;             /* fo_DrawMode */
    UBYTE pad;                  /* the byte behind fo_DrawMode */
    ULONG len;                  /* strng: length of the name */
    char str[96];
} BL_PACKED fstruct = { fstruct.str };
static_assert(sizeof fstruct == 12 + 4 + 96);

/* screen mode requester tags */
static struct {
    struct TagItem screen;      /* ASLSM_Screen */
    struct TagItem dotags[5];   /* ASLSM_DoWidth, DoHeight, DoDepth,
                                   DoOverscanType, DoAutoScroll */
    struct TagItem end;         /* TAG_END */
} stags = {
    { ASLTAG + 40, 0 },
    { { ASLTAG + 109, 0 }, { ASLTAG + 110, 0 }, { ASLTAG + 111, 0 },
      { ASLTAG + 112, 0 }, { ASLTAG + 113, 0 } },
    { 0, 0 }
};
static_assert(sizeof stags == 14 * 4);

/* an empty string (the binary points the pattern tag at a 0 word) */
static const UWORD nullstr[1] = { 0 };

/* file requester tags (the original's labels in the comments) */
static struct {
    struct TagItem screen;      /* ascreen: ASLFR_Screen */
    struct TagItem name;        /* aname: ASLFR_TitleText */
    struct TagItem path;        /* apath: ASLFR_InitialDrawer */
    struct TagItem file;        /* afile: ASLFR_InitialFile */
    struct TagItem x;           /* ax: ASLFR_InitialLeftEdge */
    struct TagItem y;           /* ay: ASLFR_InitialTopEdge */
    struct TagItem w;           /* aw: ASLFR_InitialWidth */
    struct TagItem h;           /* ah: ASLFR_InitialHeight */
    struct TagItem flags;       /* ASLFR_Flags1 (FRF_DOPATTERNS) */
    struct TagItem pattern;     /* ASLFR_InitialPattern: binary "" (source 0) */
    struct TagItem noinfo;      /* ASLFR_RejectIcons */
    struct TagItem flags2;      /* ASLFR_Flags2 (FRF_DRAWERSONLY) */
    struct TagItem end;         /* TAG_END */
} Tags = {
    { ASLTAG + 40, 0 }, { ASLTAG + 1, 0 }, { ASLTAG + 9, 0 }, { ASLTAG + 8, 0 },
    { ASLTAG + 3, 0 }, { ASLTAG + 4, 0 }, { ASLTAG + 5, 0 }, { ASLTAG + 6, 0 },
    { ASLTAG + 20, 0 }, { ASLTAG + 10, (ULONG)nullstr }, { ASLTAG + 60, (ULONG)-1 },
    { ASLTAG + 22, 0 }, { 0, 0 }
};
static_assert(sizeof Tags == 26 * 4);

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
    ULONG ok;                           /* d7: AslRequest's result */
    ULONG len = 0;                      /* d7: the font name length */

    ftags.screen.ti_Data = (ULONG)*scr;
    ftags.flags.ti_Data = flags;
    if (!aslbase)
        return 0;
    aslstruct = AllocAslRequest(ASL_FontRequest, (struct TagItem *)&ftags);
    if (!aslstruct)
        return 0;
    ok = AslRequest(aslstruct, (struct TagItem *)&ftags);
    if (ok) {
        const struct FontRequester *fo = aslstruct;
        const UBYTE *name;
        char *dst = fstruct.str;

        name = (const UBYTE *)fo->fo_Attr.ta_Name;
        /* two MOVE.l: ta_YSize/ta_Style/ta_Flags, then the pens, the draw
           mode and the byte behind it */
        *(ULONG *)&fstruct.size = *(const ULONG *)&fo->fo_Attr.ta_YSize;
        *(ULONG *)&fstruct.pen1 = *(const ULONG *)&fo->fo_FrontPen;
        len = (ULONG)-1;
        do
            len++;
        while ((*dst++ = *name++) != 0);
        fstruct.len = len;
    }
    FREEREQ(aslstruct);
    /* the test is on d7, the name length after a request: an empty font
       name returns 0 as well - not a bug (an empty name is no font
       OpenDiskFont could open; the program sees a cancelled request) */
    if (!ok || !len)
        return 0;
    return (ULONG)&fstruct;
}

/* ASLScreenRequest(flags) - BUG(orig) (#60): returns the freed requester */
ULONG BLITZ_ROUTINE doaslscreen(ULONG flags BL_REG(d0), struct Screen **scr BL_REG(a1))
{
    ULONG ok;                           /* d7 */
    int i;

    for (i = 0; i < 5; i++)
        stags.dotags[i].ti_Data = (flags & (1UL << i)) ? (ULONG)-1 : 0;
    stags.screen.ti_Data = (ULONG)*scr;
    if (!aslbase)
        return 0;
    aslstruct = AllocAslRequest(ASL_ScreenModeRequest, (struct TagItem *)&stags);
    if (!aslstruct)
        return 0;
    ok = AslRequest(aslstruct, (struct TagItem *)&stags);
    FREEREQ(aslstruct);
    if (!ok)
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
    ULONG len, filelen;                 /* d7: drawer, then the whole; d6 */

    if (!aslbase)
        return title;                   /* BUG(orig) (#62): d0 unchanged */
    Tags.screen.ti_Data = (ULONG)*scr;
    blitz_string = (UBYTE *)title;      /* the result goes here */
    Tags.name.ti_Data = allocst(title, lens[-1]);
    Tags.path.ti_Data = path;
    Tags.file.ti_Data = file;
    aslstruct = AllocAslRequest(ASL_FileRequest, (struct TagItem *)&Tags);
    if (!aslstruct || !AslRequest(aslstruct, (struct TagItem *)&Tags)) {
        FREEREQ(aslstruct);      /* filefail (also with 0) */
        *blitz_string = 0;
        Tags.flags2.ti_Data = 0;
        return 0;
    }
    fr = aslstruct;
    len = copyout((const UBYTE *)fr->fr_Drawer, (UBYTE *)path);
    *(ULONG *)(path - 4) = len;
    if (len && blitz_string[-1] != ':') {
        *blitz_string++ = '/';
        len = (UWORD)(len + 1);
    }
    filelen = copyout((const UBYTE *)fr->fr_File, (UBYTE *)file);
    *(ULONG *)(file - 4) = filelen;
    len += filelen;
    FREEREQ(aslstruct);
    Tags.flags2.ti_Data = 0;
    return len;
}

static void dims(ULONG x, ULONG y, ULONG w, ULONG h)
{
    Tags.x.ti_Data = x;
    Tags.y.ti_Data = y;
    Tags.w.ti_Data = w;
    Tags.h.ti_Data = h;
}

/* the C entry functions get all registers they may use as parameters
   (arg0-arg5 = d0-d5, their meaning depends on the form);
   the token table points to them (+1: Amiga mode only) */
#define FREQ_ARGS ULONG arg0 BL_REG(d0), ULONG arg1 BL_REG(d1), ULONG arg2 BL_REG(d2),   \
    ULONG arg3 BL_REG(d3), ULONG arg4 BL_REG(d4), ULONG arg5 BL_REG(d5),                 \
    struct Screen **scr BL_REG(a1), const ULONG *lens BL_REG(a2)

/* title, path, file */
static ULONG BLITZ_ROUTINE freqa_c(FREQ_ARGS)
{
    Tags.flags.ti_Data = 0;
    dims(20, 20, 280, 160);
    return aslreq(arg0, arg1, arg2, scr, lens);
}

/* title, path, file, pattern (-8(a2) = its length) */
static ULONG BLITZ_ROUTINE freqb_c(FREQ_ARGS)
{
    Tags.pattern.ti_Data = allocst(arg3, lens[-2]);
    Tags.flags.ti_Data = 1;                  /* FRF_DOPATTERNS */
    dims(20, 20, 280, 160);
    return aslreq(arg0, arg1, arg2, scr, lens);
}

/* title, path, file, x, y, w, h (-8(a2)) */
static ULONG BLITZ_ROUTINE freqc_c(FREQ_ARGS)
{
    Tags.flags.ti_Data = 0;
    dims(arg3, arg4, arg5, lens[-2]);
    return aslreq(arg0, arg1, arg2, scr, lens);
}

/* title, path, file, pattern, x, y, w (-12(a2)), h (-16(a2)) */
static ULONG BLITZ_ROUTINE freqd_c(FREQ_ARGS)
{
    Tags.pattern.ti_Data = allocst(arg3, lens[-2]);
    Tags.flags.ti_Data = 1;
    dims(arg4, arg5, lens[-3], lens[-4]);
    return aslreq(arg0, arg1, arg2, scr, lens);
}

/* ASLPathRequest$ title, path - d2 is no argument (BUG(orig), #61) */
static ULONG BLITZ_ROUTINE freqe_c(FREQ_ARGS)
{
    Tags.flags2.ti_Data = FRF_DRAWERSONLY_;
    dims(20, 20, 280, 160);             /* flags stay from the last call */
    return aslreq(arg0, arg1, arg2, scr, lens);
}

/* ASLPathRequest$ title, path, x, y, w, h */
static ULONG BLITZ_ROUTINE freqf_c(FREQ_ARGS)
{
    Tags.flags.ti_Data = 0;
    dims(arg2, arg3, arg4, arg5);
    Tags.flags2.ti_Data = FRF_DRAWERSONLY_;
    return aslreq(arg0, arg1, arg2, scr, lens);
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
