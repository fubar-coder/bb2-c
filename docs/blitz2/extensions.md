# Extensions of blitz2

`blitz2` compiles a Blitz Basic 2 source to a program that behaves as the
one of the CD's compiler. Its extensions give a meaning to sources the
original refuses (`#__debug`), or compile a few library calls as inline
code (the intrinsics): the program prints and writes the same, only its
executable differs from the original's.

## `#__debug`

`#__debug` is a constant the compiler defines itself (AmiBlitz3's name and
values; in `blitz2-ab3` in both modes). Its value is −1 when the program
is compiled with runtime error checking and 0 without it. The option is
the one given at the start of the compile; `RunErrsOff` in the source does
not change it. The original compiler says "Constant Not Found" for it.

Use it to keep code for debugging only in a program that is compiled with
runtime error checking:

```
CNIF #__debug
  NPrint "only with runtime error checking"
CEND
```

Details:

* It can be used wherever a constant can: in expressions, in `CNIF`, in
  constant definitions and as an assembler operand.
* A source's own `#__debug=…` replaces the predefined one, as in
  AmiBlitz3.
* A resident's `#__debug` (AmiBlitz3's `all.res` has one) gets the value
  of the compile.
* `Stop` itself stays as in Blitz Basic 2: without runtime error checking
  it reports "Debugger must be enabled for STOP". Wrap it in
  `CNIF #__debug … CEND` for a program that is also compiled without.

## Intrinsics

Four library functions are compiled as a few instructions in place of
the library call when their argument allows it (OpenSpec spec
`compiler/intrinsics`; the table `intrinsics[]` in
`programs/blitz2/src/eval.c`). The value is the library's, bit for bit,
with and without runtime error checking; the registers the compiled code
relies on (a2 to a7, the data registers held around the call) stay as
they are. Every other use compiles exactly as before.

| Function | Condition | Inline code |
|---|---|---|
| `Int` | the argument has the type Quick (a variable, an element, a field, any Quick expression) | the integer part truncated towards zero as a Long, 5 instructions; no type word, no call, no `intchk` |
| `Frac` | the argument has the type Quick, and the call is the whole right side of an assignment to a Quick (nothing but `:`, `;` or the end of the line behind it) | the fraction with the sign of the value as a Quick, 11 instructions; no Float, no `qutofl`, `Frac` or `fltoqu` |
| `Len` | the argument is a plain string variable: a name with `$` and nothing else (global, local, `Shared`, a parameter) | the length from the header at -4 of the characters, 0 without memory; no copy into the string work area |
| `Asc` | as `Len` | the first character (0 to 255), -1 for "" and a variable without memory; no copy |

Not intrinsics, compiled with the call as before: `Frac` in an
expression, a condition, an argument or with a Float or Long target (its
Float result is the library's job); `Len` and `Asc` of an expression, a
function result, a literal, an array element, a List's current element,
a field (`t\name`, `\name`) or a string variable named without `$`
(`v.s` used as `v`).

The functions are found by their library token (library and function
number), so a user library with a function of the same name is called,
and only when the loaded library has the form the inline code was
written for: the CD's vallib takes a Float for `Frac`, so the compile with
the CD's libraries keeps that call; in AB3 mode AmiBlitz3's vallib does
the same, and `Len` and `Asc` are in AmiBlitz3's library 6, so all three
stay calls there (`Int` is an intrinsic in both modes). Not in constant
expressions and not in direct mode.

The cost (A500, cycles per call of `tests/perf`): `Int` 188.7 -> 63.2,
`Frac` 1316 -> 61.5, `Len` 679 -> 63 and `Asc` 745 -> 99 (and the same at
1000 characters, where the copy cost 8261 and 8327).

An executable of a program that uses an intrinsic differs from the one of
the build before this change (`tests/README.md`, "The prev build": named
with `-DPREV_INTENDED`) and from the original's (only information since
the originals are compared by output).

## Tests

* `blitz2_debug_on`, `blitz2_debug_off`, `blitz2_stop_off`
  (`tests/blitz2/`; the section "emulator tests" of `tests/README.md`
  describes how they run);
* in AB3 mode `debugconst_on`, `debugconst_off`, `debugconst_r`,
  `debugconst_res`, `debugconst_res_r` of `ab3_cli`;
* `ab3_cli_ref` confirms `debugconst_off` (also the replacing definition)
  with AmiBlitz3, whose command line has no debugger, so `debugconst_on`
  and `debugconst_res` give 0 there (`docs/ab3/variant.md`);
* the intrinsics: `blitz2_intrinsic_int`, `_frac`, `_len`, `_asc` (the
  values and no call) and `_int_lib`, `_frac_lib`, `_len_lib` (where the
  call stays), each also `_runerrs` with runtime error checking
  (`tests/blitz2/`: the programs count the JSRs between two labels around
  the statement; the self test `blitz2_selftest_intrinsic_selftest_wrong`
  must come out REGRESSED); in AB3 mode `intrinsic_int` and
  `intrinsic_frac` of `ab3_cli`, confirmed by `ab3_cli_ref`; the cycles
  in `tests/perf/budgets.txt` (`bmath.int`, `.intn`, `.frac`, `.fracn`,
  `bstr.len`, `.asc` and their lines at 10, 100 and 1000 characters).
