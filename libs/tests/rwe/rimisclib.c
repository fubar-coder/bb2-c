/*
 * rwe rimisclib - the miscellaneous commands of misclib2.bb: the C build
 * against the reference build of the source (refbuild), method of
 * rwecmp.h.
 *
 *   rimisclib DIR       (DIR with rimisclib and floatquicklib .obj/.orig)
 *
 * EasyRequestArgs is replaced (SetFunction) by a stub that records the
 * window, the EasyStruct with its strings and the argument array and
 * returns a chosen value; GetProgramDir/GetProgramName are replaced for
 * the fallback paths of ProgPath$; the font prefs are a file the test
 * writes to ENV:SYS/Font.Prefs, with ENV: assigned to a directory of its
 * own for that part (envprivate) - the system's IPrefs watches the real
 * one and keeps a broken file in use (Workbench 3.1).
 * Covered: init; TimerReset (the TOD after it); PortSigBit; WorkbenchHere
 * on a fake IntuitionBase with fake screens (found in the first/a later
 * screen, after a partial match, an empty title); EZOutput both forms;
 * EZRequest all four forms (with '|' in the body, the arguments of every
 * type: string, long, word, byte, quick also negative, float through
 * fltoqu with a6 = mathffp, the stack written back); EZRequestArgs with
 * the same types, EZSeparator, the stale end of a longer format text
 * (BUG(orig), #747); Flip$ with 2..250 characters and with 0 and 1 (BUG(orig) #751:
 * 65536 swaps, in a big buffer); InitArgParse/NextArg$/NextArgChar$ on
 * many lines (spaces, quotes, "", unterminated quotes, separators), both
 * before InitArgParse and after the end (address 0); ProgPath$/ProgDir$/
 * ProgName$ (normal, no program name: the task name, no program dir: the
 * current dir); FromBlitz with several task names; LtoS; ASCRaw for the
 * whole table; RawASC for every character of the table; the font prefs
 * (not open, the file missing, bad FORM/PREF/PRHD/FONT, a PRHD of another
 * size, all three types, a type found in the padding behind the chunks
 * and one not found, d0's high word, CloseFontPrefs twice); BInstr; finit
 * (the buffer freed).
 * Not covered: ClickWait (needs the mouse button); WorkbenchHere's
 * endless cases; ASCRaw/RawASC outside the table (they read the code
 * behind it, DIFF); EZRequestArgs without arguments; strings longer than
 * the buffers; OpenFontPrefs when AllocMem fails.
 */
#include "rwecmp.h"
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <exec/execbase.h>

#define LIB "rimisclib"
enum { E_TIMER = 3, E_SIGBIT, E_WB, E_EZ, E_FLIP, E_PARSEINIT, E_NEXTARG, E_EZOUT,
       E_EZARGS, E_EZSEP, E_CLICK, E_PATH, E_DIR, E_NAME, E_FROMBLITZ, E_LTOS, E_ASCRAW,
       E_RAWASC, E_OPENFP, E_CLOSEFP, E_FHEIGHT, E_FNAME, E_NEXTCHAR, E_BINSTR, E_FINIT,
       E_INIT };

static UBYTE out[2][400];
static ULONG frame[2][4];
static struct Library *mathffp;

/* a string result at out+4 */
#define RESSETUP() do {                                                     \
        memset(out[cur], 0xee, sizeof out[0]);                              \
        R[cur].a[3] = (ULONG)out[cur] + 4;                                  \
    } while (0)

#define STRRES() do {                                                       \
        SAME("d0", R[cur].d[0]);                                            \
        SAME("a3", R[cur].a[3] - (ULONG)out[cur]);                          \
        SAMEBYTES("out", out, sizeof out[0]);                               \
    } while (0)

/* a string argument at out+20 (where the compiler built it), a3 behind it,
   its length at -4(a2) */
#define STRARG(s) do {                                                      \
        memset(out[cur], 0xee, sizeof out[0]);                              \
        strcpy((char *)out[cur] + 20, s);                                   \
        R[cur].d[0] = (ULONG)out[cur] + 20;                                 \
        frame[cur][0] = strlen(s);                                          \
        R[cur].a[2] = (ULONG)&frame[cur][1];                                \
        R[cur].a[3] = (ULONG)out[cur] + 20 + strlen(s) + 1;                 \
    } while (0)

/* ------------------------------------------------------------ TimerReset */

static ULONG readtod(void)
{
    volatile UBYTE *c = (volatile UBYTE *)0xbfe001;
    ULONG h = c[0xa00], m = c[0x900], l = c[0x800];
    return h << 16 | m << 8 | l;
}

