# tedab

Runs the TED A/B tests (`tests/ted-ab`, `tests/bug-orig/ted`) in Amiberry:
the scripts `*.drv` are played with `teddrive` against the original TED
and TED-C under the original compiler and blitz2-c, the runs are checked
against the expectations of the scripts and compared pairwise (`run`);
`lint` checks the scripts without the emulator. `info`, `text` and `cmp`
show the screen dumps of `teddrive` (`.scr`) by hand, decoded as `run`
decodes them. The logic is in `lib_tedab` (`tedab.h`: the directives of
the scripts, the lint, the session script, the evaluation) and
`lib_teddump` (`teddump.h`: reading, decoding and comparing the dumps),
tested by `tests/ted-ab/host/test_tedab.cpp` and `test_teddump.cpp`.

## Usage

```
Usage: tedab [OPTIONS] SUBCOMMAND

The TED A/B tests in Amiberry and the screen dumps of teddrive.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  run                         run the TED A/B tests of a pass file in Amiberry and write the
                              report
  lint                        check the test scripts of a pass file and that their sources are
                              there
  info                        the header of a screen dump of teddrive and the SHA-256 of its
                              bit planes
  text                        a screen dump of teddrive as text
  cmp                         two screen dumps of teddrive compared as text

The exit status of run in a bug-orig pass is the worst verdict: 0 FIXED, 1
REPRODUCED, 3 BROKEN, 4 DIFFERS.
```

### run

```
Usage: tedab run [OPTIONS] PASSFILE

run the TED A/B tests of a pass file in Amiberry and write the report

POSITIONALS:
  PASSFILE TEXT REQUIRED      the pass file (written by CMake; its keys: the header of
                              tests/ted-ab/runner/run.c)

OPTIONS:
  -h,     --help              Print this help message and exit

With BUGORIG_RECORD=1 in the environment a bug-orig pass (bugorig=1, orig=)
records the observation of the original instead. Exit status 0 all gates and
runs OK and every comparison as expected, 1 not, 2 error (usage, pass file,
scripts), 77 the emulator or a file is missing (skipped). A bug-orig pass gives
the worst verdict: 0 all FIXED, 1 REPRODUCED, 3 BROKEN, 4 DIFFERS (with
BUGORIG_RECORD=1 0 when recorded).
```

Runs the tests of the pass file `PASSFILE` in Amiberry and writes the
report (to `report=` and stdout); after the report one `@stat` line per
emulator session (`tests/README.md`, "Test statistics"). The runs are
packed into sessions (the combinations of a test stay together), at most
`parallel=` at the same time; a session that ends early leaves its
remaining runs for a later session. With `bugorig=1` (the bug-orig mode,
`tests/bug-orig/ted`) every run has a session of its own and each
combination but the reference gets a verdict against the recorded
original `orig=`; with `BUGORIG_RECORD=1` in the environment only the
reference runs and its observation is written to `orig=`. With `fixed=1`
in place of `orig=` the scripts are fixed tests (`; @fixed #NNN`, no
pair): every combination (the pass names C5) is judged by the script's
expectations alone, FIXED (exit status 0) or REGRESSED (1).

### lint

```
Usage: tedab lint [OPTIONS] PASSFILE

check the test scripts of a pass file and that their sources are there

POSITIONALS:
  PASSFILE TEXT REQUIRED      the pass file (written by CMake; its keys: the header of
                              tests/ted-ab/runner/run.c)

OPTIONS:
  -h,     --help              Print this help message and exit

No emulator. Exit status 0 no problem, 1 problems, 2 usage error or a pass file
that cannot be read.
```

Reads the pass file and the scripts like `run` and checks the scripts
(syntax, directives) and that the sources they name are there; no
emulator. Prints one line per problem and the summary
`tedab lint: N tests, M problems`.

### info, text, cmp

```
Usage: tedab info [OPTIONS] FILE.scr

the header of a screen dump of teddrive and the SHA-256 of its bit planes

POSITIONALS:
  FILE.scr TEXT REQUIRED      the screen dump

OPTIONS:
  -h,     --help              Print this help message and exit

Size, depth, mode, mouse position, windows. Exit status 0, 2 usage error or a
file that cannot be read.
```

