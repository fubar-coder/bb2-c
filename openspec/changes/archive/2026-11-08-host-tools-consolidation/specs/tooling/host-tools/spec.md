# Spec Delta

## Purpose
How the repository's host programs (the tools of `tools/`, the TED tools,
the corpus and TED A/B runners) are organised, invoked and documented, so
that a developer finds a tool by its theme, calls every tool the same way
and reads what it does in its directory.

## ADDED Requirements

### Requirement: Host programs are grouped by theme
Host programs that work on the same kind of input or answer the same kind
of question SHALL be one program with subcommands. The logic of every
subcommand SHALL stay in a library below `tools/lib/` (or the
component's own library); the program's `main.c` SHALL contain only the
subcommand table and the call of the shared dispatcher, and each
subcommand's argument handling lives in its own source file next to it. A
program that links the TED core SHALL NOT be merged with a program that
does not. A program with one mode of use SHALL stay a program of its own.

#### Scenario: A theme has several tools
- **WHEN** two host programs read the same file kind and are called by the
  same build steps (for example the tools on Blitz library files)
- **THEN** they are subcommands of one program, and their libraries are
  linked into it unchanged

#### Scenario: A tool needs the TED core
- **WHEN** a tool links `tedhost` and `tedcore_host`
- **THEN** it is not a subcommand of a program that does not link them, and
  a program of its own (with its siblings that do) lives in
  `programs/ted/tools/`

#### Scenario: The main file stays thin
- **WHEN** a group's `main.c` is read
- **THEN** it holds a table of subcommand names, usage texts and
  functions and one call of the dispatcher, and no parsing or output of a
  subcommand

### Requirement: One subcommand syntax
A program with subcommands SHALL be called `PROGRAM SUB [OPTIONS] ARGS`.
The subcommand SHALL be the first argument; its name is lower-case,
words separated by `-`. Options are `-x` or `--name` flags that modify
one subcommand and follow it; a mode that was selected by a flag or by the
position of an argument in an earlier tool SHALL be a subcommand. A
program whose calls are all of one mode SHALL NOT have a subcommand.

#### Scenario: Subcommand call
- **WHEN** a library is checked
- **THEN** the call is `blitzlib check LIB.obj...` and not a flag or an
  argument position that selects `check`

#### Scenario: Unknown or missing subcommand
- **WHEN** a program is called without arguments or with a name that is
  no subcommand
- **THEN** it writes the program's usage to stderr and exits with
  status 2, and runs nothing

#### Scenario: A single-mode program
- **WHEN** a program has exactly one mode of use
- **THEN** it is called as `PROGRAM [OPTIONS] ARGS` and `-h` shows its one
  usage

### Requirement: Help for the program and for every subcommand
`PROGRAM -h` and `PROGRAM --help` SHALL write the usage of the whole
program to stdout, start with `usage: `, exit 0 and write nothing to
stderr; the text SHALL have one synopsis line per subcommand.
`PROGRAM SUB -h` and `PROGRAM SUB --help` SHALL do the same for that
subcommand alone.

#### Scenario: Program help
- **WHEN** `blitzlib -h` is run
- **THEN** stdout starts with `usage: blitzlib ` and has a synopsis line for
  each subcommand, the exit status is 0 and stderr is empty

#### Scenario: Subcommand help
- **WHEN** `blitzlib compare -h` is run
- **THEN** stdout starts with `usage: blitzlib compare `, the exit status is
  0 and stderr is empty

### Requirement: Every subcommand is covered by a help test
The host test of a program's `-h` SHALL run the program's help and the
help of every one of its subcommands, and fail if a subcommand of the
program's table has no test or a help does not meet the previous
requirement. The check itself SHALL have a test with a deliberately
broken program.

#### Scenario: A subcommand without working help
- **WHEN** a subcommand's help writes to stderr or exits with 1
- **THEN** the help test of its program fails and names the subcommand

#### Scenario: A subcommand missing from the test
- **WHEN** a subcommand is added to a program's table and not to the list
  its help test is given
- **THEN** the help test fails, because the program's `-h` has a synopsis
  line the test does not run

#### Scenario: The check is broken
- **WHEN** the help check runs on a program that misses the help of one
  subcommand
- **THEN** its own test sees the failure and passes only because of it

### Requirement: Exit codes and messages
Exit status 0 SHALL mean success, 1 that the checked thing failed or an
input could not be processed, 2 a usage error (also an unknown subcommand
or option), 77 that the run was skipped because an optional input or the
emulator is missing. Messages SHALL go to stderr in the form
`PROGRAM SUB: [FILE: ]message`, one per line; a result a caller reads
(a report, a table, the text of a conversion) goes to stdout or to the
named output file. Regrouping SHALL NOT change the exit status of any
existing input of a former program.

#### Scenario: Usage error
- **WHEN** a subcommand is called with the wrong number of arguments
- **THEN** its usage is written to stderr and the exit status is 2

#### Scenario: An unreadable input
- **WHEN** a subcommand is given a file that cannot be read
- **THEN** stderr has one line starting with `PROGRAM SUB: FILE: ` and the
  exit status is the one the former program returned for it

#### Scenario: Skipped
- **WHEN** a subcommand that needs data that is not checked in (the CD, the
  previous build) finds it missing
- **THEN** it exits with 77 and says what is missing on stdout

### Requirement: A README for every host program
Every host program SHALL have a `README.md` in its source directory with
the sections purpose, usage (the synopsis lines of `-h` verbatim in a code
block, then each subcommand and option), inputs and outputs (file
formats, with a link to `docs/formats.md` where one exists), exit codes,
callers (the CMake targets, scripts and tests that run it) and an example
that runs as written. A host test SHALL fail if the README does not
contain every synopsis line of the program's `-h`, or a synopsis line of
the README is no longer printed by `-h`. `docs/structure.md`,
`tests/README.md` and `docs/development.md` SHALL link to the README
instead of describing a tool's arguments inline.

#### Scenario: A new subcommand
- **WHEN** a subcommand is added and the README is not changed
- **THEN** the README test fails and names the missing synopsis line

#### Scenario: Documents link instead of repeating
- **WHEN** `docs/structure.md` or `tests/README.md` mentions a tool
- **THEN** it names the program, says in one line what for, and links the
  tool's README; the argument list is only in the README and in `-h`

#### Scenario: Callers are listed
- **WHEN** the README of `refcheck` is read
- **THEN** it names the CMake files that run each of its subcommands

### Requirement: A group is switched over completely in one commit
The commit that introduces a program with subcommands SHALL remove the
former programs and change, in the same commit, every call site: the CMake
variables and `BUILD_BYPRODUCTS`, `add_test` and `add_custom_command` calls,
the `*.cmake` scripts, the `help_*` tests, the path patterns of
`tools/covsel/rules.txt`, the documents and the READMEs. The former names
SHALL NOT remain as aliases, wrappers or links. After the commit nothing
runs, links or points to a former program by its name: no command line,
CMake variable, program path or document link. Names of ctest tests, of
libraries (`lib_NAME`) and of the library directories `tools/lib/NAME/`
MAY keep the name, as they are not programs.

#### Scenario: No call site left
- **WHEN** a group's commit is complete
- **THEN** a search of the repository for the former programs as command,
  `${VARIABLE}` or path (outside `archive/`, `TODO.md` and `openspec/`)
  finds nothing, a clean build has no warnings, and `ctest --preset host`
  passes

#### Scenario: One topic per commit
- **WHEN** the history of this change is read
- **THEN** each group is one commit, the shared infrastructure is a commit
  before the first group, and no commit mixes two groups

### Requirement: Regrouping changes no result
A group's commit SHALL leave byte-identical the executables the compilers
generate, the files TED writes, `acidlibs`, `deflibs` and all library
files, and SHALL change the behaviour of no subcommand beyond its name,
the position of its mode selector and its message prefix. It SHALL NOT
set `PREV_INTENDED`.

#### Scenario: Previous build
- **WHEN** a group's commit is built with `-DBLITZ2_PREV=ON`
- **THEN** `prev_results` passes without `PREV_INTENDED`, and the corpus
  and `ted_ab` prev configurations pass

#### Scenario: The suites still pass
- **WHEN** `ctest --preset all` is run on the commit
- **THEN** it has no failure that the commit before had not

#### Scenario: Same data from the same input
- **WHEN** a subcommand is run on an input with its new name
- **THEN** its output files and its stdout are byte-identical to those of
  the former program on the same input
