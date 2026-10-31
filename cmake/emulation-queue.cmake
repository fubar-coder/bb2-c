# EMULATION_QUEUE_WAIT: seconds added to the ctest TIMEOUT of every test
# that runs Amiberry.  amiga_emulation lets only AMIGA_EMULATION_MAX
# sessions run at once on the machine (tools/lib/amiga_emulation, a hard
# limit); a session waits for a free slot, and that wait must not make
# ctest kill a test.  A runner without a free slot for its next session
# ends one of its own first, so the sessions of one test may run one after
# the other and each may wait: tests with a fixed number of sessions add
# the session time and the wait once per session.  The time limit of a
# session itself (the runners' timeout=, e.g. BUG_ORIG_TIMEOUT) starts
# only when it has its slot, so hangs are still caught there.
set(EMULATION_QUEUE_WAIT 7200 CACHE STRING
	"seconds added to the ctest TIMEOUT of emulator tests for the wait for an emulator slot")
