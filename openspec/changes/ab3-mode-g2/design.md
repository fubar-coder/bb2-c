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
  token comparison: decided in task 1.3, it does (findings below).

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
- (Task 1.3) AmiBlitz3's `-save NAME.bb2` in CLI mode
  (`Blitz3:Amiblitz3 -s SOURCE -save NAME.bb2`, no compile) writes the
  editor's lines as loaded, each with its 0 (`SaveSource`,
  `CLIModeSupport.ab3:1016`): the tokenised form, usable as it is. The
  comparison (`ab3_tokenise`, `tests/ab3/tokenise.cmake`) runs it for
  AmiBlitz3's test programs, the `.ab3` sources of `tests/ab3/cli/` and
  edge cases (`tests/ab3/tokens/`), and compares on the host with
  `tools/ab3tok` - `shared/tokenizer` with AB3 mode's token list
  (`blitztokens -last 80DB` on blitz2-ab3 and AmiBlitz3's BigLibs, the
  list `ab3_tokens` has equal to AmiBlitz3's), the same code ab3cli
  tokenises with; the embedded header is left out there, as AmiBlitz3
  does, where ab3cli keeps empty lines. All equal, including a word at
  the end of the file without a line end (not tokenised by either).
  `ab3_tokenise_broken` gives NPrint another token: it must fail.
- (Task 4.1) "Including the same file a second time with INCLUDE SHALL
  be an error" (spec `ab3-compat/source-mode`, "Repeated includes"):
  AmiBlitz3's `findinclude` (`AmiBlitz3.ab3` 11243, `processInclude`
  20737) looks only at the includes that are open, as BB2's `findinc`;
  measured (`ab3_cli_ref` case `incitwice`): two `INCLUDE`s of a file one
  after the other compile it twice, only an include of itself is
  "already included". Implemented as AmiBlitz3 (BB2's check, with the
  found full name); the requirement's wording should say "while it is
  open" (spec text only, no scenario affected).
- (Task 4.1) The found name is `NameFromLock`'s full name, as
  AmiBlitz3's: messages about an include name it so (`TEST:src/...` in
  the expect files). The type check (by name) comes before blitz2's
  "Already Included", in AmiBlitz3 after it - a difference only for an
  include of itself under a refused name. AmiBlitz3's includes directory
  is its tooltype `INCLUDEDIR`, by default `Blitz3:Sourcecodes/Includes`;
  blitz2-ab3 has no tooltypes and takes the default.
- (Task 4.1) The include depth for `#__include` (task 3.5, the other
  group): `ab3_incdepth()` in `ab3.h` (ab3inc.c) - the includes open now
  (`numincs`), 0 in the main source; it changes where AmiBlitz3's
  `_include` label does (`processInclude` sets it, `next_includefile`
  clears it with the last include).
- (Task 1.3) A main-source line of 250 or more characters: `LoadSource`
  re-reads it with `PeekT(…, 10)`, which does not stop at the line's 0;
  there is no LF left after tokenising, so the rest of the buffer is
  taken as that line and cut to 250 bytes - AmiBlitz3 drops every line
  after it (measured by `ab3_tokenise`). Ours cuts the line and keeps
  the following ones: a documented difference (`docs/ab3/variant.md`),
  not reproduced; the test source `tokens/longline.ab3` has the long
  line last.
- **2.1 OPTIMIZE** (`ab3dir.c`, points `ext_passstart`, `ext_token`,
  `ext_newsyntax`): the code of the statement is AmiBlitz3's, byte for
  byte: `move.l #0,d0` (`$203C`, a long 0) and `jsr` (`$4EB9`) to
  strings2lib's `setfpumode` - 12 bytes, compared with AmiBlitz3's
  executable by `opt_code` in `ab3_cli_ref` (with `-r`: with runtime
  error checking blitz2-ab3 puts its debug trap before the statement,
  AmiBlitz3 has no debugger in CLI mode). So D5's `moveq` became the
  `move.l` of AmiBlitz3.
