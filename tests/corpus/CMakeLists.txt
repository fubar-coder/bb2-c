# The corpus: blitz2-c against the original blitz2 2.10 (the compiler of
# the CD, docs/v2.10) in Amiberry, with the original and the C libraries
# (TODO.md item 9).
#   amiga/    the m68k programs of the runs: tedstub (blitz2:ted for batch
#             compiles), reqtrap / reqclose (requesters of the compiled
#             programs), reqclosetest (the gate of the requester tests),
#             rcput (rc=N in the session scripts on Kickstart 1.x)
#   tests*/   the test programs (text); tokenised here with tedsrc tok (the TED
#             core), which also refuses lines TED would cut
#   expect/   the expected data, one directory per pass (tools/lib/corpusexpect:
#             NAME.out, NAME.serial; for a program a fix makes differ from
#             the original the pairs NAME.orig.EXT / NAME.fixed-N.EXT;
#             targets corpus-expect-record and corpus-expect-update)
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
#                  (executable identical to ref's), orig-clib = the original
#                  compiler with the C libraries of this build (are they
#                  compatible with the original compiler?).  The output of
#                  all three must be the expected data that applies to
#                  them: for a program a fix makes differ from the original
#                  (the pair NAME.orig.out / NAME.fixed-N.out) ref the
#                  [orig] file, port and orig-clib either, while clib and
#                  prev in NAME the [fixed #N] one; an executable pair
#                  (NAME.orig.exe / NAME.fixed-N.exe, SHA-256) replaces the
#                  comparison of port's executable with ref's.  Skipped
#                  without the CD.
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
# the committed expected data, one directory per pass
set(CORPUS_EXPECT ${CMAKE_CURRENT_SOURCE_DIR}/expect)

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
			set(configs 3)
		endif()
		set(work ${CMAKE_CURRENT_BINARY_DIR}/${test})
		set(text "# ${test}: generated by tests/corpus/CMakeLists.txt\n")
		string(APPEND text "test=${test}\n")
		string(APPEND text "work=${work}\nreport=${work}.txt\nruntime_errors=${runtime_errors}\n"
			"serial=${serial}\ntimeout=${CORPUS_TIMEOUT}\nparallel=${CORPUS_PARALLEL}\n"
			"ted=$<TARGET_FILE:tedstub>\nlauncher=$<TARGET_FILE:${arg_LAUNCHER}>\n")
		# blitz2:defaultdbug only in the originals runs (as the original
		# installation has it): the programs are executables (CREATE
		# EXECUTABLE), which never contain the module, also not with runtime
		# error checking (runerrlib + rundebuglib), so all names none
		if(NOT kind STREQUAL "all")
			string(APPEND text "defaultdbug=${CD_BLITZ2}/DEFAULTDBUG\n")
		endif()
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
				"config=orig-clib|output|${REF_BLITZ2}|${BLITZ2_DIR}/deflibs|${BLITZ2_DIR}/acidlibs\n")
		endif()
		# the expected data (expect/PASS): every configuration's output must
		# be the file that applies to it (targets corpus-expect-record and
		# corpus-expect-update)
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
# the TED A/B tests (tests/ted-ab) use its executables and defaults.xtra:
# ted_ab those of the clib run (fixture corpus), ted_ab_originals those of
# the ref run (fixture corpus_originals)
set_tests_properties(corpus PROPERTIES FIXTURES_SETUP corpus)
set_tests_properties(corpus_originals PROPERTIES FIXTURES_SETUP corpus_originals)
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

# ctest blitz2_cd_reference (label originals, preset originals): the same
# run of the original, the table made from it must be the committed one
# (cd-reference-check.cmake); the check with broken input: a fresh run
# with another hash than the table (no emulator), and the same one passes
set(cd_check ${CMAKE_CURRENT_SOURCE_DIR}/cd-reference-check.cmake)
add_test(NAME blitz2_cd_reference COMMAND ${CMAKE_COMMAND} -DCORPUSRUN=${CORPUSRUN}
	-DRESULTS=${RESULTS} -DPASS=${CMAKE_CURRENT_BINARY_DIR}/cd-reference.pass
	-DFRESH=${CMAKE_CURRENT_BINARY_DIR}/cd-reference.tsv -DTABLE=${CD_TABLE}
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/cd-reference-check.tsv -P ${cd_check})
math(EXPR limit "3600 + ${EMULATION_QUEUE_WAIT}")
set_tests_properties(blitz2_cd_reference PROPERTIES LABELS originals SKIP_RETURN_CODE 77
	TIMEOUT ${limit})
