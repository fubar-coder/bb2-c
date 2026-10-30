/*
 * keymap.c - chunk 07 core + germs (6022) + fontcalc (5214): see keymap.h.
 *
 * The data parts of rawed.obj's convert and of handled0's umlaut
 * mapping, and the glyph lookup next to the font data.
 */
#include "tedcore.h"
#include "hooks.h"
#include "state.h"
#include "fileio.h"
#include "lines.h"
#include "loadsave.h"
#include "services.h"
#include "tokit.h"
#include "lineedit.h"
#include "keymap.h"

const UBYTE germs[2 * ngerms] = {     /* ted.s:6022 */
    228, 26, 246, 27, 252, 28,
    196, 29, 214, 30, 220, 31
};

/* handled0 RAWKEY without requester (ted.s line 5976 .notareq), data
   part (NEW name): d7 = c; found in germs -> replacement byte, bset #30 */
ULONG germs_map(UBYTE c)
{
    const UBYTE *pair = germs;                  /* a0 */
    WORD n;                                     /* d0 */

    for (n = ngerms - 1; n >= 0; n--) {         /* .gloop, dbf */
        if (*pair++ == c)
            return (ULONG)*pair | (1UL << 30);  /* .gskip */
        pair++;
    }
    return c;
}

/* rawed.obj convert (code+$10A), data part (NEW name): the compares in
   front of RawKeyConvert (rawed.obj.dsm L7-L9), all unsigned word
   compares, the qualifier is not looked at.  0 = no special key. */
UWORD rawspecial(UWORD code)
{
    if (code > 0x59) {
        if (code == 0x5f)       /* L9: Help */
            return 0x8b;
        return 0;
    }
    if (code >= 0x50)           /* F1-F10 */
        return code + 0x31;
    if (code >= 0x4c)           /* L8: cursor keys */
        return code - 0x30;
    return 0;
}

/* ted.s:5214 fontcalc */
const UBYTE *fontcalc(UBYTE c)
{
    UWORD high = c & 0xf0;                      /* d1 */

    /* no check: characters >= 128 lie behind the font (survey-05 I.11) */
    if (c >= 128)
        ORIG_HAZARD("fontcalc: character >= 128 behind the font");
    if (fonthite == 6)          /* .font6: (c >> 4) * 96 */
        return font6 + (c & 15) + (high << 1) + (high << 2);
    return font8 + (c & 15) + (high << 3);     /* (c >> 4) * 128 */
}
