# Proposal

## Why

After G2 (`ab3-mode-g2`), `blitz2-ab3` compiles AmiBlitz3 sources with
new syntax (`OPTIMIZE` bit 4). The string features of new syntax are
still missing. The `\\` escape sequences are copied into the string
literally, so the program compiles but its strings are wrong: 44 of
AmiBlitz3's `Sourcecodes/Includes` use escapes, mostly `\\hh`,
`\\__THIS_FUNCTION` in `error{…}` messages and `\\N`. Character
literals `@"ABCD"`, used in 33 includes, end the compile with a syntax
error. Phase G3 of the plan (`docs/ab3/variant.md`) covers both.
AmiBlitz3's own compiler specification
(`AmiBlitz/AmiBlitz3` `Docs/Compiler-Spec/04-lexical.md` §4.5) and the
compiler source at `f0414b11` give the exact behaviour. User decisions
of 2026-11-04 fix the scope:

- AmiBlitz3's rules are taken over.
- What looks like a bug of AmiBlitz3 is implemented as intended
  instead (I-12 to I-15 of its issue list).

## What Changes

- **Escape sequences in string literals** (AB3 mode, new syntax on):
  - `\\hh` (one byte), `\\N` (line feed), `\\\\` (a literal `\\`).
  - `\\Rddc`: the byte `c` repeated dd times, dd decimal.
  - `\\__THIS_FUNCTION`, `\\__THIS_INCLUDE` and `\\__CODELINE`.
  - `\\__VER_MAJOR`/`MINOR`/`PATCH`/`BUILD` from the source's header.
  - `\\__DATE__` (`mm.dd.yyyy`) and `\\__DATE_GER__` (`dd.mm.yyyy`),
    from the system date (`DateStamp`).
  - Names are case-insensitive.
- **Taken over as intended, not as AmiBlitz3 does it:**
  - An unknown sequence is a compile error.
  - A sequence with an empty value gives an empty text.
  - `\\R` repeats exactly dd times and does not change the case.
  - A name matches only in full.
- **`\\__THIS_POINTER` is refused** with "Not supported". AmiBlitz3
  inserts a pointer into its own compile-time memory, and no source
  uses it.
- **`Dc.b "…"`** in AB3 mode takes `\\__DATE__`, `\\__DATE_GER__` and
  `\\__VER_*`, and copies every other `\\` literally, as AmiBlitz3 does.
- **Character literals `@"…"`** (AB3 mode, new syntax on) in run-time
  expressions: an integer from 1, 2 or 4 characters (byte, word or
  long target), big-endian.
- **Test infrastructure:** `inject datestamp=YYYY-MM-DD` patches
  `dos.library/DateStamp` for the process it starts. ISO 8601 is used
  only in this value. `inject` becomes usable in the `ab3_cli` and
  `ab3_cli_ref` cases, so date output is stable for both blitz2-ab3
  and AmiBlitz3.
- Tests for each of these, and the remeasured share of AmiBlitz3's test
  programs.

## Capabilities

### New Capabilities

- `ab3-compat/strings`: escape sequences in string literals and in
  `Dc.b`, and character literals `@"…"` in AB3 mode.

### Modified Capabilities

None. Strings without escapes, BB2 mode and AB3 mode without new syntax
are unchanged. The tokeniser already leaves string literals alone
(`ab3-compat/tokenizer`, "Strings, comments and long lines").

## Impact

- **Compiler:** `programs/blitz2/src/ab3*.c` (a new `ab3str.c`) and new
  extension points in `ext.h`:
  - string literals, `litstring` in `eval.c:2150`;
  - the expression element `@` in `eval.c:1575`;
  - `Dc.b` strings, `dodc` in `tokens1.c:526`;
  - the global line counter.
- **Test tool:** `tests/bug-orig/amiga/inject.c` gets the new option
  `datestamp=`.
- **Test infrastructure:** `tests/ab3/cli.cmake` and
  `tests/ab3/CMakeLists.txt` can start the compiler through `inject`.
- **Tests and docs:**
  - new `ab3_cli`/`ab3_cli_ref` cases;
  - `docs/ab3/variant.md` (phase G3, "Differences");
  - `TODO.md`.
- **Unchanged:** `blitz2`'s results (spec `ab3-compat/bb2-equivalence`).