static void settod(void)
{
    volatile UBYTE *c = (volatile UBYTE *)0xbfe001;
    c[0xa00] = 0x70;
    c[0x900] = 0x11;
    c[0x800] = 0x22;
}

static void timer(void)
{
    ULONG tod[2];
    ROUT = "TimerReset";
    strcpy(VEC, "");
    DO2(ENT(LIB, E_TIMER, 0), if (cur) tod[0] = readtod(); settod());
    tod[1] = readtod();
    BOTH("TOD reset", tod[cur] < 20);
}

/* ------------------------------------------------- PortSigBit, WorkbenchHere */

static void sigbit(void)
{
    static UBYTE fakeport[2][40];
    struct MsgPort *mp = CreateMsgPort();
    ROUT = "PortSigBit";
    strcpy(VEC, "port");
    DO2(ENT(LIB, E_SIGBIT, 0), R[cur].d[0] = (ULONG)mp);
    SAME("d0", R[cur].d[0]);
    BOTH("= mp_SigBit", R[cur].d[0] == mp->mp_SigBit);
    DeleteMsgPort(mp);
    strcpy(VEC, "$A5");
    DO2(ENT(LIB, E_SIGBIT, 0), memset(fakeport[cur], 0x5a, sizeof fakeport[0]);
        fakeport[cur][15] = 0xa5; R[cur].d[0] = (ULONG)fakeport[cur]);
    SAME("d0", R[cur].d[0]);
}

static struct IntuitionBase fakeib;
static struct Screen fakescr[4];

static void wbhere(const char *const *titles, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        fakescr[i].Title = (UBYTE *)titles[i];
        fakescr[i].NextScreen = i + 1 < n ? &fakescr[i + 1] : NULL;
    }
    fakeib.FirstScreen = &fakescr[0];
    sprintf(VEC, "%d screens, first \"%s\"", n, titles[0]);
    DO2(ENT(LIB, E_WB, 0), R[cur].d[0] = (ULONG)&fakeib);
    SAME("d0", R[cur].d[0]);
}

static void workbenchhere(void)
{
    static const char *const a[] = { "Workbench Screen" };
    static const char *const b[] = { "Other", "My Workbench" };
    static const char *const c[] = { "WWorkbench", "Work bench", "xWorkbench" };
    /* a title ending in "W" or "Wo" would restart its screen for ever
       (BUG(orig), #762): the partial matches here end with a mismatch */
    static const char *const d[] = { "", "Wx", "Wox", "a Workbench b" };
    ROUT = "WorkbenchHere";
    wbhere(a, 1);
    wbhere(b, 2);
    wbhere(c, 3);
    wbhere(d, 4);
}

/* --------------------------------------------------------- EasyRequest */

static struct {
    ULONG called, win, idcmp, hasargs, es[2];
    UBYTE title[80], body[200], gad[40];
    ULONG args[16];
} rec[2];
static LONG ez_ret;

static void copystr(UBYTE *to, const UBYTE *s, int n)
{
    int i;
    if (!s) {
        strcpy((char *)to, "<null>");
        return;
    }
    for (i = 0; i < n - 1 && s[i]; i++)
        to[i] = s[i];
    to[i] = 0;
}

static LONG ez_stub(struct Window *w __asm("a0"), struct EasyStruct *es __asm("a1"),
                    ULONG *idcmp __asm("a2"), const ULONG *args __asm("a3"))
{
    rec[cur].called++;
    rec[cur].win = (ULONG)w;
    rec[cur].idcmp = (ULONG)idcmp;
    rec[cur].hasargs = args != NULL;
    rec[cur].es[0] = es->es_StructSize;
    rec[cur].es[1] = es->es_Flags;
    copystr(rec[cur].title, es->es_Title, sizeof rec[0].title);
    copystr(rec[cur].body, es->es_TextFormat, sizeof rec[0].body);
    copystr(rec[cur].gad, es->es_GadgetFormat, sizeof rec[0].gad);
    if (args)
        memcpy(rec[cur].args, args, sizeof rec[0].args);
    return ez_ret;
}

static void ezcheck(void)
{
    SAME("d0", R[cur].d[0]);
    BOTH("called once", rec[cur].called == 1);
    SAME("window", rec[cur].win);
    SAME("idcmp", rec[cur].idcmp);
    SAME("args", rec[cur].hasargs);
    SAME("es_StructSize", rec[cur].es[0]);
    SAME("es_Flags", rec[cur].es[1]);
    SAMEBYTES("title", ((UBYTE *[]){ rec[0].title, rec[1].title }), sizeof rec[0].title);
    SAMEBYTES("body", ((UBYTE *[]){ rec[0].body, rec[1].body }), sizeof rec[0].body);
    SAMEBYTES("gadget", ((UBYTE *[]){ rec[0].gad, rec[1].gad }), sizeof rec[0].gad);
    SAMEBYTES("arguments", ((ULONG *[]){ rec[0].args, rec[1].args }), sizeof rec[0].args);
}

