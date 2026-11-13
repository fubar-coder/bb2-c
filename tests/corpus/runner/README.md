# corpusrun

Runs the emulator passes of the compiler tests in Amiberry: one pass of
the corpus check (`pass`: the same sources compiled by several
configurations of compiler and libraries, the programs run, the results
compared with the first configuration, or in the bug-orig mode with the
expectation), and the whole CD through one compiler (`cd`: a table of the
results, optionally compared with a reference table). The logic is in
`lib_corpus` (`corpus.h`, `bugorig.h`, `serial.h`) and `lib_cdcache`
(`cdcache.h`), tested by `test_corpus.c`, `test_bugorig.c` and
`test_cdcache.c` here (ctest `corpus_runner`, `bugorig_verdict`,
`cdrun_cache`).

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
                              configuration TAG instead of comparing (port: the executables of
                              the reference and TAG)
          --fix PASS:NAME#N   with --update-expect: a program a fix makes differ from the
                              original (any number): its changed file becomes the pair
                              NAME.orig.EXT / NAME.fixed-N.EXT
          --observe TAG       run only the configuration TAG and write its observations to --to
                              DIR (NAME.out, NAME.serial, NAME.exe: the SHA-256 of the
                              executable)
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
program with the pair `NAME.orig.exe` / `NAME.fixed-N.exe` has the
SHA-256 of `ref`'s executable judged against the first and that of
`port` against the second instead of comparing the two. A malformed
directory or a missing file fails the pass before any session; a pass
with `expect_dir=` may have the reference as its only configuration.

Options:

* `--update-expect TAG`: write the expected data of `expect_dir=` from
  the configuration `TAG` instead of comparing (the target
  `corpus-expect-update`, `tests/README.md`, "Corpus"). With `clib` only
  it runs: a new program gets `NAME.out` (`NAME.serial`), a
  `NAME.fixed-N.out` is rewritten, and a changed `NAME.out` is refused
  with the program's name and the first differing line unless
  `--fix PASS:NAME#N` names the program, which makes it the pair
  `NAME.orig.out` (the old content) and `NAME.fixed-N.out` (the port's).
  With `port` (a configuration compared with the reference by its
  executable) the reference and `port` run, for the programs with an exe
  pair or a `--fix` only: an exe pair's `NAME.fixed-N.exe` is rewritten
  from `port`, and a `--fix` program whose executables differ gets the
  pair. A source that does not compile, has no output or times out is
  refused.
* `--fix PASS:NAME#N`: with `--update-expect`, a program a fix makes
  differ from the original (`PASS` the name of `expect_dir=`; any number).
* `--observe TAG --to DIR`: run only the configuration `TAG` and write
  its observations to `DIR` (emptied first): `NAME.out`, `NAME.serial`
  and `NAME.exe`, the SHA-256 of the executable (with runtime error
  checking the trap blocks zeroed). `--layout-stack` gives the session a
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
* Tests: `help_corpusrun`, `readme_corpusrun`.

## Example

```
$ corpusrun cd build/amiga/tests/corpus/corpus_cd.pass
```
