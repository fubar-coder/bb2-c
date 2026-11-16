# Spec Delta

## Purpose

Defines which 68020 integer instructions and addressing modes the inline
assembler of AB3 mode accepts, so that AmiBlitz3's includes with
`DIVU.l`/`MULU.l` and similar compile.

## ADDED Requirements

### Requirement: The 68020 integer additions are assembled
In AB3 mode the inline assembler SHALL accept the 68020 integer instructions and addressing modes that AmiBlitz3 v3.14.0's assembler accepts (to be listed in the design from its sources and probes: at least `MULU.l`, `MULS.l`, `DIVU.l`, `DIVS.l`, `DIVUL`, `DIVSL`, `EXTB.l`, and the scaled index `(a0,d0.l*4)`), and SHALL write the same opcode and extension words as AmiBlitz3. FPU, MMU and 68030+ instructions are out of scope and SHALL stay refused.

#### Scenario: DIVU.l
- **WHEN** a program contains `DIVU.l d2,d3` in inline assembler
- **THEN** it compiles, and the instruction's words equal AmiBlitz3's

#### Scenario: convert.include
- **WHEN** a program includes `convert.include.ab3` (line 609, `DIVU.l`)
- **THEN** the include compiles past that line

### Requirement: The gate is the one AmiBlitz3 has
The 68020 instructions SHALL be accepted under exactly the conditions under which AmiBlitz3 accepts them (whether they depend on `OPTIMIZE` bit 1, to be fixed from AmiBlitz3's behaviour in the first task of the design), and SHALL be refused with the message AmiBlitz3 gives otherwise. In BB2 mode every 68020 instruction SHALL stay "Illegal Assembler Instruction Size".

#### Scenario: BB2 mode
- **WHEN** a BB2-mode source has `DIVU.l d2,d3`
- **THEN** the compile fails with "Illegal Assembler Instruction Size" as before

### Requirement: No 68020 code generation
Accepting a 68020 instruction in inline assembler SHALL NOT change the code the compiler generates for BASIC statements: `OPTIMIZE` bit 1 SHALL keep producing 68000 code. A program with such an instruction SHALL need a 68020 or better to run, as AmiBlitz3's executable does.

#### Scenario: BASIC code with OPTIMIZE 5
- **WHEN** a program with `OPTIMIZE 5` and an inline `DIVU.l` is compiled
- **THEN** only the inline instruction is 68020 code
