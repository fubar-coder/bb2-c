/*
 * rwe popupwinlib - the C build of popupwinlib (userlib 16) or the
 * reference build of rwelibsrc/popuplib/popupwinlib.bb
 * (ref/popupwinlib.obj, see libs/userlibs/rwe/refbuild.cmake), one set as
 * LIBSET says, observed as cases (rwecmp.h; the expected
 * data libs/tests/expected/rwe_popupwinlib.txt).
 *
 *   popupwinlib DIR     (DIR with popupwinlib .obj/.orig)
 *
 * PopUpWindow waits for the mouse, so the test drives it: exec's
 * WaitPort is patched (SetFunction) while a popup runs, and when the
 * library waits on its port, the patch takes a snapshot of the popup
 * windows, sets their MouseX/MouseY to the next position of a script
 * (the library takes the position from there, not from the messages)
 * and puts the script's IntuiMessage (MOUSEMOVE, MOUSEBUTTONS,
 * INACTIVEWINDOW) into the port.  The windows open on the Workbench
 * screen at 0,0 (the screen object is the Workbench screen)
 * and are all closed by the library before PopUpWindow returns.
 * Observed per snapshot: the popup windows (position, size, IDCMP) and
 * their contents (copied with ClipBlit, hashed), bit 1 of CIA-A PRA
 * relative to the start (BUG(orig) #468: toggled by a menu with sub menus);
 * the number of waits and menu levels, d0 (BUG(orig) #471: always 0), the
 * kept registers; checked: no memory lost after the first popup (the
 * first windows leave memory in the system's caches).
 * Covered: init; PopUpWindow with three levels: highlighting items, a
 * separator, the mouse outside all windows, an INACTIVEWINDOW message,
 * a sub menu opened by moving onto its item, back to the parent (closes
 * the sub menu), onto the parent's active item (the sub menu stays), a
 * third level left for the first (two levels closed), a mouse button
 * with and without an active item; a menu without sub menus that starts
 * with a separator; PopUpInit (a new port, BUG(orig) #470: the old one is
 * not deleted - the test deletes it); finit.
 * Not covered: a failed AllocVec or OpenWindow (BUG(orig) #467: not
 * checked), real mouse input.
 */
#include "rwecmp.h"
#include <exec/memory.h>
#include <exec/execbase.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/intuition.h>
static struct Library *pugfx;
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME pugfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

#define LIB "popupwinlib"
enum { E_INIT, E_POPUP };
#define LVO_WaitPort (-384)

static struct Screen *scr;
static ULONG scrobj[2];                 /* screenslib objects: +0 the screen */
static struct Task *me;
static struct MsgPort *replyport;
static APTR old_waitport;
static int script_on;

/* the items: +0 next, +4 text, +12 sub menu */
static UBYTE items[16][24];

/* the script: a message class and the mouse position, relative to the
   window of menu level `level` (-1: absolute, on the screen) */
struct ev { ULONG cls; int level; WORD x, y; };
#define MAXEV 24
static struct ev script[MAXEV];
static int nscript, nev;
static struct IntuiMessage msgs[MAXEV];
static struct MsgPort *port;            /* the library's port */

/* the snapshots */
#define MAXWIN 3
struct snap { WORD l, t, w, h; ULONG idcmp, hash; };
static struct snap snaps[MAXEV][MAXWIN];
static int nwins[MAXEV];
static UBYTE cia0, cia[MAXEV];
static struct RastPort trp;
static struct BitMap *tbm;

#define CIAAPRA (*(volatile UBYTE *)0xbfe001)

/* the popup windows of the port, oldest (level 0) first */
static int popups(struct MsgPort *p, struct Window **w)
{
    struct Window *x;
    int n = 0, i;

    Forbid();
    for (x = scr->FirstWindow; x; x = x->NextWindow)
        if (x->UserPort == p && n < MAXWIN)
            w[n++] = x;
    Permit();
    for (i = 0; i < n / 2; i++) {
        x = w[i];
        w[i] = w[n - 1 - i];
        w[n - 1 - i] = x;
    }
    return n;
}

