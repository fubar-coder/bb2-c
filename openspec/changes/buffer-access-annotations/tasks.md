# Tasks

## 1. Macro header

- [ ] 1.1 Add the freestanding header with `BL_ACCESS` (guarded by `__has_attribute(access)`, no libc include) at a location on the include path of both worlds. Verify: a snippet including it compiles with m68k gcc and host gcc, and with the macro forced to nothing.
- [ ] 1.2 Make the headers of `shared/` and `tools/lib/` able to include it (include-path changes in the CMake files where needed). Verify: clean amiga and host builds without warnings.

## 2. Stage 1: shared/

- [ ] 2.1 Review by hand and annotate the headers of `shared/tokenizer`, `shared/xtra`, `shared/edstub`, `shared/ab3job` and `shared/filereq` (about 42 functions), using the probe's diff only as a candidate list; comment the numbers that are not lengths. Verify: builds without warnings, `prev_results` byte-identical.

## 3. Stage 1: tools/lib/

- [ ] 3.1 Review and annotate the headers of the first half of `tools/lib/` (alphabetical, `ab3job` to `hunkcheck`). Verify: warning-free host build, `ctest --preset host` passes.
- [ ] 3.2 Review and annotate the second half (`libscmp` to `xtra`, including `teststats` and `vasmlisting`), applying the cases of design D5. Verify: as 3.1.
- [ ] 3.3 Convert fixed-size parameters and fitting `[static N]`/VLA cases per D3, declaration and definition agreeing. Verify: a build with `-Warray-parameter=2 -Wvla-parameter` has no warning.
- [ ] 3.4 Run the probe's warning flags once on the amiga and host builds. Verify: only the two known bad-argument tests warn before task 5.1.

## 4. Host sanitizer configuration

- [ ] 4.1 Add `_FORTIFY_SOURCE=3` (with the optimisation level it needs, `-U_FORTIFY_SOURCE` first) to the `HOST_SANITIZE` branch of `host/CMakeLists.txt`, host side only. Verify: `cmake --preset amiga -DHOST_SANITIZE=ON` builds without warnings, `ctest --preset host` passes, and the Amiga compile commands carry no `_FORTIFY_SOURCE`.
- [ ] 4.2 Fix or report any run-time finding of the fortified tests (fix in the same commit and name it; draft an issue text for the coordinator). Verify: the host tests pass under HOST_SANITIZE.

## 5. Tests

- [ ] 5.1 Rework `tools/tests/test_tokenizer.cpp:230` and `tools/tests/test_xtra_ab3_binary.cpp:118` per design D7 (no file-wide pragma). Verify: warning-free build, the tests still check the refusals.
- [ ] 5.2 Add the compile-fail ctest: a snippet with an oversized length against a smaller array must fail with the diagnostic, a correct snippet must compile. Verify: temporarily removing the annotation of the used function makes the test fail (deliberately broken input); restore it.
- [ ] 5.3 Add the HOST_SANITIZE-only run-time test: a run-time length larger than the buffer aborts in an annotated function; skipped (77) without the sanitizer configuration. Verify: passes with `-DHOST_SANITIZE=ON`, skipped otherwise.

## 6. Hardenings

- [ ] 6.1 Add the `static_assert` on the `offsetof` layout for the 10-byte `memcpy` at `programs/blitz2/src/tokens1.c:785`. Verify: blitz2 stays byte-identical to the previous commit; temporarily reordering a field fails the build.
- [ ] 6.2 Make `put_value` in `tools/lib/teststats/statline.c` not write `dst[0]` when `size == 0`, with a unit test for size 0 in the teststats tests. Verify: the test fails before and passes after the change.

## 7. Documentation

- [ ] 7.1 Add the rule (new code with a buffer+length pair is annotated) to `CLAUDE.md` and a section to `docs/development.md`: the macro header, modes, array parameters, the cases of design D5, the FORTIFY configuration, the limits (run-time sizes are not checked at compile time), the stage 2 policy. Verify: text checked against the code, file:line references checked.

## 8. Acceptance

- [ ] 8.1 Clean rebuilds without warnings: amiga, host, host with `HOST_SANITIZE`. Verify: build logs.
- [ ] 8.2 `ctest --preset all` passes, including `prev_results` against a `prev` build (generated code byte-identical). Verify: ctest output.
- [ ] 8.3 `openspec validate buffer-access-annotations` passes; stage 2 stays as stated in design D10. Verify: command output.
