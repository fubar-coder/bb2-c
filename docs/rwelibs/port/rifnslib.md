# rifnslib (93): FNS bitmap fonts

Reflective Images, "FNS Library Version 1.1" (1995). Prints text in
"FNS." bitmap fonts into a Blitz bitmap, by the CPU: 16 font slots (slot 0
the font built into the library), clipping zone, origin, tab size, prefs
(bit 0 centred, 1 bold, 2 underlined, 3 right aligned), colour changes in
the text (`chr$(1)` + colour), tabs, new lines (`chr$(10)`), a shadow
form; `FNSLoad`/`FNSUnload` load a font into RIAmosFuncLib bank 55 + slot.
`FNSPrint` returns in a0 the address of the end position (x, y words).

A font: `FNS.`, +4 height, +6 width in pixels (16, 32, 48, 64; anything
else is drawn as 16), +8 underline offset, +10 bytes per character; the
width table is indexed by the character code from +0 (codes 32..159), the
glyphs from +256, row by row.

* **Source:** `rwelibsrc/FNSLib/fns-library11.bb` (as TED shows it:
  `build/amiga/rwelibsrc/FNSLib/fns-library11.bb`); it `IncBin`s
  `FNS:Personal.FNS`.
* **Reference:** BSS2 `RIFNSLIB.OBJ` (= AB3 `Rifnslib.obj`), newer than
  the source (`docs/rwelibs/README.md` §3.2); declarations equal (ctest
  `rwe_declarations_rifnslib`).
* **C file:** `libs/userlibs/rwe/rifnslib.c`; test
  `libs/tests/rwe/rifnslib.c` (snitch case "rwe rifnslib").

## The binary's state

Checked in the disassembly (code at `$5A8`, variables `$B66`–`$BF7`, font
`$BF8`–`$12A7`):

* `end_count_loop` (`$704`) is `ADDQ.w #1,d5` directly: the source's
  `TST.w d5:BEQ exit` is not there. With the guard a line that begins
  with `chr$(10)` (a leading or an empty line) ends the print and returns
  a stale end position; the binary prints the rest. The C build has the
  binary's code (the test prints `"\nleading"` and `"a\n\nb"`).
* `BRA 'p5_5` (`$828`) and `BEQ 'not_bold` (`$84A`) are short branches –
  no behaviour.
* FNSShadowPrint's help text is
  `"fontnum,x,y,a$/string_address,[prefs,colour[,shadow]]"` (the source:
  `...,[prefs,[colour,shadow]]`), as the BSS2 manual prints it.
* The font is the CD's `BSS2/REDBLITZLIBS/EXAMPLES/FNS/FONTS/PERSONAL.FNS`
  (1712 bytes) except 7 words at font offset `$4B0`–`$4BC` (an arrow
  glyph one pixel further left: `1000 1800 1c00 fe00 1c00 1800 1000`
  instead of `0800 0c00 0e00 ff00 0e00 0c00 0800`) – checked with `cmp`.

## C and assembler

The variables are one packed struct in the original's order (`temp` …
`font_add`, the font behind), so the original's overruns land in the
same places (bitstack for a depth above 8, FNSLength's unmasked font
number), and FNSPrint's a0 and FNSSlot point into it as in the original;
`_Static_assert`s check the offsets the original has. Everything is C
behind thunks except: the FNSPrint/FNSShadowPrint entries (assembler:
without a bitmap the original pushes 13 registers and returns with RTS –
to the pushed d0, a crash; a stack imbalance C cannot express – they
load the default prefs/colour for the short forms and call the C code
otherwise), and FNSVersion (an inline routine, `BLITZ_INLINE`). FNSLoad
calls RIAmosFuncLib's BLoad (form 1), Start and Erase with
`BLITZ_ALIBJSR`/`BL_JSR_D4_` (tokens `#riamosfunclib<<7` + 15/17/19, as
ripacklib). The register-level details are kept: word and byte widths,
`ASR.l` of the long in the centring, `MULU.w`/`DIVU.w` (`bl_mulu_w`,
`bl_divu_w`), signed/unsigned compares, DBRA on bytes and words, the
glyph `ROR.l`/`SWAP` as `data << (16 - x & 15)`.

