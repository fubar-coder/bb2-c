# Proposal

## Why

Since #43 (R11), a program compiled with runtime error checking ends at its next console `Print`, `NPrint`, `Edit$` or `Edit` after Ctrl-C ("***Break", return code 20), as a C program on AmigaOS does. The check lives in the error check entries of printlib and editlib (`printctrlc`, `editctrlc`, `libs/acidlibs/printlib.c:1126`, `editlib.c:394`), which jump to inputoutputlib's `ctrlcout`/`ctrlcin` (`inputoutputlib.c:395-440`) and runerrlib's `ctrlcend` ($C321). None of it is in a program compiled without runtime error checking (the error check area is not linked; `asmlibs.c:926`, `compile.c:787-797`): a release executable never reacts to Ctrl-C (regression test `tests/bug-orig/regression/rundebuglib-ctrlc-no-runerrs`).

Issue #1172 asks for an optional way to get the same behaviour in release executables. The user, when #43 was decided (2026-11-20, `~/.local/share/bb2-c-work/decisions/archive/2026-11-21-r11/decisions.md`, "group D, #43 again"): "Unterstützung für Release-Modus kommt dann später. Ist dafür ein Issue angelegt?". Design study: `~/.local/share/bb2-c-work/r11-ctrlc-spike.md`.

## Decisions of the user (2026-11-22, `~/.local/share/bb2-c-work/decisions/tasks/decisions.md`)

