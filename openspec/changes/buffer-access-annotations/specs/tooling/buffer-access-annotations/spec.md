# tooling/buffer-access-annotations Specification

## Purpose
How a function declares which number is the length of which pointer
argument, so that the signature documents the contract, the compiler
checks calls whose sizes it knows, and the host sanitizer configuration
checks the sizes at run time.

## ADDED Requirements

### Requirement: One macro declares a buffer and its length
A single freestanding header SHALL provide `BL_ACCESS(mode, ptr_index,
size_index)` for both compilation worlds (m68k gcc and the host
compiler), with modes `read_only`, `write_only`, `read_write` and `none`.
It SHALL expand to the `access` attribute where the compiler has it
(`__has_attribute(access)`) and to nothing otherwise, and SHALL NOT
include any libc header.

#### Scenario: Both worlds include the header
- **WHEN** an Amiga-side file and a host file include the header
- **THEN** both compile without warnings and without extra includes

#### Scenario: Compiler without the attribute
- **WHEN** the compiler does not support `access`
- **THEN** the macro expands to nothing and the code compiles unchanged

### Requirement: Every real buffer+length pair in the public API is annotated
Every function declared in a public header of `shared/` or `tools/lib/`
that takes a pointer together with a number that is the size or capacity
of that buffer SHALL carry `BL_ACCESS` on the declaration the callers see.
The mode SHALL match what the function does (`read_only` for input,
`write_only` for output it only fills, `read_write` otherwise). A fixed
size SHALL stay an array bound in the parameter. A number next to a
pointer that is not its length SHALL NOT be annotated, and a short
comment SHALL say so where a reader could mistake it for a length.

#### Scenario: Number that is not a length
- **WHEN** a parameter next to a pointer is a chunk length, a value to
  format, a code size, an index or an element size
- **THEN** it carries no `BL_ACCESS` and a comment states what it is

#### Scenario: Limit belongs to the other pointer
- **WHEN** a function takes a destination with a capacity and a
  0-terminated source
- **THEN** only the destination is annotated

#### Scenario: Output mode
- **WHEN** a function only fills a buffer the caller may leave
  uninitialised
- **THEN** the annotation is `write_only`, so that no
  `-Wmaybe-uninitialized` follows for a fresh allocation

### Requirement: New code annotates its buffer+length pairs
New code with a buffer+length pair SHALL be annotated, and `CLAUDE.md`
and `docs/development.md` SHALL state this rule and the cases where an
annotation is impossible or misleading.

#### Scenario: Rule documented
- **WHEN** a developer reads `CLAUDE.md` or `docs/development.md`
- **THEN** the rule, the macro's header and the list of cases that stay
  unannotated are found there

### Requirement: Wrongly sized calls are diagnosed at compile time
A call whose buffer size or length is known at compile time and violates
the annotation SHALL produce a compiler diagnostic in the build's
warning configuration, and the repository's builds (Amiga, host, host
with `HOST_SANITIZE`) SHALL stay free of warnings. A test with
deliberately broken input SHALL show that the diagnostic is produced.

#### Scenario: Oversized length at a call site
- **WHEN** a snippet calls an annotated function with a length larger
  than the array it passes
- **THEN** compiling the snippet reports the warning and the ctest
  expecting it passes

#### Scenario: Tests that pass bad arguments on purpose
- **WHEN** `test_tokenizer.cpp` and `test_xtra_ab3_binary.cpp` are built
- **THEN** no warning appears, and they still check the refusal of the
  bad arguments

### Requirement: The host sanitizer configuration checks sizes at run time
The `HOST_SANITIZE` configuration SHALL add `_FORTIFY_SOURCE=3` to the
host side only, and all host tests SHALL pass with it.

#### Scenario: Sanitizer build
- **WHEN** the host side is built with `-DHOST_SANITIZE=ON`
- **THEN** it builds without warnings and the host tests pass

#### Scenario: Amiga side unaffected
- **WHEN** the Amiga build runs
- **THEN** it uses no `_FORTIFY_SOURCE` and its results are
  byte-identical to the previous commit's (`prev_results`)

### Requirement: Hardenings found by the probe
The 10-byte copy across fields in `programs/blitz2/src/tokens1.c` SHALL be
guarded by a `static_assert` on the field offsets, and `put_value` in
`tools/lib/teststats/statline.c` SHALL NOT write to its destination when
the size is 0.

#### Scenario: Zero-size destination
- **WHEN** `put_value` is called with size 0
- **THEN** the destination is not written

#### Scenario: Field order changes
- **WHEN** the layout of the two structures no longer matches the copy
- **THEN** the build fails at the `static_assert`
