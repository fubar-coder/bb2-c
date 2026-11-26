---
title: iffiolib
weight: 29
description: The bugs of Blitz Basic 2 in the command library iffiolib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### IFF files

These are the commands that load and save IFF files, the Amiga's standard
format for pictures (ILBM), colours, sounds and animations.

* A read error of the disk or device during an IFF load counted as 65535
  bytes read, and the old contents of the read buffer were taken as the
  file's data: `LoadShape` of a device that cannot be read loaded the
  previous picture again, and the search for a chunk could hang. A read
  error is the end of the file, and the load command stops with its
  runtime error
  ([#177](https://github.com/fubar-coder/bb2-c/issues/177)).
* A chunk of length 0 was read or skipped as one of 4 GB: `LoadSound` of
  a sound file with an empty BODY copied the rest of the file over the
  memory behind the sound. A chunk of length 0 reads nothing
  ([#179](https://github.com/fubar-coder/bb2-c/issues/179)).

{{% fixed-bugs-library part="elsewhere" %}}
