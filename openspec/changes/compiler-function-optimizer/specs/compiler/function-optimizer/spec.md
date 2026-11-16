# Spec Delta

## Purpose
Defines which procedures blitz2 and blitz2-ab3 leave out of a program compiled as small code, which ones they always keep because removing them could be observed, and when the optimizer runs.

## ADDED Requirements

### Requirement: Unreachable procedures are left out of small code
When the compiler makes small code (the option "Make Smallest", which "Create Executable" always uses), it SHALL leave out every Statement and Function that cannot be reached from the main program through procedure calls, in both modes. Procedures reachable only from other unreachable procedures, and recursive procedures that nothing reachable calls, SHALL be left out too.

#### Scenario: A dead call chain
- **WHEN** the main program calls `a{}`, and `b{}` calls `c{}` but nothing reachable calls `b{}`
- **THEN** the executable contains `a` and neither `b` nor `c`

#### Scenario: Dead recursion
- **WHEN** a Function calls itself and nothing reachable calls it
- **THEN** the executable does not contain it

#### Scenario: Includes
- **WHEN** a program includes a file with 50 procedures and calls 3 of them
- **THEN** the executable contains those 3 and the procedures they call

### Requirement: The optimizer runs only with small code
The compiler SHALL apply the optimizer exactly when it makes small code today, and SHALL have no other switch for it. A run from the editor without "Make Smallest" SHALL keep every procedure.

#### Scenario: Debug output from the direct mode
- **WHEN** a program with a never-called procedure `dbg{}` is run from the editor without "Make Smallest" and the debugger's direct mode calls `dbg{}`
- **THEN** the call works

#### Scenario: Create Executable
- **WHEN** the same program is saved with "Create Executable"
- **THEN** `dbg{}` is not in the executable

#### Scenario: The AB3 header key
- **WHEN** an AB3 source has `FuncOptimize = 0` in its header and is compiled as small code
- **THEN** the unreachable procedures are left out all the same

### Requirement: Observable procedures are kept
The compiler SHALL keep a procedure, reachable or not, when leaving it out could change what the program does or whether it compiles. That is a procedure that:
- contains `Data`;
- defines a constant, NewType or macro;
- contains a text-level directive (an include, `INCBIN`, conditional compilation, `DEFTYPE`, or a `Dim` of a global array);
- contains a label referenced from another context, by BASIC code or by inline assembler.

#### Scenario: Data in an unused procedure
- **WHEN** an unused Statement contains `Data.w 111` and the main program reads the first Data item
- **THEN** the program prints 111, as without the optimizer (AmiBlitz3 prints the next Data item instead)

#### Scenario: A constant defined in an unused procedure
- **WHEN** an unused Statement defines `#c=5` and the main program uses `#c`
- **THEN** the program compiles and uses 5

#### Scenario: A label used by inline assembler
- **WHEN** the main program's inline assembler does `JSR lab2` and `lab2` is a label in an unused procedure
- **THEN** the program compiles and runs as without the optimizer

### Requirement: Libraries stay all or nothing
A library SHALL be linked when the main program or a kept procedure uses it, and not otherwise, as the compiler decides it today. The optimizer SHALL NOT remove single commands from a library.

#### Scenario: A library used only in a removed procedure
- **WHEN** only an unreachable procedure uses a library command
- **THEN** the library is not linked, as in a program that never used it

### Requirement: Programs behave the same
A program compiled with the optimizer SHALL give the same output, results and runtime errors as without it. Only the size and the code addresses may change.

#### Scenario: The corpus
- **WHEN** the corpus programs are compiled as small code with the optimizer
- **THEN** every output is the same as before, and only the executables differ
