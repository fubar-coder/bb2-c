# Spec Delta

## Purpose

Defines the type `.d` (double) in AB3 mode without the FPU bit: the
sources of AmiBlitz3 that declare `.d` compile, with the layout and the
arithmetic AmiBlitz3's compiler gives them.

## ADDED Requirements

### Requirement: .d is accepted without the FPU bit
In AB3 mode the type `.d` SHALL be accepted wherever a basic type is allowed (variables, arrays, parameters, `Function` return types, NewType fields, `Dim`, pointers, `SizeOf`) when `OPTIMIZE` has not set the FPU bit. Its slot SHALL be 8 bytes. Its value SHALL be computed as FFP, like `.f`, and SHALL be stored in the 4 bytes of the slot that AmiBlitz3 uses. In BB2 mode `.d` SHALL stay "Type Not Found".

#### Scenario: Variable
- **WHEN** a program with `OPTIMIZE 4` declares `a.d = 2.5 : NPrint a*2`
- **THEN** it compiles and prints what AmiBlitz3's executable prints

#### Scenario: SizeOf
- **WHEN** a NewType holds a `.d` field and a `.w` field
- **THEN** `SizeOf` of the NewType and the field offsets equal AmiBlitz3's

#### Scenario: BB2 mode
- **WHEN** a BB2-mode source declares `a.d`
- **THEN** the compile fails with "Type Not Found" as before

### Requirement: .d in parameters, returns and conversions
`.d` SHALL be usable as a parameter type of a `Statement` or `Function` at every position, including the 7th to the 10th, as a `Function` return type, in conversions to and from the other basic types, and as a library argument of type 8, with the results AmiBlitz3's compiler gives with the same libraries.

#### Scenario: Seventh parameter
- **WHEN** a `Function` has `.d` as its 7th parameter and is called with a constant
- **THEN** the program prints the value AmiBlitz3's executable prints

#### Scenario: Function.d
- **WHEN** a `Function.d` returns a computed value and the result is assigned to a `.f` and a `.l`
- **THEN** both values equal AmiBlitz3's

### Requirement: .d in residents
The loader of resident files SHALL accept `.d` in the NewTypes and macros of a `.res` file in AB3 mode, with the same layout as a `.d` NewType field in a source.

#### Scenario: Resident with a .d field
- **WHEN** a program loads `sdl.res.ab3` (which has a `.d` field, line 174)
- **THEN** the resident loads and the NewType has AmiBlitz3's size

### Requirement: The FPU bit still refuses
With the FPU bit of `OPTIMIZE` set, `.d` SHALL give "Not supported: OPTIMIZE 2 (FPU)" at the `OPTIMIZE` line, as every use of the FPU bit does, until FPU code generation exists (phase G6). No FFP stop-gap SHALL replace it.

#### Scenario: OPTIMIZE 6 with .d
- **WHEN** a program with `OPTIMIZE 6` declares `a.d`
- **THEN** the compile fails with that error and no executable is written