- The reference machine **has an FPU**: Amiberry's A1200 of
  `tools/lib/amiga_emulation/default.uae` has a 68882 (`fpu_model`), so
  AmiBlitz3's `_checkfpu` passes and it compiles `OPTIMIZE 7` (case
  `opt_fpu`, `opt_fpu.ref.expect`). The risk "AmiBlitz3 refuses
  `OPTIMIZE 2` without an FPU" does not apply; FPU sources can be
  compared with AmiBlitz3 in G6.
- **3.1 macro calls** (`ab3syn.c`, point `ext_macargs`): AmiBlitz3's
  `handle_macro` and its own `findit`, which ends a parameter also at the
  end of the line and at `:`/`;` outside a string (blitz2's needs the
  closing brace). `macro_ns` gives AmiBlitz3's output for every form
  (`!pm 42`, `{7}`, `3,4`, `-5`, `$10`, `#c`, a string with a comma, none,
  `:`, a macro without parameters); `macro_off` shows the BB2 rule with
  new syntax off, and the "flags replaced" scenario of 2.1: both
  compilers stop with "Garbage at End of Line" at the same line.
- **2.2 SYNTAX**: consumed as AmiBlitz3 does (the one character behind
  the token), without a debug trap of its own. `syntax2` and
  `syntax2_none` (`-r`; with runtime error checking the traps hold the
  addresses of the compiler's line items) give byte-identical
  executables - also with AmiBlitz3, so its `SYNTAX 2` changes no code
  either. `cli.cmake` keeps the executables (`TEST:out/NAME.exe`) and
  compares the pairs of `SAME`.
- **2.3 RunErrs**: AmiBlitz3's errors are the same at the same lines
  (`runerrs_unpaired`, `runerrs_overflow`; its messages have a second
  line "Too many 'RunErrs' nested", ours are one line). The nesting
  itself cannot be compared: AmiBlitz3's executables of CLI mode have no
  runtime error checking without its debugger (`runerrs.ref.expect`).
- **3.2 labels** (points `ext_namelabel` in handleascii, `ext_newsyntax`
  in handlelab2): the label ends as in AmiBlitz3 - the source back at the
  token, `lastchar` a `:` - so the statement after it is compiled as the
  next one. `label_ns`, `label_proc` give AmiBlitz3's output;
  `label_proc_off` its error at the same line (blitz2's "Label reference
  out of context").
- **3.4** (points `ext_shared`/`ext_sharedvar` in doshared/fvarlocal,
  `ext_letter` in getelement2, `ext_dotlabel` in handlemouse,
  `ext_stmtprefix` in handlechar, `ext_comment` in getcharb): every case
  gives AmiBlitz3's output. Found on the way:
  - Global `SHARED`: AmiBlitz3 moves the variable to its `sharebase`,
    which its `fvaradd` searches before it adds a local one; here the
    variable stays global and the procedures look it up after their own
    `SHARED` ones (`ext_sharedvar`) - the same for the variables a
    procedure has not made local before. AmiBlitz3 also refuses a new
    variable without a type there (`vardeclareerr`) and a known one that
    is not an array ("already declared"); those errors are not taken
    over (only more programs compile; G8 for the declaration checks).
  - The scenario "Underscore variable" says `Bin$(_start)` prints `101`:
    both compilers print the 32 digits `000…0101` (`Bin$` of a long).
    The spec's wording is short for that, no behaviour differs.
  - `:@` in a comment: comments are not tokenised (TED's tokeniser and
    AmiBlitz3's), so only code without keywords works after it, in both
    compilers (`; x :@NPrint "a"` is "Garbage at End of Line" in both).
- **3.3 `@@`** (point `ext_nextline` in nextline): not at AmiBlitz3's
  sites one by one, but where a line is read: a line with `@@` outside
  strings and comments is joined with the following ones (the rest of
  each line after `@@` dropped) into one buffer, so every place of the
  compiler - parameter lists, macro definitions and calls, multiple
  assignments - sees one line; inside an include only a `,` (and spaces)
  before `@@` is allowed. `join`, `join_inc`, `join_incok` give
  AmiBlitz3's output and error. More permissive than AmiBlitz3: a `@@`
  elsewhere also joins (`x = 1 + @@`), and "after a parameter"
  (`NPrint 1 @@` and `,2` on the next line) works here but is "Garbage
  at the end of line" at the second line in AmiBlitz3 - the requirement's
  "after a parameter" did not hold for `NPrint` there (not in a test).
  The joined line is the last one read: an error in it has that line's
  number; with runtime error checking the debug trap's offset into the
  line item is meaningless for it (only the editor's debugger reads it).
- **2.4 BSS/Chip** (`ab3sect.c`, points `ext_passend` in dopass after
  errchx2, `ext_savefile` in savefile; tokens `$80AD`/`$807F` through
  `ext_token`): AmiBlitz3's sections are parts of the *code* stream
  (`destpointer`), not of the data buffers: the code after `BSS`/`Chip`
  up to the other one or to the end of the program (before its end
  code, AmiBlitz3's `codeend`) is cut out of the code hunk; each starts
  with a NOP and on a long boundary. With runtime error checking both
  are ignored (one code hunk, case `sect_debug`); a BSS part with a byte
  other than 0 is "BSS Segment contains no Data!" (checked at the end of
  the pass, not in savefile, where an error cannot be reported). The
  hunk layout of `sect_chip` (code, data+chip) and `sect_both` (code,
  bss, data+chip) is AmiBlitz3's (`hunkcheck --hunks`, new; both
  configurations); the relocations are written per hunk and target, so
  a relocation inside the chip data is right here and wrong in
  AmiBlitz3 (`sect_both.ref.expect`, docs/ab3/variant.md). The debug
  information of the labels (option "debug info") is not written for an
  executable with sections.
- **3.5 not done: the spec does not match AmiBlitz3** (measured with
  `ab3_cli_ref`, a throw-away probe): `@#name` is not an expression
  element that gives -1/0. In AmiBlitz3 a `@` is only taken right after
  `CNIF` (`AmiBlitz3.ab3` 10677-10686, `incnif`); in such a `CNIF` *every*
  `#name` of the condition is a "defined?" test giving **1** or 0
  (`fetch_Constant` 15400-15412), without an error for an undefined one:
  `CNIF @#zq = 1` (defined), `CNIF @#zr = 0` (undefined),
  `CNIF @#zq + #zq = 2` and `CNIF @#zq + #zr = 1` are all true; a `@`
  elsewhere in the condition (`CNIF #zq + @#zq = 6`) is "Illegal
  constant expression". `#__debug` is 0 in AmiBlitz3's CLI mode (no
  debugger), `#__include` 0 in the main source, as the spec says. The
  requirement "Constants of the include mechanism" needs this wording
  before 3.5 is implemented (its include-guard scenario holds either
  way); `ab3_incdepth` (ab3inc.c) is ready for `#__include`.
- **3.5** (after the spec's correction; `ab3syn.c`, points
  `ext_cnifstart`/`ext_cnifend` in docnif, `ext_constant` in
  fetchconst): AmiBlitz3 puts `__include` and `__debug` into its
  constant list at the start of each pass (`AmiBlitz3.ab3` 3405-3440)
  and sets `__include` to 1 when an include opens (`processInclude`
  20799) and to 0 when the last one closes (`next_includefile` 19534);
  here the two names are answered in fetchconst before the label list,
  `#__include` from `ab3_incdepth()`. In a `CNIF @` condition the test
  is AmiBlitz3's `findlabel2`: any entry of the label list counts (also a
  program label), and the two names count as defined. A difference not
  measured: AmiBlitz3's two constants are real entries of the list, ours
  are answered before it, so a program's own definition of
  `#__include`/`#__debug` is ignored here (no program of AmiBlitz3's
  defines them). `#@name` (AmiBlitz3's
  automatic counter constants, `automode` in fetch_Constant) is not part
  of G2. `incguard` (INCLUDE twice and XINCLUDE: compiled once, the
  `CNIF #__include=0` block inside skipped), `cnifdef` (the measured
  conditions) and `cnifat_err` (the error at the same line) give
  AmiBlitz3's results; AmiBlitz3 reports the error also for the
  following lines (its error recovery), blitz2-ab3 stops at the first.
