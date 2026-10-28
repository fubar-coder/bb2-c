/*
 * reqclosetest - proves reqclose (run it as `reqclose reqclosetest`):
 *   1. a GadTools window "reqclose ok" with OK/Cancel buttons -> OK (id 1)
 *   2. the same as "reqclose cancel"                         -> Cancel (id 2)
 *   3. the same as "reqclose id", ENV "ok#2"                 -> id 2
 *   4. a window "reqclose none" that no ENV line matches: must time out
 * (GadTools buttons have no GadgetText: 1/2 prove the button-row rule and
 * that a synthetic GADGETUP passes GT_GetIMsg/GT_FilterIMsg; each line
 * says "expected" or "MISMATCH"; return 5 (WARN) on any MISMATCH or if
 * the closer did not answer - the corpus runner's gate for the requester
 * tests)
 *   5. ASL file requester "reqclose asl ok"                    -> TRUE
 *   6. ASL file requester "reqclose asl cancel"                -> FALSE
 *   7. ASL font requester (asl's own title)                    -> FALSE ("*")
 * The windows wait at most 4 s (Delay polling) and print TIMEOUT; the ASL
 * requesters block - only reqclose can end them, so they are skipped if
 * the closer did not answer both windows.  Output unbuffered (Write).  ENV:REQCLOSE is set at
 * the start and deleted at the end.
 */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <dos/var.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#include <string.h>

/* unbuffered output (Write to Output()) */
static void out(const char *a, const char *b, long n, const char *c)
{
    char buf[200], num[12], *p = num + sizeof num - 1;
    int neg = n < 0;
    unsigned long u = neg ? -n : n;
    *p = 0;
    do { *--p = '0' + u % 10; u /= 10; } while (u);
    if (neg) *--p = '-';
    strcpy(buf, a);
    if (b) strcat(buf, b);
    if (n != -999) { strcat(buf, " "); strcat(buf, p); }
    if (c) strcat(buf, c);
    strcat(buf, "\n");
    Write(Output(), buf, strlen(buf));
}

struct IntuitionBase *IntuitionBase;
static int mismatches;          /* results other than expected */

static void check(const char *title, int ok)
{
    out(title, ok ? ": expected" : ": MISMATCH", -999, 0);
    if (!ok) mismatches++;
}
struct Library *GadToolsBase, *AslBase;

static const char env[] =
    "ok reqclose ok\n"
    "cancel reqclose cancel\n"
    "ok#2 reqclose id\n"
    "ok reqclose asl ok\n"
    "cancel reqclose asl cancel\n"
    "ignore reqclose none\n"
    "cancel *\n";

/* the result: gadget id, 100 close, 200 key, -1 timeout; prints
   "expected"/"MISMATCH" against expect */
