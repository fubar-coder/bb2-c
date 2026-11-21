# Design

## Context

`eval5` (`programs/blitz2/src/eval.c:870-928`) reads one element, emits its code at once and decides every type from one value, the type so far. `varcont` (`eval.c:2882-2912`) makes the cut: `if (oldtype == T_LONG && *type == T_QUICK) goto conv2;` converts the Quick to Long at fetch. Operators are chosen by `doop(op, type)` through `op->h[type-1]` (`eval.c:986`); both operands are already of the type so far when it runs (`convtypef(left, *type)` at `eval.c:906`). Before the fix `eval5` therefore never sees a Quick operand under a Long type so far.

Decided (2026-11-21, `task-decisions.md`, "#1178 direction: variant C"; recomputation in the issue comment and `tr-quick-arith-variants-report.md`): variant C, order dependence as today, order independence later by an expression tree. The tree analysis (`tr-expr-tree-report.md`, sections 3-4, by the coordinator's note) finds that variant C, #1177 and #1179 do not need the tree, because `eval5` knows both operand types when it calls `doop`, and proposes doing these fixes first in today's evaluator ("step 0"), with an executable-level prev comparison over more sources. This change is that step for #1178. It depends on the change for #1177, which is done first (user decision 2).

## Decisions

### 1. Capability `compiler/expression-types`, new

The behaviour is about the compiler's typing of arithmetic, not an inline library call (`compiler/intrinsics`) and not AB3 mode's type rules (`ab3-compat/type-rules`, 3 requirements; a rule shared by both modes does not belong in an `ab3-compat` capability). #1177 and #1179 (comparisons, array indexes) can add requirements to the same capability when they are planned, and the later tree can modify them. Alternative: no new capability, but then the rule has no home.

### 2. Mechanism: the operand types reach the operator (no tree)

`varcont` stops cutting: a Quick fetched under a Long type so far stays a Quick in its register; the type so far stays Long. So `eval5` needs the operand types of both sides: the left type is `left` (`eval.c:905`), the right type is what the recursive `eval5` returns in `*type`. The change is confined to the one case that is cut today (`oldtype == T_LONG && type == T_QUICK`); every other pair keeps its code. Where the pair is (Long-or-Word, Quick) or (Quick, Long-or-Word) and the type so far is Long or Word, a new path emits the mixed operation and returns the whole-number type (Long). That is the only new code path; per the model, expressions without a cut today (6474 other expressions with a Quick operand) are not touched.

Per operator, left or right being the Quick (`q` raw 16.16, `l` whole):

| Operator | Code | Note |
|---|---|---|
| `l+q` | `l + (q>>16)` as today (swap, add) | no new cost; floor(q) as today |
| `l-q` | `l - ceil(q)`: add $FFFF, swap, sub | 2-3 more instructions |
| `q-l` | `floor(q) - l` | as today |
| `l*q`, `q*l` | `(l*qraw)>>16`, 64-bit intermediate | new library routine |
| `l/q` | `(l<<16)/qraw`, 48-bit dividend | new library routine |
| `q/l` | `qraw/l` rounded down: a Quick divided by a whole number gives a Quick quotient, floored | existing long division of the raw value, then `swap`/floor; to be checked against the model |

`Word` instead of `Long` is converted to Long first (as `convtype` does). A Quick on both sides stays Quick (not a cut case). Constant folding: the folded code runs in the compiler's process (`eval.c:870-928`, `constfold`); the new library calls are `nocando` (`eval.c:1336`) like today's multiply and divide, so the mixed operation is never folded; a constant has no fraction under a Long type so far, so a mixed constant pair does not occur (`1.5*2` is a Float constant today).

A spike is the first task: whether the right-operand type is reliable at `doop`'s call (a parenthesised or function operand, `eval2`'s unary minus, `Not`, the functions' own conversions), and which of the five patching mechanisms of the report (section 1.5) interfere (`convtypef` of the left operand after the right one was emitted, register `regat`).

### 3. Library routines: where, and compatibility

A Long times a Quick needs a 64-bit intermediate and `l<<16` does not fit 32 bits; neither can be done with today's `qmultiply`/`qdivide` ($CA00, $CB00, `main.h:106-109`) when `l` is beyond +-32767. Candidates:

* **(a) new entries in the system libraries `lmullib`/`ldivlib`** (`$CA02`, `$CB02`; the headers end at `$CA02`/`$CB02`, `libs/acidlibs/lmullib.c:21`, `ldivlib.c:20`). Fast, the library files change (named for prev), but a program compiled by the port needs the port's libraries (the original's `lmullib` has no such entry).
* **(b) inline code** for the product (three MULU) and the division: no library change, longer code, slower, still needs a division loop.
* **(c) a new system library** with its own number.

Choice: (a), because the compiler already calls these libraries for Long and Quick `*` and `/` and the budgets exist for them. Decided: (a), user decision 3.

### 4. Rounding rules to settle in the spike, then pinned in the test

* Conversion Quick to Long is floor today (`-1.5` gives -2); `Int(q)` truncates toward zero (spec `compiler/intrinsics`). Variant C rounds `+ - *` down: `floor`; division towards zero (below).
* Long division `longdiv` works on magnitudes (`ldivlib.c:1-30`, "a restoring shift/subtract division on the magnitudes") and so rounds toward zero. `l/q` with a negative inexact quotient (`10 / -1.5` = -6.67) gives -6. Decided (user decision 5): division rounds towards zero like BB2's Long division; the spike measures it and the test pins it. A whole-number Quick then gives today's result.
* Overflow of the final Long wraps modulo 2^32 as `longmult` does today (user decision 7).

