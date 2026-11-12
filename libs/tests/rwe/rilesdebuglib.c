/*
 * rwe rilesdebuglib - the LES debug library (LESDEBUGlib.bb of
 * 1996-12-01): the C build or the reference build of the source
 * (refbuild), one set as LIBSET says, observed as cases (rwecmp.h,
 * LIBS_ONE_SET; the expected data libs/tests/expected/rwe_rilesdebuglib.txt).
 *
 *   rilesdebuglib DIR
 *
 * The library does not use blitz2's runtime debugger; all of it is
 * called directly.  RawStatus (blitzkeyslib, polled by SearchMem) is a
 * stub (sysload_stubs) that counts its calls and answers a chosen value.
 * Memory the routines read is one block (results that are addresses in
 * it are written as offsets, and so are the addresses in the vectors);
 * what they write goes to a buffer.
 * Covered: init; D_GetSR (all 5 flag bits); D_HexWord (even and odd
 * source, words from below $420 and from $FFFFFFF0); D_CheckNMove,
 * D_CheckNMoveA, D_CheckNMoveWord (both forms, zero bytes, below $420);
 * D_FindStringPtr (found, the long at areg5 itself not compared, not
 * found); GetCopIns$ (MOVE to several registers, WAIT/SKIP with odd
 * position digits, with and without BFD); the font printer FLocate,
 * FPrint, FNPrint, FInverse (word 1 and $100), FYOffset, the empty
 * string, the right edge and columns from 208 on (BUG(orig) (#707)), observed on
 * both planes; FPrintDetoke with marker types 0/1, several cio and
 * lineoffset values, tokens drawn into one or two planes, an unknown
 * token, no token list, no plane 1, the inverse word's high byte set
 * (BUG(orig) (#709): the second plane's glyph), the right edge inside a token;
 * GetProgMode; SearchMem with every type (also invalid ones), found at
 * the start, inside, across the end (word/long), start = end, the string
 * search after a partial match (BUG(orig) (#723)), the empty string, the space
 * bar test (RawStatus every 127 steps; abort with 1) and SearchMemFound;
 * PeekMax$ (max -1..100, BUG(orig) (#718) length); GetHex$/GetHexAdr$ (all
 * modes); GetCustomReg for every register word $000-$1FD, $1FE and above,
 * a negative long; GetCustomRegAdr for every name, prefixes, 8 character
 * names, NOOP, an empty name; SetupLESDebug (%c) and ProcessString with
 * %p, %c, %%, an unknown %x and AddProcessPtr/ClearProcessPtrs entries;
 * ExtractLabel; D_InitParse/D_NextArg$ (empty arguments, the end, after
 * it); OutText/OutTextPen on a RastPort of the test (planes, cp_x/cp_y,
 * the pen); FindLineToken; GetDataItem (sizes -1..5, below $420,
 * $80000000, size 0: 65536 bytes); D_FindVolume (a device, a volume, an
 * assign, a missing name); finit.
 * Not covered: counts of 0 for D_HexWord/D_CheckNMove* (65536 entries
 * into the destination); GetCustomReg with a negative word but a positive
 * long (reads in front of the table: DIFF); more than 20 AddProcessPtr
 * entries and ProcessString with no parser position or a '%' at the end
 * (they read or write beyond the library's data); the real debugger
 * program that uses the library.
 */
#define LIBS_ONE_SET
#include "rwecmp.h"
#include <proto/dos.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <dos/dosextens.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME lesgfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

static struct Library *lesgfx;

#define LIB "rilesdebuglib"
enum { E_FINDVOL, E_GETSR, E_HEXWORD, E_CHECKNMOVE, E_FINDSTRPTR, E_COPINS, E_FLOCATE,
       E_FPRINT, E_FNPRINT, E_FINVERSE, E_PROGMODE, E_FYOFFSET, E_SEARCHMEM, E_SEARCHFOUND,
       E_PEEKMAX, E_CHECKNMOVEWORD, E_GETHEX, E_GETHEXADR, E_CUSTOMREG, E_CUSTOMREGADR,
       E_FPDETOKE, E_SETUP, E_EXTRACT, E_OUTTEXT, E_CHECKNMOVEA, E_OUTTEXTPEN, E_INITPARSE,
       E_NEXTARG, E_PROCESS, E_ADDPROC, E_CLEARPROC, E_FINDLINETOKEN, E_GETDATAITEM,
       E_FINIT, E_INIT };

/* the memory the routines read */
#define SHAREDSIZE 70000
static UBYTE *shared;
#define SH(off) ((ULONG)shared + (off))
/* an address in the shared area */
#define ISSH(v) ((ULONG)(v) - SH(0) < SHAREDSIZE)
/* a result in the shared area by its offset: the area's address differs
   with the memory layout */
#define SHREL(v) ((ULONG)(v) - SH(0) < SHAREDSIZE ? 0x80000000UL | ((ULONG)(v) - SH(0)) \
                  : (ULONG)(v))
