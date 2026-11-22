# Design

## Context

- #43's check (R11, commit 59d37a8c9): `BLITZ_SUBS(doprint, 0, printctrlc)`, `BLITZ_SUBS(donprint, 0, printctrlc)` (`printlib.c:113`, `:119`), the four Edit$/Edit entries with `editctrlc` as their after-check (`editlib.c:85-106`). `makelsub3` (`programs/blitz2/src/libcall.c:978`) emits before-check, routine and after-check only when `DEBUGGA_B` is set and the entry has a check; otherwise only the routine (`.norerr`, also between `Runerrsoff` and `Runerrson`, `stmts.c:113-126`). The after-checks jump to inputoutputlib's error-area entries 12/13 (`ctrlc_c`, `inputoutputlib.c:414`): test `SIGBREAKF_CTRL_C` in `tc_SigRecvd` (no OS call), compare the current handler (`iochan_out.code` / `iochan_in.code`, the library's own variables) with `defout`/`defin`, clear the signal, write "***Break\n" to `outputh`, call runerrlib's `ctrlcend` ($C321), which replaces the program's return address (runerrlib's `_ostack` + 4) by a stub that sets d0 = 20 and ends through exitslib's `endjmp`.
- In a release executable none of these are linked (error areas not copied; runerrlib only with runtime errors, `compile.c:788`).
- The six entries are reached from statements (`dolibtoke`, `compile.c:1512`) and from functions in expressions (`eval.c` `function` → `dolibtoke` → `userjsr` → `makelsub3`): `makelsub3` is the one place every call goes through. No intrinsic exists for these tokens.
- blitz2-ab3's command line: `shared/ab3job/ab3args.c` (`ab3args_parse`; options in any case, AmiBlitz3's spelling `-name`) fills `struct ab3job` (`ab3job.h`); the core reads the job's settings through extension points (`ext.h`; precedent `ext_warnsets` for `-w`/`-werr`, `ab3main.c:177`). `ext_bb2.c` (blitz2) handles nothing.
- `compilerlib` (`libs/compilerlib/compilerlib.c`, system library 64030, $D600-$D602): embedded in blitz2 and blitz2-ab3, linked first in both modes whatever library set is loaded; a program that uses none of its routines gets nothing of it.

## Goals / Non-Goals

**Goals:** #43's behaviour in an executable compiled with `-ctrlcbreak`; every compile without it byte-identical; no library file changed.

**Non-Goals:** see the proposal, "Not in scope"; in particular the TED/.xtra form of the option (user: "Erstmal nur Kommandozeile").

## Decisions

The user's decisions 1-4 are in the proposal. Open questions 1 and 2 are written with their recommended answers; another answer to question 1 replaces where decisions 3 and 4 put the routines (b: a system library 165 in `libs/acidlibs/`; c: tokens $D603-$D605 of `compilerlib`; d: normal-area tokens of inputoutputlib that read `iochan_*` directly, no probe).

