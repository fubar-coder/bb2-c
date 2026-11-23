# EMULATION_QUEUE_WAIT: seconds added to the ctest TIMEOUT of every test
# that runs Amiberry.  amiga_emulation lets only AMIGA_EMULATION_MAX
# sessions run at once on the machine (tools/lib/amiga_emulation, a hard
# limit); a session waits for a free slot, and that wait must not make
# ctest kill a test.  A runner without a free slot for its next session
# tries again until one is free, so the sessions of one test may run one after
# the other and each may wait: tests with a fixed number of sessions add
# the session time and the wait once per session.  The time limit of a
# session itself (the runners' timeout=, e.g. BUG_ORIG_TIMEOUT) starts
# only when it has its slot, so hangs are still caught there.
set(EMULATION_QUEUE_WAIT 7200 CACHE STRING
	"seconds added to the ctest TIMEOUT of emulator tests for the wait for an emulator slot")

# The owner of the snapshots (tools/lib/amiga_emulation/README.md, "Snapshot mode"):
# the snapshots of the snapshot mode belong to a build directory, the
# value of AMIGA_EMULATION_OWNER in every emulator test's environment.
# They are kept across ctest runs; a session restores one only if its key
# (a hash of the ROM, the media, the configuration, the machine, Amiberry
# and the waiter) still matches, and a new save removes the stale one of
# the same configuration.  To remove them by hand: amiga_run
# --purge-snapshots BUILDDIR.  Call it at the end of the project's
# top-level CMakeLists.txt, after all add_test: every test that runs
# Amiberry gets the owner: the labels emulator, originals and oracle (the
# comparisons with the originals and with AmiBlitz3 run the emulator
# under another label; the variable does no harm in the tests of the
# label originals that run none).  Without it a session restores the
# snapshot of the owner "none", which every build directory and worktree
# shares.  cmake/emulation-owner-test.cmake checks it.
# the rules, one place for the functions below: the owner's entry of a test
# with the labels LABELS, the coverage entry of a test NAME of the binary
# directory BIN ("" for a test that gets none)
function(_emulation_owner_entry var labels owner)
	set(e "")
	foreach(l emulator originals oracle)
		if(l IN_LIST labels)
			set(e AMIGA_EMULATION_OWNER=${owner})
		endif()
	endforeach()
	set(${var} "${e}" PARENT_SCOPE)
endfunction()
function(_emulation_coverage_entry var name labels bin)
	set(e "")
	if("emulator" IN_LIST labels AND NOT name STREQUAL "host_emulator")
		set(e AMIGA_EMULATION_COVERAGE=${bin}/coverage/${name})
	endif()
	set(${var} "${e}" PARENT_SCOPE)
endfunction()

function(emulation_snapshot_owner owner)
	set_property(GLOBAL PROPERTY EMULATION_SNAPSHOT_OWNER ${owner})
	set(dirs ${CMAKE_SOURCE_DIR})
	while(dirs)
		list(POP_FRONT dirs dir)
		get_property(sub DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
		list(APPEND dirs ${sub})
		get_property(tests DIRECTORY ${dir} PROPERTY TESTS)
		foreach(t IN LISTS tests)
			get_property(labels TEST ${t} DIRECTORY ${dir} PROPERTY LABELS)
			_emulation_owner_entry(e "${labels}" ${owner})
			if(e)
				set_property(TEST ${t} DIRECTORY ${dir} APPEND PROPERTY ENVIRONMENT ${e})
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
	set_property(GLOBAL PROPERTY EMULATION_COVERAGE ON)
	set(dirs ${CMAKE_SOURCE_DIR})
	while(dirs)
		list(POP_FRONT dirs dir)
		get_property(sub DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
		list(APPEND dirs ${sub})
		get_property(tests DIRECTORY ${dir} PROPERTY TESTS)
		get_property(bin DIRECTORY ${dir} PROPERTY BINARY_DIR)
		foreach(t IN LISTS tests)
			get_property(labels TEST ${t} DIRECTORY ${dir} PROPERTY LABELS)
			_emulation_coverage_entry(e ${t} "${labels}" ${bin})
			if(e)
				set_property(TEST ${t} DIRECTORY ${dir} APPEND PROPERTY ENVIRONMENT ${e})
			endif()
		endforeach()
	endwhile()
endfunction()

# The environment the two functions above give a test named NAME with the
# labels LABELS of the binary directory BIN, for a test declared after they
# ran: the bug-orig tests of the old form, which the registration of the
# tests of the model declares (cmake/bug-orig-order.cmake), and the tests of
# the model themselves, whose labels only the build step knows: ctest reads
# this file and calls the function for each of them when it reads their
# include file (cmake/basic-test-model.cmake, the script of the tests'
# environment), so the file must stay usable by ctest (only commands of
# the script mode at the top).  The same rule functions as theirs, after
# each of them ran.
function(emulation_test_environment var name labels bin)
	set(env "")
	get_property(owner GLOBAL PROPERTY EMULATION_SNAPSHOT_OWNER)
	if(owner)
		_emulation_owner_entry(e "${labels}" ${owner})
		list(APPEND env ${e})
	endif()
	get_property(cov GLOBAL PROPERTY EMULATION_COVERAGE)
	if(cov)
		_emulation_coverage_entry(e ${name} "${labels}" ${bin})
		list(APPEND env ${e})
	endif()
	set(${var} "${env}" PARENT_SCOPE)
endfunction()
