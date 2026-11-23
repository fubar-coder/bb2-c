## 1. The key and the label

- [x] 1.1 `flaky` in `tools/lib/testconf` (non-empty string, only in an `ID.test.toml`) and the label in `tools/lib/testreg`; verify: `test_testconf` (empty description, wrong type, in a `tests.toml`, as an axis) and `testreg_tool` (the label on both sides, once; refusals), also with `-DHOST_SANITIZE=ON`
- [x] 1.2 Docs: `tests/README.md` (key table, "Writing a test source"), `tools/testreg/README.md`; verify: `testconf_readme`, `testreg_readme`

## 2. The marked test

- [ ] 2.1 `tests/bug-orig/compiler/compiler-amigalib-a4a5.test.toml` gets `flaky` (Q-A17); verify: `ctest -N -L flaky` lists its tests
