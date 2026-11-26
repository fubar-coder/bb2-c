---
title: riwbspritelib
weight: 49
description: The bugs of Blitz Basic 2 in the RWE userlib riwbspritelib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/riwbspritelib/]
---

{{< fixed-bugs-library >}}

## Changes you may notice

### The results of `GetWBSprite` and `MoveWBSprite` {#wbsprite-results}

`GetWBSprite(x,y,shape,spritechannel)` of
[riwbspritelib]({{< relref "/docs/libraries/userlibs/riwbspritelib" >}})
makes a hardware sprite of a shape on the Workbench screen and returns
its sprite channel, 0 to 7. `MoveWBSprite(viewport,x,y,sprite_channel)`
moves it.

**What happened.** The original's `GetWBSprite` returned 0 when it
failed for some reasons and -1 for others, and 0 is also the first
channel, so a program could not tell a failure from a sprite on
channel 0. Asked for a channel the library already held, it failed but
returned the channel, as if it had got a second sprite there:

```
BitMap 0,320,64,2
Boxf 0,0,15,15,3
GetaShape 0,0,0,16,16
GetaShape 1,0,0,8,8
s.l=GetWBSprite(10,20,0,2)
NPrint s
t.l=GetWBSprite(50,60,0,2)
NPrint t
u.l=GetWBSprite(10,20,1,5)
NPrint u
```

The original printed `2`, `2` and `0`: the second call on channel 2
seemed to work, and the shape narrower than 16 pixels gave 0. The port
prints `2`, `-1` and `-1`. As a function, `MoveWBSprite` returned 0
without a sprite on the channel, and otherwise whatever value the
system had left behind, such as `8`.

**With the port.** `GetWBSprite` returns the channel it got, 0 to 7,
and -1 whenever it gets no sprite. `MoveWBSprite` returns -1 when it
moved the sprite and 0 when the channel has no sprite of the library.

**What to change.** Test the result of `GetWBSprite` for -1, or for 0
or more, as in `If ch>=0`, instead of for 0. A program that took 0 for
a failure works as before, except that a sprite on channel 0 now counts
as success.

Issues:

* [GetWBSprite reports success (the channel) when GetExtSpriteA fails on a channel the library already holds (#920)](https://github.com/fubar-coder/bb2-c/issues/920)
* [MoveWBSprite returns whatever MoveSprite leaves in d0 (#921)](https://github.com/fubar-coder/bb2-c/issues/921)

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in riwbspritelib {#riwbspritelib-fixes}

These fixes concern
[riwbspritelib]({{< relref "/docs/libraries/userlibs/riwbspritelib" >}}),
the hardware sprites of Reflective Images on the Workbench screen. The
changed results of `GetWBSprite` and `MoveWBSprite` are described in
[Changes you may notice](#wbsprite-results).

* The Workbench sprite commands work on Kickstart 1.x and 2.x, with
  sprites 16 pixels wide in up to 3 colours, made of a shape of 1 or 2
  planes. As on Kickstart 3.0, the width of the shape is rounded down to
  16: a shape 20 pixels wide makes a sprite, one of 32 pixels or more is
  refused, and with runtime error checking it stops with "Shape too big
  for a sprite below Kickstart 3.0". The original called functions of
  Kickstart 3.0 there and crashed at the first `GetWBSprite`
  ([#1151](https://github.com/fubar-coder/bb2-c/issues/1151)).
* `ChangeWBSprite` frees the new sprite image when the system refuses
  the change. The original lost a block of chip memory each time, until
  the next reboot
  ([#915](https://github.com/fubar-coder/bb2-c/issues/915)).
* A sprite made of a shape whose width is no multiple of 16 shows the
  left part of the shape, as wide as the sprite: the width of the shape
  rounded down to a multiple of 16, so 16 pixels of a shape 20 pixels
  wide. The original copied the rows of such a shape out of step, so
  its image was sheared and its colours were wrong
  ([#918](https://github.com/fubar-coder/bb2-c/issues/918)).
* With runtime error checking, every sprite command refuses a channel
  outside 0 to 7 (-1 to 7 for `GetWBSprite`) with "Sprite channel must
  be 0 to 7" or "Sprite channel must be -1 to 7". The original read and
  freed the library's own data as sprites
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#wbsprite-checks" >}}),
  [#916](https://github.com/fubar-coder/bb2-c/issues/916)).
* With runtime error checking, `GetWBSprite` and `ChangeWBSprite` refuse
  a shape of height 0 with "Illegal Shape Height" and one of depth 0 or
  more than 8 with "Illegal Shape Depth". The original would copy 65536
  rows or planes
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#wbsprite-checks" >}}),
  [#917](https://github.com/fubar-coder/bb2-c/issues/917)).
* With runtime error checking, `WBSpriteX` and `WBSpriteY` of a channel
  without a sprite stop with "No sprite on this channel". The original
  returned words from low memory
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#wbsprite-checks" >}}),
  [#922](https://github.com/fubar-coder/bb2-c/issues/922)).

{{% fixed-bugs-library part="elsewhere" %}}
