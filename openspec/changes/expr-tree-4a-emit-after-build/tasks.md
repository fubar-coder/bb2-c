# Tasks

After `expr-tree-3e-strings`. Open question 1 answered before 2.5. "The check": `expr-tree-1-typing/design.md`, "The check of every byte-identical step" (after archive: its copy in `openspec/changes/archive/`).

## 1. Tests first

- [x] 1.1 A source (`tests/blitz2`) whose only use of a library is a conversion `cmpownleft` drops (found by an instrumented scratch compiler over the CD, as proposed in `planning-order-tree-vs-fixes.md`, "Evidence"; or built by hand); verify: its executable on main contains the library; in `compiler_prev`.
- [x] 1.2 Sources with two errors in one expression (operator error before an unknown name, a type error before a missing bracket, an error inside a parameter before one after it), one per source, message and line recorded from main; verify: pass on main.
- [x] 1.3 Constant-fold cases that today's pairwise rule folds and does not fold (`1+2`, `x+1+2`, `-5`, `Not 0`, a folded `Mod` as a library's first use, #1049; a fold while the object buffer is full); verify: pass on main, in `compiler_prev`.

## 2. Implementation

- [ ] 2.1 Build-then-emit for a top-level `maineval2` (design decisions 1, 2); verify: the check.
- [ ] 2.2 The fold on nodes (decision 3), `forthstack` removed; verify: the check, 1.3.
- [ ] 2.3 #978 as a node rule, the patch removed; verify: the check, `compiler-byte-const-compare` FIXED.
- [ ] 2.4 #1177's left operand as a node rule, `cmpownleft` removed, the library use of decision 4; verify: the check, 1.1, `compiler-quick-compare` FIXED, `compiler-compare-compound-left` (#1183) REPRODUCED.
- [ ] 2.5 Arena blocks and the error of open question 1; host test of the failure path (allocation stub returns NULL: the error, no retry, the arena released); node size and the worst case measured for a macro line of 8 KB; verify: `ctest --preset host -R blitz2_exprtree` with `-DHOST_SANITIZE=ON`, the numbers in the report.
- [ ] 2.6 Stack high-water marks as in `expr-tree-2-operators` 2.3, plus the 1000-term chain; verify: not deeper than after change 2 plus 10 %, the chain flat.

## 3. Documents

- [ ] 3.1 `docs/blitz2/expression-tree.md` (build, typing, emission, fold), `docs/architecture.md`, the mechanism sentences of the #978 and #1177 rows in `docs/blitz2/deviations.md` (behaviour unchanged), `docs/blitz2/expression-types.md` where it describes patches; verify: links resolve.

## 4. Commit

- [ ] 4.1 Tests of group 1 in their own commit; then one rework commit per task 2.1-2.5 (each passes the check); the report names the compile-time and memory figures; verify: every commit passes the check.