## BUG(orig), kept

* FNSPrint/FNSShadowPrint before FNSOutput: a crash (above).
* An empty text, or a line starting with `chr$(10)`, counts width 1:
  underlined it sets 1 pixel, centred x moves by −32767.
* FNSLength returns width − 2 (at least 0), takes the prefs of the last
  FNSPrint when called without prefs, and does not mask the font number
  (`ASL.w #2` of the word): 16 on read the built-in font's bytes as font
  pointers, negative numbers the variables before `font_add`.
* A bitmap depth of 0 or above 8 overruns `bitstack` (256 / depth plane
  pointers over the variables and the font); the 16-pixel loop draws
  `(Flags<<8 | Depth)` planes, the 32/48/64-pixel loops `Depth` (256 for
  0), planes from 8 on with the colour bit `plane & 7`.
* Rows are clipped signed at ymin, unsigned at ymax; 48/64-pixel fonts
  read their 3rd/4th glyph word only for rows that are drawn, so after a
  clipped row the glyph is shifted.
* FNSSetTab 0: a tab divides by 0. The underline is clipped at x = 0, not
  at xmin. A font height of 0 draws 65536 rows. Codes from 160 end the
  text (signed byte compare).

## DIFF

What the original reads behind its variables or its font is its own
code; the C build reads other bytes there: FNSLength with a font number
outside the block, and glyphs of codes the font has no data for – the
built-in font has glyphs only up to `z` (91), codes 123..159 read behind
it.

## The font

The library builds in 1712 bytes of font (slot 0 after init; FNSPrint,
FNSHeight ... with font 0 use it), `IncBin "FNS:Personal.FNS"` in the
source. The font is not part of RWE's 1999 release (`rwelibsrc/` has no
`.FNS` file); AmiBlitz3 publishes it (GitHub `AmiBlitz/AmiBlitz3`, Apache
2.0, `Sourcecodes/Data/fns/fonts/PERSONAL.FNS`, byte-identical to the CD's
`BSS2/REDBLITZLIBS/EXAMPLES/FNS/FONTS/PERSONAL.FNS`), and AmiBlitz3's
`Rifnslib.obj` is byte-identical to the BSS2 binary. Decision of
2026-10-29: the unchanged file is in the repository as
`archive/amiblitz3-github/Sourcecodes/Data/fns/fonts/PERSONAL.FNS`
(origin and license in `archive/README.md`). The C file `#embed`s it
(its size checked with `_Static_assert`) and puts the 7 words at
`$4B0`–`$4BC` in the state of the library's binary between two `#embed`s,
so the built-in font equals the binary's byte for byte.

## Test

Bitmaps in memory (40 × 64 bytes, depth 1..8, planes in one buffer with
guards per set), fonts made in the test (16, 32, 48, 64 pixels and a
24-pixel one drawn as 16; random glyphs, widths 0..width), the built-in
font, RIAmosFuncLib replaced by stubs (BLoad/Start/Erase, calls
recorded). Compared with the original: the whole bitmap buffer after every
print, the end position at a0 (or a0 unchanged without a font), d0, the
kept registers, the stub calls. Covered: see the header of the test (all
entries and forms, all prefs, colours and `chr$(1)`, tabs and FNSSetTab,
new lines including leading/double ones, zero-width characters, codes
from 160, every x bit position, xmax, xmin, rows clipped at ymin/ymax for
every font width, empty texts, origin, FNSLength's unmasked numbers 16,
17, −1, −2, InstallFNS of a non-font, FNSLoad ok/BLoad fails/Start 0,
FNSUnload). Not covered: the crash without a bitmap, depth 0 / above 8 /
flags, FNSSetTab 0, negative xmin (writes 64 KB away), font 0 with the
codes behind `z` (the DIFF).
