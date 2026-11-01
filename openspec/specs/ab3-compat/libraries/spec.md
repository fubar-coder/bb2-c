# ab3-compat/libraries Specification

## Purpose

Defines which libraries, tokens and residents an AB3-mode program is
compiled with, so that AmiBlitz3's own binary library files can be used
unchanged as AmiBlitz3 v3.14.0 ships them.

## Requirements

### Requirement: AmiBlitz3's library files are loaded in AB3 mode
In AB3 mode `blitz2-ab3` SHALL load `baseLibs`, `utilLibs`, `OsLibs` and `csLibs` from `Blitz3:BlitzLibs/` in this order, unchanged as shipped by AmiBlitz3 v3.14.0, instead of BB2's `acidlibs` and `deflibs`.

#### Scenario: Program using commands only AB3 has
- **WHEN** an AB3-mode program uses a command that exists only in AmiBlitz3's libraries (e.g. `Trim$`)
- **THEN** it compiles and the program runs with the result AmiBlitz3's library gives

#### Scenario: A library file is missing
- **WHEN** one of the four files is missing in `Blitz3:BlitzLibs/`
- **THEN** the compile stops with an error naming the file

### Requirement: AmiBlitz3's userlibs are loaded in AB3 mode
In AB3 mode every library file in the flat directory `Blitz3:BlitzLibs/UserLibs/` SHALL be loaded after the four BigLibs; a userlib with the number of an already loaded library SHALL replace it, as in AmiBlitz3.

#### Scenario: Userlib replacing a library
- **WHEN** `UserLibs/` holds a library file with the number of a library in `baseLibs`
- **THEN** the program is compiled with the userlib's commands for that number

### Requirement: AB3's token table is used in AB3 mode
In AB3 mode sources SHALL be tokenised and compiled with AmiBlitz3 v3.14.0's compiler tokens (its 218 instructions and directives, including the tokens that differ from BB2's) followed by the tokens of the loaded AB3 libraries; the resulting token list SHALL equal AmiBlitz3's own for the same libraries.

#### Scenario: Token list equals AmiBlitz3's
- **WHEN** the token list of `blitz2-ab3` in AB3 mode with v3.14.0's libraries is compared with the token list of AmiBlitz3 v3.14.0's compiler with the same libraries
- **THEN** both lists have the same names at the same token numbers

### Requirement: The float power uses AB3's math library
In AB3 mode the operator `^` on floating point values SHALL call AmiBlitz3's math library (library 44) instead of BB2's library 192.

#### Scenario: Float power
- **WHEN** an AB3-mode program prints `a.f^10` with `a.f = 2`
- **THEN** it compiles and prints 1024

### Requirement: Residents of the embedded header are found in AB3's residents directory
In AB3 mode a resident named in the embedded header without a path SHALL be loaded from `Blitz3:BlitzLibs/Residents/`; a name with a device or directory part SHALL be used as given.

#### Scenario: Header resident by name
- **WHEN** an AB3 source's header lists the resident `all.res` and the program uses `ExecBase`
- **THEN** the resident is loaded from `Blitz3:BlitzLibs/Residents/all.res` and the program compiles

#### Scenario: Missing resident
- **WHEN** the header lists a resident that does not exist
- **THEN** the compile stops with the compiler's resident error naming it
