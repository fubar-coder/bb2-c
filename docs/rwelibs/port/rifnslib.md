# rifnslib (93): FNS bitmap fonts

Reflective Images, "FNS Library Version 1.1" (1995). Prints text in
"FNS." bitmap fonts into a Blitz bitmap, by the CPU: 16 font slots (slot 0
the font built into the library), clipping zone, origin, tab size, prefs
(bit 0 centred, 1 bold, 2 underlined, 3 right aligned), colour changes in
the text (`chr$(1)` + colour), tabs, new lines (`chr$(10)`), a shadow
form; `FNSLoad`/`FNSUnload` load a font into a RIAmosFuncLib bank (55 +
slot, or the highest free one below BankLimit).
`FNSPrint` returns in a0 the address of the end position (x, y words).

A font: `FNS.`, +4 height, +6 width in pixels (16, 32, 48, 64; anything
else is drawn as 16), +8 underline offset, +10 bytes per character; the
width table is indexed by the character code from +0, the glyphs from
+256, row by row, from code 32 to the font's last code. The format has no
count: FNSLoad takes it from the file size, `InstallFNS(fontnum,address,
characters)` (a form of the port, #657) from its third argument, the
form without it takes 91 glyphs (codes 32 to 122), the number of every
known FNS font (the 21 fonts of the CD and the 2.42 distribution, the
built-in one too).

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
same places (bitstack for a depth above 8), and FNSPrint's a0 and FNSSlot point into it as in the original;
`_Static_assert`s check the offsets the original has. Everything is C
behind thunks except: the FNSPrint/FNSShadowPrint entries (assembler:
they test for a bitmap, load the default prefs/colour for the short
forms and call the C code), FNSVersion (an inline routine, `BLITZ_INLINE`)
and the width count of FNSPrint and FNSLength (`countwidth`, inline
assembler: gcc kept its arguments on the stack, 1.42 times the original's
time; now 0.94). FNSLoad
calls RIAmosFuncLib's BLoad (form 1), Start and Erase with
`BLITZ_ALIBJSR`/`BL_JSR_D4_` (tokens `#riamosfunclib<<7` + 15/17/19, as
ripacklib). The register-level details are kept: word and byte widths,
`ASR.l` of the long in the centring, `MULU.w`/`DIVU.w` (`bl_mulu_w`,
`bl_divu_w`), signed/unsigned compares, DBRA on bytes and words, the
glyph `ROR.l`/`SWAP` as `data << (16 - x & 15)`.

## Fixed

Intended deviations, `docs/libs/deviations.md` (3a), kind F:

* FNSPrint/FNSShadowPrint before FNSOutput (no bitmap) store the prefs
  and the colour and return, a0 unchanged, as without a font. The
  original pushed 13 registers first and returned with RTS – to the
  pushed d0, a crash (#665).
* InstallFNS refuses a font of height 0 (result 0), FNSLoad a file that
  is no `FNS.` font or has a height of 0 (-1; the bank is erased, and the
  slot emptied if its font was the one FNSLoad had put in that bank).
  The original checked only the id in InstallFNS and nothing in FNSLoad
  (the manual: "A failure could either be a load error or an
  installation error"), and a height of 0 drew 65536 rows per character:
  every row of the clip zone, the last ones below ymax (#663).
* The glyph rows above ymin are passed with all their words, once per
  line (in print, before the characters). The 48 and 64 pixel loops
  read the 3rd (and 4th) word of a row only for a row they draw, so in
  the original the glyph was drawn shifted after a row above ymin, mixed
  with the next rows' words (#656).
* The underline is clipped at xmin. The original clipped it at x = 0
  only: with FNSClip's xmin above 0 the
  underline of a text that starts left of xmin stuck out of the clip
  zone (#667).
* An empty line (an empty text, or a line that starts with `chr$(10)`)
  is neither centred, aligned nor underlined; the print goes on with the
  next line. The binary has no guard for a width of 0 (the source's
  `TST.w d5:BEQ exit` ended the whole print there): the line counted
  width 1, underlined it set 1 pixel, centred x moved by −32767 (#659).
* FNSLength takes the font number & 15, as every other command. The
  original shifted the whole word (`ASL.w #2`): 16 on read the built-in
  font's bytes as font pointers, negative numbers the variables before
  `font_add` (#660).
* FNSLength without prefs takes FNSPrefs' prefs, the ones FNSPrint
  without prefs uses (the manual: "if you specify no preferences then
  this function will use the previously selected preferences … See:
  FNSPrefs"). The original took the prefs of the last FNSPrint (of them
  only the bold bit counts) (#661).
* FNSLength returns the width FNSPrint uses for the text, the sum of the
  widths (+ 1 per character for bold): "the x size, in pixels, of the
  string if it were to be printed" (the manual). The original subtracted
  2 (0 at least) (#662).
* With runtime error checking FNSClip refuses an xmin or ymin below 0:
  "FNSClip zone must not start below 0" (`clipchk`; the original has no
  error check area). The release code is unchanged: rows are clipped
  signed at ymin and unsigned at ymax, and the byte offset of x is
  `ASR.w` of the long x & $FFF0, so with a negative xmin or ymin a text
  at negative x or y is written about 64K bytes or rows away (an invalid
  argument, D-005) (#664).
* With runtime error checking FNSSetTab refuses a tab size below 1:
  "FNSSetTab size must be at least 1" (`tabchk`). The release code is
  unchanged: after FNSSetTab 0 a tab divides by 0 (#666).
* FNSLoad uses RIAmosFuncLib bank 55 + slot only when it exists (below
  `BankLimit`, 50 at the start): then it behaves as the original.
  Otherwise it takes the bank the slot's font is in (loaded there by an
  FNSLoad before, still in the slot), or the highest free bank below the
  limit (49 down, away from the program's low bank numbers), and
  remembers it per slot; FNSUnload erases that bank (55 + slot for a
  slot FNSLoad never loaded, as the original). `EraseAll` still deletes
  the font, as the manual says. The original always used 55 + slot: with
  the default 50 banks `BLoad` took the number as an address and read
  the file to the address 55 + slot (a real font destroys the vector
  table), `Start` returned 0 and FNSLoad -1. ripacklib's LoadIFF had the
  same mismatch (#770, fixed). Test `rifnslib-fnsload-bank-55` (a font
  header of 12 bytes: the original writes only the addresses 55 to 66)
  (#984).
* A code from 32 on that the font has no glyph for is printed and counted
  as a space (the width and the glyph of code 32), in FNSPrint,
  FNSShadowPrint and FNSLength; codes below 32 keep their meaning. The
  font's glyphs: above (FNSLoad: (file size − 256) / bytes per character;
  InstallFNS's new form: its character count, 1 to 224; the old form and
  RemoveFNS/FNSUnload: 91). The original took a code from 160 on for the
  end of the text (`CMPI.b #32` and a sign test) and drew the codes from
  123 to 159 from the bytes behind the font (in the built-in font: the
  library's code) (#657; user decision W6 Q2 of 2026-11-19: characters
  the font does not have as a space). One compare per character replaces
  the sign test; FNSPrint of 32 characters 7092 cycles (6292 without the
  compare, perf `bfns.fnsprint`), FNSLength 5232 (4989) in C; with the
  width count in assembler (below) 6455 and 3466 (the original: 60445
  and 3686).

* init sets every variable in front of the built-in font to its start
  value (no output bitmap, the slots 1-15 empty, FNSPrefs, FNSInk and the
  clip zone 0, the tab size 32), the glyph counts of the slots and
  FNSLoad's banks. The original set slot 0 and the origin only: a second
  run of a program from the editor (the library stays loaded) printed
  with the first run's settings, and FNSPrint before FNSOutput drew into
  the first run's freed bitmap (#1124).

* The clip zone's ymax is the first row outside it, as xmax is the
  first column: rows are drawn while y < ymax, and FNSClipOutput sets
  ymax to the bitmap's rows. The manual's example for a 320x256 bitmap,
  `FNSClip 0,0,320,256`, is the whole bitmap; the original drew rows up
  to ymax itself (and FNSClipOutput set the rows − 1), so with the
  manual's example a text at the bottom drew a row behind the bitmap
  (user decision W6 "#1128 FNSClip", option (a)) (#1128).

## BUG(orig), kept

* A bitmap depth of 0 or above 8 overruns `bitstack` (256 / depth plane
  pointers over the variables and the font); the 16-pixel loop draws
  `(Flags<<8 | Depth)` planes, the 32/48/64-pixel loops `Depth` (256 for
  0), planes from 8 on with the colour bit `plane & 7` (#658).

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

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rifnslib.txt` recorded from the original.
Bitmaps in memory (40 × 64 bytes, depth 1..8, planes in one buffer with
guards), fonts made in the test (16, 32, 48, 64 pixels and a
24-pixel one drawn as 16; random glyphs, widths 0..width), the built-in
font, RIAmosFuncLib replaced by stubs (BLoad/Start/Erase, calls
recorded). Observed: the whole bitmap buffer after every
print, the end position at a0 (or a0 unchanged without a font), d0, the
kept registers, the stub calls. Covered: see the header of the test (all
entries and forms, all prefs, colours and `chr$(1)`, tabs and FNSSetTab,
new lines including leading/double ones, zero-width characters, codes
from 160 and behind the font's glyphs, every x bit position, xmax, xmin, rows clipped at ymin/ymax for
every font width, empty texts, origin, FNSLength's font numbers 16,
17, −1, −2, InstallFNS of a non-font and with a character count (the
C build only), FNSLoad ok/BLoad fails/Start 0/no font/height 0/40
glyphs/BankLimit 80, FNSUnload, a second run). Not covered: FNSPrint without a bitmap (the original
crashes; bug-orig test `rifnslib-print-no-bitmap-crash`), depth 0 / above 8 /
flags, FNSSetTab 0, negative xmin (writes 64 KB away, #664), font 0 with the
codes behind `z` (the original reads its own code there).
