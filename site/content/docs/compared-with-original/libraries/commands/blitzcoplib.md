---
title: blitzcoplib
weight: 10
description: The bugs of Blitz Basic 2 in the command library blitzcoplib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/blitzcoplib/]
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

* `CustomCop` with an empty string, or one of 1 to 3 bytes, copied about
  four thousand million instructions from the string over all memory
  behind the new copper list; such a string inserts only the wait for its
  line
  ([#404](https://github.com/fubar-coder/bb2-c/issues/404)).
* `CustomCop` in a program whose Slice maximum in the compiler options is
  0 changed memory in 64-byte steps over 4 MB from the place of the slice
  objects on; it changes nothing there
  ([#406](https://github.com/fubar-coder/bb2-c/issues/406)).
* `CustomCop` took the copper instruction `$01FE1234` in its string for
  the mark `Slice` uses inside the copper list for a wait at line 256,
  and changed it and the instruction after it; the string goes into the
  copper list as given
  ([#405](https://github.com/fubar-coder/bb2-c/issues/405)).
* `Show`, `ShowF` and `ShowB` on a slice made without the BPLCON1
  instruction (bit 14 of its flags) wrote to a random address; they
  change nothing outside the slice
  ([#407](https://github.com/fubar-coder/bb2-c/issues/407)).

{{% fixed-bugs-library part="elsewhere" %}}