/* the argument stack: built downwards from the top (a2), as the compiler
   pushes - the lengths of the fixed string arguments, then per repeated
   argument its type word and its value (a string: length, address) */
static UBYTE stk[2][256];
static UBYTE *sp_;
struct arg { UWORD type; ULONG v; const char *s; };

static void pushl(ULONG v) { sp_ -= 4; *(ULONG *)sp_ = v; }
static void pushw(UWORD v) { sp_ -= 2; *(UWORD *)sp_ = v; }

static void buildstk(const ULONG *lens, int nlens, const struct arg *a, int n)
{
    int i;
    memset(stk[cur], 0x77, sizeof stk[0]);
    sp_ = stk[cur] + sizeof stk[0];
    for (i = 0; i < nlens; i++)
        pushl(lens[i]);
    for (i = 0; i < n; i++) {
        pushw(a[i].type);
        if (a[i].type == 7) {
            pushl(strlen(a[i].s));
            pushl((ULONG)a[i].s);
        } else if (a[i].type == 1 || a[i].type == 2)
            pushw((UWORD)a[i].v);
        else
            pushl(a[i].v);
    }
    R[cur].a[2] = (ULONG)(stk[cur] + sizeof stk[0]);
}

static UBYTE body[2][120];

/* body text in a buffer of each set: the library changes '|' in it */
#define BODY(s) (strcpy((char *)body[cur], s), (ULONG)body[cur])

static void ezsetup(void)
{
    memset(&rec[cur], 0, sizeof rec[0]);
}

static const char t_title[] = "The title", t_gad[] = "Yes|No|Maybe", t_s1[] = "string arg",
                  t_s2[] = "";

static void ezreq_args(const char *vec, const char *b, const struct arg *a, int n)
{
    ULONG lens[3];
    strcpy(VEC, vec);
    lens[0] = strlen(t_title);
    lens[1] = strlen(b);
    lens[2] = strlen(t_gad);
    DO2(ENT(LIB, E_EZ, 3), ezsetup(); R[cur].d[0] = (ULONG)t_title; R[cur].d[1] = BODY(b);
        R[cur].d[2] = (ULONG)t_gad; buildstk(lens, 3, a, n);
        R[cur].d[7] = 0xabcd0000UL | n; R[cur].a[6] = (ULONG)mathffp);
    ezcheck();
    SAMEBYTES("stack", stk, sizeof stk[0]);
    SAMEBYTES("body buffer", body, sizeof body[0]);
}

static void ezargs(const char *vec, const struct arg *a, int n)
{
    strcpy(VEC, vec);
    DO2(ENT(LIB, E_EZARGS, 0), ezsetup(); buildstk(NULL, 0, a, n);
        R[cur].d[7] = 0x12340000UL | n; R[cur].a[6] = (ULONG)mathffp);
    ezcheck();
    SAMEBYTES("stack", stk, sizeof stk[0]);
}

static void ezsep(const char *s)
{
    ROUT = "EZSeparator";
    sprintf(VEC, "\"%s\"", s);
    DO2(ENT(LIB, E_EZSEP, 0), R[cur].d[0] = (ULONG)s);
    ROUT = "EZRequestArgs";
}

