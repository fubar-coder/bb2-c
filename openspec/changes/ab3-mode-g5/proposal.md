# Proposal

## Why

A survey of 2026-11-16 (main `f7960476`, compile only, v3.14.0 sources)
measured how much of AmiBlitz3's own code `blitz2-ab3` compiles:

| Set | blitz2-ab3 | AmiBlitz3 v3.14.0 |
|---|---|---|
| Examples (221) | 139 | 165 |
| Templates (8) | 4 | 8 |
| Tests (69) | 50 | 58 |
| Includes (139, each in a wrapper) | 42 | 96 |

No source compiles with `blitz2-ab3` only. Of the 38 programs AmiBlitz3
compiles and `blitz2-ab3` does not, the first message says: the FPU bit
of `OPTIMIZE` (18, G6), the type `.d` (5), the #940 check "Illegal End
Procedure" (3), a 68020 assembler instruction (1), and a few single
words. Of the 54 includes, 33 stop at one `.d` in `useful.include.ab3:623`
and 4 more at `DIVU.l`/`MULU.l` in `convert.include.ab3:609`; these two
sit under `dos.include`, which 34 examples use.

The earlier plan (G5, then G7, then G6) does not follow from the
measurement: the old G5 items open 3 of the 38 programs, while `.d`
without the FPU bit, the 68020 assembler and #940 together open 11.
Nothing in the repository keeps the measurement, so it cannot show
progress or a regression. User decisions of 2026-11-16 (the file
`ab3-g5-decisions.md` of the coordinator's work directory) fix the scope
and the order below.

## What Changes

All of this is in AB3 mode, except item 4, which holds in both modes.
`blitz2`'s results stay byte-identical except item 4.

1. **Test `ab3_examples`.** It compiles AmiBlitz3's examples, templates,
   tests and includes (each include in a wrapper) with `blitz2-ab3` and
   compares the result of each source (compiles, or its first error) with
   a committed table, as `corpus_cd` does with `tests/data/blitz2-cd.tsv`.
   `cmake/fetch-originals.cmake` fetches the sources, `Data` and the
   residents it needs. The table improves only as tasks close gaps.
2. **`.d` without the FPU bit.** The type `.d` is accepted, takes an
   8-byte slot and is computed as FFP like `.f`, as AmiBlitz3 does without
   the FPU bit (measured: `SizeOf` 10 for `.d`, 6 for `.f`, same output).
   First, the open details are checked against AmiBlitz3 with
   `ab3_cli_ref` cases: which 4 of the 8 bytes hold the value, `.d`
   parameters from the 7th on, `.d` in residents and NewTypes, `SizeOf`,
   library arguments of type 8. With the FPU bit `.d` stays "Not
   supported" until G6.
3. **68020 integer instructions in the inline assembler** (`DIVU.l`,
   `MULU.l`, the case of `convert.include.ab3:609`): assembler tables
   only, no code generation. `OPTIMIZE` 1 still gives 68000 code.
4. **The #940 relaxation.** `End Statement` closing a `Function` with a
   numeric type is accepted, compiled as `End Function` with the
   function's default return value, and gives a warning. A string
   `Function` stays an error: that was the crash #940 fixed. The rule
   holds in AB3 and in BB2 mode. In BB2 mode it changes the result of the
   #940 fix: the CD's `DARTS_POLY.BB` and `DARTS_WIRE.BB` compile again
   (as in the original, with a warning).
5. **The small G5 items**, each verified against AmiBlitz3 first:
   `USELASTPATH`, `USEPATH2`, `Dim ... KEEP`, constants in `Data` lines,
   `Object`, casts `(.type)`.
6. **Site fix:** `site/content/docs/ab3-compared/not-supported.md` lists
   `Null` as missing, but `Null` compiles as a library call.

Not in this change:

- The FPU bit and real FPU code (G6), `RegLoad`/`RegSave`, FPU
  instructions in inline assembler.
- Library 38 (`$0026`, needed by `jpeg.include`): a source with a clear
  licence is looked for later.
- Porting AmiBlitz3's libraries: AB3 mode keeps loading AmiBlitz3's
  binary libraries (decision of 2026-10-31, confirmed 2026-11-16).
- The unclassified syntax differences of the survey; a task only
  classifies them.
- 68020 code generation, `End d0`, G8 items (`SYNTAX` checks, optimizer,
  byte-equal executables).

## Capabilities

### New Capabilities

- `testing/ab3-examples`: the test that keeps the measurement of
  AmiBlitz3's examples, templates, tests and includes against a committed
  table.
- `ab3-compat/types`: the type `.d` in AB3 mode (without the FPU bit).
- `ab3-compat/assembler`: the 68020 integer instructions of the inline
  assembler in AB3 mode.
- `compiler/procedure-end`: which `End` and `Return` may close or leave
  which procedure, in both modes (the rule of #940 and its relaxation; it
  has no spec yet).

### Modified Capabilities

- `ab3-compat/directives`: `USELASTPATH` and `USEPATH2`; the FPU bit
  text names `.d` as still refused.
- `ab3-compat/syntax-rules`: `Dim ... KEEP`, constants in `Data` lines,
  `Object`, casts `(.type)`.
- `compiler/warnings`: the warning `procend`.

## Impact

- `programs/blitz2/src`: the type tables and the type size (`.d`),
  `asmlibs.c` and its tables (the 68020 instructions), `tokens2.c`
  (`doend3`, `stateret`, `funcret`; #940), the AB3 modules for the small
  items; `ext.h` only if an item needs a new extension point.
- Residents: the loader of `.res` files must know `.d` (AmiBlitz3's
  `newtype_double`).
- Tests: `tests/ab3` cases with `ab3_cli_ref` counterparts; a new test
  `ab3_examples` with its table under `tests/data/`; the bug-orig test
  `compiler-procend-kind*` and `corpus_cd`'s table rows of `DARTS_POLY`
  and `DARTS_WIRE`.
- `cmake/fetch-originals.cmake`: more AmiBlitz3 files.
- Docs: `docs/ab3/variant.md` (measured share, G5 section, "Differences"),
  `docs/libs/deviations.md` (the #940 row), `TODO.md`.
- Site: `ab3-compared/not-supported.md` and the pages on AB3 types and
  assembler; `compared-with-original/new-errors.md` and
  `fixed-bugs/compiler.md` (the #940 entries).
- BB2 mode: no result changes except the sources of item 4.
