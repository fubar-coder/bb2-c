# corpusrun

Runs the emulator passes of the compiler tests in Amiberry: one pass of
the corpus check (`pass`: the same sources compiled by several
configurations of compiler and libraries, the programs run, the results
compared with the first configuration, or in the bug-orig mode with the
expectation), and the whole CD through one compiler (`cd`: a table of the
results, optionally compared with a reference table). The logic is in
`lib_corpus` (`corpus.h`, `bugorig.h`, `serial.h`), `lib_cdcache`
(`cdcache.h`) and `lib_runner_util` (`runwork.h`: the run directory of
`pass` and of `tedab run`, a directory of its own per run below `work=`;
`rundir.h`: the run directory where a test hands data to another),
tested by `test_corpus.c`, `test_bugorig.c`, `test_cdcache.c`,
`test_runwork.c` and `test_rundir.c` here (ctest `corpus_runner`,
`bugorig_verdict`, `cdrun_cache`, `runner_runwork`, `runner_rundir`);
`case`, one case of a test of the BASIC test model, is `lib_caserun`
(`caserun.h`, tested by `test_caserun.c`, ctest `corpusrun_caserun`), its
verdict store of the bundles `lib_verdicts` (`verdicts.h`, tested by
`test_verdicts.c`, ctest `corpusrun_verdicts`).

## Usage

```
Usage: corpusrun [OPTIONS] SUBCOMMAND

The corpus passes and the CD runs in Amiberry.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  pass                        one pass of the corpus check in Amiberry: compile, run and
                              compare the sources of a pass file
  cd                          compile every .bb2/.bb of the CD with one compiler in Amiberry, a
                              table of the results
  drop                        remove the data a pass published for other tests (the cleanup of
                              a ctest fixture)
  case                        one case of a Blitz test of the BASIC test model (ID.test.bas),
                              or all its cases, in Amiberry: the cells of one side compiled,
                              run and judged

The exit status of pass in a bug-orig pass is the worst verdict: 0 FIXED, 1
REPRODUCED, 3 BROKEN, 4 DIFFERS.
```

### pass

```
Usage: corpusrun pass [OPTIONS] PASSFILE

one pass of the corpus check in Amiberry: compile, run and compare the sources
of a pass file

POSITIONALS:
  PASSFILE TEXT REQUIRED      the pass file (its keys: tests/corpus/runner/corpus.h)

OPTIONS:
  -h,     --help              Print this help message and exit
          --update-expect TAG write the expected data of the pass's expect_dir= from the
                              configuration TAG instead of comparing
          --fix PASS:NAME#N   with --update-expect: a program a fix makes differ from the
                              original (any number): its changed file becomes the pair
                              NAME.orig.EXT / NAME.fixed-N.EXT
          --observe TAG       run only the configuration TAG and write its observations to --to
                              DIR (NAME.out, NAME.serial); not for a pass with orig=, expect=
                              or record=
          --to DIR            with --observe: the directory of the observations (emptied first)
          --layout-stack      with --observe: a Stack 16384 bytes larger (the second run of the
                              record: another memory layout)
          --only NAME         only this source (NAME without .bb2; any number)

With the pass key record=check a bug-orig pass (orig=) runs only the original
and compares its observation with the recorded one (1 if it differs), with
record=write it writes the observation to record_to= (the target
bug-orig-record). Exit status 0 no differences (--update-expect: all written;
--observe: observed), 1 differences (the report; --update-expect: a program
refused, nothing written for it), 2 error (usage, pass file, a malformed
expect_dir=), 77 skipped (no emulator, or the CD's libraries are missing). A
bug-orig pass gives the worst verdict: 0 all FIXED, 1 REPRODUCED, 3 BROKEN, 4
DIFFERS.
```