### 5. Results and comparison

Changed executables: those that contain a Long or Word and a Quick in one arithmetic expression where the cut applied (the 376 places in the CD's sources, more in the corpus: counted by comparing before and after), plus the library files `lmullib.obj`/`ldivlib.obj` (and the disk libraries of the build: `acidlibs`, `deflibs`, `libs/variants`, whichever carry them). Named in the commit message with `PREV_INTENDED`, with the list generated by the prev comparison. The expected value of a program output that changes is the 18 places whose result differs today; for the others the outputs are unchanged and the test of that is the corpus run (`corpus`, `corpus_runerr`, the CD corpus `blitz2-cd.tsv`), also by a comparison of the executables (step 0 of the tree report: an executable-level prev comparison with more sources).

### 6. Cost check

Runtime cost: `l+q` unchanged, `l-q` a few instructions more, `*` and `/` a library call as `longmult`/`longdiv` today but with a 64-bit step. The library benchmark (`ctest --preset perf`, `tests/perf/bench`, `budgets.txt`, spec `testing/performance-budgets`; D-007: a library change is checked against the benchmark) gets lines for the new entries, measured against the existing `lmullib`/`ldivlib` lines and the original's `TARGET`; an existing line must not rise. A budget that goes up is a decision named in the commit. The compile-time cost (the new branch in `eval5`) is measured by the compile time of the corpus (no bench exists; not expected to matter).

### 7. AB3 mode

blitz2 and blitz2-ab3 link one core (`programs/blitz2/CMakeLists.txt`), AmiBlitz3 v3.14.0 has the same code as the original (`expression-types.md`) and keeps the old rule. Decided (user decision 1): the new rule applies in AB3 mode too, no mode condition; `docs/ab3/variant.md` gets a deviation row, and the affected oracle tests in `tests/ab3` are marked.

### 8. Documentation, deviation, site

* `docs/blitz2/expression-types.md`: the rule (the exception section, the "type so far" list, the measured table with a "Before"/"After" column, "Where this is a problem": arithmetic becomes "fixed in the port, #1178"; the comment at `eval.c:2893` is rewritten to `Fixed (#1178)`).
* A deviation entry: the coordinator names `docs/*/deviations.md`, but only `docs/libs/deviations.md` and `docs/ted/deviations.md` exist; fixed compiler bugs are recorded in code comments (`Fixed (#N)`), `site/content/docs/compared-with-original/fixed-bugs/compiler.md` and `site/data/fixoverview.yaml` (e.g. #978 at `compiler.md:150`). `docs/blitz2/deviations.md` is created by a separate task; this change adds its kind-F entry there (user decision 6). The new library routines go to `docs/libs/deviations.md` (decision 3).
* `docs/ab3/variant.md` ("Differences to AmiBlitz3's compiler"): a row: the port differs from AmiBlitz3 v3.14.0, which keeps the old rule (user decision 1).
* The site section "The type of an expression" (`site/content/docs/how-blitz2-works/compiling.md:147`) and the fixed-bugs page entry; the overview row and label are the coordinator's step.

## Decisions of the user (2026-11-21)

From `~/.local/share/bb2-c-work/task-decisions.md`, "quick-long-arithmetic: first open questions" and the following lines.

1. **AB3 mode** gets the same rule: blitz2 and blitz2-ab3 share the core, so no mode test. A deviation row in `docs/ab3/variant.md` (AmiBlitz3 v3.14.0 keeps the old rule); the oracle tests in `tests/ab3` that the change affects are marked.
2. **#1177 is a change of its own and is done before this one.** This change depends on it (the "no cut at fetch" edit and the operand types at the operator come with it); no parallel work.
3. **Routines** are new entries in `lmullib` and `ldivlib` (variant (a) of decision 3 above), with an entry in `docs/libs/deviations.md`: a program compiled by the port needs the port's libraries, not the original's.
4. **Classification:** #1178 is fixed as an original bug with the full fix workflow (labels `bug-orig` and `wrong-result` are on the issue; a bug-orig test with `; @fixed #1178` or the equivalent; the website's fixed-bugs entry and the overview row are the coordinator's; severity 1). `tests/blitz2/quick-arith-long.bas` gets the new expected values.
5. **Division** Long/Quick rounds like BB2's Long division, towards zero (as the spike measures), so a whole-number Quick gives today's result; the test pins it. Addition, subtraction and multiplication round down (floor). So `10 / -1.5` is -6.
6. **Deviation entry:** the kind-F entry goes into `docs/blitz2/deviations.md`, which another task creates; this change adds the entry there (it starts after that file exists).
7. **Overflow** of a mixed product beyond 32 bits wraps as `longmult` does.

## Open questions

None left from the first round. The spike (task 1.1) may raise new ones (the right operand's type at `doop`, the division rounding it measures).

## Risks

* The right-operand type may not be known where code was moved or patched (the report's five mechanisms); the spike decides whether the change stays inside `eval5`.
* Code size: `l-q` and the new calls make the compiled expressions longer; short branches in generated code are not affected (the code is generated, not hand-placed), but the executables' sizes change.
* The function-optimizer change (`compiler-function-optimizer`) touches `afunction` in the same file; the two changes must not be implemented in parallel (coordinator's note).
* The later expression tree (`tr-expr-tree-report.md`) replaces `eval5`'s typing; this change's tests (values, not code) carry over, the executables are compared at the byte level only against this change's output.
