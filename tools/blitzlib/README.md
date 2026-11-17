# blitzlib

Works on Blitz Basic 2 library files and BigLib files (`blitz2:acidlibs`,
`blitz2:deflibs`, a series of library files): joins library files into a
BigLib file (`join`), takes one out of it (`extract`), checks that no
library number occurs twice (`numbers`), checks a library file built from
C (`check`), compares the declarations with the original (`compare`),
writes the command table as JSON for the website (`cmdref`) and checks
the `__entrypoint` routines of a library's objects (`entrypoints`). The
logic is in `tools/lib/biglib` (BigLib files), `tools/lib/blitzlib`
(header, token table, checks, the comparison), `tools/lib/cmdref` and
`tools/lib/epcheck`, tested by `tools/tests/test_biglib.cpp`,
`test_blitzlib.c`, `test_blitzlib_compare.c`, `test_cmdref.c` and
`test_epcheck.cpp`.

## Usage

```
Usage: blitzlib [OPTIONS] SUBCOMMAND

Blitz Basic 2 library files: joined, taken out, checked, compared, as JSON.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  join                        join Blitz Basic 2 library files into one BigLib file
  extract                     take the library with that number (or the number of LIB.obj) out
                              of a BigLib file
  numbers                     no library number twice in the FILEs (BigLib or library files)
  check                       check Blitz Basic 2 library files
  compare                     the declarations of every library of a BigLib file (or of a
                              library file) against the original
  decls                       the declarations of every library of a BigLib file (or of a
                              library file) as text
  cmdref                      the command table of a Blitz Basic 2 library file as JSON
  entrypoints                 check the __entrypoint routines of a library's objects
```

### join

```
Usage: blitzlib join [OPTIONS] OUT [LIB.obj...]

join Blitz Basic 2 library files into one BigLib file

POSITIONALS:
  OUT TEXT REQUIRED           the BigLib file written
  LIB.obj TEXT ...            the library files, in this order

OPTIONS:
  -h,     --help              Print this help message and exit

Every input must be a well-formed AmigaDOS load file; prints "OUT: N libs". Exit
status 0, 1 an input cannot be read or is no load file, or OUT cannot be
written, 2 usage error.
```

Every input must be a well-formed AmigaDOS load file; writes the
BigLib file `OUT` (`tools/lib/biglib/biglib.h`) and prints
`OUT: N libs`.

### extract

```
Usage: blitzlib extract [OPTIONS] BIGLIB WHICH OUT

take the library with that number (or the number of LIB.obj) out of a BigLib
file

POSITIONALS:
  BIGLIB TEXT REQUIRED        the BigLib file
  WHICH NUMBER|LIB.obj REQUIRED
                              the library number, or a library file whose number is taken
  OUT TEXT REQUIRED           the library file written

OPTIONS:
  -h,     --help              Print this help message and exit

Exit status 0, 1 BIGLIB cannot be read or holds no such library, WHICH is no
library file, or OUT cannot be written, 2 usage error.
```

The library with that number of `BIGLIB` (or the number of
the library file `LIB.obj`, e.g. the original of a library built here)
as a file of its own.

### numbers

```
Usage: blitzlib numbers [OPTIONS] FILE...

no library number twice in the FILEs (BigLib or library files)

POSITIONALS:
  FILE TEXT ... REQUIRED      BigLib or library files

OPTIONS:
  -h,     --help              Print this help message and exit
          --allow-33 FILE     a FILE whose library 33 is allowed although it is not literallib
                              (RIDosLib in the RWE variants)

Library 32 and a library 33 other than literallib (the literal characters,
#1018) are refused, but the library 33 of a FILE named by --allow-33. Exit
status 0, 1 a library number is twice (both named) or reserved, or a FILE cannot
be read or is no BigLib or library file, 2 usage error.
```

