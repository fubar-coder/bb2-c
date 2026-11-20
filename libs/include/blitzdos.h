/*
 * blitzdos.h - dos.library helpers from library code.
 *
 *   bl_finddosentry_v34   look up a DOS list entry below dos.library V36
 *
 * Only what several libraries need; anything specific to one library
 * stays in its source.
 */
#ifndef BLITZDOS_H
#define BLITZDOS_H

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

/* the character in upper case as utility.library's ToUpper: a-z and the
   Latin-1 letters $E0-$FE except $F7 */
static inline UBYTE bl_dosupper(UBYTE c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 0xe0 && c <= 0xfe && c != 0xf7))
        c -= 0x20;
    return c;
}

/* bl_finddosentry_v34(dos, name, flags) - the entry of the DOS device
   list named `name` (without ":", compared without case) whose type
   `flags` asks for: LDF_DEVICES for DLT_DEVICE, LDF_VOLUMES for
   DLT_VOLUME, LDF_ASSIGNS for the assigns (DLT_DIRECTORY, DLT_LATE,
   DLT_NONBINDING), as FindDosEntry; NULL if there is none.
   For dos.library below V36, which has no LockDosList: the list is
   walked from DosInfo (dl_Root->rn_Info->di_DevInfo).  The caller holds
   Forbid for the walk and for every read of the entry, as programs did
   before LockDosList existed. */
static inline struct DosList *bl_finddosentry_v34(const struct DosLibrary *dos,
                                                  const UBYTE *name, ULONG flags)
{
    const struct DosInfo *di =
        (const struct DosInfo *)BADDR(dos->dl_Root->rn_Info);
    struct DosList *dl = (struct DosList *)BADDR(di->di_DevInfo);

    for (; dl; dl = (struct DosList *)BADDR(dl->dol_Next)) {
        ULONG want = dl->dol_Type == DLT_DEVICE ? LDF_DEVICES
                   : dl->dol_Type == DLT_VOLUME ? LDF_VOLUMES : LDF_ASSIGNS;
        const UBYTE *b = (const UBYTE *)BADDR(dl->dol_Name);
        const UBYTE *n = name;
        ULONG len, i;

        if (!(flags & want) || !b)
            continue;
        len = *b++;
        for (i = 0; i < len && n[i]; i++)
            if (bl_dosupper(b[i]) != bl_dosupper(n[i]))
                break;
        if (i == len && !n[i])
            return dl;
    }
    return NULL;
}

#endif
