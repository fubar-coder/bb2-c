# Tasks

After `compare-compound-left`. Open questions 1-4 answered between groups 1 and 2.

## 1. Decision round (measure first)

- [ ] 1.1 Candidate rules for open question 1 in the matrix model as rule sets (`tools/lib/exprmatrix/model.c`, snitch tests); for each: the matrix cells that change, the 376 places of #1178 re-run (`tr-quick-arith-variants-report.md`'s method) with the places that change value, the CD executables a prototype changes, and cycles for the typical changed forms; report `~/.local/share/bb2-c-work/expr-tree-5-rules.md`; verify: every candidate has the four numbers; `ctest --preset host`.
- [ ] 1.2 The questions with the numbers to the coordinator (open-questions file); after the answers: the chosen rule in the model, the named-cell list generated (`exprmatrix expect` with the new rule), the spec delta completed with the decided scenarios (`openspec-update-change`); verify: `openspec validate expr-tree-5-operand-typing --strict`.

## 2. Tests first

- [ ] 2.1 The named cells as bug-orig/fixed tests in the matrix's form (REPRODUCED on main); `quick-arith-long` lines 20/21 and the spec scenarios as a bug-orig test; verify: `ctest --preset bug-orig -R exprmatrix|operand-order` REPRODUCED.

## 3. Implementation

- [ ] 3.1 The typing pass (design decision 1) and `exprtype.c`'s new decisions; the golden table rows changed and named; verify: `ctest --preset host -R blitz2_exprtype`, the model cross-check passes with the new rule set.
- [ ] 3.2 The tests FIXED; every other matrix cell unchanged; `ctest --preset oracle` with the marked tests (by open question 2); prev run with the listed executables; `tools/results fix-scope` against main; verify: the commands pass with only the named changes.

## 4. Documents and commit

- [ ] 4.1 `docs/blitz2/expression-types.md` (the rule, the tables, "Where this is a problem"), `docs/blitz2/deviations.md`, `docs/ab3/variant.md`, the site section (`site/content/docs/how-blitz2-works/compiling.md`); one fix commit by the bug-fix workflow as classified (open question 4); verify: links resolve, `git show --stat`.
- [ ] 4.2 If `quick-long-arithmetic` is archived first, rewrite the delta as MODIFIED/REMOVED of "The expected type flows as before"; verify: `openspec validate expr-tree-5-operand-typing --strict`.
