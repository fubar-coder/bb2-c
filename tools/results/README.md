# results

Judges a result against its expectation or the previous result: the
output of a test run against an expect file (`match`), the library files
of the build against those of the build of the previous commit (`prev`),
and the CD result table `tests/data/blitz2-cd.tsv` written from a fresh
run of the CD (`cd-update`, `cd-reference`). The logic is in
`tools/lib/outmatch`, `tools/lib/prevcmp` (with `tools/lib/previntended`)
and `tools/lib/cdtable`, tested by `tools/tests/test_outmatch.c`,
`test_prevcmp.c`, `test_previntended.c` and `test_cdtable.c`.

## Usage

```
usage: results match [--section NAME] EXPECT OUTPUT
       results prev [--all] [--intended PATH]... CUR PREV
       results cd-update NEW OLD OUT
       results cd-reference NEW OLD OUT
```

`results SUB -h` prints the description and the options of `SUB`.

### match

Compares the text `OUTPUT` with the expect file `EXPECT` line by line
(`tools/lib/outmatch/outmatch.h`): a line is literal, `~PATTERN` stands for
one line, `+PATTERN` for one or more lines matching `PATTERN` (`*` any
characters, `#` a decimal number, `\c` the character c), `\TEXT` for the
literal `TEXT`; `OUTPUT` must have no further lines. A 0 byte in `OUTPUT`
counts as `?`. With `--section NAME` only the lines after `[NAME]` up to
the next `[...]` line are compared, in both files. The first difference
goes to stderr.

### prev

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

## Inputs and outputs

`match` reads two text files and writes only a message; `prev` reads two
build directories and writes its report to stdout; `cd-update` and
`cd-reference` read two TSV tables and write the third.

## Exit codes

* 0 success (the output matches, the library files are equal or differ
  as intended, the table is written);
* 1 the output does not match; a library file differs without being
  intended or an intended one does not differ; the table has errors
  (nothing written);
* 2 usage error; also a file that cannot be read (`match`, `cd-update`,
  `cd-reference`), a broken expect file (`match`), a table that cannot be
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
* Tests: `help_results`, `readme_results` (`tools/results/CMakeLists.txt`).

## Example

```
$ results prev build/amiga build/amiga/prev
prevcmp: 787 library files, 0 differ, 0 not intended, 437 new
```
