# The map from functions to tests (README "Coverage"), after a coverage
# test run: <build>/coverage/map.tsv, made by tools/covmap from
#   - the host tests' gcov data (HOST_COVERAGE: <build>/host/coverage/<test>/,
#     read with gcov per test, recorded as test host/<test>),
#   - the coverage files the emulator tests left (<dir>/coverage/<test>/
#     <program>.cov anywhere in the build directory) with the program's
#     link map (<program>.map below <build>/programs), `nm -A` and
#     `objdump --dwarf=decodedline` of its objects (line ranges if they
#     are compiled with -g),
#   - `ninja -t deps` of the build (the source of each object).
# The inputs it hands covmap are kept in <build>/coverage/map-inputs/.
#
#   cmake -DBUILD=<build dir> [-DGCOV=gcov] -P cmake/coverage-map.cmake
#   (or the target coverage-map)
cmake_minimum_required(VERSION 3.28)

if(NOT BUILD)
	message(FATAL_ERROR "usage: cmake -DBUILD=<build dir> [-DGCOV=gcov] -P coverage-map.cmake")
endif()
get_filename_component(BUILD ${BUILD} ABSOLUTE)
get_filename_component(SRC ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
if(NOT GCOV)
	set(GCOV gcov)
endif()
set(OUT ${BUILD}/coverage)
set(IN ${OUT}/map-inputs)
set(COVMAP ${BUILD}/host/tools/covmap/covmap)
if(NOT EXISTS ${COVMAP})
	message(FATAL_ERROR "${COVMAP} is missing: build the host side first")
endif()
file(REMOVE_RECURSE ${IN})
file(MAKE_DIRECTORY ${IN}/gcov ${IN}/amiga)

# a variable of a build's cache
function(cache_value dir name var)
	file(STRINGS ${dir}/CMakeCache.txt line REGEX "^${name}:[A-Z]+=")
	string(REGEX REPLACE "^[^=]*=" "" line "${line}")
	set(${var} "${line}" PARENT_SCOPE)
endfunction()

execute_process(COMMAND git -C ${SRC} rev-parse HEAD
	OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE r)
if(r)
	message(FATAL_ERROR "git rev-parse HEAD failed in ${SRC}")
endif()
execute_process(COMMAND git -C ${SRC} status --porcelain --untracked-files=no
	OUTPUT_VARIABLE dirty)
if(dirty)
	message(STATUS "The working tree has changes against ${commit}: the selection takes "
		"them as changes since the map")
endif()

set(inputs "")
cache_value(${BUILD} CMAKE_MAKE_PROGRAM ninja)
foreach(dir ${BUILD} ${BUILD}/host)
	if(EXISTS ${dir}/build.ninja)
		string(MD5 tag ${dir})
		execute_process(COMMAND ${ninja} -t deps WORKING_DIRECTORY ${dir}
			OUTPUT_FILE ${IN}/deps-${tag}.txt RESULT_VARIABLE r)
		if(r)
			message(FATAL_ERROR "ninja -t deps failed in ${dir}")
		endif()
		string(APPEND inputs "deps\t${IN}/deps-${tag}.txt\n")
	endif()
endforeach()

# host: one gcov run per test over its .gcda files, each next to a copy of
# its .gcno (gcov finds the notes by the data file's name)
set(nhost 0)
if(IS_DIRECTORY ${BUILD}/host/coverage)
	file(GLOB tests RELATIVE ${BUILD}/host/coverage LIST_DIRECTORIES true ${BUILD}/host/coverage/*)
	foreach(t IN LISTS tests)
		set(prefix ${BUILD}/host/coverage/${t})
		if(NOT IS_DIRECTORY ${prefix})
			continue()
		endif()
		file(GLOB_RECURSE gcdas ${prefix}/*.gcda)
		set(args "")
		foreach(gcda IN LISTS gcdas)
			string(LENGTH ${prefix} n)
			string(SUBSTRING ${gcda} ${n} -1 object)
			string(REGEX REPLACE "\\.gcda$" ".gcno" gcno ${object})
			string(REGEX REPLACE "\\.gcda$" ".gcno" copy ${gcda})
			if(EXISTS ${gcno})
				file(COPY_FILE ${gcno} ${copy} ONLY_IF_DIFFERENT)
				list(APPEND args ${gcda})
			endif()
		endforeach()
		if(NOT args)
			continue()
		endif()
		execute_process(COMMAND ${GCOV} --json-format --stdout ${args}
			WORKING_DIRECTORY ${IN}/gcov
			OUTPUT_FILE ${IN}/gcov/${t}.json ERROR_VARIABLE e RESULT_VARIABLE r)
		if(r)
			message(FATAL_ERROR "${GCOV} failed for the host test ${t}: ${e}")
		endif()
		string(APPEND inputs "gcov\thost/${t}\t${IN}/gcov/${t}.json\n")
		math(EXPR nhost "${nhost} + 1")
	endforeach()
endif()

# Amiga: the coverage files, and per program its link map and objects
cache_value(${BUILD} CMAKE_NM nm)
cache_value(${BUILD} CMAKE_OBJDUMP objdump)
file(GLOB_RECURSE covs ${BUILD}/*.cov)
set(programs "")
set(namiga 0)
foreach(cov IN LISTS covs)
	get_filename_component(dir ${cov} DIRECTORY)
	get_filename_component(t ${dir} NAME)
	get_filename_component(parent ${dir} DIRECTORY)
	get_filename_component(parent ${parent} NAME)
	get_filename_component(program ${cov} NAME_WE)
	if(NOT parent STREQUAL "coverage")
		continue()
	endif()
	if(NOT program IN_LIST programs)
		file(GLOB_RECURSE map ${BUILD}/programs/${program}.map)
		list(LENGTH map n)
		if(NOT n EQUAL 1)
			message(FATAL_ERROR "no single link map ${program}.map below ${BUILD}/programs: ${map}")
		endif()
		# the program's objects of this build (not those of the toolchain)
		file(STRINGS ${map} loads REGEX "^LOAD ")
		set(objects "")
		foreach(l IN LISTS loads)
			string(REGEX REPLACE "^LOAD " "" o "${l}")
			if(NOT IS_ABSOLUTE ${o} AND EXISTS ${BUILD}/${o})
				list(APPEND objects ${o})
			endif()
		endforeach()
		execute_process(COMMAND ${nm} -A ${objects} WORKING_DIRECTORY ${BUILD}
			OUTPUT_FILE ${IN}/amiga/${program}.syms RESULT_VARIABLE r)
		if(r)
			message(FATAL_ERROR "${nm} failed on the objects of ${program}")
		endif()
		# one object per call: m68k-amigaos-objdump 2.45 crashes on the line
		# tables of several Amiga objects at once
		file(WRITE ${IN}/amiga/${program}.lines "")
		foreach(o IN LISTS objects)
			execute_process(COMMAND ${objdump} --dwarf=decodedline ${o} WORKING_DIRECTORY ${BUILD}
				OUTPUT_VARIABLE lines RESULT_VARIABLE r)
			if(r)
				message(FATAL_ERROR "${objdump} failed on ${o}")
			endif()
			file(APPEND ${IN}/amiga/${program}.lines "${lines}")
		endforeach()
		string(APPEND inputs "linkmap\t${program}\t${map}\n"
			"objsyms\t${program}\t${IN}/amiga/${program}.syms\n"
			"lines\t${program}\t${IN}/amiga/${program}.lines\n")
		list(APPEND programs ${program})
	endif()
	string(APPEND inputs "cov\t${t}\t${program}\t${cov}\n")
	math(EXPR namiga "${namiga} + 1")
endforeach()

file(WRITE ${IN}/inputs.txt "${inputs}")
execute_process(COMMAND ${COVMAP} --commit ${commit} --root ${SRC} --drop ${BUILD}
	--inputs ${IN}/inputs.txt -o ${OUT}/map.tsv RESULT_VARIABLE r)
if(r)
	message(FATAL_ERROR "covmap failed")
endif()
message(STATUS "${OUT}/map.tsv (commit ${commit}): ${nhost} host tests, "
	"${namiga} coverage files of ${programs}")
