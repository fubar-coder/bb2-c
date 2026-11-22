# Proposal

## Why

Since #43 (R11), a program compiled with runtime error checking ends at its next console `Print`, `NPrint`, `Edit$` or `Edit` after Ctrl-C ("***Break", return code 20), as a C program on AmigaOS does. The check lives in the error check entries of printlib and editlib (`printctrlc`, `editctrlc`, `libs/acidlibs/printlib.c:1126`, `editlib.c:394`), which jump to inputoutputlib's `ctrlcout`/`ctrlcin` (`inputoutputlib.c:395-440`) and runerrlib's `ctrlcend` ($C321). None of it is in a program compiled without runtime error checking (the error check area is not linked; `asmlibs.c:926`, `compile.c:787-797`): a release executable never reacts to Ctrl-C (regression test `tests/bug-orig/regression/rundebuglib-ctrlc-no-runerrs`).

Issue #1172 asks for an optional way to get the same behaviour in release executables. The user, when #43 was decided (2026-11-20, `~/.local/share/bb2-c-work/decisions/archive/2026-11-21-r11/decisions.md`, "group D, #43 again"): "Unterstützung für Release-Modus kommt dann später. Ist dafür ein Issue angelegt?". Design study: `~/.local/share/bb2-c-work/r11-ctrlc-spike.md`.

## Decisions of the user (2026-11-22, `~/.local/share/bb2-c-work/decisions/tasks/decisions.md`)

1. **Behaviour** (entry "#1172: behaviour and form; #1036: names and place"): "Genau wie #43 (Recommended)" — Ctrl-C acts at the next console Print/NPrint/Edit/Edit$, once per call, console only: "***Break", return code 20.
2. **Form** (same entry): "Oh, ich hatte gedacht, dass es eine neue Compiler-Option sei und kein Schlüsselwort. Deshalb: Erstmal nur Kommandozeile" — for now only a command-line option of blitz2-ab3; no statement in the source. A later form (a real compiler option, e.g. in TED's requesters) is not excluded ("Erstmal") and not built here. The earlier answer "CtrlCBreak (Recommended)" (entry "#1172: name, reach, Runerrsoff regions") was given for a statement and does not apply as it stands.
3. **Name** (entry "#1172 option name; #1173 open points"): "CTRLCBREAK (Recommended)" — the switch is written as the existing blitz2-ab3 options are: `-ctrlcbreak` (blitz2-ab3 takes its options in any case, `shared/ab3job/ab3job.h`, `ab3args_parse`).
4. **Runerrsoff regions** (entry "#1172: name, reach, Runerrsoff regions"): "Ja (Recommended)" — with the switch and runtime error checking on, console calls between `Runerrsoff` and `Runerrson` get the release check; the entry "#1172: behaviour and form" keeps it ("'Ja' for Runerrsoff regions still applies").

## What Changes

- **`blitz2-ab3 -ctrlcbreak`**: a compile from the command line with this switch emits a check behind every call of the six console entries (Print, NPrint, both forms of Edit$, both forms of Edit) that does not already get #43's check of runtime error checking. With `-r` that is every call; without `-r` the calls between `Runerrsoff` and `Runerrson` (decision 4).
- **Behaviour at run time as #43** (decision 1): Ctrl-C only sets the signal; the check runs once per call, after the call, and acts only when the signal is set and the current output (Print, NPrint) or input (Edit$, Edit) is the default channel (not after WindowOutput, BitMapOutput, FileOutput, their inputs, BlitzKeys On, IPOutput); then the signal is cleared, "***Break" and a newline go to the console, and the program ends with return code 20, its finits running as at `End`.
- **The check routines** live in a new internal library of the compiler, embedded in blitz2 and blitz2-ab3 next to `compilerlib` and linked only into a program compiled with the switch (worked with, open question 1 (a)): no change of `acidlibs`, `deflibs`, `compilerlib` or any other library file; it works with every BB2-mode library set whose inputoutputlib has the port's tokens (the CD's has the same code, `inputoutputlib.c` head comment).
- **AB3 mode**: `-ctrlcbreak` is refused for an AB3-mode source with a message (worked with, open question 2 (a)).
- **Results** (rule 3): without the switch every executable is byte-identical, and so is every library file. No `-DPREV_INTENDED` entry is needed. Reworked programs (rule 3: checked by the tests, named in the commit): `blitz2-ab3` (its usage text, the parser, the embedded new library) and `blitz2` (the embedded new library). A program compiled with the switch is new.
- **Docs and site**: `docs/ab3/usage.md` (the option table), `docs/blitz2/extensions.md`; the site's "Extensions" page and command-line page of AB3 mode (`site/content/docs/ab3-mode/command-line.md`); a pointer from the #43 entry of the fixed-bugs page.

