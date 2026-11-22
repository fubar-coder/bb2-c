# Proposal

## Why

`^` is computed for every operand type in Float with mathtrans.library's `SPPow` (`dopow`, `programs/blitz2/src/eval.c:1476`, which calls library 192's first entry, the inline `jsr -90(a6)` of `libs/acidlibs/mathtranslib.c:115`). `SPPow` returns |x|^y: a negative base loses its sign, also for a whole-number odd exponent. `rl.l=vf^nw` with `vf=-2.5`, `nw=3` gives 15 (expected -15), `rf.f=vf^nw` 15.625 (expected -15.625), `(-1.5)^3` 3. blitz2 1.95, the CD's 2.10, the port (both library sets) and AmiBlitz3 v3.14.0 give the same (issue #1189, found by the expression matrix, `~/.local/share/bb2-c-work/expr-matrix/classify-report.md`, "New finds" 1; D-021 class (a)). The manual: "LHS to the power of RHS".

User decision (2026-11-22, `task-decisions.md`, "Matrix: last Q-F and Q-E cells"): for the 117 Q-E cells (negative base, fractional exponent) "Baseline wie heute (Recommended)" — "the original's measured value as baseline; #1189 fixes only whole odd exponents; whether these become a runtime error stays open in #1189". Earlier ("Matrix questions Q-A, Q-C, Q-D, Q-E"): Q-E "Original, Vorzeichen per #1189 (Recommended)", and the user's correction that these answers are a baseline, not a rule (`1000^2` = 999999 stays a topic of its own).

## Dependency and order

* After `quick-long-arithmetic` (#1178: `dopow` takes a raw Quick with its fraction, its decision 5; 4 matrix tests `compiler-expr-1178-1189-*` need both), after `compare-right-operand` (#1182, step 5 of "Go for steps 1-7") and after `byte-fraction-constant` (#1188; planning choice, its design decision 6; `compiler-expr-1188-1189-db-B-01` needs both).
* Edits `eval.c` (the comment at `dopow`) and `mathtranslib.c`: one after another with the other `eval.c` changes (user: "changes that edit programs/blitz2/src/eval.c one after another").

## What Changes

- **A negative base with a whole-number exponent gives the signed power:** |x|^y from `SPPow`, negated when the exponent is odd. `(-2.5)^3` -15.625, `(-2)^2` 4, `(-2)^-1` -0.5. Whole means the exponent's Float value has no fraction (a Byte, Word, Long exponent; a Quick or Float with a whole value). The magnitude stays `SPPow`'s (its precision, `1000^2` 999999, is not in scope).
- **A negative base with a fractional exponent stays as today** (|x|^y; the recorded baseline); whether it becomes a runtime error is open (question 2).
- **Where:** the library routine of `^` in BB2's math library 192 (`mathtranslib`): today an inline `jsr -90(a6)`, after the fix a routine that passes |x| to `SPPow` and sets the sign for an odd exponent (design, decision 1; open question 1).
- **Results change** (rule 3, `PREV_INTENDED`): the library file of `mathtranslib`; the executable of every program with `^` (the inline call becomes a call of the routine); outputs of programs with a negative base and a whole odd exponent; the matrix's #1189 cells (744 cells in 24 tests, and the two-issue tests); the library capture case of `mathtranslib`'s first entry.
- **AB3 mode** uses AmiBlitz3's library 44 for `^` (`ab3-compat/libraries`, "The float power uses AB3's math library"), so the fix of decision 1 does not reach AB3 mode: AB3 mode keeps AmiBlitz3's result (open question 1).
- **Documentation:** the kind-F entry in `docs/libs/deviations.md`, `docs/blitz2/expression-types.md` (`^`), `docs/ab3/variant.md` (AmiBlitz3 has the bug; AB3 mode keeps it), the `BUG(orig) (#1189)` comment at `dopow` (`eval.c:1473`) becomes `Fixed (#1189)` with a pointer to the routine.

## Not in scope

- The precision of whole powers (`1000^2` gives 999999): a topic of its own (the user's correction of 2026-11-22).
- A negative base with a fractional exponent: baseline as today (decided); a runtime error for it is open (question 2) and not built here.
- `0^y`, overflow, `^` of a Quick beyond what #1178 sets.
- AmiBlitz3's library 44.

## Open questions

1. **Where the fix lives, and AB3 mode:** (P1, recommended) the library routine of library 192: BB2 mode only, AB3 mode keeps AmiBlitz3's library result; (P2) code that the compiler emits around the call in both modes (about 30-40 bytes per `^`, AB3 mode then deviates from AmiBlitz3's library 44, named in `docs/ab3/variant.md`). #1178's "Neue Regel auch in AB3" was given for #1178; the choice here is the user's. Worked with: P1.
2. **A negative base with a fractional exponent** (no real result): stays as today (the recorded baseline), or later a runtime error ("in #1189 open"); if an error, by D-005 in the error check area only, or (D-006) in release code if the operands typically come from outside. Not built in this change; worked with: as today.
3. **Where the matrix's #1189 cells live after the fix:** as `quick-long-arithmetic`'s open question 1 (T1/T2/T3). Worked with: the answer given for #1178, if the user gives it for every matrix issue.

## Capabilities

### New Capabilities

- `compiler/expression-types`: the power operator with a negative base. (Created by whichever of `quick-compare-types`, `quick-long-arithmetic`, `compare-right-operand`, `byte-fraction-constant` and this change is archived first.)

### Modified Capabilities

(none in `openspec/specs/`; `ab3-compat/libraries` is unchanged: AB3 mode keeps library 44)

## Impact

- `libs/acidlibs/mathtranslib.c` (the `^` entry from `BLITZ_INLINE` to a routine), `programs/blitz2/src/eval.c` (comment at `dopow`; with P2 the emitted code).
- Tests: the matrix's `compiler-expr-1189-*` (24), `compiler-expr-1178-1189-*` (4), `compiler-expr-1188-1189-db-B-01`; a hand-written bug-orig test `compiler-pow-negative-base` for the cases the matrix lacks; `libs/tests/systest_cmdlibs.c` (`mathtranslib inline` entry 0: `[orig]`/`[fixed #1189]`, and a call case); `tests/perf` (a line for `^`); the prev comparison.
- Docs as listed; site entry and overview row (coordinator; topic "Numbers & dates" as the issue proposes).
- Related: #1178, #1188.
