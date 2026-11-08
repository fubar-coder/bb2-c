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
usage: blitzres create -o OUT ITEM...
       blitzres dump RES
       blitzres source RES OUT
```

`blitzres SUB -h` prints the description and the options of `SUB`.

### create

The constants of `OUT`, in the order of the `ITEM`s:

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

The types, macros and constants of a resident, as text.

### source

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
