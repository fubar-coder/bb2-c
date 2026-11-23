# Design

## Context

- The interface this change implements: `basic-test-bundle-key`, design.md,
  "The fixed interface" (I1, I2, I4-I9) and its key and pooling functions.
- The prototype (reference only): `~/.local/share/bb2-c-work/analysis-bundling/q9/src/tests/corpus/runner/caserun.c`
  from :2835 (`caserun_bundle`): one `ctx` per source, a set of one case per
  case, `run_set` split by a phase flag (prepare / sessions / observe), cells
  pooled by key in chunks. What it did not build (q9 report, item 7): the
  store, the exit status, padding, the 77-skip per case, testreg.
- Today: `run_set` (`caserun.c:2215`) = driver, tokenise, a pass per cell,
  the session loop (cells of `c[first].key`, again for those a session did
  not reach), observe; `run_session` (`:1567`) takes any cells, takes the
  session parameters from `cs[0]`; `session_script` compiles all cells, then
  runs all; `judge_case` (`:2341`); `runwork_open`/`runwork_finish`
  (`runwork.h`); `statline_session` takes the work base as the test name.
- Snapshots: one configuration per number of `CELLn` volumes; the first use
  of a configuration on a slot costs a first save (+2.3 s); `SNAP_KEEP` 24
  per slot (`amiga_emulation.h:247`) (q9 report, item 2).

## Goals / Non-Goals

**Goals:** the bundle mode and the served mode, as the spec delta says; no
change of a stand-alone run.

**Non-Goals:** registration, COST, other sides and keys (proposal).

## Decisions

Decisions 3 and 4 and the tasks marked "(spike)" depend on the spike of
`basic-test-bundle-key` (design decision 5, G1): **(b)** one volume
`BUNDLE:` with a directory and a `CELLn:` assign per cell, no padding; or
**(a)** 16 `CELLn` volumes per session, padded. The coordinator updates
this design with the result before those tasks start.

1. **Phases.** `run_set` is split into `set_prepare` (driver, tokenise,
   passes, keys), the session loop over a list of cells of any sets, and
   `set_observe`. The stand-alone run calls the three in a row (no change of
   its behaviour); the bundle prepares every member's set, runs the pooled
   loop, observes and judges each member with its own `ctx`.
2. **Pooling.** The cells of all members are grouped by key in the members'
   order and cut into chunks of 16 (Q4, Q11). The registration gives a
   bundle members of one key (change G); the runner still groups by key, so a
   mixed bundle stays correct (more sessions).
3. **One emulator configuration per key (spike).**
   (b): `run_session` gets a bundle form: one volume `BUNDLE:` with the host
   directory `WORK/NAME/run-X/cells`, a directory `cN` per cell, and the
   session script assigns `CELLn: BUNDLE:cN` before `Assign CORPUS: CELLn:`;
   the host paths of a cell (`cell_complete`, `cr_observe`, the observe
   functions; today `WORK/TAG/vol`) become `cells/cN`. The stand-alone run
   keeps its `CELLn` volumes. Where a test depends on `CORPUS:` being a
   volume root (the list of the key design, decision 5), the spike names
   it, and `tj_bundle_case` refuses it if the list is not empty.
   (a): a pad cell is a `CELLn` volume with an empty directory and no line in
   the session script; never observed, never ends a session. Every session
   of a bundle, also a rerun behind an unclean end, has 16 volumes.
4. **Session budget.** A bundle starts at most 3 sessions per chunk (the
   first and two after unclean ends); cells not reached then are left to
   their case tests. This bounds the time the case tests wait (they DEPEND on
   the bundle) and fixes the bundle's TIMEOUT for change G: 3 x the key's
   limit + 60 + `EMULATION_QUEUE_WAIT` per chunk.
5. **Work directories.** The bundle opens one runwork below `WORK/NAME`;
   each member's set gets `ID__CASE/` in it. Exit 0 removes it
   (`runwork_finish`); a member left to its case test keeps no files there
   (its case test writes its own).
6. **Per-member skip.** `missing()` of a member's cell (a file of the cell
   missing) leaves that member unstored; only when no member can run is the
   bundle 77. A member whose source no longer preprocesses, or whose case
   is no longer bundleable (`tj_bundle_case`), is left unstored and named.
7. **Store and key** in `tests/corpus/runner/verdicts.[ch]`:
   `vstore_key` (I6), `vstore_put` (`.new-PID`, rename), `vstore_claim`
   (rename to `.claim-PID`, read, check `v=1` and the key, unlink),
   `vstore_clear` (the bundle's members at its start). The store directory:
   `rundir_path(tc_env_runner(env, "run-dir"))` + `/verdicts`.
8. **Served mode.** With `--from-bundle`, `caserun` resolves the
   configuration, computes the key and claims before `runwork_open`; served:
   print, `@served`, exit with the status (I7). Else it goes on as today.
9. **Report.** The bundle prints per member its block (marked with
   `(bundle NAME)`) and a line `TEST: VERDICT stored` or `TEST: VERDICT not
   stored (REASON)`, then `NAME: N members, M stored`.

### Planning choices (mine)

- `Point: the session budget of 3 per chunk — a chunk with several hanging members leaves more to the case tests, which then run alone (slower, not wrong) — L2 Possible / I1 Low / E2 Reasoned → Monitor`.
- `Point: (b) one volume for the bundle — a cell can reach another cell's files through BUNDLE:, which separate volumes prevent today — L2 Possible / I1 Low / E2 Reasoned → Monitor` (no cell of the first set names `BUNDLE:`; the comparison run of the registration change shows a difference).
- `Point: (a) pad cells as empty volumes, only if the spike falls back — Amiberry or the waiter might treat an empty volume differently in the snapshot key — L2 Possible / I1 Low / E1 Assumption → Monitor` (the spike measures the restore).
- `Point: a malformed entry is reported and the case runs alone, not an error — L1 Unlikely / I1 Low / E2 Reasoned → Monitor`.
- `Point: the key computed from the configuration, not the staged volume — a file the cell's pass stages that no configuration or [runner] value names (e.g. a placeholder directory) is missing from it — L2 Possible / I2 Medium / E2 Reasoned → Plan` (task 2.4 lists what `cr_cell_pass` stages for the first set and adds the placeholders a member's configuration uses).

## Risks / Trade-offs

- `Point: a false pass from leftovers of an earlier cell of the session (RAM:, assigns, screens, memory) — L1 Unlikely / I3 Observable / E2 Reasoned → Plan`: bounded by the first set (one port cell, no whole-program query, no run alone), storing only clean PASSED/REPRODUCED, the comparison run of change G (bundled against `BASIC_TEST_BUNDLE=OFF`, verdict per case) and the switch. Prototype: 46 of 46 pooled verdicts equal (q9 report).
- `Point: a verdict served across ctest runs (the bundle ran, the case test did not) — the entry is fresh for the same inputs, so it is the verdict of the same run once — L2 Possible / I1 Low / E2 Reasoned → Monitor`.

## Order

After `basic-test-bundle-key`; parallel with G and C. G's end-to-end task
needs this one merged.