static void easyrequest(void)
{
    static const struct arg all[] = {
        { 7, 0, t_s1 }, { 3, 0x12345678, NULL }, { 2, 0xfffe, NULL }, { 1, 0x0041, NULL },
        { 4, 0x00051234, NULL }, { 4, 0xfffc8000UL, NULL }, { 5, 0xe0000042UL, NULL },
        { 5, 0x900000c2UL, NULL }, { 7, 0, t_s2 },
    };
    static const struct arg one[] = { { 3, 0xffffffffUL, NULL } };
    static const struct arg many[] = {
        { 3, 1, NULL }, { 3, 2, NULL }, { 3, 3, NULL }, { 3, 4, NULL }, { 3, 5, NULL },
        { 3, 6, NULL }, { 3, 7, NULL }, { 3, 8, NULL }, { 3, 9, NULL }, { 3, 10, NULL },
        { 3, 11, NULL }, { 3, 12, NULL }, { 3, 13, NULL }, { 3, 14, NULL }, { 3, 15, NULL },
        { 3, 16, NULL },
    };
    struct Library *ib = (struct Library *)IntuitionBase;
    APTR old;
    ULONG winobj[2][2];
    unsigned i;

    Forbid();
    old = SetFunction(ib, -588, (APTR)ez_stub);
    CacheClearU();
    Permit();

    ROUT = "EZOutput";
    strcpy(VEC, "none");
    DO2(ENT(LIB, E_EZOUT, 1), (void)0);

    ROUT = "EZRequest";
    ez_ret = 1;
    strcpy(VEC, "body$");
    DO2(ENT(LIB, E_EZ, 2), ezsetup(); R[cur].d[0] = BODY("Hello|World||end|"));
    ezcheck();
    BOTH("a3 = the arguments (0)", R[cur].a[3] == 0);
    SAMEBYTES("body buffer", body, sizeof body[0]);
    ez_ret = 0;
    strcpy(VEC, "body$,gadget$");
    DO2(ENT(LIB, E_EZ, 1), ezsetup(); R[cur].d[0] = BODY("Two|lines");
        R[cur].d[1] = (ULONG)t_gad);
    ezcheck();
    SAMEBYTES("body buffer", body, sizeof body[0]);
    ez_ret = 3;
    strcpy(VEC, "title$,body$,gadget$");
    DO2(ENT(LIB, E_EZ, 0), ezsetup(); R[cur].d[0] = (ULONG)t_title;
        R[cur].d[1] = BODY("no bars"); R[cur].d[2] = (ULONG)t_gad);
    ezcheck();

    ROUT = "EZOutput";
    strcpy(VEC, "win#");
    for (cur = 0; cur < 2; cur++)
        winobj[cur][0] = 0xcafe0042UL;
    DO2(ENT(LIB, E_EZOUT, 0), R[cur].a[0] = (ULONG)winobj[cur]);
    ROUT = "EZRequest";
    ez_ret = 2;
    strcpy(VEC, "body$ on the window");
    DO2(ENT(LIB, E_EZ, 2), ezsetup(); R[cur].d[0] = BODY(""));
    ezcheck();

    ez_ret = 5;
    ezreq_args("title$,body$,gadget$,var.. all types", "%s|%ld|%d|%d|%ld|%ld|%ld|%ld|%s",
               all, sizeof all / sizeof all[0]);
    /* the arguments as RawDoFmt reads them: string, long, word, word
       (the byte), the quicks' integer parts zero extended (BUG(orig), #750),
       the floats through fltoqu (3.5 -> 3, -2.25 -> Quick -3 + .75:
       65533), string */
    BOTH("arguments of all types",
         rec[cur].args[0] == (ULONG)t_s1 && rec[cur].args[1] == 0x12345678
         && rec[cur].args[2] == 0xfffe0041UL && rec[cur].args[3] == 5
         && rec[cur].args[4] == 0xfffc && rec[cur].args[5] == 3
         && rec[cur].args[6] == 0xfffd && rec[cur].args[7] == (ULONG)t_s2);
    BOTH("body with line feeds", !strcmp((char *)rec[cur].body,
                                         "%s\n%ld\n%d\n%d\n%ld\n%ld\n%ld\n%ld\n%s"));
    ezreq_args("one long", "value %ld", one, 1);
    ezreq_args("16 longs", "many", many, 16);
    for (i = 0; i < sizeof all / sizeof all[0]; i++) {
        char v[40];
        sprintf(v, "type %u alone", all[i].type);
        ezreq_args(v, "x", &all[i], 1);
    }

    ROUT = "EZRequestArgs";
    ez_ret = 7;
    ezargs("all types", all, sizeof all / sizeof all[0]);
    BOTH("body", !strcmp((char *)rec[cur].body, "%s\n%ld\n%d\n%d\n%ld\n%ld\n%ld\n%ld\n%s"));
    BOTH("gadget Ok, no title", !strcmp((char *)rec[cur].gad, "Ok")
         && !strcmp((char *)rec[cur].title, "<null>"));
    ezargs("one (stale end, BUG(orig) #747)", one, 1);
    ezsep(",");
    ezargs("separator ,", all, 4);
    ezsep("");
    ezargs("separator 0", all, 3);
    ezsep("|x");
    ezargs("separator | again", many, 16);
    for (i = 0; i < sizeof all / sizeof all[0]; i++) {
        char v[40];
        sprintf(v, "type %u alone", all[i].type);
        ezargs(v, &all[i], 1);
    }

    ROUT = "EZOutput";
    strcpy(VEC, "none again");
    DO2(ENT(LIB, E_EZOUT, 1), (void)0);
    ROUT = "EZRequest";
    strcpy(VEC, "body$ without window");
    DO2(ENT(LIB, E_EZ, 2), ezsetup(); R[cur].d[0] = BODY("x|y"));
    ezcheck();

    Forbid();
    SetFunction(ib, -588, old);
    CacheClearU();
    Permit();
}

