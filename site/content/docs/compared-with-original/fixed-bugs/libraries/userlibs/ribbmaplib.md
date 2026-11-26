---
title: ribbmaplib
weight: 10
description: The bugs of Blitz Basic 2 in the RWE userlib ribbmaplib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ribbmaplib {#ribbmaplib-fixes}

These fixes concern
[ribbmaplib]({{< relref "/docs/libraries/userlibs/ribbmaplib" >}}), the
library by Reflective Images that reads the map and scenario files of
the game Blitz Bombers from memory. `SetBBMapLoc` and
`SetBBScenarioLoc` choose a file; the other commands look up its chunks.

* The search for a chunk skips the pad byte that the IFF standard puts
  behind a chunk of odd length; a file written without it stays
  readable. The original read the next chunk one byte early: on a 68000
  this crashed the program, on other processors the chunks behind were
  not found ([#552](https://github.com/fubar-coder/bb2-c/issues/552)).
* The search ends at the end of the file. The original read one chunk
  name behind it, in the memory after the file, and took it for a chunk
  of the file if it matched
  ([#553](https://github.com/fubar-coder/bb2-c/issues/553)).
* A failed `SetBBMapLoc` leaves the map chosen before as it was. The
  original took the new address all the same, so the map functions mixed
  the rejected file with the old map
  ([#554](https://github.com/fubar-coder/bb2-c/issues/554)).
* `BBMapScenarioID`, `BBMapWidth`, `BBMapHeight` and `BBMapInfo` return
  0 for a map without an information chunk (MFRM). The original read
  them from behind the file
  ([#551](https://github.com/fubar-coder/bb2-c/issues/551)).
* `BBScenarioLowCol` and `BBScenarioHighCol` return the colours 128 to
  255 as such. The original returned them as negative numbers
  ([#550](https://github.com/fubar-coder/bb2-c/issues/550)).
* `BBGetPalette` counts all the colours of a palette it enlarges, so the
  scenario's last colour is used and a second call keeps the palette.
  The original counted one colour less
  ([#548](https://github.com/fubar-coder/bb2-c/issues/548)).
* `BBGetPalette` with a colour range whose first colour is above the
  last stores nothing. The original wrote one colour, possibly behind
  the palette's memory
  ([#549](https://github.com/fubar-coder/bb2-c/issues/549)).
* `BBGetPalette` reads only the colours that the scenario's COLR chunk
  really holds. The original took the rest from the bytes behind the
  chunk ([#1113](https://github.com/fubar-coder/bb2-c/issues/1113)).
* With runtime error checking, `BBShadowValue` with a shadow set that
  the scenario does not have stops with the runtime error "Shadow set
  must lie in the SHDW chunk". The original returned bytes from behind
  the chunk, as the port does without error checking
  ([#1114](https://github.com/fubar-coder/bb2-c/issues/1114)).
* A program run a second time from the editor starts without the map
  and the scenario of the run before. The original's commands found the
  files of the first run, which may have been freed
  ([#1108](https://github.com/fubar-coder/bb2-c/issues/1108)).

{{% fixed-bugs-library part="elsewhere" %}}
