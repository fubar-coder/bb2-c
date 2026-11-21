# Spec Delta

## MODIFIED Requirements

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

### Requirement: The driver's labels and the check macros
The batch init and the batch end SHALL use the fixed labels `tc__init`,
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

### Requirement: Lines of the result file
The result file SHALL hold one entry per line, fields separated by TAB, each
line ended by LF. `K` SHALL be decimal; a long value SHALL be `$` and 8
upper-case hex digits; a string value SHALL hold the bytes `$20` to `$7E` other than `\` as they are and every
other byte as `\xHH`. Each value SHALL have this one form only: `K` and the
version without sign and leading zero, hex digits upper-case, `\xHH` only
for a byte that does not stand as it is.

#### Scenario: A string with a tab
- **WHEN** a string check records the actual value "a" TAB "b"
- **THEN** its field reads `a\x09b`

#### Scenario: A value in another form
- **WHEN** the reader gets `K` as `03`, a long value as `$0000001a` or a string value holding `\x41`
- **THEN** it refuses the value as malformed, naming the line

### Requirement: Entries of the result file
The entries SHALL be: `begin` (format version `1`), `block` (`init` or
`final`, or `case` and the name as a field of its own), `check` (`K`, `ok`
or `fail`, expected, actual), `checkq`, `checkflt`, `checkstr` (`K`, `ok`
or `fail`, expected, actual, `-` or `cut`), `require` (`K`, `broken`; only a failed
one), `expect-error` (`K`), `blockend`, `broken` (a reason), `incomplete`,
`end`. An entry SHALL belong to the block of the last `block` entry before
it.

#### Scenario: A failed check in a case
- **WHEN** case `a` fails check 3 with expected 16 and actual 0
- **THEN** the file holds `block` `case` `a` and later `check` `3` `fail` `$00000010` `$00000000`

#### Scenario: A block entry with the wrong fields
- **WHEN** the reader gets `block` `case` without a name, or `block` `init` with one
- **THEN** it refuses the line as a wrong number of fields, naming the line

## ADDED Requirements

### Requirement: A condition is evaluated as a long
The condition of `!Check` and `!Require` SHALL be evaluated in a Long
context: a Quick variable or function value in it that is fetched while
the type so far is Long (the Long parameter, or a Long left operand) is
cut to a Long at once; Floats and literals with a fraction are not cut. A
quick value SHALL be checked with `!CheckQEq`, a float value with
`!CheckFltNear`.

#### Scenario: A quick condition in a plain check
- **WHEN** a source with `q.q=1.5` holds `!Check{q>1}`
- **THEN** the check fails, since `q` is cut to the long 1 before the comparison; `!CheckQEq{q,1.5}` passes

### Requirement: Preconditions and failed checks of the batch init
A failed precondition SHALL write its `require` entry and `blockend`: the
library closes the block. After a failed precondition in the batch init
every case the driver calls SHALL get its `block` entry, that `require`
entry again and `blockend`, and SHALL NOT run; the batch end SHALL NOT run
and SHALL write nothing. After a failed check in the batch init every case
SHALL get the first failed check of the init again after its `block`
entry, and SHALL run.

#### Scenario: A failed precondition in the batch init
- **WHEN** the batch init fails `TLRequire 2,0` and the driver calls the cases a and b and the batch end
- **THEN** the file holds for a and for b `block` `case` NAME, `require` `2` `broken` and `blockend`, neither case runs, and nothing of the batch end

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
last field `cut`, and its verdict SHALL be that of the whole strings.

#### Scenario: A quick value
- **WHEN** a quick check records the actual value 1.5
- **THEN** its field reads `$00018000`

#### Scenario: A string check too long for a line
- **WHEN** a string check compares two equal strings of 3000 bytes
- **THEN** its entry is `checkstr` `K` `ok` with both values shortened and the last field `cut`, at most 512 bytes

### Requirement: Entries of calls out of order
A call of a routine out of order SHALL write `broken` with its reason
before the routine's own entry: `block inside a block`, `check outside a
block`, `block end without a block`, `batch init after a block`, `case
after the batch end`, `second batch end`, `begin inside the run`, `case
without a name`, `case name too long` (the case's `block` entry then names
it `?`). A library whose state was overwritten SHALL write `broken` `state
damaged` once and after it only `end`.

#### Scenario: A check outside a block
- **WHEN** a check runs after the case's `blockend` and before the next `block`
- **THEN** the file holds `broken` `check outside a block` and then the check's entry
