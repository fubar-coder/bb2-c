# The bug-orig tests with a Blitz program (README, "Tests"): a program that
# triggers a bug of the original, compiled and run in Amiberry by the
# original blitz2 1.95 with the CD's libraries (ref) and by the port (port:
# blitz2-c with the CD's libraries, clib: blitz2-c with the C libraries).
# The expectation is the correct behaviour: the test fails as long as the
# port reproduces the bug, and it is broken if the original does not show
# the bug (corpus.h expect=, bugorig.h).  The reference stays 1.95 (the
# build of blitz2.s), not 2.10 like the corpus: 2.10 fixed #2 and has all
# other bugs unchanged (docs/v2.10), so the test of #2 stays valid.
#
# One test = two files in an area directory (compiler/, libs/ ...):
#   AREA/ID.bas      the program (text, tokenised here like TED would save
#                    it); its header may set
#                      ; @pass KEY=VALUE    a line of the pass file
#                                           (tests/corpus/runner/corpus.h),
#                                           @VAR@ replaced (see below)
#                      ; @configs TAG...    the configurations judged
#                                           (port clib, default both)
#                      ; @machine NAME      the emulated machine (pass
#                                           file machine=, corpus.h:
#                                           a1200 default, a500 = 68000
#                                           with Kickstart 1.3,
#                                           a500-68010); on Kickstart
#                                           1.x the compiler and the
#                                           program run there too (no
#                                           gate, no @INJECT@/@WBRUN@:
#                                           they need V36; README,
#                                           "Tests")
#                      ; @rwe NAME...       RWE userlibs (libs/userlibs/
#                                           rwe/NAME.c) in blitz2:userlibs/
#                                           rwe/: ref (blitz2 2.10 of the
#                                           CD) gets the reference
#                                           (RWE_LIBS: the CD's binary or
#                                           the reference build, then the
#                                           test needs the fixture
#                                           rwe_refbuild), port and clib
#                                           the C build; clib with the
#                                           acidlibs/deflibs of the
#                                           library's variant, the source
#                                           tokenised with the tokens of
#                                           blitz2-c, that variant and
#                                           the libraries (blitztokens)
#   AREA/ID.expect   the correct behaviour (sections, bugorig.h)
# ID is the bug ID of the issue; ctest bug_orig_ID (- as _), labels
# bug-orig and emulator, skipped (77) without Amiberry or the CD.  The
# files are found by a glob: adding a test touches no list.
#   @VAR@ in @pass: USERLIB (amiga/userlib.c: a library for
#   blitz2:userlibs), INJECT (amiga/inject.c: faults, counts and input;
#   for the compiler with
#   wrap=@INJECT@|..., for the programs with runwrap=@INJECT@|...),
#   WBRUN (amiga/wbrun.c: started like from the Workbench, wrap=@WBRUN@ or
#   runwrap=@WBRUN@),
#   CD_BLITZ2 (the CD's BLITZ2 directory), TESTDATA
#   (the generated test data, see below; use
#   file=DEST|@TESTDATA@/NAME, the program reads CORPUS:DEST)
#
# Test data (sounds, pictures, modules) is generated, not checked in:
#   AREA/NAME.spec   a description for tools/mktestdata (bytes, IFF
#                    chunks, labels; tools/lib/mktestdata/mktestdata.h),
#                    built to testdata/NAME in the build directory
#   AREA/NAME.src    a further Blitz source as text (e.g. the source of a
#                    resident, tedstub resident=), tokenised like ID.bas
#                    to testdata/NAME
# NAME is unique over all areas and begins with the ID of the test that
# uses it (e.g. libs/ID.8svx.spec).
add_subdirectory(amiga)

