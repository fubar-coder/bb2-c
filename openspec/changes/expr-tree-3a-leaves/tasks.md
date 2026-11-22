# Tasks

After `expr-tree-2-operators`. "The check": `expr-tree-1-typing/design.md`, "The check of every byte-identical step" (after archive: its copy in `openspec/changes/archive/`).

## 1. Tests first

- [ ] 1.1 Check the coverage of the simple elements in the compared sources: compile-only cases in `tests/blitz2` for each element kind that no compared source reaches (`On`/`Off`, `Used`, `Maximum`, `Addr`, `ErrNum`, `SizeOf` of a NewType field, `?label` forward and back, `#constant` with a fraction, AB3 character literals in `tests/ab3/cli`), found with the coverage build (`testing/coverage`); verify: the new cases pass on main and are in `compiler_prev`.

## 2. Implementation

- [ ] 2.1 Constants and `#constants` (with `ext_constant`): parse/emit split; verify: the check.
- [ ] 2.2 Scalar variables and fields (`variable`, `varcont` with the facts of #978, #1177, #1178, #1186); verify: the check.
- [ ] 2.3 `&var`, `?label`, `SizeOf`, `Pi`, `On`/`Off`, `Used`, `Maximum`, `Addr`, `ErrNum`, `ext_element`; verify: the check, `ctest --preset oracle` for the AB3 cases.
- [ ] 2.4 `docs/blitz2/expression-tree.md` with the side-effect table for these kinds; comments in the form answered for `expr-tree-1-typing` open question 1; `tools/testinfo/rules.txt`; verify: links resolve, selection test.

## 3. Commit

- [ ] 3.1 Tests of 1.1 in a commit of their own; then one rework commit per task 2.1-2.3 or one for all (each passes the check alone); verify: every commit builds and passes the check.
