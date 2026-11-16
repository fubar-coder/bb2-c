## ADDED Requirements

### Requirement: User-level entries keep the documented registers
Every user-level statement and function of the acidlibs SHALL keep a4, a5, a6 and a7, and a function SHALL also keep a3 (Mark Sibly's LIBSDEV documentation, `DOCUMENTATION:794-805`). No other register SHALL be required, and the upper word of d0 after a word function is undefined, except for the listed exceptions. A function that returns a string SHALL leave a3 at the end of the string.

#### Scenario: A statement changes d1 and a0
- **WHEN** a user-level statement such as `Poke.b` returns with different d1 and a0
- **THEN** the register check of the library test passes, because only a4 to a7 are required

#### Scenario: A function loses a6
- **WHEN** a user-level function returns with a different a6
- **THEN** the register check fails

### Requirement: The default entry has no thunk
The token entry of a user-level statement or function SHALL be the C function itself, or an assembler block that keeps the registers of the first requirement. It SHALL NOT be wrapped in a register-saving thunk unless it is one of the exceptions. Nullsubs and every error check SHALL keep their thunks.

#### Scenario: A plain command
- **WHEN** the token table entry of `Peek.b` is disassembled
- **THEN** it is the routine, with no movem push and pop around a call

### Requirement: Internal tokens keep their contracts
An internal token (operators, conversions, string compare and fetch, the string assignment, the procedure prologue routines, freelast, the allocation tokens) SHALL keep the registers of its contract, stated in a KEEPS case with its reason (the compiler's code or a third-party caller), not what the original happens to keep. It needs no thunk unless its code would change them; then the thunk saves only those.

#### Scenario: An internal token
- **WHEN** the compiler calls the string fetch or an operator routine
- **THEN** the registers its contract names (design.md, Decision 17) are kept, by the code or by a thunk of those registers only

#### Scenario: A minimal thunk
- **WHEN** `copstring` or `lastring`/`astring` is disassembled
- **THEN** its thunk saves d0-d1 or d1 respectively and nothing else

### Requirement: Entries without register saves are checked in the build
A user-level entry MAY be `BLITZ_ENTRYPOINT` (`__entrypoint` with `__attribute__((no_icf))`, aliased with `BLITZ_ALIAS_EP`) where the measurement shows a gain. GCC then saves no register in it and may use a6 unsaved, so the build step of every library SHALL run `blitzlib entrypoints` and refuse a routine that writes a6 without restoring it, is called or jumped into from other code, or begins with a jump into another routine. No internal token SHALL be an `__entrypoint` routine.

#### Scenario: An entry that loses a6
- **WHEN** the code of an `__entrypoint` routine writes a6 and does not restore it
- **THEN** the build of the library fails with the routine named (tests with deliberately broken input: `blitzlib_entrypoints_broken_*`)

#### Scenario: Identical code folding
- **WHEN** a plain function and an `__entrypoint` function have the same body
- **THEN** `no_icf` keeps them apart, so the plain function keeps its register saves

#### Scenario: The run-time check
- **WHEN** the KEEPS case of an `__entrypoint` entry runs
- **THEN** it checks a6 at run time

### Requirement: Exceptions exist only for known callers
An entry SHALL keep registers beyond the rule only if a third-party library, an AB3 library or a CD program with inline assembler is shown to need them, found by an analysis and covered by a KEEPS test. Each exception SHALL name its callers in `docs/libs/deviations.md`.

#### Scenario: A listed entry
- **WHEN** a library such as wzlib calls `EMouseY` and uses the result as `EMouseX<<16 | EMouseY` with a0 live
- **THEN** a0 and the upper word of d0 are as before the call

#### Scenario: The list
- **WHEN** the exceptions are listed
- **THEN** they are: `RawStatus` keeps a1; `EventWindow` keeps a1; `EventCode` keeps a0 and the upper word of d0 (zero or kept); `EMouseX` keeps a0; `EMouseY` keeps a0 and the upper word of d0; `InitSound` keeps a0; `Rnd` keeps a0 and a1; `WBWidth`, `WBHeight` and `WBDepth` keep a0 and a1; `ExecVersion` returns a zero-extended long

#### Scenario: An entry without a known caller
- **WHEN** a statement is not in the list
- **THEN** no register beyond the rule is required of it, and it SHALL NOT get a thunk for an imagined caller

### Requirement: Getters that a caller relies on are assembler entries
A getter of two instructions in the original (`move.w x,d0 : rts`) whose registers a known caller relies on SHALL be an assembler entry of the same form, not an alias of a C function: `EMouseX`, `EMouseY` and `EventCode` of windowslib. It SHALL write only the lower word of d0, and its KEEPS statement SHALL state a0 and the upper word of d0.

#### Scenario: wzlib combines the mouse position
- **WHEN** wzlib shifts the result of `EMouseX` into the upper word of d0 and calls `EMouseY` with a0 live
- **THEN** d0 is `EMouseX<<16 | EMouseY` and a0 is unchanged, whatever code gcc generates for the rest of windowslib

### Requirement: An exception is tested and verified in the generated code
For each exception the library test SHALL state exactly the registers its callers need (and the upper word of d0 where needed) in its KEEPS statement, so that a toolchain change that loses them fails the test. An alias SHALL have its disassembly checked, and the check named in the commit; otherwise the entry SHALL be a reduced thunk of exactly the needed registers, or an assembler block.

#### Scenario: gcc starts to use a0 in an aliased exception
- **WHEN** a compiler update makes the C function of `WBWidth` change a0
- **THEN** the KEEPS case of `WBWidth` reads `lost a0` and the test fails

#### Scenario: InitSound
- **WHEN** `IncSound` of ElmoreIncLib calls `InitSound` and reads `(a0)` afterwards
- **THEN** a0 is as before the call (it is lost today)

### Requirement: The rule is documented as a deviation
`docs/libs/deviations.md` SHALL list the rule as a deviation of kind B ("user decision"), with the exceptions and their callers, and SHALL name a6 among the registers the compiler relies on. `tests/README.md` ("Kept registers") and `docs/libs/cmdlibs-port.md` SHALL state the same rule.

#### Scenario: A reader looks up the register rule
- **WHEN** a library author reads `docs/libs/deviations.md`
- **THEN** the page states the kept registers, the exceptions with the callers that need them, and that the original's accidental registers are not kept
