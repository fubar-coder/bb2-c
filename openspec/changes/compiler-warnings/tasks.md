# Tasks

The user's decisions of 2026-11-08 (design.md, "Decisions of
2026-11-08") are folded in.

## 1. Mechanism

- [ ] 1.1 Warning numbers and names (`ab3job.h` `AB3JOB_W_*`, `ab3args_warnings[]`; `parse.h` `W_RESCLASH`, `static_assert` in `ab3main.c`); `warn(w, msg, detail)` and `warnsets()` in `parse.c`/`parse.h`; `ext_warning()` and `ext_warnsets()` in `ext.h`, `ext_bb2.c` (FALSE, no change), `ab3main.c` (console line, the job's lists). Verify: the tests of D8 for each outcome (silent, shown, error)
- [ ] 1.2 Immediate path through `ed_error("Warning: ...")` outside a compile; direct mode silent. Verify: the test stub logs the line
- [ ] 1.3 Queued path for compile-time warnings: with the first compile-time warning (decided, Q4); not part of this change

## 2. Settings

- [ ] 2.1 The word `warnopt` (`.xtra` +24) in `data.c` instead of `optreq2ga5.Flags` in `optspnts`; `optreq2ga5` a visible button in the chain `ga4 -> ga5 -> ga8` with the label `WARNINGS:` and the texts `  OFF `/`  ON  `/`ERRORS`; `runerropts` steps it and restores it on Cancel. Verify: an emulator screenshot of TED with the requester; a `.xtra` saved with the setting untouched is byte-identical (`prev_results`, `ted_ab` `P5`/`Pc`)
- [ ] 2.2 `docs/formats.md` 1.1 and a name for the switch in `shared/xtra/xtra.h` (`XTRA_SW_OPTREQ2GA5` renamed `XTRA_SW_WARNINGS`, the values), `xtra_init` default stays ON. Verify: the `xtra` host tests
- [ ] 2.3 `-w LIST` and `-werr LIST` in `shared/ab3job` (`ab3args.c`, `ab3job.h`: the masks, usage text with the names); `ab3cli` ends with rc 10 when its set-up errors were warnings that are errors. Verify: `test_ab3args` with deliberately broken input; `ab3_cli` cases for rc 0, 10 and 20

## 3. The #36 check

- [ ] 3.1 `checkconstclash()` in `stmts.c` (hash table over `firstlabel`, name at 19, value `pc`; only differing values), called in `loadres` inside the error context; remove the dead `firstconst` call, the `BUG(orig)` comment becomes `Fixed (#36)`. Verify: the cases of design D8
- [ ] 3.2 Record each resident's constants (range in `firstlabel`) for the command line message. Verify: the console test names both residents
- [ ] 3.3 Limit of 8 reported clashes plus the "N more" line. Verify: a test with 10 clashing names

## 4. Tests

- [ ] 4.1 `compiler-loadres-clash-offset`: `; @fixed #36`, new `.expect`, `.orig` deleted. Verify: FIXED in `ctest --preset all`
- [ ] 4.2 Job-line key `warnings=off|on|errors` in `tests/corpus/amiga/tedstub.c`; tests `compiler-loadres-clash-werror`, `-off`, `-same`, `-many`. Verify: all pass; a broken expectation fails
- [ ] 4.3 `ab3_cli` cases and `test_ab3args` host tests (design D8)

## 5. Documentation

- [ ] 5.1 `docs/ab3/usage.md`, the `-h` text, `docs/v2.10/blitz2.md`, `docs/architecture.md` 3.6, `docs/libs/deviations.md` (kind F, #36), `tests/README.md` (the job-line key). Verify: every document names the same options, setting and message

## 6. Check and wrap-up

- [ ] 6.1 `corpus_cd` before and after: no executable changes; list the sources whose residents now warn (commit message, report). Verify: `ctest --preset all` including `corpus_cd`
- [ ] 6.2 Acceptance: `-DBLITZ2_PREV=ON`, `ctest --preset all`: results byte-identical (executables, `.xtra`); the programs `blitz2`, `blitz2-ab3`, `ab3cli` change. The report names steps 3-5 of the fix workflow (website entry, label `fixed-in-port`, close #36) for the coordinator
