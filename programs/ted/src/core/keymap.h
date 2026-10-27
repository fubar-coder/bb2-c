/*
 * keymap.h - chunk 07 core (+ the germs table of chunk 06 and fontcalc of
 * chunk 05): key code mapping of rawed.obj and handled0, font data access.
 * UI parts: src/rawkey.h (console.device), src/input.h (handled0),
 * src/screen.h (print4/qprint).
 */
#ifndef TED_KEYMAP_H
#define TED_KEYMAP_H

#include "tedcore.h"

/* ted.s:6022 germs: ngerms pairs Latin-1 umlaut -> TED charset code
   (228,26 246,27 252,28 196,29 214,30 220,31) */
extern const UBYTE germs[2 * ngerms];
/* handled0 RAWKEY without requester (ted.s:5950ff), data part (NEW name):
   c found in germs -> replacement | bit 30 (1 << 30); else c unchanged. */
ULONG germs_map(UBYTE c);

/* rawed.obj convert (code+$10A), data part (NEW name): the special keys
   before RawKeyConvert, independent of the qualifier: $50-$59 (F1-F10) ->
   +$31 = $81-$8A, $4C-$4F (cursor) -> -$30 = $1C-$1F, $5F (Help) -> $8B.
   Returns the code, or 0 if `code` is no special key (RawKeyConvert then;
   src/rawkey.c). */
UWORD rawspecial(UWORD code);

/* ted.s:5214 fontcalc (chunk 05 code, here with the font data): glyph of
   character d0 (0-127, no check: >= 128 reads behind the font, survey-05
   I.11): font8 + (c&15) + (c>>4)*128, or font6 + (c&15) + (c>>4)*96 if
   fonthite == 6.  Glyph rows are 16 bytes apart. */
const UBYTE *fontcalc(UBYTE d0);

/* resources (res.c: #embed of the original files at build time, see
   CMakeLists.txt) */
extern const UBYTE font6[768];      /* ted.s:8070 chars.b */
extern const UBYTE font8[1024];     /* ted.s:8071 chars8.b */
extern const UBYTE sleep_[704];     /* ted.s:316  sleep.bin (8 frames x 88);
                                       `sleep` is a libc name */
extern const UBYTE pointer[64];     /* ted.s:319  pointer.bin */
extern const UBYTE srcinfo[396];    /* ted.s:827  src.info (DiskObject) */

#endif
