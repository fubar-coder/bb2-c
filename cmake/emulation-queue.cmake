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

# The owner of the snapshots (amiga-emulation/README.md, "Snapshot mode"):
# the snapshots of the snapshot mode belong to a build directory, the
# value of AMIGA_EMULATION_OWNER in every emulator test's environment.
# They are kept across ctest runs; a session restores one only if its key
# (a hash of the ROM, the media, the configuration, the machine, Amiberry
# and the waiter) still matches, and a new save removes the stale one of
# the same configuration.  To remove them by hand: amiga_run
# --purge-snapshots BUILDDIR.  Call it at the end of the project's
# top-level CMakeLists.txt, after all add_test: every test with the label
# emulator gets the owner.
function(emulation_snapshot_owner owner)
	set(dirs ${CMAKE_SOURCE_DIR})
	while(dirs)
		list(POP_FRONT dirs dir)
		get_property(sub DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
		list(APPEND dirs ${sub})
		get_property(tests DIRECTORY ${dir} PROPERTY TESTS)
		foreach(t IN LISTS tests)
			get_property(labels TEST ${t} DIRECTORY ${dir} PROPERTY LABELS)
			if("emulator" IN_LIST labels)
				set_property(TEST ${t} DIRECTORY ${dir} APPEND PROPERTY
					ENVIRONMENT AMIGA_EMULATION_OWNER=${owner})
			endif()
		endforeach()
	endwhile()
endfunction()

# The coverage configuration (BLITZ2_COVERAGE, preset amiga-cov): every
# emulator test but host_emulator (the emulation library's own tests) gets
# AMIGA_EMULATION_COVERAGE, the directory its sessions' coverage files go
# to (tools/lib/amiga_emulation/amiga_emulation.h, "Coverage"):
# coverage/NAME in the test's binary directory, where ctest runs it.  Call
# it after all add_test, like emulation_snapshot_owner.
function(emulation_coverage)
	set(dirs ${CMAKE_SOURCE_DIR})
	while(dirs)
		list(POP_FRONT dirs dir)
		get_property(sub DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
		list(APPEND dirs ${sub})
		get_property(tests DIRECTORY ${dir} PROPERTY TESTS)
		get_property(bin DIRECTORY ${dir} PROPERTY BINARY_DIR)
		foreach(t IN LISTS tests)
			get_property(labels TEST ${t} DIRECTORY ${dir} PROPERTY LABELS)
			if("emulator" IN_LIST labels AND NOT t STREQUAL "host_emulator")
				set_property(TEST ${t} DIRECTORY ${dir} APPEND PROPERTY
					ENVIRONMENT AMIGA_EMULATION_COVERAGE=${bin}/coverage/${t})
			endif()
		endforeach()
	endwhile()
endfunction()
