---
title: ristonecrackerlib
weight: 44
description: The bugs of Blitz Basic 2 in the RWE userlib ristonecrackerlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ristonecrackerlib and ristcdecrunchlib {#stonecracker-fixes}

These fixes concern the two libraries by Reflective Images for data
files packed with StoneCracker 4.04:
[ristonecrackerlib]({{< relref "/docs/libraries/userlibs/ristonecrackerlib" >}}),
which tells the sizes of such a file and loads and unpacks one with
`STCLoad`, and
[ristcdecrunchlib]({{< relref "/docs/libraries/userlibs/ristcdecrunchlib" >}}),
whose `STCDecrunch` unpacks one in memory. Both use the same unpacking
code.

* `STCDecrunch` and `STCLoad` write nothing below the destination for
  damaged StoneCracker data; such data give 0. The original wrote a
  piece of the data past the start of the destination, into the memory
  below, and could crash the machine
  ([#833](https://github.com/fubar-coder/bb2-c/issues/833)).
* `STCLoad` returns 0 for a file that is not, or not wholly, a
  StoneCracker 4.04 file, and for one it cannot read. The original
  unpacked whatever it had read and could write anywhere in memory
  ([#836](https://github.com/fubar-coder/bb2-c/issues/836)).
* `STCLoad` releases the lock of the file once. The original released
  it a second time after closing the file, on every load that worked
  ([#835](https://github.com/fubar-coder/bb2-c/issues/835)).
* `STCLoad` works on Kickstart 1.x: it opens the file by its name, which
  works on every Kickstart. The original opened it with a function of
  Kickstart 2.0, and a program crashed at `STCLoad` on Kickstart 1.x
  ([#1147](https://github.com/fubar-coder/bb2-c/issues/1147)).
* ristonecrackerlib closes dos.library at the end of the program. The
  original left it open, one more time after every run
  ([#834](https://github.com/fubar-coder/bb2-c/issues/834)).

{{% fixed-bugs-library part="elsewhere" %}}
