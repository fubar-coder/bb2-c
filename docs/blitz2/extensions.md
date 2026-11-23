# Extensions of blitz2

`blitz2` compiles a Blitz Basic 2 source to a program that behaves as the
one of the CD's compiler. Its extensions give a meaning to sources the
original refuses (`#__debug`, the size `.s` of the inline assembler), or
compile a few library calls as inline code (the intrinsics): the program
prints and writes the same, only its executable differs from the
original's. The one exception is a branch of the inline assembler with
the size `.b`, which the original accepts and compiles as a word branch
(below).

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

## Short branches in the inline assembler

Blitz Basic 2's inline assembler knows the sizes `.b`, `.w` and `.l`;
`.s` is a Syntax Error, and every `Bcc`, `BRA` and `BSR` is a word branch
(4 bytes): a `Bcc` takes any size and ignores it, `BRA` and `BSR` take
none ("Illegal Assembler Instruction Size"). Sources written for other
assemblers or for AmiBlitz3 use `.s` (#1075, user decision of
2026-11-16). blitz2, and blitz2-ab3 in BB2 mode, take:

| Size | On `Bcc`, `BRA`, `BSR` | On any other instruction |
|---|---|---|
| none | word branch, as in BB2 2.10 | as in BB2 2.10 |
| `.s` | short branch: 2 bytes, the displacement in the low byte of the opcode | the same as `.b` |
| `.b` | short branch, as `.s` | as in BB2 2.10 |
| `.w` | word branch | as in BB2 2.10 |
| `.l` | `Bcc`: word branch, as in BB2 2.10; `BRA`, `BSR`: "Illegal Assembler Instruction Size" (a 32-bit displacement is 68020 only) | as in BB2 2.10 |

A short branch reaches -128 to +126 bytes from the instruction after it.
Its target is resolved at the end of the pass as every operand of the
assembler, so it may lie before or after it. A target out of range, and
the next instruction as target (the displacement 0 marks a word branch on
the 68000), is "Illegal Displacement" at the line of the branch. `DBcc`
is unchanged.

In AB3 mode the branches stay word branches whatever their size, as in
AmiBlitz3 v3.14.0 (`.s` is `.b` there too; #1068,
`docs/blitz2/deviations.md`).

`Bcc.b` changes the executable of a source that the original compiles:
the branch is 2 bytes shorter. Of the CD's sources only
`EXTRAS/LIBSDEV/USERLIBSRC/ROMULUSCRUNCHLIB.BB` has one (32 of them; all
in range, the executable is 64 bytes shorter).

## Tests

* `blitz2_debug_on`, `blitz2_debug_off`, `blitz2_stop_off`
  (`tests/blitz2/`; the section "emulator tests" of `tests/README.md`
  describes how they run);
* in AB3 mode `debugconst_on`, `debugconst_off`, `debugconst_r`,
  `debugconst_res`, `debugconst_res_r` of `ab3_cli`;
* `ab3_cli_ref` confirms `debugconst_off` (also the replacing definition)
  with AmiBlitz3, whose command line has no debugger, so `debugconst_on`
  and `debugconst_res` give 0 there (`docs/ab3/variant.md`);
* the intrinsics: `blitz2_intrinsic_int__CASE`, `_frac__CASE`,
  `_len__CASE`, `_asc__CASE` (the values and no call) and
  `_int_lib__CASE`, `_frac_lib__CASE`, `_len_lib__CASE` (where the call
  stays), each without and with runtime error checking (`tests/blitz2/`,
  tests of the BASIC test model: the programs count the JSRs between two
  labels around the statement and check the count; the self test
  `blitz2_selftest_intrinsic_selftest_wrong` must come out REGRESSED); in AB3 mode `intrinsic_int` and
  `intrinsic_frac` of `ab3_cli`, confirmed by `ab3_cli_ref`; the cycles
  in `tests/perf/budgets.txt` (`bmath.int`, `.intn`, `.frac`, `.fracn`,
  `bstr.len`, `.asc` and their lines at 10, 100 and 1000 characters);
* the short branches: `blitz2_asm_short_branch` (short branches forward
  and backward, taken and run, their opcode words and sizes, the limits
  +126 and -128, `.w`, no suffix, `Bcc.l`, `MOVE.s`),
  `blitz2_asm_short_range_fwd`, `_range_back` and `_zero` (the errors); in
  BB2 mode `asm_short_s_bb2`, `asm_short_range_bb2`, `asm_short_zero_bb2`
  and `asm_bra_l_bb2` of `ab3_cli`; AB3 mode's word branches:
  `asm_short_s` of `ab3_cli`, confirmed by `ab3_cli_ref`.
