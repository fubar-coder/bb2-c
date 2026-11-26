---
title: bblitlib
weight: 8
description: The bugs of Blitz Basic 2 in the command library bblitlib that the BB2 C port fixes.
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

* `BBlit`, `UnBuffer` and `ShowStencil` stepped from plane to plane in
  the buffer by the shape's plane size as a signed number, so with a
  shape of 32 KB or more per plane they stepped backwards and wrote and
  read outside the buffer; they step forwards
  ([#1045](https://github.com/fubar-coder/bb2-c/issues/1045)).

{{% fixed-bugs-library part="elsewhere" %}}
