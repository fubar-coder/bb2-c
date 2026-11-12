/*
 * rwe neilsreqtoolslib - the ReqTools interface of RTLib.bb: the C build
 * or the reference build of the source (refbuild), one set as LIBSET
 * says, observed as cases (rwecmp.h; the expected data
 * libs/tests/expected/rwe_neilsreqtoolslib.txt).
 *
 *   neilsreqtoolslib DIR   (DIR with neilsreqtoolslib, memlib, maxslib
 *                           .obj/.orig)
 *
 * The requesters need a user, so the test puts a fake reqtools.library
 * in the system library list (priority 10: before a real one): each of
 * its functions logs its registers, the tag list and the strings and
 * buffers it gets - pointers into the library, the test's requesters
 * and the string area made independent of the layout - and answers
 * with a chosen result (a file requester writes the chosen directory and
 * file, GetString/GetLong the chosen value, the font requester its
 * TextAttr and screen mode fields, a multi-load a file list).
 * Covered: everything before init that does not call reqtools; init when
 * OpenLibrary fails and when each of the seven rtAllocRequestA fails (the
 * library stays open, BUG(orig), #460), then ok; IsReqToolsActive, RTVersion/
 * RTRevision (d0's high word), RTReturnValue after every call; all three
 * forms of RTEZRequest with and without title (the tag list reset
 * afterwards); RTEZFlagsRequest both forms with and without gadgets (the
 * title in the wrong list, BUG(orig), #455); RTRequest; RTEZLoadFile,
 * RTEZSaveFile, RTFileRequest, RTEZPathRequest with ok/cancel and the
 * directory empty, ending in ':' or not; RTEZSetDefaultDirectory and
 * RTEZSetPattern 0-4, RTEZFreePattern 0-3 (their flags seen by the next
 * requester); RTEZMultiLoadFile (a list, cancel, a list at an address
 * ending in $00, BUG(orig), #462) and RTNextPathEntry to the end;
 * RTEZFontRequest, RTEZScreenModeRequest both forms (through
 * rtFontRequestA, BUG(orig), #466), RTEZPaletteRequest; RTEZGetLong both forms
 * (cancel returns the value, BUG(orig), #457), RTEZGetLongRange both forms;
 * RTEZGetString both forms, ok and cancel; RTLockWindow/RTUnlockWindow;
 * the asynchronous requesters (up, not up, check, end); finit (seven
 * rtFreeRequest, rtFreeFileList, the library closed).  Then, with the
 * fake removed, the real reqtools.library of the system (39.3 on the
 * emulator's Workbench) for what needs no user: init, IsReqToolsActive,
 * RTVersion/RTRevision, RTEZSetDefaultDirectory/RTEZSetPattern 0-4,
 * finit.
 * Not covered: the real requesters (they need a user); the routines
 * without reqtools.library (they call through a6 = 0); a second finit
 * (frees again).
 */
#include "rwecmp.h"

#define LIB "neilsreqtoolslib"
enum { E_EZREQ, E_LOAD, E_FONT, E_SM, E_PAL, E_GETLONG, E_GETLONGRNG, E_GETSTR, E_SAVE,
       E_SETDIR, E_PATH, E_LOCK, E_UNLOCK, E_SETPAT, E_FREEPAT, E_FLAGS, E_REQ, E_FILEREQ,
       E_MULTI, E_NEXT, E_ASYNC, E_CHECKAS, E_ENDAS, E_VER, E_REV, E_ACTIVE, E_ASYNCP,
       E_CHECKASP, E_ENDASP, E_RETVAL, E_INIT, E_FINIT };

#define CALL_HANDLER 0x80000000UL
#define RT_ReqHandler 0x80000008UL
#define RTFI_OkText 0x8000002aUL

/* ---------------------------------------------- the fake reqtools.library */

#define NVEC 27                         /* up to rtUnlockWindow (-162) */
#define NREQ 8
static struct Library *fake;
static UBYTE pool[NREQ][256];           /* the requesters */
static int npool, allocfail = -1;
static ULONG opens;
static UBYTE hinfo[16];                 /* the "rtHandlerInfo" */

static ULONG fk_ret, fk_long;
static const char *fk_dir = "", *fk_name = "", *fk_str = "";
static ULONG *fk_list;

/* the log of the calls */
struct rec {
    LONG lvo;
    ULONG d0, a0, a1, a2, a3, a4, x;
    ULONG tags[24];
    UBYTE s[2][64];
};
#define NLOG 16
static struct rec log_[NLOG];
static int nlog;
static UBYTE out[300];