## Not in scope

- A compiler option in TED's requesters, an .xtra switch or a source statement (decision 2: "Erstmal"; a later change).
- A check per statement or per character; a loop without console I/O stays unabortable (as `for(;;);` in C; spike 3.4).
- Inkey$ (never console input), `Write_(Output_(),…)` and other direct doslib calls, TED itself.
- The "Interrupt Checking" switch (stays without effect).
- Any change of #43's behaviour where #43's check is emitted.

## Open questions (for the user; worked with the recommendation)

1. **Where the check routines live.** (a) a new internal library embedded in blitz2/blitz2-ab3 like `compilerlib` (system number 64029, free in every number list), linked only when the switch emits a call: every library file and every executable without the switch stay byte-identical; (b) a new system library in `acidlibs` (number 165): the same for executables, but the `acidlibs` files change and the option works only with the port's libraries; (c) three new tokens in `compilerlib`: `compilerlib` is linked whole, so every executable that already uses its #1178 routines (a Quick operand under a Long type) grows, also without the switch (against "without the option byte-identical"); (d) tokens in inputoutputlib: changes inputoutputlib's file and every executable with input or output. The #1178 answer ("D: interne Library") created the internal-library form for #1178's routines; using that form here is yours. Worked with: (a).
2. **`-ctrlcbreak` with an AB3-mode source.** (a) refused with a message, return code 20 (AB3 mode's libraries are AmiBlitz3's, not checked for the probe of design decision 3); (b) ignored with a note on the console; (c) acting in AB3 mode too, after a check of AmiBlitz3's inputoutputlib and exitslib (a read of their tokens +3/+4, DefaultOutput/PopOutput, endjmp). Worked with: (a).

## Capabilities

### New Capabilities

- `compiler/console-break`: the command-line option that makes an executable end on Ctrl-C at its next console input or output: how it is given, which calls it covers, what the program does, and that executables without it do not change.

### Modified Capabilities

(none; `ab3-compat/command-line` describes AmiBlitz3's options and the port's extensions `-mode`, `-w`, `-werr`; the new option is added to the docs, and the requirement lives in `compiler/console-break`)

## Impact

- `shared/ab3job/ab3job.h`, `ab3args.c` (the option, a job field, the usage); `tools/tests/test_ab3args.cpp`.
- `programs/blitz2/src/ext.h`, `ext_bb2.c`, `ab3main.c` (an extension point that hands the job's switch to the core), `libcall.c` (`makelsub3`: the check behind the six entries), `main.h` (the token constants). **Not** `programs/blitz2/src/eval.c`: Edit$ and Edit as functions reach `makelsub3` through `dolibtoke` → `userjsr` (`compile.c:1512-1513`), so this change is not part of the eval.c line (task decision 2026-11-22, "Matrix limit in the eval.c line; more issues in parallel").
- `libs/compilerlib/` gets a sibling: the new internal library's source, its CMake entry and its embedding (`asmlibs.c` `loadlibs`, as `compilerlib`), its library tests.
- Tests: `tests/ab3/cli` cases with `-r -ctrlcbreak` (console, before nothing, FileOutput then DefaultOutput, Edit$ from redirected input, a program that consumes the signal, runtime errors on with a `Runerrsoff` region, an AB3-mode source refused), a case without the switch; the existing `rundebuglib-ctrlc-*` tests unchanged; the prev comparison.
- Overlaps: `byte-fraction-constant`, `array-later-index`, `expr-tree-1-typing`, `expr-tree-3a-leaves`, `expr-tree-3d-library-calls`, `expr-tree-4a-emit-after-build`, `expr-tree-4b-lookaheads` edit `libcall.c` (other functions); `asmlibs.c` (`loadlibs`, the embedded libraries) is edited by `ab3-mode-g5`, `compiler-function-optimizer`, `coverage-ted-libs`, `expr-tree-3c-calls`, `expr-tree-3d-library-calls`, `expr-tree-4b-lookaheads` (other functions); `blitz-coverage`, `buffer-access-annotations` name `ab3job` (design, "Order").
