# siteexamples

The Blitz Basic examples of the website, held against a manifest so that
none of them escapes the tests (`tests/site-examples`, `tests/README.md`).
The site shows programs in fenced blocks

````
```blitzbasic
NPrint "Hello"
```
````

in its pages (`site/content`) and in the data of the library pages
(`site/data`, where the fence is indented inside a YAML block; the
indentation of the opening fence is taken off). `siteexamples` finds
every block, names it by its file and its number in the file
(`data/libintros/acidlibs/sortlib.yaml#1`, counted from 1), and checks
that the manifest lists exactly these blocks. The logic is
`tools/lib/siteexamples` (`siteexamples.h`: the manifest format), tested by
`tools/tests/test_siteexamples.cpp` and, on fake site trees, by
`siteexamples_tool` (`tooltest.cmake`).

## Usage

```
Usage: siteexamples [OPTIONS] SUBCOMMAND

The Blitz Basic examples of the website, against their manifest.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  check                       every example has an entry and every entry an example
  extract                     the check, then every example as NAME.bas

Exit status 0 ok, 1 an example without an entry, an entry without an example, a
malformed manifest or a file that cannot be read or written, 2 usage error.
```

### check

```
Usage: siteexamples check [OPTIONS] SITE MANIFEST

every example has an entry and every entry an example

POSITIONALS:
  SITE TEXT REQUIRED          the site directory (site/: content, data, layouts, assets)
  MANIFEST TEXT REQUIRED      the manifest (tests/site-examples/examples.txt)

OPTIONS:
  -h,     --help              Print this help message and exit
```

Reads the text files of `content`, `data`, `layouts` and `assets` in
`SITE`, and prints `siteexamples: N examples, R run, C compiled only`
when every block has an entry and every entry a block. The first problem
stops it (exit status 1, on stderr): a block without an entry (the
message names its file, number and line), an entry whose block is not
there (the example changed its number or went away), a block listed
twice, a block without its closing fence, a malformed manifest line.

### extract

```
Usage: siteexamples extract [OPTIONS] SITE MANIFEST OUTDIR

the check, then every example as NAME.bas

POSITIONALS:
  SITE TEXT REQUIRED          the site directory (site/: content, data, layouts, assets)
  MANIFEST TEXT REQUIRED      the manifest (tests/site-examples/examples.txt)
  OUTDIR TEXT REQUIRED        the directory of the sources (made if missing)

OPTIONS:
  -h,     --help              Print this help message and exit
```

The same check; when it passes, `OUTDIR/NAME.bas` for every entry: a
first line `; FILE#N (run)` or `; FILE#N (compiled only)`, then the
block's lines. Nothing is written when the check fails.

## The manifest

`tests/site-examples/examples.txt`, one line per example:

```
NAME MODE FILE#N [rwe=LIB...] [REASON]
```

* `NAME`: lower case letters, digits and `-`, unique; it names the source
  and its expected output.
* `MODE`: `run` (compiled and run in the emulator, the output compared
  with the expected file) or `compile` (compiled only; the REASON, the
  rest of the line, says why it is not run: it opens a screen, waits for
  input, needs a file, a device or a library the test machine does not
  have, or its output varies).
* `FILE#N`: the block, as above.
* `rwe=LIB`, any number, directly behind `FILE#N`: the RWE userlibs
  (`libs/userlibs/rwe/LIB.c`) the example needs; they must be of one
  variant (`rwe-1999`, `rwe-1999-eval` or `rwe-1999-ahi`), and the example
  is compiled with that variant's libraries and those userlibs loaded.

Blank lines and lines beginning with `#` are comments. A new example on
the site fails the check until it is listed; there is no way to leave it
out.

## Callers

* `tests/site-examples/CMakeLists.txt`: the ctest `site_examples_manifest`
  (`check`) and the build step that writes the sources (`extract`) for
  the emulator passes.
* Tests: `siteexamples`, `siteexamples_tool`, `help_siteexamples`,
  `readme_siteexamples`: host tests, registered only in `build/amiga/host`
  (in `build/amiga` they run together as the single test `host`); run one
  with `ctest --test-dir build/amiga/host -R NAME`. `site_examples_manifest`
  is in `build/amiga`.
