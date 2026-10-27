/*
 * gadtoolslib - deflibs library 236: AmigaOS calls of gadtools.library
 * (19 entries).
 *
 * Generated once by tools/mkdeflibs.py from cd/blitz2/deflibs
 * (original: cd/extras/blitzlibs/amigalibs/gadtools.library1);
 * since then this file is the source.  Structure: notes/deflibs.md.
 *
 *   init:  OpenLibrary("gadtools.library", 0) -> d0 = base (return long:
 *          the compiler keeps it for the calls, a6).  BUG(orig):
 *          retries forever if the library cannot be opened
 *          (deflib.h)
 *   finit: CloseLibrary(base), a1 = base (!libs LIB_gadtoolslib,#la1)
 *
 * Every call: BLITZ_OSCALL(base lib, LVO, name, help, argument
 * registers); the compiler generates the call (blitz2.s stamigalib).
 */
#include "blitzlib.h"
#include "deflib.h"

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);
static ULONG libinit(void);
static void libfinit(struct Library *base BL_REG(a1));

BLITZ_OSLIBHEADER(LIB_gadtoolslib, init, BL_LONGRET, finit);

BLITZ_OSCALL(LIB_gadtoolslib, -30, "CreateGadgetA_", "(kind,gad,ng,taglist)", d0, a0, a1, a2);
BLITZ_OSCALL(LIB_gadtoolslib, -36, "FreeGadgets_", "(gad)", a0);
BLITZ_OSCALL(LIB_gadtoolslib, -42, "GT_SetGadgetAttrsA_", "(gad,win,req,taglist)", a0, a1, a2, a3);
BLITZ_OSCALL(LIB_gadtoolslib, -48, "CreateMenusA_", "(newmenu,taglist)", a0, a1);
BLITZ_OSCALL(LIB_gadtoolslib, -54, "FreeMenus_", "(menu)", a0);
BLITZ_OSCALL(LIB_gadtoolslib, -60, "LayoutMenuItemsA_", "(firstitem,vi,taglist)", a0, a1, a2);
BLITZ_OSCALL(LIB_gadtoolslib, -66, "LayoutMenusA_", "(firstmenu,vi,taglist)", a0, a1, a2);
BLITZ_OSCALL(LIB_gadtoolslib, -72, "GT_GetIMsg_", "(iport)", a0);
BLITZ_OSCALL(LIB_gadtoolslib, -78, "GT_ReplyIMsg_", "(imsg)", a1);
BLITZ_OSCALL(LIB_gadtoolslib, -84, "GT_RefreshWindow_", "(win,req)", a0, a1);
BLITZ_OSCALL(LIB_gadtoolslib, -90, "GT_BeginRefresh_", "(win)", a0);
BLITZ_OSCALL(LIB_gadtoolslib, -96, "GT_EndRefresh_", "(win,complete)", a0, d0);
BLITZ_OSCALL(LIB_gadtoolslib, -102, "GT_FilterIMsg_", "(imsg)", a1);
BLITZ_OSCALL(LIB_gadtoolslib, -108, "GT_PostFilterIMsg_", "(imsg)", a1);
BLITZ_OSCALL(LIB_gadtoolslib, -114, "CreateContext_", "(glistptr)", a0);
BLITZ_OSCALL(LIB_gadtoolslib, -120, "DrawBevelBoxA_", "(rport,left,top,width,height,taglist)", a0, d0, d1, d2, d3, a1);
BLITZ_OSCALL(LIB_gadtoolslib, -126, "GetVisualInfoA_", "(screen,taglist)", a0, a1);
BLITZ_OSCALL(LIB_gadtoolslib, -132, "FreeVisualInfo_", "(vi)", a0);
BLITZ_OSCALL(LIB_gadtoolslib, -174, "GT_GetGadgetAttrsA_", "(gad,win,req,taglist)", a0, a1, a2, a3);

BLITZ_NULLSUB_AT(init, libinit, 0, 0);
BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_gadtoolslib, BL_LA(1));
BLITZ_LIBFIN();

static const char libname[] = "gadtools.library";

/* init nullsub: d0 = library base */
static ULONG BLITZ_ROUTINE libinit(void)
{
    return deflib_openlibrary(libname);
}

/* finit nullsub: a1 = library base */
static void BLITZ_ROUTINE libfinit(struct Library *base BL_REG(a1))
{
    CloseLibrary(base);                 /* MOVE.l 4,a6:JMP CloseLibrary(a6) */
}
