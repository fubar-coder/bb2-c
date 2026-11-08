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
when the values differ, that every warning has a name and can be
disabled or made an error, and how the settings look on the command
line and in the editor.

## What Changes

- Named warnings in the compiler (the first: `resclash`) and two sets:
  the enabled warnings and the warnings that are errors; a disabled
  warning is never an error; default all enabled, none an error.
  `warn(w, msg, detail)` beside `err`: a disabled warning is silent, an
  enabled one is shown and does not stop the compile or change the
  result, one that is an error behaves exactly as `err(msg)` (design
  D1, D2).
- How a warning reaches the user in each front end: the editor (TED, and
  the headless editor `ab3cli`) through the existing error service with
  the prefix `Warning: `; on the console as
  `SOURCE: warning: MESSAGE [NAME]` in a `blitz2-ab3 -s ... -e ...` run,
  through new points in `ext.h` (design D3, D4).
- The command line of `blitz2-ab3`: `-w LIST` sets the enabled warnings,
  `-werr LIST` the ones that are errors; a list is comma-separated items
  `all`, `none`, `NAME`, `-NAME`, applied left to right to the source's
  setting; an unknown name is an argument error (rc 20) (design D4).
- The editor: the hidden option gadget `optreq2ga5` (`.xtra` word +24,
  unused by the compiler today, `$0080` in every CD file) becomes a
  visible three-state setting "WARNINGS: OFF / ON / ERRORS" in the
  second options requester for all warnings; `$0080` is "on", so old
  `.xtra` files load as "on" and are saved byte-identical (design D5).
- Return codes: warnings leave the return code at 0; a warning that is
  an error gives 10 (design D6).
- #36: constants with the same name in the residents give the warning
  "Clash in Residents: #NAME" when their values differ; same name and
  same value is silent; types and macros keep the error (design D7).
- Tests: the bug-orig test turns FIXED (`; @fixed #36`), new tests for
  the settings, the console and the option parsing (design D8).
- Documentation: option list, the options requester description, the
  deviations entry (kind F) for #36 (design D9).

Out of scope: further warnings (the mechanism makes them cheap, none is
added); the queue for compile-time warnings (with the first one);
per-warning settings in the editor (a new `.xtra` field, with the second
warning); changes to TED's own code or the `comdata` table.

## Capabilities

### New Capabilities
- `compiler/warnings`: named warnings, the enabled and error sets, the
  editor's setting, and the resident-constant clash warning.

### Modified Capabilities
- `ab3-compat/command-line`: the options `-w` and `-werr`, the console
  form of a warning, the return codes.

## Impact

- `programs/blitz2/src/parse.c`/`parse.h` (`warn`, the sets), `stmts.c`
  (`loadres`, new constant check, `runerropts`), `ext.h`, `ext_bb2.c`,
  `ab3main.c` (two new extension points and their implementation),
  `data.c` (the word `warnopt`, gadget `optreq2ga5` visible, label of the
  second options requester).
- `shared/ab3job/` (warning names, the lists in the job, `-w`/`-werr`
  parsing), `programs/ab3cli/src/main.c` (rc 10 for a warning that is an
  error), `tools/tests/test_ab3args.cpp`.
- Test stub: a job-line key `warnings=` for the compiler tests
  (`tests/corpus/amiga/tedstub.c`).
- Results: executables and `.xtra` files stay byte-identical; the first
  definition of a clashing constant still wins.
