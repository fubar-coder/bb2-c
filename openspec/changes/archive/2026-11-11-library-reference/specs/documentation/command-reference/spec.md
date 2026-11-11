# Spec Delta

## Purpose

A reference of the Blitz Basic commands on the website, generated from
the library files the build makes, so that it always describes the
libraries as they are built.

## ADDED Requirements

### Requirement: Command data from the built library files
A host tool SHALL read the token table of a built library file and write
its commands as one JSON file: every named entry with its name, kind,
help text byte for byte, result type and forms with their argument
types; for an AmigaOS call its base library, offset and registers; for
the library its number, its object and the AmigaOS library it opens. A
broken library file SHALL make the tool fail with a message and write
no file.

#### Scenario: A command library
- **WHEN** the tool reads `windowslib.obj`
- **THEN** the JSON lists `Window` as a statement with three forms and its help text, and the object `Window` with its default maximum

#### Scenario: An AmigaOS call library
- **WHEN** the tool reads the deflibs library of dos.library
- **THEN** the JSON names `dos.library` and lists `Open_` with its help text `(name,accessMode)`, offset -30 and the registers d1, d2

#### Scenario: Broken input
- **WHEN** the token table of the input runs past the end of the code
- **THEN** the tool exits with an error message and leaves no output file

### Requirement: Committed data checked by the build
The JSON file of every library of acidlibs and deflibs of the default
variant and of every RWE userlib SHALL be committed under
`site/data/libraries/GROUP/NAME.json`. The build step that builds the
library file SHALL generate the JSON and fail when it differs from the
committed file, so that the library file counts as not built. A ctest
SHALL fail when a committed JSON file has no library. Each of these
checks SHALL have a test with deliberately broken input.

#### Scenario: Library declaration changed
- **WHEN** a help text in a library source changes and the library is rebuilt
- **THEN** the build step of the library fails and names the generated and the committed file, until the generated file is copied over the committed one

#### Scenario: Stale file
- **WHEN** a JSON file is committed for a library that the build does not make
- **THEN** the completeness test fails and names the file

### Requirement: One page per library
The website SHALL render the committed data with Hugo, without a step
outside Hugo: one page per command library, per AmigaOS library (the
deflibs libraries that share a base on one page) and per userlib, in
sections of the Libraries chapter. A page SHALL show every command with
its name, kind, help text as the original has it, result type and forms
with their argument types, and every AmigaOS call with its arguments,
registers and offset. The Libraries chapter SHALL link the API
reference.

#### Scenario: Looking up a command
- **WHEN** a reader opens the page of windowslib
- **THEN** `Window` is listed with its help text, its three forms with their argument types, and the page's table of contents leads to it

### Requirement: A slot for hand-written introductions
A library page SHALL take an optional hand-written description and
introduction from a data file of its own; without it, a generated
description is used.

#### Scenario: Introduction written later
- **WHEN** a data file with a description and an introduction for a library is added
- **THEN** its page shows the introduction above the commands and uses the description in lists and search, without a change to the templates
