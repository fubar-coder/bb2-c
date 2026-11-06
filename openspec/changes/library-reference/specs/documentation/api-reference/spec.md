# Spec Delta

## Purpose

An API reference of the library headers, the library sources and the
shared code for C developers, built from the Doxygen comments and
served beside the website.

## ADDED Requirements

### Requirement: Doxygen configuration in the repository
A committed Doxyfile SHALL describe the API reference: the library
headers (`libs/include`, the headers of deflibs and the RWE userlibs),
the sources of acidlibs and the RWE userlibs, and `shared/`; not
`tools/` and not the programs. It SHALL treat the sources as text (no
Amiga toolchain), SHALL NOT list the declaration-table macros
(`BLITZ_NAME`, `BLITZ_ARGS`, ...) as functions, and SHALL NOT read the
Blitz notation `#name` in comments as a link. It SHALL NOT be part of
the CMake build.

#### Scenario: Table macros
- **WHEN** the reference of a library source is opened
- **THEN** it lists the library's routines with their comments, and no `BLITZ_NULLSUB`, `BLITZ_LIBFIN` or other table macro as a function

### Requirement: Built without warnings in the Pages workflow
The Pages workflow SHALL run Doxygen 1.18 in its official container
image, pinned by tag, with warnings treated as errors, into
`site/static/api/` before Hugo builds the site, so that the reference is
served under `api/` of the website. The generated tree SHALL NOT be
committed. A comment that makes Doxygen warn SHALL be corrected in the
comment; no result of the build SHALL change by it.

#### Scenario: A broken comment
- **WHEN** a routine's comment documents a parameter the routine does not have
- **THEN** the Doxygen run fails and the site is not published

#### Scenario: Comment fixes
- **WHEN** the comments are corrected
- **THEN** the programs, acidlibs, deflibs and every library file are byte-identical to the build before

### Requirement: The command name starts the brief
The `@brief` of a library routine that implements a Blitz command SHALL
start with the command's name. The convention SHALL be documented where
the Doxygen conventions of the repository are documented. Existing
comments are changed to it only where a Doxygen warning requires a
change anyway.

#### Scenario: A new library routine
- **WHEN** a routine implementing `WMouseY` is documented
- **THEN** its brief begins with `WMouseY`
