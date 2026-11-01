# ab3-compat/type-rules Specification

## Purpose

Defines the type rules `blitz2-ab3` applies in AB3 mode that AmiBlitz3
v3.14.0 applies to every program, independent of `SYNTAX`; the
`SYNTAX`-dependent checks are added later (phase G8).

## Requirements

### Requirement: A pointer takes no basic value other than long
In AB3 mode an assignment to a pointer variable (`*name = …`, also `*name.Type = …`) SHALL stop the compile with "Can't convert types" when the right side's last element (the last variable, address or function the expression reads, as in AmiBlitz3) is of a basic type other than long (byte, word, quick, float, string), as in AmiBlitz3, whether `SYNTAX` is set or not.

#### Scenario: Undeclared quick variable
- **WHEN** an AB3 program has `*exec.ExecBase = execbase` with `execbase` not declared (type `.q`)
- **THEN** the compile fails with "Can't convert types" at that line and no executable is written

#### Scenario: Word value
- **WHEN** an AB3 program has `w.w = 4 : *p.l = w`
- **THEN** the compile fails with "Can't convert types"

### Requirement: Accepted right sides of a pointer assignment
In AB3 mode a pointer assignment SHALL compile as before when the right side's last element (as AmiBlitz3 judges it: the last variable, address or function the expression reads) is a pointer, a long, a NewType value, or a constant or library function without a basic non-long type; a `Function` without a declared type returns a quick and is refused, as in AmiBlitz3. NewType against NewType is not checked until `SYNTAX 4` checks exist; casts come with G5.

#### Scenario: Long and function
- **WHEN** an AB3 program has `*exec.ExecBase = Peek.l(4)` and `*e2.ExecBase = ExecBase`
- **THEN** it compiles and both pointers hold exec's base

#### Scenario: Pointer and constant
- **WHEN** an AB3 program has `*a.l = 0` and `*b.l = *a`
- **THEN** it compiles

### Requirement: Other assignments are converted as in BB2
In AB3 mode assignments whose left side is not a pointer SHALL convert their value as in BB2, and in BB2 mode nothing SHALL change.

#### Scenario: Quick into word
- **WHEN** an AB3 program has `w.w = 2.5`
- **THEN** it compiles and `w` holds the value BB2's conversion gives

#### Scenario: BB2 mode
- **WHEN** a BB2-mode program has `*p.l = w` with `w.w`
- **THEN** it compiles as with `blitz2`