/* a pointer made comparable: into the library, a requester of the set,
   its string area, its handler info */
static ULONG norm(ULONG p)
{
    struct lib *l = getlib(&loaded, LIB);
    int k;
    if (p >= (ULONG)l->code && p < (ULONG)l->code + l->size)
        return 0x4c000000;
    for (k = 0; k < NREQ; k++)
        if (p >= (ULONG)pool[k] && p < (ULONG)pool[k] + 256)
            return 0x52000000 | k << 16 | (p - (ULONG)pool[k]);
    if (p >= (ULONG)out && p < (ULONG)out + sizeof out)
        return 0x4f000000 | (p - (ULONG)out);
    if (p == (ULONG)hinfo)
        return 0x48000000;
    return p;
}

static void copys(UBYTE *to, const UBYTE *s)
{
    int i;
    if (!s) {
        strcpy((char *)to, "<null>");
        return;
    }
    for (i = 0; i < 63 && s[i]; i++)
        to[i] = s[i];
    to[i] = 0;
}

static struct rec *logcall(LONG lvo, ULONG d0, ULONG a0, ULONG a1, ULONG a2, ULONG a3,
                           ULONG a4)
{
    struct rec *r;
    ULONG *t = (ULONG *)a0;
    int i;
    if (nlog == NLOG)
        return &log_[NLOG - 1];
    r = &log_[nlog++];
    memset(r, 0, sizeof *r);
    r->lvo = lvo;
    r->d0 = d0;
    r->a0 = norm(a0);
    r->a1 = norm(a1);
    r->a2 = norm(a2);
    r->a3 = norm(a3);
    r->a4 = norm(a4);
    /* the tag list of the calls that take one in a0 */
    if (lvo != -36 && lvo != -60 && lvo != -156 && lvo != -162 && t)
        for (i = 0; i < 12 && t[2 * i]; i++) {
            ULONG data = t[2 * i + 1];
            r->tags[2 * i] = t[2 * i];
            if (t[2 * i] == RT_ReqHandler)
                *(APTR *)data = hinfo;             /* reqtools sets it */
            if (t[2 * i] == RTFI_OkText)
                data = *(const ULONG *)data;            /* the text */
            r->tags[2 * i + 1] = norm(data);
        }
    return r;
}

static struct Library *fk_open(struct Library *base __asm("a6"))
{
    base->lib_OpenCnt++;
    opens++;
    return base;
}

static ULONG fk_close(struct Library *base __asm("a6"))
{
    base->lib_OpenCnt--;
    opens--;
    return 0;
}

static ULONG fk_alloc(ULONG type __asm("d0"), ULONG tags __asm("a0"))
{
    logcall(-30, type, tags, 0, 0, 0, 0);
    if (npool == allocfail || npool == NREQ)
        return 0;
    memset(pool[npool], 0, 256);
    return (ULONG)pool[npool++];
}

static ULONG fk_free(ULONG req __asm("a1"))
{
    logcall(-36, 0, 0, req, 0, 0, 0);
    return 0;
}

static ULONG fk_change(ULONG req __asm("a1"), ULONG tags __asm("a0"))
{
    logcall(-48, 0, tags, req, 0, 0, 0);
    return 0;
}

static ULONG fk_file(UBYTE *req __asm("a1"), UBYTE *file __asm("a2"), UBYTE *title __asm("a3"),
                     ULONG tags __asm("a0"))
{
    struct rec *r = logcall(-54, 0, tags, (ULONG)req, file ? 1 : 0, (ULONG)title, 0);
    copys(r->s[0], title);
    copys(r->s[1], file);
    if (fk_ret) {
        *(const char **)(req + 16) = fk_dir;
        if (file)
            strcpy((char *)file, fk_name);
    }
    return fk_list ? (ULONG)fk_list : fk_ret;
}

static ULONG fk_freelist(ULONG list __asm("a0"))
{
    logcall(-60, 0, list, 0, 0, 0, 0);
    return 0;
}

static ULONG fk_ezreq(ULONG body __asm("a1"), ULONG gad __asm("a2"), ULONG ri __asm("a3"),
                      ULONG args __asm("a4"), ULONG tags __asm("a0"))
{
    logcall(-66, 0, tags, body, gad, ri, args);
    return fk_ret;
}

