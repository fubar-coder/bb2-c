# tooling/host-tools Specification

## Purpose
How the repository's host programs (the tools of `tools/`, the TED tools,
the corpus and TED A/B runners) are organised, invoked and documented, so
that a developer finds a tool by its theme, calls every tool the same way
and reads what it does in its directory.

## Requirements

### Requirement: Host programs are grouped by theme
Host programs that work on the same kind of input or answer the same kind
of question SHALL be one program with subcommands. The logic of every
subcommand SHALL stay in a library below `tools/lib/` (or the
component's own library); the program's main source file SHALL contain
only the declaration of its subcommands and the call of the shared
command-line library, and each subcommand's options, arguments and
argument checks live in its own source file next to it. A program SHALL
NOT parse its command line by hand: options, positional arguments,
`-h`/`--help`, the selection of a subcommand and the errors of all of
them come from one shared command-line library. A program that links the
TED core SHALL NOT be merged with a program that does not. A program with
one mode of use SHALL stay a program of its own.

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
- **WHEN** a group's main source file is read
- **THEN** it holds the declaration of the subcommands and one call of the
  command-line library, and no parsing or output of a subcommand

#### Scenario: No hand-written option parsing
- **WHEN** the sources of the host programs are searched for a loop that
  compares `argv` entries with option names
- **THEN** there is none; a program with one mode (such as `amiga_run`,
  `mktestdata`, `hdftool` or `xtra`) uses the same library for its options

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
- **THEN** it writes a message to stderr that names the problem and the
  way to the help, exits with status 2, and runs nothing

#### Scenario: A single-mode program
- **WHEN** a program has exactly one mode of use
- **THEN** it is called as `PROGRAM [OPTIONS] ARGS` and `-h` shows its one
  usage

### Requirement: Help for the program and for every subcommand
`PROGRAM -h` and `PROGRAM --help` SHALL write the help of the whole
program to stdout, start with `Usage: `, exit 0 and write nothing to
stderr; the help SHALL name every subcommand with a one-line description.
`PROGRAM SUB -h` and `PROGRAM SUB --help` SHALL do the same for that
subcommand alone: its `Usage:` line, its positional arguments and every
option. The help SHALL be generated from the declaration of the options,
so that it cannot differ from what the program accepts.

#### Scenario: Program help
- **WHEN** `blitzlib -h` is run
- **THEN** stdout starts with `Usage: blitzlib `, names each subcommand
  with a description, the exit status is 0 and stderr is empty

#### Scenario: Subcommand help
- **WHEN** `blitzlib compare -h` is run
- **THEN** stdout starts with `Usage: blitzlib compare `, lists each option
  of the subcommand, the exit status is 0 and stderr is empty

#### Scenario: A program without subcommands
- **WHEN** `hdftool -h` or `amiga_run --help` is run
- **THEN** the help starts with `Usage: ` and lists every option of the
  program

### Requirement: Every subcommand is covered by a help test
The host test of a program's `-h` SHALL run the program's help and the
help of every one of its subcommands, and fail if a subcommand named in
the program's help has no test or a help does not meet the previous
requirement. This holds for every program with subcommands, `hdftool` and
`xtra` included. The check itself SHALL have a test with a deliberately
broken program.

#### Scenario: A subcommand without working help
- **WHEN** a subcommand's help writes to stderr or exits with a status
  other than 0
- **THEN** the help test of its program fails and names the subcommand

#### Scenario: A subcommand missing from the test
- **WHEN** a subcommand is added to a program and not to the list its help
  test is given
- **THEN** the help test fails, because the program's `-h` names a
  subcommand the test does not run

#### Scenario: The check is broken
- **WHEN** the help check runs on a program that misses the help of one
  subcommand
- **THEN** its own test sees the failure and passes only because of it

### Requirement: Exit codes and messages
Exit status 0 SHALL mean success, 2 a usage error (every command line the
parser rejects: an unknown subcommand or option, a missing or surplus
argument, a value of the wrong kind, a missing required option), 77 that
the run was skipped because an optional input or the emulator is missing.
Any other status is documented in the program's README and means that the
checked thing failed or an input could not be processed; a caller outside
the program SHALL depend on no status except 0, 2 and 77 unless the README
names it as a verdict of the program. A parse error SHALL write its message
and the way to the help to stderr and run nothing. Other messages SHALL go
to stderr in the form `PROGRAM SUB: [FILE: ]message`, one per line; a
result a caller reads (a report, a table, the text of a conversion) goes
to stdout or to the named output file.

#### Scenario: Usage error
- **WHEN** a subcommand is called with the wrong number of arguments, an
  unknown option, or an option without its value
- **THEN** a message on stderr names the problem, the exit status is 2 and
  nothing else is run or written

#### Scenario: An unreadable input
- **WHEN** a subcommand is given a file that cannot be read
- **THEN** stderr has one line starting with `PROGRAM SUB: FILE: ` and the
  exit status is the one the program's README documents for it

#### Scenario: Skipped
- **WHEN** a subcommand that needs data that is not checked in (the CD, the
  previous build) finds it missing
- **THEN** it exits with 77 and says what is missing on stdout

#### Scenario: Results do not depend on the command-line library
- **WHEN** a subcommand is run on an input with the options of the former
  version
- **THEN** its output files and its stdout, apart from help and error
  messages, are byte-identical to those of the former version

### Requirement: A README for every host program
Every host program SHALL have a `README.md` in its source directory with
the sections purpose, usage (the output of `PROGRAM -h` and of every
`PROGRAM SUB -h` verbatim in code blocks, then each subcommand and option
in words), inputs and outputs (file formats, with a link to
`docs/formats.md` where one exists), exit codes, callers (the CMake
targets, scripts and tests that run it) and an example that runs as
written. A host test SHALL fail if a help text of the program is not
contained verbatim in the README, or a usage block of the README is no
longer printed by the program. `docs/structure.md`, `tests/README.md` and
`docs/development.md` SHALL link to the README instead of describing a
tool's arguments inline.

#### Scenario: A new subcommand
- **WHEN** a subcommand is added and the README is not changed
- **THEN** the README test fails and names the missing help text

#### Scenario: A changed option
- **WHEN** an option is added, renamed or removed in a subcommand and the
  README's usage block of it is not changed
- **THEN** the README test fails and names the subcommand

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
