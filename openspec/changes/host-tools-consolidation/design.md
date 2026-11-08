# Design

## Context

State of the host programs (all built by the native compiler in the host
project, `host/CMakeLists.txt`; the top-level project names their paths in
variables, `CMakeLists.txt:70-97`, and lists them as `BUILD_BYPRODUCTS` of
the ExternalProject `host_tools`, `CMakeLists.txt:109`):

- 25 programs in `tools/NAME/` (`tools/CMakeLists.txt:8-34`), each
  `main.c` (40-500 lines) around `tools/lib/NAME/`; their library tests are
  `tools/tests/test_NAME.c` (inputs built in memory), their tests on files
  are `add_test` calls and `*.cmake` scripts in `tools/NAME/`
  (`tooltest.cmake`, `check.cmake`, ...).
- `tedtok`, `teddetok` in `programs/ted/tools/`, linked with the TED core
  (`tedhost`, `tedcore_host`; `programs/ted/tools/tedtok/CMakeLists.txt:3`).
  The TED core has global state, hence one program per host test
  (`CLAUDE.md`); `host/CMakeLists.txt:39-43` adds them after the TED host
  tests.
- `corpusrun`, `cdrun` in `tests/corpus/runner/` (both link `lib_corpus`;
  `cdrun` also `lib_cdcache`, `lib_sha256`), `tedab`, `teddump` in
  `tests/ted-ab/runner/` (`tedab` links `lib_tedab` which links
  `lib_teddump`); all four use `lib_runner_util`, three of them
  `lib_amiga_emulation`.
- Argument styles today: subcommand first (`biglib join|extract|check`,
  `cdtable update|reference`, `hdftool info|list|cat|check|edit`,
  `xtra dump|check|convert`, `teddump info|text|cmp`); subcommand after the
  first file (`vasmlisting LISTING map|labels|verify EXE`,
  `tools/vasmlisting/main.c:53`); mode by flag (`hunkcheck --hunks|--stored`,
  `tools/hunkcheck/main.c:33,42`; `ab3tok -cmp`, `main.c:193`;
  `bb22 --info`, `main.c:59`; `blitzres --dump|--source`, `main.c:161-163`;
  `amiga_run --purge-snapshots`, `main.c:146`; `tedab --lint`,
  `main_tedab.c:1244`); mode by argument shape (`teddetok TOKENS --check
  IN...`, `tedtok --blitz2 ...`).
- Exit codes are not uniform: an unreadable input is 1 in `hunkcheck`
  (`main.c:57`), `cmdref`, `vasmlisting` (`main.c:59`), 2 in `outmatch`,
  `cdtable`, `ab3tok`; 77 means "skipped" in `prevcmp`
  (`main.c:64`), the runners and `blitzlibcompare`. Messages are
  `tool: message` or `tool: FILE: message`.
- Callers are `CMakeLists.txt`/`*.cmake` files only (nothing is installed:
  `CMakeLists.txt:189-191`), plus the docs. Two scripts name tool paths
  literally: `cmake/coverage-map.cmake:29`, `cmake/coverage-select.cmake:30`
  (and `cmake/setup-amiga-forever.cmake:12,47`); the coverage selection has
  path patterns per tool dir (`tools/covsel/rules.txt:52-62`).
- `-h`: `cmake/check-help.cmake` runs `PROGRAM --help` and `-h` and expects
  exit 0, `^usage: ` on stdout, nothing on stderr; one `help_NAME` test per
  program from the list in `host/CMakeLists.txt:50-54`.
- The acceptance rule and the `prev` build (`-DBLITZ2_PREV=ON`,
  `tests/README.md` "The prev build"): `prev_results` runs
  `tools/prevcmp CUR PREV` over the library files; the corpus and `ted_ab`
  have `prev` configurations. The `prev` build builds the previous commit
  as a second project from `git archive` (`cmake/prev.cmake:11-13`), with
  that commit's own CMake files, so a rework of the tools in the current
  commit does not disturb it.

See `proposal.md` for the motivation.

## Goals / Non-Goals

**Goals:**

