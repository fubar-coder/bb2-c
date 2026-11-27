# Design

## Context

See `proposal.md` for the survey and the scope. State of the code:

- AB3 mode lives in `programs/blitz2/src/ab3*.c` behind the extension
  points of `ext.h`; BB2 mode is `ext_bb2.c`. `blitz2` and `blitz2-ab3`
  share one core.
- Type `.d` is refused with "Type Not Found" (`notypeerr`, `parse.c`),
  also without the FPU bit. The FPU bit gives "Not supported: OPTIMIZE 2
  (FPU)" (`ab3dir.c`).
- The inline assembler is table driven (`asmlibs.c`: an entry's size
  nibbles, a check function per entry); a 68020 size gives
  `illsizeerr` ("Illegal Assembler Instruction Size").
- The #940 rule is in `tokens2.c` (`doend3`, `stateret`, `funcret`,
  commit `712dda50`); it is core behaviour of both modes, recorded in
  `docs/blitz2/deviations.md` (kind F), the bug-orig tests
  `compiler-procend-kind*`, the CD table rows of `DARTS_POLY`/`DARTS_WIRE`
  and the two site pages (`compared-with-original/compiler.md`, `compared-with-original/extensions.md`). No
  OpenSpec spec states it.
- Named warnings exist (`compiler/warnings`, `W_RESCLASH`); `blitz2-ab3`
  prints them on the console and returns 5 after one.
- `corpus_cd` runs through `corpusrun cd` (`tests/corpus/runner`), a pass
  file and a committed table (`tests/data/blitz2-cd.tsv`), label
  `emulator`, limit 3600 s plus queue wait; `ab3_cli`/`ab3_cli_ref` run
  cases through `tests/ab3/cli.cmake`, the latter against AmiBlitz3's own
  compiler (label `oracle`).
- The survey's measurements (`analyse-ab3-state*.md/.tsv` in the
  coordinator's work directory) used one `Blitz3:` tree: the four BigLibs,
  `BlitzLibs/Residents`, an empty `UserLibs`, `Sourcecodes/Includes`,
  `Sourcecodes/Data`.

## Goals / Non-Goals

**Goals:**
- A test that fixes the measurement, so every later task shows its gain as
  a diff of one table.
- Close the gaps in the order of the decisions of 2026-11-16, each one
  verified against AmiBlitz3 v3.14.0 before it is coded.

**Non-Goals:**
- Anything of G6 (FPU code, FFP stop-gap under the FPU bit), library 38,
  porting AmiBlitz3's libraries, 68020 code generation, G8.
- Fixing the sources AmiBlitz3 itself cannot compile.

## Decisions

### D1. ab3_examples: tree, runner, table, preset

- **Fetch.** `fetch-originals.cmake` gets the sources as one GitHub
  archive of `58a11658` (one download, one SHA-256) and extracts only
  `Sourcecodes/Examples`, `Templates`, `Amiblitz3/Tests`, `Includes`,
  `Data` and `BlitzLibs/Residents` into
  `archive/amiblitz3/github-58a11658/`, where the script puts the single
  files it fetches today. Alternative: a per-file list with a hash each,
  as today; about 450 files, so rejected unless the archive turns out too
  big (task 1.1 decides after measuring). The BigLibs it already fetches
  stay.
- **Runner.** The pass reuses `corpusrun`'s emulator session machinery
  (workers, timeouts, parallel runs) as `corpus_cd` does. Task 1.2 reads
  `cmd_cd.cpp` and decides between a new pass keyword of `corpusrun cd`
  (trees, assigns, no CD) and a new command `ab3`; the result line per
  source is blitz2-ab3's console output cut to "ok" or the first error
  line.
- **Wrappers.** One wrapper per top-level include as in
  `docs/ab3/variant.md:963-968` (`Residents = "all.res"`, `OPTIMIZE 4`,
  `XINCLUDE`, `End`), generated in the work directory. `-r` is passed
  where the source's header says `RuntimeDebug = 0`. Names with spaces
  become `_`.
