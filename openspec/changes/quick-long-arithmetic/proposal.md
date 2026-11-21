# Proposal

## Why

blitz2 (like the original compiler) compiles an expression with one "type so far". A Quick (16.16 fixed point) that is fetched while the type so far is Long is converted to Long at once and its fraction is cut off (`varcont`, `blitz2.s:10491-10498`; `programs/blitz2/src/eval.c:2893`). The operation is then done with the whole number, not with the value: with `q=1.5`, `r.l=q*2` gives 2, `r.l=l-q` (l=10) gives 9, `r.l=q/h` (h=0.5) gives 2147483647 (`h` is cut to 0), `NPrint 1+1+1+q` prints 4. The result also depends on the order of the operands (`x.w=w+v+q*2` is 9, `x.w=q*2+w+v` is 10). `docs/blitz2/expression-types.md` has the rule and a measured table; issue #1178 lists the 376 places in the CD's sources where it applies, 18 of which give another result than the source says.

Issue #1178 first recorded this as a potential problem; the user has since classified it as an original bug to fix with the full workflow (labels `bug-orig`, `wrong-result`; severity 1) (the rule is deliberate: without it `adr.l=base+i*4` would be computed in Quick, range +-32768). The decision of 2026-11-21 (`task-decisions.md`, "#1178 direction: variant C") is to change it by variant C: the expected type flows as today, a Quick is not cut when it is fetched, and each operation is computed with the Quick's fraction and rounded down only when it must give a whole-number type. Computed with a model over the 376 places (not measured): 358 places unchanged, 18 right, 0 break; the 18 are the places whose result differs today (`tr-quick-arith-variants-report.md`).

## Dependency

This change depends on the change for #1177 (comparisons), which is done first: both need the Quick not cut at fetch and the operand types at the operator. No parallel work (user decision, `task-decisions.md` 2026-11-21).

## What Changes

- **A Quick is not cut when it is fetched.** The special case in `varcont` goes; a Quick operand under a Long (or Word) type so far is converted to a whole number only where the operation must give a whole number.
- **Each operation is computed with the Quick's fraction and rounded down once, at the end of the operation:** `Long OP Quick` gives `floor(l OP q)` for `+ - *`, and for `/` the quotient rounded towards zero; a Quick OP Quick stays a Quick; a Float operand makes a Float (as today). `l+q` is `l+floor(q)` as today; `l-q` is `l-ceil(q)`; `l*q` needs the 32x32-bit product shifted right by 16; `l/q` needs `(l<<16)/q`.
- **What stays as today:** the type so far flows as it does now (destination or parameter, a Long operand to the left, Word+Word and Word*Word, a shift); constants take the type so far; the order dependence stays (`base+q*4` and `q*4+base` still compute in different types). Order independence is the goal of a later step (an expression tree in the expression compiler, its own issue).
- **For a Quick without a fraction the result is today's:** `floor(l OP q)` equals `l OP floor(q)` for `+ - *` (and `/`, which rounds towards zero like BB2's Long division), as long as no intermediate value overflows.
- **Library routines** for the mixed multiplication and division, so that a Long times a Quick and a Long divided by a Quick get a 64-bit intermediate (new entries in `lmullib`/`ldivlib`; a program compiled by the port then needs the port's libraries, entry in `docs/libs/deviations.md`).
- **Results change:** the executables of programs that mix a Quick with a Long or Word in arithmetic (the compiled code of those expressions), and the library files that get new routines. Named for the previous-commit comparison.
- **Documentation:** `docs/blitz2/expression-types.md`, the site section "The type of an expression" (`site/content/docs/how-blitz2-works/compiling.md`), the deviation entry (kind F, in `docs/blitz2/deviations.md`, created by a separate task), `docs/libs/deviations.md`, `docs/ab3/variant.md` (AB3 mode gets the new rule too).

## Capabilities

### New Capabilities

- `compiler/expression-types`: what an arithmetic operator gives when one operand is a Quick and the other, or the type so far, is a whole number. (The existing specs have no capability for expression typing: `compiler/intrinsics` is about inline library calls, `ab3-compat/type-rules` about AB3 mode's type rules; see design, decision 1.)

### Modified Capabilities

(none)

## Impact

- `programs/blitz2/src/eval.c` (`varcont`, `doplus*`, `dominus*`, `dotimes*`, `dodiv*`, the type bookkeeping of `doop`), shared code that `blitz2-ab3` links (`programs/blitz2/CMakeLists.txt`).
- `libs/acidlibs/lmullib.c`, `libs/acidlibs/ldivlib.c` (system libraries `$CA00`-`$CA01`, `$CB00`-`$CB01`; `main.h:106-109`): new entries; `tests/perf` budgets for them.
- Tests: `tests/blitz2/quick-arith-long.bas` and `.expect`, new cases, the library capture tests (`libs/tests`), the prev comparison (`PREV_INTENDED`).
- Docs and site as listed above; issue #1178 (labels, comment).
- Related: #1177 (comparisons, same code area), #1179 (array index type), the later expression-tree issue.
