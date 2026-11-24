# Design

## Context

`getelement2` dispatches on the first character and emits the element into `D(regat)`; the element's type comes out through `*type` (the type so far). The series and its check: `expr-tree-1-typing/design.md`.

## Goals / Non-Goals

**Goals:** for each simple element, a parse part without emission and an emit part without parsing; the side-effect table.

**Non-Goals:** moving emission; removing the old globals.

## Decisions

### 1. Parse returns a leaf, emit consumes it, called back to back

The element's own type (declared type, value class of a constant), the type so far it was read under, and its facts are fields of the leaf. Emit uses only the leaf and the emitter state (`regat`, `lasta6`, `fuckpos`, `stackuse`). Called back to back, the bytes cannot change unless the split itself is wrong; the check catches that.

### 2. The side-effect table

For every element kind: what it changes outside the object code (variable list, label references, relocation list, string literal area, library area, `lasta6`, the error that can be raised and when). Kept in `docs/blitz2/expression-tree.md`; each later change of the series adds its kinds. A constant leaf keeps its fold fact (`constgot` = 0) for `forthstack`.

### 3. AB3 hooks in emission order

`ext_evalvalue` ("the type of the last element read at the depth of the assignment", `ab3type.c`) is called where it is called today; with emit right after parse that is unchanged. Change 8 must call it in reading order as well (noted in the table).

## Risks / Trade-offs

