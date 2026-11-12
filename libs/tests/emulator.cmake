# libs_tests in Amiberry (bb2 libs-c/tools/amiga_test.py):
#   cmake -DAMIGA_RUN=... -DBLITZLIB=... -DPROGRAM=libs_tests -DLIBS=DIR
#         -DORIGINALS=acidlibs-of-the-CD -DWORK=DIR [-DTIMEOUT=SEC]
#         [-DFILTER=snitch-filter] [-DNO_ORIGINALS=ON] [-DILLEGAL_WATCH=ON]
#         [-DUSERLIBS=OBJ|ORIG;...] [-DLIBSET=obj|orig]
#         [-DLIBS_PAD=N] [-DEXPECTED=DIR -DRESULTS=results [-DCAPTURE_CORRUPT=NAME]]
#         [-DOBJ_VARIANTS=FILE;...] [-DCAP_DUMP=KEY|BLOCK:N -DRESULTS=results]
#         [-DKEEP_BIN=ON] -P emulator.cmake
# TEST: (WORK/test) gets libs_tests, every DIR/NAME.obj and the library
# with the same number from ORIGINALS as NAME.orig; USERLIBS: further
# library files OBJ (C build, as NAME.obj) with their original ORIG (as
# NAME.orig), e.g. the RWE userlibs.  OBJ_VARIANTS: library files
# DIR2/NAME.obj that take the place of DIR/NAME.obj (a broken build, the
# test of a check: libs_emulator_lost).  The exit status is
# that of libs_tests; 77 (skipped) without the CD or the emulator.
# NO_ORIGINALS: only the NAME.obj (BLITZLIB and ORIGINALS not needed).
# LIBSET (the shell variable of the run): the library set the programs
# load (sysload.h load_set), obj (NAME.obj) or orig (NAME.orig); each
# program writes its cases to TEST:captures/NAME.txt (capture.h), which
# stays in WORK/test/captures. LIBS_PAD: the run allocates N bytes before
# each program (another memory layout, libs-capture). EXPECTED
# (libs/tests/expected): the cases are compared with it by RESULTS
# libs-compare --set LIBSET; with FILTER only the programs that wrote
# cases, else all. A block program also writes its block observations raw
# to TEST:captures/NAME.bin, from which RESULTS makes its block cases
# (capture.h); these files (about 300 MB a run) are deleted when the run
# with EXPECTED passed and kept when it failed, or with KEEP_BIN or the
# environment variable LIBS_KEEP_BIN (e.g. for results libs-compare
# --other), and the next run removes WORK anyway.
# CAP_DUMP, or the environment variable LIBS_CAP_DUMP (e.g.
# LIBS_CAP_DUMP=BLOCK:294 ctest -R libs_emulator): a key is the shell
# variable LIBS_CAP_DUMP of the run (capture.h): the program that writes
# the plain bytes case of that key writes its bytes to
# TEST:captures/NAME.dump (WORK/test/captures); a key of an observation in
# a block and BLOCK:N are dumped there from NAME.bin after the run (RESULTS
# libs-dump: its bytes or the block's observation lines). The path and
# the first lines of each dump are shown; the run fails if there is none.
# CAPTURE_CORRUPT: the last digit of the 8-digit hex number in the value of
# the run's first case of NAME changed before the compare, the test of the check.
# ILLEGAL_WATCH: Amiberry logs every access to unmapped memory
# (log_illegal_mem, --log); for every line
#   illegal-watch START END CODESTART CODEEND        (hex, from libs_tests)
# the run needs at least one logged access to START from outside the code
# range (the program's probe: the range is unmapped and the log works) and
# fails if code in [CODESTART,CODEEND) accessed [START,END).
cmake_minimum_required(VERSION 4.2.3)

set(required AMIGA_RUN PROGRAM LIBS WORK)
if(NOT NO_ORIGINALS)
	list(APPEND required BLITZLIB ORIGINALS)
endif()
if(EXPECTED)
	list(APPEND required RESULTS LIBSET)
endif()
if(NOT DEFINED CAP_DUMP AND DEFINED ENV{LIBS_CAP_DUMP})
	set(CAP_DUMP "$ENV{LIBS_CAP_DUMP}")
endif()
if(CAP_DUMP)
	list(APPEND required RESULTS)
endif()
if(DEFINED ENV{LIBS_KEEP_BIN})
	set(KEEP_BIN ON)
endif()
if(LIBSET AND NOT LIBSET MATCHES "^(obj|orig)$")
	message(FATAL_ERROR "emulator.cmake: LIBSET=${LIBSET}: obj or orig")
endif()
if(DEFINED LIBS_PAD AND NOT LIBS_PAD MATCHES "^[0-9]+$")
	message(FATAL_ERROR "emulator.cmake: LIBS_PAD=${LIBS_PAD}: a number of bytes")
