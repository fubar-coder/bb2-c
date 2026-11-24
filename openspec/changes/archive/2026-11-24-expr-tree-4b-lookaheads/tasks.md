# Tasks

After `expr-tree-4a-emit-after-build`. "The check": `openspec/changes/archive/2026-11-24-expr-tree-1-typing/design.md`, "The check of every byte-identical step".

## 1. Tests first

- [x] 1.1 Cases (coverage build) for unreached forms: arrays with a later index whose narrowing applies and one where it does not, nested calls 8 deep with commas inside string literals and brackets (`countpees`), each intrinsic as the whole right side and not, assignments with relocated left sides (global NewType arrays, string arrays, `\` paths), `Select` on an array element; verify: pass on main, in `compiler_prev`.

## 2. Implementation

- [x] 2.1 Parameter counts (decision 1): `countpees` stays, nothing to change (the user's answer); verify: the reason in design.md, Findings.
- [x] 2.2 Intrinsics (decision 2): the state machine stays, nothing to change (the user's answer); verify: the reason in design.md, Findings.
- [x] 2.3 `doselect` without the copy (decision 3); verify: the check.
- [x] 2.6 `dolet2` without the copy as the fix of #1212 (decision 3): the right side first, then the address code; one fix commit by `docs/development.md`, "Fixing an original bug" (`fixed-orig = 1212`, kind-F row in `docs/blitz2/deviations.md`, `Fixes #1212`, fix-scope); `pushstart`, `pushdo`, `pushput` removed where nothing else uses them; verify: `compiler-let-left-a6` FIXED, the check with `-DPREV_INTENDED` (the changed executables named), the cases of 1.1.
- [x] 2.4 A later array index narrowed on the node (decision 4): `arrevalchk`'s actions `idxmark` and `idxnarrow` no longer patch written code; verify: the check, the cases of 1.1.
- [x] 2.5 Compile time of `corpus_cd` before and after (information); `docs/blitz2/expression-tree.md`, comments, `tools/testinfo/rules.txt`; verify: links, selection test.

## 3. Commit

- [x] 3.1 Tests first; then one rework commit per task 2.3-2.4, the fix commit of 2.6; verify: each passes the check (2.6 with its intended changes).
