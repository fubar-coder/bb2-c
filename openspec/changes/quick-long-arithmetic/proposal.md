# Proposal

## Why

blitz2 (like the original compiler) compiles an expression with one "type so far". A Quick (16.16 fixed point) that is fetched while the type so far is Long is converted to Long at once and its fraction is cut off (`varcont`, `blitz2.s:10491-10498`; `programs/blitz2/src/eval.c:3040`). The operation is then done with the whole number, not with the value: with `q=1.5`, `r.l=q*2` gives 2, `r.l=l-q` (l=10) gives 9, `r.l=q/h` (h=0.5) gives 2147483647 (`h` is cut to 0). The result also depends on the order of the operands (`x.w=w+v+q*2` is 9, `x.w=q*2+w+v` is 10). `docs/blitz2/expression-types.md` has the rule and a measured table; issue #1178 lists the 376 places in the CD's sources where it applies, 18 of which give another result than the source says.

Issue #1178 first recorded this as a potential problem; the user has since classified it as an original bug to fix with the full workflow (labels `bug-orig`, `wrong-result`; severity 1) (the rule is deliberate: without it `adr.l=base+i*4` would be computed in Quick, range +-32768). The decision of 2026-11-21 (`task-decisions.md`, "#1178 direction: variant C") is to change it by variant C: the expected type flows as today, a Quick is not cut when it is fetched, and each operation is computed with the Quick's fraction and rounded when it must give a whole-number type. The rounding is decided ("#1178 rounding rule decided", 2026-11-21: "Zwischenlösung: / gegen Null (Recommended)"): a division towards zero, everything else (`+ - *`, negation, the conversion of a Quick to Long, Word or Byte) down. Computed with a model over the 376 places (not measured; the model predates the rounding decision): 358 places unchanged, 18 right, 0 break (`tr-quick-arith-variants-report.md`).

## Dependency and order

