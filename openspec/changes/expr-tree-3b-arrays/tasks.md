# Tasks

After `expr-tree-3a-leaves`. "The check": `expr-tree-1-typing/design.md`, "The check of every byte-identical step" (after archive: its copy in `openspec/changes/archive/`).

## 1. Tests first

- [x] 1.1 The branches of `calcvar` listed; coverage build over the compared sources; compile-only (and where they run, run) cases in `tests/blitz2` for every unreached form: list arrays, `\` paths and `UsePath`, NewType arrays and fields of array elements, pointers `*p`, multi-dimensional arrays with and without runtime checking, arrays in procedures (local, shared); verify: the cases pass on main, are in `compiler_prev`, and the coverage report shows every branch reached or named.

## 2. Implementation

- [ ] 2.1 The variable node and the parse part (design decision 1); verify: host build, the check.
- [ ] 2.2 The emit part, `variable` and `calcvar`'s statement entry through it; verify: the check, `corpus_runerr` and `corpus_exprmatrix_runerr_*` with prev.
- [ ] 2.3 Side-effect table rows, comments (form of `expr-tree-1-typing` open question 1), `tools/testinfo/rules.txt`; verify: links, selection test.

## 3. Commit

- [ ] 3.1 Tests of 1.1 in their own commit, then the rework commit; verify: each passes the check.
