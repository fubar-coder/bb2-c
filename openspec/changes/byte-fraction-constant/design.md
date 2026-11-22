# Design

## Context

`consttype2` (`programs/blitz2/src/libcall.c:404`, main c336cf473) sizes a literal by the type so far. Its `.byte` path (`:438`) calls `makeint` (SPFix, towards zero) for every literal, also one with a point; the `.word` path first sends a literal with a point to `numdofloat` (Float), `.quick` keeps the fraction as a Quick, `.long` and `.float` keep the Float. Type 0 jumps to `.byte` only for a literal without a point. `consttype2` serves expressions (via `fetchnum3`) and `Data` (`fetchdata`, `tokens1.c:698`, `dodata`): `Data.b 1.5` is accepted today as 1, `Data.w 1.5` is "Bad Data" (the Float type does not match).

#1186 (5f8a73fc1, a port defect of #1177) added `litcut`/`litat`/`fraclitfloat`: a literal that the `.byte` path cut is re-emitted as its Float when it is the whole left operand of a comparison (`cmpownleft`, `eval.c:1167`). The matrix (`expression-type-matrix`) has the fixed rule as `EM_R_1188` in `EM_RULES_FIXED` and its expected values in `compiler-expr-1188-*`; AmiBlitz3 v3.14.0 keeps the fraction (Q-F: 61 cells).

## Goals / Non-Goals

**Goals:** a fractional literal under Byte behaves as under Word; nothing else changes.

**Non-Goals:** the Float-to-Byte rounding, Byte overflow, `Data` (decision 3).

## Decisions

### 1. Where: the `.byte` path of `consttype2`

Under a Byte type so far (not type 0), a literal with `numfloat` set and a non-zero fraction goes to `numdofloat` as in `.word`. Type 0 keeps its path (a literal with a point is already a Float there). The Byte result is converted at the end by the existing Float-to-Byte conversion.

### 2. A whole-valued literal with a point stays whole (F2)

* **F1:** every literal with a point is a Float under Byte, as `.word` does: `rb=vb+2.0` becomes a Float computation (SPFlt, SPAdd, SPFix calls) where today it is one `add.b`; values the same except overflow corner cases.
* **F2 (chosen):** only a non-zero fraction makes the literal a Float (`SPFlt(makeint(v)) == v` tests it, as `.quick` tests `frac == 0`); `2.0`, `1e2` stay the whole number they are today.

Choice (mine, planning): F2. It changes no executable and no speed where the value does not change (D-007: a Byte expression with `1.0` in a loop would otherwise become library calls), and fewer executables change (rule 3). It differs from `.word`, which makes `2.0` a Float; that difference is named in `docs/blitz2/expression-types.md`. `Point: F2 keeps whole-valued point literals as whole numbers under Byte, unlike Word — L1 Unlikely / I3 Observable / E2 Reasoned → Plan` (the observable part: `rb=vb AND 2.0` keeps compiling, and Byte overflow of an intermediate stays as today for these literals).

### 3. `Data.b` keeps the cut

`consttype2` gets to know whether it sizes data (the `data` flag `fetchnum3` already has); for data the `.byte` path stays as it is. Otherwise `Data.b 1.5` would become "Bad Data" (as `Data.w 1.5` is), a new compile error that the issue does not ask for. `Point: Data.b with a fraction keeps today's value 1 — L1 Unlikely / I3 Observable / E2 Reasoned → Plan` (choice mine; the alternative, "Bad Data" as for `Data.w`, is the user's to ask for later).

### 4. The #1186 helpers

After decision 1, `litcut` is set only for a whole-valued literal (decision 2), where re-emitting it as a Float or leaving it as a whole number gives the same comparison result. Options: remove `litcut`/`litat`/`fraclitfloat` and their call in `cmpownleft` (less code; `rb.b=(2.0=vf)` changes its bytes), or keep them (no byte change). Choice (mine): the implementer removes them if the prev comparison shows no changed executable outside this change's named ones, else keeps them and says why. `Point: removing the #1186 helpers changes the code of a whole-valued literal left of a comparison under Byte — L1 Unlikely / I1 Low / E2 Reasoned → Monitor`. `regression/compiler-compare-fraction-const` (#1186's test) must pass unchanged either way.

### 5. Bit operators (decided by the user)

With decision 1 the literal is a Float and `doop` has no Float handler for `AND`, `OR`, `EOR`, the shifts, `NOT`: "Illegal Operator for Type", as `rw.w=1.5 AND vw` is today; the matrix's 29 `compiler-expr-1188-refused-*` tests expect this. The alternative keeps the cut for these operators: it needs the next operator at the time the literal is read, which `consttype2` does not know (a look-ahead, or a re-emission like #1186's: a sixth patching mechanism the tree must replay). Decision (the user, 2026-11-22, "Last answers of the session"): "Ablehnen wie unter Word (Recommended)". `Point: rb=1.5 AND vb, the shifts and NOT 1.5 become compile errors under Byte — L2 Possible / I3 Observable / E2 Reasoned → decided by the user`. Task 3.2 counts the CD sources with the form.

### 6. Order with #1189 (planning choice)

`compiler-expr-1188-1189-db-B-01` needs both fixes; it is REPRODUCED now, DIFFERS between the fixes, FIXED after both (the matrix's rule for two-issue tests). Both changes edit `eval.c` (this one through decision 4, #1189 through `dopow`'s comment), so they run one after another. #1188 first: it is listed first in the user's step 6, and its matrix cells are refused or value cells that #1189's regeneration then takes as given. `Point: order #1188 before #1189 — L1 Unlikely / I4 High / E2 Reasoned → Plan`.

### 7. The matrix at the fix

`EM_R_1188` goes into `EM_RULES_PORTNOW`; `expr-matrix-update` regenerates. Where the fixed cells go is decided (the user, 2026-11-22, "Last answers of the session": "Ja für alle" - zumindest solange bis die Test-Infrastruktur fertig umgebaut ist, T1): they stay in their bug-orig tests with `; @fixed #1188`. The 16 Q-A cells #1188 changes (Byte overflow after a Float computation) and the 11 Q-E and 1 Q-D cells are baseline cells changed by this fix: named in the commit.

### 8. AB3 mode (decided by the user)

One core; the fix applies in AB3 mode without a condition and removes a difference from AmiBlitz3 (its Q-F rows in `docs/ab3/variant.md` written by the matrix's task 3.5 go). Decision (the user, 2026-11-22, "Coordinator session (handoff-2026-11-22-quick.md), answers", D2): "Beide Modi (Recommended)".

### 9. Cost (D-007)

Only expressions with a fractional literal under Byte change; they become Float computations, as the same expression under Word is today. A correct program that relied on the cut (`rb=vb*1.5` meaning `vb*1`) is not a correct program for this bug. Whole-valued literals: no change (decision 2). No hot-path cost for correct input.

## Risks / Trade-offs

* [A Byte context with a compile-time constant requirement (a Byte parameter that must be constant, `cfetchmode`) now gets a Float] → the CD corpus and `corpus` (task 3.2) show a new compile error; reported before the merge.
* [AB3 mode differs from BB2 mode in sizing] → one core; `blitz2-ab3` BB2-mode identity (`ab3-compat/bb2-equivalence`) holds because both get the fix.
* [The matrix's model and the fixed compiler disagree on a cell] → the matrix tests are the check (task 3.1); a disagreement is a model or code error, resolved before the merge.
