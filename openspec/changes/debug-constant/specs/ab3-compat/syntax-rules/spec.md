# Spec Delta

## MODIFIED Requirements

### Requirement: Constants of the include mechanism
In AB3 mode `#__include` SHALL be 0 in the main source and 1 inside an include (0 again when the last include is closed); `#__debug` is defined by `compiler/debug-constant` (−1 with runtime error checking, 0 without). `@` SHALL be accepted only directly after `CNIF`: in that condition every `#name` is a "defined?" test giving 1 or 0, without an error for an undefined name; `@` elsewhere in a condition SHALL be "Illegal constant expression", as in AmiBlitz3.

#### Scenario: Include guard
- **WHEN** an include starts with `CNIF @#mylib_include=0` and defines `#mylib_include`, and is included twice with `XINCLUDE`
- **THEN** its code is compiled once and `CNIF #__include=0` blocks inside it are skipped
