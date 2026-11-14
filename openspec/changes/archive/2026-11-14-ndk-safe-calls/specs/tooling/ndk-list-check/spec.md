## ADDED Requirements

### Requirement: The project's list matches the NDK's
The configure step SHALL read the toolchain's NDK inline headers, collect
the functions with a parameter passed in a3, a4 or a5, and compare them
with the project's list (the functions that have a safe form and the ones
that are poisoned). Any difference, in either direction, SHALL be a
configure error that names the function and the header. The check SHALL
need no tool beyond CMake.

#### Scenario: A toolchain update adds a function
- **WHEN** the NDK headers declare a function with an a3 parameter that the project's list lacks
- **THEN** configuring fails and names that function

#### Scenario: The list names a function the NDK no longer has
- **WHEN** the project's list holds a name no NDK header declares with such a parameter
- **THEN** configuring fails and names it

#### Scenario: The lists agree
- **WHEN** the NDK and the project's list hold the same functions
- **THEN** configuring continues

### Requirement: The checks have tests with broken input
A test SHALL run the comparison against a list that lacks one name and
SHALL expect the error. A test SHALL compile a source that calls a
poisoned function and SHALL expect the compile error.

#### Scenario: Broken list
- **WHEN** the comparison runs against a list without `TextFit`
- **THEN** it fails with a message naming `TextFit`

#### Scenario: Poisoned call
- **WHEN** the test source calls a poisoned function
- **THEN** the compiler reports the poisoned name and the test passes
