# testinfo

Reads the records of test runs: statistics of ctest runs from their JUnit
files (`stats`), the coverage of each test as lcov tracefiles (`map`) and
the tests a change can affect, from that coverage (`select`). The logic is
in `tools/lib/teststats` (with `statline.h`, the `@stat` line of the
runners' emulator sessions), `tools/lib/covmap` and `tools/lib/covsel`,
tested by `tools/tests/test_teststats.c`, `test_statline.c`,
`test_covmap.c` and `test_covsel.c`. How the coverage is measured and
used: `tests/README.md`, "Coverage"; the test statistics: `tests/README.md`,
"Test statistics".

## Usage

```
Usage: testinfo [OPTIONS] SUBCOMMAND

Records of test runs: statistics, the coverage of each test, the tests a change
can affect.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  stats                       statistics of ctest runs and their emulator sessions
  map                         the coverage of each test as lcov tracefiles
  select                      the tests a change can affect, from the coverage of a coverage
                              run
```

### stats

```
Usage: testinfo stats [OPTIONS] JUNIT.xml...

statistics of ctest runs and their emulator sessions

POSITIONALS:
  JUNIT.xml TEXT ... REQUIRED ctest's JUnit files, one per run

OPTIONS:
  -h,     --help              Print this help message and exit
          --tsv               tab-separated values instead of text tables
          --factor X:POSITIVE the factor of the time limits (default 3)
          --floor SEC:NONNEGATIVE
                              the smallest suggested limit (default 30)

Statistics of ctest runs (one JUnit file per run) and the @stat lines of the
emulator sessions in them: durations, sessions, flaky results, suggested time
limits (max timed x factor, at least the floor) and COST. Exit status 0, 1
problems in a file (reported on stderr), 2 usage error or a file that cannot be
read.
```

Every JUnit file is one run (`ctest --preset P` writes
`build/amiga/junit-P.xml`; keep runs apart with `--output-junit NAME`).
Reports per test the runs, their results and durations, the emulator
sessions of the `@stat` lines, flaky results, a suggested time limit (the
largest timed duration times the factor, at least the floor) and the COST
value.

* `--tsv`: tab-separated values instead of text tables;
* `--factor X`: the factor of the time limits (default 3);
* `--floor SEC`: the smallest suggested limit (default 30).

### map

```
Usage: testinfo map [OPTIONS]

the coverage of each test as lcov tracefiles

OPTIONS:
  -h,     --help              Print this help message and exit
          --commit SHA REQUIRED
                              the commit of the coverage run, written to DIR/commit
          --root DIR          the repository (default: the current directory)
          --drop PREFIX       leave out the files below PREFIX
          --deps FILE         ninja -t deps of a build (the source of each object)
          --linkmap PROGRAM FILE
                              the link map of an Amiga program
          --objsyms PROGRAM FILE
                              nm -A of its objects (static functions too)
          --lines PROGRAM FILE
                              objdump --dwarf=decodedline of its objects (line ranges)
          --gcov TEST FILE    gcov --json-format --stdout of a host test
          --cov TEST PROGRAM FILE
                              the coverage file a test left for PROGRAM
          --inputs FILE       inputs, one per line: the option without "--" and its arguments,
                              separated by tabs
  -o DIR REQUIRED             the output directory

Writes the coverage of each test as lcov tracefile DIR/lcov/<test>.info, all of
them as DIR/lcov/all.info, and SHA as DIR/commit; files outside the root and
below a --drop prefix are left out. Exit status 0 if they are written, 1 if an
input is broken (the message on stderr), 2 usage error or a file that cannot be
read or written.
```

Writes `DIR/lcov/<test>.info` per test, `DIR/lcov/all.info` with all tests
and `DIR/commit` with `SHA`. The inputs (each may be given several times):

* `--deps FILE`: `ninja -t deps` of a build (the source of each object);
* `--linkmap PROGRAM FILE`: the link map of an Amiga program;
* `--objsyms PROGRAM FILE`: `nm -A` of its objects (static functions too);
* `--lines PROGRAM FILE`: `objdump --dwarf=decodedline` of its objects
  (line ranges);
* `--gcov TEST FILE`: `gcov --json-format --stdout` of a host test;
* `--cov TEST PROGRAM FILE`: the coverage file a test left for `PROGRAM`;
* `--inputs FILE`: inputs, one per line: the option without `--` and its
  arguments, separated by tabs.

`--root DIR` is the repository (default: the current directory); files
outside it and below a `--drop PREFIX` are left out.

### select

```
Usage: testinfo select [OPTIONS]

the tests a change can affect, from the coverage of a coverage run

OPTIONS:
  -h,     --help              Print this help message and exit
          --coverage DIR REQUIRED
                              testinfo map's output: DIR/commit, DIR/lcov/<test>.info
          --rules RULES REQUIRED
                              what a change of a file selects (tools/testinfo/rules.txt)
          --tests JSON REQUIRED
                              ctest --show-only=json-v1 of the build
          --host-tests JSON   the same of its host project (tests host/NAME, run by host or
                              host_emulator)
          --diff DIFF         git diff -U0 against the map's commit
          --deps DEPS         ninja -t deps of a build
          --untracked LIST    files git does not track, one per line (new files)
          --stale             the map's commit is not an ancestor of the base: all tests
          --root DIR          the repository (default: the current directory)
  -o SELECTED REQUIRED        the file of the selected tests

Writes the selected tests to SELECTED, one per line (for ctest
--tests-from-file), and why each one is selected to stdout. Exit status 0, 1 if
an input is broken (the message on stderr), 2 usage error or a file that cannot
be read.
```

Selects the tests a change can affect: the tests that executed a changed
function by the map of `map` (`--coverage DIR`), and what
`RULES` (`tools/testinfo/rules.txt`, the first matching pattern counts)
says for the other files. Writes the selected tests to `SELECTED`, one
per line (for `ctest --tests-from-file`), and why each one is selected to
stdout.

* `--tests JSON`: `ctest --show-only=json-v1` of the build;
* `--host-tests JSON`: the same of its host project (tests `host/NAME`,
  run by `host` or `host_emulator`);
* `--diff DIFF`: `git diff -U0` against the map's commit;
* `--deps DEPS`: `ninja -t deps` of a build;
* `--untracked LIST`: files git does not track, one per line (new files);
* `--stale`: the map's commit is not an ancestor of the base: all tests;
* `--root DIR`: the repository (default: the current directory).

## Inputs and outputs

`stats` reads ctest's JUnit files and writes its tables to stdout; a
problem in a file (a malformed file, a testcase without name or time, a
broken `@stat` line) goes to stderr, the rest is reported. `map` writes the
lcov tracefiles (format: `tools/lib/covmap/covmap.h`) and the commit file;
`select` reads them and writes the list of tests.

## Exit codes

* 0 success;
* 1 `stats`: problems in a file (reported); `map`, `select`: an input is
  broken (the message on stderr);
* 2 usage error; also a file that cannot be read (`stats`, `map`,
  `select`) or written (`map`), and running out of memory.

Messages go to stderr as `testinfo SUB: [FILE: ]message`.

## Callers

* `stats`: by hand (`tests/README.md`, "Test statistics").
* `map`: `cmake/coverage-map.cmake` (after a coverage run,
  `tests/README.md`, "Coverage").
* `select`: `cmake/coverage-select.cmake`, with `tools/testinfo/rules.txt`.
* Tests: `help_testinfo`, `readme_testinfo` (`tools/testinfo/CMakeLists.txt`);
  the libraries: `teststats`, `statline`, `covmap`, `covsel`
  (`tools/tests`; `covsel` reads `rules.txt` of this directory).

## Example

```
$ testinfo stats build/amiga/junit-host.xml | head -5
1 runs (JUnit files), 76 tests

== Tests: runs and durations (seconds)
test                                 runs  passed  failed  skipped  min/median/max
host                                 1     1       0       0        39.3 / 39.3 / 39.3
```
