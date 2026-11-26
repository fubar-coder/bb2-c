---
title: intuifontlib
weight: 34
description: The bugs of Blitz Basic 2 in the command library intuifontlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/intuifontlib/]
---

{{< fixed-bugs-library >}}

## Text

### `LoadFont` lost the style {#loadfont-style}

`LoadFont` takes a style as its last argument: 1 underlined, 2 bold, 4
italic, 8 extended, or a sum of them. The original wrote this value
one byte too far into the font object, the one that describes the font
to the system. The style itself stayed 0, and the value went into the
flags of the font, which tell the system where to look for it. A font
loaded with a style came out without it.

**What happened**

```
DEFTYPE .l
LoadFont 1,"topaz.font",9,1
a=Addr IntuiFont(1)
NPrint "style ",Peek.b(a+6)," flags ",Peek.b(a+7)
End
```

The program loads topaz 9 underlined and reads the style and flags bytes
of the font object. The original prints

```
style 0 flags 1
```

**With the port**

The style is set and the flags are 0:

```
style 1 flags 0
```

A style above 255 counts only with its low byte.

Issue: [LoadFont wrote the style into the font flags (#197)](https://github.com/fubar-coder/bb2-c/issues/197).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Fonts

* Every `LoadFont` on the same IntuiFont object lost the memory of the
  copy of the previous font name; the old copy is freed. See also
  [`LoadFont` lost the style](#loadfont-style)
  ([#198](https://github.com/fubar-coder/bb2-c/issues/198)).

{{% fixed-bugs-library part="elsewhere" %}}