set(BUG_ORIG_TIMEOUT 90 CACHE STRING "bug-orig: time limit of one emulator session (seconds)")
set(CD_BLITZ2 ${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2)
set(REF_BLITZ2 ${CMAKE_BINARY_DIR}/programs/blitz2/ref/blitz2)
set(USERLIB ${CMAKE_CURRENT_BINARY_DIR}/amiga/bugorig_userlib.obj)
set(INJECT $<TARGET_FILE:inject>)
set(WBRUN $<TARGET_FILE:wbrun>)
set(TESTDATA ${CMAKE_CURRENT_BINARY_DIR}/testdata)
# the configurations of a test run at the same time as the corpus passes
# do (CORPUS_PARALLEL, tests/corpus/CMakeLists.txt)
set(parallel ${CORPUS_PARALLEL})

# the token list: TED's plus the test library's
set(ted_tokens ${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt)
set(lib_tokens ${CMAKE_CURRENT_SOURCE_DIR}/amiga/userlib.tokens)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${ted_tokens} ${lib_tokens})
file(READ ${ted_tokens} t1)
file(READ ${lib_tokens} t2)
set(tokens ${CMAKE_CURRENT_BINARY_DIR}/tokens.txt)
file(CONFIGURE OUTPUT ${tokens} CONTENT "${t1}${t2}" @ONLY)

# bug_orig_test(BAS NAME [WANT RC]): tokenise, pass file, ctest NAME.
# WANT: the test passes if corpusrun returns RC (the self test).
set(bug_orig_sources)
set(bug_orig_deps)
function(bug_orig_test bas name)
	cmake_parse_arguments(PARSE_ARGV 2 arg "" "WANT" "")
	get_filename_component(id ${bas} NAME_WE)
	get_filename_component(dir ${bas} DIRECTORY)
	set(expect ${dir}/${id}.expect)
	if(NOT EXISTS ${expect})
		message(FATAL_ERROR "bug-orig: ${bas} has no ${id}.expect")
	endif()
	set(configs port clib)
	set(rwe "")
	set(extra "")
	# the header lines begin with ';' - the list separator: replaced first.
	# They make the pass file: a changed header configures again.
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${bas})
	file(READ ${bas} content)
	string(REPLACE ";" "<semicolon>" content "${content}")
	string(REGEX MATCHALL "(^|\n)<semicolon> @[^\n]*" header "${content}")
	foreach(h IN LISTS header)
		string(STRIP "${h}" h)
		if(h MATCHES "^<semicolon> @configs (.+)$")
			separate_arguments(configs UNIX_COMMAND "${CMAKE_MATCH_1}")
		elseif(h MATCHES "^<semicolon> @machine ([a-z0-9-]+)$")
			string(APPEND extra "machine=${CMAKE_MATCH_1}\n")
		elseif(h MATCHES "^<semicolon> @rwe (.+)$")
			separate_arguments(rwe UNIX_COMMAND "${CMAKE_MATCH_1}")
		elseif(h MATCHES "^<semicolon> @pass ([a-z_]+=.*)$")
			string(REPLACE "<semicolon>" ";" line "${CMAKE_MATCH_1}")
			string(CONFIGURE "${line}" line @ONLY)
			string(APPEND extra "${line}\n")
		else()
			message(FATAL_ERROR "bug-orig: ${bas}: bad header line: ${h}")
		endif()
	endforeach()

	# RWE userlibs: per configuration the reference or the C build, the
	# variant's libraries for clib, a token list with the libraries
	set(clib_dir ${BLITZ2_DIR})
	set(ref_blitz2 ${REF_BLITZ2})
	set(fixtures "")
	set(tok ${tokens})
	set(tokdeps "")
	if(rwe)
		# the reference compiler: 2.10 (1.95 never loads a library of
		# blitz2:userlibs/DIR, #2)
		set(ref_blitz2 ${CD_BLITZ2}/BLITZ2)
		set(variant "")
		set(objs "")
		foreach(n IN LISTS rwe)
			get_property(info GLOBAL PROPERTY RWE_LIB_${n})
			if(NOT info)
				message(FATAL_ERROR "bug-orig: ${bas}: @rwe ${n}: no RWE userlib of that name "
					"(libs/userlibs/rwe/${n}.c, RWE_LIBS)")
			endif()
			string(REPLACE "|" ";" info "${info}")
			list(GET info 0 obj)
			list(GET info 1 reffile)
			list(GET info 2 fixture)
			list(GET info 3 v)
			list(GET info 4 vout)
			if(variant AND NOT v STREQUAL variant)
				message(FATAL_ERROR "bug-orig: ${bas}: @rwe: ${n} is built for the variant ${v}, "
					"the others for ${variant}")
			endif()
			set(variant ${v})
			set(clib_dir ${vout})
			string(APPEND extra "file=blitz2/userlibs/rwe/${n}.obj|${reffile}|ref\n")
			foreach(c IN LISTS configs)
				string(APPEND extra "file=blitz2/userlibs/rwe/${n}.obj|${obj}|${c}\n")
			endforeach()
			list(APPEND fixtures ${fixture})
			list(APPEND objs ${obj})
			list(APPEND tokdeps ${v}-${n}_obj)
		endforeach()
		list(APPEND tokdeps ${variant}-acidlibs ${variant}-deflibs)
		set(tok ${CMAKE_CURRENT_BINARY_DIR}/tokens/${id}.txt)
		add_custom_command(OUTPUT ${tok}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/tokens
			COMMAND ${BLITZTOKENS} -o ${tok} $<TARGET_FILE:blitz2> ${clib_dir}/acidlibs
				${clib_dir}/deflibs ${objs}
			DEPENDS ${objs} ${clib_dir}/acidlibs ${clib_dir}/deflibs blitz2 ${BLITZTOKENS}
				host_tools
			COMMENT "bug-orig ${id}: token list with the RWE userlibs"
			VERBATIM)
		set(bug_orig_deps ${bug_orig_deps} ${tokdeps} PARENT_SCOPE)
	endif()
	set(src ${CMAKE_CURRENT_BINARY_DIR}/src/${id}.bb2)
	add_custom_command(OUTPUT ${src}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/src
		COMMAND ${TEDTOK} ${tok} ${bas} ${src}
		DEPENDS ${bas} ${tok} ${TEDTOK} host_tools
		COMMENT "Tokenising bug-orig ${id}.bas"
		VERBATIM)
	set(bug_orig_sources ${bug_orig_sources} ${src} PARENT_SCOPE)

	set(work ${CMAKE_CURRENT_BINARY_DIR}/${id})
	set(text "# ${name}: generated by tests/bug-orig/CMakeLists.txt from ${bas}\n")
	string(APPEND text "work=${work}\nreport=${work}.txt\nruntime_errors=off\n"
		"timeout=${BUG_ORIG_TIMEOUT}\nparallel=${parallel}\n"
		"ted=$<TARGET_FILE:tedstub>\nlauncher=$<TARGET_FILE:reqtrap>\n"
		"rcput=$<TARGET_FILE:rcput>\ndefaultdbug=${CD_BLITZ2}/DEFAULTDBUG\n"
		"config=ref|ref|${ref_blitz2}|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n")
	foreach(c IN LISTS configs)
		if(c STREQUAL "port")
			string(APPEND text
				"config=port|exe|$<TARGET_FILE:blitz2>|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n")
		elseif(c STREQUAL "clib")
			string(APPEND text
				"config=clib|output|$<TARGET_FILE:blitz2>|${clib_dir}/deflibs|${clib_dir}/acidlibs\n")
		else()
			message(FATAL_ERROR "bug-orig: ${bas}: unknown configuration ${c} (port, clib)")
		endif()
	endforeach()
	string(APPEND text "mount=BB2|${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1\n"
		"source=${src}\nexpect=${expect}\n${extra}")
	file(GENERATE OUTPUT ${work}.pass CONTENT "${text}")

	if(DEFINED arg_WANT)
		add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DCORPUSRUN=${CORPUSRUN}
			-DPASS=${work}.pass -DWANT=${arg_WANT} -P ${CMAKE_CURRENT_SOURCE_DIR}/want.cmake)
		set(labels emulator)
	else()
		add_test(NAME ${name} COMMAND ${CORPUSRUN} ${work}.pass)
		set(labels "bug-orig;emulator")
	endif()
	# a session per configuration (ref and the judged ones), one after the
	# other if there are not enough emulator slots free, each maybe
	# waiting for one
	list(LENGTH configs nsessions)
	math(EXPR nsessions "${nsessions} + 1")
	math(EXPR limit
		"${BUG_ORIG_TIMEOUT} * ${nsessions} + 60 + ${nsessions} * ${EMULATION_QUEUE_WAIT}")
	set_tests_properties(${name} PROPERTIES LABELS "${labels}" SKIP_RETURN_CODE 77
		TIMEOUT ${limit})
	list(REMOVE_DUPLICATES fixtures)
	list(REMOVE_ITEM fixtures "")
	if(fixtures)
		set_tests_properties(${name} PROPERTIES FIXTURES_REQUIRED "${fixtures}")
	endif()
