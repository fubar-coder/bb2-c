## MODIFIED Requirements

### Requirement: Rounding happens once per operation
A Quick OP Quick with `+`, `-`, `*`, `/` or a shift, or a Quick expression of these stored in a Quick, under a Quick type so far SHALL keep the fraction. `NOT`, `AND`, `OR` and `EOR` are not among them: they take the whole part of a Quick operand ("NOT, AND, OR and EOR under a Quick type take the whole part"). Under a whole-number type so far a chain SHALL be rounded after each operation, not computed exactly: `r.l=l+q+p` with `l=10`, `q=1.5`, `p=2.75` gives 13 (exactly 14).

#### Scenario: Quick destination keeps the fraction
- **WHEN** a program evaluates `t.q=w+v+q` with `w=3`, `v=4`, `q=1.5`
- **THEN** `t` is 8.5

#### Scenario: Chain
- **WHEN** a program evaluates `r.l=l+q+p` with `l=10`, `q=1.5`, `p=2.75`
- **THEN** `r` is 13

## ADDED Requirements

### Requirement: NOT, AND, OR and EOR under a Quick type take the whole part
`NOT`, `AND`, `OR`, `EOR`, `&` and `|` under a Quick type so far (a Quick or default-typed variable as an operand, a Quick destination or parameter) SHALL take the whole part of a Quick operand, its 16 fraction bits dropped and rounded down (`-1.5` is `-2`), and give a Quick. The shifts SHALL keep the 16.16 value and its fraction. Whole operands give the same values as before.

#### Scenario: Toggle of a default-typed flag
- **WHEN** a program evaluates `h=True : h=NOT h`, `h=-1 : h=NOT h`, `h=(l=5) : h=NOT h`, `h=On : h=NOT h` and `h=0 : h=NOT h`
- **THEN** it gets 0, 0, 0, 0 and -1

#### Scenario: NOT
- **WHEN** a program evaluates `h.q=NOT x` with `x=3`, `h=NOT 3`, `h=NOT 1.5`, `h=NOT -1.5` and `h=NOT q` with `q=1.5`
- **THEN** it gets -4, -4, -2, 1 and -2

#### Scenario: AND, OR and EOR of Quick operands
- **WHEN** a program evaluates `q AND p`, `q OR p`, `q EOR w`, `p & q` and `q | p` with `q=1.5`, `p=2.5`, `w.w=3`
- **THEN** it gets 0, 3, 2, 0 and 3

#### Scenario: Negative Quick operand and whole masks
- **WHEN** a program evaluates `q AND w` with `q=-1.5`, `w=3`, `z AND 6` with `z.q=13` and `y OR 8` with `y=7`
- **THEN** it gets 2, 4 and 15

#### Scenario: Truth test on a fraction
- **WHEN** a program evaluates `If h OR g` with `g=0` and `If h AND g` with `h=0.5`, `g=0.5`
- **THEN** `If h OR g` is false for `h=0.5` and true for `h=-0.5`, and `If h AND g` is false

#### Scenario: Shifts keep the fraction
- **WHEN** a program evaluates `x LSR 1` with `x=5`, `q LSL 1` and `q ASR 1` with `q=1.5` under a Quick destination
- **THEN** it gets 2.5, 3 and 0.75

### Requirement: A negative whole constant under a Quick type is the Quick of its value
A whole-number constant read under a Quick type so far (`On`, a `#` constant, a number in a sum or product) SHALL be the Quick whose integer part is the value and whose fraction is 0, for a negative value as for a positive one. A number outside -32768..32767 SHALL stay a Long. A minus sign before a literal or constant is not affected. A default-typed flag set to `On` and toggled with `NOT` SHALL give the same truth values as before.

#### Scenario: Negative constants
- **WHEN** a program evaluates `q.q=On`, `q=#c` with `#c=-3`, `q=1+On`, `q=0.5+#c`, `q=#c*0.5` and `q=#e` with `#e=-32768`
- **THEN** it gets -1, -3, 0, -2.5, -1.5 and -32768

#### Scenario: Unchanged
- **WHEN** a program evaluates `q=-3`, `q=-#d` with `#d=3`, `q=#g` with `#g=32767` and `q=0.25+#d`
- **THEN** it gets -3, -3, 32767 and 3.25

#### Scenario: Flag toggle with On
- **WHEN** a program sets `flag=On` and then evaluates `flag=NOT flag`, once and twice
- **THEN** `If flag` is false after one `NOT` and true after two