1. **Behaviour** (entry "#1172: behaviour and form; #1036: names and place"): "Genau wie #43 (Recommended)" — Ctrl-C acts at the next console Print/NPrint/Edit/Edit$, once per call, console only: "***Break", return code 20.
2. **Form** (same entry): "Oh, ich hatte gedacht, dass es eine neue Compiler-Option sei und kein Schlüsselwort. Deshalb: Erstmal nur Kommandozeile" — for now only a command-line option of blitz2-ab3; no statement in the source. A later form (a real compiler option, e.g. in TED's requesters) is not excluded ("Erstmal") and not built here. The earlier answer "CtrlCBreak (Recommended)" (entry "#1172: name, reach, Runerrsoff regions") was given for a statement and does not apply as it stands.
3. **Name** (entry "#1172 option name; #1173 open points"): "CTRLCBREAK (Recommended)" — written as the existing blitz2-ab3 options are: `-ctrlcbreak` (blitz2-ab3 takes its options in any case, `shared/ab3job/ab3job.h`, `ab3args_parse`).
4. **Runerrsoff regions** (entry "#1172: name, reach, Runerrsoff regions"): "Ja (Recommended)" — with the switch and runtime error checking on, console calls between `Runerrsoff` and `Runerrson` get the release check; kept by the entry "#1172: behaviour and form" ("'Ja' for Runerrsoff regions still applies").
5. **Routines** (entry "#1172: routines in inputoutputlib; AB3 mode ignored"): "In inputoutputlib" — the check routines go into inputoutputlib.
6. **AB3 mode** (same entry): "Ignorieren mit Hinweis" — `-ctrlcbreak` with an AB3-mode source compiles without the check and prints a note on the console.
7. **Changed release executables** (entry "#1172: release binaries may change"): "Zu #1172: Wir nehmen bei Bedarf die Änderung der Release-Binary-Änderungen in Kauf. Mit einer späteren Optimierung des Compilers ist das Problem dann behoben." — the issue's condition "release executables without the option stay byte-identical" is dropped for this change; the changed results are named as intended. A later compiler optimisation (linking only the used routines) is to remove the growth; it is not part of this change (issue #1196, "Link only the used routines of a library").
8. **A library set without the new entries** (entry "#1172: old inputoutputlib; the note's return code"): "Ignorieren mit Hinweis (Recommended)" — `-ctrlcbreak` with an inputoutputlib that lacks the new entries (the CD's acidlibs, the originals configurations) is ignored with a note, as for an AB3-mode source.
9. **The note** (same entry): "Einfache Zeile, RC wie ohne Schalter (Recommended)" — a plain console line; the return code is the same as without the switch.

## What Changes

- **`blitz2-ab3 -ctrlcbreak`**: a BB2-mode compile from the command line with this switch emits a call of a check behind every call of the six console entries (Print, NPrint, both forms of Edit$, both forms of Edit) that does not already get #43's check of runtime error checking. With `-r` that is every call; without `-r` the calls between `Runerrsoff` and `Runerrson` (decision 4).
- **Behaviour at run time as #43** (decision 1): Ctrl-C only sets the signal; the check runs once per call, after the call, and acts only when the signal is set and the current output (Print, NPrint) or input (Edit$, Edit) is the default channel (not after WindowOutput, BitMapOutput, FileOutput, their inputs, BlitzKeys On, IPOutput); then the signal is cleared, "***Break" and a newline go to the console, and the program ends with return code 20, its finits running as at `End`.
- **The check routines** are two new internal entries of inputoutputlib's normal code (decision 5), behind its existing entries, with the end of the program in inputoutputlib itself.
- **AB3 mode and libraries without the entries** (decisions 6, 8, 9): with an AB3-mode source, or with an inputoutputlib that lacks the new entries, the switch is ignored; blitz2-ab3 prints a plain note line and compiles as without it, with the return code it would have without the switch.
- **Results** (rule 3; decision 7; named with `-DPREV_INTENDED`): inputoutputlib's library file in every variant and the `acidlibs` files that hold it; every executable that links inputoutputlib (every program with input or output on a channel: Print, NPrint, Edit, Edit$, Inkey$, the window, bitmap and file outputs ...), with and without runtime error checking, also without the switch (a library is linked whole, so its code grows by the two routines in every such executable; the analyst's `~/.local/share/bb2-c-work/analysis-library-linking.md`, as reported by the coordinator on 2026-11-22); the library capture of inputoutputlib if it covers the code size. Programs without input or output and every other library file stay byte-identical. The programs `blitz2-ab3` (option, usage, note) and `blitz2` (the shared core) are reworked.
- **Docs and site**: `docs/ab3/usage.md` (the option table), `docs/blitz2/extensions.md`, `docs/libs/deviations.md` (inputoutputlib's new entries; a kind other than F: no original bug); the site's "Extensions" page and AB3 mode's command-line page (`site/content/docs/ab3-mode/command-line.md`); a pointer from the #43 entry of the fixed-bugs page.

## Not in scope

- A compiler option in TED's requesters, an .xtra switch or a source statement (decision 2: "Erstmal"; a later change).
- The compiler optimisation that links only the used routines of a library (decision 7: "Mit einer späteren Optimierung des Compilers"; issue #1196, "Link only the used routines of a library").
- A check per statement or per character; a loop without console I/O stays unabortable (as `for(;;);` in C; spike 3.4).
- Inkey$ (never console input), `Write_(Output_(),…)` and other direct doslib calls, TED itself.
- The "Interrupt Checking" switch (stays without effect).
- Any change of #43's check (the error check entries, runerrlib's `ctrlcend`).

## Open questions

(none: decisions 1-9 answer every question of this change)

## Capabilities

### New Capabilities

- `compiler/console-break`: the command-line option that makes an executable end on Ctrl-C at its next console input or output: how it is given, which calls it covers, what the program does, what AB3 mode does.

### Modified Capabilities

(none; the new option is added to `docs/ab3/usage.md`; its requirements live in `compiler/console-break`)

## Impact

- `shared/ab3job/ab3job.h`, `ab3args.c` (the option, a job field, the usage); `tools/tests/test_ab3args.cpp`.
- `programs/blitz2/src/ext.h`, `ext_bb2.c`, `ab3main.c` (an extension point that hands the job's switch to the core; the note), `libcall.c` (`makelsub3`: the check behind the six entries), `main.h` (the token constants). **Not** `programs/blitz2/src/eval.c`: Edit$ and Edit as functions reach `makelsub3` through `dolibtoke` → `userjsr` (`compile.c:1512-1513`), so this change is not part of the eval.c line (task decision 2026-11-22, "Matrix limit in the eval.c line; more issues in parallel").
- `libs/acidlibs/inputoutputlib.c` (two entries after `init`, the stack level in its init, the end); `libs/acidlibs/declarations-intended.txt` if the build asks for it.
- Tests: `tests/ab3/cli` cases with `-r -ctrlcbreak`; the existing `rundebuglib-ctrlc-*` tests unchanged; a `libs/tests` case; the prev comparison (`PREV_INTENDED`: inputoutputlib, the `acidlibs` files, the executables with input or output).
- Overlaps: `byte-fraction-constant`, `array-later-index`, `expr-tree-1-typing`, `expr-tree-3a-leaves`, `expr-tree-3d-library-calls`, `expr-tree-4a-emit-after-build`, `expr-tree-4b-lookaheads` edit `libcall.c` (other functions); `blitz-coverage`, `buffer-access-annotations` name `ab3job`; every change whose prev comparison runs in parallel sees the many changed executables (design, "Order").