/* areas of it */
#define A_DATA     0            /* 4096 bytes of a pattern */
#define A_SEARCH   4096         /* the SearchMem area, 1024 bytes */
#define A_TEXT     5120         /* strings, lines, the token list */
#define A_FINDPTR  8192         /* D_FindStringPtr: zeros and two longs */

static UBYTE out[800];
static ULONG frame[4];

/* a string result at out+4 */
#define RESSETUP() do {                                                     \
        memset(out, 0xee, sizeof out);                              \
        R.a[3] = (ULONG)out + 4;                                  \
    } while (0)

#define STRRES() do {                                                       \
        OBS("d0", R.d[0]);                                            \
        OBS("a3", R.a[3] - (ULONG)out);                          \
        OBSBYTES("out", out, sizeof out);                               \
    } while (0)

/* a string argument at out+600 (where the compiler built it), a3 behind
   it, its length at -4(a2) */
#define STRARG(s) do {                                                      \
        strcpy((char *)out + 600, s);                                  \
        R.d[0] = (ULONG)out + 600;                                \
        frame[0] = strlen(s);                                          \
        R.a[2] = (ULONG)&frame[1];                                \
        R.a[3] = (ULONG)out + 600 + strlen(s) + 1;                \
    } while (0)

/* --------------------------------------------------------- the RawStatus stub */

static ULONG raw_ret, raw_calls, raw_key;

static ULONG __attribute__((used)) raw_c(ULONG key)
{
    raw_calls++;
    raw_key = key;
    return raw_ret;
}

/* RawStatus keeps everything but d0 (the routines rely on a1) */
void raw_stub(void);
__asm__("   .text\n"
        "   .even\n"
        "_raw_stub:\n"
        "   movem.l d1-d7/a0-a6,-(sp)\n"
        "   move.l  d0,-(sp)\n"
        "   jsr     _raw_c\n"
        "   addq.l  #4,sp\n"
        "   movem.l (sp)+,d1-d7/a0-a6\n"
        "   rts\n");

static const struct stub stubs[] = { { 0x3102, (APTR)raw_stub }, { 0, NULL } };

/* ------------------------------------------------- registers and memory */

static void getsr(void)
{
    static const UBYTE srs[] = { 0, 1, 2, 4, 8, 16, 31, 0x15, 0x0a, 0xe5, 0xff };
    unsigned i;

    ROUT = "D_GetSR";
    for (i = 0; i < sizeof srs; i++) {
        sprintf(VEC, "$%02X", srs[i]);
        DO1(ENT(LIB, E_GETSR, 0), memset(out, 0xee, sizeof out);
            R.d[0] = (ULONG)out + 4; R.d[1] = srs[i]);
        OBSBYTES("buffer", out, sizeof out);
    }
}

/* a call of a (source,dest,count) statement: entry e, form f */
static void move3(int e, int f, ULONG src, UWORD count)
{
    /* the vector names the source as an offset in the shared area (an address
       is no part of a key), else as its value */
    sprintf(VEC, "$%lX%s,%u", ISSH(src) ? src - SH(0) : src, ISSH(src) ? "+sh" : "", count);
    DO1(ENT(LIB, e, f), memset(out, 0xee, sizeof out);
        R.d[0] = src; R.d[1] = (ULONG)out + 4; R.d[2] = count);
    OBSBYTES("dest", out, sizeof out);
}

static void hexword(void)
{
    static const ULONG low[][2] = { { 0x41c, 4 }, { 0x41b, 3 }, { 0x41f, 2 }, { 0xfffffff0UL, 2 } };
    unsigned i, n;

    ROUT = "D_HexWord";
    for (n = 1; n <= 8; n++) {
        move3(E_HEXWORD, 0, SH(100), n);
        move3(E_HEXWORD, 0, SH(301), n);
    }
    for (i = 0; i < sizeof low / sizeof low[0]; i++)
        move3(E_HEXWORD, 0, low[i][0], low[i][1]);
}

static void checknmove(void)
{
    static const struct { const char *name; int e; } r[] = {
        { "D_CheckNMove", E_CHECKNMOVE }, { "D_CheckNMoveA", E_CHECKNMOVEA },
        { "D_CheckNMoveWord", E_CHECKNMOVEWORD },
    };
    unsigned i, n;

    for (i = 0; i < 3; i++) {
        ROUT = r[i].name;
        move3(r[i].e, 0, SH(200), 0);
        move3(r[i].e, 0, SH(203), 0);
        for (n = 1; n <= 5; n++) {
            move3(r[i].e, 1, SH(200), n);
            move3(r[i].e, 1, SH(211), n);
        }
        if (r[i].e != E_CHECKNMOVE) {
            move3(r[i].e, 1, 0x41a, 3);
            move3(r[i].e, 1, 0xfffffff0UL, 2);
            move3(r[i].e, 0, 0x41e, 0);
        }
    }
}