/* the contents of a window: copied to the test's BitMap, hashed */
static ULONG winhash(struct Window *w)
{
    ULONG h = 5381;
    int d, y, x, bpr = (w->Width + 7) / 8;

    ClipBlit(w->RPort, 0, 0, &trp, 0, 0, w->Width, w->Height, 0xc0);
    WaitBlit();
    for (d = 0; d < tbm->Depth; d++)
        for (y = 0; y < w->Height; y++) {
            UBYTE *row = tbm->Planes[d] + y * tbm->BytesPerRow;
            for (x = 0; x < bpr; x++) {
                UBYTE b = row[x];
                if (x == bpr - 1 && (w->Width & 7))
                    b &= (UBYTE)(0xff00 >> (w->Width & 7));
                h = h * 33 + b;
            }
        }
    return h;
}

static struct Message *callold(struct MsgPort *p)
{
    register struct MsgPort *a0 __asm("a0") = p;
    register APTR a1 __asm("a1") = old_waitport;
    register struct ExecBase *a6 __asm("a6") = SysBase;
    register ULONG d0 __asm("d0");

    __asm volatile("jsr (%2)" : "=r"(d0), "+r"(a0), "+r"(a1) : "r"(a6) : "d1", "cc", "memory");
    return (struct Message *)d0;
}

/* exec WaitPort while a popup runs: snapshot, the next mouse position
   and message of the script */
static struct Message *hook_waitport(struct MsgPort *p __asm("a0"))
{
    if (script_on && FindTask(NULL) == me) {
        struct Window *w[MAXWIN];
        struct IntuiMessage *m;
        struct ev e = { IDCMP_MOUSEBUTTONS, -1, 600, 250 };     /* the end */
        int n, k = nev++, i, sx, sy;

        while (GetMsg(replyport))
            ;
        port = p;
        n = popups(p, w);
        if (k < MAXEV) {
            nwins[k] = n;
            cia[k] = (CIAAPRA & 2) ^ cia0;
            for (i = 0; i < n; i++) {
                struct snap *s = &snaps[k][i];
                s->l = w[i]->LeftEdge;
                s->t = w[i]->TopEdge;
                s->w = w[i]->Width;
                s->h = w[i]->Height;
                s->idcmp = w[i]->IDCMPFlags;
                s->hash = winhash(w[i]);
            }
        }
        if (k < nscript)
            e = script[k];
        sx = e.x;
        sy = e.y;
        if (e.level >= 0 && e.level < n) {
            sx += w[e.level]->LeftEdge;
            sy += w[e.level]->TopEdge;
        }
        for (i = 0; i < n; i++) {
            w[i]->MouseX = sx - w[i]->LeftEdge;
            w[i]->MouseY = sy - w[i]->TopEdge;
        }
        m = &msgs[k < MAXEV ? k : MAXEV - 1];
        memset(m, 0, sizeof *m);
        m->ExecMessage.mn_ReplyPort = replyport;
        m->ExecMessage.mn_Length = sizeof *m;
        m->Class = e.cls;
        m->Code = e.cls == IDCMP_MOUSEBUTTONS ? SELECTDOWN : 0;
        m->MouseX = sx;
        m->MouseY = sy;
        PutMsg(p, &m->ExecMessage);
    }
    return callold(p);
}

/* -------------------------------------------------------------- tests */

static void item(int i, int next, const char *text, int subs)
{
    memset(items[i], 0, sizeof items[i]);
    *(UBYTE **)(items[i] + 0) = next >= 0 ? items[next] : NULL;
    *(const char **)(items[i] + 4) = text;
    *(UBYTE **)(items[i] + 12) = subs >= 0 ? items[subs] : NULL;
}

static void ev(ULONG cls, int level, int x, int y)
{
    if (nscript < MAXEV)
        script[nscript++] = (struct ev){ cls, level, x, y };
}

