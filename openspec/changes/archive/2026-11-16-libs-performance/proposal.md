## Why

The measurement of 2026-11-15 (`~/.local/share/bb2-c-work/perf-analysis/report.md`:
cycle-exact A500 in Amiberry, CIA timer, both sides compiled by the port's
`blitz2`) shows 100 of 118 measured commands slower with the C libraries
than with the CD's originals, most by a factor of 1.3 to 2.4, Sort by 8.4 and
Long/Quick division by 2.1. The compiler is not the cause: its code costs
the same on both sides. A game that blits, divides, sorts and prints every
frame loses a quarter of a PAL frame or more. The project goal is BB2
compatibility, and a program that ran in time with the original must not
miss its frame with the port.

The measured causes: the register-saving thunk around 537 entries (70 to
118 cycles per call, the whole difference for small routines such as Peek);
gcc's register saves on every path of functions that have a rare call; copy
and shift loops that gcc compiles worse than the original's unrolled
assembler; helpers with stack arguments in the blit core; one generic compare
in Sort.

The thunk analysis (`perf-analysis/thunks.md`) shows that blitz2's generated
code needs only a4, a5, a6, a7 and, for functions, a3 kept across a
user-level command, which is what Mark Sibly's library documentation says
(`archive/cd/ultimate-blitz-basic-2.1/EXTRAS/LIBSDEV/DOCUMENTATION:794-805`).
The repository's rule is stricter (the thunks keep what the original keeps),
and that stricter rule is what costs the cycles.

## What Changes

- **Register rule for user-level statements and functions** of the acidlibs
  (decision of 2026-11-15, "Register rule for user-level entries (final)"):
  the base rule is the LIBSDEV documentation. The default is an alias of the
  C function, without a thunk. Exceptions exist only for known callers
  (third-party libraries, AB3 libraries, CD programs with inline assembler,
  `analyse-thirdparty-regs.md`): Rnd, WBWidth/WBHeight/WBDepth, EMouseX,
  EMouseY, EventCode, EventWindow, RawStatus, InitSound. Where the code keeps
  the needed registers by its nature the entry is an alias with a KEEPS test
  that states those registers (and the upper word of d0 where a caller needs
  it); otherwise a reduced thunk, or an assembler block if the entry is hot.
  Internal tokens and all error checks keep their thunks and contracts.
  A new deviation of kind B in `docs/libs/deviations.md`, with the list of
  exceptions and their callers.
- **The KEEPS judge** of the library tests follows the rule: a4, a5, a6, a7
  always, a3 for functions, plus the stated exceptions. The result and
  change cases (d0, the end of a string result in a3) stay.
- **Speed profile** (`-O2`) for medlib and trackerlib, added to
  `ACIDLIBS_SPEED` (strings1lib and strings2lib were measured, and are faster
  at `-Os` after their copy loops are rewritten, design.md Decision 15).
- **C first, assembler blocks** only for the hot routines where C stays more
  than 1 to 2 % behind the original, in the decided order: the ldivlib loop; the string copy loops of
  strings1lib and strings2lib; about 25 small entries (Peek/Poke,
  Joyx/Joyy/Joyb/Joyr, MouseX/Y, RawStatus, collisions, ShapeWidth/Height/
  Handle, Colour/Locate, QLimit/Rnd, Plot/Point); then the blit core and
  sortlib.
- **Benchmark** `perf`: a test with a cycle budget per command, in its own
  ctest preset `perf`, not part of `all`.
- **Acceptance** changes for this work: library files change on purpose, so
  the byte comparison with the `prev` build does not apply to them; see
  design.md, Decision 9.

Out of scope: error-check behaviour, the compiler, TED, fixing the related
issues listed below, any change of the observable behaviour of a command.

## Capabilities

### New Capabilities
- `libs/register-contract`: which registers a user-level statement, function
  and internal token of the acidlibs keeps, the exceptions for known
  callers, and how the exceptions are tested and documented.
- `libs/performance`: the speed profile, the assembler blocks and the
  requirement that a library change does not slow a command down.
- `testing/performance-budgets`: the benchmark with cycle budgets, its
  ctest preset and what it measures.

### Modified Capabilities
- `testing/library-capture`: the requirement "Kept registers are checked by
  the test program" (the stated set is the rule's set plus the exceptions,
  no longer the original's intersection; for the acidlibs only, the RWE
  userlib test programs keep their sets).

## Related issues (context only, not fixed by this change)

- #1058 (InitSound loses a0, which ElmoreIncLib IncSound relies on): a port
  bug today. The reduced thunk or assembler entry this change requires for
  InitSound is the fix of that issue. Whichever commit makes the change
  carries `Fixes #1058`; the work is done once (design.md, Open Questions).
- #1059 (the flags after AllocMem do not reflect the result): a flag
  contract of banklib `AllocMem`. The new rule gives no entry a flag
  guarantee; AllocMem stays as it is until #1059 is settled.
- #1045 (BBlit/UnBuffer/ShowStencil add the plane size as a signed word):
  its fix must cost no cycle on the hot path. The blit core block must not
  make a free fix harder; it does not fix it.
- #1048 (tokecode builds a jsr/rts stub for routines with error checks also
  without runtime error checking): a hot-path cost as soon as ldivlib or
  lmullib get an error check. Not touched here; the ldivlib block adds none.

## Impact

- Files: `libs/acidlibs/*.c` (aliases, assembler blocks),
  `libs/acidlibs/CMakeLists.txt` (`ACIDLIBS_SPEED`), `libs/include/blitzlib.h`
  if a thunk macro changes, `libs/tests/*` (the KEEPS judge and statements),
  `libs/tests/expected` where a result changes, a new benchmark test and a
  `perf` preset in `CMakePresets.json`, `docs/libs/deviations.md`,
  `tests/README.md` ("Kept registers"), `docs/testing.md`, the website page
  of differences from the original.
- Results: `acidlibs` and individual library files change (byte differences
  against the prev build, named per step with `PREV_INTENDED`). Executables
  the compilers generate change only through the library code linked into
  them (division, string loops, Peek/Poke, blits); their behaviour does not.
- Risk: a third-party or AB3 caller outside the analysed set may rely on a
  register the new rule no longer keeps (design.md, Risks).
