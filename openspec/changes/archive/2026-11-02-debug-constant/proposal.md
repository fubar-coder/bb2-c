# Proposal

## Why

A source cannot tell whether it is compiled for debugging. `Stop` needs
the runtime debugger: without it Blitz Basic 2 refuses the compile
("Debugger must be enabled for STOP"), with it the program waits in the
debugger. AmiBlitz3 drops `Stop` silently in a release build instead
(`#stopignore`, `AmiBlitz3.ab3` 22810-22831 at `58a11658`), which hides
the statement. The user decided on 2026-11-01 to keep BB2's behaviour
for `Stop` and to give the developer the choice the usual way (as
`DEBUG` in .NET or `NDEBUG` in C): a predefined compile-time constant,
AmiBlitz3's `#__debug` – a value, −1 with runtime debugging, 0 without –
in both modes, so a source can write `CNIF #__debug … CEND` around
`Stop` or other debug code.

## What Changes

- `#__debug` is predefined in BB2 mode (`blitz2` and `blitz2-ab3`) and
  in AB3 mode: −1 when the program is compiled with runtime error
  checking (the debugger), 0 otherwise (e.g. `-r`), as AmiBlitz3's
  `debugmode` value.
- `Stop` keeps BB2's behaviour in both modes; AmiBlitz3's silent drop
  is not taken over (the `int_tests` difference stays recorded).
- In BB2 mode this is an extension: a source using `#__debug` failed
  before ("Constant not found"); every other source compiles as before.
- Tests for both modes and both values; `ab3_cli`/`ab3_cli_ref` compare
  AB3 mode with AmiBlitz3.

## Capabilities

### New Capabilities

- `compiler/debug-constant`: the predefined `#__debug` of both modes.

### Modified Capabilities

- `ab3-compat/syntax-rules`: "Constants of the include mechanism" no
  longer says `#__debug` is 0; its value comes from
  `compiler/debug-constant`.

## Impact

- `programs/blitz2/src` (`fetchconst` in `eval.c` and the constant
  checks of `CNIF`; the core, since both modes use it; the AB3 module
  keeps only `#__include`).
- `blitz2`'s results change only for sources that use `#__debug`
  (intended, named in the commit); the bug-orig/corpus/CD results stay
  byte-identical.
- Tests (`tests/ab3/`, a BB2-mode case in the corpus or bug-orig
  style), docs (`docs/ab3/variant.md`, README, `docs/blitz2/` notes),
  `TODO.md` (the `Stop` rule item decided).
