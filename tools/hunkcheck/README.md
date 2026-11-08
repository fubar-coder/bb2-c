# hunkcheck

Checks the hunks of an AmigaDOS load file that the build or a test made:
the bytes at offsets of the first hunk (`start`), the layout of the hunks
(`layout`), and that every CODE/DATA hunk is stored whole (`stored`). The
logic is `tools/lib/hunkcheck` (`hunkcheck.h`, on `tools/lib/hunk`),
tested by `tools/tests/test_hunkcheck.c`.

## Usage

```
usage: hunkcheck start FILE OFFSET:HEXBYTES...
       hunkcheck layout LAYOUT FILE
       hunkcheck stored FILE
```

`hunkcheck SUB -h` prints the description of `SUB`.

* `start`: each `OFFSET:HEXBYTES` (an offset into the first hunk,
  decimal or hexadecimal with `0x`, and the bytes expected there in
  hexadecimal) must hold; the first hunk must be code.
  For example `0:6100 4:48e7`: a `bsr.w` at offset 0 and a `movem.l` at
  offset 4.
* `layout`: the hunks of `FILE` in order, `LAYOUT` a comma-separated list
  of `code`, `data` or `bss`, each with `+chip` if it must be loaded to
  chip memory, e.g. `code,bss,data+chip`: their number, types and chip
  flags.
* `stored`: no CODE or DATA hunk is larger than the block stored for it
  in the file (Kickstart 1.x's `LoadSeg` does not clear an unstored tail,
  #963; `cmake/baserel-nostart.ld`).

## Inputs and outputs

Input is an AmigaDOS load file (read by `tools/lib/hunk`). Nothing is written if every
check holds; each failed check is one line on stderr.

## Exit codes

* 0 every check holds;
* 1 a check fails, or the file cannot be read or is no load file;
* 2 usage error.

Messages go to stderr: a failed check as `FILE: message` (from the
library), a file that cannot be read as `hunkcheck SUB: FILE: message`.

## Callers

* `start`: POST_BUILD of `ted` (`programs/ted/CMakeLists.txt`), `ab3cli`
  (`programs/ab3cli/CMakeLists.txt`), `tedoracle`
  (`programs/ted/tests/oracle/CMakeLists.txt`), `tedstub`
  (`tests/corpus/amiga/CMakeLists.txt`).
* `stored`: POST_BUILD of `ted` and `ab3cli`.
* `layout`: `tests/ab3/cli.cmake`, the cases of `tests/ab3/CMakeLists.txt`
  with a hunk layout (`HUNKS`).
* Tests: `help_hunkcheck`, `readme_hunkcheck`
  (`tools/hunkcheck/CMakeLists.txt`).

## Example

```
$ hunkcheck start build/amiga/blitz2/ted 0:6100 4:48e7
$ hunkcheck layout code,bss build/amiga/blitz2/ted
build/amiga/blitz2/ted: hunk 1 is data, expected bss
$ echo $?
1
```
