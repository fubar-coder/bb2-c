# One run of lsclear (amiga/lsclear.c) on an emulated machine:
#   cmake -DAMIGA_RUN=... -DLSCLEAR=... -DMACHINE=a500 -DWORK=DIR
#         -DFILES=PATH;PATH... -DEXPECT="NAME=PASS|NAME=FAIL..." [-DGEN=ON]
#         [-DTIMEOUT=SEC] -P run.cmake
# TEST: (WORK/test) gets lsclear and every FILE; GEN: lsclear -gen as well
# (the files RAM:t_tail, RAM:t_zero, RAM:t_bss).  lsclear prints one line
# `result NAME PASS|FAIL tail_nonzero=H` per file.
#   FILES without EXPECT: every file must be PASS (the test of a defect;
#                         FAIL prints the counts and fails)
#   EXPECT: the verdict of each named file (NAME = the file name without
#           directory, or RAM:t_...) must be the one given, else the test
#           fails; a FAIL also needs a non-zero tail_nonzero (the checker
#           must count the garbage), a PASS needs 0 (EXPECT: list separated by |)
# Exit 77 (skipped) without Amiberry or its data.
cmake_minimum_required(VERSION 4.2.3)

foreach(v AMIGA_RUN LSCLEAR MACHINE WORK)
	if(NOT ${v})
		message(FATAL_ERROR "bug-port run: -D${v}=... missing")
	endif()
endforeach()
if(NOT TIMEOUT)
	set(TIMEOUT 120)
endif()

set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${test})
file(COPY_FILE ${LSCLEAR} ${test}/lsclear)
set(args "")
foreach(f IN LISTS FILES)
	get_filename_component(n ${f} NAME)
	file(COPY_FILE ${f} ${test}/${n})
	string(APPEND args " TEST:${n}")
endforeach()
if(GEN)
	string(PREPEND args " -gen")
endif()
# Kickstart 1.3's AmigaDOS: the redirection right behind the command name
file(WRITE ${WORK}/run.script "TEST:lsclear >TEST:lsclear.txt${args}\n")
execute_process(COMMAND ${AMIGA_RUN} --machine ${MACHINE} --timeout ${TIMEOUT}
	--dir ${WORK} ${WORK}/run.script RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(NOT EXISTS ${test}/lsclear.txt)
	message(FATAL_ERROR "bug-port run: no output (${WORK}), emulator rc=${rc}")
endif()
file(READ ${test}/lsclear.txt out)
message("${out}")
if(rc)
	message(FATAL_ERROR "bug-port run: the emulator run failed (${WORK})")
endif()

# the verdicts, by the identifier of the name: verdict_KEY, count_KEY
set(names "")
string(REGEX MATCHALL "result [^ \r\n]+ (PASS|FAIL) tail_nonzero=[0-9a-f]+" results "${out}")
foreach(r IN LISTS results)
	string(REGEX MATCH "result ([^ ]+) (PASS|FAIL) tail_nonzero=([0-9a-f]+)" m "${r}")
	set(n ${CMAKE_MATCH_1})
	string(MAKE_C_IDENTIFIER "${n}" key)
	set(verdict_${key} ${CMAKE_MATCH_2})
	set(count_${key} ${CMAKE_MATCH_3})
	list(APPEND names ${n})
endforeach()
if(NOT out MATCHES "lsclear: (PASS|FAIL)")
	message(FATAL_ERROR "bug-port run: lsclear did not finish (${test}/lsclear.txt)")
endif()
if(out MATCHES "inconclusive|cannot open|parse failed|LoadSeg failed")
	message(FATAL_ERROR "bug-port run: lsclear could not judge (see above)")
endif()

if(NOT EXPECT)
	if(NOT names)
		message(FATAL_ERROR "bug-port run: no result line")
	endif()
	foreach(n IN LISTS names)
		string(MAKE_C_IDENTIFIER "${n}" key)
		if("${verdict_${key}}" STREQUAL "FAIL")
			message(FATAL_ERROR "${MACHINE}: LoadSeg left the tail of ${n}'s hunks non-zero:"
				" 0x${count_${key}} bytes (the tail must be cleared)")
		endif()
	endforeach()
	return()
endif()
# EXPECT is a list separated by | (a ; does not survive add_test)
string(REPLACE "|" ";" EXPECT "${EXPECT}")
foreach(e IN LISTS EXPECT)
	string(REGEX MATCH "^(.*)=(PASS|FAIL)$" m "${e}")
	set(n ${CMAKE_MATCH_1})
	set(want ${CMAKE_MATCH_2})
	string(MAKE_C_IDENTIFIER "${n}" key)
	if(NOT DEFINED verdict_${key})
		message(FATAL_ERROR "bug-port run: no result for ${n}")
	endif()
	if(NOT "${verdict_${key}}" STREQUAL "${want}")
		message(FATAL_ERROR "bug-port run: ${n} is ${verdict_${key}}, expected ${want}"
			" (tail_nonzero=0x${count_${key}})")
	endif()
	if(want STREQUAL "FAIL" AND count_${key} MATCHES "^0+$")
		message(FATAL_ERROR "bug-port run: ${n} FAIL without a counted byte")
	endif()
	if(want STREQUAL "PASS" AND NOT count_${key} MATCHES "^0+$")
		message(FATAL_ERROR "bug-port run: ${n} PASS with a counted byte")
	endif()
endforeach()
