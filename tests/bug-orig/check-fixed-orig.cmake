# check-fixed-orig.cmake - a fixed bug-orig test keeps its recorded
# original.  A test is fixed by the header line "; @fixed #N" (ID.bas) or
# the directive "; @fixed #N" (ID.drv of the TED tests); its observation
# of the original, ID.orig (tests/README.md, "Recorded originals"), is the
# [orig] side of the pair of the shared test model and stays with the fix
# (the target bug-orig-record writes it).
#
# bug_orig_fixed_orig_check(ROOT) runs at configure time
# (tests/bug-orig/CMakeLists.txt) over ROOT/AREA/ID.{bas,drv} and stops with
# a FATAL_ERROR naming every fixed test without ID.orig.  In script mode
#   cmake -DROOT=DIR -P check-fixed-orig.cmake
# it does the same (the tests with fake trees: selftest/fixed-orig,
# selftest/fixed-orig-ok).
function(bug_orig_fixed_orig_check root)
	file(GLOB sources ${root}/*/*.bas ${root}/*/*.drv)
	set(missing "")
	foreach(src IN LISTS sources)
		file(READ ${src} content)
		string(REPLACE ";" "<semicolon>" content "${content}")
		if(content MATCHES "(^|\n)<semicolon> @fixed #[1-9]")
			get_filename_component(dir ${src} DIRECTORY)
			get_filename_component(id ${src} NAME_WE)
			if(NOT EXISTS ${dir}/${id}.orig)
				string(APPEND missing "\n  ${src} (no ${id}.orig)")
			endif()
		endif()
	endforeach()
	if(missing)
		message(FATAL_ERROR "a fixed bug-orig test keeps its recorded original; record it with the "
			"target bug-orig-record (-DBUG_ORIG_RECORD_FILTER=ID):${missing}")
	endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
	bug_orig_fixed_orig_check(${ROOT})
	message(STATUS "check-fixed-orig: every fixed test of ${ROOT} has its recorded original")
endif()
