# Spec Delta

## MODIFIED Requirements

### Requirement: Intended-difference lists
`refcheck data`, `rwe_declarations_NAME` and the `refcheck asm` tests SHALL accept an explicit list of intended differences with issue numbers, and each listed entry SHALL occur (a stale entry fails), as `blitzlib compare INTENDED` does. `ted_ab` scripts SHALL take a directive `; @fixed-orig #NNN PAIR...` naming the combo pairs that must differ; the directive's old name `; @fixed` SHALL be refused with an error naming the new one.

#### Scenario: Stale entry
- **WHEN** a list names a difference that no longer occurs
- **THEN** the test fails and names the entry

#### Scenario: Unlisted difference
- **WHEN** a difference occurs that no list names
- **THEN** the test fails
