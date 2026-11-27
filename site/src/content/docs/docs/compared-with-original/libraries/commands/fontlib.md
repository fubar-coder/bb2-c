---
title: fontlib
description: The bugs of Blitz Basic 2 in the command library fontlib that the
  BB2 C port fixes.
sidebar:
  order: 24
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/fontlib/
---

::fixed-bugs-library

## Text

### Umlauts were not drawn {#umlauts}

The text commands draw a character only when it lies in the range of the
font. The original compared the character with the font's first
character as a signed byte, so every character 128 or more above it was
taken as lying below the font. With the standard font, which starts at
the space, that is every character from code 160 on: the umlauts and
the other accented letters were not drawn, and the cursor did not move.

**What happened**

```
BitMap 0,320,16,1
BitMapOutput 0
Print "A"
a.l=CursX
Print Chr$(196)
b.l=CursX
DefaultOutput
NPrint "after A: ",a
NPrint "after umlaut: ",b
End
```

The program prints "A" and the umlaut Ä (code 196) on a bitmap, and after
each the column of the cursor. The original prints

```
after A: 1
after umlaut: 1
```

The Ä is not drawn, and the cursor stays where it was.

**With the port**

```
after A: 1
after umlaut: 2
```

Issue: [Characters far above the font's first character were not drawn (#148)](https://github.com/fubar-coder/bb2-c/issues/148).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Fonts

* `LoadBlitzFont` of a font that is not 8 by 8 pixels stopped with the
  error "Can't Open Font" and left the font open after the program; it
  closes the font before the error
  ([#150](https://github.com/fubar-coder/bb2-c/issues/150)).

::fixed-bugs-library{part="elsewhere"}