endfunction()

# the test data: AREA/NAME.spec -> testdata/NAME
set(testdata_files)
file(GLOB specs CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*/*.spec)
foreach(spec IN LISTS specs)
	get_filename_component(name ${spec} NAME)
	string(REGEX REPLACE "\\.spec$" "" name "${name}")
	set(out ${TESTDATA}/${name})
	if(out IN_LIST testdata_files)
		message(FATAL_ERROR "bug-orig: test data ${name} described twice (${spec})")
	endif()
	add_custom_command(OUTPUT ${out}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${TESTDATA}
		COMMAND ${MKTESTDATA} ${spec} ${out}
		DEPENDS ${spec} ${MKTESTDATA} host_tools
		COMMENT "Generating bug-orig test data ${name}"
		VERBATIM)
	list(APPEND testdata_files ${out})
endforeach()

# further sources: AREA/NAME.src (text like ID.bas) -> testdata/NAME,
# tokenised like the tests (e.g. the source of a resident, tedstub
# resident=)
file(GLOB srcs CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*/*.src)
foreach(s IN LISTS srcs)
	get_filename_component(name ${s} NAME)
	string(REGEX REPLACE "\\.src$" "" name "${name}")
	set(out ${TESTDATA}/${name})
	if(out IN_LIST testdata_files)
		message(FATAL_ERROR "bug-orig: test data ${name} described twice (${s})")
	endif()
	add_custom_command(OUTPUT ${out}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${TESTDATA}
		COMMAND ${TEDTOK} ${tokens} ${s} ${out}
		DEPENDS ${s} ${tokens} ${TEDTOK} host_tools
		COMMENT "Tokenising bug-orig source ${name}"
		VERBATIM)
	list(APPEND testdata_files ${out})
