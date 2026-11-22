# Tasks

After `expr-tree-1-typing` is merged. "The check": `expr-tree-1-typing/design.md`.

## 1. Tests first

- [ ] 1.1 `tests/blitz2/expr-limits.bas` (or one source per case where a compile error ends the compile): 63 nested brackets, 31 and 32 pending elements, 64 pending operators, an operand at `regat` 7, a chain of 1000 terms through a macro, 16 and 30 nested `Right$`; the expected outputs and compile errors (message and line) recorded from main; verify: `ctest --preset all -R expr-limits` passes on main.
- [ ] 1.2 Host test skeleton `programs/blitz2/tests/host/test_exprtree.cpp`: arena reset, node limits, the iterative walk over 100 000 equal-precedence operators (design decision 3); verify: it builds against stubs and fails until 2.1 (red).
- [ ] 1.3 Measure the live node count at the limits of 1.1 (a counter in a scratch build, not committed) and set the static block size with a margin; verify: the numbers in the change's report.

## 2. Implementation

- [ ] 2.1 `exprtree.c`/`.h`: node layout, the static arena, reset in `prepstack`; verify: `ctest --preset host -R blitz2_exprtree` with and without `-DHOST_SANITIZE=ON`.
- [ ] 2.2 `eval4`/`eval5`/`negate`/`notit`/`bracket` build operator nodes and emit from them at today's moments (design decision 1); `precstack` replaced by the nodes with the counter of decision 2; `forthstack` kept for the fold; verify: the check, and 1.1 unchanged.
- [ ] 2.3 Stack high-water mark of a compile for the #1080 cases (5/10/40/63 brackets, 16 `Right$`, `chr_mem.ab3`) and the 1000-term chain, before and after, with the fill-pattern method of #1080 (throw-away runner in the scratchpad); verify: no case deeper than before plus 10 %, the chain flat; numbers in the report.
- [ ] 2.4 `tools/testinfo/rules.txt` for the new files and tests; `docs/architecture.md` (the evaluator's structure) and comments in the form answered for `expr-tree-1-typing` open question 1; verify: selection test, links resolve.

## 3. Commit

- [ ] 3.1 The test of 1.1 in a commit of its own before the rework; then one rework commit (no result changes, the check's runs named); verify: `git log` shows the two commits, each builds.
