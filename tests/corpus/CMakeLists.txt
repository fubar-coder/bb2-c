# The corpus: blitz2-c against the original blitz2 2.10 (the compiler of
# the CD, docs/v2.10) in Amiberry, with the original and the C libraries
# (TODO.md item 9).
#   amiga/    the m68k programs of the runs: tedstub (blitz2:ted for batch
#             compiles), reqtrap / reqclose (requesters of the compiled
#             programs), reqclosetest (the gate of the requester tests),
#             rcput (rc=N in the session scripts on Kickstart 1.x)
#   tests*/   the test programs (text); tokenised here with tedsrc tok (the TED
#             core), which also refuses lines TED would cut
#   expect/   the expected outputs, one directory per pass (target
#             corpus-expect-update)
#   runner/   corpusrun (host side): the passes, see runner/README.md
add_subdirectory(amiga)

# --------------------------------------------------------- the sources
set(CORPUS_SRC ${CMAKE_CURRENT_BINARY_DIR}/src)
set(TED_TOKENS ${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt)
set(corpus_tokenised)
foreach(dir tests tests-runerr tests-req tests-serial)
	file(GLOB bas CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*.bas)
	set(corpus_${dir})
	foreach(b IN LISTS bas)
		get_filename_component(name ${b} NAME_WE)
		set(out ${CORPUS_SRC}/${dir}/${name}.bb2)
		add_custom_command(OUTPUT ${out}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${CORPUS_SRC}/${dir}
			COMMAND ${TEDSRC} tok ${TED_TOKENS} ${b} ${out}
			DEPENDS ${b} ${TED_TOKENS} ${TEDSRC} host_tools
			COMMENT "Tokenising ${dir}/${name}.bas"
			VERBATIM)
		list(APPEND corpus_${dir} ${out})
		list(APPEND corpus_tokenised ${out})
	endforeach()
endforeach()
add_custom_target(corpus_sources ALL DEPENDS ${corpus_tokenised})
# for the equivalence of blitz2-ab3 (tests/ab3)
set_property(GLOBAL PROPERTY CORPUS_SOURCES ${corpus_tokenised})

# ---------------------------------------------------------- the passes
# Every pass runs twice, as two ctest tests with their own pass files:
#
#   NAME           (ctest --preset all, also without the CD where the programs
#                  allow) the configurations clib = blitz2-c with the C
#                  libraries of this build and, with a prev build (BLITZ2_PREV,
#                  cmake/prev.cmake), prev = the compiler and libraries of the
#                  previous commit (its executable must be the one clib
#                  writes: a result the commit changed shows here).  The
#                  output of every configuration must be the committed expected
#                  output (expect/PASS/NAME.out, with the serial tests
#                  NAME.serial: the bytes sent over the serial port).
#   NAME_originals (label originals, ctest --preset originals; needs the CD)
#                  the comparison with the original: ref = the original
#                  compiler (2.10, BLITZ2/BLITZ2 of the CD) with the CD's
#                  libraries, port = blitz2-c with the CD's libraries
#                  (executable identical to ref's).  The output of both must
#                  be the expected output, except where notes.tsv lists the
#                  program (an intended deviation from the original, written
#                  by the fix commit): "noted #N", and a listed program that
#                  does not differ is stale.  Skipped without the CD.
#
# CORPUS_PARALLEL: emulator sessions at the same time (Amiberry's
# control channel is a socket in a private XDG_RUNTIME_DIR only on Linux;
# Windows: numbered named pipes, macOS: a fixed socket under /tmp - 1
# there).
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
	set(parallel_default 3)
else()
	set(parallel_default 1)
endif()
set(CORPUS_PARALLEL ${parallel_default} CACHE STRING
	"corpus: emulator sessions at the same time")
set(CORPUS_TIMEOUT 300 CACHE STRING "corpus: time limit of one emulator session (seconds)")
set(CD_BLITZ2 ${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2)
set(REF_BLITZ2 ${CD_BLITZ2}/BLITZ2)
# the committed expected outputs, one directory per pass
set(CORPUS_EXPECT ${CMAKE_CURRENT_SOURCE_DIR}/expect)
# the intended deviations from the original (tools/lib/corpusnotes): read
# by the runs of the originals
set(CORPUS_NOTES ${CMAKE_CURRENT_SOURCE_DIR}/notes.tsv)

# corpus_pass(NAME [RUNTIME_ERRORS] [SERIAL] LAUNCHER TARGET [GATE TARGET]
#             [SERIAL_SAME_AS PASS] SOURCES FILES...): the pass files + ctest
#   RUNTIME_ERRORS  runtime error checking on (else off)
#   SERIAL          a remote end on the host for the serial port
#                   (runner/serial.h)
#   LAUNCHER        the program that starts every test program (reqtrap or
#                   reqclose: they answer the programs' requesters)
#   GATE            run first; the programs only if it returns 0
#   SERIAL_SAME_AS  what the reference sent over the serial port must equal
#                   that of pass PASS (run after it; the originals only)
function(corpus_pass name)
	cmake_parse_arguments(PARSE_ARGV 1 arg "RUNTIME_ERRORS;SERIAL" "LAUNCHER;GATE;SERIAL_SAME_AS"
		"SOURCES")
	set(runtime_errors off)
	set(serial off)
	if(arg_RUNTIME_ERRORS)
		set(runtime_errors on)
	endif()
	if(arg_SERIAL)
		set(serial on)
	endif()
	foreach(kind all originals)
		if(kind STREQUAL "all")
			set(test ${name})
			set(configs 1)
		else()
			set(test ${name}_originals)
			set(configs 2)
		endif()
		set(work ${CMAKE_CURRENT_BINARY_DIR}/${test})
		set(text "# ${test}: generated by tests/corpus/CMakeLists.txt\n")
		string(APPEND text "test=${test}\n")
		string(APPEND text "work=${work}\nreport=${work}.txt\nruntime_errors=${runtime_errors}\n"
			"serial=${serial}\ntimeout=${CORPUS_TIMEOUT}\nparallel=${CORPUS_PARALLEL}\n"
			"ted=$<TARGET_FILE:tedstub>\nlauncher=$<TARGET_FILE:${arg_LAUNCHER}>\n"
			"defaultdbug=${CD_BLITZ2}/DEFAULTDBUG\n")
		if(arg_GATE)
			string(APPEND text "gate=$<TARGET_FILE:${arg_GATE}>\n")
		endif()
		if(kind STREQUAL "all")
			# the first configuration is judged against the expected files
			# like the others (corpus.h config=, expect_dir=)
			string(APPEND text
				"config=clib|ref|$<TARGET_FILE:blitz2>|${BLITZ2_DIR}/deflibs|${BLITZ2_DIR}/acidlibs\n")
			if(PREV_REQUESTED)
				string(APPEND text
					"config=prev|exe:clib|${PREV_BLITZ2_DIR}/blitz2|${PREV_BLITZ2_DIR}/deflibs|"
					"${PREV_BLITZ2_DIR}/acidlibs\n")
				string(APPEND text "${PREV_INTENDED_LINES}")
				set(configs 2)
			endif()
		else()
			if(arg_SERIAL_SAME_AS)
				string(APPEND text
					"serial_same_as=${CMAKE_CURRENT_BINARY_DIR}/${arg_SERIAL_SAME_AS}_originals/ref/vol/out\n")
			endif()
			string(APPEND text
				"config=ref|ref|${REF_BLITZ2}|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n"
				"config=port|exe|$<TARGET_FILE:blitz2>|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n"
				"notes=${CORPUS_NOTES}\nnotes_pass=${name}\n")
		endif()
		# the expected outputs (expect/PASS/NAME.out, NAME.serial): every
		# configuration's output must be these (target corpus-expect-update)
		string(APPEND text "expect_dir=${CORPUS_EXPECT}/${name}\n")
		# BB2: the CD - the tests load sounds and modules from it
		string(APPEND text "mount=BB2|${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1\n")
		foreach(s IN LISTS arg_SOURCES)
			string(APPEND text "source=${s}\n")
		endforeach()
		file(GENERATE OUTPUT ${work}.pass CONTENT "${text}")
		add_test(NAME ${test} COMMAND ${CORPUSRUN} pass ${work}.pass)
		# one session per configuration, one after the other if there are not
		# enough emulator slots free, each maybe waiting for one
		math(EXPR limit "${CORPUS_TIMEOUT} * ${configs} + 120 + ${configs} * ${EMULATION_QUEUE_WAIT}")
		if(kind STREQUAL "all")
			set_property(GLOBAL APPEND PROPERTY CORPUS_PASSES ${work}.pass)
			set_tests_properties(${test} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
				TIMEOUT ${limit})
		else()
			set_tests_properties(${test} PROPERTIES LABELS originals SKIP_RETURN_CODE 77
				TIMEOUT ${limit})
			if(arg_SERIAL_SAME_AS)
				set_tests_properties(${test} PROPERTIES DEPENDS ${arg_SERIAL_SAME_AS}_originals)
			endif()
		endif()
	endforeach()
endfunction()

# without runtime error checking: the programs of tests/
corpus_pass(corpus LAUNCHER reqtrap SOURCES ${corpus_tests})
# the TED A/B tests (tests/ted-ab) use the ref run of the originals: executables,
# defaults.xtra
set_tests_properties(corpus_originals PROPERTIES FIXTURES_SETUP corpus)
# with runtime error checking: tests/ and the programs that trigger runtime
# errors on purpose (tests-runerr/, they would crash without the checks)
corpus_pass(corpus_runerr RUNTIME_ERRORS LAUNCHER reqtrap
	SOURCES ${corpus_tests} ${corpus_tests-runerr})
# requester tests: programs that call blocking requesters run through
# reqclose (the programs say in ENV:REQCLOSE what to answer), behind the
# gate reqclosetest (if the closer does not work, they would block)
corpus_pass(corpus_req RUNTIME_ERRORS LAUNCHER reqclose GATE reqclosetest
	SOURCES ${corpus_tests-req})
# serial tests: the programs talk to a remote end on the host over the
# emulated serial port (runner/serial.h); the bytes they send over it are
# compared as well - without and with runtime error checking, and the
# reference must send the same in both
corpus_pass(corpus_serial SERIAL LAUNCHER reqtrap SOURCES ${corpus_tests-serial})
corpus_pass(corpus_serial_runerr RUNTIME_ERRORS SERIAL LAUNCHER reqtrap
	SERIAL_SAME_AS corpus_serial
	SOURCES ${corpus_tests-serial})

# the expected outputs written from the port's run (configuration clib:
# blitz2-c with the C libraries) - after a fix changed an output, or for a
# new source; the commit shows the change (git diff tests/corpus/expect).
# Not ALL: needs Amiberry and the CD (the passes mount BB2:)
get_property(passes GLOBAL PROPERTY CORPUS_PASSES)
set(update_commands)
foreach(pass IN LISTS passes)
	list(APPEND update_commands COMMAND ${CORPUSRUN} pass --update-expect clib ${pass})
endforeach()
add_custom_target(corpus-expect-update
	${update_commands}
	DEPENDS blitz2 acidlibs deflibs tedstub reqtrap reqclose reqclosetest corpus_sources
		host_tools
	USES_TERMINAL
	COMMENT "The corpus through the port's blitz2 and C libraries -> tests/corpus/expect"
	VERBATIM)

# --------------------------------------------- the CD (corpusrun cd, runner/)
# Every .bb2/.bb of the CD compiled (not run) as bb2's bb2corpus did: with
# the sources' .xtra, assigns to directories of the CD, in the directory of
# a relative INCLUDE.  The reference (target blitz2-cd-reference, not ALL:
# needs the CD and Amiberry) is the original blitz2 2.10, its table goes
# to tests/data/blitz2-cd.tsv; ctest corpus_cd compiles with blitz2-c and
# compares (result, error line and message, size, hash with the trap
# blocks zeroed).  CORPUS_CD_TIMEOUT: one session (all remaining jobs of a
# worker; the next session goes on).
set(CORPUS_CD_TIMEOUT 120 CACHE STRING "corpus_cd: time limit of one emulator session (seconds)")
set(CD_DIR ${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1)
set(CD_TABLE ${PROJECT_SOURCE_DIR}/tests/data/blitz2-cd.tsv)

# cd_pass(NAME BLITZ2 OUTPUT [REFERENCE]): the pass file
function(cd_pass name blitz2 output)
	set(work ${CMAKE_CURRENT_BINARY_DIR}/${name})
	set(text "# ${name}: generated by tests/corpus/CMakeLists.txt\n")
	string(APPEND text "work=${work}\noutput=${output}\ncd=${CD_DIR}\n"
		"tokens=${TED_TOKENS}\nblitz2=${blitz2}\n"
		"deflibs=${CD_BLITZ2}/DEFLIBS\nacidlibs=${CD_BLITZ2}/ACIDLIBS\n"
		"defaultdbug=${CD_BLITZ2}/DEFAULTDBUG\nted=$<TARGET_FILE:tedstub>\n"
		"timeout=${CORPUS_CD_TIMEOUT}\nparallel=${CORPUS_PARALLEL}\n")
	# volumes the .xtra residents and the INCLUDEs use (bb2corpus ASSIGNS);
	# blitz2: is the compiler directory, then the CD's
	foreach(a blitz2|BLITZ2 blitzlibs|EXTRAS/BLITZLIBS blitzlibs|BUM10A/BLITZLIBS
			libincs|EXTRAS/BLITZLIBS blitzincs|EXTRAS/BLITZLIBS
			includes|BUI5/MAKEFILE/INCLUDES)
		string(APPEND text "assign=${a}\n")
	endforeach()
	if(ARGN)
		string(APPEND text "reference=${ARGN}\nreport=${work}.txt\n")
	endif()
	file(GENERATE OUTPUT ${work}.pass CONTENT "${text}")
endfunction()

cd_pass(cd-reference ${REF_BLITZ2} ${CMAKE_CURRENT_BINARY_DIR}/cd-reference.tsv)
add_custom_target(blitz2-cd-reference
	COMMAND ${CORPUSRUN} cd ${CMAKE_CURRENT_BINARY_DIR}/cd-reference.pass
	COMMAND ${RESULTS} cd-reference ${CMAKE_CURRENT_BINARY_DIR}/cd-reference.tsv ${CD_TABLE} ${CD_TABLE}
	DEPENDS tedstub host_tools
	USES_TERMINAL
	COMMENT "The CD through the original blitz2 2.10 -> tests/data/blitz2-cd.tsv (note rows kept)"
	VERBATIM)

# the table from the port's result (after a fix changed a result): the note
# rows keep their note, or lose it where the port's result (executable or
# error line) is the CD's again
cd_pass(cd-update $<TARGET_FILE:blitz2> ${CMAKE_CURRENT_BINARY_DIR}/cd-update.tsv)
add_custom_target(blitz2-cd-update
	COMMAND ${CORPUSRUN} cd ${CMAKE_CURRENT_BINARY_DIR}/cd-update.pass
	COMMAND ${RESULTS} cd-update ${CMAKE_CURRENT_BINARY_DIR}/cd-update.tsv ${CD_TABLE} ${CD_TABLE}
	DEPENDS blitz2 tedstub host_tools
	USES_TERMINAL
	COMMENT "The CD through the port's blitz2 -> tests/data/blitz2-cd.tsv (note rows kept)"
	VERBATIM)

cd_pass(corpus_cd $<TARGET_FILE:blitz2> ${CMAKE_CURRENT_BINARY_DIR}/corpus_cd.tsv ${CD_TABLE})
add_test(NAME corpus_cd COMMAND ${CORPUSRUN} cd ${CMAKE_CURRENT_BINARY_DIR}/corpus_cd.pass)
math(EXPR limit "3600 + ${EMULATION_QUEUE_WAIT}")
set_tests_properties(corpus_cd PROPERTIES LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
# the same with blitz2-ab3 (BB2 mode: the editor is tedstub), against the
# same table - spec ab3-compat/bb2-equivalence for the CD's sources
cd_pass(corpus_cd_ab3 $<TARGET_FILE:blitz2-ab3> ${CMAKE_CURRENT_BINARY_DIR}/corpus_cd_ab3.tsv ${CD_TABLE})
add_test(NAME corpus_cd_ab3 COMMAND ${CORPUSRUN} cd ${CMAKE_CURRENT_BINARY_DIR}/corpus_cd_ab3.pass)
set_tests_properties(corpus_cd_ab3 PROPERTIES LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})

# ------------------------------------------- the notes with broken input
# Every check of notes.tsv needs a test on deliberately broken input
# (tests/corpus/selftest): one small pass (the source t01_int; ref and port
# for the three that run the emulator, label originals) with a notes file or
# an expected output that is wrong on purpose.  The first three need no
# emulator: the notes are read before any session.
#   a notes row that is malformed (unknown pass, no issue, twice) or a
#   missing notes file: exit status 2 and the message
#   a stale note (the program does not differ): exit status 1, STALE
#   an unlisted difference (the expected output is wrong): exit status 1,
#   OUTPUT DIFFERS
#   the same difference listed: exit status 0, "noted #1"
set(selftest ${CMAKE_CURRENT_SOURCE_DIR}/selftest)
function(corpus_notes_selftest name want regex expect_dir notes configs)
	set(work ${CMAKE_CURRENT_BINARY_DIR}/${name})
	string(CONCAT text "# ${name}: generated by tests/corpus/CMakeLists.txt\ntest=${name}\n"
		"work=${work}\nreport=${work}.txt\nruntime_errors=off\ntimeout=${CORPUS_TIMEOUT}\n"
		"parallel=${CORPUS_PARALLEL}\nted=$<TARGET_FILE:tedstub>\n"
		"launcher=$<TARGET_FILE:reqtrap>\ndefaultdbug=${CD_BLITZ2}/DEFAULTDBUG\n"
		"config=ref|ref|${REF_BLITZ2}|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n"
		"config=port|exe|$<TARGET_FILE:blitz2>|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n"
		"expect_dir=${expect_dir}\nnotes=${notes}\nnotes_pass=corpus\n"
		"source=${CORPUS_SRC}/tests/t01_int.bb2\n")
	file(GENERATE OUTPUT ${work}.pass CONTENT "${text}")
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DRUNNER=${CORPUSRUN} -DPASS=${work}.pass
		-DWANT=${want} "-DREGEX=${regex}" -P ${CMAKE_CURRENT_SOURCE_DIR}/want-report.cmake)
	if(configs)
		math(EXPR limit "${CORPUS_TIMEOUT} * 2 + 120 + 2 * ${EMULATION_QUEUE_WAIT}")
		set_tests_properties(${name} PROPERTIES LABELS originals SKIP_RETURN_CODE 77
			TIMEOUT ${limit})
	endif()
endfunction()
corpus_notes_selftest(corpus_notes_unknown_pass 2 "notes-unknown-pass.tsv: line 2: unknown pass .ted_ab."
	${CORPUS_EXPECT}/corpus ${selftest}/notes-unknown-pass.tsv OFF)
corpus_notes_selftest(corpus_notes_no_issue 2 "notes-no-issue.tsv: line 1: the note must read"
	${CORPUS_EXPECT}/corpus ${selftest}/notes-no-issue.tsv OFF)
corpus_notes_selftest(corpus_notes_twice 2 "notes-twice.tsv: line 2: corpus:t01_int is listed twice"
	${CORPUS_EXPECT}/corpus ${selftest}/notes-twice.tsv OFF)
corpus_notes_selftest(corpus_notes_missing 2 "cannot read the notes [^\n]*no-such-notes.tsv"
	${CORPUS_EXPECT}/corpus ${selftest}/no-such-notes.tsv OFF)
# a stale note: t01_int does not differ in either configuration
corpus_notes_selftest(corpus_originals_notes_stale 1
	"STALE note corpus:t01_int \\(#1: selftest row, line 2 of [^\n]*notes-t01.tsv\\)[^\n]*\n[^\n]*1 checks, 1 with differences"
	${CORPUS_EXPECT}/corpus ${selftest}/notes-t01.tsv ON)
# an expected output that is wrong: both configurations differ, and the
# program is not listed (notes-empty.tsv has no row)
corpus_notes_selftest(corpus_originals_notes_unlisted 1
	"ref: OUTPUT DIFFERS; port: exe identical, OUTPUT DIFFERS\n  ref: [^\n]*wrong/t01_int.out line 1: expected [^\n]*\n  port: [^\n]*\n1 checks, 1 with differences"
	${selftest}/wrong ${selftest}/notes-empty.tsv ON)
# the same difference listed: noted, the pass is fine
corpus_notes_selftest(corpus_originals_notes_noted 0
	"ref: OUTPUT DIFFERS \\(noted #1\\); port: exe identical, OUTPUT DIFFERS \\(noted #1\\)\n.*noted corpus:t01_int differs from the original: #1: selftest row\n1 checks, 0 with differences"
	${selftest}/wrong ${selftest}/notes-t01.tsv ON)
