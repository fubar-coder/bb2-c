# Proposal

## Why

The test library of Blitz test programs (change `basic-testlib`) and the
runner that judges them (change `basic-test-runner`) are to be built in
parallel. They meet at a contract: the macro calls an author writes, the
form the runner's preprocessor turns them into, the driver program the
runner generates, the library routines it calls, and the result file the
library writes and the runner reads. As text in one change's design it
would bind neither side; as a spec of its own, archived first, it binds
both, and one codec for the result file keeps writer and reader from
drifting apart. User decision of 2026-11-21 (chat): a change of its own.

## What Changes

- **A spec of the interface**: the source form of blocks and checks, the
  preprocessed form, the macro text, the library routines, the order of
  the driver, the result file (line form, encodings, entry kinds).
- **One codec of the result file** in `shared/basictest/`, compiled by the
  test library (Blitz library conventions) and by the host tools, with
  host tests and deliberately broken input.
- **The macro text** as a file in `shared/basictest/`, which the runner
  puts into every driver.
- The documentation of the format in `tests/README.md`.

## Capabilities

### New Capabilities

- `testing/basic-test-interface`: the contract between Blitz test sources,
  the runner's preprocessor and driver, the test library and the result
  file.

### Modified Capabilities

None.

## Impact

- `shared/basictest/` (new: codec, macro text), its host tests next to it,
  a build check that the codec compiles in the library environment.
- `tests/README.md`.
- Changes `basic-testlib` and `basic-test-runner` refer to this spec instead
  of the "Interface" section of `basic-testlib/design.md`, and start after
  this change.
- No result of the compilers or libraries changes.
