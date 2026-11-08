# blitzlib

Works on Blitz Basic 2 library files and BigLib files (`blitz2:acidlibs`,
`blitz2:deflibs`, a series of library files): joins library files into a
BigLib file (`join`), takes one out of it (`extract`), checks that no
library number occurs twice (`numbers`), checks a library file built from
C (`check`), compares the declarations with the original (`compare`) and
writes the command table as JSON for the website (`cmdref`). The logic is
in `tools/lib/biglib` (BigLib files), `tools/lib/blitzlib` (header, token
table, checks, the comparison) and `tools/lib/cmdref`, tested by
`tools/tests/test_biglib.c`, `test_blitzlib.c`, `test_blitzlib_compare.c`
and `test_cmdref.c`.

## Usage

```
usage: blitzlib join OUT LIB.obj...
       blitzlib extract BIGLIB NUMBER|LIB.obj OUT
       blitzlib numbers FILE...
       blitzlib check LIB.obj...
       blitzlib compare ORIGINAL NEW [INTENDED]
       blitzlib cmdref [-o OUT] -group GROUP -name NAME LIB.obj
```

`blitzlib SUB -h` prints the description of `SUB`.

* `join`: every input must be a well-formed AmigaDOS load file; writes the
  BigLib file `OUT` (`tools/lib/biglib/biglib.h`) and prints
  `OUT: N libs`.
* `extract`: the library with that number of `BIGLIB` (or the number of
  the library file `LIB.obj`, e.g. the original of a library built here)
  as a file of its own.
* `numbers`: no two libraries of the `FILE`s (BigLib or library files,
  e.g. acidlibs, deflibs and the userlibs of a variant) have the same
  number; a clash names both.
* `check`: each file is a valid library file: one code hunk, relocations
  only into it, the library header, the token table readable up to its
  end, no reference from the normal code into the error check area
  (`docs/development.md`, "Checks in the build"). Silent if all is right.
* `compare`: the declarations of every library of `NEW` against those of
  `ORIGINAL` (BigLib files with the same libraries in the same order, or
  two library files): header, entries, `!libfin`. `INTENDED` lists the
  intended differences, one message line per line as printed, `#`
  comments; they are not counted, and each must occur (a stale entry is a
  difference; `docs/development.md`, "Intended differences"). Prints the
  differences and a summary. Without `ORIGINAL` it is skipped.
* `cmdref`: the command table of the library file as JSON
  (`tools/lib/cmdref/cmdref.h`), to `OUT` or stdout; `OUT` is removed
  first, so a failed run leaves none.

## Inputs and outputs

Library files are AmigaDOS load files with one code hunk (layout:
`blitz2.s` `fixlibs`, `tools/lib/blitzlib/blitzlib.h`). `join` and
`extract` write a file; `compare` and `join` report on stdout; `cmdref`
writes the JSON of `site/data/libraries/GROUP/NAME.json`.

## Exit codes

* 0 success;
* 1 a check fails (`numbers`: a number twice; `check`: an invalid
  library file; `compare`: a difference or a stale intended entry) or an
  input cannot be read, is no library or BigLib file, or the output
  cannot be written;
* 2 usage error;
* 77 `compare`: `ORIGINAL` does not exist (skipped; the CD is not in the
  repository, `fetch-originals`), said on stdout.

Messages go to stderr as `blitzlib SUB: [FILE: ]message`; the findings of
`check` as `FILE: message`.

## Callers

* `join`, `numbers`: `biglib_join()` and `blitz_library(... CHECK)` in
  `cmake/BlitzLibrary.cmake` (every acidlibs and deflibs, every userlib);
  the tests `biglib_check_clash` (`libs/CMakeLists.txt`), `rwe_numbers`,
  `rwe_numbers_VARIANT`, `rwe_numbers_VARIANT_clash`
  (`libs/userlibs/rwe/CMakeLists.txt`).
* `check`: `blitz_library()` in `cmake/BlitzLibrary.cmake`, the step that
  links every library file.
* `cmdref`: `blitz_library(... CMDREF GROUP/NAME)` through
  `tools/blitzlib/cmdref-check.cmake` (the JSON equals the committed one);
  `cmdref_check_broken`, `cmdref_complete`, `cmdref_complete_broken`
  (`libs/CMakeLists.txt`, with `cmdref-complete.cmake`),
  `cmdref_not_a_library` (`tools/tests/CMakeLists.txt`); the target
  `cmdref_update`.
* `compare`: `declarations_acidlibs`, `declarations_deflibs`,
  `blitzlibcompare_single*` (`libs/CMakeLists.txt`),
  `rwe_declarations_NAME` (`libs/userlibs/rwe/CMakeLists.txt`),
  `rwe_refbuild_selftest` (`libs/userlibs/rwe/refbuild.cmake`).
* `extract`: `libs/tests/emulator.cmake` (the originals of the library
  tests).
* Tests: `help_blitzlib`, `readme_blitzlib` (`tools/blitzlib/CMakeLists.txt`).

## Example

```
$ blitzlib numbers build/amiga/blitz2/acidlibs build/amiga/libs/acidlibs/fadelib.obj
blitzlib numbers: library number 157 twice: build/amiga/blitz2/acidlibs (library 22) and build/amiga/libs/acidlibs/fadelib.obj (library 1)
$ echo $?
1
$ blitzlib extract archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS build/amiga/libs/acidlibs/fadelib.obj fadelib.orig
```
