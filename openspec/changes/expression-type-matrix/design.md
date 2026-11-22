# Design

## Context

See proposal.md, "Why". The current state that shapes the design:

* **Typing** (`docs/blitz2/expression-types.md`): one "type so far" per expression, started by the context (destination, parameter, Long for the first index, Word or Long for a later index by runtime error checking, none for `If`/`Print`), promoted by each fetched value; a Quick under a Long type so far is cut (`varcont`, #1178); comparisons start again (fixed in the port, #1177, with its limits #1182, #1183, the `-q` left limit and the Float constant left limit).
* **Test forms available now.** Old form: a bug-orig test (`ID.bas`, `ID.expect`, `ID.orig` recorded from 1.95 by `bug-orig-record`) is one bug and one verdict per test (FIXED, REPRODUCED, DIFFERS, BROKEN; `tests/README.md`, "bug-orig tests"); a corpus pass compiles many programs in one session and judges each against `tests/corpus/expect/PASS/NAME.out` with the port in `all` and with the CD's 2.10 (`ref`), the port with the CD's libraries and `orig-clib` in `originals`; a fixed program is the pair `NAME.orig.out` / `NAME.fixed-N.out` ("Corpus"). The BASIC test model (per-check verdicts, axes, declared deviations) is not usable yet: `basic-test-runner` tasks 2.2, 2.3, 3.1 (runner phase 2) are open, and phase 2 waits until after the Quick fixes (`task-decisions.md` 2026-11-21, "Quick fixes first; #1177 test in the old form; #1181 first").
* **AmiBlitz3**: `ab3_cli_case(NAME SOURCE REF REF_EXPECT FILE)` in `tests/ab3/CMakeLists.txt`; `REF` cases run AmiBlitz3's compiler in `oracle` (blocks of 70 cases, 600 s per session, no JIT); the exe side runs blitz2-ab3 in `all`.
* **Prior measurements** were made with Python generators in the work directory (`tr-1177-float-context/gen*.py`); 2.10, 1.95 and the port emitted the same code for the 93 comparison cases.
* **Limits of a source**: the default Object Buffer is 16384 bytes (`docs/architecture.md` 5.2), the bug-orig session limit `BUG_ORIG_TIMEOUT` 90 s, the silence check 8 s after the first run. Code bytes and time per cell are not measured (task 1.2).

## Goals / Non-Goals

**Goals:** every cell's value known for the four compilers; every cell's expected value traceable to a rule or an answer; a fix of one issue turns exactly its cells green.

**Non-Goals:** see proposal.md, "Not in scope". Also: no new test form, no change of the corpus runner or the bug-orig runner.

## Size of the matrix (estimate, E2 from the dimensions)

10 operand kinds (5 variables, 4 constants, 1 string). Binary numeric cells: 9 x 9 ordered pairs x 19 operators x 13 contexts = 20,007 per value set; unary: 9 x 2 x 13 = 234; strings: about 80. Two value sets (A: inside every range, positive fraction; B: negative, beyond Word and Quick range) give about 40,000 cells. At an estimated 150-250 cells per source (Object Buffer 16 KB; E1 until task 1.2) that is 160-270 sources. A reduction is decision 2.

## Decisions

Each decision gives the step of the decision process (D-011) and, where rated, the rating of the recommended option. "Planning's choice" marks a Plan or Monitor choice taken by the planning agent (D-011 (b)); it is not the user's.

### 1. A capability of its own: `testing/expression-matrix`

The requirements are about a test, not about the compiler's behaviour (that is `compiler/expression-types`, created by `quick-compare-types` / `quick-long-arithmetic`). Alternative: requirements in `compiler/expression-types`; rejected, a test structure is not compiler behaviour.
`Point: capability boundary turns out wrong — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor` (planning's choice).

### 2. Cut of the matrix and of the files (Decide)

Two questions:

**(a) How much of the product.** The user's wording names the full product (type x operator x context x order). A reduction departs from that wording: Decide (step 2).

* **F, full:** about 40,000 cells, 160-270 sources; the corpus pass grows by that many programs (several passes, each one session), the bug-orig tests by one per bug and file; `all` gains an estimated 30-60 minutes (E1). Measurement with four compilers: four times that.
* **R, reduced by the type so far (recommended):** the full product of the 81 ordered pairs x 19 operators for the six contexts that start a different type so far (destinations Byte, Word, Long, Quick, Float, and `Print`, which starts with none), value set A, and value set B only for the operators where sign and range decide the result (`+ - * / MOD ASR` and unary minus); for the seven other contexts (four parameters, the two index contexts, `If`, which also starts with none) the product of the kinds Word, Long, Quick, Float and the fraction and Long constants (6 x 6 pairs) x 19 operators, value set A. About 9,234 (A) + 3,402 (B) + 7 x 684 = 4,788, together about 17,400 cells, 70-120 sources.
* **S, sample:** pairwise coverage of the dimensions, about 2,000 cells; misses cells a fix changes by accident.

Recommendation R, with the numbers made exact by task 1.2 before the user is asked (E1 now; the short measurement that raises it is task 1.2: code bytes and compile and run time per cell with 1.95 and the port, on one source of 200 cells).
`Point: reduction R misses a cell that a fix changes — L2 Possible / I3 Observable / E1 Assumption → Decide`. No option is I5.

**(b) Files.** One source holds the cells of one context, one class (decision 6) and one operator group, at most N cells, N half of what fits the default Object Buffer by task 1.2's measurement, and at most 60 s of compile and run with 1.95. Planning's choice:
`Point: N too large, a source overflows the Object Buffer or the session limit in a slow run — L2 Possible / I1 Low / E1 Assumption → Monitor` (the generator splits by N; a smaller N is a regeneration).

### 3. Generated by a C tool

Hand-written sources are not workable for thousands of cells; a generator that produces committed files is infrastructure, so it is C with its logic in `tools/lib/exprmatrix/` and snitch tests in `tools/tests/` (CLAUDE.md rule 6, D-002: step 1, prescribed). The program `exprmatrix` (CLI11, subcommands per `docs/development.md`, "Host programs"): `generate` (sources from the description `tests/corpus/tests-exprmatrix/matrix.txt`), `classify` (reads the measured outputs, writes the cell table and the question list), `expect` (writes the expected files). The generated files are committed: a target `expr-matrix-update` writes them into the source tree (reviewed with `git diff`, like `corpus-expect-update`), and a host test `exprmatrix_committed` compares the committed files with a fresh generation (with a broken-input test: a changed file). Alternative: generate at build time into the build directory; rejected, the bug-orig and corpus globs read the source tree, and the `.orig` files must sit next to their sources.
`Point: committed generated files instead of build-time generation — L2 Possible / I2 Medium / E2 Reasoned → Plan` (planning's choice).

### 4. Test form (decided by the user)

Decided 2026-11-22 (`task-decisions.md`, "#1185 fix; matrix test form"; recorded answer for this change): "A: Alte Form, später umziehen (Recommended)". So:

* open cells in bug-orig tests per issue (`tests/bug-orig/compiler/compiler-expr-N-...bas`, `.expect`, `.orig` recorded from 1.95);
* agreement and fixed cells in a corpus pass `corpus_exprmatrix` (and `corpus_exprmatrix_runerr` for the index cells with checking on), judged with the port in `all` and with 2.10 in `originals`, fixed cells as `NAME.orig.out` / `NAME.fixed-N.out`;
* moved by changing the generator when `bug-orig-basic-tests` reaches round U4.

Not chosen: B, the BASIC test model first (needs runner phase 2 before the Quick fixes); C, agreement cells as port regression tests without a run of the original.

### 5. Measurement and where the originals' values are kept

Task 1 runs every source with 2.10 (`c210`), 1.95 (`c195`), the port with the CD's libraries (`portcd`) and with the C libraries (`clib`) through `corpusrun pass --observe`, as `tr-1177-float-context.md` did, and AmiBlitz3 through a temporary `ab3_cli` list on the branch; the cell table goes to `~/.local/share/bb2-c-work/expr-matrix/`. Committed copies of the originals' values: `ID.orig` of the bug-orig tests (1.95, by `bug-orig-record`), the corpus expected files (checked against 2.10 in `originals`) and AmiBlitz3's `REF_EXPECT` files. No committed table of all four compilers: a committed table read by a test would be expected data from the original and need a recording target of its own (`testing/test-model`, "Recording and testing are separate"). The model's reproduction of the measured values (decision 7) is checked once in task 2.3, not permanently.
`Point: without a committed four-compiler table, a later change of the model cannot be rechecked against 1.95 for agreement cells — L2 Possible / I1 Low / E2 Reasoned → Monitor` (planning's choice; 2.10 is checked permanently by `originals`).

### 6. Classes and marking of open bugs

`exprmatrix classify` gives each cell: **agree**, **fixed-N**, **open-N** (or **open-N+M** for a cell that needs two fixes), **port** (port differs from the originals without a fix: a port defect, own issue by D-021), **new** (port = originals, the expected value differs and no issue covers it: a new original bug, reported with its D-021 class; the coordinator creates the issue, D-008), **question** (no expected value), **noncompile**. The bug is attributed by the model: it computes each cell twice, by the original's rules (which must reproduce the measured 1.95 value, decision 7) and by the rules with the fixes; the first rule that differs names the issue (the cut at fetch: #1178; a later index as a Word: #1179; a comparison as right operand: #1182; a compound left operand: #1183). Open cells of one issue go into its bug-orig tests, whose header names `BUG(orig) (#N)` and lists the cells; the fix commit adds `; @fixed #N` (`tests/README.md`, "Fixed tests"). A test of two issues (`open-N+M`) is named after both and comes out DIFFERS between the two fixes (the port then neither reproduces 1.95 nor meets the expectation); its header says so and names the fix that adds `; @fixed`. #1185 has no value cells: its fix is the compile error "Expression too Complex" when an operator would need a register beyond d7 (`task-decisions.md` 2026-11-22, "#1185 fix; matrix test form": "Compile-Fehler jetzt (Recommended)"). Two-operand cells are not expected to reach that depth (E2); a cell that does is classed `noncompile` after that fix and left out, and the spike (task 1.2) reports any such cell.
`Point: DIFFERS between two fixes is read as a regression — L2 Possible / I1 Low / E2 Reasoned → Monitor` (planning's choice; named in the header and in tests/README.md).

### 7. Expected values: the model and its rules

The model (`tools/lib/exprmatrix/model.c`) computes type so far, conversions and values with integers (Quick as 16.16, Byte/Word/Long two's complement); for Float operands it does not compute FFP arithmetic: a cell whose Float part follows the same rule in the original and in the fixed rules takes the original's measured value (step 2 below). Order of derivation for a cell (spec, "The expected value comes from a recorded rule"):

1. **A recorded rule.** The rules and the entries they follow (headings and dates in `task-decisions.md`; quotes there):
   * comparisons with their own values, exact Long/Quick comparison, boolean -1/0 in the context's type, and the accepted limits (compound left #1183, `-q` left, a Long constant left under Float): "quick-compare-types (#1177): open questions", "#1177: right operand as planned; Float destination measured first", "#1177: Float context measured wrong", "#1177 Float constant left; new bugs as issues", "#1177: `-q` left limit accepted";
   * Quick arithmetic by variant C (no cut at fetch, the operation with the fraction, constants take the type so far, order dependence as today): "#1178 direction: variant C; expression tree planned";
   * rounding: division towards zero, everything else (+ - *, negation, the conversion Quick to Long/Word/Byte) floor: "#1178 rounding rule decided" ("Zwischenlösung: / gegen Null (Recommended)");
   * a Long operand (Long constant beyond the Quick range): `+`/`-` as a Long operation, `*`/`/` with full precision: "#1178 d refined; e questioned";
   * a mixed product beyond 32 bits wraps as `longmult`: "quick-long-arithmetic: first open questions" ("Wie longmult (Recommended)");
   * every array index a Long: "#1179, CRC findings, trailers, merge" ("Keine Prüfung im Release (Recommended)");
   * a comparison as right operand does not lower the left operand: "#1177 Float constant left; new bugs as issues" (#1182 created as a bug-orig issue; the expected value is the operation in the left operand's type, as the issue states).
2. **The original's value** when it is the exact result of the operation in the type the original computes it in, converted to the context as the original converts (Float to whole numbers towards zero by `SPFix`, measured, `expression-types.md`): agreement (D-003: functional compatibility; nothing to fix).
   `Point: an original value that is exact in its type is taken as expected, the user would call the type itself wrong — L1 Unlikely / I3 Observable / E2 Reasoned → Plan` (planning's choice).
3. **Otherwise a question.** "Exact" is defined only for `+ - *`, comparisons and unary minus over the operands' values, for `/` with the rounding of the rules, and for `MOD`, `AND`, `OR`, `EOR`, `Not` and the shifts over whole-number operands in the result type; overflow of the result type has no rule except the mixed product. Expected question classes: overflow of a whole-number result or destination (wrap?); bitwise operators, `Not` and shifts with a Quick or Float operand or a fractional count; `MOD` with negative or fractional operands; `^` (power) with whole-number types; order pairs whose expected values differ after variant C (the tree's goal "base+q*4 muss das gleiche ergeben wie q*4+base, aber das machen wir in einem späteren Schritt" names no test value before #1180; one datum, for its own case: "#1178 Q1 (Word+Word then + q without a destination)", "Zu #1178: Wichtig ist nur, dass nach dem Expression Tree dort 4.5 rauskommt": `NPrint 1+1+1+q` gives 4 until the tree and 4.5 after it. That expression has three operands and is outside the matrix; whether its rule, today's value pinned until the tree and the tree's value recorded for #1180, applies to the two-operand cells of the class is asked with the class, D-001); cells where 2.10, 1.95 and AmiBlitz3 disagree. `classify` writes them as `expr-matrix-questions.md` (one question per class, three example cells with the four values, the options); the coordinator puts them into the open-questions file.

Cells of classes still open are not committed (spec). The model's rules are a reading of the quotes; where they differ, the quote wins (D-001), and each rule in the code names its entry.

### 8. Values and output form

Value set A: `b=3`, `w=1000`, `l=100000`, `q=1.5`, `f=2.5`, constants `3`, `1000`, `100000`, `1.5`; value set B: `-3`, `-1000`, `-100000`, `-1.5`, `-2.5` and the negative constants. The right operand of a shift and of `^` is 2 (A) or 3 (B) in its kind; divisors are never 0; index cells read a `Dim` of 300 elements (`a(i)=i*16+1`, so the printed element names the index). An index cell's result cannot always stay inside the `Dim` (a Long constant is beyond 32767; comparisons, unary minus, `NOT` and differences give negative indexes; the spike, task 1.2). So, coordinator's Plan choice (I2, `task-coordinator-decisions.md`): in the unchecked build an index cell is measured and tested only where it addresses inside the `Dim`; the out-of-range index cells (negative, beyond the `Dim`, Long constants) only in the checked build, where the expected result is the runtime error "Array Subscript out of Range". In the measurement a cell counts as inside where the checked build of that compiler reads an element; in the unchecked run a printed value that is no element (`i*16+1`, `0 <= i <= 300`) marks the cell as outside too.
`Point: an unchecked read beyond the array gives a value that depends on the heap, or touches memory it must not — L3 Likely / I2 Medium / E3 Proven (spike: i2-sub-q-l-I 0 with 1.95, 1324285880 with the port) → Plan` (coordinator's choice). No value whose result is the FFP zero, to avoid the CD/C library difference #146 between `portcd` and `clib`. Output: `NPrint "KEY ",value` for whole-number results, `NPrint "KEY ",Hex$(Peek.l(&t))` for Quick and Float destinations and parameters (the parameter procedures print their argument that way), `If ... Then NPrint "KEY T" Else NPrint "KEY F"`, and `NPrint "KEY ",a OP b` for `Print` (the library's formatting is the cell). Planning's choice:
`Point: a value set misses the difference a fix makes (for example a fraction of .5 hides a rounding direction) — L2 Possible / I1 Low / E2 Reasoned → Monitor`; the description file can add value sets later.

### 9. AmiBlitz3

The agreement and fixed sources are `ab3_cli` cases with `REF` and `REF_EXPECT`: EXPECT is the port's (= expected) output, `REF_EXPECT` AmiBlitz3's measured output; a cell where they differ (the #1177 cells; others are reported) gets a row in `docs/ab3/variant.md`. Open-cell sources join `tests/ab3` in their fix's commit (as `quick-long-arithmetic` task 2.4 marks the oracle tests). The task checks first that both compilers take the generated source as a `.bb` source; otherwise the generator writes an `.ab3` variant. Checked in task 1.2: blitz2-ab3 takes the `.bb`; AmiBlitz3 v3.14.0 does not ("Garbage at the end of line" from the first `NAME.TYPE=` on) but takes the same text as `.ab3`, so AmiBlitz3 gets the `.ab3` variant. It refuses some cells 1.95 accepts (`1.5 LSL n` in a Byte destination), so its sources are fitted on their own (its refused cells recorded and left out).
`Point: the ab3_cli cases add blocks to all and oracle — L3 Likely / I1 Low / E2 Reasoned → Monitor` (planning's choice).

### 10. Overlaps with open changes and order

The order against #1178 is the user's (step 1, "Order of the Quick work; matrix test": "Ja, vor #1178 (Recommended)"). Files that this change and another open change both edit:

| File | Other changes |
|---|---|
| `tests/README.md` | `basic-testlib`, `basic-test-runner`, `bug-orig-basic-tests`, `blitz-coverage`, `coverage-ted-libs` |
| `tests/ab3/CMakeLists.txt` (cases) | `quick-long-arithmetic` (2.4), `ab3-mode-g5`, `blitz-coverage` (`tests/ab3/cli`) |
| `docs/ab3/variant.md` | `quick-long-arithmetic`, `ab3-mode-g5`, `compiler-function-optimizer` |
| `docs/blitz2/expression-types.md` | `quick-long-arithmetic` |
| `tools/CMakeLists.txt`, `tools/testinfo/rules.txt` | `basic-test-runner` (4.3), `blitz-coverage` (`tools/blitzcov`) |
| `tests/corpus/CMakeLists.txt` | `ab3-mode-g5`, `basic-test-runner` (`tests/corpus/runner`, not the pass list) |
| `tests/bug-orig/compiler/` (new files) | `bug-orig-basic-tests` (round U4 moves them) |

None of these is planned to run at the same time except `quick-long-arithmetic`, which starts after this change (recorded order). The edits are in different sections (a pass, a case list, a table row).
`Point: textual merge conflicts with another change touching the same file — L2 Possible / I1 Low / E2 Reasoned → Monitor` (planning's choice). `quick-long-arithmetic` (plan merged in 0a94a890f) builds on the matrix and names it "once it exists"; its tasks 1.2, 3.1, 3.4 and 5.1 need the matrix's name and files (the `; @fixed #1178` headers, the corpus pairs): updated by `openspec-update-change` after this change (task 6.2); the update of another change is I2:
`Point: quick-long-arithmetic plans its test without the matrix — L3 Likely / I2 Medium / E3 Proven (its tasks.md) → Plan` (planning's choice).

## Risks / Trade-offs

* [The model reproduces 1.95 wrongly for a class of cells] → task 2.3 requires the model's original-rules value to equal the measured 1.95 value for every cell; each mismatch is a model error or a new find, resolved before an expected value is set.
* [The question list is long] → one question per class, not per cell; classes the user answers become rules of the model with the answer's quote.
* [Test time in `all`] → the cut (decision 2) and N (2b); the open bug-orig tests run only in `bug-orig`.
* [A generated source triggers a different compiler path than hand-written code (a long source, many labels)] → the spike (task 1.2) compares 20 cells alone with the same cells inside a full source.
* [AmiBlitz3's compiler crashes under load] → its sessions run without the JIT as all `REF` cases (#976).

## Migration Plan

Additive. Rollback: remove the pass, the cases and the generated files; nothing else depends on them until `quick-long-arithmetic` uses them.

## Findings

Task 1.1 (`tools/exprmatrix`, `tools/lib/exprmatrix`; the format in `tools/exprmatrix/README.md`):

* The description names value sets (`values`, `counts`) and blocks (an operator group: contexts, operators, kinds, sets, or single `cell` lines); a block is the product of its parts, both orders of every pair of its kinds. A source holds one block, set and context, at most N cells (`cells-per-source`, `--cells`); class (decision 2b) is not known before `classify`, so it is not yet a split criterion.
* The combinations BASIC gives no meaning (a string with a number, a string under an operator other than `+` and the comparisons, a string `+` outside `print`, a string under a unary operator) are left out of the product; as a single `cell` line they are an error. Every other combination is generated, also those a compiler refuses ("Illegal Operator for Type"): which they are is measured (task 1.3), not assumed.
* A comparison in a destination is written in brackets (`rl=(a < b)`, as `compiler-quick-compare` does) so that the first `=` is the assignment; everywhere else the expression stands as it is. A Quick or Float parameter is copied by its statement to a global of the same type (`SHARED`), whose raw bits are printed; the key is printed before the call (`Print "KEY " : pq{...}`).
* A constant's size is checked against its kind (`cb` -128..127, `cw` of Word but not Byte size, `cl` beyond Word, `cf` with a `.`). The counts (the right operand of the shifts and `^`, decision 8: 2 or 3) are not checked for size: a count 2 of kind `cw` or `cl` is a Byte-size constant, the same cell as `cb`.

Task 1.3 (the measurement of cut F; report and cell table in `~/.local/share/bb2-c-work/expr-matrix/`):

* 2.10 and 1.95 agree in every cell; the port differs from 1.95 only in comparisons (#1177, #1186); the C libraries differ from the CD's in `If` on a Float expression, Quick products beyond the Quick range and #234; AmiBlitz3 refuses 546 cells more and differs in 131.
* A program can be disturbed by the programs before it in the same emulator session (a Byte destination `MOD` ended with "Software Failure" only there; unchecked index reads beyond the array): the measurement ran each source in a session of its own. The committed tests (group 3) have to take this into account (one program per corpus pass session is not the corpus runner's model; to be decided with the test form of task 3.1).
* AmiBlitz3's checked mode could not be measured: without its debugger module (`Defaultdbug`) `RuntimeDebug = 1` checks nothing. A limit of the matrix: AmiBlitz3's index cells are measured unchecked only, over 1.95's in-range set (spec, "Every cell is measured").
* The Byte destination `MOD` crash is #1049 (closed, fixed in the port): a folded constant `MOD` (Byte, Word or Quick) as the first use of modlib drops modlib's relocations, and a later Long or Float `MOD` of the same program jumps into the compiler's freed buffer in 1.95 and 2.10 (`tr-byte-mod-crash.md`). For the committed generator (task 3.1): a source puts its Long and Float `MOD` cells before its first foldable constant `MOD`, or the originals' runs of that source crash.
* The 12 string cells of the index contexts (block `fs`) were measured with checking off only; the six of them outside the array (`= <= >=`, index -1) printed memory.

Tasks 2.1-2.3 (`tools/lib/exprmatrix/model.c`, `classify.c`; report `~/.local/share/bb2-c-work/expr-matrix/classify-report.md`):

* The model follows the code blitz2 emits for a cell (registers, the type so far, `varcont`, `consttype2`, the operator handlers with their m68k semantics) and the library routines it calls, from their sources: lmullib, ldivlib (the restoring loop as it runs), modlib, floatquicklib and printlib of the CD (with #199, #233, #234 unfixed) and of the C libraries, and an emulation of Motorola FFP (round half up after truncating to 32 bits) for mathffp. With it the original's rules give 1.95's value in every cell that compiles (0 mismatches; also the refusals and the runtime errors), and the port's rules (#1177 without #1186) the port's measured values with both library sets (0 mismatches).
* Not modelled (`?`): mathtrans SPPow (`^`) where its bits matter, and the original's `If` on a Float, which tests d1 (#1187); 2,024 values. A Float power that both rule sets compute from the same operands takes 1.95's value as expected (decision 7, step 2).
* SPPow drops the sign of a negative base: `(-2.5)^3` gives 15.625 (`rl=vf^nw` 15 in set B, in 2.10, 1.95, the port and AmiBlitz3). A possible original bug; its cells are in question class Q-E.
* The fix of #1188 as its test states it (the fraction constant stays a Float under a Byte type) makes `rb=1.5 AND vb`, the shifts and `rb=NOT 1.5` a compile error, as under a Word type today; those cells are in Q-C.
* #1179 changes few cells: a later index under a Word type computes a Quick with its fraction, and in range the Word and the Long index agree, so its effect is the cut of the checked build, attributed to #1178. The 28 cells it changes are bit operators with a Quick under the later index (`ja(1,vq LSL nb)`: shifted as a Quick under a Word type, cut first under a Long; `vq AND 1.5` refused under a Long type): all in Q-C.
* Q-B (the two orders differ after variant C) has no cell: every pair whose orders differ is an overflow (Q-A) on one side, mostly a Long converted to a Quick.

Task 2.4 (the answers of 2026-11-22, `task-decisions.md`, "Matrix questions Q-A, Q-C, Q-D, Q-E", the Q-F line and the user's correction there; report `~/.local/share/bb2-c-work/expr-matrix/cells-classified-2.4.tsv`, `expr-matrix-questions-2.4.md`, `classify-run-2.4.txt`):

* **Baseline, not a rule.** The answers to Q-A (overflow), Q-C (bit operators and shifts with a Quick or Float), Q-D (`MOD` with negative or fractional operands) and Q-E (`^`, also `1000^2` = 999999) make the original's measured value the **baseline** of those cells: a regression check, so that a fix changes them only where it is meant to and named; the expected values are regenerated after the fix. They are not decisions that the behaviour is right (the user's correction): `MOD`'s sign, SPPow's precision and the overflows stay topics of their own. `classify` gives these cells the class `baseline` (the original's measured value), kept apart from `agree` (a recorded rule or the exact result gives the value). A baseline cell that an issue's fix changes is `fixed-N`/`open-N` of that issue (#1178, #1188, #199, #234 as listed with Q-A); a cell refused after #1188's fix (`rb=1.5 AND vb`) expects the refusal (Q-C).
* **#1189** (`^` with a negative base drops the sign): rule `EM_R_1189` of the fixed set, named after the issue: a whole exponent gives SPPow's magnitude with the sign of an odd power; the conversions after `^` (SPFix towards zero, floattoquick, Print) are symmetric, so a Float or Quick result the model cannot compute is the original's measured value negated (`?-`). A fractional exponent with a negative base has no rule (the issue: "to be decided"): question Q-E.
* **Q-F**: where AmiBlitz3 differs from 1.95 and an issue's fix changes the cell (92 cells: #1188 61, #1187 23, #1177 8), the recorded rules decide and AmiBlitz3's value goes to `docs/ab3/variant.md` (task 3.5); the 4 cells no issue explains stay questions: `if-add-cb-cf-A`, `if-sub-cb-cf-A`, `if-mul-cb-cf-A`, `if-div-cb-cf-A` (`If 3 + 1.5` and the like: 2.10, 1.95, the port and the expected value T, AmiBlitz3 F), shown to the user.
* **Q-B** (the orders differ) gets no cell: a pair with a baseline cell on either side keeps the original's order dependence as its baseline.
* Counts (40,561 cells): agree 24,375, baseline 7,288, noncompile 6,598, fixed-1177 296, fixed-1187 389 (with the `If` on a power, group 3), fixed-199 52, fixed-234 2, open-1178 334, open-1188 276, open-1189 744, open-1178+1189 32, open-1188+1189 8, open-1179 28, open-1177 8 and port 8 (the #1186 cells, measured before its fix: fixed-1177 by the model of the current port), question 123. Mismatches of the model: 0 (original's rules against 1.95, the port's against the port).
* **Left out** of the committed tests (the remaining questions): the 4 Q-F cells above, until the user answers; 119 Q-E cells: 117 with a negative base (also one that a Long converted to a Quick makes negative, `dq-pow-l-cf-A`) and a fractional exponent (#1189, to be decided), and 2 whose operands #1188's fix changes (`print-pow-b-cf-A`, `print-pow-cb-cf-A`: SPPow of 3^2.5 is not measured; measured with the fix).

Group 3 (tasks 3.1-3.4, 3.6; `tools/lib/exprmatrix/expect.c`, `tools/exprmatrix/README.md`, "The committed tests"):

* **Inputs.** `exprmatrix expect` reads the committed description `tests/corpus/tests-exprmatrix/matrix.txt` (option F, `cells-per-source 56`) and `measured.tsv` there: the 2,650 rows of the measurement the model cannot compute (a Float power, the original's `If` on a Float, d1) or where AmiBlitz3 differs from 1.95 (for task 3.5), written by `classify -m`. Every other value is the model's; the port is the port of this commit (`EM_RULES_PORTNOW`: #1177, #1186, #1187), so the #1186 and #1187 cells measured before their fixes are classed by the current port, and the runs of the generated tests are their re-measurement (all pass, below). Deviation from decision 5 (named for the coordinator): `measured.tsv` is a committed table of measured values that a host test reads (`exprmatrix_committed` regenerates from it); every value of it ends in an expected file that a test checks against the original (`originals`: the corpus against 2.10, the bug-orig `.orig` against 1.95) or, for AmiBlitz3, in `REF_EXPECT` (task 3.5).
* **`If` on a power** (the model, found by the generated tests' design): the fixed rules know that a power's Float condition is true (#1187: the value is tested; a power of these operands is never 0), the original's value is d1's: the 151 `if-pow-*` cells taken as baseline before are `fixed-1187` (389 in all).
* **Modes and files.** A cell is classed per mode (a destination, parameter, `If` or `Print` cell once; an index cell unchecked and checked): agree / baseline / fixed-N into corpus sources of one context, value set, mode and class (`CTX-SET[-u|-c]-TAG-NN`), open-N into bug-orig tests `compiler-expr-ISSUES-CTX-SET[-u|-c]-NN` (header: `BUG(orig) (#N)`, the cells; a two-issue test says it comes out DIFFERS between the fixes), refused cells as one representative per operator and operand classes (`regression/compiler-expr-refused-OP-L-R`, 123 tests; the cells #1188's or #1179's fix refuses: `compiler-expr-N-refused-OP-L-R`, 36 bug-orig tests). An unchecked index cell is in a test only where every compiler reads an element; the checked index cells are in sources with a `SetErr` handler in front of every cell (`SetErr : NPrint "KEY !runtime" : Goto mxN : End SetErr`): a cell outside the array prints `KEY !runtime` and the program goes on, so the 2,580 runtime-error cells need no source of their own (1.95, 2.10 and the port behave alike). Folded constant `MOD` cells (both operands constants and, by the original's rules, a Byte, Word or Quick operation; a Long or Float `MOD` is not folded) come last in a source (#1049): the first run of the originals' passes with the plain rule "both operands constants" crashed in `orig-clib` (2.10 with the C libraries) at `rb=3 MOD 100000` after a folded `rb=3 MOD 3`. `left-out.txt` lists the 175 cells left out: the 123 of the Q-E and Q-F questions, and 52 `If` cells on a Float whose original value is what d1 holds, which depends on the libraries (#1187: the port before the fix gave another value with the C libraries than with the CD's; the first originals run failed on them in `orig-clib`).
* **Counts.** 35,015 cells in 773 corpus sources (583 unchecked: `corpus_exprmatrix_NAME`, 190 checked: `corpus_exprmatrix_runerr_NAME`, every source a pass and so an emulator session of its own: coordinator's Plan choice), 1,269 cells in 60 bug-orig tests (#1178 26, #1179 1, #1188 4, #1189 24, #1178 and #1189 4, #1188 and #1189 1), 6,612 refused cells by 123 representatives, 212 cells refused after a fix by 36 representatives (#1188 29, #1179 7).
* **Port defect found** (D-021, for an issue of its own): the port's compile of 51 checked `i2` cells of value set J (shifts of constants and variables) with their `SetErr` lines does not end (no `RESULT`, session timeout; the port of this commit and the prev build); 1.95 and 2.10 compile and run all 56. 50 of them, cells 11-51, the cells without `SetErr` or without runtime error checking compile. Reproduction `~/.local/share/bb2-c-work/expr-matrix/port-hang/`. Until it is fixed a checked source holds at most half of `cells-per-source` (28); named in `expect.c`.
* **Task 3.4**: no patch of #1178, #1179, #1188 or #1189 exists (the #1178 spike of `quick-long-arithmetic` only defers today's cut and gives the original's values); every open test's expectation is the model's fixed rules (checked by the model only).

## Open Questions

None that can wait; the Decide question of decision 2(a) comes after task 1.2 and before task 1.3, the question classes of decision 7 before group 3.
