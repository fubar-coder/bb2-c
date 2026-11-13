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
/* NEW (literal characters): the Latin-1 character getline inserts as a
   literal character for the event of getinput (germs_map applied): $A0 to
   $FF without bit 30 (bit 30: an umlaut of germs, typed as 26-31); 0 for
   any other event ($80-$9F are F1-F10 and Help, rawspecial) */
UBYTE litkey(LONG event);

/* rawed.obj convert (code+$10A), data part (NEW name): the special keys
   before RawKeyConvert, independent of the qualifier: $50-$59 (F1-F10) ->
   +$31 = $81-$8A, $4C-$4F (cursor) -> -$30 = $1C-$1F, $5F (Help) -> $8B.
   Returns the code, or 0 if `code` is no special key (RawKeyConvert then;
   src/rawkey.c). */
UWORD rawspecial(UWORD code);

/* ted.s:5214 fontcalc (chunk 05 code, here with the font data): glyph of
   character c (asm d0): font8 + (c&15) + (c>>4)*128, or font6 + (c&15) +
   (c>>4)*96 if fonthite == 6.  Glyph rows are 16 bytes apart.  The
   original's fonts have 128 glyphs and it reads behind them for c >= 128
   (survey-05 I.11); TED-C's have 256 (spec editor/screen-font), so every
   code 0-255 has a glyph. */
const UBYTE *fontcalc(UBYTE c);

/* resources (res.c: #embed of the original files at build time, see
   CMakeLists.txt) */
extern const UBYTE font6[1536];     /* ted.s:8070 chars.b: ted_font6.b */
extern const UBYTE font8[2048];     /* ted.s:8071 chars8.b: ted_font8.b */
extern const UBYTE sleep_[704];     /* ted.s:316  sleep.bin (8 frames x 88);
                                       `sleep` is a libc name */
extern const UBYTE pointer[64];     /* ted.s:319  pointer.bin */
extern const UBYTE srcinfo[396];    /* ted.s:827  src.info (DiskObject) */

#endif