static void findstringptr(void)
{
    static const ULONG look[] = { 0xcafebabeUL, 0x11223344UL, 0x13572468UL, 0 };
    unsigned i;

    ROUT = "D_FindStringPtr";
    for (i = 0; i < sizeof look / sizeof look[0]; i++) {
        sprintf(VEC, "$%08lX", look[i]);
        DO1(ENT(LIB, E_FINDSTRPTR, 0), R.d[0] = SH(A_FINDPTR + 33000);
            R.d[1] = look[i]);
        OBS("d0", SHREL(R.d[0]));
    }
}

static void peekmax(void)
{
    static const WORD max[] = { -1, 0, 1, 2, 5, 11, 12, 100 };
    unsigned i;

    ROUT = "PeekMax$";
    for (i = 0; i < sizeof max / sizeof max[0]; i++) {
        sprintf(VEC, "%d", max[i]);
        DO1(ENT(LIB, E_PEEKMAX, 0), RESSETUP(); R.d[0] = SH(A_TEXT);
            R.d[1] = (UWORD)max[i]);
        STRRES();
    }
}

static void gethex(void)
{
    static const ULONG v[] = { 0, 0x12345678UL, 0xfedcba98UL, 0xff, 0x80, 0x7fff8001UL };
    static const UWORD modes[] = { 0, 1, 2, 3, 4, 0x101, 0xff02 };
    static const ULONG adr[] = { 100, 101, 102, 2000 };
    unsigned i, m;

    for (i = 0; i < sizeof v / sizeof v[0]; i++)
        for (m = 0; m < sizeof modes / sizeof modes[0]; m++) {
            ROUT = "GetHex$";
            sprintf(VEC, "$%08lX,%u", v[i], modes[m]);
            DO1(ENT(LIB, E_GETHEX, 0), RESSETUP(); R.d[0] = v[i]; R.d[1] = modes[m]);
            STRRES();
        }
    for (i = 0; i < sizeof adr / sizeof adr[0]; i++)
        for (m = 0; m < sizeof modes / sizeof modes[0]; m++) {
            if ((adr[i] & 1) && (UBYTE)modes[m] != 1)
                continue;                               /* word/long: even */
            ROUT = "GetHexAdr$";
            sprintf(VEC, "sh+%lu,%u", adr[i], modes[m]);
            DO1(ENT(LIB, E_GETHEXADR, 0), RESSETUP(); R.d[0] = SH(adr[i]);
                R.d[1] = modes[m]);
            STRRES();
        }
}

static void getdataitem(void)
{
    static const WORD sizes[] = { 1, 2, 3, 4, 5, 0x7fff, 0, -1 };
    static const ULONG adr[] = { 0x100, 0x41f, 0x80000000UL };
    unsigned i;

    ROUT = "GetDataItem";
    for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        sprintf(VEC, "sh+101,%d", sizes[i]);
        DO1(ENT(LIB, E_GETDATAITEM, 0), R.d[0] = SH(101); R.d[1] = (UWORD)sizes[i]);
        OBS("d0", R.d[0]);
    }
    for (i = 0; i < sizeof adr / sizeof adr[0]; i++) {
        sprintf(VEC, "$%lX,4", adr[i]);
        DO1(ENT(LIB, E_GETDATAITEM, 0), R.d[0] = adr[i]; R.d[1] = 4);
        OBS("d0", R.d[0]);
    }
}

static void progmode(void)
{
    static const WORD modes[] = { -1, 0, 1, (WORD)0x8000, 0x7fff };
    unsigned i;

    ROUT = "GetProgMode";
    for (i = 0; i < sizeof modes / sizeof modes[0]; i++) {
        sprintf(VEC, "%d", modes[i]);
        DO1(ENT(LIB, E_PROGMODE, 0), memset(out, 0xee, sizeof out);
            R.d[0] = (UWORD)modes[i]; R.d[1] = (ULONG)out + 4);
        OBSBYTES("buffer", out, sizeof out);
    }
}

/* ------------------------------------------------- copper and registers */

static void copins(void)
{
    static const UWORD ins[][2] = {
        { 0x0180, 0x0fff }, { 0x0100, 0x5200 }, { 0x01fe, 0 }, { 0x003e, 0x1234 },
        { 0x00e0, 0x0007 }, { 0x0106, 0x0c40 }, { 0x2c01, 0xfffe }, { 0x2c01, 0xff00 },
        { 0x5bd1, 0x7ffe }, { 0x0001, 0x8001 }, { 0xffdf, 0xfffe }, { 0xab35, 0xcdef },
        { 0xab35, 0x4dee }, { 0x1f3f, 0x8000 },
    };
    unsigned i;

    ROUT = "GetCopIns$";
    for (i = 0; i < sizeof ins / sizeof ins[0]; i++) {
        sprintf(VEC, "$%04X,$%04X", ins[i][0], ins[i][1]);
        DO1(ENT(LIB, E_COPINS, 0), RESSETUP(); R.d[0] = (ULONG)ins[i]);
        STRRES();
    }
}

