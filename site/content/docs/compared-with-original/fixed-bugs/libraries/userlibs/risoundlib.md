---
title: risoundlib
weight: 42
description: The bugs of Blitz Basic 2 in the RWE userlib risoundlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in risoundlib {#risoundlib-fixes}

These fixes concern `CludgeSound` of
[risoundlib]({{< relref "/docs/libraries/userlibs/risoundlib" >}}),
which makes a sound object of an IFF 8SVX file in memory. The second
sound library,
[risoundlib2]({{< relref "/docs/libraries/userlibs/risoundlib2" >}}),
has the same command with the same fixes. Two more fixes of
`CludgeSound` are in [More fixes](#more-fixes) and in the
[More fixes of risoundlib2]({{< relref "/docs/compared-with-original/fixed-bugs/libraries/userlibs/risoundlib2#more-fixes" >}}).

* The sample rate comes from the rate field of the file's header chunk,
  whatever the length of that chunk. The original read it at a place
  that is the rate only when the header chunk has the usual 20 bytes
  ([#832](https://github.com/fubar-coder/bb2-c/issues/832)).
* A file with a sample rate of 0 gives the result 0. The original hung
  ([#830](https://github.com/fubar-coder/bb2-c/issues/830)).
* The search for the sample data skips the pad byte that IFF puts behind
  a chunk of odd length, and stops at the end of the file's FORM. The
  original went on one byte too early behind such a chunk: it missed the
  sample data of a correct file, or ran into the data of the file or
  past it. A file written without the pad bytes, which is no valid IFF
  file but loaded with the original, gives the result 0
  ([#828](https://github.com/fubar-coder/bb2-c/issues/828)).
* A `CludgeSound` that fails on the file leaves the sound as it was. The
  original freed the old sample first and left the sound pointing to the
  freed memory, so the next `CludgeSound` of that sound stopped with
  "Unable to Free Memory"
  ([#831](https://github.com/fubar-coder/bb2-c/issues/831)).
* The whole sample is copied, up to 131071 bytes; an empty sample and
  one of 131072 bytes or more give the result 0, since the length field
  of a sound object cannot describe more; `LoadSound` refuses such a
  sample too. The original copied only a
  part of a sample of more than 64 KB, and for an empty one wrote
  64 KB into a block of 8 bytes
  ([#829](https://github.com/fubar-coder/bb2-c/issues/829)).

### More fixes {#more-fixes}

Each point says first what the original did and then what the port
does.

* `CludgeSound`, in both sound libraries, on a sound that `DecodeSound`
  made from data in chip memory stopped the program with "Unable to Free
  Memory", because it freed memory that no library had allocated; that
  memory is left alone
  ([#945](https://github.com/fubar-coder/bb2-c/issues/945)).

{{% fixed-bugs-library part="elsewhere" %}}
