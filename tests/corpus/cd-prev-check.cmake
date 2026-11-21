# cd-prev-check.cmake - the comparison of the CD compile with the prev build (corpusrun cd,
# current=, test=, intended=; tests/corpus/CMakeLists.txt, corpus_cd_prev_check) on broken input.
# One CD source is compiled by this build (pass base: its table); then the same source is
# compiled again against tables made from that one:
#   ok        the same table: passes, nothing differs
#   bad       the hash of the executable changed: fails, the row and the entry to quote named
#   intended  the table of bad, the row listed (PREV_INTENDED): passes, reported "(intended)"
#   stale     the table of ok, the row listed: fails, STALE
#   missing   a table with another source only: fails, the row is in one table only
#   cmake -DCORPUSRUN=... -DWORK=DIR -DPASSES=DIR -DSOURCE=REL -P cd-prev-check.cmake
# (the pass files are in PASSES, made by CMake; exit status 77 = no emulator or no CD)
cmake_minimum_required(VERSION 4.3)

file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK})
function(run kind)
	execute_process(COMMAND ${CORPUSRUN} cd ${PASSES}/cd-prevcheck-${kind}.pass
		RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
	set(rc ${rc} PARENT_SCOPE)
	set(out "${out}${err}" PARENT_SCOPE)
	message("== ${kind}: exit ${rc}\n${out}${err}")
endfunction()
function(expect kind want regex)
	run(${kind})
	if(rc EQUAL 77)
		cmake_language(EXIT 77)
	endif()
	if(NOT rc EQUAL want)
		message(FATAL_ERROR "cd-prev-check: ${kind}: exit ${rc}, expected ${want}")
	endif()
	if(regex AND NOT out MATCHES "${regex}")
		message(FATAL_ERROR "cd-prev-check: ${kind}: the output does not match: ${regex}")
	endif()
endfunction()

expect(base 0 "")
file(READ ${WORK}/base.tsv table)
if(NOT table MATCHES "${SOURCE}\tok\t")
	message(FATAL_ERROR "cd-prev-check: ${SOURCE} does not compile: ${table}")
endif()
# the tables the runs compare with
foreach(k ok stale)
	file(WRITE ${WORK}/cur-${k}.tsv "${table}")
endforeach()
# the hash of the executable: its first digit changed
string(REGEX REPLACE "(${SOURCE}\tok\t\t\t[0-9]+\t)(.)" "\\1x" bad "${table}")
if(bad STREQUAL table)
	message(FATAL_ERROR "cd-prev-check: no hash to change in ${table}")
endif()
file(WRITE ${WORK}/cur-bad.tsv "${bad}")
file(WRITE ${WORK}/cur-intended.tsv "${bad}")
string(REGEX REPLACE "${SOURCE}\t" "OTHER/SOURCE.BB2\t" other "${table}")
file(WRITE ${WORK}/cur-missing.tsv "${other}")

expect(ok 0 "1 sources, 1 the same as prev, 0 different")
expect(bad 1 "${SOURCE}: DIFFERS \\(prev -> now\\): sha256_masked [0-9a-f]+ -> x[0-9a-f]+;[^\n]*\n[^\n]*prev differs in: PREV_INTENDED=\"corpus_cd_prev_check_prev:${SOURCE}\"")
expect(intended 0 "${SOURCE}: DIFFERS[^\n]*\\(intended\\)")
expect(stale 1 "STALE corpus_cd_prev_check_prev:${SOURCE}: intended, but prev does not differ")
expect(missing 1 "not in the prev table|only in the prev table")
message(STATUS "cd-prev-check: all runs as expected")
