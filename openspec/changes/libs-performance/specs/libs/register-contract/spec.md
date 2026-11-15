## ADDED Requirements

### Requirement: User-level entries keep the documented registers
Every user-level statement and function of the acidlibs SHALL keep a4, a5, a6 and a7, and a function SHALL also keep a3 (Mark Sibly's LIBSDEV documentation, `DOCUMENTATION:794-805`). A statement or function SHALL NOT be required to keep d0, d1, a0, a1, or any other register, and the upper word of d0 after a word function is undefined, except for the entries listed in the exceptions requirement. A function that returns a string SHALL leave a3 at the end of the string, as before.

#### Scenario: A statement changes d1 and a0
- **WHEN** a user-level statement such as `Poke.b` returns with different d1 and a0
- **THEN** the register check of the library test passes, because only a4 to a7 are required

#### Scenario: A function loses a6
- **WHEN** a user-level function returns with a different a6
- **THEN** the register check fails

### Requirement: The default entry has no thunk
The token entry of a user-level statement or function SHALL be the C function itself, or an assembler block that keeps the registers of the first requirement, and SHALL NOT be wrapped in a register-saving thunk, unless it is one of the exceptions. Internal tokens (operators, string compare and fetch, the procedure prologue routines, freelast), nullsubs and every error check SHALL keep their thunks and their contracts.

#### Scenario: A plain command
- **WHEN** the token table entry of `Peek.b` is disassembled
- **THEN** it is the routine, with no movem push and pop around a call

#### Scenario: An internal token
- **WHEN** the compiler calls the string fetch or an operator routine
- **THEN** the registers its contract names (design.md of this change, Context) are kept as before

### Requirement: Exceptions exist only for known callers
An entry SHALL keep registers beyond the rule only if a third-party library, an AB3 library or a CD program with inline assembler is shown to need them. The exceptions are: `RawStatus` keeps a1; `EventWindow` keeps a1; `EventCode` keeps a0 and the upper word of d0 (zero or kept); `EMouseX` keeps a0; `EMouseY` keeps a0 and the upper word of d0; `InitSound` keeps a0; `Rnd` keeps a0 and a1; `WBWidth`, `WBHeight` and `WBDepth` keep a0 and a1; `ExecVersion` returns a zero-extended long. Each exception SHALL name its callers in `docs/libs/deviations.md`. A new exception SHALL be added only with a caller found by an analysis and a KEEPS test.

#### Scenario: A listed entry
- **WHEN** a library such as wzlib calls `EMouseY` and uses the result as `EMouseX<<16 | EMouseY` with a0 live
- **THEN** a0 and the upper word of d0 are as before the call

#### Scenario: An entry without a known caller
- **WHEN** a statement is not in the list
- **THEN** no register beyond the rule is required of it, and it SHALL NOT get a thunk for an imagined caller

### Requirement: An exception is tested and verified in the generated code
For each exception the library test SHALL state exactly the registers its callers need (and the upper word of d0 where needed) in its KEEPS statement, so that a toolchain change that makes the code lose them fails the test. Where the entry is an alias because the code keeps the registers by nature, the disassembly of the built library SHALL have been checked to keep them, and the check SHALL be named in the commit. Where it is not, the entry SHALL be a reduced thunk that saves exactly the needed registers, or an assembler block.

#### Scenario: gcc starts to use a0 in an aliased exception
- **WHEN** a compiler update makes the C function of `EMouseX` change a0
- **THEN** the KEEPS case of `EMouseX` reads `lost a0` and the test fails

#### Scenario: InitSound
- **WHEN** `IncSound` of ElmoreIncLib calls `InitSound` and reads `(a0)` afterwards
- **THEN** a0 is as before the call (it is lost today)

### Requirement: The rule is documented as a deviation
`docs/libs/deviations.md` SHALL list the rule as a deviation of kind B ("user decision"): the thunks no longer preserve what the original preserves, with the exceptions and their callers, and SHALL name a6 among the registers the compiler relies on. `tests/README.md` ("Kept registers") and `docs/libs/cmdlibs-port.md` SHALL state the same rule.

#### Scenario: A reader looks up the register rule
- **WHEN** a library author reads `docs/libs/deviations.md`
- **THEN** the page states the kept registers, the exceptions with the callers that need them, and that the original's accidental registers are not kept
