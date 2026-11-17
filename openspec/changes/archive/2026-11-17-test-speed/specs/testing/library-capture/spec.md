## ADDED Requirements

### Requirement: The two capture passes run at the same time
The target `libs-capture` SHALL run its two passes (the second with another padding) at the same time, each in its own work directory and session. Both must succeed; a run that fails or is skipped SHALL fail the target and nothing is written.

#### Scenario: One of the runs is skipped
- **WHEN** a pass is skipped because Amiberry is missing
- **THEN** the target fails with the message that the run was skipped
