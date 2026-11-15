## MODIFIED Requirements

### Requirement: Kept registers are checked by the test program
For every routine the test program SHALL state the set of registers the routine must keep: a4, a5, a6 and a7 (and a3 for a function), plus the registers of its exception under `libs/register-contract` (and the upper word of d0 where a caller needs it). The program SHALL check that set after each call and SHALL write one case with the value `kept` or `lost REGISTERS`. The comparison of a run and the expected data SHALL be equality of these cases, with no subset rule. A register that the original keeps and the rule does not require SHALL NOT be checked. The result and change cases (d0 as a result, a3 as the end of a string result) SHALL stay. The summary of the registers the original keeps MAY stay as information.

#### Scenario: Original and C build agree
- **WHEN** both keep all stated registers
- **THEN** both outputs have the same `kept` case

#### Scenario: The C build loses a stated register
- **WHEN** the C build changes a register that is in the routine's stated set
- **THEN** its case says `lost` with the register, the expected data says `kept`, and the compare fails on that key

#### Scenario: The C build changes a register the rule does not require
- **WHEN** a statement without an exception returns with a different d1
- **THEN** no case is written for d1 and the run passes
