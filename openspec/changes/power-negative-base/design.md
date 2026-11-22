# Design

## Context

`dopow` (`programs/blitz2/src/eval.c:1476`, main c336cf473) converts both operands to Float, marks the operation not foldable (`nocando`) and emits library token `mathq*128+1`. In BB2 mode `mathq` is library 192 (`mathtranslib`), whose first entry is `BLITZ_INLINE(dopow, "jsr -90(a6)")` (`libs/acidlibs/mathtranslib.c:115`): the compiler copies the `jsr` into the program, a6 is the mathtrans base. In AB3 mode `mathq` is AmiBlitz3's library 44 (`ab3-compat/libraries`, "The float power uses AB3's math library"), a binary library file of AmiBlitz3. `SPPow` (the ROM's mathtrans) gives |x|^y for a negative x. The library capture test reads the inline bytes of mathtranslib's entries 0-13 (`libs/tests/systest_cmdlibs.c:336-341`). The matrix's fixed rule `EM_R_1189` (`EM_F_POWSIGN`): "the sign of an odd power applied to what SPPow gives", the model's expected values in `compiler-expr-1189-*`.

## Goals / Non-Goals

**Goals:** the sign of a whole-exponent power with a negative base; the value of every other `^` unchanged.

**Non-Goals:** SPPow's precision (`1000^2`), a fractional exponent with a negative base (baseline), AmiBlitz3's library.

## Decisions

### 1. The fix in the library routine (P1; decided by the user)

`mathtranslib`'s `^` entry becomes a routine with the register contract of its neighbours (`libs/register-contract`; d0 = x, d1 = y, a6 = mathtrans base, result d0):

* `x` not negative (FFP sign bit 7 clear): `jmp -90(a6)` directly (cost for the common case: the call of the routine and one test).
* `x` negative: clear the sign bit, keep `y`'s parity (decision 2), `jsr -90(a6)` with |x|, set the sign bit of the result for an odd whole `y`.

Passing |x| does not depend on what a given mathtrans version does with a negative base, and gives today's value wherever the sign does not change (fractional exponent: |x|^y, the baseline).

Alternative P2: the compiler emits the same logic inline around the call, in both modes (30-40 bytes per `^`, an extra code form the tree must replay, and in AB3 mode a result that differs from AmiBlitz3's library 44). P1 keeps the fix where the call to `SPPow` is, one copy per program, and leaves AB3 mode with the library it is specified to use. Decision (the user, 2026-11-22, "Coordinator session (handoff-2026-11-22-quick.md), answers", D3): "P1 Library 192 (Recommended)" -> BB2 mode only; AB3 mode keeps AmiBlitz3's library 44 result, named in `docs/ab3/variant.md`. `Point: P1, BB2 mode only; AB3 mode keeps AmiBlitz3's sign error — L2 Possible / I3 Observable / E2 Reasoned → decided by the user`.

### 2. Whole and odd from the FFP bits

FFP: 24-bit mantissa (bits 31-8, normalised, top bit set), sign bit 7, exponent bits 6-0 excess 64. With e = exponent - 64: e <= 0 and y not 0: |y| < 1, fractional; e >= 24: whole and even; otherwise whole if the mantissa's low 24-e bits are 0, odd if bit 24-e of the mantissa is set. y = 0 (all bits 0): whole, even. No library call, about 10 instructions, only on the negative-base path.

### 3. Tests

The matrix's open tests are the red tests: `compiler-expr-1189-*` (24, value set B, every context), `compiler-expr-1178-1189-*` (4; DIFFERS between #1178 and #1189 by the matrix's rule, FIXED after both) and `compiler-expr-1188-1189-db-B-01`. A hand-written bug-orig test `compiler-pow-negative-base` adds what the matrix lacks: an even exponent (`(-2)^2`, unchanged), a negative odd exponent (`(-2)^-1`), a whole-valued Float and Quick exponent (`3.0`), an exponent beyond 2^24 (even), a zero exponent, a fractional exponent (baseline, in a regression test). The library capture: entry 0 of `mathtranslib inline` becomes an `[orig]`/`[fixed #1189]` pair, plus a call case with a negative base (the libs suite, `libs_tests`).

