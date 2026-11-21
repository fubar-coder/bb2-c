# A PREV_INTENDED entry ab3_cli_prev:CASE names a case of ab3_cli (a block judges only its own
# cases, so a mistyped name would never be stale): FATAL_ERROR naming the entry.
#   ab3_check_prev_entries(ENTRIES CASES)   CASES: the lines of the cases file (NAME|...)
# Run as a script (-DENTRIES=... -DCASES=... -P) by the test ab3_prev_entries_* with broken input.
function(ab3_check_prev_entries entries cases)
	foreach(e IN LISTS entries)
		if(e MATCHES "^ab3_cli_prev:(.+)$")
			set(case ${CMAKE_MATCH_1})
			if(NOT "${cases}" MATCHES "(^|\n)${case}\\|")
				message(FATAL_ERROR "PREV_INTENDED entry \"${e}\": ${case} is no case of ab3_cli")
			endif()
		endif()
	endforeach()
endfunction()
if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
	ab3_check_prev_entries("${ENTRIES}" "${CASES}")
	message(STATUS "check-prev-cases: all entries name a case")
endif()
