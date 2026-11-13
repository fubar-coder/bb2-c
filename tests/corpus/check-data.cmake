# check-data.cmake - the corpus data and the sources agree.  A corpus
# program loads its data (sounds, modules) as CORPUS:data/NAME, which the
# build generates from the description data/NAME.spec (mktestdata) and the
# passes stage (tests/corpus/CMakeLists.txt).
#
# corpus_data_check(ROOT) runs at configure time over ROOT/tests*/*.bas
# and ROOT/data/*.spec and stops with a FATAL_ERROR naming every source
# that names a CORPUS:data/NAME without the description ROOT/data/NAME.spec
# and every description no source names.  In script mode
#   cmake -DROOT=DIR -P check-data.cmake
# it does the same (the tests with fake trees: selftest/data-*).
function(corpus_data_check root)
	file(GLOB sources ${root}/tests*/*.bas)
	file(GLOB specs ${root}/data/*.spec)
	set(named "")
	set(errors "")
	foreach(src IN LISTS sources)
		file(STRINGS ${src} lines REGEX "CORPUS:data/")
		foreach(line IN LISTS lines)
			string(REGEX MATCHALL "CORPUS:data/[^\" ]+" refs "${line}")
			foreach(ref IN LISTS refs)
				string(REGEX REPLACE "^CORPUS:data/" "" name "${ref}")
				list(APPEND named ${name})
				if(NOT EXISTS ${root}/data/${name}.spec)
					string(APPEND errors "\n  ${src}: CORPUS:data/${name} has no description "
						"data/${name}.spec")
				endif()
			endforeach()
		endforeach()
	endforeach()
	foreach(spec IN LISTS specs)
		get_filename_component(name ${spec} NAME)
		string(REGEX REPLACE "\\.spec$" "" name "${name}")
		if(NOT name IN_LIST named)
			string(APPEND errors "\n  ${spec}: no corpus source names CORPUS:data/${name}")
		endif()
	endforeach()
	if(errors)
		message(FATAL_ERROR "the corpus data and its sources differ:${errors}")
	endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
	corpus_data_check(${ROOT})
	message(STATUS "check-data: the sources of ${ROOT} and their data agree")
endif()
