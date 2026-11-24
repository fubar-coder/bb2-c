# testreg

The registration of the Blitz tests that judge themselves (`ID.test.bas`,
change `basic-test-runner`; `tests/README.md`, "Blitz test sources and the
result file") at build time, without the emulator. For every source it

- preprocesses it and checks its form (`tools/lib/testpp`: the check
  calls, the blocks paired in source order with the case at fault named,
  no conditional compilation or loop crossing a block, only a comment
  after a block macro, no executable code outside the blocks, the case
  names against the token lists of `--tokens`);
- resolves its run configuration (`tools/lib/testconf`: the `tests.toml`
  files up to `root = true` and its own `ID.test.toml`, the matrix and its
  cells) and checks the check queries and case selectors of its
  assertions and deviations against the source;
- checks the test's own files (change `basic-test-local-data`): every
  file of the directory `ID.test/` beside the source is declared in the
  table `data` of `ID.test.toml`, and every declared one is there;
- registers one ctest test per case and side: `PREFIX_ID__CASE` for the
  cells without original material, `PREFIX_ID__CASE_originals` for those
  with it (a compiler other than the port, or the CD's libraries); a side
  without cells is not registered, a name given twice is an error.

The include file (for the directory property `TEST_INCLUDE_FILES`, as
`gtest_discover_tests` does it) holds per test an `add_test()` running
`COMMAND... --source SOURCE --case CASE --side port|originals` and a
`set_tests_properties()`: the labels (`originals` for the side with
original material, else `bug-orig` and `emulator` for a case with a
declared deviation of an open bug, `emulator` for the others, then the
configuration's `labels`, then `flaky` on both sides of a test with
the key `flaky`), `TIMEOUT` (the sum of the cells' `timeout`,
else `--timeout-default`, plus `--timeout-extra`), `SKIP_RETURN_CODE 77`,
`COST` and `FIXTURES_REQUIRED` from `cost` and `fixtures` (a test of the
label `bug-orig` without `cost`: the sum of its cells' session limits,
the time it can take, as it fails on purpose and ctest never learns its
cost), and the fixture of each `--cell-fixture SELECTOR|FIXTURE` whose
selector matches a cell of the side (the build uses
`compiler=orig195|originals_refbuild`: the build of `blitz2.s`, not built
by default). The tests of the label `bug-orig` come last, the largest
`COST` first, equal ones by name, together with the entries of `--order
FILE`: the bug-orig tests of the old form, which CMake records and writes
to that file (`cmake/bug-orig-order.cmake`, a line `# @test NAME COST`
before each declaration; `tests/README.md`, "Order of the bug-orig
tests"), so that all of them are declared in one sorted list. With
`--costs FILE` (`tests/data/test-costs.txt`, `tests/README.md`, "The
measured COST") a test with an entry gets the measured COST in place of
the sum of the session limits; the configuration's `cost` wins over the
entry. An entry that names no registered test is ignored (the COST only
orders the tests; the next full refresh of the file drops it). With `--test-env FILE` the include file includes `FILE` first and
calls `basic_test_environment(NAME LABELS)` after the properties of each
test it registers: the script `basic_test_register()` writes gives the
test the environment of the tests CMake declares (the snapshot owner, the
coverage directory; `emulation_test_environment`,
`cmake/emulation-queue.cmake`), which needs the labels only this step
knows. The depfile
names every file the tool read (the sources, the configuration files, the
files of each `ID.test/` and its directories, which a file added or
removed changes, the environment file, the token lists), so the build
step reruns when one of them changes. A depfile cannot name a file that does not exist yet: a
new `tests.toml` or `ID.test.toml` reaches the registration only through
a glob of the configuration files with `CONFIGURE_DEPENDS` next to the
one of the sources. The logic is `tools/lib/testreg`,
tested by `testreg_tool` (`tooltest.cmake`: the include file, the
depfile, every check with broken input, and sample CMake projects whose
`ctest -N` lists the tests after the build) and the host test `testreg`
(`tools/tests/test_testreg.cpp`: the bundle tests). Both are registered only
in `build/amiga/host` (in `build/amiga` they run together as the single
test `host`); run one with `ctest --test-dir build/amiga/host -R NAME`.

Bundle tests (change `basic-test-bundle-registration`; `tests/README.md`,
"Bundle tests"): unless `--no-bundle` is given, the case tests that may
share an emulator session with other sources (`tj_bundle_case`,
`tools/lib/testjudge`: one cell on the side port, no session of its own,
a plain session key, the case not alone in the run of all cases) are
grouped by their session key (`tc_session_key` with the environment of
`--env`), their labels and their fixtures, in the order of the sources and
cases, and each group is cut into chunks of 16. A chunk is the test
`PREFIXbundle_N` (N from 1 in the order of the groups, then the chunks):
`COMMAND... --bundle NAME` and `--member TEST SOURCE CASE` per member, the
group's labels and fixtures, `TIMEOUT` 3 x the key's time limit plus
`--timeout-extra` (the runner starts at most 3 sessions per chunk),
`SKIP_RETURN_CODE 77`, and as `COST` its entry of `--costs`, else the sum of
its members' COSTs; with the label `bug-orig` it is ordered with those
tests by that COST. Each member's case test keeps its name, labels,
`TIMEOUT` and `COST`; its command gains `--test TEST --from-bundle NAME`
(it is served from the bundle's stored verdict, or runs alone) and its
properties `DEPENDS NAME`, which orders it after the bundle but does not
select the bundle nor needs it to pass: `ctest -R CASE` runs the case
alone. Bundles stay within one run of the tool (one registration); a
bundle's name has no `__`, so it cannot be the name of a case test, and
is checked against the tests of `--order` like them.

## Usage

```
Usage: testreg [OPTIONS] SOURCE...

The ctest tests of the Blitz test sources of the model (ID.test.bas), one per
case and side, from the sources and their run configurations.

POSITIONALS:
  SOURCE FILE ... REQUIRED    the test sources (ID.test.bas)

OPTIONS:
  -h,     --help              Print this help message and exit
          --prefix P REQUIRED the prefix of the test names (bug_orig_, regression_, blitz2_)
          --output FILE REQUIRED
                              the ctest include file (TEST_INCLUDE_FILES)
          --command WORD REQUIRED
                              a word of the command of every test, once per word; then --source
                              SOURCE --case CASE --side SIDE
          --env FILE          the environment file of the build (placeholders, top)
          --depfile FILE      the depfile: every file read
          --tokens FILE       a token list (HEX NAME lines); case names must be no token
          --cell-fixture SPEC SELECTOR|FIXTURE: a test whose side has a cell the selector
                              matches requires the fixture (any number)
          --timeout-default SEC:POSITIVE
                              the time limit of a cell without timeout (default 90)
          --timeout-extra SEC:NONNEGATIVE
                              added to the sum of the cells' time limits (default 0)
          --order FILE        the bug-orig tests of the old form (cmake/bug-orig-order.cmake):
                              declared with the tests of the label bug-orig, the largest COST
                              first
          --costs FILE        the measured COST of the tests (tests/data/test-costs.txt): an
                              entry replaces the sum of the cells' session limits, the
                              configuration's cost wins over it; an entry that names no
                              registered test is ignored
          --test-env FILE     a ctest script that defines basic_test_environment(NAME LABELS),
                              the environment of a test (cmake/basic-test-model.cmake):
                              included by the include file, called for each test
          --no-bundle         no bundle tests: every case test runs alone (without it, the case
                              tests that may share an emulator session with other sources are
                              put into bundle tests PREFIXbundle_N of 16 at most, each served
                              from its bundle)

The include file is written only when every source and configuration passed the
checks; the depfile is written in any case. Exit status 0 ok, 1 a check failed
or a file cannot be read or written, 2 usage error.
```

The build runs it per directory through `basic_test_register()`
(`cmake/basic-test-model.cmake`, called by `tests/bug-orig/CMakeLists.txt`
and `tests/blitz2/CMakeLists.txt`) with the command `corpusrun case --env
ENV` (`tests/corpus/runner/caserun.h`), and with `--no-bundle` when the
cache option `BASIC_TEST_BUNDLE` is off or in a coverage build
(`BLITZ2_COVERAGE`: a served case would leave no coverage file of its
own).
