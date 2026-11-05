# Spec Delta

## Purpose

Defines how `blitz2-ab3` compiles `Statement`s and `Function`s in AB3 mode where AmiBlitz3 differs from Blitz Basic 2: optional parameters, up to 10 parameters, `FAST` and NewType return types; BB2 mode keeps Blitz Basic 2's rules.

## ADDED Requirements

### Requirement: Optional parameters
In AB3 mode a parameter written with `@` in a procedure definition SHALL be optional, and so SHALL every parameter after it, also one without `@`, as in AmiBlitz3. This SHALL hold whatever `OPTIMIZE` sets. The `@` SHALL not be part of the parameter's name.

#### Scenario: Omitted optional argument
- **WHEN** an AB3 program defines `Statement s{x.l,@b.l}` and calls `s{1}`
- **THEN** it compiles and `b` is -1 inside the call

#### Scenario: Mandatory after optional
- **WHEN** an AB3 program defines `Statement s{@a.l,b.l}` and calls `s{}`, `s{1}` and `s{1,2}`
- **THEN** it compiles and prints `-1 -1`, `1 -1` and `1 2`

#### Scenario: Without new syntax
- **WHEN** an AB3 program with `OPTIMIZE 0` defines `Statement s{x.l,@b.l}` and calls `s{1}`
- **THEN** it compiles and `b` is -1

### Requirement: Defaults of omitted arguments
An omitted argument SHALL take the default of its parameter's type, as in AmiBlitz3 without FPU:

| Type | Default |
|---|---|
| byte, word, long | -1 |
| quick | -1.0 |
| float | -1.0 (FFP) |
| string | the empty string |
| pointer, including a NewType pointer | -1 |

#### Scenario: Defaults per type
- **WHEN** an AB3 program defines `Statement s{x.l,@b.b,@w.w,@l.l,@q.q,@f.f}` and calls `s{1}`
- **THEN** it prints `1 -1 -1 -1 -1 -1`

#### Scenario: Empty string
- **WHEN** an AB3 program defines `Statement s{x.l,@t.s}` and calls `s{1}`
- **THEN** inside the call `t` is the empty string and `Len(t)` is 0

#### Scenario: NewType pointer
- **WHEN** an AB3 program defines `Statement s{a.l,@*p.nt}` and calls `s{1}`
- **THEN** inside the call `*p` is -1

### Requirement: Which arguments may be omitted
Arguments SHALL be omitted only at the end of the argument list, in statement calls and function calls alike. A call SHALL fail with "Not enough parameters" when it ends before the first optional parameter. An empty argument in the middle or a trailing comma SHALL be a syntax error, and more arguments than parameters SHALL be an error.

#### Scenario: Missing mandatory argument
- **WHEN** an AB3 program defines `Statement s{a.l,b.l,@c.l}` and calls `s{1}`
- **THEN** the compile fails with "Not enough parameters" at the call

#### Scenario: Gap in the middle
- **WHEN** an AB3 program calls `s{1,,3}` or `s{1,}`
- **THEN** the compile fails with a syntax error

### Requirement: Empty call of an only-optional procedure
When every parameter of a procedure is optional, `name{}` SHALL give each one its default, also for procedures with 7 or more parameters (AmiBlitz3 refuses those: an intended deviation).

#### Scenario: Only-optional procedure called empty
- **WHEN** an AB3 program defines `Statement s{@a.l,@t.s}` and `Function.l f{@a.l,@t.s}`, and calls `s{}` and `NPrint f{}`
- **THEN** both get `a` = -1 and `t` empty

#### Scenario: More than six only-optional parameters
- **WHEN** an AB3 program defines a statement with eight optional `.l` parameters and calls it with `{}`
- **THEN** it compiles and every parameter is -1

### Requirement: Up to 10 parameters
In AB3 mode a procedure SHALL have up to 10 parameters. An 11th parameter SHALL give "Too many parameters" at the definition. Parameters 7 to 10 SHALL be passed on the stack, so that a call with 7 or more parameters inside an argument of another such call does not change the outer call's values. AmiBlitz3 passes them through four static longs and loses the outer values, so this is an intended deviation. Byte and word parameters 7 to 10 SHALL arrive with their value as in registers.

