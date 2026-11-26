# check-unjudged.cmake - every entry PASS:CONFIG:NAME|REASON of
# CORPUS_UNJUDGED (tests/corpus/CMakeLists.txt) names a corpus pass: its
# PASS is one of the ctest names of the passes.  Whether CONFIG and NAME
# belong to that pass is the runner's check (corpus.h unjudged=).
# Included by tests/corpus/CMakeLists.txt (corpus_check_unjudged after the
# passes); as a script (the tests corpus_unjudged_check*):
#   cmake "-DENTRIES=ENTRY;..." "-DPASSES=TEST;..." -P check-unjudged.cmake

# corpus_check_unjudged(ENTRIES PASSES): FATAL_ERROR for an entry whose PASS
# is not in PASSES (both lists)
function(corpus_check_unjudged entries passes)
	foreach(u IN LISTS entries)
		string(REGEX MATCH "^[^:|]*" pass "${u}")
		if(NOT pass IN_LIST passes)
			message(FATAL_ERROR "CORPUS_UNJUDGED: ${u}: no corpus pass ${pass}")
		endif()
	endforeach()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
	corpus_check_unjudged("${ENTRIES}" "${PASSES}")
	message(STATUS "check-unjudged: every entry names a pass")
endif()
