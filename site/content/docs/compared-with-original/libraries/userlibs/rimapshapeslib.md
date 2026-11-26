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
* A program run a second time from the editor starts with an empty
  group. In the original, `AddGroupMember` before `ResetGroup` added to
  the group of the run before, which had been freed, with that run's
  block size
  ([#1134](https://github.com/fubar-coder/bb2-c/issues/1134)).

#### A group of more than 63 members {#group-63-members}

`AddGroupMember` of
[rimapshapeslib]({{< relref "/docs/libraries/userlibs/rimapshapeslib" >}})
adds a map shape to the group that `BlitGroup` draws in one go. The
group has room for 63 members.

**What happened.** The original did not count the members: from the
64th on, it wrote them behind the memory of the group.

**With the port.** A group holds at most 63 members, and
`AddGroupMember` ignores further ones. With runtime error checking, a
64th member is the error "Group holds at most 63 members". A program
that added 64 members drew them all with the original, if nothing else
used the memory behind the group; with the port, `BlitGroup` draws
the first 63.

**What to change.** Keep a group at 63 members or fewer; draw more map
shapes with a second group or with `BlitMapShape`.

Issue: [AddGroupMember does not count members: from the 64th on it writes behind the 512-byte group block (#904)](https://github.com/fubar-coder/bb2-c/issues/904).

#### `ShapeToMapShape` of a shape of depth 0 or more than 8 {#shapetomapshape-depth}

`ShapeToMapShape` of
[rimapshapeslib]({{< relref "/docs/libraries/userlibs/rimapshapeslib" >}})
makes a map shape from a Blitz shape. A map shape has room for the
pointers to 8 planes.

**What happened.** The original checked the depth of the shape
nowhere, also not with runtime error checking. A shape of depth 0
wrote 65536 plane pointers over memory, and one of 14 planes or more
wrote into the next map shape.

**With the port.** With runtime error checking, a shape of depth 0 or
more than 8 is the error "Illegal Shape Depth", as with the shape
commands of Blitz Basic 2. Without runtime error checking, the command
is as before.

**What to change.** Make map shapes from shapes of 1 to 8 planes.

Issue: [ShapeToMapShape of a shape with depth 0 writes 65536 plane pointers; depth 14 and up runs into the next map shape (#903)](https://github.com/fubar-coder/bb2-c/issues/903).

#### Map shape numbers beyond `SetNumMapShapes` {#mapshape-number-checks}

The map shapes of
[rimapshapeslib]({{< relref "/docs/libraries/userlibs/rimapshapeslib" >}})
are numbered from 0; `SetNumMapShapes num` makes room for `num` of
them. `BlitMapShapeSize` blits the top left part of a map shape.

**What happened.** The original checked the numbers nowhere, also not
with runtime error checking: a number beyond `SetNumMapShapes` read and
wrote the memory behind the map shapes, and before any
`SetNumMapShapes` the map shapes lay at address 0, and the commands
wrote over the memory there. `BlitMapShapeSize`
with a width larger than the map shape's read each row on into the
next and the last one behind the shape.

**With the port.** Without map shapes, before `SetNumMapShapes` or
after one that got no memory or asked for 0, the map shape commands
do nothing and `MapShapeAddr` returns 0. With runtime error checking,
the port reports two errors:

* For `ShapeToMapShape`, `BlitMapShape`, `BlitMapShapeSize`,
  `MapShapeAddr`, `FreeMapShapes` and `AddGroupMember`, a map shape
  number not below the number given to `SetNumMapShapes` is the error
  "Map shape not allocated by SetNumMapShapes".
* A `BlitMapShapeSize` wider than the map shape is the error
  "BlitMapShapeSize wider than the map shape".

**What to change.** Call `SetNumMapShapes` first, use only the numbers
it makes room for, and blit at most the width of the map shape.

Issue: [Map shape numbers are not checked; before SetNumMapShapes the map shapes lie at address 0 (#906)](https://github.com/fubar-coder/bb2-c/issues/906).

{{% fixed-bugs-library part="elsewhere" %}}