Runs one pass of the pass file `PASSFILE` and writes the report (to
`report=` and stdout), then one `@stat` line per emulator session
(`tests/README.md`, "Test statistics"). With the pass key `record=check`
a bug-orig pass (`orig=`) runs only the original and compares its
observation with the recorded one, `orig=`, which it never writes (the
compare test `bug_orig_record_ID`; `tests/README.md`, "Recorded
originals"); with `record=write` it writes the observation to
`record_to=` in the build directory, with `layout=stack` the session's
Stack is larger, so that the compiler's and the program's memory lies
elsewhere (the two recording runs of the target `bug-orig-record`). A pass with
`expect_dir=DIR` compares the output of every configuration (and with
`serial=on` the bytes sent over the serial port) with the committed
expected data of `DIR` (`tools/lib/corpusexpect`, `tests/README.md`,
"Corpus"): the unqualified file `DIR/NAME.out` (`DIR/NAME.serial`), or
for a program a fix makes differ from the original the file of the pair
that applies to the configuration: `NAME.orig.out` for `ref`,
`NAME.fixed-N.out` for `clib` and `prev`, either for `port` and
`orig-clib` (the report says which matched, `(matched [fixed #N])`). A
configuration `exeinfo` (`port`) is judged by its output like the others
and its executable is compared with `ref`'s for information (a different
one is reported, none fails), as against the original only outputs and
errors count; a `NAME.exe` file is refused. A malformed
directory or a missing file fails the pass before any session; a pass
with `expect_dir=` may have the reference as its only configuration.
`unjudged=TEST:CONFIG:NAME|REASON` (any number, from `CORPUS_UNJUDGED`)
leaves the output of source `NAME` in configuration `CONFIG` unjudged: the
report says `not judged` and gives `REASON`; an entry of another `test=`,
configuration or source is an error before any session, one whose output
is as expected again is `STALE` and fails the pass.

Options:

