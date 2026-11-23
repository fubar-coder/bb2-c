# testing/basic-test-interface Specification

## Purpose

Defines the contract between Blitz test sources, the runner's
preprocessor and generated driver, the test library and its result file,
so that the test library (`testing/basic-testlib`) and the runner
(`testing/basic-test-runner`) can be built and changed against one text.

## Requirements

### Requirement: Block macros of a test source
A test source SHALL mark its blocks with `!TestBatchInit` / `!EndTestBatchInit`
(at most one, before the first case), `!TestCase{NAME}` / `!EndTestCase`
(one or more) and `!TestBatchCleanup` / `!EndTestBatchCleanup` (at most one, after
the last case). Only a comment SHALL follow a block macro on its line.

#### Scenario: A source with all blocks
- **WHEN** a source has a batch init, two cases and a batch cleanup in this order
- **THEN** the preprocessor accepts it and the driver calls the batch init, the selected cases and the batch cleanup

### Requirement: Check macros of a test source
A test source SHALL check with `!Check{COND}` (ok if not 0),
`!CheckEq{ACTUAL,EXPECTED}` (long values), `!CheckQEq{ACTUAL,EXPECTED}`
(quick values, ok if equal), `!CheckFltNear{ACTUAL,EXPECTED,DELTA}` (float
values, ok if |ACTUAL-EXPECTED| <= DELTA), `!CheckStrEq{ACTUAL$,EXPECTED$}`
(strings), `!Require{COND}` (precondition) and `!ExpectError{"MESSAGE"}`.
Each MAY take a name as an extra last argument, a string literal.

#### Scenario: A named check
- **WHEN** a source holds `!CheckEq{ShapeWidth(99),16,"width"}`
- **THEN** the check is reported with the name `width`

#### Scenario: A float within its delta
- **WHEN** a source holds `!CheckFltNear{f,2.5,0.01}` and `f` is 2.501
- **THEN** the check passes, and with `f` 2.6 it fails

### Requirement: The preprocessed form
The preprocessor SHALL give every check, precondition and announcement of a
source a number `K`, counted from 0 over the whole source in source order,
SHALL write `K` as the first argument and remove the name and the message:
`!CheckEq{K,ACTUAL,EXPECTED}`, `!CheckQEq{K,ACTUAL,EXPECTED}`,
`!CheckFltNear{K,ACTUAL,EXPECTED,DELTA}`, `!CheckStrEq{K,ACTUAL$,EXPECTED$}`,
`!Check{K,COND}`, `!Require{K,COND}`, `!ExpectError{K}`.

#### Scenario: Numbers over two cases
- **WHEN** case a holds two checks and case b one
- **THEN** they get the numbers 0, 1 and 2

#### Scenario: Nothing of the text in the program
- **WHEN** a source holds `!ExpectError{"Shape too Big"}` and `!CheckEq{x,1,"one"}`
- **THEN** the driver holds neither `Shape too Big` nor `one`

