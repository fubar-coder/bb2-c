# Design

## Context

The 14 host programs (`amiga_run`, `bbsrc`, `blitzlib`, `blitzres`,
`hdftool`, `hunkcheck`, `mktestdata`, `refcheck`, `results`, `testinfo`,
`xtra`, `tedsrc`, `corpusrun`, `tedab`; main spec
`openspec/specs/tooling/host-tools/spec.md`) are C23 programs. Eleven
use `tools/lib/subcmd` (`subcmd.h`: a table of name, `run(argc, argv)`
and a usage string; the program usage is derived from the tables'
strings, `subcmd_main()` handles `-h` and unknown names, status 2).
`hdftool` (`tools/hdftool/main.c:21-33`) and `xtra`
(`tools/xtra/main.c`) dispatch on their own, and `amiga_run` and
`mktestdata` have one mode. Every subcommand parses its options with
`strcmp(argv[i], ...)`. The option names in use are listed by
`grep -rhoE 'strcmp\(.*"-[^"]*"'`: `-h`, `--help`, `-o`, `-v`, `-q`,
`--bb22`, `--ab3`, `--stale`, `--root`, `--mount`, `--lvo`, `--drop`,
`--check`, `--write`/`--write-hex`/`--replace`/`--copy` (ordered
operations of `hdftool edit`), `--set`/`--unset` (repeatable, `amiga_run`),
the single-dash long names `-group`, `-name`, `-last` of `blitzlib cmdref`,
and `-` as "stdin" argument.

The host build is `host/CMakeLists.txt`: `project(... LANGUAGES C CXX)`,
C23 for `tools/`, C++20 for snitch-based tests
(`programs/ted/tests/host/CMakeLists.txt:49`), `-Wall -Wextra`, optional
`HOST_SANITIZE` and `HOST_COVERAGE`. Library headers in `tools/lib/*/`
mostly have no `__cplusplus` guard (22 of them lack it), so C++ code
includes them inside `extern "C"` as the snitch tests do. snitch is
fetched by `cmake/Snitch.cmake` (pinned tarball URL, `URL_HASH`,
`FETCHCONTENT_SOURCE_DIR_SNITCH`); `cmake/prev.cmake:60-75` reads the
SHA256 from that file and hands the current source to
`cmake/prev-build.cmake`, which gives it to the prev build's configure so
that no network is used.

Callers that depend on exit codes other than 0 and 77 (found by grep over
`*.cmake` and `CMakeLists.txt`):

| Where | Depends on |
|---|---|
| `SKIP_RETURN_CODE 77`, `if(rc EQUAL 77)` in many files, `cmake/check-help-amiga.cmake:20` | 77 (the Amiga test programs, not the host tools, for the last one) |
| `programs/ted/tools/tedsrc/tests/run.cmake:19` | `tedsrc detok` of a bad source exits 1 |
| `tests/corpus/runner/check-cdrun-source.cmake:26,37` | `corpusrun cd` with a bad source option exits 2 |
| `tools/bbsrc/detok-tooltest.cmake:11`, `ab3-tooltest.cmake:9`, `tools/xtra/tooltest.cmake:10` | the expected status passed per call |
| `tests/bug-orig/want.cmake:9`, `tests/bug-orig/CMakeLists.txt:402-555` | the verdict codes of the runners: 0 FIXED, 1 REPRODUCED/REGRESSED, 3 BROKEN, 4 DIFFERS |
| `tools/tests/CMakeLists.txt:135` | `blitzlib cmdref` of a non-library exits 1 |

See `proposal.md` for the motivation.

## Goals / Non-Goals

**Goals:**
- One command-line library for every host program, with one help format,
  one error behaviour and exit status 2 for all parse errors.
- The README usage block cannot differ from the program's help.
- No change of any result file.

**Non-Goals:**
- Converting the logic in `tools/lib/` or the component libraries to C++.
- Converting the test programs (`tools/tests/test_*.c`,
  `tests/corpus/runner/test_*.c`, host tests); they stay C and are not
  user-facing tools. `test_amiga_emulation*.c` have a few hand-parsed
  arguments and stay as they are.
