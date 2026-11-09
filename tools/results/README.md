# results

Judges a result against its expectation or the previous result: the
output of a test run against an expect file (`match`), the library files
of the build against those of the build of the previous commit (`prev`),
the CD result table `tests/data/blitz2-cd.tsv` written from a fresh
run of the CD (`cd-update`, `cd-reference`), and the oracle data of the
TED tests `tests/data/ted-oracle.sha256` written from a fresh run of the
original TED (`ted-oracle`). The logic is in `tools/lib/outmatch`,
`tools/lib/prevcmp` (with `tools/lib/previntended`), `tools/lib/cdtable`
and `tools/lib/oracletable`, tested by `tools/tests/test_outmatch.c`,
`test_prevcmp.c`, `test_previntended.c`, `test_cdtable.c` and
`test_oracletable.c`.

## Usage

```
Usage: results [OPTIONS] SUBCOMMAND

A result against its expectation or the previous result.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  match                       the text OUTPUT against the expect file EXPECT, line by line
  prev                        the library files of a build directory against those of the prev
                              build
  cd-update                   the CD result table from the port's fresh run
  cd-reference                the CD result table from the original's fresh run
  ted-oracle                  the oracle data of the TED tests from the original TED's fresh
                              run
```

### match

```
Usage: results match [OPTIONS] EXPECT OUTPUT

the text OUTPUT against the expect file EXPECT, line by line

POSITIONALS:
  EXPECT TEXT REQUIRED        the expect file
  OUTPUT TEXT REQUIRED        the output of a test run

OPTIONS:
  -h,     --help              Print this help message and exit
          --section NAME      only the lines after "[NAME]" up to the next "[...]" line in both

A line of EXPECT is literal, "~PATTERN" one line, "+PATTERN" one or more lines
matching PATTERN ('*' any characters, '#' a decimal number, "\c" c), "\TEXT" the
literal TEXT. Exit status 0 OUTPUT matches, 1 it does not (the first difference
on stderr), 2 usage error, a file that cannot be read or a broken EXPECT.
```

Compares the text `OUTPUT` with the expect file `EXPECT` line by line
(`tools/lib/outmatch/outmatch.h`): a line is literal, `~PATTERN` stands for
one line, `+PATTERN` for one or more lines matching `PATTERN` (`*` any
characters, `#` a decimal number, `\c` the character c), `\TEXT` for the
literal `TEXT`; `OUTPUT` must have no further lines. A 0 byte in `OUTPUT`
counts as `?`. With `--section NAME` only the lines after `[NAME]` up to
the next `[...]` line are compared, in both files. The first difference
goes to stderr.

### prev

```
Usage: results prev [OPTIONS] CUR PREV

the library files of a build directory against those of the prev build

POSITIONALS:
  CUR TEXT REQUIRED           the build directory
  PREV TEXT REQUIRED          the build of the previous commit

OPTIONS:
  -h,     --help              Print this help message and exit
          --all               every difference is intended
          --intended PATH     a path relative to the build directory whose difference is
                              intended (it must differ); an entry with a colon (corpus:NAME,
                              ted_ab:CASE ...) is one of the corpus and ted_ab: checked for its
                              syntax, otherwise ignored

Compares the library files (blitz2/acidlibs, blitz2/deflibs and every .obj below
libs/ and variants/) of CUR byte by byte with those of PREV. A difference fails
unless --all or --intended names it. Exit status 0 no difference that is not
intended, 1 a difference or a stale --intended, 2 usage error or a malformed
entry, 77 PREV does not exist.
```

Compares the library files (`blitz2/acidlibs`, `blitz2/deflibs` and every
`.obj` below `libs/` and `variants/`) of the build directory `CUR` byte by
byte with those of `PREV`, the build of the previous commit
(`tests/README.md`, "The prev build"). A difference fails unless `--all`
(every difference is intended) or `--intended PATH` (relative to the build
directory, repeatable) names it; a named file must differ. An entry with a
colon (`corpus:NAME`, `ted_ab:CASE` ..., `PREV_INTENDED` of
`cmake/prev.cmake`) belongs to the corpus and `ted_ab`: it is checked for
its syntax and otherwise ignored. Prints a `DIFFERS PATH`, `MISSING PATH`
or `STALE PATH` line per finding and the summary line of the library
(`prevcmp: N library files, ...`).

### cd-update, cd-reference

```
Usage: results cd-update [OPTIONS] NEW OLD OUT

the CD result table from the port's fresh run

POSITIONALS:
  NEW TEXT REQUIRED           the port's fresh run
  OLD TEXT REQUIRED           the table so far (tests/data/blitz2-cd.tsv)
  OUT TEXT REQUIRED           the table written (may be OLD)

OPTIONS:
  -h,     --help              Print this help message and exit

Rows whose note starts with "#NNN:" and names the CD's result as "(2.10: HEX)"
(the masked hash) or "(2.10: error LINE)" (refused at that line) are note rows:
the note stays unless the port's result is the CD's again. Exit status 0 OUT
written, 1 the tables do not fit (the report on stderr), 2 usage error or a file
that cannot be read or written.
```

