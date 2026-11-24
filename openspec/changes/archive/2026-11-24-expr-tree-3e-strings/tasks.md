# Tasks

After `expr-tree-3d-library-calls`. "The check": `expr-tree-1-typing/design.md`, "The check of every byte-identical step" (after archive: its copy in `openspec/changes/archive/`).

## 1. Tests first

- [x] 1.1 Compile-only and run cases for unreached string forms (coverage build): long chains of `+` with literals and functions, string comparisons as operands of `AND`/`OR`, strings inside procedure parameters, AB3 escapes (`tests/ab3/cli`); verify: pass on main, in `compiler_prev`/`ab3_cli_prev`.

## 2. Implementation

- [x] 2.1 String leaves and string operator nodes, emitted in sequence; verify: the check, `ctest --preset oracle`.
- [x] 2.2 Side-effect table rows, comments, `tools/testinfo/rules.txt`; verify: links, selection test.

## 3. Commit

- [x] 3.1 Tests first, then the rework commit; verify: each passes the check.