* **#1177 is merged** (fe1d7b357, change `quick-compare-types`). It removed nothing of the cut: `varcont` still converts a Quick under a Long type so far, and `cmpownleft` takes that conversion back for a left operand of a comparison that is one element. The cut at fetch is removed by this change (user decision 2026-11-21, "quick-compare-types (#1177): open questions": "varcont: quick-long-arithmetic (Recommended)"); this change also replaces the Quick-to-Long case of `cmpownleft` by the operand flag (design, decision 7).
* **Order** (2026-11-22, "Order of the Quick work; matrix test": "A: Fixes zuerst (Recommended)"): the type-matrix test change comes first ("Matrix test: "Ja, vor #1178 (Recommended)""), then this change, then #1179, #1182 and the expression tree (#1180). The matrix test is the change `expression-type-matrix` (spec `testing/expression-matrix`, test form "A: Alte Form, später umziehen (Recommended)", 2026-11-22, "#1185 fix; matrix test form"); this change starts after it is merged. Its tests are this change's test of the expected behaviour and its regression test (task 1.2, design decision 13): the cells of class `open-1178` are the bug-orig tests `tests/bug-orig/compiler/compiler-expr-1178-*` (REPRODUCED on main; 4 of them, `compiler-expr-1178-1189-*`, also need #1189), the fix adds `; @fixed #1178` to them; the agreement, baseline and fixed cells are the corpus passes `corpus_exprmatrix*` (fixed cells as pairs `NAME.orig.out` / `NAME.fixed-N.out`), which must keep their values; after the fix the matrix's files are regenerated with `expr-matrix-update`.
* Not in parallel with `compiler-function-optimizer` (same file).

## What Changes

- **A Quick is not cut when it is fetched.** The special case in `varcont` goes; a Quick fetched under a Long type so far stays a raw 16.16 value in its register and a flag says so (the spike's mechanism, `tr-1178-spike-report.md` section 2). Each consumer of the value knows it: the arithmetic operators, the late conversion of a left operand, `^`, the shift, logic, `MOD` and comparison handlers, `Not`, unary minus, the end of the expression.
- **Each arithmetic operation under a whole-number type so far is computed with the Quick's fraction and rounded once, at the operator:** `+`, `-`, `*` and unary minus down (floor), `/` towards zero (as BB2's Long, Word and Quick divisions do, measured). This holds for a whole number with a Quick and for a Quick with a Quick (`r.l=q/h` with -3.0 and 2.0 gives -1). The conversion of a Quick to Long, Word or Byte stays floor (`r.l=q` with -1.5 gives -2); `Int(q)` truncates as today.
- **A Long operand that sets the type itself** (a Long constant beyond the Quick range, `&var`, `?label`, to the right of a Quick with no type so far): `+` and `-` stay a Long operation as today, `*` and `/` use the new routines (2026-11-21, "#1178 d refined"); whoever wants the old result writes `Int(q)`.
- **A Float operand and `^`** take the Quick's value with its fraction (no cut); the Float result is converted as every Float is (towards zero, `SPFix`).
- **What stays as today:** the type so far flows as it does now (destination or parameter, a Long operand to the left, Word+Word and Word*Word, a shift); constants take the type so far; the order dependence stays in general (`NPrint w+v+q` and `NPrint q+w+v` still compute in different types). Order independence is the goal of the expression tree (#1180). Shifts, `AND`, `OR`, `EOR`, `MOD` and `Not` take the Quick rounded down as today.
- **For a Quick without a fraction the result is today's**, as long as no intermediate value overflows a Long (a Quick OP Quick under a Long type so far is computed without a Quick intermediate, so `q+p` and `q*p` with 20000.0 and 300.0-sized values give today's Long results).
- **Library routines** for the mixed multiplication and division with a 64-bit (48-bit) intermediate: new entries in `lmullib`/`ldivlib`; a program compiled by the port then needs the port's libraries (entry in `docs/libs/deviations.md`).
- **The matrix's tests at the fix** (design decision 13): the regeneration after the fix (`EM_R_1178` in the generator's rule set of the port, `expr-matrix-update`) writes `; @fixed #1178` into the headers of the 26 single-issue tests `compiler-expr-1178-*` and changes no `.expect`, no `.orig` and no corpus file of the matrix. The baseline cells (the original's measured values, not decided behaviour: `task-decisions.md` 2026-11-22, "Matrix questions Q-A, Q-C, Q-D, Q-E", the correction by the user) change only where #1178 changes them; those are `open-1178` cells already, and the fix commit names them. This needs a change of `exprmatrix expect`, which today moves a fixed issue's cells into corpus pairs and removes their bug-orig tests (open question 1, answered: T1).
- **Results change** (rule 3, named with `PREV_INTENDED`): the executables of programs in which a Quick is fetched under a Long type so far (the code order of the conversion changes even where the value does not, for instance `a(i+1)` with a default-typed `i`; design, decision 6), and those with `-q`, `^`, a Float operand or a late cut by a Long constant in such an expression; the library files that get new routines; the expected values of `tests/blitz2/quick-arith-long`, and the inputs of the bug-orig test `compiler-compare-compound-left` (#1183), which this change would otherwise turn FIXED by accident (design, decision 7).
- **Documentation:** `docs/blitz2/expression-types.md` (the rule, the tables, the paragraph "Planned in #1178", and "Where this is a problem", which still says #1178 keeps the current behaviour), the #1177 limits there and in `docs/blitz2/deviations.md`, the site section "The type of an expression" (`site/content/docs/how-blitz2-works/compiling.md`, which still says the port keeps the rule), the kind-F entry in `docs/blitz2/deviations.md`, `docs/libs/deviations.md`, `docs/ab3/variant.md` (AB3 mode gets the new rule too).

## Not in scope

- Comparisons beyond what the operand flag changes for them (#1177 is done; its compound-left limit is #1183).
- Array index types (#1179), a comparison as right operand (#1182), an operand beyond d7 (#1185), order independence (#1180).
- The rounding of a Float (`SPFix`), `Int`, `Frac`; the Quick-to-whole conversion outside an operation (stays floor).

## Open questions

1. **Where the matrix's #1178 cells live after the fix** (design decision 13): answered by the user (`task-decisions.md` 2026-11-22, "Go for steps 1-7": "T1: in ihren bug-orig-Tests (Recommended)"; "Last answers of the session", "T1 for all fixes": "\"Ja für alle\" - zumindest solange bis die Test-Infrastruktur fertig umgebaut ist"): T1, they stay in their bug-orig tests, which get `; @fixed #1178`.

## Interim value

`NPrint 1+1+1+q` gives 4 and `NPrint w+v+q` gives 8 (Word+Word makes the type so far Long), as today; the expression tree (#1180) must give 4.5 (2026-11-22, "#1178 Q1"; design, "Interim value handed to the expression tree"). Not part of this change.

## Capabilities

### New Capabilities

- `compiler/expression-types`: what an arithmetic operator gives when one operand is a Quick and the other, or the type so far, is a whole number. (`quick-compare-types` adds its comparison requirements to the same capability; whichever is archived first creates the spec.)

### Modified Capabilities

(none in `openspec/specs/`; the requirement "Known limits of the left operand until the expression tree" of `quick-compare-types` changes its scenario with this change: task 4.5)

## Impact

- `programs/blitz2/src/eval.c` (`varcont`, `eval5`, `cmpownleft`, `negate`, `notit`, `dopow`, `maineval2`, the mixed paths of `+ - * /`), shared code that `blitz2-ab3` links (`programs/blitz2/CMakeLists.txt`).
- `libs/acidlibs/lmullib.c`, `libs/acidlibs/ldivlib.c` (`main.h` tokens): new entries; `tests/perf` budgets for them.
- Tests: the expression matrix (`expression-type-matrix`): the headers of `tests/bug-orig/compiler/compiler-expr-1178-*.bas` (`; @fixed #1178`, regenerated), `tools/lib/exprmatrix/expect.c` and `exprmatrix.h` (a fixed issue's open cells, the port's rule set), `tools/tests/test_exprmatrix_expect.cpp`, `tools/exprmatrix/README.md`, and `tests/corpus/tests-exprmatrix/measured.tsv` only where the model does not know the fixed port's value (design decision 13); a hand-written bug-orig test for the cases the matrix lacks; `tests/blitz2/quick-arith-long.bas` and `.expect`, `tests/bug-orig/compiler/compiler-compare-compound-left` (inputs), the library capture tests (`libs/tests`), the prev comparison (`PREV_INTENDED`).
- Docs and site as listed above; issue #1178 (comment).
- Related: #1177 (merged), #1179, #1182, #1183, #1180.
