# Design

## Context

See proposal.md (Why) and the spec `ab3-compat/strings`. Starting point:

- **`blitz2-ab3` after G2.** AB3 behaviour sits in `ab3*.c` behind the
  extension points of `ext.h`, `blitz2` links `ext_bb2.c`.
  `ext_newsyntax()` gives the `OPTIMIZE` bit 4 (`ab3dir.c`).
- **Our places for G3:**
  - String literals are made in `litstring` (`eval.c:2150`,
    `blitz2.s:10327`). Each byte is read with `getchar3` and written
    with `pokedata1b`, and the length is a word. The string is then
    copied at run time by `makelit` (`copstring`).
  - The expression element dispatch is in `eval.c:1575`. An `@` there
    ends in `syntaxerr`.
  - `Dc.b` strings are read in `dodc` (`tokens1.c:526`, the string loop
    at `:535`). `Data` strings are read in `dodata` (`tokens1.c:612`).
  - The current procedure is `thisproc` (`data.h:150`).
  - The embedded header's `Version` and `NumberOfBuilds` are parsed by
    `shared/xtra` (`xtra.h:218/224`).
- **AmiBlitz3 at `f0414b11`:**
  - `litstring`/`'writestring` (`AmiBlitz3.ab3:16210`) calls
    `check_newconstants` (`:26915`) when it sees `\\` and new syntax is
    on.
  - `Dc.b` (`:20270–20310`) takes `__DATE__`, `__DATE_GER__` and
    `__VER_*` without checking new syntax.
  - `Data` strings take no escapes (`:20390`, `'string`).
  - `@"` is handled at `:15149`: a `move.l #value,d0`, with the length
    by the target type. A quick target gives `error_convert_types`.
  - `globallinecount` is incremented per line in `.nextline` (`:2697`)
    and reset at `:3378`/`:27227`.
  - The date is `SystemDate` → `Date$`, with `/` replaced by `.`.
  - AmiBlitz3's own specification of this is in `Docs/Compiler-Spec/`
    (§4.5, issues I-12 to I-15, A6/A7, B5), together with the probe
    `verification/probes/esc_newsyntax.ab3` and its results.

## Goals / Non-Goals

**Goals:**
- the spec `ab3-compat/strings`;
- AmiBlitz3's includes with escapes and `@"…"` compile and give the
  right strings;
- the forms where AmiBlitz3 behaves as intended are compared 1:1 with
  AmiBlitz3 in `ab3_cli_ref`.

**Non-Goals:**
- `\\__THIS_POINTER` as a value (refused, user decision 2026-11-04).
- Reproducible builds outside the tests, i.e. no date option or
  variable in the compiler (user decision 2026-11-04).
- Escapes in BB2 mode, in TED and in the tokeniser.
- G4 (`@p`, `FAST`) and G5.

## Decisions

### D1: A new `ab3str.c` behind three extension points

| Extension point | Called from | Does |
|---|---|---|
| `ext_strescape(UWORD *len)` | `litstring`, when the byte read is `\` | If AB3 mode, new syntax and a second `\` follow: reads the sequence from the source, writes its bytes with `pokedata1b`, adds them to `*len` and returns TRUE. Otherwise returns FALSE, and `litstring` writes the `\` as before. |
| `ext_element(UWORD c, UWORD *type, UWORD *ret)` | the dispatch of `eval.c:1575`, before `syntaxerr` | Handles `@"…"`. |
| `ext_dcbescape(…)` | the string loop of `dodc` | Handles the `Dc.b` escapes. |

`ext_bb2.c` answers "not handled" to all three. `blitz2`'s code paths
stay byte-identical (spec `ab3-compat/bb2-equivalence`).

- **Alternative considered:** rewriting the escapes in the text before
  compiling (in `ab3cli`, like the tokeniser). Rejected for three
  reasons:
  - `\\__THIS_FUNCTION` and `\\__CODELINE` need the compiler's state.
  - A rewrite cannot tell a `Data` string from an expression string.
  - Macro text is expanded only in the compiler.

### D2: One recogniser, a table of names

A table in `ab3str.c` holds every name with its kind (date US/GER,
version part, this-function, this-include, codeline, pointer refused).
The longest name that matches in full wins, case-insensitive. Reading
continues right after the name, so `\\__THIS_FUNCTIONXY` gives
`GLOBALXY`.

The hex and `N`/`R` forms are checked first, then the names, in the
precedence of the spec. Every other form is "Unknown Escape Sequence".
`litstring` and `Dc.b` share the value functions. `Dc.b` lets only the
date and version kinds through.

- **Rejected: AmiBlitz3's fixed prefixes and skip lengths.** They are
  bugs (I-15, user decision: as intended).

### D3: The errors

- "Unknown Escape Sequence" and "Not supported: \\__THIS_POINTER" are
  compile errors of AB3 mode, raised the way G2 raises "Not supported:
  OPTIMIZE 2 (FPU)".
- `@"…"` out of its rules uses the existing `syntaxerr` and the
  "Can't Convert Types" error, as AmiBlitz3 does.

### D4: `@"…"` gives a constant of the target type

The value is built at compile time, big-endian, with up to 1, 2 or 4
characters by the target type. It is built arithmetically, as
`value = value << 8 | c` per character as `fetchqasm` does, never by
copying the bytes into an integer. So the value does not depend on the
byte order of the machine the compiler runs on. On the 68000 the value
in memory is then the characters in written order. The spec checks
this in memory (`Peek.b`, `WriteMem`) and not only through the value
(`Hex$`). It is then passed on like a numeric
constant (`fetchnum2`), so `blitz2`'s normal conversion applies. For a
float target that is the long-to-float conversion.

AmiBlitz3 writes `move.l #v,d0` for every target. For float that is
wrong, so we take it as intended.