/* ------------------------------------------------------------- Flip$ */

#define BIGSZ 140000
#define BIGAT 70000

static void flip(void)
{
    static const char *s[] = { "ab", "abc", "Hello World", "0123456789abcdef!" };
    UBYTE *big[2];
    char longstr[251];
    unsigned i;
    int k;

    ROUT = "Flip$";
    for (i = 0; i < sizeof s / sizeof s[0]; i++) {
        sprintf(VEC, "\"%s\"", s[i]);
        DO2(ENT(LIB, E_FLIP, 0), STRARG(s[i]));
        STRRES();
    }
    for (k = 0; k < 250; k++)
        longstr[k] = (char)('A' + k % 26);
    longstr[250] = 0;
    strcpy(VEC, "250 characters");
    DO2(ENT(LIB, E_FLIP, 0), STRARG(longstr));
    STRRES();

    /* 0 and 1 character: 65536 swaps around the string (BUG(orig), #751) */
    big[0] = AllocMem(BIGSZ, MEMF_ANY);
    big[1] = AllocMem(BIGSZ, MEMF_ANY);
    if (big[0] && big[1]) {
        for (k = 0; k < 2; k++) {
            sprintf(VEC, "%d characters (65536 swaps)", k);
            DO2(ENT(LIB, E_FLIP, 0), ({
                    ULONG j;
                    for (j = 0; j < BIGSZ; j++)
                        big[cur][j] = (UBYTE)(j * 7 + (j >> 8));
                    big[cur][BIGAT] = 'Q';
                    big[cur][BIGAT + k] = 0;
                    frame[cur][0] = k;
                    R[cur].a[2] = (ULONG)&frame[cur][1];
                    R[cur].d[0] = (ULONG)big[cur] + BIGAT;
                    R[cur].a[3] = (ULONG)big[cur] + BIGAT + k + 1;
                }));
            SAME("d0", R[cur].d[0]);
            SAME("a3", R[cur].a[3] - (ULONG)big[cur]);
            SAMEBYTES("memory", big, BIGSZ);
        }
    } else
        printf("rimisclib: no memory for Flip$ with 0/1 characters - skipped\n");
    if (big[0])
        FreeMem(big[0], BIGSZ);
    if (big[1])
        FreeMem(big[1], BIGSZ);
}

/* ------------------------------------------------------- arg parsing */

static void nextarg(int n)
{
    int k;
    for (k = 0; k < n; k++) {
        ROUT = "NextArg$";
        DO2(ENT(LIB, E_NEXTARG, 0), RESSETUP());
        STRRES();
    }
}

static void nextchar(UWORD c, int n)
{
    int k;
    for (k = 0; k < n; k++) {
        ROUT = "NextArgChar$";
        DO2(ENT(LIB, E_NEXTCHAR, 0), RESSETUP(); R[cur].d[0] = 0x55550000UL | c);
        STRRES();
    }
}

static void parseinit(const char *s)
{
    ROUT = "InitArgParse";
    sprintf(VEC, "'%s'", s);
    DO2(ENT(LIB, E_PARSEINIT, 0), R[cur].d[0] = (ULONG)s);
}

static void parsing(void)
{
    static const char *lines[] = {
        "", " ", "one", "one two  three ", "   lead", "LOAD \"Blitz2:\" TEST",
        "\"quoted arg\" x", "\"a\"\"b\" c", "\"ab\"cd e", "\"unterminated", "\"x\"",
        "\"a\"\"", "\"\"", "\"\" \"\"", "a\"b c\"d e", "\"a\"\"b\"\"c\" end", "\"\"\"\"",
        "tab\there x",
    };
    unsigned i;

    strcpy(VEC, "before InitArgParse");
    nextarg(1);
    nextchar(',', 1);                   /* reads address 0, BUG(orig) (#754) */
    for (i = 0; i < sizeof lines / sizeof lines[0]; i++) {
        parseinit(lines[i]);
        nextarg(6);
    }
    parseinit("one,two,,three");
    nextchar(',', 5);
    parseinit("a b,c d");
    nextarg(1);
    nextchar(',', 1);
    nextarg(3);
    nextchar(',', 2);                   /* after the end: address 0 */
    parseinit("x=1;y=2");
    nextchar('=', 1);
    nextchar(';', 1);
    nextchar(0x113d, 2);                /* the low byte '=' counts */
}

/* ----------------------------------------------- ProgPath$, FromBlitz */

