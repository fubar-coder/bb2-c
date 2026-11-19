# ribbmaplib (34): Blitz Bombers maps and scenarios

Reflective Images (Steve McNamara, Steve Matty). 35 commands that read
the map (`FORM` … `BMAP`) and scenario (`FORM` … `SCEN`) files of Blitz
Bombers in memory:

* `SetBBMapLoc address`, `SetBBScenarioLoc address` – the file to read;
  -1, or 0 if it is no `FORM` of that type.
* Scenario: `BBScenarioName`, `BBScenarioAuthor` (the strings of `ANNO`),
  `BBScenarioDepth` (`DEPF`), `BBScenarioLevel` (`LEVL`),
  `BBScenarioShapeLoc` (`GFX!`), `BBScenarioSounds` (`SFX!`),
  `BBScenarioBaddiesLoc`/`BBNumBaddies` (`BAD!`), `BBScenarioPalette`,
  `BBScenarioLowCol`, `BBScenarioHighCol`, `BBGetPalette palette#`
  (`COLR`), `BBNumBackGround`/`BBNumDestruct`/`BBNumIndestruct`
  (`SHAP`), `BBScenarioPrefix` (`PRFX`), `BBNumTunnels`/`BBTunnelHoles`/
  `BBTunnelsLoc` (`TUNL`), `BBShadowValue(set)` (`SHDW`),
  `BBDestructLoc` (`DEST`).
* Map: `BBMapScenarioID`, `BBMapWidth`, `BBMapHeight`, `BBMapInfo`
  (`MFRM`, found by `SetBBMapLoc`), `BBMapPowerUps` (`PWUP`),
  `BBMapData` (`MDAT`), `BBMapStartPos` (`STRT`), `BBMapName$` (`NAME`),
  `BBMapPUMask` (`PWMK`), `BBMapDBOverLay` (`DBOL`),
  `BBMapOnePlayerInfo` (`1PLY`), `BBMapObjects` (`OBJS`).

A missing chunk gives 0. The search starts 12 bytes into the file and
steps by 8 + the chunk length and the IFF pad byte of an odd length
(unless that byte is not 0); the last chunk found is
kept per file and returned without a search when the same id is asked
for again. init clears the files and the caches; finit is an `RTS`.

* **Source:** `rwelibsrc/blitzbomberslib/bbmaplib2.bb` (30/9/95; as TED
  shows it: `build/amiga/rwelibsrc/blitzbomberslib/bbmaplib2.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/ribbmaplib.obj`); no
  binary exists (`docs/rwelibs/README.md` §3.3). Declarations equal
  (ctest `rwe_declarations_ribbmaplib`).
* **C file:** `libs/userlibs/rwe/ribbmaplib.c`; test
  `libs/tests/rwe/ribbmaplib.c` (snitch case "rwe ribbmaplib").

## C and assembler