In a constant expression (`cfetchmode`/`constmode`), `@` stays an
error, as in AmiBlitz3. AmiBlitz3's includes have the `#…=@"…"` lines
commented out.

### D5: The line counter and the current include

- **Line counter:** `ab3str.c` keeps a long counter. The extension
  point that is called when the next source line is started (main
  source or include) increments it. It is reset at the start of every
  pass, through the existing pass-start point.
- **The header lines:** `ab3cli` keeps the embedded header of an AB3
  text source as empty lines (G2: the lines keep their numbers), of the
  main source and of a text include. AmiBlitz3 has no such lines: it
  removes the header before tokenising. So the counter does not count
  them: `blitz2-ab3` takes the number of the main source's header lines
  from the source it reads for the mode (`ab3main.c`), and `ab3cli`'s
  include service returns those of an include in `d2`. The first
  counted line is the first line after the header, as in AmiBlitz3.
- **Joined lines:** every line joined with `@@` is counted, and a
  string gives the number of its own line (user decision 2026-11-04:
  the number must find the line in the source; an intended deviation).
  `blitz2-ab3` joins the lines when the first is read (`ext_nextline`,
  `ab3syn.c`, the one join path for every `@@`, also `Dim`/`Let`), so
  the counter is at the last of them when the line is compiled.
  `ab3syn.c` keeps where each line starts in the joined text, and
  `\\__CODELINE` takes off the lines behind the one the string is in.
  AmiBlitz3 counts a joined line once (its `mergeline` enters
  `.nextline` behind the count).
- **The current include:** `\\__THIS_INCLUDE` takes the file name of
  the include whose line is being read, from the include state
  (`ab3inc.c`). Back in the including file, that file counts again.
  This takes AmiBlitz3's I-59 ("last opened") as intended.

### D6: The date from `DateStamp`, converted in C

`ab3str.c` calls `DateStamp()` and converts the day number (days since
1978-01-01) to year, month and day with its own small function. It uses
no locale. The format is then AmiBlitz3's: `mm.dd.yyyy` or
`dd.mm.yyyy`.

### D7: `inject datestamp=YYYY-MM-DD` for the tests

The new option of `tests/bug-orig/amiga/inject.c` patches
`dos.library/DateStamp` (LVO −192) with `SetFunction` for the started
process, like the existing patches:

- the day number of the ISO date, minute 0, tick 0;
- the original is restored at the end;
- an invalid date is a usage error.

In the AB3 tests:

- `tests/ab3/cli.cmake` copies `inject` onto the test volume. The
  `ab3_cli_case` macro gets an option `DATESTAMP <ISO date>` that puts
  `TEST:inject datestamp=… ` before the compiler command. That command
  is `blitz2-ab3` for `ab3_cli` and `Amiblitz3` for `ab3_cli_ref`.
- The case line format gets that as a field.

**Alternatives considered:**

- A `Date` command in the test script. It sets the clock of the whole
  session, and its input format depends on the system.
- A variable or option in the compiler. That is a feature beyond
  AmiBlitz3, and not wanted.

### D8: Tests

- **`ab3_cli`:** one case per requirement of the spec, with the
  scenarios as source and `.expect`.