static void popup(const char *what, int first, int levels)
{
    int k, i, most = 0;

    ROUT = "PopUpWindow";
    strcpy(VEC, what);
    script_on = 1;
    DO1(ENT(LIB, E_POPUP, 0),
        nev = 0; cia0 = CIAAPRA & 2;
        R.d[0] = MARK(0); R.d[1] = (ULONG)items[first];
        R.a[0] = (ULONG)scrobj);
    script_on = 0;
    while (GetMsg(replyport))
        ;
    BOTH("d0 0 (BUG(orig) #471)", R.d[0] == 0);
    BOTH("the whole script", nev == nscript);
    OBS("waits", nev);
    for (k = 0; k < nev && k < MAXEV; k++) {
        char w[160];
        if (nwins[k] > most)
            most = nwins[k];
        snprintf(w, sizeof w, "wait %d windows", k);
        OBS(w, nwins[k]);
        snprintf(w, sizeof w, "wait %d CIA-A PRA bit 1", k);
        OBS(w, cia[k]);
        for (i = 0; i < nwins[k]; i++) {
            struct snap *s = &snaps[k][i];
            snprintf(w, sizeof w, "wait %d window %d position", k, i);
            OBS(w, (ULONG)(UWORD)s->l << 16 | (UWORD)s->t);
            snprintf(w, sizeof w, "wait %d window %d size", k, i);
            OBS(w, (ULONG)(UWORD)s->w << 16 | (UWORD)s->h);
            snprintf(w, sizeof w, "wait %d window %d IDCMP", k, i);
            OBS(w, s->idcmp);
            snprintf(w, sizeof w, "wait %d window %d contents", k, i);
            OBS(w, s->hash);
        }
    }
    BOTH("the menu levels opened", most == levels);
    BOTH("no popup window left", popups(port, (struct Window *[MAXWIN]){ 0 }) == 0);
}

/* the code of a library's init (header +22) or finit (+28) nullsub */
static APTR nullsub(int off)
{
    struct lib *l = getlib(&loaded, LIB);
    UBYTE *p = (UBYTE *)L(l->code + off);
    p += 6;
    while (W(p))
        p += 4;
    return (APTR)L(p + 2 + 4);
}

