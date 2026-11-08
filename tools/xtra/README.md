# xtra

Reads, checks and converts the companion settings of a Blitz source: the
Blitz 2 `.xtra` file (versions 0 and 1), the editor part in the header of
a BB22 source and AmiBlitz3's header embedded in a source
(`docs/formats.md` 1 and 7). The logic is `shared/xtra` (`xtra.h`, the
same code the editor and the compilers use), compiled for the host as
`tools/lib/xtra`, tested by `tools/tests/test_xtra_*.cpp`.

## Usage

```
usage: xtra dump FILE.xtra [--bb22 SRC]
       xtra dump --ab3 SRC.ab3
       xtra dump --source SRC
       xtra check [--bb22|--ab3] [--exact] FILE...
       xtra convert --to-ab3 IN.xtra OUT [--bb22 SRC | --ab3 SRC]
       xtra convert --to-xtra IN.ab3 OUT.xtra
```

* `dump` prints all fields: of a Blitz 2 `.xtra` (with `--bb22 SRC` the
  cursor of the BB22 source `SRC`), of the header of an AmiBlitz3 source
  (`--ab3`), or the type of `SRC` detected and its companion `SRC.xtra` if
  it has one (`--source`).
* `check` reads every `FILE` and writes it again: a Blitz 2 `.xtra` must
  give the same bytes; `--bb22` checks the headers of BB22 sources;
  `--ab3`: the header written again is byte for byte the file's, and a new
  header written from it and read again has the same fields (`--exact`:
  the new header is byte for byte the file's, for headers in AmiBlitz3's
  current layout).
* `convert --to-ab3` writes the header lines of a Blitz 2 `.xtra` (with
  `--bb22 SRC` the cursor of `SRC`; with `--ab3 SRC`, `OUT` is the source
  `SRC` with its header updated, or a new one); `convert --to-xtra` writes
  a Blitz 2 `.xtra` from the header of an AmiBlitz3 source. The Blitz 2
  file has library numbers, the AmiBlitz3 header object names: what the
  other format cannot name is left out, said on stderr.

`xtra -h` also prints a short description of each subcommand. The
subcommands are dispatched by the program itself, not by
`tools/lib/subcmd`; `xtra SUB -h` is not a help of its own (planned with
the move of the host programs to CLI11).

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
