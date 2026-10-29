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
| Cell | 16 pixels wide (the format's minimum); the widest glyph is 11 pixels (`m`, `%`) |
| Underline | row 7 (the descender row); `_` is a line in row 7 |
| Characters | 91 bitmaps: codes 32 (space) to 122 (`z`) – digits, both cases, the ASCII punctuation; no `{ | } ~`, no codes above 122 (the advance table says 8 for them) |
| Advance | proportional, 3–12 pixels: the ink width plus a gap of 1 pixel (2 for some, e.g. `i`, `!`: 3 for a 2-pixel glyph); space 6 |

## Style

A bold, sans-serif bitmap font: vertical strokes 2 pixels wide,
horizontals 1 pixel, round letters with cut corners (`O`, `C`, `G` …),
a dotted/slashed `0` (a diagonal pixel inside), open `4`, flat-topped
`3`. It is in the style of the IBM PC's 8×8 BIOS font, made proportional:
compared with the 8×8 PC console fonts (kbd `lat1-08`, descendants of the
IBM VGA ROM fonts) the glyphs match on average to 80 % (pixel overlap,
best horizontal and vertical shift), 24 of 90 are identical
(`$ ' ( ) * + , - . / : ; < > ] _ `` ` `` c e k n o s x`), many more differ
in one row only (e.g. `a`: the last row). Own drawings are mainly `[`
(an arrow), `^` (a trade mark sign), `=`, `M`, `y`, `Q`, `X` and `V`.

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