foreach(t same|cd-table.tsv wrong|cd-fresh-wrong.tsv)
	string(REPLACE "|" ";" t "${t}")
	list(GET t 0 name)
	list(GET t 1 fresh)
	add_test(NAME blitz2_cd_reference_check_${name} COMMAND ${CMAKE_COMMAND} -DNO_RUN=ON
		-DRESULTS=${RESULTS} -DFRESH=${CMAKE_CURRENT_SOURCE_DIR}/selftest/${fresh}
		-DTABLE=${CMAKE_CURRENT_SOURCE_DIR}/selftest/cd-table.tsv
		-DOUT=${CMAKE_CURRENT_BINARY_DIR}/cd-reference-check-${name}.tsv -P ${cd_check})
endforeach()
set_tests_properties(blitz2_cd_reference_check_same PROPERTIES
	PASS_REGULAR_EXPRESSION "cd-table.tsv[ \n]+is the original's")
set_tests_properties(blitz2_cd_reference_check_wrong PROPERTIES
	PASS_REGULAR_EXPRESSION "the original's run differs[ \n]+from[ \n]+[^ \n]*cd-table.tsv")

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

# ---------------------------------- the expected data with broken input
# Every check of the expected data (tools/lib/corpusexpect) needs a test on
# deliberately broken input: one small pass (the source t01_int) against an
# expected directory made here, in the build directory, from the committed
# expected output of t01_int (right) and selftest/wrong/t01_int.out (one
# line wrong).  The malformed ones need no emulator: the directory is read
# before any session (exit status 2 and the file names).
#   corpus_expect_fixed_regressed  clib gives the [orig] value: the
#       [fixed #1] file fails (label emulator: no CD needed)
#   corpus_originals_orig_changed  ref differs from the [orig] file, port
#       matches the [fixed #1] one (label originals)
#   corpus_originals_exe_unlisted  port's executable (here with the C
#       libraries) differs from ref's without an exe pair (label originals)
set(selftest ${CMAKE_CURRENT_SOURCE_DIR}/selftest)
set(selftest_expect ${CMAKE_CURRENT_BINARY_DIR}/selftest-expect)
file(READ ${CORPUS_EXPECT}/corpus/t01_int.out t01_right)
file(READ ${selftest}/wrong/t01_int.out t01_wrong)
file(REMOVE_RECURSE ${selftest_expect})
# corpus_selftest_dir(NAME FILE|CONTENT...): the directory NAME, CONTENT
# right, wrong or a text
function(corpus_selftest_dir name)
	file(MAKE_DIRECTORY ${selftest_expect}/${name})
	foreach(f IN LISTS ARGN)
		string(REPLACE "|" ";" f "${f}")
		list(GET f 0 file)
		list(GET f 1 what)
		if(what STREQUAL "right")
			set(what "${t01_right}")
		elseif(what STREQUAL "wrong")
			set(what "${t01_wrong}")
		endif()
		file(WRITE ${selftest_expect}/${name}/${file} "${what}")
	endforeach()
endfunction()
corpus_selftest_dir(both t01_int.out|right t01_int.orig.out|wrong t01_int.fixed-1.out|right)
corpus_selftest_dir(two-fixed t01_int.orig.out|wrong t01_int.fixed-1.out|right
	t01_int.fixed-2.out|right)
corpus_selftest_dir(qualifier t01_int.new.out|right)
corpus_selftest_dir(equal t01_int.orig.out|right t01_int.fixed-1.out|right)
corpus_selftest_dir(lone t01_int.fixed-1.out|right)
corpus_selftest_dir(missing t02_float.out|right)
corpus_selftest_dir(exe-plain t01_int.out|right
	"t01_int.exe|0000000000000000000000000000000000000000000000000000000000000000\n")
corpus_selftest_dir(regressed t01_int.orig.out|right t01_int.fixed-1.out|wrong)
corpus_selftest_dir(orig-changed t01_int.orig.out|wrong t01_int.fixed-1.out|right)
corpus_selftest_dir(unlisted t01_int.out|right)