static ULONG fk_getstr(UBYTE *buf __asm("a1"), ULONG max __asm("d0"), ULONG title __asm("a2"),
                       ULONG ri __asm("a3"), ULONG tags __asm("a0"))
{
    struct rec *r = logcall(-72, max, tags, buf ? 1 : 0, title, ri, 0);
    copys(r->s[0], buf);
    if (fk_ret)
        strcpy((char *)buf, fk_str);
    return fk_ret;
}

static ULONG fk_getlong(ULONG *v __asm("a1"), ULONG title __asm("a2"), ULONG ri __asm("a3"),
                        ULONG tags __asm("a0"))
{
    struct rec *r = logcall(-78, 0, tags, (ULONG)v, title, ri, 0);
    r->x = *v;
    *v = fk_long;
    return fk_ret;
}

static ULONG fk_font(UBYTE *req __asm("a1"), UBYTE *title __asm("a3"), ULONG tags __asm("a0"))
{
    struct rec *r = logcall(-96, 0, tags, (ULONG)req, 0, (ULONG)title, 0);
    int i;
    copys(r->s[0], title);
    if (fk_ret) {
        for (i = 16; i < 64; i++)
            req[i] = (UBYTE)(i * 3 + 1);
        *(const char **)(req + 16) = "fake.font";
    }
    return fk_ret;
}

static ULONG fk_palette(ULONG title __asm("a2"), ULONG ri __asm("a3"), ULONG tags __asm("a0"))
{
    logcall(-102, 0, tags, 0, title, ri, 0);
    return fk_ret;
}

static ULONG fk_handler(ULONG h __asm("a1"), ULONG sigs __asm("d0"), ULONG tags __asm("a0"))
{
    logcall(-108, sigs, tags, h, 0, 0, 0);
    return fk_ret;
}

static ULONG fk_lock(ULONG win __asm("a0"))
{
    logcall(-156, 0, win, 0, 0, 0, 0);
    return 0x10c0000 + win;
}

static ULONG fk_unlock(ULONG win __asm("a0"), ULONG lock __asm("a1"))
{
    logcall(-162, 0, win, lock, 0, 0, 0);
    return 0;
}

static void fake_add(void)
{
    static APTR vec[NVEC + 1];
    vec[0] = (APTR)fk_open;
    vec[1] = (APTR)fk_close;
    vec[4] = (APTR)fk_alloc;            /* -30 */
    vec[5] = (APTR)fk_free;             /* -36 */
    vec[7] = (APTR)fk_change;           /* -48 */
    vec[8] = (APTR)fk_file;             /* -54 */
    vec[9] = (APTR)fk_freelist;         /* -60 */
    vec[10] = (APTR)fk_ezreq;           /* -66 */
    vec[11] = (APTR)fk_getstr;          /* -72 */
    vec[12] = (APTR)fk_getlong;         /* -78 */
    vec[15] = (APTR)fk_font;            /* -96 */
    vec[16] = (APTR)fk_palette;         /* -102 */
    vec[17] = (APTR)fk_handler;         /* -108 */
    vec[25] = (APTR)fk_lock;            /* -156 */
    vec[26] = (APTR)fk_unlock;          /* -162 */
    fake = fake_library("reqtools.library", 10, vec, NVEC);
    fake->lib_Revision = 0x1234;
}

/* ----------------------------------------------------------- helpers */

static void logreset(void)
{
    nlog = 0;                           /* in the setup of the call */
}

/* the calls of the library: each record (its LVO first, the pointers
   into the test program as such) */
static void logcheck(void)
{
    int k;
    OBS("reqtools calls", nlog);
    for (k = 0; k < nlog; k++) {
        char what[40];
        sprintf(what, "call %d", k);
        OBSPTRBYTES(what, &log_[k], sizeof(struct rec));
    }
}

/* RTReturnValue (the result of the last requester: a pointer into the
   test program as such) */
static void retval(void)
{
    const char *rout = ROUT;
    ROUT = "RTReturnValue";
    DO1(ENT(LIB, E_RETVAL, 0), (void)0);
    OBSPTR("d0", R.d[0], NULL, 0);
    ROUT = rout;
}

/* the string arguments at out+20 one after another, a3 behind the last,
   their lengths below a2 (the first at -4) */
static ULONG frame[4];
static void strargs(const char *s1, const char *s2, const char *s3)
{
    const char *s[3] = { s1, s2, s3 };
    UBYTE *p;
    int i;
    memset(out, 0xee, sizeof out);
    p = out + 20;
    for (i = 0; i < 3; i++) {
        frame[2 - i] = s[i] ? strlen(s[i]) : 0;
        if (!s[i])
            continue;
        strcpy((char *)p, s[i]);
        R.d[i] = (ULONG)p;
        p += strlen(s[i]) + 1;
    }
    R.a[2] = (ULONG)&frame[3];
    R.a[3] = (ULONG)p;
}

