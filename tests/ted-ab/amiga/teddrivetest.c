/*
 * teddrivetest - gate D of the TED A/B tests: proves teddrive on a program
 * that reads its input like TED (bb2 ted-c/notes/PHASE3-PLAN.md 4.2):
 * its own screen cloned from the Workbench (SA_LikeWorkbench, depth 2),
 * a backdrop borderless main window with a menu strip, requesters as
 * untitled windows on that screen, WaitPort on the port of the newest
 * window only (TED's pickwindow), every message replied before it is
 * handled (TED's handled0), keys through RawKeyConvert of console.device.
 *
 *   teddrive tests/emu/gate-d.drv PREFIX teddrivetest main
 *   teddrive tests/emu/gate-d-rescue.drv PREFIX teddrivetest rescue
 *
 * Expected input (one line "expected"/"MISMATCH" each):
 *   main:   RAWKEY $20 -> "a"; four keys -> "Ab1\r" (MapANSI + qualifier);
 *           menu 0/1 -> requester window (OK id 1, Cancel id 2, string id 3,
 *           integer id 4); GADGETUP 1 with string "hello", integer 42 ->
 *           requester closed; menu 0/2 -> busy 1 s (Delay) - no input may
 *           arrive meanwhile; RAWKEY $21 -> "s"; menu 0/9 -> quit
 *   rescue: menu 0/1 -> requester; GADGETUP 1 (teddrive's rescue, the
 *           script fails on purpose); menu 0/9 (rescue: QUIT) -> quit
 * After every handled message the port must be empty (teddrive sends only
 * when the program waits again - "idle").  A filled rectangle (pen 3) at
 * screen x 16..47, y 32..39 is checked in the dumps by the runner
 * (tests/ted-ab/runner, tedab).
 * Everything is bounded by a 20 s timer (TIMEOUT).  Return 0, or 5 (WARN)
 * on any mismatch or timeout - the runner takes it as the gate.
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <devices/timer.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/console.h>
#include "tdout.h"

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Device *ConsoleDevice;

static int mismatches, step, rescue;
static struct Screen *scr;
static struct Window *mainw, *reqw;

static void check(const char *what, int ok)
{
    o_str("teddrivetest: ");
    o_str(what);
    o_str(ok ? ": expected" : ": MISMATCH");
    o_line();
    if (!ok) mismatches++;
}

/* ------------------------------------------------------------ menu */

static struct IntuiText itext[10];
static struct MenuItem items[10];
static const char *const inames[10] = {
    "0 -", "1 Requester", "2 Busy", "3 -", "4 -", "5 -", "6 -", "7 -", "8 -", "9 Quit"
};
static struct Menu menu = { NULL, 0, 0, 100, 10, MENUENABLED, (BYTE *)"Test", &items[0] };

static void makemenu(void)
{
    int i;
    for (i = 0; i < 10; i++) {
        itext[i].FrontPen = 0;
        itext[i].BackPen = 1;
        itext[i].DrawMode = JAM2;
        itext[i].LeftEdge = 2;
        itext[i].TopEdge = 1;
        itext[i].IText = (UBYTE *)inames[i];
        items[i].NextItem = i < 9 ? &items[i + 1] : NULL;
        items[i].TopEdge = i * 10;
        items[i].Width = 120;
        items[i].Height = 10;
        items[i].Flags = ITEMTEXT | ITEMENABLED | HIGHCOMP;
        items[i].ItemFill = &itext[i];
    }
}

/* ------------------------------------------------------- requester */

static UBYTE strbuf[40], intbuf[12], undo[40];
static struct StringInfo strinfo = { strbuf, undo, 0, sizeof strbuf };
static struct StringInfo intinfo = { intbuf, undo, 0, sizeof intbuf };
static struct IntuiText okt = { 1, 0, JAM1, 4, 2, NULL, (UBYTE *)"OK", NULL };
static struct IntuiText cat = { 1, 0, JAM1, 4, 2, NULL, (UBYTE *)"Cancel", NULL };
static struct Gadget gint = { NULL, 160, 50, 80, 10, GFLG_GADGHCOMP,
    GACT_RELVERIFY | GACT_LONGINT, GTYP_STRGADGET, NULL, NULL, NULL, 0, &intinfo, 4, NULL };
static struct Gadget gstr = { &gint, 20, 50, 120, 10, GFLG_GADGHCOMP,
    GACT_RELVERIFY, GTYP_STRGADGET, NULL, NULL, NULL, 0, &strinfo, 3, NULL };
