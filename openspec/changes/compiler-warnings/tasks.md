# Tasks

Planning only so far; the implementation follows after the user approved
the design (open questions Q1-Q6 in design.md).

## 1. Mechanism

- [ ] 1.1 `warn()` in `parse.c`/`parse.h`, `werror()` predicate on `optreq2ga5`; `ext_warning()` in `ext.h`, `ext_bb2.c` (FALSE), `ab3main.c` (prints in a command line run). Verify: a test of both outcomes (message shown; `err` equivalence with the option on)
- [ ] 1.2 Immediate path through `ed_error("Warning: ...")` outside a compile; direct mode silent. Verify: the test stub logs the line
- [ ] 1.3 (after Q4) queued path for compile-time warnings, only together with its first user

## 2. Option

- [ ] 2.1 `optreq2ga5` visible in `data.c` (chain `ga4 -> ga5 -> ga8`, label "Warnings only", layout checked on the emulator). Verify: a requester test or screenshot; a `.xtra` saved with the option untouched is byte-identical (`prev_results`, `ted_ab` `P5`/`Pc`)
- [ ] 2.2 `docs/formats.md` 1.1 and a name for the switch in `shared/xtra/xtra.h` (`XTRA_SW_OPTREQ2GA5` renamed), `xtra_init` default stays ON. Verify: the `xtra` host tests
- [ ] 2.3 `-Werror` in `shared/ab3job` (`ab3args.c`, `ab3job.h`, usage text), applied by `ab3cli` after `load_options`. Verify: `tools/lib/ab3job` host tests with deliberately broken input; `ab3_cli` cases for rc 0 and 10

## 3. The #36 check

- [ ] 3.1 `checkconstclash()` in `stmts.c` (walk `firstlabel`, name at 19, value `pc`), called in `loadres` inside the error context; remove the dead `firstconst` call and the `BUG(orig)` comment (`Fixed (#36)`). Verify: the cases of design D8
- [ ] 3.2 Record each resident's constants (range in `firstlabel`) for the command line message. Verify: the console test names both residents
- [ ] 3.3 Limit of 8 reported clashes plus the "N more" line. Verify: a test with 10 clashing names

## 4. Tests

- [ ] 4.1 `compiler-loadres-clash-offset`: `; @fixed #36`, new `.expect`. Verify: FIXED in `ctest --preset all`
- [ ] 4.2 Job-line key `werror=on` in `tests/corpus/runner`; tests `compiler-loadres-clash-werror` and `compiler-loadres-clash-same`. Verify: both pass; a broken expectation fails
- [ ] 4.3 `ab3_cli` cases and `ab3job` host tests (design D8)

## 5. Documentation

- [ ] 5.1 `docs/ab3/usage.md`, the `-h` text, `docs/v2.10/blitz2.md`, `docs/architecture.md` 3.6, `docs/*/deviations.md` (kind F, #36), `tests/README.md` (the job-line key). Verify: every document names the same option, label and message

## 6. Check and wrap-up

- [ ] 6.1 `corpus_cd` before and after: no executable changes; list the sources whose residents now warn (TODO.md). Verify: `ctest --preset all` including `corpus_cd`
- [ ] 6.2 Acceptance: `-DBLITZ2_PREV=ON`, `ctest --preset all`: results byte-identical (executables, `.xtra`); the programs `blitz2`, `blitz2-ab3`, `ab3cli` change. The report names steps 3-5 of the fix workflow (website entry, label `fixed-in-port`, close #36) for the coordinator
