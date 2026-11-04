# Spec Delta

## Purpose

Defines how `blitz2-ab3` treats `Macro` definitions in AB3 mode where AmiBlitz3's rules differ from Blitz Basic 2's.

## ADDED Requirements

### Requirement: A one-line If inside a macro ends with its macro line
In AB3 mode, a line of a `Macro` definition that contains `Then` and no `End If` after it SHALL be stored with `: End If` appended, as AmiBlitz3 does. A one-line `If … Then` inside a macro body SHALL therefore cover only the rest of its own macro line, not the following lines of the body or the rest of the calling line. Lines joined with `@@` SHALL count as one macro line. A `Then` inside a string literal or a comment SHALL not count.

#### Scenario: Macro with a one-liner inside a loop of another macro
- **WHEN** an AB3 program defines macro `a` with the line `` If `1 > 0 Then `1 = 0 `` and macro `b` with `` While `1 > 0 `` / `` !a{`1} `` / `Wend`, and runs `x.l = 1 : !b{x} : NPrint x`
- **THEN** it compiles and prints `0`

#### Scenario: Lines after the one-liner are not conditional
- **WHEN** an AB3 program defines a macro with the lines `` If `1 > 0 Then `1 = 0 `` and `` `2 = `2 + 10 ``, and calls it with x = 0, y = 5
- **THEN** y is 15

#### Scenario: Line already closed
- **WHEN** a macro line reads `` If `1 > 0 Then `1 = 0 : End If ``
- **THEN** no second `End If` is appended and the program compiles

#### Scenario: Then joined over lines
- **WHEN** a macro has the line `` If `1 > 0 Then `1 = 0 : @@ `` followed by `` `2 = 0 ``
- **THEN** both assignments are conditional

### Requirement: A caller's one-line If covers the whole macro
In AB3 mode, `If c Then !m{…}` SHALL keep the whole expanded macro and the rest of the calling line conditional, as in AmiBlitz3 and Blitz Basic 2.

#### Scenario: A caller's Then covers the whole macro
- **WHEN** an AB3 program has `If c Then !m{}` with c = 0 and a macro `m` of three `NPrint` lines, followed by `NPrint "after"`
- **THEN** it prints only `after`

### Requirement: BB2 mode stores macro text as written
In BB2 mode a macro line with `Then` SHALL be stored without an added `End If`, as in Blitz Basic 2 (issue #967).

#### Scenario: BB2 mode
- **WHEN** the first scenario's program is compiled in BB2 mode
- **THEN** the compile fails with "Illegaly nested Condition", as in Blitz Basic 2