- New features of the tools; new subcommands; regrouping.
- Colour, completion scripts, man pages.

## Decisions

### D1. CLI11, fetched like snitch, as the target `CLI11::CLI11`

`cmake/CLI11.cmake` (new) declares `FetchContent` with the release tarball
URL and `URL_HASH SHA256=...`, `EXCLUDE_FROM_ALL`, `SYSTEM`, with
`CLI11_BUILD_TESTS/EXAMPLES/DOCS` and `CLI11_INSTALL` off, and calls
`FetchContent_MakeAvailable`. It is included from `host/CMakeLists.txt`
next to `Snitch.cmake`. Programs link `CLI11::CLI11` (header only, so no
library is built). A locally existing copy is taken the same way as
snitch's (`FETCHCONTENT_SOURCE_DIR_CLI11` given by the top-level build).
Alternative: the single-header asset `CLI11.hpp` downloaded with
`file(DOWNLOAD)` - rejected: a second mechanism beside FetchContent, and
the target also carries the C++ standard requirement.

### D2. The prev build reuses the main build's CLI11

`cmake/prev.cmake` extracts a second pin from `cmake/CLI11.cmake`
(`URL_HASH SHA256=`, as for snitch) and gives `-DCLI11_PIN=` and
`-DCLI11_SOURCE_DIR=` to `cmake/prev-build.cmake`. The source directory is
`${cli11_SOURCE_DIR}` that `FetchContent_MakeAvailable(CLI11)` sets (not a
target property: the target of a header-only library has no reliable
`SOURCE_DIR`); `CLI11.cmake` also stores it in a cache variable so that
the top-level build, whose host side is an ExternalProject, can pass it
(`FETCHCONTENT_SOURCE_DIR_CLI11`) the way it passes snitch's.
`cmake/prev-build.cmake`
adds `-DFETCHCONTENT_SOURCE_DIR_CLI11=` to the prev build's configure if
the previous commit's `cmake/CLI11.cmake` pins the same SHA256, exactly as
for snitch; a previous commit without `cmake/CLI11.cmake` (the first
commits of this change) needs nothing. The symlink fallback of snitch
(for commits that do not pass the variable on to their host
ExternalProject) is not repeated: every commit that has `CLI11.cmake` also
has the pass-on. Verify: the prev build with the network switched off
(`unshare -rn` or no route) configures.

### D3. A small C++ helper, `tools/lib/cli/`, for the program shell

`tools/lib/cli/cli.hpp` (header only, C++20) holds what every program
repeats and nothing else:

```cpp
// cli::run: parses argv, calls the selected subcommand, returns the status
int run(CLI::App &app, int argc, char **argv, int &status);
```

