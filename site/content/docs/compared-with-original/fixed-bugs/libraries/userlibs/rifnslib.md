---
title: rifnslib
weight: 23
description: The bugs of Blitz Basic 2 in the RWE userlib rifnslib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes you may notice

### `FNSLength` measured a text 2 pixels short {#fnslength-width}

`FNSLength(fontnum,a$[,prefs])` of
[rifnslib]({{< relref "/docs/libraries/userlibs/rifnslib" >}}) returns
the width in pixels that `a$` has when `FNSPrint` prints it in the font
of slot `fontnum`, for example to centre a text or to print another one
behind it. The style `prefs` counts because bold adds one pixel per
character; without it, the style set with `FNSPrefs` counts, as for
`FNSPrint` without one.

**What happened.** The original subtracted 2 from the width, and gave
0 for a text narrower than that. Without `prefs` it took the style of
the last `FNSPrint` instead of the one of `FNSPrefs`, so after a bold
print it counted one pixel more per character. In the font built into
the library, the five characters of `Hello` are 30 pixels wide:

```
NPrint FNSLength(0,"Hello",0)
```

With the original this printed `28`; with the port it prints `30`.

**What to change.** A program that added 2 to the result takes the
result as it is. One that relied on the style of the last print gives
`FNSLength` the style itself.

Issues:

* [FNSLength returns the text width minus 2 (#662)](https://github.com/fubar-coder/bb2-c/issues/662)
* [FNSLength without prefs uses the prefs of the last FNSPrint instead of those of FNSPrefs (#661)](https://github.com/fubar-coder/bb2-c/issues/661)

### Characters an FNS font has no glyph for {#fns-missing-glyphs}

A font of
[rifnslib]({{< relref "/docs/libraries/userlibs/rifnslib" >}}) holds a
glyph for each character from the space, code 32, up to its last one.
The font built into the library and every known FNS font end at `z`,
code 122, so they have no accented letters, no braces and no tilde.

**What happened.** The original took every character from code 160 on,
such as the accented letters, for the end of the text: `FNSPrint`
printed nothing from there on, and `FNSLength` stopped counting. The
characters from code 123 to 159 it drew from the memory behind the
font, for the built-in font from the library's own code. So
`FNSPrint 0,10,10,"Grüße"` printed only `Gr`.

**With the port.** `FNSPrint`, `FNSShadowPrint` and `FNSLength` take a
character the font has no glyph for as a space, with the width of the
space; the control characters below 32 keep their meaning. How many
glyphs a font has, `FNSLoad` takes from the size of the file. A font in
memory does not say it, so `InstallFNS(fontnum,address)` takes the 91
glyphs of the known fonts, up to `z`, and the form
`InstallFNS(fontnum,address,characters)` takes the number given.

**What to change.** A text with such characters goes on behind them.
A font of your own with glyphs behind `z` that a program puts into a
slot with `InstallFNS` needs the form with the number of its
characters; loaded with `FNSLoad`, it works as it is.

Issue: [Character codes from 160 on end the text (sign test after CMPI.b #32) (#657)](https://github.com/fubar-coder/bb2-c/issues/657).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rifnslib {#rifnslib-fixes}

These fixes concern
[rifnslib]({{< relref "/docs/libraries/userlibs/rifnslib" >}}), the
library by Reflective Images that prints text in its own FNS fonts into
a bitmap with the processor. The fixes of
[the width `FNSLength` returns](#fnslength-width) and of
[characters a font has no glyph for](#fns-missing-glyphs) are described
above.

* `FNSPrint` and `FNSShadowPrint` before `FNSOutput` has chosen a
  bitmap print nothing. The original crashed the machine
  ([#665](https://github.com/fubar-coder/bb2-c/issues/665)).
* `InstallFNS` returns 0 for a font of height 0, and `FNSLoad` returns
  -1 for such a font and for a file that is no FNS font. The original
  took them, and a font of height 0 drew 65536 rows for every
  character, over the whole clip zone and below it
  ([#663](https://github.com/fubar-coder/bb2-c/issues/663)).
* `FNSLoad` loads the font into bank 55 plus the slot number of
  [riamosfunclib]({{< relref "/docs/libraries/userlibs/riamosfunclib" >}})
  only when the program has raised the number of banks with `BankLimit`
  so that this bank exists. Otherwise it takes the highest free bank
  below the limit, and `FNSUnload` frees the bank the font is in. The
  original always took bank 55 plus the slot: with the 50 banks
  riamosfunclib starts with, it wrote the file into memory at the
  address 55 plus the slot, over the system's vectors, and returned -1
  ([#984](https://github.com/fubar-coder/bb2-c/issues/984)).
* Characters of 48 and 64 pixel fonts whose top rows lie above the clip
  zone are drawn as they should be. The original drew the rest of such
  a character garbled
  ([#656](https://github.com/fubar-coder/bb2-c/issues/656)).
* The bottom edge of the clip zone works like its right edge: `ymax` of
  `FNSClip` is the first row not drawn in, as `xmax` is the first
  column, and `FNSClipOutput` sets the zone to match. The guide's
  example for a bitmap of 320 x 256 pixels, `FNSClip 0,0,320,256`, is
  the whole bitmap. The original drew rows up to `ymax` itself, so with
  that example a text at the bottom wrote a row behind the bitmap. A
  program that gave the last row of the zone as `ymax` gives one more
  ([#1128](https://github.com/fubar-coder/bb2-c/issues/1128)).
* The underline is clipped at the left edge of the clip zone, like the
  characters. The original clipped it at the left edge of the bitmap, so
  it stuck out of a zone that starts further right
  ([#667](https://github.com/fubar-coder/bb2-c/issues/667)).
* An underline that lies wholly outside the clip zone draws nothing.
  The original set one pixel at the edge of the zone
  ([#1130](https://github.com/fubar-coder/bb2-c/issues/1130)).
* An empty text, or an empty line in a text, is neither underlined,
  centred nor aligned. The original set one pixel for it when the style
  was underlined, and centred it moved the position far to the left
  ([#659](https://github.com/fubar-coder/bb2-c/issues/659)).
* `FNSLength` counts only the low 4 bits of the font number, so 16 is
  slot 0 again, as with every other command of the library. The
  original read a font number from 16 on, or a negative one, from
  memory behind or before the slots and returned a meaningless width
  ([#660](https://github.com/fubar-coder/bb2-c/issues/660)).
* With runtime error checking, `FNSClip` with an `xmin` or `ymin` below
  0 stops with "FNSClip zone must not start below 0", and
  `FNSSetTab` with a size below 1 with "FNSSetTab size must be at least
  1" ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#fns-clip-tab-checks" >}}),
  [#664](https://github.com/fubar-coder/bb2-c/issues/664),
  [#666](https://github.com/fubar-coder/bb2-c/issues/666)).
* A program run a second time from the editor starts with no bitmap
  chosen, the slots 1 to 15 empty, and the style, colour, tab size and
  clip zone at their start values. The original kept those of the run
  before, and `FNSPrint` before `FNSOutput` drew into that run's bitmap,
  which had been freed
  ([#1124](https://github.com/fubar-coder/bb2-c/issues/1124)).

{{% fixed-bugs-library part="elsewhere" %}}
