# Delta: testing/ab3-tests

## MODIFIED Requirements

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

## ADDED Requirements

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
