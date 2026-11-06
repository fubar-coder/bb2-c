# ab3-compat/syntax-rules Specification

## Purpose

Defines the syntax rules `blitz2-ab3` applies in AB3 mode beyond Blitz
Basic 2's: those of AmiBlitz3's "new syntax" mode (`OPTIMIZE` value 4)
and those AmiBlitz3 v3.14.0 has on in every program.

## Requirements

### Requirement: New syntax - macro calls without braces
With new syntax on, a call `!name` of a macro whose body references parameters SHALL take its parameters without braces from after the name (`!mac a,b`); the first character after the name decides (name characters, `#`, `$`, `-`, `,` and `{` start parameters, anything else means none). With new syntax off, macro calls SHALL behave as in BB2.

#### Scenario: Parameters without braces
- **WHEN** new syntax is on and a macro `mac` printing its first parameter is called as `!mac 42`
- **THEN** the program prints 42

#### Scenario: New syntax off
- **WHEN** the same call is compiled without new syntax
- **THEN** the result is the same as with BB2's rules (empty parameters, ` 42` left in the line)

### Requirement: New syntax - labels without colon
With new syntax on, a name at the start of a statement followed by a token whose low byte is not in AmiBlitz3's `allowtab` (`LSL`, `LSR`, `ASL`, `ASR`, `MOD`, `NOT`, `BitTst`, `BitSet`, `BitClr`, `BitChg`) SHALL be a label definition; followed by one of those it SHALL be the BB2 assignment shortcut.

#### Scenario: Label without colon
- **WHEN** new syntax is on and a line is `loop NPrint "x"` and later `Goto loop` is reached once
- **THEN** `loop` is a label and the program prints `x` twice

#### Scenario: Shortcut kept
- **WHEN** new syntax is on and a line is `a LSL 2` with `a=1`
- **THEN** `a` becomes 4

### Requirement: New syntax - labels of another procedure
With new syntax on, a forward reference to a label from another procedure context SHALL be accepted instead of BB2's reference error.

#### Scenario: Reference across procedures
- **WHEN** new syntax is on and a program references a label defined in another procedure context as AmiBlitz3's tests do
- **THEN** it compiles as with AmiBlitz3

### Requirement: Line joining with @@
In AB3 mode `@@` at a parameter boundary (start of a parameter list, after a parameter, after `,`; also in macro text and definitions and in multiple assignments) SHALL join the next line, the rest of the line after `@@` being ignored; strings and comments SHALL not be affected. `@@` at other places of an expression, for example after an operator, SHALL join the next line as well (AmiBlitz3: syntax error; intended deviation). Inside an include, outside a `Macro` definition, only `,` followed by `@@` SHALL be allowed, other forms giving "Joining lines with @@ does not work in includes". Inside a `Macro` definition every form SHALL join, also in an include, as in AmiBlitz3. `@@` on a line with `End Macro` before it SHALL be the error "Joining lines with @@ does not work after End Macro" (AmiBlitz3: the `@@` is ignored; intended deviation).

#### Scenario: Tag list over lines
- **WHEN** a statement's parameters are split with `,@@` over three lines
- **THEN** it compiles as if written on one line

#### Scenario: @@ in a string
- **WHEN** a string literal contains `@@`
- **THEN** the string keeps the two characters

#### Scenario: Join after an operator
- **WHEN** a program has `b.l = 1 + @@` and `2` on the next line
- **THEN** it compiles as `b.l = 1 + 2` and `b` is 3

#### Scenario: Macro definition in an include
- **WHEN** an include defines a macro with the line `` If `1 > 0 Then `1 = 0 : @@ `` followed by `` `2 = 0 ``, and the main source includes it and calls the macro
- **THEN** it compiles, and the two lines are one line of the macro

#### Scenario: End Macro with @@
- **WHEN** a macro definition ends with the line `End Macro @@` followed by `NPrint "after"`
- **THEN** the compile fails with "Joining lines with @@ does not work after End Macro" at the `End Macro` line and no executable is written

### Requirement: Global SHARED
In AB3 mode `SHARED` outside a procedure SHALL make the variable shared with every procedure, instead of BB2's error.

#### Scenario: Global SHARED
- **WHEN** the main program has `SHARED count` and a procedure increments `count`
- **THEN** the main program sees the new value

### Requirement: Identifiers beginning with underscore
In AB3 mode a name at the start of an expression element that begins with `_` or another character AmiBlitz3 counts as a letter (`[`, `\`, `]`, `^`, `` ` ``) SHALL be accepted as in AmiBlitz3, without BB2's syntax error.

#### Scenario: Underscore variable in an expression
- **WHEN** an AB3-mode program has `_start.l = 5 : NPrint Bin$(_start)`
- **THEN** it compiles and prints `Bin$` of 5 (the 32 digits ending in `101`), as AmiBlitz3

### Requirement: Constants of the include mechanism
In AB3 mode `#__include` SHALL be 0 in the main source and 1 inside an include (0 again when the last include is closed); `#__debug` is defined by `compiler/debug-constant` (−1 with runtime error checking, 0 without). `@` SHALL be accepted only directly after `CNIF`: in that condition every `#name` is a "defined?" test giving 1 or 0, without an error for an undefined name; `@` elsewhere in a condition SHALL be "Illegal constant expression", as in AmiBlitz3.

#### Scenario: Include guard
- **WHEN** an include starts with `CNIF @#mylib_include=0` and defines `#mylib_include`, and is included twice with `XINCLUDE`
- **THEN** its code is compiled once and `CNIF #__include=0` blocks inside it are skipped

### Requirement: Code after :@ in a comment
In AB3 mode, the text after `:@` inside a comment SHALL be compiled as code, as in AmiBlitz3.

#### Scenario: Comment with :@
- **WHEN** a line is `; note :@b=1` and the program prints `b`
- **THEN** it prints 1

### Requirement: The + prefix
In AB3 mode a `+` at the start of a statement SHALL mark it as compiled without type check; since type checks are not performed yet (G8), the statement SHALL compile as without the `+`.

#### Scenario: + prefix
- **WHEN** a line is `+a.l = b.w`
- **THEN** it compiles as `a.l = b.w`

### Requirement: Labels written as .name
In AB3 mode `.name` at the start of a statement followed by a token SHALL be a label definition.

#### Scenario: .name label
- **WHEN** a line is `.start NPrint "go"` and `Goto start` jumps there
- **THEN** the program prints `go` again

### Requirement: A Function may be called as a statement
In AB3 mode a call `name{…}` of a `Function` at the start of a statement SHALL compile like a call of a `Statement`: the function is called with its parameters and its result is discarded, as in AmiBlitz3; in BB2 mode it SHALL stay the error "Illegal Procedure Call".

#### Scenario: Numeric function as a statement
- **WHEN** an AB3 program defines `Function.l myadd{a.l,b.l}` that prints its sum and returns it, and has the line `myadd{2,3}`
- **THEN** it compiles and prints 5

#### Scenario: String function as a statement
- **WHEN** an AB3 program calls a `Function.s` that prints a text as a statement, followed by `NPrint "next"`
- **THEN** it compiles, prints the text and then `next`, as AmiBlitz3's executable does

#### Scenario: BB2 mode
- **WHEN** a BB2-mode program has the line `myadd{2,3}` with `myadd` a `Function`
- **THEN** the compile fails with "Illegal Procedure Call", as with `blitz2`
