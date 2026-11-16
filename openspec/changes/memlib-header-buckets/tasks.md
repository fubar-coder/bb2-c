# Tasks

## 1. Analysis (before any code)

- [ ] 1.1 Find every user of memlib's list chain and table outside memlib: acidlibs, RWE userlibs, the CD's third-party libraries, AB3 libraries with inline assembler, and programs of the corpus with inline assembler. Check that none frees, allocates or keeps the address of a list header itself, and that memlib is not documented or used as interrupt-safe (SetInt). Confirm that AB3 mode does not use this repository's memlib. Write the result to `~/.local/share/bb2-c-work/analyse-memlib-buckets.md`; stop and report if a caller would break.
- [ ] 1.2 Confirm the LIFO claim: no path other than freelast (and the finit/$C006 loop over it) removes a header, including err1/err2, SetErr/ErrFail, the debugger's continue and `End` inside a procedure. Record it in the same report.

## 2. Design details

- [ ] 2.1 Write the bucket state machine into design.md: the fields, the fast paths of newmem/freelast, when a bucket is allocated, when the spare is kept or freed, finit, and err1. Count its static cycles on the fast paths against today's newmem (214) and freelast (360). It must not be slower without exec.

## 3. Tests first

- [ ] 3.1 Library cases in `libs/tests/systest.c` for newmem/freelast at the bucket transitions: the first entry, a full bucket, a new bucket, the spare kept, the spare freed, and depth back to 1. Check the results and the KEEPS cases. Record them from the original with libs-capture (the original has no buckets, so the cases observe results and registers, not the bucket state); libs_emulator_originals passes.
- [x] 3.2 A leak test (bug-orig or library test with inject's AllocMem/FreeMem counts): after the end of a program, all memory memlib took is freed, also after a runtime error at recursion depth 10 and after SetErr. It passes on main before the change.
- [x] 3.3 perf lines: `bproc` stays, plus a recursive procedure (depth 20, back to 0, repeated) with its original value from perf-original.

## 4. Implementation

- [ ] 4.1 Buckets in memlib (newmem, freelast, codeinit/codefinit, err1) with k = 4; the tests of 3.1-3.3 pass; libs/tests/expected unchanged; prev_results names memlib with its variants, blitz2/acidlibs and the executables.
- [ ] 4.2 Measure k = 4 against k = 8 on bproc and the recursion line; keep the better one (or 4 when equal) and set the budgets by the README rule.
- [ ] 4.3 `docs/libs/deviations.md` row of kind B (headers in buckets, up to two empty buckets stay allocated), and the memlib paragraph of `docs/libs/cmdlibs-port.md`.

## 5. Acceptance

- [ ] 5.1 `ctest --preset all`, `bug-orig`, `originals` and `perf` (three runs) pass with the intended differences named. fix-scope reports only the added cases.
