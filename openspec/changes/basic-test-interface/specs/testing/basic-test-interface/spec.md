# Spec Delta

## Purpose

Defines the contract between Blitz test sources, the runner's
preprocessor and generated driver, the test library and its result file,
so that the test library (`testing/basic-testlib`) and the runner
(`testing/basic-test-runner`) can be built and changed against one text.

## ADDED Requirements

### Requirement: Block macros of a test source
A test source SHALL mark its blocks with `!TestBatchInit` / `!EndTestBatchInit`
(at most one, before the first case), `!TestCase{NAME}` / `!EndTestCase`
(one or more) and `!TestBatchEnd` / `!EndTestBatchEnd` (at most one, after
the last case). Only a comment SHALL follow a block macro on its line.

#### Scenario: A source with all blocks
- **WHEN** a source has a batch init, two cases and a batch end in this order
- **THEN** the preprocessor accepts it and the driver calls the batch init, the selected cases and the batch end

### Requirement: Check macros of a test source
A test source SHALL check with `!Check{COND}` (ok if not 0),
`!CheckEq{ACTUAL,EXPECTED}` (long values), `!CheckStrEq{ACTUAL$,EXPECTED$}`
(strings), `!Require{COND}` (precondition) and `!ExpectError{"MESSAGE"}`.
Each MAY take a name as an extra last argument, a string literal.

#### Scenario: A named check
- **WHEN** a source holds `!CheckEq{ShapeWidth(99),16,"width"}`
- **THEN** the check is reported with the name `width`

### Requirement: The preprocessed form
The preprocessor SHALL give every check, precondition and announcement of a
source a number `K`, counted from 0 over the whole source in source order,
SHALL write `K` as the first argument and remove the name and the message:
`!CheckEq{K,ACTUAL,EXPECTED}`, `!CheckStrEq{K,ACTUAL$,EXPECTED$}`,
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
The batch init and the batch end SHALL use the fixed labels `tc__init`,
`tc__init_end`, `tc__final` and `tc__final_end`; labels with `__` SHALL
belong to the driver. The check macros SHALL call the library routines
with `K` as the first argument.

#### Scenario: A case named like a driver label
- **WHEN** a case would be named `_init`
- **THEN** it is refused, since a case name starts with a letter and holds no `__`

### Requirement: The library routines
The library SHALL provide `TLBegin path$` (opens the result file),
`TLInit`, `TLFinal` and `TLCase(name$)` (1, or 0 on the second return after
a failed precondition), `TLBlockEnd`, `TLCheck K,expected,actual`,
`TLCheckStr K,expected$,actual$`, `TLRequire K,cond` and `TLExpectError K`,
and a finit that writes `incomplete` if a block is open, then `end`.

#### Scenario: A failed precondition
- **WHEN** `TLRequire K,0` runs inside a case
- **THEN** the `TLCase` call of that case returns a second time with 0 and the case returns

### Requirement: The order of the driver
The driver SHALL define the macro text, call `TLBegin` with the name of the
result file, jump over the preprocessed text of the source, and then call
with `Gosub` the batch init (only if the source has one and a case is
selected), each selected case in source order, and the batch end (only if
the source has one), and end.

#### Scenario: No case selected
- **WHEN** the driver is generated with no case selected
- **THEN** it calls neither the batch init nor the batch end

### Requirement: Lines of the result file
The result file SHALL hold one entry per line, fields separated by TAB, each
line ended by LF. `K` SHALL be decimal; a long value SHALL be `$` and 8
upper-case hex digits; a string value SHALL hold the bytes `$20` to `$7E`
other than `\` as they are and every other byte as `\xHH`.

#### Scenario: A string with a tab
- **WHEN** a string check records the actual value "a" TAB "b"
- **THEN** its field reads `a\x09b`

### Requirement: Entries of the result file
The entries SHALL be: `begin` (format version `1`), `block` (`init`,
`case NAME` or `final`), `check` and `checkstr` (`K`, `ok` or `fail`,
expected, actual), `require` (`K`, `broken`; only a failed one), `expect-error`
(`K`), `blockend`, `broken` (a reason), `incomplete`, `end`. An entry SHALL
belong to the block of the last `block` entry before it.

#### Scenario: A failed check in a case
- **WHEN** case `a` fails check 3 with expected 16 and actual 0
- **THEN** the file holds `block` `case a` and later `check` `3` `fail` `$00000010` `$00000000`

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
