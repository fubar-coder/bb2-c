# Proposal

## Why

Sources written for AmiBlitz3 cannot be compiled with this repository's
blitz2 today: blitz2 only takes BB2-tokenised sources from TED and loads
BB2's libraries. The spike of 2026-10-31 (`docs/ab3/variant.md`,
"Spike result") showed that today's blitz2 already loads AmiBlitz3
v3.14.0's binary libraries unchanged and compiles AB3 programs that use
only BB2 syntax with them; what is missing is the way in (text sources,
AB3's token table, AB3's library files) and two conventions (the
library number of `^`, resident paths). This change is phase G1 of the
plan; the later phases (G2–G7) extend the language on top of it.

## What Changes

- A second build of the compiler, `blitz2-ab3`, next to the unchanged
  `blitz2`. It has a BB2 mode (behaves exactly as `blitz2`) and an AB3
  mode, chosen per compiled program.
- A command line compile modelled on AmiBlitz3's CLI (`-s SOURCE`,
  `-e EXE`, `-r`, plus an option to force the mode), without TED.
- Detection of AB3 sources: text with AB3's embedded `; XTRA` header,
  a file named `*.ab3`, a tokenised source with an AB3-format `.xtra`,
  or the option; anything else is BB2.
- In AB3 mode: text sources and their text includes are tokenised with
  AB3's token table (AB3's compiler tokens plus the tokens of AB3's
  libraries); a tokenised include of the other mode is rejected with an
  error.
- In AB3 mode: AmiBlitz3 v3.14.0's library files are loaded as AB3
  ships them (the four BigLibs in AB3's order, the flat userlibs
  directory), the float power uses AB3's math library number, and
  residents named in the embedded header are found relative to AB3's
  residents directory.
- Tests: host tests for detection and the command line; emulator tests
  that compile and run AB3 test programs with `blitz2-ab3`, compared
  with expected output and with AmiBlitz3's own compiler; a check that
  `blitz2-ab3` gives the same executables as `blitz2` for BB2 sources.
  AmiBlitz3's files are fetched at the pinned commit (SHA-256), not
  checked in.
- Not in this change: AB3 language extensions (`SYNTAX`, `OPTIMIZE`,
  optional parameters, escapes, FPU, 68020 – phases G2–G7), the runtime
  debugger in AB3 mode, TED, porting AB3's libraries to C.

## Capabilities

### New Capabilities

- `ab3-compat/source-mode`: which mode (BB2 or AB3) a compiled program
  gets, how AB3 sources are recognised, the scope of the mode, and the
  handling of includes of the other mode.
- `ab3-compat/command-line`: compiling a source from the shell with
  `blitz2-ab3`, its options, results and errors.
- `ab3-compat/libraries`: the library set, token table, resident lookup
  and library conventions used in AB3 mode.
- `ab3-compat/bb2-equivalence`: the guarantees that the default
  `blitz2` stays byte-identical and that `blitz2-ab3` in BB2 mode
  produces the same results as `blitz2`.

### Modified Capabilities

None (no specs exist yet).

## Impact

- `programs/blitz2/` (CMake: the second target; sources: start-up and
  command line, library loading, `mathq`, compiler token table, resident
  names – all behind the build define, so `blitz2` is unchanged).
- A headless editor for the command line, built on the edstub frame
  (`shared/edstub`) and TED's core (`programs/ted/src/core`, tokeniser
  and text loading) with `shared/xtra` for the embedded header.
- `cmake/fetch-originals.cmake` (AmiBlitz3 v3.14.0 files: BigLibs,
  residents, test sources, the `Amiblitz3` binary), tests under
  `tests/` (a new suite for AB3 sources), README and `docs/ab3/`.
- Licence: AmiBlitz3's files are only downloaded for tests and loaded
  at run time; the compiler changes are this repository's own C,
  written after reading AB3's source (Apache-2.0 / GPL-2+, both
  compatible), referenced as `AmiBlitz/AmiBlitz3@hash`.
