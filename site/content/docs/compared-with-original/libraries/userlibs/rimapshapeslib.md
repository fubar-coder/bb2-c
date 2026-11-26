---
title: rimapshapeslib
weight: 32
description: The bugs of Blitz Basic 2 in the RWE userlib rimapshapeslib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/rimapshapeslib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rimapshapeslib {#rimapshapeslib-fixes}

These fixes concern
[rimapshapeslib]({{< relref "/docs/libraries/userlibs/rimapshapeslib" >}}),
the map shapes and groups of Reflective Images' map editor.

* Programs with the library run on Kickstart 1.3. The original
  allocated its memory with a function of Kickstart 2.0, and every
  program with it crashed at its start
  ([#1141](https://github.com/fubar-coder/bb2-c/issues/1141)).
* `BlitGroup` calls the graphics library itself. The original called
  whatever library the command before had left in the register for it,
  so it worked only after a command that left the graphics library there
  ([#902](https://github.com/fubar-coder/bb2-c/issues/902)).
* `AddGroupMember` before the first `ResetGroup` adds to the empty
  group. The original wrote the member at address 0, over the pointer
  to exec at address 4
  ([#901](https://github.com/fubar-coder/bb2-c/issues/901)).
* Without memory for the group at the start of the program,
  `ResetGroup` returns 0 and does nothing, and `AddGroupMember` and
  `BlitGroup` do nothing. The original wrote the group at address 0
  ([#905](https://github.com/fubar-coder/bb2-c/issues/905)).
* A group holds at most 63 members, and `AddGroupMember` ignores
  further ones; with runtime error checking a 64th member stops with
  "Group holds at most 63 members". The original wrote them behind the
  memory of the group
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#group-63-members" >}}),
  [#904](https://github.com/fubar-coder/bb2-c/issues/904)).
* With runtime error checking, `ShapeToMapShape` of a shape of depth 0
  or more than 8 stops with "Illegal Shape Depth". The original wrote
  65536 plane pointers over memory, or into the next map shape
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#shapetomapshape-depth" >}}),
  [#903](https://github.com/fubar-coder/bb2-c/issues/903)).
* Without map shapes, before `SetNumMapShapes` or after one that got no
  memory, the map shape commands do nothing and `MapShapeAddr` returns
  0. The original took the map shapes at address 0 and wrote over the
  memory there. With runtime error checking, a map shape number beyond
  `SetNumMapShapes` stops with "Map shape not allocated by
  SetNumMapShapes", and a `BlitMapShapeSize` wider than the map shape
  with "BlitMapShapeSize wider than the map shape"
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#mapshape-number-checks" >}}),
  [#906](https://github.com/fubar-coder/bb2-c/issues/906)).
* A program run a second time from the editor starts with an empty
  group. In the original, `AddGroupMember` before `ResetGroup` added to
  the group of the run before, which had been freed, with that run's
  block size
  ([#1134](https://github.com/fubar-coder/bb2-c/issues/1134)).

{{% fixed-bugs-library part="elsewhere" %}}
