# Proposal

## Why

Bundled runs of the tests of the BASIC model (round tb, item O2: option a1,
several sources' cells pooled into one emulator session, with ctest form b3)
are split into four changes. Three of them (the runner's bundle mode, the
registration of bundles, the COST rules) can be built in parallel only if
what they share is fixed and merged first: the session key both the
registration (testreg) and the runner group by, the rule which cases may be
pooled, and the command lines and files through which a bundle test hands a
case's verdict to the case's own test. This change builds the first two and
fixes the third as the interface of the other three (design.md, "The fixed
interface").

## Decisions of the user

Quoted from `~/.local/share/bb2-c-work/decisions/rounds/tb/decisions.md`
(2026-11-23) and `archive/2026-11-23-tr/decisions.md`:

- "O1-O3 of the handoff answered", O2: "OK" — "the spike first, then planning
  a1 with ctest form b3 (planning agent)".
- "Q3-Q9: the S0 questions as recommended": "Q3-Q9: Wie empfohlen" — among
  them Q6 "first the single-port-cell sources of the three plain keys
  (default, timeout=30, timeout=60), multi-cell/isolate/serial stay"; Q8 the
  key moved to tools/lib as a change of its own (done, 77c5f1625).
- "Q10-Q13: bundling details as recommended": "Q10-Q13: Wie empfohlen" —
  among them Q12 "the environment's default timeout resolved into the
  session key".
- "Bundling answers; hang-after stays at 8 s", quote (4): "Vorerst nicht"
  (the originals side is not bundled).
- "K2, R1, Q17" (K2): "Ok, wir lassen das **vorläufig** außen vor. Aber
  eigentlich müsste das doch funktionieren?"; (R1): "Es betrifft nur das
  Build. Deine Lesart ist richtig."
- "K1, K2 follow-up, G1": "Zu K1: Ok, Option (c)" — "Zu K2: Ok,
  Folge-Change" — "Zu G1: Ok, wie empfohlen falls möglich (b) ... und löst
  das nicht auch das editor-run-Problem von K2?"

## What Changes

- **The session key resolves the environment's time limit** (Q12): a cell
  without `timeout` gets the key of a cell with `timeout` set to the
  environment's `[runner] timeout`. `tc_session_key` takes the environment;
  the runner and testreg pass the same `model-env.toml`.
- **A test that sets `hang-after` itself is not bundled** (K1 (c));
  `hang-after` is no part of the key's resolution.
- **A spike** checks one volume for the whole bundle with a directory and
  a `CELLn:` assign per cell (G1 (b)); its result decides between that and
  padding with empty volumes (design decision 5).
- **Whether a case may be pooled** becomes a function of `tools/lib`
  (`testjudge`), used by testreg and by the runner: the runner's
  `cr_case_alone` moves there unchanged (as `tj_case_alone`), and a new
  predicate names the cases of the first set (Q6).
- **The environment names the run directory** (`[runner] run-dir`, the
  build's `Testing/runs/default`; `BB2C_TEST_RUN` wins, as for the passes):
  the place of the verdict store.
- **The fixed interface** of the three following changes, in design.md:
  the bundle's command line, the case test's added options, the store's
  place, entry format, claim protocol and key inputs, the `@served` line.
- No result changes (rule 3): no compiler, library or TED output is
  touched. Within one test, a cell without `timeout` and a cell with the
  environment's value now share a session; the tests' verdicts stay.

## Not in scope

- The bundle mode of the runner and the store (change
  `basic-test-bundle-runner`), the registration of bundles (change
  `basic-test-bundle-registration`), the COST rules (change
  `basic-test-bundle-costs`).
- Cells with test-local data and runs from the editor (K2: "vorläufig"):
  a follow-up change after the first set (a TODO line, task 4.1).
- The originals side ("Vorerst nicht"); multi-cell, isolate and serial
  sources (Q6); one cell per source with the driver of all cases (Q10:
  "later as an optimisation"); `corpus_exprmatrix` (it joins after its
  conversion, archive entry "Bundling answers", quote (5)).

## Open questions

(none: K1, K2, G1 and R1 are answered; the spike of design decision 5
decides G1's form)

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `testing/basic-test-runner`: a cell's session key with the environment's
  time limit resolved.

## Impact

- `tools/lib/testconf/testconf.h`, `testmatrix.c` (`tc_session_key` with the
  environment); `tools/tests/test_testmatrix.cpp`.
- `tools/lib/testjudge` (`tj_case_alone` moved from
  `tests/corpus/runner/caserun.c`, `tj_bundle_case` new); its tests;
  `tests/corpus/runner/caserun.[ch]`, `test_caserun.c` (callers).
- The spike (task 1.1; no commit; a separate build directory).
- `cmake/basic-test-model.cmake` (`[runner] run-dir` in the environment
  file).
- Docs: `tests/corpus/runner/README.md`, the comment of `tc_session_key`;
  `TODO.md` (the follow-up change of K2).
- Overlaps: the three bundle changes build on it (order: this one first);
  `tests/corpus/runner/caserun.c` is edited again by
  `basic-test-bundle-runner`, `cmake/basic-test-model.cmake` again by
  `basic-test-bundle-registration`; `TODO.md` also by
  `basic-test-bundle-registration` and `coverage-ted-libs`.
