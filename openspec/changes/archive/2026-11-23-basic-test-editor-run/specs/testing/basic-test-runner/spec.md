## ADDED Requirements

### Requirement: A program run from the editor
A run setting `editor-run` SHALL take `off` (the default) or `on`. In a
cell with `on` the driver SHALL be compiled as in any cell and then run
once with the editor's COMPILE/RUN instead of from the shell, without
arguments; its result file SHALL be judged as that of a shell run.

#### Scenario: The run of a cell from the editor
- **WHEN** a case of a cell with `editor-run = "on"` passes its checks
- **THEN** the program ran once, from the editor, and the case passes from that run's result file

#### Scenario: No second writer of the result file
- **WHEN** a cell with `editor-run = "on"` has run
- **THEN** no shell run of the driver followed in that cell

### Requirement: The observations of a run from the editor
In a cell with `editor-run = "on"` the printed output SHALL be what the
program wrote to the editor's console. The exit code and the requesters
SHALL be not captured there: their default assertions SHALL NOT apply, and
an assertion on `rc` or `requester` in such a cell SHALL be refused naming
file, line and the setting.

#### Scenario: The console is the output
- **WHEN** the program of an editor-run cell prints "start" and ends
- **THEN** the query `output` holds "start"

#### Scenario: An assertion on the exit code
- **WHEN** a test sets `editor-run = "on"` and asserts `rc = "== 0"` for the same cells
- **THEN** the configuration is refused naming the file, the line of the assertion and `editor-run`

### Requirement: Settings that need a shell run
A cell with `editor-run = "on"` SHALL be refused, naming file, line and
both keys, when it sets `runwrap`, `runargs`, `launcherargs` or `pad = "on"`;
a case of such a cell with an announced runtime error SHALL be refused
naming the source line of the announcement.

#### Scenario: A wrapper of the program
- **WHEN** a test sets `editor-run = "on"` and `runwrap = "@INJECT@|guard"`
- **THEN** the configuration is refused naming both keys

#### Scenario: An announced runtime error
- **WHEN** a case of an editor-run cell holds `!ExpectError{"Shape too Big"}`
- **THEN** the test is refused naming the file and the line of the announcement

### Requirement: The debugger module of a cell
A run setting `defaultdbug` SHALL name a file, as a path setting, that
SHALL be `blitz2:defaultdbug` in every cell the setting applies to, on
either side. Without it a cell with original material SHALL get the CD's
module and another cell none. A missing module SHALL skip the side as a
missing file does.

#### Scenario: The CD's module in a cell with the C libraries
- **WHEN** a test sets `defaultdbug = "@CD_BLITZ2@/DEFAULTDBUG"` and its cell has the C libraries
- **THEN** that cell's `blitz2:defaultdbug` is the CD's module and the cell stays on the side port

#### Scenario: Without the CD
- **WHEN** the module a cell of the side port names is missing
- **THEN** the test of that side exits 77 before any session

### Requirement: A debugger module needs a run from the editor
A cell that sets `defaultdbug` and not `editor-run = "on"` SHALL be
refused naming file, line and both keys: a module is used only by a run
from the editor and is never part of an executable.

#### Scenario: A module without an editor run
- **WHEN** a test sets `defaultdbug` and leaves `editor-run` off
- **THEN** the configuration is refused naming `defaultdbug` and `editor-run`

### Requirement: The query of inject's report of the compiler
When a cell's compiler runs through inject (the `wrap` of inject), the
query `compile.inject` SHALL hold the lines starting with `inject:` of the
compiler's output; otherwise it SHALL be not captured, which fails an
assertion that queries it. It SHALL count as an observation of the whole
program for the cases that run on their own.

#### Scenario: The tracked allocations
- **WHEN** a cell's compiler runs through inject with `track` and inject reports "inject: track: 0 allocations left, 0 bytes"
- **THEN** `compile.inject` contains that line and `compile.log` keeps it

#### Scenario: A compiler without inject
- **WHEN** a cell's compiler does not run through inject and an assertion queries `compile.inject`
- **THEN** the cell fails naming the assertion as not captured

#### Scenario: A case that asserts it runs on its own
- **WHEN** case b of a source with the cases a, b and c asserts `compile.inject`
- **THEN** in the run of all cases b runs on its own

### Requirement: The driver's source in a job line
The placeholder `@DRIVER@` in an element of `jobline` SHALL stand for the
driver's tokenised source on the cell's volume; elsewhere it SHALL be an
error naming file, line and placeholder.

#### Scenario: A second compile of the test's source
- **WHEN** a test sets `jobline = ["job=j001|@DRIVER@|CORPUS:out/again"]`
- **THEN** the cell's job list compiles the driver's source a second time as job j001

### Requirement: The prev comparison leaves out the run from the editor
The comparison with the prev build SHALL compile the driver of an
editor-run test as that of any other test and SHALL NOT run it from the
editor; the debugger module SHALL NOT be staged for it.

#### Scenario: An editor-run test with a prev build
- **WHEN** the prev comparison of a test with `editor-run = "on"` runs
- **THEN** both compilers compile the driver as job j000 and no program runs
