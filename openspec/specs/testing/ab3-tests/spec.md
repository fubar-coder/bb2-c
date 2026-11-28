# AB3 Tests Specification

## Purpose

Specify how AB3 mode is tested, how AmiBlitz3 supplies reference
results, and which cases use the command-line suite.

## Requirements

### Requirement: The run-output cases of AB3 mode are tests of the model
A case of AB3 mode whose expectation is the output and exit code of the
compiled program, compiled without arguments or with `-r` in the default
`Blitz3:` tree, SHALL be a case of a test of the BASIC test model in
`tests/ab3/model/` with the cells `compiler = "ab3"` and, where AmiBlitz3
is the oracle of the case, `compiler = "amiblitz3"`, unless the next
requirement keeps it in the command-line form.

#### Scenario: A moved case
- **WHEN** the case `proc_defaults` of `tests/ab3` is moved
- **THEN** it is the case `proc_defaults` of a test in `tests/ab3/model/`, registered as `ab3_ID__proc_defaults` and `ab3_ID__proc_defaults_oracle`, and `tests/ab3/CMakeLists.txt` no longer lists it

### Requirement: The cases that keep the command-line form
A case SHALL stay a case of `ab3_cli` / `ab3_cli_ref` if its subject is
the compiler's console output or return code (but for a warning case of
the requirement "Warning cases are tests of the model"), another tree,
other arguments than `-r`, a fixed date, an executable that is not run,
two identical executables, the hunk layout, one of AmiBlitz3's own
fetched test programs, the lookup of an include or resident by its name,
or a source form a driver cannot have.

#### Scenario: A compile-error case
- **WHEN** a new AB3-mode case expects "Illegal Operator for Type" and return code 10
- **THEN** it is added as an `ab3_cli_case` with its `.expect` file

#### Scenario: A warning case with a warning list
- **WHEN** a case compiles with `-werr datafrac`
- **THEN** it stays an `ab3_cli_case`

### Requirement: A moved case checks the values of its expect file
A moved case SHALL check every line of the `[run]` section of its former
`NAME.expect`: each value line as the expected value of one check, or the
whole section as the expected printed output where printing is the
subject of the case; its exit code SHALL be judged as before. A moved
case SHALL be committed together with the removal of its former case,
and only after both passed on the same build.

#### Scenario: The proof of a move
- **WHEN** a case with five value lines is moved
- **THEN** its result file holds five checks whose expected values are the five lines in order, and all pass

### Requirement: AmiBlitz3 is the oracle side
The cells of `compiler = "amiblitz3"` SHALL be the side `oracle`, with
the label `oracle` and without the label `emulator`, so that the presets
`emulator` and `all` leave them out and the preset `oracle` runs them.

#### Scenario: The preset all
- **WHEN** `ctest --preset all` runs
- **THEN** no test of the side `oracle` runs

### Requirement: A difference of AmiBlitz3 is a declared deviation with its row
What AmiBlitz3 does differently from the expected value SHALL be declared
in the table `deviations."compiler=amiblitz3"` of the test's own file
with the key `variant` naming the row of `docs/ab3/variant.md` that
records the difference; it SHALL need no issue and SHALL NOT make the
case a test of an open bug. A `variant` value that names no row of
`docs/ab3/variant.md` SHALL fail the build naming the test and the value.

#### Scenario: AmiBlitz3 makes no executable
- **WHEN** AmiBlitz3 refuses `b.b=@"A"` and the test declares `compile = "contains \"convert\""` with `variant = "chr_nospace"`
- **THEN** the oracle side passes, and the port side checks the value 65

#### Scenario: A row that does not exist
- **WHEN** a test declares `variant = "no_such_row"`
- **THEN** the build fails naming the test and `no_such_row`

### Requirement: New run-output cases of AB3 mode are written in the model form
A new case of AB3 mode within the first requirement's scope SHALL be
written as a case of a test in `tests/ab3/model/`, not as an `ab3_cli`
case.

#### Scenario: A case of a later change
- **WHEN** a change adds a probe of `Dim ... KEEP` whose program prints values
- **THEN** it is a case of a test in `tests/ab3/model/` with checks of those values

### Requirement: Warning cases are tests of the model
A case whose subject is the warnings of a compile without other arguments
than `-r`, and the return code with them, SHALL be a case of a test of the
BASIC test model, judged with the queries of the warnings and of the
compile's return code (`testing/basic-test-runner`).

#### Scenario: A warning case
- **WHEN** a new AB3-mode case expects one warning line and return code 5 from a compile without arguments
- **THEN** it is a case of a test in `tests/ab3/model/` that asserts `compile.warnings` and `compile.rc`

### Requirement: A moved warning case keeps its compile lines
A moved case whose former `[compile]` section holds warning lines SHALL
assert each of them as an entry of the query of the warnings, in the same
order and with the place of the test source, no further entry, and its
`rc=` line as the expected return code of the compile; its `[run]` section
SHALL be checked as for any moved case.

#### Scenario: A moved warning case
- **WHEN** the case `datafrac_many` with ten warning lines, `rc=5` and one value line is moved
- **THEN** its test asserts the ten entries of `compile.warnings` in order and no more, `compile.rc == 5`, and one check of the value
