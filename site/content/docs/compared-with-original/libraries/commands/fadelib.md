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
* With runtime error checking, `FadeIn` and `FadeOut` took a palette whose
  colours lie in the lowest 64 KB of memory as uninitialised, and gave
  two errors the wrong message: a palette that was never made was "Not
  enough colours in Slice", a colour beyond the colours of the slice
  "Uninitialized Slice". The palette is accepted, and the messages are
  "Uninitialized Palette" and "Not enough colours in Slice"
  ([#135](https://github.com/fubar-coder/bb2-c/issues/135),
  [#330](https://github.com/fubar-coder/bb2-c/issues/330),
  [#331](https://github.com/fubar-coder/bb2-c/issues/331);
  [The checks of `FadeIn` and `FadeOut`]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#fade-checks" >}})).

{{% fixed-bugs-library part="elsewhere" %}}
