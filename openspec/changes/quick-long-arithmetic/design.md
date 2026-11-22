# Design

## Context

`eval5` (`programs/blitz2/src/eval.c:881-958`, line numbers of main 03fa9fd75) reads one element, emits its code at once and decides every type from one value, the type so far. `varcont` (`eval.c:3036-3052`) makes the cut: `if (oldtype == T_LONG && *type == T_QUICK) goto conv2;` converts the Quick to Long at fetch. Operators are chosen by `doop(op, type)` through `op->h[type-1]`; both operands are already of the type so far when it runs (`convtypef(left, *type)` at `eval.c:936`). So `eval5` never sees a Quick operand under a Long type so far.

#1177 is merged (fe1d7b357): comparisons go through `compareop(op, left, right)` (`eval.c:1227`), the right operand of a comparison starts at type 0, and `cmpownleft` (`eval.c:1149`) takes back the conversion `varcont` recorded in `conv` for a one-element left operand. The cut itself is still in place; removing it is this change's (user decision 2).

Decided (2026-11-21, `task-decisions.md`, "#1178 direction: variant C"): variant C, order dependence as today, order independence later by the expression tree (#1180). Order of the work (2026-11-22, "Order of the Quick work; matrix test"): the type-matrix test change (`expression-type-matrix`; its tests are on main, c336cf473), then this change, then #1179, #1182, the tree. The spike (task 1.1, `tr-1178-spike-report.md`, line numbers of 7ff47a161) gives the mechanism and the consumers; `tr-1178-rounding-count.md` and `tr-float-rounding.md` are the measurements behind the rounding decision; `docs/blitz2/expression-types.md`, "Converting to a whole number: Float and Quick differ", has the measured Float/Quick table.

## Decisions

### 1. Capability `compiler/expression-types`, new

The behaviour is about the compiler's typing of arithmetic, not an inline library call (`compiler/intrinsics`) and not AB3 mode's type rules (`ab3-compat/type-rules`; a rule shared by both modes does not belong in an `ab3-compat` capability). `quick-compare-types` adds its comparison requirements to the same capability; whichever change is archived first creates the spec. The later tree can modify them.

### 2. Mechanism: a raw-Quick flag per operand (spike, E3)

From the spike (section 2; a prototype with the flag and today's values passed `blitz2_*`, `corpus`, `corpus_runerr`, `blitz2_cd_reference` and 40 consumer cases):

* `varcont`: instead of `goto conv2`, keep `*type = T_LONG` and set `rawq` ("D(regat) holds a raw Quick, the type so far is Long"). Every element starts with `rawq = FALSE` (`getelement2`, next to `negnext = FALSE`, `eval.c:1794`), so constants, `&`, `?`, strings and AB3 elements are never raw.
* `eval5`: `lraw = rawq` after `eval2`, `rraw = rawq` after the recursion; at `doop` the operand types are (`lraw ? Quick : left`, `rraw ? Quick : *type`). After the operator the flag is FALSE (every operation under a whole-number type so far gives a whole number, decision 3); at each `return prec`, `rawq = lraw`. Brackets need nothing (`bracket` returns with `rawq` as the inner `eval5` left it).
* `maineval2`: converts a raw Quick (floor) before `convtype(t, asked)`, and also when `unknown` is set. Every caller and every parameter goes through it.
* The spill around d6 (`getelement`) chooses `.l` for a raw Quick (its `*type` is Long): correct as it is.

The consumers that must know the flag (spike, section 3):

| Consumer | With a raw operand |
|---|---|
| `+ - * /` (`doop`) | the mixed path of decision 3 |
| the late conversion `convtypef(left, *type)` (`eval.c:936`) | a raw left and a Float right: `quicktofloat`, not `longtofloat` (the raw value would be 65536 times too large); a Quick left and a Long right with no type so far (the late cut): decision 4 |
| `dopow` (`eval.c:1458`) | `quicktofloat` from the raw value (decision 5) |
| shifts (`shpoke`), `AND`, `OR`, `EOR`, `MOD` | cut before the handler: today's value (the spec changes only `+ - * /`) |
| comparisons (`compareop`) | the left type is Quick; compared exactly (#1177's code); replaces the Quick-to-Long case of `cmpownleft` (decision 7) |
| `notit` (`eval.c:1753`) | cut before the `not`: today's value |
| `negate` (`eval.c:1723`) | negate the raw value and keep the flag (decision 3: floor(-q)) |
| the end of the expression (`maineval2`) | floor, as the conversion today |

Of the five patching mechanisms of `tr-expr-tree-report.md` 1.5 only the byte-constant patch (#978) is next to the new code and does not interfere (the mixed code is emitted after the `byteconstext` check); constant folding, `divzerochk`, `pushstart`/`pushput`, the look-ahead and `undodebug` do not interfere (spike, E3 for the prototype).

### 3. The operations and their rounding (user decisions 5, 8, 9)

Rounding (2026-11-21, "#1178 rounding rule decided": "Zwischenlösung: / gegen Null (Recommended)"): a division rounds towards zero; `+ - *`, negation and the conversion of a Quick to Long, Word or Byte round down. Every operation under a whole-number type so far gives a whole number at the operator (variant C, "each operation computed with the fraction and rounded down when it must give a whole-number type"; the model of the 358/18/0 count rounds per operation, `tr-quick-arith-variants/sim.py`, class `VarC`).

`q`, `p`, `h` raw 16.16, `l` whole (a Word is extended to Long first, as `convtype` does); type so far Long:

| Operation | Result | Code |
|---|---|---|
| `l+q`, `q+l` | `floor(l+q)` = `l + floor(q)` | as today (swap, extend, add) |
| `l-q` | `floor(l-q)` = `l - ceil(q)` | 2-3 more instructions (add $FFFF before the swap, or negate then floor; the edge value -32768.0 to be handled) |
| `q-l` | `floor(q) - l` | as today |
| `l*q`, `q*l` | `floor(l*q)`: `(l*qraw)>>16`, 64-bit intermediate | new library routine |
| `l/q` | `trunc(l/q)`: `(l<<16)/qraw` truncated, 48/32 bits on the magnitudes | new library routine; **not** a Quick quotient followed by `quicktolong`, which floors (`10/-1.5` would give -7, decided is -6; spike section 1) and inherits the loop's 1-ulp rounding up |
| `q/l` | `trunc(q/l)`: the raw value divided by `l` (Long division, towards zero), then the fraction dropped towards zero | existing `longdiv`, then truncating (not `quicktolong`): `-2.0/10` gives 0 as today, `-1.5/10` gives 0 |
| `q+p`, `q-p` | `floor(q±p)`, computed without a 32-bit raw intermediate (no Quick overflow: `20000.0+20000.0` gives 40000 as today) | inline |
| `q*p` | `floor(q*p)`: the high long of the 64-bit product of the raw values | new library routine (a second entry or a variant of the `l*q` one) |
| `q/h` | `trunc(q/h)`: `qraw/hraw`, Long division towards zero (the scale cancels) | existing `longdiv`; `r.l=q/h` with -3.0 and 2.0 gives -1 (2026-11-21, "#1178 c") |
| `-q` | `floor(-q)` (raw negation, the flag stays) | `neg.l`; `r.l=-q` and `r.l=0-q` with 1.5 give -2, `r.l=-q*2` gives -3 |
| shift, `AND`, `OR`, `EOR`, `MOD`, `Not` | `floor(q)` first, as today | today's code |

A Quick on both sides under a Quick type so far stays a Quick (not a cut case). Constant folding: a raw Quick only comes from `varcont`, whose elements are marked non-constant, so a mixed operation is never folded and `divzerochk` never runs on it (spike); `nocando` for the new library calls is harmless.

Overflow of the final Long wraps modulo 2^32 as `longmult` does (user decision 7).

### 4. A Long operand that sets the type itself (user decision 10)

`q*100000` with no type so far (`NPrint`, `If`): `q` sets Quick, the constant does not fit a Quick and turns the type Long, and `convtypef(Quick, Long)` at `eval.c:936` floors `q` late. Likewise `&var` and `?label`, which set Long without looking at the type so far. Decided (2026-11-21, "#1178 d refined"): "Bei "+" oder "-" ist die Long-Nutzung korrekt, Bei "*" oder "/" brauchen wir für optimale Genauigkeit die neuen Routinen. Wer das alte Verhalten will, soll die Quick-Variable in ein Int(q) kapseln." So at `eval.c:936`, with a Quick left and a Long right: `*` and `/` take the mixed path of decision 3 (`q*100000` with 1.5 gives 150000, today 100000); `+`, `-` and the other operators keep the late `convtypef` (for a whole `L`, `floor(q)±L` equals `floor(q±L)`: the decided "as today" and the rounding rule agree). A Long that comes first (`&var-q`, `100000-q`) makes the type so far Long before `q` is fetched: that is the `varcont` case of decision 3 ("&var ist wie ein Long"), so `100000-q` with 1.5 gives 99998.

### 5. A Float operand and `^`

A raw Quick that meets a Float (`r.l=q+f`, `r.l=l+q*f` with the Float inside the right side) or `^` is converted with `quicktofloat`, fraction included: variant C does not cut at fetch, and the model computes these exactly. The Float result is converted to the whole-number type by `SPFix` (towards zero, unchanged). `r.l=q+f` with `q=1.5`, `f=0.5` gives 2 (today 1); `r.l=q^2` gives 2 (today 1).

### 6. Where the conversion code goes: the proposal to keep the cut (planning choice)

The earlier planning agent proposed (`planning-order-tree-vs-fixes.md`, "Remarks", proposed, E2) to keep the cut at today's position where the consuming operator floors anyway (`+`, shifts, logic, comparisons) and to defer it only for `-`, `*`, `/`, Quick OP Quick and unary minus, so that fewer executables change. Options:

* **K1 (the spike's mechanism):** the flag everywhere; the cut is emitted by the consumer. For a raw right operand the consumer's cut comes right after the right operand's code, at the place `varcont` emitted it: the same bytes where the value does not change (`l+q`, `a(q)`, `r.l=q`). For a raw left operand the cut comes after the right operand's code: other bytes, same values (`a(i+1)` with a default-typed `i`, `r.l=q AND $fff0`).
* **K2 (the proposal as written):** cut at fetch when the consumer is `+`, a shift, a logic operator. Wrong for `+` with a raw left operand: the right operand is not known at fetch, and `q+p` under Long must give `floor(q+p)` (4 for 1.5 and 2.75), not `floor(q)+floor(p)` (3). Rejected.
* **K3 = K1 plus the safe part of K2:** cut at fetch when the element's consumer, known when `eval2` returns (the next operator if it binds tighter than the pending one, else the pending one), is a shift, `AND`, `OR`, `EOR` or `MOD` (today's value in every case). `+` with a raw left operand stays deferred.
* **K4:** a look-ahead over the source for the right operand's type (like `countpees`), so that `a(i+1)` keeps its bytes: a sixth patching-like mechanism that the tree must replay; rejected for its cost.

Choice (mine, planning): **K3.** Values are the same in K1, K3 and K4; K3 keeps the bytes for a left Quick of a shift or a logic operator at a few lines' cost, K4 would keep more bytes but adds a mechanism that tree steps 1-4 must reproduce. The left `+`/`-` cases (`a(i+1)`, `r.l=q+1`) change their code order in every option except K4; the prev comparison (task 3.3) counts them. If the count turns out large and the user wants fewer changed executables, K4 is the option to come back to. Rating in "Planning choices".

### 7. Consequences for #1177's fix

* `cmpownleft`'s Quick-to-Long case (`from == T_QUICK && to == T_LONG`) has no conversion to take back any more; the raw flag gives the left type Quick directly. The case is removed (or kept consistent with the flag); `compiler-quick-compare` (#1177's test) must stay FIXED.
* A compound left operand under a Long type so far is still computed in that type (#1183), but its operations now round per decision 3: `r.l=(q*2>2)` with 1.5 gives -1 (3 > 2; `quick-compare-types` spec scenario: 0 "as before"), and `-q` on the left keeps the raw value (`r.l=(-q<-1)` gives -1, the correct value). The #1183 limit remains for an operation whose fraction is rounded away (`r.l=(q*2>2.4)` with 1.25: 2 > 2.4, 0; correct -1).
* `tests/bug-orig/compiler/compiler-compare-compound-left` (#1183) uses `q=1.5`, where `q*2` and `q+q` are whole: with this change all three lines give the expected -1 and the bug-orig test would come out FIXED without #1183 being fixed. Its inputs change to values whose fraction survives (for instance `q=1.25` with `q*2>2.4`, `q+q>2.4`, `q*2=2.5`), `.expect` and `.orig` (recorded from the CD's compiler, `bug-orig-record`) follow; it stays REPRODUCED. Named in the commit (rule 3: an intended change of a test).
* The #1177 limit texts (`docs/blitz2/expression-types.md` "Limits until the expression tree", the #1177 row of `docs/blitz2/deviations.md`, the requirement "Known limits of the left operand until the expression tree" of `quick-compare-types`) get the new examples; the `-q` limit goes.

### 8. Library routines: where, and compatibility (user decision 3)

`l*q`, `q*p` need a 64-bit intermediate and `l/q` a 48-bit dividend; today's `qmultiply`/`qdivide` cannot do it beyond +-32767. Decided: new entries in the system libraries `lmullib`/`ldivlib` (the headers end at `$CA02`/`$CB02`, `libs/acidlibs/lmullib.c`, `ldivlib.c`), with the register contract of the neighbours, an entry in `docs/libs/deviations.md`: a program compiled by the port needs the port's libraries. `q/l` and `q/h` use the existing `longdiv` (decision 3).

### 9. Results and comparison

Changed executables: those in which a Quick is fetched under a Long type so far and its consumer is not a shift or a logic operator (decision 6), those with a late cut by a Long constant, `&var` or `?label` under `*` or `/`, and the library files `lmullib`/`ldivlib` (and whichever disk libraries of the build carry them). Named in the commit message with `PREV_INTENDED`, with the list from the prev comparison. Changed program outputs: the 18 places of the issue, the negation, `^` and Float cases of decisions 3-5 where a fraction is present (counted in task 3.3; the model's 358/18/0 predates the rounding decision and did not model negation and `^`). The corpus runs (`corpus`, `corpus_runerr`, the CD corpus `blitz2-cd.tsv`) and an executable-level comparison over more sources check the rest.

### 10. Cost check

Runtime: `l+q` unchanged, `l-q` a few instructions more, `*` and `/` a library call as today but with a 64-bit step. The library benchmark (`ctest --preset perf`, `tests/perf/budgets.txt`, D-007) gets lines for the new entries, measured against the existing `lmullib`/`ldivlib` lines and the original's target; an existing line must not rise. Compile time: measured by the corpus (not expected to matter).

### 11. AB3 mode (user decision 1)

blitz2 and blitz2-ab3 link one core; the new rule applies in AB3 mode too, no mode condition; `docs/ab3/variant.md` gets a deviation row, the affected oracle tests in `tests/ab3` are marked.

### 12. Documentation, deviation, site

* `docs/blitz2/expression-types.md`: the exception section, the "type so far" list, the Float/Quick table (a Quick column after the fix), the paragraph "Planned in #1178" (becomes the rule), the measured examples with "Before"/"After", the #1177 limits (decision 7), and "Where this is a problem": arithmetic becomes "original bug #1178, fixed in the port" (the section still says #1178 keeps the current behaviour). The comment at `eval.c:3036` becomes `Fixed (#1178)`.
* `docs/blitz2/deviations.md` (exists): the kind-F entry for #1178 and the changed #1177 limits; `docs/libs/deviations.md` for the new library entries; `docs/ab3/variant.md`: AmiBlitz3 v3.14.0 keeps the old rule.
* The site section "The type of an expression" (`site/content/docs/how-blitz2-works/compiling.md:147-151`, which says the port keeps the rule) and the fixed-bugs entry; the overview row and label are the coordinator's step.

### 13. Tests: the expression matrix (change `expression-type-matrix`)

The matrix (spec `testing/expression-matrix`; `tools/exprmatrix/README.md`, "The committed tests") is generated by `exprmatrix expect` from `tests/corpus/tests-exprmatrix/matrix.txt` and `measured.tsv`; its files are not edited by hand (`exprmatrix_committed`). What it holds for #1178 on main (c336cf473):

* **Bug-orig tests of the cells of class `open-1178`:** 26 tests `tests/bug-orig/compiler/compiler-expr-1178-CTX-SET[-u|-c]-NN` (header `BUG(orig) (#1178)` and the cells; `.expect` the model's fixed rules, `EM_R_1178` in `tools/lib/exprmatrix/model.c`, whose rules name their entries: variant C, the rounding rule, "#1178 d refined", the `longmult` overflow; `.orig` recorded from 1.95, REPRODUCED). 4 tests `compiler-expr-1178-1189-*` hold the cells of class `open-1178+1189` (a Quick base of `^` in value set B): they come out DIFFERS after this fix and are marked fixed by #1189's fix (their header says so). The operators of the cells: `+ - * /`, unary minus, `^` and `MOD` with a Float; no shift, logic operator or `Not` (unchanged, decision 3).
* **Corpus passes** `corpus_exprmatrix_NAME` and `corpus_exprmatrix_runerr_NAME` for the agreement (`agree`), baseline (`base`) and fixed cells (`fixN`, pairs `NAME.orig.out` / `NAME.fixed-N.out` of #1177 and #1187). None of their cells is a #1178 cell: a cell whose value the fixed rules change is `open-1178` whatever its value class. So this change must leave every corpus file of the matrix as it is. The baseline cells are the original's measured values kept as a regression check, not decided behaviour (`task-decisions.md` 2026-11-22, "Matrix questions Q-A, Q-C, Q-D, Q-E", the correction by the user: "dass danach die Messergebnisse aktualisiert würden"); the #1178 cells with a baseline value class (an overflow, `MOD`, `^`) are `open-1178` cells and are named in the fix commit as the baseline cells #1178 changes.
* **Left out** (`left-out.txt`): no #1178 cell.

The cases of spec `compiler/expression-types` that the matrix does not hold (it has two operands without brackets, variables and literal constants only: proposal of `expression-type-matrix`, "Not in scope") go into a hand-written bug-orig test beside it (task 1.2).

**After the fix** `EM_R_1178` joins `EM_RULES_PORTNOW` (`tools/lib/exprmatrix/exprmatrix.h`, "the port of this commit") and `expr-matrix-update` regenerates. Where the model does not know the fixed port's value (`?`: a Float power from a Quick with its fraction), `expect` takes the port's columns of `measured.tsv`, which hold the port before the fix; those rows are measured again with the fixed port (`corpusrun pass --observe`, as task 1.3 of the matrix did, `classify -m`); the originals' columns stay.

**The generator at a fix: answered, T1 (user decision 13).** Today `expect` (`expect.c`, the classification per mode) classes a cell with original != expected = port as `fixed-N` and writes it into a corpus source `CTX-SET[-u|-c]-fix1178-NN` with `NAME.orig.out` / `NAME.fixed-1178.out`, removes the bug-orig tests it no longer writes, and leaves a cell with original != port != expected (the `1178+1189` cells after this fix) out. That contradicts the spec `testing/expression-matrix` ("A fix of an open bug": the bug-orig tests of the cells of #1178 come out FIXED; "A cell of two open bugs": the header names the fix that turns it FIXED) and the headers the generator writes; `tools/results fix-scope` would report the removed open bug-orig cases (`libscmp_scope.c`, `own()`). The recorded test form "A: Alte Form, später umziehen (Recommended)" says "agreement and fixed cells as a corpus pass"; whether it covers cells fixed after the matrix was committed is not clear (D-011 step 2). Options:

* **T1 (recommended; the tasks are written with it):** `expect` keeps an open cell of an issue fixed after the matrix (`EM_R_1178` and later; #1177, #1186, #1187 keep their corpus pairs) in its bug-orig test and writes `; @fixed #N` into its header; `.expect` and `.orig` stay; a two-issue test stays without `@fixed` (DIFFERS) until the second fix. A commit of its own before the fix (task 1.2), with a snitch test. The fix's regeneration then changes only the 26 headers; `fix-scope` counts an added `@fixed #1178` header as cases of #1178 (spec `testing/bug-fix-workflow`). The move to the BASIC test model (round U4) is unchanged.
* **T2:** the generator as it is: the cells go into about 30 corpus sources with pairs, the 30 bug-orig tests are removed (named in the fix commit; `fix-scope` reports them), the 32 `1178+1189` cells leave the tests until #1189 unless the generator changes for them; the spec `testing/expression-matrix` gets a delta for the two scenarios (another change's spec).
* **T3:** T1 in the fix commit, the move into corpus pairs later (with #1189 or in U4): two generator rules in turn.

`Point: the fixed #1178 cells stay in their bug-orig tests with @fixed (T1) where the user meant them to move into corpus pairs — L2 Possible / I2 Medium / E3 Proven (expect.c, the classification per mode; libscmp_scope.c, own()) → Plan`; by step 2 (the reach of the recorded test form is unclear; the generator contradicts the spec) the choice is the user's. T2 re-plans the spec of `expression-type-matrix` beside this change: I4 High; no option is I5 (no result of the build changes by the choice).

## Decisions of the user

From `~/.local/share/bb2-c-work/task-decisions.md`.

1. **AB3 mode** (2026-11-21, "quick-long-arithmetic: first open questions"): "Neue Regel auch in AB3 (Recommended)".
2. **#1177 first, the cut removed here**: "Zwei, #1177 zuerst (Recommended)"; "varcont: "quick-long-arithmetic (Recommended)"" (2026-11-21, "quick-compare-types (#1177): open questions"). #1177 is merged (fe1d7b357).
3. **Routines**: "Neue Einträge in lmullib/ldivlib (Recommended)".
4. **Classification**: "Ja, als Original-Bug fixen (Recommended)" (labels `bug-orig`, `wrong-result`, the fix workflow in full, severity 1); `tests/blitz2/quick-arith-long.bas` gets the new expected values.
5. **Division** Long/Quick: "Wie BB2s Long-Division (Recommended)": towards zero (measured, spike section 1), so `10 / -1.5` is -6.
6. **Deviation entry** in `docs/blitz2/deviations.md` ("docs/blitz2/deviations.md anlegen (Recommended)", then "Abschnitt 5 dorthin verschieben (Recommended)"); the file exists.
7. **Overflow** of a mixed product: "Wie longmult (Recommended)".
8. **Rounding** (2026-11-21, "#1178 rounding rule decided"): "Zwischenlösung: / gegen Null (Recommended)": division towards zero, everything else (`+ - *`, negation, the conversion Quick to Long/Word/Byte) floor; replaces the earlier answers and the answer to case e: with `q=1.5`, `r.l=-q` and `r.l=0-q` give -2.
9. **Quick/Quick division under a Long type so far** (2026-11-21, "#1178 c"): follows the division rule; `r.l=q/h` with -3.0/2.0 gives -1.
10. **A Long operand that sets the type** (2026-11-21, "#1178 d refined"): `+`/`-` as a Long operation as today, `*`/`/` with the new routines, the old behaviour by `Int(q)`.
11. **Order** (2026-11-22, "Order of the Quick work; matrix test"): "A: Fixes zuerst (Recommended)"; the matrix test "Ja, vor #1178 (Recommended)", the regression test of #1178 and the tree.
12. **Word+Word then `+ q` without a destination** (2026-11-22, "#1178 Q1"): "Zu #1178: Wichtig ist nur, dass nach dem Expression Tree dort 4.5 rauskommt": 4 and 8 in this change (interim, section "Interim value handed to the expression tree").
13. **Where a fixed issue's matrix cells live** (2026-11-22, `task-decisions.md`, "Go for steps 1-7"): "T1: in ihren bug-orig-Tests (Recommended)"; for every fix ("Last answers of the session", "T1 for all fixes"): "\"Ja für alle\" - zumindest solange bis die Test-Infrastruktur fertig umgebaut ist".

## Planning choices (mine, not the user's)

* Decision 6, K3: `Point: keep the conversion at fetch only where the consumer is a shift or a logic operator; left +/- operands change their code order — L2 Possible / I2 Medium / E2 Reasoned → Plan`.
* Decision 3, Quick OP Quick under a Long type so far gives a whole number at the operator, computed without a Quick intermediate: `Point: per-operation rounding for q+p, q*p as for l+q (variant C, its model, case c) — L1 Unlikely / I3 Observable / E2 Reasoned → Plan`. The alternative of the spike's section 4 (a raw Quick result, cut at the end) lets `q+p` and `q*p` overflow the Quick range where today's Long computation does not: whole-number Quicks would change their results unintended (I5 Critical; not recommended).
* Decision 7, new inputs for `compiler-compare-compound-left`: `Point: #1183's bug-orig test would turn FIXED by this change — L3 Likely / I1 Low / E2 Reasoned → Monitor` (a task covers it anyway, 1.4).
* Decision 3, `l/q`, `q/l`, `q/h` computed directly as truncated quotients: by the spike (E3), the consequence of decision 5; prescribed, no rating.
* Decision 13, the cases the matrix lacks in one hand-written bug-orig test `tests/bug-orig/compiler/compiler-quick-long-arith` beside the matrix (not in `matrix.txt`, which has no three-operand cells): `Point: the name or the split of the hand-written test turns out wrong — L1 Unlikely / I1 Low / E2 Reasoned → Monitor`.
* Decision 13, `EM_R_1178` joins `EM_RULES_PORTNOW` in the fix commit (the rule set names the port of its commit; `exprmatrix_committed` fails in any other split): `Point: tool code in the fix commit beside the compiler change — L2 Possible / I1 Low / E2 Reasoned → Monitor`.
* Decision 13, the generator change of T1 in this change (task 1.2) and not in `expression-type-matrix`: the matrix change is merged before this one starts (recorded order); its open tasks 3.5 and 5.1 edit `tools/lib/exprmatrix/expect.c` and `tools/exprmatrix/README.md` first. `Point: the generator change lands in the wrong change, or meets an unmerged matrix task in the same files — L2 Possible / I2 Medium / E2 Reasoned → Plan`.
* Decision 13, a #1178 cell whose fixed port differs from its `.expect`: a contradiction between the model's rule (and the entry it names) and this design or the implementation, reported (rule 2), not settled by editing the model to the port's value: prescribed by rule 2, no rating.

## Interim value handed to the expression tree

**A Long type so far without a destination** (2026-11-22, `task-decisions.md`, "#1178 Q1": "Zu #1178: Wichtig ist nur, dass nach dem Expression Tree dort 4.5 rauskommt"): in this change Word+Word makes the type so far Long, so `+ q` gives a Long by decision 3: `NPrint 1+1+1+q` gives 4 and `NPrint w+v+q` gives 8 (`q=1.5`, `w=3`, `v=4`; today's values), while `NPrint q+w+v` stays 8.5. This is an interim value. A requirement handed to the expression tree (#1180), not part of this change: after the tree, `NPrint 1+1+1+q` with `q=1.5` gives 4.5 (no destination asks for a whole number).

## Risks

* The flag must reach every consumer; a missed one floors the wrong way or uses the raw value as a Long (65536 times too large). The spike's prototype found the list above by 40 consumer cases; the matrix test is the guard (decision 13: its #1178 tests turn FIXED, its corpus passes keep their values). Its expected #1178 values are the model's only: no patch checked them (`expression-type-matrix` task 3.4).
* `l-q` as `l - ceil(q)`: the edge Quick values (-32768.0, raw values near the top) need care in the inline code.
* Code size: the new calls and `l-q` make compiled expressions longer; executables' sizes change.
* The function-optimizer change (`compiler-function-optimizer`) touches the same file; not in parallel.
* The tree (#1180) must reproduce this change's bytes in its steps 1-4; K3 adds one rule (cut at fetch for a shift or logic consumer) to replay.

## Open questions

1. The matrix's #1178 cells after the fix (decision 13): answered by the user (`task-decisions.md` 2026-11-22, "Go for steps 1-7": "T1: in ihren bug-orig-Tests (Recommended)"; "Last answers of the session", "T1 for all fixes": "\"Ja für alle\" - zumindest solange bis die Test-Infrastruktur fertig umgebaut ist"): T1.
2. **The new library entries and the other library sets** (found by the implementation, 2026-11-22; open, the user's). The new entries $CA02/$CA03 (lmullib) and $CB02 (ldivlib) of decision 8 exist only in the port's libraries. A program the port compiles with another library set that has a mixed `*` or `/` fails to compile with "Token Not Found : $CA02." (or $CA03, $CB02): with the CD's libraries 12 of the CD's 724 sources (`corpus_cd`, and the same 12 in `corpus_cd_ab3`; e.g. `EXAMPLES/BLITZMODE/DISPLAY_TEST.BB` line 44 `flags.l=bp+smooth*$10+...`, a default-typed Quick times a constant under a Long), the `port` configuration (CD's libraries) of every bug-orig test with such an expression (`compiler-compare-compound-left`, the `compiler-expr-1178-1189-*` tests, which stay open), and AB3 mode, which loads AmiBlitz3's libraries (user decision 1 asks for the new rule there). Decision 8 said "a program compiled by the port needs the port's libraries" for running; that compiling fails with any other set was not named. Measured with the implementation on the branch (C libraries: the 26 matrix tests and `compiler-quick-long-arith` FIXED). Also: a whole-number default-typed Quick times a whole number under a Long (`i*4`, the address idiom) now calls mixmult (four MULU) instead of longmult (one MULU for small factors): same value, slower (not measured yet, task 3.2).
