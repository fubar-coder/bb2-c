# refcheck

Checks the port against the originals: the assembler routines of the
acidlibs that stay assembler against the CD's libraries (`asm`), the code
labels of `ted.s` in TED's C sources (`labels`), the data of TED's C build
against the reference build of `ted.s` (`data`), and the views of a vasm
listing of a reference build (`listing-map`, `listing-labels`,
`listing-verify`). The logic is in `tools/lib/asmcmp`,
`tools/lib/checklabels`, `tools/lib/checkdata` and `tools/lib/vasmlisting`,
tested by `tools/tests/test_asmcmp.c`, `test_checklabels.c`,
`test_checkdata.c` and `test_vasmlisting.c`. The checks in the build are
listed in `docs/development.md`, "Checks in the build"; the intended
differences of `data` and the rule of `asm` in "Intended differences".

## Usage

```
Usage: refcheck [OPTIONS] SUBCOMMAND

The port against the originals: assembler routines, labels, data, vasm listings.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  asm                         asm routines of a C library against the original, instruction by
                              instruction
  labels                      every code label of FILE.s exactly once in the C sources of
                              SRCDIR
  data                        data of a C build against the reference data of the assembler
                              original
  data-ref                    the reference data of data from the reference build
  listing-map                 the rows of a vasm listing with bytes, as TSV: address,
                              file:line, bytes, source
  listing-labels              the labels of a vasm listing, as TSV: address, label, file:line
  listing-verify              every byte of a vasm listing against the first hunk of a load
                              file (relocated longwords skipped)
```

### asm

```
Usage: refcheck asm [OPTIONS] OBJDUMP ORIGINAL LIBNUM LINKED ROUTINE...

asm routines of a C library against the original, instruction by instruction

POSITIONALS:
  OBJDUMP TEXT REQUIRED       the m68k objdump
  ORIGINAL TEXT REQUIRED      the BigLib file with the original library (e.g. the CD's
                              BLITZ2/ACIDLIBS)
  LIBNUM TEXT REQUIRED        the number of the library in ORIGINAL
  LINKED TEXT REQUIRED        NAME.linked, the C library linked with symbols
  ROUTINE TEXT ... REQUIRED   NAME:ORIGADDR:CSYMBOL:COUNT, a routine: its name, its address in
                              the original, the C symbol and the number of instructions

OPTIONS:
  -h,     --help              Print this help message and exit

Exit status 0 every routine is equal, 1 a routine differs or an input cannot be
read, 2 usage error, 77 ORIGINAL does not exist (skipped: the CD is not in the
repository, fetch-originals).
```

