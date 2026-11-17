## 1. Specs

- [x] 1.1 Delta specs `testing/test-runs`, `testing/bug-orig`, `testing/library-capture`. Verify: `openspec validate test-speed`.

## 2. Jobs and slots

- [x] 2.1 CMake 4.3 minimum, presets version 11 and `"jobs": ""` (own commit; the build outputs stay identical). Verify: build, `ctest --preset host`.
- [x] 2.2 Slot default of 60 % of the CPUs (`amiga_emulation_slots_for_cpus`). Verify: `test_amiga_emulation` (table, broken counts).
- [x] 2.3 `ctest --preset all` with the new defaults at 6 slots (16 jobs): no failure, the worst ratio of a finished session to its limit 0.50 (`ted_ab` s004, 59.5 of 120 s); 358 s against 574 s with 4 jobs.

## 3. Hang check of the program run

- [x] 3.1 `params.hang_arm`, pass key `hang_from=run`. Verify: `test_corpus` (parse, refused combinations), `amiga_emulation_hang` (armed by a file).
- [x] 3.2 `BUG_ORIG_RUN_HANG_AFTER` in the bug-orig passes; three tests with a healthy 11 s silence set `; @pass hang_after=30`. Verify: the verdict of all 154 tests of `ctest --preset bug-orig` is the same with and without it.

## 4. TED gates

- [x] 4.1 `gates_only=1` and `gates_from=` in `tedab run`, the session script without the gates. Verify: `test_tedab` (script with and without), `ted_gates_broken`.
- [x] 4.2 The fixture `ted_gates` and its use by the TED bug-orig tests; the sessions keep `tedprobe` (the layout decides #340 in seven tests). Verify: ctest runs the setup once, a broken gate leaves the tests Not Run.

## 5. The rest

- [x] 5.1 `libs-capture`: the two passes at the same time. Verify: `libs_capture_target`.
- [x] 5.2 `amiga_emulation_supervisor` out of the `host` preset. Verify: `ctest --preset host -N`.

## 6. Documentation and measurement

- [x] 6.1 `docs/testing.md`, `tests/README.md`, `AMIGA-EMULATION-SETUP.md`, tool READMEs.
- [x] 6.2 Wall times before and after, report.
