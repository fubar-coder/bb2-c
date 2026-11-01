# Design

## Context

See proposal.md (Why) and the research of 2026-11-01 (AmiBlitz3 source
at `58a11658`; summary in the scratch file of that session, the facts
taken into the specs and below). Starting point:

- `blitz2-ab3` after `blitz2-ab3-extension-points` (prerequisite): the
  chunk sources are one shared core, AB3 differences are extension
  points in `ext.h` implemented in `ab3*.c`, `blitz2` links `ext_bb2.c`.
- `ab3cli` tokenises with TED's core (`programs/ted/src/core/tokit.c`,
  globals `tokens`, `quote`, `remark`, `invflag`), for main sources and
  text includes (`tokfile`).
- Places in our compiler the G2 features touch (research §2–§8):
  macro calls `compile.c:434-481` (`blitz2.s:2133-2158`); statement
  start `compile.c:1190` (`handleascii`, `blitz2.s:2977`); label fix-up
  `compile.c:1281` (`handlelab2`, `blitz2.s:3011/3042`); expression
  elements `eval.c:1512/1569` (`getelement2`, `blitz2.s:9718/9782`);
  `doshared` `tokens1.c:685` (`blitz2.s:12716`); comment skipping
  (`getchar`); `CNIF` and constants; the include path (`doinc2`,
  `readinc`); `savefile` (hunks, for `BSS`/`Chip`); the compiler token
  table `toklist.h` (`SYNTAX` in the `Jimi` slot `$8014`, `BSS`/`Chip`
  in the `TAS`/`MOVEP` slots, `LastRunErrsMode` `$80D9`).
- AmiBlitz3's OPTIMIZE values: 1 = 68020, 2 = FPU, 4 = new syntax, 8 =
  nothing, 16 = short branches; each `OPTIMIZE` replaces all flags;
  reset at every pass (`Aclearallvars`); writes `move.l #0/#2,d0` +
  `JSR setfpumode` (`$C703`).

## Goals / Non-Goals

**Goals:** the specs `ab3-compat/directives`, `syntax-rules`,
`tokenizer` and the modified `source-mode`; about 10 more of AB3's test
programs compiling (research table: hex_error, hexdigits, int_problem,
int_tests, jsr, kick13, macro_test, memoryleak_test, unknown_function,
windowtags), exec_data and check_processor behaving as in AmiBlitz3.

**Non-Goals:** `@"…"` and string escapes (G3); `@p`/`FAST` (G4); FPU
(G6); 68020 code (G7); `SYNTAX` checks and code equality (G8); TED
using the new tokeniser (a later change, user decision 2026-11-01).

## Decisions

### D1: Every G2 feature behind an extension point
Following the rule of `blitz2-ab3-extension-points`, each feature is an
extension point called from the chunk code, implemented in `ab3*.c`
(`ab3dir.c` directives, `ab3syn.c` syntax rules, `ab3inc.c` includes)
and answered "not handled" by `ext_bb2.c`. New-syntax branches are
`if (ext_newsyntax()) …` (always false in `blitz2`). Per-pass state
(OPTIMIZE flags, RunErrs stack) is reset through the existing pass-start
point.

