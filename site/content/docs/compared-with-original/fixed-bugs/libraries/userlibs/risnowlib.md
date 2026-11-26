---
title: risnowlib
weight: 40
description: The bugs of Blitz Basic 2 in the RWE userlib risnowlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in risnowlib {#risnowlib-fixes}

These fixes concern
[risnowlib]({{< relref "/docs/libraries/userlibs/risnowlib" >}}), the
falling snow of Reflective Images.

* With runtime error checking, every snow command with a field number
  outside 0 to 3 stops with "Snow list number must be 0 to 3",
  `LDoSnow`, `LDoSnowActivate` and `KillPoint` on a field without
  `InitSnowDrop` with "Snow list needs InitSnowDrop", and `KillPoint`
  outside the bitmap with "KillPoint outside the bitmap". The original
  read and wrote outside the library's table, at address 0 or around
  the bitmap
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#snow-list-checks" >}}),
  [#913](https://github.com/fubar-coder/bb2-c/issues/913)).
* `InitSnowDrop` with 1024 points or more allocates enough memory for
  the field's lists. The original computed the size in 16 bits, got a
  block far too small and wrote the lists behind it
  ([#912](https://github.com/fubar-coder/bb2-c/issues/912)).
* `LDoSnowActivate` before the first `LDoSnow` of a field works on the
  field's lists. The original wrote its entries to address 0
  ([#908](https://github.com/fubar-coder/bb2-c/issues/908)).
* A second `LDoSnowActivate` before the next `LDoSnow` keeps the snow
  the first one let fall. In the original it overwrote those flakes,
  and they vanished without falling
  ([#909](https://github.com/fubar-coder/bb2-c/issues/909)).
* `InitSnowDrop` of a field that is already set up frees the field's
  old lists. The original lost their memory until the end of the
  program
  ([#911](https://github.com/fubar-coder/bb2-c/issues/911)).
* `LDoSnow` takes the bit of a new flake from the random table modulo
  8. In the original, a flake whose bit was above 7 looked up its moves
  in the wrong place and could jump through memory, up to 32 KB away,
  every frame again
  ([#910](https://github.com/fubar-coder/bb2-c/issues/910)).
* The lists of a field take no more flakes than they have room for:
  `LDoSnow` adds no new flake to a full list, `LDoSnowActivate` leaves
  settled snow where it is when the list of falling snow is full, and
  `KillPoint` does nothing and returns 0 when its list is full. In the
  original, a field whose flakes piled up ran over into the next field's
  lists, and the last field behind its memory
  ([#914](https://github.com/fubar-coder/bb2-c/issues/914)).
* A program run a second time from the editor starts with no field set
  up. In the original, a snow command before `InitSnowDrop` worked on
  the lists of the run before, which had been freed
  ([#1135](https://github.com/fubar-coder/bb2-c/issues/1135)).

{{% fixed-bugs-library part="elsewhere" %}}
