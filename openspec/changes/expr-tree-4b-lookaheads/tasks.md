# Tasks

After `expr-tree-4a-emit-after-build`. "The check": `expr-tree-1-typing/design.md`, "The check of every byte-identical step" (after archive: its copy in `openspec/changes/archive/`).

## 1. Tests first

- [ ] 1.1 Cases (coverage build) for unreached forms: nested calls 8 deep with commas inside string literals and brackets (`countpees`), each intrinsic as the whole right side and not, assignments with relocated left sides (global NewType arrays, string arrays, `\` paths), `Select` on an array element; verify: pass on main, in `compiler_prev`.

## 2. Implementation

- [ ] 2.1 Parameter counts from the node (decision 1); verify: the check.
- [ ] 2.2 Intrinsics as node properties (decision 2), state machine and look-aheads removed; verify: the check, `ctest --preset oracle`, spec `compiler/intrinsics` scenarios.
- [ ] 2.3 `dolet2` and `doselect` without the copy (decision 3); verify: the check.
- [ ] 2.4 Compile time of `corpus_cd` before and after (information); `docs/blitz2/expression-tree.md`, comments, `tools/testinfo/rules.txt`; verify: links, selection test.

## 3. Commit

- [ ] 3.1 Tests first; then one rework commit per task 2.1-2.3; verify: each passes the check.