static void prog(int e, const char *what)
{
    ROUT = e == E_PATH ? "ProgPath$" : e == E_DIR ? "ProgDir$" : "ProgName$";
    strcpy(VEC, what);
    DO2(ENT(LIB, e, 0), RESSETUP());
    STRRES();
}

static void progs(void)
{
    struct Task *me = FindTask(NULL);
    char *name = me->tc_Node.ln_Name;
    struct Library *dos = (struct Library *)DOSBase;
    APTR old;

    prog(E_DIR, "first (calls ProgPath$)");
    prog(E_NAME, "");
    prog(E_PATH, "");
    prog(E_DIR, "again");

    me->tc_Node.ln_Name = "rimisc task";
    Forbid();
    old = SetFunction(dos, -576, (APTR)rwe_fail0);      /* GetProgramName */
    CacheClearU();
    Permit();
    prog(E_PATH, "no program name: the task name");
    prog(E_NAME, "no program name");
    Forbid();
    SetFunction(dos, -576, old);
    old = SetFunction(dos, -600, (APTR)rwe_fail0);      /* GetProgramDir */
    CacheClearU();
    Permit();
    prog(E_PATH, "no program dir: the current dir");
    prog(E_DIR, "no program dir");
    Forbid();
    SetFunction(dos, -600, old);
    CacheClearU();
    Permit();
    me->tc_Node.ln_Name = name;
    prog(E_PATH, "normal again");
}

static void fromblitz(void)
{
    static const char *n[] = {
        "Blitz ][ Program Process", "Blitz ][ Program Proc", "Blitz ][ Program Prox",
        "Blitz ][ Program Pro", "Blitz", "", "blitz ][ Program Process",
        "Blitz ][ Program Process and more",
    };
    struct Task *me = FindTask(NULL);
    char *name = me->tc_Node.ln_Name;
    unsigned i;

    ROUT = "FromBlitz";
    strcpy(VEC, "the test's name");
    DO2(ENT(LIB, E_FROMBLITZ, 0), (void)0);
    SAME("d0", R[cur].d[0]);
    for (i = 0; i < sizeof n / sizeof n[0]; i++) {
        sprintf(VEC, "\"%s\"", n[i]);
        me->tc_Node.ln_Name = (char *)n[i];
        DO2(ENT(LIB, E_FROMBLITZ, 0), (void)0);
        me->tc_Node.ln_Name = name;
        SAME("d0", R[cur].d[0]);
        if (i < 2)
            BOTH("from Blitz", R[cur].d[0] == 0xffffffffUL);
    }
}

/* -------------------------------------------- LtoS, ASCRaw, RawASC */

static void conversions(void)
{
    static const ULONG l[] = { 0x41424344, 0x00ff7f80, 0x1f20217e, 0x80818283, 0, 0xffffffffUL };
    static UBYTE ch[2][4];
    unsigned i;
    int k;

    ROUT = "LtoS";
    for (i = 0; i < sizeof l / sizeof l[0] + 8; i++) {
        ULONG v = i < sizeof l / sizeof l[0] ? l[i] : rnd();
        sprintf(VEC, "$%08lX", v);
        DO2(ENT(LIB, E_LTOS, 0), RESSETUP(); R[cur].d[0] = v);
        STRRES();
    }
    ROUT = "ASCRaw";
    for (k = 0; k < 114; k++) {
        sprintf(VEC, "%d", k);
        DO2(ENT(LIB, E_ASCRAW, 0), RESSETUP(); R[cur].d[0] = 0x77770000UL | k);
        STRRES();
    }
    /* RawASC: every character of the table (found at its first index) */
    ROUT = "RawASC";
    for (k = 0; k < 114; k++) {
        /* the character from ASCRaw of the original */
        DO2(ENT(LIB, E_ASCRAW, 0), RESSETUP(); R[cur].d[0] = k);
        for (cur = 0; cur < 2; cur++) {
            ch[cur][0] = out[0][4];
            ch[cur][1] = 'z';
            ch[cur][2] = 0;
        }
        sprintf(VEC, "char %u (index %d)", ch[0][0], k);
        DO2(ENT(LIB, E_RAWASC, 0), R[cur].d[0] = (ULONG)ch[cur]);
        SAME("d0", R[cur].d[0]);
        BOTH("<= index", R[cur].d[0] <= (ULONG)k);
    }
}

/* -------------------------------------------------------- font prefs */

#define FPFILE "ENV:SYS/Font.Prefs"
#define FPSIZE 16384
#define FPENV "RAM:rimisclib-env"

