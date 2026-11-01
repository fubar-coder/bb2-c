# Design

## Context

See proposal.md (Why) and the plan `docs/ab3/variant.md` (decisions and
spike result of 2026-10-31). Relevant current state:

- `blitz2` (`programs/blitz2/src`, a C translation of `blitz2.s`) gets
  its source only from an editor: it loads `blitz2:ted` with `LoadSeg`
  and talks to it through `comdata` (lines already tokenised with the
  token list blitz2 built from its compiler tokens and the loaded
  libraries; services like `error`, `tokone`, `loadxtra`).
- `tests/corpus/amiga/tedstub.c` is a headless editor on the edstub
  frame (`shared/edstub`) that blitz2 loads as `blitz2:ted`; it compiles
  jobs, but only tokenised sources.
- TED's core (`programs/ted/src/core`: `tokit`, `fileopen2`/loadsave)
  tokenises text with a token list; `tedtok` runs it on the host.
- `shared/xtra` detects source types (`xtra_detect`) and reads
  AmiBlitz3's embedded header (`XTRA_T_AB3`); an m68k object already
  exists (`xtra_m68k`, `-fbaserel`, freestanding).
- Libraries: `loadlibs` (`asmlibs.c`) loads `acidlibs` and `deflibs`
  with `loadlibgroup`, then the selected `userlibs/DIR/` files;
  `loadres` (`stmts.c`) loads residents by the names in the `.xtra`.
  `mathq` is a constant (192) in `main.h`.
- The spike: AB3 v3.14.0's four BigLibs load unchanged with
  `loadlibgroup` (end mark present); all internal entries blitz2 calls
  exist in them; only `mathq` and resident paths break.

## Goals / Non-Goals

**Goals:** the smallest set of changes that lets `blitz2-ab3` compile
BB2-language AB3 sources from the command line with AB3's binary
libraries (spec `ab3-compat/*`), while `blitz2` stays byte-identical.

**Non-Goals:** AB3 language features (G2–G7: `SYNTAX`, `OPTIMIZE`,
`@@`, escapes, optional parameters, `_`-identifiers, `Data` constants,
FPU, 68020); AB3's preprocessor; the runtime debugger in AB3 mode;
TED changes; porting AB3's libraries; byte-equal executables with
AmiBlitz3's compiler (G8).

## Decisions

### D1: `blitz2-ab3` is a second target of the same sources
One CMake definition builds `blitz2` and `blitz2-ab3` from the same
chunk sources; `blitz2-ab3` adds `-DBLITZ2_AB3` and the extra objects.
Every AB3 addition in the chunk sources sits under `#if BLITZ2_AB3`.
*Why:* `blitz2` stays byte-identical by construction (the normal
acceptance proves it); one code base. *Alternatives:* a run-time mode
in the one `blitz2` (default no longer byte-identical, every commit a
named deviation), a fork of the sources (drift).

### D2: The command line front end is a headless editor, `ab3cli`
With `-s` on the command line, `blitz2-ab3` parses the arguments,
detects the mode (D4), loads the libraries of that mode (D5) and then
loads `PROGDIR:ab3cli` instead of `blitz2:ted` through the existing
editor load path, handing it the job (source, executable, flags). `ab3cli`
is built like `tedstub` on the edstub frame and compiled with TED's
options; it loads the source with TED's core (`fileopen2`: text is
tokenised with the token list blitz2 hands to every editor), reads the
embedded header with `shared/xtra`, gives blitz2 the options and
residents as a BB2 `.xtra` image through `loadxtra`, triggers CREATE
EXECUTABLE, and prints errors (file, line, message) on the console; it
sets the return code.
*Why:* `comdata` is the existing, tested seam; the tokeniser stays in
the editor world (TED's globals, `-fbaserel` with TED's a4) instead of
being linked into blitz2's world; `tedstub` already proves the batch
side. *Alternatives:* linking TED's core into blitz2 (two compilation
worlds in one binary, CLAUDE.md "Two compilation worlds"); a new
tokeniser in blitz2 (duplicates TED's, which matches the original for
every CD source).

