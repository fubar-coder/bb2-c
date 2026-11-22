# Proposal

## Why

The first index of an array is always evaluated as a Long (`arreval`, `blitz2.s:8828`). Every later index is evaluated as a Word without runtime error checking and as a Long with it (`arrevalchk`, `blitz2.s:8809-8811`; `programs/blitz2/src/eval.c:621`, `sharreval` `:642`; called from `calcvar`, `parse.c:1374`). With a Quick operand the two evaluations compute differently, so the same source addresses different elements in the two builds, and the first index differs from the later ones: `g(1,y*200/2)` with a default-typed `y=200` is `g(1,52768)` without the checking (`y*200` overflows the Quick range) and `g(1,20000)` with it; `b(1,q ASL 4)` with `q=1.5` is `b(1,24)` against `b(1,16)`. The checked build therefore does not report the element the unchecked build really uses. blitz2 1.95, 2.10 and the port do the same (issue #1179, measured; `~/.local/share/bb2-c-work/tr-1179-array-limits.md`).

Recorded decisions:

* Direction (2026-11-21, `task-decisions.md`, "#1179, CRC findings, trailers, merge"): "Keine Prüfung im Release (Recommended)" (direction A: every index a Long, the checked build keeps arrszchk; benchmark before the fix). NEWTYPE arrays: fixed size by a constant (BLITZ2_MANUAL:1747-1752), not part of the fix.
* Variant (2026-11-22, "Last answers of the session"): "A0 (Recommended)" -> direction A plus inline Word mul/div in a later index and no upper word in the unchecked build (tr-1179-variants-cost.md); B1/B2 not taken. Replaces the proposed variants.
* Matrix cells (same entry): "\"Ja für alle\" - zumindest solange bis die Test-Infrastruktur fertig umgebaut ist" -> fixed matrix cells stay in their bug-orig tests with `; @fixed #N` for every fix, until the test infrastructure (runner phase 2, bug-orig-basic-tests) is rebuilt.

The variants and their measured cost: `~/.local/share/bb2-c-work/tr-1179-variants-cost.md` (A500 cycles and bytes per access, the CD, the matrix's 121 `i2` sources).

## Dependency and order

* **After `quick-long-arithmetic` (#1178) is merged.** On today's `main` a Long evaluation cuts a Quick when it is fetched; direction A on top of that moves 49 agreement cells of the matrix's unchecked `i2` sources (`i2-*-u-agree`, e.g. `ja(1,vq*vw)`: 1.5 x 20 reads element 30 today, 20 under A on today's main) to the cut value. With #1178 (variant C: no cut at fetch) merged first, the Long evaluation keeps the fraction and these cells keep their value (variants file, "Order: #1178 first").
* **Recorded order** (2026-11-22, "Order of the Quick work; matrix test": "A: Fixes zuerst (Recommended)" -> #1178 -> #1179 -> #1182 -> (#1185, see below), then tree steps 1-4 (byte-identical), then #1183, then tree step 5). So this change starts after `quick-long-arithmetic` is merged and before `compare-right-operand` (#1182), whose tasks.md header already says "Starts after `quick-long-arithmetic` (#1178) and the change of #1179 are merged". The tree changes rely on it being merged (`expr-tree-1-typing` task 2.1 "#1179 index", `expr-tree-3b-arrays` "`arrevalchk` with #1179's rule as merged").
* **Not in parallel** with any change that edits `programs/blitz2/src/eval.c` (2026-11-22, "Go for steps 1-7": "changes that edit programs/blitz2/src/eval.c one after another"): `quick-long-arithmetic`, `compare-right-operand`, `byte-fraction-constant`, `power-negative-base`, `compare-compound-left`, `compiler-function-optimizer`, the `expr-tree-*` changes.
* **The matrix's generator at a fix (T1)** is changed by `quick-long-arithmetic` task 1.2 (a commit of its own before #1178's fix); this change uses it.

## What Changes

- **Every later index is evaluated as a Long, with and without runtime error checking** (direction A): `arrevalchk` evaluates like `arreval` in both builds; the check for 65536 (`arrszchk`) stays only in the checked build. Without the checking an index the checked build reports still uses its low word, as today (no check in the release build, recorded).
- **Inline Word multiplication and division in a later index** (A0): a `*` or `/` whose two operands are Byte or Word values (variables, and constants up to 32767) is compiled as `muls.w` / `divs.w` with `ext.l` instead of a call of `lmullib` / `ldivlib`, in both builds. The value is the Long operation's in every case (the 32-bit product is exact; the one overflowing quotient, -32768 / -1, gives 32768: design, decision 3). The first index and other Long evaluations are not changed.
- **No upper word in the unchecked build** (A0): without the checking, the final sign extension of a later index that is one Byte, Word or Quick element is left out, and a constant index is loaded with `move.w`; the array code multiplies only the low word of a later index (`makelmul`, `parse.c:1279`). So `a(i,j)`, `b(i,j,k)`, `a(5,7)` and a loop over `a(x,y)` keep today's code byte for byte (measured: 0 cycles, 0 bytes).
- **Results that change** (CLAUDE.md rule 3, named with `PREV_INTENDED`):
  * without the checking: a later index whose Word and Long evaluations differ after #1178: a Quick calculation beyond the Quick range (`g(1,y*200/2)` becomes `g(1,20000)`), a shift or bit operator with a Quick (`b(1,q ASL 4)` becomes `b(1,16)`, the matrix's 14 cells of `compiler-expr-1179-i2-I-u-01`), and a bit operator or shift with a fraction constant, which becomes the compile error "Illegal Operator for Type" that the checked build and the first index give today (the matrix's 7 tests `compiler-expr-1179-refused-*`; on the CD no source changes its compile result, variants file);
  * the executables of programs compiled without the checking whose later index has an operator with Byte, Word or Quick operands (+2 to +8 bytes; the CD: 1 executable of the 496 that compile, `CAFFEINEFREE.BB`, +8, measured on main before #1178 and counted again in task 3.3), and of programs with a Word `*` or `/` in a later index in either build (no CD source has one);
  * programs compiled with the checking keep their values.
- **Documentation:** `docs/blitz2/expression-types.md` (the item "Array indexes", `:222`), `docs/blitz2/deviations.md` (kind-F row), the site section "Array indexes" (`site/content/docs/how-blitz2-works/compiling.md:184`), `docs/ab3/variant.md` (the row on array indexes, point 3, AB3 mode gets the fix: decision 1 of the user), a `BUG(orig) (#1179)` comment at `arrevalchk` with the red tests, `Fixed (#1179)` with the fix.

## Not in scope

- Indexes above 65535 and arrays of 64 KB or more per dimension: the unchecked build keeps using the low word (direction A without a release check, recorded); AmiBlitz3's defects there (`docs/ab3/variant.md`) are not this issue.
- An array inside a NEWTYPE (`\f[i]`): a fixed size by a constant, not part of the fix (recorded). List arrays (one dimension).
- The first index (already a Long; no inline Word forms there), Long evaluations outside array indexes.
- Saturation or an overflow flag (variants B1, B2: not taken).
- Further index savings that the expression tree allows (an unchecked index made only of `+ - AND OR` and left shifts needs no extension at all: variants file, "Prototype notes"); `MOD` and results known to fit in a Word are not marked as Word values.
- Quick arithmetic (#1178), comparisons as right operand (#1182), order independence (#1180).

## Decisions of the user (2026-11-22)

1. **AB3 mode** (`blitz2-ab3`, the same `eval.c`; answered with the options shown, scope: this case; `~/.local/share/bb2-c-work/task-decisions.md`, entry "2026-11-22 Coordinator session (handoff-2026-11-22-quick.md), answers"): "Beide Modi (Recommended)" -> one code path, the fix in BB2 and AB3 mode; AmiBlitz3 v3.14.0 itself evaluates a later index as a Word without the checking unless its 68020+ option is set: a row in `docs/ab3/variant.md`.
2. **The new compile error** in a later index without the checking (`ja(1,vq AND 1.5)`, the shifts with a fraction constant; same entry): "Ja, mit new-errors.md (Recommended)" -> it comes with the fix, with an entry in `new-errors.md`.

## Capabilities

### New Capabilities

- `compiler/expression-types`: how an array index is evaluated. (The capability is also added by `quick-compare-types`, `quick-long-arithmetic`, `compare-right-operand` and others; whichever change is archived first creates the spec, the later ones add to it.)

### Modified Capabilities

(none in `openspec/specs/`)

## Impact

- `programs/blitz2/src/eval.c` (`arrevalchk`, `maineval2`, `eval5`, `getelement2`, `varcont`, `dotimesl`, `dodivl`), `programs/blitz2/src/parse.c` (a comment at `makelmul` and in `calcvar` on the low word that A0 relies on); shared by `blitz2-ab3`.
- Tests: the matrix's bug-orig tests of #1179 (`tests/bug-orig/compiler/compiler-expr-1179-*`: `; @fixed #1179` by the generator; `EM_R_1179` into `EM_RULES_PORTNOW` in `tools/lib/exprmatrix/exprmatrix.h`), a hand-written bug-orig test `tests/bug-orig/compiler/compiler-array-later-index` for what the matrix lacks, an expect test `tests/blitz2/array-index-code` (code size and library calls of the index forms), the corpus and CD corpus, the prev comparison (`PREV_INTENDED`), `tests/ab3` if AB3 mode changes.
- Docs and site as listed above; site fixed-bugs entry, `new-errors.md` entry and overview row (coordinator); a comment on #1179 (its text predicts `b(1,q*10)` becomes `b(1,10)`, which #1178 changes: after both fixes it is `b(1,15)` in both builds).
- Related: #1178, #1180, #1182, #1037 (65535 is never a valid index).
