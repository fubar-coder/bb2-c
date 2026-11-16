# Spec Delta

## ADDED Requirements

### Requirement: USELASTPATH and USEPATH2
In AB3 mode `USELASTPATH` and `USEPATH2` SHALL be accepted and SHALL change the include and resident search path exactly as AmiBlitz3 v3.14.0 does (to be fixed by the verification task: probes with an include in a directory the main source names, compared through `ab3_cli_ref`). They SHALL no longer give "Not supported: NAME". In BB2 mode they SHALL keep their BB2 meaning or error.

#### Scenario: USELASTPATH
- **WHEN** a program has `USELASTPATH` and includes a file found through the path AmiBlitz3 uses for it
- **THEN** it compiles and includes the same file as AmiBlitz3

#### Scenario: image.include
- **WHEN** a program includes `image.include.ab3` (which has `USELASTPATH`)
- **THEN** the compile does not stop at that word

## MODIFIED Requirements

### Requirement: The FPU bit is refused until FPU support exists
`OPTIMIZE` with bit 2 SHALL stop the compile with the error "Not supported: OPTIMIZE 2 (FPU)" until FPU code generation exists (phase G6). The type `.d` is accepted without the FPU bit (`ab3-compat/types`); no FFP code SHALL stand in for the FPU bit.

#### Scenario: OPTIMIZE 7
- **WHEN** a program with `OPTIMIZE 7` is compiled
- **THEN** the compile fails with that error at the `OPTIMIZE` line and no executable is written

#### Scenario: OPTIMIZE 7 and .d
- **WHEN** a program with `OPTIMIZE 7` also declares `a.d`
- **THEN** the error is the same and appears at the `OPTIMIZE` line
