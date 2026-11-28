# Delta: compiler/warnings

## ADDED Requirements

### Requirement: The type exceptions warn
Each place where a type exception of `compiler/expression-types` gives another type or conversion than the standard rules SHALL give that exception's warning, also where the value is the same, in both modes: `divfrac` (the whole-division exception), `divquick` (a whole division beside a Quick), `floattop` (the top operation under a Float destination), `shiftfrac` (the shift exception), `shiftwrap` (the left-shift wrap). The standard rules SHALL NOT warn.

#### Scenario: A whole division under a Quick destination
- **WHEN** a source evaluates `x.q=w/9` with `w.w=200` and `y.q=4/2`
- **THEN** each line gives the warning `divfrac`, and the executable is the same as with the warning disabled

#### Scenario: A typed parameter and no destination
- **WHEN** a source with `Statement pq{x.q}` calls `pq{1/16}` and executes `NPrint q+w/2`
- **THEN** the first line gives `divfrac` and the second `divquick`

#### Scenario: No exception
- **WHEN** a source evaluates `r.l=w/9` and executes `NPrint w+v`
- **THEN** no warning is given

#### Scenario: A shift under a Quick destination
- **WHEN** a source evaluates `x.q=1 LSR 7` and `y.q=1 LSL 16`
- **THEN** the first line gives `shiftfrac` and the second `shiftwrap`

### Requirement: The texts of the type warnings
The type warnings SHALL have these texts: `divfrac` `Whole division keeps its fraction`, `divquick` `Whole division beside a Quick keeps its fraction`, `floattop` `Quick operation computed in Float`, `shiftfrac` `Whole shift computed with fraction bits`, `shiftwrap` `Left shift wraps instead of saturating`.

#### Scenario: The text on the console
- **WHEN** line 3 of `prog.ab3` is `x.q=1 LSL 16` and it is compiled with `blitz2-ab3 -s prog.ab3 -e RAM:prog`
- **THEN** the console shows `prog.ab3:3: warning: Left shift wraps instead of saturating [shiftwrap]`

### Requirement: A Float conversion that wraps warns
A conversion of a Float to a Long that the compiler can tell at compile time wraps beyond the Long range SHALL give the warning `longwrap` with the text `Float beyond the Long range wraps`; a conversion known only at run time SHALL NOT warn.

#### Scenario: A constant beyond the Long range
- **WHEN** a source evaluates `r.l=1e10` and `s.l=f` with a Float variable `f`
- **THEN** the first line gives `longwrap`, the second gives none

### Requirement: A Data.b value with a fraction warns
A `Data.b` value with a point whose fraction is not zero SHALL give the warning `datafrac` with the text `Fraction of a Data.b value discarded`.

#### Scenario: Data.b values
- **WHEN** a source contains `Data.b -1.5,1.0`
- **THEN** one warning `datafrac` is given, for -1.5

### Requirement: The number of type warnings shown
In the editor, the warnings `divfrac`, `divquick`, `floattop`, `shiftfrac`, `shiftwrap`, `longwrap` and `datafrac` SHALL be shown for at most 8 places per name and compile, followed by one message `NAME: N more` for that name. On the console every place SHALL be printed, as `SOURCE:LINE: warning: MESSAGE [NAME]`.

#### Scenario: Many places in the editor
- **WHEN** a source with 10 places of `divfrac` is compiled in the editor with warnings on
- **THEN** 8 requesters name the first 8 places and one says `divfrac: 2 more`

#### Scenario: Many places on the console
- **WHEN** the same source is compiled with `blitz2-ab3 -s prog.ab3 -e RAM:prog`
- **THEN** the console shows 10 warning lines, the executable is written and the return code is 5
