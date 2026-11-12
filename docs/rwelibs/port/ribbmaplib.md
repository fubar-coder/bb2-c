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
steps by 8 + the chunk length (no IFF padding); the last chunk found is
kept per file and returned without a search when the same id is asked
for again. init and finit are `RTS`.

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

## BUG(orig), kept

* `SetBBMapLoc` stores the address before it checks it: after a failed
  call the info chunk (`BBMapScenarioID`/`Width`/`Height`/`Info`), the
  cache and the end of the old map stay, the searches start at the new
  address (#554). A map without `MFRM` makes the info chunk the address
  behind the last id read (#551).
* The search stops only after an id that does not match: the id behind
  the end of the file is read and found if it matches. The addresses
  are compared signed (#553). An odd chunk length makes the following
  ids odd (address error on a 68000) (#552).
* `BBGetPalette`: when the high colour is not below the object's colour
  count it allocates a block of high+1 colours but sets the count (in
  the object and the block) to high, so the last colour is outside
  (#548); the colours are written from low up to high, at least one
  (low > high writes the colour at low, possibly behind the block)
  (#549).
* `BBScenarioLowCol`/`BBScenarioHighCol` sign-extend the byte;
  `BBGetPalette` and `BBNumBaddies` do not (#550).

## Original behaviour, not defects

* The scenario routines before a `SetBBScenarioLoc`, and the map chunk
  routines before a `SetBBMapLoc`, search from address 12: a program
  sets the location first.
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
the end of a file (found on odd rounds), `BBShadowValue` with a word
index overflow and a negative index, author strings at odd and even
addresses, `BBGetPalette` without and with a new block (from 0–4 and 4
colours, twice), with 200 colours, low > high and without `COLR`, a
failed `SetBBScenarioLoc` and a failed `SetBBMapLoc` after a good one.
Observed: d0 (addresses relative to the
buffer), strings and a3, palette objects and colour blocks, kept
registers. Not covered: calls before the first `Set…`, odd chunk
lengths, `BBGetPalette` writing behind its block.