* `--update-expect TAG`: write the expected data of `expect_dir=` from
  the configuration `TAG` instead of comparing (the target
  `corpus-expect-update`, `tests/README.md`, "Corpus"). With `clib` only
  it runs: a new program gets `NAME.out` (`NAME.serial`), a
  `NAME.fixed-N.out` is rewritten, and a changed `NAME.out` is refused
  with the program's name and the first differing line unless
  `--fix PASS:NAME#N` names the program, which makes it the pair
  `NAME.orig.out` (the old content) and `NAME.fixed-N.out` (the port's).
  A source that does not compile, has no output or times out is
  refused.
* `--fix PASS:NAME#N`: with `--update-expect`, a program a fix makes
  differ from the original (`PASS` the name of `expect_dir=`; any number).
* `--observe TAG --to DIR`: run only the configuration `TAG` and write
  its observations to `DIR` (emptied first): `NAME.out` and `NAME.serial`;
  not for a pass with `orig=`, `expect=` or
  `record=` (refused, 2). `--layout-stack` gives the session a
  Stack 16384 bytes larger: the two runs of the target
  `corpus-expect-record`, accepted by `results corpus-accept`.
* `--only NAME`: only this source of the pass (any number).

### cd

```
Usage: corpusrun cd [OPTIONS] PASSFILE

compile every .bb2/.bb of the CD with one compiler in Amiberry, a table of the
results

POSITIONALS:
  PASSFILE TEXT REQUIRED      the pass file (written by CMake; its keys: the header of
                              tests/corpus/runner/cd.c)

OPTIONS:
  -h,     --help              Print this help message and exit

The table is optionally compared with a reference table. CDRUN_FORCE=1 in the
environment runs even if the cache= record says nothing changed. Exit status 0
written (and no difference), 1 differences or a source= that does not compile, 2
error (usage, pass file), 77 the emulator or a file is missing (skipped).
```

Compiles every `.bb2`/`.bb` of the CD with one compiler and writes the
table `output=`; with `reference=` it compares the table with it and
writes the comparison to `report=` and stdout. With `source=REL|OUT` only
the named sources are compiled and their executables copied to `OUT` (the
RWE reference build). With `cache=PATH` a successful run records its
inputs and outputs; the next run with the same inputs and unchanged
outputs ends at once without the emulator (`CDRUN_FORCE=1` in the
environment runs anyway).

### drop

```
Usage: corpusrun drop [OPTIONS] NAME...

remove the data a pass published for other tests (the cleanup of a ctest
fixture)

POSITIONALS:
  NAME TEXT ... REQUIRED      the fixtures (the publish= of the passes)

OPTIONS:
  -h,     --help              Print this help message and exit
          --run-dir DIR       the run directory when the environment variable BB2C_TEST_RUN is
                              not set (the default one is removed with Testing/runs if empty)

A fixture that a failed test read stays (the next run of the passes that make it
replaces it); a directory given by BB2C_TEST_RUN is the caller's: only the
fixtures are removed from it. Exit status 0 done (also nothing to remove), 1 a
fixture could not be removed.
```

The cleanup of a ctest fixture (`tests/corpus/runner/rundir.h`): a pass
with `publish=NAME` puts its passing run in the run directory as
`NAME`, the tests that read it name their input relative to it, and
`drop` removes `NAME` after the last reader unless a failed reader
marked it. The run directory is the environment variable
`BB2C_TEST_RUN`, else `--run-dir` (the default directory of the build,
which is removed again with `Testing/runs` if empty).

### case

```
Usage: corpusrun case [OPTIONS]

one case of a Blitz test of the BASIC test model (ID.test.bas), or all its
cases, in Amiberry: the cells of one side compiled, run and judged

OPTIONS:
  -h,     --help              Print this help message and exit
          --env FILE REQUIRED the environment file of the build (tests/model-env.toml)
          --source FILE Excludes: --bundle
                              the test source ID.test.bas (not with --bundle)
          --case CASE Excludes: --bundle
                              the case; without it every case of the source (not with
                              --print-config, --write-driver, --prev)
          --side SIDE:{port,originals,oracle}
                              the cells without original material (port, the default), with it
                              (originals: the original compilers, the CD's libraries), or of
                              AmiBlitz3's compiler (oracle: compiler = "amiblitz3")
          --require-deviation a bug-orig test: a test that declares no deviation of the
                              original is broken
          --print-config      print the effective configuration (every key with its value and
                              origin, the axes, the cells) and run nothing
          --print-driver      print the driver of the case (without --case of every case, the
                              text --write-driver writes) and run nothing
          --write-driver FILE write the driver of all cases, tokenised, to FILE (its text
                              beside it as .bas) and run nothing: the build step of the
                              comparison with the prev build
          --prev              compare with the prev build: the driver of --driver compiled by
                              this build's compiler and by --prev-blitz2, once per value of
                              runtime-errors of the side port (the compiles ID and ID-runerrs)
          --driver FILE       with --prev: the driver --write-driver wrote
          --test TEST         with --prev: the ctest name, the TEST of the PREV_INTENDED
                              entries; with --from-bundle the case test's name
          --prev-blitz2 FILE  with --prev: the previous commit's compiler
          --prev-libs DIR     with --prev: the directory of its deflibs and acidlibs
          --intended ENTRY    with --prev: a PREV_INTENDED entry (TEST:NAME of any test, or ON;
                              any number)
          --from-bundle NAME Needs: --case --test
                              with --case and --test: the bundle that may have stored the
                              case's verdict; a fresh one is printed and is the exit status,
                              else the case runs alone
          --bundle NAME Excludes: --case --source
                              run the members (--member) in shared emulator sessions and store
                              each clean PASSED or REPRODUCED for its case test
          --member TEST SOURCE CASE Needs: --bundle
                              with --bundle: a member, the case test's name, its source (an
                              absolute path) and the case (any number)

The cells of a side that share the session settings run in one emulator session.
Without --case every case of the source runs: the cases share one run of the
driver of all of them, each judged from its own block; a case that asserts an
observation of the whole program or announces a runtime error, the case open
when a shared run did not end cleanly, and a case that did not pass in a shared
run run on their own ("(alone)"), and a line per case and a summary line report
them. Exit status: 0 passed, 1 reproduced (an open bug shows as declared; on the
side originals 0, the report saying "REPRODUCED (as declared, side originals)"),
2 error (a configuration, source or result that cannot be judged; usage), 3
broken, 4 failed, 77 skipped (no emulator, or a file of the side is missing);
without --case the worst verdict of the cases, in the order error, broken,
failed, skipped, reproduced, passed. With --prev: 0 the same, 1 a difference
that is not intended or a stale entry, 2 error, 77 skipped. With --bundle: 0 it
ran through (whatever its members' verdicts), 77 no session started (no
emulator, no member that can run), 2 an error of the bundle.
```

Runs one case of a test of the BASIC test model (`ID.test.bas` and its
run configuration; `tests/README.md`, "Tests that judge themselves") in
the cells of one side and judges them (`caserun.h`; the logic of the
configuration, the preprocessor, the driver and the judge is in
`tools/lib/testconf`, `testpp`, `testjudge`). The ctest tests the build
registers (`tools/testreg`, `cmake/basic-test-model.cmake`) call it as
`corpusrun case --env ENV [--require-deviation] --source SOURCE --case
CASE --side SIDE`. It preprocesses the source, writes the driver with
the result file `CORPUS:out/test.res` and tokenises it (`tedsrc tok`),
writes a pass of `corpus.h` per cell (pass key `cell=on`) and stages it,
and runs the cells whose session settings agree in one emulator session:
each cell's directory is the volume `CELLk:`, `Assign CORPUS: CELLk:`
selects it, the compiles of all cells come first, then the runs. A
session that ends early gives its end to the first cell that did not
finish; the cells behind it run again. The report names each failed
check with its case, name, source line, expression and values, a compile
error with `ID.test.bas:LINE`, and the verdict; the run directory below
the environment's `work` stays when the case did not pass.
`--print-config` prints the effective configuration (every key with its
value and the file and line it comes from, the axes, the cells and their
side), `--print-driver` the driver (without `--case` the driver of all
cases, the text `--write-driver` writes); neither starts a session. A cell
that the run setting `isolate` names (`"on"`, or a list of cell selectors)
gets a session of its own (`tc_session_key`). The session settings are
the cell's after the environment's: a cell without `timeout` has the
environment's `[runner] timeout`, so it shares a session with a cell that
sets `timeout` to that value. A cell whose program runs
through inject (`pad`, or a `runwrap` of inject) has inject's report, the
output's `inject:` lines, as its query `inject`; a cell whose compiler
runs through inject (a `wrap` of inject) has the `inject:` lines of the
compiler's output `blitz2.out` as its query `compile.inject`.

A cell with `editor-run = "on"` (change `basic-test-editor-run`) runs the
driver from the editor instead of from the shell: its pass has
`editrun=on`, and `corpus.c` appends to its job list, behind the jobs and
the `jobline=` lines, `run=r001|CORPUS:src/test.bb2|CORPUS:out/test.con`
(tedstub's COMPILE/RUN, the editor's console the file `test.con`) and
writes no shell run for it. The output of such a cell is `test.con`; its
exit code and requesters are not captured (the defaults of `rc` and
`requester` do not apply). The cell has ended when `blitz2.out` ends with
`rc=`, and the hang check of `hang_from=run` arms on `test.con`, which
tedstub opens when the program starts. The run setting `defaultdbug` is
the cell's `blitz2:defaultdbug` (it replaces the runner's `cd-defaultdbug`
of a cell with original material); a missing module skips the side (77).
In a `jobline` element `@DRIVER@` stands for the driver's source on the
cell's volume (`CORPUS:src/test.bb2`; in the comparison with the prev
build the driver of that pass). The observation of a cell from the files
of its volume is `cr_observe`.

