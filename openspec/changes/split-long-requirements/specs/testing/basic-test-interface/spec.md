## MODIFIED Requirements

### Requirement: Entries of the result file
The entries of the result file SHALL be those of the requirements "Entries of a block", "Entries of a check" and "Entries of an order error" and those of further requirements. An entry SHALL belong to the block of the last `block` entry before it.

#### Scenario: A failed check in a case
- **WHEN** case `a` fails check 3 with expected 16 and actual 0
- **THEN** the file holds `block` `case` `a` and later `check` `3` `fail` `$00000010` `$00000000`

#### Scenario: A block entry with the wrong fields
- **WHEN** the reader gets `block` `case` without a name, or `block` `init` with one
- **THEN** it refuses the line as a wrong number of fields, naming the line

## ADDED Requirements

### Requirement: Entries of a block
The entries that frame the file and its blocks SHALL be: `begin` (format version `1`), `block` (`init` or `final`, or `case` and the name as a field of its own), `blockend`, `incomplete`, `end`.

#### Scenario: The format version
- **WHEN** the reader gets `begin` with the format version `1`
- **THEN** it accepts the entry

#### Scenario: A case block
- **WHEN** a case `a` runs
- **THEN** the file holds `block` `case` `a`, the entries of the case and `blockend`

### Requirement: Entries of a check
The entries of a check SHALL be: `check` (`K`, `ok` or `fail`, expected, actual), `checkq`, `checkflt`, `checkstr` (`K`, `ok` or `fail`, expected, length, CRC32, actual, length, CRC32, `-` or `cut`), `require` (`K`, `broken`; only a failed one), `expect-error` (`K`).

#### Scenario: A failed require
- **WHEN** `!Require` number 2 of a case fails
- **THEN** the file holds `require` `2` `broken` and no `require` entry for a passed one

#### Scenario: A string check
- **WHEN** a string check of the case numbered 5 compares two strings that differ
- **THEN** the file holds `checkstr` `5` `fail` with expected, length, CRC32, actual, length, CRC32 and `-` or `cut`

### Requirement: Entries of an order error
The entry of an order error SHALL be `broken` (a reason).

#### Scenario: An order error
- **WHEN** a check is called outside a block
- **THEN** the file holds `broken` `check outside a block` and then the check's entry
