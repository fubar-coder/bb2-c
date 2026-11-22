# testing/expression-matrix Specification

## Purpose
A systematic test of the type and value blitz2 gives an expression of two operands, or of one operand under a unary operator, in each context: which cells are tested, how the expected value of a cell is derived from the user's recorded rules, where each cell's test lives, and how cells of open original bugs fail while all other cells pass.

## Requirements

### Requirement: The dimensions of the matrix
The matrix SHALL be the product of the operand kinds, the operators, the contexts and both orders of two operands, reduced only as the design states with the user's decision. Operand kinds: Byte, Word, Long, Quick and Float variables; a string variable for `+` and the comparisons; literal constants of Byte, Word and Long size and with a fraction.

#### Scenario: Both orders
- **WHEN** a cell with two operand kinds of different types exists
- **THEN** the cell with the two kinds swapped exists too, with its own key

### Requirement: The operators and contexts of the matrix
The operators SHALL be `+ - * / MOD ^ AND OR EOR LSL LSR ASL ASR` and `= <> < <= > >=` for two operands, unary minus and `Not` for one. The contexts SHALL be a destination of type Byte, Word, Long, Quick or Float; a Long, Word, Quick or Float parameter; `If`; the first and a later array index; `Print`.

#### Scenario: A parameter context
- **WHEN** the matrix holds the cell "Word parameter, `+`, Quick then Word"
- **THEN** the program passes `q+w` to a procedure with a Word parameter, which prints its argument

### Requirement: A cell prints one keyed line
A cell SHALL have a unique key that names its context, operator, operand kinds in order and value set, and SHALL print one line with that key and its value; a Quick or Float value SHALL be printed as its raw bits.

#### Scenario: A cell and its key
- **WHEN** the cell "Long destination, `*`, Quick then Long, value set A" runs
- **THEN** the program prints one line that starts with that cell's key and holds the value of `r.l=q*l`

### Requirement: Every cell is measured with the originals and the port
Every cell SHALL be measured with the CD's blitz2 2.10, blitz2 1.95 built from `blitz2.s`, AmiBlitz3 v3.14.0, and the port with the CD's libraries and with the C libraries, before its expected value is set. A cell that does not compile with an original SHALL be recorded as such and left out of the test sources.

#### Scenario: The originals disagree
- **WHEN** 2.10, 1.95 and AmiBlitz3 do not give the same value for a cell
- **THEN** the cell is reported with the three values and gets no expected value until the user decides

### Requirement: Index cells outside the array only with checking
An array index cell SHALL be measured with runtime error checking on, and with it off only where the checked build of that compiler reads an element of the array; an index cell outside the array SHALL be measured and tested only with checking on.

#### Scenario: An index outside the array
- **WHEN** an index cell's expression gives an index below 0 or beyond the `Dim` in the checked build
- **THEN** the cell's measured and expected result is the runtime error "Array Subscript out of Range" with checking on, and the cell is in no source run with checking off

### Requirement: AmiBlitz3's index cells without checking
AmiBlitz3's index cells SHALL be measured with runtime error checking off only, over the cells that 1.95's checked build reads inside the array: AmiBlitz3's checked mode needs its debugger module and checks no index without it.

#### Scenario: AmiBlitz3 and an index cell
- **WHEN** an index cell is inside the array in 1.95's checked build
- **THEN** AmiBlitz3's value of the cell is measured with checking off, and no checked value of AmiBlitz3 is recorded

### Requirement: The expected value comes from a recorded rule
The expected value of a cell SHALL come from a recorded decision of the user that covers the operation, and each rule of the model SHALL name its entry by heading and date or ID. Where no decision covers it, the expected value SHALL be the original's value if that is the exact result of the operation in the type the original computes it in, converted to the context as the original converts.

#### Scenario: A rule covers the cell
- **WHEN** a cell is `r.l=q*2` with `q=1.5`
- **THEN** its expected value is 3, derived from the rule of #1178 (variant C), and the model names that rule

#### Scenario: The original is exact
- **WHEN** no decision covers a cell and the original gives `r.l=l+w` as the exact sum of 100000 and 1000
- **THEN** the expected value is 101000

### Requirement: Cells without a rule are questions
A cell that neither a recorded decision nor the exact-result rule covers SHALL get no expected value, SHALL be listed with its class as a question for the user, and SHALL NOT be part of a committed test.

#### Scenario: No rule covers the cell
- **WHEN** a cell is a bitwise operator with a Quick operand that has a fraction
- **THEN** the cell is listed in its class as a question, and no committed test contains it

### Requirement: Each cell has a class
Each cell SHALL have one class: agreement (the originals, the port and the expected value agree), fixed (the port gives the expected value, the originals do not; issue named), open (the port gives the originals' value, the expected value differs; issue named), port defect (the port differs from the originals without a fix), or question.

#### Scenario: A fixed cell
- **WHEN** a cell is `r.l=(q>1)` with `q=1.5`
- **THEN** its class is fixed with issue #1177: the originals give 0, the port and the expected value -1

### Requirement: Each class has its test
Agreement cells SHALL be tested against the port in `all` and against the original in `originals`; a fixed cell SHALL be a pair of the original's and the port's value; open cells SHALL be in bug-orig tests that come out REPRODUCED while the issue is open and FIXED after its fix, so that a fix turns its own cells green and leaves every other cell as it was.

#### Scenario: A fix of an open bug
- **WHEN** the fix of #1178 is merged
- **THEN** the bug-orig tests of the cells of #1178 come out FIXED, and every agreement and fixed cell gives its previous value

#### Scenario: A fix changes another cell
- **WHEN** a fix changes the value of a cell that is not one of its issue's cells
- **THEN** a test of the matrix fails naming that cell's test

#### Scenario: A cell of two open bugs
- **WHEN** a cell needs the fixes of two issues
- **THEN** it is in a test of its own named after both issues, whose header names the fix that turns it FIXED

### Requirement: Generated sources and their check
The test sources and expected files of the matrix SHALL be generated by a C tool of `tools/` from a committed description of the matrix and the model's expected values, written into the source tree by a hyphenated build target and reviewed with `git diff`; a host test SHALL fail when a committed file differs from the generator's output, and SHALL be checked with a deliberately changed file.

#### Scenario: A committed file edited by hand
- **WHEN** a generated source or expected file is changed without the generator
- **THEN** the host test fails naming the file

### Requirement: AmiBlitz3 checks the agreement cells
The sources of the agreement and fixed cells SHALL also be cases of `tests/ab3` compiled by AmiBlitz3 v3.14.0's own compiler in the preset `oracle`, judged against AmiBlitz3's recorded output; a cell where AmiBlitz3 differs from the port SHALL be named in `docs/ab3/variant.md`.

#### Scenario: AmiBlitz3 keeps the old rule
- **WHEN** a cell is a fixed cell of #1177
- **THEN** AmiBlitz3's recorded output holds the original's value and the difference is named in `docs/ab3/variant.md`

### Requirement: The matrix changes no result
Adding the matrix SHALL change no executable the compilers write, no library file and no file TED writes.

#### Scenario: Previous-commit comparison
- **WHEN** `ctest --preset all` runs with the prev build after the change
- **THEN** no result differs from the previous commit