*As built:* the job is a plain-data struct (`shared/ab3job/ab3job.h`:
magic, forced and decided mode, `-r`, source, executable, ab3cli's
`started`/`result`, and the `tokfile` entry of task 3.5) that
`blitz2-ab3` passes in a0 of the editor's main entry instead of the file
name; `ab3cli` writes its return code back into it and `main` returns
that (20 if ab3cli did not run). The command line is parsed in
`shared/ab3job/ab3args.c` (C99, freestanding, host test
`tools/tests/test_ab3args`), the mode decided and the job set up in
`programs/blitz2/src/ab3main.c` (only in blitz2-ab3); `main.c` has three
`#if BLITZ2_AB3` hooks (parse before `argc == 0`, editor name and a0 in
`begin`, the return code). ab3cli (`programs/ab3cli`) compiles the
edstub frame itself with TED's `-fbaserel` (the frame's entries now load
the program's a4 under `__baserel__`) and `EDSTUB_INIT` (a set-up hook
before the compiler loads its libraries; `edstub_quiet` keeps the
frame's log off the console). The options reach the compiler through
`loadxtra`, which reads a DOS file handle: the `.xtra` image of an
embedded header (or of `-r`) is written to a temporary file in `T:`
(else `RAM:`); a companion `.xtra` without `-r` is handed over as TED
does. Relative names are relative to the shell's current directory.
Not carried over from the header: its `Max NAME` lines (the Blitz 2
`.xtra` needs library numbers) – a G2+ item.

### D3: AB3 compiler tokens as a second static token chain
`main.c` gets a second compiler token chain under `#if BLITZ2_AB3`,
written by hand from AmiBlitz3's source (`AmiBlitz3.ab3` token table,
`AmiBlitz/AmiBlitz3@58a11658`): BB2's chain with AB3's differences
(`Chip` for `MOVEP`, `BSS` for `TAS`, `SYNTAX`/`EXTB` in the two `Jimi`
slots, the 8 new tokens $80D4–$80DB). In G1 the new tokens only need to
exist (tokenising, error "not supported" if compiled); their meaning
comes with G2–G7. The library tokens are appended as today.
*Why:* token numbers must match AB3's so that tokenised AB3 sources and
AB3's residents (macros with AB3 tokens) work. *Check:* the token list
test of `ab3-compat/libraries` against AB3's binary.

*Findings (task 4.1/4.4, 2026-10-31):*
- AB3's table is not in `AmiBlitz3.ab3` itself but in
  `Compiler/internal_commands.ab3` (included at `.internal_commands`).
  Besides the four renamed slots, AB3 also writes `RunErrsOn`/`RunErrsOff`
  where BB2 has `Runerrson`/`Runerrsoff` (same tokens $80D2/$80D3): six
  names differ, 218 = 210 + 8 tokens. AB3's asm data (`!a` entries) equals
  BB2's except `BRA`/`BSR` (destination shift 0 and size 0 instead of
  $FF/$FFFF, AB3's own assembler); blitz2-ab3 keeps BB2's data (it
  assembles as blitz2; 68020 forms are G7).
- To share the data instead of a second hand-written copy, the nodes
  are an X-macro list (`programs/blitz2/src/toklist.h`, `M(bb2, ab3)` for
  the six names, `TK3` for the new tokens); `main.c` expands it into
  `tokimg` (blitz2 byte-identical) and, in blitz2-ab3, into `tokimg_ab3`.
  `tokens` (main.h) is the image of `progmode`; `begin` hands it to the
  editor (`edstruct`), so `progmode` must be set before `begin` (D4:
  "before anything is loaded").
