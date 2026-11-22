# Tasks

Starts after `docs/blitz2/deviations.md` exists (a separate task). Decisions: design.md and `task-decisions.md` (2026-11-21). **Not parallel** to `quick-long-arithmetic` or `compiler-function-optimizer` (same file, `eval.c`). **`quick-long-arithmetic` depends on this change** and starts after it is merged. User decisions 1-3 are in design.md.

## 1. Spike and tests first

- [x] 1.1 Spike in `eval.c` on a branch (report, no commit): (a) the right operand's type at `doop` when a comparison starts its right operand at type 0 (a variable, function, bracket, constant, unary minus, `Not`); (b) where the comparison operators are emitted (`eval.c:1070-1093`, `1520-1525`) and today's code for `Long OP Quick` (instruction counts); (c) the left operand: whether the marker patch (design 2a, decided) covers a variable, field, array element, function value, `&var`, constant, and what a re-evaluation (2b) would have to undo; (d) the byte-constant patch (`eval.c:880-905`) in all four orders of the #978 test; (e) `convtypef` and `regat`/`fuckpos` (`eval.c:906`); (f) the exact instruction sequence for part 2 with counts and cycles against today's compare; (g) Word and Float against Quick or Long (user decision 3); (h) strings (`If a$=b$` with a Long destination). Report: `~/.local/share/bb2-c-work/tr-1177-spike-report.md`; the Float context: `tr-1177-float-context.md`.
- [x] 1.2 Red tests of the expected behaviour: the bug-orig test `tests/bug-orig/compiler/compiler-quick-compare.bas` with `; @fixed #1177`: every line of the issue (both orders, `r.l=(q>1)`, `(q=1)`, `(q>1.4)`, `(1.4<q)`, `Hex$(q>1)`, the Word destination), equality by the fraction, negative values, a Long beyond +-32768 against a Quick, Word and Byte operands, a Quick function value and array element, a comparison inside arithmetic (`r.l=l+(q>1)`), `OR`/`AND` of comparisons, whole-number Quicks (equal to the old results), the byte-constant cases of #978, strings; the measured wrong cases of `tr-1177-float-context.md` (whole numbers under a Float destination in both orders, a Float parameter, a Quick destination with `lw=65537`). `.expect` with the expected values; `.orig` recorded from the CD's compiler by `bug-orig-record` (needs the CD). Red on main.
- [x] 1.3 `tests/blitz2/quick-arith-long.bas` pins arithmetic only (its header says so): check that none of its lines is a comparison and that it is unchanged by this change.

## 2. Implementation

- [x] 2.1 `eval5`: a comparison operator starts its right operand at type 0; the left operand per the spike's mechanism (design 2: the marker for Quick -> Long, Byte/Word/Long -> Quick, Byte/Word/Long/Quick -> Float, `lasta6` restored; at register depth 7 the finding of design 2: the left operand in `d7` keeps the original's code, `getelement` saves the long for an element without a wanted type); the operand types reach the comparison; every other pair keeps today's handler.
- [x] 2.2 Part 2: the exact sequence for Long (and, if decided, Word) against Quick in both orders for the six operators; no library call.
- [x] 2.3 The comment at `eval.c:2893`: `Fixed (#1177)` for comparisons, the arithmetic cut stays `BUG(orig) (#1178)`; comments at the new code in the density of the neighbours, `blitz2.s:NNN` references where the structure still maps.
- [x] 2.4 AB3 mode: nothing mode-specific; mark the affected oracle tests in `tests/ab3`.
- [x] 2.5 Host tests for any new helper (snitch, with the component).

## 3. Checks

- [x] 3.1 `ctest --preset host` and the targeted tests: the bug-orig test (FIXED), `blitz2_quick-arith-long` (unchanged), the #978 test, `corpus`, `corpus_runerr`, the CD corpus if available; `-DHOST_SANITIZE=ON` for touched host tests; clean rebuild without warnings.
- [x] 3.2 Cost (D-007): a compiled loop condition `Long OP Quick` (default-typed variable) and `If n/2=Int(n/2)` inside the loop, before and after, in `tests/perf` with a line in `budgets.txt` if the spike's measurement says it matters; a whole-number loop condition with a constant changes by the constant's own typing (design 2), its cost is stated too. No existing budget rises (`ctest --preset perf`); the cost is stated in the commit.
- [x] 3.3 Prev comparison (`cmake --preset amiga -DBLITZ2_PREV=ON`): every differing executable has a comparison of the kinds in design 5; name them in `PREV_INTENDED` and the commit message; library files, `acidlibs`, `deflibs` and TED output identical. The executable-level comparison of #1181 if it exists.
- [x] 3.4 The 12 comparison places of the #1177 comment (8 distinct): compile them and check their results do not change (whole numbers); the code may. Closed by the evidence, not compiled (the programs are interactive): the spike's whole-number values (E2) and the whole-number cases of `compiler-quick-compare`; the user's answer "Per Nachweis schließen (Recommended)" (`task-decisions.md`, 2026-11-22).
- [x] 3.5 `tools/results fix-scope` against `main`: only `[fixed #1177]` / `[orig]` cases changed (spec `testing/bug-fix-workflow`).
- [x] 3.6 `ctest --preset oracle` (`tests/ab3` is touched).

## 4. Documents

- [x] 4.1 `docs/blitz2/expression-types.md` (comparison section, the partial fix and its limits: a compound left operand and `-q` (#1183), a Long constant on the left under Float; the Float and Quick contexts; "Where this is a problem", the row `r.l=(q>1)` before/after); `docs/blitz2/survey-06.md` if a sentence changes.
- [x] 4.2 The kind-F entry in `docs/blitz2/deviations.md` (exists by then); `docs/ab3/variant.md` row (AmiBlitz3 keeps the old rule).
- [x] 4.3 The site: the comparison paragraph in `site/content/docs/how-blitz2-works/compiling.md` ("Until then" goes; the limit for a compound left operand is stated); the fixed-bugs entry and the overview row are the coordinator's (proposed: severity 1, topic Compiler, label `fixed-in-port`).
- [x] 4.4 `quick-long-arithmetic`: correct its dependency text (the cut is removed there, user decision 2) and add the comparison exception to "The expected type flows as before".

## 5. Commit and report

- [x] 5.1 One fix commit: code, the converted bug-orig test (with a case showing the limit) (`; @fixed #1177`, `.orig`), `Fixed (#1177)`, the documents, the changed results named (`PREV_INTENDED`), a line `Fixes #1177`. The coordinator adds the label and the site row, runs `all`, `bug-orig` and `originals` once after the merge, and comments on the issue, naming the partial fix and the limit (compound left operand, #1180).
