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

# Fresh snapshots per ctest run (amiga-emulation/README.md, "Snapshot
# mode"): the snapshots of the snapshot mode belong to a build directory
# (the owner, AMIGA_EMULATION_OWNER in every emulator test's environment),
# and the fixture test amiga_emulation_snapshot_reset removes those of
# this build directory before the first emulator test of a run (amiga_run
# --purge-snapshots; the snapshots of other build directories stay).
# Call it at the end of the project's top-level CMakeLists.txt, after all
# add_test: every test with the label emulator gets the fixture and the
# owner.  Two overlapping ctest runs of one build directory are not
# supported (the second one's purge removes snapshots the first restores;
# its sessions then boot cold).
function(emulation_snapshot_fixture amiga_run owner)
	add_test(NAME amiga_emulation_snapshot_reset COMMAND ${amiga_run} --purge-snapshots ${owner})
	set_tests_properties(amiga_emulation_snapshot_reset PROPERTIES
		LABELS emulator FIXTURES_SETUP amiga_emulation_snapshots)
	set(dirs ${CMAKE_SOURCE_DIR})
	while(dirs)
		list(POP_FRONT dirs dir)
		get_property(sub DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
		list(APPEND dirs ${sub})
		get_property(tests DIRECTORY ${dir} PROPERTY TESTS)
		foreach(t IN LISTS tests)
			get_property(labels TEST ${t} DIRECTORY ${dir} PROPERTY LABELS)
			if(NOT "emulator" IN_LIST labels OR t STREQUAL "amiga_emulation_snapshot_reset")
				continue()
			endif()
			set_property(TEST ${t} DIRECTORY ${dir} APPEND PROPERTY
				FIXTURES_REQUIRED amiga_emulation_snapshots)
			set_property(TEST ${t} DIRECTORY ${dir} APPEND PROPERTY
				ENVIRONMENT AMIGA_EMULATION_OWNER=${owner})
		endforeach()
	endwhile()
endfunction()