endforeach()

file(GLOB tests CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*/*.bas)
foreach(bas IN LISTS tests)
	get_filename_component(dir ${bas} DIRECTORY)
	get_filename_component(area ${dir} NAME)
	if(area STREQUAL "selftest")
		continue()
	endif()
	get_filename_component(id ${bas} NAME_WE)
	string(REPLACE "-" "_" name "bug_orig_${id}")
	bug_orig_test(${bas} ${name})
endforeach()

# the self test: an expectation equal to what the original does must be
# reported as broken (exit status 3), not as a reproduced bug
bug_orig_test(${CMAKE_CURRENT_SOURCE_DIR}/selftest/bug-orig-selftest-broken.bas
	bug_orig_selftest_broken WANT 3)

add_custom_target(bug_orig_sources ALL DEPENDS ${bug_orig_sources} ${testdata_files})
if(bug_orig_deps)
	list(REMOVE_DUPLICATES bug_orig_deps)
	add_dependencies(bug_orig_sources ${bug_orig_deps})
endif()

# The bug-orig tests of TED: ted/ID.drv, a TED A/B script (tests/ted-ab,
# runner/tedab.h; @file, @command and the @expect kinds data, stdout, log,
# screen are made for these tests) whose expectations are the correct
# behaviour.  tedab in its bug-orig mode plays it against the original TED
# (R5: TED 1.1 under blitz2 2.10, the reference) and TED-C (C5), each run in
# an emulator session of its own, and gives the verdict like corpusrun
# (exit status 0 FIXED, 1 REPRODUCED, 3 BROKEN, 4 DIFFERS); report
# ID/report.txt, the runs in ID/sNNN/vol/runs/ID/XX.  ctest bug_orig_ID,
# labels bug-orig and emulator, skipped (77) without Amiberry or the CD.
# The scripts without the emulator: ctest bug_orig_ted_lint (host).
set(ted_scripts ${CMAKE_CURRENT_SOURCE_DIR}/ted)
# the two runs of a test in parallel, unless CORPUS_PARALLEL says 1
if(CORPUS_PARALLEL GREATER 1)
	set(ted_parallel 2)
else()
	set(ted_parallel 1)
endif()
function(bug_orig_ted_test scripts id name)
	cmake_parse_arguments(PARSE_ARGV 3 arg "" "WANT" "")
	set(work ${CMAKE_CURRENT_BINARY_DIR}/${id})
	set(text "# ${name}: generated by tests/bug-orig/CMakeLists.txt from ${scripts}/${id}.drv\n")
	string(APPEND text "work=${work}\nreport=${work}.txt\nscripts=${scripts}\nfilter=${id}\n"
		"gates=${PROJECT_SOURCE_DIR}/tests/ted-ab/scripts\n"
		"src=${PROJECT_SOURCE_DIR}/tests/ted-ab/src\ncombos=R5,C5\nbugorig=1\n"
		"corpus_src=${CMAKE_BINARY_DIR}/tests/corpus/src/tests\n"
		"corpus_out=${CMAKE_BINARY_DIR}/tests/corpus/corpus/ref/vol/out\n"
		"expect_dir=${CMAKE_BINARY_DIR}/tests/ted-ab/expect\n"
		"cd=${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1\n"
		"oracle=${PROJECT_SOURCE_DIR}/tests/data/ted-oracle.sha256\n"
		"fonts=${PROJECT_SOURCE_DIR}/archive/sources/ted-1.1\n"
		"teddrive=$<TARGET_FILE:teddrive>\nteddrivetest=$<TARGET_FILE:teddrivetest>\n"
		"tedprobe=$<TARGET_FILE:tedprobe>\nreqtrap=$<TARGET_FILE:reqtrap>\n"
		"inject=${INJECT}\nwbrun=${WBRUN}\n"
		"ted_ref=${CMAKE_BINARY_DIR}/programs/ted/ref/ted\nted_c=$<TARGET_FILE:ted>\n"
		"blitz2_ref=${CD_BLITZ2}/BLITZ2\nblitz2_c=$<TARGET_FILE:blitz2>\n"
		"timeout=${BUG_ORIG_TIMEOUT}\nparallel=${ted_parallel}\n")
	file(GENERATE OUTPUT ${work}.pass CONTENT "${text}")
	if(DEFINED arg_WANT)
		add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DCORPUSRUN=${TEDAB}
			-DPASS=${work}.pass -DWANT=${arg_WANT} -P ${CMAKE_CURRENT_SOURCE_DIR}/want.cmake)
		set(labels emulator)
	else()
		add_test(NAME ${name} COMMAND ${TEDAB} ${work}.pass)
		set(labels "bug-orig;emulator")
	endif()
	# two sessions (one after the other with CORPUS_PARALLEL=1 or without
	# enough free emulator slots), each maybe repeated once (gates failed),
	# each maybe waiting for a slot
	math(EXPR limit "${BUG_ORIG_TIMEOUT} * 4 + 120 + 4 * ${EMULATION_QUEUE_WAIT}")
	set_tests_properties(${name} PROPERTIES LABELS "${labels}" SKIP_RETURN_CODE 77
		TIMEOUT ${limit})
endfunction()

file(GLOB drvs CONFIGURE_DEPENDS ${ted_scripts}/*.drv)
foreach(drv IN LISTS drvs)
	get_filename_component(id ${drv} NAME_WE)
	string(REPLACE "-" "_" name "bug_orig_${id}")
	bug_orig_ted_test(${ted_scripts} ${id} ${name})
endforeach()
if(drvs)
	# the scripts (syntax, directives) without the emulator
	set(lint ${CMAKE_CURRENT_BINARY_DIR}/ted_lint.pass)
	string(CONCAT text
		"work=${CMAKE_CURRENT_BINARY_DIR}/ted_lint\nreport=${CMAKE_CURRENT_BINARY_DIR}/ted_lint.txt\n"
		"scripts=${ted_scripts}\nsrc=${PROJECT_SOURCE_DIR}/tests/ted-ab/src\n"
		"corpus_src=-\ncorpus_out=-\nexpect_dir=-\ncd=-\noracle=-\nfonts=-\n"
		"teddrive=-\nteddrivetest=-\ntedprobe=-\nreqtrap=-\nted_ref=-\nted_c=-\n"
		"blitz2_ref=-\nblitz2_c=-\n")
	file(GENERATE OUTPUT ${lint} CONTENT "${text}")
	add_test(NAME bug_orig_ted_lint COMMAND ${TEDAB} --lint ${lint})
endif()

# the self test of the TED variant: an expectation equal to what the
# original TED does must be reported as broken (exit status 3)
bug_orig_ted_test(${CMAKE_CURRENT_SOURCE_DIR}/selftest ted-bug-orig-selftest-broken
	bug_orig_ted_selftest_broken WANT 3)