- The token check derives AB3's list from the fetched `Amiblitz3` binary:
  its token list is a linked node list like blitz2's (relocated `next`
  pointers), so `blitztokens` reads it unchanged; the library tokens are
  numbered by `blitztokens` for both sides (AB3's `process_libraries` is
  blitz2's `fixlibs`). `blitztokens -last 80DB` picks blitz2-ab3's AB3
  list (the binary holds both). Not compared with AB3's `-dump token`
  output in the emulator.
- "Not supported: NAME" is raised for the new tokens as a statement
  (`handletoken`) and in an expression (`function`); `KEEP` after `Dim`
  is parsed by `dodim` and gives its syntax error. `SYNTAX` and `EXTB`
  keep the handling of the `Jimi` slots (syntax error).

### D4: Mode detection in blitz2-ab3 with `shared/xtra`
`blitz2-ab3` links `xtra_m68k` and decides the mode before anything is
loaded: `-mode` if given; else `xtra_detect` on the source (`XTRA_T_AB3`
→ AB3; `XTRA_T_TED` → AB3 if its companion `.xtra` has AB3's 15-entry
format, else BB2; else BB2). For includes (`tokens1.c` include path)
the same rule is applied to a tokenised include; if its mode differs
from the program's, the compile stops with a new error "Include of
another mode: NAME". Text includes are tokenised by the editor's
`tokone` service, i.e. with the program's token list.
*Assumption (to verify in the first task):* text includes already go
through `tokone`; if blitz2's include path only takes tokenised files,
`ab3cli` tokenises the include on request instead.

*Finding (task 2.3, 2026-10-31): the assumption is wrong.* blitz2's
include path only takes tokenised files: `doinclude`/`doxinclude`
(`tokens1.c`, blitz2.s:12811/12760; `XInclude` is the once-only
`Include`) both end in `doinc2`, which opens the file and leaves it to
`readinc` (`libcall.c`, blitz2.s:11972): it reads the file raw in
`INCLEN` (1024) byte chunks and takes every `$00`-terminated piece as a
tokenised line; a chunk without `$00` becomes one "line" of up to 1023
bytes. Nothing on that path tokenises: `tokone` (comdata+56) is only
called by direct mode (`stmts.c`, blitz2.s `dodirect`) and the
calculator requester (`data.c`). A text include (LF lines, no `$00`) is
therefore compiled as raw text by blitz2 today (errors). `IncBin` reads
raw bytes, unrelated. Consequences, within the spec:
- The other-mode check (task 2.3) sits in `doinc2` under
  `#if BLITZ2_AB3` (`checkincmode`): the first `INCLEN` bytes are read
  (then the file position is set back) and given to `xtra_detect`; only
  a tokenised include (`XTRA_T_TED`, `XTRA_T_BB22`) gets `xtra_mode`
  with its companion `.xtra`, and the error "Include of another mode:
  NAME" if that is not `progmode`. A text include is not judged.
- Text includes take the second fallback: a new ab3cli service
  (`tokfile` in the job, called by `doinc2` through `callregs` only when
  `ab3job` is set) reads the whole include with TED's `readline` – the
  same way TED loads a source, so tokenised with the program's token
  list – and returns its lines; `readinc` reads them instead of the file
  (`ab3_incread`), `closeinc`/`freeincs` free them. `tokone` line by line
  was not used: it would skip TED's load rules (TAB expansion, line
  length, token pairs in text). In an editor session `readinc` stays
  exactly blitz2's, so the BB2 equivalence holds (ab3_bb2_equivalence
  passes).

