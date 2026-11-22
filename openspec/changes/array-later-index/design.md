# Design

## Context

`calcvar` (`programs/blitz2/src/parse.c`, the `.notalist` branch) evaluates the first index with `arreval` (a Long, `maineval2` with type Long and no push), runs `arrszchk` (`cmpi.l #65536`) when `debugga` is set, multiplies it by the element size and then, per further dimension, runs the dimension check (`cmp.l x(a5),Dn`, checked build), evaluates the next index with `arrevalchk` and adds it with `makelmul`. `arrevalchk` (`eval.c:621`) is `sharreval` (a Word evaluation) without runtime error checking and `arreval` + `arrszchk` with it. `makelmul` (`parse.c:1279`) reads only the low word of the later index (`move dr,d(r-1)`, `mulu x+2(a5),dr`, `mulu x(a5),d(r-1)`: "d1.w=this subscript", `blitz2.s:8082`); the checked build's compare instructions read the whole Long.

The measured variants and the prototype (`#define IDXVAR`, `~/.local/share/bb2-c-work/tr-1179-variants-cost/prototype.diff`) were built on `main` at `c336cf473`, before #1178. The user chose A0 (proposal, "Recorded decisions").

## Goals / Non-Goals

**Goals:** a later index evaluated as a Long in both builds; the A0 code forms (inline Word `*`/`/`, no upper word in the unchecked build) with the Long values; today's code for the common forms.

**Non-Goals:** see proposal, "Not in scope". No change of `makelmul`, of the first index or of `arrszchk`.

## Decisions

### 1. Direction A in `arrevalchk` (recorded)

