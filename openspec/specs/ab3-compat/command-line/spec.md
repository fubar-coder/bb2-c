# ab3-compat/command-line Specification

## Purpose

Lets a source be compiled from the shell with `blitz2-ab3`, with the
option names of AmiBlitz3's CLI mode, so that build scripts written for
AmiBlitz3 work and tests can compile without an editor.

## Requirements

### Requirement: Compile a source to an executable
`blitz2-ab3 -s SOURCE -e EXE` SHALL compile SOURCE and write the executable EXE without starting the editor; the long forms `-source` and `-exe` SHALL be accepted as well.

#### Scenario: Successful compile
- **WHEN** `blitz2-ab3 -s prog.ab3 -e RAM:prog` is run on a source without errors
- **THEN** `RAM:prog` is written, no editor window opens, and the return code is 0

#### Scenario: Long option names
- **WHEN** `blitz2-ab3 -source prog.ab3 -exe RAM:prog` is run
- **THEN** the result is the same as with `-s` and `-e`

### Requirement: Compile errors are reported on the console
A compile error SHALL be printed on the console with the file name, the line number and the compiler's message, the executable SHALL NOT be written, and the return code SHALL be 10 (ERROR).

#### Scenario: Source with an error
- **WHEN** a source with a syntax error in line 12 is compiled with `-s` and `-e`
- **THEN** the console shows the file name, line 12 and "Syntax Error", no executable exists afterwards, and the return code is 10

### Requirement: Release executables
With `-r` (or `-release`) the executable SHALL be created without debug information and without runtime error checking, as AmiBlitz3's release build.

#### Scenario: Release build
- **WHEN** a source is compiled with `-s`, `-e` and `-r`
- **THEN** the executable has no symbol or debug hunks and no runtime error checks

### Requirement: The mode can be forced
An option (`-mode ab3` or `-mode bb2`, an extension of this repository) SHALL force the mode of the program (see `ab3-compat/source-mode`).

#### Scenario: Forced BB2 mode for a .ab3 file
- **WHEN** a file named `prog.ab3` is compiled with `-mode bb2`
- **THEN** the program is compiled in BB2 mode

### Requirement: Unsupported options are refused
Options of AmiBlitz3's CLI that this change does not support (`-d`/`-debug`, `-run`, `-dump`, `-PED`, `-save` and the others) SHALL be refused with a message naming the option and return code 20 (FAIL), without compiling.

#### Scenario: Debug option
- **WHEN** `blitz2-ab3 -s prog.ab3 -e RAM:prog -d` is run
- **THEN** the console says that `-d` is not supported, nothing is compiled, and the return code is 20

### Requirement: Usage and argument errors
`-h`/`-help` SHALL print the supported options with return code 0; a missing `-s` or `-e` argument, a missing source file or an unknown option SHALL print a message and the usage with return code 20.

#### Scenario: Missing executable name
- **WHEN** `blitz2-ab3 -s prog.ab3` is run without `-e`
- **THEN** a message and the usage are printed and the return code is 20

#### Scenario: Missing source file
- **WHEN** `blitz2-ab3 -s nosuchfile.ab3 -e RAM:prog` is run
- **THEN** a message naming the file is printed and the return code is 20

### Requirement: The editor start stays unchanged
Started without `-s` (from the shell with at most a project name, or from the Workbench), `blitz2-ab3` SHALL start the editor exactly like `blitz2`.

#### Scenario: Start without options
- **WHEN** `blitz2-ab3` is started from the Workbench
- **THEN** it loads the editor and behaves as `blitz2` does