/* a call, its reqtools calls, RTReturnValue after it; RES = the
   registers after the call */
static struct regs RES;
#define DOREQ(e, form, setup) do {                                          \
        DO1(ENT(LIB, e, form), logreset(); setup);                          \
        /* a pointer into the library or the test program: as such */      \
        OBSPTR("d0", norm(R.d[0]), NULL, 0);                                \
        logcheck();                                                         \
        RES = R;                                                            \
        retval();                                                           \
    } while (0)

/* a string result (after DOREQ / after DO1) */
#define STRRES() do {                                                       \
        OBS("a3", RES.a[3] - (ULONG)out);                                   \
        OBSBYTES("out", out, sizeof out);                                   \
    } while (0)
#define STRRES2() do {                                                      \
        RES = R;                                                            \
        STRRES();                                                           \
    } while (0)

/* ------------------------------------------------------------ cases */

static void before_init(void)
{
    int k;
    strcpy(VEC, "before init");
    ROUT = "IsReqToolsActive";
    DO1(ENT(LIB, E_ACTIVE, 0), (void)0);
    OBS("d0", R.d[0]);
    retval();
    ROUT = "RTEZFreePattern";
    for (k = 0; k < 4; k++)
        DO1(ENT(LIB, E_FREEPAT, 0), R.d[0] = k);
    ROUT = "RTNextPathEntry";
    DO1(ENT(LIB, E_NEXT, 0), memset(out, 0xee, sizeof out);
        R.a[3] = (ULONG)out + 4);
    OBS("d0", R.d[0]);
    STRRES2();
}

static void init_case(const char *what, int failat)
{
    ROUT = "init";
    strcpy(VEC, what);
    allocfail = failat;
    DO1(ENT(LIB, E_INIT, 0), logreset(); npool = 0; opens = 0);
    logcheck();
    OBS("library opened", opens);
    allocfail = -1;
    ROUT = "IsReqToolsActive";
    DO1(ENT(LIB, E_ACTIVE, 0), (void)0);
    OBS("d0", R.d[0]);
}

static void inits(void)
{
    APTR old;
    int k;

    ROUT = "init";
    strcpy(VEC, "OpenLibrary fails");
    Forbid();
    old = SetFunction((struct Library *)SysBase, -552, (APTR)rwe_fail0);
    CacheClearU();
    DO1(ENT(LIB, E_INIT, 0), logreset());
    SetFunction((struct Library *)SysBase, -552, old);
    CacheClearU();
    Permit();
    OBS("d0", R.d[0]);
    logcheck();
    ROUT = "IsReqToolsActive";
    DO1(ENT(LIB, E_ACTIVE, 0), (void)0);
    OBS("d0", R.d[0]);
    ROUT = "finit (inactive)";
    DO1(ENT(LIB, E_FINIT, 0), logreset());
    logcheck();

    fake_add();
    for (k = 0; k < 7; k++) {
        char what[40];
        sprintf(what, "rtAllocRequestA %d fails", k);
        init_case(what, k);
        /* BUG(orig) (#460): the library stays open, finit does nothing */
        ROUT = "finit (inactive)";
        DO1(ENT(LIB, E_FINIT, 0), logreset(); opens = 0);
        logcheck();
        OBS("library closed", opens);
    }
    init_case("ok", -1);
    BOTH("active", R.d[0] == 0xffffffffUL);
    BOTH("seven requesters", nlog == 7 && npool == 7);
}

static void versions(void)
{
    strcpy(VEC, "");
    ROUT = "RTVersion";
    DO1(ENT(LIB, E_VER, 0), (void)0);
    OBS("d0", R.d[0]);
    BOTH("99", (UWORD)R.d[0] == 99);
    ROUT = "RTRevision";
    DO1(ENT(LIB, E_REV, 0), (void)0);
    OBS("d0", R.d[0]);
}

