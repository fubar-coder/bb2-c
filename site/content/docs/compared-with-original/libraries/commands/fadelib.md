---
title: fadelib
weight: 21
description: The bugs of Blitz Basic 2 in the command library fadelib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/fadelib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Blitz mode displays

These are the slices and their copper list, the copper lists of the
display library and the fades. Two more fixes of them change what an
existing program shows:
[`CustomSprites` with an offset]({{< relref "/docs/compared-with-original/libraries/commands/displaylib#customsprites-ccoffset" >}}) and
[`CustomColors` with a `thru256flag` of 0]({{< relref "/docs/compared-with-original/libraries/commands/displaylib#customcolors-thru256flag" >}}).

* `FadeIn` and `FadeOut` of palette 2 or a higher one made the red or the
  green of the first colour run ahead of the others in the steps of the
  fade; all colours fade evenly
  ([#134](https://github.com/fubar-coder/bb2-c/issues/134)).

#### The checks of `FadeIn` and `FadeOut` {#fade-checks}

**What happened.** With runtime error checking, `FadeIn` and `FadeOut`
check the used slice, the palette and the range of colours. The original
took a palette whose colours lie in the lowest 64 KB of memory as one
that was never made, and stopped the program with an error. Two errors
had the wrong message: a palette that was never made gave "Not enough
colours in Slice", and a colour of the range beyond the colours of the
slice gave "Uninitialized Slice".

**With the port.** A palette is accepted wherever its colours lie. A
palette that was never made is the error "Uninitialized Palette", and a
colour beyond the colours of the slice the error "Not enough colours in
Slice". A slice that is not set up is still "Uninitialized Slice".

**What to change.** Nothing; a program that stopped with one of these
errors stops with the message that names its cause.

Issues:

* [The checks of the fades tested only the upper word of the palette (#135)](https://github.com/fubar-coder/bb2-c/issues/135)
* [An uninitialised palette reported "Not enough colours in Slice" (#330)](https://github.com/fubar-coder/bb2-c/issues/330)
* [A too large colour range reported "Uninitialized Slice" (#331)](https://github.com/fubar-coder/bb2-c/issues/331)

{{% fixed-bugs-library part="elsewhere" %}}