- **Table.** `tests/data/ab3-examples.tsv`: id, group, source, blitz2-ab3
  result, first message, and the AmiBlitz3 v3.14.0 column (recorded once
  from the survey file `analyse-ab3-state-results.tsv`, later by an
  `originals` target). `ab3-examples-update` writes the first columns.
  The table starts as the survey's measurement; the first run must equal
  it (acceptance of task 1), otherwise the difference is a finding and is
  not silently taken over.
- **Preset and label: `emulator`, in `all`.** The survey run of 437
  compiles took about 3 minutes; `corpus_cd` is longer and is in `all`.
  The test is skipped (77) without the sources or Amiberry. If task 1.3
  measures more than 10 minutes inside `ctest --preset all`, it moves to
  a label outside `all` and the coordinator decides; the proposal states
  the plan, not a measured value.
- **Broken input.** The comparison (a C function in `tools/lib` with a
  host test) is checked with tables that have a changed, an extra and a
  missing row (no emulator).

### D2. .d without the FPU bit

AmiBlitz3 without the FPU bit treats `.d` as an FFP float in an 8-byte slot
(probe of the survey: `SizeOf` 10 for `.d` and 6 for `.f`, same output; the
executables differ in 15 bytes, all sizes and offsets). The port does the
same: a type entry `.d` with size 8 whose arithmetic path is `.f`'s.

Open details are verified first (task 2.1, run-output probes in `tests/ab3/model/`):

| Detail | How it is verified |
|---|---|
| which 4 of the 8 bytes hold the FFP value | a NewType with `.d`, a `Peek.l` of each half after an assignment |
| `.d` as the 7th to 10th parameter | the G4 stack slot layout: the size of a `.d` slot |
| `.d` in residents and NewTypes | `sdl.res.ab3:174`, a `.res` with a `.d` field loaded |
| `SizeOf` | NewType with `.d`, array of `.d`, pointer arithmetic |
| library arguments of type 8 | a library command whose argument type is 8 (AmiBlitz3 `Docs/Compiler-Spec/09-blitzlibs.md:186`) called with `.d` and `.f` |
| conversions and `Function.d` | `.d` to `.l`/`.f`/`.s`, return value registers |

If a probe contradicts "same as `.f`", the spec scenario is changed to
AmiBlitz3's measured behaviour and the difference is named before coding.
With the FPU bit set the `OPTIMIZE` error stays; nothing changes there.

BB2 mode must not gain `.d`: it stays "Type Not Found"
(`ab3-compat/bb2-equivalence`).

### D3. 68020 integer instructions in the assembler

- Tables only. The set is the 68020's integer additions to the 68000:
  long-word `MULU`/`MULS`/`DIVU`/`DIVS` (`.l`), `DIVUL`/`DIVSL`, `EXTB.l`,
  the scaled index and base displacement modes `(d8,An,Xn.size*scale)`,
  and whatever else AmiBlitz3's assembler tables hold. Task 3.1 reads that
  assembler in AmiBlitz3's compiler sources at `58a11658` and closes the
  list; the closed list is written back to the spec. FPU, MMU, 68030+ and
  coprocessor instructions stay out.
- The gate. The survey shows that `convert.include.ab3:609` compiles in
  AmiBlitz3 under a wrapper with `OPTIMIZE 4` (bit 1 clear), so as far as
  measured the instructions are not gated by the 68020 bit. Task 3.1
  confirms it with `tests/ab3/model/` cases (`OPTIMIZE 4`, `OPTIMIZE 5`, no
  `OPTIMIZE`) and copies AmiBlitz3's rule, whatever it is.
- `asmlibs.c` is shared by both modes: the new entries are reachable only
  in AB3 mode (a mode check where the size nibbles are read), so BB2 mode
  keeps its error and its tables' results.
- The tests compare the emitted words with AmiBlitz3's: a `tests/ab3/model/`
  case assembles each instruction and prints the bytes.

### D4. The #940 relaxation

- **Rule.** In `doend3` (and what it calls) an `End Statement` met while a
  numeric `Function` is open is treated as `End Function`: the same
  epilogue, the same default return value, plus the warning `procend`. The
  decision is made on the open procedure's type: byte, word, long, quick,
  float (double in AB3 mode) are relaxed; string and NewType are refused as
  today. `End Function` in a `Statement` and the `Return` mismatches stay
  errors (`712dda50`).
