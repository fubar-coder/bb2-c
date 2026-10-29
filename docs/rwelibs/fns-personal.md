# The FNS font "Personal"

The font built into Reflective Images' FNS library (RIFNSLib, library
93): `IncBin "FNS:Personal.FNS"` in `rwelibsrc/FNSLib/fns-library11.bb`.
The file is `archive/amiblitz3-github/Sourcecodes/Data/fns/fonts/PERSONAL.FNS`
(1712 bytes; origin and license in `archive/README.md`); the format is in
[docs/formats.md](../formats.md) §6. Analysis of 2026-10-29 [B]; the
comparisons were made with throw-away scripts (`.cache/fns/`).

## Metrics

| | |
|---|---|
| Height | 8 rows: cap height 7 (rows 0–6), x-height 5 (rows 2–6), descenders in row 7 (`, ; g j p q y` and `Q`'s tail) |
| Cell | 16 × 8 pixels (width × height; the width is the format's minimum, a multiple of 16); 87 of the 91 glyphs are at most 8 pixels wide, the wider ones are `%` (11), `m` (10) and the TM on `^` (9) |
| Underline | row 7 (the descender row); `_` is a line in row 7 |
| Characters | 91 bitmaps: codes 32 (space) to 122 (`z`) – digits, both cases, the ASCII punctuation; no `{ | } ~`, no codes above 122 (the advance table says 8 for them) |
| Advance | proportional, 3–12 pixels: the ink width plus a gap of 1 pixel (2 for some, e.g. `i`, `!`: 3 for a 2-pixel glyph); space 6 |

## Style

A bold, sans-serif bitmap font: vertical strokes 2 pixels wide,
horizontals 1 pixel, round letters with cut corners (`O`, `C`, `G` …),
a slashed `0` (a short diagonal inside the counter), closed `4`,
round-topped `3` (the forms of all characters: "Character forms"
below). It is in the style of the IBM PC's 8×8 BIOS font, made proportional:
compared with the 8×8 PC console fonts (kbd `lat1-08`, descendants of the
IBM VGA ROM fonts) the glyphs match on average to 80 % (pixel overlap
of the left 8 columns of the 16-pixel cell – the whole glyph except for
`%`, `m` and `^`, which are cut there –, best horizontal and vertical
shift), 24 of 90 are identical
(`$ ' ( ) * + , - . / : ; < > ] _ `` ` `` c e k n o s x`), many more differ
in one row only (e.g. `a`: the last row). Drawn differently (31–61 %
overlap, against 75–100 % for most others): `[` and `^` carry other
symbols (an arrow, a "TM"); `=` has its bars in rows 2 and 4 (IBM 2 and
5); `M` is 8 wide with a V from the corners (IBM: 7 wide, a closed top);
`y` is built of diagonals like a `v` with a descender (IBM: a `u` with a
hook); `X` has flatter diagonals meeting in 2 pixels; `V` narrows to a
single pixel (IBM: straight stems for 5 rows); `Q` has the wider body of
this font's `O` and its tail in the descender row. That the designers
started from the IBM font is a conclusion from the pixels [Schluss].

Two keys have other symbols: `[` (code 91) is a right-pointing arrow,
`^` (code 94) a small "TM". The library's binary (BSS2 `RIFNSLIB.OBJ`,
equal to AmiBlitz3's `Rifnslib.obj`) has the font with its 7 words at
offset $4B0–$4BC changed: exactly the arrow of code 91, moved one pixel
to the left (rows `…#…` → `..#…`, 7 instead of 8 pixels to its tip);
the C port applies that change (`libs/userlibs/rwe/rifnslib.c`).

## Similar free fonts

Measured the same way (`.cache/fns/similar.py`): `font8x8` by Daniel
Hepper (public domain, after the IBM VGA fonts;
<https://github.com/dhepper/font8x8>, `font8x8_basic.h`) – 0.80 on
average, 23 of 90 identical. A free substitute of the same look would
therefore be font8x8 (public domain, compatible with the GPL-3.0) made
proportional with the advance widths of the table below and the two
symbol glyphs redrawn; the IBM-derived kbd console fonts have no clear
license. Not needed so far: the original font is in the repository
(AmiBlitz3 publishes it).

## Characters

"Advance": byte of the advance table (pixels to the next character);
"Ink width": columns with set pixels (from the left edge of the cell);
"vs. IBM 8×8": pixel overlap with the same character of kbd `lat1-08`.

| Code | Key | Glyph shows | Unicode | Advance | Ink width | vs. IBM 8×8 |
|---|---|---|---|---|---|---|
| 32 | space | (space) | U+0020 SPACE | 6 | 0 | – |
| 33 | `!` | `!` | U+0021 EXCLAMATION MARK | 3 | 2 | 86 % |
| 34 | `"` | `"` | U+0022 QUOTATION MARK | 6 | 5 | 75 % |
| 35 | `#` | `#` | U+0023 NUMBER SIGN | 8 | 7 | 94 % |
| 36 | `$` | `$` | U+0024 DOLLAR SIGN | 7 | 6 | identical |
| 37 | `%` | `%` | U+0025 PERCENT SIGN | 12 | 11 | 62 % |
| 38 | `&` | `&` | U+0026 AMPERSAND | 9 | 8 | 71 % |
| 39 | `'` | `'` | U+0027 APOSTROPHE | 4 | 3 | identical |
| 40 | `(` | `(` | U+0028 LEFT PARENTHESIS | 5 | 4 | identical |
| 41 | `)` | `)` | U+0029 RIGHT PARENTHESIS | 5 | 4 | identical |
| 42 | `*` | `*` | U+002A ASTERISK | 9 | 8 | identical |
| 43 | `+` | `+` | U+002B PLUS SIGN | 7 | 6 | identical |
| 44 | `,` | `,` | U+002C COMMA | 4 | 3 | identical |
| 45 | `-` | `-` | U+002D HYPHEN-MINUS | 7 | 6 | identical |
| 46 | `.` | `.` | U+002E FULL STOP | 3 | 2 | identical |
| 47 | `/` | `/` | U+002F SOLIDUS | 8 | 7 | identical |
| 48 | `0` | `0` | U+0030 DIGIT ZERO | 8 | 7 | 94 % |
| 49 | `1` | `1` | U+0031 DIGIT ONE | 5 | 4 | 71 % |
| 50 | `2` | `2` | U+0032 DIGIT TWO | 8 | 7 | 67 % |
| 51 | `3` | `3` | U+0033 DIGIT THREE | 8 | 7 | 78 % |
| 52 | `4` | `4` | U+0034 DIGIT FOUR | 8 | 7 | 93 % |
| 53 | `5` | `5` | U+0035 DIGIT FIVE | 8 | 7 | 77 % |
| 54 | `6` | `6` | U+0036 DIGIT SIX | 8 | 7 | 75 % |
| 55 | `7` | `7` | U+0037 DIGIT SEVEN | 8 | 7 | 77 % |
| 56 | `8` | `8` | U+0038 DIGIT EIGHT | 8 | 7 | 69 % |
| 57 | `9` | `9` | U+0039 DIGIT NINE | 8 | 7 | 75 % |
| 58 | `:` | `:` | U+003A COLON | 3 | 2 | identical |
| 59 | `;` | `;` | U+003B SEMICOLON | 4 | 3 | identical |
| 60 | `<` | `<` | U+003C LESS-THAN SIGN | 6 | 5 | identical |
| 61 | `=` | `=` | U+003D EQUALS SIGN | 7 | 6 | 33 % |
| 62 | `>` | `>` | U+003E GREATER-THAN SIGN | 6 | 5 | identical |
| 63 | `?` | `?` | U+003F QUESTION MARK | 7 | 6 | 89 % |
| 64 | `@` | `@` | U+0040 COMMERCIAL AT | 7 | 6 | 82 % |
| 65 | `A` | `A` | U+0041 LATIN CAPITAL LETTER A | 9 | 8 | 65 % |
| 66 | `B` | `B` | U+0042 LATIN CAPITAL LETTER B | 9 | 8 | 66 % |
| 67 | `C` | `C` | U+0043 LATIN CAPITAL LETTER C | 8 | 7 | 77 % |
| 68 | `D` | `D` | U+0044 LATIN CAPITAL LETTER D | 8 | 7 | 71 % |
| 69 | `E` | `E` | U+0045 LATIN CAPITAL LETTER E | 8 | 7 | 77 % |
| 70 | `F` | `F` | U+0046 LATIN CAPITAL LETTER F | 8 | 7 | 77 % |
| 71 | `G` | `G` | U+0047 LATIN CAPITAL LETTER G | 8 | 7 | 67 % |
| 72 | `H` | `H` | U+0048 LATIN CAPITAL LETTER H | 8 | 7 | 65 % |
| 73 | `I` | `I` | U+0049 LATIN CAPITAL LETTER I | 3 | 2 | 88 % |
| 74 | `J` | `J` | U+004A LATIN CAPITAL LETTER J | 8 | 7 | 64 % |
| 75 | `K` | `K` | U+004B LATIN CAPITAL LETTER K | 8 | 7 | 87 % |
| 76 | `L` | `L` | U+004C LATIN CAPITAL LETTER L | 8 | 7 | 78 % |
| 77 | `M` | `M` | U+004D LATIN CAPITAL LETTER M | 9 | 8 | 58 % |
| 78 | `N` | `N` | U+004E LATIN CAPITAL LETTER N | 9 | 8 | 71 % |
| 79 | `O` | `O` | U+004F LATIN CAPITAL LETTER O | 8 | 7 | 65 % |
| 80 | `P` | `P` | U+0050 LATIN CAPITAL LETTER P | 8 | 7 | 76 % |
| 81 | `Q` | `Q` | U+0051 LATIN CAPITAL LETTER Q | 8 | 7 | 55 % |
| 82 | `R` | `R` | U+0052 LATIN CAPITAL LETTER R | 8 | 7 | 68 % |
| 83 | `S` | `S` | U+0053 LATIN CAPITAL LETTER S | 8 | 7 | 62 % |
| 84 | `T` | `T` | U+0054 LATIN CAPITAL LETTER T | 9 | 8 | 75 % |
| 85 | `U` | `U` | U+0055 LATIN CAPITAL LETTER U | 8 | 7 | 63 % |
| 86 | `V` | `V` | U+0056 LATIN CAPITAL LETTER V | 8 | 7 | 61 % |
| 87 | `W` | `W` | U+0057 LATIN CAPITAL LETTER W | 9 | 8 | 67 % |
| 88 | `X` | `X` | U+0058 LATIN CAPITAL LETTER X | 9 | 8 | 61 % |
| 89 | `Y` | `Y` | U+0059 LATIN CAPITAL LETTER Y | 9 | 8 | 64 % |
| 90 | `Z` | `Z` | U+005A LATIN CAPITAL LETTER Z | 7 | 6 | 88 % |
| 91 | `[` | → | U+2192 RIGHTWARDS ARROW | 8 | 8 | 31 % |
| 92 | `\` | `\` | U+005C REVERSE SOLIDUS | 9 | 8 | 86 % |
| 93 | `]` | `]` | U+005D RIGHT SQUARE BRACKET | 5 | 4 | identical |
| 94 | `^` | ™ | U+2122 TRADE MARK SIGN | 10 | 9 | 40 % |
| 95 | `_` | `_` | U+005F LOW LINE | 8 | 7 | identical |
| 96 | `` ` `` | `` ` `` | U+0060 GRAVE ACCENT | 4 | 3 | identical |
| 97 | `a` | `a` | U+0061 LATIN SMALL LETTER A | 7 | 6 | 90 % |
| 98 | `b` | `b` | U+0062 LATIN SMALL LETTER B | 7 | 6 | 75 % |
| 99 | `c` | `c` | U+0063 LATIN SMALL LETTER C | 7 | 6 | identical |
| 100 | `d` | `d` | U+0064 LATIN SMALL LETTER D | 7 | 6 | 70 % |
| 101 | `e` | `e` | U+0065 LATIN SMALL LETTER E | 7 | 6 | identical |
| 102 | `f` | `f` | U+0066 LATIN SMALL LETTER F | 6 | 5 | 64 % |
| 103 | `g` | `g` | U+0067 LATIN SMALL LETTER G | 7 | 6 | 88 % |
| 104 | `h` | `h` | U+0068 LATIN SMALL LETTER H | 7 | 6 | 92 % |
| 105 | `i` | `i` | U+0069 LATIN SMALL LETTER I | 3 | 2 | 92 % |
| 106 | `j` | `j` | U+006A LATIN SMALL LETTER J | 5 | 4 | 93 % |
| 107 | `k` | `k` | U+006B LATIN SMALL LETTER K | 7 | 6 | identical |
| 108 | `l` | `l` | U+006C LATIN SMALL LETTER L | 4 | 3 | 88 % |
| 109 | `m` | `m` | U+006D LATIN SMALL LETTER M | 11 | 10 | 61 % |
| 110 | `n` | `n` | U+006E LATIN SMALL LETTER N | 7 | 6 | identical |
| 111 | `o` | `o` | U+006F LATIN SMALL LETTER O | 7 | 6 | identical |
| 112 | `p` | `p` | U+0070 LATIN SMALL LETTER P | 7 | 6 | 91 % |
| 113 | `q` | `q` | U+0071 LATIN SMALL LETTER Q | 7 | 6 | 84 % |
| 114 | `r` | `r` | U+0072 LATIN SMALL LETTER R | 6 | 5 | 63 % |
| 115 | `s` | `s` | U+0073 LATIN SMALL LETTER S | 7 | 6 | identical |
| 116 | `t` | `t` | U+0074 LATIN SMALL LETTER T | 6 | 5 | 84 % |
| 117 | `u` | `u` | U+0075 LATIN SMALL LETTER U | 7 | 6 | 86 % |
| 118 | `v` | `v` | U+0076 LATIN SMALL LETTER V | 8 | 7 | 62 % |
| 119 | `w` | `w` | U+0077 LATIN SMALL LETTER W | 9 | 8 | 65 % |
| 120 | `x` | `x` | U+0078 LATIN SMALL LETTER X | 8 | 7 | identical |
| 121 | `y` | `y` | U+0079 LATIN SMALL LETTER Y | 9 | 8 | 40 % |
| 122 | `z` | `z` | U+007A LATIN SMALL LETTER Z | 7 | 6 | 90 % |

## Character forms

The form of every glyph, described with the categories type designers
use for the alternative forms of a font (OpenType character variants
`cvNN` and stylistic sets `ssNN`, as documented e.g. for Source Code
Pro, JetBrains Mono, Iosevka, Fira Code). Read from the bitmaps of
`PERSONAL.FNS` [B] (rendered with a throw-away script); rows 0–7 from
the top, columns 0–15 from the left edge of the cell. Code 91 is
described as in the file; the library's copy has its arrow head one
pixel further left (section "Style"). Statements marked [Schluss] are
judgements, not pixel facts.

### General features

* **Grid.** Baseline under row 6; cap height 7 rows (0–6), x-height
  5 rows (2–6, a large x-height: 5/7), ascenders end at the cap height
  (`b d f h k l t` reach row 0, no ascender above the capitals),
  descender 1 row (row 7). Because the descender is so short, the
  descending letters `g p q` have a bowl of 4 rows (2–5) instead of 5,
  and `j`, `g` end in a flat 1-row tail. Digits are lining (rows 0–6).
  Math axis row 3: `-`, the bar of `+`, the middle of `=`, `*`, `<`,
  `>` are centred on it.
* **Stroke.** Vertical stems 2 pixels wide, horizontal strokes 1 pixel
  (a "double-width vertical" bold, the construction of the CGA/IBM PC
  8×8 fonts); diagonals are 2-pixel runs stepped by one column per row.
  The only glyph with 1-pixel stems is the superscript TM on `^`.
* **Round letters.** A rectangle with each corner pixel cut away: top
  and bottom arcs are 1-pixel rows inset by one column (`.#####.`),
  the sides straight 2-pixel stems (`O`: sides rows 1–5). Where a bowl
  joins a stem the corner stays square (`b`, `n`, `p`, `B`, `P`, `R`,
  shoulders of `h m n`), only the outer corner is cut. `D` has a
  two-step diagonal at both right corners. Terminals of open round
  letters are short vertical pieces (`C`, `G`, `S`, `c`, `2`, `3`, `5`),
  except for the flat terminals of `e` and `s`.
* **Widths.** Capitals mostly 7 pixels of ink (cols 0–6); 8 for
  `A B M N T W X Y`; `Z` 6, `I` 2. Lower case 6 (`a`–`u` except
  `f i j l m r t`), the diagonal letters wider: `v x` 7, `w y` 8, `m` 10.
  Advance widths: section "Characters".
* **Serifs.** None, except the flags of `1` and `l` and the hooks of
  `f`, `j`, `t` – the font distinguishes `I l 1` by them (`I` plain
  bar, `l` with a top-left flag, `1` with a diagonal flag).
* **Genre.** A bold grotesque pixel font in the "PC BIOS" (IBM CGA/VGA
  8×8) style, made proportional; the measured overlap with the IBM
  font is ~80 % (section "Style"). Where it differs from the IBM
  shapes it tends to diagonals and pointed forms (`A`, `M`, `V`, `v`,
  `X`, `y`) [Schluss].
* **Irregularities** [B]: in `"` the right mark is 1 row (row 1), the
  left one 2 rows (0–1); in `#` the right vertical starts in row 1, the
  left one in row 0; `J` starts in row 1 (6 rows instead of 7); the dot
  of `j` is 1 pixel (col 2), that of `i` 2 pixels; `/` has 6 rows
  (1–6), `\` 7 (0–6). That these are slips rather than design is a
  guess [Schluss].

### Variant choices of this font

In the manner of a font's list of character variants:

| Character | Variant |
|---|---|
| `0` | slashed: short diagonal (bottom left to top right) inside the counter, joined to both sides; not dotted, not crossing the outline |
| `1` | flag as a filled diagonal wedge; no base |
| `3` | round top (not flat) |
| `4` | closed (diagonal joins the stem at the top); crossbar passes the stem by 1 pixel |
| `6` `9` | straight diagonal stem/tail (not curved) |
| `7` | plain (no crossbar); diagonal bending into a vertical stem |
| `$` | stroke only at the ends (top and bottom), not through the S |
| `*` | 6 points (× plus horizontal bar), centred on the math axis (not raised) |
| `&` | traditional "et" form |
| `@` | square, IBM style: a solid inner bar instead of an `a` |
| `%` | two rings and a diagonal (rings, not dots or slashed zeros) |
| `'` `` ` `` | slanted (comma-shaped / its mirror), not straight |
| `"` | straight (vertical marks) |
| `A` | pointed: triangular with a 2-pixel flat apex; low crossbar (row 4) |
| `G` | crossbar, no spur |
| `I` | plain bar (no serifs) |
| `J` | without top bar (hook to the left) |
| `K` `k` | arm and leg meet at the stem (one vertex) |
| `M` | straight stems; vertex slightly below the middle (row 4 of 0–6); diagonals from the top outer corners |
| `Q` | tail crossing the bowl at the lower right, descending into row 7 |
| `R` | straight vertical leg |
| `V` `v` | pointed bottom (1 pixel) |
| `W` | straight stems, middle vertex at the middle (row 3), an upside-down `M` |
| `X` `x` | crossing diagonals, meeting in 2 (`X`) / 3 (`x`) pixels |
| `a` | double-storey, no spur |
| `f` | hooked top, crossbar at x-height on both sides, no descender |
| `g` | single-storey, closed bowl, flat tail to the left |
| `i` | plain stem, square dot (2 × 1) |
| `j` | flat tail to the left, 1-pixel dot |
| `l` | top-left flag only (no foot, no tail) |
| `r` | diagonal arm, no serif |
| `t` | tall (to the cap height), cross at x-height on both sides, tail curving right |
| `u` | symmetric, no spur |
| `w` | converging stems with flat 2-pixel feet, middle vertex at the middle (row 4 of 2–6) |
| `y` | v-shaped diagonals, straight tail (continuation of the right diagonal) |

### All characters

"Form": category and value where a usual variant category fits,
otherwise a short description. The match with the IBM 8×8 font:
section "Characters".

| Code | Glyph | Unicode | Form | Notes |
|---|---|---|---|---|
| 32 | (space) | U+0020 SPACE | – | empty bitmap, advance 6 |
| 33 | `!` | U+0021 EXCLAMATION MARK | plain stem with square dot | stem rows 0–4, dot (2 × 1) row 6 |
| 34 | `"` | U+0022 QUOTATION MARK | quotes: straight | left mark rows 0–1, right mark row 1 only (cols 3–4) |
| 35 | `#` | U+0023 NUMBER SIGN | upright (vertical strokes), bars in rows 2 and 4 | bars 7 px; right vertical starts in row 1 |
| 36 | `$` | U+0024 DOLLAR SIGN | `$`: stroke only at the ends | stroke 2 px in rows 0 and 6 above/below the S |
| 37 | `%` | U+0025 PERCENT SIGN | rings and diagonal | two 5 × 3 rings (rows 1–3, 4–6), 11 px wide |
| 38 | `&` | U+0026 AMPERSAND | traditional (et) | top loop rows 0–3, leg ends at the right in rows 4 and 6 |
| 39 | `'` | U+0027 APOSTROPHE | quote: slanted (comma-shaped) | rows 0–1 cols 1–2, row 2 cols 0–1 |
| 40 | `(` | U+0028 LEFT PARENTHESIS | straight middle, diagonal ends | 3 straight rows (2–4), full height 0–6 |
| 41 | `)` | U+0029 RIGHT PARENTHESIS | as `(`, mirrored | |
| 42 | `*` | U+002A ASTERISK | `*`: 6 points, centred | × plus horizontal bar (row 3, 8 px), rows 1–5 |
| 43 | `+` | U+002B PLUS SIGN | plain | 6 × 5, bar row 3 |
| 44 | `,` | U+002C COMMA | square dot with tail to the left | dot rows 5–6, tail row 7 |
| 45 | `-` | U+002D HYPHEN-MINUS | plain | row 3, 6 px |
| 46 | `.` | U+002E FULL STOP | square (2 × 2) | rows 5–6 |
| 47 | `/` | U+002F SOLIDUS | straight diagonal | rows 1–6, 7 px wide |
| 48 | `0` | U+0030 DIGIT ZERO | zero: slashed (short diagonal inside) | diagonal (4,2)–(3,3)–(2,4), joined to both sides; oval as `O` |
| 49 | `1` | U+0031 DIGIT ONE | 1: flag (diagonal wedge), no base | stem cols 2–3, flag rows 1–2 |
| 50 | `2` | U+0032 DIGIT TWO | round top, curved neck, flat base | base row 6, 7 px |
| 51 | `3` | U+0033 DIGIT THREE | 3: round top | short middle bar (row 3, cols 3–5); terminals rows 1 and 5 |
| 52 | `4` | U+0034 DIGIT FOUR | 4: closed | diagonal meets the stem in rows 0–1; crossbar row 4 passes the stem by 1 px |
| 53 | `5` | U+0035 DIGIT FIVE | flat top, vertical stem | square join of stem and bowl (row 2) |
| 54 | `6` | U+0036 DIGIT SIX | 6: straight (diagonal) stem | diagonal rows 0–2, bowl rows 3–6 |
| 55 | `7` | U+0037 DIGIT SEVEN | 7: no crossbar | diagonal rows 1–4, vertical rows 4–6 |
| 56 | `8` | U+0038 DIGIT EIGHT | two equal bowls | waist row 3 as wide as top and bottom |
| 57 | `9` | U+0039 DIGIT NINE | 9: straight (diagonal) tail | bowl rows 0–3, diagonal rows 4–6 |
| 58 | `:` | U+003A COLON | square dots | rows 1–2 and 5–6 |
| 59 | `;` | U+003B SEMICOLON | square dots, comma tail | as `:` shifted 1 col right, tail row 7 |
| 60 | `<` | U+003C LESS-THAN SIGN | straight, full height | rows 0–6, vertex row 3 |
| 61 | `=` | U+003D EQUALS SIGN | plain | bars rows 2 and 4 (6 px) |
| 62 | `>` | U+003E GREATER-THAN SIGN | as `<`, mirrored | |
| 63 | `?` | U+003F QUESTION MARK | round top, diagonal neck | dot row 6 |
| 64 | `@` | U+0040 COMMERCIAL AT | `@`: square, solid inner bar (IBM style) | inner block cols 3–5 rows 2–4; open at the right in row 5 |
| 65 | `A` | U+0041 LATIN CAPITAL LETTER A | A: pointed (flat 2-px apex), low crossbar | apex row 0 cols 3–4, crossbar row 4 |
| 66 | `B` | U+0042 LATIN CAPITAL LETTER B | larger lower bowl | lower bowl 1 px wider (to col 7) |
| 67 | `C` | U+0043 LATIN CAPITAL LETTER C | closed aperture, vertical terminals | terminals rows 1 and 5 |
| 68 | `D` | U+0044 LATIN CAPITAL LETTER D | two-step diagonal corners | straight side rows 2–4 |
| 69 | `E` | U+0045 LATIN CAPITAL LETTER E | shorter middle arm | arms 7/5/7 px, middle row 3 |
| 70 | `F` | U+0046 LATIN CAPITAL LETTER F | shorter middle arm | as `E` without the bottom arm |
| 71 | `G` | U+0047 LATIN CAPITAL LETTER G | G: crossbar, no spur | bar row 3 cols 4–6; top terminal row 1 |
| 72 | `H` | U+0048 LATIN CAPITAL LETTER H | plain | crossbar row 3 |
| 73 | `I` | U+0049 LATIN CAPITAL LETTER I | I: plain bar (no serifs) | 2 px wide |
| 74 | `J` | U+004A LATIN CAPITAL LETTER J | J: without top bar | rows 1–6 only; hook to the left with terminal rows 4–5 |
| 75 | `K` | U+004B LATIN CAPITAL LETTER K | K: arm and leg meet at the stem | junction row 3 |
| 76 | `L` | U+004C LATIN CAPITAL LETTER L | plain | |
| 77 | `M` | U+004D LATIN CAPITAL LETTER M | M: straight stems, vertex slightly below the middle | vertex row 4; 1-px tips in row 0 (diagonals start at the outer corners) |
| 78 | `N` | U+004E LATIN CAPITAL LETTER N | plain | diagonal rows 1–5 |
| 79 | `O` | U+004F LATIN CAPITAL LETTER O | cut-corner rectangle | sides rows 1–5 |
| 80 | `P` | U+0050 LATIN CAPITAL LETTER P | bowl closed at the stem | bowl rows 0–3 |
| 81 | `Q` | U+0051 LATIN CAPITAL LETTER Q | Q: crossing tail at the lower right, descending | tail pixels row 5 cols 3–4 inside, row 7 cols 5–6 below |
| 82 | `R` | U+0052 LATIN CAPITAL LETTER R | R: straight vertical leg | `P` plus the right stem in rows 4–6 |
| 83 | `S` | U+0053 LATIN CAPITAL LETTER S | vertical terminals | terminals rows 1 (right) and 5 (left), spine row 3 |
| 84 | `T` | U+0054 LATIN CAPITAL LETTER T | plain | bar 8 px, stem cols 3–4 |
| 85 | `U` | U+0055 LATIN CAPITAL LETTER U | symmetric, no spur | |
| 86 | `V` | U+0056 LATIN CAPITAL LETTER V | V: pointed (1 px) | straight rows 0–1, point row 6 col 3 |
| 87 | `W` | U+0057 LATIN CAPITAL LETTER W | W: straight stems, middle vertex at the middle | vertex row 3; an upside-down `M` (without the tips) |
| 88 | `X` | U+0058 LATIN CAPITAL LETTER X | X: straight diagonals, crossing in 2 px | crossing row 3 |
| 89 | `Y` | U+0059 LATIN CAPITAL LETTER Y | straight arms, short stem | junction rows 3–4, stem rows 4–6 |
| 90 | `Z` | U+005A LATIN CAPITAL LETTER Z | Z: no crossbar | 6 px wide |
| 91 | → | U+2192 RIGHTWARDS ARROW | arrow with a solid triangular head (on `[`) | shaft row 3 cols 0–7, head cols 4–7 rows 0–6; in the library's copy 1 px further left |
| 92 | `\` | U+005C REVERSE SOLIDUS | straight diagonal | rows 0–6, 8 px wide |
| 93 | `]` | U+005D RIGHT SQUARE BRACKET | plain | 4 px wide, stem cols 2–3; no `[` (code 91 is the arrow) |
| 94 | ™ | U+2122 TRADE MARK SIGN | superscript "TM" (on `^`) | rows 0–3, 1-px strokes; the M's vertex in row 2 |
| 95 | `_` | U+005F LOW LINE | plain | row 7 (the underline row), 7 px |
| 96 | `` ` `` | U+0060 GRAVE ACCENT | backtick: slanted (mirror of `'`) | rows 0–1 cols 0–1, row 2 cols 1–2 |
| 97 | `a` | U+0061 LATIN SMALL LETTER A | a: double-storey, no spur | bowl rows 4–6, top arc row 2 |
| 98 | `b` | U+0062 LATIN SMALL LETTER B | square at the stem, no spur | bowl rows 2–6 |
| 99 | `c` | U+0063 LATIN SMALL LETTER C | closed aperture, vertical terminals | terminals rows 3 and 5 |
| 100 | `d` | U+0064 LATIN SMALL LETTER D | no spur | mirror of `b` with a round left side |
| 101 | `e` | U+0065 LATIN SMALL LETTER E | horizontal bar, flat terminal | bar row 4 full width; open at the right in row 5 |
| 102 | `f` | U+0066 LATIN SMALL LETTER F | f: hooked top, crossbar at x-height, no descender | crossbar row 2 on both sides of the stem |
| 103 | `g` | U+0067 LATIN SMALL LETTER G | g: single-storey, closed bowl | bowl rows 2–5, flat tail to the left in row 7 |
| 104 | `h` | U+0068 LATIN SMALL LETTER H | square shoulder | as `n` with an ascender |
| 105 | `i` | U+0069 LATIN SMALL LETTER I | i: plain (no serifs), square dot | dot 2 × 1 in row 0, stem rows 2–6 |
| 106 | `j` | U+006A LATIN SMALL LETTER J | j: flat tail to the left | dot 1 px (col 2); tail row 7 cols 0–2 |
| 107 | `k` | U+006B LATIN SMALL LETTER K | k: arm and leg meet at the stem | junction row 4 |
| 108 | `l` | U+006C LATIN SMALL LETTER L | l: top-left flag, no tail | flag row 0 col 0, stem cols 1–2 |
| 109 | `m` | U+006D LATIN SMALL LETTER M | square shoulders | 10 px; the second arch separate from the first in row 2 |
| 110 | `n` | U+006E LATIN SMALL LETTER N | square shoulder | top row 2 flush with the stem |
| 111 | `o` | U+006F LATIN SMALL LETTER O | cut-corner rectangle | |
| 112 | `p` | U+0070 LATIN SMALL LETTER P | square at the stem | bowl rows 2–5, stem to row 7 |
| 113 | `q` | U+0071 LATIN SMALL LETTER Q | plain stem, no spur | bowl rows 2–5, stem to row 7 |
| 114 | `r` | U+0072 LATIN SMALL LETTER R | r: diagonal arm, no serif | arm leaves the stem in row 3, ends in row 2 cols 3–4 |
| 115 | `s` | U+0073 LATIN SMALL LETTER S | flat terminals | terminals are the rows 2 and 6 themselves |
| 116 | `t` | U+0074 LATIN SMALL LETTER T | t: tall, cross at x-height, tail curving right | ascender to row 0; cross row 2 on both sides |
| 117 | `u` | U+0075 LATIN SMALL LETTER U | u: symmetric, no spur | |
| 118 | `v` | U+0076 LATIN SMALL LETTER V | v: pointed (1 px) | straight rows 2–3, point row 6 |
| 119 | `w` | U+0077 LATIN SMALL LETTER W | w: converging stems, flat feet, middle vertex at the middle | vertex row 4; feet 2 px each in row 6 |
| 120 | `x` | U+0078 LATIN SMALL LETTER X | x: straight diagonals, crossing in 3 px | crossing row 4 |
| 121 | `y` | U+0079 LATIN SMALL LETTER Y | y: v-shaped, straight tail | arms meet in row 4, tail continues the right diagonal to row 7 |
| 122 | `z` | U+007A LATIN SMALL LETTER Z | z: no crossbar | |
