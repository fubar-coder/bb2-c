---
title: palettelib
weight: 44
description: The bugs of Blitz Basic 2 in the command library palettelib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Screens and palettes

### Decoded palettes were black on Kickstart before 3.0 {#palette-kickstart-13}

Before Kickstart 3.0 the palette library sets colours with 4 bits per
component, and it took these from the wrong byte of the 32-bit colour
values that `DecodePalette` and the fades produce. Such colours came out
black on an Amiga with Kickstart 1.3 or 2.0.

**What happened**

On Kickstart 1.3:

```
DEFTYPE .l
Screen 0,0,0,320,200,3,0,"pal",1,2
Dim mm.l(20) : a=&mm(0)
Poke.l a,$464F524D : Poke.l a+4,24 : Poke.l a+8,$494C424D
Poke.l a+12,$434D4150 : Poke.l a+16,12
For i=0 To 11 : Poke.b a+20+i,i*21+5 : Next
DecodePalette 0,a : ShowPalette 0
DefaultOutput
For i=1 To 3 : NPrint Red(i)," ",Green(i)," ",Blue(i) : Next
CloseScreen 0
End
```

The program builds a small IFF colour map in memory, decodes it into a
palette, shows it, and prints the red, green and blue of colours 1 to 3.
The original prints

```
0 0 0
0 0 0
0 0 0
```

**With the port**

```
4 5 6
8 9 10
12 13 14
```

Issue: [Faded and decoded palettes were wrong or black on Kickstart before 3.0 (#224)](https://github.com/fubar-coder/bb2-c/issues/224).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Palettes

These are the palette objects, their colour cycles and the commands that
read colours from IFF pictures.

* `DecodePalette` and `LoadPalette` cycled a colour range that the
  picture marks as disabled, and ran ranges backwards without their
  reverse flag; a range cycles only when enabled, backwards only when
  marked so
  ([#1002](https://github.com/fubar-coder/bb2-c/issues/1002)).
* `DecodePalette` did not skip the pad byte behind a chunk of odd length,
  such as a name or a short text in the picture, so it missed the colours
  or the colour cycles behind it; it finds them
  ([#225](https://github.com/fubar-coder/bb2-c/issues/225)).
* `DecodePalette` with an offset into a palette too small for the
  colours cleared the palette first, so its colours below the offset were
  lost; the palette grows and keeps them, as with `PalRGB`
  ([#226](https://github.com/fubar-coder/bb2-c/issues/226)).
* The list of colour cycles of a palette object that was never used has
  no end mark. `InitPalette`, and `PalRGB` or `AGAPalRGB` on such a
  palette, left it so, and `CyclePalette` and `Cycle` read on into the
  next palette object; they end the list
  ([#229](https://github.com/fubar-coder/bb2-c/issues/229)).
* `PaletteRange` with the same start and end colour divided by zero; it
  sets that colour to the start colour of the gradient
  ([#227](https://github.com/fubar-coder/bb2-c/issues/227)).
* After `StopCycle`, a new `Cycle` did not start the colour cycling
  again; it does
  ([#228](https://github.com/fubar-coder/bb2-c/issues/228)).

{{% fixed-bugs-library part="elsewhere" %}}