Compares routines of a C library that are written in assembler with the
original library, instruction by instruction, disassembled with the
toolchain's `OBJDUMP`. `ORIGINAL` is the BigLib file with the original
library (the CD's `BLITZ2/ACIDLIBS`), `LIBNUM` its number, `NAME.linked`
the C library linked with symbols. Each routine is
`NAME:ORIGADDR:CSYMBOL:COUNT`: its name, its offset in the original, the C
symbol (or `SYMBOL+HEX`, `HEX` may be negative, for a routine without a
symbol of its own) and the number of instructions. Operands are
normalised, as the addresses differ (`tools/lib/asmcmp/asmcmp.h`). Without
`ORIGINAL` it is skipped.

### labels

```
Usage: refcheck labels [OPTIONS] FILE.s SRCDIR

every code label of FILE.s exactly once in the C sources of SRCDIR

POSITIONALS:
  FILE.s TEXT REQUIRED        the assembler source (FILE in "FILE:NNN label" is its name)
  SRCDIR TEXT REQUIRED        the directory of the C sources (every *.c below it)

OPTIONS:
  -h,     --help              Print this help message and exit
  -v Excludes: -q             every label
  -q Excludes: -v             the report only if a label is missing or doubled

Prints the missing and doubled labels and a summary. Exit status 0, 1 a label is
missing or doubled or an input cannot be read, 2 usage error.
```

Every code label of `FILE.s` must occur exactly once as
`FILE.s:NNN label` in the C sources (`*.c`) below `SRCDIR`. Prints the
missing and doubled labels and a summary; `-q` only if there are any, `-v`
every label.

### data

```
Usage: refcheck data [OPTIONS] TABLE REFDATA CBUILD [INTENDED]

data of a C build against the reference data of the assembler original

POSITIONALS:
  TABLE TEXT REQUIRED         the checks (e.g. programs/ted/checkdata.tsv)
  REFDATA TEXT REQUIRED       the reference data (data-ref, e.g. tests/data/ted-checkdata.txt)
  CBUILD TEXT REQUIRED        the C build with symbols
  INTENDED TEXT               the checks that differ on purpose (CSYM<TAB>#ISSUE<TAB>NOTE per
                              line), each of them must differ

OPTIONS:
  -h,     --help              Print this help message and exit
  -v Excludes: -q             every check
  -q Excludes: -v             the report only if a check failed

Prints the failed checks and a summary. Exit status 0, 1 a check failed, an
INTENDED entry is stale or an input cannot be read, 2 usage error.
```

Compares the data of the C build `CBUILD` (linked with symbols) with the
reference data `REFDATA` (written by `data-ref`), for every entry of
`TABLE` (`programs/ted/checkdata.tsv`: C symbol, start label, end label or
size, a note; relocated longwords of the reference are skipped). A check
whose range is not in `REFDATA` fails (after a change of the table:
`data-ref` again). `INTENDED` lists the checks that differ on purpose
(fixed original bugs, `CSYM<TAB>#NNN<TAB>NOTE` per line): each of them
must differ, a stale entry fails. Prints the failed checks and a summary;
`-q` only if a check failed, `-v` every check.

### data-ref

```
Usage: refcheck data-ref [OPTIONS] TABLE LISTING REFERENCE

the reference data of data from the reference build

POSITIONALS:
  TABLE TEXT REQUIRED         the checks (e.g. programs/ted/checkdata.tsv)
  LISTING TEXT REQUIRED       the vasm listing of the reference build
  REFERENCE TEXT REQUIRED     the reference build (a load file)

OPTIONS:
  -h,     --help              Print this help message and exit
  -o TEXT REQUIRED            the reference data to write

Writes the bytes of every range of TABLE. Exit status 0, 1 a label is not in the
listing (said on stdout) or an input cannot be read, 2 usage error.
```

Writes the reference data of `data`: for every range of `TABLE` (start
label and end label or size) the bytes of the first hunk of `REFERENCE`
from the labels of its vasm listing `LISTING`, one line
`START<TAB>END|SIZE<TAB>HEX` per range (`..` for a byte of a relocated
longword), behind a `#` header. `tests/data/ted-checkdata.txt` is written
so (target `ted-checkdata`), the check in the build needs no reference
build.

### listing-map, listing-labels, listing-verify

```
Usage: refcheck listing-map [OPTIONS] LISTING

the rows of a vasm listing with bytes, as TSV: address, file:line, bytes, source

POSITIONALS:
  LISTING TEXT REQUIRED       a vasm listing (vasm -L)

OPTIONS:
  -h,     --help              Print this help message and exit

Exit status 0, 1 the listing cannot be read, 2 usage error.
```

```
Usage: refcheck listing-labels [OPTIONS] LISTING

the labels of a vasm listing, as TSV: address, label, file:line

POSITIONALS:
  LISTING TEXT REQUIRED       a vasm listing (vasm -L)

OPTIONS:
  -h,     --help              Print this help message and exit

Exit status 0, 1 the listing cannot be read or written, 2 usage error.
```

```
Usage: refcheck listing-verify [OPTIONS] LISTING EXE

every byte of a vasm listing against the first hunk of a load file (relocated
longwords skipped)

POSITIONALS:
  LISTING TEXT REQUIRED       a vasm listing (vasm -L)
  EXE TEXT REQUIRED           the load file

OPTIONS:
  -h,     --help              Print this help message and exit

Exit status 0, 1 a row differs or an input cannot be read, 2 usage error.
```

Views of a vasm listing (`vasm -L FILE -Lall`):

* `listing-map`: the rows with bytes, as TSV: address, `file:line`, the
  bytes, the source;
* `listing-labels`: the labels, as TSV: address, label, `file:line`;
* `listing-verify`: every byte of the listing against the first hunk of
  the load file `EXE` (relocated longwords skipped); prints the rows that
  differ and a summary line.

## Inputs and outputs

The reports go to stdout: one line per routine (`asm`), label or check
(`labels`, `data`, with `-v` or a failure) and a summary line; its form
comes from the libraries (`checklabels: 278 code labels, ...`,
`checkdata: 534 checks, ...`, `verify: 7280 listing rows, ...`). The
listing views write TSV to stdout.

## Exit codes

* 0 success;
* 1 a check failed (a routine, a label, a data check or a listing row
  differs, an intended entry is stale) or an input cannot be read or is
  broken;
* 2 usage error;
* 77 `asm`: `ORIGINAL` does not exist (skipped; the CD is not in the
  repository, `fetch-originals`), said on stdout.

Messages go to stderr as `refcheck SUB: [FILE: ]message`.

## Callers

* `asm`: the tests `asmcmp_runerrlib`, `asmcmp_debuglib`,
  `asmcmp_dhandlerlib`, `asmcmp_memacclib` (`asm_check` in
  `libs/acidlibs/CMakeLists.txt`).
* `labels`: `programs/ted/CMakeLists.txt`, POST_BUILD of `ted`.
* `data`: `programs/ted/CMakeLists.txt`, POST_BUILD of `ted_sym`; the
  tests `ted_checkdata_intended`, `ted_checkdata_unlisted`,
  `ted_checkdata_stale`.
* `data-ref`: the target `ted-checkdata` and the test
  `ted_checkdata_reference` (`programs/ted/CMakeLists.txt`).
* `listing-map`, `listing-labels`, `listing-verify`: the reference builds
  `ted_ref` (`programs/ted/CMakeLists.txt`) and `blitz2_ref`
  (`programs/blitz2/CMakeLists.txt`), in the step that assembles them.
* Tests: `help_refcheck`, `readme_refcheck` (`tools/refcheck/CMakeLists.txt`).

## Example

```
$ refcheck listing-verify build/amiga/programs/ted/ref/ted.lst build/amiga/programs/ted/ref/ted
verify: 7280 listing rows, 0 differ
$ refcheck listing-map build/amiga/programs/ted/ref/ted.lst | head -2
000000	ted.s:22	61000202	bsr	edstart
000004	ted.s:24	33fcffff000062f2	outside	move	#-1,returncode
$ refcheck labels archive/sources/ted-1.1/ted.s programs/ted/src
checklabels: 278 code labels, 0 missing, 0 more than once
```