static void customreg(void)
{
    static const ULONG extra[] = { 0x1fe, 0x1ff, 0x200, 0x7fff, 0xffff0010UL, 0x80000000UL };
    ULONG r;
    unsigned i;

    ROUT = "GetCustomReg";
    for (r = 0; r <= 0x1fd; r++) {
        sprintf(VEC, "$%03lX", r);
        DO1(ENT(LIB, E_CUSTOMREG, 0), RESSETUP(); R.d[0] = r);
        STRRES();
    }
    for (i = 0; i < sizeof extra / sizeof extra[0]; i++) {
        sprintf(VEC, "$%08lX", extra[i]);
        DO1(ENT(LIB, E_CUSTOMREG, 0), RESSETUP(); R.d[0] = extra[i]);
        STRRES();
    }
}

/* the names of GetCustomReg's table, as GetCustomRegAdr's arguments */
static void customregadr1(const char *name)
{
    ROUT = "GetCustomRegAdr";
    sprintf(VEC, "\"%.40s\"", name);
    DO1(ENT(LIB, E_CUSTOMREGADR, 0), memset(out, 0, sizeof out); STRARG(name));
    OBS("d0", R.d[0]);
}

static void customregadr(void)
{
    static const char *extra[] = {
        "DMACONXYZ", "DMACON", "DMA", "", "dmacon", "COLOR00", "BLTCON0L", "BLTCON0LBLTSIZV",
        "DENISEID", "NOOP", "FMODE", "<UNUSED>", "STR", "STRX", "BPLCON0 ", "SPR7DATB",
    };
    char name[12];
    ULONG r;
    unsigned i;

    for (r = 0; r <= 0x1fe; r += 2) {
        /* the name GetCustomReg gives (up to the space) */
        DO1(ENT(LIB, E_CUSTOMREG, 0), RESSETUP(); R.d[0] = r);
        memcpy(name, out + 4, 8);
        name[8] = 0;
        for (i = 0; i < 8 && name[i] != ' ' && name[i] != (char)0xee; i++)
            ;
        name[i] = 0;
        customregadr1(name);
    }
    for (i = 0; i < sizeof extra / sizeof extra[0]; i++)
        customregadr1(extra[i]);
}

/* -------------------------------------------------------- the printer */

#define PLANESIZE (80 * 80)
static UBYTE planes[2][PLANESIZE];
static struct BitMap bm;

static void clearplanes(void)
{
    memset(planes, 0, sizeof planes);
}

static void sameplanes(void)
{
    OBSBYTES("plane 0", planes[0], PLANESIZE);
    OBSBYTES("plane 1", planes[1], PLANESIZE);
}

static void flocate(UWORD x, UWORD y)
{
    ROUT = "FLocate";
    sprintf(VEC, "%u,%u", x, y);
    DO1(ENT(LIB, E_FLOCATE, 0), R.d[0] = x; R.d[1] = y);
}

static void fword(int e, const char *name, UWORD v)
{
    ROUT = name;
    sprintf(VEC, "$%04X", v);
    DO1(ENT(LIB, e, 0), R.d[0] = v);
}

static void fprint(int e, const char *s)
{
    ROUT = e == E_FPRINT ? "FPrint" : "FNPrint";
    sprintf(VEC, "\"%.40s\"", s);
    DO1(ENT(LIB, e, 0), STRARG(s); R.a[0] = (ULONG)&bm);
    sameplanes();
}

static void printer(void)
{
    static char all[129];
    int i;

    for (i = 0; i < 128; i++)
        all[i] = (char)(i + 1);
    clearplanes();
    flocate(0, 0);
    fprint(E_FPRINT, "Hello");
    fprint(E_FPRINT, " World");
    fprint(E_FNPRINT, "!");
    fprint(E_FPRINT, "next row");
    fprint(E_FPRINT, "");
    fword(E_FINVERSE, "FInverse", 1);
    fprint(E_FPRINT, "inverse");
    fword(E_FINVERSE, "FInverse", 0x100);
    fprint(E_FNPRINT, " high byte");
    fword(E_FINVERSE, "FInverse", 0);
    fword(E_FYOFFSET, "FYOffset", 3);
    flocate(75, 2);
    fprint(E_FPRINT, "0123456789");
    fprint(E_FPRINT, "x");
    fword(E_FYOFFSET, "FYOffset", 0);
    flocate(0, 3);
    fprint(E_FPRINT, all);
    fprint(E_FPRINT, "\x80\x81\xa0\xc4\xe9\xff");
    flocate(210, 4);
    fprint(E_FPRINT, "from column 210 on, the byte compare lets it print on and on and on "
                     "through the next row up to column 80");
    flocate(79, 7);
    fprint(E_FNPRINT, "ab");
    fprint(E_FPRINT, "row 8");
}