### Requirement: The macro text of a case
The macro text SHALL make a case a subroutine labelled `tc_NAME` that
returns at once when the library says the block does not run, and whose
opening macro refers with `Dc.l` to a label `tc_end_N` that its closing
macro defines (`N` the macros' use counter).

#### Scenario: A missing closing macro
- **WHEN** the driver holds a case without its closing macro
- **THEN** the compile fails

### Requirement: The driver's labels and the check macros
The batch init and the batch cleanup SHALL use the fixed labels `tc__init`,
`tc__init_end`, `tc__final` and `tc__final_end`; labels with `__` SHALL
belong to the driver. The check macros SHALL call the library routines
with `K` as the first argument; `!Check{K,COND}` SHALL call
`TLCheck K,-1,(COND)<>0`, `!CheckQEq{K,A,E}` SHALL call `TLCheckQ K,E,A`
and `!CheckFltNear{K,A,E,D}` SHALL call `TLCheckFlt K,E,A,D`, so that the
parameter types of the routine set the type in which the values are
evaluated.

#### Scenario: A case named like a driver label
- **WHEN** a case would be named `_init`
- **THEN** it is refused, since a case name starts with a letter and holds no `__`

#### Scenario: A failed condition
- **WHEN** `!Check{K,COND}` runs with `COND` 0
- **THEN** its `check` entry is `fail` with expected `$FFFFFFFF` and actual `$00000000`

### Requirement: The library routines
The library SHALL provide `TLBegin path$` (opens the result file and the
libraries the routines need), `TLInit`, `TLFinal` and `TLCase(name$)` (1,
or 0 on the second return after a failed precondition or if the block does
not run), `TLBlockEnd`, `TLCheck K,expected,actual`, `TLCheckQ`,
`TLCheckFlt`, `TLCheckStr K,expected$,actual$`, `TLRequire K,cond` and
`TLExpectError K`, and a finit that writes `incomplete` if a block is open,
then `end`.

#### Scenario: A failed precondition
- **WHEN** `TLRequire K,0` runs inside a case
- **THEN** the `TLCase` call of that case returns a second time with 0, the case returns, and the file holds `require` `K` `broken` and `blockend`

### Requirement: The order of the driver
The driver SHALL define the macro text, call `TLBegin` with the name of the
result file, jump over the preprocessed text of the source, and then call
with `Gosub` the batch init (only if the source has one and a case is
selected), each selected case in source order, and the batch cleanup (only if
the source has one), and end.

#### Scenario: No case selected
- **WHEN** the driver is generated with no case selected
- **THEN** it calls neither the batch init nor the batch cleanup

### Requirement: Lines of the result file
The result file SHALL hold one entry per line, fields separated by TAB, each
line ended by LF. `K` and a length SHALL be decimal; a long value and a
CRC32 SHALL be `$` and 8 upper-case hex digits; a string value SHALL hold
the bytes `$20` to `$7E` other than `\` as they are and every other byte
as `\xHH`. Each value SHALL have this one form only: `K`, a length and the
version without sign and leading zero, hex digits upper-case, `\xHH` only
for a byte that does not stand as it is.

#### Scenario: A string with a tab
- **WHEN** a string check records the actual value "a" TAB "b"
- **THEN** its field reads `a\x09b`

#### Scenario: A value in another form
- **WHEN** the reader gets `K` as `03`, a long value as `$0000001a`, a string value holding `\x41`, a length as `+3` or a CRC32 as `$cbf43926`
- **THEN** it refuses the value as malformed, naming the line

### Requirement: Entries of the result file
The entries SHALL be: `begin` (format version `1`), `block` (`init` or
`final`, or `case` and the name as a field of its own), `check` (`K`, `ok`
or `fail`, expected, actual), `checkq`, `checkflt`, `checkstr` (`K`, `ok`
or `fail`, expected, length, CRC32, actual, length, CRC32, `-` or `cut`),
`require` (`K`, `broken`; only a failed one), `expect-error` (`K`),
`blockend`, `broken` (a reason), `incomplete`, `end`. An entry SHALL
belong to the block of the last `block` entry before it.

#### Scenario: A failed check in a case
- **WHEN** case `a` fails check 3 with expected 16 and actual 0
- **THEN** the file holds `block` `case` `a` and later `check` `3` `fail` `$00000010` `$00000000`

#### Scenario: A block entry with the wrong fields
- **WHEN** the reader gets `block` `case` without a name, or `block` `init` with one
- **THEN** it refuses the line as a wrong number of fields, naming the line

### Requirement: One codec for writer and reader
Writing and reading of entries SHALL be one module, compiled by the test
library and by the host tools. The reader SHALL refuse an unknown entry, a
wrong number of fields, a malformed value and an unknown format version,
naming the line.

#### Scenario: A malformed value
- **WHEN** the reader gets a `check` entry whose actual value is `$1234`
- **THEN** it fails naming the line

#### Scenario: Round trip
- **WHEN** the host tests write every kind of entry with the codec and read it back
- **THEN** every field comes back unchanged

### Requirement: A condition is evaluated as a long
The condition of `!Check` and `!Require` SHALL be evaluated in a Long
context: with the original compilers a Quick operand fetched while the
type so far is Long is cut to a Long at once (Floats and literals with a
fraction are not), with the port since the fix of #1177 only a compound
left operand (#1183). A quick value SHALL be checked with `!CheckQEq`, a
float value with `!CheckFltNear`.

#### Scenario: A quick condition in a plain check
- **WHEN** a source with `q.q=1.5` holds `!Check{q>1}`
- **THEN** with the original compilers (1.95, 2.10) the check fails, since `q` is cut to the long 1 before the comparison, and with the port it passes; `!CheckQEq{q,1.5}` passes with all of them

### Requirement: Preconditions and failed checks of the batch init
A failed precondition SHALL write its `require` entry and `blockend`: the
library closes the block. After a failed precondition in the batch init
every case the driver calls SHALL get its `block` entry, that `require`
entry again and `blockend`, and SHALL NOT run; the batch cleanup SHALL NOT run
and SHALL write nothing. After a failed check in the batch init every case
SHALL get the first failed check of the init again after its `block`
entry, and SHALL run.

#### Scenario: A failed precondition in the batch init
- **WHEN** the batch init fails `TLRequire 2,0` and the driver calls the cases a and b and the batch cleanup
- **THEN** the file holds for a and for b `block` `case` NAME, `require` `2` `broken` and `blockend`, neither case runs, and nothing of the batch cleanup

#### Scenario: A failed check in the batch init
- **WHEN** the batch init fails check 0 with expected 1 and actual 2, and the driver calls case a
- **THEN** the file holds after `block` `case` `a` the entry `check` `0` `fail` `$00000001` `$00000002`, and case a runs

### Requirement: Typed checks of quick and float values
`TLCheckQ K,expected.q,actual.q` SHALL be ok if the two quick values are
equal and write `checkq` (`K`, `ok` or `fail`, expected, actual);
`TLCheckFlt K,expected.f,actual.f,delta.f` SHALL be ok if
|actual-expected| <= delta, computed with mathffp.library, and write
`checkflt` (`K`, `ok` or `fail`, expected, actual, delta). The parameter
types of the routines set the type in which the macro's values are
evaluated. No routine SHALL call a library that `TLBegin` did not open.

#### Scenario: Floats outside the delta
- **WHEN** `TLCheckFlt 4,2.5,2.6,0.01` runs
- **THEN** the file holds `checkflt` `4` `fail` and the FFP bits of 2.5, 2.6 and 0.01

### Requirement: Quick and float values and the length of a line
A quick or float value SHALL be written as its 32 bits in the form of a
long value: the quick as the long of its 16.16 fixed point, the float as
its Motorola FFP number. A line SHALL be at most 512 bytes with its LF; a
`checkstr` entry whose values do not fit SHALL have them shortened and the
last field `cut`, and its verdict, lengths and CRC32s SHALL be those of the
whole strings.

#### Scenario: A quick value
- **WHEN** a quick check records the actual value 1.5
- **THEN** its field reads `$00018000`

#### Scenario: A string check too long for a line
- **WHEN** a string check compares two equal strings of 3000 bytes
- **THEN** its entry is `checkstr` `K` `ok` with both values shortened, both lengths `3000`, two equal CRC32s and the last field `cut`, at most 512 bytes

### Requirement: Entries of calls out of order
A call of a routine out of order SHALL write `broken` with its reason
before the routine's own entry: `block inside a block`, `check outside a
block`, `block end without a block`, `batch init after a block`, `case
after the batch cleanup`, `second batch cleanup`, `begin inside the run`, `case
without a name`, `case name too long` (the case's `block` entry then names
it `?`). A library whose state was overwritten SHALL write `broken` `state
damaged` once and after it only `end`.

#### Scenario: A check outside a block
- **WHEN** a check runs after the case's `blockend` and before the next `block`
- **THEN** the file holds `broken` `check outside a block` and then the check's entry

### Requirement: Length and CRC32 of a string value
Each string value of a `checkstr` entry SHALL be followed by the length of
the whole string in bytes and the CRC32 of the whole string, whether the
value is cut or not. The CRC32 SHALL be the standard CRC-32 of IEEE 802.3
(as in zlib and PNG): the polynomial $04C11DB7 used reflected ($EDB88320),
input and output reflected, the register set to $FFFFFFFF before the first
byte and the result XORed with $FFFFFFFF; it is $CBF43926 for the nine
bytes `123456789` and $00000000 for the empty string.

#### Scenario: A short string check
- **WHEN** check 5 compares the expected value "123456789" with the actual value "12345678"
- **THEN** its entry is `checkstr` `5` `fail` `123456789` `9` `$CBF43926` `12345678` `8` `$9AE0DAAF` `-`

### Requirement: A cut string value
A string value SHALL be cut if it is shorter than its length; it is then
the start of the whole string. The last field of a `checkstr` entry SHALL
be `cut` if one of its values is cut, and `-` if none is. The reader SHALL
refuse as malformed, naming the line, a value longer than its length, a
value that is not cut whose CRC32 is not that of its bytes, an entry with
the last field `-` and a cut value, and one with the last field `cut` and
no cut value.

#### Scenario: A cut value with its whole length
- **WHEN** a string check compares two different strings of 400 bytes
- **THEN** its entry holds the start of each, the length `400` and the CRC32 of each whole string, and the last field `cut`

#### Scenario: A wrong CRC32 of a complete value
- **WHEN** the reader gets a `checkstr` entry whose last field is `-` and whose expected value `ab` has the length 2 and a CRC32 other than that of `ab`
- **THEN** it refuses the value as malformed, naming the line