static int window(const char *title, int expect)
{
    struct Screen *scr = LockPubScreen(NULL);
    APTR vi = GetVisualInfoA(scr, NULL);
    struct Gadget *glist = NULL, *g;
    struct NewGadget ng = { 0 };
    struct Window *w;
    int i, done = 0, res = -1;

    g = CreateContext(&glist);
    ng.ng_TopEdge = scr->WBorTop + scr->Font->ta_YSize + 20;
    ng.ng_Width = 80;
    ng.ng_Height = 14;
    ng.ng_TextAttr = scr->Font;
    ng.ng_VisualInfo = vi;
    ng.ng_LeftEdge = 20;
    ng.ng_GadgetText = "OK";
    ng.ng_GadgetID = 1;
    g = CreateGadgetA(BUTTON_KIND, g, &ng, NULL);
    ng.ng_LeftEdge = 160;
    ng.ng_GadgetText = "Cancel";
    ng.ng_GadgetID = 2;
    g = CreateGadgetA(BUTTON_KIND, g, &ng, NULL);
    w = OpenWindowTags(NULL, WA_Title, (ULONG)title, WA_Left, 40, WA_Top, 40,
                       WA_Width, 280, WA_Height, 90, WA_Gadgets, (ULONG)glist,
                       WA_IDCMP, BUTTONIDCMP | IDCMP_CLOSEWINDOW | IDCMP_VANILLAKEY,
                       WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_ACTIVATE,
                       WA_PubScreen, (ULONG)scr, TAG_END);
    if (!w) {
        out(title, ": no window", -999, 0);
    } else {
        GT_RefreshWindow(w, NULL);
        for (i = 0; i < 100 && !done; i++) {
            struct IntuiMessage *m;
            Delay(2);
            while ((m = GT_GetIMsg(w->UserPort)) != NULL) {
                ULONG class = m->Class;
                UWORD code = m->Code;
                APTR addr = m->IAddress;
                GT_ReplyIMsg(m);
                if (class == IDCMP_GADGETUP) {
                    out(title, ": gadget", ((struct Gadget *)addr)->GadgetID, 0);
                    res = ((struct Gadget *)addr)->GadgetID;
                    done = 1;
                } else if (class == IDCMP_CLOSEWINDOW) {
                    out(title, ": close", -999, 0);
                    res = 100;
                    done = 1;
                } else if (class == IDCMP_VANILLAKEY) {
                    out(title, ": key", code, 0);
                    res = 200;
                    done = 1;
                }
            }
        }
        if (!done)
            out(title, ": TIMEOUT", -999, 0);
        CloseWindow(w);
    }
    check(title, res == expect);
    FreeGadgets(glist);
    FreeVisualInfo(vi);
    UnlockPubScreen(NULL, scr);
    return res;
}

static void aslfile(const char *title, int expect)
{
    struct FileRequester *fr = AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, (ULONG)title, ASLFR_InitialDrawer, (ULONG)"RAM:",
        ASLFR_InitialFile, (ULONG)"reqclose.txt", TAG_END);
    LONG r;

    if (!fr) {
        out(title, ": no requester", -999, 0);
        mismatches++;
        return;
    }
    r = AslRequest(fr, NULL);
    out(title, ":", r != 0, 0);
    check(title, (r != 0) == expect);
    if (r) {
        out("  drawer [", (char *)fr->fr_Drawer, -999, "]");
        out("  file [", (char *)fr->fr_File, -999, "]");
    }
    FreeAslRequest(fr);
}

int main(void)
{
    struct FontRequester *fo;
    int answered;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    GadToolsBase = OpenLibrary("gadtools.library", 39);
    AslBase = OpenLibrary("asl.library", 39);
    if (!IntuitionBase || !GadToolsBase || !AslBase) {
        out("libraries missing", 0, -999, 0);
        return 20;
    }
    SetVar("REQCLOSE", (STRPTR)env, -1, GVF_GLOBAL_ONLY);
    /* answered: the closer reacted at all (else the ASL tests would block) */
    answered = window("reqclose ok", 1) >= 0;
    answered += window("reqclose cancel", 2) >= 0;
    window("reqclose id", 2);
    window("reqclose none", -1);
    if (answered < 2) {
        /* the ASL requesters block: only with a working closer */
        out("skipping the ASL tests (windows not answered)", 0, -999, 0);
    } else {
        aslfile("reqclose asl ok", 1);
        aslfile("reqclose asl cancel", 0);
        fo = AllocAslRequestTags(ASL_FontRequest, TAG_END);
        if (fo) {
            LONG r = AslRequest(fo, NULL) != 0;
            out("font:", 0, r, 0);
            check("font", r == 0);
            FreeAslRequest(fo);
        } else
            mismatches++;
    }
    DeleteVar("REQCLOSE", GVF_GLOBAL_ONLY);
    CloseLibrary(AslBase);
    CloseLibrary(GadToolsBase);
    CloseLibrary((struct Library *)IntuitionBase);
    /* WARN: the closer did not work (the corpus runner's gate) */
    return answered < 2 || mismatches ? 5 : 0;
}
