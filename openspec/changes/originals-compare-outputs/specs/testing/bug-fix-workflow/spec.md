## MODIFIED Requirements

### Requirement: Results change only as intended
A fix SHALL change the results (executables, TED files, library files) only where the fix needs it and SHALL name the changed result in the commit; every other result SHALL stay byte-identical to the previous commit. The comparison with the previous commit stays byte exact (`PREV_INTENDED`); the comparison with the original does not look at executables (`testing/original-comparisons`), so a changed executable needs no issue, no executable pair and no note in the CD table unless it changes an output or a compile result.

#### Scenario: Unrelated result differs
- **WHEN** the `prev` comparison shows a changed file the commit does not name
- **THEN** the commit is not accepted

#### Scenario: Changed executable, same results
- **WHEN** a commit changes the port's executables and no output or compile result
- **THEN** the commit names the change for `prev` (`PREV_INTENDED`) and adds nothing for the comparison with the original