- **`ab3_cli_ref`:** the same sources where AmiBlitz3 behaves as
  intended:
  - hex, `\\N`, `\\\\`;
  - `\\__THIS_FUNCTION` and `\\__VER_*`;
  - date with `DATESTAMP`;
  - `\\__CODELINE` in the main source;
  - `@"…"` for byte, word and long.
- **The intended deviations** (`\\R`, unknown forms, empty values,
  `\\__THIS_POINTER`, `\\__THIS_INCLUDE` after an include,
  `\\__CODELINE` in lines joined with `@@`, `@"…"` with a float
  target): separate cases with `REF_EXPECT`, which records
  AmiBlitz3's output. Each one also gets a row in "Differences" of
  `docs/ab3/variant.md`.

### D9: `: End If` after a one-line `If` in a macro line

Added 2026-11-04 (user decision, after the include case of task 4.2 failed;
issue #967). `domacro` (`tokens1.c`) stores a macro body as one line, each
line end becoming `:`; a one-line `If … Then` is closed only at the end of a
physical line (`compile.c`, the "un-thens"). AmiBlitz3's `do_Macro`
(`AmiBlitz3.ab3` 20903-20939, `thenmode`) appends `: End If` to a macro line
that has `Then` ($8021) and no `End If` ($8002 $800B) after it; a line
joined with `@@` is one line, the `End If` goes at its real end.

- A new extension point in `domacro`, FALSE in `ext_bb2.c`, AB3 code in
  `ab3*.c`: it watches the tokens of the current macro line (outside
  strings and comments) and, at the line end, writes `:` and the tokens
  `$8002 $800B` before the `:` of the line end, the same bytes as
  AmiBlitz3.
- Rejected: keeping the line ends of the body. It breaks `If c Then
  !m{…}` with a multi-line macro (only the first line would be
  conditional; 41 call sites in AmiBlitz3's sources rely on the whole
  macro, measured), needs changes to the expansion buffer, `doline` and
  the line counting, and fixes nothing more than this rule.
- BB2 mode keeps the original's behaviour (#967 documents it; whether it
  follows is a separate decision).
- Findings (task 6.2, measured with AmiBlitz3 v3.14.0): `thenmode` is a
  flag, cleared at the start of the definition (20840, 20878), set by
  every `Then` token (20929), cleared by `End` followed by `If` while set
  (20932-20939) and after the append (20903-20907); the append comes
  after `nextline`, only when text was stored (`d2`), before the `:` of
  the line end, so the last body line gets it too (the trailing `:` is
  removed by `.mclean`). Strings: `Then` there is no token; comments are
  not stored. Two bugs, implemented as intended (ext points
  `ext_macrotoken`/`ext_macrolineend`, `ab3syn.c` next to the `@@` joins
  and `ext_macrodef`): two `Then`s on a line get one `End If` (the outer
  `If` leaks to the end of the expanded line; ours counts), and `EndIf`
  (its own token `$80C0`) does not clear the flag (a second `End If`,
  "'End' condition without condition"; ours closes one `Then` with it).
  Cases `macthen_two`, `macthen_endifword` with `REF_EXPECT`.
- `@@` in a definition (task 6.5): `do_Macro` joins at a line end after
  `@@` (20885) and at `@@` behind a character (20986-20991), both without
  the `ininclude` check of the parameter joins (3113, 3224, 8117, 8286);
  the rest of the `Macro` line is gathered the same way. Ours: lines read
  while `domacro` gathers (`ext_macrodef`) and a line that starts with
  `Macro` join every form in an include; a macro call keeps the rule.
  By its source (not measured) AmiBlitz3 also joins at `@@` inside a
  string of a macro line; ours keeps strings unchanged (spec
  `ab3-compat/syntax-rules`).