static void ezrequests(void)
{
    static const char *titles[] = { "Title", "" };
    int t;

    ROUT = "RTEZRequest";
    for (t = 0; t < 2; t++) {
        sprintf(VEC, "title \"%s\"", titles[t]);
        fk_ret = 3 + t;
        DOREQ(E_EZREQ, 0, strargs(titles[t], "Body text", "Ok|Cancel"));
        DOREQ(E_EZREQ, 1, strargs(titles[t], "Body", "_Yes|_No"); R.d[3] = 4);
        DOREQ(E_EZREQ, 2, strargs(titles[t], "Body", "A|B|C"); R.d[3] = 0;
              R.d[4] = 2; R.d[5] = 0x5);
    }
    strcpy(VEC, "defaults again");
    DOREQ(E_EZREQ, 0, strargs("", "x", "y"));

    ROUT = "RTEZFlagsRequest";
    for (t = 0; t < 2; t++) {
        sprintf(VEC, "gadgets %s", t ? "none" : "\"Ok\"");
        fk_ret = 7;
        DOREQ(E_FLAGS, 0, strargs("Flags title", "Body", t ? "" : "Ok");
              R.d[3] = 0x00000208);
        BOTH("no title (BUG(orig) #455)", log_[0].tags[0] == 0x80000014UL
             && log_[0].tags[1] == 0 && (log_[0].a2 != 0) == !t);
        DOREQ(E_FLAGS, 1, strargs("Flags title", "Body", t ? "" : "Ok");
              R.d[3] = 0x10; R.d[4] = 1);
    }
    ROUT = "RTEZRequest";
    strcpy(VEC, "after RTEZFlagsRequest, no title");
    DOREQ(E_EZREQ, 0, strargs("", "x", "y"));

    ROUT = "RTRequest";
    {
        static const ULONG tags[] = { 0x80000003UL, 0, 0x80000014UL, (ULONG)"t", 0, 0 };
        strcpy(VEC, "tag list");
        fk_ret = 0;
        DOREQ(E_REQ, 0, strargs("Body", "Gad", NULL); R.d[2] = (ULONG)tags);
    }
}

/* a file requester: ok (dir, file) and cancel */
static void filereq(int e, int form, const char *what, ULONG tags)
{
    static const char *dirs[] = { "", "RAM:", "RAM:t", "Work:a/b" };
    unsigned i;
    for (i = 0; i < sizeof dirs / sizeof dirs[0]; i++) {
        sprintf(VEC, "%s dir \"%s\"", what, dirs[i]);
        fk_ret = 1;
        fk_dir = dirs[i];
        fk_name = i == 2 ? "" : "chosen.file";
        DOREQ(e, form, strargs("A title", "default.name", NULL); R.d[2] = tags);
        STRRES();
        if (e == E_LOAD && i == 3)
            BOTH("Work:a/b/chosen.file at the title",
                 !strcmp((char *)out + 20, "Work:a/b/chosen.file")
                 && RES.d[0] == 20 && RES.a[3] == (ULONG)out + 41);
    }
    sprintf(VEC, "%s cancel", what);
    fk_ret = 0;
    DOREQ(e, form, strargs("A title", "default.name", NULL); R.d[2] = tags);
    STRRES();
}

static void filereqs(void)
{
    static const ULONG tags[] = { 0x80000028UL, 0x10, 0, 0 };
    int k;

    ROUT = "RTEZLoadFile";
    filereq(E_LOAD, 0, "", 0);
    ROUT = "RTEZSaveFile";
    filereq(E_SAVE, 0, "", 0);
    ROUT = "RTFileRequest";
    filereq(E_FILEREQ, 0, "", (ULONG)tags);
    ROUT = "RTEZPathRequest";
    filereq(E_PATH, 0, "", 0);

    for (k = 0; k < 5; k++) {
        ROUT = "RTEZSetDefaultDirectory";
        sprintf(VEC, "requester %d", k);
        DOREQ(E_SETDIR, 0, logreset(); R.d[0] = k; R.d[1] = (ULONG)"Work:dir");
        ROUT = "RTEZSetPattern";
        DOREQ(E_SETPAT, 0, logreset(); R.d[0] = k; R.d[1] = (ULONG)"#?.info");
    }
    /* the flags the pattern set: seen by the next requesters */
    ROUT = "RTEZLoadFile";
    filereq(E_LOAD, 0, "with pattern", 0);
    ROUT = "RTEZSaveFile";
    filereq(E_SAVE, 0, "with pattern", 0);
    for (k = 0; k < 4; k++) {
        ROUT = "RTEZFreePattern";
        sprintf(VEC, "requester %d", k);
        DO1(ENT(LIB, E_FREEPAT, 0), R.d[0] = k);
        OBS("d0", R.d[0]);
    }
    ROUT = "RTEZLoadFile";
    filereq(E_LOAD, 0, "pattern freed", 0);
    ROUT = "RTEZSaveFile";
    filereq(E_SAVE, 0, "pattern freed", 0);
}

