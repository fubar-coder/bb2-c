# blitzres

Blitz Basic 2 resident files (`.res`): writes a resident with constants
directly, in the format blitz2's *Create Resident* writes (`create`),
prints the contents of any resident (`dump`), and writes the constants of
a resident as Blitz source (`source`), from which *Create Resident* makes
the same resident. It makes the residents of the RWE reference build from
the toolchain's NDK (`docs/rwelibs/README.md`). The logic is
`tools/lib/blitzres` (`blitzres.h`), tested by `tools/tests/test_blitzres.c`;
the format: `docs/formats.md` §2.

## Usage

```
Usage: blitzres [OPTIONS] SUBCOMMAND

Blitz Basic 2 resident files: written with constants, shown, as Blitz source.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  create                      a Blitz Basic 2 resident with constants, as blitz2's Create
                              Resident writes it
  dump                        the types, macros and constants of a resident
  source                      the constants of a resident as Blitz source, #NAME=VALUE lines
```

### create

```
Usage: blitzres create [OPTIONS]

a Blitz Basic 2 resident with constants, as blitz2's Create Resident writes it

OPTIONS:
  -h,     --help              Print this help message and exit
  -o OUT REQUIRED             the resident written
          --fd FILE           the public functions of an FD file as _NAME=-offset
          --fd-varargs FILE   the same, and _NAME for a NAMEA without NAME
          --equ FILE          the NAME EQU/SET EXPR lines of an assembler include
          --lvo FILE          the _LVONAME EQU -offset lines of an LVO include as NAME
          --const NAME=VALUE  a constant (also a symbol for --equ)
          --sym NAME=VALUE    a symbol for the expressions of --equ only
          --skip NAME         never NAME as constant

The constants of OUT in the order of the options (the items). A name again with
the same value is left out (blitz2 keeps the first); with another value it is an
error. OUT is read back and must hold exactly these constants. Exit status 0 OUT
written, 1 an input or OUT is wrong (OUT is not left), 2 usage error or a
--const/--sym that is no NAME=VALUE.
```

The constants of `OUT`, in the order of these options (`-o` may stand anywhere):

* `--fd FILE`: the public functions of an FD file as `_NAME=-offset`;
* `--fd-varargs FILE`: the same, then `_NAME` for a `NAMEA` without `NAME`;
* `--equ FILE`: the `NAME EQU/SET EXPR` lines of an assembler include;
* `--lvo FILE`: the `_LVONAME EQU -offset` lines of an LVO include as
  `NAME=-offset`;
* `--const NAME=VALUE`: a constant (also a symbol for `--equ`);
* `--sym NAME=VALUE`: only a symbol for the expressions of `--equ`;
* `--skip NAME`: `NAME` is never a constant (before the items).

`VALUE` is decimal, `0x` or `$` hexadecimal, maybe negative. A name again
with the same value is left out (blitz2 keeps the first); with another
value it is an error. The written file is read back and must hold exactly
these constants; else it is removed.

### dump

```
Usage: blitzres dump [OPTIONS] RES

the types, macros and constants of a resident

POSITIONALS:
  RES TEXT REQUIRED           the resident

OPTIONS:
  -h,     --help              Print this help message and exit

Exit status 0, 1 RES cannot be read or is no resident, 2 usage error.
```

The types, macros and constants of a resident, as text.

### source

```
Usage: blitzres source [OPTIONS] RES OUT

the constants of a resident as Blitz source, #NAME=VALUE lines

POSITIONALS:
  RES TEXT REQUIRED           a resident with constants only
  OUT TEXT REQUIRED           the source written

OPTIONS:
  -h,     --help              Print this help message and exit

blitz2's Create Resident makes the same resident of OUT. Exit status 0, 1 RES
cannot be read or holds more than constants, or OUT cannot be written, 2 usage
error.
```

The constants of `RES` (a resident with constants only) as a Blitz source
text `OUT`, one `#NAME=VALUE` line each.

## Inputs and outputs

`create` reads FD files and assembler includes and writes `OUT`; `dump`
writes to stdout; `source` writes the text file `OUT`.

## Exit codes

* 0 success;
* 1 an input cannot be read or is broken, a constant clashes, the
  read-back check fails, `OUT` cannot be written;
* 2 usage error (also a `--const`/`--sym` that is not `NAME=VALUE`, an
  unknown option, an option without its argument).

Messages go to stderr as `blitzres SUB: [FILE: ]message`.

## Callers

* `create`, `source`: `libs/userlibs/rwe/residents.cmake` (the residents
  of the RWE reference build and their sources); `create`:
  `tests/ab3/CMakeLists.txt` (`wclash1.res`, `wclash2.res`).
* `dump`: `tools/blitzres/check.cmake`, run by the tests `rwe_residents_cd`
  (`libs/userlibs/rwe/residents-cd.cmake`) and
  `rwe_residents_check_broken`.
* Tests: `help_blitzres`, `readme_blitzres` (`tools/blitzres/CMakeLists.txt`).

## Example

```
$ blitzres create -o c.res --const c=1 --const max=0x100
$ blitzres dump c.res
const c=1
const max=256
$ blitzres source c.res c.bas && cat c.bas
#c=1
#max=256
```
