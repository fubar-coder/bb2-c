# libs/local-memory Specification

## Purpose
Defines how memlib provides and returns the memory of procedure-local lists in Blitz programs: when exec is called, how much memory may stay allocated while the program runs, and what the end of the program frees.

## Requirements

### Requirement: List headers come from buckets
memlib SHALL keep the list headers of procedure-local memory in buckets of a fixed number of entries. A procedure entry SHALL take a free entry of the existing buckets without calling exec, and SHALL allocate a new bucket from exec only when every bucket is full.

#### Scenario: A loop of calls at one depth
- **WHEN** a program calls a procedure with a local string 2000 times in a loop
- **THEN** exec's AllocMem is called for no list header after the first call

#### Scenario: A deep recursion
- **WHEN** a procedure recurses to depth n with buckets of k entries
- **THEN** exec allocates at most ceil(n/k) buckets for the headers, plus the global list's

### Requirement: Empty buckets go back to the system
When a procedure exit leaves the last two buckets empty, memlib SHALL free the last one, so that at most two empty buckets stay allocated while the program runs.

#### Scenario: The recursion goes back
- **WHEN** a recursion of depth 40 with buckets of 8 entries returns to depth 1
- **THEN** at most two empty buckets are still allocated

#### Scenario: Calls at a bucket boundary
- **WHEN** a program calls and returns repeatedly so that the depth moves across the boundary of two buckets
- **THEN** no bucket is allocated or freed on these calls after the first time

### Requirement: The end of the program frees everything
The library finit and the token that frees all memory SHALL free every bucket and every memory block of every list, also when lists were left open by a runtime error, SetErr or the debugger.

#### Scenario: A runtime error inside a recursion
- **WHEN** a program raises a runtime error at depth 10 of a recursion and ends
- **THEN** all memory memlib took from exec has been freed

### Requirement: Programs see no other change
The procedure entry and exit SHALL keep their register contracts and results. A Blitz program SHALL see the same values and output as before; only the time and the moment memory goes back to the system change.

#### Scenario: The library tests
- **WHEN** the library tests of memlib run against the port
- **THEN** every case, including the KEEPS cases of newmem and freelast, is the same as before the change

#### Scenario: The benchmark
- **WHEN** the perf benchmark `bproc.pstr` runs
- **THEN** it is clearly below the original's value (the prototype of the simpler pool measured about 7800 against 10038)