/* ENV: of the font prefs part: FPENV with an empty SYS directory.  The
   real ENV:SYS/Font.Prefs is watched by IPrefs (a notification), which
   reads every file the test writes there and so changes the system's
   fonts; Workbench 3.1's IPrefs (Amiga Forever) even keeps the file with
   the bad PRHD in use for good, so every later write fails
   (ERROR_OBJECT_IN_USE).
   The library opens the name ENV:SYS/Font.Prefs, so it reads the private
   file too.  oldenv: the lock of the real ENV: (0: no private ENV:) */
static BPTR oldenv;

static BOOL envprivate(void)
{
    BPTR l;

    DeleteFile((STRPTR)FPENV "/SYS/Font.Prefs");
    if ((l = CreateDir((STRPTR)FPENV)))
        UnLock(l);
    if ((l = CreateDir((STRPTR)FPENV "/SYS")))
        UnLock(l);
    oldenv = Lock((STRPTR)"ENV:", SHARED_LOCK);
    l = Lock((STRPTR)FPENV, SHARED_LOCK);
    check("rimisclib: a private ENV: for the font prefs", oldenv && l);
    if (!oldenv || !l || !AssignLock((STRPTR)"ENV", l)) {
        if (l)
            UnLock(l);
        if (oldenv)
            UnLock(oldenv);
        oldenv = 0;
    }
    return oldenv != 0;
}

/* the real ENV: again (AssignLock unlocks the private one), FPENV removed */
static void envrestore(void)
{
    if (oldenv && !AssignLock((STRPTR)"ENV", oldenv))
        UnLock(oldenv);
    oldenv = 0;
    DeleteFile((STRPTR)FPENV "/SYS/Font.Prefs");
    DeleteFile((STRPTR)FPENV "/SYS");
    DeleteFile((STRPTR)FPENV);
}

static UBYTE fp[FPSIZE + 100];

