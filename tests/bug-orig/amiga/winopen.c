/*
 * winopen - open four small windows on the Workbench screen (three with
 * OpenWindow, the last with OpenWindowTagList) and print for each whether
 * it opened, for the self test of inject's openwindowfail= option
 * (tests/bug-orig/inject-openwindow.cmake):
 *
 *   window 1: ok
 *   window 2: FAILED
 *
 * usage: winopen        (rc 0; the windows are closed again)
 */
#include <exec/types.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>

struct IntuitionBase *IntuitionBase;

int main(void)
{
    static struct NewWindow nw = { 20, 20, 120, 40, 0, 1, 0, WINDOWDRAG | ACTIVATE, NULL, NULL,
                                   (UBYTE *)"winopen", NULL, NULL, 50, 20, 640, 200,
                                   WBENCHSCREEN };
    struct Window *w[4];
    char line[40];
    int i;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
    if (!IntuitionBase)
        return 20;
    for (i = 0; i < 4; i++) {
        if (i < 3) {
            nw.LeftEdge = (WORD)(20 + 30 * i);
            w[i] = OpenWindow(&nw);
        } else {
            w[i] = OpenWindowTags(&nw, WA_Title, (ULONG)"winopen", TAG_DONE);
        }
        sprintf(line, "window %d: %s\n", i + 1, w[i] ? "ok" : "FAILED");
        Write(Output(), line, strlen(line));
    }
    for (i = 0; i < 4; i++)
        if (w[i])
            CloseWindow(w[i]);
    CloseLibrary((struct Library *)IntuitionBase);
    return 0;
}