`run` catches `CLI::ParseError`, calls `app.exit(e)` (help on stdout,
errors with the hint on stderr) and returns 0 for a help request and 2 for
every other parse error (CLI11's own codes 100+ are never returned). A
program's `main.cpp` is

```cpp
int main(int argc, char **argv)
{
    CLI::App app{"Blitz library files", "blitzlib"};
    int status = 0;
    app.require_subcommand(1);
    cmd_join(app, status);      // each declares its CLI::App *sub and callback
    cmd_check(app, status);
    ...
    return cli::run(app, argc, argv, status);
}
```

Each `cmd_NAME.cpp` provides `void cmd_NAME(CLI::App &app, int &status)`:
`auto *sub = app.add_subcommand("name", "one-line description")`, its
options bound to local state, `sub->callback([=, &status]{ status =
run_NAME(...); })`. The message prefix `PROGRAM SUB: ` comes from
`sub->get_name()` and the program name in one function of the helper.
The Formatter is set once in `cli::run` (column width, no empty
`[Option Group]` headings) so that all programs print the same shape.
Alternative: no helper, each `main.cpp` repeats the try/catch - rejected:
the exit-code rule (parse error to 2) must be in one place.

### D4. C and C++ meet at the library headers

C++ files include `tools/lib` headers inside `extern "C" { ... }` (the
convention of the snitch tests, CLAUDE.md). The headers are not edited.
The program targets become `LANGUAGES C CXX` targets with
`CXX_STANDARD 20`; `-Wall -Wextra` stay and a clean build has no warnings.
Code that was in `cmd_*.c` and is not option handling moves to
`cmd_NAME.cpp` unchanged except for the C++ constructs it needs to
compile (casts of `malloc`, no designated-initialiser order issues);
whatever is large and pure C stays in the library. Alternative: keep the
bodies in `.c` files and only the option layer in `.cpp` - rejected for the
"only the program and command layer becomes C++" rule being about files
(`main.c`, `cmd_*.c`), and a second file per subcommand doubles the
files without a gain; where a body is big and already a plain function,
it may stay in its `.c` file and is called from the `.cpp`.

### D5. Mapping of today's options

- Flags, valued options, repeatable options (`--set K=V` ...), positional
  lists and `-`: plain CLI11 (`add_flag`, `add_option`, `->expected`,
  `->take_all`, positional `add_option("FILE", files)->required()`).
- `-v|-q` (`refcheck labels`): two flags, `->excludes()`.
- `hdftool edit ... OP...` (`--write PATH TEXT`, `--write-hex PATH HEX`,
  `--replace PATH OLD NEW`, `--copy FROM TO`): four options with
  `->expected(2|3)->multi_option_policy(CLI::MultiOptionPolicy::TakeAll)`;
  the order of the operations is `app.parse_order()` (CLI11 returns the
  options in the order they were given), so the operations are applied as
  today.
- A value that starts with `-` (a TEXT for `--write`, `--replace`) is read
  by CLI11 as an option. The calls that pass free text (the callers of
  `hdftool edit`, in `cmake/setup-amiga-forever.cmake` among others) are
  checked by grep at implementation; see Open Question 5.
- Single-dash long names (`-group`, `-name`, `-last`) become `--group`,
  `--name`, `--last`; all callers are in `tools/blitzlib/cmdref-check.cmake`,
  `cmdref-complete.cmake` and `tools/tests/CMakeLists.txt` and change in the
  `blitzlib` group's commit. CLI11 could accept the single-dash form with
  `allow_non_standard_option_names()`, but that makes `-name` ambiguous with
  `-n -a -m -e` and is rejected.
- `--`: terminates options in CLI11 as before.
- Abbreviated long options stay off (CLI11's default), as today.
- Combined short flags (`-vq`) CLI11 accepts; they were no error before
  only by accident.

### D6. The README keeps equal to the help by comparing blocks

A host program's help is the CLI11 text, so the old rule "every synopsis
line of `-h` is a line of the README" no longer finds the options
(there are no synopsis lines per option). New rule: the README contains,
in fenced code blocks, the output of `PROGRAM -h` and of each
`PROGRAM SUB -h`, verbatim; the surrounding text explains in words.

`cmake/check-readme.cmake` (rewritten) takes `PROGRAM`, `README` and the
`SUBCOMMANDS` list the help test already has. It runs `-h` and each
`SUB -h`, and
1. fails and names the help whose text (trailing blanks and `\r` removed)
   is not a contiguous part of the README ("README: help of `blitzlib
   compare` is missing or out of date");
2. reads every fenced block of the README whose first line starts with
   `Usage: NAME` and fails and names a block that equals none of the
   helps ("README: usage block of ... no longer printed by ...").

The example in the README that "runs as written" and the other checks stay.
The text of the blocks is long (the options), but it is the single place
that documents the options, and a change of an option makes the test fail
until the README is regenerated (the failing message prints the new help).
Alternative: only compare the `Usage:` lines - rejected: options would not
be covered. Alternative: generate the README block at build time -
rejected: the README is read on GitHub and must be in git. The help text
must not depend on the terminal: `cli::run` fixes the width, so the
output is the same on every machine.

`cmake/check-help.cmake` changes in step: stdout starts with `Usage: `
(and `Usage: NAME SUB` for a subcommand), exit 0, stderr empty, for `-h`
and `--help`; the subcommands of the program are read from its help
(the "Subcommands:" section of CLI11's formatter) and every one must be in
`SUBCOMMANDS`; one more case runs the program with an unknown option and
requires exit status 2 and a message on stderr (D7).

Tests with deliberately broken input (project rule): the fake program
`tools/tests/fake_cli11.cpp` (replacing `fake_subcmd.c`; the same
`alpha`/`beta`, plus `fake_cli11_broken` whose `beta -h` writes to stderr)
and fake READMEs `ok.md`, `missing.md` (a subcommand help absent),
`changed.md` (an option line differs) and `stale.md` (a usage block of
a subcommand that does not exist); `help_check_*` and `readme_check_*`
as now; `test_subcmd.c` is replaced by `test_cli.cpp` (a snitch or a plain
C++ test of `cli::run`: help is 0, unknown subcommand, unknown option,
missing value and surplus argument are 2, a subcommand's status is
returned, 77 passes through).

### D7. Exit codes

0 and 77 are returned by the subcommand body as before. `cli::run` maps
every parse error to 2. Nothing else is changed in a body: a status 1
(failed check, unreadable input) stays 1, the runners' verdict codes (3,
4) stay. The table in Context lists what CMake relies on; each of those
callers is unchanged except where a parse-error status changes: the grep
at implementation (task 1.4) repeats the search for `EQUAL <n>`,
`WILL_FAIL`, `PASS_REGULAR_EXPRESSION` over all CMake files below `tools/`,
`tests/`, `programs/`, `libs/`, `cmake/`, `host/` and the top level, and
every program's group task re-checks its own callers. A caller that
expects another status than 0, 2, 77 or a documented verdict is changed to
what the README documents. Exit codes in the READMEs follow the code.

### D8. Order of work

Infrastructure first (CLI11 pin, prev pass-on, helper, new checks with
their broken-input tests, in parallel to the old `subcmd` and `check-*`,
which stay until the last program is converted: the new scripts get the
names `check-help-cli.cmake` and `check-readme-cli.cmake`, and the
`host_help_test()`/`host_readme_test()` functions take the script as an
argument), then one commit per program: a pilot with three small
subcommands (`hunkcheck`), the `subcmd` users in the order of the earlier
change, the single-mode programs, `hdftool`, `xtra`; a final sweep removes
`tools/lib/subcmd`, `fake_subcmd`, `test_subcmd` and the old check scripts
(the new ones take the old names) and updates the documents. Each program
commit switches its callers in the same commit (as "A group is switched
over completely in one commit"), changes messages that tests match, and
keeps every result byte-identical.

## Risks / Trade-offs

- [C++ exceptions and `-fsanitize`] CLI11 throws on parse errors; the
  helper catches them all (`CLI::ParseError`, and `std::exception` for a
  failed validator) and returns 2 → covered by `test_cli`.
- [CLI11 treats a value that starts with `-` as an option] A caller that
  passes such a value (TEXT of `hdftool edit --write`, a pattern) breaks →
  grep all callers per group; the group's task lists the calls; where
  needed use `--name=VALUE` for single-value options.
- [Help text differs per CLI11 version] The README blocks and the tests
  would fail on an update → the version is pinned; an update of the pin is
  a deliberate change whose commit regenerates the README blocks (the
  failing message prints them).
- [Behavioural differences of parsing: options after positionals, `-vq`,
  `--opt=value`, repeated flags] More is accepted than before, nothing that
  was accepted is refused except the renamed single-dash options and
  values starting with `-` → the per-group `cmp` of the results on the same
  inputs, with the old calls translated.
- [Build time and binary size grow] The 14 programs include CLI11 (header
  only, about 8000 lines each) → accepted; `cli.hpp` includes it once per
  translation unit, and only `main.cpp` and the `cmd_*.cpp` of the program
  do.
- [Network at first configure] Like snitch; a local copy
  (`FETCHCONTENT_SOURCE_DIR_CLI11`) and the prev build's reuse avoid it
  after that.
- [Exit code of a runner collides with 2] `corpusrun` and `tedab` use 0, 1,
  3, 4 as verdicts and 2 for usage → unchanged; checked in their groups.

## Migration Plan

One commit per group on a branch of `main`; each commit builds without
warnings and passes `ctest --preset host` (also `-DHOST_SANITIZE=ON`),
`prev_results` without `PREV_INTENDED` against the commit before the first
group of this change, and the corpus and `ted_ab` prev parts. Rollback is
reverting the group's commit; the programs not yet converted run with the
old `subcmd` until the final sweep.

## Open Questions (decided 2026-11-09)

The user decided all eight as recommended (task 0.2); the notes in
"Implementation notes" say how each one was carried out.

1. **CLI11 version.** Recommendation: the newest 2.x release at the start
   of the work (2.5.0 or later; check the GitHub releases page), pinned by
   release tarball URL and SHA256 computed from the downloaded archive,
   recorded in `cmake/CLI11.cmake`. Not a snapshot of `main`.
2. **Tarball with the `CLI11::CLI11` target or the single header.**
   Recommendation: the tarball with the target (D1).
3. **Shape of `-h`.** Recommendation: CLI11's default formatter with fixed
   width, description under `Usage:`, the options table, the `Subcommands:`
   section for programs with subcommands, and a footer line for exit codes
   only where a program has verdict codes (`corpusrun`, `tedab`, `results
   prev`). No colour. The README blocks follow whatever the formatter
   prints (D6).
4. **Option renames callers see.** Recommendation: only the three
   single-dash names of `blitzlib cmdref` (`-group`, `-name`, `-last` to
   `--group`, `--name`, `--last`), changed in the same commit with
   `cmdref-check.cmake`, `cmdref-complete.cmake` and
   `tools/tests/CMakeLists.txt:136`; every other name stays. Confirm by the
   grep in task 1.4 and again per group.
5. **Values that start with `-`.** Recommendation: leave the options as
   they are, check the calls of `hdftool edit` and of every option with free
   text by grep; if a call passes such a value, change that call to
   `--name=VALUE` where the option takes one value, or document `--` where
   it takes positionals. Decide per case in the group's task, not in
   advance.
6. **Verdict codes 3 and 4 of `corpusrun`/`tedab`.** Recommendation: keep
   (CMake depends on them: `tests/bug-orig/want.cmake`); document them in
   the READMEs as verdicts, and mention them in the help footer.
7. **`hdftool edit` operations.** Recommendation: keep the four options and
   apply them in `parse_order()` order (D5); the alternative of one
   subcommand per operation changes the call syntax and is not worth it.
8. **Where the programs' description text lives.** Recommendation: the
   one-line description in the `add_subcommand` call and the long
   description (what the README says first) in `sub->description` set by
   `->description()`; no separate text files.

## Implementation notes

Findings and adaptations while implementing (2026-11-09).

- **Changes on `main` since the plan was written** (expected-results-testing
  7.x): `corpusrun pass --update-expect TAG PASSFILE` and the pass-file key
  `expect_dir=` (`tests/corpus/runner`), and `results ted-oracle NEW OLD
  OUT` (`tools/results/cmd_ted_oracle.c`, `tools/lib/oracletable`). They
  are converted in their groups (`results` in group 5, `corpusrun` in
  group 11) like the other subcommands.
- **CLI11 2.7.2** (the newest 2.x release on 2026-11-09), the GitHub
  release tarball `archive/refs/tags/v2.7.2.tar.gz` with SHA256
  `46eef310...83da` computed from the download (Q1, Q2).
- **The top-level build fetches CLI11 too** (D2): the host side is an
  ExternalProject configured at build time, so its cache cannot give the
  top-level build a source directory. The top-level `CMakeLists.txt`
  includes `cmake/CLI11.cmake` with `CLI11_SOURCE_ONLY` (FetchContent with
  a `SOURCE_SUBDIR` that does not exist: the source is downloaded, no
  target is made in the m68k build) and passes `cli11_SOURCE_DIR` to the
  host side as `FETCHCONTENT_SOURCE_DIR_CLI11` and to `prev-build.cmake`
  (`-DCLI11_PIN`, `-DCLI11_SOURCE_DIR`), as for snitch. The host side
  downloads nothing.
- **Help shape** (Q3): CLI11's default formatter puts the description
  before the usage line and the usage without `Usage:`; the spec wants
  stdout to start with `Usage: `. `cli::Formatter` derives from
  `CLI::Formatter` and overrides only `make_help()`: `Usage: ` and the
  usage line, the description, then CLI11's own `POSITIONALS:`,
  `OPTIONS:`, `SUBCOMMANDS:` sections and the footer, at the widths of
  CLI11's defaults set explicitly (column 30, paragraphs 80). The labels
  stay CLI11's (`OPTIONS`, `POSITIONALS`, `SUBCOMMANDS`); the help check
  reads the `SUBCOMMANDS:` section. A subcommand inherits the formatter
  when it is added, so `cli::run()` sets it on the whole tree.
- **Description texts** (Q8): CLI11 has one description per subcommand,
  and it is also the subcommand's line in the program's help. The
  one-line description is given to `add_subcommand()`; the longer text of
  a former usage (what the README says first, the exit codes) goes into
  the subcommand's `footer()`.
- **Unknown subcommand**: CLI11 reports `prog gamma` as "A subcommand is
  required"; `cli::run()` checks the first argument first and writes
  `PROGRAM: NAME is no subcommand` and the hint (status 2), so the message
  names the problem (spec "Unknown or missing subcommand").
- **A thrown `std::exception`** (a validator, D7) is caught by `cli::run()`
  and gives `PROGRAM: what` and status 2.
- **Test names during the change** (D8): the new checks' tests are
  `help_check_cli_*` and `readme_check_cli_*`, the fake READMEs are in
  `tools/tests/fake-readme-cli/`, beside the old `help_check_*`,
  `readme_check_*` and `fake-readme/`; the final sweep gives the new ones
  the old names. `host_help_test(PROGRAM CLI SUB...)` and
  `host_readme_test(PROGRAM README.md CLI SUB...)` select the new scripts;
  the final sweep drops the `CLI` keyword.
- **`-last` is `bbsrc tokens`'s** (Q4, task 1.4): `blitzlib cmdref` has
  `-o`, `-group`, `-name`; the single-dash `-last` is an option of `bbsrc
  tokens` (callers `tests/ab3/tokenise.cmake`, `tests/ab3/tokens.cmake`).
  It becomes `--last` in the `bbsrc` group by the same rule (D5), with its
  callers.
- **Inventory** (task 1.4): `~/.local/share/bb2-c-work/cli11-callers.md`.
- **Pilot `hunkcheck`** (task 2.1), lessons for the other groups:
  - CLI11 refuses `:` and `=` in a name (they separate values), so an
    argument such as `OFFSET:HEXBYTES` gets a plain name (`SPEC`) and its
    form goes into the description.
  - An option bound to a `std::vector` takes all following arguments that
    are no option (it would swallow positional arguments) and refuses a
    second `--name` by default. A repeatable option with N values per
    time is `cli::add_repeated(app, "--name", vec, N, ...)`
    (`type_size(N)`, `expected(1)`, `allow_extra_args(false)`, `TakeAll`;
    with `expected(N)` CLI11 takes as few as one value per time, found
    with `hdftool edit --write PATH` and fixed in commit "tools/lib/cli: a
    repeated option with N values needs all N each time").
  - `app.parse_order()` names an option once per value, not once per time
    it was given; `cli::occurrences(app)` groups them into the times with
    their values, for operations applied in command-line order (`hdftool
    edit`, `testinfo map --inputs` among the other inputs). Both helpers
    are tested by `test_cli`.
  - A value that starts with `-` is taken by a one-value option if it is
    no option name of the subcommand (`--text -x` gives `-x`);
    `--name=VALUE` always works (Q5).
  - **Several subcommands in one file**: where the predecessor change put
    the bodies of several subcommands in one `cmd_*.c` (one former main,
    for example `hunkcheck`'s `cmd_check.c`), they stay in one
    `cmd_*.cpp` (a git move) instead of one file per subcommand (C1); the
    shared helpers of the file stay private to it.
  - The subcommands get the exit codes in their footer; the program help
    has the one-line descriptions only.
- **`testinfo`** (task 3.1): CLI11 prints an option with N values as
  `--linkmap PROGRAM FILE x 2`; `cli::Formatter` drops the ` x N`, since
  the type name lists the N values. `testinfo map` reads its inputs in
  the order given, an `--inputs` file where it is named
  (`cli::occurrences`), as before. Checked with a host coverage run
  (`amiga-cov`, the host tests, `cmake/coverage-map.cmake` and
  `cmake/coverage-select.cmake` with the old and the new `testinfo`):
  `coverage/` (45 tracefiles, `all.info`, `commit`, `selected.txt` with
  849 tests, the inputs) and both logs identical.
- **`blitzlib`** (task 7.1): `cmdref -group/-name` became `--group`/`--name`
  (callers `tools/blitzlib/cmdref-check.cmake`, the test
  `cmdref_not_a_library`). `tools/lib/blitzlib/blitzlib.h` names the
  second parameter of `blitzlib_compare()` `other` instead of `new` (a C++
  keyword; only the prototype and its comment, the implementation is
  unchanged): the one change of a library header (D4), needed to include
  it from C++.
- **`bbsrc`** (task 8.1): `tokens -last` became `--last` (callers
  `tests/ab3/tokenise.cmake`, `tests/ab3/tokens.cmake`,
  `docs/libs/readable-sources.md`). The TOKENS options of `detok`
  (`--tokens`, `--blitz2`, `--last`, `--lib`) are read in the order given
  (`cli::occurrences`), as before; a `detok` without one of them or with a
  `--last` that no `--blitz2` follows says so (status 2 as before, a
  message instead of the usage text). An `OUT` of an earlier run is left
  as it is when the token list fails, as before.
- **`tedsrc`** (task 9.1): the token list is still made by
  `host_tokens_args()` (`programs/ted/tests/host/tokref.c`, unchanged):
  the subcommand gives it `TOKENS` or `--blitz2 BLITZ2 --lib LIB...` as
  parsed. `--blitz2`, `--lib` and `--check` may now stand anywhere (before
  they had to come first, `--check` after `TOKENS`); the arguments
  `[TOKENS] IN OUT` / `[TOKENS] IN...` are one positional list `FILE...`,
  whose forms the README and the help name. A wrong count, or `--lib`
  without `--blitz2`, gives 2 with a message.
- **`tedab`** (task 10.1): the body of `run` and `lint` (1300 lines) stays
  C in `run.c` (moved from `cmd_run.c`, entry `tedab_start()`, `run.h`;
  D4: a big body may stay in its `.c` file); `cmd_run.cpp` declares the
  two subcommands. `info`, `text` and `cmp` (`cmd_dump.cpp`) take only
  their own options (before, `info` accepted and ignored `--font`,
  `--fonts`, `--no-mask`, `text` `--no-mask`). The verdict codes are in
  the footers of the program and of `run`.
- **`corpusrun`** (task 11.1): the body of `cd` (1150 lines) stays C in
  `cd.c` (moved from `cmd_cd.c`, entry `corpusrun_cd()`, `cd.h`), as
  `tedab run`; `cmd_cd.cpp` declares the subcommand. `pass
  --update-expect TAG` may stand after `PASSFILE`; an empty `TAG` is
  still a usage error (2).