static void multiload(void)
{
    static ULONG list[3][3];
    static ULONG *aligned;
    static UBYTE space[600];
    int k, n;

    list[0][0] = (ULONG)list[1];
    list[0][1] = 5;
    list[0][2] = (ULONG)"a.txt";
    list[1][0] = (ULONG)list[2];
    list[1][1] = 5;
    list[1][2] = (ULONG)"dir/b";
    list[2][0] = 0;
    list[2][1] = 0;
    list[2][2] = (ULONG)"";
    aligned = (ULONG *)(((ULONG)space + 255) & ~255UL);
    aligned[0] = 0;
    aligned[1] = 4;
    aligned[2] = (ULONG)"only";

    ROUT = "RTEZSetPattern";
    strcpy(VEC, "multi-load");
    DOREQ(E_SETPAT, 0, R.d[0] = 3; R.d[1] = (ULONG)"#?");
    for (n = 0; n < 3; n++) {
        ROUT = "RTEZMultiLoadFile";
        sprintf(VEC, "%s", n == 0 ? "list of 3" : n == 1 ? "list at $xx00" : "cancel");
        fk_ret = n < 2;
        fk_list = n == 0 ? list[0] : n == 1 ? aligned : NULL;
        fk_dir = n == 0 ? "Work:dir" : "RAM:";
        DOREQ(E_MULTI, 0, strargs("Multi", NULL, NULL));
        if (n == 1)
            BOTH("the list, not -1 (BUG(orig) #462)", RES.d[0] == (ULONG)aligned);
        fk_list = NULL;
        for (k = 0; k < 4; k++) {
            ROUT = "RTNextPathEntry";
            sprintf(VEC, "list %d entry %d", n, k);
            DO1(ENT(LIB, E_NEXT, 0), logreset(); memset(out, 0xee, sizeof out);
                R.a[3] = (ULONG)out + 4);
            OBS("d0", R.d[0]);
            STRRES2();
            logcheck();
        }
    }
}

static void others(void)
{
    UBYTE res[16];
    int k;

    ROUT = "RTEZFontRequest";
    for (k = 0; k < 2; k++) {
        sprintf(VEC, "%s", k ? "cancel" : "ok");
        fk_ret = !k;
        DOREQ(E_FONT, 0, strargs("Font", NULL, NULL));
        if (!k) {
            memcpy(res, (UBYTE *)RES.d[0], 8);
            OBSPTRBYTES("the TextAttr", res, 8);
        }
    }
    ROUT = "RTEZScreenModeRequest";
    for (k = 0; k < 4; k++) {
        sprintf(VEC, "form %d %s", k & 1, k & 2 ? "cancel" : "ok");
        fk_ret = !(k & 2);
        DOREQ(E_SM, k & 1, strargs("Screen", NULL, NULL); R.d[1] = 0x12345);
        BOTH("rtFontRequestA (BUG(orig) #466)", nlog == 1 && log_[0].lvo == -96);
        if (!(k & 2)) {
            memcpy(res, (UBYTE *)RES.d[0], 16);
            OBSPTRBYTES("the screen mode", res, 16);
        }
    }
    ROUT = "RTEZPaletteRequest";
    for (k = 0; k < 2; k++) {
        sprintf(VEC, "ret %d", 5 - 6 * k);
        fk_ret = 5 - 6 * k;
        DOREQ(E_PAL, 0, strargs("Palette", NULL, NULL); R.d[1] = 3 + k);
    }

    ROUT = "RTEZGetLong";
    for (k = 0; k < 4; k++) {
        sprintf(VEC, "form %d %s", k & 1, k & 2 ? "cancel" : "ok");
        fk_ret = !(k & 2);
        fk_long = 1000 + k;
        DOREQ(E_GETLONG, k & 1, strargs("Number", "Enter it", NULL); R.d[2] = 77 + k);
        BOTH("the value, also on cancel (BUG(orig) #457)", RES.d[0] == 1000 + (ULONG)k);
    }
    ROUT = "RTEZGetLongRange";
    for (k = 0; k < 4; k++) {
        sprintf(VEC, "form %d %s", k & 1, k & 2 ? "cancel" : "ok");
        fk_ret = !(k & 2);
        fk_long = 2000 + k;
        DOREQ(E_GETLONGRNG, k & 1, strargs("Range", "Enter it", NULL); R.d[2] = 10;
              R.d[3] = 20 + k; R.d[4] = 15);
    }

    ROUT = "RTEZGetString";
    for (k = 0; k < 6; k++) {
        sprintf(VEC, "form %d %s", k & 1, k >= 4 ? "empty" : k & 2 ? "cancel" : "ok");
        fk_ret = !(k & 2);
        fk_str = k >= 4 ? "" : "typed text";
        DOREQ(E_GETSTR, k & 1, strargs("String", "Enter it", NULL); R.d[2] = 40;
              R.d[3] = (ULONG)"default");
        STRRES();
    }

    ROUT = "RTLockWindow";
    {
        static ULONG winobj[2];
        winobj[0] = 0xcafe0042UL;
        strcpy(VEC, "");
        DOREQ(E_LOCK, 0, R.a[1] = (ULONG)winobj);
        ROUT = "RTUnlockWindow";
        DOREQ(E_UNLOCK, 0, R.a[1] = (ULONG)winobj; R.d[1] = 0x10c1234);
    }
}

