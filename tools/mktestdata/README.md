# mktestdata

Writes binary test data (8SVX, ILBM, MED and tracker modules, `.xtra`
files, anything with a fixed layout) from a small text description,
deterministically, so that the data of a test is a reviewable text file
in the repository and the binary is made in the build. The logic is
`tools/lib/mktestdata` (`mktestdata.h`: the description format), tested
by `tools/tests/test_mktestdata.c`.

## Usage

```
usage: mktestdata SPEC OUT
```

Builds the data the description `SPEC` gives and writes them to `OUT`;
`OUT` is written only when the description is valid.

## Inputs and outputs

The description is a list of lines; `;` or `#` starts a comment. Values
are big-endian (68000): `b V...` bytes, `w V...` words, `l V...` longs,
`s "TEXT"` characters, `fill N V`, `align N`, labels `NAME:`, IFF
structure with `form TYPE [LEN]`, `chunk ID [LEN]` and `end` (the length
filled in, a pad byte after an odd length). A value is an expression of
numbers (`$hex`, `0xhex`, decimal), `'ABCD'` and `@NAME` (the offset of a
label) joined by `+` and `-`. The complete format is in the header of
`tools/lib/mktestdata/mktestdata.h`. The output is the data, nothing else.

## Exit codes

* 0 written;
* 1 `SPEC` cannot be read, the description has an error (`OUT` is not
  written) or `OUT` cannot be written;
* 2 usage error.

Messages go to stderr: `mktestdata: cannot read FILE`, `mktestdata:
cannot write FILE`, and an error of the description as `SPEC: line N:
message`.

## Callers

* `tests/bug-orig/CMakeLists.txt`: the test data of the bug-orig tests
  (`tests/bug-orig/AREA/NAME.spec`, in the build `testdata/NAME`);
* `tests/ab3/CMakeLists.txt`: the AmiBlitz3 `.xtra` files of the
  command-line tests (`tests/ab3/cli/ab3xtra.spec`).
* Tests: `help_mktestdata`, `readme_mktestdata`.

## Example

```
$ build/amiga/host/tools/mktestdata/mktestdata tests/ab3/cli/ab3xtra.spec ab3.xtra
```
