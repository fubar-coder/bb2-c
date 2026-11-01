# Proposal

## Why

After G1 (`ab3-mode-g1`), `blitz2-ab3` compiles AmiBlitz3 sources that
use only BB2 syntax – about 42 of AmiBlitz3's 68 test programs. Most
other AB3 sources start with `OPTIMIZE`/`SYNTAX`, use `@@`, `_`-names or
the include conventions of AmiBlitz3's `Sourcecodes/Includes`
(`#__include`, `@#const`), and AmiBlitz3 tokenises case-sensitively,
which G1 does not (the `exec_data` difference). Phase G2 of the plan
(`docs/ab3/variant.md`) covers the directives and the syntax rules that
are on in AmiBlitz3 v3.14.0 without FPU or 68020 code. Research of
2026-11-01 (AB3 source at `58a11658`) gives the exact behaviour; user
decisions of 2026-11-01 fix the scope.

## What Changes

- **Directives (AB3 mode):** `OPTIMIZE n` (bit 1 68020 accepted, 68000
  code generated – documented deviation until G7; bit 2 FPU refused
  "Not supported" until G6; bit 4 new syntax; bits 8 and 16 accepted;
  reset at every pass; the `setfpumode` call AmiBlitz3 writes);
  `SYNTAX n` accepted with no checks (checks are G8); `RunErrsOff` as a
  stack with `LastRunErrsMode`; `BSS` and `Chip` as section directives.
- **New syntax (OPTIMIZE bit 4):** macro calls without braces, labels
  without colon (with AmiBlitz3's `allowtab`), references to labels of
  another procedure. (`@"…"` and `\\` escapes stay in G3.)
- **Always on in AB3 mode:** `@@` line joining, global `SHARED`,
  identifiers beginning with `_` (and the other characters AmiBlitz3
  counts as letters), `#__include`, `#__debug`, `@#name`, code after
  `:@` in a comment, the `+` prefix (no type check), `.name` followed
  by a token as a label.
- **Tokenising (AB3 mode):** a tokeniser component (`shared/tokenizer`,
  an object with a rule set) with AmiBlitz3's rules – case-sensitive
  whole words, the characters after which no token starts, `name$`
  before `name`, first list entry wins, 250-character lines; used by
  `ab3cli` for AB3 sources and text includes and by the host tools.
  TED keeps its own tokeniser (switching TED is a later change).
- **Includes (AB3 mode):** AmiBlitz3's rules: `INCDIR` + name, search
  order, extension fallback, type by the name's last three characters,
  other names refused, repeated `INCLUDE` an error, repeated `XINCLUDE`
  skipped.
- Tests for each of these, and the AB3 test programs that G2 makes
  compile (about 10 more, research table) in `ab3_cli`/`ab3_cli_ref`.
- **Depends on** `blitz2-ab3-extension-points`: every G2 addition sits
  in `ab3*.c` behind an extension point.

## Capabilities

### New Capabilities

- `ab3-compat/directives`: `OPTIMIZE`, `SYNTAX`, `RunErrsOn`/`Off`/
  `LastRunErrsMode`, `BSS`/`Chip` in AB3 mode.
- `ab3-compat/syntax-rules`: the new-syntax features of `OPTIMIZE` bit 4
  and the rules that are always on in AB3 mode.
- `ab3-compat/tokenizer`: how AB3-mode text is tokenised.

### Modified Capabilities

- `ab3-compat/source-mode`: includes of an AB3-mode program follow
  AmiBlitz3's rules (type by name instead of by content; search order;
  refusal of other names; repeated includes).

## Impact

- `programs/blitz2/src/ab3*.c` and the extension points in `ext.h`
  (parser start of statement, macro calls, label fix-up, expression
  elements, comment skipping, `SHARED`, constants, includes, section
  handling in `savefile`).
- New `shared/tokenizer/` (object, rule sets; host and m68k builds,
  host tests), `programs/ab3cli` (uses it), `tools/` host tools as
  needed.
- Tests under `tests/ab3/` (new cases), docs (`docs/ab3/variant.md`
  phase G2, README).
- `blitz2`'s results stay unchanged (spec `ab3-compat/bb2-equivalence`).
