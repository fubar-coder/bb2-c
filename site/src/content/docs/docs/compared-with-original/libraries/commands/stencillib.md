---
title: stencillib
description: The bugs of Blitz Basic 2 in the command library stencillib that
  the BB2 C port fixes.
sidebar:
  order: 57
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/stencillib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Shapes and sprites

These are the commands of shapes, their changes such as `Scale` and
`Rotate`, sprites, stencils and the collisions of sprites. One more fix
of them can stop an existing program:
[`LoadShape` refuses pictures too large to blit](/docs/compared-with-original/libraries/commands/shapeslib/#loadshape-blit-size).

* `Stencil` of a bitmap of more than 1024 lines, or wider than 1024
  pixels, filled the stencil only in part; it fills the whole stencil
  ([#284](https://github.com/fubar-coder/bb2-c/issues/284)).

::fixed-bugs-library{part="elsewhere"}