All C behind `BLITZ_THUNK`s that keep d1/a0/a1 (the original changes
d3/a0/a1 in the chunk search, d1/d2 in the string routines, up to
d0–d7/a0–a3 in `BBGetPalette`). d0 is reproduced in full: the word
functions have `$FFFF` in the upper word (the `MOVEQ #-1,d0` of the
search), `BBMapWidth`/`BBMapHeight` the caller's upper word. The strings
are written at a3 without a terminating 0 (a3 ends behind the last
character). `BBGetPalette` calls memlib's globalloc/globfree.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* init clears the map and scenario locations, the info chunk and the
  caches. The original's init is an `RTS`, so a second run of a program
  with the library still loaded (from the editor) started with the run
  before's files: `BBMapScenarioID`, `BBMapWidth`, `BBMapHeight` and
  `BBMapInfo` before a `SetBBMapLoc` read the old map (perhaps freed
  memory), the cached chunks were returned and the searches ran through
  the old files (#1108).
* `SetBBMapLoc` stores the address only after its checks: a failed call
  leaves the map before whole (as `SetBBScenarioLoc`). The original
  stored it first: after a failed call the info functions used the old
  map's `MFRM`, the searches ran from the rejected address to the old
  end, the cache returned a chunk of the old map, and before any good
  call the info functions read from address 0 (#554).
* A map without `MFRM` has no info chunk: `BBMapScenarioID`,
  `BBMapWidth`, `BBMapHeight` and `BBMapInfo` give 0, `SetBBMapLoc`
  still succeeds (the other chunks are usable). The original took the
  address behind the last chunk id read, behind the file, as the info
  chunk (#551).
* The search tests the end of the file before it reads an id: a chunk
  counts only if its id and length lie inside the file, and a length
  that reaches the end (also one that would wrap the address) ends the
  search. The original read the id behind the end and found it if it
  matched (e.g. the next file in the same buffer), and compared the
  addresses signed (#553).
* The search skips the IFF pad byte behind a chunk of odd length; a
  byte other than 0 in its place is taken as the start of the next
  chunk, so files written without pad bytes stay readable (the shape of
  #189). The original stepped by 8 + length: after an odd chunk the next
  id was read one byte early, at an odd address (on a 68000 an address
  error, on a 68020 a wrong id and a wrong length) (#552).
* `BBScenarioLowCol`/`BBScenarioHighCol` read the colour byte unsigned,
  as `BBGetPalette` and `BBNumBaddies` read theirs; the original
  sign-extended it, so colours 128-255 came out as -128..-1 (#550).
* `BBGetPalette` sets the count of a new colour block (in the block and
  the object) to high+1, the number of colours it holds. The original
  set it to high: the scenario's last colour lay outside the palette
  (`Use Palette` did not set it), and every further `BBGetPalette`
  allocated a new block again (#548).
* `BBGetPalette` with low > high in the `COLR` chunk stores nothing (an
  empty range). The original's loop tested its end only after a
  colour: it wrote the colour at low, behind the colour block when low
  was not below its count (#549).
* `BBGetPalette` reads only the colours that lie inside the `COLR` chunk
  (and the file): high is cut to the last colour the chunk holds, and
  nothing is stored when it holds none. The original read the colours
  low..high from the bytes behind a shorter chunk: the next chunk, or
  memory behind the file (#1113).
* `BBShadowValue` with runtime error checking refuses a set outside the
  `SHDW` chunk (negative, from $2000 on, where the word index wraps, or
  not below the chunk's length / 4): the runtime error "Shadow set must
  lie in the SHDW chunk" (`shadowchk`, an error check area the original
  has not). The release code is unchanged: such a set is an invalid
  argument (D-005) and reads the following chunk or memory behind the
  file, as in the original (#1114).

## Original behaviour, not defects

* Before the first `SetBBScenarioLoc`/`SetBBMapLoc` the routines give 0
  and read nothing: the location and the end are 0, the search's end
  test (#553) ends it at once. (The original searched from address 12:
  it read the long there and found the chunk if it matched; a program
  sets the location first.)
* The cache is the id alone: a failed `SetBBScenarioLoc` leaves the
  file and its cache as they were (the old file stays usable).
* `BBGetPalette` frees the old colour block with the size count*12:
  memlib's globfree ignores the size.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_ribbmaplib.txt` recorded from the
original. One buffer holds the files: a full
scenario, a full map, a scenario with only `GFX!`, `SFX!` and a `COLR`
of $81..$83 (or none), a map without `MFRM`, a `FORM` of the wrong type;
chunks shuffled with random lengths and contents, six rounds. Every
accessor twice (search and cache), present and missing, an id behind
the end of a file (on odd rounds; the original finds it),
`BBShadowValue` with a word index overflow and a negative index, author
strings at odd and even addresses, `BBGetPalette` without and with a
new block (from 0–4 and 4 colours, twice), with 200 colours, low > high
and without `COLR`, a failed `SetBBScenarioLoc` and a failed `SetBBMapLoc` after a good one;
a second run (init after finit, as from the editor with the library
still loaded, the run before ending with a good scenario and map): the
map info functions, two cached chunks and a search before any `Set…`.
Observed: d0 (addresses relative to the
buffer), strings and a3, palette objects and colour blocks, kept
registers. Not covered: calls before the first `Set…` of the first run,
odd chunk lengths (the bug-orig tests `ribbmaplib-odd-chunk-length` and
`ribbmaplib-odd-chunk-length-68000`), `BBGetPalette` writing behind its
block (`ribbmaplib-getpalette-low-gt-high`).