/* the token list: nodes { next.l, token.w, name } */
static ULONG *toklist;

static void buildtokens(void)
{
    static const struct { UWORD tok; const char *name; } t[] = {
        { 0x0001, "Print" }, { 0x0123, "If" }, { 0x7fff, "Last" }, { 0x0a80, "Then" },
        { 0x1234, "VeryLongTokenName" },
    };
    UBYTE *p = shared + A_TEXT + 512;
    ULONG *prev = NULL;
    unsigned i;

    for (i = 0; i < sizeof t / sizeof t[0]; i++) {
        ULONG *n = (ULONG *)p;
        n[0] = 0;
        *(UWORD *)(p + 4) = t[i].tok;
        strcpy((char *)p + 6, t[i].name);
        p += (6 + strlen(t[i].name) + 1 + 1) & ~1;
        if (prev)
            prev[0] = (ULONG)n;
        else
            toklist = n;
        prev = n;
    }
}

static void setup(ULONG list, UBYTE tokenise)
{
    ROUT = "SetupLESDebug";
    sprintf(VEC, "%s,%u", list ? "list" : "0", tokenise);
    DO1(ENT(LIB, E_SETUP, 0), R.d[0] = list; R.d[1] = 0xff00 | tokenise);
}

static void detoke(const UBYTE *line, UWORD cio, UWORD typ, UWORD loff)
{
    ROUT = "FPrintDetoke";
    sprintf(VEC, "line %02X%02X..,%u,%u,%u", line[0], line[1], cio, typ, loff);
    DO1(ENT(LIB, E_FPDETOKE, 0), R.d[0] = (ULONG)line; R.d[1] = cio;
        R.d[2] = typ; R.d[3] = loff; R.a[0] = (ULONG)&bm);
    sameplanes();
}

static void fprintdetoke(void)
{
    static const UBYTE l1[] = "\x80\x01 \"x\":\x81\x23 a=1 \x8a\x80 \xff\xff";
    static const UBYTE l2[] = "a\x80\x01\x92\x34 b";
    static const UBYTE l3[] = "\x80\x05?";
    static const UBYTE l4[] = "";
    static const UBYTE *lines[] = { l1, l2, l3, l4 };
    static const UWORD cios[] = { 0, 1, 3, 4, 9, 100 };
    static const UWORD loffs[] = { 0, 1, 2, 7, 30 };
    unsigned t, l, c, o, typ;
    UWORD row = 0;

    buildtokens();
    for (t = 0; t < 2; t++) {
        setup((ULONG)toklist, (UBYTE)t);
        for (l = 0; l < 4; l++)
            for (c = 0; c < sizeof cios / sizeof cios[0]; c++)
                for (o = 0; o < sizeof loffs / sizeof loffs[0]; o++)
                    for (typ = 0; typ < 2; typ++) {
                        if (!row)
                            clearplanes();
                        flocate((UWORD)(o * 3), row);
                        detoke(lines[l], cios[c], (UWORD)(typ ? 0x0101 : 0xff00), loffs[o]);
                        row = (row + 1) % 10;
                    }
    }
    /* the right edge, inside a token and in front of it */
    clearplanes();
    setup((ULONG)toklist, 1);
    flocate(74, 0);
    detoke(l2, 0, 0, 0);
    flocate(77, 1);
    detoke(l1, 0, 1, 0);
    flocate(79, 2);
    detoke(l1, 2, 0, 0);
    /* the inverse word: 1 (not tested by FPrintDetoke), $100 (all
       inverted; the second plane gets another glyph, BUG(orig) (#709)) */
    fword(E_FINVERSE, "FInverse", 1);
    flocate(0, 3);
    detoke(l1, 3, 0, 0);
    fword(E_FINVERSE, "FInverse", 0x100);
    flocate(0, 4);
    detoke(l1, 3, 0, 0);
    flocate(0, 5);
    fprint(E_FPRINT, "after");
    fword(E_FINVERSE, "FInverse", 0);
    /* no token list: every token "?????"; no plane 1 */
    setup(0, 1);
    flocate(0, 6);
    detoke(l1, 1, 0, 0);
    bm.Planes[1] = NULL;
    setup((ULONG)toklist, 1);
    flocate(0, 7);
    detoke(l2, 2, 1, 1);
    bm.Planes[1] = planes[1];
    sameplanes();
}

/* OutText/OutTextPen on a RastPort of the test; its planes in chip memory:
   Text() draws with the blitter (graphics.library 40 of Workbench 3.1
   draws nothing into planes in fast memory) */
