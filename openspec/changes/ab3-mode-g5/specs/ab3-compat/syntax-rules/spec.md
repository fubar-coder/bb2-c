# Spec Delta

## ADDED Requirements

### Requirement: Dim with KEEP
In AB3 mode `Dim` SHALL accept the word `KEEP` as AmiBlitz3 v3.14.0 does (to be fixed by the verification task: its position and effect, from the `Dim KEEP` case among the examples), and an array re-dimensioned with it SHALL behave as AmiBlitz3's.

#### Scenario: Dim KEEP
- **WHEN** a program re-dimensions an array with `KEEP`
- **THEN** it compiles and prints what AmiBlitz3's executable prints

### Requirement: Constants in Data lines
In AB3 mode a `Data` line SHALL accept a constant (`#name`, as in `Data.w #c`) where it accepts a number, with the value and the error cases of AmiBlitz3.

#### Scenario: Data.w with a constant
- **WHEN** a program has `#c = 5` and `Data.w #c`
- **THEN** the data word is 5 as in AmiBlitz3

### Requirement: Casts with (.type)
In AB3 mode an expression SHALL accept the cast `(.type) value` of the forms AmiBlitz3 v3.14.0 accepts (the verification task lists them), with its result type and its conversion; a cast that AmiBlitz3 refuses SHALL be refused.

#### Scenario: Cast of a long to a float
- **WHEN** a program contains `x.f = (.f) a.l`
- **THEN** it compiles and gives the value AmiBlitz3 gives

### Requirement: Object
In AB3 mode `Object` SHALL be accepted as in AmiBlitz3 (the verification task fixes its meaning and the sources that use it, `winuaeDLL`), and no longer give "Not supported: Object".

#### Scenario: winuaeDLL
- **WHEN** a wrapper includes the include that uses `Object`
- **THEN** the include compiles past that word
