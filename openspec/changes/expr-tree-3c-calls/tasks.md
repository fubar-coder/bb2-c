# Tasks

After `expr-tree-3b-arrays`. "The check": `expr-tree-1-typing/design.md`, "The check of every byte-identical step" (after archive: its copy in `openspec/changes/archive/`).

## 1. Tests first

- [ ] 1.1 Compile-only cases in `tests/blitz2` for what the compared sources do not reach (coverage build): a function call nested in its own parameter, a forward-referenced function, a call at `regat` 5 and 6 (push mode), an OS call twice in a row (no second base load) and after a library call, AB3 procedure forms in `tests/ab3/cli`; verify: pass on main, in `compiler_prev`/`ab3_cli_prev`.

## 2. Implementation

- [ ] 2.1 Call node with enter/exit (design decision 1) for `afunction`; verify: the check.
- [ ] 2.2 The same for `amigalib`; verify: the check.
- [ ] 2.3 Stack high-water mark for 16 and 30 nested calls (method of `expr-tree-2-operators` task 2.3); verify: not deeper than before plus 10 %.
- [ ] 2.4 Side-effect table rows, comments, `tools/testinfo/rules.txt`; verify: links, selection test.

## 3. Commit

- [ ] 3.1 Tests first, then the rework commit; verify: each passes the check.
