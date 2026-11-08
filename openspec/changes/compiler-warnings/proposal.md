# Proposal

## Why

blitz2 knows only errors: `err` (`programs/blitz2/src/parse.c:545`) shows
the message through the editor's modal error requester (`ed_error`,
`comdata+12`) and aborts the compile; there is no way to say "this is
suspicious but compile on". The first case that needs one is #36:
`loadres` (`programs/blitz2/src/stmts.c:1266`) checks the residents for
clashes with `checkclash` for types and macros, but the call for
constants walks `firstconst`, a list nothing fills (`stmts.c:1315`; the
constants of the residents are labels in `firstlabel`, name at offset
19, appended by `fixconsts`, `stmts.c:1105`). Two residents that define
the same constant are never detected, and the first one silently wins
(the bug-orig test `compiler-loadres-clash-offset`, verdict REPRODUCED).

Making the check work as an error would reject setups that compile today
(the residents of the CD's sources, `.xtra` files of 724 sources); the
user decided (2026-11-08) that #36 gives a warning "Clash in Residents"
and that a compiler option, like gcc's `-Werror`, turns warnings into
errors.

## What Changes

- A warning mechanism in the compiler: `warn(msg)` beside `err`. A
  warning is shown to the user and does not stop the compile or change
  the result; with the option "warnings are errors" it behaves exactly
  as `err(msg)` (design D1, D2).
- How a warning reaches the user in each front end: the editor (TED, and
  the headless editor `ab3cli`) through the existing error service with
  the prefix `Warning: `; on the console as `SOURCE: warning: MESSAGE` in
  a `blitz2-ab3 -s ... -e ...` run, through a new point in `ext.h`
  (design D3, D4).
- The option: `-Werror` on the `blitz2-ab3` command line; in the editor
  the hidden option gadget `optreq2ga5` (stored in `.xtra` word +24,
  unused by the compiler today) becomes a visible check box "Warnings
  only" in the second options requester, so `.xtra` files stay
  byte-identical (design D5).
- Return codes: warnings leave the return code at 0; with `-Werror` a
  warning is an error (10) (design D6).
- #36: constants with the same name in the residents give the warning
  "Clash in Residents: #NAME" when their values differ; types and macros
  keep the error (design D7).
- Tests: the bug-orig test turns FIXED (`; @fixed #36`), new tests for
  `-Werror` and the option gadget, host tests of the shared pieces
  (design D8).
- Documentation: option list, the options requester description, the
  deviations entry (kind F) for #36 (design D9).

Out of scope: further warnings (the mechanism makes them cheap, none is
added); a `-w` switch to silence warnings; per-warning options
(`-Werror=NAME`); changes to TED's own code or the `comdata` table.

## Capabilities

### New Capabilities
- `compiler/warnings`: the warning mechanism, the "warnings are errors"
  option, and the resident-constant clash warning.

### Modified Capabilities
- `ab3-compat/command-line`: the option `-Werror`, the console format of
  a warning, the return codes.

## Impact

- `programs/blitz2/src/parse.c`/`parse.h` (`warn`, option test),
  `stmts.c` (`loadres`, new constant check), `ext.h`, `ext_bb2.c`,
  `ab3main.c` (one new extension point and its implementation),
  `data.c` (option gadget `optreq2ga5` visible, label and layout of the
  second options requester).
- `shared/ab3job/` (flag in the job and `-Werror` parsing),
  `programs/ab3cli/src/main.c` (applies the flag to the options),
  `tools/lib/ab3job` host tests.
- Test runner: a job-line key to switch the option for the compiler
  tests (`tests/corpus/runner`).
- Results: executables and `.xtra` files stay byte-identical; the first
  definition of a clashing constant still wins.