A test with data of its own (the directory `ID.test/`, its table `data`;
change `basic-test-local-data`) has them prepared once in `data/` of the
run directory: a data source (`basic-compile`) preprocessed (its
placeholders, no check) and tokenised with the cells' token list, a
`.spec` made by `mktestdata`, a copied file as it is. Each cell's pass
gets them (`cr_data_pass`): a file as `file=ID.test/NAME|...`, a data
source of the cell's own build configuration (compiler, libraries,
runtime error checking) as a job of the cell's job list
(`build_job=-|SRC|ID.test/NAME`), and every other build configuration as
a compile part of its own (`build=ID.test/.build-N|BLITZ2|DEFLIBS|ACIDLIBS|on|off`:
its compiler, `ted` and libraries in `CORPUS:ID.test/.build-N/blitz2/`,
its job list and log there, compiled before the cell's compile in the
cell's session) with its jobs (`build_job=ID.test/.build-N|SRC|ID.test/NAME`).
The jobs are `bNNN`, in the order of the data sources; their results are
the query `data.FILE` (`ok`, `error LINE MESSAGE` with the line of the
data source, `none`), by default `== ok`: a result its assertion does not
ask for makes the cases of the cell BROKEN, the report naming
`ID.test/FILE:LINE` and the message. In a `jobline` element `@DATA:FILE@`
stands for the staged source of the data source `FILE`
(`CORPUS:src/bNNN.bb2`, or `CORPUS:ID.test/.build-N/src/bNNN.bb2` in a
compile part; `cr_data_staged`, from the count `cr_data_pass` uses).

The compile jobs of a cell have names (change `named-compile-jobs`):
`j000`, `r001` of a run from the editor, the IDs of the `jobline` jobs,
the file of a data source. `cr_observe` gives the judge the block of each
in tedstub's log (`bugorig_block` in `bugorig.c`, beside
`bugorig_compile`: the lines of the job's steps, the size of
`RESULT ID ok` removed; for a data source from the log of the session
that compiled it, its `bNNN` written as its file and its staged source as
`@DATA:FILE@`) as the query `compile.log.NAME`, and the compile result of
a `job=` or `resident=` job of `jobline` as `compile.NAME`.