- [An error raised by today's emitting code (e.g. a type error inside `varcont`) moves to the parse part or the other way round] -> the first of two errors can change; the CD's error rows (`corpus_cd_prev`) and the limits test of change 2 catch it.

## Planning choices (mine, not the user's)

- `Point: the side-effect table as a document instead of a runtime check — L2 Possible / I2 Medium / E2 Reasoned → Plan` (the runtime proof is change 8's byte identity).

## Findings (implementation)

- Task 1.1's coverage: a run of the compared sources (`corpus*`, `blitz2_*`, `regression_*`, `ab3_cli*`, `site_examples*`, `compiler*`: 422 tests) with the coverage compilers (`amiga-cov`, function coverage) reached `fetchon`, `fetchoff`, `fetchsize`, `fetchpi`, `fetchconst`, `fetchfrac`, `fetchhex`, `fetchbin`, `ampersand`, `qmark`, `doused` (only `regression_compiler_object_used_below`), `domaximum`, `fetchpi` (only `corpus_cd`), `doaddr`, `variable`, and in `blitz2-ab3` `ext_element` (the `chr_*` sources of `tests/ab3/cli`); `doern` in no test. The branches function coverage cannot show were looked for in the sources. The new test `tests/blitz2/expr-leaves.test.bas` (BASIC test model, as `tests/README.md` asks for a new test) puts every simple element, under each type so far it can be read under, into one compared source (`blitz2_expr_leaves_prev`: its own prev pass, as it runs with and without runtime error checking).
- `ErrNum` (`doern`, the token "Ern") has no name in the token list (`tokcode.h`, `TOK_ERN`): no source text reaches it, so no test can; its split is checked by reading only.
- A `#constant` cannot have a fraction: `#c=3.5` is "Illegal Constant Expression" (a constant is a whole number). The case is read as a `#constant` in an expression with a fraction (`q=#k+0.5`, `q=#k/2`). A constant's name that begins with a token is the token (`#frac` is `#` and `Frac`: "Syntax Error"), so the test's constants have other names.
- Found while writing the test: a negative whole constant under a Quick type so far is wrong in the original and the port (`consttype2` `.qint`, `swap d0`: `q=On` gives `$FFFFFFFF`, `q=#c` with `#c=-3` `$FFFDFFFF`). An open original bug of class (a) (D-021: valid code, normal use); `BUG(orig)` note in `libcall.c`, bug-orig test `compiler-quick-negative-const` (REPRODUCED; its originals cell shows the declared deviations with 1.95). Not fixed here (the change changes no result).
- The leaf is a node of the arena (`struct exprnode`, kind `EXN_LEAF`, `op` its element kind `ELK_*`): the node has the fields `value`, `aux`, `vtype`, `off`, `own` and `lflags` (`ELF_*`) for it, 32 -> 46 bytes on the 68000 (the block of 136 nodes 4352 -> 6256 bytes). It is released after its emission (the window); the operand's opaque leaf of step 2 (`exprtree_climb`) stays as it is, change 8 makes the two one node. The bound stays 129: an element's leaf makes no node while it lives, behind the run of minus signs of the innermost level it is the 129th, never with the 65th counted node. Measured high water (a throw-away build of `blitz2-ab3` with a counter): 10 parentheses 14, 63 parentheses ("Precedence Stack Overflow") 66, 16 `Right$` 22, 63 parentheses and 31 calls 65, `chr_mem.ab3` 7, 63 times `-(` 128, 63 times `-(` with `-b` 129 (new; the host test pins it), 200 minus signs 3 (was 2), the operand in d7 22.
- Every constant element (literals, `#constants`, `SizeOf`, `On`, `Off`, `Maximum`, a label's value and `"abcd"` in a constant expression, AmiBlitz3's `@"..."`) ends in `fetchnum3`, which makes the constant's leaf and emits it (`constemit`): one place for all of them; their reading functions are their parse parts, unchanged.
- `varmode` (a4 or a5, set by `fetchvar`) is read by `pokewda5s`/`pokela5s` when the variable's code is written: a fact of the variable, so it is in the leaf (`ELF_LOCAL`) and the emit part sets it from there. The statement's line and procedure that a label reference records are the emitter's state.
- The char after an element is read after its code, as before: `getchar3` can run a `CNIF` or a `~` macro constant (`evalconst2`), which writes code at `pc`.
- An array, a pointer or a `\` path: `calcvar` still emits the address code while the variable is read (the parse part), step 3b splits it; a field `v\x` is a leaf whose address code comes before it.
- No error moved between the parts: the errors of reading are raised before any code of the element, the two of emission ("Out of Memory" of a label reference, `constemit`'s "Type Mismatch", which `consttype2` already caught) after, as before.
- The stack: `element`'s frame grew by 4 bytes (`qmark`'s two locals in memory, inlined into it), so every bracket level; `qmark` is not inlined now and the frame is the one of main. The high-water mark (the runner of step 2, `blitz2-ab3` with a 200000-byte stack; main = 0fe945039): 10 parentheses 4007 -> 4007, 63 parentheses 16899 -> 16899, 16 `Right$` 10179 -> 10191, 63 parentheses with 31 calls (#1080) 24091 -> 24091, `chr_mem.ab3` 3099 -> 3111, 63 times `-(` 24207 -> 24207, with `-b` 24323 -> 24323, 200 minus signs 1643 -> 1643, the operand in d7 3275 -> 3275. Against main before step 2 (60d25326f, step 2's figures): 63 parentheses +8.2 %, the #1080 case +5.6 %, 16 `Right$` +3.5 %.
- `blitz2` 113108 -> 114252 bytes, `blitz2-ab3` 151972 -> 153116.
- The planning choice (the side-effect table as a document): as planned, the table is in `docs/blitz2/expression-tree.md`, "Step 3a"; the runtime proof stays change 8's byte identity.
- The check (prev 0fe945039, main at the branch point, as `-DBLITZ2_PREV=<hash>`, no `PREV_INTENDED`): `prev_results` and the 92 `corpus_exprmatrix*` 93 passed; the corpus, `corpus_cd`, `corpus_cd_ab3` and their prev runs, the `bug_orig_*_prev`, `blitz2_*_prev` (with `blitz2_expr_leaves_prev`), `regression_*_prev`, `compiler_prev`, `ab3_cli_prev_*`, the site examples, `ted_ab` and the cases of `blitz2_expr_limits` and `blitz2_expr_leaves` 236 passed; `ctest --preset oracle -R ab3` 6 passed; `ctest --preset bug-orig -R compiler` REPRODUCED as on main (`compare_compound_left`, `panic_remtask`) and the new `quick_negative_const`; `blitz2_exprtree`, `blitz2_exprtype` and `covsel` also with `-DHOST_SANITIZE=ON`.
