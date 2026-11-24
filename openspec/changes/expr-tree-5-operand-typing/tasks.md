# Tasks

After `compare-compound-left`. Group 1 comes first, before the decision round. Open questions 1-4 answered between groups 2 and 3.

## 1. Readable tables of today's type rules (before the decision round)

Decided by the user, 2026-11-24 (`rounds/tree/decisions.md`, "expr-tree-5: readable tables of today's type rules first"; design decision 3).

- [ ] 1.1 The tool: logic in `tools/lib/exprtype_tables/` (links `programs/blitz2/src/exprtype.c`, as `test_exprtype.cpp` does), command `tools/exprtables`, host tests in `tools/tests/` (snitch). First read `exprtype.c` for how the wanted type enters (the initial type so far; `exprtype_binop(op, left, right, facts)` takes the type so far after the left operand and after the right one, `exprtype_element(tsf, own, facts)` the type so far before an element) and document it in the design before coding; the output tables follow design decision 3. Verify: `ctest --preset host -R exprtables`, also with `-DHOST_SANITIZE=ON`; every cell of `programs/blitz2/tests/host/exprtype.tsv` that the tables claim to cover is reproduced by the tool (a test compares the two).
- [ ] 1.2 The tables in the docs (`docs/blitz2/expression-type-tables.md`, generated, linked from `docs/blitz2/expression-types.md`), with the exceptions as notes naming their issues (#1178, #1182, #1188, #1209, #978, #1179), and a check in the build that the page equals the tool's output, in the build step of the page (CLAUDE.md "Code conventions"), with a test on deliberately broken input (a changed cell, a missing note). Verify: a clean build of the docs target has no warnings; the broken-input test fails the check; links resolve; `ctest --preset host`.

## 2. Decision round (measure first)

- [ ] 2.1 Candidate rules for open question 1 in the matrix model as rule sets (among them the integral/fractional constants of open question 1) (`tools/lib/exprmatrix/model.c`, snitch tests); for each: the matrix cells that change, the 376 places of #1178 re-run (`tr-quick-arith-variants-report.md`'s method) with the places that change value, the CD executables a prototype changes, and cycles for the typical changed forms; report `~/.local/share/bb2-c-work/expr-tree-5-rules.md`; for each candidate the tool of 1.1 prints the same tables as today's page (design decision 3) with the changed cells and the CD places marked, in the report; verify: every candidate has the four numbers and its tables; `ctest --preset host`.
- [ ] 2.2 The questions with the numbers to the coordinator (open-questions file); after the answers: the chosen rule in the model, the named-cell list generated (`exprmatrix expect` with the new rule), the spec delta completed with the decided scenarios (`openspec-update-change`); verify: `openspec validate expr-tree-5-operand-typing --strict`.

## 3. Tests first

- [ ] 3.1 The named cells as bug-orig tests in the matrix's form (`exprmatrix expect`: tests of the BASIC test model `compiler-expr-ISSUE-CTX-SET[-u|-c]-NN`, open cells without `fixed-orig`, the original's values the deviations; REPRODUCED on main); `quick-arith-long` lines 20/21 and the spec scenarios as a bug-orig test; verify: `ctest --preset bug-orig -R "compiler_expr_ISSUE|operand_order"` REPRODUCED (`ISSUE` the issue of the rule).
- [ ] 3.2 The type check of `blitz2_exprtype` against the matrix model by types, not only by values: `tools/lib/exprmatrix/model.c` returns the type so far of an evaluated cell (for example a field in `em_value`), and `programs/blitz2/tests/host/test_exprtype.cpp` compares the type of `exprtype_binop` with it for every two-operand cell both define (first with the original rules, before the rule of 2.2 is applied to the function); a planted wrong result type that keeps the value fails; verify: `ctest --preset host -R "blitz2_exprtype|exprmatrix"`, also with `-DHOST_SANITIZE=ON`. Source: rounds/tree/decisions.md 2026-11-23 "Model check: types later, before step 5".

## 4. Implementation

- [ ] 4.1 The typing pass (design decision 1) and `exprtype.c`'s new decisions; the golden table rows changed and named; verify: `ctest --preset host -R blitz2_exprtype`, the model cross-check passes with the new rule set.
- [ ] 4.2 The tests FIXED (the generator adds `fixed-orig = N`; the deviations of the port with the CD's libraries change with the fix, as that configuration then gives the fixed value); every other matrix cell unchanged; `ctest --preset oracle` with the marked tests (by open question 2); prev run with the listed executables; `tools/results fix-scope` against main; verify: the commands pass with only the named changes.

## 5. Documents and commit

- [ ] 5.1 `docs/blitz2/expression-types.md` (the rule, "Where this is a problem") and `docs/blitz2/expression-type-tables.md` regenerated with the new rule (the build check of 1.2 passes; the changed cells are the named ones), `docs/blitz2/deviations.md`, `docs/ab3/variant.md`, the site section (`site/content/docs/how-blitz2-works/compiling.md`); one fix commit by the bug-fix workflow as classified (open question 4); verify: links resolve, `git show --stat`.
- [ ] 5.2 If `quick-long-arithmetic` is archived first, rewrite the delta as MODIFIED/REMOVED of "The expected type flows as before"; verify: `openspec validate expr-tree-5-operand-typing --strict`.