Without `--case` it runs every case of the source on the side (`caserun.h`,
"The run of all cases"): the cases share a run of the driver of all of them
and each is judged from its own block; a case that asserts an observation
of the whole program or announces a runtime error runs on its own from the
start (`tj_case_alone` of `tools/lib/testjudge`; it looks at each cell in
the one mode its verdict uses, so the deviations of the original's cells
do not set a case of a fixed bug apart on the port), a shared run that does not end cleanly runs the case open at its
end on its own and the others again, and a case that does not pass in a
shared run is confirmed on its own. In the cells `isolate` names every case
runs on its own. The report has a block per case (` (alone)` when judged
on its own, a line when the verdict on its own differs from the shared
one) and the line `ID.test.bas: N cases, side SIDE: VERDICT`; the exit
status is the worst verdict (error, broken, failed, skipped, reproduced,
passed). On the side originals an open bug that shows as declared passes:
the verdict lines say `REPRODUCED (as declared, side originals)` and the
exit status is 0, so the preset `originals` is green while the original
shows the bug; the side port of the same case still fails with 1 (preset
`bug-orig`). The runs are `shared-N`, `alone-CASE` and `isolated-CASE` in the
run directory of `ID` (`ID_originals`).

The comparison with the prev build of a test of the model (`tests/README.md`,
"The prev build"; `cmake/prev.cmake`, `prev_compile_model_test`) has two
steps: the build step writes the driver of all cases, tokenised
(`--write-driver NAME.bb2`, its text beside it as `NAME.bas`; a source
whose driver cannot be written fails it), and the test `PREFIX_ID_prev`
(`--prev --driver NAME.bb2 --test PREFIX_ID_prev --prev-blitz2 FILE
--prev-libs DIR [--intended ENTRY]...`) compiles that driver once per
value of `runtime-errors` among the cells of the side port, a pass of
`corpus.h` with `run=off`, `clib` and `prev|exe:clib` each (`cr_prev_pass`;
without the keys of a run, also without `editrun` and `defaultdbug`: a
test that runs from the editor is compiled as any other):
the compiles `ID` and `ID-runerrs`, the PREV_INTENDED entries
`PREFIX_ID_prev:ID` and `:ID-runerrs`. The data sources of the test are
compiled in the pass of their setting of runtime error checking
(`cr_prev_data`, `source=PATH|NAME`): `ID/FILE` and `ID/FILE-runerrs`.
An entry of the test that names no compile of it is stale
(`cr_prev_unknown`).

