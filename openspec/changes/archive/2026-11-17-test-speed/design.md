## Decisions

**Jobs.** `"jobs": ""` (ctest `-j` without a number: the CPU count) needs
presets version 11, so the minimum of CMake is 4.3 (own commit). Waiting for a
slot costs nothing: a test that holds no slot sits in a spare job. The slot
count is the bound for emulator sessions: `amiga_emulation_slots_for_cpus(n)` =
`(6 n + 5) / 10`, at least 1, at most `AMIGA_EMULATION_SLOTS_LIMIT` (64); the
online CPU count is read per call (`sysconf`), the function takes the count as a
parameter so that a host test can inject it.

**The hang check of the normal run.** The check that exists (`hang_after`)
counts silence from the start of the job. The compile is silent for long under
load (23 s seen), so for the port and clib sessions the check must not count
it. The library gets `hang_arm`, a file whose existence starts the check; the
corpus runner names the file the first program run creates (`out/gate.run`
with a gate, else `out/NAME.run` of the first source) with the pass key
`hang_from=run`. A hang or crash before it (the compile) keeps the session
limit: no safe sign separates it from a slow compile. After it, the alert
phase of a crash (about 10 s silent before the reboot) and an endless loop
write nothing, as for the recorded original (`BUG_ORIG_HANG_AFTER`, 6 s). The
default is `BUG_ORIG_RUN_HANG_AFTER` 8 s. Validation: the verdict of every
`bug-orig` test is the same as with the session limit alone.

**The TED gates.** Gate 0 (tedprobe) and gate D (teddrive against
teddrivetest, twice) test the emulator's environment and the tools, not the
tested TED, so once per run is enough. `tedab run` gets `gates_only=1`: one
session without a test that runs the two gates, evaluates them and publishes
its run directory as `RUN/ted_gates` (rundir.h) with the file `gates-ok` when
both are OK; at its start it removes an older `gates-ok`. A pass with
`gates_from=NAME` checks `RUN/NAME/gates-ok`: there, its sessions run no gates
(the script says `GATES checked before`, the report says so; `tedprobe` still
runs, unchecked, because the memory layout it leaves decides #340 in seven tests
that fail allocations: without it they hit #340 in every session); without it
(started by hand) they run the gates as before. The ctest fixture `ted_gates`
is the setup test (`ted_gates`, label `emulator`) with a cleanup test; every
TED bug-orig test requires it. A failing gate fails the setup test, ctest marks
the dependent tests Not Run and the run fails. Test with broken input: a gate
D that fails on purpose (`selftest/gates-broken`) gives exit status 1 and
publishes nothing.

**Capture.** The two runs are independent (own work directories and
sessions). They are the two commands of one `execute_process` (CMake starts
both and waits); their output, which a run hardly has, would go from the first
to the second, so they write nothing to stdout and their errors are collected
in one variable.

**The supervisor test.** Its limits are real seconds (2 and 30), so it cannot
be shortened. It gets the label `emulator` and runs in `host_emulator`; it
needs no Amiberry and does not skip.