#define RPPLANE (80 * 40)
static void outtext(void)
{
    static const char *texts[] = { "Hello", "", "A longer line of text" };
    UBYTE (*rpplanes)[RPPLANE] = AllocMem(sizeof(UBYTE[2][RPPLANE]), MEMF_CHIP | MEMF_CLEAR);
    static struct BitMap rbm;
    static struct RastPort rp;
    unsigned i;

    check("rilesdebuglib: the RastPort's planes in chip memory", rpplanes != NULL);
    if (!rpplanes)
        return;
    InitBitMap(&rbm, 2, 640, 40);
    rbm.Planes[0] = rpplanes[0];
    rbm.Planes[1] = rpplanes[1];
    InitRastPort(&rp);
    rp.BitMap = &rbm;
    SetDrMd(&rp, JAM1);
    for (i = 0; i < 3; i++) {
        ROUT = "OutTextPen";
        sprintf(VEC, "%u", i + 1);
        DO1(ENT(LIB, E_OUTTEXTPEN, 0), R.d[0] = (ULONG)&rp; R.d[1] = i + 1);
        OBS("FgPen", rp.FgPen);
        ROUT = "OutText";
        sprintf(VEC, "\"%s\"", texts[i]);
        DO1(ENT(LIB, E_OUTTEXT, 0), strcpy((char *)out + 600, texts[i]);
            R.d[0] = 10 + 40 * i; R.d[1] = 0xabcd0000UL | (3 + 9 * i);
            R.d[2] = (ULONG)&rp; R.d[3] = (ULONG)out + 600);
        OBS("cp_x", rp.cp_x);
        OBS("cp_y", rp.cp_y);
        OBSBYTES("plane 0", rpplanes[0], RPPLANE);
        OBSBYTES("plane 1", rpplanes[1], RPPLANE);
    }
    {
        ULONG n = 0;
        for (i = 0; i < RPPLANE; i++)
            n += rpplanes[0][i] != 0;
        BOTH("text drawn", n > 0);
    }
    FreeMem(rpplanes, sizeof(UBYTE[2][RPPLANE]));
}

/* ------------------------------------------------------------ SearchMem */

static void search1(UBYTE type, ULONG value, ULONG start, ULONG end, const char *what)
{
    ROUT = "SearchMem";
    sprintf(VEC, "%u,$%lX%s,%lu,%lu %s", type, ISSH(value) ? value - SH(0) : value,
            ISSH(value) ? "+sh" : "", start - SH(A_SEARCH), end - SH(A_SEARCH), what);
    raw_calls = 0;
    raw_key = 0;
    DO1(ENT(LIB, E_SEARCHMEM, 0), R.d[0] = 0xffffff00UL | type; R.d[1] = value;
        R.d[2] = start; R.d[3] = end);
    OBS("d0", SHREL(R.d[0]));
    OBS("RawStatus calls", raw_calls);
    OBS("RawStatus key", raw_key);
    ROUT = "SearchMemFound";
    DO1(ENT(LIB, E_SEARCHFOUND, 0), (void)0);
    OBS("d0", SHREL(R.d[0]));
}

static void searchmem(void)
{
    UBYTE *a = shared + A_SEARCH;
    ULONG s = SH(A_SEARCH);
    UBYTE *str = shared + A_TEXT + 256;
    int i;

    for (i = 0; i < 1024; i++)
        a[i] = (UBYTE)(i * 7 + (i >> 5));
    memcpy(a + 500, "xaab..needle.ab", 15);
    a[900] = 0x99;
    a[1022] = 0x12;
    a[1023] = 0x34;
    strcpy((char *)str, "needle");
    strcpy((char *)str + 16, "ab");
    strcpy((char *)str + 32, "xaab");
    str[48] = 0;
    str[49] = 'Q';
    strcpy((char *)str + 64, "notthere");

    raw_ret = 0;
    search1(1, a[0], s, s + 1024, "byte at start");
    search1(1, 0x99, s, s + 1024, "byte");
    search1(1, 0x99, s, s + 900, "byte at end (excluded)");
    search1(1, 0x99, s + 10, s + 10, "start = end");
    search1(1, 0xffffff99UL, s + 800, s + 1024, "byte, high bits");
    search1(2, a[300] << 8 | a[301], s, s + 1024, "word");
    search1(2, 0x9999, s, s + 1024, "word not found");
    search1(2, 0x1234, s, s + 1023, "word across the end");
    search1(2, 0xabcd0000UL | (a[11] << 8 | a[12]), s, s + 1024, "word, high bits");
    search1(3, (ULONG)a[600] << 24 | a[601] << 16 | a[602] << 8 | a[603], s, s + 1024, "long");
    search1(5, (ULONG)a[1020] << 24 | a[1021] << 16 | 0x1234, s, s + 1021, "long across");
    search1(6, 0xdeadbeefUL, s, s + 1024, "long not found");
    search1(4, (ULONG)str, s, s + 1024, "string");
    search1(4, (ULONG)str + 16, s, s + 1024, "string ab");
    search1(4, (ULONG)str + 16, s + 500, s + 520, "\"ab\" after \"aab\"");
    search1(4, (ULONG)str + 32, s, s + 1024, "string xaab");
    search1(4, (ULONG)str + 48, s, s + 1024, "empty string");
    search1(4, (ULONG)str + 64, s, s + 1024, "string not found");
    search1(0, 0, s, s + 1024, "type 0");
    search1(7, 0, s, s + 1024, "type 7");
    search1(0x81, 0, s, s + 1024, "type $81");
    raw_ret = 0xffff;
    search1(1, 0x99, s, s + 1024, "space bar down");
    search1(1, 0x99, s, s + 100, "space bar down, short");
    search1(4, (ULONG)str + 64, s, s + 1024, "space bar, string");
    raw_ret = 0;
}

