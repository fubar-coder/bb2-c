# Proposal

## Why

AmiBlitz3's procedures have features that Blitz Basic 2 lacks, and the
includes depend on them:

| Feature | Procedures in AmiBlitz3's `Sourcecodes` |
|---|---|
| optional parameters (`@p`) | 531 |
| more than 6 parameters | 136 |
| `FAST` | 317 |
| a NewType return type | 172 |

Today `blitz2-ab3` cannot compile them:

- **Optional parameters:** `@` is still BB2's pointer flag. A `@t.s`
  parameter fails with "Can't Convert Types". A `@mode.b` compiles
  silently wrong. Calls that leave arguments out stop with "Not Enough
  Parameters".
- **More than 6 parameters, `FAST`, NewType return type:** a 7th
  parameter gives "Too many parameters", `FAST` gives "Syntax Error",
  and `Function.nt` gives "Illegal Type".

Phase G4 of the plan (`docs/ab3/variant.md`) covers these features. The
behaviour was read from AmiBlitz3's compiler at `58a11658` and measured
with probes in the emulator (research notes of 2026-11-04/05).

## What Changes

All of this is in AB3 mode only. BB2 mode and `blitz2` stay as they are.

- **Optional parameters:**
  - From the first parameter written with `@`, every later parameter is
    optional, also one without `@`, as in AmiBlitz3. This works whatever
    `OPTIMIZE` is set.
  - An omitted argument gets the default of its type:
    - -1 for byte, word and long;
    - -1.0 for quick and float (FFP, no FPU);
    - the empty string for a string;
    - -1 for a pointer.
  - Arguments can only be left out at the end of the list:
    - `name{}` gives every parameter its default when all of them are
      optional.
    - A missing mandatory argument gives "Not enough parameters".
  - `name{}` also fills all parameters of a procedure with 7 or more
    only-optional ones. AmiBlitz3 refuses that, so it is an intended
    deviation.
- **Up to 10 parameters:**
  - Parameters 1–6 stay in d0–d5.
  - Parameters 7–10 are passed on the stack. The slot is 2 bytes for
    byte and word, 4 bytes for long, quick and float, and 8 bytes for a
    string.
  - AmiBlitz3 passes them through four static longs, which a nested call
    overwrites. Taking the stack instead is an intended deviation.
  - String parameters keep AmiBlitz3's 8-byte slot at every position,
    including the code for an omitted string (pointer -1).
- **`FAST` procedures** (`Statement FAST name{…}`, `Function.t FAST
  name{…}`): the locals are not cleared at entry. A `FAST` procedure
  that needs local memory (string, NewType, array) is an error, as in
  AmiBlitz3.
- **NewType return type** (`Function.nt name{…}`): it returns the
  pointer as a long, and the result is accepted by a pointer assignment
  (`*p.nt = mk{}`). AmiBlitz3's check of the `Function Return`
  expression against the declared NewType is left to G8.

Not in this change:
- FPU parameters and defaults (`.f`/`.d` in FPU mode): G6.
- The `SYNTAX` type checks of arguments: G8.
- The function optimizer: G8.
- Inlining `Null`: G8. `Null` already compiles as a library call.

## Capabilities

### New Capabilities

- `ab3-compat/procedures`: definitions and calls of `Statement`s and
  `Function`s in AB3 mode where AmiBlitz3 differs from Blitz Basic 2:
  - optional parameters and their defaults;
  - up to 10 parameters;
  - `FAST`;
  - NewType return types;
  - BB2 mode keeping BB2's rules.

### Modified Capabilities

None.

## Impact

- `programs/blitz2/src`:
  - the procedure definition (`tokens2.c` `dofunc`/`dofunc2`, `linkput`,
    `procfixer`, the epilogue);
  - the calls (`compile.c` `handleproc`, `eval.c` `afunction` and
    `maineval2`, `asmlibs.c` `fetchpees_p`);
  - `ext.h`/`ext_bb2.c`, and a new AB3 module `ab3proc.c`.
- Tests:
  - `tests/ab3/cli` cases with AmiBlitz3's results in `ab3_cli_ref`;
  - a BB2-mode case for `@`;
  - the include case `inc_datetime`, which must keep passing with real
    optional parameters.
- Docs: `docs/ab3/variant.md` (G4 section, "Differences"), `TODO.md`.
- No change to `blitz2` or to any BB2-mode result.
