/*
 * ab3libs.c - the libraries of AB3 mode (blitz2-ab3; extension points of
 * ext.h; spec ab3-compat/libraries, design D5 of the change ab3-mode-g1).
 *
 * AmiBlitz3's four BigLibs in its load order instead of acidlibs and
 * deflibs (loadlibgroup each), then every file of the flat userlibs
 * directory, a userlib replacing a loaded library of the same number
 * (AmiBlitz/AmiBlitz3@7ab0350b).  In BB2 mode every point handles
 * nothing: the chunk files load blitz2's libraries.
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "blitz2.h"
#include "ab3.h"

#define AB3LIBS 4
/* AmiBlitz3's paths of a default installation (AmiBlitz3.ab3:
   config\path_baselibs ... from the tool types, BlitzLibs/UserLibs) */
static const char *const ab3libname[AB3LIBS] = {
    "Blitz3:BlitzLibs/baseLibs", "Blitz3:BlitzLibs/utilLibs",
    "Blitz3:BlitzLibs/OsLibs", "Blitz3:BlitzLibs/csLibs",
};
#define AB3USERLIBS "Blitz3:BlitzLibs/UserLibs"
static ULONG ab3libs[AB3LIBS];          /* their sizes (makelibs, 0 = missing) */
static APTR ab3libsat[AB3LIBS];         /* the BigLib buffers (ab3libs bytes) */
const char *ab3liberr;                  /* ab3.h */

/* makelibs of AB3 mode: the sizes of the four files; the first missing
   one is the error of the compile.  No libslist: the userlibs directory
   is flat (ext_loaduserlibs) */
BOOL ext_makelibs(BOOL *ok)
{
    static char msg[sizeof "Library file not found: " + 32] = "Library file not found: ";
    int i;

    if (progmode != XTRA_MODE_AB3)
        return FALSE;
    ab3liberr = NULL;
    for (i = 0; i < AB3LIBS; i++) {
        ab3libs[i] = checklib(ab3libname[i]);
        if (!ab3libs[i] && !ab3liberr) {
            strcpy(msg + sizeof "Library file not found: " - 1, ab3libname[i]);
            ab3liberr = msg;
        }
    }
    *ok = ab3liberr == NULL;
    return TRUE;
}

/* loadlibs of AB3 mode: the four BigLibs */
BOOL ext_loadlibs(struct node **tail)
{
    int i;

    if (progmode != XTRA_MODE_AB3)
        return FALSE;
    for (i = 0; i < AB3LIBS; i++)
        ab3libsat[i] = loadlibgroup(ab3libs[i], (STRPTR)ab3libname[i], tail);
    return TRUE;
}

/* loadblitzlibs of AB3 mode: every file of the flat directory AB3USERLIBS
   (no .info, no directories) in directory order, as AmiBlitz3's
   load_userlibs.  A userlib with the number of a library loaded before
   replaces it (AmiBlitz/AmiBlitz3@7ab0350b read_userlibsfile: the first
   one of that number is unlinked): a BigLib's stays in its group buffer,
   an earlier userlib is freed.  (menu8, reloading, is an editor menu:
   never in AB3 mode.) */
BOOL ext_loaduserlibs(struct node *tail)
{
    char path[sizeof AB3USERLIBS + 1 + 108];
    size_t dirlen = sizeof AB3USERLIBS - 1;
    BPTR lk;

    if (progmode != XTRA_MODE_AB3)
        return FALSE;
    if (!(lk = Lock((STRPTR)AB3USERLIBS, ACCESS_READ)))
        return TRUE;
    memcpy(path, AB3USERLIBS, dirlen);
    path[dirlen++] = '/';
    if (Examine(lk, namebufffib) && namebufffib->fib_DirEntryType >= 0) {
        while (ExNext(lk, namebufffib)) {
            struct blitzlib *nl, *l;
            struct node *prev;

            if (namebufffib->fib_DirEntryType >= 0)
                continue;
            if (endininfo(namebufffib->fib_FileName))
                continue;
            strcpy(path + dirlen, namebufffib->fib_FileName);
            prev = tail;
            loadalib((STRPTR)path, &tail);
            if (tail == prev)
                continue;               /* could not be opened */
            tail->next = NULL;          /* the list ends here for now */
            nl = (struct blitzlib *)tail;
            for (prev = (struct node *)&firstlib; (l = (struct blitzlib *)prev->next) != nl;
                 prev = prev->next) {
                struct node *n;
                BOOL user;

                if (l->libnum != nl->libnum)
                    continue;
                /* a userlib if it comes after blitzlibsat */
                for (user = FALSE, n = blitzlibsat->next; n != (struct node *)nl && !user;
                     n = n->next)
                    user = n == (struct node *)l;
                prev->next = (struct node *)l->next;
                if ((struct node *)l == blitzlibsat)
                    blitzlibsat = prev;
                if (user) {
                    if (l->nrelocs)
                        FreeMem(l->relocs, l->nrelocs << 2);
                    FreeMem(l, l->size);
                }
                break;
            }
        }
    }
    UnLock(lk);
    tail->next = NULL;
    return TRUE;
}

/* freelibs: the BigLib buffers (of either mode: none loaded in BB2) */
void ext_freelibs(void)
{
    int i;

    for (i = 0; i < AB3LIBS; i++)
        if (ab3libs[i] && ab3libsat[i]) {
            APTR p = ab3libsat[i];
            ab3libsat[i] = NULL;
            FreeMem(p, ab3libs[i]);
        }
}