#### Scenario: Ten parameters
- **WHEN** an AB3 program defines a `Statement` and a `Function.l` with 10 parameters of types `.l`, `.w` and `.b` and calls them with 1 to 10
- **THEN** the statement prints `1 2 3 4 5 6 7 8 9 10` and the function returns 55

#### Scenario: Nested call
- **WHEN** an AB3 program calls `f{1,2,3,4,5,6,7,g{1,2,3,4,5,6,100}}`, where `f` prints its parameters 7 and 8 and `g` has 7 parameters and returns its 7th
- **THEN** it prints `p7=7 p8=100`

#### Scenario: Eleven parameters
- **WHEN** an AB3 program defines a procedure with 11 parameters
- **THEN** the compile fails with "Too many parameters" at the definition

### Requirement: Strings and optional parameters beyond the sixth
String parameters and optional parameters SHALL work at every position up to the 10th. A string parameter SHALL take an 8-byte stack slot at every position, as AmiBlitz3 lays it out. For a string in parameters 1 to 6, an omitted string SHALL generate AmiBlitz3's code.

#### Scenario: Optional string as ninth parameter
- **WHEN** an AB3 program defines `Statement s{a.l,b.l,c.l,d.l,e.l,f.l,@g.l,@h.w,@t.s}` and calls it with 6, 7 and 9 arguments (the last one `"str"`)
- **THEN** it prints `1 6 -1 -1 [] 0`, `1 6 7 -1 [] 0` and `1 6 7 8 [str] 3`

### Requirement: FAST procedures
In AB3 mode the word `FAST` between `Statement` and the name, or between `Function[.type]` and the name, SHALL mark the procedure as FAST: its locals SHALL not be cleared at entry and keep what the stack holds, as in AmiBlitz3. A FAST procedure that needs local memory (string literals or variables, NewType variables, arrays) SHALL give "FAST directive is not allowed if local strings or newtypes are used". `FAST` SHALL only be recognised in upper case and followed by a space.

#### Scenario: FAST function and statement
- **WHEN** an AB3 program defines `Function.l FAST ff{a.l,b.l}` returning `a+b` through a local and `Statement FAST sf{a.l}` printing `a`, and calls `ff{2,3}` and `sf{7}`
- **THEN** it prints `5` and `7`

#### Scenario: Locals not cleared
- **WHEN** a FAST statement sets a local `c` only when its parameter is 1, prints `c`, and is called with 1 and then with 2
- **THEN** it prints `42` twice, where a normal statement prints 42 and 0

#### Scenario: FAST with a local string
- **WHEN** a FAST procedure has a local string variable
- **THEN** the compile fails with "FAST directive is not allowed if local strings or newtypes are used"

### Requirement: NewType return type
In AB3 mode `Function.nt` with a NewType `nt` SHALL compile. The function SHALL return the address it is given as a long. The function result SHALL be accepted by a pointer assignment to `*p.nt`. The check of the `Function Return` expression against the declared NewType SHALL not be made, so a return that AmiBlitz3 refuses compiles. This is an intended deviation until G8.

#### Scenario: Returning a NewType pointer
- **WHEN** an AB3 program defines `Function.nt mk{}` with `Function Return &one` (`one` a shared `.nt` variable), then `*p.nt = mk{} : *p\x = 5 : NPrint one\x`
- **THEN** it prints `5`

### Requirement: BB2 mode keeps Blitz Basic 2's procedures
In BB2 mode `@` in a parameter SHALL stay Blitz Basic 2's pointer flag. A procedure SHALL have at most 6 parameters. `FAST` SHALL not be a keyword. `Function.nt` SHALL be refused as in Blitz Basic 2. Every BB2-mode executable SHALL stay byte-identical to `blitz2`'s.

#### Scenario: @ in BB2 mode
- **WHEN** a BB2-mode program defines `Statement s{x.l,@b.l}` and calls `s{1}`
- **THEN** the compile fails with "Not Enough Parameters" as with `blitz2`

#### Scenario: Seven parameters in BB2 mode
- **WHEN** a BB2-mode program defines a procedure with 7 parameters
- **THEN** the compile fails with "Too many parameters" as with `blitz2`
