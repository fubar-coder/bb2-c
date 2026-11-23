# Spec Delta

## Purpose

Runs cases of several test sources of the BASIC model in shared emulator
sessions and hands each case's verdict to the case's own registered test,
so that a test is not slower than it would be alone.

## ADDED Requirements

### Requirement: A bundle runs its members in shared sessions
The runner SHALL run a bundle's members, each a case of a source, with one
cell per member, the cells of the same session key in sessions of at most
16 member cells in the members' order, every session of a key with the same
emulator configuration whatever its number of cells. A session that ends
before a cell completed SHALL be followed by one for the cells behind it.

#### Scenario: Twenty members of one key
- **WHEN** a bundle has 20 members of the same session key and every cell ends
- **THEN** the runner starts two sessions, the first with 16 member cells and the second with 4, both with the same emulator configuration

#### Scenario: A hanging member
- **WHEN** the 3rd of 5 members hangs until the session ends
- **THEN** members 1 and 2 are judged from that session and members 4 and 5 run again in the next

### Requirement: A member is judged as its case alone
Each member SHALL be judged from its cell by the assertions and deviations
of the run of its case alone, with the same report block.

#### Scenario: The same report
- **WHEN** a member passes in a bundle
- **THEN** its block lists the same checks and the same verdict line as the run of the case alone

### Requirement: Only clean passes and reproductions are stored
The bundle SHALL store a member's verdict only if it is PASSED or
REPRODUCED and its cell ended cleanly, also when a later cell ended the
session; every other member SHALL be left to its case test, named in the
bundle's report with the reason.

#### Scenario: A failed member
- **WHEN** a member fails in a bundle
- **THEN** no verdict is stored for it and its case test runs the case alone

#### Scenario: A member before a hang
- **WHEN** a member's cell completed cleanly and passed before a later cell hung
- **THEN** its verdict is stored

### Requirement: The exit status of a bundle
A bundle SHALL exit 0 when it ran through, whatever its members' verdicts,
77 when it was skipped before any session, and 2 on an error of its own
(the store cannot be written, out of memory, a wrong command line).

#### Scenario: A bundle with a failed member
- **WHEN** one member of a bundle fails and the others pass
- **THEN** the bundle exits 0

#### Scenario: No emulator
- **WHEN** the emulator cannot start
- **THEN** the bundle exits 77 and stores nothing

### Requirement: A stored verdict is fresh only for the same inputs
A stored verdict SHALL name the inputs it was judged from; a case test
SHALL use it only if the inputs are still the same (the source, the
configuration files, the test's own data, the environment and the files it
names, the runner, the emulator) and SHALL remove it when read.

#### Scenario: A source changed after the bundle
- **WHEN** the source of a member is changed after the bundle stored its verdict
- **THEN** the case test runs the case alone and the entry is gone afterwards

#### Scenario: Read twice
- **WHEN** a case test was served a verdict and is run again
- **THEN** it runs the case alone

### Requirement: A served case test
A case test that names its bundle SHALL, with a fresh stored verdict,
print the stored report block and the bundle's name and exit with the
stored verdict's status without starting a session; without a fresh entry
it SHALL run the case as without the bundle. A malformed entry SHALL be
named and the case run alone.

#### Scenario: Served
- **WHEN** the bundle stored PASSED for a case and the case test runs next with the same inputs
- **THEN** the case test exits 0, prints the block and names the bundle, and starts no session

#### Scenario: Without the bundle
- **WHEN** only the case test is run (`ctest -R` of its name)
- **THEN** it runs the case alone, as without bundles

#### Scenario: A broken entry
- **WHEN** the entry of a case is cut after its key line
- **THEN** the case test names the malformed entry and runs the case alone