int main(int argc, char **argv)
{
    static const char *names[] = { LIB, NULL };
    struct MsgPort *oldport;
    ULONG avail;
    LONG ports;
    int s, ih, y0, yA, yB, ySep, yC;

    if (argc < 2) {
        printf("usage: popupwinlib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("init", d2-d7|a2|a3);
    KEEPS("PopUpWindow", a3|a6);
    KEEPS("PopUpInit", d2-d7|a2|a3);
    KEEPS("finit", d2-d7|a2|a3);
    pugfx = OpenLibrary((STRPTR)"graphics.library", 39);
    scr = LockPubScreen(NULL);
    replyport = CreateMsgPort();
    if (!pugfx || !scr || !replyport) {
        check("popupwinlib: graphics.library 39, the Workbench screen, a port", FALSE);
        return 20;
    }
    tbm = AllocBitMap(scr->Width, scr->Height, GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH),
                      BMF_CLEAR, scr->RastPort.BitMap);
    if (!tbm) {
        check("popupwinlib: a BitMap", FALSE);
        return 20;
    }
    InitRastPort(&trp);
    trp.BitMap = tbm;
    scrobj[0] = (ULONG)scr;
    me = FindTask(NULL);
    Forbid();
    old_waitport = SetFunction((struct Library *)SysBase, LVO_WaitPort, (APTR)hook_waitport);
    Permit();

    ROUT = "init";
    strcpy(VEC, "");
    DO1(nullsub(22), (void)0);

    /* the menus: 0 Open, 1 Save, 2 ---, 3 More >, 4 Quit;
       5 Sub 1, 6 Sub 2 >; 7 Deep, 8 Deeper */
    item(0, 1, "Open", -1);
    item(1, 2, "Save", -1);
    item(2, 3, NULL, -1);
    item(3, 4, "More", 5);
    item(4, -1, "Quit", -1);
    item(5, 6, "Sub 1", -1);
    item(6, -1, "Sub 2", 7);
    item(7, 8, "Deep", -1);
    item(8, -1, "Deeper", -1);
    ih = scr->RastPort.Font->tf_YSize + 6;
    y0 = 2 + ih / 2;                    /* the middle of the first item */
    yA = y0;
    yB = y0 + ih;
    ySep = 2 + 2 * ih + 2;
    yC = 2 + 2 * ih + 4 + ih / 2;
    nscript = 0;
    ev(IDCMP_MOUSEMOVE, 0, 10, yA);     /* Open */
    ev(IDCMP_MOUSEMOVE, 0, 10, yB);     /* Save */
    ev(IDCMP_MOUSEMOVE, 0, 10, ySep);   /* the separator: none */
    ev(IDCMP_MOUSEMOVE, 0, 12, yB);     /* Save */
    ev(IDCMP_MOUSEMOVE, -1, 600, 250);  /* outside: none */
    ev(IDCMP_INACTIVEWINDOW, -1, 600, 250);
    ev(IDCMP_MOUSEMOVE, 0, 10, yA);     /* Open */
    ev(IDCMP_MOUSEMOVE, 0, 10, yC);     /* More: the sub menu opens */
    ev(IDCMP_MOUSEMOVE, 1, 10, y0);     /* Sub 1 */
    ev(IDCMP_MOUSEMOVE, 0, 10, yB);     /* back to Save: the sub menu closes */
    ev(IDCMP_MOUSEMOVE, 0, 10, yC);     /* More again */
    ev(IDCMP_MOUSEMOVE, 0, 14, yC);     /* still on More: the sub menu stays */
    ev(IDCMP_MOUSEMOVE, 1, 10, y0 + ih);        /* Sub 2: the third level */
    ev(IDCMP_MOUSEMOVE, 2, 10, y0 + ih);        /* Deeper */
    ev(IDCMP_MOUSEMOVE, 0, 10, yA);     /* to Open: two levels closed */
    ev(IDCMP_MOUSEMOVE, 0, 10, yC);     /* More */
    ev(IDCMP_MOUSEMOVE, 1, 10, y0);     /* Sub 1 */
    ev(IDCMP_MOUSEBUTTONS, 1, 10, y0);  /* chosen: all closed */
    popup("three levels", 0, 3);
    /* the memory from here on: the first windows leave memory in the
       system's caches (layers' ClipRects ...) */
    avail = AvailMem(MEMF_ANY);
    rwe_memothers = TRUE;
    rwe_mem_start();

    nscript = 0;
    ev(IDCMP_MOUSEMOVE, 0, 10, yA);     /* Open */
    ev(IDCMP_MOUSEMOVE, -1, 600, 250);
    ev(IDCMP_MOUSEBUTTONS, -1, 600, 250);       /* nothing active */
    popup("a button without an item", 0, 1);

    /* PopUpInit: a new port; the old one stays */
    oldport = port;
    ROUT = "PopUpInit";
    DO1(ENT(LIB, E_INIT, 0), R.d[0] = MARK(0));
    item(9, 10, NULL, -1);
    item(10, 11, "One", -1);
    item(11, -1, "Two", -1);
    nscript = 0;
    ev(IDCMP_MOUSEMOVE, 0, 10, 2 + 4 + ih / 2);         /* One */
    ev(IDCMP_MOUSEBUTTONS, 0, 10, 2 + 4 + ih / 2);
    popup("starting with a separator, a new port", 9, 1);
    BOTH("a new port", port != oldport);

    ROUT = "finit";
    strcpy(VEC, "");
    DO1(nullsub(28), (void)0);
    DeleteMsgPort(oldport);             /* BUG(orig) (#470): left by PopUpInit */

    Forbid();
    SetFunction((struct Library *)SysBase, LVO_WaitPort, old_waitport);
    Permit();
    /* init's ports are freed now: this task's balance (rwemem.h) is
       just the port of the first init, once input.device has freed what
       intuition allocated in this task (a few blocks of 36 bytes; wait up
       to a second) */
    ports = (LONG)RWE_MEMSIZE(sizeof(struct MsgPort));
    for (s = 0; s < 50 && rwe_membal - rwe_memotherfree != -ports; s++)
        Delay(1);
    {
        LONG own = rwe_mem_stop(), bal = own + ports;
        LONG other = (LONG)(avail - AvailMem(MEMF_ANY)) - own;
        if (other)
            printf("popupwinlib: %ld bytes allocated by other tasks meanwhile\n", other);
        if (bal)
            printf("popupwinlib: %ld bytes not freed\n", bal);
        check("popupwinlib: all memory back", bal == 0 && rwe_memallocs > 0 && !rwe_memfull);
    }
    FreeBitMap(tbm);
    DeleteMsgPort(replyport);
    UnlockPubScreen(NULL, scr);
    CloseLibrary(pugfx);
    return rwe_done(LIB);
}