A bundle (`--bundle NAME --member TEST SOURCE CASE ...`; `caserun.h`,
"The bundles"; change `basic-test-bundle-runner`) runs the cases of
several sources in shared emulator sessions, so that a case test is not
slower than it would be alone. Each member is a case with one cell on the
side port that `tj_bundle_case` (`tools/lib/testjudge`) admits; it is
prepared as for its run alone, in the directory `ID__CASE` of the
bundle's run directory below `WORK/NAME`. The cells are grouped by their
session key in the members' order and cut into chunks of at most 16
(`cr_bundle_chunks`). A chunk's session has one writable volume `BUNDLE:`
(the directory `bundle` of the run directory), whatever its number of
cells, so every session of a key has the same emulator configuration and
snapshot: each cell's staged volume is renamed to `bundle/cK` before the
session and back after it, the script starts with `Assign CELLk:
BUNDLE:ck` for every cell (`cr_session_script`), and the hang check arms
on the first cell's run (`bundle/c0/out/test.run`). A cell whose mount is
named `BUNDLE` is refused (`cr_bundle_mount_ok`). After a session that
ends before a cell completed, the cells behind it run again, at most 3
sessions per chunk (`cr_bundle_pending`); the cells not reached then are
left to their case tests. Each member is observed and judged as the run
of its case alone; a PASSED or REPRODUCED from a cell that ended cleanly
(`cr_cell_clean`) is stored for its case test, every other member is
named with the reason (`TEST: VERDICT not stored (REASON)`). The report
has each member's block with its verdict line marked ` (bundle NAME)`, a
line `TEST: VERDICT stored`, and `NAME: N members, M stored`; the `@stat`
lines carry `test=NAME` and `config=sN`, and before each session a line
`bundle cell cK: TEST` names each cell.

