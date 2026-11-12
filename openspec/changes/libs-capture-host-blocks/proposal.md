# Proposal

## Why

The two block programs of the library tests (`systest_med`,
`rwe_ridisasmlib`) spend about half of their emulator time formatting and
FNV-hashing their 6.6 million observations on the 68000. A measurement
with throwaway patches (user decision of 2026-11-12) showed that writing
the raw observations and formatting and hashing them on the host halves
both programs (systest_med 53.5 s -> 26.3 s, rwe_ridisasmlib 67.2 s ->
32.6 s, about 40 % of `libs_emulator`, and the same in every recording
run) and reproduces the committed `BLOCK:N` cases byte for byte. With the
raw observations on the host, a differing block can also be listed at
once and a `lost` inside a block becomes visible to the record.

## What Changes

- In block mode (`cap_blocks(1)`: `systest_med`, `rwe_ridisasmlib`, the
  two `libtest` programs) a test program writes each observation as a
  compact binary record to `NAME.bin` beside `NAME.txt`, and in
  `NAME.txt` only a placeholder line per block at the place where the
  block case belongs. Plain cases (every other program, and
  `not captured` in block mode) stay text as today.
- The judge (`tools/lib/libscmp`, `results`) reads `NAME.bin` with
  `NAME.txt` and produces exactly today's `BLOCK:N` cases from it (format,
  key escaping, value formats, FNV-1a, `first=`); the committed expected
  data do not change.
- `results libs-compare` lists the observations of a differing block (a
  file beside the run's output and its first lines in the report) and,
  given a second run with its `.bin` (`--other`), names the first
  observation in which the two runs differ.
- `results libs-accept` names the first differing observation of a
  non-deterministic block and refuses a `lost` observation inside a block
  (the limit stated in design D5/D6 of `libs-tests-capture` goes away).
- `LIBS_CAP_DUMP=BLOCK:N` (and a key of a block observation) is served on
  the host from `NAME.bin` (new subcommand `results libs-dump`); the
  option names (`CAP_DUMP`, the environment variable `LIBS_CAP_DUMP`)
  stay. The test programs no longer dump blocks themselves.
- The `.bin` files (about 300 MB per run) are deleted after a passed
  test or record and kept when it fails (`LIBS_KEEP_BIN` keeps them).

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `testing/library-capture`: block observations are recorded raw and
  hashed by the judge; a differing block is listed (and named to the
  observation against a second run); the record refuses `lost` inside a
  block; the dump of a block is made on the host; the raw files are
  removed after a pass.

## Impact

- `libs/tests/capture.[ch]` (block mode writes `NAME.bin` and placeholder
  lines; no block dump), `libs/tests/host/test_capture_format.cpp`.
- `tools/lib/libscmp` (new reader of `NAME.bin`, merge on load, listing,
  dump, lost check), `tools/results` (`libs-compare --other`,
  `libs-dump`), `tools/tests` (host tests from memory with broken input).
- `libs/tests/emulator.cmake`, `libs/tests/expected.cmake`,
  `libs/tests/CMakeLists.txt` (host dump, deleting `.bin`).
- Docs: `docs/testing.md`, `tests/README.md`, `tools/results/README.md`,
  `site/content/docs/project-structure/library-test-cases.md`.
- No result changes: library files, compiler output and the committed
  expected data stay byte-identical.
