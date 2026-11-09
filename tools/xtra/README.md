# xtra

Reads, checks and converts the companion settings of a Blitz source: the
Blitz 2 `.xtra` file (versions 0 and 1), the editor part in the header of
a BB22 source and AmiBlitz3's header embedded in a source
(`docs/formats.md` 1 and 7). The logic is `shared/xtra` (`xtra.h`, the
same code the editor and the compilers use), compiled for the host as
`tools/lib/xtra`, tested by `tools/tests/test_xtra_*.cpp`.

## Usage

```
Usage: xtra [OPTIONS] SUBCOMMAND

The companion settings of a Blitz source: the Blitz 2 .xtra (vers 0 and 1), the
cursor in the header of a BB22 source, AmiBlitz3's embedded header
(docs/formats.md 1 and 7).

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  dump                        all fields of a Blitz 2 .xtra, a BB22 source's cursor or an
                              AmiBlitz3 header
  check                       every FILE read and written again
  convert                     a Blitz 2 .xtra into AmiBlitz3 header lines, or a header into a
                              .xtra
```

### dump

```
Usage: xtra dump [OPTIONS] [FILE.xtra]

all fields of a Blitz 2 .xtra, a BB22 source's cursor or an AmiBlitz3 header

POSITIONALS:
  FILE.xtra TEXT Excludes: --ab3 --source
                              a Blitz 2 .xtra

OPTIONS:
  -h,     --help              Print this help message and exit
          --bb22 SRC Needs: FILE.xtra Excludes: --ab3 --source
                              with FILE.xtra: the cursor of the BB22 source SRC
          --ab3 SRC Excludes: --bb22 --source FILE.xtra
                              instead of FILE.xtra: the header of an AmiBlitz3 source
          --source SRC Excludes: --ab3 --bb22 FILE.xtra
                              instead of FILE.xtra: the type of SRC detected, with its
                              companion SRC.xtra if it has one

Exit status 0, 1 a file cannot be read or is refused (with the offset or line),
2 usage error.
```

`dump` prints all fields: of a Blitz 2 `.xtra` (with `--bb22 SRC` the
cursor of the BB22 source `SRC`), of the header of an AmiBlitz3 source
(`--ab3`), or the type of `SRC` detected and its companion `SRC.xtra` if
it has one (`--source`).

### check

```
Usage: xtra check [OPTIONS] FILE...

every FILE read and written again

POSITIONALS:
  FILE TEXT ... REQUIRED      Blitz 2 .xtra files (default), or the sources

OPTIONS:
  -h,     --help              Print this help message and exit
          --bb22 Excludes: --ab3
                              the FILEs are BB22 sources: their headers
          --ab3 Excludes: --bb22
                              the FILEs are AmiBlitz3 sources: the header written again gives
                              the same bytes, and as a new header the same fields
          --exact             with --ab3: the new header gives the same bytes too

A Blitz 2 .xtra must give the same bytes. Prints "N files ok". Exit status 0, 1
a file cannot be read, is refused or differs, 2 usage error.
```

`check` reads every `FILE` and writes it again: a Blitz 2 `.xtra` must
give the same bytes; `--bb22` checks the headers of BB22 sources;
`--ab3`: the header written again is byte for byte the file's, and a new
header written from it and read again has the same fields (`--exact`:
the new header is byte for byte the file's, for headers in AmiBlitz3's
current layout).

### convert

```
Usage: xtra convert [OPTIONS] IN OUT

a Blitz 2 .xtra into AmiBlitz3 header lines, or a header into a .xtra

POSITIONALS:
  IN TEXT REQUIRED            the .xtra or the AmiBlitz3 source
  OUT TEXT REQUIRED           the file written

OPTIONS:
  -h,     --help              Print this help message and exit
          --to-ab3 Excludes: --to-xtra
                              IN is a Blitz 2 .xtra, OUT the header lines
          --to-xtra Excludes: --to-ab3
                              IN is an AmiBlitz3 source, OUT a Blitz 2 .xtra
          --bb22 SRC Needs: --to-ab3 Excludes: --ab3
                              with --to-ab3: with the cursor of the BB22 source SRC
          --ab3 SRC Needs: --to-ab3 Excludes: --bb22
                              with --to-ab3: OUT is the source SRC with its header updated from
                              IN (a new header if it has none)

Maxima the other format cannot name are left out (message on stderr). Exit
status 0, 1 a file cannot be read or is refused (with the offset or line) or OUT
cannot be written, 2 usage error.
```

`convert --to-ab3` writes the header lines of a Blitz 2 `.xtra` (with
`--bb22 SRC` the cursor of `SRC`; with `--ab3 SRC`, `OUT` is the source
`SRC` with its header updated, or a new one); `convert --to-xtra` writes
a Blitz 2 `.xtra` from the header of an AmiBlitz3 source. The Blitz 2
file has library numbers, the AmiBlitz3 header object names: what the
other format cannot name is left out, said on stderr.

## Inputs and outputs

Blitz 2 `.xtra` files, BB22 sources, AmiBlitz3 sources (`.ab3`,
`.ab2`, text with the embedded header); formats in `docs/formats.md`.
`dump` and `check` write a report to stdout, `convert` the file `OUT`.

## Exit codes

* 0 success;
* 1 a file cannot be read or is refused (message with offset or line), a
  check fails, `OUT` cannot be written;
* 2 usage error.

## Callers

By hand (`docs/libs/readable-sources.md`); the tests `xtra_tool`
(`tooltest.cmake`: broken inputs with their messages), `xtra_originals`
(`originals.cmake`: the CD's and rwelibsrc's `.xtra`, the BB22 sources;
skipped without them) and `xtra_ab3_examples` (`ab3examples.cmake`:
AmiBlitz3's sources), all in `CMakeLists.txt` here; `help_xtra`,
`readme_xtra`.

## Example

```
$ build/amiga/host/tools/xtra/xtra dump --source archive/aminet/amibl2src/rwelibsrc/AppLib/applib3.bb
companion        archive/aminet/amibl2src/rwelibsrc/AppLib/applib3.bb.xtra
type             BB22
source header    16 bytes kept
source           Blitz 2 .xtra
...
```
