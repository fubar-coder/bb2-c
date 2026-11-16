## MODIFIED Requirements

### Requirement: Kept registers are checked by the test program
For every routine of the acidlibs the test program SHALL state the set of registers the routine must keep: a4, a5, a6 and a7 (and a3 for a function), plus the registers of its exception under `libs/register-contract` (and the upper word of d0 where a caller needs it). The program SHALL check that set after each call and SHALL write one case with the value `kept` or `lost REGISTERS`. The comparison of a run and the expected data SHALL be equality of these cases, with no subset rule. A register that the original keeps and the rule does not require SHALL NOT be checked. The result and change cases (d0 as a result, a3 as the end of a string result) SHALL stay. The summary of the registers the original keeps MAY stay as information. The change covers only the acidlibs: the test programs of the RWE userlibs (`DO1` in `libs/tests/rwe/rwecmp.h`) and every other routine that is not a user-level entry keep the `KEEPS(routine, regs)` set of what the original keeps in all calls of the scenario, and a register of it that shows `lost` still fails the run. Internal tokens state their contract (the compiler's or a third-party caller's) with its reason.

#### Scenario: Original and C build agree
- **WHEN** both keep all stated registers
- **THEN** both outputs have the same `kept` case

#### Scenario: The C build loses a stated register
- **WHEN** the C build changes a register that is in the routine's stated set
- **THEN** its case says `lost` with the register, the expected data says `kept`, and the compare fails on that key

#### Scenario: The C build changes a register the rule does not require
- **WHEN** a statement without an exception returns with a different d1
- **THEN** no case is written for d1 and the run passes

#### Scenario: An RWE userlib routine
- **WHEN** an RWE test program calls a routine of a userlib and the C build loses a register the original keeps in all calls
- **THEN** its case says `lost` with the register and the run fails, as before this change