- `End` at the end of a macro line (task 6.7, issue #968, user decision
  2026-11-05: AB3 mode only, BB2 mode keeps the original). `domacro`
  reads the character behind `End` with `getchar3`; a line end (0, also
  for a `;` comment: `getcharb` has stepped past the line's NUL) is
  stored as a word and the gathering goes on behind the line. AmiBlitz3's
  `do_Macro` (20944-20957) peeks with `a5` saved and, for 0, `:` or `;`,
  stores only `$8002` and returns to `'gather`. Ours: extension point
  `ext_macroend` after `End` is stored; in AB3 mode, for 0 or `:`,
  `domacro` backs up (`bakup`: `srcptr` and `quoteflag` as before the
  character) and gathers it again, so the line end goes through
  `nextline` and `ext_macrolineend` (`If c Then End` gets its `: End
  If`). A `;` comment needs no own test: `getcharb` returns it as the
  line end. AmiBlitz3 also stores only `End` before `End noa7`, a
  statement AB3 mode does not have. Cases `macend_*`, among them the
  include case of the issue (`macend_inc`, `End` right before `End
  Macro`).

## Risks / Trade-offs

- **[AmiBlitz3's `SystemDate` may not use `DateStamp`]** If it reads
  the clock another way, `inject datestamp` does not reach it, and the
  ref date case shows the real date.
  - Mitigation: check it first (task 1.1). If so, the ref date case
    gets `REF_EXPECT` with a pattern check of the format, or it is not
    run against the reference. Recorded in variant.md.
- **[The line counter differs from AmiBlitz3's in corner cases]**
  Possible cases: macro expansions, `@@`-joined lines, `CNIF`-skipped
  lines, `XINCLUDE` skipped as repeated.
  - Mitigation: an `ab3_cli_ref` case for each of these whose output
    can be compared. Any difference found is taken over if it is a rule
    of AmiBlitz3, otherwise recorded as a deviation.
- **[A string literal grows past its word length]** For example many
  `\\R99`.
  - Mitigation: the existing length check of `litstring` applies to the
    expanded bytes. There is a case with a long expansion.
- **[`@` in expressions conflicts with other AB3 uses of `@`]** Those
  uses are `@p` (G4), `@#name` (G2) and `@@` (G2).
  - Mitigation: `ext_element` handles only `@` directly followed by
    `"`. Every other `@` goes on as before.

## Migration Plan

None. AB3 mode only, BB2 mode and `blitz2` are unchanged. Rollback is
reverting the commits.

## Open Questions

- None. (Answered in task 4.1: AmiBlitz3's text for `q.q=@"AB"` is
  "Can't convert types", at the line of the literal (`ab3_cli_ref` case
  `chr_quick`, console output of the run); ours is blitz2's "Can't
  Convert Types". The spec names the error, so it is unchanged.)

## Findings during the implementation

- (Task 2.1, 2026-11-04) `\\Rdd` takes as c the next character of the
  string; the closing quote or the end of the line is no character of
  the string, so `"[\\R03"` is an unknown form ("Unknown Escape
  Sequence", `ab3_cli` case `esc_repeat_quote`, not compared with
  AmiBlitz3). By its rule 3 AmiBlitz3 takes the `"` as c (not checked).
  The source is left out of `ab3_tokenise`: AmiBlitz3's `-save` writes
  its lines, but the `-save` of a later source in the same session ends
  in a program error (8000000A, at `incother_ab3`); without it the
  session passes. Not investigated further.
- (Task 2.1) AmiBlitz3's output of the cases with `REF_EXPECT` is that of
  its Compiler-Spec (§4.5.2, probe `esc_newsyntax`): `\\R05*` gives 6
  stars, `\\R03ab` gives `AAAab`, `\\R12-` 13 dashes; `\\R00x` runs over
  (`[` and NUL bytes, the rest of the string lost, the program goes on:
  matched as `~> [*`); `"[\\qq]abc"` gives `[QQ]ABC"qq]abc` with the
  warning "detected unknown const" once per pass.
- (Task 2.4) Measured with AmiBlitz3 (`ab3_cli_ref`): `\\__CODELINE`
  counts from the first line after the main source's header (line 21
  of `esc_codeline.ab3`, 4 header lines: 17); a macro gives the line it
  is expanded in, the lines of a skipped `CNIF` block count, an include
  counts its lines, an `XINCLUDE` skipped as repeated counts none
  (`esc_codeline`); an include's embedded header is not counted
  (`esc_codeline_hdr`). All of these are the same in `blitz2-ab3`.
- (Task 2.4) Lines joined with `@@` (`esc_codeline_join`, `REF_EXPECT`;
  in the main source and in an include): AmiBlitz3 counts a joined line
  once - every string of it gives the number of its first line, and the
  lines after it count one less per joined line (`> [5] [5] [5]`,
  `> [8] [8]`, `> next [9]`, `> after [10]`; `blitz2-ab3`:
  `> [5] [6] [7]`, `> [10] [11]`, `> next [12]`, `> after [13]`).
  Implemented as intended (user decision 2026-11-04, D5). AmiBlitz3's
  `Dim … @@` path calls `nextline` and counts the joined line (not
  compared); `blitz2-ab3` treats it like every other join.
- (Task 2.4) `\\__THIS_INCLUDE` with AmiBlitz3 (`esc_include`,
  `REF_EXPECT`): in the main source before any include the empty value
  runs over (`[` and a NUL byte, rule 7 of its §4.5.2); after an
  include ends it keeps the name of the include opened last (I-59), in
  the outer include and in the main source.
- (Task 4.1, 2026-11-04) `@"…"` is in a new `ab3chr.c` (not in
  `ab3str.c`): `ext_element` is called by `getelement2` behind the last
  element, before `syntaxerr`, as AmiBlitz3 tests `@` behind `?`. So a
  constant expression has stopped before: `#c=@"ILBM"` is "Illegal
  Constant Expression" (`cfetchmode`, blitz2's `badconerr`), AmiBlitz3's
  is "Illegal constant expression". AmiBlitz3's other texts: too many
  characters "Too many chars for data type", new syntax off "You need
  'newsyntax' mode for that"; ours is "Syntax Error" (spec). Only the
  `[run]` part is compared, so these cases (`chr_toolong`,
  `chr_toolong_l`, `chr_off`, `chr_space`, `chr_quick`, `chr_const`) are
  in `ab3_cli_ref` without `REF_EXPECT`.
- (Task 4.1) AmiBlitz3's output of the cases with `REF_EXPECT`: a float
  target gets the long unconverted (`f.f=@"AB"` prints `0.000015`,
  `@"A"` prints `0`; ours `16706` and `65`, `chr_float`). A string target
  (`a$=@"AB"`) compiles in AmiBlitz3 (its reading of `@"` checks only
  the byte, word and quick targets); the program printed a line of 28
  NUL bytes (`chr_string`, matched as any line). Ours is "Can't Convert
  Types" (spec).
- (Task 4.1) A byte above `$7F` between the quotes is read by
  `getcharb` together with the next byte as a token; `ab3chr.c` counts
  it as two characters (as written). Not compared with AmiBlitz3, which
  reads bytes. AmiBlitz3 also shifts a value of fewer characters back
  with `asr.l`, so three characters for a long whose first byte is above
  `$7F` give it a sign-extended value (ours: arithmetic, `$00xxxxxx`);
  not compared, no source has one.
- (Task 4.2) `@"…"` next to the other `@` forms: `CNIF @#name`, `@@`
  after a `,` behind a literal and `@@` inside a literal (`@"@@AB"` is
  `$40404142`) give the same results in both compilers (`chr_at`).
  Found on the way, not part of G3: `b.l = 1 + @@` (a join after an
  operator, not at a parameter boundary) is a syntax error in AmiBlitz3,
  ours joins it (`3`).
- (Task 4.2) An include case with a chunk ID is not added yet:
  `file.include.ab3` needs optional parameters (`@pos.l`, G4), and
  `longlong.include.ab3` (v3.14.0, no includes of its own; its macro
  `ll_ToStr` makes digits with `Chr$(@"0"+…)`) gives "Illegaly nested
  Condition" at the line calling `!ll_ToStr` in ours, while AmiBlitz3
  compiles it (`1234567`). Not the literal: the same body without
  `@"0"` fails the same way as a macro, and compiles written out in the
  main source. It is a `While` loop around calls of the nested macros
  `ll_Mod`/`ll_Divu`/`ll_Divu6432`/`ll_UShift` (`If … End If` blocks, a
  one-line `While … Wend`); each of those alone, also inside a `While`
  or another macro, compiles. A case for later: `XINCLUDE
  "longlong.include.ab3"`, `!ll_ToStr{digits.s, x}` of 1234567, and an
  IFF header `FORM`/`ILBM` written with `WriteMem` and read back,
  compared with `Select`/`Case @"…"` and `Mkl$` (AmiBlitz3's output:
  `1234567`, `FORM 4`, `ILBM ILBM`).
- (Task 6.6, 2026-11-05) `End Macro @@` followed by `NPrint "after"`
  (an `End Macro` line of its own and behind a one-line definition,
  `join_endmacro`, `join_endmacro1`): `blitz2-ab3` joined the next line
  onto the `End Macro` line and `domacro` dropped the rest behind
  `End Macro` (`dolineabort`), so the line was lost without a message
  (`m`, `end`). AmiBlitz3 v3.14.0 ignores the `@@` there and compiles
  the next line on its own (`after`, `m`, `end`). User decision: an
  error ("Joining lines with @@ does not work after End Macro", at the
  `End Macro` line; `isendmacro` in `ext_nextline`), not AmiBlitz3's
  behaviour; recorded as a difference in variant.md.
