# Proposal

## Why

AmiBlitz3 refuses the assignment of a basic non-long value to a pointer
variable – `*exec.ExecBase = execbase` with `execbase` an undeclared
`.q` variable gives "Can't convert types" – independent of `SYNTAX`
(`AmiBlitz3.ab3` 23540-23570 at `58a11658`: the jump to the error comes
before the test of `syntax_typecheck`). `blitz2-ab3` follows BB2 and
converts silently, so the pointer gets a converted value and the program
reads through a wrong address (the `exec_data` difference, recorded in
`docs/ab3/variant.md`). The user decided on 2026-11-01 to take over
only this rule, as a small change before G8; all other conversions stay
BB2's until `SYNTAX 4` type checks come with G8.

## What Changes

- In AB3 mode, an assignment whose left side is a pointer variable
  stops the compile with "Can't convert types" when the right side's
  value is of a basic type other than long (byte, word, quick, float,
  string), with no cast; a pointer, a long, a value without a type
  (e.g. a constant) and a NewType value are accepted (NewType against
  NewType is the `SYNTAX 4` check of G8).
- Nothing else changes: other assignments convert as in BB2, `SYNTAX`
  stays without checks, BB2 mode and `blitz2` are unchanged.
- Tests: `ab3_cli` cases for each accepted and refused right side,
  compared with AmiBlitz3 (`ab3_cli_ref`); `exec_data` then gives
  AmiBlitz3's error, its "Differences" entry is removed.
- Depends on `ab3-mode-g2` (the AB3 tokeniser keeps `execbase` a
  variable; extension-point structure).

## Capabilities

### New Capabilities

- `ab3-compat/type-rules`: type rules of AB3 mode that AmiBlitz3 applies
  without `SYNTAX`; G8 adds the `SYNTAX` checks here later.

### Modified Capabilities

None.

## Impact

- `programs/blitz2/src`: one extension point after the right side of an
  assignment is evaluated (`dolet`), implemented in an `ab3*.c` module;
  `ext_bb2.c` answers "not handled".
- `tests/ab3/` cases, `docs/ab3/variant.md` ("Differences", G8 note).
