# Tests

Reference of the suites (an introduction for people: [docs/testing.md](../docs/testing.md)). The test presets say **where** the tests run;
labels select within them.

| Preset | Runs | Needs |
|---|---|---|
| `native` | the host tests of the native profile (for IDEs: `build/native`) | native compiler |
| `host` | tools with broken inputs, host tests, declaration comparisons (against committed data), token lists (seconds) | native compiler |
| `emulator` | the C libraries against the committed expected data of the library tests (`libs_emulator`), corpus against the committed expect files, TED A/B (`ted_ab`: TED-C under blitz2-c), the expect tests (compilers' extensions, port regression tests), the Blitz examples of the website (`site_examples_run`, `site_examples_compile`, ...; "Site examples"), the fixed bug-orig tests (`; @fixed #N`), the compilation of the CD's sources (`corpus_cd`, `corpus_cd_ab3`) | Amiberry (`AMIGA-EMULATION-SETUP.md`); the CD only for `corpus_cd` and `corpus_cd_ab3` |
| `all` | `host` and `emulator`, the acceptance run: it starts no session of an original; without the CD it skips only `corpus_cd` and `corpus_cd_ab3` | Amiberry, native compiler; the CD for the skipped ones |
| `oracle` | `ab3_cli_ref_1` … `ab3_cli_ref_N` and their broken-input tests: our expect files of `tests/ab3` against AmiBlitz3 v3.14.0's own compiler (label `oracle`, not in `emulator`/`all`) | Amiberry; the fetched AmiBlitz3 files |
| `bug-orig` | original bugs not fixed yet (label `bug-orig`) and known port defects (label `bug-port`): they fail on purpose; a fixed test leaves this preset. Judged against the recorded observation of the original (`ID.orig`), no session of the original | Amiberry; the CD for the TED tests, for `port` and for the few tests that mount `BB2:` |
| `originals` | everything that compares with the original (label `originals`, in no other preset): the compare tests of the recorded originals (`bug_orig_record_ID`: the original's observation of every bug-orig test, open or fixed, against its `tests/bug-orig/*/ID.orig`; they write nothing, the target `bug-orig-record` records), the corpus runs of the original compiler with the original and with the C libraries (configurations `ref` and `orig-clib`) and of the port with the CD's libraries (`corpus_originals`, `corpus_runerr_originals`, ...; "Corpus"), `libs_emulator_originals` (the library tests' scenarios with the original libraries against the same expected data), the recording and update targets are not tests (`libs-capture`, `libs-update`: "Libraries"), `ted_ab_originals` ("TED A/B"), the fixed TED bug-orig tests with the original compiler (`bug_orig_ID_originals`), the RWE reference build (`rwe_refbuild`, `rwe_refbuild_NAME`, `rwe_refbuild_selftest`), `asmcmp_*`, `rwe_residents_cd`, `rwe_lesfont_cd`, `tokens_cd`, and the committed data against a fresh run of the original: `declarations_GROUP_data`, `blitz2_cd_reference` (the CD result table), `ted_oracle_reference` (the oracle data of the TED tests), `ted_checkdata_reference` | Amiberry; the CD (and the reference build for the RWE tests) |
| `coverage` | `all` and the original bugs with the coverage compilers (`coverage/` per test) | as `all` |

Setting up the emulator (ROMs, hardfiles, slot limit):
`AMIGA-EMULATION-SETUP.md`. The tool the emulator tests use, its sessions,
slots and snapshot mode: `tools/lib/amiga_emulation/README.md`.

Contents: [The test model](#the-test-model) ·
[Blitz test sources](#blitz-test-sources-and-the-result-file) ·
[Original material and presets](#original-material-and-presets) ·
[host](#host-tests) · [emulator](#emulator-tests) · [prev build](#the-prev-build) ·
[bug-orig](#bug-orig-tests) ·
[bug-port](#bug-port-tests) · [oracle data](#oracle-data-of-the-ted-tests) ·
[Test statistics](#test-statistics) · [Coverage](#coverage)

## The test model

The suites that compare program behaviour with an expectation follow one
model (spec `testing/test-model`; implemented for the library tests
("Libraries"), the bug-orig tests (`ID.orig` the `[orig]` side,
`ID.expect` the expected one, [Recorded originals](#recorded-originals))
and the corpus (`NAME.orig.out` / `NAME.fixed-N.out`, [Corpus](#corpus));
the TED oracle keeps its own form for now: rows with an issue). A test is a list of **cases**; a case is a key, an input
and an expected output, and its verdict is the difference of the actual
output against the expected one, reported by key. The expected output
(the **expected data**) is committed with the test; a test only compares
and never writes it. A case on which the original and the port agree is
one case with an unqualified key, run against both. A **fixed original
bug** is two cases: `KEY [orig]` holds the original's behaviour and runs
only against the original (label `originals`), `KEY [fixed #N]` the
port's behaviour after the fix and runs only against the port (`all`);
`#N` is the issue. Writing expected data is a separate, deliberate
operation with a hyphenated build target, reviewed with `git diff`: to
**record** is to write it from the original (`libs-capture`,
`bug-orig-record`; they accept a result only if two runs with another
memory layout agree), to **update**
is to write it from the port (`libs-update`, like `corpus-expect-update`).
What a record writes is the **recorded original**; the expectation is
never called "the reference": `ref` means the original program (the
corpus configuration `ref`, `ted_ref`, `blitz2_ref`).

## Blitz test sources and the result file

A Blitz test source judges itself: its cases check values with macros,
and the test library writes one entry per check to a result file that the
runner reads. This section is the contract between the source, the
runner's preprocessor and driver, the test library and the result file
(spec `testing/basic-test-interface`). `shared/basictest/` holds the codec
of the result file (`basictest.h`: writer and reader, compiled by the test
library and by the host tools as `lib_basictest`) and the macro text of the
drivers (`macros.bas`); the test library and the runner are written
against them (changes `basic-testlib`, `basic-test-runner`).

### The source form

A source marks its blocks with macros, each on a line of its own (only a
comment may follow it on the line):

| Block | Macros | |
|---|---|---|
| batch init | `!TestBatchInit` … `!EndTestBatchInit` | at most one, before the first case |
| case | `!TestCase{NAME}` … `!EndTestCase` | one or more; `NAME` starts with a letter, holds no `__` and is no token |
| batch end | `!TestBatchEnd` … `!EndTestBatchEnd` | at most one, after the last case |

It checks with these macros; each takes a name as an extra last argument,
a string literal, under which the check is reported:

| Macro | Ok if |
|---|---|
| `!Check{COND}` | `COND` is not 0 |
| `!CheckEq{ACTUAL,EXPECTED}` | the two long values are equal |
| `!CheckQEq{ACTUAL,EXPECTED}` | the two quick values are equal |
| `!CheckFltNear{ACTUAL,EXPECTED,DELTA}` | the two float values differ by at most `DELTA` |
| `!CheckStrEq{ACTUAL$,EXPECTED$}` | the two strings are equal |
| `!Require{COND}` | a precondition: `COND` is not 0; else the block ends at once |
| `!ExpectError{"MESSAGE"}` | announces the runtime error the program expects next |

`COND` of `!Check` and `!Require` is evaluated in a Long context: a Quick
variable or function value fetched while the type so far is Long (the
Long parameter, or a Long left operand) is cut to a Long at once; Floats
and literals with a fraction are not cut. So with `q.q=1.5`
`!Check{q>1}` fails (1>1), as `If l<q` with `l.l=1` is false. Check a
quick value with `!CheckQEq` and a float value with `!CheckFltNear`;
their routines take quick and float parameters, so their values are
evaluated in that type.

```
!TestBatchInit
  w.l=16
  !Require{w=16,"set up"}
!EndTestBatchInit

!TestCase{shape_size}
  !CheckEq{w,16,"width"}
  !Check{w>0}
  q.q=w/32
  !CheckQEq{q,0.5}
  !CheckFltNear{w*1.5,24,0.01}
!EndTestCase

!TestCase{tab_string}               ; a comment is allowed here
  a$="a"+Chr$(9)+"b"
  !CheckStrEq{a$,"ab","tab"}
  Dim a(5)
  !ExpectError{"Array Subscript out of Range"}
  a(9)=1
!EndTestCase
```

### The preprocessed form

The runner's preprocessor gives every check, precondition and announcement
a number `K`, counted from 0 over the whole source in source order, writes
it as the first argument and removes the name and the message. It keeps
the table of `K`, the namespace (case, procedure, `init`, `end`), the name,
the line and the checked expression for the report; nothing of that text
reaches the program:

| Source | Preprocessed |
|---|---|
| `!Check{COND}` | `!Check{K,COND}` |
| `!CheckEq{ACTUAL,EXPECTED}` | `!CheckEq{K,ACTUAL,EXPECTED}` |
| `!CheckQEq{ACTUAL,EXPECTED}` | `!CheckQEq{K,ACTUAL,EXPECTED}` |
| `!CheckFltNear{ACTUAL,EXPECTED,DELTA}` | `!CheckFltNear{K,ACTUAL,EXPECTED,DELTA}` |
| `!CheckStrEq{ACTUAL$,EXPECTED$}` | `!CheckStrEq{K,ACTUAL$,EXPECTED$}` |
| `!Require{COND}` | `!Require{K,COND}` |
| `!ExpectError{"MESSAGE"}` | `!ExpectError{K}` |

The example above becomes:

```
!TestBatchInit
  w.l=16
  !Require{0,w=16}
!EndTestBatchInit

!TestCase{shape_size}
  !CheckEq{1,w,16}
  !Check{2,w>0}
  q.q=w/32
  !CheckQEq{3,q,0.5}
  !CheckFltNear{4,w*1.5,24,0.01}
!EndTestCase

!TestCase{tab_string}               ; a comment is allowed here
  a$="a"+Chr$(9)+"b"
  !CheckStrEq{5,a$,"ab"}
  Dim a(5)
  !ExpectError{6}
  a(9)=1
!EndTestCase
```

### The driver and the library

The driver the runner writes holds the macro text
(`shared/basictest/macros.bas`), calls `TLBegin` with the name of the
result file, jumps over the preprocessed text of the source, and then calls
with `Gosub` the batch init (only if the source has one and a case is
selected), each selected case in source order and the batch end (only if
the source has one), and ends. In the macro text a case is a subroutine
`tc_NAME`: its opening macro refers with `Dc.l` to the label `tc_end_N`
its closing macro defines, so a case without its closing macro does not
compile, and it returns at once when `TLCase` gives 0. The batch init and
end have the fixed labels `tc__init`, `tc__init_end`, `tc__final` and
`tc__final_end`; labels with `__` belong to the driver.

| Routine | Called by | |
|---|---|---|
| `TLBegin path$` | the driver | opens the result file |
| `TLInit`, `TLFinal`, `TLCase(name$)` | the block macros | 1, or 0 on the second return after a failed precondition |
| `TLBlockEnd` | the closing block macros | |
| `TLCheck K,expected,actual` | `!Check`, `!CheckEq` | `!Check{K,COND}` is `TLCheck K,-1,(COND)<>0` |
| `TLCheckQ K,expected.q,actual.q` | `!CheckQEq` | |
| `TLCheckFlt K,expected.f,actual.f,delta.f` | `!CheckFltNear` | compared with mathffp.library, which `TLBegin` opens |
| `TLCheckStr K,expected$,actual$` | `!CheckStrEq` | |
| `TLRequire K,cond` | `!Require` | |
| `TLExpectError K` | `!ExpectError` | |
| (finit) | the end of the program | writes `incomplete` if a block is open, then `end` |

The library is `libs/userlibs/testlib` (user library 81, built as
`variants/bb2-2.10/userlibs/testlib/testlib.obj`; its logic `tlcore.c`
has the host test `testlib_core`). Between `TLBegin` and the end of the
run it allocates and frees nothing, and every routine keeps `d2`-`d7`
and `a2`-`a6` besides the registers of the register rule (the free
memory is the same before and after each routine with the result file
on `CORPUS:out/`; a handler that keeps the file in the Amiga's memory
may take some as it grows). What it writes besides the entries the
routines name:

* A failed precondition writes `require K broken` and `blockend`: the
  library closes the block and its call returns a second time.
* A failed precondition in the batch init breaks every case: each case
  the driver calls gets `block case NAME`, the precondition's `require`
  entry again and `blockend`, and does not run; the batch end does not
  run and writes nothing.
* A failed check in the batch init fails every case: each case gets the
  first failed check of the init again, after its `block` entry, and
  runs.
* In Blitz mode, under `Forbid` or `Disable` (exec's `TDNestCnt` or
  `IDNestCnt` 0 or more) the entries are kept, up to 1024 bytes, and
  written before the next entry made in Amiga mode or by the finit;
  entries that do not fit are lost and followed by `broken entries lost
  while kept`. In Amiga mode each entry is one dos `Write` at once.
* A `checkstr` entry longer than 512 bytes has its two values cut until
  it fits and the last field `cut`; its verdict, and the length and CRC32
  each value carries, are those of the whole strings.
* A call out of order writes `broken` with the reason before its own
  entry: `block inside a block`, `check outside a block`, `block end
  without a block`, `batch init after a block`, `case after the batch
  end`, `second batch end`, `begin inside the run`; a case without a
  name or with one too long for a line is written as `?`, followed by
  `case without a name` or `case name too long`. A state whose guard words a wild write changed writes `broken
  state damaged` once; after that only the finit writes, its `end`.

### The result file

One entry per line, fields separated by TAB, each line ended by LF. `K`
and a length are decimal (no sign, no leading zero, up to 4294967295); a
long value and a CRC32 are `$` and 8 upper-case hex digits; a string value
holds the bytes `$20` to `$7E` other than `\` as they are and every other
byte as `\xHH` (upper-case hex digits), so the string "a" TAB "b" reads
`a\x09b`. An entry belongs to the block of the last `block` entry before
it.

Each string value of `checkstr` is followed by the length of the whole
string and its CRC32, whether the value was cut or not: the standard
CRC-32 of IEEE 802.3, as in zlib and PNG (the polynomial `$04C11DB7`
reflected, `$EDB88320`; input and output reflected; the register
`$FFFFFFFF` at the start, the result XORed with `$FFFFFFFF`): `$CBF43926`
for `123456789`, `$00000000` for the empty string (`basictest_crc32` in
`shared/basictest/basictest_crc.c`). A value shorter than its length is
cut: it is the start of the string. The runner decides `==` and `!=` on a
string value by length and CRC32, so they hold for cut values as well;
`contains` and `matches` take only values that are not cut.

| Entry | Fields after the name | |
|---|---|---|
| `begin` | the format version, `1` | the first entry |
| `block` | `init`, `final`, or `case` and the name (a string value) | a block starts |
| `check` | `K`, `ok` or `fail`, expected, actual (long values) | |
| `checkq` | `K`, `ok` or `fail`, expected, actual (the 32 bits of the quicks, as long values) | |
| `checkflt` | `K`, `ok` or `fail`, expected, actual, delta (the FFP bits of the floats) | |
| `checkstr` | `K`, `ok` or `fail`, expected, its length, its CRC32, actual, its length, its CRC32, `-` or `cut` | `cut`: a value was shortened so that the line fits 512 bytes |
| `require` | `K`, `broken` | only a failed precondition |
| `expect-error` | `K` | |
| `blockend` | | the block ended |
| `broken` | a reason (string value) | |
| `incomplete` | | the program ended inside a block |
| `end` | | the last entry |

The reader (`basictest_read`) refuses an unknown entry, a wrong number of
fields, a malformed value (also one not in the form the writer gives it,
such as lower-case hex or `\x41`; a string value longer than its length,
a complete one whose CRC32 is not that of its bytes, `-` with a cut value
and `cut` without one) and an unknown format version, naming the line.
The example with both cases selected, when `a$` is not `"ab"` and
`a(9)=1` ends the program with the announced runtime error, gives (TAB
between the fields; the host test `basictest_readme` reads every line of
the blocks marked `basictest-result` back with the codec):

```basictest-result
begin	1
block	init
blockend
block	case	shape_size
check	1	ok	$00000010	$00000010
check	2	ok	$FFFFFFFF	$FFFFFFFF
checkq	3	ok	$00008000	$00008000
checkflt	4	ok	$C0000045	$C0000045	$A3D70A3A
blockend
block	case	tab_string
checkstr	5	fail	ab	2	$9E83486D	a\x09b	3	$C42AC338	-
expect-error	6
incomplete
end
```

### The run configuration

How a test of the model runs and what the run must show besides its
checks is its run configuration (change `basic-test-runner`; the logic in
`tools/lib/testconf`, the format in `tools/lib/testtoml`). It is the merge
of the test's own `ID.test.toml` (optional, next to `ID.test.bas`) and the
`tests.toml` files of its directory and the directories above it, up to
and including the first one with `root = true`; without such a file the
test is refused. The nearest file wins per key, the tables (`matrix`,
`asserts`, `asserts."SELECTOR"`) merge per key, and a list value
replaces, never extends. A relative path in a path setting (`files`,
`mount`, `floppy`: the field after the first `|`; `wrap`, `runwrap`: the
field before it) is relative to the directory of the file that sets it;
an absolute path or one with a `:` (an Amiga path) stays as it is. A
placeholder `@NAME@` (`NAME` is `[A-Z][A-Z0-9_]*`) in a string or list
element of a setting or an axis is replaced from the build's environment
file; an unknown one is an error naming file, line and placeholder.

The files are a strict subset of TOML (valid TOML, line by line): one
`KEY = VALUE` per line (a bare key `[A-Za-z0-9_-]+` or a quoted one, no
dotted keys), values a string (`"..."` with the escapes `\"`, `\\`, `\t`,
`\n`, `\r`, or `'...'` without escapes), a decimal integer, `true` or
`false`, or a list of strings on one line; table headers `[NAME]` and
`[NAME."SELECTOR"]`; comments from `#`. Everything else of TOML
(multi-line strings, floats, dates, inline tables, arrays of tables, other
escapes) is refused with file and line.

| Key | Value | |
|---|---|---|
| `compiler` | `port`, `orig195`, `orig210` | blitz2-c (default), blitz2 1.95 (the build of `blitz2.s`), the CD's blitz2 2.10 |
| `libs` | `cd`, `clib` | the CD's libraries, the C libraries (default) |
| `machine` | string | the emulated machine (default `a1200`) |
| `runtime-errors` | `off`, `on` | runtime error checking (default `off`) |
| `rwe` | list | the RWE user libraries the program needs |
| `timeout`, `hang-after` | integer | time limit of a session, silence that ends it as a hang (seconds) |
| `stack` | integer | the stack of the compiler and the program |
| `serial`, `rtg`, `pad` | `off`, `on` | the serial peer, the RTG emulator, inject's `pad` (default `off`) |
| `wrap`, `runwrap` | `PATH[\|ARGS]` | the compiler, the program started through PATH |
| `runargs`, `launcherargs` | string | arguments of the program, options of the launcher |
| `files` | list of `DEST\|SRC[\|TAG]` | files staged as `CORPUS:DEST` |
| `dirs`, `remove` | list | empty directories, files removed after staging |
| `mount`, `floppy` | list of `NAME\|DIR`, `N\|SRC` | read-only volumes, disk images |
| `uae`, `jobline` | list | changes of the emulator configuration, lines of tedstub's job list |
| `labels`, `fixtures` | list | further ctest labels, the ctest fixtures required |
| `cost` | integer | the ctest `COST` |
| `exclude` | list of selectors | the combinations of axis values that are no cell |
| `root` | boolean | only in a `tests.toml`: the search stops here |
| `fixed-orig`, `fixed` | integer | only in an `ID.test.toml`: the issue of the fix of an original bug, of another bug; not both |

The keys from `compiler` to `jobline` are the **run settings**; `labels`,
`cost` and `fixtures` describe the registered ctest test, which is one per
case and side, not per cell. A key the runner does not know is an error
naming file, line and key, and so is a key of a single test (`fixed-orig`,
`fixed`, the tables `deviations`) in a `tests.toml` or `root` in an
`ID.test.toml`. `fixed-orig` needs a declared deviation, `fixed` must have
none.

**The matrix.** The table `matrix` names the axes: each key a run
setting that is no list, each value a non-empty list of its values as
strings (an integer as `"60"`). A list value outside this table is no
axis; any other key in it is an error naming the key. The cells are the
product of the axes minus the cells that an `exclude` selector matches; a
cell is named by its values in the order of the table above, `,` between
them (`orig195,cd`), and the only cell of a test without axes is `default`.
If a nearer file sets a key that a farther one makes an axis, it is no
axis (and the other way round); both in one file is an error. A cell needs
original material if its compiler is not `port` or its libraries are the
CD's; such cells are the side `originals` of a case.

```toml
# tests/bug-orig/libs/tests.toml
root = true
timeout = 90
exclude = ["compiler=orig195,libs=clib"]

[matrix]
compiler = ["port", "orig195"]
libs = ["cd", "clib"]          # three cells: port,cd  port,clib  orig195,cd
```

**Cell selectors** name cells by the values of their settings:
`TERM[,TERM...]` without blanks, a `TERM` is `KEY=VALUE` with `KEY` a run
setting that is no list and `VALUE` one of its values; in the selector of
an assertion `case=NAME` also restricts it to the case `NAME`. A cell
matches if its value of every `KEY` (the axis value, else the setting,
else the default of the table above) is `VALUE`. The more terms, the more
specific the selector.

#### Assertions

An assertion judges what the program cannot state itself: a key `QUERY =
"PREDICATE"` in the table `asserts` (all cells) or `asserts."SELECTOR"`
(the cells the selector matches). The grammar:

```
query      = "rc" | "requester" | "compile" | "compile.log" | "output"
           | "serial" | "inject" | "session" | "end.case" | "end.run"
           | "check." NAMESPACE "." NAME [ ".expected" | ".actual" ]
predicate  = CMP value | ["not "] "contains" string | "matches" string
           | ["not "] "exists" | "count" CMP integer
CMP        = "==" | "!=" | "<" | "<=" | ">" | ">="
value      = integer | string | word
integer    = ["-"] digits | "$" 1*8 hex-digits     ; "$": 32 bits, signed
string     = '"' { char | '\"' | '\\' } '"'
word       = "ok" | "fail" | "not-run" | "done" | "timeout" | "halt" | "none"
```

Blanks may stand around the operator and the value. `NAMESPACE` is the
case, the procedure, `init` or `end` a check stands in, `NAME` the check's
name (a check that a configuration names must have one).

| Query | What | Predicates |
|---|---|---|
| `rc` | the exit code of the program | `CMP integer`, `exists` |
| `requester` | the requesters shown, but the runtime errors the case announced | `contains`, `matches`, `exists`, `count` |
| `compile` | `ok`, `error LINE MESSAGE` or `none` | `==`/`!=` with `ok`, `none` or a string, `contains`, `matches` |
| `compile.log`, `output`, `serial`, `inject` | the compiler's log, the printed output, the serial data, inject's report | `==`/`!=` string, `contains`, `matches`, `exists` (not empty) |
| `session` | how the session ended | `==`/`!=` with `done`, `timeout`, `halt` |
| `end.case`, `end.run` | the end entry of the case, of the run | `exists`, `not exists` |
| `check.NS.NAME` | the check's verdict: `fail` if any of its entries failed, `not-run` without entries | `==`/`!=` with `ok`, `fail`, `not-run` |
| `check.NS.NAME.expected`, `.actual` | the expected, the actual value of its first failed entry, else of its first | `CMP integer` (long values; the 32 bits of a quick or FFP float value, `$00018000` for the quick 1.5), `==`/`!=` string (decided by the length and CRC32 of the whole value, also of a cut one), `contains`, `matches` (strings; only on a value the library did not cut to fit the line: on a cut one the cell cannot be judged); a cut value is reported as its start with `(cut, length N)` |

`contains` holds if the text (a requester, for `requester`) holds the
string; `matches` if a line of it matches the pattern of `outmatch` (`*`
any characters, `#` a decimal number, `\c` the character `c`).

**One assertion per query.** For a query only one assertion applies to a
cell: that of the nearest file, and in it that of the most specific
selector; assertions are never added up. Two different ones of the same
rank are an error naming both. Without one, the defaults apply:

```toml
[asserts]
rc = "== 0"
requester = "count == 0"
"end.case" = "exists"          # a query with a "." is a quoted key
"end.run" = "exists"
```

and every check with entries must have passed, unless an assertion names
it. A test that expects another exit code sets `rc = "== 20"` in its own
file and keeps the other defaults.

**Deviations of the original.** A test of an original bug declares in its
`ID.test.toml` what the original's faulty part changes, as assertions in
the table `deviations."SELECTOR"`, the selector naming the cells with the
faulty part (`libs=cd` for a library bug, `compiler=orig195` for a compiler
bug). In those cells a deviation replaces the assertion of its query;
further deviations of the same table (the actual value of the failing
check) are its validators.

```toml
# tests/bug-orig/libs/shapeslib-frames-maximum.test.toml
fixed-orig = 265

[deviations."libs=cd"]
"check.frames_maximum.width" = "== fail"
"check.frames_maximum.width.actual" = "== 0"
```

**The environment file** of a build (written by CMake) gives the
placeholders and the directory where the search for `root = true` stops:

```toml
top = "/path/of/the/repository"
[placeholders]
TESTDATA = "/path/of/the/test/data"
```

## Original material and presets

Some tests need original material that is not in the repository (the CD,
the Aminet archive of the sources): once

```
cmake -P cmake/fetch-originals.cmake
```

(downloads about 40 MB, see `archive/README.md`; needs no build
directory). In an already configured build,
`cmake --build --preset amiga --target fetch-originals` works too. Without
these data the affected tests are skipped.

Building uses the preset `amiga` (cross compiler for AmigaOS; it also
builds the tools and host tests for it with the native compiler). The test
presets say **where** the tests run:

```
ctest --preset host       # on the host, without emulator (seconds)
ctest --preset emulator   # in Amiberry (label emulator)
ctest --preset all        # both
ctest --preset bug-orig   # original bugs not fixed yet, known port bugs: fail on purpose
ctest --preset oracle     # AmiBlitz3's own compiler checks the expectations of tests/ab3
ctest --preset originals  # compare with the originals (writes no file)
ctest --preset perf       # the benchmark of the libraries: cycles per command against budgets
```

The preset `oracle` (label `oracle`) holds the tests that run the original
tool to validate our expectations: `ab3_cli_ref_1` … `ab3_cli_ref_N`
compile the cases of `tests/ab3` marked REF with AmiBlitz3 v3.14.0's own
compiler (Amiberry without the JIT: the compiler crashes in its exit path
under the JIT when the host is loaded, #976) and compare the program
output with the same expect files; `ab3_cli_ref_broken` and
`ab3_cli_ref_broken_date` check it with a wrong expect file.

The cases of `ab3_cli` and `ab3_cli_ref` run in blocks, one emulator
session and one test each: `ab3_cli_1` … `ab3_cli_N`, `ab3_cli_ref_1` …
`ab3_cli_ref_N` (`tests/ab3/CMakeLists.txt`, `ab3_cli_blocks`). A block
holds at most `AB3_CLI_BLOCK` cases (cache variable; default 70, in a
build with `BLITZ2_COVERAGE`, preset `amiga-cov`, 20: the coverage
compilers need about 5 s per case and a session ends after
`AB3_CLI_TIMEOUT`, 600 s); the cases are spread evenly over the blocks
in their order of the list. The two cases of a `SAME` pair (their
executables must be byte-identical) stay in one block, which may grow
past the size for them; each block checks the `SAME` pairs and `HUNKS`
layouts of its own cases. A new case can move cases into the next block
and change the number of blocks; select the tests by `-R '^ab3_cli'`.
The compiler never changes, so the result changes only with the cases and
expectations: run `ctest --preset oracle` before merging a change of
`tests/ab3` (cases, `cli/`, expect files, `cli.cmake`); `testinfo select`
selects the tests then. `host`, `emulator` and `all` leave them out.

The preset `perf` (label `perf`) holds the benchmark of the libraries
(`perf`, "The benchmark" below): it measures, it does not compare
results, and it starts eight emulator sessions (about 15 s in all), so
`host`, `emulator`, `all` and the others leave it out. Run it for a change
that can make a library command slower or faster.

## host tests

The tools with deliberately broken inputs, the host tests of
the TED core (compiled natively, snitch), the declaration comparison (`declarations_*`: header,
entries, `!libfin` of every built library against the original of the
CD, same libraries in the same order; `blitzlib compare`; the
intended differences of fixed original bugs are listed in
`libs/GROUP/declarations-intended.txt`, each of them must occur; the
same for an RWE userlib, `rwe_declarations_NAME`, with
`libs/userlibs/rwe/NAME.declarations-intended.txt`) and the
instruction comparison (`asmcmp_*`: the routines of runerrlib, debuglib,
dhandlerlib, memacclib that remained in assembler against the originals of the CD,
with `objdump` of the toolchain; `refcheck asm`,
[`tools/refcheck`](../tools/refcheck/README.md); only routines still
equal to the original are listed, a fix that changes one removes it
from the list; label `originals`, preset `originals`: a committed dump of
the routines would be a copy of the CD's code, so it is skipped without
the CD). The declaration comparison reads the declarations of the
originals as committed data, `tests/data/declarations/GROUP.txt` and
`rwe/NAME.txt` (`blitzlib decls`: one line per entry with its name and
help text as readable text, then argument types and numbers in hex; no
code; the format in `tools/blitzlib/README.md`), so it needs no CD;
`declarations_GROUP_data` and
`rwe_declarations_NAME_data` (label `originals`) check that the
originals still give them byte for byte (a changed original fails there;
with broken input `declarations_data_check_wrong`, and
`declarations_data_broken`: one changed byte of the data is a
difference), and `cmake --build --preset amiga --target
declarations-data` writes them (the RWE reference builds first:
`ctest --preset originals -R '^rwe_refbuild$'`). The check of TED's data in the build
(`refcheck data`) takes the intended differences of
`programs/ted/checkdata-intended.txt` (one check per line: C symbol,
`#NNN`, note; each must differ); `ted_checkdata_intended`,
`ted_checkdata_unlisted` and `ted_checkdata_stale` run it with a
deliberately wrong table entry, `blitzlibcompare_single_stale` the
declaration comparison with a stale entry. The formats of the lists and
the rule of the instruction comparison, with examples: `docs/development.md`, "Intended differences".
The token list (`tokens_*`: `bbsrc tokens`
makes it from a compiler and its libraries the way blitz2 builds it for
the editor) must equal `tests/data/ted-tokens.txt` – with the CD's
compiler and libraries (skipped without the CD) and with the ones built
here; `tokens_broken` checks that a missing library is noticed.
`rwe_residents_check_broken`: the comparison of two residents
(`tools/blitzres/check.cmake`) notices other contents. The libraries'
shadow headers of the NDK (`cmake/ndk-calls.cmake`,
`docs/libs/cmdlibs-port.md` "OS calls with a parameter in a3, a4 or a5"):
`ndk_list_ok` runs the configure check of `libs/include/ndk/functions.list`
alone, `ndk_list_missing` (TextFit taken out), `ndk_list_extra` (a
function the NDK does not have) and `ndk_list_pending` (TextFit as
`pending`, a state the list may not hold) with the list changed in
memory, each expecting the error naming the function;
`ndk_poisoned_call` compiles `cmake/ndk/poisoned.c` (a call of
BuildSysRequest, which has no safe form) with the library options and
expects the poison error, `ndk_poisoned_call_broken` the same without the
shadow headers, which compiles; `ndk_varargs_inline` compiles
`cmake/ndk/varargs.c` (OpenWindowTags) and expects only its wrapper, with
the NDK's code and without a warning, `ndk_varargs_inline_broken` without
the shadow headers, where every wrapper of intuition.h is emitted
(`cmake/ndk-test.cmake`; assembler text on stdout, no file).
ccache and `#embed` (`docs/development.md`, "ccache";
`cmake/ccache-embed-test.cmake`): `ccache_embed` (m68k compiler) and
`ccache_embed_host` (host compiler, in the host side) compile a source
that embeds a file with the build's compiler launcher, in a cache of
their own in the work directory: the same compilation twice must be a
cache hit, a changed embedded file must give the object the compiler
gives without the launcher; `ccache_embed_broken` and
`ccache_embed_host_broken` write the embedded file unchanged and expect
the stale cache hit to be reported. Skipped (77) without a launcher
(`BB2C_CCACHE=OFF`). The host side on its own:
`ctest --test-dir build/amiga/host`.
With `-DHOST_SANITIZE=ON` the host side is built with AddressSanitizer
and UBSan. With `-DHOST_COVERAGE=ON` it is built with gcov
(`--coverage -O0`) for the program code the host tests compile (the
sources below `programs/*/src/` and `shared/`; not the tools, the test
sources or the TED tools), and each host test `X` writes its `.gcda`
files below `build/<preset>/host/coverage/X/` (`GCOV_PREFIX`).

## emulator tests

Run in Amiberry; without Amiberry or without the data in `amiga-emulation/`
(`AMIGA-EMULATION-SETUP.md`) they are skipped.

### Running them

* **Parallelism:** the test presets run as many tests at once as the
  machine has CPUs (`"execution": { "jobs": "" }`, CMake presets version 11,
  so CMake 4.3; `perf` keeps 8); the number of emulator sessions is bound
  by the slots alone (below), a test that waits for one sits in a spare job.
  ctest starts the tests that took longest in the last run first, from
  `Testing/Temporary/CTestCostData.txt`. The tests of the emulation library
  that wait for real time (`amiga_emulation_supervisor`, 40 s) have the
  label `emulator` and run in `host_emulator`, not in the preset `host`.
  Each test uses up to `CORPUS_PARALLEL` emulators (default 3 under
  Linux, otherwise 1), as far as there are free slots (a runner that
  holds a slot does not wait for more, it runs its sessions one after
  another). `CORPUS_TIMEOUT` limits a session; reports are in
  `build/<preset>/tests/corpus/NAME.txt`.
* **Run directories:** `corpusrun pass` and `tedab run` work in a
  directory of their own below the pass file's `work=` directory
  (`WORK/run-XXXXXX`, `tests/corpus/runner/runwork.h`), so two runs of
  the same test at the same time (two ctest runs in one build directory)
  are independent. A run that passes leaves nothing in the build
  directory (its directory and `WORK`, if empty, go). A run that does
  not end with 0 stays and prints its directory (`the files of the run:
  DIR`), where its files are for diffing, for example
  `build/<preset>/tests/bug-orig/ID/run-XXXXXX/port/vol/out/` (the
  program's output and executable); it stays until the next run of the
  same pass starts: a run directory no runner holds any more (`flock`),
  the leftover of a failed, crashed or interrupted run, is removed then,
  and a run never removes a directory another run still uses. A run
  without a session (a skip, an error before the sessions) leaves
  nothing. Less than `min_free=` MiB free (pass key, default 64) in
  `WORK` before or after the sessions ends the run with exit status 2,
  `no space left in WORK (...)`, instead of a verdict on files a full disk
  may have cut short.
* **Fixtures and the run directory:** where a test reads what another
  test made, ctest knows it: `corpus` (setup) makes the executables and
  `defaults.xtra` that `ted_ab` reads (`corpus_originals` and
  `ted_ab_originals` likewise), and a cleanup test after the last reader
  (`corpus_cleanup`, `corpus_originals_cleanup`) removes the data. The
  setup's pass publishes its run (pass key `publish=NAME`) in the **run
  directory** `RUN`, `RUN/NAME`; the reader names its input relative to it
  (`corpus_out=run:corpus/clib/vol/out`, `tests/corpus/runner/rundir.h`).
  `RUN` is the environment variable `BB2C_TEST_RUN` of the ctest call,
  else `build/<preset>/Testing/runs/default`, which the cleanup removes
  again (a passed run leaves nothing in the build directory). Nothing
  locks the default directory: **two ctest runs in one build directory at
  the same time each set `BB2C_TEST_RUN`** to a directory of their own
  (`env BB2C_TEST_RUN=(mktemp -d ...) ctest --preset all` in fish), which
  the caller owns: the tests put only `NAME` and `NAME.keep` there and
  remove only those. A reader whose input is missing (the setup has not
  run, or the two runs name different directories) ends with exit status
  2 and a message that names `BB2C_TEST_RUN`, before any session, never
  with 77. A reader that fails marks the fixture it read
  (`RUN/NAME.keep`): the cleanup keeps the data then, for a look, until
  the next run of the setup replaces them. `ctest -R ted_ab` runs the
  corpus pass first (a short run). `ted_ab_parallel` and the TED bug-orig
  passes read no corpus run (`corpus_out=-`; `tedab lint` refuses a
  script with `@expect corpus` or `@xtra norunerr` in such a pass). The
  persistent caches `originals_refbuild` and `rwe_refbuild` are build
  products, not test results, and stay.
* **Work directories of the cmake script tests:** a test that runs a
  `cmake -P` script on a work directory (`-DWORK=`: the library tests in
  the emulator, the TED oracle check, `help_amiga`, the tool tests of
  `tools/`) runs it through `cmake/with-workdir.cmake` (`-DTEST_SCRIPT=...
  -P ${WITH_WORKDIR}`): the script gets a new directory
  `WORK/run-XXXXXX`, or `NAME-XXXXXX` below `BB2C_TEST_RUN` if the caller
  set it (the caller's directory: only the run directory is put there and
  removed). A script that passes or is skipped leaves nothing (the
  directory, and `WORK` if empty); one that fails keeps the directory and
  prints `the files of the run: DIR`. `-DWANT_FAIL=ON` for a test about a
  script that must fail (the selftests whose output ctest matches): its
  failure counts as the pass, a pass is kept. Two ctest runs in one build
  directory share no such directory; `with_workdir` tests the wrapper.
  Not yet through the wrapper: the tests whose script uses `WORK` as the
  parent of a layout of its own (`corpus_record_target`,
  `bug_orig_record_target`, `*_min_free`, the `want.cmake` tests), the
  `inject_*` tests, `tests/ab3`, `tests/bug-port` and the host test
  programs that take a directory (`runner_runwork`, `runner_rundir`).
* **Slots:** over everything (all runners, build directories and
  worktrees of the machine) at most `AMIGA_EMULATION_MAX` Amiberry
  sessions run at once (environment, 0 = no limit; without it the number
  in the file `max` of the slot directory, default 60 % of the online CPUs,
  rounded, at least 1, at most 64: 10 on 16 CPUs; how to change it
  while tests run: `AMIGA-EMULATION-SETUP.md`). A further session waits
  for a free slot (`tools/lib/amiga_emulation/README.md`, "Slots: how runners wait"); the
  wait does not count into the session's timeout, and every emulator test
  has `EMULATION_QUEUE_WAIT` seconds (default 7200,
  `cmake/emulation-queue.cmake`) more in its ctest TIMEOUT for it.
* **Snapshot mode:** a session with a slot starts from a savestate of the
  booted system instead of booting (about 13 s less per session; the
  first session per slot and configuration boots and saves it).
  `AMIGA_EMULATION_SNAPSHOT=0` turns it off. The snapshots belong to the
  build directory and are kept across ctest runs; a session restores one
  only while its key still matches, otherwise it boots, saves anew and
  removes the stale snapshot. `amiga_run --purge-snapshots [BUILDDIR]`
  removes a build directory's snapshots by hand. Details:
  `tools/lib/amiga_emulation/README.md`, "Snapshot mode".
* **A halted CPU:** a session whose emulated CPU Amiberry has halted (a
  double fault, a jump to no memory: it never runs again) ends at once
  with the state `halt` instead of waiting for its time limit
  (`timeout`) (`tools/lib/amiga_emulation/README.md`, "Session states").
* **Logs:** the presets `emulator`, `all` and `bug-orig` set
  `AMIGA_EMULATION_LOG=1` and `AMIGA_EMULATION_TIMEOUT_STATE=1`: every
  session writes Amiberry's log into `amiberry.log` of its working
  directory (in the build directory), and a session that runs into its
  time limit or halts leaves `timeout.txt` there (how it ended, CPU
  registers, whether the CPU is halted, its threads) with the machine's
  state (`timeout.uss`, `timeout.uae`;
  `tools/lib/amiga_emulation/amiga_emulation.h`).

### Libraries

The test program `libs/tests/libs_tests` (snitch, m68k) runs the test
programs of the libraries: `libtest` (`ldivlib`, `lmullib`), the six
`systest*` programs and one program per RWE userlib (`libs/tests/rwe/`).
A program loads **one** library set like the compiler does and writes
every observation (a result, a register, a buffer) as a case; the cases
are compared with the expected data in `libs/tests/expected/NAME.txt`
([The test model](#the-test-model)). The shell variable `LIBSET` chooses
the set: `obj` is the C build of this repository, `orig` the original
(acidlibs from `BLITZ2/ACIDLIBS` of the CD, RWE libraries from the
reference build of their sources). A program cannot see both sets, so
it cannot compare the C build with itself. Selection: the snitch filters
`[sys]`, `[mathstr]`, `[runtime]`, `[system]`, `[cmd]`, `[blit]`, `[gfx]`,
`[med]`, `[libtest]`, `[rwe]`, `"libtest ldivlib"`, `"rwe riarrayslib"`, passed as
`cmake ... -DFILTER="[mathstr]" -P libs/tests/emulator.cmake` or, for
the targets below, as `LIBS_CAPTURE_FILTER`.

**Cases.** The output of a program, `captures/NAME.txt` in the run's
`test` directory, is one header line `# capture NAME 1` and one line
`KEY<TAB>VALUE` per case. The key is `ROUTINE|CHECK|VECTOR|OCC`: the
routine under test, the check text, the input of the case (`VECTOR`,
free text) and the occurrence number of that triple, counted from 1,
which makes a check that recurs with the same input unique. A TAB, `|`
and `\` in a key part are written `\t`, `\|`, `\\`, other bytes outside
`$20`-`$7E` as `\xHH`. A key holds the input of the case, never a result
or an address. The value is `$XXXXXXXX` (a number), `bytes N HEX` (up to
64 bytes; longer buffers `bytes N fnv XXXXXXXX`, the FNV-1a hash),
`"text"` or `null`, `prog` or `NAME+$OFF` (an address inside the test
program, or relative to the base of a library or buffer: an address is
never written as a number), `kept` or `lost d3 a1` ("Kept registers"),
or `not captured: REASON`. The format as the writer produces it:
`libs/tests/capture.h`; as the judge reads it:
`tools/lib/libscmp/libscmp.h`.

A test program writes cases with the macros of `libs/tests/sysload.h`
and `libs/tests/rwe/rwecmp.h` (`#define LIBS_ONE_SET` before the
include): `OBS(what, value)`, `OBSBYTES`, `OBSSTR`, `OBSPTR(what, v,
name, base)` and `OBSPTRBYTES` for addresses, `NOT_CAPTURED(what,
reason)`, `KEEPS(routine, regs)`; in the RWE programs `DO1` makes the
call and writes the kept registers and the popped stack. `check()`
stays for a condition of the program's own (a memory balance, a handle
found) that holds in every run of either set; whatever depends on the
library is an `OBS`, so there is no comparison that could pass without
looking. The summary line `NAME: N checks` counts every `OBS` and every
`kept` case once.

**Expected data**, `libs/tests/expected/NAME.txt` (one file per program,
two for `libtest`; `/libs/tests/expected/** -text diff` in
`.gitattributes`), has the same format. A key may carry a qualifier after
the occurrence number: `KEY [orig]`, `KEY [orig #N]` or `KEY [fixed #N]`.
An unqualified case is run against both sets, `[orig]` and `[orig #N]`
only against the original, `[fixed #N]` only against the C build. The order of the lines does not
matter to the judge, but the writers (`libs-accept`, `libs-update`) put
them in one order, so that a new recording or a fix changes only the
lines of its cases: the header, then the keyed cases ordered by
`ROUTINE`, `CHECK` and `VECTOR`, each compared as text (bytewise on the
escaped spelling; a part that is a prefix of the other comes first),
then by `OCC` as a number (`|2` before `|10`), with `KEY [orig]`
directly before `KEY [fixed #N]` (`KEY [orig #N]` stands alone at the
place of its key); then the block cases `BLOCK:N` by `N` as a number. A file edited by hand is put in that order with
`build/amiga/host/tools/results/results libs-sort libs/tests/expected`;
no check in the build enforces the order, the next writer restores it.
A fixed original bug is such a pair; a case that exists on one
side only is `KEY [orig]` or `KEY [fixed #N]` alone. A case of the
original that the port no longer reaches because of the fix of issue N
(a state the fix removes, so no `[fixed #N]` counterpart is possible) is
`KEY [orig #N]`: judged as `[orig]`, and `fix-scope` counts it to issue
N. A key has at most one of `[orig]` and `[orig #N]`, and `[orig #N]`
never with a `[fixed #N]` case; the issue number is decimal from 1. The judge is
`tools/results libs-compare` (`tools/results/README.md`): per program the
first differing case with both values (`NAME: KEY: expected VALUE, got
VALUE`) and up to ten more, then the cases missing from the run and the
ones it adds; a malformed or ambiguous file is named with its line.

**The operations**

| What | Command | Needs | Writes |
|---|---|---|---|
| test the C build | `ctest --preset all -R '^libs_emulator$'` (label `emulator`, in `all`) | Amiberry | nothing |
| test the original | `ctest --preset originals -R libs_emulator_originals` (label `originals`) | Amiberry, the CD, the RWE reference build (`rwe_refbuild`) | nothing |
| record (from the original) | `cmake --build --preset amiga --target libs-capture` | as the line above | `libs/tests/expected/` |
| update (from the port) | `cmake --build --preset amiga --target libs-update` | Amiberry | the `[fixed #N]` cases of `libs/tests/expected/` |
| either on another machine | the same with `-DLIBS_MACHINE=NAME` | as above | `libs/tests/expected-NAME/` |
| fix scope | `cmake --build --preset amiga --target fix-scope` | git | nothing |

`libs_emulator` and `libs_emulator_originals` only compare: they run the
programs with their `LIBSET`, then `results libs-compare --set obj|orig`.
A changed expected file selects both (`tools/testinfo/rules.txt`). A
failure names the program and the key (`libtest_ldivlib: |routine
count||1: expected $00000002, got $00000000`). The output of the whole run
stays in the work directory, for diffing: the port's in
`build/amiga/libs/tests/libs_emulator/test/captures/NAME.txt` (the
original's in `libs_emulator_originals/`; the program's log is
`test/libs_tests.txt` next to `captures/`), compared with
`diff -u libs/tests/expected/NAME.txt` against it. The case of a long
buffer holds only its hash; the bytes are written by a run in which the
AmigaDOS variable `LIBS_CAP_DUMP` is set to the full key of the case (with
its occurrence number) or to `BLOCK:N`, to `captures/NAME.dump`. Set it
for one test through the environment, for example
`LIBS_CAP_DUMP=BLOCK:294 ctest --preset all -R '^libs_emulator$'`, or
with `emulator.cmake -DCAP_DUMP=KEY`; the run prints the path and the
first 40 lines of the dump and fails if there is none, so do not set the
variable for a whole run of the libs tests. The program dumps a plain
case itself; a block, and an observation inside a block, are dumped
after the run from the program's `NAME.bin` (`results libs-dump`, below).

**Record** (`libs-capture`) runs `libs_tests` twice at the same time (own work
directories and sessions; it needs two free slots to run side by side) with `LIBSET=orig`,
the second time with `LIBS_PAD=4104` (the program allocates that many
bytes first, which shifts the heap and the slots), and
`results libs-accept` writes the expected data only if both runs are
identical. Otherwise nothing is written and the report says
`test not deterministic: NAME` with the differing cases (`NAME: KEY: run
1 VALUE, run 2 VALUE`); the runs stay in
`build/amiga/libs/tests/libs-capture/run1` and `run2`. It also refuses a
program whose original loses a stated register (a `lost` case). A key
that has a `[fixed #N]` case keeps that case, and the original's value
becomes its `[orig]` case; a key with only an `[orig]` or `[orig #N]`
case keeps that qualifier. Such a case whose key the run lacks is
removed and reported. The file is written in the order of
"Expected data". The cache variable `LIBS_CAPTURE_FILTER`
limits the run to a snitch filter (`cmake --preset amiga
"-DLIBS_CAPTURE_FILTER=rwe riarrayslib"`, set it empty again
afterwards; an environment variable of that name has no effect and is
refused by the target, whose comment shows the active filter); the other programs' files stay as they are. Use a tag
filter (`[blit]`) or the full name of a test case, no `*`: the filter is
a quoted argument of the AmigaDOS script, where `*` is the escape
character (`"systest_blit*"` matches nothing, and nothing is written).
The result is reviewed with `git diff libs/tests/expected` and committed
with the change of the scenario. Two identical runs prove the
independence from the memory layout only: date, time and emulator
timing are not varied, so a value that depends on them and happens
to agree twice is not found; `libs_emulator_originals` on another day
is the detector.

**Update** (`libs-update`, no CD) runs the C build once and rewrites the
value of every `[fixed #N]` case. To turn an unqualified case into a pair
after a fix, name it: `cmake --preset amiga "-DLIBS_UPDATE_FIX=KEY#N"`
(several separated by `;`; a cache variable, an environment variable
`LIBS_UPDATE_FIX` has no effect and is refused; the target's comment shows
the active filter), then the target; KEY is the key as written
in the file, N the issue. The case becomes `KEY [orig]` (the old value)
and `KEY [fixed #N]` (the port's value), or `KEY [orig #N]` alone if
the port's run lacks the key; a `KEY [orig]` alone that the port's run
lacks becomes `KEY [orig #N]` the same way. Nothing else changes but the
order of "Expected data"; a fix whose port value equals the expected
value, or whose key is qualified otherwise, is refused. The run's output
stays in `build/amiga/libs/tests/libs-update/run/test/captures/`.
`-DLIBS_UPDATE_NEW=N` (an issue; it needs `LIBS_CAPTURE_FILTER`) writes
the expected data of a program of the filter that has none: every case
of the port's run becomes `KEY [fixed #N]` (`results libs-update --new`;
the data of a machine on which the original does not run, "Another
machine"); a program that has expected data is updated as above.

**A fixed original bug** is therefore two cases in the expected data,
written by `libs-update` with `LIBS_UPDATE_FIX` in the commit of the fix
and reviewed with `git diff`. **No scenario dodging:** a fix does not
change the inputs or the steps of a scenario so that it no longer meets
the bug (spec `testing/expected-results`). **A fix changes only its own
cases:** the target `fix-scope` compares `libs/tests/expected` (and the
expected data of the other machines, every `libs/tests/expected-NAME` of
the base or the checkout) and the bug-orig tests (`tests/bug-orig`, each projected onto cases: one per
section of `ID.expect` and `ID.orig`, `[fixed #N]` with the header
`; @fixed #N`; `tools/lib/bugorigcmp`) of the checkout with its merge
base with `main` (`-DFIX_SCOPE_BASE=BRANCH` for another base) and
reports, with the key, every case added, changed or removed that is not
a `[fixed #N]` or `[orig #N]` case of an issue named in a `Fixes #N`
line of the branch's commits, or its paired `[orig]` case (an `[orig]`
case that becomes `[orig #N]` with its value is no change; a bug-orig
fix: the
header added and `ID.expect` changed; tests of new open bugs are
reported as added cases and reviewed). A
reported case is no error by itself: it is reviewed and named in the
commit with its reason, or gets its own issue and `[fixed #M]` case. The
agent of a fix runs the check and names its result in the report; the
coordinator runs it again before the merge (`tools/results/README.md`,
"fix-scope").

**Kept registers.** The register rule (`docs/libs/deviations.md`,
"Register preservation"; Mark Sibly's LIBSDEV documentation): a
user-level statement keeps a4, a5, a6 and a7, a function also a3 (a
string function leaves a3 at the end of its result); nothing else,
except the registers of an exception, an entry whose known callers need
more (EventWindow a1, EMouseY a0 and the upper word of d0, ...). Internal
tokens the compiler calls, init and finit, nullsubs and error checks keep
their contracts. The program loads marker values before each call and
writes one case per call, `kept` or `lost d3 a1`, for the registers the
routine must keep (a4 and a5 are always added; `d0u` is the upper word of
d0): `KEEPS_STATEMENT(routine, extra)`, `KEEPS_FUNCTION(routine, extra)`
and `KEEPS_STRFUNCTION(routine, extra)` state the rule of the kind plus
`extra`, the registers of the exception (`none`, `a0|a1`, `a0|d0u`), and
`KEEPS(routine, regs)` the whole contract of any other routine
(`d2-d7|a0|a6`, today the intersection of what the original keeps in all
calls of the scenario). A register the original keeps and the rule does
not require is not checked. A stated register that shows `lost` fails
the run with the key, and a recording refuses it; a result in a stated
register (the a3 of `Peek.s`) is exempted for the call with `DONTCARE`.
The summary at the end of `test/libs_tests.txt` lists, as information,
the calls, the mask kept in all of them as a `KEEPS(...)` line, and the
stated set, and marks a routine that returns a result in a4 or a5
(`DONTCARE`). A routine called without a stated set fails once. The
checks of the rule have their test with broken input in
`libs_keeps_wrong` (a function that loses a6 and a word function that
changes the upper word of d0 it states fail, a statement that changes d1
and a0 passes). **Accepted limit:** a register of a contract that the
original keeps in some calls only is not in the set and not compared.

**`not captured`.** A part that cannot be reproduced has a case
`NOT_CAPTURED(what, "REASON")`; the judge skips it on both sides, so the
reason is text for the reader (the 39 DMACONR reads of the F00 phase of
`ciatrklib`: the original writes into the hardware, the C build into a
RAM copy). A value that differs from run to run is made deterministic
first (a fixed seed; `riencryptlib` answers the token of `vallib`'s `Rnd`
with a stub).

**Block programs.** `systest_med` (3.57 million observations),
`rwe_ridisasmlib` (3.06 million) and the two `libtest` programs would be
hundreds of MB as text, or random numbers nobody can read. Between
`OBS_BLOCKS(1)` and `OBS_BLOCKS(0)` every 4096 observations are one case
`BLOCK:N<TAB>COUNT XXXXXXXX first=KEY`, with the FNV-1a hash of the lines
they would have been. The program does not format them: it writes each
observation raw to `captures/NAME.bin` (the records:
`tools/lib/libscmp/libscmp.h`) and, in `NAME.txt`, the line
`BLOCK:N<TAB>COUNT in NAME.bin` where the block case belongs; `results`
makes the block cases from `NAME.bin` when it reads the run (a missing,
cut off or otherwise broken `NAME.bin` fails the compare with the file
and the offset). A differing block is reported with its number and its
range of observations (`rwe_ridisasmlib: BLOCK:294 (observations
1200129..1204224): expected ..., got ...`), and its observations are
written at once to `captures/NAME.block-N.obs` (the first three blocks
of a program; the report shows their first lines). Given another run's
captures with its `NAME.bin` (`results libs-compare --other DIR`, for
example the original's run against the port's), the report names the
first observation of the block in which the two runs differ; a recording
does so for its two runs. `LIBS_CAP_DUMP=BLOCK:N` (or the key of an
observation in a block) writes the dump from `NAME.bin` after the run
(`results libs-dump`). The `NAME.bin` files, about 300 MB per run, are
deleted when the test or the recording passed and kept when it failed;
the environment variable `LIBS_KEEP_BIN` (or `-DKEEP_BIN=ON` of
`emulator.cmake`) keeps them. Limits: a fix inside such a program
changes a block hash nobody can review, and two fixes in one block share
one pair and one issue number; a change that adds or removes an
observation shifts the number of every later block (in a plain program,
the occurrence numbers within a key). A `lost` observation inside a
block is refused by a recording like a `lost` case, named with its key
and block. The two large programs take about
26 and 30 s of the emulator run on an idle machine (the whole
`libs_emulator` about 80 s); its limit is the cache variable
`LIBS_TESTS_TIMEOUT` (default 900 s, `-DLIBS_TESTS_TIMEOUT=N` for a slower
machine; the ctest time limit adds 300 s and the queue wait), that of the
small tests below `LIBS_SELFTEST_TIMEOUT` (60 s).

**Another machine** (`emulator.cmake` `-DMACHINE=NAME`, `-DUAE=KEY=VALUE;...`:
`amiga_run --machine`, `--set`): the libs tests run on the a1200 by
default; a library's path for an older Kickstart runs in a test of its
own with `MACHINE` (and its own expected data `libs/tests/expected-NAME/`
where its cases differ from the default machine's). The machines and
the configuration changes `libs_tests` needs on each are listed once,
in `libs/tests/CMakeLists.txt` (`libs_machines`, `libs_uae_NAME`); the
tests of a machine and the targets take them from there. `libs_tests` is linked with libnix20 (Kickstart
2.0 or newer, not the a500) and needs about 8.5 MB, 7.5 MB of it in one
block: more than a 68000 machine has ("not enough memory available" on
the a500-204 and the a600), so on the a500-204 it runs with
`UAE=cpu_type=68020;cpu_model=68020;cpu_24bit_addressing=false;z3mem_size=32`
(a 68020 with 32 MB Zorro III fast memory, Kickstart 2.04 unchanged).
`libs_emulator_machine` runs `libtest ldivlib` there against the expected
data. `libs_emulator_riwbspritelib_a500_204` runs `rwe riwbspritelib`
there, the library's path below graphics.library V39 (#1151), the C
build only (the original crashes there), against its own expected data
`libs/tests/expected-a500-204/rwe_riwbspritelib.txt`: every case
`[fixed #1151]`.

The cache variable `LIBS_MACHINE` (one of `libs_machines`; empty: the
a1200) makes `libs-capture` and `libs-update` run on that machine, with
its configuration changes, and write its expected data
`libs/tests/expected-NAME/` in place of `libs/tests/expected/`; every
other option (`LIBS_CAPTURE_FILTER`, `LIBS_UPDATE_FIX`) works as on the
a1200. The target refuses a name that is not in the list, and an
environment variable `LIBS_MACHINE` (or `LIBS_UPDATE_NEW`), which would
have no effect. Set it empty again afterwards.
The data of a program whose original does not run on the machine is
made from the port with `LIBS_UPDATE_NEW`, for example the a500-204's:

```
cmake --preset amiga -DLIBS_MACHINE=a500-204 "-DLIBS_CAPTURE_FILTER=rwe riwbspritelib" -DLIBS_UPDATE_NEW=1151
cmake --build --preset amiga --target libs-update
cmake --preset amiga -DLIBS_MACHINE= -DLIBS_CAPTURE_FILTER= -DLIBS_UPDATE_NEW=
```

Once the file exists, `libs-update` with `LIBS_MACHINE` (and without
`LIBS_UPDATE_NEW`) rewrites its `[fixed #N]` cases from the port like
those of the a1200, and `fix-scope` checks every `expected-NAME/` like
`libs/tests/expected/`.

**The checks have tests with broken input** (no CD): `libs_compare_*` and
`libs_accept_*` (host: the judge on fake files: a changed value, a
missing and an added case, a changed block, the qualifiers, a regressed
`[fixed #N]`, a run that is not deterministic, a `lost` case, a block
place without its `NAME.bin`), `libscmp_bin` (the raw block file from
memory: every record formatted, the merge, a truncated record, an
unknown type, a bad length, places and blocks that do not fit, the
dump, the first differing observation, `lost` inside a block),
`libs_dump_*`, `libs_capture_format` (the case writer; block mode gives
the lines and hashes of the text path), `libs_capture_target` (the
record and update targets with a fake emulator run; on another machine
a name not in the list, `LIBS_UPDATE_NEW` without a filter, not a number
or for a record), `libs_update_new_wrong` (`--new 0`), `fix_scope_*` (the
fix-scope check; `fix_scope_target` also on the data of another machine:
a changed, an added and a removed case), `libs_emulator_wrong` (an emulator run whose output has
a changed case fails with that key), `libs_emulator_lost` (a datalib
whose thunks change d2: `lost d2`, the run fails),
`libs_emulator_machine_unknown` and `libs_emulator_machine_uae` (a
machine the emulation does not know, refused by `amiga_run` before a
session; a configuration change without `KEY=`), `libs_keeps_wrong`
(a wrong `KEEPS` statement, a lost register of the rule) and `libs_mem_wrong` (the check "all memory
back", the task's own AllocMem/FreeMem balance of `rwe/rwemem.h`: a
block not freed, a foreign block freed, no allocation seen; and the marks of
`rwe_mem_mark()` that the "old one freed" checks of `systest_runtime` use). The test programs of the original's bugs
(`libs/tests/bugorig.c`) run the C build only and judge the expected
behaviour, like the other bug-orig tests ("bug-orig tests").

`rwe_residents_cd`: the residents `blitzres` writes have the contents
(`blitzres dump`: types, macros, constants without the unused node
bytes) of those blitz2 2.10 of the CD and blitz2-c save with Create
Resident of the same constants as Blitz source (`blitzres source`,
one corpusrun pass with `run=off` and tedstub `resident=`;
`libs/userlibs/rwe/residents.cmake`; skipped without the CD).

### Extensions of blitz2

`blitz2_ID` (`tests/blitz2/`): the extensions of blitz2
(`docs/blitz2/extensions.md`), which have no reference to compare with.
They are **expect tests**, the same suite as the
[port regression tests](#port-regression-tests): blitz2 compiles and runs
`ID.bas` (header `; @runerrs on|off`, default off) with the C libraries
(`clib`), and the result must equal `ID.expect` (the sections of the
bug-orig tests, `tests/corpus/runner/bugorig.h`; one corpusrun pass with
`expect=` and `regression=on`). There is no reference session and nothing
of the CD is used: the verdict is FIXED (the expected behaviour) or
REGRESSED (exit status 1), label `emulator`, skipped (77) only without
Amiberry. Both suites run without the CD:

```
ctest --preset all -R 'blitz2_|regression_'
```

The expectation is the correct behaviour, not the original's: a fix (of
an original bug or of the port) that changes what an expect test shows
changes its `ID.expect` in the same commit and names the issue
(`#N` in the commit message), like a [fixed test](#fixed-tests); the
program is not edited to avoid the difference.

The tests of the intrinsics (`intrinsic-*.bas`, the library functions
blitz2 compiles inline, `openspec/specs/compiler/intrinsics`) check the
compiled code as well as the value: the program counts the JSR
instructions (`$4EB9`, the form of every library call) between two labels
around the statement (`jsrs{?a,?b}`, the routine in each test) and prints
the count, so its expectation says which cases call the library and
which do not. The self test `blitz2_selftest_intrinsic_selftest_wrong`
(`tests/blitz2/selftest/`, an expectation of no call for `Int` of a
Float) must come out REGRESSED with the case in the report (its line
`; @want REGEX`, the ctest `PASS_REGULAR_EXPRESSION`: the observed
`int f: calls 1`).

### Corpus

The corpus (`tests/corpus/`): `corpus` (71 test programs
without runtime error checking), `corpus_runerr` (135 with), `corpus_req`
(3 with blocking requesters, behind the lock `reqclosetest`),
`corpus_serial` / `corpus_serial_runerr` (a program that talks over the
emulated serial port to a peer on the host; the bytes it sends over the
serial port are compared as well). Every pass is two ctest tests that
compile the same sources in several configurations and run the programs:

* `NAME` (`corpus`, `corpus_runerr`, ...; in `emulator` and `all`):
  clib (blitz2-c with the C libraries of the build) and, with a prev
  build, prev (executable equal to clib's, "The prev build"). No test
  of the original: it runs every program without the CD. The files the
  programs load (the sounds of `t110_audio`, the modules of
  `t111_tracker` and `t112_med`) are generated by the build from text
  descriptions `tests/corpus/data/NAME.spec` (`mktestdata`, the format
  in `tools/lib/mktestdata/mktestdata.h`) and staged by `corpus` and
  `corpus_runerr` as `CORPUS:data/NAME`, the same files for every
  configuration; configuring fails on a `CORPUS:data/NAME` in a source
  without its description and on a description no source names
  (`tests/corpus/check-data.cmake`, tests `corpus_data_broken_source`,
  `corpus_data_broken_spec`, `corpus_data_ok`). A changed description
  changes the programs' output: record them again (with the CD). No
  pass of `all` stages `blitz2:defaultdbug`: the programs are
  executables (CREATE EXECUTABLE), and an executable never contains the
  debugger module, also not with runtime error checking (it links
  `runerrlib` and `rundebuglib`; only COMPILE/RUN from the editor hands
  the module to `dhandlerlib`), so the passes with runtime error checking
  (`corpus_runerr`, `corpus_req`, `corpus_serial_runerr`) run the same
  with and without the CD.
* `NAME_originals` (`corpus_originals`, `corpus_runerr_originals`, ...;
  label `originals`, only in `ctest --preset originals`): ref (original
  blitz2 2.10 = `BLITZ2/BLITZ2` of the CD, libraries of the CD) and port
  (blitz2-c with the libraries of the CD: executable equal to ref's) and
  orig-clib (the original blitz2 2.10 with the acidlibs and deflibs of this
  build: are the C libraries compatible with the original compiler?); the
  serial passes judge ref's serial bytes against committed files
  (`expect/corpus_serial/` and `expect/corpus_serial_runerr/`, a host test
  checks that they stay equal: ref sends the same with and without runtime
  error checking). Skipped without the CD. This is where the
  original stays a reference of behaviour; a fix that makes the port
  differ makes the program's file a pair of cases (below).

The output of every configuration must be the committed expected data
`tests/corpus/expect/PASS/NAME.out` (what the program writes, with the
`rc=N` line behind it; one directory per pass, because some programs
write something else with runtime error checking) and, for the serial
tests, `NAME.serial` (the bytes sent over the serial port), byte by byte
(`expect_dir=` of `corpusrun pass`,
[`tests/corpus/runner`](corpus/runner/README.md); the judge is
`tools/lib/corpusexpect`). Each file is one case of the test model, key
`PASS|NAME|OBS|1` (`OBS` out, serial). Where original and port agree
the file is unqualified; a fix of an original bug that changes a
program's output makes it the **pair** `NAME.orig.out` (the original's
output) and `NAME.fixed-N.out` (the port's after the fix of issue N),
likewise for `.serial`. Each configuration is judged against the file
that applies to it:

| Configuration | Judged against |
|---|---|
| clib, prev (`NAME`) | `NAME.out` or `NAME.fixed-N.out` |
| ref (`NAME_originals`) | `NAME.out` or `NAME.orig.out` |
| port, orig-clib (`NAME_originals`) | `NAME.out`, or either file of the pair: the report says which matched, `output as expected (matched [fixed #6])` |

port and orig-clib mix an original and a ported part (whether a fix
shows depends on the bug), so they accept either file; a port that slips
back to the original's output fails in clib, a changed original in ref.
Against the original only outputs and errors count, not executables
(spec `testing/original-comparisons`): `port` (the configuration
`exeinfo`) is judged by its output like the others, and its executable is
compared with ref's for information: a different one is reported
(`exe differs (informational: HASH, HASH)`, the first 12 digits of each
SHA-256) and does not fail, a missing one (the port does not compile what
the original does) fails. So a change that only touches the code (an
optimization) needs no file and no issue here. The comparison with the
previous commit stays byte-exact: `prev` (`exe:clib`) must write the
executable `clib` writes, a change is named in `PREV_INTENDED`. A
difference names the configuration, the file and the first differing line.
A file of another form (a `NAME.exe`, `NAME.orig.exe` or
`NAME.fixed-N.exe` is an unknown observation: a stale file is refused), an
unqualified file beside a qualified one, two `fixed` files, a pair of
equal files, one file of a pair without the other, and a missing file
fail the pass before any session (exit status 2, with the file names).

The files are written by two targets, never by a test, and reviewed with
`git diff tests/corpus/expect`:

```
cmake --build --preset amiga --target corpus-expect-record   # from the original; needs Amiberry and the CD
cmake --build --preset amiga --target corpus-expect-update   # from the port; needs Amiberry (and the CD)
```

* `corpus-expect-record` (`tests/corpus/record.cmake`) runs every pass's
  ref twice (`corpusrun pass --observe ref`, the second with a Stack
  16384 bytes larger: another memory layout) and accepts a program only
  if both runs agree (`results corpus-accept`); otherwise it reports
  `test not deterministic: PASS:NAME` with the first differing line and
  leaves the program's files as they are (the target fails). It writes
  `NAME.out` / `NAME.serial`, or the `[orig]` file where the program has a
  pair (the `[fixed #N]` file stays); it writes no executable.
  `-DCORPUS_RECORD_FILTER=PASS` or `PASS:NAME`
  (several separated by `,`) limits it; the others are not touched.
  The proof of determinism is limited to the memory layout.
* `corpus-expect-update` (`tests/corpus/update.cmake`) writes from the
  port: clib's outputs (`corpusrun pass --update-expect clib`), a new
  program's files, a `[fixed #N]` file rewritten; a changed unqualified
  file is refused with the program's name and the first differing line,
  unless `-DCORPUS_UPDATE_FIX=PASS:NAME#N` (several separated by `,`)
  names it: then it becomes the pair, `NAME.orig.out` with the old
  content and `NAME.fixed-N.out` with the port's. It writes no
  executable. On an unchanged tree it leaves every file as it is.

A fix that changes a corpus output names the program in
`CORPUS_UPDATE_FIX`, runs the update, and commits the pair with its
`Fixes #N` line (never the program, to avoid the difference); fix-scope
(`tests/corpus/expect`, each file a case) then sees only its own cases.
With a prev build the configuration prev still gives the old output: the
commit names the program in `PREV_INTENDED` (`corpus:NAME`,
`corpus_runerr:NAME`, ...). The checks have tests with broken input: the
malformed forms without the emulator (`corpus_expect_malformed_*`: both
files, two `fixed`, an unknown qualifier, an equal pair, a lone file, a
missing file, a `.exe` file alone or as a pair),
`corpus_expect_fixed_regressed`
(clib gives the `[orig]` output), in the `originals` preset
`corpus_originals_orig_changed` (ref differs from the `[orig]` file, port
matches the `[fixed #1]` one), `corpus_originals_exe_differs` (port's
executable differs from ref's, the output is as expected: passes with the
difference reported) and `corpus_originals_exe_differs_output` (the same
with a wrong expected output: fails), all on the program
`t01_int` with expected directories made in the build directory; the
targets' scripts in `corpus_record_target` (fake runs: a program not
deterministic, a pair, a filter, a failed run) and `corpus_update_fix`
(refused without FIX, the pair with it). The logic is tested by
`corpusexpect` (`tools/tests`) and `corpus_runner`.

`corpus_cd`: all 724 sources of the CD compiled with blitz2-c (like bb2's
bb2corpus: `.xtra` of the sources, assigns to CD directories, directory
of the relative INCLUDEs) and compared with `tests/data/blitz2-cd.tsv`
(result, error line and message; size and hash with masked trap blocks
are the original's executable, reference data: another executable of a
source that compiles is reported, `executable differs (informational)`,
and does not fail; host tests `cdrun_compare` for the comparison and
`cdtable` for the table update: a different result, error line or message
fails, another executable alone does not). The
table is generated by the original blitz2 2.10:

```
cmake --build --preset amiga --target blitz2-cd-reference   # about 35 s
```

A row whose note begins with an issue number (`#940: … (2.10: error 383)`)
is a note row: it holds the port's compile result for an intended
deviation (`docs/blitz2/deviations.md`, kind F: an error the original does
not give, another error line, a source the port refuses), and the note
names the reference's result: `(2.10: error LINE)` where the original
compiler refused the source at line LINE (the result and line fields of
its error row), or its masked hash `(2.10: HEX)` where the original
compiled a source the port refuses. A row gives that result when it is an
error row at that line, whatever its message. A port that compiles a
source to another executable (an optimization) needs no note, no issue and
no row change; an older hash note on a source the port still compiles is
obsolete (`blitz2-cd-update` drops it). Both targets write
the table through `results cd-update` and `results cd-reference`
([`tools/results`](../tools/results/README.md)) and keep the note rows:

```
cmake --build --preset amiga --target blitz2-cd-update      # from the port's result, about 35 s
```

`blitz2-cd-update` rewrites the table from the port's run (after a fix
changed a compile result): a row with the reference's compile result keeps
the reference's size and hash (the report counts the rows with another
executable), a note row keeps its note, and loses it where the port's
result is the reference's again. A row whose compile result changed
without a note is written and reported (`changed without a note`); add
`#NNN: … (2.10: HEX)` or `#NNN: … (2.10: error LINE)` by hand, with the
old result the report names. `blitz2-cd-reference` writes the original's result and
leaves the note rows as they are; a note that names another result than
the original's is an error (stale note) and nothing is written, and so is
a note row without a well formed `(2.10: …)` part. On an unchanged tree both leave the table byte for
byte as it is. `blitz2_cd_reference` (label `originals`) checks the
committed table that way without writing it: the run of the original,
`results cd-reference` into the build directory, and the result must be
the committed table byte for byte.

### The expression matrix

The expression matrix (OpenSpec change `expression-type-matrix`, spec
`testing/expression-matrix`; the generator
[`tools/exprmatrix`](../tools/exprmatrix/README.md)) tests the type rules
of `docs/blitz2/expression-types.md` cell by cell: a cell is a context
(a Byte, Word, Long, Quick or Float destination, a Long, Word, Quick or
Float parameter, `If`, the first or a later array index, `Print`), an
operator, the operand kinds in order (five variables, four constants,
a string) and a value set (A: inside every range with a positive
fraction, B: negative and beyond the Word and Quick ranges); every cell
is one line of a generated source that prints its key and its value.
The description `tests/corpus/tests-exprmatrix/matrix.txt` makes 40,561
cells. Each was measured with 2.10, 1.95, the port with the CD's and with
its C libraries and AmiBlitz3 v3.14.0; the expected value comes from the
model of `tools/lib/exprmatrix/model.c` (the original's rules and the
rules with the fixes, each rule naming its entry in the user's
decisions), the measured values it does not compute (a Float power, the
original's `If` on a Float, AmiBlitz3's values where they differ) from
`tests/corpus/tests-exprmatrix/measured.tsv`.

**The classes.** A cell is looked at once (index cells twice: unchecked,
only where every compiler reads an element of the array, and with runtime
error checking, where a cell outside the array is the runtime error
"Array Subscript out of Range"). Where the original, the expected value
and the port agree the cell is an **agreement** cell (a recorded rule or
the exact result gives the value) or a **baseline** cell: no recorded
rule covers it (an overflow, a bit operator or shift with a Quick or
Float, `MOD` with negative or fractional operands, `^`), so the
original's measured value is kept as a regression check - a fix changes
these cells only where it is meant to, and after the fix the expected
values are regenerated; it is not a decision that the value is right. A
cell whose value an issue's fix changes is **fixed-N** (the port gives
the expected value) or **open-N** (the port still gives the original's);
**noncompile** cells are refused by the originals and the port.

**The tests**, all written by the generator:

* the corpus passes `corpus_exprmatrix_NAME` (unchecked) and
  `corpus_exprmatrix_runerr_NAME` (the checked index cells, with a
  `SetErr` handler in front of every cell, so a runtime error prints
  `KEY !runtime` and the program goes on): one source of one context,
  value set, mode and class (`tests/corpus/tests-exprmatrix/CTX-SET[-u|-c]-agree|base|fixN-NN.bas`,
  at most `cells-per-source` cells, 2000: every group is one source, up to
  3,300 lines and 125 KB, which the compiler takes in both builds), each
  a pass and an emulator session of its own (a program can disturb the
  next one in a session); the expected files are
  `expect/corpus_exprmatrix[_runerr]/NAME.out`, for fixed cells the pair
  `NAME.orig.out` / `NAME.fixed-N.out` ([Corpus](#corpus)); the
  `_originals` runs check them against 2.10;
* the bug-orig tests of the open cells,
  `tests/bug-orig/compiler/compiler-expr-ISSUES-CTX-SET[-u|-c]-NN`, whose
  header names `BUG(orig) (#N)` and the cells, with `ID.orig` recorded
  from 1.95 ([Recorded originals](#recorded-originals)); they come out
  REPRODUCED until the fix, whose commit adds `; @fixed #N`
  ([Fixed tests](#fixed-tests)). A test of cells that need two fixes
  (`compiler-expr-1178-1189-...`) comes out **DIFFERS between the two
  fixes** - after the first the port neither reproduces 1.95 nor meets the
  expectation - which is no regression; its header says so, and the
  commit of the second fix marks it fixed;
* one test per operator and operand classes for the refused cells
  (`tests/bug-orig/regression/compiler-expr-refused-OP-L-R`) and for the
  cells a fix will refuse (`compiler-expr-N-refused-OP-L-R`, bug-orig);
* the AmiBlitz3 cases of the agreement and fixed sources without runtime
  error checking (`tests/ab3/cli/exprmatrix/`, in `ab3_cli_N` and the
  oracle's `ab3_cli_ref_N`): the sources as `.ab3` without the cells
  AmiBlitz3 refuses (and the `If` cells on a folded Float constant, where
  AmiBlitz3's d1 depends on the line before), compiled with `-r`; their
  expectation is the output of blitz2-ab3 in AB3 mode (the port's
  compiler with AmiBlitz3's libraries), AmiBlitz3's measured output is
  `REF_EXPECT` where it differs (`docs/ab3/variant.md`, "Differences to
  AmiBlitz3's compiler").

**Left out** (`tests/corpus/tests-exprmatrix/left-out.txt`, with the
reason): cells without an expected value (a question to the user: a
power whose operands a fix changes, so that its value is not measured),
`If` cells on a Float whose original value is what d1 holds and depends
on the libraries (#1187), and cells where the port differs.

**Updating.** The generated files are committed;
`cmake --build --preset amiga --target expr-matrix-update` writes them
from the description and `measured.tsv` (after a change of the
description, the model or a fix's rules; review with `git diff`), and
the host test `exprmatrix_committed` fails when a committed file is not
the generator's (`exprmatrix_committed_broken`: an edited and an extra
file are named). A changed bug-orig test needs its `ID.orig` recorded
again (`-DBUG_ORIG_RECORD_FILTER=ID`, `bug-orig-record`). A new
measurement goes through `exprmatrix classify`: `-m` writes
`measured.tsv`, `-q` the questions file (one question per class of
cells no recorded rule covers, with example cells, for the user; not
committed).

### Site examples

The website shows Blitz Basic programs in fenced ` ```blitzbasic ` blocks
(`site/content`, and the introductions and command descriptions of the
library pages in `site/data`). `tests/site-examples/` compiles every one
of them with blitz2-c and runs those that can be run. The manifest
`tests/site-examples/examples.txt` lists **every block once**, by its file
and its number in the file (`NAME MODE FILE#N [rwe=LIB...] [REASON]`,
[`siteexamples`](../tools/siteexamples/README.md)):

* `run`: compiled and run in the emulator (the launcher `reqtrap`
  answers requesters), the output with the `rc=N` line compared with
  `tests/site-examples/expect/GROUP/NAME.out`.
* `compile`: compiled only; the REASON says why it is not run (it opens
  a screen or a window, waits for input, needs a file, a device, a
  library or a display the test machine does not have, installs an
  interrupt, is a fragment that needs the program around it, or its
  output varies or depends on the system). There is no other way to leave
  an example out: a block of the site without an entry, an entry without
  a block (the example changed its number or went away) and a block
  listed twice fail the build step that writes the sources
  (`siteexamples extract`) and the host test `site_examples_manifest`,
  which also prints the counts (`siteexamples: N examples, R run, C
  compiled only`).
* `rwe=LIB`: the RWE userlibs the example needs (`libs/userlibs/rwe/LIB.c`).
  Examples of one variant of those libraries (`rwe-1999`, `rwe-1999-eval`,
  `rwe-1999-ahi`: `libs/userlibs/rwe/CMakeLists.txt`) form a group of their
  own: its pass uses the variant's acidlibs and deflibs, the userlibs in
  `blitz2:userlibs/rwe` and a token list that has their commands; the
  other examples are the group `default`.

The build step tokenises the sources like TED (`tedsrc tok`) and the
passes run them, one emulator session each: `site_examples_run` and
`site_examples_compile` (and `_rwe_1999`, `_rwe_1999_eval`,
`_rwe_1999_ahi` for the groups that have examples of the kind), label
`emulator`, no CD needed. The run passes are a corpus pass with
`expect_dir=` (clib; with a prev build also prev, whose executables must
equal clib's; not for the groups with userlibs); the compile passes use
the runner's `run=off` with `must_compile=on` (`corpusrun pass`: a source
the compiler refuses fails the pass, the report names the error and its
line), one configuration is enough. The expected files are written by
`cmake --build --preset amiga --target site-examples-expect-update` (the
port's output; a changed output is refused by the runner, a new example
gets its file; review with `git diff tests/site-examples/expect`), never
by a test. An example whose output varies from run to run
(`Rnd`, the date, addresses, the system) is `compile`.

Checks with deliberately broken input: `siteexamples_tool` and
`siteexamples` (host: a block without an entry, an entry without a block,
a block listed twice, an unclosed fence, malformed manifest lines),
`site_examples_run_broken` (a run example with a wrong expected file must
fail with the first line that differs), `site_examples_compile_broken` (a
source that does not compile must fail a compile pass), `corpus_runner`
(the key `must_compile`).

### TED A/B

`tests/ted-ab/`: the scripts `tests/ted-ab/scripts/*.drv` played with
`teddrive` against TED in the combinations R5 (original TED + blitz2
2.10), C5 (TED-C + 2.10), Cc (TED-C + blitz2-c) and Rc (original TED +
blitz2-c); every run is checked against the expectations of the script,
and the runs of a script are compared pairwise (result, requesters,
screen dumps, files). The requesters are compared as teddrive logged them
when it first saw their window (title and gadget labels), without the
file requester's lines (gadgets with id 0): the requester adds one per
directory entry while it reads the directory, so their number at that
moment depends on the timing (#1021). The list is compared instead as
teddrive logs it at `wait freq`, once the requester is idle (`files TITLE
"LINE" ...` in the report). Two tests:

* `ted_ab` (label `emulator`, preset `all`): Cc with the port's
  libraries (`libs=` of the pass file; Pc on top with the prev build),
  the corpus programs and `defaults.xtra` from the clib run of `corpus`
  (fixture, "Fixtures and the run directory" above). Needs no CD: without it the scripts that read it (`@source
  K`, `@expect cd`) are left out and listed as `NOCD` in the report;
* `ted_ab_originals` (label `originals`): the four combinations
  (`-DTED_AB_COMBOS=R5,C5`, the reference first) with the CD's libraries
  and the ref run of `corpus_originals` (fixture); needs the CD and the
  reference build of `ted.s`. Against the original only results count
  (`exe_info=1` in its pass file): the executable of the port's compiler
  (`Cc`, `Rc`) that differs from the corpus' (`@expect corpus`), and such
  a file in the pairs `R5-Rc` and `C5-Cc`, is reported
  (`informational`) and does not fail; `R5` and `C5` (the original's
  compiler), every other file, the dumps and the return codes stay strict,
  and so does `ted_ab` (`all`: the port against the port). Tests:
  the cases `exe_info` of `ted_ab_tedab` (`tests/ted-ab/host`).

Selection: `-DTED_AB_FILTER=02-*,29-cmp-*`, sessions at the same time
`-DTED_AB_PARALLEL=N` (default 5, as far as emulator slots are free);
reports in `build/<preset>/tests/ted-ab/ted_ab.txt` and
`ted_ab_originals.txt`, about 13 minutes for the four combinations.
`-DTED_AB_TIMEOUT=SEC` (default 120) limits a session in wall-clock
time; on a host under load the emulator runs several times slower, and
the limit can hit a script that is still running. A run the limit cuts
off while its log was written within the last 10 s goes again once, in a
session of its own, and the session is reported as slow (`tedab run:
... slow session`, `== session sNNN slow` in the report, `verdict=slow`
in its `@stat` line; #1022); a run whose log stood still, or that is cut
off a second time, is a `TIMEOUT`.
Known bugs (`; @known REASON` in the script) are not run and are listed
as KNOWN in the report; an exact name in the filter runs them anyway.
`ted_ab_lint` checks the scripts without the emulator; a script with
`@expect corpus` or `@xtra norunerr` in a pass without a corpus run
(`corpus_out=-`) is a problem (`ted_ab_lint_corpus_dash`, with its twin
`ted_ab_lint_corpus_dash_ok`), and a reader whose corpus run is missing
ends with exit status 2 and names `BB2C_TEST_RUN`
(`ted_ab_fixture_missing`, `ted_ab_fixture_missing_env`). The runner is
`tedab run` (`tedab lint` for the lint; the screen dumps by hand with
`tedab info|text|cmp`):
[`tests/ted-ab/runner/README.md`](ted-ab/runner/README.md).

**A fixed TED bug** (`; @fixed #NNN PAIR...` in the script, NNN the
issue of the bug): once TED-C no longer shows a bug of the original TED,
the runs of the original and of TED-C differ. The commit that fixes the
bug adds the line to the scripts that show it, naming the pairs that now
differ, as the report names them (`R5-C5`, `Rc-Cc`, `R5-Rc`, `C5-Cc`;
judged where both combinations run, `ted_ab_originals`).
A named pair must differ: FIXED, in the report
`R5<->C5: FIXED #NNN (differs)` with the differences below it. A named
pair that is the same is BROKEN (the fix does not show, or it was undone:
the directive is stale) and fails the test. The pairs not named are
compared as before: they must be the same. One line per issue, a pair
at most once, and both combinations of a named pair must run (`@combos`);
`ted_ab_lint` refuses anything else. Example
(`tests/ted-ab/lint/fixed/fixed-sample.drv`, checked by
`ted_ab_lint_fixed`; `ted_ab_lint_fixed_wrong` checks scripts broken on
purpose):

```
; Round trip of t08_tokquirk after a fix of TED bug #123 (the issue number
; of the example): the original TED (R5, Rc) and TED-C (C5, Cc) now differ,
; so the pairs R5-C5 and Rc-Cc must differ; R5-Rc and C5-Cc stay the same.
; @source T t08_tokquirk
; @roundtrip
; @fixed #123 R5-C5 Rc-Cc
timeout 10
wait req "OKEE DOKEE"
gad 1
wait screen
dump start
menu 0 2
wait freq "Name to save file as"
freq ok
wait screen
dump saved
menu 0 9
wait exit
```

### Reference build of the RWE sources

`rwe_refbuild` (fixture `rwe_refbuild`, about 20 s; `corpusrun cd`,
[`tests/corpus/runner/README.md`](corpus/runner/README.md), with
`source=` lines over the staging directory, `RWE_REFBUILD_PARALLEL`,
`RWE_REFBUILD_TIMEOUT`; table
`build/<preset>/libs/userlibs/rwe/refbuild.tsv`) compiles the source of
every `RWE_LIBS` entry and fails if one does not compile.
It does not start the emulator again while nothing changed: after a
successful run, `corpusrun cd` (`cache=` of the pass) writes a record of the
digest of its inputs (the staging directory with the sources, the
residents and `LIBNUMS.BB`, the CD's `blitz2` and libraries, the pass
file, the `corpusrun` executable with `amiga_emulation` and its templates)
and the hashes of its outputs to
`build/<preset>/libs/userlibs/rwe/refwork/inputs.sha256`; the next
run with the same digest and all outputs unchanged ends at once
("no emulator run"). Anything missing or different means a full run
(host test `cdrun_cache`). `CDRUN_FORCE=1 ctest …` runs it anyway, e.g.
after changing the ROM, the hardfiles or Amiberry, which the digest
does not cover.

`rwe_refbuild_NAME`: the result equals the CD binary byte for byte, for
every entry of stage A (not for those in `RWE_REFBUILD_NEWER`, whose
reference is a newer binary); `rwe_refbuild_selftest` takes the dry
entry `RWE_REFBUILD_SELFTEST` (REFERENCE `refbuild`) through the
declaration comparison. These tests have the label `originals`; the
tests of other presets that need the reference build (the declarations
of a `refbuild` entry, `libs_emulator_originals`, the bug-orig tests of
such a library) require the fixture, and ctest starts a fixture's setup even
for a test outside the label filter.

### The benchmark

`perf` (labels `perf` and `emulator`, `ctest --preset perf`;
`tests/perf/`, spec `testing/performance-budgets`) measures what each
command of the C libraries costs in 68000 cycles and fails when one is
slower than its budget. Only the preset `perf` runs it. It is one test per
program (`perf_bmath`, `perf_bstr`, ...), each in an emulator session of its
own; the preset runs them side by side (`jobs` 8), about 15 s in all. The
cycle values do not depend on the host's load or on what else runs
("Speed" below), so the tests need no `RUN_SERIAL`.

* **The programs:** `tests/perf/bench/*.bas`, one per area (`bmath`,
  `bstr`, `bgfx`, `binput`, `bmisc`, `bdisp`, `bmod`, `bproc`, `brec`, `bbblit`, `bstc`, `banim`, `bfns`, `bchunky`, `bmapshapes`, `bsnow`, `bzone`, `bcmp`), tokenised by
  `tedsrc tok` like the corpus. In Blitz mode each runs one
  `For i=1 To N ... Next` loop per command (`N` = 2000 for most) and times
  it with a CIA-B cascade: timer A divides the E clock by 100, timer B
  counts its underflows, so one unit is 1000 CPU cycles (0.5 cycles per
  call at `N` = 2000). Back in Amiga mode it prints one line per loop,
  `TEST T N PER` (`T` units, `PER` calls per iteration or `frame`). Its
  first loop, `empty`, costs 126 to 130 cycles per iteration; that is
  subtracted, so a value includes the call and argument code the compiler
  generates, and for blits the wait for the blitter. `emptylong` (the
  empty loop again, 150000 iterations) must come out at 0. `bmod.vblmod`
  is the empty loop while PlayModule plays: its value is the cycles per
  PAL frame (142102 cycles) the interrupt takes. In `bmod` the `empty` loop
  is the very loop the module plays under (the subroutine `lp`, 150000
  iterations, run before PlayModule and after a `VWait` while it plays), so
  the value does not move with the code layout of the libraries (padding
  medlib by 0 to 18002 bytes: 2298 to 2328; a loop at another address gave
  2256 to 2423); a session whose snapshot was made anew still varies by
  about 110.
  `bproc` has the string procedure loops (a Statement and a Function with a
  string parameter and a local string, and a Statement whose local string
  grows: memlib's locfree) in a program of their own: every call
  allocates the local string with exec's AllocMem and frees it again, and
  AllocMem and FreeMem walk a free list whose shape is the history of the
  program's allocations, so in `bstr` they moved by up to 2 % whenever a loop
  was added there (and differed by 375 cycles between a session of the program
  alone and the session of all programs). Its targets are measured the same
  way, in a session of its own (the pass `perf_bproc.pass`, tag `orig`), not
  in `perf-original`'s one session; keep its setup as it is. `brec` is the
  same for a recursion: a Statement with a string parameter that calls itself
  from depth 20 down to 0, 21 calls per iteration, the value per call; its
  target too comes from its own pass (`perf_brec.pass`, tag `orig`).
  `bbblit` measures every command of an RWE userlib (`bb_blitlib`: the
  per-frame blits and the set-up commands). Its header line `; @rwe NAME...`
  names the libraries, as in a bug-orig test ("A bug of an RWE userlib"):
  the program is tokenised with a token list of blitz2-c, the libraries'
  variant and the named libraries; `clib` gets the variant's acidlibs and
  deflibs and the C build in `blitz2:userlibs/rwe/`, `orig` the CD's
  libraries and the library's reference. Such a program is not in
  `perf.pass`; its targets come from its own pass (`perf_bbblit.pass`,
  tag `orig`) after the reference build (`ctest -R rwe_refbuild`).
  `bstc` measures the S404 decruncher of `ristcdecrunchlib` (shared with
  `ristonecrackerlib`) on real data: one `STCDecrunch` of ancient's
  `test_C1.pack410` (152089 bytes of text) per loop, and the call on a
  file of one byte. A header line `; @file DEST|PATH` gives a program a
  file, `CORPUS:DEST` in the session (`@ANCIENT@` in PATH: the files of
  ancient that `cmake/fetch-originals.cmake` fetches, `@CD@` the CD's
  directory `archive/cd/ultimate-blitz-basic-2.1`); without the file
  the program is left out (configure says so).
  `banim` measures `RINextAnimFrame` of `rianimlib`, the per-frame call of
  a game: an ANIM5 of the CD (`BUI9/SHED/IFF/INTRO.ANIM`, 320 x 256 x 3,
  its frames 3 to 14 a hundred times) and an ANIM7 the program builds in
  memory (320 x 256 x 5, long data, 2400 longs per frame); the value is
  the cycles per frame.
  `bfns` measures `FNSPrint` and `FNSLength` of `rifnslib` (a score or
  a message printed per frame): the built-in font, a text of 32
  characters, a 320 x 256 x 3 bitmap; the value is the cycles per call.
  `bchunky` measures `richunkylib`: `RaycastSky`, a ray caster's call per
  screen column, with the heights 1, 10, 100 and 200, and `ChunkyToPlanar`
  of a 320 x 256 chunky bitmap into a 320 x 256 x 8 bitmap (4 calls per
  loop: more would overflow the timer's 16 bits).
  `bmapshapes` measures `rimapshapeslib`: `BlitMapShape` (a map editor's
  call per tile) of a 16 x 16 and a 32 x 32 map shape, `BlitMapShapeSize`
  and `BlitGroup` of four members, into a window on the Workbench screen.
  Blitz mode owns the blitter, and `BltBitMapRastPort` waits for it: the
  program gives it back (`DisownBlitter_`) for its loops. It has no
  targets: the original library's init calls `AllocVec` (exec V36), so
  on Kickstart 1.3 the program crashes with it at start-up (#1141; the
  original's values on an a600 are in the comment of `budgets.txt`).
  `bsnow` measures `LDoSnow` of `risnowlib`, the call per frame of a snow
  field, on two fields of 500 points: snowfall (an empty 320 x 256 x 3
  bitmap, a flake per row falling) and snowpile (a ground and obstacles,
  flakes move aside and stop); 200 calls per loop.
  `bzone` measures `Zone` and `ZoneTest` of `rizonejoylib` (a game's
  per-frame test of a point against the zone table): table 0 with 12
  rectangles and 4 circles in its first 16 of 256 zones, a point in none
  of them, in a rectangle and in a circle, and `ZoneTest` of the 16 zones
  and of one zone; the A500's 68000 runs the library's 68000 mode.
  `bcmp` measures no library but the code blitz2 writes for comparisons
  with a default-typed (Quick) variable (a Long against a Quick in both
  orders, `If n/2=Int(n/2)`, a constant on the right, a `While`
  condition; #1177): its targets are the values of the original's code,
  measured with the prev build (`budgets.txt`), not by `perf-original`.
* **The run:** `tests/perf/perf.cmake` (through `cmake/with-workdir.cmake`)
  runs the configuration `clib`, this build's `blitz2` with its C
  libraries, of the pass `build/<preset>/tests/perf/perf_NAME.pass` (the
  one program NAME) with `corpusrun pass --observe`: the machine `a500`
  (68000, Kickstart 1.3, ECS) with cycle-exact CPU, memory and blitter
  (`cpu_cycle_exact`, `cpu_memory_cycle_exact`, `blitter_cycle_exact`,
  `cycle_exact`, `cpu_speed=real`) in warp mode (`warp=true`), runtime
  error checking off. Then `results perf --budgets` (`tools/lib/perfbudget`)
  computes the cycles per call, prints the table and judges it; with
  `-DPROGRAM=NAME` `perf.cmake` keeps the lines `NAME.*` of `budgets.txt`
  (a program without a line fails). `perf.pass` (all programs in one
  session) is for the target `perf-original`.
* **Speed:** the emulated machine waits for the real time by default, so
  one session of all seven programs took 168 s. Two things make it faster
  and leave the values as they are (measured on 2026-11-15, 127 commands,
  several runs of each setup; the values of one setup differ from each
  other by as much as those of different setups do):
  * *Warp mode* (`uae=warp=true`: Amiberry's turbo emulation; only the
    wait for the host's clock goes, the cycle-exact CPU, chipset and
    blitter run as before): one session of all programs takes 30 s, one
    program 5 to 13 s. `cpu_speed=max` is not this: it speeds the CPU up
    against the chipset (values 10 to 40 % lower, the same 165 s), and
    `turbo_emulation=true` in the configuration file does nothing.
  * *One session per program*, side by side: the CIA timer counts emulated
    cycles, so a loaded host (24 busy loops on 16 cores) gives the same
    values. A program run alone differs by a few cycles per call from the
    same program in the one-session run (the memory layout of the run
    moves the timer phase); that is as much as two runs of either differ.

  A repeated run differs by up to 3 cycles per call for the commands the
  CPU alone does; loops that wait for the blitter or follow the frame vary
  more (Cls by 0.6 %, `bgfx.poly3` by 0.4 %, the PlayModule frame value by
  8 %). The first run after a session slot's snapshot was made anew (the
  first session of a configuration in a slot, or `amiga_run
  --purge-snapshots`) gives other values for `bmod.vblmod`, `bgfx.cls`,
  `bgfx.poly3` and `bmisc.ntsc`. A loop that varies by more than its
  plain budget's margin carries its own tolerance in `budgets.txt`
  (below). Without Amiberry or the emulation data the test is skipped
  (77).
* **The budgets:** `tests/perf/budgets.txt`, one line per command,
  `PROGRAM.TEST BUDGET TARGET LOOP`. `BUDGET` is the highest allowed
  value: the measurement of the build that set it (the larger value of
  two runs) plus 1 %, rounded up, plus 3 cycles. A loop whose repeated
  runs differ by more than 1 cycle per call and by more than that margin
  absorbs (it waits for the blitter or follows the frame: Cls, Circle,
  BBlit, QBlit, Print, PlayModule, ...; also a few small loops whose
  runs differ by 2 to 3 cycles) is written `B~T`: `B` the highest value
  of at least five runs (the current table: 38 runs, ten with the snapshots of the slots
  made anew) of the setup of the test, `T` the tolerance, twice the
  spread of those runs (rounded up); `B + T` is
  the highest allowed value (a failure names both: `budget 200 + tolerance
  16`). The table marks such a command `(varies, tolerance T)`. A step that
  changes such a loop measures five runs again and sets `B` and `T`
  anew. `TARGET` is the original's value (the same programs with
  the CD's `ACIDLIBS` and `DEFLIBS`), the goal: shown with the factor to
  it, never judged. A command over its budget fails the test with its
  name and both values (`bmath.peekb: measured 217.0 cycles per call,
  budget 130`); so do a measured command without a budget and a budget
  without a measurement.
* **Changing a budget:** a step that makes a command faster sets its
  budget anew in the same commit (the new measurement plus the
  tolerance) and names the cycles before and after; a budget that goes up
  is a decision named in the commit. A new loop needs its line in
  `budgets.txt`.
* **The original's values:** the target `perf-original` (not a test;
  needs the CD and Amiberry) runs the same pass with the CD's libraries
  (configuration `orig`) and prints its table without budgets: the
  `TARGET` column.
* **The checks:** `perf_budget_broken` judges the committed observation
  `tests/perf/selftest/obs` (Peek.b at 217.0 cycles per call) against a
  budget of 130 and must fail with the command and both values;
  `perf_budget_ok` judges it against the right budget and must pass (no
  emulator, both in `host` and `all`). `perf_budget_program_ok`,
  `perf_budget_program_bstr` and `perf_budget_program_bdisp` check the
  filter of one program on two programs' budgets (`selftest/budgets-two.txt`):
  bmath passes, bstr has a budget without a measurement, bdisp has no
  line. The host test `perfbudget`
  (`tools/tests/test_perfbudget.cpp`) covers the arithmetic, the judge and
  the malformed observations and budget files.

## The prev build

The acceptance rule of CLAUDE.md (a rework changes no result; a fix
changes only the results it names) is a test: the project can build the
Amiga side of the previous commit next to the current build and compare
the results of both. It builds the project a second time (about 30 s on
16 cores), so it is only made on request:

```
cmake --preset amiga -DBLITZ2_PREV=ON   # once: the option stays in the cache
cmake --build --preset amiga            # also builds build/amiga/prev/
ctest --preset all                      # compares
```

`BLITZ2_PREV` is `ON` (the commit before `HEAD`, `HEAD~1`: the first
parent of a merge), any revision (`-DBLITZ2_PREV=HEAD~3`, a hash, a
branch), or the absolute path of a build directory of another commit
(then nothing is built). A revision is taken from `git archive` into
`build/<preset>/prev/src`, so the checkout and its uncommitted changes
are untouched, and built there by the ExternalProject `blitz2_prev`; its
results are in `build/<preset>/prev/blitz2/` like those of the current
build. Every `cmake --build` follows the revision: after a commit the
previous commit is another one, and `prev/` is built afresh. Without the
option nothing of this is configured.

The prev build needs no network: it takes the snitch source of the
current build (the one its configure fetched into `_deps/snitch-src`, or
the `FETCHCONTENT_SOURCE_DIR_SNITCH` it was given) if the commit pins the
same snitch (the `URL_HASH` in `cmake/Snitch.cmake`). Its configure gets it
as `FETCHCONTENT_SOURCE_DIR_SNITCH`, which the top-level `CMakeLists.txt`
passes on to its host ExternalProject. Older commits do not pass it on
and their `cmake/Snitch.cmake` looks for it at
`../../ab3c/build/amiga-release/_deps/snitch-src` relative to its own
directory, so `prev/ab3c/build/amiga-release/_deps/snitch-src` links to
it. A commit that pins another snitch downloads its own; the build says
so (`prev: ... pins snitch SHA256=...`). CLI11 (`cmake/CLI11.cmake`) is
passed on the same way. snitch and CLI11 are the build's only
`FetchContent` dependencies; every build other than a prev build
downloads them into its own `_deps`. The originals (`archive/cd`, ...)
are linked from the checkout.

What is compared (without a prev build `prev_results` is skipped, not
passed; "without" means `BLITZ2_PREV` unset or `OFF` - a leftover
`build/<preset>/prev/` from an earlier prev build is ignored, tests
`prev_stale` and `prev_stale_broken`, `cmake/prev-stale-test.cmake`; and the corpus, `ted_ab` and the others below run without their prev parts):

* `prev_results` (host test, `results prev`,
  [`tools/results`](../tools/results/README.md)): `blitz2/acidlibs`,
  `blitz2/deflibs` and every library file (`libs/**.obj`,
  `variants/**.obj`: acidlibs, deflibs, their variants, the userlibs; not
  the copies in `CMakeFiles` or in the run directories `run-*` of test
  runs)
  byte by byte. A difference fails and names the file. A file the
  commit adds is counted, not judged. Intended differences (a fix names
  them in its commit): `-DPREV_INTENDED=ON` lets every file differ, or
  `-DPREV_INTENDED="libs/acidlibs/2dlib.obj;blitz2/acidlibs"` lists the
  files (paths relative to `build/<preset>`; each listed file must
  differ: a stale entry fails like a difference). The other entries of
  the list (below) are only checked for their syntax by `results prev`. The
  tool and its tests with broken input: `prevcmp_*` (`tools/tests`, small
  build directories) and `test_prevcmp`.
* the corpus: a fourth configuration `prev` (`config=prev|exe:clib|…`:
  the previous commit's blitz2 with its libraries) must write the
  executable that `clib` writes and give the expected output (like every
  configuration, "Corpus"), so that a changed code generation or library
  shows as `prev: EXE DIFFERS` or `prev: ... OUTPUT DIFFERS` in the report
  of `corpus`, `corpus_runerr`, `corpus_req` and the serial passes.
  `exe:BASE` / `output:BASE` in a `config=` line of the pass file
  compares with the configuration BASE instead of the reference (with
  `expect_dir=` the output is compared with the expected file).
* `ted_ab`: the combination `Pc` (TED, blitz2 and libraries of the
  previous commit) is added to `Cc` and compared with it (the pair
  `Cc-Pc`, reported like the others). A change of what TED does shows
  there. (The pair `C5-P5`, P5 the previous commit's TED with blitz2
  2.10, runs only where a pass names P5 in its combinations.)
* the sources of the compiler tests, compiled (not run) by the previous
  commit's compiler as well: its executable, or its error, must be the one
  this build's compiler gives, like the corpus's `prev` configuration but
  with `run=off` and without an expectation (what the program does is the
  test's own check). The compile-only passes have `exe:clib` as the corpus
  and use no emulator time for running. Every one works in a run directory
  of its own (`corpusrun pass` and `corpusrun cd`: `work=/run-XXXXXX`;
  `ab3_cli*` and `corpus_cd_prev_check` through `cmake/with-workdir.cmake`):
  two runs at the same time do not share files, a run that passes leaves
  nothing, a failed one stays and is named:
  * `corpus_cd_prev`, `corpus_cd_ab3_prev`: the 724 sources of the CD
    (`corpus_cd`, `corpus_cd_ab3`) compiled with the previous commit's
    `blitz2` / `blitz2-ab3` and the CD's libraries; their table is
    compared row by row with the one `corpus_cd` / `corpus_cd_ab3` wrote
    (`corpusrun cd`, pass key `current=`; `cdcmp_prev` in `tests/corpus/runner/cdcmp.h`): result,
    line, message, size and the hash of the executable with the trap
    blocks zeroed, unlike the comparison with the original, where the
    executable is information only. They run after the test they compare
    with (fixtures `corpus_cd_table`, `corpus_cd_ab3_table`) and need the
    CD and Amiberry. `corpus_cd_prev_check` (CD and Amiberry, no prev
    build) is their test with broken input: tables made from one source
    (the same, a changed hash, a listed difference, a stale entry, a row in
    one table only) and `cdrun_source` the malformed pass keys.
  * `site_examples_run`, `site_examples_compile` and their variant groups
    (`site_examples_run_rwe_1999` ...): the prev configuration is in every
    group, with the variant's libraries and userlibs below the prev build
    directory.
  * `compiler_prev`: every source of `tests/blitz2` and every
    compiler bug-orig test (`tests/bug-orig/compiler`, the compiler's port
    regression tests `regression/compiler-*`) that needs nothing special
    in its pass (no `@pass` lines for the compile, no runtime error
    checking) in one pass: one session per configuration for all of them.
    A test that has them (a userlib or `inject` wrapper, joblines, other
    machines, an include, runtime error checking) has its own pass and
    test `NAME_prev` (`bug_orig_ID_prev`, `regression_ID_prev`,
    `blitz2_ID_prev`: `cmake/prev.cmake`, `prev_compile_test`), configured
    by the same keys as the test itself (the lines for a program run are
    left out). A test with a Compile/Run job (`jobline=run=...`) compiles
    with runtime error checking on (blitz2's start options; its defaults
    cannot be saved) although its pass does not say so: its pass gets
    `mask_traps=on`, the trap blocks of the executables are zeroed before
    the comparison as with runtime error checking. Not compared, with the
    reason in the CMake file:
    `compiler-maxs-libjsr` (#335: addresses of the compiler's memory in the
    executable) and three tests where the compiler gives up before the job
    (no result: `compiler-optreqprep-allocmem`, `compiler-wb-info-progfile`,
    `compiler-wbexit-returncode`); the tests with `@rwe` userlibs are left out (their variants' libraries).
    `compiler_selftest_*_prev` (no prev build needed) are their tests with
    broken input: a source whose include differs between `clib` and `prev` fails
    and names the source and the line to quote; the same one passes; a
    listed difference passes as `(intended)`; a stale entry and an entry
    for a source that is not in the pass fail.
    A prev session that ends without a result (timeout, crash of the previous
    compiler) does not fail the pass by itself: the sources without a result
    differ like any other and are judged against `PREV_INTENDED` (named: passes,
    `(intended)`; unnamed: fails; a named source with a result stays STALE); a
    lost session without any differing source fails. The other sessions still fail
    the pass, and every configuration keeps its own session, so a crash hangs
    nothing else. Tests with broken input: `compiler_selftest_lost_intended_prev`
    and `_unnamed_prev` (prev's compiler is `hangstub`, a program that never ends).
  * `ab3_cli_prev_N`: the cases of the block `ab3_cli_N` compiled by the
    previous commit's `blitz2-ab3` (`tests/ab3/cli-prev.cmake`, `cli.cmake`
    with `COMPILE_ONLY`; two emulator sessions at the same time): the
    executables (`results exe-same`: the trap blocks zeroed, as in the
    corpus) and the console output of every case must be the same. The
    tests with broken input are `ab3_cli_prev_same`, `_differs` (prev's
    `blitz2-ab3` is another program), `_intended` and `_stale`
    (`cli-prev-test.cmake`, no prev build needed); `ab3_prev_entries_ok` and
    `_unknown` check the configure-time test of an `ab3_cli_prev:CASE` entry
    (`check-prev-cases.cmake`).

A fix changes results on purpose: its commit names them (CLAUDE.md), the
differences that report must be exactly those, and `-DPREV_INTENDED`
names them for all of these comparisons with one rule: a listed difference
must really exist (a stale entry fails), an unlisted difference fails, and
`ON` allows every one. The entries (`;`-separated, no blanks):

| Entry | Meaning |
| --- | --- |
| `libs/acidlibs/2dlib.obj` | a library file (`prev_results`), path relative to `build/<preset>` |
| `corpus:NAME` | the source `NAME` (without `.bb2`) in the pass `corpus`: its `prev` configuration may differ from `clib` (executable) and from the expected output or serial data |
| `corpus_runerr:NAME`, `corpus_req:NAME`, `corpus_serial:NAME`, `corpus_serial_runerr:NAME` | the same for the other corpus passes |
| `ted_ab:CASE` | the `ted_ab` case `CASE`: the pairs `C5-P5` and `Cc-Pc` may differ, at least one must |
| `ted_ab:CASE:C5-P5`, `ted_ab:CASE:Cc-Pc` | only that pair |
| `corpus_cd_prev:PATH`, `corpus_cd_ab3_prev:PATH` | the CD source `PATH` (below `ultimate-blitz-basic-2.1`, as in `tests/data/blitz2-cd.tsv`): its row may differ from the one of `corpus_cd` / `corpus_cd_ab3` in any field |
| `compiler_prev:ID`, `bug_orig_ID_prev:ID`, `regression_ID_prev:ID`, `blitz2_ID_prev:ID` | the compiler test source `ID` in that compile-only pass (`compiler_prev` for the tests without special needs, else the test's own pass `NAME_prev`): the executable or the compile result of `prev` may differ from `clib`'s |
| `site_examples_run:NAME`, `site_examples_compile:NAME`, `site_examples_run_rwe_1999:NAME` ... | the site example `NAME` in that pass (the ctest name of the pass, `_` for `-` in the group) |
| `corpus_exprmatrix_NAME:SOURCE`, `corpus_exprmatrix_runerr_NAME:SOURCE` | the source of a corpus pass of the expression matrix (one pass per source, e.g. `corpus_exprmatrix_dl_A_agree_01:dl-A-agree-01`): its `prev` configuration may differ from `clib` |
| `ab3_cli_prev:CASE` | the case `CASE` of `ab3_cli` (any block; a name that is no case is refused when CMake configures) |

The part before the first colon is the ctest name of the test, so an
entry says which comparison it is for: the same source is listed once per
pass it differs in (`corpus` and `corpus_runerr` run the same sources),
and a source that is not in the pass (a typo) is stale. Entries without a
colon are library files, others are checked for their syntax by every
runner (an unknown test, an empty name, a pair that is no `C5-P5` or
`Cc-Pc` are errors). For the tests of the compile-only comparisons the
configure step checks the name as well: the test part of an entry must be a
corpus pass, `ted_ab` or a comparison that is made (`prev_register` in
`cmake/prev.cmake`; a mistyped name or a test that is left out, like
`bug_orig_compiler_maxs_libjsr_prev`, is a CMake error naming the entry;
tests `prev_entries_*`).
`PREV_INTENDED` is passed to the runners as `intended=ENTRY` lines of
their pass files; a `ted_ab` run with `TED_AB_FILTER` does not call a
case it did not run stale. In the reports a difference reads
`prev: EXE DIFFERS (intended)` (corpus) or `C5<->P5: INTENDED` (`ted_ab`),
a stale entry `STALE corpus:NAME: ...`, and every run with differences
ends with a line to quote in the commit message:

```
prev differs in: PREV_INTENDED="corpus:t01_int;corpus:t02_float"
```

Configure again after changing the list, and set it back to `OFF` for the
next commit. The parsing and matching is `tools/lib/previntended`, tested
by `test_previntended` (listed and different, listed and the same, not
listed and different, `ON`, a pair, malformed entries), the pass file keys
by `corpus_runner` and `ted_ab_tedab`.

## bug-orig tests

Tests of original bugs (`BUG(orig)`). Original bugs are fixed
deliberately (CLAUDE.md "Original bugs are fixed deliberately"); the
tests check the *correct* (expected) behaviour. As long as the bug is
not fixed the test **fails** (REPRODUCED); the failure message names the
bug. Label `bug-orig`: `host`, `emulator` and `all` exclude it, only the
preset `bug-orig` runs these tests. A test whose issue was closed with
the label `as-designed` (the original tests the input only with runtime
error checking, BB2 style; the code note reads `By design (#N)`) may
stay: it comes out REPRODUCED and documents the behaviour without the
check. **A fix turns its test into a fixed
test in the same commit** (header `; @fixed #N`, [Fixed tests](#fixed-tests)):
it then runs in `emulator` and `all` and must come out FIXED there.
The TED scripts (`tests/bug-orig/ted/`) have their fixed form too
(`; @fixed #N` without a pair, [bug-orig tests of TED](#bug-orig-tests-of-ted)).
The snitch tests of `libs/tests/bugorig.c` have no fixed form yet
(OpenSpec change `expected-results-testing`); a fix of such a bug names
its test in the report. Naming: ctest `bug_orig_ID` (ID with `-` as `_`),
snitch test case `ID`; the issue names the test. In the snitch programs they carry the hidden tag `[.bug-orig]`,
so a plain run of the program skips them too (`libs_tests "[bug-orig]"`
or the name runs them). Tests in the emulator also have the label
`emulator` and are skipped (77) without Amiberry. So far
(`libs/tests/bugorig.c`, in Amiberry, only the C build of the
libraries, no CD needed, one emulator run each, time limit
`LIBS_BUG_ORIG_TIMEOUT`, default 60 s):
`bug_orig_cliargs_parpath_cli` (ParPath$ of a CLI argument: lock 0 to
NameFromLock), `bug_orig_crunch_implode_overread` (Implode reads
behind its data: the data end at the end of the highest memory region,
Amiberry logs the accesses to the unmapped memory behind it,
`log_illegal_mem`; `emulator.cmake -DILLEGAL_WATCH=ON` evaluates the
log), `bug_orig_crunch_decrunch_corrupt` (Deplode, CrMDecrunch,
PPDecrunch with corrupt data write outside the destination; guard
bytes), `bug_orig_crunchlib_crm_minsecdist`,
`bug_orig_crunchlib_crm_overlap_4gb`, `bug_orig_crunchlib_crm2_readtab`,
`bug_orig_crunchlib_pp_readd1` (one bug of the decrunchers each, #115
to #118), `bug_orig_debuglib_dout2_length` (dout2, the string output
of the debugger's evaluate, called directly; #126, fixed: it passes),
`bug_orig_debuglib_findoff_flags` (#127, fixed: it passes),
`bug_orig_sortlib_word_pivot` (Sort of a one element word array reads
behind it, the array at the end of the highest memory region as for
Implode; #275, fixed: it passes).

### With a Blitz program

**bug-orig tests with a Blitz program** (`tests/bug-orig/`, runner:
the corpus runner, `corpusrun pass` with `expect=`; verdict in
`tests/corpus/runner/bugorig.h`): a program that triggers the bug is
compiled and run in Amiberry by the port (port: blitz2-c with the
libraries of the CD, clib: blitz2-c with the C libraries), one emulator
session each (in parallel). The original (blitz2 1.95 with the libraries
of the CD, the reference `ref`) does not run in the normal test: what it
did is committed as `ID.orig`, recorded once ([Recorded
originals](#recorded-originals)). Without the CD the configuration `port`
is not judged and the run goes without `blitz2:defaultdbug` (also with
runtime error checking, `; @pass runtime_errors=on`: the programs are
executables, which never contain the module); the CD is still needed by
one whose source names `BB2:` (the volume is mounted only for those), and by one that
mounts it with `; @pass mount=BB2|@CD@` because its crash depends on the
memory layout, which the volume changes (`blitzcoplib-maxslice-zero`: a
time out with it, a halt of the CPU without). A fixed test: only clib,
nothing of the CD, [Fixed tests](#fixed-tests). What a
configuration shows (the *observation*) are sections: `[session]`
(`done`, `timeout` for a crash or hang, `halt` for a crash that halted
the emulated CPU), `[compile]` (`ok`,
`error LINE MESSAGE`, `none`), `[run]` (output of the program with
`rc=N`), `[blitz2]` (what the compiler or the wrapper writes besides
the log, with `rc=N`), `[log]` (the log of tedstub). The expectation is
the **correct** behaviour, given only for the sections that matter
(`[session]` not given: `done`); a line ending in `*` matches by
prefix, a line `...` any number of lines. Verdict per configuration:
FIXED (matches the expectation), REPRODUCED (shows what the original
shows, that is `ID.orig`: the normal state, the test fails), DIFFERS (neither:
the port does not reproduce the original). **A valid bug-orig test** is one
whose original does *not* match the expectation; if it does, the test
is BROKEN (the original does not have the bug, or the expectation
describes the bug) and fails with that message, checked on `ID.orig`
before any session starts. The test passes only
when every configuration is FIXED. Exit status of `corpusrun pass`: 0 FIXED,
1 REPRODUCED, 2 error (also a missing `ID.orig`), 3 BROKEN, 4 DIFFERS.
`bug_orig_verdict` (host) checks the
verdict with inputs from memory, REPRODUCED, DIFFERS and BROKEN against
an observation that was written to a file and read back, without a
reference (among them an expectation equal to the
buggy output); `bug_orig_selftest_broken` (label `emulator`, not
`bug-orig`) runs such a broken test on a recorded file and must come out
BROKEN (it needs no session). Reports: `build/<preset>/tests/bug-orig/ID.txt`, the
observation of each configuration in `.../ID/run-XXXXXX/TAG/observation`
(a test that fails keeps its run directory and prints it).
Helpers (`tests/bug-orig/amiga/`): `inject` runs the compiler with
faults (no free signal, failing AllocMem of a size, also only after
SKIP of them, Read or Write error (a Read also of a file that
OpenFromLock opened from a Lock of the name), failing Examine, failing
OpenLibrary or OpenDevice of a name, failing CreateProc, failing OpenWindow
(the N-th, `openwindowfail=N[/COUNT]`; self test `inject_openwindow`), an UnLock of a
lock that a file owns after OpenFromLock counted and not done
(`ownedunlock`), DisplayAlert answered at once, exec's Alert of the process recorded and not shown (`alertlog`:
e.g. the AN_SemCorrupt that exec raises for one ReleaseSemaphore too
many and would otherwise show later, at a random moment; a dead-end
alert stops the process, the session ends by its time limit),
recorded allocations and FreeMem of another size than allocated;
with `doslist` whether the DOS list can be locked for writing after the
program (a read lock left behind is reported and released, so the
session goes on: `ridoslib-doslist-locked-at-end`);
with `freefill=XX` every block of the process is set to XX at its
FreeMem, as a memory checker does (a read after the FreeMem finds XX,
`animlib-freeanim-after-free`);
with `pad` 64 zeroed bytes behind every allocation, also a hunk of
LoadSeg, a FreeMem inside an allocation is reported and not done, and
one whose 64 bytes are no longer zero is reported (written behind); with
`guard` the 64 bytes get an address-dependent pattern instead, checked
at FreeMem and at the end: writes behind a block are reported; with
`guardseg` also 32 KB behind the hunk of the command itself, which is
otherwise loaded before the patches (a data block of a library overrun
at the end of a Blitz program, `freqlib-64-lines`; self test
`inject_guardseg`); for the
compiled programs with `runwrap=@INJECT@|...`) and reports what the
command leaves behind (library opens minus closes of the process, its
current directory and whether it unlocked the start lock; with
`libs=PATH` the directory PATH is in the `LIBS:` assign while the command
runs, for a library the Workbench does not have, e.g. the real
`req.library` of the rireqlib tests, `@REQLIB@` (`archive/aminet/ReqLib28`,
`cmake -P cmake/fetch-originals.cmake`; `file=libs/req.library|@REQLIB@`
stages it, the test is skipped without it); with
`cacheflush=NAME` whether the task that starts a process NAME flushed
the caches after its last AllocMem, paths still in
use); it also gives a program input while it runs: gadget clicks as
synthetic IntuiMessages (`click=TITLE/TEXT`, a `_` in TITLE or TEXT also
matches a blank, TITLE `*` any window that has the gadget, also one
without a title, which is then reported as `click window: "TITLE"`;
optionally while the DOS list is locked for writing; with `undobuf` it
reports whether a click changed the UndoBuffer pointers of the window's
string gadgets) and
several RAWKEY events in one call of
the input handlers at controlled times (`keys=`, an input handler with
a clock of its own). The `|` of `@INJECT@|...` only separates the helper
from its options; several options are separated by blanks
(`runwrap=@INJECT@|guardseg click=FQ/CANCEL`); the text after the `|`
reaches the command line as it is. A `|` between two options joins them
into one word, without a message: `openlibfail=graphics.library|track`
fails an OpenLibrary of the name `graphics.library|track`, and `track` is
not set. `wbrun` starts the compiler
(`wrap=@WBRUN@|...`) or a program (`runwrap=@WBRUN@|...`) like the
Workbench and reports the current directory at the end, how often the
process opened minus closed dos.library (`dosopen`: exec's OpenLibrary,
OldOpenLibrary and CloseLibrary are patched while it runs; the patches are
trampolines of `patchblock.h` in a block that is never freed, and the
removal waits for the tasks inside; self test `wbrun_race`: the option
`racehold` holds a helper process inside a patch while the patches are
removed, 0 calls inside after the removal; `wbrun_race_broken`:
`racehold=nowait` removes without waiting, the check must fail with 1 call
inside) and files left in use; with
`timeout=SEC` it waits at most SEC seconds for the reply and says
whether a tool that did not answer ended or still runs (self test
`wbrun_timeout` with `wbtool`, a tool that answers late or never;
`wbrunt` is `wbrun` with a default of 5 s, `runwrap=@WBRUNT@` starts the
program of a test like that: `regression/wbstartuplib-without-wbstartup`
and `-end`, a program without WBStartup. The Kickstart 3.1 ROM stops such
a program with the requester "Software Failure" (error 87000004, gadgets
"Suspend" and "Reboot"): at its first DOS call that waits for a packet
(the first test, `Lock_`: wbrun then waits for the end, the session ends by
the hang check), or, if it makes none, at its exit, when DOS frees a lock
(the second test: wbrun says "no reply, the process ended", the exit hook
has run, and the requester comes before that line). The launcher `reqtrap`
records the requester and answers "Suspend"; both tests expect that line.
Every test is its own emulator session, the suspended process and the
requester are gone with it);
with `freeseg` it overwrites and frees the seglist right after the reply, as
a memory checker would (a Blitz program
needs the command WBStartup, as from the real Workbench; what it
prints is lost), `userlib.c` is a library for
`blitz2:userlibs` (function `BugOrigUserLib`, token in
`userlib.tokens`). tedstub also runs menu items
(`menu=ID|ITEM|ANSWER`) and logs the busy pointer after them;
`run=ID|SOURCE|CONSOLE|PANIC` runs a source with COMPILE/RUN as from
the editor (the program in blitz2's own process, the stop requester
open; its console is the file CONSOLE, the output goes into the log),
optionally with !PANIC! hit PANIC ticks after the program started, and
logs the requesters left open;
`strbuf=N` sets the string buffer of the default options (strings
longer than 10240 bytes), `macrobuf=N` their macro buffer, `objbuf=N`
their object buffer, `warnings=off|on|errors` their warnings setting
(word +24 of the `.xtra`; the compiler's default is on),
`max=LIB|N` the object maximum of the library number LIB (e.g. 160 =
bitmaplib: more BitMap objects);
`resident=ID|SOURCE|RESFILE` (up to 8) compiles SOURCE before the first
job with the start options, saves it as the resident RESFILE (Create
Resident) and enters RESFILE in the resident list of the default
options, so the test program is compiled with the residents
(`resident=ID|SOURCE|RESFILE|defaults`: SOURCE is compiled with the
default options, without the resident list, so `macrobuf=` and the other
settings apply to it too);
`bitmap=BMAPMUL|GUARD` gives the compiler a bitmap of BMAPMUL bytes per
row with GUARD bytes before and after it and logs after every job the
progress calls, the stripe and the guards (`PROGRESS`, `STRIPE`,
`GUARD` lines, `shared/edstub/edstub.h`); `progress=on|off` (in the
order of the jobs, e.g. before a further `job=`) announces TED-C's
progress call or not (#980, `compiler-racing-stripe-wide`). All as
`; @pass jobline=...`.

### Adding a test

*Adding a test* (no shared list, a glob finds the files): two files in
an area directory (`compiler/`, `libs/` ...), named after the bug ID of
the issue: `ID.bas` (the program as text; in its header comment
`; @pass KEY=VALUE` for extra lines of the pass file, see
`tests/corpus/runner/corpus.h`: `file=`, `dir=`, `remove=`, `wrap=`,
`runwrap=`, `runargs=` (arguments of the program, a CLI start), `launcherargs=` (options
of the launcher: `-harden`, below),
`uae=KEY=VALUE` (a change of the emulator configuration), `jobline=`,
`floppy=N|SRC` (a disk image in drive DFN, cold boot, below),
`timeout=`, `runtime_errors=on`, `serial=on` (the remote end of the
serial tests on the emulated serial port, `tests/corpus/runner/serial.h`:
canned replies, among them `N` with a byte count and `S`, an XOFF that
holds the program's writes), `rtg=on` (the RTG variant of the emulator,
`amiga_emulation_rtg`: the uaegfx board and the Workbench hardfile with
its monitor, for a screen taller than the native modes), with `@USERLIB@`,
`@INJECT@`, `@WBRUN@`, `@CD_BLITZ2@`, `@TESTDATA@`, `@ANCIENT@` (the files of
ancient that `cmake/fetch-originals.cmake` fetches: real S404 data); `; @configs port` restricts the
configurations judged; `; @rwe NAME...` for the RWE userlibs, below) and `ID.expect` (the correct behaviour; a line
beginning with `[` is a section header, so the program's output must
not begin with `[`). Test
data (sounds, pictures, modules) is generated, not checked in: a
description `NAME.spec` in an area directory (`tools/mktestdata`:
bytes, words, longs, texts, labels, IFF chunks with their lengths filled
in; `tools/lib/mktestdata/mktestdata.h`) becomes
`build/<preset>/tests/bug-orig/testdata/NAME`, passed to the program
with `; @pass file=DEST|@TESTDATA@/NAME` (the program reads
`CORPUS:DEST`); NAME begins with the ID of the test. The exception is a
CD image for `uaescsi.device` (`; @pass uae=cdimage0=@TESTDATA@/NAME.cue,image`):
one image per library, shared by all its tests and named after the
library (`rwe/ricompactdisclib.cue.spec` with its tracks
`ricompactdisclib-t1.bin.spec`, `-t2.bin.spec`), so that the emulator
keeps one snapshot configuration for them. A further Blitz
source (e.g. the source of a resident) is a text `NAME.src`, tokenised
like `ID.bas` to `testdata/NAME`. The expectation is the correct
behaviour (from the documentation of the command or the issue), not
what the original does; the observation of the configurations
(`.../ID/run-XXXXXX/TAG/observation`, kept when the test fails) gives its format. Then record the original
(needs Amiberry and the CD): `cmake --preset amiga
-DBUG_ORIG_RECORD_FILTER=ID` and `cmake --build --preset amiga --target
bug-orig-record` write `ID.orig` next to `ID.expect` from two agreeing
runs; read it (it is what the original did; a line that varies gets a
wildcard with its reason, [Recorded originals](#recorded-originals)) and
commit it with the test. Run
`ctest --preset bug-orig -R bug_orig_ID`: an open bug comes out
REPRODUCED (not BROKEN: `ID.orig` shows the bug); check the
expectation if possible with a temporary fix in the port (then FIXED);
name the test in the issue. The commit that fixes the bug adds
`; @fixed #N` to the header ([Fixed tests](#fixed-tests)); a header
line moves the program down one line, so a compile error line in the
expectation grows by one.

### Recorded originals

**Recorded originals** (`ID.orig`, next to `ID.bas` / `ID.drv`): the
observation of the original, committed, so that the tests need no session
of it; in the terms of the shared test model (`docs/testing.md`) the
`[orig]` value of the test, `ID.expect` the expected value (the
`[fixed #N]` value once the bug is fixed). Every bug-orig test has one,
open or fixed; port regression tests have none. Format: the sections of
the observation as above (`[session]`, `[compile]`, `[run]`, `[blitz2]`,
`[log]`), raw, with a first line `# NOTE` that says which original it is
(blitz2 1.95 built from `blitz2.s`, or the CD's 2.10 for `; @rwe`
tests); the TED tests: `[state]` and `[problems]`
([bug-orig tests of TED](#bug-orig-tests-of-ted)). The normal run of an
open test judges `port` and `clib` against `ID.expect` (FIXED) and
`ID.orig` (REPRODUCED, else DIFFERS); the run of every test, open or
fixed, first checks that `ID.orig` does not meet `ID.expect` (BROKEN,
before any session). Only the sections of the expectation count, as with
a live reference. A test without `ID.orig` fails with exit status 2 and
the hint to record it.

**Varying lines.** A line that varies from run to run for a reason
outside the tested behaviour (an address, a time, the way a crash ends)
is a wildcard line of the expectation's kind (`*` at the end of a line,
`...` for any number of lines) directly behind a comment line
`# varies: REASON`; the wildcard covers only that line, and the comment is
no part of the section. A wildcard line without its reason, or a reason
without a wildcard line behind it, is an error of the runner (exit status
2, file and line; `bug_orig_wildcard_noreason`,
`bug_orig_ted_wildcard_noreason`). The lines are matched against the
port's observation as the lines of an expectation are (`bugorig.h`, the
matcher of `tools/lib/bugorigcmp`). Where a whole section varies with the
memory layout, the test pins the layout (`; @pass mount=BB2|@CD@`, as
`blitzcoplib-maxslice-zero` does) where that helps, and otherwise gets the
wildcard with its reason.

**The compare test** `bug_orig_record_ID` (label `originals`, preset
`originals` only; it needs the CD and the reference builds) runs the
original alone (pass key `record=check`, `ID-record.pass`) and compares
its observation with `ID.orig`, wildcard lines included: it fails naming
the section and the first differing line (`DIFFERS: [run] line 2:
recorded "...", observed "..."`) and writes no file of the source tree. It
detects a changed original (another CD) and a recording that is not
deterministic. `bug_orig_record_changed` and `bug_orig_ted_record_changed`
check it with a recorded original whose line was changed.

**A hanging original ends early.** A test whose `ID.orig` has
`[session]` `timeout` (the original hangs, or crashes into an alert that
reboots the machine, which then runs the job again and crashes again) would
wait the whole session limit (`BUG_ORIG_TIMEOUT`, 30 to 120 s) in the
compare test and in every recording run. The configure step therefore adds
the pass key `hang_after=N` (`BUG_ORIG_HANG_AFTER`, default 6 s, 0 = off) to
the passes of the compare test and the recording runs of such a test
(`bug_orig_record_ID`, `bug-orig-record`): the session ends as a timeout
when the job has changed no file of its volumes and of `TEST:` for N
seconds (`tools/lib/amiga_emulation/README.md`, "Session states"). What it
leaves is what a run to the limit leaves, because the hang is silent: the
alert of a crash stays about 10 s before the reboot, and the observation is
taken inside it. The sessions of the port and clib in the normal run have
a check of their own, below. A
test whose hang the recording does not know yet (no `ID.orig`) waits the
whole limit once. A test that computes silently for longer than N seconds
before it hangs would be cut early (the compare test then fails with the
differing section): such a test sets the key itself, `; @pass hang_after=N`
for the normal run, and a larger `BUG_ORIG_HANG_AFTER` for the recording.
The key is the pass file's (`corpus.h`, `hang_after=`).

**A hanging program ends early in the normal run.** The port's and clib's
sessions of the normal run (open, fixed and regression tests) get
`hang_after=N` with `hang_from=run` (`BUG_ORIG_RUN_HANG_AFTER`, default 8 s,
0 = off; not for a `run=off` test). The silence counts from the first program
run, from the file the gate or the first program creates
(`amiga_emulation_params.hang_arm`), never from the start of the job: the
compile is silent for long on a loaded host (23 s seen), and a session is
never ended in it. A program that hangs, or crashes into the alert that
reboots the machine, ends as a timeout after N seconds, the observation of a
run to the limit. A compile that hangs keeps the whole limit: no sign separates
it from a slow compile. A program that computes silently for longer than N
seconds sets `; @pass hang_after=N`, `; @pass hang_from=run` stays. The
verdict of every test is the same as with the limit alone (checked against all
tests of the preset `bug-orig`).


**Recording** is the build target `bug-orig-record` (not ALL, not a
test; `tests/bug-orig/record.cmake`):

```
cmake --preset amiga -DBUG_ORIG_RECORD_FILTER=ID1,ID2   # IDs or globs; empty: all
cmake --build --preset amiga --target bug-orig-record
git diff tests/bug-orig                                 # review
```

`BUG_ORIG_RECORD_FILTER` is a cache variable: an environment variable of
that name has no effect and is refused by the target; the target's comment
shows the active filter.

It runs the original twice per test (`corpusrun pass` / `tedab run` with
the pass key `record=write`, the observation into the build directory);
the second run has another memory layout: its Stack is 16384 bytes
larger (pass key `layout=stack` of corpusrun, `stack=` of tedab), so that
what the compiler, the program or TED allocate lies elsewhere, and nothing
else of the run changes. `results bug-orig-accept`
writes `ID.orig` only if both runs agree; otherwise it reports `test not
deterministic: ID` with the differing sections and lines and leaves the
file as it was, and the target fails at the end. The wildcard lines of
the committed file are kept where both runs match them and reported
where one does not; a line of the original that would read as a wildcard
is refused. The tests the filter leaves out are not touched, and a test
whose runs are skipped (no Amiberry, the reference build of an RWE
library missing: `ctest --preset originals -R rwe_refbuild`) is reported
as skipped. The proof of determinism is limited to the memory layout
(date, time and emulator timing are not varied). The orchestration is tested with fake runs (`bug_orig_record_target`), the
acceptance with files (`bug_orig_accept`). When the original is wrong in
a way that is not the bug, the test is not valid.

A **fixed test keeps its `ID.orig`**: configuring stops with an error
naming a test with the header `; @fixed #N` (`.bas`) or the directive
`; @fixed #N` (`.drv`, TED) that has none
(`tests/bug-orig/check-fixed-orig.cmake`; ctest
`bug_orig_fixed_orig_broken` checks the check on a fake tree,
`tests/bug-orig/selftest/fixed-orig/`); its compare test
`bug_orig_record_ID` runs in `originals` as an open test's does, and its
validity check (BROKEN) in every run (`bug_orig_fixed_equal_broken`,
`bug_orig_ted_fixed_equal_broken`).

### Fixed tests

**Fixed tests** (`; @fixed #NNN` in the header of `ID.bas`, NNN the
issue of the bug; pass key `fixed=NNN`): the bug is fixed in the port.
The commit that fixes the bug adds the line and keeps `ID.orig` (the
`[orig]` side of the pair, compared with the original by
`bug_orig_record_ID` in `originals`; configuring stops if it is
missing). The test then has no reference session and uses nothing of
the CD (no `BB2:` volume, no
`blitz2:defaultdbug`; a test that needs them names them with
`; @pass mount=BB2|@CD@` or `; @pass defaultdbug=@CD_BLITZ2@/DEFAULTDBUG`
and is skipped without the CD); only `clib` (blitz2-c with the C
libraries, as the port ships) is judged, against the expectation alone:
FIXED, or REGRESSED (the configuration does not show the expected
behaviour: the fix is undone); before that its `ID.orig` must not meet
the expectation (BROKEN, as for an open test). The test passes only when it is FIXED
(exit status of `corpusrun pass`: 0 FIXED, 1 REGRESSED). A test that
checks a case the original already gets right (the boundary of a fix:
the longest text that still fits, a word offset that still fits) is no
fixed test, since its `ID.orig` would meet the expectation (BROKEN), but
a [port regression test](#port-regression-tests) whose comment names the
fix (e.g. `regression/compiler-object-index-below`, #939).
ctest `bug_orig_ID`
as before, label `emulator` only: it runs in the presets `emulator`
and `all`, not in `bug-orig`, and it is declared directly, not in the
order of the bug-orig tests (it passes, ctest learns its cost).
`ctest --preset all -N` lists it, `ctest --preset bug-orig -N` does not.

**An extension of the port** (`; @fixed #NNN/port` in the header of
`ID.bas`, NNN the issue of the extension; user decision 2026-11-22):
a test of behaviour the port adds where the original refuses or
differs, not of an original bug (the first: #1166, ANIM interleaves
that the original refuses). For
now it is the same as `; @fixed #NNN` (pass key `fixed=NNN`, `ID.orig`
recorded, FIXED, the `[fixed #NNN]` case of fix-scope); `/port` only
marks it, right behind the number (`bugorigcmp` refuses anything else
there, `test_bugorigcmp`). The rework of the tests gives extensions a
form of their own and replaces the marker.

### A bug of an RWE userlib

*A bug of an RWE userlib* (`libs/userlibs/rwe/NAME.c`): the same, in
`tests/bug-orig/rwe/ID.bas` with the header line `; @rwe NAME...` (the
libraries the program uses, e.g. also one that NAME calls). The
"original" is the library's reference (`RWE_LIBS`: the CD's or
AmiBlitz3's binary, or the reference build `rwe_refbuild` - then the
test requires that fixture, ctest runs it first, about 20 s): ref is
blitz2 2.10 of the CD (1.95 never loads a library of
`blitz2:userlibs/DIR`, #2) with the CD's libraries and the reference
in `blitz2:userlibs/rwe/`; port (blitz2-c, the CD's libraries) and clib
(blitz2-c with the acidlibs/deflibs of the library's variant,
`variants/rwe-1999`) get the C build there (the pass file's
`file=DEST|SRC|TAG`: a file for one configuration). The source is
tokenised with a token list of blitz2-c, that variant and the named
libraries (`bbsrc tokens`, `tests/bug-orig/tokens/ID.txt` in the build
directory). The libraries of one test must have the same variant
(rievallib has its own, `rwe-1999-eval`, whose polylib number the CD's
acidlibs give to polylib: its tests need a ref with other acidlibs,
not done yet).

### A bug on a 68000/68010 or Kickstart 1.x or 2.x

*A bug that only shows on a 68000/68010 or Kickstart 1.x or 2.x*: the
header line `; @machine NAME` (pass key `machine=`) runs every
configuration on that machine, `a500` (68000, Kickstart 1.3, Workbench
1.3.5), `a500-68010` (`AMIGA-EMULATION-SETUP.md`, "A500 machine"),
`a500-204` (68000, Kickstart 2.04, Workbench 2.1; "A500+ machine") or
`a600` (68000, Kickstart 3.1); compiler and program run there, as on
the A1200 (e.g. `libs/multi-68010-vbr`, #42;
`rwe/risortlib-finit-closelibrary-unchecked`, #824). Several names,
`; @machine a500 a500-204 a1200`, run the same test on each machine: one
ctest per machine, `NAME_MACHINE` (`-` as `_`, e.g.
`bug_orig_ID_a500_204`), each with its own pass file and work directory
`ID-MACHINE`, all judged against the one `ID.expect`; the recorded
original (`ID.orig`, its compare test `bug_orig_record_ID_MACHINE` and
the recording runs) is that of the first machine, so a test whose
original behaves differently per machine needs a file per machine
instead (`tests/bug-orig/machines.cmake`; its test `bug_orig_machines`
with broken input: a bad name, a name twice, no name; and the self test
`regression_selftest_machines_a1200`/`_a500_204`). On Kickstart 2.x
(`a500-204`) everything of the A1200 runs: the session script in 2.0
syntax, the gate of corpus_req, `@INJECT@`, `@WBRUN@`, the snapshot
mode, the reference blitz2 1.95 and the CD's 2.10 (checked 2026-11-20);
not what needs Workbench 3.0 or newer. On
Kickstart 1.x the session script is in AmigaDOS 1.3 syntax (the
redirection right behind the command name; no `$RC` and no `>>`:
`rcput` - `tests/corpus/amiga/rcput.c` - appends the `rc=N` of the
command before), tedstub (with the edstub frame) uses only Kickstart
1.2 functions, and `reqtrap` patches only AutoRequest there and calls
the program itself (LoadSeg, as the 1.3 CLI calls a command: only
machine code programs, not the BCPL commands of `C:`). reqtrap and
rcput are linked with libnix13 (libnix20, the toolchain's default,
does not start on Kickstart 1.x). The reference blitz2 1.95 is
assembled with `-kick1hunks` (HUNK_RELOC32: Kickstart 1.x's LoadSeg
knows no HUNK_DREL32). Not there on Kickstart 1.x: `gate=`
(reqclosetest, reqclose), `@INJECT@`, `@WBRUN@` (V36 functions).
blitz2-c starts there since #936 (linked with libnix13). A crash on Kickstart 1.3 ends in the requester
"Software error - task held", which reqtrap answers with Cancel: a
Guru that waits for the mouse, the session runs into its timeout.

**The handlers of reqtrap** (`tests/corpus/amiga/ctxwrite.h`). The
hardening below is an option: `reqtrap -harden COMMAND ...` (the first
argument; `reqclose` the same) and `inject ctxwrite alerts ...`
(with `alerts` or `alertlog`). Without it the tools behave as they did
before the hardening: no writer process (nothing is allocated before the
program runs, so the memory layout of a run is that of the program alone,
which the recorded originals depend on), every handler writes its line in
the context of the caller and every requester gets the rightmost answer
(0; `AutoRequest` FALSE), also "Software Failure" ("Reboot", which halts
the machine); the guard against a re-entry works only with the writer (it
needs the writer's message, so it is part of the option). The corpus
runner passes the launcher option with the pass key `launcherargs=-harden`
(`CORPUS:run -harden ...`; the two `regression/wbstartuplib-without-wbstartup*`
tests set it with `; @pass launcherargs=-harden`), `reqtrap-test.cmake` with
`-DLAUNCHER_ARGS=-harden`. Use it for a test that starts a program the
Workbench way without WBStartup (wbrun) or that needs the requester
"Software Failure" answered; a test without it sees the old output and
layout.

With the option: a handler
runs in the context of the task that raises the requester, and that can
be inside DOS: a program that leaves its WBStartup message in its process
port gets the requester "Software Failure" from the packet wait of its
next DOS call (or of its exit), and every DOS call of the handler (a
`Write`) takes that message for a late reply again and raises the
requester again, every level costing about 360 bytes of stack, until the
CPU halts. The handlers therefore make no DOS call: they collect the
line in memory and send it to the writer, a process of reqtrap
(priority 1), with a message and wait for the reply with exec calls only,
so that the order of the lines against the output of the program stays
(the writer is not started on Kickstart 1.x, where the command runs in
reqtrap's own process and the handler writes itself). A task that is
already inside a handler, and the writer itself, record nothing and are
counted (`reqtrap: N requesters not recorded (a task already inside
one)` at the end). The requester "Software Failure" is answered with
"Suspend" (`-> answered 1`): "Reboot", the rightmost gadget the other
requesters get, halts the machine, "Suspend" stops only the process.
`reqclose` is the same program (checked: its closer process makes its
DOS calls in its own process, never in the context of the program).
`inject` had handlers of the same kind, the alert patches (`alerts`:
DisplayAlert answered at once, `alertlog`: the dead-end alert of exec): they
write through the same writer, started only with the option `ctxwrite` (a
line `inject: N alerts not recorded ...` at the end if any was skipped;
without it they write in the caller, as before);
its other patches only count and write at the end. The writer takes the
signal SIGB_SINGLE when the task has no free signal left (`signals`).
Tests (label `emulator`, `reqtraptest`: a helper process with an unreplied
message in its port calls `Lock`): `reqtrap_stray` and `reqclose_stray` (one
`REQUESTER:` line, answered 1, then the helper is stopped inside `Lock`; all
with `-harden`),
`reqtrap_stray_broken` (`reqtrapdirect`: reqtrap built with `-DCW_DIRECT`,
the old handlers, the caller writes, at most 2 levels deep: the line comes
twice and the count of the one not recorded, what the check for one line
must not accept); `inject_alert_stray` (`reqtraptest alert` as the COMMAND
of inject, which has the unreplied message in its own port while it calls
DisplayAlert: the line of the alert, then the program's, no requester) and
`inject_alert_stray_broken` (`injectdirect`, the old patch: the alert line,
then the requester that reqtrap answers with "Suspend", the process stays
suspended, the run ends by its time limit of 20 s, which that test expects).

**The patches of reqtrap and teddrive** (`tests/corpus/amiga/patchblock.h`,
one source for both). Both patch system functions for the time of a run
(AutoRequest and EasyRequestArgs; CloseWindow and dos Open), and a task
that is inside a patch when it is removed must not run into the freed
memory of the tool. The stub of every patch and the data it uses (the
ring of closed windows teddrive reads) are therefore copied into an
`AllocMem` block that is never freed (about 100 bytes per run) in chip
memory: the programs under the tool take public (fast or slow) memory, and
a block there would shift every address of the program, with the cycle
counts of the perf tests (cycle-exact) and the garbage and the allocation
order that recorded original bugs depend on; every call
counts itself in before it looks at the stub's `active` flag, and the
removal clears `active`, restores the vector and waits (up to 10 s) for the
calls still inside; later calls jump to the original function. Tests:
`patchblock` (label `emulator`, `patchblocktest`: a helper process of a
higher priority is inside a patched `Delay` while the main process removes
the patch; the removal must wait for it, and the helper returns into the
stub) and `patchblock_broken` (the same with a removal that does not wait:
the check "removed" must fail, exit status 5).

`inject` (`tests/bug-orig/amiga/inject.c`) uses the same mechanism for its
about 20 patches, among them the system-wide AllocMem, FreeMem and
OpenLibrary: its stubs decide in C and stay in the program, so each patch
gets a trampoline of `patchblock.h` (`PB_THRU`) in a block that is never
freed. While the patch is active the trampoline counts the call and calls
inject's stub as a function; once it is removed it jumps to the old
function and touches nothing of the program. The removal restores all
vectors (newest first) and then waits up to 10 s for the calls inside
(`inject: N calls still inside a patch after 10 s` otherwise). Self test:
the option `racehold` holds a helper process inside a patch (`Read`) when
the patches are removed; `inject_race` (the removal waits: 0 calls inside
after it, the helper returns) and `inject_race_broken` (`racehold=nowait`,
no waiting: the check must fail with 1 call inside).

A floppy disk: `; @pass floppy=N|SRC` puts the image SRC into drive DFN
(0 to 3, once per drive; the other drives stay empty, the templates have
one drive, DF0, and the key switches DF1 to DF3 on). Tests of
ritrackdisklib use DF1 and leave DF0 empty. SRC is a generated test image,
e.g. `@TESTDATA@/ritrackdisklib-blank.adf` from
`rwe/ritrackdisklib-blank.adf.spec` (`tools/mktestdata`; a DD disk is 901120
bytes, an all-zero ADF reads as 1760 valid sectors of zeros, AmigaDOS sees no
DOS disk; marks in sectors 0, 11 and 1759). Every configuration gets its own
writable copy `WORK/TAG/emu/dfN.adf` (the image itself stays as it is), so
a write of the program is seen by that configuration only. A session with a
floppy boots cold, without a snapshot: the snapshot key holds the identity
of every medium (path, inode, time), and the copy is new in every run, so
every session would save a state that is never used again. Consequences: a
session takes the boot (about 3 s on the A1200 and the a500-204) and there
is no hang check (it works in snapshot mode only), a
test sets `timeout=`. The key comes before the `uae=` changes: `uae=floppy1wp=true`
write protects the disk. A disk in DF0 without a boot block is skipped by the
boot (DH0 boots). Example and check of the key:
`regression/ritrackdisklib-floppy-key` (a500, a500-204 and a1200), and the
disk tests `rwe/ritrackdisklib-disk-*`; the key's parser and the copy have host
tests with broken input (`corpus_runner`).


**Port regression tests** (`tests/bug-orig/regression/ID.bas`, same
files and header lines): a bug of the port, not of the original, that
needs what only this runner gives (an RWE userlib, `uae=` such as a CD
image, `@INJECT@` ...). They are expect tests like those of
[Extensions of blitz2](#extensions-of-blitz2) (pass key
`regression=on`): no reference session and nothing of the CD (as a
[fixed test](#fixed-tests): no `BB2:` volume, no `blitz2:defaultdbug`,
with `; @rwe` no reference library; a test that needs the CD names it in
its `; @pass` lines and is skipped without it); only `clib` is judged
unless `; @configs` names others, against the expectation alone: FIXED
or REGRESSED, and the test passes when every configuration is FIXED.
The expectation is the correct behaviour (for a port bug usually what
the original does, checked once when the test is written). ctest
`regression_ID`, label `emulator` only (not `bug-orig`: it must pass in
the presets `emulator` and `all`); `regression_selftest_wrong` (an
expectation the port does not meet) must come out REGRESSED. ID is
named after the issue of the port bug. A fix that changes the expected
behaviour changes `ID.expect` in its commit and names the issue, as in
the extensions' suite.

### bug-orig tests of TED

**bug-orig tests of TED** (`tests/bug-orig/ted/ID.drv`): a TED A/B
script (`tests/ted-ab`, directives in `runner/tedab.h`) whose
expectations are the correct behaviour. `tedab` in its bug-orig mode
(pass key `bugorig=1`) plays it with TED-C under blitz2 2.10 (C5) in an
emulator session of its own and gives the verdict like corpusrun (FIXED,
REPRODUCED, BROKEN, DIFFERS; same exit status) against the recorded
observation of the original TED 1.1 under blitz2 2.10 (R5, the
reference): `ID.orig` next to `ID.drv`, pass key `orig=`
([Recorded originals](#recorded-originals)). The
observation of a run is its result (OK, `FAIL step N ...`, `TIMEOUT`
for a crash or hang, `HALT` for a crash that halted the emulated CPU) and the expectations it misses
(file: `[state]` and `[problems]`, the problem lines as the report has
them); REPRODUCED needs the same observation (the A/B comparison of the
whole run directories, which a live R5 run allowed, is gone: the
expectations are what counts). `ID.orig` says OK without a problem: BROKEN
before any session. The compare test `bug_orig_record_ID` (label
`originals`, pass key `record=check`) plays R5 only and compares it with
`ID.orig`; the target `bug-orig-record` records it (pass key
`record=write`, two runs, the second with a larger Stack). Made for these tests: `; @file NAME "DATA"` (a file in `work/`,
`"..."` with `\n`, `\t`, `\xHH`, repeated lines append), `; @source F
NAME` (that file is loaded), `; @command ARGS` (instead of
`blitz2:blitz2 FILE`: e.g. `blitz2:ted @FILE@` for TED alone, or with
`TEDAB:tools/inject`/`TEDAB:tools/wbrun` in front, or `Execute` of a
script made with `@file`), `; @expect data NAME "DATA"`, `; @expect
[no]stdout "TEXT"`, `[no]log` (teddrive's log), `[no]screen DUMP
"TEXT"` (the decoded dump), `[no]file NAME "TEXT"`. ctest `bug_orig_ID`,
report `build/<preset>/tests/bug-orig/ID.txt`, the runs in
`.../ID/run-XXXXXX/sNNN/vol/runs/ID/{R5,C5}` (kept when the test fails); `bug_orig_ted_lint` (host) checks
the scripts, `bug_orig_ted_selftest_broken` (label `emulator`) must come
out BROKEN on a recorded file. Adding a test: `ted/ID.drv`, found by a
glob, and `ted/ID.orig` from the target `bug-orig-record`
(`-DBUG_ORIG_RECORD_FILTER=ID`).

**The gates, once per ctest run.** Every TED session starts with gate 0
(`tedprobe`: the screen the emulator gives) and gate D (`teddrive` against
`teddrivetest`, twice, one script failing on purpose): about 8 s of a
10 s test, and the same for every test, because they check the emulator's
environment and the tools, not the TED under test. The fixture `ted_gates`
runs them once: its setup test (`ted_gates`, label `emulator`, pass key
`gates_only=1`) is one session without a test; when both gates are OK it
publishes its run directory as `ted_gates` in the run directory
(`rundir.h`) with the file `gates-ok`, and `ted_gates_cleanup` removes it
after the last reader. Every TED bug-orig test (open, fixed, `_originals`,
`bug_orig_record_ID`, the self tests) requires the fixture, stays one ctest
entry and has `gates_from=ted_gates` in its pass file: with `gates-ok`
its sessions run no gate D and do not evaluate gate 0 (the session log
says `GATES checked before`, the report `gates: checked before ...`);
`tedprobe` still runs in every session, unchecked: the tests that fail
allocations depend on the memory layout it leaves (#340: TED's `a0toname`
splits a file name by the low byte of an address), and without it seven of
them fail in every session. Started by hand without the fixture the
sessions run the gates as before. A failing gate fails `ted_gates`
(exit status 1, nothing published, an older mark removed first) and ctest
does not run the tests that require it ("Not Run"), so the run fails
visibly. `ted_gates_broken` (label `emulator`) runs the same pass with a
gate D that fails on purpose (`selftest/gates-broken/`) and needs exit status
1; the session script without the gates is checked on the host
(`test_tedab`, "session script without the gates"). `ted_ab` keeps the gates
in its sessions.

**A fixed TED bug-orig test** (`; @fixed #N` in the header of
`ted/ID.drv`, N the issue, **no pair**; pass key `fixed=1`): the commit
that fixes the bug in TED-C adds the line, keeps `ID.orig` (configuring
stops if it is missing; `orig=` is only checked against the script,
BROKEN before any session) and keeps the script's expectations, which
are the correct behaviour. The test then runs TED-C
only, no session of the original, and is judged by the script's
expectations alone: FIXED (exit status 0: the run is OK and no
expectation is missed) or REGRESSED (1: the report says `bug-orig Cc:
REGRESSED: bug #N is fixed, but the expected behaviour does not show` and
lists what is missing). It is two tests: ctest `bug_orig_ID` (label
`emulator` only: it runs in `emulator` and `all`, not in `bug-orig`)
runs Cc, TED-C under blitz2-c with the libraries of the build (pass key
`libs=`), and needs no CD; `bug_orig_ID_originals` (label `originals`)
runs C5, TED-C under the CD's blitz2 2.10 with the CD's libraries.
Its compare test `bug_orig_record_ID` (label `originals`) runs R5 as
an open test's does. The directive with a pair
(`@fixed #N R5-C5`, the `tests/ted-ab` form) is refused in
`tests/bug-orig/ted`, and the form without a pair outside it (`tedab`
parses the scripts of a `bugorig=1` pass in the bug-orig form,
`bug_orig_ted_lint` too); a `fixed=1` pass over a script without
`@fixed`, and a pass without `fixed=1` over a fixed one, end with exit
status 2 before any session. Self tests: `bug_orig_ted_fixed_selftest_fixed`
(an expectation that holds of TED-C: FIXED) and
`bug_orig_ted_fixed_selftest_regressed` (one that does not: REGRESSED,
naming the bug; label `emulator`), the refusals
`bug_orig_ted_fixed_open_script`, `bug_orig_ted_open_fixed_script`,
`bug_orig_ted_fixed_with_record`, `bug_orig_ted_fixed_no_bugorig`,
`bug_orig_ted_no_orig_no_fixed`, `bug_orig_ted_record_no_to`,
`bug_orig_ted_record_bad` and `bug_orig_ted_fixed_wrong_lint`
(scripts of `selftest/ted-fixed-wrong/`: a pair, twice, no `#`; host),
`bug_orig_ted_fixed_equal_broken` (a fixed test whose `ID.orig` says OK:
BROKEN; host); the
verdicts in the host test `tedab_bugorig_verdict` ("fixed bug-orig test"
case of `tests/ted-ab/host/test_tedab.cpp`).

**A TED bug-orig test fixed in the port's pair only** (`; @fixed #N Cc`:
the word `Cc` after the number): for a bug whose fix is in blitz2-c, so
that TED-C under the CD's blitz2 2.10 (C5) cannot have it and keeps the
bug. The Cc test is the one above (`bug_orig_ID`, label `emulator`, pass
key `fixed=1` with the combination Cc alone: FIXED or REGRESSED by the
script's expectations). `bug_orig_ID_originals` (label `originals`) is no
fixed run: it runs C5 like an open test, judged against `ID.orig`, and
passes when that is REPRODUCED (exit status 1, checked by `want.cmake`
with `WANT=1`); a C5 that shows the expected behaviour (FIXED, 0) or
another observation (DIFFERS, 4) fails it, which says the original has
changed or `ID.orig` is stale. Only `Cc` may follow the number (a pair, a
word twice, `C5` are refused by the lint and the runner); a `fixed=1`
pass that names C5 over such a script ends with exit status 2 before any
session. Self tests: `bug_orig_ted_fixed_port_selftest_fixed` and
`..._regressed` (Cc FIXED, REGRESSED; the `_originals` run of the second
REPRODUCED, that of the first FIXED, so that the real test's check fails:
`returned 0, expected 1`), `bug_orig_ted_fixed_port_c5` (before any
session), the lint scripts `e-port-twice`, `f-port-c5`, `g-port-ok` of
`selftest/ted-fixed-wrong/`, and the parse cases of the host test "fixed
bug-orig test: @fixed #N without a pair" (`test_tedab`).

### Order of the bug-orig tests

**Order of the bug-orig tests:** ctest with several jobs runs the
tests that failed in its last run first, in declaration order and
without regard to `COST`, and the bug-orig tests fail on purpose. So
all of them (those of `libs/tests` too) are recorded with
`bug_orig_add_test` and declared at the end of
`tests/bug-orig/CMakeLists.txt`, sorted by their `COST` (session limit
times sessions), the largest first (`cmake/bug-orig-order.cmake`);
ctest `bug_orig_order` (host) checks the order, `bug_orig_order_broken`
the check with a wrong one.

**Order of the long tests of the all suite:** the longest tests
(`ted_ab`, the ab3 equivalence passes, `libs_emulator`, the corpus
passes, ...) get a `COST` (rounded seconds, `cmake/test-cost.cmake`), so
that a cold run, without `CTestCostData.txt`, starts them first and does
not end with one of them alone.

## bug-port tests

Tests of known defects of the port itself (not of the
original), each with an issue. The test checks the *correct* behaviour
and **fails** until the defect is fixed; the fix removes the label from
the test. Label `bug-port` (with `emulator`): `host`, `emulator` and
`all` exclude it like `bug-orig`; the preset `bug-orig` runs both
(`ctest --preset bug-orig -L bug-port`: only these; with no open
defect the label has no test, and ctest reports "No tests were found"
with success). `tests/bug-port/` (`bug_port_lsclear(... BUG ...)` sets
the label): `bug_port_963_ted` and `bug_port_963_ab3cli` (#963, fixed:
no label any more, they run in `emulator` and `all`): `lsclear`
(`amiga/lsclear.c`, no libc) dirties all free memory with `$AA` on the
`a500` machine (Kickstart 1.3), loads the program with `LoadSeg` and
checks that every byte between a hunk's stored block and its header
size is 0 (Kickstart 1.x clears only `HUNK_BSS`; the baserel programs
without startup code have their `.bss` in the data hunk, which
`cmake/baserel-nostart.ld` stores whole). The checker has tests of its own (label
`emulator`, they pass): `lsclear_selftest_a500` / `_a1200` (generated
load files with an unstored tail, a stored zero tail and a `HUNK_BSS`:
on Kickstart 1.3 only the first is FAIL, with counted bytes; all PASS
on Kickstart 3.2), `lsclear_selftest_wrong_*` (wrong expectations must
fail), `lsclear_ted_a1200` (control: `ted` passes on Kickstart 3.2).

A defect of AB3 mode is a one-case `ab3_cli` session of its own (label
`bug-port`), its expectation checked by a one-case `ab3_cli_ref` session
(label `oracle`) against AmiBlitz3's compiler; the fix moves the case into
the lists of the `ab3_cli`/`ab3_cli_ref` blocks and removes both sessions (#1068, the
size suffix `.s`: case `asm_short_s`).


## Oracle data of the TED tests

The oracle data of the TED tests (`tests/data/ted-oracle.sha256`: SHA-256
of detok, tokit2 and load/save for every source of the CD;
`tests/data/ted-tokens.txt`: the token list) are generated by the original
TED itself: `programs/ted/tests/oracle` (`tedoracle`, not in the default
build) loads the reference build of `ted.s` as a library, under the
original blitz2 1.95 in Amiberry (needs the CD; the target builds both,
in the preset `originals` the fixture `originals_refbuild`):

```
cmake --build --preset amiga --target ted-oracle     # about 40 s
cmake -DORACLE_LIMIT=5 build/amiga                   # trial run, tests/data stays
```

A fix on the path of these outputs (detok, tokit2, load and save of TED)
changes the port's hash of the sources it touches, and the tests
`ted_core_*` compare with the port's hash. Such a row is a note row: the
port's hash, a tab and a note that names the issue and the original's
hash,

```
<port's hash>  save/PATH	#NNN: what the fix changes (TED 1.1: <original's hash>)
```

written by hand in the fix commit: with `TED_TEST_DUMP=DIR` in the
environment the failing test writes the port's output to
`DIR/KIND/PATH` (`/` of PATH as `_`, `programs/ted/tests/host/corpus.hpp`),
and its `sha256sum` is the port's hash. The
target `ted-oracle` writes the file through `results ted-oracle`
([`tools/results`](../tools/results/README.md)): every row is the
original's, the note rows stay as they are, and a note must name the
original's hash of this run (else it is stale and nothing is written).
Without note rows the target leaves the file byte for byte as it is.
`ted_oracle_reference` (label `originals`) checks the committed files that
way without writing them: the same run into the build directory, and
`ted-oracle.sha256` and `ted-tokens.txt` must be the committed ones byte
for byte.

## Reference data of TED's data check

`tests/data/ted-checkdata.txt` holds the bytes of the reference build of
`ted.s` that the check `checkdata` of `ted.sym` compares with (one line
per range of `programs/ted/checkdata.tsv`: start label, end label or
size, the bytes in hex, `..` for a relocated longword), so a default
build assembles no `ted.s`. After a change of `checkdata.tsv` (a new
range) the check fails with "not in the reference data"; the target
writes the file again from the reference build:

```
cmake --build --preset amiga --target ted-checkdata
```

`ted_checkdata_reference` (label `originals`) writes it from a fresh
reference build into the build directory and compares it with the
committed file byte for byte; `ted_checkdata_reference_check_wrong` (a
changed byte must fail) and `_check_right` check that comparison.

## Test statistics

Every test preset writes a JUnit file of its run into the build
directory: `build/amiga/junit-PRESET.xml` (`build/native/junit-native.xml`),
replaced by the next run of the preset. To keep several runs, name the
file on the command line (relative to the build directory; it overrides
the preset's):

```
ctest --preset bug-orig --output-junit junit-bug-orig-1.xml
ctest --preset bug-orig --output-junit junit-bug-orig-2.xml
```

The presets keep the whole output of passing tests as well (up to 1 MiB
per test, cut at the front if longer: `maxPassedTestOutputSize`,
`testOutputTruncation`), so it is in the file (`<system-out>`).

The runners of the emulator tests (`corpusrun pass`, `corpusrun cd`,
`tedab run`, also in the bug-orig mode) write one line per emulator
session to their output, after their report:

```
@stat v=1 test=audiolib-diskplay-endless config=clib machine=a1200 snapshot=restored session=45.16 wait=0.00 timed=45.15 limit=45 end=timeout verdict=REPRODUCED
```

test (the pass), config (corpus: clib/prev, in the originals runs ref/port; TED: R5/C5/... in the
bug-orig mode, else the session sNNN; corpusrun cd: the worker), machine, what
the snapshot mode did (cold, restored, first-save, fallback), the session
time without the wait for an emulator slot, the wait, the part counted
against the time limit, the limit, the end (done, timeout, halt,
exited, failed) and the verdict of the config (bug-orig FIXED/REPRODUCED/DIFFERS/
BROKEN, reference; corpus same/differs; TED A/B ok/fail). The format is
defined in `tools/lib/teststats/statline.h`. `testinfo stats`
([`tools/testinfo/README.md`](../tools/testinfo/README.md)) reads one or
more JUnit files (each one a run):

```
build/amiga/host/tools/testinfo/testinfo stats build/amiga/junit-*.xml
```

It prints per test the runs and their durations (min/median/max), per
test and config the sessions (max session time, max time counted against
the limit, max slot wait, timeouts, verdicts), the flaky ones (ctest
result, verdict or end differs between runs), suggested time limits (max
timed × factor, default 3, at least the floor, default 30 s; a config
that ended by timeout in every session of every run is listed as hanging,
without a number) and a suggested ctest `COST` per test (the median
duration; a test's duration includes its waits for emulator slots). Broken input (malformed XML, a testcase without name or time,
a bad `@stat` line) is reported on stderr (exit status 1), the rest
still counts.

## Coverage

Which code each test executes, as one lcov tracefile per test (openspec
change `test-coverage`). It is made from a coverage run, not checked in:

```
cmake --preset amiga-cov && cmake --build --preset amiga-cov
ctest --preset coverage                                  # about 26 minutes
cmake --build build/amiga-cov --target coverage-map      # seconds
```

The configure preset `amiga-cov` (directory `build/amiga-cov/`) builds
`blitz2` and `blitz2-ab3` with function coverage (`BLITZ2_COVERAGE`) and
the host side with gcov (`HOST_COVERAGE`); the normal configuration and
its results stay as they are. The test preset `coverage` runs every test,
`all` and `bug-orig` together, in that build: the tests of reproduced
original bugs fail on purpose there as in `ctest --preset bug-orig` (291),
all others pass.

* **host tests:** a build with `-DHOST_COVERAGE=ON` (gcov, `-O0`) of the
  program code they compile (`programs/*/src/`, `shared/`: the TED core,
  the `.xtra` model, the tokenizer, ...; gcc's
  `-fprofile-filter-files`); the tools, the test sources and the TED
  tools are not instrumented. Each host test `X` leaves its `.gcda` files
  below `build/<preset>/host/coverage/X/` and is recorded as test
  `host/X`.
* **compilers in the emulator:** the coverage configuration builds
  `blitz2` and `blitz2-ab3` instrumented; each emulator test leaves the
  coverage files of the compilers it ran as
  `coverage/<test>/<program>.cov` in the build directory (format:
  `shared/cov/covfmt.h`). Line ranges of their functions come from the
  objects if they are compiled with `-g` (which leaves the code
  unchanged); without them a change anywhere in a source file counts for
  all its functions.

Before a run clear `build/amiga-cov/host/coverage/` (gcov adds to data
that is there). A rebuild of the host side after the run makes gcov refuse
the data of the rebuilt objects (a stamp mismatch, and `coverage-map`
fails): run the host tests again (`ctest --preset coverage -R
'^(host|host_emulator)$'`). After the run

```
cmake --build build/amiga-cov --target coverage-map
```

runs `cmake/coverage-map.cmake`: gcov per host test, `nm -A` and
`objdump --dwarf=decodedline` of each measured program's objects, and
`testinfo map` ([`tools/testinfo/README.md`](../tools/testinfo/README.md))
over all of it. The results, in `build/<preset>/coverage/`:

* `lcov/<test>.info`: the lcov tracefile (format of lcov 2.x) of each
  test (`lcov/host/X.info` for host test `X`), its name as `TN:` (in
  lcov's characters: `[A-Za-z0-9_]`, every other one as `_`). Per source
  file every function of the measured code the test ran with its first
  and last line (`FN:`) and its count (`FNDA:`); the host tests also with
  line and branch counts (`DA:`, `BRDA:`). The compilers' coverage files
  give each function a count of 1 if it was entered, else 0, and no line
  counts: the one `DA:` of such a function is on its first line and holds
  the function's count (its entry, so that `genhtml` shows the file); a
  function whose lines are not known has `FN:1,<name>` without a last
  line and no `DA:`. Details: `tools/lib/covmap/covmap.h`.
* `lcov/all.info`: all tests in one tracefile, for `genhtml`
  (`genhtml -o <dir> build/amiga-cov/coverage/lcov/all.info`, lcov 2.0
  or later; lcov is not needed by the build), editors and coverage
  services; `--branch-coverage` adds the host tests' branches (checked
  with lcov 2.6). The report lists the compilers' files with their
  functions; their "lines" are the functions' entry lines, so a file's
  line rate there is the rate of its functions. With
  `--branch-coverage`, lcov warns `inconsistent: no branch data for
  <test> in <file>` for the files both the host tests and the compilers
  measure (the compilers give no branches); it is a diagnostic only
  (`--ignore-errors inconsistent` silences it). A file whose function
  lines are not known (`glue_direct.c`) is not in the report.
* `commit`: the commit the run was made from.

The inputs handed to `testinfo map` stay in `build/<preset>/coverage/map-inputs/`.

While working on a change, the tracefiles choose the tests it can affect:

```
cmake -DBUILD=build/amiga -DCOVERAGE=build/amiga-cov/coverage \
      [-DBASE=<commit>] -P cmake/coverage-select.cmake
ctest --preset all --tests-from-file coverage/selected.txt
ctest --preset bug-orig --tests-from-file coverage/selected.txt
```

(ctest reads a relative `--tests-from-file` in the build directory.)

`cmake/coverage-select.cmake` runs `git diff -U0` of the working tree
against the coverage's commit (and lists the files git does not track),
`ninja -t deps` and `ctest --show-only=json-v1` of the build and its
host project, and hands it all to `testinfo select`. It prints why each
test is selected and writes `build/amiga/coverage/selected.txt`. The
rules for what a change of a file selects are in
`tools/testinfo/rules.txt`, the first matching pattern counts:

* a changed line of a measured source (`programs/blitz2/src/*.c`): the
  tests that executed the function around it; a line outside every
  function, or a source without line ranges, the tests of all its
  functions;
* a header of the compilers: each source whose object depends on it
  counts as changed as a whole, by its own rule (a measured source: the
  tests of all its functions; glue, which is not measured: all tests);
* a test source or expect file: the tests of the suites that read it (by
  name or label); a host test source: the host tests; the Amiga helper
  programs of the suites (`tests/*/amiga/`), which several suites use:
  all tests;
* what the emulator tests run on the host: the runners
  (`tests/*/runner/`), the emulator sessions
  (`tools/lib/amiga_emulation/`), `amiga_run`, the `@stat` line
  (`tools/lib/teststats/`) and `results` (its `match` compares the
  output of emulator tests): every emulator test and the host tests
  (`results` also `prev_results`);
* the tools only the host tests and the coverage scripts use
  (`testinfo`): the host tests; every other tool makes or
  checks results of the build (`blitzlib`, `hunkcheck`,
  `xtra`, `bbsrc`, ...): all tests;
* `libs/`: the emulator tests and the checks of the libraries;
* documentation: no test;
* the build (CMake files, presets), TED, shared code, the test data, and
  every file no rule knows: all tests.

Tests without a tracefile that names an executed function are always
selected: new ones, those that run no measured program, and those whose
compiler never reaches its normal end (the coverage file is written
there: a compiler that hangs or crashes under a bug-orig test leaves
none). With the map of the coverage run below that is a fixed part of
162 of the 495 tests in every selection; a change of one `eval.c`
function entered by two tests selects 164. If the coverage's commit is
not an ancestor of `BASE` (default: the coverage's commit), the coverage
is stale and all tests are selected. The selection is for the work in
between: the full runs (`ctest --preset all`, `ctest --preset bug-orig`)
stay the acceptance of every program change.

### Compiler functions no test reaches

The coverage run of commit `e2db70e` (all 495 tests): of the 1132
functions of `blitz2` and `blitz2-ab3` (their sources in
`programs/blitz2/src/` and the shared code they link), 888 are entered by
an emulator test, 53 only by host tests (the shared code of the `.xtra`
files and the tokenizer), and 191
by no test. Of these, 9 are not measured (the assembler entry points of
`glue.c` and `glue_direct.c`, and `shared/cov/cov.c` itself); the other
182:

* 103 of the 108 in `parse.c` are compile error entry points (one
  function per message, `alconsterr` "Constant already defined",
  `mismatcherr` "Type Mismatch", ...): the tests provoke few compile
  errors. Four more such functions are in `compile.c`, `main.c` and
  `tokens1.c`;
* the editor side of the compiler: 23 of the 27 functions of `data.c`
  (the menu actions: options, Create Resident, the list and type
  requesters) and 10 in `main.c` (`ed_*` screen, gadget and tokenising
  calls, `flash`, `menu8`): the tests compile from the CLI or through
  `tedstub`;
* 11 in `eval.c` (code emitters of rarely used operators such as
  `doeorb`, `doasrb`, `dolslb`, and `pevalu`/`bakpevalu`), 5 in
  `ext_bb2.c` (the BB2 side of extension points whose syntax no test
  uses, e.g. `ext_dcbescape`), the rest single functions in `asmlibs.c`
  (`fetchpees*`), `stmts.c`, `tokens1.c`, `tokens2.c`, `compile.c`,
  `main.c` and `libcall.c`.

The list (file, function, lines) is made from the tracefiles of a
coverage run: the functions with `FNDA:0` in every tracefile.

