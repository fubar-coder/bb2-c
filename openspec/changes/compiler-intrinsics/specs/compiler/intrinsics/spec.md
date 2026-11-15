# compiler/intrinsics Specification

## Purpose
Says when blitz2 and blitz2-ab3 compile a library function as inline code
instead of a call, and that the inline code gives the same value, leaves
the same registers and keeps the same error checks as the call.

## ADDED Requirements

### Requirement: Int of a Quick is compiled inline
When the argument of `Int` has the static type Quick (a variable, an array
element, a field, a function result or any expression of type Quick), the
compiler SHALL emit the integer part inline and SHALL NOT emit the type
word push or the library call. The value SHALL be the integer part
truncated towards zero as a Long, bit for bit what the library returns:
`Int(1.75)` is 1, `Int(-1.75)` is -1, `Int(-0.5)` is 0, `Int(-32768.0)` is
-32768, `Int(32767.99998)` is 32767.

#### Scenario: Positive and negative Quick
- **WHEN** a program prints `Int(q)` for q = 1.75, -1.75, -0.5, 0, 32767.99998 and -32768.0
- **THEN** it prints 1, -1, 0, 0, 32767 and -32768

#### Scenario: No library call
- **WHEN** a program has `c=Int(q1)` with `q1` a Quick variable
- **THEN** the compiled code holds no call of the `Int` library entry and no push of a type word for it

#### Scenario: Sweep equals the expected value
- **WHEN** `Int` of a fixed sweep of Quick values (every integer part at three fractions, plus the boundary values) is compared with the value computed from the Long bits of the Quick with integer arithmetic (magnitude shifted right by 16, sign restored)
- **THEN** every result is equal

### Requirement: Int keeps its error check for other types
In the runtime-error-checking mode the compiler SHALL keep the `Int`
check for every argument that is not a Quick. For a Quick argument the
check cannot fail and SHALL NOT be emitted.

#### Scenario: Quick argument with runtime error checking
- **WHEN** a program with runtime error checking on has `c=Int(q1)` for a Quick `q1`
- **THEN** the result is as above and the compiled code holds no call of `intchk`

#### Scenario: String argument with runtime error checking
- **WHEN** a program with runtime error checking on calls `Int` with a string argument
- **THEN** it stops with the runtime error "Illegal Type", as before

### Requirement: Int does not use the intrinsic for other types
An argument of type Byte, Word, Long or Float SHALL be compiled as before,
through the library.

#### Scenario: Float argument
- **WHEN** a program has `c=Int(f)` with `f` a Float
- **THEN** the compiled code calls the library `Int` and the value is the truncated Float

### Requirement: Frac of a Quick assigned as a Quick is compiled inline
When the argument of `Frac` has the static type Quick and the call is
the whole right-hand side of an assignment to a Quick (the compiler is
asked for a Quick and nothing follows the closing parenthesis in the
expression), the compiler SHALL emit the fraction inline and SHALL NOT
make a Float, call the library or convert back. The Quick result SHALL
be the exact fraction of the argument with the sign of the argument: the
low 16 bits of its magnitude, negated for a negative argument. This is
the same value that `Frac` returns as a Float after #1064 and the
assignment converts back to a Quick.

#### Scenario: Positive and negative Quick
- **WHEN** a program has `q2=Frac(q1)` for q1 = 1.25, -1.25, 0.5, -0.5, 3.0, -3.0 and 30000.0001
- **THEN** q2 is 0.25, -0.25, 0.5, -0.5, 0, 0 and the fraction word of 30000.0001 (the low fraction bits are kept)

#### Scenario: Equal to the library's result
- **WHEN** `q2=Frac(q1)` is compiled as the intrinsic and, for comparison, as `f=Frac(q1) : q2=f` (a Float target, so through the library) for a sweep of Quick values
- **THEN** q2 is the same Quick for every value

#### Scenario: No Float and no call
- **WHEN** a program has `q2=Frac(q1)` with `q1` and `q2` Quick
- **THEN** the compiled code holds neither a call of `qutofl` or `fltoqu` nor a call of the `Frac` library entry

### Requirement: Frac does not use the intrinsic outside its conditions
In every other case the compiler SHALL call the library as before: an
argument of type Float, Long, Word or Byte; a Float or Long target; a call
that is part of a larger expression (`q2=Frac(q1)*2`, `q2=1+Frac(q1)`); a
call as an argument of another function; a call in a condition. The
values are those of the library.

#### Scenario: Float target
- **WHEN** a program has `f=Frac(q1)` with `f` a Float and `q1` a Quick
- **THEN** the compiled code calls the library, and f holds the exact fraction as a Float

#### Scenario: Larger expression
- **WHEN** a program has `q2=Frac(q1)*2.5`
- **THEN** the compiled code calls the library

#### Scenario: Float argument
- **WHEN** a program has `q2=Frac(f)` with `f` a Float
- **THEN** the compiled code calls the library

### Requirement: Len and Asc of a plain string variable read the header
When the argument of `Len` or `Asc` is a plain string variable (a global
or local variable, with no index, no field, no function and no operator),
the compiler SHALL read the length, or the first character, from the
variable's memory header and SHALL NOT copy the string into the string
work area. `Len` returns the length (0 for a variable without memory).
`Asc` returns the code of the first character (0 to 255) and -1 for an
empty string, as the library does. a3 SHALL be the same after the call as
before it.

#### Scenario: Len of a variable
- **WHEN** a program has `a$="hello"`, `c=Len(a$)`
- **THEN** c is 5 and the compiled code makes no string copy for the call

#### Scenario: Empty and unset variable
- **WHEN** `a$` was never assigned, or is ""
- **THEN** `Len(a$)` is 0 and `Asc(a$)` is -1

#### Scenario: a3 unchanged
- **WHEN** a string expression `b$=a$+Chr$(Len(a$))` is evaluated
- **THEN** `b$` holds the same characters as before this change

### Requirement: Len and Asc do not use the intrinsic for other arguments
An expression (`Len(a$+b$)`), a function result (`Len(Left$(a$,2))`), an
array element, a literal and a field SHALL be compiled as before.

#### Scenario: Expression argument
- **WHEN** a program has `c=Len(a$+b$)`
- **THEN** the string is built in the work area and the library is called, as before

#### Scenario: Array element
- **WHEN** a program has `c=Len(s$(3))`
- **THEN** the compiled code is the same as before this change

### Requirement: The intrinsics apply in both modes and change no result
Every requirement above SHALL hold in the same way with runtime error
checking on and off, and in the BB2 and the AB3 mode of the compiler. A
program that uses an intrinsic SHALL print and write the same as with the
library call; only the executable differs.

#### Scenario: Corpus
- **WHEN** the corpus programs are compiled and run
- **THEN** their outputs equal the recorded ones; the executables of programs that use an intrinsic differ from the `prev` build (named by `PREV_INTENDED`), all others are byte-identical
