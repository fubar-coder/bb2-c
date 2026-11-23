# Tasks

## 1. Runner

- [x] 1.1 `tools/lib/testconf`: the validation (design decision 3) and the expansion (decision 2) of `abnormal = ["any", ...]`; the head comment of the alternative ends in `testconf.h`. Verify: host test 1.2 passes; no warning.
- [x] 1.2 Host test in `tools/tests/test_testassert.cpp`: the expansion (names and assertions, with and without extra elements), the ordinary `abnormal` list unchanged, and the broken inputs (a string `"any"`, `["all"]`, `any` under another key or not first, an extra on `rc`, a key `abnormal_after_case` next to the built-in), each naming file and line. Verify: `ctest --preset host`, also with `-DHOST_SANITIZE=ON`.

## 2. Tests

- [ ] 2.1 The 58 tests with the four alternatives rewritten to `abnormal = ["any", EXTRA...]`. Verify: the touched tests give the same verdicts before and after on both sides (originals that flake compared by verdict over repeated runs); `ctest --preset host`.

## 3. Documents

- [ ] 3.1 tests/README.md, "Alternative ends" and "An original that corrupts memory", and the guide wherever the four-alternative block is shown. Verify: no four-alternative block left in the docs (`grep -rn abnormal_after_case`).
