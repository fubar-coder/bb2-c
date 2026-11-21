# The test part of the PREV_INTENDED entries (cmake/prev.cmake, tests/README.md "The prev
# build") must name a test that compares with the prev build: one of the corpus passes and ted_ab
# (the runners judge those) or one registered with prev_register (the compile-only comparisons).
# A mistyped name, or the name of a test that is left out, would be accepted by every runner and
# judged by none.
#   prev_check_entries(ENTRIES REGISTERED)   FATAL_ERROR naming the entry
# Run as a script for the test with broken input (cmake/prev-check-entries-test.cmake).
function(prev_check_entries entries registered)
	set(builtin corpus corpus_runerr corpus_req corpus_serial corpus_serial_runerr ted_ab)
	foreach(e IN LISTS entries)
		if(e MATCHES "^([^:]+):")
			set(t ${CMAKE_MATCH_1})
			if(NOT t IN_LIST builtin AND NOT t IN_LIST registered)
				message(FATAL_ERROR "PREV_INTENDED entry \"${e}\": no test \"${t}\" compares with the prev build "
					"(the corpus passes, ted_ab and the comparisons made with prev_register: cmake/prev.cmake)")
			endif()
		endif()
	endforeach()
endfunction()