static void async(void)
{
    static struct MsgPort port;
    static UBYTE obj[100];
    int k;

    port.mp_SigBit = 5;
    *(ULONG *)(obj + 86) = (ULONG)&port;
    for (k = 0; k < 2; k++) {
        sprintf(VEC, "%s", k ? "not up" : "up");
        ROUT = "RTASyncRequest";
        fk_ret = k ? 2 : CALL_HANDLER;
        DOREQ(E_ASYNC, 0, strargs("Async", "Body", "Ok"); R.a[1] = (ULONG)obj);
        ROUT = "RTCheckASyncRequest";
        DOREQ(E_CHECKAS, 0, (void)0);
        fk_ret = 0;
        DOREQ(E_CHECKAS, 0, (void)0);
        ROUT = "RTEndASyncRequest";
        fk_ret = 1;
        DOREQ(E_ENDAS, 0, (void)0);

        ROUT = "RTASyncPaletteRequest";
        fk_ret = k ? 2 : CALL_HANDLER;
        DOREQ(E_ASYNCP, 0, strargs("Async palette", NULL, NULL); R.d[1] = 7;
              R.a[1] = (ULONG)obj);
        ROUT = "RTCheckASyncPaletteRequest";
        fk_ret = CALL_HANDLER;
        DOREQ(E_CHECKASP, 0, (void)0);
        fk_ret = 3;
        DOREQ(E_CHECKASP, 0, (void)0);
        ROUT = "RTEndASyncPaletteRequest";
        fk_ret = 1;
        DOREQ(E_ENDASP, 0, (void)0);
    }
}

/* with the real reqtools.library, what needs no user: init (seven real
   requesters), the version, the directory and pattern of each requester,
   finit */
static void realphase(void)
{
    struct Library *real = OpenLibrary((STRPTR)"reqtools.library", 38);
    UWORD cnt;
    int k;

    if (!real) {
        printf("neilsreqtoolslib: no reqtools.library V38 - the real one not tested\n");
        return;
    }
    cnt = real->lib_OpenCnt;            /* others may have it open too */
    strcpy(VEC, "real reqtools.library");
    ROUT = "init";
    DO1(ENT(LIB, E_INIT, 0), (void)0);
    BOTH("a requester", R.d[0] != 0);
    ROUT = "IsReqToolsActive";
    DO1(ENT(LIB, E_ACTIVE, 0), (void)0);
    OBS("d0", R.d[0]);
    BOTH("active", R.d[0] == 0xffffffffUL);
    ROUT = "RTVersion";
    DO1(ENT(LIB, E_VER, 0), (void)0);
    OBS("d0", R.d[0]);
    BOTH("= lib_Version", (UWORD)R.d[0] == real->lib_Version);
    ROUT = "RTRevision";
    DO1(ENT(LIB, E_REV, 0), (void)0);
    OBS("d0", R.d[0]);
    for (k = 0; k < 5; k++) {
        ROUT = "RTEZSetDefaultDirectory";
        sprintf(VEC, "real, requester %d", k);
        DO1(ENT(LIB, E_SETDIR, 0), R.d[0] = k; R.d[1] = (ULONG)"RAM:");
        OBS("d0", R.d[0]);
        ROUT = "RTEZSetPattern";
        DO1(ENT(LIB, E_SETPAT, 0), R.d[0] = k; R.d[1] = (ULONG)"#?.txt");
        OBS("d0", R.d[0]);
    }
    ROUT = "finit";
    strcpy(VEC, "real reqtools.library");
    DO1(ENT(LIB, E_FINIT, 0), (void)0);
    printf("neilsreqtoolslib: real reqtools.library %u.%u, open count %u before init, "
           "%u after finit\n", real->lib_Version, real->lib_Revision, cnt, real->lib_OpenCnt);
    check("neilsreqtoolslib: real reqtools closed by finit", real->lib_OpenCnt == cnt);
    CloseLibrary(real);
}