static void put32(UBYTE *p, ULONG v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
static void put16(UBYTE *p, UWORD v) { p[0] = v >> 8; p[1] = (UBYTE)v; }

/* the file: FORM PREF, PRHD (prhd bytes), three FONT chunks of 164 bytes,
   padded to `size`; `bad` 1-4 spoils FORM/PREF/PRHD/FONT */
static void writefp(int bad, int prhd, LONG size)
{
    static const char *names[] = { "topaz.font", "helvetica.font", "xen.font" };
    static const UWORD ys[] = { 8, 11, 13 };
    UBYTE *p = fp;
    BPTR fh;
    int i;

    memset(fp, 0, sizeof fp);
    memcpy(p, bad == 1 ? "FORN" : "FORM", 4);
    put32(p + 4, size - 8);
    memcpy(p + 8, bad == 2 ? "PRFF" : "PREF", 4);
    memcpy(p + 12, bad == 3 ? "PRHX" : "PRHD", 4);
    put32(p + 16, prhd);
    p += 20 + prhd;
    for (i = 0; i < 3; i++) {
        memcpy(p, bad == 4 ? "FONX" : "FONT", 4);
        put32(p + 4, 156);
        put16(p + 8 + 14, (UWORD)i);                    /* fp_Type */
        p[8 + 16] = 1;
        p[8 + 17] = 0;
        put16(p + 8 + 24, ys[i]);                       /* ta_YSize */
        p[8 + 26] = (UBYTE)i;
        strcpy((char *)p + 8 + 28, names[i]);           /* fp_Name */
        p += 164;
    }
    /* a "chunk" of type 7 in the padding, where the search steps */
    put16(p + 164 * 20 + 8 + 14, 7);
    put16(p + 164 * 20 + 8 + 24, 0x77);
    strcpy((char *)p + 164 * 20 + 8 + 28, "padding.font");
    fh = Open((STRPTR)FPFILE, MODE_NEWFILE);
    if (fh) {
        Write(fh, fp, size);
        Close(fh);
    }
}

static void fontq(const char *what)
{
    static const ULONG types[] = { 0, 1, 2, 7, 3, 0x12340001UL, 0xffff0002UL };
    unsigned i;
    for (i = 0; i < sizeof types / sizeof types[0]; i++) {
        sprintf(VEC, "%s, type $%lX", what, types[i]);
        ROUT = "DefaultFontHeight";
        DO2(ENT(LIB, E_FHEIGHT, 0), R[cur].d[0] = types[i]);
        SAME("d0", R[cur].d[0]);
        ROUT = "DefaultFontName";
        DO2(ENT(LIB, E_FNAME, 0), RESSETUP(); R[cur].d[0] = types[i]);
        STRRES();
    }
}

static void openfp(const char *what)
{
    ROUT = "OpenFontPrefs";
    strcpy(VEC, what);
    DO2(ENT(LIB, E_OPENFP, 0), (void)0);
    SAME("d0", R[cur].d[0]);
}

static void closefp(const char *what)
{
    ROUT = "CloseFontPrefs";
    strcpy(VEC, what);
    DO2(ENT(LIB, E_CLOSEFP, 0), (void)0);
    SAME("d0", R[cur].d[0]);
}

static void fontprefs(void)
{
    static const char *bad[] = { "bad FORM", "bad PREF", "bad PRHD", "bad FONT" };
    int i;

    if (!envprivate())
        return;
    fontq("not open");
    closefp("not open");
    DeleteFile((STRPTR)FPFILE);
    openfp("no file");
    fontq("no file");
    for (i = 1; i <= 4; i++) {
        writefp(i, 6, FPSIZE);
        openfp(bad[i - 1]);
        fontq(bad[i - 1]);
    }
    writefp(0, 6, FPSIZE);
    openfp("ok");
    BOTH("opened", R[cur].d[0] == 0xffffffffUL);
    fontq("ok");
    ROUT = "DefaultFontName";
    strcpy(VEC, "type 1");
    DO2(ENT(LIB, E_FNAME, 0), RESSETUP(); R[cur].d[0] = 1);
    BOTH("helvetica.font", R[cur].d[0] == 14 && !strcmp((char *)out[cur] + 4, "helvetica.font"));
    ROUT = "DefaultFontHeight";
    DO2(ENT(LIB, E_FHEIGHT, 0), R[cur].d[0] = 2);
    BOTH("13", R[cur].d[0] == 13);
    closefp("open");
    closefp("closed");
    fontq("closed");
    writefp(0, 10, FPSIZE + 50);
    openfp("PRHD of 10 bytes, longer file");
    fontq("PRHD of 10 bytes");
    closefp("");
    writefp(0, 6, FPSIZE);
    openfp("open for finit");
    envrestore();
}

/* ------------------------------------------------------------- BInstr */

static void binstr(void)
{
    static const char *p[][2] = {
        { "hello world", "o" }, { "abcabc", "abc" }, { "abc", "" }, { "", "a" },
        { "", "" }, { "aaa", "aa" }, { "abc", "abcd" }, { "abc", "x" }, { "abc", "c" },
        { "abc", "a" }, { "a needle in a haystack", "needle" }, { "xyzxyz", "zx" },
    };
    unsigned i;

    ROUT = "BInstr";
    for (i = 0; i < sizeof p / sizeof p[0]; i++) {
        sprintf(VEC, "\"%s\",\"%s\"", p[i][0], p[i][1]);
        DO2(ENT(LIB, E_BINSTR, 0), memset(out[cur], 0, sizeof out[0]);
            strcpy((char *)out[cur] + 20, p[i][0]);
            strcpy((char *)out[cur] + 100, p[i][1]);
            R[cur].d[0] = (ULONG)out[cur] + 20; R[cur].d[1] = (ULONG)out[cur] + 100;
            frame[cur][1] = strlen(p[i][1]); frame[cur][2] = strlen(p[i][0]);
            R[cur].a[2] = (ULONG)&frame[cur][3];
            R[cur].a[3] = (ULONG)out[cur] + 200);
        SAME("d0", R[cur].d[0]);
    }
}

/* --------------------------------------------------------------- main */

int main(int argc, char **argv)
{
    static const char *names[] = { "floatquicklib", LIB, NULL };

    if (argc < 2) {
        printf("usage: rimisclib DIR\n");
        return 20;
    }
    if (!load_sets(argv[1], names))
        return 20;
    mathffp = OpenLibrary((STRPTR)"mathffp.library", 0);
    if (!mathffp) {
        check("rimisclib: mathffp.library", FALSE);
        return 20;
    }
    strcpy(VEC, "");

    ROUT = "init";
    DO2(ENT(LIB, E_INIT, 0), (void)0);
    SAME("d0 = IntuitionBase", R[cur].d[0] == (ULONG)IntuitionBase);

    timer();
    sigbit();
    workbenchhere();
    easyrequest();
    flip();
    parsing();
    progs();
    fromblitz();
    conversions();
    binstr();
    fontprefs();

    ROUT = "finit";
    strcpy(VEC, "font prefs open");
    rwe_mem_start();
    DO2(ENT(LIB, E_FINIT, 0), R[cur].a[6] = (ULONG)DOSBase);
    check("rimisclib: finit frees both font prefs buffers",
          -rwe_mem_stop() >= (LONG)(2 * RWE_MEMSIZE(FPSIZE)));
    CloseLibrary(mathffp);
    return rwe_done(LIB);
}