`arrevalchk` evaluates with `arreval` in both builds; `arrszchk` only with `debugga >> 8`. By precedent: "Keine Prüfung im Release (Recommended)" (2026-11-21) and "A0 (Recommended)" (2026-11-22). Without the checking an index the checked build would report wraps modulo 65536 as today (the same entry; no rating). The refusal of a bit operator with a fraction constant (`ja(1,vq AND 1.5)`, "Illegal Operator for Type") follows from the Long typing as for the first index and the checked build today; the matrix committed it as the expected value of the 7 tests `compiler-expr-1179-refused-*` (`expression-type-matrix` design, "Modes and files"). Answered by precedent (direction A and the issue's "Expected": every index "as a Long, like the first index and like the checked build"); named as a new compile error for the site (`new-errors.md`).

### 2. The Word-value mark for the inline forms (A0, recorded; the form is planning's)

Per entry of the constant-fold stack (`forthstack`) a mark "word value": set in `varcont` for a Byte or Word variable and in `getelement2` for an integer literal or `#constant` from 0 to 32767; cleared by every operator result and by unary minus. A flag in `maineval2` (saved and restored, so a nested evaluation such as a function argument or an index of an index does not inherit it) says "this evaluation is a later index". In `eval5`, when both operands carry the mark and the flag is set, `dotimesl` emits `muls.w` and `dodivl` emits `divs.w` (decision 3) instead of `longmult` / `longdiv`. This is the prototype's mechanism (IDXVAR 8), measured on the matrix and the CD.
Not marked (no value changes, only cost): function results of Byte/Word type, `MOD`, results of an earlier operation known to fit in a Word.
`Point: a form that could be inline stays a library call (function results, MOD) — L3 Likely / I1 Low / E3 Proven (prototype notes) → Monitor` (mine).

### 3. The overflowing Word division

With two sign-extended Word operands `divs.w` overflows only for -32768 / -1 (V set, destination unchanged); the Long division gives 32768. A0 adds "no value" (variants file, "Answer"), so the inline form must give 32768: `divs.w Dy,Dx` / `bvs.s` to `neg.l Dx` / else `ext.l Dx`, in both builds (the unchecked build needs it too: a following `/`, `ASR`/`LSR` or comparison reads the upper word of the quotient). Alternatives: inline only in the unchecked build (the checked build keeps `ldivlib`; against A+'s "Word mul/div faster" in the checked build that A0 includes), or accept -32768 (the checked build would report `g(1,wv/wm)` that it accepts today: a result change no decision names).
`Point: the fall-back costs about 2 to 4 bytes and a not-taken branch per inline Word division beyond the prototype's +4 bytes / +8 cycles — L4 Certain / I1 Low / E2 Reasoned → Plan` (mine; measured in task 3.2).

### 4. No upper word in the unchecked build (A0, recorded; the form is planning's)

Without the checking, after `arreval` of a later index: if the emitted code is exactly `move.l #n,Dn` (checked by opcode, not by length: a folded comparison is `move.w #n` + `ext.l`, which broke 96 matrix cells in a first prototype), it becomes `move.w #n,Dn`; else if the last code is the Byte/Word/Quick -> Long conversion of the element (`conv.end == pc`, `conv.to == T_LONG`, at this evaluation depth), its final `ext.l` is dropped. On top of #1178 the conversion of a raw Quick at the end of the evaluation may be emitted by another path than `convtype`; task 2.3 checks that the peephole still finds it (the measured A0 forms must stay at 0 cycles / 0 bytes against `main` before this change).
`Point: A0 relies on makelmul reading only the low word; a later change there (indexes above 65535) silently breaks it — L2 Possible / I2 Medium / E2 Reasoned → Plan` (mine, the analyst's rating): a comment at both places (`arrevalchk`, `makelmul`), and the range cases of the hand-written test (task 1.2) and the code test (task 2.4) cover it.

### 5. Tests

* **The matrix** (`expression-type-matrix`, the regression test): `EM_R_1179` joins `EM_RULES_PORTNOW`; `expr-matrix-update` writes `; @fixed #1179` into the headers of `compiler-expr-1179-i2-I-u-01` and of the 7 `compiler-expr-1179-refused-*` (T1, recorded); no `.expect`, `.orig` or corpus file changes. Every agreement and fixed cell keeps its value, among them the 49 cells of the `i2-*-u-agree` sources with a Quick operand that A on today's main moved (variants file); the checked `i2` sources (2556 cells) do not change.
* **A hand-written bug-orig test** `tests/bug-orig/compiler/compiler-array-later-index` (old form `.bas` / `.expect` / `.orig`, without the checking): the cases the matrix lacks (its index cells read a `Dim` of 300, two dimensions, variables and literal constants): the issue's table (three dimensions, a NEWTYPE element array, a store, a Quick function, a Float, `g(1,y*200/2)`, `g(1,y*300/2)`, `q ASL 4`), the large indexes (40000, `wv*2`, `wv*3`, `lb/2`), Word `*` and `/` with products beyond 32767 and -32768 / -1. Its `.expect` is the Long evaluation by #1178's rules; it is checked against the port's checked build of the same lines after #1178 (where the checked build gives no runtime error, it is the expected value). Lines whose port value already differs from 1.95 on main are left out, so the test is REPRODUCED before the fix.
* **An expect test** `tests/blitz2/array-index-code` (with `; @runerrs off` and a checked twin `array-index-code-runerrs`): the code bytes between two labels (`?b-?a`) of `a(iw,jw)`, `a(5,7)`, `b(iw,jw,kw)`, `a(iq,jq)` and the JSR count (`$4EB9`, as the intrinsics tests do) of `a(iw,jw*kw)`, `a(iw,jw/2)`, `a(il,jl*kl)`: pins A0's code (spec, "The index code of a plain later index does not grow") so that a later change (the tree's steps) cannot grow it silently. Its `.expect` is written from the port after the fix and checked against the byte counts of today's code for the forms A0 keeps.
* Test form: the old form, as the matrix's open tests are and as `compare-right-operand` chose; the BASIC test model is not usable for compiler tests yet (`basic-test-runner`, runner phase 2 after the Quick fixes, 2026-11-21 "Quick fixes first; #1177 test in the old form"): prescribed by the state of the infrastructure, not rated.

### 6. Cost after #1178 (D-007)

The cost table was measured before #1178. A Quick operand in a later index is computed by #1178's Long rules after both changes (for example `q*4` with a Quick and a constant: the new mixed routine of `lmullib` instead of today's Quick multiplication in the Word evaluation), which the variants file could not measure ("Open"). Task 3.2 measures the forms of the cost table again on top of #1178 with the analyst's benchmark (`tr-1179-variants-cost/bench/abench.bas`, `abytes.bas`). D-007 ("ask when unsure"): if a form gets slower than today's unchecked code by more than the A0 table's +16 cycles (the `m(x+21 AND 255,y+j AND 63)` row), the numbers go to the coordinator for the user before the fix commit (task 3.2 stops there).
`Point: Quick multiplications in a later index get slower after #1178 + #1179 than today's unchecked Word evaluation — L2 Possible / I1 Low (speed alone) / E1 Assumption → Monitor` (mine); the gate of task 3.2 raises the evidence, and D-007 puts the result to the user if it is a slow-down.

### 7. AB3 mode (decided by the user: both modes)

One core: without a mode condition the fix applies in AB3 mode too. AmiBlitz3 v3.14.0 evaluates a later index as a Word without the checking unless its 68020+ option (`optimize_integer`) is set (read from the source, not run; `docs/ab3/variant.md`, row on array indexes, point 3). The user's answer (proposal, "Decisions of the user", 1): both modes; task 4.1 writes the row.

### 8. Overlap with other open changes

Files this change edits that other open changes edit too: `programs/blitz2/src/eval.c` (`quick-long-arithmetic`, `compare-right-operand`, `byte-fraction-constant`, `power-negative-base`, `compare-compound-left`, `compiler-function-optimizer`, `expr-tree-*`), `programs/blitz2/src/parse.c` (`expr-tree-3a-leaves`, `expr-tree-3b-arrays`: `calcvar`), `docs/blitz2/expression-types.md`, `docs/blitz2/deviations.md`, `docs/ab3/variant.md`, `site/content/docs/how-blitz2-works/compiling.md`, `tools/lib/exprmatrix/exprmatrix.h` (`quick-long-arithmetic`, `byte-fraction-constant`, `power-negative-base`: `EM_RULES_PORTNOW`). The order with the Quick work and the tree is recorded (proposal). `compiler-function-optimizer` has no recorded order against this change: not in parallel, no order set here.
`Point: this change and compiler-function-optimizer meet in eval.c — L1 Unlikely (different functions: afunction against the index and operator paths) / I2 Medium (a rebase of one change) / E2 Reasoned → Monitor` (mine).

## Risks / Trade-offs

* [The Word-value mark survives an operator or leaks into a nested evaluation and a value is computed in 16 bits] → cleared by every operator result and unary minus, the flag saved in `maineval2`; the matrix's 1341 + 2556 `i2` cells and the hand-written test's products beyond 32767.
* [The A0 peephole removes an `ext.l` that is not the element's conversion] → checked by opcode and depth, not by length; the matrix (96 cells broke in the first prototype) and the code test.
* [#1178 changes the code path of a Quick at the end of an index] → task 2.3 measures the A0 forms again on top of #1178.
* [Many executables change] → the prev comparison names them (task 3.3); on the CD 1 of 496 (measured before #1178).

## Planning choices (mine, not the user's)

* Decision 2, the mark's coverage: L3 / I1 / E3 → Monitor.
* Decision 3, the overflow fall-back of the inline division in both builds: L4 / I1 / E2 → Plan.
* Decision 4, the reliance on `makelmul`'s low word, covered by comments and tests: L2 / I2 / E2 → Plan.
* Decision 5, the code test `tests/blitz2/array-index-code`: pins the code size; `Point: the code test fails on a later intended change of the index code and must be updated — L3 Likely / I1 Low / E2 Reasoned → Monitor`.
* Decision 6, the cost gate after #1178: L2 / I1 / E1 → Monitor (the result can make it a D-007 question to the user).
* Decision 8, no order against `compiler-function-optimizer`: L1 / I2 / E2 → Monitor.
* No new budget in `tests/perf` (it measures library commands; the index code is the compiler's, pinned by the code test instead): `Point: a later change makes an index slower without growing its code — L1 Unlikely / I1 Low / E2 Reasoned → Monitor`.

By precedent (no rating): direction A and no release check ("#1179, CRC findings, trailers, merge"); A0 with inline Word `*`/`/` in both builds and no upper word in the unchecked build ("Last answers of the session", A0 = A+ plus the unchecked narrowing, variants file table); the order #1178 -> #1179 -> #1182 ("Order of the Quick work; matrix test"); the matrix cells in their bug-orig tests with `; @fixed #1179` (T1, "Last answers of the session"); NEWTYPE arrays out of scope; the refusal of decision 1.

## Open Questions

* None. AB3 mode is answered (proposal, "Decisions of the user", 1: both modes, no mode condition).