/* --------------------------------------------------------------- main */

int main(int argc, char **argv)
{
    static const char *names[] = { "memlib", "maxslib", LIB, NULL };

    if (argc < 2) {
        printf("usage: neilsreqtoolslib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("IsReqToolsActive", d1-d7|a0-a3|a6);
    KEEPS("RTReturnValue", d1-d7|a0-a3|a6);
    KEEPS("RTEZFreePattern", d1-d7|a0-a3|a6);
    KEEPS("RTNextPathEntry", d3-d7|a2|a6);
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("finit (inactive)", d1-d7|a0-a3|a6);
    KEEPS("RTVersion", d1-d7|a0|a2|a3|a6);
    KEEPS("RTRevision", d0-d7|a0|a2|a3|a6);
    KEEPS("RTEZRequest", d2-d7|a3|a6);
    KEEPS("RTEZFlagsRequest", d2-d7|a3|a6);
    KEEPS("RTRequest", d2-d7|a3|a6);
    KEEPS("RTEZLoadFile", d2-d7|a6);
    KEEPS("RTEZSaveFile", d2-d7|a3|a6);
    KEEPS("RTFileRequest", d2-d7|a6);
    KEEPS("RTEZPathRequest", d2-d7|a6);
    KEEPS("RTEZSetDefaultDirectory", d2-d7|a2|a3|a6);
    KEEPS("RTEZSetPattern", d2-d7|a2|a3|a6);
    KEEPS("RTEZMultiLoadFile", d2-d7|a3|a6);
    KEEPS("RTEZFontRequest", d2-d7|a2|a3|a6);
    KEEPS("RTEZScreenModeRequest", d2-d7|a2|a3|a6);
    KEEPS("RTEZPaletteRequest", d2-d7|a3|a6);
    KEEPS("RTEZGetLong", d2-d7|a3|a6);
    KEEPS("RTEZGetLongRange", d2-d7|a3|a6);
    KEEPS("RTEZGetString", d2-d7|a6);
    KEEPS("RTLockWindow", d2-d7|a2|a3|a6);
    KEEPS("RTUnlockWindow", d2-d7|a2|a3|a6);
    KEEPS("RTASyncRequest", d2-d7|a3|a6);
    KEEPS("RTCheckASyncRequest", d2-d7|a2|a3|a6);
    KEEPS("RTEndASyncRequest", d2-d7|a2|a3|a6);
    KEEPS("RTASyncPaletteRequest", d2-d7|a3|a6);
    KEEPS("RTCheckASyncPaletteRequest", d2-d7|a2|a3|a6);
    KEEPS("RTEndASyncPaletteRequest", d2-d7|a2|a3|a6);
    KEEPS("finit", d2-d7|a2|a3);
    ROUT = "memlib init";
    VEC[0] = 0;
    markregs(&R);
    callregs(entry_form(getlib(&loaded, "memlib"), 7, 0), &R, 0, 0);

    {
        /* for the notes only: the fake is used either way */
        struct Library *real = OpenLibrary((STRPTR)"reqtools.library", 0);
        printf("neilsreqtoolslib: reqtools.library %s in the system\n",
               real ? "is" : "is not");
        if (real)
            CloseLibrary(real);
    }
    before_init();
    inits();
    versions();
    ezrequests();
    filereqs();
    multiload();
    others();
    async();

    ROUT = "finit";
    strcpy(VEC, "");
    DO1(ENT(LIB, E_FINIT, 0), logreset(); opens = 1);
    logcheck();
    OBS("library closed", opens);
    BOTH("seven requesters and the file list freed", nlog == 8);
    check("neilsreqtoolslib: fake closed (only the failed inits' opens left)",
          fake->lib_OpenCnt == 7);

    markregs(&R);
    callregs(entry_form(getlib(&loaded, "memlib"), 8, 0), &R, 0, 0);
    Forbid();
    fake->lib_OpenCnt = 0;
    Permit();
    fake_remove(fake);
    realphase();
    return rwe_done(LIB);
}
