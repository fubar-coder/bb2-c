# check-fixed-orig.cmake - a fixed bug-orig test keeps no recorded original.
# A test is fixed by the header line "; @fixed #N" (ID.bas) or the directive
# "; @fixed #N PAIR..." (ID.drv of the TED tests); its observation ID.orig
# (tests/README.md, "Recorded originals") is deleted with the fix.
#
# bug_orig_fixed_orig_check(ROOT) runs at configure time
# (tests/bug-orig/CMakeLists.txt) over ROOT/AREA/ID.{bas,drv} and stops with
# a FATAL_ERROR naming every stale ID.orig.  In script mode
#   cmake -DROOT=DIR -P check-fixed-orig.cmake
# it does the same (the test with a fake tree: selftest/fixed-orig).
function(bug_orig_fixed_orig_check root)
	file(GLOB sources ${root}/*/*.bas ${root}/*/*.drv)
	set(stale "")
	foreach(src IN LISTS sources)
		file(READ ${src} content)
		string(REPLACE ";" "<semicolon>" content "${content}")
		if(content MATCHES "(^|\n)<semicolon> @fixed #[1-9]")
			get_filename_component(dir ${src} DIRECTORY)
			get_filename_component(id ${src} NAME_WE)
			if(EXISTS ${dir}/${id}.orig)
				string(APPEND stale "\n  ${dir}/${id}.orig (${src} is fixed)")
			endif()
		endif()
	endforeach()
	if(stale)
		message(FATAL_ERROR "a fixed bug-orig test keeps its recorded original; "
			"delete it with the fix:${stale}")
	endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
	bug_orig_fixed_orig_check(${ROOT})
endif()