*Finding (task 3.6, 2026-11-01):* blitz2 has no place of an error inside
an include: `linenumat` counts only the lines of the main source
(`nextline`), so `menuret` is the line of the `INCLUDE`. In blitz2-ab3 a
command line run counts the lines `readinc` takes of every open include
(`ab3_incopen` in `doinc2`, `ab3_incline` in `imakeend`, a table of 16
nested includes in `ab3main.c`, shared with the text includes of 3.5), and
`err` places the first error of the compile (`ab3_errpos`, before the
editor's error service): the innermost include with a line read - one
just opened (an error of opening it or of its mode) belongs to the
`INCLUDE` line outside. The place goes to ab3cli in the job (`errset`,
`errline`, `errfile`); ab3cli prints `INC:LINE: MESSAGE`, else the
source's line as before. Editor sessions: nothing changes (no job).
Limits: the lines of a text include are counted as TED loads them (a line
TED splits counts twice); a 17th nested include's errors are placed at
the 16th.

### D5: Library sets per mode
Under `#if BLITZ2_AB3`, `loadlibs` takes a list of group files per mode:
BB2 `blitz2:acidlibs`, `blitz2:deflibs`; AB3
`Blitz3:BlitzLibs/baseLibs`, `utilLibs`, `OsLibs`, `csLibs`, each
through the unchanged `loadlibgroup`. In AB3 mode the userlibs step
scans the flat `Blitz3:BlitzLibs/UserLibs/` and unlinks a loaded
library of the same number before linking the userlib (AmiBlitz3
`@7ab0350b`). `mathq` becomes a variable (192, or 44 in AB3 mode). In
`loadres`, AB3 mode prefixes a name without `:` or `/` with
`Blitz3:BlitzLibs/Residents/`.
*Why:* the spike showed no loader change is needed; the editor path
(BB2) keeps today's code.

*Findings (tasks 4.2/4.3, 2026-10-31):*
- `makelibs`/`loadlibs` run at start-up, before any compile, where there
  is nothing to stop yet (and `err` has no error context). A missing
  BigLib ("Library file not found: NAME", the first missing one) and a
  resident that cannot be loaded ("Can't Load Resident: NAME", the
  compiler's resident error with the name; `loadres` runs when the editor
  hands the `.xtra`, where blitz2 reports it at once through the editor
  and goes on) are therefore remembered in AB3 mode and reported by
  `compile` before the first pass: the compile stops with that error,
  through the editor's error service like any compile error. BB2 mode
  is unchanged (silent missing groups, immediate resident error).
- The userlib replacement follows AB3's `read_userlibsfile`: the first
  loaded library of the same number is unlinked; a BigLib's stays in its
  group buffer, an earlier userlib is freed. The flat directory is read
  in directory order (AB3 too), not sorted like BB2's `libslist`.
- `mathq` is a macro on `progmode` (`main.h`), not a variable.

*Finding (residents, 2026-11-01):* AB3's residents load unchanged; the
loader ends the list of a resident's constants (`fixconsts`, #34, as
AmiBlitz3's `CLR.l (a2) ;bugfix for object`) in both modes - AmiBlitz3's
`all.res` relies on it. Without it every compile's `freelabels` freed
memory behind the resident's hunk (exec alerts 8100 0005 / 0100 000F,
the "hang" of the command line run with `all.res`).