### D2: The tokeniser as an object with rule sets (`shared/tokenizer`)
`struct tokenizer` holds the token list (built from the compiler chain
and the libraries as blitz2 hands it to the editor), the quote/remark
state and a rule set (`TOK_RULES_AB3` in G2; `TOK_RULES_TED` reserved
for the later TED change). `tokenizer_line()` turns one text line into
tokenised bytes. Built like `shared/xtra`: compiled by each user with
its own environment (ab3cli with TED's options, host tools natively),
host tests with inputs from memory. The AB3 rules follow
`preproc_Tokenize` (`Shared/preprocessor.include.ab3:358`): byte-exact
hash lookup, word start rules, `name$` first, first entry wins, strings
and comments untouched, 250-character lines.
*Why:* the user's idea of class-like components; one place for AB3's
rules, testable on the host. *Alternatives:* extending TED's `tokit`
with flags (TED's globals, risk for TED's byte-exact oracle behaviour).

### D3: ab3cli uses the tokeniser in AB3 mode only
`ab3cli` tokenises AB3-mode sources and text includes with
`shared/tokenizer`; BB2-mode command line runs keep TED's `tokit`.

### D4: Includes by AmiBlitz3's rules in AB3 mode
The include extension point resolves the name (INCDIR, search order,
extension fallback), decides the type by the last three characters and
refuses other names; tokenised includes still get the other-mode check
of G1 (companion `.xtra`). A table of included files gives the
repeated-`INCLUDE` error and skips repeated `XINCLUDE`s. `#__include`
follows the include depth.

### D5: OPTIMIZE codes and setfpumode
`OPTIMIZE` evaluates a constant expression (as `CNIF`), sets the flags,
refuses value 2 (FPU) with "Not supported: OPTIMIZE 2 (FPU)", accepts 1
(no 68020 code yet), 8 and 16 (no code change), and emits `moveq #0,d0`
equivalent to AmiBlitz3's `move.l #0,d0` plus the library call of
strings2lib's `$C703` through the normal library-call path. *Assumption:*
the exact instruction form does not matter (functional equality, G8
for byte equality).

### D6: BSS and Chip in savefile
The compiler records the start of the BSS/Chip part of the data (once
each) and `savefile` writes it as a `HUNK_BSS` or a hunk with the chip
memory flag; with the runtime debugger on they are ignored (AmiBlitz3's
`exdebug`). Only AB3 mode; the BB2 hunk layout is unchanged.

### D7: Tests
- Host: `shared/tokenizer` (each rule, broken input), the OPTIMIZE
  expression and flag logic where it is pure C.
- Emulator (`ab3_cli`, new cases): one case per requirement scenario
  that is observable by output or compile error; the research table's
  headless programs (hex_error, hexdigits, int_problem, int_tests, jsr,
  macro_test, memoryleak_test with its `OPTIMIZE 13` line kept, plus
  exec_data and unknown_function as expected errors) in `ab3_cli` and
  `ab3_cli_ref`.
- Tokens: the tokeniser's output for the test programs compared with
  AmiBlitz3's (`-save` in CLI mode, or a token dump), as spec
  `ab3-compat/tokenizer` asks.
- `blitz2`: results unchanged (`ab3_bb2_equivalence*`, `corpus_cd`).

## Risks / Trade-offs

- [`ab3_cli_ref` on Amiberry: AmiBlitz3 refuses `OPTIMIZE 2` without an
  FPU] → the reference machine is the A1200 without FPU; FPU cases are
  G6.
- [AmiBlitz3's `-save` may not give a comparable token dump] → fall
  back to comparing compile results and outputs; recorded if so.
- [The 68020 bit accepted with 68000 code differs in border cases
  (arrays > 65536 elements, code blocks > 32 KB)] → documented
  deviation until G7.
- [`:@` in comments changes BB2-valid code] → AB3 mode only; BB2 mode
  unchanged.

## Open Questions

- Whether AmiBlitz3's `-save` writes the tokenised form usable for the
  token comparison (decided during the tokenizer task; the spec allows
  "an equivalent dump").

## Findings during implementation

- (Task 1.2, 2026-11-01) `exec_data`: with AmiBlitz3's tokeniser
  `execbase` stays a variable, as in AmiBlitz3, but the program still
  compiles (a quick 0 assigned to the pointer `*exec`). AmiBlitz3's
  "Can't convert types --> execbase" is not its tokeniser but its
  assignment rule: with a pointer on the left the right side must be a
  pointer or a long (`AmiBlitz3.ab3` 23540-23570 at `58a11658`, after
  `eval` in the assignment; independent of `SYNTAX`/`syntax_typecheck`).
  That rule is in none of the G2 specs; the scenario "Lower-case library
  name" of `ab3-compat/tokenizer` ("the compile gives the same result as
  AmiBlitz3") and the verification of task 1.2 need it. Open: a spec
  decision (a syntax rule of its own, or the scenario limited to the
  tokens).
- (Task 1.2) The embedded header of a main source and of an include is
  skipped as AmiBlitz3 does, but as empty lines: the line numbers of
  errors stay those of the file. AmiBlitz3's `ParseAB3Header` (main
  source) and `SkipXtraHeader` (include) find the end differently; both
  are kept (`shared/tokenizer`).
- (Task 1.2) The 250-character cut of `LoadSource` re-reads a long line
  with `PeekT(…, 10)`; after tokenising there is no LF left, so here the
  line is taken to its 0 and cut (guess, checked by the token comparison
  of task 1.3).
