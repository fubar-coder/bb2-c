## MODIFIED Requirements

### Requirement: AmiBlitz3 checks the agreement cells
The sources of the agreement and fixed cells SHALL also be tests of the
BASIC test model in `tests/ab3/model/exprmatrix/`, with the cells
`compiler = "ab3"` and `compiler = "amiblitz3"`, the latter run in the
preset `oracle`; a cell where AmiBlitz3 differs from the port SHALL be a
deviation of the `amiblitz3` cells naming its row of `docs/ab3/variant.md`
(`testing/ab3-tests`).

#### Scenario: AmiBlitz3 keeps the old rule
- **WHEN** a cell is a fixed cell of #1177
- **THEN** the `amiblitz3` cell's check fails with the original's value, declared as a deviation naming its row in `docs/ab3/variant.md`
