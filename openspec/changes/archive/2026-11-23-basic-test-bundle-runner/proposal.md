# Proposal

## Why

A test of the BASIC model runs each case in an emulator session of its own:
about 1 s per case, against about 0.1-0.24 s per case when the cells of
several sources share one session (prototype,
`~/.local/share/bb2-c-work/analysis-bundling/q9/report.md`: 3.7x-6.4x less
session time, every pooled verdict equal to its stand-alone verdict). The
user's goal: "Ein Test nach der neuen Methodik sollte im Idealfall nicht
langsamer sein als die alten Tests. Wenn das Bündeln den Performance-Nachteil
mitigiert, dann ist das Bündeln natürlich ein Vorteil über den man nachdenken
sollte" (archive/2026-11-23-tr/decisions.md, "Bundling answers; hang-after
stays at 8 s", quote (2)). This change gives the runner the bundle mode and
the verdict store of option a1 with ctest form b3 ("O1-O3 of the handoff
answered", O2: "OK"); a starter of several sources is a mode of corpusrun in
C (quote (3) of the same entry, proposed; CLAUDE.md rule 6).

## Decisions of the user

`rounds/tb/decisions.md`, "Q3-Q9: the S0 questions as recommended" ("Q3-Q9:
Wie empfohlen") and "Q10-Q13: bundling details as recommended" ("Q10-Q13:
Wie empfohlen"), as their Rule lines read: Q3 store PASSED and REPRODUCED
from a clean shared run; Q4 chunks of 16 cells per session; Q5 the bundle
test passes when it ran through, verdicts only from the case tests; Q6 the
first set; Q10 one cell per case first; Q11 chunks of exactly 16 cells, the
remainder padded with empty cells; Q13 a cell judged complete from a session
that a later cell ended may be stored. The originals side is not bundled
("Vorerst nicht"). "K1, K2 follow-up, G1": "Zu G1: Ok, wie empfohlen falls
möglich (b) ..." — one volume for the bundle with a `CELLn:` assign per cell;
the spike of `basic-test-bundle-key` (its design, "The result of the
spike") showed it works, so there is no padding.

## What Changes

- **`corpusrun case --bundle NAME --member TEST SOURCE CASE ...`**: one
  context per member's source, one set of one case per member, the cells
  pooled by `tc_session_key` into sessions of at most 16 cells with one
  emulator configuration (one volume `BUNDLE:` for the bundle, a directory
  and a `CELLn:` assign per cell, no padding),
  observed and judged per member as the run of the case alone judges it;
  the verdicts PASSED and REPRODUCED of a clean cell stored for the case
  test (interface I1, I4-I6, I8, I9 of `basic-test-bundle-key`).
- **`corpusrun case ... --test TEST --from-bundle NAME`**: a fresh entry of
  the store is claimed, printed and its status returned (I2, I7); else the
  case runs alone as today.
- A session budget per bundle (design decision 4).
- No result changes (rule 3): the compilers, libraries and TED are not
  touched; nothing registers a bundle yet (change
  `basic-test-bundle-registration`), so no ctest run changes.

## Not in scope

- Cells with `[data]` and runs from the editor (K2: a follow-up change).
- Registration, DEPENDS, labels, the CMake switch (`basic-test-bundle-registration`);
  COST (`basic-test-bundle-costs`); the originals side; multi-cell, isolate,
  serial sources; one cell per source (Q10 later); `corpus_exprmatrix`.

## Open questions

- None of its own; it builds on the interface of `basic-test-bundle-key`
  (questions K1-K3 there).

## Capabilities

### New Capabilities

- `testing/basic-test-bundles`: bundled runs of cases of several sources in
  shared sessions, the verdict store and the served case.

### Modified Capabilities

(none)

## Impact

- `tests/corpus/runner/caserun.c`, `caserun.h` (bundle mode, store, served
  mode), `cmd_case.cpp` (options), a new `tests/corpus/runner/verdicts.[ch]`
  (store and key), `test_caserun.c` / a new `test_verdicts.c`,
  `tests/corpus/runner/CMakeLists.txt` (an emulator self-test of the bundle).
- Docs: `tests/corpus/runner/README.md`, `tests/README.md` ("Running and
  reading a test").
- Overlaps: `basic-test-bundle-key` edits `caserun.c` before (order);
  `tests/README.md` also by `basic-test-bundle-registration`,
  `basic-test-bundle-costs`, `coverage-ted-libs`.
