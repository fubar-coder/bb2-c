# Design

## Context

- The session key: `tc_session_key(conf, m, cell)` (`tools/lib/testconf/testmatrix.c:321`,
  commit 77c5f1625) writes `machine`, `rtg`, `timeout`, `hang-after` (an unset
  one as ""), the elements of `uae`, `floppy`, `mount`, and for a `serial`
  or isolated cell the test and cell number. Callers: `run_set`
  (`tests/corpus/runner/caserun.c:2285`), `test_caserun.c`, `test_testmatrix.cpp`.
- The environment's time limit: `[runner] timeout` of `model-env.toml`
  (`BASIC_TEST_TIMEOUT`, 90; `cmake/basic-test-model.cmake`); testreg gets
  the same number as `--timeout-default` and the same file as `--env`.
- `cr_case_alone` (`caserun.c:814`, `caserun.h:247`) needs `tc_config`,
  `tc_matrix` and `tp_source`; testreg already links `lib_testjudge`
  (`tools/lib/testreg/CMakeLists.txt`), which links `testconf` and `testpp`.
- The run directory of fixtures: `rundir_path` (`tests/corpus/runner/rundir.h`):
  `BB2C_TEST_RUN`, else a default the caller gives (the passes:
  `run_dir=`, the build's `Testing/runs/default`).
- The spike and prototype: `~/.local/share/bb2-c-work/analysis-bundling/s0/report.md`,
  `q9/report.md` (pooled verdicts equal to stand-alone in 46 of 46 cells;
  0.8 s + 0.11-0.24 s per cell; a remainder chunk of another size is another
  snapshot configuration).

## Goals / Non-Goals

**Goals:** the key and the pooling rule in `tools/lib`, so the registration
and the runner cannot disagree on a group; the interface of the three
following changes fixed before they start.

**Non-Goals:** the proposal, "Not in scope".

## Decisions

1. **`tc_session_key(conf, m, cell, env)`.** An unset `timeout` is written
   as the environment's `[runner] timeout` (Q12); an environment without it
   keeps "". `hang-after` stays as it is (K1 (c): a test that sets it
   is not bundled, decision 3). The comment drops
   "a time limit left to the environment differs from one set to the same
   value". testreg passes its `tc_env`; the runner its `x->env`.
2. **`tj_case_alone`** is `cr_case_alone` moved to `tools/lib/testjudge`
   with its tests; `caserun.c` calls it.
3. **`tj_bundle_case(conf, m, src, env, case, &cell)`** returns 1 and the
   matrix cell when the case may be a member of a bundle (the ADDED
   requirement): side port, one cell on it, not `tc_cell_isolated`, `serial`
   not "on", no `data` entry, no run from the editor (K2, for now), no
   `hang-after` set by the test's configuration (K1 (c); the 15 tests that
   set it stay alone), the key equal to the
   key of a configuration with only `timeout` set to the environment's value,
   30 or 60 (Q6, "the three plain keys"), and `tj_case_alone` 0 for that
   cell.
4. **`[runner] run-dir`** in `model-env.toml`: `${CMAKE_BINARY_DIR}/Testing/runs/default`.
5. **The spike of G1 (task 1.1): one volume for a bundle.** (b): one
   writable volume `BUNDLE:` for the session, a directory per cell in it
   and `Assign CELLn: BUNDLE:cN` before the cell's part of the script, so the
   emulator configuration (and its snapshot) is the same for any number of
   cells; no padding, a bundle of one case is no special case. If (b) works
   it is the design of `basic-test-bundle-runner` and
   `basic-test-bundle-registration`; else (a): every session of a key has 16
   `CELLn` volumes, the free ones empty, and a group or last chunk of one
   member is not bundled (G1 (a)). The coordinator brings the result into
   both changes (openspec-update-change) before their implementation passes
   the point marked "(spike)".
   What depends on `CELLn:` being a real volume, and the spike checks
   (`tests/corpus/runner/corpus.c`, `caserun.c`):
   - `CORPUS:` becomes an assign to a directory, not a volume root: a path
     `CORPUS:/x` (parent of the root), `Info`/`DiskInfo` of `CORPUS:`, the free
     space a program reads, a requester naming the volume
     ("Please insert volume CELL3") differ; tests that query these must stay
     out of bundles or keep their own volume.
   - `names_volume` (`corpus.c:933`, a source naming a mount) and the mounts
     (`mount_name`) beside it: no name clash with `BUNDLE`.
   - The host side: `cell_complete`, `cr_observe`, `observe_*` read
     `WORK/TAG/vol/out/...`, and `run_session` builds the volume list
     (`caserun.c:1584`); with (b) the host directory of cell n is
     `WORK/bundle/cN`.
   - The serial output goes to the first volume's `out/` (`caserun.c`, `serial_stop`);
     serial cells are not bundled.
   - Isolation is weaker: one cell can reach another's files through
     `BUNDLE:`; today a cell sees only its own volume.
   Note for the follow-up change of K2 (not planned here): in a run from the
   editor the hang check is armed once per session from the first cell's
   run (`hang_after`/`hang_from=run`, `caserun.c` around :342-352,
   `corpus.c:1570-1573`; `editrun` per cell); a shared volume does not change
   that.