The verdict store (`verdicts.h`) is the directory `verdicts` of the run
directory of the fixtures (`rundir.h`: `BB2C_TEST_RUN`, else the
environment's `[runner] run-dir`), one file per stored case named by its
case test, written as `TEST.new-PID` and renamed. An entry holds the key
of the inputs it was judged from (`vstore_key`: a sha256 over the
digests of the source, the configuration files `tc_resolve` read, the
files below `ID.test/`, the environment file and every file a `[runner]`
value names, the files the cell's pass stages or runs, the running
`corpusrun`, and the emulator's inputs: the snapshot key of the session
without a session, `amiga_emulation_inputs_key`, and the session key),
the bundle, the case test's exit status and the report block. A case
test with `--test TEST --from-bundle NAME` claims its entry before its
run directory (renamed to `TEST.claim-PID`, read, removed): a fresh entry
of that bundle is printed with `corpusrun case: verdict from the bundle
NAME` and `@served v=1 test=TEST bundle=NAME` (no `@stat` line, no
session) and its status is the exit status; without an entry, with one of
other inputs or of another bundle, or a malformed one (named) the case
runs alone as without `--from-bundle`. An entry is read once. A bundle
removes its members' entries at its start, but not the files of a process
that still lives.

## Inputs and outputs

The pass files are written by CMake, lines `KEY=VALUE` and `#` comments;
the keys of `pass` are described in `corpus.h`, those of `cd` in the
header of `cd.c`. The table of `cd` is TSV (Latin-1): path, result,
line, message, size, masked SHA-256, xtra, note
(`tools/lib/cdtable/cdtable.h`; the reference is
`tests/data/blitz2-cd.tsv`); the cache record is described in
`cdcache.h`.

## Exit codes

* `pass`: 0 no differences, 1 differences (report), 77 skipped (no
  emulator, or the CD's libraries are missing), 2 error. In the bug-orig
  mode the verdict (`corpus.h` `expect=`, `bugorig.h`): 0 every
  configuration FIXED, 1 REPRODUCED (or REGRESSED for a fixed test or
  an expect test), 3 BROKEN, 4 DIFFERS, 2 error; a recording run 0, 2 if
  the session failed; `--update-expect` 0 every program written, 1 a
  program refused (nothing written for it), 2 error (also a pass without
  `expect_dir=` or without `TAG`, a malformed directory); `--observe` 0
  observed, 2 the session failed.
* `drop`: 0 removed or kept or nothing to remove, 1 a fixture could not be removed.
* `case`: the verdict of the case: 0 passed, 1 reproduced (an open bug
  shows as declared; on the side originals 0, reported as `REPRODUCED (as
  declared, side originals)`), 2 error, 3 broken, 4 failed, 77 skipped (no
  emulator, or a file of the side is missing); without `--case` the worst
  verdict of the cases (SKIPPED between failed and reproduced). With `--prev` 0 the same, 1 a difference
  that is not intended or a stale entry, 2 error, 77 skipped; with
  `--write-driver` 0 written, 2 error. With `--from-bundle` and a fresh
  entry the stored status. With `--bundle` 0 it ran through (whatever its
  members' verdicts), 77 no session started (no emulator, no member that
  can run), 2 an error of the bundle (the store cannot be written, a volume
  that cannot be renamed back, a wrong command line, out of memory).
* `cd`: 0 written (and no difference), 1 differences or a `source=` that
  does not compile, 77 the emulator or a file is missing (skipped),
  2 error.

Messages go to stderr as `corpusrun SUB: [FILE: ]message`; the log lines
of `cd` (sessions, skipped runs) go to stdout with the same prefix.

## Callers

* `pass`: the corpus tests (`NAME` with clib and prev, `NAME_originals` with
  ref, port and orig-clib) and the targets `corpus-expect-update`
  (`tests/corpus/update.cmake`) and `corpus-expect-record`
  (`tests/corpus/record.cmake`), `tests/ab3`,
  `tests/blitz2`, the bug-orig tests and their recordings
  (`tests/bug-orig/CMakeLists.txt`, also through `want.cmake`), the test
  `rwe_residents_cd` (`libs/userlibs/rwe/residents.cmake`,
  `residents-cd.cmake`).
* `cd`: the tests `corpus_cd`, `corpus_cd_ab3` and the targets
  `blitz2-cd-reference`, `blitz2-cd-update` (`tests/corpus/CMakeLists.txt`);
  the test `rwe_refbuild` (`libs/userlibs/rwe/refbuild.cmake`); the test
  `cdrun_source` (`check-cdrun-source.cmake` here, without emulator).
* `case`: the tests of the BASIC test model (`PREFIX_ID__CASE`,
  `PREFIX_ID__CASE_originals`; `cmake/basic-test-model.cmake`), their
  drivers for the comparison with the prev build (`basic_test_driver`)
  and the comparisons `PREFIX_ID_prev` (`cmake/prev.cmake`), the self
  tests `blitz2_selftest_model_*` (`tests/blitz2/CMakeLists.txt`), the
  self test of the bundles `corpusrun_bundle` (`bundle-selftest.cmake`,
  `tests/corpus/CMakeLists.txt`) and the refused command lines
  `corpusrun_bundle_refused_*`.
* `drop`: the tests `corpus_cleanup`, `corpus_originals_cleanup`
  (fixture cleanups, `tests/corpus/CMakeLists.txt`).
* Tests: `help_corpusrun`, `readme_corpusrun`.

## Example

```
$ corpusrun cd build/amiga/tests/corpus/corpus_cd.pass
```