- Fewer programs (31 to 14), grouped by what they work on, one invocation
  syntax, `-h` per subcommand, one README per program.
- No change of any build result or test verdict; each group is one
  reviewable commit that can be checked against the previous commit.
- Documents link to the READMEs instead of describing arguments.

**Non-Goals:**

- No change of tool logic, file formats, library code or test content. A
  bug found while reading a tool is an issue, not part of this change.
- No change of the Amiga-side programs (`blitz2`, `ted`, test helpers such
  as `reqtrap`): they are not host programs, and `blitz2`/`ted` reproduce
  the originals (`TODO.md`, "-h / --help for all callable programs").
- No harmonisation of the exit status of existing inputs (D4).
- Not merging the libraries in `tools/lib/`: they stay as they are (the
  reuse between tools goes through the libraries already, for example
  `lib_hunk` in `lib_asmcmp`, `lib_biglib`, `lib_blitzlib`,
  `lib_checkdata`, `lib_hunkcheck`, `lib_vasmlisting`).

## Decisions

### D1. The grouping

Each assignment was checked in `main.c`, `tools/lib/NAME/CMakeLists.txt`
(the libraries a tool links) and the call sites. Counts are in D10.

| Program | Subcommand | Was | Input | Output | Called by |
|---|---|---|---|---|---|
| `blitzlib` | `join` | `biglib join` | library files | BigLib file | `cmake/BlitzLibrary.cmake`, `libs/` |
| | `extract` | `biglib extract` | BigLib file | library file | `libs/tests` |
| | `numbers` | `biglib check` | library files | status (no library number twice) | `cmake/BlitzLibrary.cmake` |
| | `check` | `blitzlibcheck` | library files | status | `cmake/BlitzLibrary.cmake` (link step) |
| | `compare` | `blitzlibcompare` | two library/BigLib files, intended list | differences | `libs/CMakeLists.txt`, `libs/userlibs/rwe` |
| | `cmdref` | `cmdref` | library file | JSON | `cmake/BlitzLibrary.cmake`, `libs/CMakeLists.txt` |
| `bbsrc` | `tokens` | `blitztokens` | compiler, libraries | token list | `libs/tests`, `tests/ab3`, `tests/bug-orig` |
| | `detok` | `bbdetok` | tokenised source, token lists | text | `tests/ab3`, tests of the tool |
| | `comment` | `bblines --comment` | tokenised source | tokenised source | `libs/userlibs/rwe/refbuild.cmake` |
| | `bb22` | `bb22 IN OUT` | BB22 source | TED-format source | `libs/userlibs/rwe` |
| | `bb22-info` | `bb22 --info` | BB22 source | text | tests of the tool |
| | `ab3` | `ab3tok` | text source, token list | tokenised source (AmiBlitz3's form) | `tests/ab3` |
| | `ab3-cmp` | `ab3tok -cmp` | the same, a second file | status, first differing line | tests of the tool |
| `tedsrc` (in `programs/ted/tools/tedsrc/`) | `tok` | `tedtok` | text source, token list | tokenised source (TED's) | corpus, `tests/*`, `libs/userlibs/rwe` |
| | `detok` | `teddetok` | tokenised source | text (as TED shows it); `--check` | `libs/userlibs/rwe`, `tools/bb22` tests |
| `refcheck` | `asm` | `asmcmp` | objdump, original BigLib, linked library | differences | `libs/acidlibs/CMakeLists.txt` |
| | `labels` | `checklabels` | `ted.s`, C sources | status | `programs/ted/CMakeLists.txt` |
| | `data` | `checkdata` | table, listing, reference, C build | status | `programs/ted/CMakeLists.txt` |
| | `listing-map`, `listing-labels`, `listing-verify` | `vasmlisting LISTING map\|labels\|verify EXE` | vasm listing (and a load file) | TSV, status | `programs/ted`, `programs/blitz2` |
| `hunkcheck` | `start`, `layout`, `stored` | `hunkcheck FILE OFF:HEX`, `--hunks`, `--stored` | load file | status | `programs/ted`, `programs/ab3cli`, `tests/ab3`, `tests/corpus/amiga` |
| `results` | `match` | `outmatch` | expect file, output | status | `tests/ab3`, `tests/bug-orig` |
| | `prev` | `prevcmp` | two build directories | status (77: no prev build) | `CMakeLists.txt` (`prev_results`) |
| | `cd-update`, `cd-reference` | `cdtable update\|reference` | run, table | table | `tests/corpus/CMakeLists.txt` |
| `testinfo` | `stats` | `teststats` | JUnit files | tables | by hand |
| | `map` | `covmap` | coverage inputs | lcov files | `cmake/coverage-map.cmake` |
| | `select` | `covsel` | coverage map, diff | list of tests | `cmake/coverage-select.cmake` |
| `corpusrun` (in `tests/corpus/runner/`) | `pass`, `cd` | `corpusrun PASS`, `cdrun PASS` | pass file | report (emulator) | `tests/corpus`, `tests/ab3`, `tests/blitz2`, `tests/bug-orig`, `libs/userlibs/rwe` |
| `tedab` (in `tests/ted-ab/runner/`) | `run`, `lint`, `info`, `text`, `cmp` | `tedab PASS`, `tedab --lint PASS`, `teddump info\|text\|cmp` | pass file; `.scr` dumps | report; text; status | `tests/ted-ab`, `tests/bug-orig` |
| `amiga_run`, `mktestdata`, `blitzres`, `xtra`, `hdftool` | standalone | | | | see the deviations below |

Subcommand names are fixed here so that the READMEs and call sites can be
written without a second round; an implementer may rename one for a reason
recorded in the group's commit message.

**Deviations from the proposed list and why:**

- `hunkcheck` is not in `blitzlib`. Its input is a load file of a program
  (`ted`, `ab3cli`, the corpus helpers; `programs/ted/CMakeLists.txt`,
  `programs/ab3cli/CMakeLists.txt`), not a Blitz library file. It stays a
  program of its own; it does not join `refcheck` either because it
  compares nothing with the original (see open question 1). Its three
  invocation forms (bytes at an offset, hunk layout, stored size) become
  subcommands `start`, `layout`, `stored`; today they are selected by flag
  or argument count.
- `tedtok` and `teddetok` are not in `bbsrc` but in `tedsrc`, in
  `programs/ted/tools/`. They link the TED core (`tedhost`,
  `tedcore_host`) with its global state, which `bbsrc`'s tools do not
  need; merging would make five unrelated tools depend on the TED core
  and its host library order (`host/CMakeLists.txt:37-43` adds the TED
  tools only after the TED host tests) and pull TED's globals into
  `ab3tok`. The theme "sources and tokens" is kept by the README of both
  linking to each other (`bbdetok --ted` is the nearest equivalent of
  `teddetok`).
- `vasmlisting`'s three views are three subcommands of `refcheck`
  (`listing-map`, `listing-labels`, `listing-verify`): its library is
  linked by `lib_checkdata` and its callers are the same
  `programs/ted/CMakeLists.txt` that call `checkdata`.
- `outmatch`, `prevcmp`, `cdtable` form `results` (a result against an
  expectation or the previous result); `teststats` is not in it. It reads
  records of test runs (JUnit, the `@stat` lines, `lib_statline`), like
  `covmap`/`covsel` read coverage of test runs. `covmap` and `covsel`
  share `covutil` (`tools/lib/covmap/covutil.c`) and are always run in
  sequence by `cmake/coverage-*.cmake`. The group is called `testinfo`
  instead of `coverage`.
- `corpusrun`/`cdrun` become one program `corpusrun` with the subcommands
  `pass` and `cd`: the exit codes and the pass-file idea are the same
  (0/1/77/2, `main.c:5-9`), both link `lib_corpus`, `cdrun.c` is a second
  `main` over it. The two pass-file formats stay different (`cdrun.c` has
  its own `parse`). `cdrun.c`'s `main` becomes the subcommand function.
- `tedab` and `teddump` become one program `tedab`: `teddump` is the by-hand
  view of the screen dumps `tedab` evaluates (`lib_tedab` links
  `lib_teddump`); `teddump info|text|cmp` become `tedab info|text|cmp`, the
  runs `tedab run` and `tedab lint` (`--lint` is a mode, not an option).
  `TED_FONTS_DIR` (compile definition of `teddump` today,
  `tests/ted-ab/runner/CMakeLists.txt:17`) moves to the program.
- Considered and rejected: one `testrun` for corpus, CD and TED A/B runs
  (three different pass-file formats, and `tedab` is called only by
  `tests/ted-ab` and `tests/bug-orig`); `blitzres` into `blitzlib`
  (residents are inputs of the compiler, not library files; its callers
  are the RWE reference build); `xtra` into `bbsrc` (its own subcommands
  and modes already, large, and its name in the docs is the file extension
  `.xtra` more often than the tool, so the rename costs the most for no
  gain).
- Standalone, kept as programs: `amiga_run` (one mode with options: it runs
  a script; the maintenance mode `--purge-snapshots` stays an option, see
  open question 2), `mktestdata` (one mode), `blitzres` (becomes
  `create|dump|source`, its three modes today are `-o OUT ITEM...`,
  `--dump`, `--source`), `xtra` and `hdftool` (already subcommand-first;
  README only).

Result: `blitzlib`, `bbsrc`, `tedsrc`, `refcheck`, `hunkcheck`, `results`,
`testinfo`, `corpusrun`, `tedab`, `amiga_run`, `mktestdata`, `blitzres`,
`xtra`, `hdftool` (14 programs).

### D2. Subcommand syntax

`PROGRAM SUB [OPTIONS] ARGS`, subcommand first, kept from `biglib`,
`cdtable`, `hdftool`, `xtra`, `teddump`. The five other styles above
become this. Names are lower-case with `-` between words. A program with
only one mode keeps `PROGRAM [OPTIONS] ARGS` (`amiga_run`, `mktestdata`):
a leading `run` would change 25 and 13 call sites for nothing. Options
stay `--name`/`-x` flags of their subcommand (`--tsv`, `--strict`,
`--section NAME`, `-o OUT`); the mode flags listed above are the only ones
that become subcommands. `-v`/`-q` of `checklabels` and `checkdata` stay
options of `labels` and `data`.

### D3. Dispatch library

`tools/lib/subcmd/` (`subcmd.h`, `subcmd.c`, library `lib_subcmd`):

```c
struct subcmd {
    const char *name;                   /* "compare" */
    int (*run)(int argc, char **argv);  /* argv[0] = "PROGRAM SUB" for messages */
    const char *usage;                  /* the synopsis line(s) and a one-line description */
};
int subcmd_main(const char *program, const struct subcmd *table, size_t n,
                int argc, char **argv);
```

`subcmd_main` implements: no argument or unknown name: the program usage
(the usages of the table, in order) on stderr, return 2; `-h`/`--help` as
the only argument: the program usage on stdout, return 0; `SUB -h|--help`
as the only argument after the name: that subcommand's usage on stdout,
return 0; otherwise `run(argc - 1, argv + 1)` with `argv[0]` replaced by
the string `"PROGRAM SUB"`, so the existing `fprintf(stderr, "tool: ...")`
calls get their new prefix by using `argv[0]`. The former `main.c` bodies
become `int cmd_NAME(int argc, char **argv)` in `cmd_NAME.c` (git follows the
move of `main.c` to `cmd_NAME.c` with a similarity above 80%, which keeps
`git log --follow` and blame useful), and keep their own `-h` handling
removed (it moves to the dispatcher), their `usage` text moves to the table.

Why a library and not a macro or copy in each `main.c`: 14 programs need
the same handling of five cases, and `tools/tests/test_subcmd.c` can test
it once with inputs in memory (project rule: logic in `tools/lib/`, tests in
`tools/tests/`). The TED tools and the runners link it as well; the
libraries `tools/lib/*` are defined by `tools/CMakeLists.txt` before
`programs/ted/tools` and `tests/*/runner` are added
(`host/CMakeLists.txt:36-47`).

### D4. Exit codes and messages

- 0 success; 1 the checked thing failed or an input could not be processed;
  2 usage error (also unknown subcommand/option); 77 skipped, for a missing
  optional input (the CD, the previous build) or the emulator. This is what
  the programs say in their header comments today (`ab3tok` main.c:17,
  `corpusrun` main.c:5-9, `prevcmp`, `teststats` main.c:13).
- The existing exit status of every existing input stays as it is (an
  unreadable file is 1 in some tools and 2 in others today). Harmonising
  them would change what `WILL_FAIL` tests and `SKIP_RETURN_CODE`
  properties expect; it is a separate change. Each subcommand's exit codes
  are written in its README as they are, which also makes the differences
  visible (open question 3).
- Messages to stderr: `PROGRAM SUB: [FILE: ]message`. Existing messages
  already have the form `tool: [FILE: ]message`; the new prefix is the
  `argv[0]` the dispatcher passes (D3). Tests that match a message
  (`tools/*/tooltest.cmake`, `tooltest`-style `MATCHES` on stderr) are
  updated with their group. Results (reports, tables, converted text) stay
  on stdout or in the named output file.

### D5. The `-h` tests

`cmake/check-help.cmake` gets a variable `SUBCOMMANDS` (a list; empty for a
single-mode program). With it the script also runs `PROGRAM SUB -h` and
`PROGRAM SUB --help` for each name, with the same three checks, and checks
that the program's `-h` text contains `usage: PROGRAM SUB ` or
`       PROGRAM SUB ` for each name (so a subcommand the test does not
list but the table has shows up: the script also finds every
`PROGRAM NAME` synopsis line in the output and requires each NAME to be
in the list). The loop in `host/CMakeLists.txt:50-54` becomes a function
`host_help_test(PROGRAM [SUBCOMMANDS...])` with one call per program; the
group's `CMakeLists.txt` keeps its own subcommand list in one variable that
the test and the README check (D6) share. Test names: `help_PROGRAM`,
as today. A test of `check-help.cmake` with a fake program (a small C test
program in `tools/tests/`, one subcommand with help on stderr, another
missing) must fail, and the script is run on it as `WILL_FAIL`.

### D6. READMEs

Each program directory gets `README.md`, and only that: purpose; usage
(the synopsis lines of `-h` verbatim in a code block, then one paragraph
per subcommand with its options); inputs and outputs (formats, link to
`docs/formats.md` where it has one); exit codes; callers (CMake file and
target or test name); an example that was run. The group of the TED tools
and the two runners have their README in their own directory. A script
`cmake/check-readme.cmake` runs `PROGRAM -h`, takes the synopsis lines
(lines starting with `usage: PROGRAM ` or seven spaces and `PROGRAM `)
and requires every one of them to occur verbatim in the README, and every
such line in the README to occur in the output; test `readme_PROGRAM`
(host side), with a test on a fake program and a README that lacks a line
(`WILL_FAIL`). The check is a ctest test, not a build step: a README is not
a build result.

`docs/structure.md` (lines 51, 69-110 describe the tools) and the tool
descriptions in `tests/README.md` (for example lines 65-92, 370-380,
823-950) are shortened to one line per program with a link; arguments
stay in the README. `docs/development.md` (lines 107-216) names tools in
the check list; the names change and a link is added. The README of
`tools/lib/amiga_emulation/` stays; `tools/amiga_run/README.md` links it.

### D7. Directory layout

`tools/blitzlib/` (`main.c`, `cmd_join.c`, ..., `README.md`, the test
scripts of the former tools such as `tools/blitztokens/check.cmake`,
`tools/cmdref/check.cmake`), likewise `bbsrc`, `refcheck`, `hunkcheck`,
`results`, `testinfo`. The old directories are removed. The libraries stay
in `tools/lib/NAME/`; `lib_NAME` target names do not change (so
`tools/tests` is untouched except for the new `test_subcmd`). ctest names
of existing tests (`bbdetok_tool`, `tedtok_libs`, ...) stay: tools and
cost data refer to them, and a rename gives no gain; only `help_*` tests
change and `readme_*` are new. `tools/covsel/rules.txt` patterns follow
the directories (`tools/testinfo/** name:host/*`, `tools/results/**
label:emulator name:host/*` because `outmatch` is used by emulator tests,
as `tools/outmatch/**` today at lines 55-56).

### D8. No aliases for the old names

The old names are not kept. All callers are in this repository (D10's
counts; nothing is installed, `CMakeLists.txt:189-191`); an alias (a
symlink, a wrapper program or a second `add_executable`) keeps the
31 targets, the 31 variables and the confusion that this change is meant
to end, and silently hides a call site that was missed: with the names
gone, a missed call site fails the build or the test that runs it. The
price is that anyone with private scripts must change them; the commit
message of each group lists old and new calls.

### D9. Commits and verification

One commit per group, in this order (small and independent first, so that
the pattern, the dispatch library and the checks are proven before the
big groups): infrastructure (D3, D5, D6 scripts, no tool changes),
`testinfo`, `refcheck`, `results`, `hunkcheck`, `blitzres`, `blitzlib`,
`tedab`, `corpusrun`, `tedsrc`, `bbsrc`, then the READMEs of the programs
that have no code change (`amiga_run`, `mktestdata`, `xtra`, `hdftool`)
and the final cleanup of documents. Each group commit contains: the
program, the removal of the old directories, all call sites (D10: the
variables in `CMakeLists.txt` including `BUILD_BYPRODUCTS`, `add_test`,
`add_custom_command`, `*.cmake`), the `help_*` and `readme_*` tests,
`rules.txt`, `README.md` of the program, the doc updates, and the
subcommand's host tests when it gets a test of its own.

Verification per group (the acceptance rule, `CLAUDE.md`):

1. clean build without warnings (`cmake --preset amiga && cmake --build
   --preset amiga`), also with `-DHOST_SANITIZE=ON` for the host side;
2. `-DBLITZ2_PREV=ON`, build, then `prev_results` passes and the corpus and
   `ted_ab` `prev` configurations pass, with no `PREV_INTENDED`;
3. `ctest --preset host`, `ctest --preset all` (needs Amiberry; skipped
   tests are listed in the commit message when the CD is missing), and
   `ctest --preset oracle` if `tests/ab3` was touched;
4. a search for the old names as commands, variables and paths over the
   repository finds nothing outside `archive/`, `TODO.md`, `openspec/`;
5. for each subcommand a before/after run on the same inputs (the call
   sites of the tests do this; for subcommands without a test on files,
   one run by hand with `cmp` of stdout and output file, recorded in the
   commit message).

### D10. Call sites per group

Counted with `grep` on 2026-11-08 over `CMakeLists.txt` and `*.cmake` files
of `cmake docs host libs programs shared tests tools` and the top-level files;
a count is one occurrence of `${NAME}`, `$<TARGET_FILE:name>`, a path
`/name/name` or `COMMAND name`, so the `set(...)` of the variable and its
`BUILD_BYPRODUCTS` entry are included (counting `${TEDTOK}`, `${BBDETOK}`
and `${AMIGA_RUN}` alone gives 25, 23 and 20 occurrences). "Files" is the number of files with at least one occurrence.
"Docs" counts the word in `*.md` files (README, TODO, docs, tests, tools,
programs; it overestimates: `xtra` is also the file extension).

| Group | Tool | CMake occurrences / files | Docs / files |
|---|---|---|---|
| `blitzlib` (31) | biglib | 12 / 6 | 34 / 20 |
| | blitzlibcheck | 4 / 2 | 15 / 10 |
| | blitzlibcompare | 8 / 4 | 19 / 13 |
| | cmdref | 7 / 5 | 7 / 3 |
| `bbsrc` (56) | blitztokens | 14 / 9 | 27 / 12 |
| | bbdetok | 24 / 4 | 14 / 4 |
| | bblines | 4 / 2 | 6 / 5 |
| | bb22 | 6 / 4 | 28 / 9 |
| | ab3tok | 8 / 7 | 7 / 5 |
| `tedsrc` (45) | tedtok | 27 / 11 | 9 / 4 |
| | teddetok | 18 / 10 | 22 / 7 |
| `refcheck` (22) | asmcmp | 3 / 2 | 16 / 9 |
| | checklabels | 3 / 2 | 14 / 6 |
| | checkdata | 6 / 2 | 28 / 9 |
| | vasmlisting | 10 / 3 | 10 / 6 |
| `hunkcheck` (10) | hunkcheck | 10 / 7 | 8 / 5 |
| `results` (25) | outmatch | 8 / 6 | 3 / 3 |
| | prevcmp | 13 / 2 | 1 / 1 |
| | cdtable | 4 / 2 | 1 / 1 |
| `testinfo` (7) | teststats | 0 / 0 | 8 / 2 |
| | covmap | 4 / 1 | 7 / 2 |
| | covsel | 3 / 1 | 5 / 2 |
| `corpusrun` (22) | corpusrun | 13 / 8 | 14 / 5 |
| | cdrun | 9 / 5 | 14 / 4 |
| `tedab` (10) | tedab | 10 / 3 | 43 / 7 |
| | teddump | 0 / 0 | 18 / 5 |
| `blitzres` | blitzres | 12 / 5 | 21 / 7 |
| standalone | amiga_run | 25 / 15 | 20 / 8 |
| | mktestdata | 13 / 3 | 6 / 3 |
| | xtra | 6 / 4 | 250 / 35 (mostly the extension) |
| | hdftool | 5 / 1 | 10 / 3 |

Total 289 CMake occurrences (recounted on 2f44b9d4; 284 when written, see "Implementation notes"). The largest commits are `bbsrc` (56
occurrences in 18 files, 82 doc mentions; `bbdetok`'s 24 are mostly in
`tools/bbdetok/tooltest.cmake`) and `tedsrc` (45 in 21 files, the call
sites are spread over `libs/userlibs/rwe`, `tests/bug-orig`, `tests/ab3`,
`tests/corpus`); `blitzlib` (31, the build of every library: the highest
risk, because `cmake/BlitzLibrary.cmake` is in every library target) and
`corpusrun` (22, in the emulator tests) follow. `teststats` and `teddump`
have no CMake call (run by hand); `covmap`/`covsel` are called by name in
two scripts.

## Risks / Trade-offs

- **A missed call site** shows up as a failing build or test only where
  the call is run: tests that need the CD, Amiberry or `archive/aminet`
  are skipped without them. Mitigation: the search of D9 step 4 over all
  files, not only the ones built; the implementer runs the group's
  tests with the originals and Amiberry present (`ctest --preset all`).
- **`BUILD_BYPRODUCTS` and `host_tools`:** the ExternalProject must list
  the new program paths or Ninja does not know them
  (`CMakeLists.txt:109`); a forgotten entry gives "missing and no known rule
  to make it". Part of the group checklist.
- **A tool binary gets larger** (all libraries of a group linked in):
  irrelevant for host tools; `HOST_COVERAGE` and the sanitizers see the
  same code as before.
- **Merging `main.c` files renames static functions and `usage` strings**
  that were separate per program (several define `read_all`): the
  `cmd_NAME.c` files keep them `static`, so no clash; two programs defining
  the same global would be a link error at the group commit.
- **Subcommand names are API** once documented and used by a dozen scripts;
  changing one later costs a sweep like this one. D1 fixes them now, and
  open question 1 and 2 are for the user to settle before the groups
  `hunkcheck` and `amiga_run` are done.
- **Messages change their prefix** (`bbdetok:` to `bbsrc detok:`): tests
  that match stderr text are updated in the group commit; a missed one
  fails.
- **Commit size:** `bbsrc` and `tedsrc` touch 18-21 files each. Splitting a
  group into two commits would leave a state where half the group is
  switched; the spec forbids that. Review is easier because most of the
  change is mechanical (rename of the program in a command line).
- **Not harmonised exit codes** (D4) leave an inconsistency the READMEs
  document; the alternative changes verdicts and is a separate decision.

## Open questions (decided 2026-11-08)

Settled by the coordinator as this design proposes (task 0.3):

1. `hunkcheck` stays a program of its own (`start`, `layout`, `stored`).
2. `amiga_run` keeps `--purge-snapshots` as an option of its one mode.
3. The exit status for an unreadable input stays as it is (1 in some
   tools, 2 in others) and is documented per tool in its README;
   harmonising it would be a separate change.
4. `testinfo` with `stats`, `map`, `select`.
5. `corpusrun` (`pass`, `cd`) and `tedab` (`run`, `lint`, `info`, `text`,
   `cmp`) stay two programs.

## Implementation notes

Findings and adaptations while implementing (2026-11-08).

- **Changes on `main` since the design was written**, none of which
  changes the plan: `cdtable` writes error notes `(2.10: error LINE)`
  (`tools/lib/cdtable`, 8f494806; `cdtable update|reference` keep their
  arguments, the group `results` takes it over as it is); the prev build
  passes the snitch source to its nested configures (`cmake/prev.cmake`,
  `cmake/prev-build.cmake`, a744716e; the prev build is still built from
  the previous commit's own CMake files, so the rename of the tools does
  not disturb it); `tools/lib/amiga_emulation/default.uae` uses
  `comp_flushmode=hard` (emulator configuration only).
- **Call sites recounted** on 2f44b9d4 with the method of D10: as D10
  except `prevcmp` 13 / 2 (the `prevcmp_*` tests of `tools/tests` gained
  three calls) and `blitzres` 12 / 5; total 289 instead of 284.
- **Baseline** (task 0.2, `~/.local/share/bb2-c-work/tools-baseline.md`):
  `ctest --preset host` passes; `ctest --preset all` fails only
  `host_emulator`, because of `amiga_emulation_snapshot` (a restored
  snapshot does not start; Amiberry, not a tool of this change).
- **`subcmd` (D3)**: the usage text of an entry is the subcommand's own
  `-h` output and starts with `usage: PROGRAM SUB`; a subcommand prints
  the same text on a usage error (`cmd_NAME_usage`, defined in
  `cmd_NAME.c`, referenced by the table). The program usage is not the
  whole texts but their synopsis lines (of each entry the first line and
  the lines after it that start with seven spaces), the `usage: ` of every
  entry after the first replaced by seven spaces, and a last line
  `The description and options of a subcommand: PROGRAM SUB -h`: with the
  descriptions of all subcommands, `-h` of a program with six of them was
  pages long and its synopsis hard to find. The synopsis lines have the
  form D5 and D6 expect. Besides `subcmd_main()` the
  library has `subcmd_main_to()` (the streams given, for the test),
  `subcmd_check()` (the table rules: a name, a function, a usage that
  starts with `usage: PROGRAM NAME`, one `usage: `, a final newline, no
  name twice; a broken table makes every call exit 1) and
  `subcmd_usage()`.
- **The broken-input tests of the checks** (D5, D6) match the failure
  message with `PASS_REGULAR_EXPRESSION` instead of `WILL_FAIL`, as the
  other broken-input tests of the build do (`cmdref_check_broken`): a
  failure for another reason (a missing program) does not pass them.
  `check-help.cmake` gets the subcommand list comma-separated
  (`-DSUBCOMMANDS=a,b`), because `add_test` would split a `;`-list into
  arguments.
- **Messages**: a former `main.c` keeps its messages; only the prefix
  becomes `argv[0]` (`static const char *me`, set first in `cmd_NAME`),
  also of the messages written to stdout (`asmcmp`'s "skipped"). A
  message that is not of the form `tool: FILE: message` today (for
  example `teststats: cannot read FILE`) keeps its form: the spec allows
  no other change than the prefix.
- **Report lines of the libraries** keep the library's name: the
  summary lines `checkdata: 534 checks, ...`, `checklabels: 278 code
  labels, ...`, `prevcmp: 787 library files, ...` and `asmcmp: SPEC: ...`
  are written by `lib_checkdata`, `lib_checklabels`, `lib_prevcmp`,
  `lib_asmcmp` to the report (stdout), which must stay byte-identical, and
  the libraries keep their names (D7); tests that match them
  (`ted_checkdata_*`, `test_checkdata`) are unchanged.