1. **The option.** `ab3args_parse` takes `-ctrlcbreak` (no value; given twice counts once) into a job byte `ctrlcbreak` (the free `pad` byte after `result`, so the job's layout and size stay; ab3cli and blitz2-ab3 are built together anyway). The usage line and `docs/ab3/usage.md` list it. The host test `test_ab3args.cpp` gets the cases: given, given in upper case, absent.
2. **The way to the code emission.** A new extension point `BOOL ext_ctrlcbreak(void)` (ext.h): `ext_bb2.c` returns FALSE; `ab3main.c` returns `ab3job && ab3job->ctrlcbreak`. blitz2-ab3 refuses the switch for an AB3-mode source when it decides the mode (open question 2 (a)): the message "-ctrlcbreak needs a BB2-mode source", return code 20 (`AB3JOB_RC_FAIL`), no executable. `makelsub3`, on its `.norerr` path, emits `jsr` of the output check behind the routine of Print/NPrint and of the input check behind Edit$/Edit when `ext_ctrlcbreak()` is TRUE. The six entries are identified by library number (printlib 187, editlib 186; constants in `main.h` from the variant's numbers) and token offset. The value is constant for a compile, so every pass emits the same code.
3. **The routines: a new internal library `cbreaklib`** (working name; system library 64029, its token range below `compilerlib`'s, both checked free in task 1.1), embedded and loaded as `compilerlib` (`asmlibs.c` `loadlibs`, both modes) but linked only into a program that calls it. Tokens `cbreakout`, `cbreakin` (the release counterparts of `ctrlcout`/`ctrlcin`) and `cbreakinit`. They test the signal bit first (no OS call; without the signal: the test and an `rts`). Only with the signal set they find out whether the current handler is the default one through inputoutputlib's tokens: `DefaultOutput` (pushes the current handler, sets `defout`), `dosetout` (+4: returns the previous handler, i.e. `defout`'s address, and pushes), two `PopOutput` back to the program's handler, `dosetout` again to read it, `PopOutput`; the same with the input tokens. Pushes write only above the stack's top, so the ring of 16 is left as it was. The spike (task 1.1) tries it in the emulator first (spike 3.1: "not tried").
4. **The end and the stack level.** The program's return address is needed for return code 20 (as `ctrlcend`: replace it at the level runerrlib's `_ostack` holds by a stub `moveq #20,d0`, then exitslib's `endjmp`). `cbreakinit` is that level's source: the compiler emits a `jsr cbreakinit` at the program's start when the switch is on (as it emits other start code; place found in task 1.1), and it stores sp. An init entry of the library (the `BLITZ_NULLSUB_AT(init, ...)` form, run only in programs that link it) is the simpler form; task 1.1 decides by measurement which one keeps programs without the switch byte-identical. Clearing the signal and writing "***Break\n" to `Output()` (if there is one) as `ctrlc_c`.
5. **With runtime error checking** (decision 4 of the user): where `makelsub3` emits the error checks, the entry's own after-check (#43) is the check and nothing is added; on the `.norerr` path (release, or a `Runerrsoff` region) the release check is emitted.
6. **Registers.** The two checks keep every register, and the CCR is no result (they come behind a routine that returns d0 and a3, as `ctrlcout`/`ctrlcin`); a KEEPS case in the library test (`libs/register-contract`).

### Planning choices (mine)

- `Point: the switch written -ctrlcbreak, case-insensitive, no value — by the user's "CTRLCBREAK, written as the existing blitz2-ab3 options are" and ab3args_parse's rules — L1 Unlikely / I1 Low / E3 Proven (ab3job.h, ab3args_parse comment) → Monitor` (precedent: the user's answer, decision 3).
- `Point: the switch reaches makelsub3 through a new extension point ext_ctrlcbreak, not a global set by ab3main.c — L1 Unlikely / I1 Low / E2 Reasoned → Monitor` (the pattern of ext.h; blitz2 keeps its code).
- `Point: the job byte from the pad after result — another change in flight (blitz-coverage, buffer-access-annotations name ab3job) may take the same byte — L2 Possible / I1 Low / E2 Reasoned → Monitor` (the later merge picks another byte or extends the struct).
- `Point: the check in libcall.c's makelsub3, not in eval.c — a call path that bypasses makelsub3 (a later intrinsic for one of the six tokens) would miss the check — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor` (the tests call Edit$ inside an expression).
- `Point: the console test by the token probe of decision 3 — it may not link from an internal library or may disturb the channel stack — L2 Possible / I2 Medium / E2 Reasoned → Plan` (task 1.1 first, stop and report on failure).
- `Point: the stack level for return code 20 by cbreakinit or the library's init — L2 Possible / I2 Medium / E2 Reasoned → Plan` (task 1.1 measures both; the test checks return code 20 and the cleanup).

### Order

- Not in the eval.c line (`eval.c` is not edited).
- `asmlibs.c` (`loadlibs`: one embedded library more): other functions of it are edited by `ab3-mode-g5`, `compiler-function-optimizer`, `coverage-ted-libs` and the `expr-tree-3c/3d/4b` changes; no order set. `compilerlib.c` (#1191, running) is not edited.
- `libcall.c`: other functions are edited by `byte-fraction-constant` (running), `array-later-index`, the `expr-tree-*` changes; no order set, the later merge resolves the text. `Point: merge conflict in libcall.c — L2 Possible / I1 Low / E2 Reasoned → Monitor`.
- No shared file with `original-products-commands` any more (no library number, no `acidlibs` entry).

## Risks / Trade-offs

- [The switch is only on blitz2-ab3's command line] → TED users cannot set it (user's decision 2, "Erstmal"); a later change can add a TED/.xtra form, which needs a free switch place (all 14 taken).
- [A program between its own Ctrl-C test and its next console call] → ended before its own test sees the signal (spike 4); such a program is compiled without the switch.
- [Option (c) of question 1 would change executables without the switch] → `compilerlib` is linked whole; every program with #1178's routines would grow. Hence the separate internal library of (a).
