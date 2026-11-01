# Proposal

## Why

AmiBlitz3 lets a `Function` be called as a statement (`myfunc{1,2}` on
a line of its own, the result discarded): its check that a procedure
called as a statement is a `Statement` is commented out
(`AmiBlitz3.ab3` 4416-4426 at `58a11658`, `;BPL.w illprocerr`), in
every mode. Blitz Basic 2 refuses it with "Illegal Procedure Call"
(`blitz2.s:3083 handleproc`, `bpl illprocerr`; `compile.c` `handleproc`).
AmiBlitz3's test program `jsr` relies on it and is one of the 11 test
programs AmiBlitz3 compiles and `blitz2-ab3` does not (G2 measurement).
The user decided on 2026-11-01 to take this over.

## What Changes

- In AB3 mode a call of a `Function` at the start of a statement
  (`name{…}`) compiles like a call of a `Statement`: the function runs,
  its result is discarded.
- BB2 mode and `blitz2` are unchanged ("Illegal Procedure Call").
- Tests: `ab3_cli` cases (numeric and string functions, with and
  without parameters), compared with AmiBlitz3 in `ab3_cli_ref`;
  AmiBlitz3's `jsr` added; the measured share in `docs/ab3/variant.md`
  updated.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `ab3-compat/syntax-rules`: adds the rule that a `Function` may be
  called as a statement in AB3 mode.

## Impact

- `programs/blitz2/src/compile.c` `handleproc`: one extension point
  instead of the unconditional `illprocerr`; AB3 implementation in
  `ab3syn.c`, `ext_bb2.c` keeps the error.
- `tests/ab3/` cases, `docs/ab3/variant.md`, `TODO.md`.
- Depends on `ab3-mode-g2` (its `syntax-rules` spec is the base).
