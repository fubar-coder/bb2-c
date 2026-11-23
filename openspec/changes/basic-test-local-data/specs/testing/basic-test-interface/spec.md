## ADDED Requirements

### Requirement: Entries of a started program
Besides the entries of "Entries of the result file", the result file SHALL have `run` (`K`; a program is started) and `runend` (`K` and the return code as a long value, or `K` and `-` where the system gives none), each belonging to the block of the last `block` entry before it.

#### Scenario: A started program that returns
- **WHEN** `!RunProgram{K,c$}` starts a program that ends with return code 20
- **THEN** the file holds `run` `K` and then `runend` `K` `$00000014`

#### Scenario: A started program that does not return
- **WHEN** the started program hangs and the session ends
- **THEN** the file holds `run` `K` without a `runend` `K`

#### Scenario: A runend with a malformed code
- **WHEN** the reader gets `runend` `K` `20`
- **THEN** it refuses the value as malformed, naming the line

### Requirement: The run macro of a test source
A test source SHALL start a program with `*r.TLRun = !RunProgram{COMMAND$}`, which MAY take a name as an extra last argument, a string literal. The preprocessor SHALL number it like a check, also inside an expression, and write `!RunProgram{K,COMMAND$}`; the macro text SHALL call the function `TLRunProgram(K,COMMAND$)`, which gives the address of the run's `TLRun`.

#### Scenario: A named run
- **WHEN** a case holds `*r.TLRun=!RunProgram{"@TESTDIR@/child <NIL:","child"}`
- **THEN** the preprocessed form is `*r.TLRun=!RunProgram{K,"CORPUS:ID.test/child <NIL:"}` and the table names `K` with the name `child`

### Requirement: The run routine
`TLRunProgram(K,command$)` SHALL write `run K`, start the command as a shell command line (arguments and redirections as a shell takes them) with its output to the file `CORPUS:out/run-K.out`, wait for its end, fill the library's `TLRun` from the return code and that file, write `runend K` with the return code, and give the address of the `TLRun`.

#### Scenario: Output of a started program
- **WHEN** `TLRunProgram(4,"CORPUS:ID.test/child")` starts a child that prints `exec 37` and returns 0
- **THEN** `CORPUS:out/run-4.out` holds `exec 37`, the result file `run 4` and `runend 4 $00000000`, and the `TLRun` it gives has `rc` 0 and `printed` `"exec 37"` followed by LF

### Requirement: The NewType of a run
The macro text SHALL declare `NEWTYPE .TLRun` with the fields `rc.l` (the return code, 0 if unknown), `rcknown.w` (-1 if the system gave the return code, else 0), `cut.w` (-1 if `printed` is cut, else 0), `size.l` (the bytes the program wrote) and `printed.s` (the first 1024 bytes of what it wrote, as they are). No field name SHALL be a token of a library set.

#### Scenario: A long output
- **WHEN** a started program writes 3000 bytes
- **THEN** `printed` holds the first 1024 of them, `cut` is -1 and `size` is 3000

#### Scenario: A field name that is a token
- **WHEN** a library set gains a token `Size`
- **THEN** the build check of the macro text fails, naming the field