/* ---------------------------------------------- tokens, labels, parsing */

static void findlinetoken(void)
{
    static const char *l[] = { "abc\x81\x23 x", "\x80\x01", "plain", "", "a\xff\xff" };
    UBYTE *p = shared + A_TEXT + 1024;
    unsigned i;

    ROUT = "FindLineToken";
    for (i = 0; i < sizeof l / sizeof l[0]; i++) {
        strcpy((char *)p, l[i]);
        sprintf(VEC, "%u", i);
        DO1(ENT(LIB, E_FINDLINETOKEN, 0), R.d[0] = (ULONG)p);
        /* a word result: the high word is p's (its address moves with the layout) */
        OBS("d0", (R.d[0] ^ (ULONG)p) >> 16 ? R.d[0]
                   : 0x50000000UL | (R.d[0] & 0xffff));
    }
}

static void extractlabel(void)
{
    static const char *l[] = {
        "012345678.label: x", "012345678 more.name end", "0123456789ab.x:y",
        "012345678..", "012345678.a b", ".nope.....still.found", "012345678.verylonglabelname",
    };
    UBYTE *p = shared + A_TEXT + 1100;
    unsigned i;

    ROUT = "ExtractLabel";
    for (i = 0; i < sizeof l / sizeof l[0]; i++) {
        memset(p, 0, 64);
        strcpy((char *)p, l[i]);
        sprintf(VEC, "\"%s\"", l[i]);
        DO1(ENT(LIB, E_EXTRACT, 0), RESSETUP(); R.d[0] = (ULONG)p);
        STRRES();
    }
}

static void nextarg(int n)
{
    int i;

    ROUT = "D_NextArg$";
    for (i = 0; i < n; i++) {
        sprintf(VEC, "%d", i);
        DO1(ENT(LIB, E_NEXTARG, 0), RESSETUP());
        STRRES();
    }
}

static void initparse(const char *s)
{
    ROUT = "D_InitParse";
    sprintf(VEC, "\"%s\"", s);
    DO1(ENT(LIB, E_INITPARSE, 0), strcpy((char *)out + 600, s);
        R.d[0] = (ULONG)out + 600);
}

static void process(const char *s)
{
    initparse(s);
    ROUT = "ProcessString";
    DO1(ENT(LIB, E_PROCESS, 0), RESSETUP());
    STRRES();
}

static void addproc(const char *ch, const char *str)
{
    ROUT = "AddProcessPtr";
    sprintf(VEC, "\"%s\",\"%s\"", ch, str);
    DO1(ENT(LIB, E_ADDPROC, 0), STRARG(ch); R.d[1] = (ULONG)str);
}

static void parsing(void)
{
    nextarg(1);                                         /* before D_InitParse */
    initparse("one,two,,three");
    nextarg(6);
    initparse("");
    nextarg(2);
    initparse("no commas here");
    nextarg(2);
    initparse(",");
    nextarg(3);
    setup(0x12ab89efUL, 0);
    process("Hello %p, list at $%c; 100%% done");
    process("%x unknown %y and %%%%");
    addproc("a", "<A>");
    addproc("bcd", "<B>");
    addproc("%", "<percent>");
    process("%a%b%c %d %%");
    ROUT = "ClearProcessPtrs";
    strcpy(VEC, "");
    DO1(ENT(LIB, E_CLEARPROC, 0), (void)0);
    process("%a %b %p");
    addproc("b", "<new b>");
    process("%a %b");
    initparse("x,y");
    nextarg(1);
    ROUT = "ProcessString";
    strcpy(VEC, "after D_NextArg$");
    DO1(ENT(LIB, E_PROCESS, 0), RESSETUP());
    STRRES();
}

/* ------------------------------------------------------- D_FindVolume */

static void findvolume(void)
{
    static const char *names[] = { "RAM", "Ram Disk", "SYS", "TEST", "C", "NOSUCHNAME", "" };
    unsigned i;

    ROUT = "D_FindVolume";
    for (i = 0; i < sizeof names / sizeof names[0]; i++) {
        sprintf(VEC, "\"%s\"", names[i]);
        DO1(ENT(LIB, E_FINDVOL, 0), strcpy((char *)out + 600, names[i]);
            R.d[0] = (ULONG)out + 600; R.a[6] = (ULONG)DOSBase);
        OBS("d0", R.d[0]);
    }
}

