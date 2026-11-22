# Tasks

After `expr-tree-3c-calls`. "The check": `expr-tree-1-typing/design.md`, "The check of every byte-identical step" (after archive: its copy in `openspec/changes/archive/`).

## 1. Tests first

- [ ] 1.1 Compile-only cases for unreached forms (coverage build): a library function whose parameter is another library's function, each used first there; forms with repeats; unknown-type parameters; each intrinsic with a Quick, a Long and a string argument, as the whole right side and inside an expression; verify: pass on main, in `compiler_prev`.

## 2. Implementation

- [ ] 2.1 `findtoke` split (design decision 1); verify: the check.
- [ ] 2.2 Library call nodes with enter/exit; the intrinsic state stored in the node; verify: the check, `ctest --preset oracle`.
- [ ] 2.3 Side-effect table rows, comments, `tools/testinfo/rules.txt`; verify: links, selection test.

## 3. Commit

- [ ] 3.1 Tests first, then one rework commit per task 2.1 and 2.2; verify: each passes the check.
