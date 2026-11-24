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
