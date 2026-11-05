# The tests a change can affect (README "Coverage"), from the tracefiles of
# a coverage run (cmake/coverage-map.cmake): runs git, ninja and ctest and
# hands their output to tools/covsel with tools/covsel/rules.txt.  Writes
# <build>/coverage/selected.txt for
#   ctest --preset all --tests-from-file <build>/coverage/selected.txt
# (and the same with --preset bug-orig).
#
#   cmake -DBUILD=<build dir> [-DCOVERAGE=<coverage dir>] [-DBASE=<commit>]
#         -P cmake/coverage-select.cmake
#
# BUILD: the build whose tests are selected.  COVERAGE (default:
# <build>/coverage): the output of coverage-map.cmake, the tracefiles
# lcov/<test>.info and the commit file commit.  The changes are those of
# the working tree against the coverage's commit (the line numbers of the
# tracefiles are of that commit), files git does not track included.  BASE
# (default: the coverage's commit): if the coverage's commit is not an
# ancestor of it, the coverage is stale and all tests are selected.
cmake_minimum_required(VERSION 3.28)

if(NOT BUILD)
	message(FATAL_ERROR "usage: cmake -DBUILD=<build dir> [-DCOVERAGE=<coverage dir>] "
		"[-DBASE=<commit>] -P coverage-select.cmake")
endif()
get_filename_component(BUILD ${BUILD} ABSOLUTE)
get_filename_component(SRC ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
if(NOT COVERAGE)
	set(COVERAGE ${BUILD}/coverage)
endif()
get_filename_component(COVERAGE ${COVERAGE} ABSOLUTE)
set(COVSEL ${BUILD}/host/tools/covsel/covsel)
set(IN ${BUILD}/coverage/select-inputs)
set(OUT ${BUILD}/coverage/selected.txt)
foreach(f ${COVERAGE}/commit ${COVERAGE}/lcov ${COVSEL})
	if(NOT EXISTS ${f})
		message(FATAL_ERROR "${f} is missing (README \"Coverage\")")
	endif()
endforeach()
file(REMOVE_RECURSE ${IN})
file(MAKE_DIRECTORY ${IN})

file(STRINGS ${COVERAGE}/commit first LIMIT_COUNT 1)
if(NOT first MATCHES "^([0-9a-f]+)$")
	message(FATAL_ERROR "${COVERAGE}/commit: no commit")
endif()
set(commit ${CMAKE_MATCH_1})
if(NOT BASE)
	set(BASE ${commit})
endif()

set(stale "")
execute_process(COMMAND git -C ${SRC} merge-base --is-ancestor ${commit} ${BASE}
	RESULT_VARIABLE r ERROR_QUIET)
if(r)
	set(stale --stale)
	message(STATUS "The coverage's commit ${commit} is not an ancestor of ${BASE}: all tests")
endif()

execute_process(COMMAND git -C ${SRC} diff -U0 --no-color --no-ext-diff --no-renames
		--src-prefix=a/ --dst-prefix=b/ ${commit}
	OUTPUT_FILE ${IN}/diff.txt RESULT_VARIABLE r)
if(r)
	set(stale --stale)
	file(WRITE ${IN}/diff.txt "")
	message(STATUS "git diff against ${commit} failed: all tests")
endif()
execute_process(COMMAND git -C ${SRC} ls-files --others --exclude-standard
	OUTPUT_FILE ${IN}/untracked.txt RESULT_VARIABLE r)
if(r)
	message(FATAL_ERROR "git ls-files failed")
endif()

file(STRINGS ${BUILD}/CMakeCache.txt ninja REGEX "^CMAKE_MAKE_PROGRAM:[A-Z]+=")
string(REGEX REPLACE "^[^=]*=" "" ninja "${ninja}")
set(deps "")
foreach(dir ${BUILD} ${BUILD}/host)
	if(EXISTS ${dir}/build.ninja)
		string(MD5 tag ${dir})
		execute_process(COMMAND ${ninja} -t deps WORKING_DIRECTORY ${dir}
			OUTPUT_FILE ${IN}/deps-${tag}.txt RESULT_VARIABLE r)
		if(r)
			message(FATAL_ERROR "ninja -t deps failed in ${dir}")
		endif()
		list(APPEND deps --deps ${IN}/deps-${tag}.txt)
	endif()
endforeach()

execute_process(COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${BUILD} --show-only=json-v1
	OUTPUT_FILE ${IN}/tests.json RESULT_VARIABLE r)
if(r)
	message(FATAL_ERROR "ctest --show-only failed in ${BUILD}")
endif()
set(host "")
if(EXISTS ${BUILD}/host/CTestTestfile.cmake)
	execute_process(COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${BUILD}/host --show-only=json-v1
		OUTPUT_FILE ${IN}/host-tests.json RESULT_VARIABLE r)
	if(r)
		message(FATAL_ERROR "ctest --show-only failed in ${BUILD}/host")
	endif()
	set(host --host-tests ${IN}/host-tests.json)
endif()

execute_process(COMMAND ${COVSEL} --root ${SRC} --coverage ${COVERAGE} --rules ${SRC}/tools/covsel/rules.txt
		--tests ${IN}/tests.json ${host} --diff ${IN}/diff.txt ${deps}
		--untracked ${IN}/untracked.txt ${stale} -o ${OUT}
	RESULT_VARIABLE r)
if(r)
	message(FATAL_ERROR "covsel failed")
endif()
message(STATUS "Run them: ctest --test-dir ${BUILD} --tests-from-file ${OUT} "
	"(or with --preset all / --preset bug-orig)")
