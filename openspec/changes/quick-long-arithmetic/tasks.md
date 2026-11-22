# Tasks

Starts after the type-matrix test change is merged (order A, 2026-11-22; the matrix change is planned in parallel and named here once it exists). #1177 is merged (fe1d7b357); `docs/blitz2/deviations.md` exists. Decisions in design.md (user decisions 1-12, planning choices). Not parallel to `compiler-function-optimizer` (same file). Before #1179, #1182 and the tree (#1180).

## 1. Spike and tests first

- [x] 1.1 Spike in `eval.c` on a branch (2026-11-21, `tr-1178-spike-report.md`): the raw-Quick flag, the consumers, the division measured (towards zero), the `l/q` trap.
- [ ] 1.2 Tests of the expected behaviour (red on main), on top of the type-matrix test: the matrix's #1178 cells (type x operator x context x order) are this change's bug-orig cases in the form the matrix change defines; the cases of spec `compiler/expression-types` that the matrix lacks are added there or beside it: the quotient set of the spike (`l/n` 10/-1.5 -> -6, `m/n` 9/-1.5 -> -6, `k/q` -10/1.5 -> -6, `n/l` -1.5/10 -> 0, `n/2` -> 0, `g/h` -3.0/2.0 -> -1, the four sign combinations, a whole-number Quick), negation (`r.l=-q`, `r.l=0-q`, `x.w=-q`, `r.l=-q*2`), Quick OP Quick under Long with large whole values (`20000.0+20000.0`, `300.0*200.0`), the late cut (`q*100000`, `q/100000`, `q+&l-&l`, `Int(q)*100000`), a Float operand and `^` (`r.l=q+f`, `r.l=q^2`), shifts/logic/`MOD`/`Not` unchanged, `l*q` overflow, brackets, a Quick array element, field and function value, the address idiom, AB3 mode. The old output recorded from the CD's compiler (`bug-orig-record`); before the fix the cases come out REPRODUCED.
- [ ] 1.3 `tests/blitz2/quick-arith-long.bas` and `.expect`: the new expected values (user decision 4; e.g. 01 -> 3, 02 -> 4, 03 -> 8, 05 -> -3, 07 -> 3, 08 -> 15, 10 -> 3, 17 -> c, 18 -> 3, 24 -> 10, 32 -> 15, 33 -> 8, 35 -> 150000, 37 -> 1, 38 -> 6), its header comment rewritten (it pins the fixed rule); lines 20/21 stay 4 and 8 (user decision 12, interim until #1180).
- [ ] 1.4 `tests/bug-orig/compiler/compiler-compare-compound-left` (#1183): inputs whose fraction survives the operation (design decision 7, e.g. `q=1.25`), `.expect` and `.orig` re-recorded; it must stay REPRODUCED after the fix.

## 2. Implementation

- [ ] 2.1 `varcont`: remove the cut (`eval.c:3040`), set the raw flag; `getelement2`, `eval5`, `maineval2` as design decision 2; the consumers of the table there (late `convtypef` at `eval.c:936`, `dopow`, `shpoke`/logic/`MOD` handlers, `compareop`, `notit`, `negate`).
- [ ] 2.2 The operations of design decision 3 (inline `l+q`, `l-q`, `q-l`, `q±p`, `-q`; `q/l`, `q/h` by `longdiv` and truncation) and the late cut of decision 4; the cut placement of decision 6 (K3).
- [ ] 2.3 Library routines for `l*q`, `q*p` and `l/q` (user decision 3): in `lmullib.c`/`ldivlib.c` as assembler with the register contract of the neighbours (`libs/register-contract`), `main.h` tokens, `nocando` for folding, the library capture tests (`libs/tests`) with the new routines' cases (signs, overflow, a divisor with fraction bits), `declarations-intended.txt` where it applies.
- [ ] 2.4 `cmpownleft`: the Quick-to-Long case replaced by the raw flag (design decision 7); `compiler-quick-compare` stays FIXED.
- [ ] 2.5 The comment at `eval.c:3036`: "see issue #1178" becomes `Fixed (#1178)` with the rule.
- [ ] 2.6 AB3 mode gets the same rule (no mode test); the affected oracle tests in `tests/ab3` are marked.

## 3. Checks

- [ ] 3.1 `ctest --preset host` and the targeted tests: the matrix test, `blitz2_quick-arith-long`, `compiler-quick-compare`, `compiler-compare-compound-left` (REPRODUCED), `libs_tests` for the new routines, the corpus.
- [ ] 3.2 Cost: new lines in `tests/perf/bench` and `budgets.txt` for the new entries, measured against `lmullib`/`ldivlib`'s lines and the original's target; no existing budget rises (`ctest --preset perf`; D-007).
- [ ] 3.3 Prev comparison (`cmake --preset amiga -DBLITZ2_PREV=ON`): every differing executable has a Quick fetched under a Long type so far, a late cut under `*`/`/`, `-q`, `^` or a Float operand in such an expression; list them, count the left `+`/`-` operands of design decision 6 (code order only) apart from the value changes, and name them in `PREV_INTENDED` and the commit message, with the library files. `corpus`, `corpus_runerr`, the CD corpus (`blitz2-cd.tsv`): only the places of the issue and the negation/`^`/Float cases with a fraction differ; an executable-level comparison over more sources as the tree report proposes.
- [ ] 3.4 `tools/results fix-scope` against `main`: only `[fixed #1178]` / `[orig]` cases changed (spec `testing/bug-fix-workflow`), and the named #1183 input change.
- [ ] 3.5 With `-DHOST_SANITIZE=ON` for the host tests touched; clean rebuild without warnings.
- [ ] 3.6 `ctest --preset oracle` if `tests/ab3` changes.

## 4. Documents

- [ ] 4.1 `docs/blitz2/expression-types.md`: the exception section, the type-so-far list, the Float/Quick table and the paragraph "Planned in #1178" (the rule now), the measured examples (before/after), the #1177 limits ("Limits until the expression tree": the `r.l=(q*2>2)` example and the `-q` limit), and **"Where this is a problem"**, whose arithmetic item still says #1178 keeps the current behaviour (now: original bug #1178, fixed in the port); `survey-06.md:155` if its sentence changes.
- [ ] 4.2 `docs/blitz2/deviations.md`: the kind-F entry for #1178 and the #1177 row's limits (design decision 7); `docs/libs/deviations.md` for the new library entries (a program compiled by the port needs the port's libraries); `docs/ab3/variant.md` (a row: AmiBlitz3 keeps the old rule).
- [ ] 4.3 The site: the section "The type of an expression" in `site/content/docs/how-blitz2-works/compiling.md` (it says the port keeps the rule, `:147-151`); the fixed-bugs entry (`fixed-bugs/compiler.md`) and the row in `site/data/fixoverview.yaml` (coordinator's step; the agent proposes severity and topic).
- [ ] 4.4 Issue #1178: a comment with the decided rule and the measured comparison (coordinator, D-008).
- [ ] 4.5 The requirement "Known limits of the left operand until the expression tree" (`compiler/expression-types`, from `quick-compare-types`): its scenario `r.l=(q*2>2)` gives -1 after this change and the `-q` limit goes; a MODIFIED delta in this change once `quick-compare-types` is archived (its spec then exists in `openspec/specs/`), or the archive order set by the coordinator.

## 5. Commit and report

- [ ] 5.1 One fix commit with the code change, the converted tests, `Fixed (#1178)`, the documents, the changed results named (`PREV_INTENDED`) and a line `Fixes #1178`; the library routines in a separate commit if the change is separable (one topic per commit). The coordinator adds the label `fixed-in-port` and the site row, runs `all`, `bug-orig` and `originals` once after the merge, and comments on the issue (the comparison with the model: 358 / 18 / 0, and the negation/`^`/Float cases).
