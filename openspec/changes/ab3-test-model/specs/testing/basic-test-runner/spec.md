## ADDED Requirements

### Requirement: The compilers of AB3 mode
The run setting `compiler` SHALL also take `ab3` (blitz2-ab3 of the build,
started from the command line in the session with the driver as its
source) and `amiblitz3` (AmiBlitz3 v3.14.0's own compiler in CLI mode).
Such a cell SHALL compile without `-r` when runtime error checking is on
and with `-r` when it is off. A cell whose AmiBlitz3 files are missing
SHALL be skipped.

#### Scenario: A cell of blitz2-ab3
- **WHEN** a test's cell has `compiler = "ab3"` and `runtime-errors = "off"`
- **THEN** its driver is compiled as `blitz2-ab3 -s DRIVER -e EXE -r` and run, and its result file is judged as that of any cell

### Requirement: The Blitz3 tree of an AB3 session
A session with a cell of `ab3` or `amiblitz3` SHALL have the default
`Blitz3:` tree (the BigLibs, the residents, the test library as a user
library) staged and SHALL assign it again before every compile of
AmiBlitz3. A session with a cell of `amiblitz3` SHALL run without the
JIT.

#### Scenario: Two AmiBlitz3 compiles in one session
- **WHEN** two `amiblitz3` drivers are compiled in one session
- **THEN** `Blitz3:` is assigned before each compile and both compile

### Requirement: The source mode of the driver
The run setting `source-mode` SHALL be `ab3` (the default; the driver is
an ASCII source `ID.ab3` in AB3 mode) or `bb2` (an ASCII source `ID.bb`,
blitz2-ab3's BB2 mode); it SHALL apply only to the compiler `ab3` and
`amiblitz3`, and `bb2` with `amiblitz3` SHALL be refused naming file and
line.

#### Scenario: A BB2-mode case of blitz2-ab3
- **WHEN** a test sets `source-mode = "bb2"` and has only the cell `ab3`
- **THEN** the driver is written as `ID.bb` and compiled by blitz2-ab3 in BB2 mode

### Requirement: Line joining in an AB3 test source
In `source-mode = "ab3"` the preprocessor SHALL read a line that ends with
`@@` outside a string and a comment as joined with the next line, so that
a check macro may be continued with `@@`; such a check SHALL be written to
the driver on one line followed by empty lines for the joined ones. Lines
without a check macro SHALL reach the driver as written, with their `@@`.
In BB2 mode `@@` SHALL not be special, and the driver of a BB2 source
SHALL stay as before byte for byte.

#### Scenario: A check continued with @@
- **WHEN** an AB3 test source has `!CheckStrEq{a$, @@` and `"ab","join"}` on the next line
- **THEN** the driver compiles, the check is reported with the source line of its first line, and the next driver lines keep their source line numbers

#### Scenario: @@ outside a check
- **WHEN** an AB3 test source has `a$=Str$(@@` and `12)` on the next line
- **THEN** both lines reach the driver unchanged

### Requirement: The header block of an AB3 test source
In `source-mode = "ab3"` the comment lines `; KEY = VALUE` before the first
other line of the test source SHALL be the first lines of the driver.

#### Scenario: RuntimeDebug in the header
- **WHEN** an AB3 test source starts with `; RuntimeDebug = 0`
- **THEN** that line is the first line of the driver

### Requirement: The compile result of a command-line compile
For a cell of `ab3` or `amiblitz3` the query `compile` SHALL be `ok` if
the compiler wrote the executable, else `error LINE MESSAGE` with the
line of the test source where the compiler names one, and `compile.log`
SHALL be the compiler's console output.

#### Scenario: AmiBlitz3 refuses a line
- **WHEN** AmiBlitz3 writes no executable for a driver and returns 0
- **THEN** `compile` is an `error` with AmiBlitz3's message, and the cases of the cell are judged by that

### Requirement: The side oracle
A cell of `compiler = "amiblitz3"` SHALL be on the side `oracle`: its
tests SHALL be registered as `PREFIX_ID__CASE_oracle` with the label
`oracle`, and its deviations SHALL carry the key `variant` instead of an
issue (`testing/ab3-tests`). A deviation of such a cell without
`variant`, and `variant` in any other place, SHALL be refused naming file
and line. A test with `!ExpectError` and a cell of `amiblitz3` SHALL be
refused naming file and line (AmiBlitz3 runs without its debugger).

#### Scenario: A deviation without its row
- **WHEN** a test declares `[deviations."compiler=amiblitz3"]` without `variant`
- **THEN** the registration fails naming the file and the line of the table

## MODIFIED Requirements

### Requirement: Registration at build time without the emulator
A build step SHALL register the ctest tests from the sources and their
resolved configurations, without the emulator: one test per case and side
(the cells with original material, AmiBlitz3's cells, the others), and the
bundle tests of `testing/basic-test-bundles`. A side without cells SHALL
NOT be registered. Labels, time limit, cost and fixtures SHALL come from
the configuration; the step SHALL depend on every configuration file it
read.

#### Scenario: A changed directory configuration
- **WHEN** a `tests.toml` changes and the project is built
- **THEN** the registration of the tests below it is rebuilt

#### Scenario: A test without cells of the original
- **WHEN** a test's matrix has no cell with original material
- **THEN** only the test of the other side is registered for each case

#### Scenario: A test of AB3 mode
- **WHEN** a test's matrix has the cells `ab3` and `amiblitz3`
- **THEN** each case has a test of the side port and one of the side oracle

### Requirement: The cases that may share a session with other sources
A case SHALL share a run with other sources of its session key only if
it is on the side port, its source has one cell there, with no session of
its own (`isolate`, `serial`), no test-local data, no run from the editor
and no `hang-after` of its own, its key is that of the default settings
(also with `compiler = "ab3"`) with the environment's time limit, 30 or 60
seconds, and the case would not run on its own in the run of all cases.

#### Scenario: A plain case
- **WHEN** a source with one cell on the side port and the default settings has a case that asserts only check results
- **THEN** that case may share a session with other sources

#### Scenario: A case that asserts the output
- **WHEN** the same source has a case that asserts the printed output
- **THEN** that case does not share a session with other sources

#### Scenario: A test that sets hang-after
- **WHEN** a source with one port cell and otherwise the default settings sets `hang-after = 20`
- **THEN** none of its cases shares a session with other sources

#### Scenario: Two cells on the side
- **WHEN** a source has the cells `port,cd` and `port,clib`
- **THEN** none of its cases shares a session with other sources

#### Scenario: Two AB3 sources
- **WHEN** two sources each have one `ab3` cell on the side port with otherwise default settings
- **THEN** their cases may share a session with each other, and not with a source of `compiler = "port"`

#### Scenario: An AmiBlitz3 cell
- **WHEN** a source has an `amiblitz3` cell with otherwise default settings
- **THEN** its cases on the side oracle share no session with other sources