```
Usage: tedab text [OPTIONS] FILE.scr

a screen dump of teddrive as text

POSITIONALS:
  FILE.scr TEXT REQUIRED      the screen dump

OPTIONS:
  -h,     --help              Print this help message and exit
          --font 8|6          the height of TED's font the dump is decoded with: 8 (default) or
                              6
          --fonts DIR         chars8.b and chars.b of TED (default: archive/sources/ted-1.1 of
                              this repository)

Exit status 0, 2 usage error, a file that cannot be read or no TED screen.
```

```
Usage: tedab cmp [OPTIONS] A.scr B.scr

two screen dumps of teddrive compared as text

POSITIONALS:
  A.scr TEXT REQUIRED         the one dump
  B.scr TEXT REQUIRED         the other dump

OPTIONS:
  -h,     --help              Print this help message and exit
          --font 8|6          the height of TED's font the dump is decoded with: 8 (default) or
                              6
          --fonts DIR         chars8.b and chars.b of TED (default: archive/sources/ted-1.1 of
                              this repository)
          --no-mask           compare the status field "Free" too (masked by default)

Prints SAME, or DIFF and the differences. Exit status 0 the same, 1 different, 2
usage error or a file that cannot be read.
```

`info` prints the header of the dump (size, depth, mode, mouse position,
windows) and the SHA-256 of its bit planes. `text` prints the dump as
text, decoded with TED's font of height 8 (default) or 6 (`--font`) from
`chars8.b` / `chars.b` in `DIR` (`--fonts`, default
`archive/sources/ted-1.1` of this repository). `cmp` compares two dumps as
text and prints `SAME`, or `DIFF` and the differences; the status field
"Free" is masked unless `--no-mask`.

## Inputs and outputs

The pass file (written by CMake) has lines `KEY=VALUE` and `#` comments;
the keys are described in the header of `run.c` (directories of the
scripts, sources and the CD, the programs of the combinations, `filter=`,
`combos=`, `timeout=`, `parallel=`, `intended=` of the prev build,
`bugorig=`, `orig=` ...). The scripts are described in `tedab.h` and
`tests/README.md` ("TED A/B"). `run` writes below `work=` one directory
per session (`sNNN/vol` the volume `TEDAB:`, `sNNN/emu` Amiberry); the
dumps get a decoded `NAME.txt`. The dump format is in `teddump.h`.

## Exit codes

* `run`: 0 all gates and runs OK and every comparison as expected, 1 not,
  77 the emulator or a file is missing (skipped), 2 error (usage, pass
  file, scripts). In the bug-orig mode: 0 all FIXED, 1 REPRODUCED,
  3 BROKEN, 4 DIFFERS (the worst of them), 2 error; with
  `BUGORIG_RECORD=1` 0 when recorded.
* `lint`: 0 no problem, 1 problems, 2 usage error or a pass file that
  cannot be read.
* `info`, `text`, `cmp`: 0 success (`cmp`: the same), 1 `cmp`: different,
  2 usage error or a file that cannot be read.

Messages go to stderr as `tedab SUB: [FILE: ]message`; the log lines of
`run` (sessions, skipped tests) go to stdout with the same prefix.

## Callers

* `run`: the tests `ted_ab` (`tests/ted-ab/CMakeLists.txt`) and
  `bug_orig_ted_*`, `bug_orig_record_ted_*` (`tests/bug-orig/CMakeLists.txt`,
  also through `tests/bug-orig/want.cmake`).
* `lint`: the tests `ted_ab_lint`, `ted_ab_lint_fixed`,
  `ted_ab_lint_fixed_wrong` (`tests/ted-ab/CMakeLists.txt`) and
  `bug_orig_ted_lint` (`tests/bug-orig/CMakeLists.txt`).
* `info`, `text`, `cmp`: by hand, on the dumps of a run.
* Tests: `help_tedab`, `readme_tedab` (`tests/ted-ab/runner/CMakeLists.txt`).

## Example

```
$ tedab lint build/amiga/tests/ted-ab/ted_ab-lint-fixed.pass
tedab lint: 1 tests, 0 problems
```