No two libraries of the `FILE`s (BigLib or library files,
e.g. acidlibs, deflibs and the userlibs of a variant) have the same
number; a clash names both. Library 32 and a library 33 other than
`literallib` are refused: the first byte `$90` is reserved for the literal
characters (#1018, `docs/token-table.md`); `--allow-33 FILE` allows the
library 33 of that file (RIDosLib in the RWE variants).

### check

```
Usage: blitzlib check [OPTIONS] LIB.obj...

check Blitz Basic 2 library files

POSITIONALS:
  LIB.obj TEXT ... REQUIRED   the library files

OPTIONS:
  -h,     --help              Print this help message and exit
          --allow-33          a library 33 other than literallib is allowed (RIDosLib in the
                              RWE variants)

One code hunk, relocations only into it, the library header, the token table
readable up to its end, no reference from the normal code into the error check
area, no error check thunk in the error check area that calls a check function
in the normal code, not the reserved library numbers 32 and 33 (33 only for
literallib, the literal characters, or with --allow-33). Silent if all is right.
Exit status 0, 1 a file is no valid library file (the findings on stderr), 2
usage error.
```

Each file is a valid library file: one code hunk, relocations
only into it, the library header, the token table readable up to its
end, no reference from the normal code into the error check area
(`docs/development.md`, "Checks in the build"), no error check of the
token table in the error check area that is the thunk of a C check
function (`MOVEM.l regs,-(a7):JSR f`, `BLITZ_ERRCHK_THUNK`, also behind
the Blitz/Amiga pair of `BLITZ_ERRCHK_AB`) and calls a function in the
normal code (#1056), not the number 32 or 33
(33 only for `literallib`, identified by its 127 dummy tokens named
`$81`-`$FF`, or with `--allow-33`; #1018). Silent if all is right.

### compare

```
Usage: blitzlib compare [OPTIONS] ORIGINAL NEW [INTENDED]

the declarations of every library of a BigLib file (or of a library file)
against the original

POSITIONALS:
  ORIGINAL TEXT REQUIRED      the original BigLib or library file, or its declaration data
                              (decls)
  NEW TEXT REQUIRED           the BigLib or library file built here
  INTENDED TEXT               the intended differences, one message line per line (# comments),
                              each of them must occur

OPTIONS:
  -h,     --help              Print this help message and exit

Both hold the same libraries in the same order. Prints the differences and a
summary. Exit status 0 no difference, 1 a difference or a stale INTENDED line,
or a file cannot be read, 2 usage error, 77 (skipped) ORIGINAL does not exist
(fetch-originals).
```

The declarations of every library of `NEW` against those of
`ORIGINAL` (BigLib files with the same libraries in the same order, or
two library files): header, entries, `!libfin`. `INTENDED` lists the
intended differences, one message line per line as printed, `#`
comments; they are not counted, and each must occur (a stale entry is a
difference; `docs/development.md`, "Intended differences"). Prints the
differences and a summary. Without `ORIGINAL` it is skipped. `ORIGINAL`
may be the declaration data `decls` wrote (a text file beginning with
`#` or `library`): the comparison and its messages are the same as with
the library files.

### decls

```
Usage: blitzlib decls [OPTIONS] FILE

the declarations of every library of a BigLib file (or of a library file) as
text

POSITIONALS:
  FILE TEXT REQUIRED          the BigLib or library file

OPTIONS:
  -h,     --help              Print this help message and exit
  -o TEXT REQUIRED            the declaration data to write

Writes what compare compares (no code): the data compare takes as ORIGINAL. Exit
status 0, 1 a library does not read or a file cannot be read or written, 2 usage
error.
```

The declaration data of `FILE` (`tests/data/declarations/`, so the
comparisons with the original libraries need no CD): behind a `#` header,
per library `library<TAB>NUM<TAB>RELOCS` (the relocations inside the
declaration part), `header<TAB>TOKENS`, one line per entry of the token
table and `end<TAB>TOKENS` (the `$FFFF` and `!libfin`). An entry with a
name block is `entry<TAB>K<TAB>NAME<TAB>HELP<TAB>TOKENS<TAB>TOKENS`: its
name and help text, then the bytes in front of the name (kind, forms,
argument types) and those behind the help text's 0 (padding, often
empty); a nullsub (or a name block that is not plain text) is
`entry<TAB>K<TAB>-<TAB>TOKENS`. For example

```
entry	0	PopUpInit	-- initialise the popup window system	0001........000102..0000--------++++++++--------ffff............	
```

`NAME` and `HELP` are written as they are; a backslash is `\\` and any
other byte that is not printable ASCII (control characters, bytes from
`$7F`) is `\xHH`, so reading gives back the same bytes (any other
backslash, `\x00` or an escape of a printable character is an error).
`TOKENS` has two characters per byte: hex digits for a declaration byte,
`..` for a byte filled in at run time or padding, `++++++++` or
`--------` for a code offset that is set or zero. No code and no code
offsets (`tools/lib/blitzlib/blitzlib.h`, `blitzlib_decls_write`).

### cmdref

```
Usage: blitzlib cmdref [OPTIONS] LIB.obj

the command table of a Blitz Basic 2 library file as JSON

POSITIONALS:
  LIB.obj TEXT REQUIRED       the library file

OPTIONS:
  -h,     --help              Print this help message and exit
  -o OUT                      the JSON written (default: stdout); removed first
          --group GROUP REQUIRED
                              the group of the library (acidlibs, deflibs, ...)
          --name NAME REQUIRED
                              the name of the library

The data of the website's command reference,
site/data/libraries/GROUP/NAME.json. Exit status 0, 1 LIB.obj is no readable
library file or OUT cannot be written (no OUT is left), 2 usage error.
```

The command table of the library file as JSON
(`tools/lib/cmdref/cmdref.h`), to `OUT` or stdout; `OUT` is removed
first, so a failed run leaves none.

### entrypoints

```
Usage: blitzlib entrypoints [OPTIONS] FILE...

check the __entrypoint routines of a library's objects

POSITIONALS:
  FILE TEXT ... REQUIRED      the static library or the objects of the library

OPTIONS:
  -h,     --help              Print this help message and exit
          --objdump OBJDUMP REQUIRED 
                              the toolchain's objdump

A routine compiled with BLITZ_ENTRYPOINT (GCC's __entrypoint: no callee-saved
register saved) and marked by BLITZ_ALIAS_EP must not write a6 without saving it
at its start (the register rule keeps a6), must not be called or jumped into
from other code (GCC does not know that it keeps no d2-d7/a2; identical code
folding makes a function with the same body a jump to it), and must not begin
with a jump into another routine. Silent if all is right. Exit status 0, 1 a
finding (on stderr), 2 usage error.
```

The routines are the C functions with `BLITZ_ENTRYPOINT` that
`BLITZ_ALIAS_EP` makes token table entries (`libs/include/blitzlib.h`):
each is marked by the global symbol `___blitz_entrypoint_NAME` and ends at
the next symbol of `.text`. The check reads `objdump -t` and
`objdump -d` of each `FILE` (`tools/lib/epcheck/epcheck.h`); a finding is
`OBJECT: NAME: ...` or `OBJECT: OFFSET: INSTRUCTION: ...`.

## Inputs and outputs

Library files are AmigaDOS load files with one code hunk (layout:
`blitz2.s` `fixlibs`, `tools/lib/blitzlib/blitzlib.h`). `join` and
`extract` write a file; `compare` and `join` report on stdout; `cmdref`
writes the JSON of `site/data/libraries/GROUP/NAME.json`.

## Exit codes

* 0 success;
* 1 a check fails (`numbers`: a number twice; `check`: an invalid
  library file; `entrypoints`: a finding; `compare`: a difference or a stale intended entry) or an
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
* `decls`: `tools/blitzlib/decls-data.cmake` - the target
  `declarations-data` and the tests `declarations_GROUP_data`,
  `rwe_declarations_NAME_data`, `declarations_data_check_*`
  (`libs/CMakeLists.txt`).
* `compare`: `declarations_acidlibs`, `declarations_deflibs`,
  `declarations_data_broken`, `blitzlibcompare_single*` (`libs/CMakeLists.txt`),
  `rwe_declarations_NAME` (`libs/userlibs/rwe/CMakeLists.txt`),
  `rwe_refbuild_selftest` (`libs/userlibs/rwe/refbuild.cmake`).
* `extract`: `libs/tests/emulator.cmake` (the originals of the library
  tests).
* `entrypoints`: `blitz_library()` in `cmake/BlitzLibrary.cmake`, the
  step that links every library file (on its static library); the tests
  `blitzlib_entrypoints_good`, `blitzlib_entrypoints_broken_a6`, `_call`,
  `_folded` (`libs/CMakeLists.txt`, `cmake/entrypoint/`).
* Tests: `help_blitzlib`, `readme_blitzlib` (`tools/blitzlib/CMakeLists.txt`).

## Example

```
$ blitzlib numbers build/amiga/blitz2/acidlibs build/amiga/libs/acidlibs/fadelib.obj
blitzlib numbers: library number 157 twice: build/amiga/blitz2/acidlibs (library 22) and build/amiga/libs/acidlibs/fadelib.obj (library 1)
$ echo $?
1
$ blitzlib extract archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS build/amiga/libs/acidlibs/fadelib.obj fadelib.orig
```
