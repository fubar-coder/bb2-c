/*
 * ab3res.c - the residents of AB3 mode and the set-up errors of its
 * compile (blitz2-ab3; extension points of ext.h, spec
 * ab3-compat/libraries).  In BB2 mode loadres loads a resident as blitz2.
 */
#include <string.h>
#include <proto/dos.h>
#include "blitz2.h"
#include "ab3.h"

/* AmiBlitz3.ab3: config\path_residents */
#define AB3RESIDENTS "Blitz3:BlitzLibs/Residents/"

/* the error of the first resident that could not be loaded ("Can't Load
   Resident: NAME"), NULL if none: the compile reports it */
static const char *ab3reserr;

void ext_resbegin(void)
{
    ab3reserr = NULL;
}

/* AB3 mode: a resident named without a device or directory is AmiBlitz3's,
   in AB3RESIDENTS; one that cannot be loaded is the error of the compile
   (the first one; the others are still loaded) */
BOOL ext_loadres(struct resentry *res)
{
    static char msg[sizeof "Can't Load Resident: " + sizeof resbuff->name]
        = "Can't Load Resident: ";
    char path[sizeof AB3RESIDENTS + sizeof resbuff->name];
    const char *name = res->name;

    if (progmode != XTRA_MODE_AB3)
        return FALSE;
    if (strchr(name, ':') || strchr(name, '/'))
        res->seg = LoadSeg((CONST_STRPTR)name);
    else {
        strcpy(path, AB3RESIDENTS);
        strncat(path, name, sizeof resbuff->name - 1);
        res->seg = LoadSeg((CONST_STRPTR)path);
    }
    if (!res->seg && !ab3reserr) {
        msg[sizeof "Can't Load Resident: " - 1] = 0;
        strncat(msg, name, sizeof resbuff->name - 1);
        ab3reserr = msg;
    }
    return TRUE;
}

/* AB3 mode: a library file or a resident that could not be loaded stops
   the compile */
void ext_compilestart(void)
{
    if (progmode == XTRA_MODE_AB3 && ab3liberr)
        err(ab3liberr);
    if (progmode == XTRA_MODE_AB3 && ab3reserr)
        err(ab3reserr);
}
