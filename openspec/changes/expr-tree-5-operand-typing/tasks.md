# Tasks

After `compare-compound-left`. Open questions 1-4 answered between groups 1 and 2.

## 1. Decision round (measure first)

- [ ] 1.1 Candidate rules for open question 1 in the matrix model as rule sets (among them the integral/fractional constants of open question 1) (`tools/lib/exprmatrix/model.c`, snitch tests); for each: the matrix cells that change, the 376 places of #1178 re-run (`tr-quick-arith-variants-report.md`'s method) with the places that change value, the CD executables a prototype changes, and cycles for the typical changed forms; report `~/.local/share/bb2-c-work/expr-tree-5-rules.md`; verify: every candidate has the four numbers; `ctest --preset host`.
- [ ] 1.2 The questions with the numbers to the coordinator (open-questions file); after the answers: the chosen rule in the model, the named-cell list generated (`exprmatrix expect` with the new rule), the spec delta completed with the decided scenarios (`openspec-update-change`); verify: `openspec validate expr-tree-5-operand-typing --strict`.

## 2. Tests first

- [ ] 2.1 The named cells as bug-orig/fixed tests in the matrix's form (REPRODUCED on main); `quick-arith-long` lines 20/21 and the spec scenarios as a bug-orig test; verify: `ctest --preset bug-orig -R exprmatrix|operand-order` REPRODUCED.
- [ ] 2.2 The type check of `blitz2_exprtype` against the matrix model by types, not only by values: `tools/lib/exprmatrix/model.c` returns the type so far of an evaluated cell (for example a field in `em_value`), and `programs/blitz2/tests/host/test_exprtype.cpp` compares the type of `exprtype_binop` with it for every two-operand cell both define (first with the original rules, before the rule of 1.2 is applied to the function); a planted wrong result type that keeps the value fails; verify: `ctest --preset host -R "blitz2_exprtype|exprmatrix"`, also with `-DHOST_SANITIZE=ON`. Source: rounds/tree/decisions.md 2026-11-23 "Model check: types later, before step 5".

## 3. Implementation

- [ ] 3.1 The typing pass (design decision 1) and `exprtype.c`'s new decisions; the golden table rows changed and named; verify: `ctest --preset host -R blitz2_exprtype`, the model cross-check passes with the new rule set.
- [ ] 3.2 The tests FIXED; every other matrix cell unchanged; `ctest --preset oracle` with the marked tests (by open question 2); prev run with the listed executables; `tools/results fix-scope` against main; verify: the commands pass with only the named changes.

## 4. Documents and commit

- [ ] 4.1 `docs/blitz2/expression-types.md` (the rule, the tables, "Where this is a problem"), `docs/blitz2/deviations.md`, `docs/ab3/variant.md`, the site section (`site/content/docs/how-blitz2-works/compiling.md`); one fix commit by the bug-fix workflow as classified (open question 4); verify: links resolve, `git show --stat`.
- [ ] 4.2 If `quick-long-arithmetic` is archived first, rewrite the delta as MODIFIED/REMOVED of "The expected type flows as before"; verify: `openspec validate expr-tree-5-operand-typing --strict`.
