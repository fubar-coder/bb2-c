# Design

## Context

`eval5` (`programs/blitz2/src/eval.c:870-928`) reads one element, emits its code at once and decides every type from one value, the type so far. The wanted type is handed to every element unchanged: `eval5` -> `eval2` -> `getelement(type)`, also through brackets (`eval4(type)`, `eval.c:1600-1607`); the right operand of an operator starts with the type of the left operand (`eval.c:894-897`). `varcont` (`eval.c:2882-2912`) converts a value up to the type so far and cuts a Quick to Long when the type so far is Long. A comparison is only known after its left operand's code is emitted (`eval2` reads the operator after `getelement`); no code treats it differently. Its handlers `doeqb`...`dogef` are chosen by `op->h[type-1]` (`eval.c:1520-1525`, precedence 2000), emit `cmpit` (`cmp.x D(regat+1),D(regat)`) and `cpoke` (`Scc`, `ext.w`; the result is a Word, `eval.c:1070-1093`). Both operands are converted to one common type before `doop` runs (`convtypef(left, *type)`, `eval.c:906`): for `Long OP Quick` the type so far becomes Quick and the Long is converted to Quick (range +-32768); for `Quick OP Long` the Long is fetched under Quick.

Analysis: `~/.local/share/bb2-c-work/tr-compare-type-report.md` (sections 2 and 4) and `tr-expr-tree-report.md` (section 3: these fixes need no tree, `eval5` knows both operand types when it calls `doop`; section 2.3: for #1177 "the fix has to find a way to switch the type so far off before the compiler knows that a comparison follows").

Decided (`task-decisions.md`, 2026-11-21): original bug, parts 1 and 2 exact; part 2 with `floor(q)` (high word, sign-extended) and on equality the fraction, no float conversion; checked with the benchmark (D-007); a change of its own before `quick-long-arithmetic`; the new rule in AB3 mode too; the deviation entry in `docs/blitz2/deviations.md`.

## Decisions

### 1. Capability `compiler/expression-types`, shared with `quick-long-arithmetic`

Both changes ADD requirements to the same new capability. The names do not collide:

| This change | `quick-long-arithmetic` |
|---|---|
| The operands of a comparison are compared with their own values | A Quick operand is computed with its fraction |
| A Long or Word against a Quick is compared exactly | Rounding happens once per operation, to a whole-number type |
| A comparison gives a boolean in the type of the context | The expected type flows as before |
| Comparisons follow the same rule in both compiler modes | The rule is the same in both compiler modes |
| Comparisons that were right keep their results | A mixed product beyond 32 bits wraps; No other result changes |

Whichever change is archived first creates the spec; the other one's delta then also ADDs to an existing capability (its Purpose text is merged by hand). One sentence needs a touch: `quick-long-arithmetic`'s requirement "The expected type flows as before" says the type reaches the operands as before; after this change the operands of a comparison are the exception. Task 4.4 puts that exception into `quick-long-arithmetic`'s delta.

### 2. Mechanism: no tree, edits in `eval5`

* **Right operand.** The operator is known before the right operand is read. If `cur` is a comparison operator, the right operand starts with type 0 instead of `left`; `*type` after the recursive `eval5` is then its own type. (Today it is the promoted type, so the operand types are not known at `doop`.)
* **Left operand.** The operator is read after the operand's code is emitted, so the wanted type has already been applied. Candidates, decided by the spike (task 1.1):
  * **(a) Patch**, like the byte-constant rule (`eval.c:880-905`): `eval5` knows `lstart`/`lend`; `varcont` leaves a marker (like `bytevalend`) where its conversion code starts. When the left operand is one element ending in that conversion, the code from the marker is dropped (`pc` back) and `left` becomes the element's own type. Covers `q>1`, `fq{1}>1`, `l<q`, Quick function values, fields, array elements; does not cover an operand that is an expression (`r.l=(q*2>3)`: `q` is cut inside it).
  * **(b) Re-evaluate** the left operand with type 0: on seeing a comparison operator after an operand evaluated under a wanted type, set `pc` and the source cursor back to the operand's start and evaluate again. Covers every operand; needs the undo of every side effect of the first evaluation: relocations (`constfold` frees them, `eval.c:852-861`), the first use of a library (stays linked, #1049, so library order and offsets would change), variables created, string literals in the data area, `lasta6`, `regat`/`fuckpos`, the string work area (`tr-expr-tree-report.md` sections 1.5, 1.6). Risky for the byte identity of everything that is not a comparison.
  * **(c) Look ahead** in the source for a comparison operator at the operand's level before evaluating (like `fracend`, `eval.c:2324-2331`): a precedence-aware scan of the text (brackets, strings, parameter lists, `\` paths, AB3 extensions); fragile.
  * Excluded: keeping the cut and only recording the own type: `q>1` for `q=1.5` would still compare 1 with 1.
  **Decided: (a)** (user decision 1, `task-decisions.md` 2026-11-21 "quick-compare-types (#1177): open questions"): a patch with a marker from `varcont` for a one-element left operand. It covers the issue's cases and every one of the 12 places on the CD. A compound left operand stays wrong until #1180; the fix is partial and says so (proposal, spec scenario, deviation entry, `expression-types.md`, site, issue comment).
* **Operand types at `doop`.** For a comparison the operand types are `left` and the right operand's `*type`; a function `compareop(op, left, right)` next to `doop` converts both to the common type (`convtypef` for the left as today, nothing for the right if it already is the common type) and emits `cmpit`/`cpoke`; the mixed Long/Quick pairs go to part 2. Every other pair calls today's handler with today's type, so its code is unchanged.

### 3. Part 2: Long against Quick, inline

Registers as in `cmpit`: left `D(regat)`, right `D(regat+1)`. `l` is a Long (a Word is extended first, as `convtype` does), `q` the raw 16.16 value. With `f = floor(q)` (the high word sign-extended: `swap` then `ext.l` on a copy) and the fraction `frac` (the low word):

* `l = q` is `l = f AND frac = 0`; `l <> q` its negation;
* `l < q` is `l < f OR (l = f AND frac <> 0)`; `l >= q` its negation;
* `l > q` is `l > f`; `l <= q` its negation;
* with the Quick on the left the operator is swapped (`q > l` is `l < q`).

The exact instruction sequence, using no library call and no float conversion, is the spike's result (task 1.1 f); expected 4-6 instructions against today's `swap`/`clr.w` and one `cmp.l`. D-007: the benchmark (decision 6).

A Word against a Quick is exact today (a Word converts to Quick without loss, then `cmp.l`); the implementation may keep that code, the requirement holds either way (user decision 3). A Byte is extended to a Word first.

### 4. The byte-constant rule (#978) stays

`eval5` keeps `lbyte`/`lconst`/`byteconstext` (`eval.c:880-905`) for `=` and `<>`. With the right operand now starting at type 0, a constant is typed by `consttype2` itself and `bytevalend`/`constat` are still left by `varcont`/`consttype2`. The spike checks all four orders of the #978 test; if the patches interfere, the byte-constant check runs first.

### 5. Results and comparison

Changed executables: those with a comparison of a Quick variable or function value under a Long wanted type, or of a Long or Word with a Quick. The 12 places on the CD compare whole numbers: their results stay, their code may change. No library file, TED or `acidlibs`/`deflibs` change. The prev comparison (`cmake --preset amiga -DBLITZ2_PREV=ON`) lists the differing executables; each must contain such a comparison; they are named in `PREV_INTENDED` and in the commit message. The executable-level comparison over more sources is issue #1181; used if it exists by then.

### 6. Cost check (D-007)

Whole-number and Quick-Quick comparisons keep their code. Part 1 saves a conversion for correct cases. Part 2 is a few instructions more than the Quick compare, no call. The library benchmark does not cover compiler output; the check is a compiled loop condition (`While count<i`, Long against a default-typed variable) in `tests/perf`, before and after (spec `testing/performance-budgets`); the cost is stated in the commit; a budget that rises is a decision named there.

### 7. AB3 mode

One core, one rule, no mode condition (decision 2026-11-21). `docs/ab3/variant.md` gets a row (AmiBlitz3 v3.14.0 keeps the old rule); the `tests/ab3` oracle tests affected are found by `ctest --preset oracle` and marked.

### 8. Documentation, deviation, site

* `docs/blitz2/expression-types.md`: "A comparison starts again", "Where this is a problem" (comparisons: fixed in the port, #1177), the table row `r.l=(q>1)` with before and after; the comment at `eval.c:2893` names `Fixed (#1177)` for comparisons, the arithmetic part stays until `quick-long-arithmetic`.
* Deviation: kind F in `docs/blitz2/deviations.md` (not in `docs/libs/deviations.md`).
* `docs/ab3/variant.md`, "Differences to AmiBlitz3's compiler" (line 822 on): a row.
* Site: the comparison paragraph in `site/content/docs/how-blitz2-works/compiling.md:155-170` ("Until then" goes); the fixed-bugs entry and the overview row are the coordinator's (proposed: severity 1, topic Compiler).

## Risks

* The left operand (decision 2): a one-element patch does not fix `r.l=(q*2>3)`; a re-evaluation is risky.
* A comparison whose operand overflows as Quick but not as Long changes: `ok.l=(i*4+adr>lim)` with a default-typed `i` is computed in Long today (the context reaches it), after the fix as `If i*4+adr>lim` is today (Quick, range +-32768). Named in `tr-compare-type-report.md` section 4; goes into the deviation entry and the `docs/ab3/variant.md` row. No place on the CD (12 conditions, whole numbers, no such arithmetic in them).
* `quick-long-arithmetic` says the "no cut at fetch" edit comes with this change; this change leaves the cut for arithmetic alone (user decision 2).
* The byte-constant patch (decision 4) and `regat` (`convtypef` "could possibly fuck regat+1", `eval.c:906`).
* The expression tree (#1180) replaces `eval5`'s typing; this change's tests (values) are its regression suite.

## Decisions of the user (2026-11-21)

From `~/.local/share/bb2-c-work/task-decisions.md`, "quick-compare-types (#1177): open questions".

1. **Left operand:** option (a), the patch for a one-element left operand; compound left operands such as `r.l=(q*2>3)` stay wrong until #1180; named as a partial fix.
2. **The cut in `varcont`** is removed by `quick-long-arithmetic`, not by this change; task 4.4 corrects that change's text.
3. **Word against Quick** keeps today's code if it measures exact; **Float** comparisons are unchanged (the spike confirms with a measured case). The overflow risk of a compound operand (`ok.l=(i*4+adr>lim)`) stays documented as a risk.

## Open questions

None left from the first round. The spike (task 1.1) may raise new ones.