# corpus_expect_selftest(NAME WANT REGEX DIR CONFIGS LABEL): the pass with
# the configurations CONFIGS (clib or ref-port or ref-port-clib)
function(corpus_expect_selftest name want regex dir configs label)
	set(work ${CMAKE_CURRENT_BINARY_DIR}/${name})
	string(CONCAT text "# ${name}: generated by tests/corpus/CMakeLists.txt\ntest=${name}\n"
		"work=${work}\nreport=${work}.txt\nruntime_errors=off\ntimeout=${CORPUS_TIMEOUT}\n"
		"parallel=${CORPUS_PARALLEL}\nted=$<TARGET_FILE:tedstub>\n"
		"launcher=$<TARGET_FILE:reqtrap>\n")
	if(configs STREQUAL "clib")
		string(APPEND text
			"config=clib|ref|$<TARGET_FILE:blitz2>|${BLITZ2_DIR}/deflibs|${BLITZ2_DIR}/acidlibs\n")
		set(n 1)
	else()
		string(APPEND text "defaultdbug=${CD_BLITZ2}/DEFAULTDBUG\n"
			"config=ref|ref|${REF_BLITZ2}|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n")
		if(configs STREQUAL "ref-port-clib")
			# port with the C libraries: another executable than ref's
			string(APPEND text
				"config=port|exe|$<TARGET_FILE:blitz2>|${BLITZ2_DIR}/deflibs|${BLITZ2_DIR}/acidlibs\n")
		else()
			string(APPEND text
				"config=port|exe|$<TARGET_FILE:blitz2>|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n")
		endif()
		set(n 2)
	endif()
	string(APPEND text "expect_dir=${selftest_expect}/${dir}\nsource=${CORPUS_SRC}/tests/t01_int.bb2\n")
	file(GENERATE OUTPUT ${work}.pass CONTENT "${text}")
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DRUNNER=${CORPUSRUN} -DPASS=${work}.pass
		-DWANT=${want} "-DREGEX=${regex}" -P ${CMAKE_CURRENT_SOURCE_DIR}/want-report.cmake)
	if(label)
		math(EXPR limit "${CORPUS_TIMEOUT} * ${n} + 120 + ${n} * ${EMULATION_QUEUE_WAIT}")
		set_tests_properties(${name} PROPERTIES LABELS ${label} SKIP_RETURN_CODE 77
			TIMEOUT ${limit})
	endif()
endfunction()
corpus_expect_selftest(corpus_expect_malformed_both 2
	"selftest-expect/both/t01_int.out and t01_int.orig.out: an unqualified and a qualified file"
	both clib "")
corpus_expect_selftest(corpus_expect_malformed_two_fixed 2
	"selftest-expect/two-fixed/t01_int.fixed-1.out and t01_int.fixed-2.out: two fixed files"
	two-fixed clib "")
corpus_expect_selftest(corpus_expect_malformed_qualifier 2
	"selftest-expect/qualifier/t01_int.new.out: unknown qualifier .new." qualifier clib "")
corpus_expect_selftest(corpus_expect_malformed_equal 2
	"selftest-expect/equal/t01_int.orig.out and t01_int.fixed-1.out: a pair of equal files"
	equal clib "")
corpus_expect_selftest(corpus_expect_malformed_lone 2
	"selftest-expect/lone/t01_int.fixed-1.out: without its .orig. file" lone clib "")
corpus_expect_selftest(corpus_expect_malformed_missing 2
	"no [^\n]*selftest-expect/missing/t01_int.out \\(the expected output" missing clib "")
corpus_expect_selftest(corpus_expect_malformed_exe 2
	"selftest-expect/exe-plain/t01_int.exe: an executable is only a pair" exe-plain clib "")
corpus_expect_selftest(corpus_expect_fixed_regressed 1
	"clib: OUTPUT DIFFERS\n  clib: [^\n]*regressed/t01_int.fixed-1.out line 1: expected \"667 1801 -699678 -3\", got \"667 1801 -699678 -2\"\n1 checks, 1 with differences"
	regressed clib emulator)
corpus_expect_selftest(corpus_originals_orig_changed 1
	"ref: OUTPUT DIFFERS; port: exe identical, output as expected \\(matched \\[fixed #1\\]\\)\n  ref: [^\n]*orig-changed/t01_int.orig.out line 1: expected \"667 1801 -699678 -3\", got \"667 1801 -699678 -2\"\n1 checks, 1 with differences"
	orig-changed ref-port originals)
corpus_expect_selftest(corpus_originals_exe_unlisted 1
	"ref: output as expected; port: EXE DIFFERS, output as expected\n1 checks, 1 with differences"
	unlisted ref-port-clib originals)