### D6: Return codes and `-r`
Return codes follow AmigaOS conventions: 0 compiled, 10 compile error,
20 argument/setup error (AmiBlitz3's own codes are not documented;
assumption, recorded here). `-r` clears runtime error checking and
debug information in the options handed to blitz2 (AmiBlitz3: "release
executable, size optimized"); further size optimisations are G8.

Errors the compiler reports outside a compile (library or resident
loading in BB2 mode) end the run with 20; in AB3 mode these come from
`compile` (group 4) and are printed like a compile error (`FILE: MESSAGE`
without a line when the compiler gives none) with 10. `-r` clears the
switches runtime errors, debugger and debug information of the image
(when the source has no options, of the compiler's own, read back
through `savextra`).

*Finding (task 5.4, 2026-11-01):* AmiBlitz3 v3.14.0 in CLI mode returns
0 also after a compile error (`exec_data.ab3`: "Can't convert types",
"A unknow error happend!", no executable). blitz2-ab3 keeps 10 for a
compile error; recorded in docs/ab3/variant.md ("Differences to
AmiBlitz3's compiler").

### D7: Tests
- Host: `xtra` tests extended for the AB3 `.xtra` format detection;
  argument parsing of `ab3cli`/`blitz2-ab3` as a C module in `tools/lib`
  style with a host test (valid, missing, unknown, unsupported options).
- Fetch: `cmake/fetch-originals.cmake` gets AmiBlitz3 v3.14.0's
  `BlitzLibs/{baseLibs,utilLibs,OsLibs,csLibs}`, `BlitzLibs/Residents/
  all.res`, the `Amiblitz3` binary and the test sources used, from
  GitHub raw at `58a11658` with SHA-256, into `archive/amiblitz3/`
  (ignored); tests skip (77) without them.
- Emulator: a suite `ab3` (label `emulator`): each AB3 test program of
  the spike's set is compiled with `blitz2-ab3 -s … -e …` and run; its
  output is compared with an expected file, and – configuration `ref` –
  with AmiBlitz3 v3.14.0's own compiler in CLI mode on the same
  machine. Error cases (other-mode include, missing library, missing
  resident, unsupported option) have their own tests.
- Token list: `blitztokens` on AB3's BigLibs with `blitz2-ab3`'s AB3
  compiler chain vs. the list dumped by AmiBlitz3 (`-dump token`); a
  deliberately changed list must fail.
- Equivalence: the corpus and bug-orig sources compiled with
  `blitz2-ab3` (through `tedstub`, i.e. BB2 mode) give byte-identical
  executables to `blitz2`'s (a ctest comparing the two output trees).

*As built (tasks 5.1-5.4, 2026-11-01):*
- `tests/ab3/cli.cmake` (control as CMake): one emulator session per
  configuration; `TEST:` gets `blitz2:` (blitz2-ab3, ab3cli, BB2's
  libraries of the build), three `Blitz3:` trees made from the fetched
  files (full; without `csLibs`; with the test userlib
  `tests/ab3/amiga/userlib.c`, library 152 = baseLibs' freqlib, in
  `UserLibs/`; each with the `Amiblitz3` binary), `TEST:src` with
  AmiBlitz3's test programs (memoryleak_test's `OPTIMIZE` line removed
  when the session is set up) and the cases' sources of
  `tests/ab3/cli/` (tokenised includes made in the build with `tedtok`,
  the AB3-format `.xtra` with `mktestdata`, `cli/ab3xtra.spec`). Every
  case: `Assign Blitz3:` to its tree, compile, run the program through
  `reqtrap` (a requester is printed and answered, nothing hangs), output
  file `[compile]` (console output, `rc=`) + `[run]` (output, `rc=` or
  "no executable").
- The comparison is a host tool, `tools/outmatch` (lib + tool + host
  test): an expect file of literal lines and `~PATTERN`/`+PATTERN` lines
  (`*` any characters, `#` a decimal number) for the values that depend
  on the machine or on timing (`AvailMem_`, `SetInt` counts, exec's
  version, the hardfile's language); sections, so that `ref` compares
  only `[run]`.
- `ref` (task 5.4) needs no setup beyond the fetched files: AmiBlitz3's
  binary runs from `Blitz3:Amiblitz3` with `Blitz3:BlitzLibs/` on the
  A1200 configuration (it only reports the missing
  `System/token_mapping_*.txt` and `Defaultdbug`, not needed for
  compiling). Where AmiBlitz3 differs, the case has `NAME.ref.expect`
  (its actual result) and the difference is in docs/ab3/variant.md as a
  G2+ item: `exec_data` (AmiBlitz3 refuses `execbase`, TED tokenises it
  as the library function `ExecBase`), AmiBlitz3's return code 0 after a
  compile error, includes (AmiBlitz3 tokenises an include by its name:
  `.ab3`/`.ab2`; `.bb`/`.bb2` are read as tokenised; blitz2-ab3 by its
  contents), a program without `End` with runtime error checking
  (blitz2's empty requester, BUG(orig) #6).
- Broken input: `ab3_cli_broken` and `ab3_cli_ref_broken` run atoi
  against `cli/broken.expect` (one digit changed) and must report that
  line.

## Risks / Trade-offs

- [blitz2's globals must stay below 64 KB with `-fbaserel`; the second
  token chain and new code add data] → measure in the first build task;
  if needed `blitz2-ab3` is built with `BLITZ2_BASEREL=OFF` (it is not
  the byte-identical target).
- [AmiBlitz3's tokenising rules differ from TED's in details
  (preprocessor, `@@`, case of keywords)] → G1 accepts TED's rules for
  BB2-language sources; differences found by the test set become G2–G5
  items.
- [Names of AB3 commands become tokens inside identifiers, as with TED
  (spike: `show{`, `sub1`)] → that is also AmiBlitz3's behaviour for
  tokenised sources; the test set uses AB3's own sources.
- [AmiBlitz3's compiler in Amiberry may need its own setup (assigns,
  libraries)] → the `ref` configuration is optional at first: the
  expected files are the acceptance, the `ref` comparison is added when
  it runs.
- [The editor path of `blitz2-ab3` must stay exactly `blitz2`'s] →
  equivalence test over corpus and bug-orig.

## Migration Plan

New target and files only; `blitz2` and every existing output are
unchanged. Rollback is removing the target.

## Open Questions

- The exact AB3 `.xtra` 15-entry format details for detection
  (formats.md §1.3 has the layout; whether older AB3 `.xtra` files must
  also count as AB3 can be decided when such a file shows up).
