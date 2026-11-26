---
title: collslib
weight: 14
description: The bugs of Blitz Basic 2 in the command library collslib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/collslib/]
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
[`LoadShape` refuses pictures too large to blit]({{< relref "/docs/compared-with-original/libraries/commands/shapeslib#loadshape-blit-size" >}}).

#### `SColl` with two channels of the same pair {#scoll-same-pair}

**What happened.** The sprite channels are used in pairs, 0 and 1, 2 and
3, 4 and 5, 6 and 7, and the hardware reports collisions only between
sprites of different pairs. `SColl(0,1)` tested the collision of other
channels and returned its result. `SColl` with channel 0 or 1 as the
second channel, such as `SColl(4,0)`, tested the collision of other
channels too.

**With the port.** With runtime error checking, two channels of the same
pair are the error "Sprite channels must be of different pairs". Two
equal channels are the error "Sprite channels must be different", as in
the original. For two channels of different pairs, the order of the
channels does not matter.

**What to change.** Test the collision of two sprites on channels of
different pairs.

Issue: [SColl with channel 0/1 as the second argument or channels of one pair tested the wrong bit (#110)](https://github.com/fubar-coder/bb2-c/issues/110).

{{% fixed-bugs-library part="elsewhere" %}}