### 4. Cost (D-007)

`^` costs `SPPow` (exp and log, several thousand cycles on a 68000). The fix adds, for a non-negative base, the call into the routine and a sign test (about 30 cycles with `jsr`/`jmp`); for a negative base about 10 instructions more. A new line in the benchmark (`tests/perf/bench`, `budgets.txt`) with a positive and a negative base, target the CD's library (the inline call). No existing budget rises. `Point: the extra call slows a loop with ^ measurably — L1 Unlikely / I1 Low / E2 Reasoned → Monitor` (speed alone is not I3; the report says whether the fix adds work on the hot path for correct input, as D-007 asks; if the benchmark shows more than the noise for the positive base, the coordinator asks the user, D-007 "ask when unsure").

### 5. Results

Every executable with `^` changes (inline `jsr -90(a6)` becomes a call of the routine), and the library file of `mathtranslib`. Outputs change only for a negative base with a whole odd exponent. The prev comparison names them.

### 6. The fractional exponent (open question 2, still open)

Not built. If the user later wants a runtime error, D-005 puts it in the error check area (a programming error) unless D-006 applies (data from outside); that would be a change of its own. The proposal and the issue keep the question open.

## Risks / Trade-offs

* [The FFP parity test is wrong near e = 24 or for denormal-looking bit patterns] → the bug-orig test's edge cases (decision 3); FFP has no denormals, every non-zero value is normalised.
* [An inline entry turned into a routine changes how the compiler links the library] → the library build's checks (`blitzlib check`) and the capture test; if the library format cannot have it at the same token, the implementer stops and reports (I2, this change re-planned).
* [The matrix's model predicts SPPow's magnitude negated, the routine computes it from |x|] → the same value: SPPow returns |x|^y for x and -x alike (measured with 2.10, 1.95, the port and AmiBlitz3; `classify-report.md`); the matrix tests check it.
* [AB3 mode keeps the bug] → named in `docs/ab3/variant.md` (decided: P1).

## Planning choices (mine, not the user's)

* Decision 2, the parity test from the FFP bits rather than `SPFix`/`SPFlt` round trips (no extra library calls, no overflow of `SPFix` for large exponents): L1 / I1 / E2 → Monitor.
* Decision 4: L1 / I1 / E2 → Monitor.
* Order after `byte-fraction-constant`: that change's design decision 6 (L1 / I4 / E2 → Plan).

## Findings (implementation)

* **The matrix tests after the regrouping** (one source per group of cells, merged before this change): `compiler-expr-1189-*` are 12 tests now, not 24 (the 744 cells unchanged); with the 4 `compiler-expr-1178-1189-*` and `compiler-expr-1188-1189-db-B-01` 17 tests. On main a56d8be9f: the 12 REPRODUCED, the 5 two-issue tests DIFFERS (task 1.1).
* **Decision 2, the bound e = 24:** "e >= 24: whole and even" is off by one. With e = exponent - 64 the value is M * 2^(e-24), so for e = 24 it is M itself, a whole number that is odd when bit 0 of M is set (16777215 = 2^24-1, FFP `$FFFFFF58`); whole and even holds from e = 25 on (2^24 and above). The rest of the rule ("whole if the low 24-e bits are 0, odd if bit 24-e is set") covers e = 24 as written. The routine and the tests take 0 < e <= 24 as the range of the bit test; the bug-orig test has `(-1)^16777215` (-1), `(-1)^16777216` (1) and `(-1)^8388609` (-1), the capture `-1 ^ 16777215`.
* **The library capture of the call** (task 1.3): the original's inline entry has no RTS, so the test calls it as the compiler does, its code copied with an RTS behind it; an entry that is not inline is called directly. Recorded with the original (`libs-capture`, filter `[cmd]`, machine a500-204): SPPow gives the same value for -2.5 and 2.5 (`$F9FFFB44`), `$C5A46945` for -2.5^3.5, 1 for -1^16777215. The contract stated (`KEEPS("mathtranslib pow", d2-d7|a2-a6)`): the SPPow call's, a6 stays the mathtrans base.
