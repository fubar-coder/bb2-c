## ADDED Requirements

### Requirement: The benchmark measures cycles per command
A benchmark test SHALL run one loop per command in the emulated A500 (68000, Kickstart 1.3, ECS) with cycle-exact CPU, memory and blitter (`cpu_cycle_exact`, `cpu_memory_cycle_exact`, `blitter_cycle_exact`, `cpu_speed=real`), with runtime error checking off, one emulator session at a time. It SHALL measure with a CIA timer, subtract the cost of the empty loop, and report cycles per call. A repeated run of a command the CPU does alone SHALL differ by at most 1 cycle per call. A loop that waits for the blitter or depends on the frame (Cls, Circle, BBlit, PlayModule per frame) varies more; its budget SHALL state a tolerance derived from the spread of repeated runs.

#### Scenario: A run
- **WHEN** the benchmark runs
- **THEN** it reports one value per command in cycles per call, the same within 1 cycle in a second run for a command the CPU does alone, within its stated tolerance for a loop that waits for the blitter or follows the frame

### Requirement: Each command has a budget
A budget file SHALL state per measured command the highest allowed cycles per call. The test SHALL fail for a command whose measurement exceeds its budget and SHALL name the command, the measured and the budgeted value. The budgets SHALL cover at least the commands of the measurement of 2026-11-15 that are slower than the originals (division, string assignment and compare, blits, Show, Print, collisions, Joy, Peek and Poke, Rnd, Sgn/Int/Abs, Plot/Point, drawing primitives, Sort). A budget SHALL be tightened when a step makes the command faster.

#### Scenario: Over budget
- **WHEN** a library change makes `Peek.b` cost 217 cycles against a budget of 130
- **THEN** the test fails with the command, 217 and 130

#### Scenario: Within budget
- **WHEN** every command is within its budget
- **THEN** the test passes

### Requirement: The benchmark is its own preset
The benchmark SHALL carry the ctest label `perf` and SHALL run through `ctest --preset perf`. It SHALL NOT be part of `ctest --preset all`, `host` or `emulator`. It SHALL skip with return value 77 if Amiberry or the data are missing, as the other emulator tests do.

#### Scenario: The acceptance run
- **WHEN** `ctest --preset all` runs
- **THEN** the benchmark does not run

#### Scenario: Without the emulator
- **WHEN** `ctest --preset perf` runs without Amiberry
- **THEN** the test is skipped, not failed