/* --------------------------------------------------------------- main */

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    int i;

    if (argc < 2) {
        printf("usage: rilesdebuglib DIR\n");
        return 20;
    }
    sysload_stubs = stubs;
    if (!load_set(argv[1], names)) {
        sysload_stubs = NULL;
        return 20;
    }
    sysload_stubs = NULL;
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("finit", d2-d7|a2|a3);
    KEEPS("D_GetSR", d0|d2-d7|a1-a3|a6);
    KEEPS("D_HexWord", d1|d3-d5|a3|a6);
    KEEPS("D_CheckNMove", d1|d3-d6|a2|a3|a6);
    KEEPS("D_CheckNMoveA", d1|d3|d4|a2|a3|a6);
    KEEPS("D_CheckNMoveWord", d1|d3|d4|a2|a3|a6);
    KEEPS("D_FindStringPtr", d1-d7|a1|a3|a6);
    KEEPS("PeekMax$", d3-d7|a1|a2|a6);
    KEEPS("GetHex$", d3-d6|a1|a2|a6);
    KEEPS("GetHexAdr$", d3-d6|a2|a6);
    KEEPS("GetDataItem", d2-d7|a1-a3|a6);
    KEEPS("GetProgMode", d0-d7|a2|a3|a6);
    KEEPS("GetCopIns$", d4-d7|a1|a6);
    KEEPS("GetCustomReg", d1-d7|a1|a2|a6);
    KEEPS("GetCustomRegAdr", d2-d7|a6);
    KEEPS("FLocate", d0-d7|a0-a3|a6);
    KEEPS("FInverse", d0-d7|a0-a3|a6);
    KEEPS("FYOffset", d0-d7|a0-a3|a6);
    KEEPS("FPrint", d3|d4|a6);
    KEEPS("FNPrint", d3|d4|a6);
    KEEPS("SetupLESDebug", d0|d3-d7|a2|a3|a6);
    KEEPS("FPrintDetoke", d4|a6);
    KEEPS("OutTextPen", d2-d7|a2|a3);
    KEEPS("OutText", d2-d7|a2|a3);
    KEEPS("SearchMem", d1|d4|d5|a0|a6);
    KEEPS("SearchMemFound", d1-d7|a0-a3|a6);
    KEEPS("FindLineToken", d1-d7|a1-a3|a6);
    KEEPS("ExtractLabel", d3-d7|a1|a2|a6);
    KEEPS("D_NextArg$", d2-d7|a1|a2|a6);
    KEEPS("D_InitParse", d0-d7|a2|a3|a6);
    KEEPS("ProcessString", d2-d7|a2|a6);
    KEEPS("AddProcessPtr", d0-d7|a2|a3|a6);
    KEEPS("ClearProcessPtrs", d0-d7|a0-a3|a6);
    KEEPS("D_FindVolume", d4-d6|a2|a3|a6);
    shared = AllocMem(SHAREDSIZE, MEMF_ANY | MEMF_CLEAR);
    lesgfx = OpenLibrary((CONST_STRPTR)"graphics.library", 0);
    if (!shared || !lesgfx) {
        check("rilesdebuglib: memory, graphics.library", FALSE);
        return 20;
    }
    for (i = 0; i < 4096; i++)
        shared[A_DATA + i] = (UBYTE)(i * 13 + (i >> 8));
    for (i = 200; i < 240; i += 3)
        shared[i] = 0;
    for (i = 65536; i < SHAREDSIZE; i++)
        shared[i] = (UBYTE)i;
    strcpy((char *)shared + A_TEXT, "PeekMax text");
    *(ULONG *)(shared + A_FINDPTR + 9000) = 0xcafebabeUL;
    *(ULONG *)(shared + A_FINDPTR + 33000) = 0x11223344UL;
    InitBitMap(&bm, 2, 640, 80);
    bm.Planes[0] = planes[0];
    bm.Planes[1] = planes[1];
    strcpy(VEC, "");

    ROUT = "init";
    DO1(ENT(LIB, E_INIT, 0), (void)0);

    getsr();
    hexword();
    checknmove();
    findstringptr();
    peekmax();
    gethex();
    getdataitem();
    progmode();
    copins();
    customreg();
    customregadr();
    printer();
    fprintdetoke();
    outtext();
    searchmem();
    findlinetoken();
    extractlabel();
    parsing();
    findvolume();

    ROUT = "finit";
    strcpy(VEC, "");
    DO1(ENT(LIB, E_FINIT, 0), (void)0);
    FreeMem(shared, SHAREDSIZE);
    CloseLibrary(lesgfx);
    return rwe_done(LIB);
}
