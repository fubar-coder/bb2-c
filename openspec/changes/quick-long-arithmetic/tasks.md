# Tasks

Starts after the change for #1177 (comparisons) is done, and after `docs/blitz2/deviations.md` exists (a separate task); decisions in design.md (user decisions 1-7). Not parallel to `compiler-function-optimizer` (same file) or to the change for #1177.

## 1. Spike and tests first

- [ ] 1.1 Spike in `eval.c` on a branch: where the right operand's type is known at `doop`, which of the patching mechanisms of `tr-expr-tree-report.md` section 1.5 interfere, and what BB2's division does with negative inexact quotients (measured on the CD's compiler with the quotient cases of the test). Report; confirms the division rounding (towards zero).
- [ ] 1.2 Tests of the expected behaviour (red on main): extend `tests/blitz2/quick-arith-long.bas` with the cases of spec `compiler/expression-types`: every line of the measured table of `docs/blitz2/expression-types.md`, negative values (`n*2`, `l*n`, `m/n`, `l/n` inexact), division both ways, `Quick*Long` with a product beyond +-32768, chains (`l+q+p`), Word and Byte operands, Quick and Long functions, a Quick array element and field, the address idiom `adr.l=base+i*4`, a Quick destination, a Float operand, the order cases `x.w=w+v+q*2` / `x.w=q*2+w+v` (the old values 9 and 10 stay or change as the design says), and whole-number Quicks (equal to the old results). The test is a bug-orig test with `; @fixed #1178` (or the equivalent) and the old output as `.orig`, recorded from the CD's compiler by `bug-orig-record`; `tests/blitz2/quick-arith-long.bas` gets the new expected values.

## 2. Implementation

- [ ] 2.1 `varcont`: remove the cut (`eval.c:2893`); `eval5`: pass the operand types to the mixed path (design decision 2); `l-q`, `l+q`, `q-l` inline.
- [ ] 2.2 Library routines for `l*q` and `l/q` (user decision 3): in `lmullib.c`/`ldivlib.c` as assembler with the register contract of the neighbours (`libs/register-contract`), `main.h` tokens, `nocando` for folding, the library capture tests (`libs/tests`) with the new routines' cases, `declarations-intended.txt` where it applies.
- [ ] 2.3 The comment at `eval.c:2893`, `BUG(orig)` becomes `Fixed (#1178)` as the workflow requires.
- [ ] 2.4 AB3 mode gets the same rule (no mode test); the affected oracle tests in `tests/ab3` are marked.

## 3. Checks

- [ ] 3.1 `ctest --preset host` and the targeted tests: `blitz2_quick-arith-long`, `libs_tests` for the new routines, the corpus.
- [ ] 3.2 Cost: new lines in `tests/perf/bench` and `budgets.txt` for the new entries, measured against `lmullib`/`ldivlib`'s lines and the original's target; no existing budget rises (`ctest --preset perf`; D-007).
- [ ] 3.3 Prev comparison (`cmake --preset amiga -DBLITZ2_PREV=ON`): every differing executable is a program with a Quick mixed with a Long or Word in arithmetic; list them and name them in `PREV_INTENDED` and the commit message, with the library files. `corpus`, `corpus_runerr` and the CD corpus (`blitz2-cd.tsv`) outputs: only the 18 places of the issue (or fewer in the sources that compile) differ; an executable-level comparison over more sources as the tree report proposes (step 0).
- [ ] 3.4 `tools/results fix-scope` against `main`: only `[fixed #1178]` / `[orig]` cases changed (spec `testing/bug-fix-workflow`).
- [ ] 3.5 With `-DHOST_SANITIZE=ON` for the host tests touched; clean rebuild without warnings.
- [ ] 3.6 `ctest --preset oracle` if `tests/ab3` changes.

## 4. Documents

- [ ] 4.1 `docs/blitz2/expression-types.md` (rule, measured table before/after, "Where this is a problem"), the comment pointing to it, `survey-06.md:155` if its sentence changes.
- [ ] 4.2 The kind-F entry in `docs/blitz2/deviations.md`; `docs/libs/deviations.md` for the new library entries (a program compiled by the port needs the port's libraries); `docs/ab3/variant.md` (a row: AmiBlitz3 keeps the old rule).
- [ ] 4.3 The site: the section "The type of an expression" in `site/content/docs/how-blitz2-works/compiling.md`; the fixed-bugs entry (`fixed-bugs/compiler.md`) and the row in `site/data/fixoverview.yaml` (coordinator's step; the agent proposes severity and topic).

## 5. Commit and report

- [ ] 5.1 One fix commit with the code change, the converted test, `Fixed (#1178)`, the documents, the changed results named (`PREV_INTENDED`) and a line `Fixes #1178`; the library routines in a separate commit if the change is separable (one topic per commit). The coordinator adds the label `fixed-in-port` and the site row, runs `all`, `bug-orig` and `originals` once after the merge, and comments on the issue (the comparison with the model: 358 / 18 / 0).
