## Why

The analysis of 2026-11-17 (`analyse-test-speed.md`) found where the test runs
wait. The `all` preset is bound by emulator slots while `jobs: 4` keeps most
of the machine's 10 slots idle. 77 % of the emulator time of `bug-orig` is
sessions that wait out their time limit. Every fixed TED bug-orig test spends
about 8.5 of its 10.7 s on the harness gates. `ctest --preset host` takes 53 s
of which 42 s is one test that sleeps, and an unfiltered `libs-capture` runs
its two passes one after the other.

## What Changes

- **Jobs and slots are separate.** The test presets use the CPU count as ctest
  jobs (presets version 11, `"jobs": ""`; CMake 4.3 is the minimum now). The
  slot limit of `tools/lib/amiga_emulation` defaults to 60 % of the online
  CPUs (rounded, at least 1, at most `AMIGA_EMULATION_SLOTS_LIMIT`) instead of
  4; the `max` file and `AMIGA_EMULATION_MAX` still override. The `perf`
  preset keeps its 8 jobs.
- **A hang check for the program run of the normal bug-orig run.** The
  silence counts from the first program run (`hang_arm` of the emulation
  library, pass key `hang_from=run` of the corpus runner): a compile that is
  silent for long on a loaded host never counts. A program that hangs or
  crashes into the Software Failure alert ends its session after
  `BUG_ORIG_RUN_HANG_AFTER` seconds of silence instead of at the session limit.
- **The TED gates once per ctest run.** Gate 0 and gate D run in a fixture
  (`ted_gates`, `tedab run` with `gates_only=1`); the TED bug-orig tests (open,
  fixed, `_originals`, the compare tests) stay single ctest entries, require
  the fixture and leave the gates out of their sessions (`gates_from=`). A
  failing gate fails the fixture and the tests do not run.
- **`libs-capture` runs its two passes at the same time.**
- **The emulation library's supervisor test leaves the `host` preset.**
  `amiga_emulation_supervisor` (42 s, its limits are real seconds) runs with
  `host_emulator`, so in `all`, `emulator` and `coverage`.

Out of scope: warp mode (unchanged everywhere), a split of `libs_emulator`.

## Impact

Specs `testing/test-runs` (new), `testing/bug-orig`, `testing/library-capture`.
Code: `tools/lib/amiga_emulation`, `tests/corpus/runner`, `tests/ted-ab/runner`,
`tests/bug-orig/CMakeLists.txt`, `libs/tests/expected.cmake`,
`tools/tests/CMakeLists.txt`, `CMakePresets.json`; documentation
`docs/testing.md`, `tests/README.md`, `AMIGA-EMULATION-SETUP.md`.