static struct Gadget gcan = { &gstr, 160, 20, 64, 12, GFLG_GADGHCOMP,
    GACT_RELVERIFY | GACT_ENDGADGET, GTYP_BOOLGADGET, NULL, NULL, &cat, 0, NULL, 2, NULL };
static struct Gadget gok = { &gcan, 20, 20, 64, 12, GFLG_GADGHCOMP,
    GACT_RELVERIFY | GACT_ENDGADGET, GTYP_BOOLGADGET, NULL, NULL, &okt, 0, NULL, 1, NULL };

static void openreq(void)
{
    strbuf[0] = intbuf[0] = 0;
    intinfo.LongInt = 0;
    reqw = OpenWindowTags(NULL, WA_CustomScreen, (ULONG)scr, WA_Left, 100, WA_Top, 100,
                          WA_Width, 300, WA_Height, 80, WA_Gadgets, (ULONG)&gok,
                          WA_IDCMP, IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_RAWKEY | IDCMP_CLOSEWINDOW,
                          WA_Flags, WFLG_DRAGBAR | WFLG_CLOSEGADGET | WFLG_ACTIVATE, TAG_END);
    check("requester window opened", reqw != NULL);
}

static void closereq(void)
{
    if (reqw) {
        struct Message *m;
        while ((m = GetMsg(reqw->UserPort)) != NULL) ReplyMsg(m);
        CloseWindow(reqw);
        reqw = NULL;
    }
}

/* ------------------------------------------------------------ keys */

static struct IOStdReq conio;
static char typed[16];

static char convert(UWORD code, UWORD qual)
{
    struct InputEvent ie;
    UBYTE buf[4];
    LONG n;
    memset(&ie, 0, sizeof ie);
    ie.ie_Class = IECLASS_RAWKEY;
    ie.ie_Code = code;
    ie.ie_Qualifier = qual;
    n = RawKeyConvert(&ie, (STRPTR)buf, sizeof buf, NULL);
    return n == 1 ? buf[0] : '?';
}

/* ------------------------------------------------------------ events */

static int quit;

static void event(ULONG class, UWORD code, UWORD qual, APTR addr)
{
    o_str("teddrivetest: event ");
    o_dec(step);
    o_str(" class ");
    o_hex(class, 8);
    o_str(" code ");
    o_hex(code, 4);
    o_str(" qual ");
    o_hex(qual, 4);
    o_line();
    if (rescue) {
        switch (step++) {
        case 0:
            check("menu 0/1", class == IDCMP_MENUPICK && MENUNUM(code) == 0 && ITEMNUM(code) == 1);
            openreq();
            return;
        case 1:
            check("rescue: gadget 1", class == IDCMP_GADGETUP && addr == &gok);
            closereq();
            return;
        case 2:
            check("rescue: menu QUIT 0/9", class == IDCMP_MENUPICK && MENUNUM(code) == 0 && ITEMNUM(code) == 9);
            quit = 1;
            return;
        }
        check("unexpected event", 0);
        return;
    }
    switch (step++) {
    case 0:
        check("rawkey $20 -> a", class == IDCMP_RAWKEY && code == 0x20 && convert(code, qual) == 'a');
        return;
    case 1: case 2: case 3: case 4:
        if (class == IDCMP_RAWKEY) {
            char c[2] = { convert(code, qual), 0 };
            strcat(typed, c);
        }
        if (step == 5)
            check("type \"Ab1\\n\" -> Ab1\\r", !strcmp(typed, "Ab1\r"));
        return;
    case 5:
        check("menu 0/1", class == IDCMP_MENUPICK && MENUNUM(code) == 0 && ITEMNUM(code) == 1
                          && ITEMNUM(code) != NOITEM && SUBNUM(code) == NOSUB);
        openreq();
        return;
    case 6:
        check("gadget 1", class == IDCMP_GADGETUP && addr == &gok);
        check("string gadget 3 = hello", !strcmp((char *)strbuf, "hello") && strinfo.NumChars == 5);
        check("integer gadget 4 = 42", intinfo.LongInt == 42);
        closereq();
        return;
    case 7:
        check("menu 0/2 (busy)", class == IDCMP_MENUPICK && ITEMNUM(code) == 2);
        Delay(50);
        return;
    case 8:
        check("rawkey $21 -> s", class == IDCMP_RAWKEY && code == 0x21 && convert(code, qual) == 's');
        return;
    case 9:
        check("menu QUIT 0/9", class == IDCMP_MENUPICK && MENUNUM(code) == 0 && ITEMNUM(code) == 9);
        quit = 1;
        return;
    }
    check("unexpected event", 0);
}

