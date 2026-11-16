## Why

`Frac(q)` of a Quick costs about 3750 cycles in `tests/perf` (`bmath.frac`):
`qutofl` turns the Quick into a Float (about 1900 cycles), mathffp computes
the fraction, and the assignment turns the Float back with `fltoqu` (about
1760). Even after the library change of libs-performance task 10.9 (issue
#1064, `Frac` with an argument of unknown type) the Float result and the
conversion back stay. `Len(a$)` and `Asc(a$)` cost about 680 and 760 cycles
(`bstr.len`, `bstr.asc`; the originals 731 and 797) although the library
routine itself is two instructions: blitz2 copies the whole string into the
string work area first (issue #1062). The library cannot fix this, because
the cost is in the code blitz2 generates around the call. blitz2 knows the
argument's type and the shape of the argument at compile time and can emit
the few instructions itself (an intrinsic).

## What Changes

- **Priority: `Frac` and `Int` of a Quick.** blitz2 emits the result as inline
  code instead of a library call when the argument is of type Quick:
  - `Int(q)`: the integer part, truncated towards zero (as the library does),
    as a Long, a few instructions; no type word is pushed, no call, and no
    `intchk` check in the runtime-error-checking mode (a Quick cannot fail it).
  - `Frac(q)`: when the result is assigned as a Quick (the whole right-hand
    side of an assignment to a Quick, nothing else in the expression), the
    fraction word with the sign of the value, as a Quick; no Float is made and
    nothing is converted back. In every other context (a Float context, part
    of a larger expression) the library is called as before (after #1064 with
    its fast Quick path).
- **`Len(a$)` and `Asc(a$)` of a plain string variable** (issue #1062): the
  length, or the first character, is read from the variable's memory
  header; the string is not copied into the work area and a3 is unchanged.
- **Candidates** for the same method are listed and measured, not decided
  (design.md, "Candidates").
- The compiled executables of every program that uses these calls change on
  purpose; the results of the programs do not.

Out of scope: any change of what a command returns; new error checks; the
library side of `Frac` (#1064, task 10.9); intrinsics for functions that are
not listed; an intrinsic for a Float argument.

## Capabilities

### New Capabilities
- `compiler/intrinsics`: when blitz2 compiles a library function inline, the conditions under which it does, and the guarantee that results, registers and error checks equal the library call.

### Modified Capabilities

None. (`testing/performance-budgets` exists only in the open change `libs-performance`; the new budgets are a task here, not a requirement change.)

## Impact

- `programs/blitz2/src/eval.c` (`function`, `libfunction`) and the argument
  evaluation in `programs/blitz2/src/compile.c` (`dolibtoke`, `fetchlibps`);
  `blitz2` and `blitz2-ab3` share the code.
- Every executable that uses `Frac`/`Int` of a Quick or `Len`/`Asc` of a
  string variable changes: `prev_results`, the corpus configuration `prev`
  and `ted_ab` `P5`/`Pc` need `-DPREV_INTENDED` for those programs; the
  corpus outputs (what the programs print) do not change.
- Library files and library tests do not change; `libs/tests/expected` stays.
- New expect tests in `tests/blitz2/` (generated code and result, both
  runtime-error-checking modes); the `perf` budgets for `bmath.frac`,
  `bmath.int`, `bstr.len`, `bstr.asc` are tightened; the entry in
  `docs/blitz2/extensions.md` (the list of changes of the compiler that have
  no reference to compare with).
- Depends on #1064: the Quick result of the intrinsic `Frac` must equal the
  library's result after it (the exact fraction); before it, the library
  loses fraction bits for integer parts of 256 and more.
