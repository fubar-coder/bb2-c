# testing/performance-budgets Specification

## Purpose
Defines the library benchmark on the cycle-exact A500 in warp mode: cycles per command, tolerances for varying loops, one budget per command in tests/perf/budgets.txt with the original's value as the target, and its own ctest preset.

## Requirements

### Requirement: The benchmark measures cycles per command
A benchmark test SHALL run one loop per command in the emulated A500 (68000, Kickstart 1.3, ECS) with cycle-exact CPU, memory and blitter, runtime error checking off and the emulator in warp mode (no wait for the host clock). It SHALL run one emulator session per bench program, the sessions side by side, measure with a CIA timer, subtract the empty loop, and report cycles per call. The values SHALL NOT depend on the host's load or on how many sessions run.

#### Scenario: A run
- **WHEN** the benchmark runs
- **THEN** it reports one value per command in cycles per call, the same whether one session or seven run at a time

#### Scenario: Repeated runs
- **WHEN** the benchmark runs a second time
- **THEN** a command the CPU does alone differs by at most 3 cycles per call, and a loop that waits for the blitter or follows the frame stays within its own tolerance

### Requirement: Varying loops carry a tolerance
A loop that waits for the blitter or depends on the frame (Cls, Circle, BBlit, PlayModule per frame), and a few small loops that move by 2 to 3 cycles, SHALL carry their own tolerance in the budget file, written `B~T`: `B` the highest value of at least five runs, `T` twice their spread; `B + T` is the limit.

#### Scenario: A varying loop
- **WHEN** a command's runs differ by more than the plain margin
- **THEN** its budget is `B~T`, and a failure names both values

### Requirement: Each command has a budget
A budget file SHALL state per measured command the highest allowed cycles per call (the larger value of two runs plus 1 %, rounded up, plus 3 cycles, or `B~T`) and the original's value as the target, shown with its factor and never judged. A command over its budget, a measured command without a budget and a budget without a measurement SHALL fail the test, naming the command and the values. A budget SHALL be tightened when a step makes the command faster.

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