int main(int argc, char **argv)
{
    struct MsgPort *tport = NULL;
    struct timerequest *treq = NULL;
    int timer = 0, timedout = 0, con = 0;
    UWORD pens[] = { (UWORD)~0 };

    rescue = argc > 1 && !strcmp(argv[1], "rescue");
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR)"intuition.library", 39);
    GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    if (!IntuitionBase || !GfxBase) {
        check("libraries", 0);
        goto done;
    }
    con = !OpenDevice((CONST_STRPTR)"console.device", (ULONG)-1, (struct IORequest *)&conio, 0);
    if (!con) {
        check("console.device", 0);
        goto done;
    }
    ConsoleDevice = conio.io_Device;
    tport = CreateMsgPort();
    treq = tport ? (struct timerequest *)CreateIORequest(tport, sizeof *treq) : NULL;
    if (!treq || OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)treq, 0)) {
        check("timer.device", 0);
        goto done;
    }
    timer = 1;
    treq->tr_node.io_Command = TR_ADDREQUEST;
    treq->tr_time.tv_secs = 20;
    treq->tr_time.tv_micro = 0;
    SendIO((struct IORequest *)treq);

    /* like TED: clone of the Workbench, depth 2 */
    scr = OpenScreenTags(NULL, SA_LikeWorkbench, TRUE, SA_Depth, 2, SA_Pens, (ULONG)pens,
                         SA_Title, (ULONG)"teddrivetest", TAG_END);
    if (!scr) {
        check("screen", 0);
        goto done;
    }
    makemenu();
    mainw = OpenWindowTags(NULL, WA_CustomScreen, (ULONG)scr, WA_Left, 0, WA_Top, 12,
                           WA_Width, scr->Width, WA_Height, scr->Height - 12,
                           WA_IDCMP, IDCMP_RAWKEY | IDCMP_GADGETUP | IDCMP_GADGETDOWN
                                     | IDCMP_MENUPICK | IDCMP_MOUSEBUTTONS,
                           WA_Flags, WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_ACTIVATE, TAG_END);
    if (!mainw) {
        check("main window", 0);
        goto done;
    }
    SetMenuStrip(mainw, &menu);
    SetAPen(mainw->RPort, 3);
    RectFill(mainw->RPort, 16, 20, 47, 27);         /* screen y 32..39 */
    o_str("teddrivetest: ready, mode ");
    o_str(rescue ? "rescue" : "main");
    o_str(", screen ");
    o_dec(scr->Width);
    o_str("x");
    o_dec(scr->Height);
    o_line();

    while (!quit) {
        /* like TED's pickwindow: only the newest window is read */
        struct MsgPort *port = reqw ? reqw->UserPort : mainw->UserPort;
        struct IntuiMessage *m;
        ULONG sig = Wait(1UL << port->mp_SigBit | 1UL << tport->mp_SigBit);
        if (sig & 1UL << tport->mp_SigBit) {
            check("TIMEOUT (20 s)", 0);
            timedout = 1;
            break;
        }
        while (!quit && (m = (struct IntuiMessage *)GetMsg(port)) != NULL) {
            ULONG class = m->Class;
            UWORD code = m->Code, qual = m->Qualifier;
            APTR addr = m->IAddress;
            ReplyMsg(&m->ExecMessage);              /* like handled0: reply first */
            if (class == IDCMP_RAWKEY && (code & IECODE_UP_PREFIX))
                continue;                       /* key up (real keyboard only) */
            event(class, code, qual, addr);
            /* teddrive must wait until we wait again */
            if (!quit) {
                struct MsgPort *p = reqw ? reqw->UserPort : mainw->UserPort;
                if (!IsListEmpty(&p->mp_MsgList))
                    check("port empty after handling (idle sync)", 0);
            }
        }
    }
    if (!timedout)
        check(rescue ? "all 3 events" : "all 10 events", step == (rescue ? 3 : 10));

done:
    closereq();
    if (mainw) {
        ClearMenuStrip(mainw);
        CloseWindow(mainw);
    }
    if (scr) CloseScreen(scr);
    if (timer) {
        if (!CheckIO((struct IORequest *)treq)) AbortIO((struct IORequest *)treq);
        WaitIO((struct IORequest *)treq);
        CloseDevice((struct IORequest *)treq);
    }
    if (treq) DeleteIORequest((struct IORequest *)treq);
    if (tport) DeleteMsgPort(tport);
    if (con) CloseDevice((struct IORequest *)&conio);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    o_str(mismatches ? "teddrivetest: FAIL " : "teddrivetest: OK ");
    o_dec(mismatches);
    o_str(" mismatches");
    o_line();
    return mismatches ? 5 : 0;
}
