# The `prev` build: the results of the previous commit, to compare the
# results of this build with (CLAUDE.md "Acceptance", tests/README.md
# "The prev build").  Not part of the default configure - it builds the
# project a second time.
#
#   -DBLITZ2_PREV=ON            HEAD~1 (the first parent)
#   -DBLITZ2_PREV=<revision>    any commit: HEAD~3, a hash, a branch
#   -DBLITZ2_PREV=<directory>   an existing build directory of another
#                               commit (absolute path, not built here)
#
# A revision is built from `git archive` in build/<preset>/prev/ (the
# results in prev/blitz2/), by the ExternalProject blitz2_prev, which is
# part of ALL; it follows the revision on every build (cmake/prev-build.cmake).
#
# -DPREV_INTENDED names the differences from the prev build that are
# intended (a fix names them in its commit): OFF (none), ON (every one), or
# a ;-list of entries, each of which must really differ (a stale entry
# fails like a difference; tests/README.md "The prev build"):
#   libs/acidlibs/2dlib.obj      a library file (prev_results), path relative
#                                to the build directory
#   corpus:NAME                  the source NAME in the pass corpus, likewise
#   corpus_runerr:NAME           corpus_req, corpus_serial,
#   ...                          corpus_serial_runerr
#   ted_ab:CASE[:C5-P5|:Cc-Pc]   the test CASE of ted_ab (one of its two pairs
#                                with P5/Pc, or either)
# The entries are checked by the tools (tools/lib/previntended).
#
# Sets PREV_DIR: the directory of the prev build, whether it exists or not
# (the tests that need it are skipped without it), and PREV_BLITZ2_DIR.

set(BLITZ2_PREV "" CACHE STRING
	"build of the previous commit: ON (HEAD~1), a revision, or the path of a build directory")
set(PREV_DIR ${CMAKE_BINARY_DIR}/prev)
set(PREV_REQUESTED OFF)

if(BLITZ2_PREV)
	set(PREV_REQUESTED ON)
	if(IS_ABSOLUTE "${BLITZ2_PREV}" AND IS_DIRECTORY "${BLITZ2_PREV}")
		set(PREV_DIR ${BLITZ2_PREV})
	else()
		if(BLITZ2_PREV MATCHES "^(ON|TRUE|YES|Y|1)$")
			set(PREV_REV HEAD~1)
		else()
			set(PREV_REV ${BLITZ2_PREV})
		endif()
		execute_process(COMMAND git -C ${PROJECT_SOURCE_DIR} rev-parse --verify --quiet
			${PREV_REV}^{commit}
			OUTPUT_VARIABLE prev_commit OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE rc)
		if(rc OR NOT prev_commit)
			message(FATAL_ERROR "BLITZ2_PREV=${BLITZ2_PREV}: ${PREV_REV} is no commit of "
				"${PROJECT_SOURCE_DIR} (a directory is given as an absolute path)")
		endif()
		set(prev_args -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
			-DAMIGA_TOOLCHAIN_PREFIX=${AMIGA_TOOLCHAIN_PREFIX}
			-DHOST_SANITIZE=${HOST_SANITIZE})
		if(DEFINED CMAKE_C_FLAGS_DEBUG)
			list(APPEND prev_args "-DCMAKE_C_FLAGS_DEBUG=${CMAKE_C_FLAGS_DEBUG}")
		endif()
		if(BLITZ2_COVERAGE)
			list(APPEND prev_args -DBLITZ2_COVERAGE=ON)
		endif()
		# the list as one argument: ExternalProject would split a ;-list
		string(REPLACE ";" "|" prev_args "${prev_args}")
		# snitch: this build's source (the target snitch of libs/tests), with
		# the SHA256 this build pins (cmake/Snitch.cmake); prev-build.cmake
		# gives it to every configure of the prev build if that pins the same
		file(STRINGS ${PROJECT_SOURCE_DIR}/cmake/Snitch.cmake pin REGEX "URL_HASH +SHA256=")
		string(REGEX REPLACE ".*SHA256=([0-9a-fA-F]+).*" "\\1" pin "${pin}")
		ExternalProject_Add(blitz2_prev
			PREFIX ${CMAKE_BINARY_DIR}/prev-ep
			DOWNLOAD_COMMAND "" UPDATE_COMMAND "" CONFIGURE_COMMAND "" INSTALL_COMMAND ""
			BUILD_COMMAND ${CMAKE_COMMAND} -DREPO=${PROJECT_SOURCE_DIR} -DREV=${PREV_REV}
				-DPREV_DIR=${PREV_DIR} -DGENERATOR=${CMAKE_GENERATOR}
				"-DCONFIGURE_ARGS=${prev_args}" -DSNITCH_PIN=${pin}
				-DSNITCH_SOURCE_DIR=$<TARGET_PROPERTY:snitch,SOURCE_DIR>
				-P ${PROJECT_SOURCE_DIR}/cmake/prev-build.cmake
			BUILD_ALWAYS ON
			USES_TERMINAL_BUILD ON
			BUILD_BYPRODUCTS ${PREV_DIR}/blitz2/blitz2 ${PREV_DIR}/blitz2/ted
				${PREV_DIR}/blitz2/acidlibs ${PREV_DIR}/blitz2/deflibs)
		message(STATUS "prev build: ${PREV_REV} = ${prev_commit} in ${PREV_DIR}")
	endif()
endif()
set(PREV_BLITZ2_DIR ${PREV_DIR}/blitz2)

set(PREV_INTENDED OFF CACHE STRING
	"differences from the prev build that are intended: ON (all) or a ;-list of library paths, corpus:NAME, corpus_runerr:NAME, ted_ab:CASE[:PAIR] ...")
# for the runners: PREV_INTENDED_LINES = "intended=ENTRY\n" per entry of the
# corpus and ted_ab kind (with a colon) and ON, to put in a pass file;
# PREV_INTENDED_PREVCMP_ARGS = the arguments of prevcmp (it also takes the
# entries with a colon, only to check their syntax)
set(PREV_INTENDED_LINES)
set(PREV_INTENDED_PREVCMP_ARGS)
if(PREV_INTENDED STREQUAL "ON")
	set(PREV_INTENDED_LINES "intended=ON\n")
	set(PREV_INTENDED_PREVCMP_ARGS --all)
elseif(PREV_INTENDED AND NOT PREV_INTENDED STREQUAL "OFF")
	foreach(f IN LISTS PREV_INTENDED)
		list(APPEND PREV_INTENDED_PREVCMP_ARGS --intended ${f})
		if(f MATCHES ":")
			string(APPEND PREV_INTENDED_LINES "intended=${f}\n")
		endif()
	endforeach()
endif()