## The result of the spike (task 0.1)

**(b) works.** One writable volume `BUNDLE:` per session, the directories
`c0`..`cN-1` in it and `Assign CELLn: BUNDLE:cN` for every cell at the start
of the script (the compile and run parts unchanged: `Assign CORPUS: CELLn:`
before each cell's part; the assign of an assign is the directory). The
emulator configuration and its snapshot are the same for any number of
cells: no padding, a bundle of one case is a plain session.

The spike (no commit; `~/.local/share/bb2-c-work/agent-bundle-key/spike`
and `spike2`, separate build directories): the Q9 prototype
(`corpusrun case --bundle`), once on its own source (9dcd48bef) and once
ported onto this change's code with `tj_bundle_case` as the filter, plus a
switch for (b) in `run_session` and `corpus_session_start`. The staged
volume of each cell (`WORK/TAG/vol`) is renamed to
`WORK_FIRST/bundle/cN` before the session and back after it, so
`cell_complete`, `cr_observe` and `observe_*` read the paths they read
today.

- Snapshot: a first save with 4 cells, then 16, 4, 1 and 16 cells all
  `snapshot=restored` (with a volume per cell, 16 volumes were another
  configuration: `snapshot=single`). In the first-set run below, 48
  sessions of 2 to 16 cells: 7 `first-save` (one per slot and time
  limit), 1 `single`, 40 `restored`.
- Times (restored, load about 2): 1 cell 1.00 s, 4 cells 1.21 s, 16 cells
  2.41-2.51 s; with a volume per cell the same sets took the same (16:
  2.42-2.50 s, the S0 set of 14 cells 4.02 s either way).
- Verdicts: the Q9 sets (pool16 16, risk 16 with FAILED, BROKEN and
  REPRODUCED cells, S0 14) equal in (b), with a volume per cell and alone
  (46 of 46); the hanging case in the middle of 5 ends the session after
  9.97 s, the cells behind it run again, all 5 verdicts as alone (the hang
  check armed from `c0`'s run file, below).
- The first set: of the 1085 registered case tests of the side port,
  `tj_bundle_case` admits 580 (550 sources; keys of the environment's 90 s,
  30 s and 60 s). All 580 ran in bundles (7 runner processes, chunks of 16
  per key, 48 sessions, no session ended early) and once alone with the
  build's `corpusrun` and their ctest command line: 580 of 580 verdicts
  equal (573 PASSED, 7 REPRODUCED). Session time 147 s bundled against
  708 s alone (4.8x).

What depends on `CELLn:` being a real volume (the list of decision 5,
checked):

- `CORPUS:/`, `Info`/`DiskInfo`, free space, the volume name: no source of
  the first set uses `CORPUS:/` or `DiskInfo`/`Info`; the 13 sources a
  search for paths, locks and names finds (`ProgPath$`/`ProgDir$`,
  `GetPathName`, `DeviceName`, `ParPath$`, `DataTypeOpen("CORPUS:")`, a
  drawer as a module, `riapplib-appfile-unlock`, `ridoslib-*`,
  `rimisclib-progpath-*`, a chunk named `info`) give the same verdicts
  bundled: the tests check the part after the `:` or no path
  (tests/README.md, "Preconditions of the program's environment"). What a
  program sees changes: a path in `CORPUS:` is `BUNDLE:cN/...`, not
  `CELLn:...`; the README's sentence on `CELL0:` needs that for bundles. The
  `AvailMem` of the leak tests is per case and not affected.
- `names_volume` and `mount`: no test sets `mount`, and a cell with `mount`
  has another key and is not bundled; `BUNDLE` can clash with no mount of a
  bundle member. The runner should still refuse a mount named `BUNDLE`.
- The host side: with the rename the observations are unchanged; staging
  directly into `WORK/bundle/cN` is the other way (then `cell_complete`,
  `cr_observe`, `observe_*` and `run_session`'s volume list need the
  bundle's path).
- Found beyond the list: the hang check's arm file. `corpus_session_start`
  takes the arm file (`out/FILE.run`, `out/gate.run`, `out/FILE.con`) below
  the first volume's host directory (`vols[0].host_dir`); with (b) that is
  `WORK/bundle`, so the arm path must be `WORK/bundle/c0/out/...`. Without
  that the hang check never arms (a hang costs the full time limit).
- The serial output (`serial_stop` into the first volume's `out/`): serial
  cells are not bundled.
- Isolation: a cell can reach the others' directories through `BUNDLE:`;
  no source of the first set does.

## The fixed interface (for the three following changes)

Fixed here so that `basic-test-bundle-runner` (R), `basic-test-bundle-registration`
(G) and `basic-test-bundle-costs` (C) can be built in parallel. A change of
it is a change of this design (openspec-update-change) before any of the
three relies on the new form.

- **I1 The bundle test** (G writes it, R parses it):
  `corpusrun case --env ENV [--require-deviation] --bundle NAME --member TEST SOURCE CASE ...`
  (`--member` repeated, three values: the case's ctest name, its source as an
  absolute path, the case). The members are in the order of the chunk; a
  session holds at most 16 cells (Q4), with one emulator configuration per
  key (Q11; decision 5 and the spike: one volume `BUNDLE:`, no padding).
- **I2 The case test** (G writes it, R parses it): the command of today plus
  `--test TEST --from-bundle NAME`. Without `--from-bundle` nothing changes.
- **I3 The bundle test's name**: `PREFIX + "bundle_" + N` (N from 1 in the
  order of the groups, then the chunks); no `__`, so no name of a case test
  (`PREFIX_ID__CASE`) can equal it; `treg_check_names` covers it.
- **I4 The store**: the directory `RUN/verdicts/`, RUN = `rundir_path`
  with the environment's `run-dir` as default. One file per stored case,
  named TEST. Written as `TEST.new-PID`, then renamed to `TEST` (atomic).
  A case test claims it by renaming `TEST` to `TEST.claim-PID`, reads it and
  removes it; the directory is removed when empty (rundir's "a passed run
  leaves nothing"). The bundle removes `TEST` and stale `TEST.*` of its
  members (no live PID) at its start. Two claimants: one gets the file, the
  other finds none and runs alone.
- **I5 The entry**: text lines, LF:
  `bundle-verdict v=1`, `key=HEX64`, `bundle=NAME`, `status=N` (the case
  test's exit status, `cr_side_status`), then the case's report block as the
  run of the case alone prints it (`judge_case`'s `==` line to the verdict
  line). Only PASSED and REPRODUCED from a cell that is `cr_cell_clean`
  (Q3; Q13: also when a later cell ended the session).
- **I6 The key**: sha256 (`tools/lib/sha256`) over a text of NAME=LEN:VALUE
  lines: `v=1`, TEST, the side, the case, the bytes of the source, of every
  configuration file `tc_resolve` read (in its order), of every file below
  `ID.test/`, of the environment file, of every existing file a `[runner]`
  value of the environment names, and of the running `corpusrun`
  (`/proc/self/exe`); plus the emulator inputs of question K3. The bundle
  and the case test compute it with the same function, from the
  configuration, not from a staged volume (the case test stages nothing).
- **I7 A served case** prints the stored block, a line
  `corpusrun case: verdict from the bundle NAME` and
  `@served v=1 test=TEST bundle=NAME`, no `@stat` line, and exits with the
  stored status. A missing, foreign-key or malformed entry: the case runs
  alone as today; a malformed one is named in the output.
- **I8 The bundle's `@stat` lines** carry `test=NAME` (the bundle) and
  `config=sN` (its session); the log names per cell `bundle cell cK: TEST`.
- **I9 The bundle's exit status** (Q5: it passes when it ran through): 0 when
  it ran through, whatever the members' verdicts; 77 when skipped before any
  session (no emulator, a file missing); 2 for its own error (store not
  writable, out of memory, a bad command line). A member it cannot prepare is
  left unstored and named; that is no error of the bundle.

## Questions (for the coordinator; the user's where marked)

- **K1, K2, G1, R1** are answered ("K2, R1, Q17"; "K1, K2 follow-up, G1"):
  K1 (c), decision 3; K2 for now out, a follow-up change; G1 the spike,
  decision 5; R1 the store is no matter of the build rule.
- **K3 (Plan, mine).** The emulator in the key: the Amiberry binary, the ROM
  and the hardfiles by path, size and modification time (their contents are
  large; the snapshot key of `amiga_emulation` hashes the same inputs). A task
  of change R reads whether `amiga_emulation` offers that key without a session
  and uses it if so.
  `Point: an input missing from the key serves an old verdict once (entries are removed on read) — L2 Possible / I2 Medium / E2 Reasoned → Plan`.
- **K4 (Plan, mine).** The store below the run directory (I4) rather than below
  `work=` (`model-runs/`): the run directory is where a test hands data to
  another (rundir.h) and `BB2C_TEST_RUN` separates two ctest runs.
  `Point: store place — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor`.

## Risks / Trade-offs

- `Point: within one test, cells without timeout and with timeout = 90 now share a session — a test that relied on two sessions — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor` (the host tests of `tc_session_key`; `ctest -L emulator -R 'blitz2_|regression_'` shows a changed verdict).

## Order

This change first (with the spike's result); then R, G and C in parallel. G's end-to-end task needs R
merged; C needs only I7/I8.
