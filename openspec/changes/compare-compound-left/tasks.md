# Tasks

After `expr-tree-4b-lookaheads`. Open question 1 answered before group 2. Fix workflow: `docs/development.md`, "Fixing an original bug".

## 1. Count and tests first

- [ ] 1.1 Prototype both scopes (design decision 2) on a scratch branch; compile the CD (`corpus_cd` with prev = main): count the executables that change and the comparisons whose value changes for (a) and (b); cycles of the changed code for `If w+v>c`, `If q*2>x`, `If -q<x` (68000 timing tables); report in `~/.local/share/bb2-c-work/`; verify: the numbers the open questions need.
- [ ] 1.2 The bug-orig test `compiler-compare-compound-left`: the cases of the spec added (`-q`, the Long constant under Float, the whole-number compound), `.expect` with the expected values, `.orig` re-recorded (`bug-orig-record`); REPRODUCED on main; check that no cell of the expression matrix has a compound or negated left operand of a comparison; verify: `ctest --preset bug-orig -R compound-left` REPRODUCED, `ctest --preset originals -R bug_orig_record_compiler_compare_compound_left`.

## 2. Fix

- [ ] 2.1 The comparison node rule (design decision 1, scope by the answer); `exprtype.c` and its golden table rows for comparisons changed and named; verify: `ctest --preset host -R blitz2_exprtype`.
- [ ] 2.2 `; @fixed #1183`, the test FIXED; `BUG(orig) (#1183)` -> `Fixed (#1183)`; verify: `ctest --preset all -R compound-left` FIXED, `compiler-quick-compare` FIXED, `tools/results fix-scope` against main: only #1183's cases changed.
- [ ] 2.3 Prev comparison: the differing executables are those of 1.1's count for the chosen scope; listed in `PREV_INTENDED` and the commit message; the matrix outputs unchanged; verify: the prev run with the list passes, no stale entry.

## 3. Documents

- [ ] 3.1 `docs/blitz2/deviations.md` kind-F entry; `docs/blitz2/expression-types.md` (the limits section goes, the rule); `docs/ab3/variant.md` row if AB3 mode follows (it shares the evaluator; #1177 and #1178 apply in both modes); verify: links resolve.

## 4. Commit and spec

- [ ] 4.1 One fix commit: code, test, `Fixed (#1183)`, docs, the changed results named, `Fixes #1183`; the agent proposes severity 1 and topic Compiler; verify: `git show --stat`.
- [ ] 4.2 If `quick-compare-types` or `quick-long-arithmetic` is archived before this change, rewrite the spec delta as MODIFIED/REMOVED of "Known limits of the left operand until the expression tree" (`openspec-update-change`); verify: `openspec validate compare-compound-left --strict`.
