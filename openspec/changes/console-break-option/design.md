# Design

## Context

- #43's check (R11, commit 59d37a8c9): `BLITZ_SUBS(doprint, 0, printctrlc)`, `BLITZ_SUBS(donprint, 0, printctrlc)` (`printlib.c:113`, `:119`), the four Edit$/Edit entries with `editctrlc` as their after-check (`editlib.c:85-106`). `makelsub3` (`programs/blitz2/src/libcall.c:978`) emits before-check, routine and after-check only when `DEBUGGA_B` is set and the entry has a check; otherwise only the routine (`.norerr`, also between `Runerrsoff` and `Runerrson`, `stmts.c:113-126`). The after-checks jump to inputoutputlib's error-area entries +12/+13 (`ctrlc_c`, `inputoutputlib.c:414`): test `SIGBREAKF_CTRL_C` in `tc_SigRecvd` (no OS call), compare the current handler (`iochan_out.code` / `iochan_in.code`) with `defout`/`defin`, clear the signal, write "***Break\n" to `outputh`, call runerrlib's `ctrlcend` ($C321), which replaces the program's return address (runerrlib's `_ostack` + 4) by a stub that sets d0 = 20 and ends through exitslib's `endjmp`.
- In a release executable none of these are linked (error areas not copied; runerrlib only with runtime errors, `compile.c:788`).
- inputoutputlib's entries: DefaultInput, DefaultOutput, +3..+9 the channel routines other libraries call, PopInput, PopOutput, +12/+13 `ctrlcout`/`ctrlcin` (error area), +14 `init` (`inputoutputlib.c:90-127`). Other libraries (printlib, editlib, windowslib, fontlib, fileiolib, the RWE userlibs ...) reach it by these numbers. It is linked into every program with channel input or output.
- The six entries are reached from statements (`dolibtoke`, `compile.c:1512`) and from functions in expressions (`eval.c` `function` → `dolibtoke` → `userjsr` → `makelsub3`): `makelsub3` is the one place every call goes through. No intrinsic exists for these tokens.
- blitz2-ab3's command line: `shared/ab3job/ab3args.c` (`ab3args_parse`; options in any case, `-name`) fills `struct ab3job`; the core reads the job's settings through extension points (`ext.h`; precedent `ext_warnsets`, `ab3main.c:177`). `ext_bb2.c` (blitz2) handles nothing.
- Linking: a library is linked whole into a program that uses one of its entries, so the two new inputoutputlib routines are in every executable that links inputoutputlib, with and without the switch (the analyst's `~/.local/share/bb2-c-work/analysis-library-linking.md`, as reported by the coordinator on 2026-11-22; `compilerlib.c` head comment; spike 3.3). Linking only the used routines is issue #1196, not this change.

## Goals / Non-Goals

**Goals:** #43's behaviour in an executable compiled with `-ctrlcbreak`; the check in inputoutputlib (user decision 5) with direct access to its channel variables; the changed results named (user decision 7).

**Non-Goals:** see the proposal, "Not in scope": the TED/.xtra form ("Erstmal"), the compiler optimisation that links only used routines (decision 7, issue #1196), a change of #43's check.

## Decisions

The user's decisions 1-9 are in the proposal.

1. **The option.** `ab3args_parse` takes `-ctrlcbreak` (no value; given twice counts once) into a job byte `ctrlcbreak` (the free `pad` byte after `result`; the job's layout and size stay). The usage line and `docs/ab3/usage.md` list it.
2. **The way to the code emission.** A new extension point `BOOL ext_ctrlcbreak(void)` (ext.h): `ext_bb2.c` returns FALSE; `ab3main.c` returns `ab3job && ab3job->ctrlcbreak` in BB2 mode. In AB3 mode `ab3main.c` prints the note "-ctrlcbreak ignored: AB3-mode source" once when it has decided the mode and returns FALSE (user decisions 6 and 9: a plain line, no warning, the return code as without the switch). `makelsub3`, on its `.norerr` path, emits a `jsr` of inputoutputlib's output check behind the routine of Print/NPrint and of its input check behind Edit$/Edit when `ext_ctrlcbreak()` is TRUE. The six entries are identified by library number (printlib 187, editlib 186; constants in `main.h`) and token offset. The value is constant for a compile, so every pass emits the same code. If the loaded inputoutputlib has no such entries (user decision 8): the same kind of note, "-ctrlcbreak ignored: the libraries have no Ctrl-C check", and no check emitted.
3. **The routines: two entries of inputoutputlib's normal code, after `init`** (+15 `cbreakout`, +16 `cbreakin`), so the numbers +0..+14 that other libraries use stay. They do what `ctrlc_c` does, in normal code: test the signal bit (no OS call; without the signal: the test and an `rts`), compare `iochan_out.code` / `iochan_in.code` with `defout`/`defin` (the library's own variables: no probe through tokens), clear the signal, write "***Break\n" to `outputh` (if there is one), end the program (decision 4). Normal code must not reference the error area (`docs/development.md`, check routines); the error-area `ctrlc_c` of #43 stays as it is (it may call the new code, but that would change #43's path: not done).
4. **The end.** inputoutputlib's init records the stack level (as runerrlib's `_ostack`: every library init sees the same level, spike 3.2, used by #43's `ctrlcend`). The end replaces the program's return address at that level + 4 by a stub `moveq #20,d0` and jumps to exitslib's `endjmp` (ALibJsr). The init grows by one `move.l sp`; that is part of the accepted growth (decision 7).
5. **With runtime error checking** (decision 4): where `makelsub3` emits the error checks, the entry's own after-check (#43) is the check and nothing is added; on the `.norerr` path (release, or a `Runerrsoff` region) the release check is emitted.
6. **Registers.** The two checks keep every register, and the CCR is no result (they come behind a routine that returns d0 and a3, as `ctrlcout`/`ctrlcin`); a KEEPS case in the library test (`libs/register-contract`).

### Planning choices (mine)

- `Point: the switch written -ctrlcbreak, case-insensitive, no value — by the user's "CTRLCBREAK, written as the existing blitz2-ab3 options are" and ab3args_parse's rules — L1 Unlikely / I1 Low / E3 Proven (ab3job.h, ab3args_parse comment) → Monitor` (precedent: decision 3).
- `Point: the switch reaches makelsub3 through a new extension point ext_ctrlcbreak — L1 Unlikely / I1 Low / E2 Reasoned → Monitor`.
- `Point: the job byte from the pad after result — another change (blitz-coverage, buffer-access-annotations name ab3job) may take the same byte — L2 Possible / I1 Low / E2 Reasoned → Monitor`.
- `Point: the new entries after init (+15, +16), not in front of it — inserting before init would renumber init — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor`.
- `Point: #43's error-area check stays untouched, the release check is a second copy of its logic in normal code — L2 Possible / I1 Low / E2 Reasoned → Monitor` (a later clean-up can let `ctrlc_c` call the normal code; it changes executables with runtime error checking).
- `Point: the stack level recorded in inputoutputlib's init — the level of its init could differ from runerrlib's — L2 Possible / I2 Medium / E2 Reasoned → Plan` (task 1.2 measures it; the test checks return code 20 and the cleanup).
- `Point: the check in libcall.c's makelsub3, not in eval.c — a call path that bypasses makelsub3 would miss the check — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor` (the tests call Edit$ inside an expression).

### Order

- Not in the eval.c line (`eval.c` is not edited).
- The prev comparison: the commit changes inputoutputlib and every executable with channel I/O (decision 7). A change whose prev comparison runs against a main without this commit would list these executables too. The coordinator merges this change on its own and runs its prev comparison right after, not in one comparison with another results-changing change; no order between planned changes is set by this. `Point: other changes' PREV_INTENDED lists mixed with this change's many changed executables — L2 Possible / I2 Medium / E2 Reasoned → Plan`.
- `libcall.c`: other functions are edited by `byte-fraction-constant` (running), `array-later-index`, the `expr-tree-*` changes; no order set. `Point: merge conflict in libcall.c — L2 Possible / I1 Low / E2 Reasoned → Monitor`.
- `inputoutputlib.c` is edited by no other open change.

## Risks / Trade-offs

- [Every executable with channel I/O grows by the two routines, also without the switch] → accepted by the user (decision 7) until the compiler links only used routines; the size is measured in task 4.2 and named in the commit.
- [The switch is only on blitz2-ab3's command line] → TED users cannot set it (decision 2, "Erstmal"); a later form needs a free switch place (all 14 taken).
- [A program between its own Ctrl-C test and its next console call] → ended before its own test sees the signal (spike 4); such a program is compiled without the switch.