```
Usage: results cd-reference [OPTIONS] NEW OLD OUT

the CD result table from the original's fresh run

POSITIONALS:
  NEW TEXT REQUIRED           the original's fresh run
  OLD TEXT REQUIRED           the table so far (tests/data/blitz2-cd.tsv)
  OUT TEXT REQUIRED           the table written (may be OLD)

OPTIONS:
  -h,     --help              Print this help message and exit

Rows whose note starts with "#NNN:" and names the CD's result as "(2.10: HEX)"
(the masked hash) or "(2.10: error LINE)" (refused at that line) are note rows:
the row stays, and its note must name NEW's result. Exit status 0 OUT written, 1
the tables do not fit (the report on stderr), 2 usage error or a file that
cannot be read or written.
```

Write the CD result table `OUT` (`OUT` may be `OLD`) from the fresh run
`NEW` and the table `OLD` (format: `tools/lib/cdtable/cdtable.h`). Rows
whose note starts with `#NNN:` and names the CD's result as `(2.10: HEX)`
(the masked hash) or `(2.10: error LINE)` (refused at that line) are note
rows:

* `cd-update`: `NEW` is the port's run; every row is `NEW`'s, a note stays
  unless the port's result is the CD's again; a changed row without a
  note is reported (it needs a note by hand);
* `cd-reference`: `NEW` is the original compiler's run; the note rows of
  `OLD` stay, and their note must name `NEW`'s result.

Both write nothing if a note row is missing from `NEW`, names no result or
a table is malformed; the report goes to stdout (written) or stderr (not
written).

### ted-oracle

```
Usage: results ted-oracle [OPTIONS] NEW OLD OUT

the oracle data of the TED tests from the original TED's fresh run

POSITIONALS:
  NEW TEXT REQUIRED           the original TED's fresh run
  OLD TEXT REQUIRED           the oracle data so far (tests/data/ted-oracle.sha256)
  OUT TEXT REQUIRED           the file written (may be OLD)

OPTIONS:
  -h,     --help              Print this help message and exit

Rows with a note "#NNN: TEXT (TED 1.1: HASH)" behind a tab are note rows (the
port's hash after a fix): the row stays, and its note must name NEW's hash. Exit
status 0 OUT written, 1 the files do not fit (the report on stderr), 2 usage
error or a file that cannot be read or written.
```

Writes the oracle data of the TED tests `OUT` (`OUT` may be `OLD`) from
the original TED's fresh run `NEW` (rows `HASH  KIND/PATH`, the target
`ted-oracle`) and the file `OLD` (format:
`tools/lib/oracletable/oracletable.h`). A row with a tab and a note
`#NNN: TEXT (TED 1.1: HASH)` behind the path is a note row: the port's
hash after the fix of issue `NNN`, the note names the original's. Every
row is `NEW`'s except the note rows of `OLD`, which stay; their note must
name `NEW`'s hash (else the note is stale). Nothing is written if a note
row is missing from `NEW`, a note is not of that form or names the row's
own hash, or a file is malformed; the report goes to stdout (written) or
stderr (not written).

## Inputs and outputs

`match` reads two text files and writes only a message; `prev` reads two
build directories and writes its report to stdout; `cd-update` and
`cd-reference` read two TSV tables and write the third, `ted-oracle` two
lists of hashes and writes the third.

## Exit codes

* 0 success (the output matches, the library files are equal or differ
  as intended, the table is written);
* 1 the output does not match; a library file differs without being
  intended or an intended one does not differ; the table or the oracle
  data have errors (nothing written);
* 2 usage error; also a file that cannot be read (`match`, `cd-update`,
  `cd-reference`, `ted-oracle`), a broken expect file (`match`), a table
  or the oracle data that cannot be
  written, a malformed `--intended` entry or another error (`prev`);
* 77 `prev`: `PREV` does not exist (no prev build: skipped), said on
  stderr.

Messages go to stderr as `results SUB: [FILE: ]message`.

## Callers

* `match`: `tests/ab3/cli.cmake` (the `ab3_cli*` tests of
  `tests/ab3/CMakeLists.txt`), `tests/bug-orig/inject-fill.cmake` and
  `inject-datestamp.cmake` (the `inject_*` tests of
  `tests/bug-orig/CMakeLists.txt`).
* `prev`: the test `prev_results` (`CMakeLists.txt`, with
  `PREV_INTENDED`); the tests `prevcmp_*` (`tools/tests/CMakeLists.txt`).
* `cd-reference`, `cd-update`: the targets `blitz2-cd-reference` and
  `blitz2-cd-update` (`tests/corpus/CMakeLists.txt`), after the CD run.
* `ted-oracle`: the target `ted-oracle`
  (`programs/ted/tests/oracle/ted-oracle.cmake`), after the run of the
  original TED.
* Tests: `help_results`, `readme_results` (`tools/results/CMakeLists.txt`).

## Example

```
$ results prev build/amiga build/amiga/prev
prevcmp: 787 library files, 0 differ, 0 not intended, 437 new
```
