# Design

## Context

The model's program is the driver the runner generates
(`tests/README.md`, "The driver and the library"); a cell is a pass of
`corpus.h` with `cell=on`, exactly one source, and the cells of a side
that share the session settings share one emulator session: each cell's
directory is `CELLk:`, `Assign CORPUS: CELLk:`, the compiles of all cells
first (`blitz2:blitz2 CORPUS:jobs` through tedstub, one job list per cell,
`runerr=off` for the whole list), then the runs (`caserun.h`). Files are
staged into a cell's `CORPUS:` with `files = ["DEST|SRC[|SELECTOR]"]`;
test data made from `NAME.spec` by `tools/mktestdata` are
`@TESTDATA@/NAME` (`tests/README.md`, "Adding a test": "Test data ... is
generated, not checked in"). The configuration files are a strict TOML
subset: table headers `[NAME]` and `[NAME."SELECTOR"]`, no inline tables
(user, 2026-11-21: "eine strikte Teilmenge zu Beginn").

The recorded answers this design rests on
(`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`): the
entry "Test-local data `ID.test/` with build actions (the user's idea)"
(2026-11-23; the idea's Q3 place "OK", Q4 observations "OK, dem Case
zuordnen", Q5 "Auch andere Werte könnten möglich sein", Q6 "Die Daten
gelten pro .test.bas"; its Q1 and Q2 were answered by the next entry);
"basic-test-local-data: form, compiler values, !RunProgram" and
"basic-test-local-data: dhandlerlib, timeout, order, spec split"
(2026-11-23, the answers to the plan's questions, below); "After the U2
analysis: PutReg, menuslib, run-off-end, texts" (run-off-end: "With the
child feature (Recommended)"); "Driver control data: D5 stays" ("Bei D5
bleiben"); "basic-test-runner: findings of implementer B, findit"
(compiler values `port | orig195 | orig210`).

## Goals / Non-Goals

**Goals:** a test brings its own files and programs; a case starts a
program and the run judges what it did; the six tests that start
themselves and the run-off-end scenario of #6 move to the model.

**Non-Goals:** checks inside a started program (it writes no result file;
its observations are its output, return code, requesters and end);
starting the compiler or TED from a test; a driver with parameters (D5);
moving the tests that build files with `Poke`; data shared by several
tests (a directory's data: refused by the idea's Q6);
`dhandlerlib-no-cache-flush` and the runner addition it needs (own change
before U4, see "The plan's questions, answered"); the split of "Entries
of the result file" (own change).

## Decisions

### D1 The table `data`

The user's answer (Q1): "Table per file (Recommended)": a table per file
whose header names it, one option per line, and the short form
`"FILE" = "ACTION"` in `[data]` for a file without options. The values of
`compiler` (Q2): "Axis values + same (Recommended)".

```toml
# an example, not a file of the tree
fixed-orig = 224

[data]
"cmap.spec" = "mktestdata"         # short form: the action, all options by default

[data."child.bas"]
action = "basic-compile"
compiler = "same"                  # same | port | orig195 | orig210
libs = "same"                      # same | cd | clib
runtime-errors = "same"            # same | on | off
```

Both are valid TOML and inside the strict subset (the header form
`[NAME."KEY"]` exists for `asserts` and `deviations`); no change of
`testtoml`. A file named in both forms in one file is refused (TOML
itself forbids it). Every key keeps its file and line for the
diagnostics and `--print-config`. The key is the path relative to
`ID.test/` (`sub/x.bin` allowed, no `..`, no `:`).

### D2 Actions and their results

- `basic-compile`: `NAME.bas` → `CORPUS:ID.test/NAME`; tokenised on the
  host with `tedsrc tok` and the cell's token list (as the driver); the
  placeholders `@TEST@`, `@TESTDIR@`, `@TESTDATA@` are replaced, a check
  macro is refused (the source has no result file).
- `copy`: the file → `CORPUS:ID.test/PATH`. It does what `files` already
  does with a path relative to the test, under the test's own directory;
  for text files and small committed files.
- `mktestdata`: `NAME.spec` → `CORPUS:ID.test/NAME`, made in the build
  directory by `tools/mktestdata` (binary data stay generated, not
  checked in).

Plan choice (the planning agent's, not the user's): these three first,
by the idea's Q5 ("Auch andere Werte könnten möglich sein") the action is a closed
list in `testconf` that a later change extends. Point: a needed action
is missing for a user — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor.

### D3 Where a data source is compiled

Plan choice: in the cell's own session, before the driver's compile. The
runner groups the data sources of a cell by their build configuration
(compiler, libraries, runtime errors); the configuration equal to the
cell's adds jobs to the cell's job list (the executable written straight
to `CORPUS:ID.test/NAME`), each other one is a compile part of its own
in the cell's script (its compiler and libraries staged in
`CORPUS:ID.test/.build-N/blitz2`, `Assign blitz2:` to it, its job list
with its own `runerr=`). No change of tedstub; `corpus.h` gets a key
`build=` for `cell=on` passes. Alternatives: a separate emulator session
per test before the cells (one more cold or warm start per run: about as
long as the test itself); a ctest fixture (a build of all children before
the tests: couples every test to it). Point: a second compiler in the
cell's volume costs staging time and space — L2 Possible / I1 Low / E2
Reasoned → Monitor (the `@stat` lines of the moved tests show it).

### D4 Original material and the sides

A fixed `compiler` other than `port`, or `libs = "cd"`, needs the CD; the
preset `all` starts no session of an original (`tests/README.md`, the
preset table: spec, step 1). So such a data file is refused in a test
with a cell of the port side (`testing/basic-test-runner`, "Data that
need original material"). With `same` (the default, enough for every
user of this change) the side of a cell is unchanged.

### D5 The query `data.FILE` and BROKEN

Plan choice: the compile of a data source is a query like `compile`,
default `== ok`; an unexpected result makes the cases of the cell BROKEN
(the setup failed, the tested behaviour was not reached), a declared one
lets the cases run without the file. Point: a child compile error is the
original's bug itself and BROKEN hides it — L1 Unlikely / I2 Medium / E2
Reasoned → Monitor (the deviation can declare it).

### D6 `!RunProgram`, its result and its output

The user's answers: Q3 "!RunProgram (Recommended)", Q4 "None
(Recommended)" (no timeout), and "!RunProgram gives the case rc and
output (NewType pointer)" (2026-11-23): "NewType pointer + file
(Recommended)" — `*r.TLRun = !RunProgram{cmd$,"name"}` returns the
address of a NewType of the test library with the return code and the
output as a string (cut, with a flag); the case checks them with the
check macros; the output file and the toml queries stay for a hang or
crash.

**The routine.** `TLRunProgram(K,command$)` (a function; the type is
named `TLRun`, so the routine cannot be: the tokeniser would make the
type name the library's token) writes `run K`, opens
`CORPUS:out/run-K.out` as the output, starts the command line (dos
`SystemTagList` from Kickstart 2.0, `Execute` on 1.x: the input `NIL:`
unless the line redirects it), closes the file, fills the `TLRun` from
the return code and the file, writes `runend K` with the return code
(`-` where the system gives none; measured in task 3.1) and gives the
address. The macro `!RunProgram{K,CMD$}` expands to that call, so it
stands in an expression; the preprocessor numbers it there too.

**The NewType** (declared in `macros.bas`, before the source):

```
NEWTYPE .TLRun
  rc.l        ; the return code, 0 if unknown
  rcknown.w   ; -1: the system gave it; 0: Kickstart 1.x Execute gave none
  cut.w       ; -1: printed holds only the first 1024 bytes
  size.l      ; the bytes the program wrote (the file's length)
  printed.s   ; the first 1024 bytes of the output, as they are (LF kept)
End NEWTYPE
```

Plan choices (the planning agent's): the names avoid tokens (`Len`,
`Output` exist as words of the language; a build check of `macros.bas`
against the token lists keeps them free); `rcknown` so that a 1.x run is
not taken as rc 0.

**N = 1024.** The block is static data of the library (the rule
"allocates nothing" holds for it): 1024 + 5 bytes for the string plus 12
for the fields. Every user's child prints less than 200 bytes (the
`.expect` files of D8's tests); 1024 is the bound of the entries kept in
Blitz mode and twice a result line (512), so a `!CheckStrEq` of the whole
value is written cut but judged by length and CRC32 over all of it.
Larger costs library memory on a 512 KB A500 (`multi-68010-vbr`) for no
user. Point: a later child prints more than 1024 bytes and its tail is the
observation — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor (`cut`,
`size` and the toml query `run.NS.NAME.output` see the whole file).

**Layout of `printed`.** A Blitz string is the address of its characters
with its length as a long at -4 and a 0 behind them (the library side:
`-4(a2)`, docs/libs/deviations.md, stringfunclib Cvi); the field holds the
address of the characters in the library's static block. Reading it
(`x$=*r\printed`, `!CheckStrEq{*r\printed,...}`) copies the string;
assigning to it would make the program free library memory, so the
fields are read-only (the help text and tests/README.md say so). Point:
1.95 or 2.10 read a string field of a NewType otherwise than by that
address — L2 Possible / I2 Medium / E1 Assumption → Plan: task 3.1 spikes
it with the three compilers first; if a compiler differs, the fallback
is a field `printed.l` (the address) and a function `TLRunText$(*r)`.

**A second call** overwrites the one block: the address of an earlier run
then shows the later one (Plan choice: one static block, no ring of
results; a case that needs both copies the first, `a$=*r\printed`,
`c=*r\rc`). Point: an author keeps two pointers and is surprised — L2
Possible / I1 Low / E2 Reasoned → Monitor (the help text and a test).

**The file and the queries stay.** The runner reads `run-K.out` from the
cell's volume, also when the program never returned: what the child
wrote before a hang or crash is kept. The queries `run.NS.NAME`
(`done|open|not-run`), `.rc` and `.output` judge what the case cannot
(the run that did not return, a crash); default `run.NS.NAME == done`
only — the return code is checked in BASIC, so it has no default (Plan
choice; a default `rc == 0` would make every case that expects 20 also
assert it in the toml). The output file and the start allocate: the
library's rule gets `TLRunProgram` as its exception (`testing/basic-testlib`,
MODIFIED).

The started program's requesters come in the program's own output
(reqtrap catches them system-wide, as in the old tests' `.expect`); a
case with a `!RunProgram` runs on its own (Plan choice: as a case with
`!ExpectError`, extension D2), so every whole-program observation of that
run is the case's (the idea's Q4).

Format version stays `1` (Plan choice): writer and reader are one codec
built from the same commit, no result file is committed.

### D7 Placeholder and the name length

`@TESTDIR@` is `CORPUS:ID.test` (the idea's Q3 "OK"). The longest user id gives
`palettelib-make4bitrgbs-lowbyte.test` (36 characters, over the 30 of an
OFS/FFS name). The cell volume is a host directory (UAE filesystem), and
the old form already compiled `CORPUS:src/palettelib-make4bitrgbs-lowbyte.bb2`
(the job line of its `.orig`; Kickstart 3.1). Point: a long name under
Kickstart 1.3 — L2 Possible / I1 Low / E2 Reasoned → Monitor; task 1.4
measures it on `a500` and the registration refuses an id too long for
the cell's machine if it fails there.

### D8 The users

| Test | Data | The case | In BASIC (`r`) | In the toml |
|---|---|---|---|---|
| `palettelib-make4bitrgbs-lowbyte` (#224) | `child.bas` (screen, `DecodePalette`, prints the colours) | sets exec's version to 37, `!RunProgram`, puts it back | `printed` = "exec 37", "cols 4,5,6 8,9,10 12,13,14"; the version back; `libs=cd`: `.actual` the `0,0,0` colours | — |
| `editlib-edit-eof` (#132) | `child.bas` (Edit$/Edit at the end of the input) | `!RunProgram{"@TESTDIR@/child <NIL:"}` | `printed` = "edit$ 0", "edit$dflt 0", "edit 0"; `rc` 0 | — |
| `rundebuglib-ctrlc-edit` (#43) | `child-s.bas`, `child-f.bas`; `input.txt` (`copy`) | two runs with `<@TESTDIR@/input.txt` | `rc` 20 and `printed` with "***Break" for each run (port); `libs=cd`: rc 0, "not stopped" | — |
| `rundebuglib-supertrap-d0-lost` (#246) | `child.bas` (SetInt with the runtime error) | own TRAP #0 handler around the run, the kept d0 checked in the case | `printed` = "child"; d0 | `requester` (the child's "Uninitialized Shape"); `libs=cd`: none, "child back" |
| `rundebuglib-two-programs` (#1069) | `a.bas`, `b.bas` | `Execute_` of `Run >NIL: @TESTDIR@/a`, then `!RunProgram` of `b` | `printed` of `b` = "b started", "b still running" | `requester` (Array Subscript); `libs=cd`: `end.case not exists` (the parent never returns) |
| `multi-68010-vbr` (#42) | `child.bas` (cr_vbr, SetErr, TRAP #0) | moves the VBR, `!RunProgram` with the table's address, VBR back | `printed` = "cr_vbr is the moved table -1", "SetErr handler reached"; `rcknown` may be 0 (Kickstart 1.3) | `libs=cd`: the child crashes: `run.NS.NAME == open`, `.output` "…table 0", the Software error requester, `session` |
| `rundebuglib-printerr-numeric-code` (#6) | `noend.bas` (a program without `End`) | new case `end_of_program_run` | — (the child ends in its requester) | `requester` "End of Program"; `libs=cd` pins the empty text `AutoRequest body ""` |

`dhandlerlib-no-cache-flush` (#409) is no user of this change (Q5).

## The plan's questions, answered

Asked by the planning agent with options; answered by the user on
2026-11-23 (`rounds/tr/decisions.md`, the two entries named in Context):

- Q1 the form of the options: "Table per file (Recommended)" (D1).
- Q2 the values of `compiler`: "Axis values + same (Recommended)" (D1,
  D4).
- Q3 the started program: "!RunProgram (Recommended)" (D6).
- Q4 a timeout of `!RunProgram`: "None (Recommended)": the session's hang
  check ends a hanging child, `run.NS.NAME == open`, its output kept (D6).
- Q5 `dhandlerlib-no-cache-flush`: "Own step before U4 (Recommended)":
  the runner addition for a program run from the editor with the
  debugger (a setting `defaultdbug`, a query on the inject lines of the
  compiler's run) is a small change of its own before round U4, designed
  with the needs of the U4 compiler tests; the test moves then; not in
  this change.
- Q6 the order: "In parallel with U3-U7 (Recommended)": implemented
  alongside the rounds; the moves land before task 4.1 of
  `bug-orig-basic-tests`.
- Q7 the requirement "Entries of the result file": "Split, own change
  (Recommended)": until that change, the entries `run` and `runend` are
  the ADDED requirement "Entries of a started program" of this change's
  delta of `testing/basic-test-interface`, beside the list of the
  existing requirement.

## Risks / Trade-offs

- [The return code under Kickstart 1.x is not available from `Execute`]
  → `runend K -`, the configuration asserts the output instead; task 3.1
  measures it (E1 until then: a short spike on `a500-68010`).
- [A child started in Blitz mode or under `Forbid`] → refused with
  `broken` (the library cannot start a process then).
- [Moved tests change their observation] → each move's report names a
  changed observation as U2 did (the analyst's check of
  `analysis-u2-observations.md`).

## Migration Plan

Additive: no key, entry or query of an existing test changes; the old
form of the users is removed in the commit that adds the model files
(as in the rounds).

## Findings

### Task 3.1: the spike (2026-11-23)

A model test outside the tree (the compilers `port`, `orig195`,
`orig210`; the libraries `cd` and `clib`, the originals only with `cd`;
the machines `a1200` and `a500-68010`: eight cells), run with
`corpusrun case` on both sides:

- **The string field.** A static block in the program's `Dc` data with
  the layout of D6 (the field `printed.s` holding the address of the
  characters, their length as a long at -4, a 0 behind them) is read
  alike by the three compilers with both library sets on both machines:
  `x$=*r\printed`, `!CheckStrEq{*r\printed,...}`, `Len(*r\printed)`,
  `Left$(*r\printed,3)` and `*r\printed+*r\printed` give the characters
  and the length; a field holding 0 reads as the empty string. The
  fallback (`printed.l` and `TLRunText$`) is not needed.
- **The syntax of a field of a pointer.** BB2 reads a field through a
  pointer variable as `*r\printed`: `*r.TLRun=p` followed by `r\size`
  is "Garbage at End of Line" (`r` alone is another variable). The texts
  of this change (D6, the tasks, the scenarios of the deltas) say
  `*r\FIELD`.
- **The names.** `TLRun`, `rc`, `rcknown`, `cut`, `size`, `printed`,
  `RunProgram` and `TLRunText` are no token of the build's seven token
  lists (blitz2 with the default sets and every variant) nor of
  AmiBlitz3 v3.14.0's list (`shared/basictest/check-macros.cmake` run on
  them as macro names), and the NewType compiles with the CD's libraries
  under 1.95 and 2.10.
- **The return code.** Kickstart 3.1 (exec 40): `SystemTagList` with
  `SYS_Input` `NIL:` and `SYS_Output` the file gives the return code of
  the command line (`Echo hello` LF `C:Quit 20`: 20). Kickstart 1.3
  (exec 34): `Execute(cmd,nil,file)` returns -1 and leaves the caller's
  `cli_ReturnCode` as it was (12345 before and after): the system gives
  no return code, so the `-` rule of D6 holds (`runend K -`, `rcknown`
  0, `rc` 0).
- **The output.** On both Kickstarts the file holds what the command
  line wrote and the shell's own line for a return code at or above the
  fail limit: `hello` LF `C:Quit failed returncode 20` LF (34 bytes). A
  child that fails thus leaves that line in `printed` too.

### Task 1.4: the name length (2026-11-23)

A model test outside the tree staged a file as
`CORPUS:palettelib-make4bitrgbs-lowbyte.test/x.txt` (the 36-character
directory of D7) and, on `a1200` (Kickstart 3.1) and `a500` (Kickstart
1.3), read it with dos `Open`/`Read`, copied `C:Type` into the directory
and ran it from there with the file as its argument (`Execute`): both
cells passed. The cell volume is a host directory of the UAE filesystem,
which takes the long name on both Kickstarts, so the registration needs
no refusal of a long id.

### Task 2.3: the prev comparison of data sources (2026-11-23)

- A data source is compiled where the driver is: in the pass of its
  setting of runtime error checking (the cell's, or the one its option
  fixes; a pass of the data alone with the settings of the first cell of
  the side port when no cell has that setting). The name of the compile
  is `ID/FILE`, with runtime error checking `ID/FILE-runerrs` (as the
  driver's `ID-runerrs`): the spec's `ID/FILE` names the compile without
  it.
- The PREV_INTENDED grammar (`tools/lib/previntended`) refused a `/` in
  the name of every test but `corpus_cd*`; it takes one now for the
  tests of the model (`blitz2_*_prev`, `bug_orig_*_prev`,
  `regression_*_prev`), so `TEST:ID/child.bas` is an entry.
- The corpus pass gets `source=PATH|NAME` (a source with a name of its
  own); its files in the volume are `nNNN` (a name `ID/FILE` would meet
  the driver's executable `out/ID`).

### Task 3.3: TLRunProgram (2026-11-23)

- **Registers.** The method of basic-testlib's register test (`m8`): a
  program takes `TLRunProgram`'s address from the `JSR` of a call that
  never runs and calls it from assembler with markers in `d2`-`d7`,
  `a3`, `a6`, the command's length stack in `a2` and the program's
  `a4`/`a5`: `d2`-`d7` and `a2`-`a6` are unchanged after the call, and
  the result it gives holds the command's output, with 1.95, 2.10 and
  the port, both library sets, on `a1200` and `a500`
  (`~/.local/share/bb2-c-work/accept-local-data/scratch/rp/rpregs.test.bas`,
  outside the tree as `m8` is).
- **A direct call** (`*r.TLRun=TLRunProgram(7,"Echo hello"+Chr$(10)+"C:Quit 20")`)
  gives on Kickstart 3.1 `rc` 20, `rcknown` -1, `size` 34 and `printed`
  `hello` LF `C:Quit failed returncode 20` LF; on 1.3 the same output
  with `rc` 0 and `rcknown` 0; a second call overwrites the block.
- The command line is cut to 512 characters; the output file is
  `CORPUS:out/run-K.out` (written new, then read back: the first 1024
  bytes into the block, its length by `Seek`); the input is `NIL:`
  unless the line redirects it.
