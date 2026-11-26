---
title: collslib
weight: 14
description: The bugs of Blitz Basic 2 in the command library collslib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Shapes and sprites

These are the commands of shapes, their changes such as `Scale` and
`Rotate`, sprites, stencils and the collisions of sprites. One more fix
of them can stop an existing program:
[`LoadShape` refuses pictures too large to blit]({{< relref "/docs/compared-with-original/fixed-bugs/libraries/commands/shapeslib#loadshape-blit-size" >}}).

* `SColl` with channel 0 or 1 as the second channel, such as
  `SColl(4,0)`, tested the collision of other channels; the order of the
  channels does not matter. Two channels of the same pair, such as
  `SColl(0,1)`, have no collision of their own: with runtime error
  checking they are the error "Sprite channels must be of different
  pairs"
  ([#110](https://github.com/fubar-coder/bb2-c/issues/110)).

{{% fixed-bugs-library part="elsewhere" %}}