- **Default return value.** What `End Function` returns today is read from
  the code in task 4.1; the spec's "default return value" is named by value
  there. The claim "same as `End Function`" is tested by comparing two
  executables that differ only in the keyword.
- **Both modes.** The rule is core code, not an extension point. This
  changes BB2 mode: it is an intended change of the #940 fix and the only
  allowed BB2 result change of this change. It owns the following work,
  all in task 4:
  - `corpus_cd`: the rows of `DARTS_POLY.BB` and `DARTS_WIRE.BB`
    (`BALLBLITZ.BB2` if its error moves) change from refused to compiles;
    `-DPREV_INTENDED` names the change and `blitz2-cd-update` rewrites the
    table;
  - the bug-orig tests `compiler-procend-kind*` that expect "Illegal End
    Procedure" for a numeric Function get the new expectation; one test
    keeps the string Function error;
  - `docs/blitz2/deviations.md` (row #940, kind F) is reworded; the draft of
    the issue comment for the coordinator names the narrowing of the fix;
  - the site pages `compared-with-original/extensions.md` ("Forms the
    original refused") and `compared-with-original/compiler.md` (the #940
    entry) need an update: the CD
    example `gendart` compiles again, with a warning; only the string case
    and the other mismatches remain errors. The tasks draft the text; the
    coordinator owns the pages.
- **Owner of the rule:** the new spec `compiler/procedure-end` (both
  modes) and the warning in `compiler/warnings`. The `ab3-compat` specs
  need no requirement of their own; AB3 sources like `image.include` are
  covered by `testing/ab3-examples` and its table.
- **Rejected alternative:** a relaxation in AB3 mode only. The decision
  of 2026-11-16 is both modes (the CD's `DARTS_POLY`/`DARTS_WIRE`).

### D5. The small items

Each item starts with a verification task: a probe (a model case for run output or a CLI case for include lookup and compiler diagnostics)
fixes AmiBlitz3's behaviour, the spec text that says "as AmiBlitz3" is
completed with it, then the code follows. Sources that use them (survey):
`USELASTPATH` (the `image.include` family, 27 programs), `USEPATH2` (1
example), `Dim KEEP` (1), `Data.w #c` (1 test program), `Object`
(`winuaeDLL`), casts `(.type)` (2 programs, 4 in total). An item whose
probe shows more than the survey guessed (casts: "small to medium") is
split off as its own task with its own estimate before it is coded; if it
is large it moves to a later change and its spec requirement is removed
from this one.

### D6. Order and acceptance

The order is the decisions' order: 1 table, 2 `.d`, 3 assembler, 4 #940,
5 small items, 6 site. After each task the table is updated; its diff is
the measure of that task. Targeted tests run in the task; the coordinator
runs `ctest --preset all`, `bug-orig` and `oracle` after the merge.
BB2-mode results are compared with the `prev` build; only task 4 passes
`-DPREV_INTENDED`.

## Risks / Trade-offs

- [The table breaks on a toolchain or AmiBlitz3 pin change] → the table
  names the pinned commit in its header; a bump regenerates it.
- [The fetched archive is large] → extract by pattern; measure in task
  1.1; fall back to per-file hashes.
- [`.d` probes show it is not plain FFP in some context (parameters,
  library calls)] → the spec scenarios follow the measured behaviour;
  where the port cannot match it cheaply the item shrinks and the table
  shows which sources stay.
- [Accepting `.d` lets programs get further and fail at the FPU bit] →
  intended; the first message of such programs moves and the table
  records it.
- [The #940 relaxation hides a real mistake in BB2 sources] → the
  warning; the warning-as-error setting turns it into an error again.
- [68020 instructions in assembler give executables that need a 68020] →
  as AmiBlitz3's; named on the site page on assembler.
- [A 3-minute emulator test in `all`] → decision D1, with its fallback.

## Migration Plan

No data migration. Each task is its own commit (code, tests, table
changes, docs). The #940 task changes BB2 results and is named so in its
commit. Rollback is a revert of the task's commit.

## Open Questions

- Whether the `originals` target that records AmiBlitz3's column of the
  table is built in task 1 or later; it changes no spec.
- The exact text of the `procend` warning; the spec fixes the name and
  the meaning.