endif()
foreach(v IN LISTS required)
	if(NOT ${v})
		message(FATAL_ERROR "emulator.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT TIMEOUT)
	set(TIMEOUT 120)
endif()
if(NOT NO_ORIGINALS AND NOT EXISTS ${ORIGINALS})
	message(STATUS "libs_emulator: no ${ORIGINALS} - skipped (fetch-originals)")
	cmake_language(EXIT 77)
endif()

set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${test})
file(COPY_FILE ${PROGRAM} ${test}/libs_tests)
file(GLOB objs ${LIBS}/*.obj)
foreach(obj IN LISTS objs)
	get_filename_component(name ${obj} NAME_WE)
	file(COPY_FILE ${obj} ${test}/${name}.obj)
	if(NOT NO_ORIGINALS)
		execute_process(COMMAND ${BLITZLIB} extract ${ORIGINALS} ${obj} ${test}/${name}.orig
			RESULT_VARIABLE rc)
		if(rc)
			message(FATAL_ERROR "libs_emulator: no original of ${name}")
		endif()
	endif()
endforeach()
foreach(v IN LISTS OBJ_VARIANTS)
	get_filename_component(name ${v} NAME_WE)
	if(NOT EXISTS ${test}/${name}.obj)
		message(FATAL_ERROR "libs_emulator: OBJ_VARIANTS: no ${LIBS}/${name}.obj to replace")
	endif()
	file(COPY_FILE ${v} ${test}/${name}.obj)
endforeach()

foreach(u IN LISTS USERLIBS)
	string(REPLACE "|" ";" u "${u}")
	list(GET u 0 obj)
	list(GET u 1 orig)
	get_filename_component(name ${obj} NAME_WE)
	file(COPY_FILE ${obj} ${test}/${name}.obj)
	if(NOT NO_ORIGINALS)
		if(NOT EXISTS ${orig})
			message(STATUS "libs_emulator: no ${orig} - skipped (fetch-originals)")
			cmake_language(EXIT 77)
		endif()
		file(COPY_FILE ${orig} ${test}/${name}.orig)
	endif()
endforeach()

file(MAKE_DIRECTORY ${test}/captures)
set(vars_cmd "")
if(LIBSET)
	string(APPEND vars_cmd "Set LIBSET ${LIBSET}\n")
endif()
if(LIBS_PAD)
	string(APPEND vars_cmd "Set LIBS_PAD ${LIBS_PAD}\n")
endif()
if(CAP_DUMP AND NOT CAP_DUMP MATCHES "^BLOCK:")
	# a key holds | $ spaces: quoted, with AmigaDOS's escape * before * " $
	string(REGEX REPLACE "([*\"$])" "*\\1" v "${CAP_DUMP}")
	string(APPEND vars_cmd "Set LIBS_CAP_DUMP \"${v}\"\n")
endif()

set(args "")
if(FILTER)
	set(args " \"${FILTER}\"")
endif()
set(run_opts "")
if(ILLEGAL_WATCH)
	set(run_opts --log --set log_illegal_mem=true)
endif()
file(WRITE ${WORK}/run.script "${vars_cmd}TEST:libs_tests${args} >TEST:libs_tests.txt\n"
	"Echo \"rc=$RC\" >>TEST:libs_tests.txt\n")
execute_process(COMMAND ${AMIGA_RUN} ${run_opts} --timeout ${TIMEOUT} --dir ${WORK}
	${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(EXISTS ${test}/libs_tests.txt)
	file(READ ${test}/libs_tests.txt out)
	message("${out}")
endif()
if(rc)
	message(FATAL_ERROR "libs_emulator: the emulator run failed (${WORK})")
endif()
if(NOT out MATCHES "rc=([0-9]+)[\r\n]*$")
	message(FATAL_ERROR "libs_emulator: no result in ${test}/libs_tests.txt")
endif()
set(result ${CMAKE_MATCH_1})

set(watch_failed "")
if(ILLEGAL_WATCH)
	string(REGEX MATCHALL "illegal-watch [0-9a-f]+ [0-9a-f]+ [0-9a-f]+ [0-9a-f]+" watches "${out}")
	if(NOT watches)
		message(FATAL_ERROR "libs_emulator: ILLEGAL_WATCH, but libs_tests named no range")
	endif()
	file(STRINGS ${WORK}/amiberry.log illegal REGEX "Illegal [bwl](get|put) at [0-9a-f]+")
	foreach(w IN LISTS watches)
		string(REPLACE " " ";" w "${w}")
		list(TRANSFORM w PREPEND "0x" AT 1 2 3 4)
		list(GET w 1 start)
		list(GET w 2 end)
		list(GET w 3 code)
		list(GET w 4 codeend)
		math(EXPR start "${start}")
		math(EXPR end "${end}")
		math(EXPR code "${code}")
		math(EXPR codeend "${codeend}")
		set(probe 0)
		set(hits 0)
		set(first "")
		set(highest -1)
		foreach(line IN LISTS illegal)
			if(NOT line MATCHES "Illegal ([bwl])(get|put) at ([0-9a-f]+).* PC=([0-9a-f]+)")
				continue()
			endif()
			math(EXPR at "0x${CMAKE_MATCH_3}")
			math(EXPR pc "0x${CMAKE_MATCH_4}")
			if(at LESS start OR NOT at LESS end)
				continue()
			endif()
			if(pc LESS code OR NOT pc LESS codeend)
				if(at EQUAL start)
					math(EXPR probe "${probe} + 1")
				endif()
				continue()
			endif()
			math(EXPR hits "${hits} + 1")
			if(NOT first)
				set(first "${line}")
			endif()
			math(EXPR off "${at} - ${start}")
			if(off GREATER highest)
				set(highest ${off})
			endif()
		endforeach()
		math(EXPR start_hex "${start}" OUTPUT_FORMAT HEXADECIMAL)
		if(NOT probe)
			message(FATAL_ERROR "libs_emulator: no logged access to ${start_hex} by the probe"
				" - the range is mapped or the log is off (${WORK}/amiberry.log)")
		endif()
		if(hits)
			math(EXPR highest "${highest} + 1")
			string(APPEND watch_failed "the library code read behind the data at ${start_hex}:"
				" ${hits} logged accesses, up to ${highest} bytes behind it (first: ${first})\n")
		endif()
	endforeach()
endif()

# the cases against the expected data (results libs-compare)
set(compare_failed "")
if(EXPECTED)
	file(GLOB written RELATIVE ${test}/captures ${test}/captures/*.txt)
	if(CAPTURE_CORRUPT)
		# the last 8-digit hex number of the first case, its last digit changed
		set(t ${test}/captures/${CAPTURE_CORRUPT}.txt)
		if(NOT EXISTS ${t})
			message(FATAL_ERROR "libs_emulator: CAPTURE_CORRUPT: no ${t}")
		endif()
		file(READ ${t} text)
		if(NOT text MATCHES "^(# [^\n]*\n[^\t\n]*\t[^\n]*)([0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F])([0-9A-F])")
			message(FATAL_ERROR "libs_emulator: ${t}: no value to corrupt")
		endif()
		set(low 0)
		if(CMAKE_MATCH_3 STREQUAL "0")
			set(low 1)
		endif()
		string(LENGTH "${CMAKE_MATCH_0}" n)
		string(SUBSTRING "${text}" ${n} -1 rest)
		file(WRITE ${t} "${CMAKE_MATCH_1}${CMAKE_MATCH_2}${low}${rest}")
	endif()
	set(only "")
	if(FILTER)
		foreach(w IN LISTS written)
			string(REGEX REPLACE "\\.txt$" "" w "${w}")
			list(APPEND only --only ${w})
		endforeach()
	endif()
	if((FILTER AND NOT written) OR (NOT written AND NOT EXISTS ${EXPECTED}))
		message(STATUS "libs_emulator: no program wrote cases, nothing to compare")
	else()
		execute_process(COMMAND ${RESULTS} libs-compare ${test}/captures ${EXPECTED} --set ${LIBSET}
			${only} RESULT_VARIABLE rc)
		if(rc)
			set(compare_failed "the cases of ${test}/captures differ from ${EXPECTED} (set ${LIBSET})")
		endif()
	endif()
endif()

# the dump of CAP_DUMP
set(dump_failed "")
if(CAP_DUMP)
	# the observations in blocks: from NAME.bin
	file(GLOB bins ${test}/captures/*.bin)
	if(bins)
		execute_process(COMMAND ${RESULTS} libs-dump ${test}/captures "${CAP_DUMP}"
			RESULT_VARIABLE rc)
		if(rc GREATER 1)
			set(dump_failed "LIBS_CAP_DUMP ${CAP_DUMP}: the dump from the block files failed (above)")
		endif()
	endif()
	file(GLOB dumps ${test}/captures/*.dump)
	if(dump_failed)
	elseif(NOT dumps)
		set(dump_failed "LIBS_CAP_DUMP ${CAP_DUMP}: no program wrote a dump (no bytes case or block of that key)")
	endif()
	foreach(d IN LISTS dumps)
		file(STRINGS ${d} lines LIMIT_COUNT 40)
		list(LENGTH lines n)
		list(JOIN lines "\n" lines)
		message("libs_emulator: the dump of ${CAP_DUMP}: ${d} (the first ${n} lines)\n${lines}")
	endforeach()
endif()

if(watch_failed)
	message(FATAL_ERROR "libs_emulator: ${watch_failed}")
endif()
if(dump_failed)
	message("libs_emulator: FAIL ${dump_failed}")
	message(FATAL_ERROR "libs_emulator: no dump of LIBS_CAP_DUMP (above)")
endif()
if(compare_failed)
	message(FATAL_ERROR "libs_emulator: ${compare_failed}")
endif()
if(NOT result EQUAL 0)
	message(FATAL_ERROR "libs_emulator: libs_tests returned ${result}")
endif()
# passed: the raw block files are not needed any more
if(EXPECTED AND NOT KEEP_BIN)
	file(GLOB bins ${test}/captures/*.bin)
	if(bins)
		file(REMOVE ${bins})
	endif()
endif()
