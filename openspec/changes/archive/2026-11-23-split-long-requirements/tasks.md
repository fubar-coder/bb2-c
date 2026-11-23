## 1. Split

- [x] 1.1 The delta of `testing/basic-test-interface` splits "Entries of the result file"; verify: `openspec validate split-long-requirements --strict`
- [x] 1.2 The delta of `bug-orig-basic-tests` (`testing/bug-orig`) splits the two long requirements; verify: `openspec validate bug-orig-basic-tests --strict`
- [x] 1.3 `openspec validate basic-test-local-data --strict` still passes
